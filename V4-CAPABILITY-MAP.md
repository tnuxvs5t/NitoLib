# NitoriSTL v4 能力映射表

这张表是 v4 的验收账本，不是把 v3 文件名机械搬到 v4 的目录清单。每一行至少要
回答：v3 的对象/操作在 v4 放在哪里、是否保留其语义、用什么独立证据验收。`pending`
表示尚未开始逐字重写，不能解释成“已经兼容”。

## 已完成或正在工作的核心

| v3 能力 | v4 归宿 | 状态 | 语义/证据 |
| --- | --- | --- | --- |
| `nidx_t`, `nlen`, index-width gate | `src-v4/core.hpp` | done | signed position、`nidx_wider_v`；树和 view 编译覆盖 |
| `nview`, `nall`, `ntabulate`, `nrange`, `nsub` | `src-v4/view.hpp` | done | 借用/拥有边界、浅 const、半开位置域 |
| `nreverse`, `nproject`, `nmap`, `ngather`, `nzip` | `src-v4/view.hpp` | done | 返回类别、惰性访问、最短 zip、可传播 inverse |
| `nproduct` | `src-v4/view.hpp` | done | 左主序、最后一维最快、pair/tuple、inverse |
| `nfunc`, `nkeys`, `nvalues`, `nentries` | `src-v4/func.hpp` | done | position/key/value 分离，引用类别保持 |
| `nredomain`, `nmap_values`, `ncompose` | `src-v4/func.hpp` | done | 只改枚举域；value transform 不吞引用 |
| `nanchors` | `src-v4/func.hpp` | done | 显式 locator、结构 inverse、hash fallback、dense ordinal |
| `nhash`, `nhash_inverse`, `nmake_hash_inverse`, `ninvert` | `src-v4/hash.hpp` | done | 结构 hash、固定容量开放寻址、碰撞回归 |
| `ngraph`, `nvertices` | `src-v4/graph.hpp` | done | 最小 callable graph port |
| `nroot`, `nrooted` | `src-v4/rooted.hpp` | done | dense/custom key domain、root algebra、递归 lambda |
| `nhld`, `npath_piece`, path/LCA projections | `src-v4/tree.hpp` | done | `par dep sz heavy head pos at rt`；路径顺序和 key adapter |
| `nargsort`, `nindexed_span`, `nrun_bounds` | `src-v4/sequence.hpp` | done | 位置排序计划、borrowed gather span、run snapshot |
| `nselect`, `nslice`, stride/filter/unique/indexed | `src-v4/discrete.hpp` | done | 位置计划保持 alias；nfunc 保留 semantic keys |
| scans, write kernels, folds, predicates, bounds, order/sort | `src-v4/discrete.hpp` | done | 左到右调用顺序、源长度写入、半开 bounds |
| `nchunks`, `nblock`, `nblocks`, `nwindows`, `nruns` | `src-v4/discrete.hpp` | done | detached chunk 共享 descriptor；interval key 保持完整 |
| Fenwick, DSU, potential/rollback DSU | `src-v4/ds.hpp` | done | group/action order、势能差、rollback history |
| queue/deque aggregation, sparse table | `src-v4/ds.hpp` | done | 非交换顺序、重建边界、幂等 sparse query |
| `nrotate`, permutation rank/unrank | `src-v4/permutation.hpp` | done | mixed-radix rank；rank 类型必须覆盖 n! |
| append-only handle arena | `src-v4/arena.hpp` | done | handle survives vector relocation；无 generation/owner 层 |
| `nfhq`, destructive root algebra | `src-v4/fhq.hpp` | done | 多根 split/merge、lazy policy port、kth/rank/sequence |
| `nsegment_trace`, `nsegment_cover`, `nseg` | `src-v4/segment.hpp` | done | 纯拓扑访问、ordered fold、max_right/min_left、pointwise |
| `nlazyseg`, `nlazy_ops`, `nlazy_addsum` | `src-v4/segment.hpp` | done | action composition、push/pull、range apply/query |
| `nsparse_seg` | `src-v4/segment.hpp` | done | destructive/persistent roots、长坐标、query 不分配 |
| `ngraph` edge identity port | `src-v4/graph.hpp` | done | vertices/next/target/edge_id 最小 callable port |
| `ncsr`, undirected CSR expansion | `src-v4/graph_store.hpp` | done | input order、logical edge ID、borrowed view |
| shortest paths, topo, Euler, SCC | `src-v4/graph_algo.hpp` | done | graph port independent；signed/0-1/heap contracts |
| lowlink and block-cut forest | `src-v4/graph_algo.hpp` | done | multiedge-safe edge IDs、recursive DFS |
| flow, matching and minimum spanning forest | `src-v4/flow.hpp` | done | 增量残量流与 cut、一次物化邻接的分层匹配、只保存输入位置的 Kruskal；`test-v4/flow_property.cpp` 覆盖随机最小割、暴力匹配、非连通森林和 move-only 边 |
| pooled chains and ordered multisets | `src-v4/list.hpp`, `bag.hpp`, `vec_bag.hpp` | done | `nlist` 保持 handle/root destructive 链操作；`nbag` 保持 FHQ handle 与只读 query；`nvec_bag` 保持位置语义；`test-v4/list_property.cpp`、`bag_property.cpp` 覆盖回收、跨链搬段、代理值、投影 bound 与 move-only 值 |
| persistent repeated-pattern tree | `src-v4/reftree.hpp` | done | 逻辑重复、aligned `block/paste/set`、无分配 `leaf_at/find_first/kth`、历史根与非交换 Info；`test-v4/reftree_property.cpp` 覆盖穷举位图、随机历史版本、字符串顺序和 64 位地址 |
| wavelet matrix and fixed top-k summaries | `src-v4/wavelet.hpp`, `topk.hpp` | done | rank-compressed static sequence queries；ordinary/keyed top-k retain ordered evidence and monoid semantics；`test-v4/wavelet_property.cpp`、`topk_property.cpp` 覆盖空域、重复值、代理外的泛型值和结合性 |

独立证据：

```text
test-v4/view_func_property.cpp
test-v4/hash_property.cpp
test-v4/tree_property.cpp
test-v4/key_tree_property.cpp
test-v4/flow_property.cpp
test-v4/list_property.cpp
test-v4/bag_property.cpp
test-v4/reftree_property.cpp
test-v4/wavelet_property.cpp
test-v4/topk_property.cpp
```

这些测试已在 C++20/C++23、`-O2` 下通过；树核心另有 ASan/UBSan 与 64-bit index
验证记录。测试只覆盖当前变更相关的能力，不代表 v4 全量完成。

## 尚未逐字重写的 v3 能力

| v3 头文件 | 主要对象/操作 | v4 归宿计划 | 状态 |
| --- | --- | --- | --- |
| `io.hpp` | `nread`, `nwrite`, `nscan`, `nprint` | `io.hpp` | pending |
| `debug.hpp` | `ndebug`, `nwrite` debug path | `debug.hpp` | pending |
| `discrete.hpp` | `nselect`, `nfilter`, `norder`, `nunique`, chunks/runs | `discrete.hpp` | done |
| `sequence.hpp` | `nrun_bounds`, `nindexed_span` | `sequence.hpp` | done |
| `permutation.hpp` | `nargsort`, `nrotate`, rank/unrank | `permutation.hpp` | done |
| `mdview.hpp` | `nmdview` | `mdview.hpp` | pending |
| `segment.hpp` | `nseg`, lazy/sparse segment trees, trace/cover | `segment.hpp` | done |
| `ds.hpp` | Fenwick/DSU/queue/deque/sparse table | `ds.hpp` | done |
| `arena.hpp` | `narena` | `arena.hpp` | done |
| `fhq.hpp` | `nfhq` | `fhq.hpp` | done |
| `reftree.hpp` | `nreftree` | `reftree.hpp` | done |
| `bag.hpp` | `nbag` | `bag.hpp` | done |
| `vec_bag.hpp` | `nvec_bag` | `vec_bag.hpp` | done |
| `list.hpp` | `nlist` | `list.hpp` | done |
| `wavelet.hpp` | `nwavelet` | `wavelet.hpp` | done |
| `topk.hpp` | `ntopk` | `topk.hpp` | done |
| `graph_store.hpp` | `ncsr`, graph views | `graph_store.hpp` | done |
| `graph_algo.hpp` | BFS/DFS, SCC, lowlink, Euler, shortest paths | `graph_algo.hpp` | done |
| `flow.hpp` | Dinic, matching, MST | `flow.hpp` | done |
| `dynamic_tree.hpp` | Euler-tour forest | `dynamic_tree.hpp` | pending |
| `link_cut.hpp` | `nlct` | `link_cut.hpp` | pending |
| `math.hpp` | modular arithmetic, CRT, floor sum, sieve | `math.hpp` | pending |
| `number.hpp` | primality/factorization/mod64 | `number.hpp` | pending |
| `divisor.hpp` | divisor/multiple zeta and factor lists | `divisor.hpp` | pending |
| `frac.hpp` | `nfrac` | `frac.hpp` | pending |
| `bitmath.hpp` | xor basis, bit transforms | `bitmath.hpp` | pending |
| `poly.hpp` | NTT/convolution/poly operations | `poly.hpp` | pending |
| `recurrence.hpp` | BM and linear recurrence | `recurrence.hpp` | pending |
| `linear.hpp` | matrix, GF(2), linear solve | `linear.hpp` | pending |
| `string.hpp` | KMP, suffix array, Manacher, LCP | `string.hpp` | pending |
| `automata.hpp` | `nac` | `automata.hpp` | pending |
| `geom.hpp` | point/line/intersection/hull | `geom.hpp` | pending |
| `opt.hpp` | Li Chao | `opt.hpp` | pending |

## 每个 pending 条目的完成条件

1. 先在 v3 中列出有效对象、函数、代数/复杂度和失效条件，不以实现细节代替契约。
2. 在 v4 中逐字重写，不复制 v3 文件，再决定是否需要新的短 helper；禁止 `xxx_detail`。
3. 普通遍历、排序、复制、变换优先用 ranges/views；只有位置计划、root algebra、
   graph port 等 Nitori 语义保留自有对象。
4. 为危险边界添加固定断言和独立随机/property test；只运行与本次变更有关的测试。
5. 在此表中补上复杂度、lifetime/invalidation 和实际测试命令后，才能把状态改为 done。
