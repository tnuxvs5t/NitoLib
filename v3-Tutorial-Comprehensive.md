# Nitori v3 Tutorial Comprehensive

> 状态：从零重建中。本文只描述 `src-v3/` 当前已经存在并经过独立测试的能力。
> V2/X 及更早代码已离开活动工作树；V3 暂时没有统一头文件。

## 0. V3 到底在改革什么

Nitori v3 不是把 V2 换一套名字，也不是把所有类型塞进统一资源池。它有两个目标：

1. **结构化复用改革**：复用真正的算法内核，例如位置投影、离散函数、根代数、
   邻接端口和代数操作；不复用重复的包装层。
2. **自由度革命**：模板只要求实现真正用到的表达式，数学限制、生命周期和失效规则
   写在实现旁的契约注释里，不建立 trait/concept/npre 森林。

自由不是“没有前提”。V3 的区分是：

```text
编译期只检查实际被实例化的表达式
数学定律与生命周期由局部注释说明
测试用独立暴力和 sanitizer 攻击契约内行为
违反前提时不承诺诊断、修复或稳定结果
```

当前源码预算为 **128 KiB 语义代码**。注释和排版空白不计入预算，因此应当用注释解释
承重契约，而不是用复杂类型系统把用户锁进唯一组合方式。

## 1. 使用方式与头文件关系

V3 使用 C++23，直接包含需要的模块：

```cpp
#include "src-v3/view.hpp"
#include "src-v3/segment.hpp"
```

模块可以正常依赖其他模块，用户不需要手工补齐传递依赖。当前依赖骨架是：

```text
core
├── view
│   ├── func ─┬─ discrete
│   │          └─ graph ── graph_algo / graph_store / tree
│   ├── ds ── flow
│   └── string / automata / wavelet / linear / geom
├── arena
│   ├── fhq ── dynamic_tree
│   ├── segment ── dynamic_tree / link_cut
│   └── opt
├── math ── poly
└── number
```

不存在 `Nitori-v3.h`，也不要求形成它。这样每个模块可以先独立成熟，避免为了 amalgamate
顺序产生虚假的底层抽象。

## 2. 全局约定

- 长度、位置和稠密顶点编号使用 `int`。
- 区间统一为半开区间 `[left,right)`。
- `len()` 是 V3 有限对象的长度接口；`nlen(x)` 也能读取普通容器的 `size()`。
- `nchmin(target,candidate)` / `nchmax(target,candidate)` 在严格序更优时原地赋值，并返回是否发生更新。
- V3 默认不做边界检查、溢出检查和契约恢复。
- view、func、graph descriptor 都可能只是借用；owner 移动、销毁或结构修改后，旧投影可能失效。
- 操作对象通常用 `id()` 给单位元，用 `operator()(left,right)` 合并。
- 结合、交换、幂等、可逆、单调等要求由具体算法的注释决定，不由统一 concept 宣称。
- `[[no_unique_address]]` 让无状态策略通常不占额外空间。

一个最小加法幺半群是：

```cpp
struct sum {
    long long id() const { return 0; }
    long long operator()(long long a, long long b) const { return a + b; }
};
```

是否允许非交换操作，要看具体结构。`nseg` 保序，`nett_forest` 因为会旋转 Euler 环而要求
交换；这类差异不会被一个万能 algebra trait 抹平。

## 3. `nview`：只表达有限位置投影

头文件：`src-v3/view.hpp`

```cpp
template<class Access>
struct nview {
    int length;
    Access access;
    int len() const;
    decltype(auto) operator[](int position) const;
};
```

`nview` 拥有 accessor，但通常不拥有 accessor 引用的数据。const 是浅 const：如果 accessor
返回 `T&`，那么 const view 仍然能修改底层对象。

### 3.1 创建和切片

```cpp
vector<int> a{10, 20, 30, 40};
auto all = nall(a);             // 只接受 lvalue，借用 a
auto ids = nrange(2, 6);        // 2,3,4,5
auto sq = ntabulate(5, [](int i) { return i * i; });
auto mid = nsub(all, 1, 3);     // a[1],a[2]
auto rev = nreverse(all);
```

`nall(vector<int>{...})` 故意不能编译，因为临时 owner 会立刻销毁。`nsub` 不复制元素，
也不检查区间。`ntabulate` 每次访问都会重新调用函数。

### 3.2 projection 与 materialization

```cpp
struct item { int key, value; };
vector<item> a{{1, 7}, {2, 9}};

auto values = nproject(nall(a), [](item& x) -> int& { return x.value; });
values[0] = 100;                       // 修改 a[0].value

auto doubled = nmap(values, [](int x) { return x * 2; });
int x = doubled[0];                    // 本次访问产生一个值
```

`nproject` 保留 callable 的返回类别；`nmap` 把返回结果物化为值。两者都仍是惰性访问，
`nmap` 不是缓存容器。

### 3.3 组合器

```cpp
auto picked = ngather(nall(a), ntabulate(2, [](int i) { return 1 - i; }));
auto zipped = nzip(nrange(3), ntabulate(3, [](int i) { return i * i; }));
auto cells = nproduct(nrange(2), nrange(3));
```

- `ngather(values,positions)` 按位置重排或重复访问。
- `nzip` 取最短输入长度，tuple 元素保留引用。
- `nproduct` 是左主序笛卡尔积，长度乘积必须能放进 `int`。

`nview` 还提供随机访问 iterator，因此可以 range-for，也能交给真正接受随机访问 iterator
的标准算法。迭代器内部借用 view 本身，不能比 view 活得更久。

## 4. `nfunc`：把位置、key 和 value 分开

头文件：`src-v3/func.hpp`

```cpp
template<class Domain, class Eval>
struct nfunc {
    Domain domain;
    Eval eval;
    int len() const;
    decltype(auto) key(int position) const;
    decltype(auto) operator[](int position) const;
    decltype(auto) operator()(Key&& key) const;
};
```

`domain[position]` 给语义 key，`eval(key)` 给 value：

```cpp
vector<string> names{"alice", "bob"};
vector<int> score{80, 95};
unordered_map<string, int> locate{{"alice", 0}, {"bob", 1}};

auto f = nfunc_bind(nall(names), nall(score),
                    [&](const string& key) { return locate.at(key); });

assert(f.key(1) == "bob");
assert(f[1] == 95);             // 按 position
assert(f("alice") == 80);      // 按 key
```

稠密下标是常见特例：

```cpp
auto dense = nfunc_bind(nall(score));  // key 就是 0..n-1
```

### 4.1 `nanchors`：无工作记忆地锚定两组枚举

当 `keys[i]` 就应映射到 `values[i]`，但调用者不想另外声明和维护 locator 时：

```cpp
vector<string> names{"alice", "bob"};
vector<int> score{80, 95};

auto f = nanchors(nall(names), nall(score));
assert(f[1] == 95);
assert(f("alice") == 80);
```

`nanchors` 在构造时物化并拥有一个 `key -> position` 的 `unordered_map`，返回值仍是普通
`nfunc`，因此可以直接继续使用 `nvalues`、`nentries`、`nredomain` 和 `nmap_values`。
构造期望 `O(n)`，按 position 或 key 求值期望 `O(1)`，完整枚举期望 `O(n)`，索引额外占用
`O(n)` 空间；它不会退化成逐次线性扫描。

默认入口使用 `std::hash<Key>` 和 `std::equal_to<Key>`。无标准哈希的 key 可以显式给出
规则：

```cpp
auto f = nanchors(nall(keys), nall(values), key_hash, key_equal);
```

调用者保证：两组序列等长、key 唯一、哈希与相等关系一致、查询的 key 存在。descriptor
借用的 owner 必须活得足够久，而且建表后 key 的长度、次序和值不可改变；payload value 可以修改。
这些是注释契约，不会生成 concept/trait 或非法输入恢复层。

如果题目已经有更紧凑的数组 inverse、坐标压缩结果或其他 locator，继续使用
`nfunc_bind(keys,values,locate)`，避免重复建哈希表。两者的分工是：

```text
nanchors    自动拥有索引，消除调用者的 locator 工作记忆
nfunc_bind  接入已有 locator，保留题目特定的时间与空间结构
```

### 4.2 组合接口

```text
nkeys(f)                       domain
nvalues(f)                     按 domain 枚举 value 的 view
nentries(f)                    (key,value) view
nredomain(f,new_domain)        只更换枚举域
nrestrict(f,new_domain)        nredomain 的语义别名
nmap_values(f,transform)       保持 key，变换 value
ncompose(outer,inner)          outer(inner(key))
nselect_positions(f,positions) 按原 position 选择新 domain
```

`nkeys/nvalues/nentries` 对左值 `nfunc` 只借用，对右值 `nfunc` 则移入并由返回的
descriptor 拥有。因此 move-only 或内部有状态的 evaluator 不会因为遍历左值而被暗中
复制。借用结果不得比原 `nfunc` 活得更久。

`nredomain` 和 `nrestrict` 不检查新 key 是否能被 evaluator 处理。`nmap_values` 保留 transform
的返回类别；这也是 V3 保留该名字而不把它降级成普通 `nmap` 的原因。

`nproduct` 与 `nmap_values` 是稳定的公共名字，不会因为底层实现缩小而消失。

### 4.3 `discrete.hpp`：用位置计划打通 view 与 func

V2 的问题不是 `nruns/nsort` 这些需求不存在，而是它们曾经被 holder、
locator、trait 和结果稳定化协议包得太重。`src-v3/discrete.hpp` 只增加一个内核：

```text
位置列表是结构计划
nview + 计划  -> 重排位置的 nview
nfunc + 计划  -> 重排 domain、保留 evaluator 的 nfunc
```

```cpp
#include "src-v3/discrete.hpp"

vector<int> a{40, 10, 30, 20};
auto picked = nselect(nall(a), vector<int>{3, 1, 3}); // 20,10,20
auto odd = nfilter(nall(a), [](int x) { return x & 1; });
auto sorted_view = norder(nall(a));                  // 懒重排，a 未变
auto materialized = ncollect(sorted_view);           // vector<int>{10,20,30,40}
```

`vector<int>` 位置计划会被移入返回的 descriptor，所以 `nfilter/norder` 不会借用已销毁的
局部下标容器。重复位置是合法的；若 source 产生左值，它们会别名到同一元素。
`ncollect` 是明确物化边界，会递归移除 `nzip/nproduct` 所产生 pair/tuple 内部的引用。

`nindexed` 只是 `nrange + nzip` 的命名结构内核，不分配也不物化：

```cpp
for (auto&& value : nvalues(f)) {}
for (auto&& [key, value] : nentries(f)) {}
for (auto&& [position, value] : nindexed(nvalues(f))) {}
for (auto&& [position, entry] : nindexed(nentries(f))) {
    auto&& [key, value] = entry;
}
```

`nfunc` 本身不定义含糊的默认 `begin/end`：调用者要显式选择 key、value 或 entry。

结构投影：

```text
nselect(source,positions)             按位置选择/重排
nslice(source,left,right)             [left,right)
nstride(source,first,last,step)       非零步长
nstride(source,step)                  正数从左，负数从右
nfilter(source,predicate)             稳定保留命中位置
nindexed(source)                      (position,value) 惰性 view
nargsort(source,compare,projection)   vector<int> 排序计划
norder(source,compare,projection)     应用计划，不移动值
```

值操作与基础序列内核：

```text
nprefix(source,identity={},operation=plus<>)  含 ID 的左扫描
nsuffix(source,identity={},operation=plus<>)  含 ID 的右扫描
nsort(source,compare,projection)      原地排序 source[position] 左值
nreverse_inplace(source)              原地反转值
naccumulate(source,initial,operation) 从左到右折叠
neach(source,action)                  按序调用并返回 action
nfind_if / ncount_if                  返回位置（未找到为 len）/数量
nall_of / nany_of / nnone_of         量词
nargmin / nargmax                     极值位置（空序列为 len）
nlower / nupper                       已排序位置序列的插入位置
```

`nprefix/nsuffix` 不是切片别名，而是物化全部前后缀折叠值。默认 accumulator 是 source
元素的去引用值类型，默认 ID 为该类型的 `{}`，默认 OP 为 `plus<>`：

```cpp
vector<int> a{2, 3, 5};
auto prefix = nprefix(nall(a)); // {0,2,5,10}
auto suffix = nsuffix(nall(a)); // {10,8,5,0}
```

二者都返回 `n+1` 个值，严格顺序为：

```text
prefix[0]   = ID
prefix[i+1] = OP(prefix[i], source[i])

suffix[n] = ID
suffix[i] = OP(source[i], suffix[i+1])
```

因此非交换 OP 不会被偷偷倒序。显式 ID 同时决定 accumulator 类型，OP 可以任意自定义：

```cpp
auto prefix = nprefix(nall(a), 7LL, operation);
auto suffix = nsuffix(nall(a), string("I"), operation);
```

OP 根据保存的 accumulator 构造一个新值，不应修改旧 accumulator；ID、OP 及其返回类型
满足上述表达式即可。两者时间和空间均为 `O(n)`。只想取得一段序列时使用 `nslice`，不再
为 `source[0,count)` 和 `source[n-count,n)` 维护重复名字。

`nsort(nfunc)` 只交换按 domain 枚举到的值，key 和 domain 顺序不变。这与
`norder(nfunc)` 正好相反：后者重排 domain，但 evaluator 与底层值都不变。原地操作要求
source 产生可交换左值；对重复别名位置排序没有有用的排列语义，调用者应避免它。

分块不伪造“起点 key”，而是返回以完整 `[left,right)` 为 domain key 的 `nfunc`：

```cpp
auto blocks = nblocks(nall(a), 3);       // 最后一块可较短
auto interval = blocks.key(0);           // pair{0,3}
auto first = blocks[0];                  // nslice(a,0,3)

auto windows = nwindows(nall(a), 2, 1);  // 只产生完整窗口
auto runs = nruns(nall(a));              // 相邻相等的极大段
```

```text
nchunks(source,intervals)       通用区间键分块
nblock(source,width,index)      一个定宽块
nblocks(source,width)           覆盖整序列，尾块可短
nwindows(source,width,step=1)   width/step > 0，只枚举完整窗口
nruns(source,together={})       together(previous,current) 定义相邻归并
```

chunk 的每次求值都会复制普通 source descriptor，这使取出的子块可与外层
chunk function 分离，代价是 descriptor 必须可复制。owner 的寿命仍然不会被延长：
普通容器应传 `nall(owner)`，不应把 owner 本身作为 source 期待隐式共享。

主要复杂度：已有计划的 select/slice/stride 构造 `O(1)`；filter/runs 构造 `O(n)`；
argsort/order/sort 为 `O(n log n)`；blocks/windows 的 interval domain 是惰性
`O(1)` 描述，但枚举全部子块当然与子块数成正比。

## 5. `narena` 与根代数

头文件：`src-v3/arena.hpp`

`narena<T>` 是 append-only `vector<T>`，`make(...)` 返回 `int` handle。handle 在 vector
扩容后仍有效，引用和指针不保证有效。它没有 generation、epoch、owner、自动回收或
跨类型身份协议。

这是有意的：需要删除复用、陈旧 handle 检测或事务的结构，可以在这块最小地基之上
单独实现，而不是让所有竞赛结构为这些能力付费。

## 6. `nfhq`：一个 kernel，多棵根

头文件：`src-v3/fhq.hpp`

`nfhq<T,Pull,Push>` 把节点池、随机优先级和策略放在一个 kernel 中。`-1` 是空根；
同一 kernel 可以同时持有任意多棵互不相交的树。

```cpp
nfhq<int> q;
vector<int> a{1, 2, 3, 4};
int root = q.build(nall(a));
auto [left, right] = q.split(root, 2);
root = q.merge(right, left);        // 3,4,1,2
int handle = q.kth(root, 1);
```

主要接口：

```text
make(value)                  新节点 handle
size(root)                   子树节点数
merge(left,right)            destructive 拼接
split(root,left_size)        destructive 按位置切分
split_by(root,predicate)     单调谓词切分
kth(root,position)           第 position 个节点 handle
rank(handle)                 节点在当前根中的位置
root_of(handle)              当前根
expose(handle) / rebuild(h)  保存 handle 的修改协议
sequence(root)               按中序访问 payload 的 nview
```

承重契约：

- merge 的两棵树必须来自同一个 kernel 且节点集合不相交。
- split/merge 消耗旧的根语义；不要把输入 root 继续当独立树使用。
- `split_by` 的谓词沿中序必须先真后假。
- `Pull`/`Push` 接收 `(*this,handle)`，不能跨分配保存节点引用。
- 复杂度是随机优先级下的期望 `O(log n)`。

这解决了 V2 merge/split 卡手的根因：交易对象是同一 kernel 中的普通整数根，不再由
每棵树的 owner/domain 类型阻止组合。安全边界放在清楚的 destructive contract 中。

## 7. 区间结构：按真正不同的 merge 语义拆分

头文件：`src-v3/segment.hpp`

### 7.1 只走拓扑：`nsegment_trace / nsegment_cover`

这两个函数不理解 aggregate、tag、pushup 或 pushdown，只把二叉线段拓扑交给 callback：

```cpp
nsegment_trace(root, lo, hi, position, child, visit);
nsegment_cover(root, lo, hi, left, right, child, visit);
```

`child(node,side)` 返回现存孩子、动态创建的孩子，或用负 handle 表示不存在。`trace` 按
root 到 leaf 的顺序访问包含 position 的路径；`cover` 按从左到右的顺序访问
`[left,right)` 的规范分解节点。visit 可以只接收 `node`，也可以接收完整
`(node,node_left,node_right)`。

静态 heap topology 有直接重载，base 是覆盖长度的二次幂：

```cpp
vector<multiset<int>> tags(2 * base);

nsegment_trace(base, position, [&](int node) {
    tags[node].insert(value);               // 点插入写入所有祖先
});

nsegment_cover(base, left, right, [&](int node) {
    answer += query(tags[node]);             // 区间查询规范分解
});
```

动态开点只需更换 child callable，同一份 walk 不变：

```cpp
using outer_tree = nsparse_seg<multiset<int>, monostate>;
outer_tree outer(lo, hi);
int root = outer.make(multiset<int>{});

auto open_child = [&](int node, int side) {
    int next = side ? outer[node].right : outer[node].left;
    if (next < 0) {
        next = outer.make(multiset<int>{});
        if (side) outer[node].right = next;
        else outer[node].left = next;
    }
    return next;
};

nsegment_trace(root, lo, hi, x, open_child, [&](int node) {
    outer[node].aggregate.insert(y);
});
```

内层并不需要是 STL 容器。对于**强制在线**的动态区间次序统计，应把两个维度反过来：
`nsparse_seg<int,monostate>` 划分固定值域，每个 aggregate 保存一棵维护数组位置的 FHQ
root，所有内层根共同使用一个 `nfhq<int>` kernel：

```cpp
int insert_position(int root, int position) {
    auto [a, b] = positions.split_by(root, [&](int y) { return y < position; });
    return positions.merge(positions.merge(a, positions.make(position)), b);
}

nsegment_trace(root, value_lo, value_hi, value, open_child, [&](int node) {
    outer[node].aggregate = insert_position(outer[node].aggregate, position);
});
```

FHQ 对位置做 `count_less(right)-count_less(left)`，即可判断某个值域节点中有多少元素落在
查询位置区间。排名用 `nsegment_cover` 分解值域前缀；第 k 小则从值域根向下，每层查询
左孩子的位置计数并选择分支。修改只需沿旧值路径删除 position，再沿新值路径插入。
整个过程逐条读取并回答操作，不预读修改值，也不做离散化。

每个数组元素在每个值域祖先中拥有不同的 FHQ 节点，因此挂在不同外层节点上的活动根
互不共享，满足 destructive merge 的契约；整数 root 只是交易句柄，不需要 owner facade。
设值域高度为 `B`，排名、第 k 小、前驱、后继和单点修改都是期望
`O(B log n)`。示例把删除后已经脱离所有根的单节点 handle 放进 problem-local free list，
经 `operator[]` 重置后再插入，因此 FHQ 池为 `O(nB)`；这是整数 handle 与公开结构接口带来
的自由，而不是 kernel 暗中管理所有权。外层拓扑仍会保留历史出现过的值路径。完整可提交装配见
[`examples-v3/dynamic_interval_order_statistics.cpp`](./examples-v3/dynamic_interval_order_statistics.cpp)。

这同时覆盖树套树的两组对偶装配：

```text
点更新、区间查询：trace 写沿途节点，cover 读取规范节点
区间更新、单点查询：cover 写规范节点，trace 读取沿途节点
```

静态 trace/cover 为 `O(log n)`；动态版本为 `O(log(hi-lo))`，空间只由 child 是否开点
决定。动态 child 在 `make` 后必须重新用 handle 索引 parent，不能跨 arena 扩容保存 node
引用。

### 7.2 `nseg`

`nseg<T,M>` 是迭代线段树。`M` 只需单位元和结合律，合并保持左到右顺序，允许非交换。

```cpp
vector<long long> a{1, 2, 3, 4};
nseg<long long> seg(nall(a));
assert(seg.fold(1, 4) == 9);
seg.set(2, 10);
```

`pointwise(other)` 对同位置叶子逐点 destructive merge，再重建内部节点。两棵树必须长度
相同，且操作对象具有同样的数学意义。复杂度 `O(n)`，不是伪装成 `O(log n)` 的根合并。

### 7.3 `nlazyseg`

```cpp
nlazy_addsum<long long> seg(nall(a));
seg.apply(1, 4, 5);
assert(seg.fold(0, 4) == 25);
```

一般形式是 `nlazyseg<State,Tag,Merge,Action>`：

```text
Merge.id()
Merge(left,right)                    结合、保序
Action.tag_id()
Action.compose(newer,older)          先执行 older，再执行 newer
Action.apply(aggregate,tag,length)   对区间聚合施加 tag
```

`apply` 必须对 Merge 可分配。查询会 push lazy，因此逻辑上是查询、物理上可能修改内部缓存。

### 7.4 `nsparse_seg`

一个 append-only kernel 同时承载 destructive 与 persistent 根：

```cpp
nsparse_seg<long long> seg(0, 1LL << 60);
int a = -1;
a = seg.set(a, 100, 7);          // destructive
int b = seg.set_copy(a, 200, 9); // persistent path-copy
int c = seg.merge_copy(a, b);    // 保留旧根
```

```text
set / combine             destructive 单点赋值/合并
set_copy / combine_copy   path-copy
merge                     destructive 物化节点 union
merge_copy                persistent union，可共享空侧子树
clone                     深拷贝为独占根
fold / get / aggregate    查询
make(value,left,right)    公开创建任意节点
make()                    创建单位聚合节点
operator[](handle)        直接访问节点
pull(handle)              从两个孩子重算 aggregate
```

`make/operator[]` 是刻意保留的结构逃生口。若之后还调用内建 fold/merge，调用者创建的
value、left、right 必须已经满足 aggregate 不变量；若只把它用作拓扑和 tag storage，可以
像上例一样用 `monostate` 架空 merge，并只操作公开节点。`make` 可能令 node 引用失效，整数
handle 始终稳定。

destructive 根必须独占且互不重叠；persistent 根可能共享节点，不能直接送进 destructive
操作，除非先 `clone`。这不是一种“万能 merge”，而是同一节点内核上的四种明确交易。

## 8. 基础数据结构

头文件：`src-v3/ds.hpp`

### `nfenwick<T,Group>`

要求 Abel 群：结合、交换、单位元、逆元。`add`、`prefix`、`fold`、`get`、`set` 均为
`O(log n)`。`lower(target)` 还要求前缀在比较器下单调；找不到时返回 `len()`。

### `ndsu`

路径压缩 + 按大小合并。`find/same/size/merge` 摊还近似常数。

### `npotential_dsu<T,Group>`

带势能并查集要求 Abel 群。`merge(a,b,delta)` 施加
`value(b)-value(a)=delta`，返回新约束是否一致；已被推出且一致也返回 true。
`difference(a,b)` 在连通时返回差值，否则为 `nullopt`。路径压缩和按大小合并保留。

### `nrollback_dsu`

不做路径压缩，成功 merge 才写历史。用 `time()` 记录检查点，`rollback(time)` 回退。
单次 merge/find 为 `O(log n)` 上界，undo 为 `O(1)`。

### `nqueue_agg<T,M>`

双栈聚合队列，保持非交换顺序。`push/pop/front/fold` 摊还 `O(1)`。

### `nsparse_table<T,O>`

要求结合、交换、幂等，查询必须非空。预处理 `O(n log n)`，查询 `O(1)`。

### `nwavelet<T>`

头文件：`src-v3/wavelet.hpp`

静态 Wavelet Matrix 先把任意可排序值压缩成 rank，因此不要求整数值域：

```cpp
vector<long long> a{7, -2, 7, 4, 9};
nwavelet wave(nall(a));
auto x = wave.kth(1, 5, 2);          // 子数组第 2 小，0-based
int y = wave.less(0, 5, 7);          // 严格小于 7 的数量
int z = wave.count(0, 5, 4LL, 9LL);  // 值域 [4,9)
```

还提供 `access`、单值 `count`、`next(>=lower)` 和 `previous(<upper)`。构造
`O(n log n+n log sigma)`，空间 `O(n log sigma)`，查询 `O(log sigma)`；所有区间合法，
`kth` 的 order 必须落在子数组内。

## 9. 图不是容器：`ngraph` 三端口

头文件：`src-v3/graph.hpp`

```cpp
ngraph graph{
    vertices,                       // 有限 key descriptor
    [&](Key vertex) { return adjacency(vertex); },
    [&](const Edge& edge) { return edge.to; }
};
```

算法只使用：

```text
graph.vertices
graph.edges(vertex)
graph.target(edge)
```

邻接结果只需能被 range-for。顶点不是稠密整数时，再传 `index(key)->[0,n)`。没有
`ngraph_like` concept，也没有默认 edge trait。

### 9.1 任意后端示例

```cpp
vector<vector<int>> adjacency{{1, 2}, {2}, {}};
auto graph = ngraph{
    nrange(3),
    [&](int vertex) -> const vector<int>& { return adjacency[vertex]; }
};
auto distance = nbfs(graph, 0);
```

只要端口表达式成立，同一个 BFS 也可以运行在隐式状态图、forward-star、CSR 或用户自定义
压缩结构上。

### 9.2 `ncsr`

头文件：`src-v3/graph_store.hpp`

```cpp
struct edge { int from, to; long long weight; };
vector<edge> edges = ...;
auto graph = nmake_csr(n, nall(edges),
                       [](const edge& e) { return e.from; },
                       [](const edge& e) { return e.to; });
```

构造 `O(V+E)`，每个 source 桶内保持输入顺序，edge record 不要求默认构造。`ncsr` 自己
已经满足图端口，也可用 `.view()` 得到借用它的轻量 `ngraph`。`.view()` 在 owner 移动或
销毁后失效。

## 10. 图算法

头文件：`graph.hpp`、`graph_algo.hpp`、`flow.hpp`

```text
nbfs / nbfs_many     O(V+E)，返回按稠密 position 存储的距离
n01bfs                O(V+E)，边权严格为 0/1，不可达为 -1
ndijkstra            O((V+E)log V)，边权非负
ntoposort             O(V+E)，返回稠密 position；长度不足表示有环
nscc                  O(V+E)，Kosaraju，需要正图和反图
nkruskal              O(E log E)，返回最小生成森林的权值和输入 edge positions
ndinic                Dinic 最大流与残量 cut
nhopcroft_karp        二分图最大匹配
```

`nscc(forward,reverse,index)` 要求两张图的 vertex descriptor 在相同 position 上表示同一个
key。V3 不替用户偷偷构造反图，因为反图的存储策略本来就是自由度的一部分。

`ndinic<C>` 的容量类型要支持零值、比较、加减与 `min`。DFS 是递归实现，极深层次图需要
由调用者评估栈深度。

## 11. 静态 rooted projection 与 HLD

### 11.1 `nroot`

`nroot(graph,roots,index)` 对给定根做 first-discovery，返回 `nrooted`：

```text
parents() subtree_sizes() depths() components() positions()
order() roots() children(vertex)
```

这些语义入口大量使用 `nfunc`，从而保留“语义 key 与内部 position 不相等”的自由。
`parent[root]==root`。没有被 roots 覆盖的点保留 unseen metadata。

`nroot` 可以接受有向图甚至有环图；它只生成遍历森林，不声称原图本身是树。需要树语义
的算法必须由调用者保证输入是森林。

### 11.2 `nhld`

头文件：`src-v3/tree.hpp`

核心构造端口是：

```cpp
auto layout = nhld(vertices, roots, children, index);
```

便利入口 `nhld(rooted)` 只是把 `nrooted` 投影接入同一个核心，不制造另一套树 owner。

```text
parents() depths() subtree_sizes() heads() positions()
order() lca(a,b) path(a,b)
```

`path(a,b)` 返回按 a 到 b 的遍历顺序排列的 `npath_piece{left,right,reverse}`。reverse 为真
表示该 HLD 基区间要从右向左读取。这一位不能在字符串拼接、矩阵乘法等非交换路径聚合中
丢掉。

HLD 需要 roots/children 真正描述一片 rooted forest，每个非根恰好出现一次；`lca/path`
的两个顶点必须在同一组件。构造 `O(n)`，LCA 和分段数 `O(log n)`。

### 11.3 `nreroot`

```cpp
auto answer = nreroot(graph, base, lift, merge, index);
```

输入是每条无向边以两个方向各出现一次的森林。`base(vertex)` 产生点自身状态；
`lift(state,from,edge_from_to)` 把 from 排除 to 后的聚合跨边送给 to；`merge` 提供单位元并
满足结合律。合并严格按每个点的邻接顺序进行，因此不强制交换律。返回值按稠密 position
存储，时间和空间都是 `O(V+E)`。邻接必须可重复枚举。

## 12. 动态森林：Euler Tour Tree 是独立工具，不是假万能树

头文件：`src-v3/dynamic_tree.hpp`

`nett_forest<T,M>` 在 `nfhq` 上维护 Euler tour。每个顶点有一个稳定 token，每条无向边
增加两个 occurrence：

```cpp
vector<long long> value{1, 2, 3, 4};
nett_forest<long long> forest(nall(value));
forest.link(0, 1);
forest.link(1, 2);
assert(forest.connected(0, 2));
assert(forest.fold(0) == 6);       // 整个组件聚合
forest.cut(1, 2);
```

接口：`connected`、`component_size`、`fold`、`set`、`reroot`、`link`、`cut`。

`M` 必须结合、交换并提供单位元，因为 reroot 会旋转 Euler 环。`link(a,b)` 要求不同组件，
`cut(a,b)` 要求边存在。操作期望 `O(log n)`；edge 查找平均 `O(1)`。

当前 arena 不回收被 cut 的两个 occurrence，所以空间与“历史上执行过的 link 数”有关，
不是只与当前边数有关。这是明确的现阶段边界。

ETT 解决连通性、组件大小和交换聚合，不负责路径聚合。动态路径应使用单独适配的结构，
不能为了统一而强迫 Euler Tour Tree 承担错误语义。

### 12.1 `nlct`：动态有序路径

头文件：`src-v3/link_cut.hpp`

`nlct<T,M>` 是独立的 Link-Cut Tree。顶点是稳定稠密 position；`M` 只要求单位元和
结合律。每个辅助节点同时维护正向与反向聚合，因此字符串拼接等非交换操作也能保持路径
方向。

```text
link / cut              动态森林边，要求 link 跨组件、cut 是现存直接边
connected / find_root   动态连通性
make_root / access      标准 preferred-path 操作
set / get               顶点值
fold(a,b)               从 a 到 b 的有序顶点路径聚合
path_size(a,b)          路径点数
```

所有操作摊还 `O(log n)`。LCT 不复用 FHQ 的物理旋转代码，因为 splay preferred path 与
随机堆序列不是同一个结构；它复用的是同一套显式代数契约和“不同语义不强行共用 owner”
原则。

## 13. 数学与多项式

### 13.1 `math.hpp`

```text
ndiv_floor / ndiv_ceil   任意单一整数类型的数学向下/向上整除
npow                 泛型快速幂
next_gcd             扩展 gcd，返回 {gcd,x,y}
ninv_mod             可选模逆
ncrt                 两同余合并，不相容返回 nullopt
nmodint<MOD>         静态模整数
ncomb<Mint>          阶乘/逆阶乘、排列数和组合数
nsieve               线性筛、最小质因子、范围内分解与 phi
```

`ndiv_floor(a,b)` / `ndiv_ceil(a,b)` 要求 `b != 0`，返回数学意义上的
`floor(a / b)` / `ceil(a / b)`，同时处理负数除数和无符号整数。模板参数 `I` 是同一个
内建整数类型；带符号最小值除以 `-1` 不在契约内，因为普通 C++ 除法本身无法表示该商。

`ncrt` 要求最终 lcm 放进 `long long`。`nmodint::inv()` 和除法要求逆元存在；实现不会把
不可逆除法改写成别的运算。`ncomb` 要求阶乘中用到的每个分母可逆。

### 13.2 64 位素数与分解

头文件：`src-v3/number.hpp`

```text
nmulmod64 / npowmod64   __uint128_t 保护的模乘与模幂
nisprime                对整个 uint64_t 确定性的 Miller-Rabin
npollard                Brent 风格 Pollard-Rho，输入为合数
nfactor                 升序返回带重数的质因子
```

`nfactor` 要求输入至少为 1，期望复杂度依赖质因子形状；它不是小范围筛法的替代品。

### 13.3 `poly.hpp`

```text
nntt<MOD,ROOT>
nconvolution<MOD,ROOT>
npoly_derivative
npoly_integral
npoly_inverse<MOD,ROOT>
```

`nntt` 的长度必须是非零二次幂并整除 `MOD-1`，ROOT 必须是原根。卷积在很小规模时自动
使用朴素算法，否则使用 NTT。FPS inverse 要求常数项可逆。

### 13.4 矩阵与线性代数

头文件：`src-v3/linear.hpp`

`nmatrix<T>` 是紧凑 row-major owner，提供 `(row,column)` 和行 view。当前线性代数面向
精确域：零比较必须可靠，每个非零 pivot 可除。浮点 eps 不会被默认藏入实现。

```text
nmatmul / nmatpow       矩阵乘法与非负整数幂
nrref                    Gauss-Jordan；可限制允许成为 pivot 的列
ndeterminant             方阵行列式，空矩阵为 1
ninverse                 可逆时返回逆矩阵，否则 nullopt
nlinear_solve            particular + nullspace basis，或 inconsistent
```

朴素乘法 `O(rmk)`；RREF、行列式和求逆为标准三次复杂度。维度相容、方阵条件和右端长度
属于调用者契约。

## 14. 字符串

头文件：`src-v3/string.hpp`

所有算法直接依赖 `.len()` 和 `operator[]`，因此传普通 string 时先 `nall(string_lvalue)`。

```text
nprefix_function 前缀函数
nz               Z 函数，非空时 z[0]=n
nkmp             所有匹配位置；空模式匹配每个边界
nmanacher        odd/even 回文半径
nsuffix_array    可比较字母表，O(n log n)
nlcp             Kasai，相邻后缀 LCP
```

`nlcp(sequence,suffix)` 要求 suffix 是该序列的合法排列。算法不重复验证。

### 14.1 `nac`：只保留自动机内核

头文件：`src-v3/automata.hpp`

```cpp
nac automaton(26, nlowercase{});
vector<int> terminal;
for (string& pattern : patterns) terminal.push_back(automaton.add(nall(pattern)));
automaton.build();
auto count = automaton.occurrences(nall(text));
```

`add` 返回模式终止 state，payload 由调用者在外部自由组织。全部模式必须先 add，再调用一次
build；build 会补全缺失转移，此后不能继续结构插入。`step` 做单步转移，`walk` 返回文本
每一位后的状态，`occurrences` 沿 failure 反向传播计数。空模式位于 state 0，出现 `n+1`
次。字母映射只是返回 `[0,sigma)` 的普通 callable，没有 alphabet trait。

## 15. 几何

头文件：`src-v3/geom.hpp`

```text
npoint<T>              二维点及基本线性运算
ndot / ncross          点积、叉积、三点定向面积
non_segment            点是否位于闭线段
nsegment_intersect     两闭线段是否相交
npolygon_area2         有向面积的两倍
nconvex_hull           严格凸包，删除边上共线内部点
nline_intersection     无限直线交点，平行/重合返回 nullopt
```

整数谓词要求坐标乘积能放进 `T`。`npolygon_area2` 要求多边形非空。浮点鲁棒策略没有被
藏进默认 eps；需要浮点容差时由题目层明确提供。

### 15.1 `nlichao`：函数与根都保持自由

头文件：`src-v3/opt.hpp`

`nlichao<Line,X,Y,Eval,Better>` 是整数坐标 `[lo,hi)` 上的稀疏 Li Chao kernel。
`Eval(line,x)` 求值，`Better(a,b)` 决定取最小还是最大；任意两条合法函数至多相交一次。

```cpp
nlichao<nline<long long>, long long, long long> tree(-1000000, 1000001, INF);
int root = -1;
root = tree.add(root, {2, 7});
root = tree.add_segment(root, -10, 20, {-3, 5});
long long answer = tree.query(root, x);
```

同一 kernel 可持有多棵互不共享节点的普通整数根，更新是 destructive。整条函数和线段
函数插入、单点查询均为 `O(log(hi-lo))`；空根返回构造时给出的 infinity。

## 16. 跨结构装配

### 16.1 CSR → rooted projection → HLD → 区间结构

```cpp
auto graph = nmake_csr(n, edges, from, to);
auto rooted = nroot(graph.view(), roots);
auto hld = nhld(rooted);

vector<long long> base(n);
for (int p = 0; p < n; ++p)
    base[p] = vertex_value[hld.order()[p]];
nseg<long long> seg(nall(base));
```

这里没有把 vertex handle、Euler/HLD position 和 segment node 强制统一成一种 token。
它们通过 `nview/nfunc` 进行显式映射：身份与位置不是一回事，避免了旧架构中跨 DS 交易
必须伪造公共 domain 的问题。

### 16.2 多根 kernel 的 destructive 交易

FHQ 与 sparse segment 都采用：

```text
一个策略/节点 kernel
多个普通整数根
局部注释声明根是否独占、共享、被消耗
```

因此 merge/split 不需要复制整个 owner，也不需要 runtime same_domain 检查。若题目确实
需要跨 kernel 迁移，应显式遍历/克隆；V3 不把高成本迁移伪装成常数时间 merge。

## 17. V2 全量能力审计后的取舍

V3 不以“恢复全部 V2 公共符号”为目标。当前取舍如下。

### 17.1 已从零重建并保留

```text
位置投影：nview、range/sub/reverse/project/map/gather/zip/product
离散函数：nfunc、bind/anchors/keys/values/entries/redomain/restrict/map_values/compose
离散算法：select/slice/stride/filter/collect/order/sort、序列折叠、区间键 chunks/blocks/windows/runs
节点内核：narena、隐式 FHQ、多根 destructive split/merge
区间结构：Fenwick、迭代/懒/稀疏/持久根线段树、聚合队列、Sparse Table、Wavelet Matrix
并查集：普通、带势能与 rollback
图端口：隐式/任意邻接 descriptor、CSR owner
图算法：BFS、Dijkstra、拓扑、SCC、Kruskal、Dinic、Hopcroft-Karp
树：rooted projection、HLD/LCA、Euler Tour 动态森林
数学：快速幂、exgcd、CRT、静态模数、组合表、线性筛、NTT/FPS inverse
字符串：prefix/Z/KMP/Manacher/SA/LCP、AC 自动机内核
几何：精确二维基础、线段、凸包、面积、直线交点
```

### 17.2 当前明确不搬运

```text
checked/unsafe 双实现与 npre 恢复链
nmaybe、nvector、ndeque、nheap 等 STL 同义包装
覆盖整个 STL 的 nfill/ncopy/nreplace 等同义改名层
nenumerable 游标森林与循环宏
nresource_pool/nnode_domain/generation/epoch 的全局身份体系
nset/nmap/nbije/nrel 等大规模关联容器包装族
统一图 edge trait、统一 owner facade、自动反图和自动纠错
竞赛 I/O 包装
强制 amalgamate 和统一 Nitori.h
```

不是说这些能力永远无用，而是它们目前不能证明值得占据 V3 的结构复杂度预算。

### 17.3 高价值但尚未实现

```text
路径恢复
更完整的几何查询
```

这些是候选清单，不是承诺。每个候选进入 `src-v3` 前都要回答：它能否复用现有内核、
是否比题内手写更划算、契约是否能用短注释说清、是否能写独立变态对拍。

## 18. 测试、benchmark 与预算

V3 不复用 V2 测试，历史归档也不参与 include、搜索或测试发现。独立入口：

```bash
cd /home/tnuzy/NitoriSTL
python3 test-v3/run.py
python3 test-v3/run.py graph_store_property tree_hld_property
python3 test-v3/audit.py
python3 bench-v3/run.py
python3 test-v3/measure.py
```

`run.py` 对每个测试分别执行：

```text
debug + _GLIBCXX_ASSERTIONS
-O2 + NDEBUG
ASan + UBSan
```

并启用 `-Wall -Wextra -Wpedantic -Wshadow -Werror`。性质测试使用独立朴素模型、随机操作、
非交换操作、空结构、重复值、多组件、深链、星形、根共享和 destructive/persistent 混用
边界。测试通过只证明已经覆盖的契约内行为，不等于所有模板实例都天然正确。

`measure.py` 通过词法扫描排除注释和布局空白，同时保留字符串/raw string 内的内容，并有
自测试保护计量规则。总预算上限是 `131072` 语义字节；`discrete.hpp` 另有
`10240` 语义字节的局部闸门，防止它再长成包装森林。

`audit.py` 检查旧源码是否回流到活动路径、历史归档校验和是否被解包、V2 include
泄漏、会在 `NDEBUG` 消失的 assert 测试、concept/domain 压力回流、文档本地链接以及
每个头文件能否独立 include。

benchmark 是 deterministic workload，用于观察直接排序与投影排序、结构 order/runs、FHQ 节点大小、split/merge、线段树、
Wavelet Matrix、LCT 路径、直接邻接与 graph port、CSR 构造/BFS、rooted projection、
稀疏节点数和峰值 RSS。时间值受机器波动影响，checksum 与规模必须稳定。

## 19. 扩展 V3 的自检流程

新增能力前：

1. 从题目需求写出最小操作集合和暴力基线。
2. 判断它是新的数学结构，还是现有 root/view/graph port 的一个策略。
3. 只模板化真实自由维度，不为“以后也许”增加 trait。
4. 把合法输入、代数定律、生命周期、失效条件和复杂度写在实现旁。
5. 先写固定反例，再写独立随机 oracle。
6. 运行窄测三模式；阶段闸再运行全库、benchmark 和 size audit。
7. 检查是否出现重复 owner、重复位置映射、伪 `O(1)` merge 或隐藏 materialization。

停止信号：

```text
为了一个算法新增多个仅用于通过 concept 的空类型
同一份 parent/order/position 被不同 owner 重复维护
简单 split/merge 被 domain/epoch/facade 包裹成事务框架
算法只能吃某个默认容器而不能吃端口
注释删掉后已经没人能说清数学前提
模块增长很多，但用户代码并没有更短、更自由或更可验证
```

V3 的完成标准不是符号数量，而是：核心结构短、组合路径直、前提可见、错误能由独立台架
打出来，并且用户可以在比赛中拆开、修改和重新装配。

## 20. 当前公共符号速查

```text
core:       nlen nchmin nchmax
view:       nview nall ntabulate nrange nsub nreverse nproject nmap ngather nzip nproduct
func:       nfunc nkeys nvalues nentries nredomain nrestrict nmap_values ncompose
            nselect_positions nfunc_bind nanchors
discrete:   nselect nslice nstride nfilter nindexed ncollect nprefix nsuffix
            naccumulate neach nfind_if ncount_if nall_of nany_of nnone_of
            nargmin nargmax nlower nupper nargsort norder nsort nreverse_inplace
            nchunks nblock nblocks nwindows nruns
memory:     narena
fhq:        nfhq nfhq_noop nmake_fhq
segment:    nsegment_trace nsegment_cover nadd nseg nlazyseg naddsum_action
            nlazy_addsum nsparse_seg
ds:         nsum_group nfenwick ndsu npotential_dsu nrollback_dsu nqueue_agg
            nsparse_table nwavelet
graph:      ngraph nto_self nordinal nbfs nbfs_many nrooted nroot ncsr nmake_csr
graph alg:  n01bfs ndijkstra ntoposort nscc nscc_result
tree:       npath_piece nhld_layout nhld nreroot nett_forest nlct
flow:       ndinic nmatching nhopcroft_karp nmst_result nkruskal
math:       ndiv_floor ndiv_ceil npow negcd_result next_gcd ninv_mod ncrt nmodint ncomb nsieve
number:     nmulmod64 npowmod64 nisprime nsplitmix64 npollard nfactor
poly:       nntt nconvolution npoly_derivative npoly_integral npoly_inverse
linear:     nmatrix nmatmul nmatpow nrref_result nrref ndeterminant ninverse
            nlinear_solution nlinear_solve
string:     nprefix_function nz nkmp npalindrome_radii nmanacher nsuffix_array nlcp
automata:   nlowercase nac
geom:       npoint ndot ncross non_segment nsegment_intersect npolygon_area2
            nconvex_hull nline_intersection
opt:        nline nline_eval nlichao
```

这个索引只帮助搜索。真正的前提以 `src-v3/*.hpp` 中紧邻实现的注释和本文对应章节为准。
