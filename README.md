# NitoriSTL

**Nitori V4 · 面向算法竞赛的 C++23 头文件库**

> 把算法留在眼前，把重复交给齿轮。

Nitori 提供从序列投影、区间结构到图论、数学与字符串的可组合组件。
它不替代 STL，也不把题解藏进框架：普通代码直接写，真正需要复用的结构再交给库。

[快速开始](#快速开始) · [设计取向](#设计取向) · [模块一览](#模块一览) · [阅读与使用](#阅读与使用)

## 快速开始

使用支持 C++23 的 GCC，将仓库根目录加入头文件搜索路径。
无需构建或链接独立的 Nitori 库。

```cpp
#include "nv4"

int main() {
    vector<long long> a{3, 1, 4, 1, 5};
    nseg<long long> sum(a);

    cout << sum.fold(1, 4) << '\n'; // [1, 4)：1 + 4 + 1 = 6
    sum.set(2, 10);
    cout << sum.fold() << '\n';     // 整段之和：20
}
```

```bash
g++ -std=c++23 -O2 -I/path/to/NitoriSTL main.cpp -o main
./main
```

[`nv4`](nv4) 是无扩展名的总入口，引用当前全部 V4 模块。
它不是展开后的独立单文件，使用时需保留 `src-v4/` 目录。
也可以只包含本题需要的头文件：

```cpp
#include "src-v4/segment.hpp"
#include "src-v4/tree.hpp"
```

使用 VS Code CPH 时，将 `-I/path/to/NitoriSTL` 加入 `cph.language.cpp.Args`；
仅配置编辑器补全路径，不会改变 CPH 的编译参数。

## 设计取向

### 普通代码保持普通

`vector`、`span`、STL 算法和局部 lambda 都是正常入口。
不为调用一个算法先建立一套容器包装，也不把普通排序改造成另一种语言。
公开接口使用全局 `n*` 名字，优先短而可检查的竞赛代码。

### 区分位置、键与值

`nview` 表达有限的位置投影；`nfunc` 区分枚举位置、语义键和计算结果。
切片、重排、组合与重新域化可以在这些明确的坐标关系上复用，
而不是把不同含义的整数混成同一种下标。

### 复用机制，而不是堆叠外壳

区间结构接收合并与作用规则，图算法接收邻接与边的访问端口。
根内核支持多根操作，但破坏性合并与持久化共享保持各自的语义。
你可以替换存储、运算或访问方式，不必把整道题交给一个万能对象。

### 关键约定留在代码旁边

运算顺序、借用关系、根的消耗和合法输入前提写在对应实现旁。
简短不是省略条件：有序折叠不假设交换律，懒标记的复合顺序也不由调用者猜测。

## 模块一览

| 方向 | 提供的能力 | 主要入口 |
| --- | --- | --- |
| 序列与映射 | 位置投影、键值函数、结构逆映射、重排计划、分块与窗口 | [`view.hpp`](src-v4/view.hpp) · [`func.hpp`](src-v4/func.hpp) · [`discrete.hpp`](src-v4/discrete.hpp) |
| 基础数据结构 | Fenwick、普通 / 势能 / 可回滚 DSU、队列聚合、稀疏表 | [`ds.hpp`](src-v4/ds.hpp) |
| 区间查询 | 线段树、懒标记、稀疏与持久化根、Wavelet Matrix、Top-K 摘要 | [`segment.hpp`](src-v4/segment.hpp) · [`wavelet.hpp`](src-v4/wavelet.hpp) · [`topk.hpp`](src-v4/topk.hpp) |
| 根与容器 | 节点池、FHQ、多链操作、有序多重集、持久化重复模式树 | [`arena.hpp`](src-v4/arena.hpp) · [`fhq.hpp`](src-v4/fhq.hpp) · [`list.hpp`](src-v4/list.hpp) · [`bag.hpp`](src-v4/bag.hpp) · [`reftree.hpp`](src-v4/reftree.hpp) |
| 图与静态树 | BFS、最短路、拓扑序、SCC、lowlink、Euler、HLD、LCA、换根 DP | [`graph_algo.hpp`](src-v4/graph_algo.hpp) · [`tree.hpp`](src-v4/tree.hpp) |
| 流与匹配 | 最大流与最小割、二分图匹配、最小生成森林 | [`flow.hpp`](src-v4/flow.hpp) |
| 动态森林 | Euler Tour Tree、Link-Cut Tree、连通块与有序路径聚合 | [`dynamic_tree.hpp`](src-v4/dynamic_tree.hpp) · [`link_cut.hpp`](src-v4/link_cut.hpp) |
| 数学 | 模运算、CRT、素性与分解、分式、线性基、NTT、多项式、递推与线性代数 | [`math.hpp`](src-v4/math.hpp) · [`number.hpp`](src-v4/number.hpp) · [`poly.hpp`](src-v4/poly.hpp) · [`linear.hpp`](src-v4/linear.hpp) |
| 字符串 | KMP、后缀数组、LCP、Manacher、AC 自动机 | [`string.hpp`](src-v4/string.hpp) · [`automata.hpp`](src-v4/automata.hpp) |
| 几何与优化 | 点与直线、相交判定、凸包、面积、Li Chao Tree | [`geom.hpp`](src-v4/geom.hpp) · [`opt.hpp`](src-v4/opt.hpp) |
| 输入与调试 | 含 128 位整数的读写、递归容器与视图调试输出 | [`io.hpp`](src-v4/io.hpp) · [`debug.hpp`](src-v4/debug.hpp) |

完整源码见 [`src-v4/`](src-v4/)。具体前提与接口以对应头文件为准，
迁移状态和已有验证证据见 [V4 能力映射表](V4-CAPABILITY-MAP.md)。

## 使用约定

- **位置与区间**：默认使用有符号 `nidx_t = int`；位置区间采用半开 `[left, right)`。
- **64 位索引**：需要时，在首次引用前定义 `NITORI_INDEX_64`；所有翻译单元必须一致。
  它只改变索引类型，数值计算的位宽仍需自行选择。
- **借用与根**：借用视图不延长原对象生命周期；破坏性根操作的输入不能继续作为独立旧根使用。
  持久化操作另按对应内核的共享约定使用。
- **工具链与提交**：实现使用 GCC 的 `<bits/stdc++.h>`，部分模块使用 `__int128`。
  本地总入口不等于 OJ 可提交单文件；提交前需要按平台要求整理依赖。

## 阅读与使用

| 想做什么 | 从这里开始 |
| --- | --- |
| 在本地写题 | [`nv4`](nv4) 或所需的 [`src-v4/`](src-v4/) 模块 |
| 确认接口、前提和运算顺序 | 对应头文件的公开接口与局部契约 |
| 找调用例子与独立验证 | [`test-v4/`](test-v4/) 中的相关 property test |
| 查看能力迁移与验证记录 | [V4 能力映射表](V4-CAPABILITY-MAP.md) |
| 了解设计与版本边界 | [V4 总规划](V4-PLAN.md) |

**当前主线是 V4，仍在进行接口收口。** V3 保留在 [`src-v3/`](src-v3/)，
其教程只对应 V3，不是 V4 的 API 文档；不要在同一翻译单元混用两个版本。

验证按改动选择相关模块与独立 oracle，不以全量测试或未测路径替代正确性说明。
修改贡献约定见 [`AGENTS.md`](AGENTS.md)。

## 许可证

仓库目前未附独立许可证；复用与分发的授权范围请向作者确认。
