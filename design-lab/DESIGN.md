# Nitori：让题目的结构直接留在代码里

2026-09-13 · 第一轮可执行设计 · 实验候选，不是全库迁移完成声明。

## 设计命题

CF 时希望借助库更快写对题，ICPC 纸面时希望从有限页数中恢复算法并改出变体。
两者共享的核心是：一个局部操作的状态、复杂度和不变量，可以在局部读懂。

本轮的设计单位是**一个能独立使用的机制与题型配方**。不建立跨所有结构的用户模型，
不追求每个算法都以同一种 descriptor 接入。具体容器、数学对象和遍历过程各自保留原貌。

这不是停止泛型化：邻接可以来自 vector、CSR、隐式状态转移；合并可以非交换；
Tag 可以是仿射或赋值。删除的是使用这些自由以前必须先携带的额外语义。

## 三种粒度，各自承担什么

| 粒度 | 例子 | 应当满足 |
| --- | --- | --- |
| 机制 | 迭代区间 fold、HLD 分解、FHQ split/merge | 不变量短，局部能修改；只接收真实变化点 |
| 配方 | 区间仿射和、非交换树上路径、排序后分组 | 可以编译运行，标明题目里需要改的位置 |
| 扩展 | 任意 key 的域映射、Beats、动态森林 | 有实际需求时引入；不成为普通题的入口前提 |

“常用”决定默认展示顺序，不直接决定删库。低频但容易写错、很难临场恢复的数论或几何
仍可有很高的纸面价值。一个人能默写的五行循环反而不必做成一族库函数。

## 第一展区：把计划留在明处

```cpp
auto order = nitori::argsort(rows, std::less<>{}, &Row::key);
auto sorted = nitori::gather(std::span(rows), std::span<const int>(order));
auto groups = nitori::run_bounds(sorted, same_key);
for (auto [l, r] : groups)
    for (int i = l; i < r; ++i) use(rows[order[i]]);
```

这里有三种对象：rows 拥有记录，order 拥有位置，groups 拥有边界。sorted 只借用两个数组。
切片切的是 `span<const int>(order).subspan(l,r-l)`，没有自动复制计划、自动建表或共享计数。

`argsort` 返回稳定的等键顺序；实现用原位置打破比较等价，不依赖 stable_sort 的额外缓存。
成本是 O(n log n) 和一个位置数组。借用 gather 的访问成本就是一次位置访问加一次源访问。
`run_bounds` 只求相邻相等的最大段，返回边界快照；复杂增量分段先写在题内。

代价也显式存在：数组必须活得足够久，重分配会使 span 失效。共享所有权没有被换成
“自动安全”的新框架。要返回一组独立结果，就显式复制结果；要保存计划，就保存 order。

本轮没有复刻 nfunc、inverse-first、nproduct 或 nmap_values。V3 的这些能力完整保留。
如果题目确实是函数复合或笛卡尔积，仍可选用；实验要验证的是普通排序分组不必依赖它们。

## 第二展区：图是邻接，树链是数组

```cpp
auto result = nitori::dijkstra(n, source,
    [&](int v) -> const auto& { return adj[v]; },
    &Edge::to, &Edge::cost, INF);
auto route = nitori::restore_path(result.parent, target);

nitori::hld tree(n, root, [&](int v) -> const auto& { return adj[v]; });
auto [l, r] = tree.subtree(v);
```

顶点永远是 `[0,n)` 中的 int，输出数组也以同一个顶点号索引。特殊 key 在进入算法前
映射；坐标大不要求节点号大。端口只用于读取邻接与边目标，没有 graph owner 的强制要求。

最短路提供距离和前驱。路径恢复是高频的可用性需求，本轮选择默认付出一个 int 前驱数组，
无需 visitor/result policy 开关。这会比 V3 的只返回 distance 多出约 4n 字节；若实际
大规模任务证明不划算，可以把“只求距离”做成独立纸面变体，而不立即造策略矩阵。

HLD 自己生成并拥有 parent/depth/size/heavy/head/pos/order。构造时邻接只在调用期间借用，
离开构造后不保留指向图、rooted descriptor 或 key 集合的引用。BFS 序建父子关系、
逆序聚合子树、显式 pending 栈铺重链，因此线性深链不消耗线性调用栈。

为了保持子树连续，沿重链向下时先把浅层的轻子树压栈；深层轻子树后压先处理。
这些轻子树在返回浅层旁支之前被完整展开，故每个节点的子树仍是一个连续区间。
性质测试对随机根的所有祖孙关系检查 `[pos[v],pos[v]+size[v])`。

路径分解从 a 端立即输出片段，从 b 端暂存片段，再反序输出。reverse 表示底层区间反读，
不是交换合并参数就够了；非交换配方为每个区间保存 forward/backward 两份摘要。
边权按深点存储时，仅在最后同链片段排除 LCA，`a==b` 的边路径因此为空。

明确收缩：这个 HLD 只接受一棵连通无向树，不支持局部覆盖、多个组件和任意 key。
不把坏输入变成生成树，不加自动修复。需要森林时在题内明确分组件，或后续独立设计。
构造读取邻接两遍，所以 next 必须可重复；该前提直接写在纸面条目。

## 第三展区：普通区间结构的完整状态在同一处

`segtree<Monoid>`：Monoid 有 `value_type / id / join`。
`lazy_segtree<Algebra>`：在此基础上有 `tag_type / apply / compose`。
这是一个模板参数内的局部命名约定，不是 trait 注册、继承体系或全局代数框架。

```cpp
struct AffineSum {
    using value_type = long long;
    struct tag_type { long long a = 1, b = 0; };
    long long id() const { return 0; }
    long long join(long long x, long long y) const { return x + y; }
    long long apply(long long s, tag_type f, int n) const { return f.a*s + f.b*n; }
    tag_type compose(tag_type newer, tag_type older) const {
        return {newer.a*older.a, newer.a*older.b+newer.b};
    }
};
```

普通 lazy 只有 data、内部节点的 lazy 与 pending。pending 明确区分“没有动作”和默认 tag，
不需要 tag_id 或比较 Tag 是否恒等。apply 覆盖节点时改摘要与延迟；部分覆盖先 push，再
递归，再 pull。查询只 push，不重写父摘要；点赋值先推到叶子再向上重建。
所有真实更新必须落在 `[0,n)`，二次幂补齐的叶子保持单位元。空树和空区间有固定测试。

保留普通 segtree 的 max_right/min_left，并用字符串前缀/后缀检查非交换方向。
**本轮 lazy 尚未提供边界搜索。** 它是范围受限的原型，不能替代已具备这些操作的完整 V3。

普通 lazy 与 Beats 分开，意味着可能保留少量相似递归；这是有意用实现局部重复换取
调用者不必理解 try_apply 的失败语义、叶子成功条件及任意策略钩子。
Beats 也不应被重新写成残缺版本：保留 V3 已验证的底层机制，待真实题型对照再决定边界。

## 横向约束

1. **编号固定 int，数值宽度按题选。** 原型没有全程序 INDEX_64 开关；是默认使用面的收缩，
   不是宣称 64 位位置永远无用。编号和两倍补齐容量须可表示。
2. **半开区间继续统一。** 这条约定消除了实际错误，值得保留。
3. **泛型不证明数学。** 结合律、恒等元、作用分配律、复合顺序写在本地；独立 oracle 攻击它们。
4. **模板类型显式。** 用户能看到 Monoid/Algebra，不靠很多 CTAD 与同义工厂猜出类型。
5. **没有公共大杂烩 core。** 本轮五个模块彼此无本地 include 依赖，只依赖标准库。
6. **原始数组允许被看见。** pos、parent、lazy 状态可检查；修改内部字段时必须保持不变量。
7. **不承诺零成本抽象。** 性能计入源访问、投影、比较、值复制与用户运算的实际成本。

## 对整个库的处置设计

| V3 模块 | 建议归宿 | 当前状态 |
| --- | --- | --- |
| core / view / func / hash / discrete | 普通路径拆到标准容器与显式计划；结构组合为可选层 | sequence 原型；其余未迁移 |
| graph / graph_algo / graph_store / tree | 稠密算法 + 独立存储；特殊 key 在边界处理 | BFS/Dijkstra/HLD 原型；SCC/CSR/reroot 等保留 V3 |
| segment | 普通 seg、普通 lazy、稀疏根、Beats 按语义拆开 | 前两项原型，功能范围不齐 |
| ds | Fenwick、DSU、带势能、rollback、SWAG、RMQ 各有纸面条目 | 未重写；先抽取，再按需求减入口 |
| arena / fhq / bag | 多根内核保留，默认配方只暴露必要操作 | 未迁移；须保留根消费、历史节点增长契约 |
| vec_bag | heuristic/local-search 选件 | 不进入 CF 默认入口 |
| list | 稳定 handle 多链选件 | 保留，按实际使用决定纸面页 |
| dynamic_tree / link_cut | 独立 ETT / LCT 专题条目 | 保留，禁止为了统一而混同路径与连通块 |
| wavelet / opt | 静态顺序统计 / Li Chao 专题 | 保留；缩小抄录依赖 |
| math / number / poly / linear | 以可错点与算法用途拆成条目 | 保留；不要为了总量减小而损失难重写能力 |
| string / automata / geom | 题型索引与独立算法条目 | 保留；按语义与边界组织 |
| io / debug | CF 开发工具与少量纸面基础片段 | 不捆绑进所有算法 |

这张表的“保留”是能力方向，不是已经审查了每个实现的正确性。未迁移项继续以 V3 为准。

## 为什么不直接把所有东西换成 ACL 风格

固定函数指针模板能让依赖很显式，但不便持有运行期模数、比较状态等数据。
全局 struct Ops 可以把状态保留下来，代价是每个简单求和需写一小段类型定义。
本轮选择后者来做原型，验证其纸面可读性；不会同时维护三套等价 public API。

为什么还保留 gather？它表达一个常见、局部的两次下标访问，而且不会隐藏新的 owner。
为什么不为普通向量加 fill/find/count 包装？这些需求已有 STL/循环，暂未证明新名字的收益。
为什么不把坐标压缩也塞进首轮？排序计划已能支撑显式压缩配方；新增结构需通过题目证明。

## 证据如何解释

本轮成本对照会同时显示 V3 的普通左值切片、V3 的显式借用切片与原型 span 切片。
后两者都可以零分配，所以不能声称原型创造了 V3 做不到的优化；收益在于默认入口更少，
调用者不用判断 descriptor 深处是否有一个持有的计划。

HLD 的 path 零堆分配也是 V3 已有能力；本轮保留它。真正新增的证据是：固定 8 MiB 栈下
30 万点链的 BFS/HLD，以及无需 V3 include 的有序路径配方。

五个原型头的规模只与各自的范围对应，不与完整 V3 作缩水百分比。构造时间、运行时间
尚未做严格同 workload 基准，因此不声称算法更快。测试通过也不证明任意模板实例正确。

## 纸面设计

每个模块页都印完整的独立头文件（含标准 include），每个配方页印完整调用和预期输出。
保留 namespace 与类型，不用极小字号追求页数。生成材料从已测试源码直接读取，避免两份
实现漂移。PDF 是这五个实验模块的审阅样张，不是完整 ICPC notebook。

真正赛场版本还需你依据具体赛事页数、编译器和个人默写习惯裁剪，并进行“只看打印件重录”
验收。自动编译头文件不能代替人类的纸面重录测试。

## 下一步的推进标准

先选 6–10 道已完成的代表题，分别使用 V3 与该设计复写。每题记录：首次可编译用时、
查文档次数、为题目改了哪些内部代码、错误发生在数学还是 API 层。
当普通图/分组路线确实省心，再迁移 SCC、Fenwick 等。FHQ、Beats、LCT 的边界另用真实题目验证。

没有真实调用收益的模块不扩张；新的兼容别名只用于有期限的迁移；每个阶段保持独立 oracle。
只有当各能力的迁移和赛场条目验收完成，才讨论替换 V3 的正式权威。
