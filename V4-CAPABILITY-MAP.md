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

独立证据：

```text
test-v4/view_func_property.cpp
test-v4/hash_property.cpp
test-v4/tree_property.cpp
test-v4/key_tree_property.cpp
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
| `permutation.hpp` | `nargsort`, `nrotate`, rank/unrank | `permutation.hpp` | pending |
| `mdview.hpp` | `nmdview` | `mdview.hpp` | pending |
| `segment.hpp` | `nseg`, lazy/sparse segment trees, trace/cover | `segment.hpp` | pending |
| `ds.hpp` | Fenwick/DSU/queue/deque/sparse table | `ds.hpp` | pending |
| `arena.hpp` | `narena` | `arena.hpp` | pending |
| `fhq.hpp` | `nfhq` | `fhq.hpp` | pending |
| `reftree.hpp` | `nreftree` | `reftree.hpp` | pending |
| `bag.hpp` | `nbag` | `bag.hpp` | pending |
| `vec_bag.hpp` | `nvec_bag` | `vec_bag.hpp` | pending |
| `list.hpp` | `nlist` | `list.hpp` | pending |
| `wavelet.hpp` | `nwavelet` | `wavelet.hpp` | pending |
| `topk.hpp` | `ntopk` | `topk.hpp` | pending |
| `graph_store.hpp` | `ncsr`, graph views | `graph_store.hpp` | pending |
| `graph_algo.hpp` | BFS/DFS, SCC, lowlink, Euler, shortest paths | `graph_algo.hpp` | pending |
| `flow.hpp` | Dinic, matching, MST | `flow.hpp` | pending |
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
