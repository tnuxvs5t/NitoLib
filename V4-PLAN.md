# NitoriSTL v4 总规划

状态：2026-10-01，v4 第一阶段核心重写完成。
范围：以当前 `src-v3/`、`test-v3/`、`bench-v3/` 和教程冻结出的 v3 能力为输入；不把
`design-lab/` 当作 v4 权威，也不把任何历史归档当作设计来源。

这不是 v3 的小修版。v4 允许对实现、对象布局、模块边界、命名空间组织、descriptor
体系和泛型接口做强破坏改动；但是破坏必须服务于更短的竞赛核心、更清楚的不变量和更稳
定的 Nitori 组件关系，而不是为了追逐某个外部库的外形。

当前能力账本见 `V4-CAPABILITY-MAP.md`。它把“已经逐字重写并有测试证据”和“计划中”
分开，不能用 v4 文件存在来冒充 v3 能力已经覆盖。

当前里程碑：

- `903f36c` 冻结 v3 final；v4 不再向 v3 回填设计。
- core/view/func、图树、root kernel、线段树、动态树、流与容器结构已完成竞赛化重写。
- 2026-10-01 已完成 math、number、divisor、frac、bitmath、poly、recurrence、linear、
  string、automata、geom、opt、io、debug 的逐字重写；每块均有 `test-v4/*_property.cpp`
  独立证据，并针对相关模块跑过 C++20/C++23、优化、ASan/UBSan 或 64 位索引窄测。
- `mdview.hpp` 明确 abandoned；它不再占用 v4 的能力预算，也不制造空壳兼容文件。

---

## 0. 一句话目标

> **让普通题的关键逻辑直接留在代码里，让 Nitori 独有的语义仍能作为工程齿轮稳定复用。**

v4 不是生产库，也不是 STL 的替换层。它服务于稳定 OJ、CF/ICPC 风格题解和个人/小组
竞赛代码。默认假设是：题目给出有限的 stack/memory limit，递归深度、数组空间和模板
实例化成本都应当在题目约束下讨论；不为了模拟无限输入而牺牲竞赛代码的可读性。

---

## 1. 冻结边界与兼容观

### 1.1 v3 先冻结

当前正式 v3 工作面先提交为：

```text
v3 final
```

冻结内容包括当前已经验证的：

```text
src-v3/
test-v3/
bench-v3/
v3-Tutorial-Comprehensive.md
README.md
AGENTS.md
```

`design-lab/`、编辑器配置和其他未纳入 v3 正式 authority 的实验文件不参与 v3 final
提交；它们不阻塞 v4，也不被当作 v4 规范。

### 1.2 “不丢能力”具体指什么

v4 必须保留 v3 的有效对象、操作集合和数学/生命周期语义，包括：

```text
普通值结构、root kernel、持久化结构、图/树端口、数学、字符串、几何、调试与 I/O
```

但不承诺以下内容永远不变：

```text
内部字段名
内部 namespace
内部 descriptor 层次
公共数据布局
构造器参数的冗余顺序
只为 v3 实现方便存在的 holder/adapter
```

兼容优先级为：

```text
正确性与代数语义
> 复杂度与失效契约
> 稳定算法函数名
> 竞赛抄写/阅读成本
> v3 内部布局与偶然调用形式
```

### 1.3 稳定函数名不随便动

已有函数名如果已经准确表达算法操作，就保留，不因为短或模仿其他库而改名：

```text
fold, query, set, apply, walk, split, merge, join
max_right, min_left
lca, path, visit_path, children
parents, depths, subtree_sizes, positions, order
```

例如 `fold` 就是稳定词，不改成 `get`、`ask` 或 `range_value`。v4 的主要命名工作对象
是无效冗长的字段、局部阶段名、重复坐标名和纯实现层包装。

---

## 2. v4 的硬规则

### 2.1 竞赛优先，不生产化

禁止为了生产库式的抽象完整性引入：

```text
owner facade
resource registry
全局 trait/concept 注册表
生命周期协议对象
自动反图/自动纠错层
异常安全外壳
与题目无关的通用配置矩阵
```

代码应当优先回答：

```text
这段题解的关键状态在哪里？
哪个循环维护不变量？
运算顺序是什么？
调用者需要提供什么？
```

### 2.2 禁止 `xxx_detail` 架构层

v4 不建立 `nxxx_detail`、`xxx_detail` 这种 namespace 来藏实现。

一段逻辑只有三种归宿：

1. 它是可复用的竞赛机制：直接成为公开、短命名的核心对象/函数；
2. 它是某个对象不可分的局部步骤：直接写在对象成员函数或构造函数中；
3. 它只服务一次调用：使用 `standalone { ... }`、局部 lambda、递归 lambda 或局部
   结构体分割命名域，不建立长期名字。

不允许通过 `xxx_detail` 把“尚未决定接口的代码”伪装成架构。

允许使用 standalone block：

```cpp
{
    vector<int> par(n, -1), dep(n), ord;
    auto dfs = [&](auto&& self, int v, int p) -> void {
        // 局部不变量与局部 token
    };
    dfs(dfs, root, root);
}
```

它的作用是控制临时变量命名域、缩短题解可见范围和隔离阶段状态，而不是制造新的
库层级。

### 2.3 递归 Lambda 是默认表达方式

树 DFS、图 DFS、子树回溯、递归分治等默认写成递归 lambda：

```cpp
auto dfs = [&](auto&& self, nidx_t v) -> void {
    for (auto e : next(v)) {
        if (...) self(self, ...);
    }
};
```

只有在算法本身需要显式顺序、回溯栈就是状态、或递归形式确实不能表达时，才使用显式
stack。禁止仅仅因为“可能爆 stack”就把清晰的递归换成长的迭代模拟。

验证仍然要把题目约束、stack limit 和递归深度写清楚；但这属于竞赛边界分析，不是架构
上强迫所有实现迭代化的理由。

### 2.4 泛型只覆盖真实变化点

优先使用：

```cpp
std::invoke
requires requires(...)
std::ranges
std::views
std::span
std::identity
std::remove_cvref_t
std::invoke_result_t
```

泛型参数应该对应题目中真实可替换的东西：

```text
值类型、merge/action、比较器/投影、邻接访问、边目标/权值、坐标和值域
```

不为每个表达式建立 trait；不为每个算法建立 policy class；不把结合律、逆元、作用
分配律等数学事实伪装成“能调用即可”。

---

## 3. 核心分层

### 3.1 基础层

`core` 只保留真正跨模块的竞赛基础：

```text
nidx_t
nlen
nchmin / nchmax
最小的表达式约定
```

不在 core 里塞入所有数据结构的共同父类、owner、位置 token 或全局类型注册。

### 3.2 标准库层

普通遍历、排序、复制、填充、反转、变换优先直接用 STL：

```cpp
ranges::sort
ranges::copy
ranges::fill
ranges::reverse
views::iota
views::transform
views::reverse
```

已有的 `nfind_if`、`nall_of`、`ncopy` 等函数可以保留为兼容入口，但应成为薄适配，
不再各自发展一套平行算法框架。

### 3.3 Nitori 语义层

`nview` 和 `nfunc` 不删除，但全面空心化。

`nview` 只承担：

```text
有限位置投影
position -> value
可选 inverse
借用/拥有 accessor 的明确关系
```

`nfunc` 只承担：

```text
position -> key -> value
operator[] 的位置语义
operator() 的 key 语义
key/value 重新域化与 anchors
```

它们不再承担普通算法、所有权管理或万能数据结构接口。

### 3.4 位置计划层

保留 Nitori 真正有价值、而 STL view 不能等价替代的对象：

```text
nselect
norder
nfilter 的位置快照语义
nunique
nchunks / nblocks / nwindows
nruns
nargsort / permutation plans
```

这些对象的关键是“位置计划是一个可保存、可复制、可交给其他算法的结果”，不是一般
惰性 view。

### 3.5 数据结构层

数据结构按真实语义拆开，不建立万能结构：

```text
nseg
nlazyseg
nreftree
nfhq
nbag
nsparse_seg
nfenwick
ndsu / npotential_dsu / nrollback_dsu
```

普通结构优先竞赛化；lazy、persistent、destructive root 等特殊语义保持独立。

### 3.6 图和树层

图算法使用最小 callable 端口；树算法直接操作 dense position 数组。

树对象的字段应使用算法稳定 token：

```text
par dep sz heavy head pos at tin tout low
```

而不是：

```text
parent_position
depth_value
vertex_at_position
children_position
```

对象内部一次声明坐标系，字段名不重复编码坐标系。

---

## 4. nroot/nhld 的 v4 目标

函数名保持：

```text
nroot
nhld
nreroot
lca
path
visit_path
```

`nrooted` 的字段目标：

```cpp
V vertices;
vector<nidx_t> par, dep, comp, ord, sz;
    vector<nidx_t> rt, off, ch;
```

`nhld_layout` 的字段目标：

```cpp
V vertices;
vector<nidx_t> par, dep, sz, heavy;
    vector<nidx_t> head, pos, at, rt;
```

方法名保持工程稳定：

```text
keys, locate, positions, parents, depths, components,
subtree_sizes, order, roots, children, lca, visit_path, path
```

实现不再通过 `nhld_detail` 等 namespace 分阶段；采用：

```text
公开结构体
公开/稳定的短字段
成员函数中的 standalone block
递归 Lambda DFS
```

第一阶段只改变实现与字段组织，不改变 `nroot`、`nhld` 的算法含义、区间约定、路径方向
和非交换聚合顺序。

---

## 5. 模块迁移方针

| v3 区域 | v4 处理 | 原则 |
| --- | --- | --- |
| `core/io/debug` | 保留并压缩 | 不做生产化包装 |
| `view/func` | 保留语义，缩小机制 | 不当第二套 STL |
| `discrete/sequence/permutation` | STL 适配 + 位置计划 | 普通算法不重复造轮子 |
| `arena/fhq/reftree` | 保留根代数 | root 消费/共享必须显式 |
| `segment` | 普通、lazy、稀疏、Beats 分语义 | 不建万能 segment |
| `ds` | 一个结构一个竞赛核心 | 保留 `fold` 等稳定词 |
| `graph/graph_store` | callable 端口 + 独立存储 | 不统一 vertex/edge/position |
| `graph_algo/flow` | 算法直写关键循环 | 返回结果保留可复用证据 |
| `rooted/tree` | 先重写 | 清除 `xxx_detail` 与冗长字段 |
| `dynamic_tree/link_cut` | 独立专题 | 不为了统一而抽象路径结构 |
| `math/number/poly/linear` | 允许按数学内核重新合并、拆分和重排 | 不把 v3 文件边界当兼容目标；先消灭重复模运算/数论/多项式胶水，再固定公共入口 |
| `string/automata/geom/opt` | 独立竞赛条目 | 语义和边界优先于统一外观 |

“保留”表示能力不丢，不表示 v3 的文件边界、类型布局或调用方式必须原样复制。

---

## 6. 迁移顺序

### Phase 0：冻结 v3

完成 `v3 final` 提交。之后不再向 v3 偷塞 v4 设计。

### Phase 1：公共词汇与最小骨架

建立 v4 的：

```text
字段 token 表
坐标系说明
区间约定
递归/显式 stack 选择规则
standalone block 规则
标准 range 使用边界
```

### Phase 2：树图第一批

首先重写：

```text
nroot
nhld
nreroot
ngraph
```

理由：这批最能检验短字段、callable port、递归 Lambda、非生产化和工程齿轮是否能同时
成立。

### Phase 3：`nview/nfunc` 空心化

保留 `inverse`、key/value 分离、anchors 和位置计划；把普通 transform/filter/copy/
sort 退回 ranges/STL 或薄兼容入口。

### Phase 4：普通数据结构

按以下顺序：

```text
nseg
nfenwick
ndsu
nsparse_table
nqueue_agg / ndeque_agg
nlazyseg
```

`fold`、`set`、`query`、`max_right`、`min_left` 等已有稳定函数名不改。

### Phase 5：root kernel 与复杂结构

迁移：

```text
nfhq
nreftree
nbag
nsparse_seg
nlct
dynamic_tree
```

此阶段重点验证 destructive/persistent 交易和历史节点增长，不为统一接口制造 owner。

### Phase 6：数学、字符串、几何与工具

数学先于字符串、几何和工具。数学模块允许异常强的破坏性重组：以模运算、整数数论、
组合/生成函数、线性代数和多项式等真实内核重新安排文件，而不是逐个照抄 v3 文件。
公共函数名和能力要保留，内部 helper、文件边界和对象布局不保留；普通算术优先回到
`<numeric>`、ranges 和局部 lambda。`mdview.hpp` 明确放弃，不为不存在的独立语义保留
空壳文件。

---

## 7. 验证契约

每次 v4 迁移必须至少提供：

```text
旧 v3 能力对应表
关键不变量
复杂度
最小固定反例
独立随机 oracle（适用时）
实际编译/运行证据
```

不允许用“模板能实例化”代替数学契约，也不允许用“对拍没炸”代替复杂度证明。

测试顺序：

```text
受影响模块的固定测试
受影响模块的属性测试
必要时 sanitizer
必要时 index-width / compiler mode
```

继续遵守 v3 当前工作树的规则：不无目标地跑全量测试，不用全量审计伪装验证。

---

## 8. 第一票工作

v4 开工后的第一票不是增加新算法，而是清理第一组架构污染：

```text
1. 建立 v4 目录/文件边界，不改 design-lab；
2. 将 nroot/nhld 从 detail 分层改为公开短字段结构；
3. 用 standalone block 和递归 Lambda 重写构造逻辑；
4. 保留 nroot/nhld/lca/path/visit_path 等稳定函数名；
5. 迁移并运行对应的 rooted/HLD 属性测试；
6. 记录字段压缩前后的语义字节、抄写长度和编译结果；
7. 以这批结果校准其余模块，不先全库大爆破。
```

第一票验收标准：

```text
没有 xxx_detail namespace
没有因为防爆 stack 把清晰递归改成长显式栈
稳定函数名未被随意重命名
HLD 主循环能脱离工程包装直接读懂
工程方法仍能复用同一份短字段数据
相关固定/随机测试通过
```

---

## 9. 明确拒绝的 v4 方向

```text
把 ecnerwala 当成唯一规范
把所有 Nitori 对象都改成 range
把所有结构统一成一个 owner/handle 协议
为每个局部函数建立 xxx_detail
为了兼容无限深输入默认迭代化
把 fold/query/lca/path 等稳定词随意换名
用长名字重复表达 position/value/vertex 等坐标
为了源码体积删掉难以手写但实际有竞赛价值的数学/结构能力
```

v4 的破坏性应该破坏偶然复杂度，不应破坏已经被题解和证明使用的语义词汇。
