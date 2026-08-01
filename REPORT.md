# NitoriSTL 施工报告

> **历史档案，不是 Nitori v2 权威资料。** 当前公共实现只认
> `/home/tnuzy/NitoriSTL/v2/Nitori.h`，当前用户文档只认
> `/home/tnuzy/NitoriSTL/NITORI_DOCUMENT.md`。不得从本报告推断 v2 API。

版本：`Nitori.h` 当前工作树（GNU++20）  
定位：个人训练、ICPC 现场、纸质材料；不是 ABI/工程/标准兼容库。

这份报告记录**当前实际代码**，不是理想路线图。所有测试和 benchmark 均以命令实际运行的结果为准；没有运行的项目会明确标成“未测”。

---

## 1. 目标与总原则

Nitori 的压缩目标不是把 STL 名字换成 `n`，而是把竞赛中反复出现的语义压成一套短协议：

```text
标量/哨兵 → 枚举游标 → 存储 → 代数操作包 → 序列/集合/映射
→ 有限数学对象 → 代数数据结构 → 数论/组合/线代/多项式
→ 图/字符串/几何/概率/博弈/优化 → IO → 默认绑定
```

核心不变量：

1. 所有公开全局名字以小写 `n` 开头；实现细节只在短的 `ni` 命名空间或对象内部。
2. Nitori 协议不提供 `begin/end/iterator/allocator`。内部可以使用 STL，但 STL 只在适配层或算法实现边界出现。
3. 部分操作同时给哨兵出口和显式 default 出口；非法前置条件仍然 `nassert`，不拿 default 掩盖损坏的不变量。
4. 默认调用短，复杂泛型只来自真实数学差异；后端替换集中在 `99 ndefault`。
5. 任何可枚举对象优先接入 `nenumerate`/`nfor`，而不是仿造 ranges/iterator 体系。
6. 代码允许压行和宏，但宏只压机械重复；状态、不变量和算法边界仍必须可检查。

---

## 2. 依赖 DAG

### 2.1 章节级 DAG

```text
00 nconfig
   ↓
01 nbase ───────────────────────────────────────────────────────────────┐
   ↓                                                                    │
02 nenum → 03 nmem → 04 nop                                             │
   ↓          ↓        ↓                                                │
10 nseq ──────┴────────┴──────────────────────────────────────────────┐ │
   ↓                                                                  │ │
11 nassoc ───────────────────────────────────────────────────────────┐ │ │
   ↓                                                                 │ │ │
12 nfinite (rel/func/inj/bije/compress/perm/partition)                │ │ │
   ↓                                                                 │ │ │
20 nds (Fenwick/segment/lazy/sparse/queue/DSU)                        │ │ │
   ↓                                                                 │ │ │
30 ninteger → 31 ncomb → 32 nlinear → 33 npoly                     │ │ │
   ↓             ↓          ↓            ↓                           │ │ │
40 ngraph/tree/flow/matching      41 nstring      42 ngeom            │ │ │
   │                 │                 │             │                │ │ │
   └─────────────────┴─────────────────┴─────────────┴→ 43 nprob      │ │ │
                                                        44 ngame       │ │ │
                                                        45 nopt        │ │ │
                                                        50 nio         │ │ │
                                                                  ↓   │ │ │
                                                               99 ndefault
```

更精确的边如下（箭头表示“代码或契约依赖”，不是继承）：

| 模块 | 直接依赖 | 依赖理由 |
|---|---|---|
| `nbase` | `<bits/stdc++.h>`、C++20 concepts/`<bit>` | 标量、比较、宽位、随机、哈希、断言 |
| `nenum` | `nbase` | `nrange`、游标与宏需要 `nassert`/宽位 |
| `nmem` | `nbase`,`nenum` | `nspan`/`npool` 的边界和枚举 |
| `nop` | `nbase` | 半群/幺半群/群操作包与 `npow` |
| `nseq` | `nbase`,`nenum`,`nmem`,`nop` | 线性/高维连续容器、排序、fold、堆、视图 |
| `nassoc` | `nbase`,`nenum`,`nmem`,`nop` | treap/splay/hash 表需要游标、节点池、哈希与增广代数 |
| `nfinite` | `nbase`,`nenum`,`nmem`,`nassoc` | 关系、函数、双射、压缩、置换、划分 |
| `nds` | `nbase`,`nmem`,`nop`,`nfinite` | 维护代数聚合、DSU 输出划分 |
| `ninteger` | `nbase` | 整除、GCD、CRT、模数、筛、Pollard-Rho |
| `ncomb` | `ninteger`（模运算接口） | 阶乘逆元、子集 zeta |
| `nlinear` | `nop`,`ncomb` 的域接口 | 矩阵、Gauss、行列式、逆、XOR basis |
| `npoly` | `nseq`,`ninteger`,`nlinear` | NTT、卷积、FPS、BM/Kitamasa |
| `ngraph` | `nbase`,`nenum`,`nfinite`,`nds`,`nseq` | 图协议、forward-star/CSR/过滤视图、路径、SCC、流、匹配、树 |
| `nstring` | `nbase`,`nseq` | 字符/序列访问与返回 `nvector` |
| `ngeom` | `nbase`,`nseq` | 宽位叉积、点集返回凸包 |
| `nprob` | `nbase`,`nseq` | 权重序列、随机抽样 |
| `ngame` | `nbase`,`nseq`,`ngraph` | Nim 与 DAG Grundy |
| `nopt` | `nbase`,`nseq` | 二分、三分、离散 Li Chao |
| `nio` | `nbase` | 哨兵、宽位 IO |
| `ndefault` | 所有需要默认实现的家族 | 只在此处集中绑定 |

### 2.2 STL/系统依赖边界

`Nitori.h` 目前只依赖 GNU C++20 的单头 `<bits/stdc++.h>`。内部使用了 `vector/deque/array/optional/set/unordered_map/queue/priority_queue/sort/partial_sum` 等标准组件，以及 `chrono/fread/fwrite`。这些是实现材料，不是 Nitori 公共协议。

`nset_stl` 和 `nmap_hash` 的游标现在不把 STL iterator 作为成员公开：set 游标按序号重新定位并返回值，hash-map 游标保存对象指针和整数位置，iterator 只在方法局部出现。`nvector_stl` 等 `_stl` 后端仍保留公开存储成员 `.a` 以压缩竞赛实现；这不是 `begin/end` 协议，纸面代码应优先使用 Nitori 成员函数。

---

## 3. 公共协议与默认绑定

### 3.1 名称、后端、默认出口

| 语义家族 | 实现 | 当前默认 |
|---|---|---|
| vector | `nvector_stl<T>` | `nvector` |
| fixed-rank array | `narray<T,K>` | 单一直接实现 |
| deque | `ndeque_ring<T>` | `ndeque` |
| heap | `nheap_binary<T,C>` | `nheap` |
| set | `nset_fhq<T,C,false,A>`、`nset_splay<T,C,false,A>`、`nset_stl` | `nset_fhq` |
| multiset | `nset_fhq<T,C,true,A>`、`nset_splay<T,C,true,A>` | `nbag` 为 FHQT multiset |
| map | `nmap_flat`、`nmap_hash`/`nmap_stl` | `nmap_flat` |
| relation | `nrel_scan` | `nrel` |
| partial function | `nfunc_hash` | `nfunc` |
| injection/bijection | `nbije_hash` | `ninj`、`nbije` |
| compression | `nbije_rank` | `ncompress` |
| partition | `npart_dense` | `npart` |
| modular integer | `nmod_static<M>`、`nmod_dynamic<Tag>` | `nmod`、`ndmod` |
| gcd/primality/factor | Euclid/binary、trial/Miller、Pollard-Rho | `ngcd`、`nisprime`、`nfactor` 宏 |
| convolution | `nconv_naive`、`nconv_ntt`、`nconv_auto` | `nconv` 宏 |
| SCC | `nscc_tarjan`、`nscc_kosaraju` | `nscc` 宏为 Kosaraju |
| SG | `nsg_dag` | `nsg` 宏 |
| graph/flow/matching/Li Chao | forward-star+CSR/Dinic/Hopcroft-Karp/discrete Li Chao | `ngraph`/`ngraph_csr`/`nflow`/`nbimatch`/`nlichao` |

改变默认实现只应修改 `99 ndefault`；算法正文不应散落后端条件编译。

### 3.2 哨兵与 default

- 位置/编号：`npos=-1`。
- 指针查找：`nullptr`。
- 普通不可达数值：`ninf<T>`/`nninf<T>`，但路径结果的 `reach()` 现在依据父指针 `p[v]!=npos`，不会把合法距离恰好等于 `bad` 误判为不可达。
- 任意值域无法借数值哨兵表达时：`nmaybe<T>`。
- 常见部分操作同时提供 `f(x)` 和 `f(x,default)`，例如 `get/kth/lower/upper/pop/front/back/dist/at`。
- 非法前置条件（越界、零模数、非 DAG 求 Grundy、非群调用逆元等）使用 `nassert`，不静默吞掉。

### 3.3 无 iterator 枚举协议

最小游标为：

```cpp
auto e=nenumerate(a);
while(e.ok()){ use(e.val()); e.next(); }
```

可选 `idx()`、`key()`。`nfor/nfori/nforkv` 通过 `__COUNTER__` 隔离临时变量，循环表达式只初始化一次；`break/continue` 仍作用于用户循环。容器临时对象若游标保存外部指针则删除 `enumerate()&&`，拥有值的 `nvector`/`nrange` 才允许安全临时枚举。

---

## 4. 模块与实现原理

以下复杂度均以元素数 `n`、边数 `m`、树高/状态数等常用变量表示；哈希后端的 `O(1)` 是平均/摊还，除非另行注明。

### 4.1 `nbase`

- `nmaybe<T>`：`optional<T>` 外壳，`ok/val/default` 三路出口；存储 `O(sizeof(T))`，访问 `O(1)`。
- `ninf/nninf/npos`：按类型选择无穷或安全数值哨兵。
- `nmag/nabs`：对有符号极小值先转无符号，避免 `-LLONG_MIN` 溢出；结果是无符号幅值。
- `nwide_t`：几何/整除中把 64 位整数提升到 `__int128`/`__uint128`。
- `nbitceil`：统一保护 `bit_ceil` 的 `int` 容量边界。
- `nrng`：SplitMix64 风格可复制 PRNG；区间采样用无符号跨度，避免 `r-l` 在有符号域溢出。`nhash` 在标准 hash 上叠加随机盐，降低固定卡哈希数据风险。

### 4.2 `nenum`、`nmem`、`nop`

- `nrange`：半开区间 `[l,r)`，支持正/负步长；长度按宽位整除计算，递增时检测步长推进溢出。`nreverse(nrange)` 采用值拥有 view，避免临时范围悬空。
- `nzip`：两个游标并行，长度取较短者；`nproduct` 保存第二游标初值，字典序笛卡尔积。
- `nspan`：裸指针+长度，不拥有内存；`sub` 为 `O(1)` 视图。
- `npool_dynamic`：整数句柄从 1 开始，空闲栈复用；当前用 `optional<T>` 真正释放 `del` 对象并防止重复释放，`make/del/get` 为均摊 `O(1)`。
- `nnode<S>`：只读、非拥有、带 epoch 的节点拓扑视图；统一 `val/count/len/info/left/right`，空儿子仍是带 owner 的当前视图。树修改后旧视图通过 `current/ok` 明确失效，整数 `handle` 不承诺跨修改稳定。
- `nadd/nmul/nxor/nmin/nmax`：极小静态操作包；`nmonoid/ngroup` 只检查必要接口，`ncommutative_monoid` 还要求操作包以常量 `commutative=true` 声明交换律，不建立复杂类型层次。`npow` 二进制幂 `O(log |e|)`。
- `naugment<A,T>`：可状态化的有序幺半群策略，接口为 `id/one(value,count)/op(left,right)`；`nempty_augment` 让默认集合不承担额外有效 metadata。

### 4.3 `nseq`

- `nvector_stl`：连续存储，push 均摊 `O(1)`，`del` `O(n)`，`swapdel` `O(1)`；`.get(i,default)` 避免手写边界分支。
- `narray<T,K>`：编译期固定维数、运行期维长的有主连续数组。坐标按最后一维最快的混合进制映射到一维 `nvector_stl`，二维映射与 `nmat` 的 `i*w+j` 一致；`operator()` 按坐标访问，`operator[]` 按展平位置访问。坐标访问为 `O(K)`，平坦访问为 `O(1)`，总元素数受统一 `int/INT_MAX` 容量约束。
- `ndeque_ring`：容量保持 2 的幂，环下标用 `& (cap-1)`；两端 push/pop 均摊 `O(1)`，扩容搬移 `O(n)`。
- `nreverse`：lvalue 序列是反向索引游标；rvalue 普通对象被删除，范围对象用值 view。
- `nsort/nlower/nupper/nfind_sorted/nunique/nfold`：直接针对 `len/[]/data` 协议实现，避免 STL iterator 适配层。
- `nheap_binary`：数组二叉堆，`up/down` 维护父子优先关系；push/pop `O(log n)`，建堆 `O(n)`。push 返回上滤后的实际元素引用，而非插入前的 `back()`。

### 4.4 `nassoc`

#### 原生有序树公共力学

FHQ 与 Splay 现在共享同一节点和增广契约。节点同时维护集合本体的 `sz/count` 与可选的 `info`，聚合顺序严格为：

```text
info(u)=op(op(info(left),one(value,count)),info(right))
```

因此字符串拼接等非交换 monoid 也能跨旋转保持正确。策略是对象而非静态回调，可保存只读运行时参数；默认
`nempty_augment` 使用 `[[no_unique_address]]`，不把 cardinality 再塞进每个用户策略。

`root()` 返回 `nnode`，`walk` 接受 `left/take/right` 决策并沿一条根叶路径下降；`first_prefix` 与
`last_suffix` 对单调聚合谓词做左右对称搜索。三者均为 `O(h)` 个节点访问，前后缀搜索至多做常数次
metadata 合并/层，不枚举整棵树。`nset_stl` 没有真实子树聚合，因此不满足 `nnode_tree`，不伪造线性版本。

节点视图捕获树 epoch。FHQ 的成功修改、Splay 的旋转或成功修改、容器赋值/移动都会使旧视图失效；只读视图
不能直接写 key、儿子或 metadata。视图 owner 必须仍存活。

#### Treap set/multiset (`nset_fhq`)

节点保存 key、随机优先级、左右子树、子树总大小和重复计数。`split` 按 key 分裂，`merge` 按优先级合并；BST 顺序与堆序共同保证结构正确。`rank/kth/lower/upper` 沿子树大小走。

- 平均插入/删除/查找/排名：`O(log n)`；期望空间 `O(n)`。
- `| & - ^` 通过公共枚举和查找组合，集合运算最坏可达 `O((n+m)log(n+m))`。
- multiset 只在同 key 节点增加 `c`，因此重复元素不制造额外树节点。

#### Splay set (`nset_splay`)

父指针旋转，zig/zig-zig/zig-zag 后把访问节点伸展到根；势能法给出摊还 `O(log n)`，无需随机数。适合局部性强的操作，最坏单次可 `O(n)`。

#### STL set adapter (`nset_stl`)

内部 `std::set`，普通查找 `O(log n)`；由于拒绝 iterator/order-statistics 扩展，`rank/kth` 用 `distance/advance`，为 `O(n)`。它是正确性/接口参考后端，不是默认的排名性能后端。

#### Hash map (`nmap_flat`/`nmap_hash`)

`nmap_flat` 是扁平节点数组+开放寻址线性探测：桶值 `0` 为空、负值 tombstone、正值为节点编号；负载超过约 0.7 或墓碑过多时重哈希。节点移动时修复其桶指针，因此删除不需要链表。平均查找/插入/删除 `O(1)`，重哈希 `O(n)`，最坏 `O(n)`。容量用受保护的 2 次幂，避免哈希截断和 `bit_ceil` 溢出。

`nmap_hash`/`nmap_stl` 适配 `unordered_map`，接口与扁平后端一致。

### 4.5 `nfinite`

- `nrel_scan<A,B>`：边向量去重，`has/add/del` 线性扫描 `O(m)`；`image/preimage` `O(m)`；集合 operator 按关系语义合并。
- `nfunc_hash<A,B>`：部分函数只维护左键→右值；`bind` 拒绝同左键不同值，`set` 覆盖，`unbind` 删除。平均 `O(1)`。
- `nbije_hash<A,B>`：两个哈希映射 `f:A→B`、`g:B→A` 同步更新。`bind` 只有两端都空或已绑定到同一对才成功；因此保持“左至多一、右至多一、数量相等”。正反查询互逆，`inverse/~` 交换两张表，`operator*` 按 `(f*g)(x)=f(g(x))` 组合。
- `ninj` 当前复用同一双向唯一存储：允许右侧存在未使用元素，但已绑定部分仍是单射。
- `nbije_rank/ncompress`：排序、去重后编号 `[0,k)`，`to` 二分 `O(log k)`，`from` `O(1)`；压缩总成本 `O(n log n)`。
- `nperm`：数组和逆数组双存储，构造时验证双射；乘法先应用右侧再左侧，`O(n)`；幂为二进制快速幂 `O(n log |k|)`；符号由环分解 `O(n)`。
- `npart_dense`：每个位置保存整数类号，`groups` 按类号聚集 `O(n)`；`ndsu.partition()` 将动态并查集结果转成数学划分。

### 4.6 `nds`

- `nfenwick<T,O>`：树状数组，每个节点保存一个低位块的 `O::op` 聚合；任意点更新要求 `O` 是交换幺半群，并以 `O::commutative=true` 显式声明，更新/前缀 `O(log n)`。区间差和 `get` 还需要群逆元；`lower` 还需要前缀对 `<` 单调。
- `nseg_iter<T,O>`：完整 2 的幂叶数组，父节点为 `O::op(left,right)`；点更新和半开区间 fold `O(log n)`，`maxr/minl` 用树上跳跃，要求谓词对聚合满足单调性且 `f(id)=true`。
- `nlazyseg<S,F,M,A>`：节点信息 `S`、懒标签 `F`、合并幺半群 `M`、作用包 `A`；`put` 先作用到节点再合并标签，区间更新/查询 `O(log n)`。`nlazy_addsum` 是“区间加、区间和”实例。
- `nsparse`：预计算长度 `2^k` 的块，查询用两个重叠块；要求幂等/重叠安全运算，预处理 `O(n log n)`，查询 `O(1)`。
- `nqueue_agg`：双栈队列，每栈保存从栈底到当前位置的聚合；搬移一次后每元素只进出一次，push/pop/front/fold 均摊 `O(1)`。
- `ndsu`：按大小合并+路径压缩，反 Ackermann 摊还；`ndsu_rollback` 不压缩路径，以变更栈支持 `rollback`，单次 `O(log n)`（按大小高度）；`npotential_dsu` 在群上维护根势能，`diff(a,b)=w[b]-w[a]`。

### 4.7 `ninteger`、`ncomb`

- `nfloor_div/nceil_div`：先以 `__int128` 计算商余数，再按数学定义修正负余数。
- `ngcd_euclid/ngcd_binary`：前者余数迭代，后者移位减法；都对 `LLONG_MIN` 使用无符号幅值。默认 `ngcd_binary`。
- `nextgcd`：扩展 Euclid，系数和 gcd 使用 `__int128`，可处理 `LLONG_MIN` 极值。
- `ncrt`：先求 `g=gcd(m1,m2)`，检查 `(a2-a1)%g`，再求最小公共模；不相容或 lcm 超过 `long long` 返回空 `nmaybe`。
- `nfrac`：每次运算先约分、规范分母为正；加乘使用交叉 gcd 降低中间量。存储仍是 `T`，因此表示结果必须落在 `T` 范围内。
- `nmod_static/dynamic`：规范到 `[0,M)`；乘法用 `__uint128`，逆元由扩展 Euclid，非单位元 `tryinv` 返回空。
- Miller-Rabin 使用固定 64 位确定性 bases；Pollard-Rho 用随机多项式和 gcd 分裂，因子排序后返回。Pollard 是随机/启发式时间界。
- `nprime_table`：Euler 线性筛同时生成最小质因子、`phi`、`mu`；预处理 `O(n)`，分解按质因子次数，枚举因子 `O(τ(x) log τ(x))`。
- `ncomb`：阶乘和逆阶乘，`C/P/H` 查询 `O(1)`；前提是底层 `T` 为可除的域/模域。
- 子集/超集 zeta：每个 bit 做一次 butterfly，`O(2^k k)`；空向量现在是合法 no-op。

### 4.8 `nlinear`、`npoly`

- `nmat`：行主序稠密矩阵，乘法三重循环 `O(hwk)`，零项剪枝依赖 `v==Add::id()`；方阵幂 `O(n^3 log e)`。
- `ngauss`：逐列找主元、归一化并消去所有其他行，返回一致性、秩、一个解和零空间基；域上 `O(m n min(m,n))`。
- `ndet/ninverse`：高斯消元，`O(n^3)`；无逆返回空。
- `nxorbasis`：按最高位维护线性基，插入/判定 `O(B)`，最大异或贪心 `O(B)`。
- `nberlekamp`：Berlekamp-Massey 维护最短线性递推，当前实现 `O(NL)`；`nrec_nth` 用 Kitamasa 多项式折半，`O(L^2 log k)`。
- `nntt`：位逆置换+分层蝶形，根由 `nntt_info` 特化；长度必须是模数 `M-1` 的因子。单次 `O(n log n)`，卷积两次变换加点乘。
- `nconv_naive`：`O(|a||b|)`；`nconv_auto` 在乘积不大时选朴素，否则在已知 NTT 模数时选 NTT。
- `npoly`：规范化去高位零；加减线性，乘法走 `nconv_auto`；`inv/log/exp` 使用 Newton doubling，理论上依赖卷积复杂度 `M(n)`，当前小规模自动落到朴素卷积。

### 4.9 `ngraph`

图的只读公共协议是 `len()` 与可枚举的 `g[u]`；每条 `nedge` 统一公开 `from/to/id/w`。`vertices()` 与
`arcs()` 分别枚举所有点和所有有向边，`ngraph_where` 在不复制图的前提下产生过滤视图。所有普通图算法只依赖
这条协议，不再读取某个后端的 `h/to/nx`；因此用户图、过滤视图和 CSR 可以共享同一套算法。

`ngraph_forward` 是 forward-star：`h[u]` 指向边链，边数组保存 `to/w/next`，边号就是数组位置；加边 `O(1)`，
遍历一个点的出边与度成正比。`add2` 只是一对反向边的便捷构造。可变枚举中的 `e.w` 是引用，此外
`find/has/weight/set` 提供带 `npos/nullptr/default` 的受控边查询与权值修改。

枚举顺序固定为：`vertices()` 按 `0..n-1`；`arcs()` 按起点递增，再按该后端的邻接顺序。forward-star 的邻接
顺序是同一起点的逆插入序；CSR 构造时保持来源图当时的邻接顺序，并依 `arcs()` 顺序重新编号。`find(u,v)`
返回邻接顺序中的第一条平行边，`O(outdegree(u))`；forward-star 的 `degree` 同界，CSR 的 `degree` 为 `O(1)`。
非法顶点是断言失败；非法边号在 `weight/set` 中分别返回 `nullptr/false`。增删拓扑会使活动游标和边权引用失效，
不得在一次枚举中进行拓扑修改。

`ngraph_csr` 是静态拓扑后端：从任意满足协议的图构造，`O(V+E)`，邻接连续且省去 `next`；转换后边号按 CSR
顺序重新分配。拓扑不可增删，但权值仍可修改。`ngraph_where` 是非拥有视图，源图和谓词状态必须长寿于视图，
且一次算法执行期间谓词结果必须稳定。

- `nbfs`：FIFO 松弛无权边，`O(V+E)`。
- `n01bfs`：0 边入 deque 前端，1 边入后端，`O(V+E)`；断言投影后的权值只能是 0/1。
- `ndijkstra`：非负权+优先队列，`O((V+E)log V)`；`ncapadd` 以宽位加法封顶到 `inf`。结构化边数据可通过
  权值投影参与 `n01bfs/ndijkstra/nkruskal/nlca_binary`，无需把整个 payload 伪装成数值。
- `ntopo`：入度队列，DAG 为 `O(V+E)`；有环返回空 `nmaybe`。
- `nscc_tarjan`：一次 DFS 的 low-link 栈，`O(V+E)`；`nscc_kosaraju` 使用显式第一遍栈和反图，避免第二遍递归。
- `nkruskal`：按边权排序+DSU，`O(E log E)`；输入应按无向边语义提供（`add2` 会产生重复反向边但 DSU 会跳过）。
- `nlca_binary`：森林 BFS 建父、深度、距离，再建倍增表；预处理 `O(V log V)`，LCA/jump/kth `O(log V)`，距离 `O(log V)`。
- `nhld`：非递归 DFS 求重儿子和重链编号；分解 `O(V)`，每条路径被切成 `O(log V)` 段，具体聚合由用户在段上接 `nseg` 等结构。
- `nflow_dinic`：残量网络、BFS 分层、当前弧 DFS；一般图上经典上界 `O(V^2E)`，竞赛常见网络通常更快；`reset` 恢复初始容量。
- `nbimatch_hopcroft`：分层增广路，`O(E√V)`；`mincover` 按 Konig 定理从未匹配左点交替可达集构造最小点覆盖。

### 4.10 `nstring`

- `nzfunc`：Z-box `[l,r)` 复用，`O(n)`。
- `nprefix/nkmp`：失配跳 failure，预处理/匹配 `O(n+m)`；空模式返回所有边界位置。
- `nmanacher`：奇/偶中心半径及 `pal(l,r)`，总 `O(n)`。
- `nsuffix_array`：初始比较排序，之后 doubling + counting sort，`O(n log n)`；`nlcp_array` 用 Kasai，`O(n)`。
- `nac<K,Base>`：稠密转移表 BFS 建 fail，节点转移内存 `O(K·nodes)`；匹配 `O(text+输出数)`，计数先按 fail 树逆拓扑累加。

### 4.11 `ngeom`

点运算使用 `nwide_t` 做点积/叉积；浮点方向判断用 `nsgn_eps`。

- 线交：行列式判平行，`O(1)`，平行返回空。
- 线段相交：方向符号+共线包围盒，`O(1)`。
- 凸包：排序去重后 Andrew 单调链，`O(n log n)`；`keep=true` 保留边界共线点。
- 点在多边形：边界返回 0，内部 1，外部 -1，射线法 `O(n)`。
- 凸包直径：旋转卡壳，`O(h)`，输入应为逆时针凸包。

### 4.12 `nprob`、`ngame`、`nopt`、`nio`

- `nprob`：权重向量；`sum/expect/normalize` `O(n)`，浮点 `draw` 线性扫描。负权或非正总和不被视为概率，规范化/抽样返回空/default。
- `nnim`：维护堆 xor；非零 xor 必胜，扫描找到 `h[i]^x<h[i]` 的必胜移动，`O(k)`。
- `nsg_dag`：先拓扑排序，逆拓扑按后继 Grundy 标记求 mex，`O(V+E)`；有环返回空哨兵。
- `nfirst_true/nlast_true`：单调判定二分 `O(log range)`；`nternary_min` 固定迭代次数。
- `nlichao_static`：离散 x 坐标线段树，每节点保留当前更优直线并递归到交叉侧；插入/查询 `O(log X)`，支持全域和半开下标区间添加。
- `ninput/noutput`：64 KiB 缓冲 `fread/fwrite`；整数（含 `__int128`）手写转换，读写总成本线性于字节数。

---

## 5. 正确性契约与失败边界

### 5.1 必须由用户保证的数学前提

| 组件 | 前提 |
|---|---|
| `nset_*` | comparator 是严格弱序；multiset 计数非负；增广 `op` 满足结合律、`id` 为单位元，且影响结果的策略状态在树非空期间稳定 |
| `first_prefix/last_suffix` | `predicate(id())==false`，且谓词分别随前缀扩展/后缀向左扩展单调 |
| `nmap_*` | hash/equality 一致；自定义 hash 不制造不可接受的攻击退化 |
| `nfenwick` | `O` 是以 `commutative=true` 声明的交换幺半群；区间差/get 还要求群；`lower` 还要求前缀严格单调 |
| `nseg_iter` | `O` 是幺半群；`maxr/minl` 的 predicate 对聚合单调 |
| `nsparse` | 运算幂等或重叠合并安全 |
| `nlazyseg` | `A` 的 compose/apply 满足作用律，`M` 与作用兼容 |
| `npotential_dsu` | 势能运算所在类型可加、可取逆（通常是群） |
| `nmod/ncomb/ngauss/FPS` | 除法对象必须是单位元；模数/域条件满足 |
| `nntt` | 模数是已登记 NTT 模数，长度整除 `M-1` |
| `Dijkstra` | 边权非负且距离不超过所选 `inf` 表示范围 |
| `n01bfs` | 边权恰为 0/1 |
| `nkruskal` | 输入边按无向图解释 |
| `nconvex_diameter2` | 输入是逆时针凸包（通常先 `nconvex_hull`） |
| `nternary_min` | 目标在区间上单峰/凸；离散整数需自行离散化 |

### 5.2 明确的退化风险

1. Tarjan、Dinic DFS、Hopcroft-Karp DFS 仍是递归；极深 adversarial 图可能爆栈，Kosaraju 是无递归替代。
2. `nset_fhq` 是期望复杂度；随机源被人为固定或攻击时不提供确定最坏界。`nset_splay` 单次最坏 `O(n)`。
3. `nset_stl::rank/kth` 为 `O(n)`，只作为参考后端；需要 order-statistics 时不要把它绑定为默认。
4. `nnode` 是非拥有拓扑快照，不是稳定节点身份；树修改、Splay 旋转、赋值或移动后必须重新取视图，owner 销毁后任何访问都非法。
5. `nmap_flat` 的最坏探测是 `O(n)`；随机盐只降低固定哈希攻击，用户自定义 `H` 仍可能故意退化。桶容量用 `int`，当前保护上限约 `2^30`。
6. `nfrac<T>` 的归约结果仍存回 `T`；若中间或最终分子/分母超范围，debug 会断言，NDEBUG 下只能得到受实现定义转换保护的空/回退值，不能把它当任意精度有理数。
7. 几何整数坐标的点运算虽提升到 128 位，但若坐标乘积超过 128 位仍会溢出；浮点几何的 `eps` 必须由题目尺度选择。
8. `nmod_dynamic<Tag>` 的模数是 tag 级全局状态；切换模数后旧对象不应和新模数对象混算。全局 RNG/hash 盐、`nin/nout` 也不是线程安全设施。
9. `nprime_table`、矩阵、线段树、图等构造参数需要非负且在内存/`int` 容量范围内；debug 会断言，release 对非法输入只提供保守空构造，不能替代正确的约束检查。
10. `nsg_dag` 对含环图返回空 `nmaybe`；这不是 Grundy 值 0。
11. `nprob` 只把非负、正总和对象当概率；浮点 NaN/负权会走空/default。

---

## 6. 宏与代码熵审查

- `.clang-format` 固定 LLVM 基线、4 空格、120 列，并把 `nrep/nfor/nfori/nforkv` 识别为 foreach 语法宏；当前 `Nitori.h` 由机器上的 clang-format 21 格式化且 dry-run 幂等。
- `nrep/nrrep`：范围表达式只放入一次临时变量；索引是 `int`，适合竞赛规模。
- `nfor/nfori/nforkv`：`nenumerate` 只初始化一次，`__COUNTER__` 隔离嵌套变量；`if(false){}else` 结构让用户名字只在循环体可见。
- `ncat` 只用于生成局部唯一名；内部生成宏没有泄漏新的大写宏。
- `ni_mod_ops` 是唯一明显的成组 operator 生成宏，使用后立即 `#undef`。
- 默认函数宏集中在 `ndefault`：`ngcd/nisprime/nfactor/nconv/nscc/nsg`；类型家族使用 `using`，因此模板诊断较稳定。
- 宏不隐藏状态转移，不重复求值用户表达式；边界（嵌套、break、continue、单次求值、临时对象）由 `test/base.cpp` 与 `test/audit.cpp` 覆盖。

---

## 7. 实际验证记录

### 7.1 测试工件

```text
test/base.cpp    基础标量、range、宏、pool、极值
test/seq.cpp     vector/fixed-rank array/deque/heap/排序/视图
test/finite.cpp  set/map/rel/func/bije/compress/perm
test/extensibility.cpp  nnode/增广/树下降的双后端、非交换、随机与复杂度契约
test/ds.cpp      Fenwick/segment/lazy/sparse/queue/DSU
test/math.cpp    CRT/mod/筛/组合/矩阵/NTT/FPS/递推
test/graph.cpp   BFS/Dijkstra/topo/SCC/flow/SG
test/tree.cpp    LCA/HLD/matching/0-1 BFS/MST
test/string.cpp  Z/KMP/Manacher/SA/LCP/AC
test/geom.cpp    点、交、凸包、点内、直径
test/opt.cpp     二分、三分、Li Chao、prob/Nim
test/io.cpp      缓冲输入输出及宽位极值
test/audit.cpp   pool 析构、路径哨兵冲突、负概率、zeta 空集、边界对拍
test/tutorial.cpp 教程公开调用的编译与行为烟测
```

### 7.2 已实际运行的命令

最终代码状态上执行的验证流程为：

```bash
for f in test/*.cpp; do
  g++ -std=gnu++20 -O2 -Wall -Wextra "$f" -o build/test-$(basename "$f" .cpp)
  build/test-$(basename "$f" .cpp)
done
```

```bash
for f in test/*.cpp; do
  g++ -std=gnu++20 -O2 -DNDEBUG "$f" -o build/test-$(basename "$f" .cpp)-ndebug
  build/test-$(basename "$f" .cpp)-ndebug
done
```

```bash
for f in test/*.cpp; do
  g++ -std=gnu++20 -O1 -g -fsanitize=address,undefined \
      -fno-omit-frame-pointer "$f" -o build/test-$(basename "$f" .cpp)-san
  ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
      build/test-$(basename "$f" .cpp)-san
done
```

本次节点/增广硬重构后，共 14 个测试工件重新执行的结果：

```text
O2 + warnings       全部通过
O2 + DNDEBUG        全部通过
ASan + UBSan        全部通过
头文件独立 -fsyntax-only 通过
clang-format 21 --dry-run --Werror 通过
```

测试源码本身存在几条非库错误告警（随机数到 int、测试 `fread` 返回值、测试行的 misleading indentation）。库头在独立 `-Wall -Wextra -Wconversion` 语法检查下无告警；额外打开 `-Wshadow` 会报告竞赛式构造函数/局部变量同名，额外打开 `-Wpedantic` 会报告 GNU `__int128`，均是已知的压缩风格/平台扩展诊断，不是构建错误。

### 7.3 真实 benchmark

工件：`bench/bench.cpp`；固定 xorshift 种子和 `nseed(1)`，所有结果是实际运行的微秒（`steady_clock`），末列为防优化校验和。一次 O2 运行如下：

| 项目 | 负载 | 时间（µs） | 校验和 |
|---|---:|---:|---:|
| `map_flat` | 20,000 插入+查找+删除 | 1,092 | 200003333 |
| `map_stl` | 同上 | 2,710 | 200003333 |
| `set_fhq` | 20,000 插入+rank+删除 | 16,519 | 200003333 |
| `set_splay` | 同上 | 22,372 | 200003333 |
| `set_stl` | 同上（rank 为线性 distance） | 1,711,822 | 200003333 |
| `fenwick` | 50,000 点加+前缀 | 1,297 | 624066189 |
| `segment` | 50,000 点设+前缀 fold | 3,551 | 625872190 |
| `conv_naive_1024` | 1024×1024 | 2,476 | 6405748 |
| `conv_ntt_8192` | 8192×8192 | 2,572 | 4229362 |
| `dijkstra` | V=20,000, E=50,000 | 2,260 | 7225598 |
| `dinic` | V=120, E=1,500 | 63 | 288 |

这是单机一次 O2 运行；调度和缓存会造成明显波动，不能替代渐进复杂度或最坏界证明。

环境：

```text
CPU: 12th Gen Intel(R) Core(TM) i5-1240P, 16 logical CPUs
OS:  Linux 6.17.0-35-generic
Compiler: g++ 14.2.0 (Ubuntu 14.2.0-4ubuntu2~24.04.1)
Flags: -std=gnu++20 -O2 -pipe
Runtime RSS: 约 5.2 MiB（/usr/bin/time / getrusage）
```

节点/增广重构后单独编译 benchmark（包含完整头文件）实际测得：`2.54 s` wall、`2.41 s` CPU、编译峰值 RSS `364228 KiB`；该 benchmark 运行测得 `1.76 s` wall、`1.76 s` CPU、峰值 RSS `5216 KiB`（程序内部 `getrusage` 为 `5156 KiB`）。这说明默认空增广没有制造可见的数量级回退，但单次微基准不能证明“零开销”；完整单头模板实例化仍是主要编译成本，纸质材料不受此成本影响。

---

## 8. 扩展与替换规则

新增组件必须按以下顺序施工：

1. 先写对象语义、operator、哨兵/default、非法前提和复杂度。
2. 先做最短参考后端或暴力模型，作为契约测试 oracle。
3. 再做原生后端，并明确它降低的是代码量、常数、最坏界、内存还是增加持久化/撤销等能力。
4. 所有后端跑同一契约测试、随机对拍、边界和 sanitizer。
5. 最后只在 `ndefault` 绑定；报告补依赖边和退化风险。

新增原生有序树后端还必须满足 `nnode_tree`；若维护增广，则复用 `naugment`、`nwalk`、
`nfirst_prefix/nlast_suffix`，并运行 `test/extensibility.cpp` 的同协议测试。不能为新树再发明一套节点字段名、
`make(left,x,right)` 回调或线性“树上二分”。没有真实能力的参考后端应让 concept 检测为 false。

推荐的后续自然数学扩展（不改变现有接口）：

- `neqrel<T>`：以 `ndsu`/`npart` 为维护结构的有限等价关系对象；
- `norder/nposet`：有限偏序的 DAG 表示、传递闭包和拓扑枚举；
- `nsurj`：在 `ninj/nbije` 两侧补齐满射契约；
- 可撤销/持久化 map、rope、动态树，以及更快的 BM/FPS 后端。

这些扩展应复用已有 `nrel/nfunc/nbije/npart/nop/nenum`，不要再次引入互不兼容的“标签→编号”临时代码。

---

## 9. 复现与提交前门禁

```bash
# 头文件最小语法
printf '#include"Nitori.h"\nint main(){}\n' |
  g++ -std=gnu++20 -O2 -Wall -Wextra -Wconversion -Wshadow -Wpedantic \
      -x c++ -I. -fsyntax-only -

# 全量测试
for mode in O2 NDEBUG SAN; do ...; done

# benchmark
g++ -std=gnu++20 -O2 -pipe bench/bench.cpp -o /tmp/nitori-bench
/tmp/nitori-bench
```

提交前必须再次确认：

```text
[ ] 新公开名字以 n 开头
[ ] 没有 begin/end/iterator/allocator 协议
[ ] 默认调用和 default 出口仍短
[ ] 复杂度与代码循环一致
[ ] 空对象、极值、临时对象、宏单次求值有测试
[ ] 所有后端契约一致
[ ] O2/NDEBUG/ASan/UBSan 全通过
[ ] benchmark 与报告环境、种子、负载、结果同步
[ ] 任何未验证结论都明确标注为风险或前提
```

---

## 10. 当前结论

当前 `Nitori.h` 已形成一条从底层标量、枚举、内存和代数操作，到容器、有限数学对象、代数数据结构、数论/组合/线代/多项式，再到图、字符串、几何、概率、博弈、优化和 IO 的单头闭环。默认后端偏向短代码、低常数和可纸抄；`_stl` 后端保留作为正确性/接口参照，而不是把 STL 的 iterator 历史协议重新暴露。

第一块结构硬化已经把原先封闭在 FHQ/Splay 内部的节点、metadata 与下降能力提炼为公共契约：用户可以在不接触裸儿子和旋转细节的情况下写一次增广与树上二分，并在两个原生后端间替换。epoch 又把“视图何时失效”从默契变成可测试条件。这不是新增题型能力，而是消除容器层最重的一处 ad hoc 力学。

最重要的剩余边界不是“再堆更多模板”，而是：

```text
继续用契约测试保护每个 operator；
让后续树结构复用 nnode/naugment，而不是继续复制局部节点协议；
为递归和数值范围写清失效边界；
在真实竞赛负载上选择默认后端；
让 neqrel/poset 等自然数学对象复用已有有限语义；
```

这保持了 Nitori 的目标：**先量约束，再装配机关；AC 只是结果，能把机关迁移到下一题才算真正理解。**
