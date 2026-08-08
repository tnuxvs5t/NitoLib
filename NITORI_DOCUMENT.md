# Nitori X

> 从第一份可提交程序，到能从题目约束反推出合适工具的唯一权威文档
>
> 适用版本：`nversion == 30000`（Nitori v3）
>
> checked 头文件：`/home/tnuzy/NitoriSTL/Nitori.h`
>
> unsafe 头文件：`/home/tnuzy/NitoriSTL/Nitori_unsafe.h`

Nitori X 的目标不是让你记住更多以 `n` 开头的名字，而是让你在下一道题中更快完成这
条链：

```text
题目约束 → 暴力瓶颈 → 关键不变量 → 合适的抽象 → 可提交代码 → 反例与复盘
```

这份文档同时是教程、题型装配手册和 API 参考。三者会出现同一个组件，但承担的任务
不同：教程回答“为什么需要”，装配手册回答“什么时候选择”，参考卡回答“精确怎样调用”。
为了不让不同读者互相挡路，正文会明确标出学习路径、题型路径和维护路径。

Nitori X 是面向算法竞赛的 GNU C++20 单头文件系统。它允许同一算法作用于 owner、
矩阵行列、步长序列、lambda 映射位置和离散函数，同时保留真实引用、明确复杂度和可检查
的生命周期。它不是给 STL 名字机械加 `n`，也不是要求选手先学习一套工程框架。

本文是 Nitori X 唯一的用户文档。公共签名与真实运行行为以
`/home/tnuzy/NitoriSTL/Nitori.h` 为事实来源；本文件负责解释语义、前提、复杂度、
配方和维护流程。若二者不一致，停止扩散，先以实现和测试重建结论，再修正文档。

---

## 怎样阅读本文

### A. 第一次学习：沿着主线走

```text
第一份程序
→ [l,r) 与 int 下标
→ owner/view 生命周期
→ nview 与 projection
→ 从暴力选择区间数据结构
→ 图、数学、字符串、几何的题型入口
→ 调试、对拍与提交
```

第 1～5 章是逐步学习，不需要先读 concept、trait 或“代数系统”。遇到不熟的符号，先
看本章的最小例子，再跳到对应契约卡；不要从第 21 章开始背索引。

### B. 正在做题：先找题目阻碍

| 题目中看到的阻碍 | 推荐入口 |
|---|---|
| 区间查询但没有修改 | 第 9.1、10.8、10.10 节 |
| 单点修改 + 区间查询 | 第 10.1～10.2 节 |
| 区间修改或复杂聚合 | 第 10.3 节 |
| 坐标范围巨大且只改少数位置 | 第 10.7 节 |
| 需要访问旧答案 | 第 10.6 节 |
| 有序集合还要维护子树信息 | 第 9.12 节的 AST 树 |
| 序列需要按位置切分、区间 tag、翻转或搬运 | 第 9.13 节的契约与第 17.19 节的完整配方 |
| BFS、最短路、树、流、匹配 | 第 11 章 |
| gcd、模运算、组合、博弈 | 第 12 章 |
| 矩阵、线性方程、卷积、多项式 | 第 13 章 |
| KMP、后缀数组、Trie、AC 自动机 | 第 14 章 |
| 几何、Li Chao、单峰优化 | 第 15 章 |
| 已知多个组件，想看完整装配 | 第 17 章 |

### C. 只查接口：直接看契约卡

每个非平凡组件都尽量使用同一个顺序：

```text
解决什么问题 → 识别信号 → 最小接口 → 隐形性质 → 不变量
→ 复杂度 → 失败边界 → 最小反例 → 相关组件
```

参考卡可以压缩，学习正文不能删掉“为什么”。如果你已经会算法，只需读契约卡和反例。

### D. 开发 Nitori：最后再读维护路径

第 3、5、9、18、19、20、21 章主要服务于 profile、局部契约、AST、测试、扩展、迁移
和公共符号审计。它们不是第一次做题的前置课程。

### 本文的教学约定

- **先具体，后抽象**：先看小题目和手算，再给模板参数。
- **一次只引入一个主要机关**：不要用一个例子同时教学 view、projection 和 DP。
- **性质必须解释用途**：不只说“需要结合”，要说不结合时哪一步会错。
- **每章都留迁移信号**：最后回答“下次看到什么，可以想到这个方法？”
- **篇幅服务理解**：允许重复，但同一事实有一个权威契约位置，重复部分只承担动机、
  例子或反例中的一种职责。

---

## 目录

### Part I：从第一份程序到核心模型

1. [第一份可提交程序](#1-第一份可提交程序)
2. [不可违反的全局约定](#2-不可违反的全局约定)
3. [checked 与 unsafe](#3-checked-与-unsafe)
4. [基础值、哨兵和可选结果](#4-基础值哨兵和可选结果)
5. [操作对象与隐形契约](#5-操作对象与隐形契约)

### Part II：从问题阻碍到数据结构

6. [统一 Range/View/Projection：先学轻量位置，再学语义函数](#6-统一-rangeviewprojection先学轻量位置再学语义函数)
7. [枚举协议与组合视图](#7-枚举协议与组合视图)
8. [拥有型序列与通用算法](#8-拥有型序列与通用算法)
9. [机制、内存、关联对象与 AST](#9-机制内存关联对象与-ast)
10. [可合并区间信息、历史版本与离线算法](#10-可合并区间信息历史版本与离线算法)

### Part III：图论与竞赛数学工具箱

11. [图、树、流与匹配](#11-图树流与匹配)
12. [整数、模运算、组合与博弈](#12-整数模运算组合与博弈)
13. [矩阵、线性代数与多项式](#13-矩阵线性代数与多项式)
14. [字符串、Trie 与 AC 自动机](#14-字符串trie-与-ac-自动机)
15. [几何与优化](#15-几何与优化)
16. [竞赛 I/O](#16-竞赛-io)

### Part IV：题型装配与迁移

17. [题型装配配方](#17-题型装配配方)
18. [调试、测试与提交工作流](#18-调试测试与提交工作流)
19. [扩展 Nitori 的规则](#19-扩展-nitori-的规则)
20. [旧版到 Nitori X 的迁移桥梁](#20-旧版到-nitori-x-的迁移桥梁)
21. [完整公共符号索引](#21-完整公共符号索引)

---

## 1. 第一份可提交程序

本章只建立最小工作台：包含头文件、读入、拥有序列、循环、排序和输出。目标不是一次
学完 Nitori，而是先让程序跑起来，再逐层增加抽象。`nview`、projection、区间结构和
unsafe 都故意放到后面；第一次阅读时不要跳过本章的检查点。

### 1.1 准备头文件

源代码始终写：

```cpp
#include "Nitori.h"
```

训练和调试时使用 checked 头。可以把
`/home/tnuzy/NitoriSTL/Nitori.h` 放在源码旁边，也可以保留权威文件并添加 include
路径：

```bash
g++ -std=gnu++20 -O2 -Wall -Wextra \
    -I/home/tnuzy/NitoriSTL solution.cpp
```

最小环境自检：

```cpp
#include "Nitori.h"

static_assert(nversion == 30000);
static_assert(!nunsafe); // 训练阶段应该成立

int main() {
    nprintln("Nitori ready");
}
```

如果 `nmatrix`、`nsub`、`nassign` 等名字突然“未声明”，先检查实际包含了哪个
`Nitori.h`。`#include "Nitori.h"` 会优先搜索源码所在目录；附近残留的 v1/旧副本会
覆盖 `-I` 路径。保留上面的 `nversion` 自检，或者用编译器的 include trace 定位，
不要在错误头文件上继续猜 API。

不要在题解里写机器相关的绝对 include 路径。最终提交使用 `deploynitori` 生成一个
自包含源文件：

```bash
deploynitori --checked solution.cpp -o submit.cpp
```

只有 checked 版本已经通过样例、边界和对拍后，才考虑：

```bash
deploynitori --unsafe solution.cpp -o submit.cpp
```

`deploynitori --unsafe` 会先编译 checked bundle，再生成并独立编译 unsafe bundle。

### 1.2 第一份完整程序

下面的程序读入整数序列，排序、去重并输出：

```cpp
#include "Nitori.h"

int main() {
    int n;
    nin >> n;

    nvector<int> a(n);
    nrep(i, n)
        nin >> a[i];

    nsort(a);
    nunique(a);

    nprintln(a.len());
    nrep(i, a.len()) {
        if (i)
            nout.write(' ');
        nout << a[i];
    }
    nout.write('\n');
}
```

输入：

```text
7
4 1 4 2 1 9 2
```

输出：

```text
4
1 2 4 9
```

这份程序已经覆盖最常用的六个入口：

| 写法 | 含义 |
|---|---|
| `nin >> x` | 快速输入 |
| `nvector<T>` | 拥有型动态数组 |
| `a.len()` | 返回 `int` 长度 |
| `nrep(i,n)` | 枚举 `0..n-1` |
| `nsort(a)` | 原地排序 |
| `nprintln(...)`、`nout` | 快速输出 |

### 1.3 从 STL 迁移时先记这张表

| 常见 STL 写法 | Nitori 写法 | 备注 |
|---|---|---|
| `vector<T>` | `nvector<T>` | owner，真正拥有元素 |
| `a.size()` | `a.len()` | 返回 `int`，便于竞赛下标 |
| `a.push_back(x)` | `a.push(x)` | 返回新元素引用 |
| `a.pop_back()` | `a.pop()` | 返回被取走的值 |
| `sort(a.begin(),a.end())` | `nsort(a)` | 同样适用于非连续 view/deque |
| `reverse(...)` | `nreverse_inplace(range)` | `nreverse(range)` 本身是倒序 view |
| `fill(...)` | `nfill(range,value)` | 接受矩阵行、切片、lambda view |
| `copy(source,destination)` | `nassign(destination,source)` | 目标在前，要求等长 |
| `find(...)` | `nfind` / `nfind_if` | 返回 `int` 位置，失败为 `npos` |
| `lower_bound(...)` | `nlower(a,x)` | 返回插入位置 |
| range-for | `nfor(x,a)` | 保留引用类别 |
| `for(int i=0;i<n;++i)` | `nrep(i,n)` | 普通单层循环语义 |

不要把 `.data()`、指针加法和 STL iterator 当作默认逃生通道。先检查算法是否已经接受
range；若没有，优先补正确的最小能力接口，而不是把 view 拆回裸指针。

### 1.4 输入、输出和三种循环

```cpp
int n;
long long limit;
nin >> n >> limit;

nvector<long long> a(n);
nrep(i, n)
    nin >> a[i];

nfor(value, a)
    value *= 2;                  // value 通常是元素引用

nfori(index, value, a)
    nchmax(value, 1LL * index);  // 同时取得枚举位置

nprintln(n, limit);
```

倒序循环：

```cpp
nrrep(i, n) {
    // i = n-1, n-2, ..., 0
}
```

`nfor`、`nfori`、`nforkv`、`nrep`、`nrrep` 都只有一层真实 `for`。因此：

- `break` 退出整个当前宏循环；
- `continue` 进入下一项；
- range/count 只求值一次；
- 可以正常嵌套，但库不会暗中再套一层循环。

`nforkv(key,value,object)` 用于 map 和离散函数，初学时可以等需要键值枚举再学。

### 1.5 第一个容器工具箱

#### `nvector<T>`

```cpp
nvector<int> a;          // 空
nvector<int> b(10);      // 10 个值初始化元素
nvector<int> c(10, -1);  // 10 个 -1
nvector<int> d{3, 1, 4};

a.reserve(100);
a.push(7);
a.push(9);
int last = a.pop();

a.front();
a.back();
a.resize(20, 0);
a.clear();
```

#### `ndeque<T>`

```cpp
ndeque<int> q;
q.pushl(1);
q.pushr(2);
int left = q.popl();
int right = q.popr();
```

`ndeque` 支持索引但不保证连续；`nsort(q)` 仍然可用。

#### `nheap<T>`

```cpp
nheap<int> heap; // 默认最小堆
heap.push(5);
heap.push(2);
heap.push(8);

int minimum = heap.top();
int removed = heap.pop();
```

先用这三个就能覆盖大量入门题。set、map、图和数据结构按题目需要再查对应章节。

### 1.6 第一组通用算法

```cpp
nvector<int> a{4, 1, 3, 1, 5};

nfill(nsub(a, 1, 3), 0);                 // a = {4,0,0,1,5}
int first_zero = nfind(a, 0);             // 1
bool has_five = ncontains(a, 5);          // true
int zeros = ncount(a, 0);                 // 2
int positive = ncount_if(a, [](int x) { return x > 0; });
bool all_small = nall_of(a, [](int x) { return x < 10; });
int maximum_at = nargmax(a);              // 4

nsort(a);
int first_at_least_three = nlower(a, 3);
```

复制或投影写入统一使用 `nassign`：

```cpp
nvector<int> source{1, 2, 3};
nvector<long long> destination(3);

nassign(destination, source, [](int x) {
    return 1LL * x * x;
});
// destination == {1,4,9}
```

`nassign` 不暗中分配，也不提供隐藏的重叠快照。需要保留 source 原值时先显式
`ncollect(source)`。完整契约见 8.5。

### 1.7 第一个 view：只改变访问位置

现在只增加一个概念：**view 不拥有元素，它只描述“第 `i` 项去哪里取”。**

```cpp
nvector<int> a{9, 4, 7, 1, 8};
auto middle = nsub(a, 1, 4); // 借用 a 的 [1,4)
nsort(middle);                // 原地写回 a
// a == {9, 1, 4, 7, 8}
```

先停一下：如果希望得到独立数组，不能写 `auto copy = middle`。那只会复制访问描述，
必须明确写：

```cpp
auto copy = ncollect(middle); // O(length)，现在 copy 拥有自己的元素
```

完整的 view 组合器和生命周期规则在第 6 章；第一次这里只记住 owner/view 这组区分。

### 1.8 先分清两种 projection

```cpp
struct item { int key, payload; };
nvector<item> items{{3,30}, {1,10}, {2,20}};

nsort(items, nless<>{}, &item::key); // 按 key 比较，交换完整 item
```

而 `nproject(items, &item::key)` 是“只暴露 key 字段的可写 view”。两者都叫 projection，
但前者改变算法的观察方式，后者改变算法实际访问的位置。详细对照见第 6 章。

### 1.9 训练阶段的最小闭环

```text
checked 编译
→ 样例
→ 空、单点、极值边界
→ 小规模暴力对拍
→ 最后才生成 unsafe 提交
```

checked/unsafe 的完整差异和 `npre` 的含义见第 3 章。unsafe 不会修复错误；它只把调用者
已经证明的前提交给优化器。

### 1.10 本章检查点

在继续学习前，应该能用自己的话回答：

1. `nvector` 和 `nsub(a,l,r)` 谁拥有元素？
2. 为什么 `ncollect` 是显式操作，而不是 view 的默认行为？
3. `nsort(items, ..., &item::key)` 和 `nsort(nproject(items, &item::key))` 哪个会
   保持 `key/payload` 的配对？
4. 为什么 `[l,r)` 比闭区间更适合组合？
5. 一个 checked 程序在什么证据之后才适合切换 unsafe？

答不出时不要继续背容器表；回到对应例子手算一次。第 6 章会把这里的直觉正式化，
第 17 章会展示这些部件如何在真实题型中装配。

---

## 2. 不可违反的全局约定

### 2.1 索引和区间

- 长度、位置、顶点、版本号统一使用 `int`。
- 不存在的位置使用 `npos == -1`。
- 区间统一使用半开区间 `[l,r)`。
- 合法空区间满足 `l == r`。
- 越界不是可恢复查询，而是前置条件错误。

半开区间不是书写偏好，它让三个高频推理直接成立：

```text
长度              = r - l
[l,m) 与 [m,r)    无重无漏拼成 [l,r)
空区间            = [x,x)
```

因此线段树分裂、前缀和相减、view 切片和 DP 边界可以共享同一套公式。若题面给闭区间
`[L,R]`，只在输入边界转换一次为 `[L,R+1)`；先检查 `R+1` 的表示范围，不要让两种约定
在算法内部混用。

长度和位置使用 `int` 是竞赛接口约定，不表示所有数值都应使用 `int`。权值、路径和、
坐标差和乘积仍要根据范围选择 `long long` 或 `__int128_t`。

### 2.2 所有权

先把对象分成两类：

| 类型 | 拥有什么 | 复制意味着什么 |
|---|---|---|
| owner，如 `nvector/nmatrix` | 元素和存储 | 复制元素或结构 |
| view，如 `nsub/ncolumn/nproject` | 访问规则 | 复制描述，仍别名 owner |

只要一个算法返回或保存 view，就必须能指出最底层 owner 是谁、会活到什么时候、期间会不
会扩容或改变索引拓扑。下面的局部使用是稳定的：

```cpp
nvector<int> a{4,3,2,1};
auto middle = nsub(a, 1, 3);
nsort(middle); // a 仍存活，也没有结构修改
```

若 view 必须跨越 owner 的扩容、销毁或未知调用边界，先 `ncollect` 形成独立 owner。

- `nvector`、`ndeque`、`narray`、`nmatrix`、`nmap`、`nset` 等拥有存储。
- `nview` 是唯一公共引用视图类型；`nall`、`nsub`、`nstride`、`nproject`、`nzip`、
  `nproduct`、`nwindows`、`nrow`、`ncolumn`、`ndiagonal` 都返回某个 `nview` 实例。
- `nfunc_value`、`nfunc_ref`、`nfunc_bind`、`nredomain`、`nrestrict`、
  `nselect_positions`、`ncompose` 等离散函数适配器对左值参数借用、对右值参数接管
  所有权；holder 的 `const` 会一致传播到被拥有或被借用对象。evaluator 自己捕获的
  引用仍由用户负责。
- 组合器对普通左值 owner 保存引用；`nview` 描述符按值复制，对其他左值 range 描述符
  借用，对安全的右值 range 描述符接管所有权。中间 view 因此可以嵌套，但它仍不延长
  最底层借用 owner 的生命周期。
- owner 发生扩容、销毁或破坏索引拓扑的修改后，旧 view 可能失效。
- 会立即悬垂的临时 owning container 被能力约束拒绝，例如 `nall(nvector<int>{...})`；
  临时 view 则可以安全进入下一层组合器。

### 2.3 命名

- 公共全局名字以小写 `n` 开头。
- 内部实现位于 `ni`，用户不得依赖。
- 成员已由对象作用域隔离，不重复加 `n`：`a.len()`、`f.fold(l,r)`、
  `g.add(u,v)`。

### 2.4 数学前提

C++ 的语法约束只能验证接口形状，不能证明结合律、单位元、交换律、可逆性、单调性、
树结构或非负边权。调用者必须保证真实数学条件成立。

### 2.5 整数与溢出

- 几何整数积和直线函数求值通过 `nwide_t<T>` 扩为 `__int128_t`。
- `nlcm`、输入解析和若干尺寸计算检查表示范围。
- 普通用户表达式和权值相加不会自动无限扩宽；选对 `long long`/`__int128_t`。
- `ninf<T>` 是算法哨兵，不是数值类型的真正无穷。

---

## 3. checked 与 unsafe

两个 profile 不是“慢版本”和“快版本”两套算法。它们在合法输入上必须有相同语义；区别
只在于前置条件失败时，checked 给出靠近故障点的诊断，unsafe 允许优化器假设失败永远
不会发生。

因此切换 unsafe 的证据顺序应当是：

```text
数学前提已证明
→ checked 编译和样例通过
→ 边界/反例通过
→ 复杂结构完成小规模对拍
→ 再独立编译 unsafe
```

如果一份代码只有换成 unsafe 才“能跑”，那通常不是优化成功，而是未定义行为被隐藏。

| 项目 | checked | unsafe |
|---|---|---|
| 文件 | `Nitori.h` | `Nitori_unsafe.h` |
| `nunsafe` | `false` | `true` |
| `npre(false)` | 打印表达式、文件、行号后 `abort()` | `__builtin_unreachable()` |
| 合法输入语义 | 与 unsafe 相同 | 与 checked 相同 |
| 用途 | 训练、开发、调试、性质测试 | 已验证的竞赛提交 |

两个头文件由 `src/manifest.txt` 的同一语义源生成。任何手改生成头都会被
freshness 审计拒绝。

`npre` 只保护已经编码进去的结构前提，例如下标和区间。它无法检查 Dijkstra 边权是否
非负、predicate 是否单调、操作是否结合、模数下除数是否可逆；这些仍需要证明和针对性
测试。

```bash
cd /home/tnuzy/NitoriSTL
python3 tools/amalgamate.py --check
python3 tools/audit.py
```

---

## 4. 基础值、哨兵和可选结果

### 4.1 常量和类型

| API | 含义 |
|---|---|
| `nversion` | 当前为 `30000`（Nitori v3） |
| `nunsafe` | 当前头文件是否为 unsafe profile |
| `npos` | 不存在的位置/编号，值为 `-1` |
| `nwide_t<T>` | integral → `__int128_t`，其他 → `long double` |
| `ninf<T>` | 正向算法哨兵 |
| `nninf<T>` | 负向算法哨兵 |

### 4.2 `nmaybe<T>`

用于值域没有空闲哨兵的部分结果。

```cpp
nmaybe<int> x;
if (!x) { /* empty */ }

x = 7;
int a = x.val();       // 空值触发 npre
int b = x.val(-1);     // 空值回退
bool ok = x.ok();
x.reset();
```

支持 `operator bool`、`*`、`->`、`val()` 和 `val(fallback)`。

### 4.3 常用帮助函数

```cpp
nchmin(a, candidate);  // 变小时赋值并返回 true
nchmax(a, candidate);  // 变大时赋值并返回 true
nlen(object);          // 优先 integral len()，否则 integral size()；转 int 前检查范围
nbitceil(n);           // 至少为 1 的二次幂上取整，要求 0 <= n <= 2^30

nrng random(seed);
random();              // uint64_t
random(bound);         // [0,bound)
random(first, last);   // [first,last)
nseed(seed);           // 同时重置 nrng_global 和默认 nhash salt
```

比较器：`nless<>`、`ngreater<>`、`nequal<>`。`nhash<T>` 是带进程随机盐的哈希器，
并为 `pair` 提供组合特化；需要可复现实验时在创建哈希容器前调用 `nseed`。

---

## 5. 操作对象与隐形契约

Nitori 不再维护注册数学定律的 `concept/trait` 系统。模板只保留真正影响接口选择、
生命周期和存储布局的能力检查；结合、单位、交换、可逆、分配和数值域等性质由调用者
在使用点负责。这样可以减少“声明通过但证明仍在库外”的双重维护负担。

这不是把数学前提藏起来，而是把它放回真正使用它的地方。阅读任意操作对象时，先问四
个问题：

```text
空区间应该返回什么？
左右区间能否按原顺序合并？
我是否需要从一个结果中消去另一段？
连续两次修改的先后是否影响结果？
```

答案决定接口能否使用；C++ 类型系统通常只能检查“函数存在”，不能检查这些等式对所有
输入都成立。后文每个结构的契约卡都会把“性质”和“它保护的步骤”放在一起。

### 5.1 内建操作

| 操作包 | 单位元 | 适用约定 |
|---|---|---|
| `nadd<T>` | `T{}` | 加法应结合；需要逆元的接口还要提供 `inv` |
| `nmul<T>` | `T{1}` | 乘法应结合且单位为 `T{1}` |
| `nxor<T>` | `T{}` | 异或的单位和自逆性质 |
| `nmin<T>` | 类型的真实上界；浮点为 `+infinity` | 需要全序、幂等和单位元 |
| `nmax<T>` | 类型的真实下界；浮点为 `-infinity` | 需要全序、幂等和单位元 |

`ninf/nninf` 是为安全加减保留余量的算法哨兵，不能充当 `nmin/nmax` 的数学单位元；
两者故意是不同概念。

`naddsum_action<T>` 实现“区间加、区间和”：

```cpp
tag_id()                       // 0
compose(newer, older)          // older 后执行 newer
apply(sum, delta, length)      // sum + delta*length
```

有符号整数仍要求调用者保证运算不溢出；其他标量也必须在使用点确认这些等式。

### 5.2 自定义非交换操作

```cpp
struct nconcat {
    string id() const { return {}; }
    string operator()(string a, const string& b) const { return a += b; }
};

nseg<string, nconcat> seg(nvector<string>{"a", "bc", "d"});
assert(seg.fold(0, 3) == "abcd");
```

`nseg` 和 `nfold` 保持顺序，不假设交换。`id()` 必须是真正的空区间值，二元操作必须
结合；这些是实现无法从 C++ 类型系统恢复的隐形契约。

通用结合操作快速幂：

```cpp
auto x = npow(base, exponent, operation);
```

非负指数只要求结合操作和正确单位；负指数要求 `operation` 另外实现真正的逆元。复杂度
`O(log |exponent|)` 次合并，`LLONG_MIN` 也不会因取负溢出。

### 5.3 自定义 lazy action

作用协议固定为：

```cpp
struct action {
    F tag_id() const;
    F compose(const F& newer, const F& older) const;
    S apply(S aggregate, const F& tag, int length) const;
};

```

`compose(newer, older)` 表示原有 `older` 后再追加 `newer`。顺序错误是 lazy segment
tree 最常见的隐蔽 WA。调用者必须保证 tag 单位、compose 的结合与单位，以及 `apply`
对聚合和区间拼接的兼容性；接口检查不能证明这些等式。

---

## 6. 统一 Range/View/Projection：先学轻量位置，再学语义函数

本章分成两条难度不同的路线。6.1～6.7 是基础路线：把“一段元素”从具体容器中
抽出来，学习如何切片、组合、排序和物化；6.8 是高级路线：当位置不再等于题目中的
语义 key 时，才使用 `nfunc`。

不要把 `nfunc` 当作更漂亮的 `nview`。二者的选择不是风格问题，而是数据模型问题：

```text
算法只关心第 i 个位置在哪里？       → nview
算法还需要 key、support、求值和索引？ → nfunc
```

本章每个抽象都先回答“它消除了哪一次重复或哪一个错误”，再给出接口。若只是想把
一个范围排序或复制，读到 6.7 就足够；只有题目真正出现语义 key、有限定义域、分块
边界或惰性分支时，才继续读 6.8。

### 6.1 三层协议，不建立 iterator/trait 森林

先看一个需求：算法只想读取“第 `i` 项”，它并不关心元素来自 vector、deque、矩阵列，
还是 lambda 计算出的间接位置。如果为了每种来源复制一份算法，真正重复的不是代码语法，
而是同一个不变量。

Nitori 因此先描述算法能做什么，再决定对象叫什么：

```text
Range       = 算法能枚举或按位置读取的对象
nview       = 可复制的零拥有访问描述符，是唯一公共引用视图类型
Projection  = 算法临时把元素映射为 key 的可调用对象
nfunc       = nview 上层的有限离散函数，把 position、semantic key、value 接起来
```

Range 不是具体类。最小随机访问协议只有 `nlen(a)` 与 `a[i]`；最小枚举协议只有
`nenumerate(a)` 产生的 `ok/val/idx/next` 游标。算法按能力约束，不要求用户伪造 STL
iterator，也不通过一串 `iterator_traits/range_traits/proxy_traits` 推断语义。

`nrange_tag` 只表示一个对象可安全按值进入下一层组合器；`nview_tag` 只由真正的
`nview` 携带。两者是生命周期标记，不是按容器种类分派算法的 trait 表。

### 6.2 唯一 `nview<T,Accessor>`

连续、lambda、切片、步长、zip、窗口与二维布局都属于同一模板家族。差异只存在于
编译期 `Accessor`，没有虚函数、堆分配或运行时 type erasure。

```cpp
int raw[] = {3, 1, 2};
nview<int> a(raw);             // 连续数组
auto b = nview(raw, 3);        // CTAD：指针 + 长度
nview<const int> read_only(a); // 兼容的只读连续 view

a.len(); a.empty(); a.data();
a[i];
a.get(i);                      // 越界返回 nullptr

nvector<int> owner{5, 4, 3, 2, 1};
auto odd = nview(3, [&](int i) -> int& { return owner[2 * i]; });
nsort(odd);                    // 直接改写 owner 的 0/2/4 位置
```

连续 accessor 暴露 `data()`；一般 lambda/stride accessor 不伪造连续性。于是同一个
`nsort` 对连续 view 使用 `std::sort`，对可交换的非连续 view 使用原地 heapsort。

复制 `nview` 只复制访问描述，不复制元素：

```cpp
auto alias = odd;
alias[0] = 9;                  // 写回同一 owner
auto copy = ncollect(odd);     // nvector<int>，独立物化
```

`ncollect<T>(range)` 可显式指定目标值类型；对 `pair/tuple` 递归去掉引用。物化是显式
边界，库不会因为复制一个 view 而暗中做 `O(n)` 工作。

### 6.3 先问算法实际会做什么

不要从 concept 名字反推算法。先逐项检查真实操作：

| 算法行为 | 对输入的实际要求 | 典型算法 |
|---|---|---|
| 只按位置读取 | 能取得长度和 `a[i]` | `nfind`、二分、只读 fold |
| 要原地改值 | `a[i]` 必须是真实可写左值 | `nfill`、`nassign` 目标 |
| 要交换元素 | 两个位置必须能交换 | `nsort`、原地 reverse |
| 要走连续快路径 | 另外必须真实提供连续 `data()` | 连续 view 的排序快路径 |
| 要删除重复尾部 | 除可交换外还必须 resize | `nunique` |
| 只枚举、不随机访问 | 能产生 `ok/val/idx/next` 游标 | 图邻接、map、生成序列 |

这张表解释了为什么 `ndeque` 虽不连续仍能排序，也解释了为什么一个返回临时值的 lambda
view 不能原地排序：算法缺少的不是某个容器名，而是“可交换真实位置”。

维护模板或阅读诊断时，对应实现侧名字如下；普通做题无需背诵：

| 实现侧名字 | 表达的能力 |
|---|---|
| `nindexed<A>` | `nlen(a)` 与 const/non-const `a[i]` 存在 |
| `nindex_reference_t<A>` / `nindex_value_t<A>` | 索引引用类型及其值类型 |
| `nreference_indexed<A>` | 索引得到 lvalue reference |
| `nswappable_indexed<A>` | 元素是可写且可交换的 lvalue |
| `ncontiguous_indexed<A>` | 另外真实提供 contiguous `data()` |
| `nresizable<A>` | 提供 `resize(int)` |
| `nrange_object<A>` / `nview_object<A>` | 可安全接管的描述符 / 真正的 `nview` |
| `nviewable_indexed<A>` | indexed 左值 owner，或可进入组合器的安全描述符 |

### 6.4 半开切片与组合器

```cpp
auto all = nall(a);                    // 全序列引用
auto middle = nsub(a, 2, 7);           // [2,7)
auto every_second = nstride(a, 0, a.len(), 2);       // 0,2,4,... < len
auto by_stride = nstride(a, a.len() - 1, -1, -1);   // len-1,...,0
auto backwards = nreverse(a);
auto keys = nproject(items, [](item& x) -> int& { return x.key; });

auto nested = nreverse(nsub(a, 2, 7));
auto paired = nzip(nsub(a, 0, 3), nreverse(nsub(b, 1, 4)));
auto window = nwindows(nsub(a, 1, 8), 3)[1];
```

`nsub(a,first,last)` 与 `nstride(a,first,last,step)` 都使用终点排除语义。正步长要求
`0 <= first <= last <= len`；负步长沿下降方向在到达 `last` 前停止，并允许 `last == -1`
表示包含索引 `0`；空负步长区间另允许 `first == last` 位于 `[-1,len]`。`step == 0`
非法。这个协议与 `nrange(first,last,step)` 一致，不再有
“有时第三参数是 count、有时是 end”的双重心智模型。
组合结果按值携带中间 view，因此上例不会借用已经销毁的包装对象；它们仍借用 `a/b`
本体，所以 owner 必须继续存活且索引拓扑稳定。

原地算法接受左值 owner，也接受临时 view，因此无需为每一层机械命名：

```cpp
nsort(nreverse(nsub(a, 2, 7)));
nreverse_inplace(nsub(a, 0, 4));
nzeta_subset(nsub(table, offset, offset + (1 << bits)));
```

同样的调用若把临时 owning container 放在最底层会在编译期拒绝。

### 6.5 View projection 与 algorithm projection 必须分开

```cpp
struct item { int key, payload; };
nvector<item> a{{3,30}, {1,10}, {2,20}};

auto key_fields = nproject(a, [](item& x) -> int& { return x.key; });
nsort(key_fields); // 只重排 key 字段；payload 留在原位置

nsort(a, nless<>{}, [](const item& x) { return x.key; });
// 按 key 比较，但交换完整 item；key/payload 关系保持
```

`nproject(range,p)` 改变引用拓扑，结果是新的 `nview`；算法的 projection 只改变观察
方式，不改变被交换/移动的元素。`nsort/nfind/nlower/nupper/nfind_sorted/nfold/`
`nunique_compact/nunique/nsort_unique` 都接受 projection；比较器仍只负责 key 之间的
比较。成员指针也可作为 projection。

### 6.6 二维仍是同一个 `nview`

```cpp
int raw[12];
auto grid = nview(raw, 3, 4); // row-major；len()==12
grid.rows(); grid.cols(); grid.dim(0); grid(2, 3);

auto indirect_grid = nview(3, 4, [&](int row, int column) -> int& {
    return storage[index(row, column)];
}); // 二维 lambda 布局，不伪造连续性

auto row = nrow(grid, 1);       // 连续 nview，保留 data()
auto col = ncolumn(grid, 2);    // 非连续 nview
auto dia = ndiagonal(grid);     // 主对角线
nsort(dia);

// 显式二维 stride：base, rows, cols, row_stride, column_stride
auto transposed = nview(raw, 4, 3, ptrdiff_t{1}, ptrdiff_t{4});
```

二维 accessor 同时实现 flat `operator[](i)` 与 `operator()(row,column)`。`nrow/`
`ncolumn/ndiagonal` 把二维布局降为普通一维 view，因此所有序列算法直接复用；没有
`sort_diagonal` 特例。显式 stride 的地址合法性由调用者保证，view 不拥有底层存储。

`nmatrix<T>::view()` 返回这种二维 nview；其成员 `row/column/diagonal` 只是同名桥梁：

```cpp
nmatrix<int> m{{9,2,7}, {6,8,3}, {1,0,5}};
auto column = m.column(1);
nsort(column);

auto diagonal = m.diagonal();
nsort(diagonal);

auto upper = m.diagonal(1);   // offset > 0：主对角线上方
```

### 6.7 从 view 实例化独立 owner：`ncollect`

复制 view 对象只复制“如何访问”的描述符，仍然别名同一个底层 owner：

```cpp
auto alias = view;             // alias[i] 与 view[i] 指向同一元素
auto copy = ncollect(view);    // nvector<T>，逐元素独立复制
auto wide = ncollect<long long>(view); // 显式指定目标值类型
```

`ncollect` 接受任意 `nenumerable`，按枚举顺序建立 `nvector`；若源提供 `nlen`，会先
reserve。组合 view 可直接实例化，无需给中间层命名：

```cpp
auto reversed = ncollect(nreverse(nsub(a, l, r)));
auto indices = ncollect(nrange(n));
auto pairs = ncollect(nzip(a, b)); // nvector<pair<AValue,BValue>>，不是 pair 引用
```

结果的 `nvector` 存储独立，普通值元素不再别名源对象，修改结果不会写回 view；指针、
`reference_wrapper` 等元素自身声明的引用语义仍会保留。复杂度 `O(n)`，额外空间
`O(n)`。materialization 是一层的：若枚举元素本身仍是 view（例如 `nwindows` 的窗口），
收集到的是这些 view 描述符；需要深层副本时，显式对每个窗口再次 `ncollect`：

```cpp
auto blocks = ncollect(nproject(nwindows(a, width), [](auto window) {
    return ncollect(window);
})); // nvector<nvector<T>>
```

### 6.8 `nfunc`：用结构成本购买语义清晰

> **重量级边界：`nfunc` 不是 `nview` 的替代品。**
>
> `nview` 是位置到引用的轻量访问拓扑，适合内层循环、逐行/逐块临时建立和批量创建；
> `nfunc` 是它上面的语义层，可能拥有 domain 快照、哈希索引、分段边界、共享状态或
> 用户捕获的 evaluator。不要在每个元素、每条边、每次 DP 转移或热点循环中批量创建
> `nfunc`。通常一组数据只建立少数几个有名字的 `nfunc`，再用 view 或循环完成密集计算。

先用这张表决定要不要拿起这个重工具：

| 需求 | 应使用 | 原因 |
|---|---|---|
| 第 `i` 项引用哪个元素；切片、倒序、步长、矩阵行列 | `nview` | 只表达位置拓扑，通常是小描述符 |
| 热点循环中建立很多同形窗口、块、行 | `nview` / `nsub` / `nstride` | 不为每个对象建立语义索引 |
| 让枚举位置携带另一套状态 key | `nfunc_bind` / `nanchors` | 明确区分 position 与 key |
| 按 key 计算值 | `nfunc_value` / `nfunc_ref` | 由调用者明确选择值或借用结果 |
| 组合离散映射、给 DP 加哨兵状态 | `ncompose` / `nbranch_value` / `nbranch_ref` | 需要函数语义而不只是引用拓扑 |
| 把连续段作为可再次组合的离散对象 | `nruns` | 一次扫描建立段边界和起点索引 |
| 只想得到独立副本 | `ncollect` / `ntabulate` | 物化 owner，不需要函数层 |

#### 三个坐标，三种显式构造

有限函数给一个有限枚举加上语义自变量。给定有限 support
`D = [d_0,d_1,...,d_{n-1}]` 和 evaluator `phi`：

```text
f.key(i) = d_i       // 枚举位置 i 对应的语义自变量
f[i]     = phi(d_i)  // 第 i 项的函数值，可为真实引用
f(x)     = phi(x)    // 直接按语义自变量求值，不要求 x 已列在 D 中
```

最快的记法是：

```text
key(i)  position -> semantic key
f[i]    position -> value
f(x)    semantic key -> value
```

**方括号永远接收枚举位置，圆括号永远接收语义参数。** 即使 support 恰好是
`nrange(n)`，也不要靠“位置和 key 数值相同”混用两者。

#### 第一件事：把求值规则挂到有限 support 上

计算结果不再由重载猜测，必须明确选择返回策略：

| API | 结果契约 |
|---|---|
| `nfunc_value(domain,evaluator)` | 每次结果物化成独立值 |
| `nfunc_ref(domain,evaluator)` | evaluator 必须返回 `T&` 或 `const T&` |
| `nfunc_eval(domain,evaluator)` | 稳定自动策略：安全时保留 `T&/const T&`，临时参数产生的引用和公共 `T&&` 物化成值 |
| `nfunc_bind(domain,values)` | 按枚举位置绑定已有 value source，并快照 domain |

最小值函数：

```cpp
auto square = nfunc_value(nrange(6), [](int x) { return x * x; });

square.len();       // 6
square.key(4);      // 4
square[4];          // 16：按枚举位置
square(4);          // 16：按语义自变量
```

`nfunc_value/ref/eval` 的 `operator()(x)` 不做“x 是否在有限域中”的成员检查；有限域
规定枚举边界，evaluator 规定求值能力。需要在另一组 key 上重新枚举而不限制调用时用
`nredomain`；需要调用时也检查成员关系时用 `nrestrict`。

`nfunc_ref` 是显式借用契约：evaluator 返回的引用必须指向调用结束后仍存活的对象，不能
只是把临时 key 参数原样返回。不能证明这一点时使用 `nfunc_eval` 或 `nfunc_value`；前者
会在 key/argument 是临时对象时物化任何引用结果。

`nkeyed_indexed<F>` 检查 indexed value 与 `key(i)` 两层接口；
`ndiscrete_function<F>` 进一步检查 `f(key)`。因此纯 key/value 枚举可以进入
`nselect_positions`，却不必伪装成完整 callable。evaluator 构造的有限枚举域不要求
唯一、有序或可哈希；新代码直接写准确能力。需要运行时绑定/解绑键值时使用
`nfunc_hash`/`npartial`。

这一形式通常构造为 `O(1)`，但 evaluator 可以捕获任意状态，库不承诺它必然是
view 大小、无分配或适合海量复制。若只需 `a[index[i]]` 这样的密集位置访问，仍应写成
`nview`。

#### 第二件事：按枚举绑定两套序列

`nfunc_bind` 按位置绑定 indexed value source：

```cpp
auto f = nfunc_bind(nrange(0, 5), nrange(100, 90, -2));
// (key,value) = (0,100), (1,98), (2,96), (3,94), (4,92)

f[3]; // 94：按枚举位置
f(3); // 94：按语义 key
```

这个构造按枚举序号严格绑定，要求 domain 与 values 等长；它不像 `nzip` 那样截断到
较短一侧。绑定构造会拥有一份稳定的 domain 快照，避免外部修改 key 后使索引失效；
values 左值仍被借用并保持写回。domain 还必须能成为函数索引：重复 key 在 checked
构造时立即失败。

- `nrange` 域通过 `position(key)` 算术定位，构造、额外空间与查询均为 `O(1)`；
- 其他可哈希域构造一个 `nmap<Key,int>`，期望 `O(n)` 时间、`O(n)` 空间，语义查询
  `f(key)` 期望 `O(1)`；
- 不存在隐藏线性查询退化。无法建立受支持索引的 key 类型应改用 evaluator 构造。

这正是 `nfunc_bind` 与 `nview` 的成本分界：普通可哈希 domain 会在构造期建立索引，以换取
之后按 key 的期望 `O(1)` 查询。不要为了少写一个下标，在 `O(n)` 次外层迭代里反复
构造这个表；把函数提升到循环外复用，或退回位置 view。

#### 第三件事：重新锚定，而不是重新求值

`nanchors(source,anchors)` 按枚举位置为 source 换一套 key：

```text
result.key(i) = anchors[i]
result[i]     = source[i]
```

例如把倒序块号和正向位置按枚举序号绑定：

```cpp
auto schedule = nfunc_bind(nrange(blocks - 1, -1, -1),
                           nrange(0, blocks * width, width));

nforkv(block, begin, schedule) {
    // block 倒序，begin 正序
}
```

若正向位置已经是一个 source，等价的重新锚定写成：

```cpp
auto schedule = nanchors(positions, nrange(blocks - 1, -1, -1));
```

`nanchors` 与 `nredomain`/`nrestrict` 完全不同：前者保留第 `i` 个 value、替换它的
key；后两者把新 domain 中每项当作语义参数，重新调用源函数。

#### 看、改、复制：先分清别名和 owner

```cpp
auto keys    = nkeys(f);       // 零复制 key view
auto values  = nvalues(f);     // 零复制 value view
auto entries = nentries(f);    // pair<key-reference,value-reference> view

auto value_copy = ntabulate(f);             // 独立 nvector<Value>
auto same       = ncollect(f);               // 与 ntabulate 等价
auto table_copy = ncollect(nentries(f));     // 独立 pair 值表
```

evaluator 函数还提供引用限定成员 `keys()`：左值函数返回借用 view，右值函数把整个函数
对象移入 view。它不会再复制 domain holder，因此拥有大型 domain 时也没有隐藏的大复制；
通用代码仍优先写 `nkeys(f)`。

复制 `f`、`nkeys(f)` 或 `nvalues(f)` 不会自动复制底层 value；绑定构造的 `f` 可能同时
复制自己拥有的 key 索引，`nruns` 的段状态则共享。只有 `ntabulate`/`ncollect` 才创建
独立 value owner。`nentries` 的即时元素保留引用类别，便于 `nforkv(key,value,f)` 原地
改值；收集后会递归去掉 `pair/tuple` 中的引用。

域参数遵循统一 lifetime bridge：左值域被借用，右值域由离散函数拥有。因此下式不会
悬垂：

```cpp
auto f = nfunc_value(nvector<int>{10, 20, 30}, [](int x) { return x + 1; });
```

但 lambda 捕获的 `&storage` 仍只是普通 C++ 引用；`storage` 必须存活，结构修改也不能
使 evaluator 返回的引用失效。

holder 的深 const 契约固定为“非 const holder 得到可用的底层引用，const holder 得到
const 底层引用”，不因当前是拥有还是借用而改变。上面的借用规则描述 evaluator 构造；
按枚举绑定为了保护 key 索引会拥有 domain 快照，
只借用左值 values。values 的元素可原地修改，但其长度和索引拓扑在函数存活期间必须
稳定；checked 访问会验证长度仍与 domain 一致。

#### 第四件事：显式选择 branch 的值/引用策略

```cpp
auto state = nfunc_ref(nrange(n), [&](int i) -> long long& { return dp[i]; });
auto safe = nbranch_value(state, [](int i) { return i == -1; }, 0LL);

safe(-1); // 0；不会调用 state(-1)
safe(5);  // dp[5]
```

`nbranch_value` 总是返回两个分支去掉 cv/ref 后的公共值类型；即使 base 返回引用，也不会
产生可写回接口。`nbranch_ref` 则要求两个分支返回**完全相同的左值引用类型**：

```cpp
long long sentinel = 0;
auto writable = nbranch_ref(
    state,
    [](int i) { return i == -1; },
    [&](int) -> long long& { return sentinel; });

writable(-1) = 7; // 写 sentinel
writable(3) = 9;  // 写 dp[3]
```

两个操作都保留 base 的有限 support，并且只执行 predicate 选中的一个分支。
alternative 在 value 版本中可为 callable 或常量，在 ref 版本中必须是返回匹配引用的
callable。`f[-1]` 仍然非法，因为 `[]` 接收枚举位置；扩展的是 `f(-1)` 的语义求值。
若还要枚举 `-1`，使用 `nredomain(safe,nrange(-1,n))`；只有希望 `safe(x)` 也拒绝
域外参数时才用 `nrestrict`。

branch 会把待判定 key 当作具名对象复用：predicate 与选中的分支都以这个具名 key 的
左值类别调用，即使 key 最初来自纯右值或 `T&&`。这样同一个 key 只求值一次，也不会被
predicate 先移动后再交给分支；只接受消费性右值参数的 callable 不适合作 branch 的
key 处理器。

这是 DP 哨兵、边界状态和分段定义最直接的用法。predicate 与两个分支都可以很重，
但每次求值只执行被选中的分支；不要先算出 alternative 再传入，破坏惰性。

#### 第五件事：区分重定义域、真正限制与位置选择

```cpp
auto r = nredomain(f, domain);             // 只改变枚举 support
auto q = nrestrict(f, domain);             // support + 调用成员检查
auto g = nselect_positions(f, positions);  // 选择源枚举位置
```

- `nredomain(f,{x...})` 重新列出语义定义域，并以 `f(x)` 求值；`result(y)` 仍可调用
  support 外的 `y`。
- `nrestrict(f,{x...})` 枚举行为相同，但 `result(y)` 会检查 `y` 确实属于新 domain。
  有 `position(y)` 的 domain 直接定位；其他 domain 显式做 `O(n)` 成员扫描。
- `nselect_positions(f,{i...})` 从原枚举中选第 `i` 项，保留原 `key/value`；允许重复位置，
  引用结果会有意别名同一对象。
- `g.source_index(j)` 返回新枚举第 `j` 项来自哪个源位置；不要把它叫作 key 到 ordinal
  的逆映射 `position(key)`。

当原域恰好是 `nrange(n)` 时，自变量与位置数值相同，看起来两者等价；换成坐标、状态
编号或压缩后的 key 就不再等价。不要凭数值巧合混用这两个齿轮。

常用适配器按“改变什么”整理如下：

| 适配器 | 输入中的项目表示 | 结果保留什么 |
|---|---|---|
| `nredomain(f,domain)` | 语义 key | 新 support，调用域不受限 |
| `nrestrict(f,domain)` | 语义 key | 新 support，调用时检查成员关系 |
| `nselect_positions(f,positions)` | 源枚举位置 | 原 key 与原 value 引用 |
| `nsubfunc(f,l,r)` | 位置区间 `[l,r)` | 该段原 key/value |
| `nblock(f,b,w)` | 第 `b` 个位置块 | 尾块自动缩短 |
| `nblocks(f,w)` | 全部位置块 | 一个轻量 block view |
| `ncompose(outer,inner)` | 函数组合 | `outer(inner(x))` |
| `nmap_values(f,g)` | 值变换 | key 不变，值为 `g(f(x))` |

#### 第六件事：分块、组合和子序列

```cpp
auto sub = nsubfunc(f, l, r);       // 按源枚举位置取 [l,r)
auto one = nblock(f, block, width); // 第 block 块，尾块可短
auto all = nblocks(f, width);       // view of discrete-function blocks

auto h = ncompose(outer, inner);    // h(x) = outer(inner(x))
auto y = nmap_values(f, transform); // transform(f(x))，key 不变
```

所有公共离散函数结果只允许 `T`、`T&`、`const T&`；底层 callable 返回 `T&&` 时会
在适配器边界物化为 `T`。组合还有更强的稳定规则：若 inner 产生临时值，则 outer 从该
临时值产生的任何引用都会在 `ncompose` 返回前物化。因此
`ncompose(nidentity{}, value_returning_function)` 返回独立值，不会把局部中间对象送出。

`nsubfunc`、`nblock`、`nblocks` 都保留语义 key 和 value 引用，因此每个块仍可直接进入
通用算法：

```cpp
nvector<int> a{9,1,8,2,7,3,6,4};
auto cell = nfunc_ref(nrange(a.len()), [&](int i) -> int& { return a[i]; });

nfor(block, nblocks(cell, 3))
    nsort(block);
// a == {1,8,9, 2,3,7, 4,6}
```

#### 第七件事：用 `nruns` 把连续段升级成可组合对象

`nruns(source,together)` 扫描相邻 value；`together(previous,current)` 为真时留在同一段，
否则开始新段。省略 together 时使用 `nequal<>`。结果仍是离散函数：第 `j` 项的 key 是
该段在 source 中的起始枚举位置，value 是保留 source key 与引用的专用 run segment。

```cpp
nvector<int> colors{1,1,2,2,5,4,4};
auto runs = nruns(colors); // keys = {0,2,4,5}

auto items = nmap_values(runs, [](auto run) {
    return Item{run[0], run.len()};
});

auto chains = nruns(items, [](const Item& a, const Item& b) {
    return abs(a.color - b.color) == 1;
});

auto answer = ncollect(nmap_values(chains, [](auto chain) {
    return ncollect(chain);
})); // nvector<nvector<Item>>
```

构造 `nruns` 为 `O(n)` 次 source 访问，并保存 `O(k)` 个有序起点，其中 `k` 为段数；
按段位置访问为 `O(1)`，按精确段起点查询用二分，为 `O(log k)`。实现只有一个
`shared_ptr<nrun_state>` 分配；segment 直接保存共享 state 与 `[left,right)`，不再叠加
shared-function/subfunc/gather 适配器。detached segment 仍能延长源状态生命周期。边界是
构造时快照：之后修改 source value 不会自动重新分段，但段内 value 仍保持原引用语义。

子序列 DP 中，依赖关系本来就是一组离散位置。`nselect_positions` 把“按前驱表取状态”压成一个
可枚举函数，而不复制 DP：

```cpp
nvector<int> dp(n, 1);
auto state = nfunc_ref(nrange(n), [&](int i) -> int& { return dp[i]; });

for (int v = 0; v < n; ++v) {
    auto previous = nselect_positions(state, predecessor[v]);
    nforkv(from, best, previous)
        nchmax(dp[v], best + 1);
}
```

这里 `from` 是原状态的语义 key，`best` 是 `dp[from]` 的引用。`nforkv` 与 `nfor`
一样只有一层真实 `for`：`break` 会退出整个枚举，`continue` 会进入下一项。

#### 成本台账：不要把重工具当免费语法糖

| 操作 | 构造 | 单次访问/求值 | 额外状态 |
|---|---:|---:|---:|
| `nfunc_value/ref/eval(domain,evaluator)` | 通常 `O(1)` | evaluator 自身复杂度 | domain/evaluator holder |
| `nfunc_bind(nrange,values)` | `O(1)` | `O(1)` | domain 快照与 value holder |
| `nfunc_bind(generic_hashable_domain,values)` | 期望 `O(n)` | 期望 `O(1)` 按 key 查询 | `O(n)` key 索引 |
| `nanchors(source,anchors)` | 取决于 anchors locator；通常 `O(1)` 或期望 `O(n)` | 通常或期望 `O(1)` | 与 anchors domain 相同 |
| `nbranch_value/ref` | 通常 `O(1)` | predicate + 一个选中分支 | base/predicate/alternative |
| `nredomain` | `O(1)` | 源函数成本 | domain holder |
| `nrestrict` | `O(1)` | 源函数 + `O(1)` locator 或 `O(n)` 扫描 | domain holder |
| `nselect_positions` / `nsubfunc` | `O(1)` | 源函数成本 | position holder |
| `nblocks` | `O(1)` | 生成每个块描述为 `O(1)` | 一个 block view |
| `nruns` | `O(n)` 扫描 | `O(1)` 按段位置，`O(log k)` 按起点 | 一次共享分配 + `O(k)` 起点 |
| `ncollect` / `ntabulate` | `O(n)` | 物化后 `O(1)` 索引 | `O(n)` 独立 owner |

`nfunc` 家族追求的是**正确的结构复杂度**，不是一律零分配。需要按 key 查询时就建立
索引，需要连续段时就保存边界；它拒绝用隐藏的 `O(n)` 线性查找假装接口轻巧。代价应
在粗粒度边界支付一次，然后被算法复用。

若 `nselect_positions` 含重复位置，多个结果会别名同一 value；原地排序等依赖独立交换位置的
算法通常没有合理语义，应先去重位置或显式物化。

#### 上场前的五问

```text
1. 我只需要 position -> reference 吗？是：用 nview。
2. 我正在热点循环里批量创建同形对象吗？是：优先用 view、索引或直接循环。
3. 我确实需要 key -> value、惰性分流、函数组合或连续段语义吗？是：用 nfunc。
4. domain、values 和 evaluator 捕获的 owner 会活得足够久且拓扑稳定吗？
5. 我需要别名还是独立副本？独立副本必须显式 ncollect/ntabulate。
```

一句话收束：**`nview` 用轻量位置拓扑服务密集计算；`nfunc` 用索引、状态和组合能力
购买更短、更可靠的高层算法表达。前者通常可以批量造，后者应少量造、命名并复用。**

### 6.9 本章检查点

1. `auto b = nsub(a,l,r)` 与 `auto b = ncollect(nsub(a,l,r))` 的时间、空间和别名语义
   分别是什么？
2. 为什么 algorithm projection 会交换完整元素，而 `nproject` 可能只交换字段？
3. 一个 lambda view 返回 `int` 而不是 `int&` 时，哪些原地算法不再合法？
4. `f[i]`、`f.key(i)`、`f(x)` 的参数分别处于哪个坐标系？
5. 什么需求只需要 `nview`，什么需求才值得支付 `nfunc` 的索引和状态成本？

如果第 4～5 题不能直接回答，先手算一个 domain 不是 `nrange(n)` 的例子，再继续使用
`nredomain/nrestrict/nselect_positions`。

---

## 7. 枚举协议与组合视图

如果算法只需要“依次访问所有对象”，就不应要求对象伪造一套 STL iterator。Nitori 的
枚举协议把真正需要的四步显式化：是否还有元素、当前值、当前位置、前进。这样图邻接、
哈希映射、窗口序列和生成器可以共享 `nfor`/`ncollect`，而不必先物化成 vector。

本章主要服务两个场景：实现一个新 enumerable，或理解 `nzip`、笛卡尔积、窗口为何能
被普通算法直接消费。只写普通 `nvector` 题目时，先掌握第 1 章的 `nrep/nfor` 即可。

### 7.1 游标协议

任意对象可通过 `nenumerate(a)` 产生游标：

```cpp
auto e = nenumerate(a);
while (e.ok()) {
    use(e.val());
    int i = e.idx();
    e.next();
}
```

具有自定义 `enumerate()` 的对象优先使用它；普通 `nindexed` 左值使用借用索引游标；
可拥有的临时 indexed 对象由游标接管，避免立即悬垂。
`nenumerator_t<A>` 是产生的游标类型，`nenumerable<A>` 检查 `ok/val/idx/next`
完整协议；`ngraph_like` 等高层 capability 会在入口处使用它，而不是等模板深处才报错。

### 7.2 循环宏

```cpp
nfor(x, sequence) {
    // x 保留 cursor.val() 的值类别，常见容器中为 T&
}

nfori(i, x, sequence) {
    // i 是游标编号
}

nforkv(key, value, keyed_sequence) {
    // 离散函数取 semantic key/value；map 取映射键和值
}
```

三者都只建立一个具有普通 C++ 语义的循环：`break` 立即结束整个宏循环，`continue`
进入下一次枚举，`sequence` 只求值一次。宏内部不得用额外循环模拟元素绑定。

### 7.3 `nrange`

```cpp
nrange(last)
nrange(first, last)
nrange(first, last, step)

nrep(i, count)   // i = 0, 1, ..., count-1
nrrep(i, count)  // i = count-1, ..., 1, 0
```

`range.position(value)` 在 value 恰好属于该算术序列时返回其 `0-based` 枚举位置，否则
返回 `npos`；正负 step 都是 `O(1)`，并使用宽整数检查差值和整除关系。

仅接受有符号整数。正 step 枚举 `< last`，负 step 枚举 `> last`，step 不能为零。
任何 `nrange` 的元素总数都必须能放入 `int`；构造时即验证，不能靠提前 `break`
绕过游标编号溢出。`nrep/nrrep` 的循环变量固定为 `int`，`count` 只求值一次；非正次数为空，
正次数必须不超过 `INT_MAX`。两者的 `break/continue` 都是普通单循环语义。

### 7.4 zip、笛卡尔积和窗口

```cpp
auto z = nzip(a, b);          // 长度 min(len(a),len(b))，元素为引用 pair
auto p = nproduct(a, b);      // 左主序笛卡尔积，元素为引用 pair
auto w = nwindows(a, 3, 2);   // 宽 3、起点步长 2 的 nsub 视图序列
```

`nproduct` 的总元素数必须能放入 `int`。`nwindows` 要求 `width >= 0`、`step > 0`；
宽度超过 owner 长度时为空，宽度为零时有 `len(owner)+1` 个空窗口；若该数量超过
`INT_MAX`，构造失败而不是发生有符号溢出。

---

## 8. 拥有型序列与通用算法

本章是查阅台，不是让读者一次背完所有容器。选择顺序建议是：

```text
需要稳定连续数组       → nvector
需要两端进出            → ndeque
需要按优先级取一个      → nheap
需要固定多维布局        → narray/nmatrix
需要写入/读取/排序      → 第 8.5 节通用算法
```

每个算法表后都应回到题目约束：它到底需要只读索引、真实引用、可交换位置、连续存储，
还是 resize？接口能编译不代表题目的引用关系和复杂度已经正确。

### 8.1 `nvector<T>`

构造：空、`nvector(n)`、`nvector(n,value)`、initializer list。

| 成员 | 语义 |
|---|---|
| `len/cap/empty/data` | 长度、容量、空、连续指针 |
| `operator[](i)` | 检查后索引 |
| `get(i)` | 合法返回指针，否则 `nullptr` |
| `get(i,fallback)` | 按值返回或回退 |
| `reserve/resize/clear` | 容量与长度管理 |
| `push(args...)` | `emplace_back` 语义并返回新元素引用 |
| `pop()` / `pop(fallback)` | 取走尾元素；空时前者失败、后者回退 |
| `front/back` | 引用访问；另有带 fallback 的按值访问 |
| `del(i)` | 保序删除位置 `i`，线性移动后缀 |
| `swapdel(i)` | 取走 `i` 并以末项补洞，不保序 |
| `operator+=` | 追加一项 |

公开接口故意不是 STL 的 `.size()`/`.push_back()`。`nvector_stl<T>` 是同一实现的兼容
别名，不表示另一种后端。

### 8.2 `ndeque<T>`

`ndeque<T>` 默认是自主环形缓冲 `ndeque_ring<T>`；`ndeque_stl<T>` 是行为对照/迁移
后端。两者都提供 `len/empty`、索引与 `get`、`pushl/pushr`、`popl/popr`、带 fallback
的 pop、`front/back/clear` 与 `operator+=`。环形版本另有 `cap/reserve`。

deque 不连续，但满足普通 `nsort` 所需的可交换索引能力；Nitori 对非连续路径使用
原地 heapsort。因此不会再出现“STL 套壳有索引却进不了通用算法”的层间矛盾。

### 8.3 `nheap_binary<T,C>`

默认 `nheap<T,C>` 是真正的二叉堆后端 `nheap_binary<T,C>`，比较器表示“谁应位于堆顶”；
默认 `nless<T>` 因而得到最小堆。

```cpp
nheap<int> q;
q.push(4); q.push(1); q.push(7);
q.top();              // 1
q.pop();              // 取走 1
q.replace(5);         // 非空时替换堆顶并下沉
```

也可从任意 `nenumerable` 在 `O(n)` 建堆。`push/pop/replace` 为 `O(log n)`，`top` 为
`O(1)`；提供 `len/empty/reserve/clear` 与 `top/pop(fallback)`。

### 8.4 `narray<T,Rank>`

```cpp
narray<int, 3> cube({2, 3, 4});
cube(1, 2, 3) = 7;
cube(array<int,3>{1,2,3});
cube.pos(coord, npos);
cube.dim(axis, fallback);
cube.shape();
```

存储为 row-major 连续数组。每一维都必须非负，即使其他维为零也不会跳过检查；
总体积必须能放入 `int`。可变参数坐标只接受整数，并在转成 `int` 前验证表示范围，
不会把超大 `long long` 静默截断成另一个合法坐标。

### 8.5 通用序列算法

#### 基本写入台架

| API | 语义与前提 | 复杂度 |
|---|---|---|
| `nfill(destination,value)` | 把可写 indexed 目标的每个位置赋成 `value` | `O(n)`，零额外空间 |
| `nassign(destination,source,projection)` | 目标与任意 enumerable 源等长；从左到右投影、写入 | `O(n)`，零额外空间 |
| `nswap_ranges(left,right)` | 要求等长且两侧元素可交换；逐位置交换 | `O(n)`，零额外空间 |

目标参数始终在前。写入算法接受左值 owner，也接受 `nsub(...)`、`matrix.row(...)` 等
临时 view 描述符；owner 本身不会被按值复制。

```cpp
nfill(matrix.row(r), NEG);
nassign(checkpoint.row(block), dp);
nassign(keys, records, &record::key);
nswap_ranges(nsub(a, l, r), nsub(b, x, x + r - l));
```

`nassign` 不偷偷物化 source，也不承诺类似 `memmove` 的重叠快照语义。它严格从左到右
读写；若较早的写入会改变较晚位置所读到的 source，这个变化就是实际语义。需要原值快照时
应在边界上明确物化：

```cpp
auto snapshot = ncollect(source);
nassign(destination, snapshot);
```

这条约束避免一个看似方便的基本操作在热路径中暗藏分配。checked profile 会拒绝
`nassign` / `nswap_ranges` 的长度不等；对没有 `nlen` 的 source，`nassign` 会通过
游标耗尽位置验证等长。unsafe profile 要求调用者已经保证等长。

#### 基本读取台架

| API | 语义与前提 | 复杂度 |
|---|---|---|
| `nfind_if(a,predicate,projection,fallback)` | 返回第一个满足谓词的枚举位置；失败返回 `fallback`，默认 `npos` | 最坏 `O(n)` |
| `ncontains(a,x,projection)` | 是否存在 `projection(element) == x` | 最坏 `O(n)` |
| `ncount(a,x,projection)` | 统计投影值等于 `x` 的元素数 | `O(n)` |
| `ncount_if(a,predicate,projection)` | 统计投影后满足谓词的元素数 | `O(n)` |
| `nall_of/nany_of/nnone_of(a,predicate,projection)` | 全称、存在与全否定判断，支持短路 | 最坏 `O(n)` |
| `nsame(a,b,equal,proj_a,proj_b)` | 两个 enumerable 锁步比较；长度不同也返回 false | 最坏 `O(n)` |
| `nargmin/nargmax(a,compare,projection)` | 返回最小/最大投影值的 indexed 位置 | `O(n)` |

读取算法只要求它真正使用的能力：查找、计数、量词与 `nsame` 接受任意
`nenumerable`；只有必须返回可重用随机访问位置的 `nargmin/nargmax` 要求 indexed。
量词在空序列上分别是 `true / false / true`。`nargmin/nargmax` 在空序列上返回
`npos`，相等时保留第一个位置。

projection 的默认值是 `nidentity{}`。成员指针与 lambda 都可以直接作为 projection：

```cpp
int first_heavy = nfind_if(edges, [](int w) { return w > limit; }, &edge::weight);
int cheapest = nargmin(edges, nless<>{}, &edge::cost);
bool same_keys = nsame(records, keys, nequal<>{}, &record::key, nidentity{});
```

#### 排序、二分、折叠与去重

| API | 前提 | 复杂度 |
|---|---|---|
| `nsort(a,cmp,proj)` | viewable + `nswappable_indexed`，cmp 在投影 key 上为严格弱序 | `O(n log n)`；连续走 `std::sort`，否则 heapsort |
| `nreverse_inplace(a,l,r)` | 可交换；默认全区间 | `O(r-l)` |
| `nfind(a,x,proj,fallback)` | 可读 indexed；比较 `proj(a[i]) == x` | `O(n)` |
| `nlower/nupper(a,x,cmp,proj)` | 已按同一 cmp/proj 排序 | `O(log n)` |
| `nfind_sorted(a,x,cmp,proj,fallback)` | 已排序；按投影 key 的 cmp 等价查找 | `O(log n)` |
| `nfold(a,l,r,op,proj)` | op 有正确单位元且结合 | `O(r-l)`，保持顺序 |
| `nunique_compact(a,equal,proj)` | 相邻投影 key 等价元素压缩 | `O(n)`，返回保留长度但不 resize |
| `nunique(a,equal,proj)` | 另需 `resize` | `O(n)`，并缩短容器 |
| `nsort_unique(a,cmp,equal,proj)` | 可排序且可 resize | `O(n log n)`，排序后去重 |

`nunique` 只压缩相邻等价项；通常先用相同 projection 执行 `nsort`。

---

## 9. 机制、内存、关联对象与 AST

本章收纳的是“为算法提供结构能力，但通常不是题目最终算法名”的机制。为了避免把它
当成杂物表，按用途分成四组阅读：

```text
9.1～9.2   扫描与单调搜索：减少重复求值
9.3～9.6   回滚、临时内存、arena/pool：控制状态和生命周期
9.7～9.11  有限集合、映射、关系与坐标压缩：建立对象对应
9.12       有序树 AST：沿节点信息调度，而不是封死树实现
9.13       隐式 FHQ：把 FHQ 结构和序列语义彻底分开
```

做题时按阻碍跳到对应小节，不要求线性读完。尤其不要因为 `narena`、`npool` 看起来更
底层，就在普通序列题里提前引入手工句柄；只有删除、复用或稳定 handle 真正影响复杂度
和正确性时才使用。

### 9.1 扫描

```cpp
auto prefix = nscan(a, op);          // 长度 n+1，prefix[0] = id
auto suffix = nsuffix_scan(a, op);   // 长度 n+1，suffix[n] = id
```

两者保持操作顺序，因此支持非交换的结合聚合。

### 9.2 单调二分

```cpp
I nfirst_true(first, last, predicate); // [first,last) 中首个 true；全 false 返回 last
I nlast_true(first, last, predicate);  // 最后 true；要求 first > lowest
```

两者都要求 `first <= last`，调用者保证谓词单调。`nlast_true` 返回 `first-1`
表示区间内没有 true。

### 9.3 `nrollback<T>`

```cpp
nrollback<int> log;
int checkpoint = log.time();
log.save(x);
log.assign(y, 10);
log.mutate(x, [](int& v) { v += 3; });
log.undo();
log.rollback(checkpoint);
```

日志保存目标地址，因此目标必须在 rollback 前保持存活且地址稳定。

### 9.4 `nscratch<T>`

```cpp
nscratch<int> scratch;
scratch.reserve(n);
auto work = scratch.space(n);       // 旧内容可能保留
auto zero = scratch.filled(n, 0);   // 显式填充
```

下一次 resize 可能使之前返回的 span 失效。

### 9.5 `narena<T>`

```cpp
narena<Node> pool;
int mark = pool.mark();
int id = pool.make(args...);
Node& x = pool[id];
pool.rollback(mark);
```

handle 是当前 arena 中的整数下标。扩容可能使引用失效，但 handle 仍有效；rollback
删除后的 handle 不再有效并可能被后续 `make` 复用。

### 9.6 `npool<T>`：可删除、可复用 handle

`narena` 的 handle 是当前连续前缀的 0-based 下标，适合整体 rollback；`npool<T>`
（实现名 `npool_dynamic<T>`）提供独立删除和槽位复用，handle 从 1 开始，`0` 永远无效：

```cpp
npool<Node> pool;
int h = pool.make(args...);
pool[h];
pool.get(h);       // 无效/已删除返回 nullptr
pool.del(h);       // 析构对象；后续 make 可复用 h
pool.len();        // 当前存活对象数
pool.cap();        // 已分配过的 handle 上界
```

另有 `reserve/clear/empty`。删除后旧 handle 不是代际句柄：若槽位被复用，数值相同的
旧 handle 会指向新对象；需要防 ABA 时由上层附加 generation。扩容也可能使已取得的
`T&/T*` 失效，整数 handle 本身仍可重新查询。

### 9.6.1 v3 节点底座：`nresource_pool`、`nnode_domain` 与 `nnode_view`

v3 不再让每个树结构偷偷复制一份可复用节点池。底层分成三层，但不建立 concept
森林：

```text
nresource_pool<T>   只负责槽位、回收、generation 和整数 handle
nnode_domain<T>     负责共享资源池、结构 epoch 与跨 owner 生命周期
nnode_view<S>       负责从 owner 读取领域字段的借用结构视图
```

`npool<T>`、`npool_dynamic<T>` 和有序树内部的 `ni::nslot_pool<T>` 仍保留，都是资源池的
兼容名字。`nnode<S>` 仍可使用，但它现在是 `nnode_view<S>` 的旧拼写；新代码优先写出
`nnode_view`，以免把“节点身份”和“节点访问界面”混成一个领域结构。

资源池的 generation 解决槽位 ABA；`nnode_view` 和 `nseg_node` 现在共同使用内部的
`ni::nnode_stamp`，快照会保存 owner/domain、epoch 和非空 handle 的 generation。因而
`current()` 必须同时看到同一 owner、同一 domain、同一 epoch 和同一资源身份；即使底层
调用点尚未推进 epoch，删除后复用槽位也不会让旧视图误认新对象。固定布局线段树没有可
复用资源槽位，generation 使用稳定的后端标记，结构修改仍由 epoch 负责。共享 domain 的
复制只是复制 domain handle，拥有型 DS 的复制构造必须显式 `clone()` 出独立资源，不能让
两个逻辑容器意外共享同一棵树。

隐式 FHQ 已接入这层底座：

```cpp
auto domain = nseq_fhq<int>{}.domain();
nseq_fhq<int> left(domain), right(domain);
left.merge_from(move(right));       // 同 domain，right 被消费
auto [prefix, suffix] = move(left).split_at(k);
```

`merge_from` 要求两个 root 不重叠、策略语义等价且属于同一 domain；这些是调用点的
数学/所有权契约，故意写在接口注释中而不是编码成一排 concepts。合并或切分会推进
共享 epoch，使所有相关 owner 的旧视图一起失效。不同 domain 的 root 不能直接复制
handle；必须 clone/rebuild 或显式迁移。

### 9.7 `npartition`

由任意整数 labels 构造，首次出现顺序被压成稠密类编号。

```cpp
npartition p(nvector<int>{8,8,3,5,3});
p.len(); p.classes(); p.empty();
p.classof(i, fallback);
p[i]; p.same(a,b);
auto groups = p.groups();
```

别名：`npart`、`npart_dense`。

### 9.8 `nperm`

```cpp
nperm id(n);
nperm p{2,0,1};
int y = p(x);
auto inv = ~p;
auto composed = f * g;   // (f*g)(x) = f(g(x))
auto power = p.pow(k);   // k 可负，包括 INT64_MIN
auto cycles = p.cycles();
auto pulled = p.pull(a); // result[i] = a[p(i)]
auto pushed = p.push(a); // result[p(i)] = a[i]
```

构造映射必须是 `[0,n)` 上的双射。

### 9.9 有序集合、多重集和后端

```cpp
nset<int> s{4,1,4};   // 默认 nset_fhq，唯一集合
nbag<int> b;          // 默认 nset_fhq<...,Multi=true>，保留重数

s.ins(x); s.del(x); s.delall(x);
s.has(x); s.count(x); s.get(x);
s.rank(x);            // 严格小于 x 的元素数，重数计入
s.kth(k);             // 0-based，第 k 项；越界为空 nmaybe
s.lower(x); s.upper(x);
s.min(); s.max();
```

集合按序枚举，并支持 `| & - ^` 及其赋值版本。可选后端：

| 类型 | 结构 | 搜索/更新/rank/kth | 说明 |
|---|---|---|---|
| `nset_fhq<T,C,Multi,A,L>` | 随机 FHQ treap | 期望 `O(log n)` | 默认；支持重数、augmentation 和可选 lazy action |
| `nset_splay<T,C,Multi,A>` | splay | 摊还 `O(log n)` | 访问会旋转；支持重数和 augmentation |
| `nset_stl<T,C>` | `std::set` 参考后端 | 搜索/更新 `O(log n)`，rank/kth `O(n)` | 仅唯一集合，不支持 AST augmentation |

`nset_fhq` 和 `nset_splay` 也可以从同一 `domain()` 构造多个协作 owner。两者都提供
消费式 `split_by(value)`，结果是 `[key < value]` 与 `[key >= value]` 两个共享 domain 的
集合；`merge_from(move(other))` 再把右侧集合接到左侧。合并要求 root 不重叠、domain 相同、
比较器/augmentation/action 语义等价，并且 `max(left) < min(right)`；这些是调用点的
数学与所有权契约，代码旁的注释是故意保留的自由接口边界，不以 concept 森林替代它们。
合并/切分/清空只释放当前 owner 的 root，不会把同 domain 的兄弟 owner 一起清空；共享 epoch
会让所有旧 `nnode_view` 一起失效。复制集合会 `clone()` 出独立 domain，不能意外修改原树。

```cpp
auto domain = nset_fhq<int>{}.domain();
nset_fhq<int> left(domain), right(domain);
left.ins(1); right.ins(9);
left.merge_from(move(right));
auto [small, large] = move(left).split_by(5);
```

`nseed(seed)` 可固定 FHQ priority 流和默认哈希盐，便于可复现对拍。比较器必须满足严格
弱序；等价定义为 `!cmp(a,b) && !cmp(b,a)`。

### 9.10 哈希表、映射和关系

```cpp
nmap<K,V> a;          // 默认 nmap_flat：开放寻址 + 稠密 entry 数组
nmap_hash<K,V> b;     // unordered_map 参考后端
nmap_stl<K,V> c;      // nmap_hash 的兼容别名

a.ins(key, value);    // 已存在不覆盖
a.set(key, value);    // 插入或覆盖，返回 V&
a[key];               // 缺失则值初始化
a(key);               // 必须存在
a.get(key); a.has(key); a.del(key);
nforkv(key, value, a) { /* value 是 V& */ }
```

平均查找/更新 `O(1)`，最坏受哈希碰撞影响。`nmap_flat` 删除时会把最后 entry 移入空洞，
扩容会重排 bucket，因此不要依赖枚举顺序或跨结构修改保存 entry 引用。

`nrel<L,R>` 是有限二元关系：`add/del/has`、`image(left)`、`preimage(right)` 和集合
运算。当前 `nrel_scan` 后端以边数组线性扫描，单次查询/修改最坏 `O(E)`；它用于小型
胶水关系，不冒充图邻接或高性能数据库索引。

### 9.11 部分函数、双射与坐标压缩

```cpp
npartial<A,B> f;      // 同义：nfunc_hash<A,B>、底层 npartial_hash
f.bind(x, y);         // x 未绑定则插入；已绑定同值返回 true，不同值返回 false
f.set(x, y);          // 覆盖
f.to(x); f(x); f.unbind(x);

nbije<A,B> bij;       // 同义：nbije_hash；ninj 当前是同一一一映射契约
bij.bind(a, b);       // 两侧都不能与不同对象冲突
bij.to(a); bij.from(b);
bij.unbindl(a); bij.unbindr(b); bij.set(a,b);
auto inverse = ~bij;
auto composition = outer * inner;
```

`nfunc_hash` 是“键可能尚未绑定”的关联对象，与 6.8 的
`nfunc_value/ref/eval(domain,evaluator)` 有意分名；后者是无需存表的有限 keyed
函数。

坐标压缩建立值到 `[0,k)` rank 的双射：

```cpp
auto rank = ncompress(values);       // 任意 Nitori enumerable
auto rank2 = ncompress_stl(stl);     // STL range 桥
rank.to(value);                      // 不存在返回 npos
rank.from(index);                    // const T* 或 fallback
rank(index);                         // 必须存在的反向值
```

类型 `nbije_rank<T,C>` 构建为 `O(n log n)`，查询 `O(log n)`。

### 9.12 可扩展有序树 AST

普通 `set`/`multiset` 只回答“某个值是否存在、排名是多少”。但不少题目还会问：

- 第 `k` 个元素在哪个节点；
- 第一次使前缀和达到 `target` 的键是什么；
- 某棵子树的元素数、总和或自定义信息是多少；
- 找到一个结构节点后，对整棵子树施加合法的 lazy tag。

把这些能力分别封成 `kth_sum_tree`、`prefix_tree`、`tagged_tree` 会产生许多互不兼容的
树。Nitori 采用另一条路线：owner 维护平衡树和不变量，`nnode<S>` 暴露只读 AST 快照，
通用调度器只依赖节点的 `left/right/len/info` 等最小能力。

#### 第一层：先用结构，不加聚合

下面的下降只依赖左右子树长度和当前键重数，因此普通 FHQ/splay 都能复用：

```cpp
int remaining = k; // 0-based
auto found = tree.walk([&](auto node) {
    int left = node.left().len();
    if (remaining < left)
        return nbranch::left;
    if (remaining < left + node.count())
        return nbranch::take;
    remaining -= left + node.count();
    return nbranch::right;
});
```

每次选择都会丢掉一整侧子树；在树高为 `h` 时只访问 `O(h)` 个节点。这里的正确性来自
中序顺序：左子树、当前键的全部重数、右子树正好连续覆盖当前排名区间。

`nwalk` 不替你证明下降决策正确。若 `remaining` 的更新少减了 `node.count()`，接口仍然
成立，但排名不变量已经破坏。

#### 第二层：让节点携带可合并信息

FHQ 和 splay 不是封死的容器。模板参数 `A` 描述子树信息：

```cpp
struct sum_augment {
    using info_type = long long;
    long long id() const { return 0; }
    long long one(const int& value, int count) const { return 1LL * value * count; }
    long long op(long long left, long long right) const { return left + right; }
};

using bag = nset_fhq<int, nless<int>, true, sum_augment>;
bag t;
t.ins(5, 3);
auto root = t.root();
root.info(); root.len(); root.left(); root.right();
```

`A` 需要提供 `info_type`、`id/one/op`；`nnode<S>` 是只读快照，暴露 `val/count/len/info`
和左右子树。`nwalk(tree,decide)` 以 `nbranch::left/take/right` 实现自定义下降；
`nfirst_prefix`/`nlast_suffix` 依赖 augmentation 做前缀/后缀单调定位。对应成员
`walk/first_prefix/last_suffix` 只是短桥。

四个接口各自有明确职责：

```text
id()                 空子树的信息
one(value,count)     当前键及其重数的信息
op(left,right)       按中序顺序合并相邻信息
info_type            节点缓存的信息类型
```

`op` 必须结合，但不必交换。字符串拼接可以把整棵树的有序键展开成字符串；如果实现把
右信息放在左信息前面，树仍能编译，却会悄悄颠倒语义。

#### 第三层：用聚合做单调下降

`nfirst_prefix(predicate)` 从左到右累积信息，寻找第一次使 predicate 为真的节点；
`nlast_suffix` 对称地从右到左寻找。承重前提不是“有 info 就能二分”，而是 predicate
随前缀或后缀扩展保持正确单调性。

例如键均非负、`info` 是子树和时，`sum >= target` 单调；如果值允许正负交替，前缀和
达到 target 后可能再次跌落，这时聚合下降不再保证找到真正的第一次。

#### 第四层：只给保持有序性的变换加 tag

FHQ 另有第五个模板参数 `L`，为 AST 节点提供真正的 lazy tag：

```cpp
struct add_tag {
    using tag_type = int;
    int tag_id() const { return 0; }
    int compose(int newer, int older) const { return newer + older; }
    int apply_value(int value, int tag, int) const { return value + tag; }
    long long apply_info(long long info, int tag, int length) const {
        return info + 1LL * tag * length;
    }
};

using tagged_bag = nset_fhq<int, nless<int>, true, sum_augment, add_tag>;
tagged_bag t(nless<int>{}, sum_augment{}, add_tag{});
t.apply(tag);                 // 整棵树
auto node = t.root().left();
t.apply(node, tag);           // AST 选中的子树
t.root().tag();               // 当前节点尚未下推的 tag
```

`L` 需要提供 `tag_type`、`tag_id/compose/apply_value/apply_info`；`nempty_tag<T,I>` 是
默认 no-op action。`compose(newer,older)` 仍表示先 `older` 后 `newer`。
`apply_value` 的第三个参数是当前键的重数，`apply_info` 的第三个参数是整棵子树长度。
调用者必须保证 tag 单位、compose 结合，以及逐键作用与整棵子树信息作用相容；这些
等式不能从接口形状推出。

这是 **FHQ 有序树** 的 lazy action，不是隐式序列 treap。调用者必须保证把 tag 作用于所选
子树后，比较器下的中序顺序和等价类仍然合法；整树平移是典型合法例子，对任意局部子树
取负通常会破坏 BST 不变量。Splay 当前仍只有 augmentation，没有 lazy action。读取子节点
会按需下推 tag；这是逻辑只读的表示变化，不会单独让 epoch 失效。

两种聚合定位都要求 predicate 随前缀/后缀扩展具有正确单调性；它们只利用子树聚合
下降，不会替调用者证明“首次为真”之后不会再次变假。

节点快照还保存资源 generation；任何可能改变拓扑的操作都会令旧快照
`current()==false`，删除并复用资源槽位也会令它失效。尤其 splay 的
`has/get/rank/...` 也可能旋转，不能把 `nnode` 跨下一次树操作保存。FHQ 的纯查询当前不
改拓扑，但公共安全规则仍是“用完即弃”。`nempty_augment<T>` 是默认空信息，`nwalk`、
`nfirst_prefix` 和 `nlast_suffix` 直接按调用点所需接口实例化，不再额外注册 AST concept；
因此自定义 owner 可以自由调度节点，但必须自己满足上述接口和聚合单调性。

#### FHQ、splay 与节点快照怎样选择

| 需求 | 选择与边界 |
|---|---|
| 一般有序集合/多重集、期望 `O(log n)` | FHQ；随机优先级意味着复杂度保证是期望意义 |
| 希望访问热点节点被旋到上层 | splay；即使逻辑只读查询也可能改变拓扑 |
| 需要 augmentation | 两者都支持 |
| 需要有序树 lazy tag | 当前使用 FHQ；splay 还没有该 action |
| 保存节点供下一次操作继续使用 | 不要这样做；`nnode` 是带 epoch 的即时快照 |

**迁移信号：**当题目不仅要维护有序集合，还要沿子树信息自定义下降时，想到 AST；当
你只是需要第 `k` 小、前驱后继等已有成员时，优先调用现成接口，不必为了“通用”手写
`nwalk`。

### 9.13 隐式 FHQ：自由的按位置序列引擎

`nset_fhq` 是 **按 key 排序** 的 FHQ；区间翻转、搬运和按位置切分不应该被硬塞进它。
隐式 FHQ 的中序顺序就是序列顺序，树只维护四件与语义无关的事情：随机优先级、左右
孩子、子树长度和父链接。它不要求所有题目都把信息写成 `id/one/op`，也不要求所有 tag
都能被同一个 `compose/apply_info` 公式解释。

```cpp
template <class T, class P = nfhq_policy<T>>
class nimplicit_fhq;
template <class T, class P = nfhq_policy<T>>
using nseq_fhq = nimplicit_fhq<T, P>;
```

#### 先看它解决什么

```text
按位置插入/删除       split + merge，期望 O(log n)
区间查询               isolate [l,r)，读取中间 AST 的 info
区间 tag               isolate [l,r)，调用 P::apply(node, tag)
区间翻转               一个普通用户 tag：交换两支，并在用户 info 中完成重排
搬运/循环移位          splice / rotate，仍然只组合 split + merge
从任意子树继续下降     nwalk(node, decide)，不是只能从 owner root 开始
```

所有区间都是 `[l,r)`。`fold(l,r)` 为了让任意 `info_type` 都能工作，会临时 split 再
merge；它保持逻辑序列不变，但属于结构操作，调用后旧 `nnode` 快照不应继续使用。
`fold()` 全树读取不需要拆树。

#### 核心不是“翻转接口”，而是策略拥有语义

策略 `P` 只需在实际用到的地方提供：

```text
info_type                    子树信息类型
state_type                   用户自己的待下推状态，可以很复杂
id()                         空子树信息
leaf(value)                  单节点初始信息
state_id()                   清空待下推状态
pull(node)                   根据当前左右孩子和值重建 node.info()
push(node)                   把 node.state() 分派到孩子，并清空自身状态
apply(node, tag)             处理任意 tag；可改 value/info/state/孩子顺序
```

`node` 是策略专用的可编辑 AST 句柄，提供：

```text
val() / info() / state()     对当前节点的可写引用
left() / right()             子节点句柄（空子树为 false）
len()                        当前子树长度，只读
apply(tag)                   递归调用同一策略的 apply
exchange_children()          唯一内建的顺序拓扑原语
```

这里没有 `reverse_subtree`，也没有一个库级“反向聚合”协议。翻转只是用户策略的一种
合法 action：它可以交换孩子、交换括号匹配信息的前后字段、更新滚动哈希的方向，甚至
同时改变多个自定义状态。树核心不猜这些语义，也不会强迫每个 augmentation 额外存一份
镜像信息。若某个问题的摘要在反转后确实无法从已有状态得到，策略必须把它需要的状态
存进去；这是信息论边界，不是模板限制。

#### 一个同时支持 assign/reverse 的 run 统计策略

下面的 `info_type` 维护最长相同值段、前缀段和后缀段。注意翻转并不是调用库里某个专用
函数，而是策略自己决定如何重排摘要；`push` 还可以按需要把不同 tag 分发给左右孩子。

```cpp
struct reverse_tag {};
struct assign_tag { int value; };

struct run_policy {
    using info_type = run_info;
    struct state_type {
        bool assigned = false;
        int value = 0;
        bool reversed = false;
    };

    info_type id() const { return {}; }
    info_type leaf(int x) const { return {1,x,x,1,1,1}; }
    state_type state_id() const { return {}; }

    void pull(auto node) const;       // join(left, leaf(node.val()), right)
    void push(auto node) const;       // 分别下发 reversed / assigned

    void apply(auto node, assign_tag tag) const {
        node.val() = tag.value;
        node.info() = uniform_info(node.len(), tag.value);
        node.state().assigned = true;
        node.state().value = tag.value;
    }
    void apply(auto node, reverse_tag) const {
        node.exchange_children();
        swap(node.info().first, node.info().last);
        swap(node.info().prefix, node.info().suffix);
        node.state().reversed = !node.state().reversed;
    }
};

nimplicit_fhq<int, run_policy> t{0,1,1,0,1,0};
t.apply(1, 5, reverse_tag{});
t.apply(2, 5, assign_tag{0});
t.splice(1, 4, 3);      // 位置 3 是删除该段后的序列坐标
t.rotate(1, 3, 6);      // [1,3) 与 [3,6) 交换
```

这个例子故意没有偷偷加入 `reverse_subtree`、双份聚合或额外 concept。策略自己拥有
“什么状态足以回答题目”的判断；引擎只保证 split/merge 之后每个节点仍恰好属于一棵
树、长度正确、父链接可追踪、旧快照可诊断。

#### 标准 adapter 与自由策略的关系

`nfhq_policy<T,A,L>` 是方便迁移的轻量 adapter：它复用旧的 `A::id/one/op` 和
`L::apply_value/apply_info`，适合区间加、赋值等逐元素且不改变顺序的操作。它不是
`nimplicit_fhq` 的上限。遇到反转、位置相关 tag、多状态 DP 摘要或需要把一个 tag 拆成
左右两个不同 tag 时，直接写自定义 `P`，不要再给 adapter 堆模板参数。

#### 位置 API 与不变量

```cpp
t.ins(index, value);             // [0,len] 插入
t.del(l, r);                     // 删除 [l,r)，返回删除数量
t.set(index, value);             // 单点替换并重建祖先摘要
t.get(index);                    // 按位置读取值
t.fold(l, r);                    // 任意 info_type 的区间摘要
t.apply(l, r, tag);              // 调用 P::apply
t.apply(t.root().left(), tag);   // 对选定 AST 子树施加 tag
t.mutate(l, r, callback);        // 直接编辑隔离后的策略节点
t.splice(l, r, at);              // 删除后把该段插到 at
t.rotate(l, m, r);               // [l,m) 与 [m,r) 交换
```

`mutate` 是最后一道逃生舱：callback 必须让 `value/info/state/孩子顺序` 重新组成合法
的懒表示，库只负责向上调用 `pull`。它适合一次性构造或实验，不应被用来绕过策略的
`apply/push` 不变量。所有改变拓扑、删除节点或调用 `fold(l,r)` 的操作都会推进 epoch；
读取子节点时按需 `push` 不额外推进 epoch，因为它不改变逻辑序列。

#### 证明骨架与迁移信号

1. **split 正确**：按左子树长度递归，左输出恰好含前 `k` 个中序节点，右输出含其余节点。
2. **merge 正确**：只在左序列全部先于右序列时按 priority 选根，因此中序拼接不变。
3. **lazy 正确**：`apply` 立即更新当前摘要，`push` 只把尚未解释的 state 分派给孩子；
   `pull` 只在孩子已解释当前节点 state 后重建摘要。
4. **epoch 正确**：快照不是稳定指针；任何结构操作后继续使用旧快照都属于错误的时间点。

看到“序列顺序会变化”“区间操作需要按位置切分”“摘要不是简单可交换和”时，想到
`nimplicit_fhq`。看到“按 key 搜索并维护有序集合”时，仍然使用 `nset_fhq`；不要让两种
FHQ 互相承担不属于自己的不变量。

### 9.14 AST 检查点

1. `nnode` 为什么是快照而不是稳定节点指针？
2. `one(value,count)` 和 `info()` 中的长度分别表示键重数还是整棵子树长度？
3. `first_prefix` 的 predicate 需要什么单调性？允许负数时会怎样失败？
4. 为什么整树平移通常是合法 tag，而任意局部取负通常不合法？
5. FHQ 与 splay 的只读查询对 epoch 有什么不同？
6. 隐式 FHQ 的 `apply` 为什么必须立即更新当前 info，不能只把 tag 留进 state？
7. `exchange_children()` 为什么不能自动替策略更新首尾、哈希或括号摘要？
8. `splice(l,r,at)` 的 `at` 属于删除前还是删除后的坐标系？
9. 什么时候 `nfhq_policy<T,A,L>` 已经够用，什么时候应直接写自由策略？

---

## 10. 可合并区间信息、历史版本与离线算法

这一章不从“哪一个模板参数最复杂”开始，而从一个反复出现的题目阻碍开始：同一段
区间被查询很多次，或者同一批位置被修改很多次，直接枚举会重复计算。

### 10.0 先找重复计算，再选择结构

假设有数组 `a`，需要回答区间 `[l,r)` 的某个聚合结果。先写出暴力：逐项合并，单次
`O(r-l)`。当查询数为 `q` 时，总成本可能达到 `O(nq)`。接下来每一种结构都只是在回答
一个更具体的问题：

| 题目特征 | 首先考虑 | 关键前提 |
|---|---|---|
| 没有修改，只查询和/计数 | 前缀和或静态表 | 查询前可以预处理 |
| 单点修改 + 前缀/区间查询 | `nfenwick` | 区间结果可以用逆操作消去前缀 |
| 单点修改 + 一般有序合并 | `nseg` | 空区间有单位值，合并保持顺序 |
| 区间修改 + 区间查询 | `nlazyseg` | tag 能组合，并能作用到聚合信息 |
| 坐标范围巨大、只写少量点 | `ndynamic_seg` | 未开节点代表单位信息 |
| 坐标范围巨大、还需要区间 tag | `ndynamic_lazyseg` | 额外满足动态长度和 tag 边界 |
| 查询必须看到历史版本 | `npersistent_seg` | 旧节点不可变，更新复制路径 |
| 只读、离线、排序或第 k 小 | wavelet/Mo/sparse 等 | 先确认查询顺序和静态性 |

这张表不是“看到关键词就套模板”。例如 `min` 没有普遍可用的逆操作，因此即使它是
幂等聚合，也不能直接照搬 Fenwick 的区间消去公式；非交换字符串拼接则可以使用普通
线段树，但不能随意交换左右子树的合并顺序。

### 10.0.1 本章统一的节点语言

固定、lazy、persistent、dynamic 线段树都可以暴露一个只读 `nseg_node<S>` 视图；它和
`nnode_view<S>` 共用 `ni::nnode_stamp` 的 owner/domain/epoch/identity 检查：

```text
区间边界 + 区间宽度 + 聚合信息 + 左右子节点 + 当前句柄/epoch
```

这样“从根向下找第一个满足条件的位置”的算法只写一次，由 `nseg_walk` 调度；后端只
负责提供节点能力。动态树的缺失子树代表单位元，不应因为读取而分配节点；持久化树的
旧节点不可变，因此旧版本视图可以继续使用；普通/lazy 树修改拓扑后，旧快照会失效。

先理解这套不变量，再阅读后面的具体类。模板参数只是把这些角色填进去，不是新的学习
目标。

线段树家族共享结构视图 `nseg_node<S>`。`nseg`、`nlazyseg`、`npersistent_seg`、
`ndynamic_seg` 与 `ndynamic_lazyseg` 都提供 `root()`；持久化树另有 `root(version)`：

```cpp
auto node = seg.root();
node.aggregate(); node.info();
node.left_bound(); node.right_bound(); node.width(); node.leaf();
node.left(); node.right(); node.handle();
node.identity(); node.same_domain(other);

auto found = nseg_walk(seg, decide);             // 从 root() 调度
auto old_found = nseg_walk(p.root(version), decide); // 从指定节点调度
```

`decide(node)` 返回 `nbranch::left/take/right`。固定树和持久化树的视图区间是内部
`[0,bit_ceil(n))`，`n` 以后的叶子为单位元；
动态树的视图区间就是构造时给出的坐标域。普通/lazy/dynamic 树修改后旧视图的 epoch 失效；
资源槽位删除并复用时 generation 也会使旧视图失效；持久化节点不可变，所以旧版本视图保持
有效。带 lazy 的节点额外有 `tag()`，它返回当前
节点尚未下推的表示 tag，而不是从根到节点的累计作用；沿 `left()/right()` 取得的子视图会
携带祖先未下推作用，因此 `aggregate()` 仍是该区间的逻辑聚合，且不会为动态树的只读视图
补开缺失节点。

### 10.1 Fenwick：`nfenwick<T,O>`

Fenwick 适合“修改一个位置，并频繁询问前缀”的场景。它把若干相邻位置压进由最低位
决定的块中；一次更新只触碰覆盖该位置的 `O(log n)` 个块，一次前缀查询也只拆成
`O(log n)` 个块。

为什么区间 `[l,r)` 额外需要逆操作？因为实现实际得到的是：

```text
prefix(r) = a[0] op ... op a[r-1]
prefix(l) = a[0] op ... op a[l-1]
```

要从前者消去后者，必须存在真正的 `inv`，而且默认 Fenwick 的块组合还依赖交换性。
加法满足；一般的 `min` 不满足——知道 `min(前缀)` 后无法恢复删掉前缀后的最小值。

`O` 应满足交换结合和单位元；区间查询和单点读取还要求 `O::inv` 存在并实现真正的逆。

```cpp
nfenwick<long long> f(n);
nfenwick<long long> f(a);
f.add(i, delta);
f.prefix(r);          // fold [0,r)
f.fold(l, r);         // 要求 inverse
f.get(i);             // 要求 inverse
f.lower(target);      // 首个使 prefix >= target 的 0-based 位置；不存在 npos
f.clear();
```

`lower` 要求前缀相对比较器单调；默认适用于非负增量的前缀和。更新、查询 `O(log n)`。

**迁移信号：**看到“单点加、前缀统计”先想到 Fenwick；如果合并不可交换、不能消去，
或者题目需要区间 tag，就继续看线段树，而不是勉强给 Fenwick 补接口。

### 10.2 迭代线段树：`nseg<T,O>`

线段树只要求每个父节点能够由“左区间答案”和“右区间答案”合并出来。核心不变量是：

```text
node.aggregate = op(left.aggregate, right.aggregate)
```

区间 `[l,r)` 会被拆成 `O(log n)` 个互不重叠的节点。查询必须按从左到右的顺序合并，
所以字符串拼接、矩阵乘法等非交换操作也合法；只是不能把左右累加器调换。

要求操作有正确单位元并满足结合；合并保持顺序，不要求交换。

```cpp
nseg<string, nconcat> s(source, nconcat{});
s.set(i, value);
s.get(i);
s.fold(l, r);
s.fold();
s.clear();
```

单点与区间操作 `O(log n)`；整体 fold `O(1)`。别名 `nseg_iter`。

两个相同长度的固定树可以消费式做逐点结构合并：

```cpp
left.merge_from(move(right));       // leaf: left_value op right_value
```

这不是对两个 root aggregate 只调用一次 `op`；实现会按叶子合并后重建父节点，因而仍然
保持非交换操作的坐标顺序。代价 `O(bit_ceil(n))`，`right` 被清空，双方旧视图失效。

正确性的承重步骤只有两个：每个节点始终等于自己区间的有序聚合；查询选出的节点恰好
无重无漏覆盖 `[l,r)`。单点修改后沿祖先重算，因而第一个不变量恢复。

### 10.3 Lazy 线段树：`nlazyseg<S,F,M,A>`

若一次修改覆盖很长区间，逐个更新叶子会重新退化到线性。lazy tree 把“这一整段还应
执行什么”存成 tag；只有查询或继续部分更新必须进入孩子时才下推。

这里有两个不同的组合：`M` 合并相邻区间的信息，`A` 合并时间顺序上的修改。若节点先
积累 `older`，后来又收到 `newer`，约定固定为：

```text
compose(newer, older) = 先执行 older，再执行 newer
```

对区间赋值、仿射变换等非交换 tag，写反顺序会产生很小却隐蔽的 WA。`apply` 还必须能
只凭聚合值、tag 和区间长度更新整段信息；做不到这一点的修改不能只靠 lazy tag 表达。

- `S`：聚合类型。
- `F`：tag 类型。
- `M`：`S` 上有单位元的有序结合操作。
- `A`：满足 action 协议。

```cpp
nlazyseg<S,F,M,A> seg(n, merge, action);
nlazyseg<S,F,M,A> seg(source, merge, action);
seg.apply(l, r, tag);
seg.fold(l, r);
seg.set(i, value);
seg.get(i);
seg.clear();
```

更新/查询 `O(log n)`。常用别名：

```cpp
nlazy_addsum<long long> seg(source);
```

`nlazyseg` 也有同长度的消费式 `merge_from(move(other))`。它先下推两棵树的 pending tag，
再按叶子执行 `M(this, other)` 并重建；因此 `compose(newer,older)` 的时间顺序不会被
一个“合并 root”捷径掩盖。复杂度 `O(bit_ceil(n))`，`other` 清空且旧视图失效。

**最小自检：**用两个不同的 tag 连续覆盖同一区间，再与逐元素暴力比较；只测试交换的
区间加法，无法发现 `compose` 顺序写反。

### 10.4 聚合队列：`nqueue_agg<T,O>`

保持队列顺序的双栈聚合：`push`、`front`、`pop`、`pop(fallback)`、
`fold`、`len`、`empty`。`front` 返回 `const T&`；缓存聚合要求已入队元素不可原地改写。
全部操作摊还 `O(1)`，适合滑动窗口最值或非交换聚合。

### 10.5 DSU

`ndsu`：路径压缩 + 按大小合并。

```cpp
ndsu d(n);
d.find(v); d(v);
d.size(v); d.same(a,b);
d.merge(a,b);          // 返回合并后根
d.partition();         // 稠密 class labels
```

摊还逆 Ackermann。

`nrollback_dsu` / `ndsu_rollback`：不做路径压缩，支持 `time/undo/rollback`。
成功 merge `O(log n)` 高度，回滚 `O(1)` 每次；重复 merge 返回 false 且不写历史。

### 10.6 持久化线段树：`npersistent_seg<T,O>`

持久化的机关不是“复制整棵树”，而是旧节点不可变。一次点修改只复制根到叶子的
`O(log n)` 条路径，其余子树与旧版本共享。因此版本号保存的是根句柄，而不是一份数组。

```cpp
npersistent_seg<long long> p(source);
int v1 = p.set(0, index, value);  // 从版本 0 创建新版本
int v2 = p.fork(v1);              // 相同 root 的新版本
p.fold(version, l, r);
p.get(version, i);
p.root(version);                     // nseg_node 结构视图
p.versions(); p.nodes();
p.reserve_nodes(count);
```

点设与查询 `O(log n)`，每次 set 产生 `O(log n)` 新节点。

版本也可以做持久化的**逐点结构合并**：

```cpp
int merged = p.merge(version_left, version_right);
```

它按每个对应叶子执行 `left_value op right_value`，不是把两个区间整体相接；一侧缺失
的子树直接共享，双方都存在的路径才做 path-copy。旧版本和旧 `nseg_node` 不会被改写，
代价是两棵版本树物化节点并集的 `O(U)`。`domain()` 只暴露 append-only 节点域；普通复制
仍会 `clone()`，不会因后续版本追加而和源 owner 共享所有权。

它适合“询问历史状态”或“不同方案从同一版本分叉”。如果题目只是坐标巨大但不需要
旧版本，动态开点通常更直接；持久化和动态开点解决的是两个不同维度。

### 10.7 动态开点线段树

动态开点解决的是**坐标域巨大但实际写入稀疏**。固定线段树会为整个坐标域分配空间；
动态树只在更新路径上建立节点。缺失节点的数学含义必须稳定：它代表整个区间均为聚合
单位元，而不是“未知”或“尚未计算”。

先比较另一条常见路线：如果所有坐标能提前读完，而且操作只依赖顺序，坐标压缩通常
常数更小；若坐标在线产生、必须保留真实区间长度，或不能预先收集，动态开点更自然。

`ndynamic_seg<T,O>` 在 `long long` 坐标域 `[lo,hi)` 上只为写入路径开点；未开节点表示
合并操作的单位元：

```cpp
ndynamic_seg<long long> seg(-1'000'000'000'000LL, 1'000'000'000'000LL);
seg.set(x, value);
seg.combine(x, delta);       // leaf = op(old, delta)，顺序不可交换
seg.get(x); seg.fold(l, r); seg.fold();
seg.nodes(); seg.reserve_nodes(capacity); seg.clear();
```

多个 owner 可以显式共享 `domain()`，再做消费式逐点合并：

```cpp
auto domain = ndynamic_seg<long long>(lo, hi).domain();
ndynamic_seg<long long> a(domain, lo, hi), b(domain, lo, hi);
a.merge_from(move(b));
```

`merge_from` 要求同一 domain、相同坐标界、root 不重叠以及 `O` 的语义等价；它对对应叶子
按 `a_value op b_value` 合并，缺失节点是单位元，不是把两个区间的 aggregate 直接做一次
`op`。这是 destructive merge，访问两个物化结构的并集，成功后 `b` 为空且共享 epoch 使
旧结构视图失效。复制动态树会 clone domain，因此不会意外把后续写入传回源树。

设坐标域宽度为 `W`：点修改和区间查询为 `O(log W)`，每个首次写入的点至多增加
`O(log W)` 节点；只读查询不分配节点。坐标域宽度必须能装入 `long long`。

`ndynamic_lazyseg<S,F,M,A>` 使用与 `nlazyseg` 相同的合并/作用协议，支持动态开点
区间 tag：

```cpp
ndynamic_lazyseg<S,F,M,A> seg(lo, hi, merge, action);
seg.apply(l, r, tag);
seg.fold(l, r); seg.set(x, value); seg.get(x);

ndynamic_addsum<long long> sum(lo, hi); // 区间加、区间和
```

更新、查询均为 `O(log W)`；查询通过携带未下推 tag 计算，不为缺失子树开点。更新在下推
已有 tag 时可能建立两个孩子，总空间仍为 `O(q log W)`。由于统一 action 的 length 是
`int`，lazy 动态树要求 `hi-lo <= INT_MAX`；坐标本身仍可为 `long long`。

lazy 动态树也支持同域 owner 的 `merge_from(move(other))`。两棵树在双方根都存在的区间先
下推 pending tag，再逐点按 `M` 合并；单侧缺失时直接转移子树，不因合并读取而伪造节点。
因此非交换 `M` 与非交换 action 的先后都保留在局部契约中，合并复杂度为物化节点并集
`O(U)`（加上为暴露 pending tag 所需的开点）。

**最小反例方向：**记录 `nodes()`，执行大量纯查询后它必须不增长；对 lazy 动态树再
测试“整域加、局部查询”，确认缺失子树能继承逻辑 tag，而不需要为了读取补开节点。

### 10.8 Wavelet Matrix：`nwavelet<T>`

静态 integral 序列，支持有符号顺序编码。

```cpp
nwavelet<int> w(a);
w.kth(l, r, rank);                 // rank 从 0 开始
w.count_less(l, r, bound);
w.count(l, r, value);
w.count(l, r, low, high);          // 值域 [low,high)
w.predecessor(l, r, bound);        // 最大 < bound
w.successor(l, r, bound);          // 最小 >= bound
```

时间 `O(bit-width(T))`，可选结果使用 `nmaybe<T>`。

不要把它想成“神秘的第 k 小模板”。构建时从最高位到最低位，对当前序列做稳定的
`0-bit | 1-bit` 分组，并记录每个前缀中有多少个 0。稳定分组保护了一个关键事实：原区间
`[l,r)` 中落到 0 组或 1 组的元素，在下一层仍各自形成连续区间。于是查询只需用前缀计数
把 `[l,r)` 映射到下一层，不必保存每个查询的一份子序列。

求第 `rank` 小时，若区间内 0 的数量大于 `rank`，答案当前位为 0 并进入 0 组；否则答案
当前位为 1、扣掉全部 0 后进入 1 组。`count_less` 做的是同一棵决策树：当 bound 当前位
为 1，当前区间所有 0 都可以立刻计入答案。有符号整数会先翻转符号位，使编码的无符号
字典序等于原数值顺序；直接按二进制解释负数会把它们错误地排到正数之后。

构建时间和空间都是 `O(n * bit-width(T))`。它只描述构造时的静态序列；出现在线修改时，
应重新判断能否离线、改用树套结构，或换成题目特定的统计方法，而不是期待 `kth` 暗中
维护修改。最小自检应同时包含负数、重复值、空值域计数以及不存在的前驱/后继。

### 10.9 Mo 调度

```cpp
struct ninterval_query { int left, right, id; };
auto order = nmo_order(queries, universe);

nrun_mo(queries, universe,
    add_left, add_right, remove_left, remove_right, answer);

nrun_mo(queries, universe, add, remove, answer); // 左右操作相同
```

每个查询必须满足 `0 <= left <= right <= universe`。`answer` 接收 query 的 `id`。
调度器按引用转发回调，不复制回调状态；简化重载的左右移动会调用同一个 `add/remove`
对象，也支持不可复制 functor。

Mo 不是一种区间答案公式，而是一种**移动窗口的离线顺序**。处理某个查询前必须始终
维护：当前状态恰好等于半开区间 `[left,right)` 的答案。四个方向的回调与边界变化顺序是：

```text
--left  后 add_left(left)       把新左端加入
add_right(right) 后 ++right     把旧 right 位置加入
remove_left(left) 后 ++left     删除旧 left 位置
--right 后 remove_right(right)  删除旧 right-1 位置
```

若答案依赖顺序，左右回调不能合并；例如维护字符串、方向边或端点贡献时，左侧加入与右侧
加入不是同一运算。只有状态是无序 multiset 统计时，三回调简化重载才自然。

令 `U=universe`。当前块宽约为 `sqrt(U)`，蛇形排序使左端总移动量至多约 `O(q sqrt U)`，
右端至多约 `O(U sqrt U)`；再乘以一次 add/remove 的真实成本。若一次回调本身是
`O(log U)`，总复杂度也要带上这项。Mo 要求查询全部预先可见、更新可逆且单次移动便宜；
在线回答、难以删除
元素或只有很少查询时，预处理/树结构往往更合适。

### 10.10 Disjoint Sparse Table：`nsparse<T,O>`

`nsparse` 的名字兼容经典 sparse table，但实现是 disjoint sparse table：只要求操作有
单位元且结合，不要求幂等或交换。因此字符串拼接等操作也能 `O(1)` 查询：

```cpp
nsparse<string, nconcat> table(words);
table.fold(l, r);                 // [l,r)，空区间返回 op.id()
table.fold(l, r, fallback);       // 空区间改用 fallback
table[i];
```

预处理 `O(n log n)` 时间和空间，非空区间查询 `O(1)`。合并严格保持从左到右的顺序；
若只需 `min/max/gcd`，默认 `nmin<T>` 仍可直接用。

### 10.11 带势能并查集：`npotential_dsu<T>`

维护可加群意义下的差值约束：

```cpp
npotential_dsu<long long> d(n);
d.bind(left, right, delta); // 要求 potential(right)-potential(left)==delta
auto x = d.diff(left, right);
```

`bind` 在两个连通块间合并；同块时只检查新约束是否一致并返回 `bool`。`diff` 在不连通
时返回空 `nmaybe`，另有 fallback 重载。类型需支持 `+=`、一元负号和相等比较。
路径压缩与按大小合并给出摊还逆 Ackermann 复杂度。

### 10.12 区间结构选择检查点

三种经常混淆的“树”可以先按坐标、时间和顺序三个维度拆开：

| 结构 | 元素怎样定位 | 是否保留历史 | 顺序能否变化 | 主要空间来源 |
|---|---|---|---|---|
| `npersistent_seg` | 固定下标 | 是，版本根分叉 | 否 | 每次点改复制 `O(log n)` 路径 |
| `ndynamic_seg` | 巨大真实坐标 | 否 | 否 | 实际写入路径 `O(q log W)` |
| `nimplicit_fhq` | 当前序列位置 | 否 | 是，split/merge 重排 | 每个当前元素一个节点 |

“dynamic”说的是坐标节点按需分配，不等于保留旧状态；“persistent”说的是旧节点不可变，
不等于位置顺序可搬运；“implicit”说的是 key 被子树长度替代，不等于坐标域巨大。先确定
题目变化的是哪一维，再选结构，通常比背类名稳定。

1. 为什么 Fenwick 的区间 fold 需要逆操作，而线段树不需要？
2. 非交换操作进入 `nseg` 时，查询必须保护什么顺序？
3. lazy action 的 `compose(newer,older)` 表示哪一个时间顺序？
4. 动态开点的缺失节点代表什么，为什么只读查询不能开点？
5. persistent 与 dynamic 分别解决“历史”和“稀疏”哪个维度？
6. 一个 `nseg_node` 快照在什么修改后失效，持久化旧版本为什么例外？
7. Wavelet 每一层稳定分组后，原查询区间为什么还能映射成连续区间？
8. Mo 的四个回调分别在边界变化前还是变化后接收哪个位置？
9. 为什么位置会被搬运的序列不能用普通动态开点线段树自然表达？

能回答这些问题，才算真正掌握本章的选择路径；只会写构造函数还不足以安全换后端。

---

## 11. 图、树、流与匹配

图算法的第一步不是选 BFS 还是 Dijkstra，而是把“邻居”定义清楚：图是否有向、边权
是否非负、树是否真的连通、邻接对象是否拥有或借用。Nitori 的图算法统一接受可枚举
邻接对象，因此显式边表和按需生成的网格邻居可以共用同一套遍历逻辑。

建议按下面顺序判断：

```text
只关心可达性              → BFS/DFS
无权最短路                → BFS
非负权最短路              → Dijkstra
边方向有拓扑顺序          → topo/SCC
连通代价                  → MST
树上路径/子树聚合         → LCA/HLD/reroot
容量与守恒                → flow/matching
```

每个入口都要把题目条件写在算法名字前面；例如 Dijkstra 的“边权非负”不是库风格，而是
松弛后已确定距离不会被更短路径推翻的证明前提。

### 11.1 统一图协议

边可以是整数目的点，或具有 integral `.to` 和可选 arithmetic `.weight` 的对象。
目的点转成 `int`、权值转成算法指定类型时都会先检查表示范围；窄化不会靠回绕
伪装成另一个合法顶点或权值。

```cpp
template<class W=int> struct narc { int to; W weight; };
nedge_to(edge);
nedge_weight(edge); // integral edge 的默认权为 1
```

图满足：

```cpp
graph.vertices();
graph.neighbors(vertex); // 返回任意可枚举邻接对象，可按值或按引用
```

`vertices()` 必须返回 integral；算法在转成 `int` 前验证表示范围。概念名：
`ngraph_like<G>`。

### 11.2 显式和隐式图

```cpp
ngraph<long long> g(n, expected_edges); // 默认 ngraph_forward
int id = g.add(u, v, w);                // 有向弧，返回稳定 edge id
auto [a,b] = g.add2(u, v, w);           // 两条反向弧
g.len(); g.edges(); g.degree(u);
g.neighbors(u); g.arcs(); g.vertices();
g.find(u,v); g.has(u,v); g.weight(id); g.set(id,new_weight);
```

三种显式后端：

| 类型 | 拓扑 | 适用场景 |
|---|---|---|
| `ngraph_forward<W>` / `ngraph<W>` | forward-star，动态追加弧 | 默认竞赛图；低常数、edge id、权值可改 |
| `ngraph_csr<W>` | CSR，构造后拓扑静态 | 密集遍历、缓存局部性；可由任意 `ngraph_like` 构建 |
| `ngraph_list<W>` | `vector<vector<narc<W>>>` | 最朴素参考/隐式接口对照 |

`ngraph_forward/csr` 的规范弧是 `nedge<WRef>{from,to,id,w}`，遍历可直接修改非 const
图的 `edge.w`；`ngraph_list` 的邻接项是 `narc<W>{to,weight}`。高层算法只通过
`nedge_to/nedge_weight` 消除表示差异。`reverse()` 返回反图；CSR 仍只冻结拓扑，
`set/weight` 可以修改权值。

隐式图：

```cpp
auto grid_graph = ngraph_view(rows * cols, [&](int v) {
    return neighbors_of(v); // 任意可枚举值/引用对象
});
```

算法依赖 `ngraph_like`，不依赖 `ngraph_list`。算法保留 `neighbors` 的返回类别：按引用
返回的邻接对象不会被隐式复制，按值生成的临时邻接对象则存活到本次遍历结束。

通用枚举桥：

```cpp
auto vertices = nvertices(graph); // nrange(0,V)
auto arcs = narcs(graph);          // 统一 nedge view，借用左值 graph
```

`narcs` 逐顶点展开邻接，并正确接管 `neighbors(v)` 按值返回的临时邻接 view；它拒绝从
临时 graph 借用。若后端有自身 `arcs()`，通常可少一次通用适配。

过滤不会复制图或邻接：

```cpp
auto light = ngraph_where(graph, [](const auto& e) {
    return nedge_weight(e) <= limit;
});
auto d = ndijkstra(light, source);
```

`ngraph_where` 是只读 `ngraph_like`，保留所有顶点，只过滤弧，并借用左值 graph；谓词
按值存入 view。构造 `O(1)`，遍历成本等于扫描原弧，`edges()` 因而是 `O(V+E)`。

### 11.3 遍历与最短路

| API | 前提 | 返回 | 复杂度 |
|---|---|---|---|
| `nbfs(g,s)` | 无权图 | `npos` 表示不可达的距离 | `O(V+E)` |
| `nbfs_path(g,s)` | 无权图 | 距离、父亲和路径恢复 | `O(V+E)` |
| `n01bfs(g,s)` | 权仅 0/1 | `npos` 表示不可达 | `O(V+E)` |
| `ndijkstra<D>(g,s,inf)` | 权非负，距离严格小于 `inf` 且和不溢出 | 距离向量 | `O((V+E)logV)` |
| `ndijkstra_path<D>(g,s,inf)` | 同上 | 距离、父亲和路径恢复 | `O((V+E)logV)` |

`ndijkstra` 的默认 `inf` 使用 `D` 的真实顺序上界（浮点为正无穷），而不是保留余量的
`ninf<D>`。`inf` 本身必须非负且不能为 NaN；若合法最短路可能等于该哨兵，请改用
更宽的距离类型或显式策略。

`npath_result<D>` 公开 `d`、`p`、`bad`，并提供 `reach(v)`、`dist(v,fallback)`、
`operator[]` 和 `path(v)`；源点父亲指向自己，不可达点路径为空。

选择这些接口时，证明的核心分别是：

- BFS 出队顺序按距离非降；第一次发现顶点时已经是最短边数。
- 0-1 BFS 只把新距离放入双端队列的前端或后端，队列中的距离差保持在 1 以内。
- Dijkstra 每次从堆中取出的最小暂定距离，在非负边权条件下不可能再被未处理路径改小。

负边会破坏最后一条不变量；“图没有负环”仍不够，普通 Dijkstra 也不能直接接受负边。

### 11.4 DAG、SCC 与 LCA

```cpp
auto order = ntoposort(g);          // 有环返回空 nmaybe
auto same = ntopo(g);               // 同义；另有 fallback 重载
auto c1 = nscc(g);                  // 默认迭代 Kosaraju
auto c2 = nscc_kosaraju(g);         // 显式同后端
auto c3 = nscc_tarjan(g);           // 递归 Tarjan
```

拓扑排序和 SCC 均为 `O(V+E)`。`nscc_tarjan` 的递归深度可能达到 `V`；极深图优先默认
迭代 Kosaraju。

SCC 的证明目标不是“DFS 恰好染成几种颜色”，而是把互相可达关系压成等价类。压缩后，
任意跨分量边 `u -> v` 变成 `component[u] -> component[v]`；若缩点图仍有有向环，环上
分量本应互相可达，和“已经是最大强连通分量”矛盾，所以 condensation 一定是 DAG。
`npartition` 的数字类号只是表示标签，不承诺拓扑先后；需要在缩点图上做 DP 时，显式
建边后再 `ntoposort`，不要把类号大小当拓扑序。平行缩点边是否去重由后续算法决定：
求可达性可以保留，统计不同关系时通常要去重。

Kosaraju 用原图完成顺序和反图遍历换取迭代稳定性；Tarjan 在一次 DFS 中用 discovery/
low-link 和栈识别“尚未离开当前搜索区域”的分量，内存路线更直接但递归深度是实际风险。
二者得到的数字标签可以不同，只应比较 `same(a,b)` 代表的分区是否相同。

`nlca tree(g, root)` 要求图严格描述一棵以 root 为根的树，接受两种且仅两种存储：

- 每个父到子恰好一条弧，共 `V-1` 条；
- 每条树边恰好一对反向弧，共 `2(V-1)` 条。

普通连通图、额外边、自环、漏反向或重复弧不会被静默解释成某棵 BFS 树。

```cpp
tree.depth(v);
tree.jump(v, steps);          // 超过根返回 npos
tree(a, b);                   // LCA
tree.distance(a, b);
tree.kth_on_path(a, b, k);    // 超出路径返回 npos
```

构建 `O(V log V)`，查询 `O(log V)`。

`nhld` 提供另一种树上桥梁：

```cpp
nhld h(tree, root);
h.position(v); h.vertex(pos);
h.lca(a,b);
auto segments = h.path(a,b, edge_mode);
auto [l,r] = h.subtree(v, edge_mode);
h.each(a,b, callback, edge_mode);
```

`nhld_segment{l,r,rev}` 始终使用 `[l,r)`；`rev` 指明该段在原路径上是否逆序，非交换
路径聚合必须保留它。构建 `O(V+E)`，一条路径拆为 `O(log V)` 段。输入和 `nlca` 一样
必须是严格树（单向父子弧或成对双向弧）。

`nlca_binary<W>` 是 v1 风格的带权/森林脚手架，接口为 `lca/dist/kth/jump`。它按 BFS
建立每个连通块，调用者必须保证输入本来就是森林；与严格 `nlca` 不同，它不审计额外
环边。新代码需要拓扑诊断时优先 `nlca`，需要带权距离或多棵树时可用
`nlca_binary<W>` 并显式承担森林前提。

树上路径题的选择信号是：只问祖先关系或距离，LCA 足够；路径上还要反复区间修改/聚合，
才把树映射到 HLD 的位置区间。`nhld_segment.rev` 不是附加装饰：如果路径聚合非交换，
反向段必须先翻转，再按原路径方向合并。

先用最朴素的顶点序列看懂 `rev`：

```cpp
nvector<int> path_vertices;
nfor(segment, h.path(a, b)) {
    if (segment.rev) {
        for (int p = segment.r; p-- > segment.l; )
            path_vertices.push(h.vertex(p));
    } else {
        for (int p = segment.l; p < segment.r; ++p)
            path_vertices.push(h.vertex(p));
    }
}
```

`h.path(a,b)` 已经按从 `a` 到 `b` 的段顺序返回；`rev` 只说明某一段在 base array 中需要
倒读。要把上面逐点版本升级为 `O(log^2 V)` 的非交换 fold，每段摘要必须存在题目定义的
`O(1)` 方向重排，或使用其他足以重排的状态；若仅凭一个前向摘要无法恢复倒序答案，那是
信息不足，不能无视 `rev`，也不应让 HLD 核心臆造统一的“双份聚合”。

### 11.5 Rerooting

```cpp
auto answer = nreroot(
    tree,
    identity,
    merge,
    vertex,
    lift,
    root
);
```

回调语义：

- `merge(left,right)`：按邻接顺序合并贡献，可非交换。
- `vertex(aggregate, v)`：把所有邻边贡献转成以 `v` 为根的状态。
- `lift(state, from, to)`：把 `from` 的状态变成对 `to` 的边贡献。

输入必须是双向存储的树：每条无向边必须恰好提供一对反向弧，弧数严格为
`2(V-1)`，连通、无自环且无额外环。checked profile 会验证这套树拓扑契约；仅满足
弧数而方向或重数错误也会失败。复杂度 `O(V+E)` 次回调，prefix/suffix 保持 merge 顺序。

为什么第二遍需要 prefix/suffix？设顶点 `v` 按邻接顺序收到贡献
`c[0],c[1],...,c[d-1]`。给第 `i` 个孩子的父侧状态必须排除 `c[i]`，暴力为每个孩子重算
会在星形树退化成 `O(d^2)`。预处理：

```text
prefix[i]   = c[0] merge ... merge c[i-1]
suffix[i]   = c[i] merge ... merge c[d-1]
without(i)  = merge(prefix[i], suffix[i+1])
```

于是每条边只做常数次合并，总复杂度线性。即使 `merge` 不交换，这个公式也保持原邻接
顺序；把 suffix 的合并方向写反，会在加法测试中被交换性掩盖，因此至少再用字符串或
顺序敏感状态做一次小测试。

### 11.6 MST：Prim 与 Kruskal

```cpp
auto mst = nprim<long long>(g, root);
if (mst) {
    auto weight = mst->weight;
    auto edges = mst->edges;
}
```

无连通生成树返回空。无向边必须以对称弧提供；边权转换和总权累加必须能由 `D`
表示，checked profile 会验证。复杂度 `O(E log E)`。

Kruskal 同时支持不连通图，返回最小生成森林：

```cpp
auto forest = nkruskal(g);
auto custom = nkruskal(g, [](const auto& edge) { return cost(edge); });

forest.weight;       // 同义兼容字段 forest.cost
forest.edges;        // 选中的 (from,to)
forest.edge;         // 对应规范弧 id
forest.components;
forest.connected();
```

复杂度 `O(E log E)`。若无向边以两条对称弧提供，DSU 会只选择其中一条；平行边允许。
`nprim` 的 `nmst_result` 也填 `weight/cost/edges/components`，但不保证可还原通用图的
规范 edge id。

Prim 的安全性来自“当前树割中最轻的跨割边可以加入某棵最小生成树”；Kruskal 的安全性
来自按权排序后，每条不会形成环的边都能被加入某棵最小生成树。输入若是有向弧，必须
先确认题目意义确实代表无向边；不能只因为接口接受弧就把有向图当无向图。

### 11.7 最大流

默认 `nflow<C>` 是可复用 Dinic 后端 `nflow_dinic<C>`：

```cpp
nflow<long long> flow(n, expected_edges);
int id = flow.add(u, v, capacity, reverse_capacity);
auto sent = flow(source, sink, limit); // 同 flow.flow(...)
flow.used(id);
auto side = flow.cut(source);          // 当前残量图中 source 可达
flow.reset();                          // 恢复初始容量
```

多次 `flow` 会在当前残量图上继续增广；`reset` 后可重算。只有在一次无 `limit` 的增广
确实达到最大流后，`cut(source)` 才具有最小割语义。容量是排除 `bool` 的非负整数，
`source != sink`，残量与累计流不得溢出。一般网络的标准最坏界 `O(V^2E)`；DFS 递归
深度最坏为 `V`。

`nmaxflow<C>` 保留独立的非递归 push-relabel 实现，适合与 Dinic 对拍或特定稠密图：

```cpp
nmaxflow<long long> reference(n);
reference.add(u,v,c);
auto value = reference.flow(s,t);
auto side = reference.mincut(s);
```

它是一次性对象：开始 `flow` 后不能再 `add` 或再次求流。顶点数不超过 `INT_MAX/2`，
基础最坏界 `O(V^3)`。

流算法维护的是残量网络：一条正向边的剩余容量和反向边的可退流量共同表示当前解。
只有在残量图中源点到汇点不可达时，增广才终止；此时可达点集合给出割，流值等于割容量。
`cut` 因而必须在完整最大流后解释，限流或尚未结束的中间状态不能直接当最小割。

### 11.8 二分图最大匹配

图对象只表示左侧邻接，邻接目的点是 `[0,right_vertices)` 的右侧编号。

```cpp
auto result = nhopcroft_karp(left_graph, right_vertices);
result.size;
result.left[u];   // 匹配的右点或 npos
result.right[v];  // 匹配的左点或 npos
```

实现非递归，复杂度 `O(E sqrt(V))`，内部复制一次邻接以支持显式栈分层搜索。

需要逐条加边、重复求解和最小点覆盖时使用状态对象：

```cpp
nbimatch matching(left_vertices, right_vertices); // nbimatch_hopcroft
matching.add(l, r);
int size = matching.solve();
matching.left(l); matching.right(r);
auto pairs = matching.pairs();
nbicover cover = matching.mincover(); // cover.l / cover.r
```

每次加边会令旧解失效；`pairs/mincover` 要求之后已经 `solve()`。`mincover` 使用
Kőnig 定理从最大匹配恢复最小点覆盖，额外 `O(V+E)`。

Hopcroft–Karp 的分层 BFS 只保留最短增广路，DFS 在这些层上寻找一批互不冲突的增广路；
分层长度每轮增长，因而达到 `O(E sqrt(V))`。最小点覆盖的恢复依赖“最大匹配已经完成”，
并非任意当前匹配都能直接调用 `mincover`。

### 11.9 图与树检查点

1. 一个按值生成的 `neighbors(v)` 临时对象为什么能在一次遍历中安全使用，又为什么不能
   把其中引用保存到下一轮？
2. Dijkstra 的非负边前提具体保护了“哪个已确定量不会再变小”？
3. SCC 类号为什么不能直接当缩点 DAG 的拓扑序？Kosaraju 与 Tarjan 应比较什么结果？
4. HLD 的 `rev` 描述的是段返回顺序，还是 base array 内的读取方向？
5. reroot 排除一个孩子时，为什么 `prefix[i] merge suffix[i+1]` 不会改变非交换顺序？
6. Dinic 限流返回以后，`cut(source)` 为什么还不能自动解释成最小割？
7. `mincover()` 为什么要求当前匹配已经达到最大，而不是仅仅合法？
8. 输入有 `2(V-1)` 条弧为什么仍不足以证明它是一棵正确的双向树？

---

## 12. 整数、模运算、组合与博弈

数学工具的危险点通常不是公式本身，而是前提被省略。看到除法、逆元、概率期望、SG
或 NTT 时，先检查对象所在的数值域和状态图，再选择 API。本章将“能调用的接口”和
“必须由题目保证的性质”并排说明，避免把类型名当成证明。

### 12.1 整数基础

`ninteger<T>` 表示排除 `bool` 的整数类型，并包含 GNU++20 下的 128-bit 整数。

| API | 语义 |
|---|---|
| `nmag/nabs` | 无符号绝对值，能表示有符号最低值的幅度 |
| `ngcd` | 默认二进制 gcd；非负无符号结果 |
| `ngcd_euclid/ngcd_binary` | 显式选择欧几里得/二进制后端 |
| `nlcm` | 检查无符号结果溢出 |
| `nfloor_div/nceil_div` | 数学向下/向上整除，除数不可零 |
| `nmodulo(value,modulus)` | 标量余数规范到 `[0,modulus)`，modulus > 0 |
| `nextgcd(a,b)` | 至多 64-bit 输入；`{gcd,x,y}` 满足 `ax+by=gcd`，系数为 `__int128_t` |
| `nmulmod` | uint64 乘法模，内部 `__uint128_t` |
| `npowmod` | uint64 模快速幂 |
| `nisprime` / `nisprime_miller` | 对全部 uint64 确定性的 Miller–Rabin bases |
| `nisprime_trial` | 试除参考实现 |
| `nprimes(limit)` | 线性筛返回 `<= limit` 的所有素数 |
| `npollard(value)` | Pollard-Rho 找一个非平凡因子；质数返回自身 |
| `nfactor/nfactor_rho` | uint64 完整质因数，按非降序且保留重数 |

`nmod(x,m)` 不再是标量函数，因为 Nitori X 保留模整数类型名 `nmod<M>`；机械迁移
标量余数时必须改成 `nmodulo(x,m)`，不能让同名承担两个互斥语义。

整数辅助函数的教学重点是“数学定义”和 C++ 截断的区别。`nfloor_div(-5,2)` 应为 `-3`，
不能直接依赖有符号整数 `/` 的向零截断；`nmodulo(-1,m)` 规范到 `[0,m)`，便于后续
同余状态直接作为数组下标。所有乘法、lcm 和系数回代仍要先估计表示范围。

### 12.2 静态与动态模整数

```cpp
using mint = nmodint<998244353>;
mint a = -3;
a.val();
a += b; a -= b; a *= b; a /= b;
auto p = a.pow(exponent);
auto inv = a.inverse();       // gcd(a,M)!=1 时为空
auto same = a.tryinv();       // 兼容同义接口
auto safe = a.inv(fallback);  // 不可逆时 fallback
auto raw = mint::raw(value);  // 要求 value < M
mint::mod();
```

除法要求被除数存在乘法逆元；模数不必为质数，但 composite modulus 下并非每个非零值
都可逆。`nadd` 的逆元实现只在模数或值域允许时有意义；调用者必须确认实际运算不会溢出。

别名：

```cpp
nmodint<M>       // 核心静态实现
nmod_static<M>   // 显式后端名
nmod<M>          // v1 兼容短名

ndmod<Tag>       // nmod_dynamic<Tag>
ndmod<Tag>::setmod(modulus);
```

每个动态 `Tag` 拥有独立的进程内模数，所有该类型对象共享它；`setmod` 要求正模数且
不应在已有对象仍参与计算时改模。动态模的质数性只能在运行时确认；需要域运算的算法
必须由调用者先检查模数和可逆性。

模除法的推导只有在 `gcd(x,M)=1` 时成立：存在 `y` 使 `xy ≡ 1 (mod M)`，于是
`a/x` 才能解释成 `a*y`。素数模下非零元素都可逆；合数模下例如 `2 mod 4` 不可逆。
`inverse/tryinv/inv(fallback)` 的差别只是结果表达，不会改变这个前提。

### 12.3 阶乘组合表 `ncomb<Mint>`

```cpp
ncomb<mint> c(max_n);
c.factorial(n);
c.choose(n, k);   // k 越界返回 0
c.permute(n, k);  // k 越界返回 0
```

构造要求 `factorial(max_n)` 可逆。素数模下常见充分条件是 `max_n < modulus`。
预处理 `O(n)`，查询 `O(1)`。

组合表把 `C(n,k)=n!/(k!(n-k)!)` 的重复阶乘和逆阶乘计算缓存下来。`max_n` 过大或
模数使 `factorial(max_n)` 不可逆时，checked 构造会拒绝，unsafe 中则不属于有效输入；
此时组合公式本身也不再适用，需要 Lucas、质因数计数或其他题目特定方法，不能只把数组
开大。

### 12.4 子掩码与集合变换

```cpp
nfor(sub, nsubmasks(mask)) {
    // 从 mask 递减枚举到 0，包含 0
}
```

`mask` 为无符号整数，popcount 最多 30。

长度必须是非零二次幂：

```cpp
nzeta_subset(a);      nmobius_subset(a);
nzeta_superset(a);    nmobius_superset(a);
nfwht_xor(a, inverse);

auto c1 = nconv_or(a,b);
auto c2 = nconv_and(a,b);
auto c3 = nconv_xor(a,b);
```

变换与 OR/AND/XOR 卷积均为 `O(n log n)`。整数 XOR 逆变换依赖每项能被 `n` 精确整除；
模类型要求 `n` 可逆。

两种 zeta 与 XOR 变换本质上都是“按每一位组合两个子空间，再按相反步骤恢复”。长度不是二次幂时，
位分层无法覆盖所有状态；XOR 逆变换则额外需要除以长度。先确认状态空间是完整的
`[0,2^k)`，再选择变换方向。

方向可以用一个两元素关系表记住，而不是背函数名：

| 目标 | 每个状态从哪里收集 | 更新形状 |
|---|---|---|
| subset zeta | `sub ⊆ mask` 的子集 | `a[mask] += a[mask ^ bit]`（有 bit 才更新） |
| superset zeta | `mask ⊆ super` 的超集 | `a[mask] += a[mask | bit]`（无 bit 才更新） |
| subset/superset Möbius | 对应 zeta 的逆 | 把 `+=` 换成 `-=`，位循环相同 |
| XOR/FWHT | 两个半块 | `(x,y) -> (x+y,x-y)`，逆变换最后除以 `n` |

例如 OR 卷积的系数是 `c[mask] = Σ_{i|j=mask} a[i]b[j]`，它先做 subset zeta；AND 卷积
则对应交集关系，使用 superset zeta。把两者方向对调，在全 0/全 1 的对称样例上可能看不
出错，最好用只有一个 bit 的不对称小数组手算。

### 12.5 分数、同余与筛表

```cpp
nfrac<long long> x(numerator, denominator); // 自动约分，分母规范为正
x.p; x.q;                                  // 公开规范化分子/分母
x.floor(); x.ceil();
x.trydiv(y);                               // 除零返回空

ncongruence c(a, m);       // x == a (mod m)，m > 0，a 自动规范
c.has(x);
auto merged = ncrt(c1, c2); // 不相容为空；不要求模数互质
```

`nfrac<T>` 支持四则、比较、一元负号与 fallback 除法；中间算术仍受 `T` 表示范围约束，
不是任意精度有理数。`ncrt` 使用 `int64_t` 模数/余数并检查合并后的 lcm 可表示。

CRT 合并不是简单把两个余数相乘：先检查 `a1 ≡ a2 (mod gcd(m1,m2))`，不相容时没有
解；相容后模数变成 lcm。模数不互质并不会自动失败，但必须走这一步 gcd 条件。

手算一个非互质例子最能固定公式：

```text
x ≡ 2 (mod 6),  x ≡ 5 (mod 9)
gcd(6,9)=3，且 2 ≡ 5 (mod 3)，所以可能相容。
x = 2 + 6k，代入第二式得 6k ≡ 3 (mod 9)
除以 gcd 后：2k ≡ 1 (mod 3)，故 k ≡ 2 (mod 3)
x ≡ 14 (mod lcm(6,9)=18)
```

若把右式改成 `x ≡ 1 (mod 9)`，模 3 的余数变成 `2` 与 `1`，第一步就判定无解。这个
检查必须先于求逆；“模数不互质”本身不是失败条件，“余数不相容”才是。

```cpp
nprime_table sieve(limit);
sieve.p;             // 素数表
sieve.phi[x];
sieve.mu[x];
sieve.isprime(x);
sieve.factor(x);     // vector<pair<prime,exponent>>
sieve.divisors(x);   // 正因数
```

线性筛预处理 `O(limit)` 时间和空间；表内分解复杂度与质因数个数相关。

### 12.6 概率、Nim、异或基与 SG

`nprob<P>` 是有限离散权重 owner，不偷偷假设已归一化：

```cpp
nprob<double> p{1,2,3};
p.sum(); p.nonnegative();
auto q = p.normalized();                 // 总权非正或含负权时为空
q->expect([](int i) { return value(i); });
int sample = p.draw(rng, fallback);       // 另有全局 nrng 重载
auto e = nexpect(p, evaluator);
```

归一化和期望为 `O(n)`；抽样当前也是线性扫描，适合小状态/随机台架，不是 alias table。

```cpp
nxorbasis<uint64_t> basis;
basis.ins(x); basis.has(x); basis.max(seed); basis.len();

nnim<unsigned> game(heaps);
game.nim_sum(); game.win(); game.winning(); // 返回 (heap,new_value)
```

异或基每次操作 `O(bit-width)`。`nnim` 的 heap 类型必须无符号，普通 Nim 的必胜修改由
异或和直接恢复。

```cpp
auto grundy = nsg_dag(game_graph); // 有环返回空 nmaybe
auto same = nsg(game_graph);       // 默认同义
```

每个顶点的后继必须是合法状态；实现先拓扑排序，再取 mex，总复杂度
`O(V+E+Σ outdegree)` 及相应临时空间。一般有环博弈不能直接套 SG DAG。

期望的线性性只允许把同一概率模型下的有限权重逐项加权；`nprob` 不会替你归一化，也
不会把负权解释为概率。Nim 的必胜条件依赖每一步只减少一个堆且没有额外规则；改变移动
集合后应重新建模状态图，再考虑 SG，而不是继续读取 `nim_sum()`。

### 12.7 数学工具检查点

1. `nfloor_div(-7,3)` 为什么不是 C++ `/` 的结果？余数规范化后能直接当数组下标需要
   哪个区间契约？
2. 合数模下非零元素为什么可能不可逆？`inverse()` 为空时，算法层应如何改写？
3. `ncomb` 的阶乘表为什么需要可逆性，而不是只需要模数非零？
4. subset zeta 与 superset zeta 各自累积哪一种包含关系？如何用一个单 bit 反例区分？
5. CRT 合并的第一步 gcd 检查是什么；合并后的模数为什么是 lcm 而不是乘积？
6. `nprob` 的权重何时才是概率？期望的线性性隐含了哪些有限性条件？
7. Nim 改变允许操作后，为什么原来的异或和结论不能继续使用？SG 的状态图需要满足什么？

---

## 13. 矩阵、线性代数与多项式

矩阵章节有两条不同的主线：`nmatrix` 解决拥有、切片和布局；矩阵算法解决维度、运算
和数值域。先确定“我要操作哪一块存储”，再确认加法、乘法、零元、单位元和可逆性，
不要把 `nmat` 的短运算符当成自动证明。

### 13.1 `nmatrix<T>`

row-major 拥有型矩阵：

```cpp
nmatrix<int> a(rows, cols);
nmatrix<int> b(rows, cols, initial);
nmatrix<int> c{{1,2}, {3,4}};

a.rows(); a.cols(); a.len(); a.empty(); a.data();
a(r,c); a[flat_index];
a.view();          // 二维 nview
a.row(r);          // 连续一维 nview
a.column(c);       // 非连续一维 nview
a.diagonal(offset);// 一维 nview，正数向上，负数向下
```

`view/row/column/diagonal` 只允许从左值矩阵借用。它们与自由函数
`nrow/ncolumn/ndiagonal` 使用同一实现，因此 lambda 布局、显式 stride 布局和矩阵 owner
不会分裂成三套算法接口。

### 13.2 矩阵运算

```cpp
auto id = nmatrix_identity<T>(n, add, multiply);
auto c = nmatmul(a, b, add, multiply);
auto p = nmatpow(square, exponent, add, multiply);
```

默认使用 `nadd<T>` 与 `nmul<T>`。乘法要求维度相容；朴素乘法 `O(r*k*c)`，快速幂
为 `O(n^3 log exponent)`。操作包必须满足矩阵乘法所需的单位、结合、分配和零吸收等
跨操作定律；库不再要求额外的命名 concept 或 trait 声明。

`nmat<T,Add,Mul>` 是建立在同一 `nmatrix` 存储上的代数绑定 facade，恢复 v1 的短代码：

```cpp
nmat<long long> a{{1,2},{3,4}};
auto id = nmat<long long>::eye(2);
auto c = a + b;
auto d = a * b;
auto p = a.pow(k);
auto t = a.trans();
a.get(row, col, fallback);
```

`nmatrix` 负责拥有与 view 拓扑，`nmat` 负责把固定 `Add/Mul` 绑定进运算符；两者不是
互相替代的重复矩阵。调用者仍必须保证矩阵乘法真正需要的分配律、零吸收等跨操作定律。

矩阵快速幂的触发信号是“固定维度状态做很多次同一个线性转移”。先写一次状态转移，
确认下一状态能由当前状态的线性组合得到，再把转移写成矩阵；只是看到很大的指数并不
足够。如果状态维度也很大，`O(k^3 log n)` 可能比原 DP 更慢。

### 13.3 RREF、行列式与线性方程

```cpp
nvector<int> pivots;
int rank = nrref(matrix, &pivots); // 原地化为 RREF
T det = ndeterminant(square);      // 按值复制输入
auto solution = nlinear_solve(A, b);

T same_det = ndet(square);         // 兼容短名
auto inverse = ninverse(square);   // 奇异时为空 nmaybe
auto legacy = ngauss(A, b);        // 总返回 result，以 consistent 标记无解
```

`nlinear_solve`：

- 无解：空 `nmaybe`。
- 有解：`particular`（兼容字段 `one`）为特解，`basis` 为齐次解空间基，`rank` 为秩。
- 任意解形如 `particular + Σ c_i*basis[i]`。

`ngauss` 返回同一个 `nlinear_solution<T>` 形状，但无解时 `consistent == false`；
`nlinear_solve` 则用空 `nmaybe` 表示无解。新代码优先后者，迁移旧模板时可保留前者。

这些算法要求 `T{}` 是零、`T{1}` 是一、非零 pivot 可除。普通整数的除法可能截断，
合数模的非零元素可能不可逆，浮点数还需要 eps 与选主元策略；调用者必须在进入算法前
确认域性质。整数行列式应另用 Bareiss 等整环算法。复杂度为三次量级。

RREF 的不变量是：每次选择一个可逆 pivot，把该列化成唯一主元并消去其他行；未处理列
继续表示剩余自由变量。若 pivot 不能除，消元步骤并不在当前数值域中成立，不能把 C++
整数除法产生的截断结果当作线性代数答案。

读 `nlinear_solve` 的返回值时，先按秩分类，而不是只看“有没有一个向量”：

| 条件 | 结论 | 参数化形状 |
|---|---|---|
| 增广列出现 `0 ... 0 | nonzero` | 无解 | 空 `nmaybe` |
| 一致且 `rank = 变量数` | 唯一解 | `particular`，`basis` 为空 |
| 一致且 `rank < 变量数` | 多解（在当前域上） | `particular + Σ c_i basis[i]` |

最小反例是方程 `x+y=1, 2x+2y=2`：第二行没有提供新 pivot，因此 rank 为 1，不能把一
个随意特解误报成唯一答案。相反 `x+y=1, x+y=2` 在消元时产生矛盾行，应该报告无解。
这也是为什么 `basis` 不是“额外输出装饰”，它描述了所有自由变量方向。

### 13.4 卷积

```cpp
auto c = nconv_naive(a, b); // O(nm)
auto c = nconv_ntt(a, b);   // 静态 nmodint，O(k log k)
auto c = nconv_auto(a, b);  // 根据真实 NTT 前提选择
auto c = nconv(a, b);       // 当前默认转发到 auto
```

NTT 前提（由 `nconv_ntt` 在运行时检查）：

- 系数类型是 `nmodint<M>`。
- `M` 为不超过 `UINT32_MAX` 的质数。
- 变换长度为二次幂且整除 `M-1`。

`nconv_auto` 仅在两边长度至少 32 且上述条件成立时选择 NTT，否则使用朴素卷积。
输入为空时结果为空。

卷积的题型信号是“答案下标由两个选择的下标相加”，例如多项式系数、两组和计数。
朴素算法枚举全部 `(i,j)`，复杂度 `O(nm)`；NTT 把卷积变成点值乘法，但只有模数存在
足够阶的二次幂单位根时才成立。短序列上 NTT 常数反而更大，因此 `nconv_auto` 保留
朴素分支。

`nntt_info<Mint>` 是 primitive-root 快速入口。内建为 `998244353`、`1004535809`、
`469762049` 提供根 3；其他适用 `nmodint<M>` 会在首次使用时自动分解 `M-1` 搜根。
若为另一个常用 `nmodint<M>` 显式提供快速根入口，必须保证：

```cpp
template<> struct nntt_info<nmodint<M>> {
    static constexpr bool ok = true;
    static constexpr uint64_t root = ...; // 真正的本原根
};
```

错误 root 会静默产生错误卷积，测试必须与 `nconv_naive` 随机对拍。

`nntt_info` 只回答“这个类型可以从哪里开始找根”的实现入口，不证明给定模数真的是质数，
也不证明当前长度满足 `length | (M-1)`。把它当成证明会把一个错误 root 变成静默 WA；
正确做法是保留运行时前提检查，并用朴素卷积做独立 oracle。

### 13.5 多项式/FPS 工具

系数按低次到高次排列：

```cpp
auto d = npoly_derivative(a);
auto i = npoly_integral(a);       // 常数项为 0，要求 i+1 可逆
auto y = npoly_evaluate(a, x);    // Horner
auto inv = nfps_inverse(a, terms);// a[0] 非零且可逆
```

`npoly_integral` 与 `nfps_inverse` 现在按接口实例化；调用者必须确保除数和常数项可逆，
避免普通整数的截断除法或合数模的不可逆元素。
FPS 逆使用 Newton 迭代，复杂度由 `nconv` 后端决定；NTT 可用时约为 `O(M(n)log n)`。

拥有型 facade `npoly<T>` 会自动删除尾部零系数，零多项式表示为空：

```cpp
npoly<mint> f{1,2,3};
f.len(); f.deg(); f[i]; f.at(i); f(x);
f + g; f - g; f * g;
f.deriv(); f.integral(); f.cut(terms);
f.inv(terms); f.log(terms); f.exp(terms);
```

`operator[]` 越过次数返回零值，`at` 则要求真实系数位置。`inv` 要求常数项非零，
`log` 要求常数项 1，`exp` 要求常数项 0；除法相关接口只接受浮点或精确域。

线性递推桥：

```cpp
auto recurrence = nberlekamp(sequence);       // Berlekamp–Massey
auto value = nrec_nth(initial, recurrence, k);// 空 nmaybe 表示信息不足
```

返回的递推满足 `a[n] = Σ recurrence[i] * a[n-i-1]`。BM 要求系数具备域除法，当前由
表达式实例化约束，调用者必须选精确域；`nrec_nth` 使用多项式模快速幂，阶数 `d` 时
为 `O(d^2 log k)`，另有 fallback 重载。

FPS 的前置条件来自系数递推本身：若 `f[0]` 不可逆，就连第一步 `g[0]=1/f[0]` 都没有
定义；若做积分，第 `i` 次项要除以 `i+1`；`log` 与 `exp` 还额外固定常数项，使形式幂
级数中的递推从零阶开始。预先列出这些除法，再决定 Mint/有理数/浮点类型，通常比等到
模板实例化错误时更快定位问题。

### 13.6 线性代数与多项式检查点

1. `nmatmul` 的三个维度分别代表什么？为什么交换两个非方阵仍可能不合法？
2. RREF 中矛盾行、唯一解和自由变量分别如何反映在 `nlinear_solution` 字段上？
3. 为什么整数类型的 `/` 不能自动充当消元中的域除法？
4. NTT 长度、模数质数性和本原根各自承担哪一条证明前提？
5. `nconv_auto` 为什么对短序列保留朴素分支？
6. FPS `inv/log/exp/integral` 的常数项或可逆性要求分别是什么？
7. Berlekamp–Massey 找到的递推方向如何与 `nrec_nth` 的下标约定对应？

---

## 14. 字符串、Trie 与 AC 自动机

字符串算法接受任意 `nindexed` 符号序列，不要求 `std::string`。选择路径通常是：

```text
单模式匹配/前后缀       → prefix/Z/KMP
回文半径                → Manacher
大量后缀比较            → suffix array + LCP
前缀集合               → Trie
多模式同时匹配          → AC automaton
```

先明确算法需要“边界信息”“后缀排序”还是“前缀状态机”，再决定是否需要拥有节点结构。

### 14.1 线性字符串算法

```cpp
nprefix_function(sequence);  // prefix[i]：结尾 i 的最长真 border
nz_function(sequence);       // z[0]=n
nkmp_find(text, pattern);    // 返回全部起点；空 pattern 匹配 0..n
```

兼容短名分别是 `nprefix`、`nzfunc`、`nkmp`。均为线性复杂度。

KMP 的关键不是“记住 next 数组”，而是失配后保留已经知道的最长 border；文本指针不
回退，因此总扫描是线性的。Z 函数则维护一个已知匹配区间 `[l,r)`，当前位置落在区间内
时先镜像复制已知结果，再只扩展尚未比较的部分。两者都依赖半开区间和 border 定义的一致性。

### 14.2 Manacher

```cpp
auto pal = nmanacher(sequence);
pal.odd_radius(center);
pal.even_radius(right_center);
pal.pal(l, r);               // [l,r) 是否回文，空串为 true
```

奇半径包含中心；偶半径中心位于 `right_center-1` 与 `right_center` 之间。
结果类型名为 `npalindrome_index`，兼容别名 `nmanacher_result`。

Manacher 的半径数组维护一个最右回文中心及其边界；新中心落在边界内时先镜像复制，再
扩展超出部分。每个字符最多被扩展到最右边界一次，因此为 `O(n)`。奇半径和偶半径的
中心定义不同，混用会造成只在偶长回文上出现的边界错误。

### 14.3 后缀数组与 LCP

```cpp
auto sa = nsuffix_array(sequence, compare);
auto lcp = nlcp_array(sequence, sa);
```

`lcp[i]` 是 `sa[i-1]` 与 `sa[i]` 的 LCP，`lcp[0]=0`。当前 suffix array 使用倍增排序，
复杂度 `O(n log^2 n)`（每轮比较排序 `O(n log n)`，轮数 `O(log n)`）；LCP 为 `O(n)`。
传给 `nlcp_array` 的 suffix 必须是 `[0,n)` 的排列，范围与重复项都会验证。

后缀数组解决的是“把所有后缀按字典序排列后，许多子串问题能转成相邻后缀的 LCP”。
因此 `sa` 的排列合法性不是形式检查：若缺失或重复一个后缀，`lcp` 的相邻意义就消失。

任意两个后缀 `x,y` 的 LCP，可以转成它们 rank 之间所有相邻 LCP 的最小值。原因是：
字典序区间中的每一条相邻边都至少共享长度 `k`，等价于整个区间所有后缀都共享该长度；
第一条小于 `k` 的边正是公共前缀不能再延长的位置。因此静态大量查询可以装配：

```cpp
auto sa = nsuffix_array(text);
auto lcp = nlcp_array(text, sa);
int n = nlen(text);
nvector<int> rank(n);
for (int i = 0; i < sa.len(); ++i) rank[sa[i]] = i;
nsparse<int, nmin<int>> rmq(lcp);

auto suffix_lcp = [&](int x, int y) {
    if (x == y) return n - x;
    int a = rank[x], b = rank[y];
    if (a > b) swap(a, b);
    return rmq.fold(a + 1, b + 1);
};
```

这个桥梁常用于重复子串、后缀比较和二分长度。注意这里只把**构建后的 LCP 数组**交给
静态 RMQ；原字符串若会修改，suffix array、rank、LCP 和 RMQ 都同时失效。

### 14.4 `ntrie<Alphabet>`

输入元素类型必须是 integral，符号值必须位于 `[0,Alphabet)`；范围判断发生在转成
`int` 之前，不会让超大整数截断成合法符号。字符可用 `nproject` 映射：

```cpp
string text = "apple";
auto letters = nproject(text, [](char c) { return c - 'a'; });

ntrie<26> trie;
int terminal_node = trie.add(letters);
trie.find(letters, fallback);
trie.count(letters);          // 完整串插入次数
trie.count_prefix(prefix);    // 经过该前缀的串数
trie.parent(node);
trie.symbol(node);
```

Trie 把每个字符串的前缀共享成一条根到节点的路径；节点计数必须区分“完整串结束次数”
和“经过该前缀的次数”。字母表越大，分支存储的常数和内存越重要，因此输入符号应在
进入索引前检查范围，而不是窄化后碰巧落入 `[0,Alphabet)`。

### 14.5 `nac<Alphabet>`

AC 自动机是在 Trie 上补失败链接：当前字符边不存在时，沿最长可行后缀跳转，而不是把
文本指针退回。构造和扫描的正确性依赖失败链接确实指向最长后缀；输出统计还要先决定
是报告每次匹配、模式总数，还是只判断存在。多模式题若只需存在性，不必为每次输出保存
全部模式列表。

```cpp
nac<26> ac;
int id = ac.add(pattern); // id 按添加顺序，从 0 开始；空 pattern 禁止
ac.build();               // 只能一次；之后不能 add

ac.match(text, [&](int inclusive_end, int pattern_id) {
    // 每个匹配一次回调
});

long long total = ac.count(text);
auto all = ac.matches(text); // nvector<nmatch>{start,end,id}，区间 [start,end)
ac.each(text, callback);     // callback(start,end,id)，统一半开区间
int next_state = ac.step(state, symbol);
```

构建/扫描线性于节点与文本，另加实际报告匹配数。`match` 按引用转发 callback，
不会复制其内部状态，也支持不可复制 functor。

AC 的失败链接只解决“下一状态去哪儿”，不自动解决“输出哪些模式”。如果一个节点沿
失败链还继承多个终止模式，逐条回调的总成本应写成 `O(text + reported_matches)`；题目只
问是否出现时，可在第一次命中后停止，或为节点缓存布尔汇报结果。不要把“构建线性”误读
成“所有匹配输出也线性”。`build()` 之后禁止 `add`，因为新增模式会改变整棵失败树；若题目
在线增删模式，需要另一种动态字典方案。

### 14.6 字符串检查点

1. KMP 失配时保留的 border 为什么不会让文本指针回退？
2. Z 函数的 `[l,r)` 窗口和镜像位置分别保证了什么？
3. 后缀数组的 `lcp[i]` 为什么只比较相邻后缀，任意两后缀查询如何迁移到 RMQ？
4. Trie 的“经过前缀次数”和“完整串结束次数”为什么必须分开？
5. AC 失败链接指向最长后缀的哪个性质让扫描保持线性？
6. `reported_matches` 很大时，为什么 AC 的总复杂度不能只写 `O(text + patterns)`？

---

## 15. 几何与优化

几何题先决定谓词精度：整数坐标通常应优先使用叉积和 `__int128_t`，只有输入或输出本身
是浮点时才引入 epsilon。优化题先确认目标是否单调、单峰或可由直线包络表示；不能因为
看到了“最小值”就直接三分或套 Li Chao。

### 15.1 点与精确整数谓词

```cpp
npoint<T> p{x,y};
p += q; p -= q; p *= scale; p /= scale;
p + q; p - q; p * scale; p / scale;

ndot(a,b);
ncross(a,b);
norient(a,b,c);       // (b-a) x (c-a)
ndist2(a,b);
non_segment(p,a,b);   // 含端点
nsegment_intersect(a,b,c,d);

nsign(value);
nsgn_eps(value, eps);
norient(a,b,c,eps);             // 返回 -1/0/1
nonseg(a,b,p,eps);              // epsilon 版本，注意参数顺序
nsegment_intersect(a,b,c,d,eps);
```

integral 坐标的差、乘积与累加使用 `__int128_t`，并在转换和运算前验证结果可表示；
极端 64-bit 坐标若需要超过 128-bit 的叉积/距离会触发契约，超出 signed 128-bit
输入域的更宽无符号坐标也会被拒绝，而不是先窄化或发生有符号 UB。
`non_segment` 名字读作“n-on-segment”，不是英文否定词 `non`。

几何证明应尽量建立在符号谓词上：`cross > 0/<0/=0` 分别表示左转、右转和共线。整数
坐标下先扩宽再做叉积可以避免 epsilon；浮点下则必须统一 epsilon，并接受“接近共线”
是题目定义的一部分，而不是实现噪声。

### 15.2 凸包和多边形

```cpp
auto hull = nconvex_hull(points, keep_collinear);
auto twice_signed_area = npolygon_area2(polygon);
auto diameter_squared = nconvex_diameter2(points);
int where = npoint_in_poly(polygon, point, eps);
```

凸包去重并以逆时针顺序返回；默认移除边上中间共线点。全共线时，默认只返回两端。
凸包 `O(n log n)`，面积 `O(n)`，直径包含建 hull 为 `O(n log n)`。
`npoint_in_poly` 返回 `1` 内部、`0` 边界、`-1` 外部；空多边形返回外部。

单调链凸包的栈不变量是：当前链始终保持所需转向；加入新点造成错误转向时，栈顶不可能
属于最终外壳，因此反复弹出。是否保留共线点会改变“错误转向”的等号处理，必须和题目
对边界点的定义一致。

### 15.3 直线交点

```cpp
auto point = nline_intersection(a,b,c,d);

nline2<long double> x{origin, direction};
auto same = nline_intersect(x, y, epsilon);
```

`nline_intersection(a,b,c,d)` 以两组点表示无限直线，平行或重合返回空；该旧入口当前
使用 `denominator == 0`。`nline2<T>{p,v}` 明确使用点加方向向量，`nline_intersect`
接受 epsilon。两者成功时都返回 `npoint<long double>`，浮点输入的误差策略仍由调用者负责。

“线段相交”和“无限直线交点”不是同一个问题：前者还要检查交点是否落在两段范围内，
并处理共线重叠；后者只在方向不平行时返回唯一点。不要用一个成功的直线交点结果替代
线段边界判断。

几何代码至少应手造下面这些退化输入；它们比随机均匀点更容易暴露等号错误：

| 退化情形 | 必须先决定的语义 |
|---|---|
| 重复点、零长度线段 | 是否视为一个合法点/闭线段 |
| 三点共线 | 中间点算边界还是应从 hull 删除 |
| 两线段只在端点相触 | “相交”是否包含端点 |
| 两线段共线且部分重叠 | 返回 bool、交集段还是唯一点 |
| 无限直线平行或重合 | 都没有唯一交点，不能强造坐标 |
| 全部点共线、只有 0/1/2 个点 | hull/diameter 的返回形状 |
| 点落在多边形边或顶点 | `0` 边界与内外必须分离 |
| 64-bit 极端坐标或近共线浮点 | 扩宽是否足够、epsilon 是否统一 |

先把题面需要的等号语义写在纸上，再选择 `keep_collinear`、闭区间相交或 epsilon。几何 WA
常常不是公式不会，而是两个函数对“边界算不算”的答案不同。

### 15.4 Li Chao Tree

```cpp
nlichao<long long> minimum(left, right); // 整数域 [left,right)
minimum.add(slope, intercept);
minimum.add_segment(slope, intercept, l, r);
auto y = minimum.query(x);               // 无覆盖线时为空

nlichao<long long, ngreater<__int128_t>> maximum(left, right);
```

`nline_function<T>` 保存 slope/intercept；integral 求值扩为 `__int128_t` 并检查乘加范围。
整条线插入和查询 `O(log(domain width))`，线段插入 `O(log^2(domain width))`，节点按需创建。

坐标已离散时，不要用巨大整数域硬撑动态树：

```cpp
nlichao_static<long long> tree(coordinates);
tree.add(nline<long long>{slope, intercept});
tree.addseg(line, x_left, x_right); // 只覆盖离散坐标中的 [x_left,x_right)
tree.addidx(line, l, r);            // 按压缩下标 [l,r)
tree.get(x);                        // x 不在坐标表时为空
tree(x, fallback);
```

构造会排序去重坐标，`x` 公开展示规范坐标表；`hasx` 检查是否可查询。整线插入和查询
`O(log n)`，线段插入 `O(log^2 n)`，空间 `O(n)`。`nline<T>{m,b}` 是压缩后端短线型，
可转换为动态后端的 `nline_function<T>`。

Li Chao 的机关是：两条直线的优劣最多交换一次。节点只需保留在中点更优的直线，把另
一条可能获胜的半边递归下去。它适合直线函数和固定比较方向；若候选函数可以多次交叉，
这个单交点不变量消失，不能继续使用同一结构。

选择动态或离散后端首先看查询坐标是否预知：在线出现任意整数 `x` 用 `nlichao(left,right)`；
全部查询点可以先收集时，`nlichao_static(coordinates)` 只在真实会问的位置比较，坐标跨度
再大也不增加树高。二者都要求每对候选在有序域上至多一次优劣交换；把一般折线、多次
交叉的二次函数或不固定比较方向硬塞进去，会破坏“失败线只可能在一侧翻盘”的证明。

### 15.5 离散单峰搜索

```cpp
auto argmin = nunimodal_arg(first, last, function);
auto argmax = nunimodal_arg(first, last, function, ngreater<>{});

auto x = nternary_min(left, right, continuous_function, iterations);
```

搜索整数 `[first,last)`，要求非空且目标相对比较器单峰。返回一个最优位置，约
`O(log range)` 次求值，尾部至多四点暴力。每轮固定先求左探针、再求右探针，
不依赖函数实参的未指定求值顺序。
`nternary_min` 用于浮点闭区间上的近似单峰最小值位置，默认 100 轮，返回最终区间中点；
它不提供误差证明，精度由区间宽度、轮数和函数数值稳定性共同决定。

离散单峰搜索允许最优平台，但必须确认函数先不劣、后不优；存在多个局部谷底时，三分
缩区间可能直接丢掉全局最优。若“给定答案是否可行”具有单调性，优先二分答案，不要把
单调判定问题包装成单峰优化。

### 15.6 几何与优化检查点

1. 整数叉积为什么要在相减和相乘之前扩宽，而不是计算后再转成 `__int128_t`？
2. 凸包的 `keep_collinear` 改变了哪一个等号分支？全共线输入希望返回什么？
3. 线段相交与无限直线唯一交点分别需要哪些退化判断？
4. Li Chao 中“另一条线只需递归一侧”的单交点证明是什么？哪些函数会破坏它？
5. 预知全部查询坐标时，离散 Li Chao 相比巨大动态整数域省掉了什么？
6. 单峰搜索允许平台，但为什么不允许两个分离的局部谷底？
7. 如果存在单调可行性判定，为什么二分答案通常比三分目标值更容易证明？

---

## 16. 竞赛 I/O

I/O 章节只处理边界，不改变算法模型。输入失败、整数溢出和输出格式错误应尽早暴露；
不要为了“更快”把解析写成无法检查的裸指针。正常竞赛中 `nin/nout` 已足够，文件构造和
fallback 主要服务测试与工具。

### 16.1 输入

```cpp
int n;
long long x;
string s;
char c;

nin >> n >> x >> s >> c;       // EOF/非法 token 触发 npre
bool ok = nin.read(n);          // EOF 返回 false
bool all = nread(n, x, s, c);   // 任一 EOF 返回 false
```

支持除 `bool` 外的 integral 以及 `__int128_t`/`__uint128_t`；`char` 使用字符重载。
整数溢出、负数读入无符号类型、数字后紧跟非空白字符等非法 token 都会触发前置条件。

可为测试或文件构造：

```cpp
ninput in(file);
```

### 16.2 输出

```cpp
nout << value << ' ' << text << '\n';
nout.flush();

nprint(a, b, c);    // 自动以一个空格分隔，不追加换行
nprintln(a, b, c);  // 空格分隔并换行
```

`noutput` 支持 char、C string、`string_view`、`string` 和整数含 128-bit。析构自动 flush，
也可用 `noutput out(file)`。缓冲区大小为 64 KiB。

输出契约仍是题面的一部分：`nprint` 不加换行，`nprintln` 加换行，多个参数之间恰好
一个空格。不要把调试信息写到 stdout；训练阶段可以使用独立日志或 checked 诊断。

### 16.3 最小可提交程序与边界

下面的程序同时展示 EOF 分支、普通整数输入和 128-bit 累加输出：

```cpp
#include "Nitori.h"

int main() {
    int n;
    if (!nread(n)) return 0;

    __int128_t sum = 0;
    nrep(i, n) {
        long long x;
        nin >> x;
        sum += x;
    }
    nprintln(sum);
}
```

若题目保证输入完整，后续使用 `nin >> x` 可以让截断输入立即失败；只有“EOF 本来就是
正常终止条件”时才用 `read/nread` 的 bool 返回。`__int128_t` 不需要自己写十进制输出器，
但乘法前仍要把操作数提升到目标宽度，`__int128_t(a*b)` 无法挽救已经在 `long long`
中溢出的乘积。

交互题必须在每次询问后显式 `nout.flush()`；析构 flush 只发生在程序结束，不能让评测机
在中途收到问题。自定义 `ninput/noutput(FILE*)` 时，`FILE*` 的生命周期必须覆盖包装对象。

**I/O 检查点：**

1. `read` 返回 false 与 `operator>>` 触发前置条件分别适合哪种 EOF 语义？
2. 负数字面量读入无符号类型为什么必须拒绝，而不是按 C++ 转换回绕？
3. 128-bit 累加怎样避免“转换发生得太晚”的中间溢出？
4. `nprint` 与 `nprintln` 的空格和换行差别是什么？
5. 交互题为什么不能依赖全局 `nout` 的析构 flush？

---

## 17. 题型装配配方

这一章不按类名排序，而按“题目里先出现了什么信号”组织。配方不是替代证明的模板；
它负责把已经证明的算法映射到最小 Nitori 能力。阅读时先遮住代码，回答三个问题：

```text
最直接暴力是什么？
重复工作或错误风险在哪里？
题目真正需要的能力是什么？
```

每个较完整的配方依次给出：题目原型、暴力瓶颈、机关、装配、正确性、复杂度、失败
边界和迁移信号。短配方只演示一种已经在前文证明过的组合，不重复全部背景。

### 17.0 先选能力，不先选类名

```text
只重排/选择位置                         → nview
按字段观察完整记录                     → algorithm projection
需要 semantic key 和有限 support       → nfunc
单点改、前缀可消去                     → Fenwick
一般有序区间聚合                       → segment tree
区间修改                               → lazy segment tree
巨大稀疏坐标                           → dynamic segment tree
历史版本                               → persistent segment tree
有序集合 + 子树信息/自定义下降         → AST augmentation
按位置动态序列 + 重排/自定义 tag        → nimplicit_fhq
```

如果一行题面同时命中多个信号，先找最强约束。例如“坐标巨大”不自动推出动态开点：坐标
全部离线可见且只依赖次序时，压缩往往更小更快；只有真实区间长度、在线坐标或无法预读时，
动态开点才成为主要机关。

### 17.1 矩阵主对角线排序

**题目特征：**只操作矩阵的一条几何线，但希望复用普通序列算法。

```cpp
nmatrix<int> a(rows, cols);
auto d = a.diagonal();
nsort(d);
```

其他对角线：

```cpp
for (int offset = -rows + 1; offset < cols; ++offset) {
    auto d = a.diagonal(offset);
    nsort(d);
}
```

`diagonal(offset)` 返回可写 view，没有复制矩阵元素。`nsort` 看到的是一组可交换位置，
不需要知道二维布局；每条对角线的排序复杂度为 `O(k log k)`。如果需要保留原矩阵，先
`ncollect(d)`，不要误以为复制 view 已经复制元素。

### 17.2 排序 deque 或 lambda 映射位置

**题目特征：**目标位置不连续，但仍然形成确定的 `0..k-1` 序列。

```cpp
ndeque<int> q{4,1,3};
nsort(q); // 非连续原地 heapsort

auto selected = nview(k, [&](int i) -> int& { return storage[index[i]]; });
nsort(selected);
```

lambda 必须返回真实 `T&`；如果 `index` 含重复位置，多个 view 项会别名同一元素，原地
排序依赖的“不同位置可独立交换”不再成立。先去重位置，或明确 `ncollect` 后排序副本。

### 17.3 把数组升级为可分块离散函数

```cpp
nvector<int> a(n);
auto cell = nfunc_ref(nrange(n), [&](int i) -> int& { return a[i]; });

nfor(block, nblocks(cell, width)) {
    // block.key(i) 是原数组位置，block[i] 是 a[原位置] 的引用
    nsort(block);
}
```

这不是复制分块：`cell` 只在外层建立一次并被所有块复用，`nblocks` 构造为 `O(1)`，
尾块自动缩短。块内操作直接写回 `a`。不要在块循环里重复建立 `cell`；若只要快照，
在边界处调用 `ntabulate(block)`。

### 17.4 用依赖表装配子序列 DP

```cpp
nvector<int> dp(n, 1);
auto state = nfunc_ref(nrange(n), [&](int i) -> int& { return dp[i]; });

for (int v = 0; v < n; ++v) {
    auto incoming = nselect_positions(state, predecessor[v]);
    nforkv(from, best, incoming)
        nchmax(dp[v], best + transition(from, v));
}
```

`predecessor[v]` 存源函数的枚举位置；若它存的本来是语义状态 key，则应改用
`nredomain(state, predecessor[v])`；若还要求任意调用也只能使用这些 key，则用
`nrestrict`。先分清 position/key，能消掉大量“数值恰好相同”
掩盖的 WA。

这里每个状态临时建立一次 `O(1)` 的 `nselect_positions`，是用高阶表达购买调试时间；若状态数
达到百万级、对象要长期保存或基准显示描述符成本进入热点，就改用 `nview`/直接位置循环，
不要批量保存 `nfunc` 家族对象。

### 17.5 滑动窗口最小值

**题目原型：**输出每个长度为 `width` 的窗口最小值。暴力每个窗口扫描一次是
`O(n * width)`。

```cpp
nqueue_agg<int, nmin<int>> q;
for (int i = 0; i < n; ++i) {
    q.push(a[i]);
    if (q.len() > width) q.pop();
    if (i + 1 >= width) answer.push(q.fold());
}
```

聚合队列用两个带缓存的栈保持队列顺序；每个元素最多进入和搬移常数次，所以全部
`push/pop/fold` 为摊还 `O(1)`，总复杂度 `O(n)`。入队后的元素不能被原地修改，否则
缓存聚合与真实队列不再一致。

### 17.6 隐式网格 BFS

邻接 accessor 可以返回按需生成的 indexed/view 对象；算法不要求先构造所有边。

```cpp
auto graph = ngraph_view(rows * cols, [&](int v) {
    int r = v / cols, c = v % cols;
    // 返回当前点的合法邻居序列；其生命周期必须覆盖这次 neighbors 使用。
    return build_neighbors(r, c);
});
auto distance = nbfs(graph, source);
```

机关是“BFS 只需要枚举当前顶点的邻居”，不需要提前拥有整张边表。邻接结果的生命周期
只需覆盖当前一次 neighbors 枚举，但不能返回引用到已经销毁的局部 owner。总复杂度仍是
`O(V+E)`，其中 `E` 指实际被枚举出的邻接项数量。

### 17.7 随机对拍台架

```text
小规模生成器
→ 直接暴力
→ Nitori 高效实现
→ 比较全部输出
→ 保存首个失败样例
→ 缩小并手算
```

Nitori X 自身的 property tests 就采用这一模式；复杂结构不能靠样例一次通过证明正确。

### 17.8 二维 DP 行的初始化与检查点复制

`nmatrix::row` 是可写 view，不需要降级到 `.data()`、指针加法或 `std::copy`：

```cpp
nmatrix<ll> checkpoint(blocks + 1, S + 1, NEG);
nvector<ll> dp(S + 1, NEG);

nfill(checkpoint.row(block), NEG);
nassign(checkpoint.row(block), dp);
```

若需要从记录序列抽取一列，也仍然是同一个写入原语：

```cpp
nvector<int> weight(items.len());
nassign(weight, items, &item::weight);
```

这里的桥梁是“可写 indexed 目标 + enumerable 源 + projection”，不是连续内存。
因此相同代码可落在 vector、deque、矩阵行/列、步长 view 或 lambda 映射 view 上。

### 17.9 单点加、区间和：先用 Fenwick

**题目原型：**有一个长度为 `n` 的数组，反复执行 `add(i,delta)` 和 `sum(l,r)`。

**暴力瓶颈：**每次 `sum(l,r)` 逐项相加是 `O(r-l)`；当修改和查询都达到 `O(n)` 时，
总复杂度可能达到 `O(nq)`。

**机关：**加法可以把前缀拆成树状块，区间和用两个前缀相减。于是每次操作只访问
`O(log n)` 个块。

```cpp
nfenwick<long long> bit(initial);
bit.add(i, delta);
long long answer = bit.fold(l, r); // [l,r)
```

**正确性不变量：**每个内部块保存固定低位长度区间的和；`prefix(r)` 依次去掉末尾已
覆盖块，恰好拼成 `[0,r)`。`fold(l,r)` 使用加法逆元消去 `[0,l)`。

**失败边界：**若把 `nmin<T>` 直接替换成操作对象，通常没有“撤销前缀”的逆元；此时
应该换 `nseg`，而不是假设 Fenwick 能处理任意聚合。

**迁移信号：**“单点变化 + 前缀/区间统计 + 可消去”是 Fenwick 的触发器；只要出现
非交换顺序、区间 tag 或不可逆信息，就看 17.10～17.11。

### 17.10 单点改、非交换区间聚合：`nseg`

**题目原型：**字符串序列支持单点替换和区间拼接。拼接顺序不能交换。

```cpp
struct concat {
    string id() const { return {}; }
    string operator()(string left, const string& right) const {
        return left += right;
    }
};

nseg<string, concat> seg(initial, concat{});
seg.set(pos, value);
string answer = seg.fold(l, r); // 仍按原序拼接 [l,r)
```

**为什么 Fenwick 不合适：**`right + left` 与 `left + right` 不同，且没有普遍的逆操作。
线段树把查询区间拆成若干有序片段，左累加器必须在前、右累加器必须在后。

**正确性不变量：**每个节点缓存的字符串等于其区间的中序拼接；单点修改后只重算祖先。
查询选出的节点不重叠且按从左到右合并，因此得到的正是目标区间。

**最小反例：**数组 `{"a","b"}`，若查询实现把两个结果以右-左顺序合并，输出 `"ba"`
而不是 `"ab"`；交换性没有被类型系统证明，必须在操作对象附近写出约定。

### 17.11 区间加、区间和：`nlazyseg`

**题目原型：**对 `[l,r)` 中每个元素加 `delta`，并查询任意区间和。

逐点更新会在一次操作中写 `O(r-l)` 个叶子。lazy tag 把整段“以后都要加多少”留在
节点上：聚合和增加 `delta * length`，只有需要访问部分子树时才下推。

```cpp
nlazy_addsum<long long> seg(initial);
seg.apply(l, r, delta);
long long answer = seg.fold(qleft, qright);
```

**tag 顺序：**先加 `2`、再加 `5` 的总 tag 是 `7`；对仿射、赋值等非交换 tag，必须遵守
`compose(newer, older)` 的“先旧后新”。

**验证台架：**随机生成小数组，同时执行暴力逐点更新和 `seg.apply/fold`，特别安排两次
不同 tag 覆盖同一区间，再检查所有子区间，而不是只检查整段。

**迁移信号：**如果修改覆盖区间而答案也能由“聚合 + tag + 区间长度”直接更新，考虑
lazy；如果 tag 不能在聚合层闭合，就需要重新设计状态，而不是强行套模板。

### 17.12 巨大稀疏坐标：`ndynamic_seg`

**题目原型：**坐标范围为 `[-10^{12},10^{12})`，只有少量点被写入，查询真实坐标区间。

```cpp
ndynamic_seg<long long> seg(-1'000'000'000'000LL,
                             1'000'000'000'000LL);
seg.combine(x, delta);       // 只在更新路径开点
auto value = seg.fold(l, r);
```

未开节点表示单位元 `0`，不是“没有答案”。因此纯查询不应改变 `nodes()`；这既是空间
复杂度的一部分，也是调试动态树时最有价值的可观察不变量。

**选择对比：**所有坐标可以提前收集时，`ncompress` + 普通线段树通常更节省节点；坐标
在线出现、必须保留真实区间长度或无法预读时，动态开点避免了不自然的压缩层。

**复杂度：**令 `W = hi-lo`，点修改和区间查询均为 `O(log W)`；`q` 次首次写入最多产生
`O(q log W)` 个节点。坐标宽度必须能表示，lazy 动态树还受 `int` 区间长度限制。

### 17.13 查询历史状态：`npersistent_seg`

**题目原型：**每次修改都从某个旧版本分叉，查询任意版本的区间结果。

```cpp
npersistent_seg<long long> tree(initial);
int v1 = tree.set(0, index, value); // 从版本 0 产生 v1
int v2 = tree.set(v1, index2, value2);
long long old_answer = tree.fold(0, l, r);
long long new_answer = tree.fold(v2, l, r);
```

一次更新只复制根到叶子的路径，其余节点共享；旧节点永不改写，所以 `root(0)` 的快照
在产生新版本后仍有效。不要把“动态开点”误认为“自动持久化”：动态树的旧状态会被
后续修改覆盖，只有 persistent 的版本根提供历史语义。

### 17.14 从聚合节点找到第一个位置

假设所有叶值非负，需要找到最小 `i` 使前缀和至少为 `target`。先确认总和足够，再沿
`nseg_node` 下降：

```cpp
long long remaining = target;
auto node = nseg_walk(seg, [&](auto current) {
    if (current.leaf())
        return nbranch::take;
    long long left_sum = current.left().aggregate();
    if (remaining <= left_sum)
        return nbranch::left;
    remaining -= left_sum;
    return nbranch::right;
});
int answer = node ? int(node.left_bound()) : npos;
```

**为什么能走一条路径：**非负性使前缀和单调；如果左子树已经达到剩余目标，答案在左
边，否则整棵左子树都可以安全跳过并扣除其总和。这个下降逻辑可以复用于固定、lazy、
persistent 和 dynamic 后端，因为它们都提供同一种节点视图。

**失败边界：**允许负数时，左子树总和不足不代表左边没有更早的命中；此时需要不同的
聚合信息或改用二分答案/其他算法。

### 17.15 有序多重集的子树聚合与合法 tag

当题目要求“维护有序键，同时维护所有键的总和”，使用 augmentation：

```cpp
using bag = nset_fhq<int, nless<int>, true, sum_augment>;
bag tree;
tree.ins(5, 3);
tree.ins(9, 1);
auto hit = tree.first_prefix([&](long long sum) { return sum >= target; });
```

`sum_augment` 的不变量是：

```text
info(node) = info(left) + value * count + info(right)
```

如果增加一个整树平移 tag，键的相对顺序不变，tag 可以同时更新当前键和子树总和；如果
对一个局部子树任意取负，可能让节点跑到比较器顺序的另一侧，从而破坏 BST，不是因为
`apply` 接口不存在，而是因为题目的结构前提不再成立。

节点只读快照携带 epoch。拓扑修改后旧快照不能继续下降；读取 FHQ 子节点可能下推表示
tag，但不会单独让 epoch 失效。使用模式应是“拿快照、完成一次下降、立即丢弃”。

### 17.16 view 生命周期：最短的正确写法不一定是最安全的写法

```cpp
nvector<int> a{5, 4, 3, 2, 1};
auto middle = nsub(a, 1, 4);
nsort(middle);       // 合法：a 仍存活且没有扩容

auto snapshot = ncollect(middle); // 如果后续要扩容/销毁 a，先物化
```

复制 `middle` 只复制访问描述；如果 `a.push(...)` 触发扩容，旧 view 可能悬垂。这个
反例不是 Nitori 特有的魔法，而是所有借用引用的普通 C++ 生命周期规则；checked 只能
在能观测的位置帮助你发现，不能让失效引用重新合法。

### 17.17 位置、key、value：决定是否需要 `nfunc`

若 `predecessor[v]` 保存的是源数组位置，用：

```cpp
auto previous = nselect_positions(state, predecessor[v]);
```

若它保存的是语义 key，需要按 key 重新枚举，用 `nredomain`；若还要求任意调用也拒绝
不在 support 中的 key，用 `nrestrict`。当 domain 恰好是 `nrange(n)` 时三者数值看起来
相同，换成坐标或压缩编号后混用会产生很难察觉的 WA。

### 17.18 模数下的“除法”：先检查可逆性

组合数、期望和矩阵题中看到 `/x`，不要直接把它翻译成 `x * inv(x)`。先确认：

```text
模数是否为质数或已知可求逆结构？
gcd(x, mod) 是否为 1？
使用的 Mint 类型是否真的拥有 inverse 契约？
```

不可逆时，算法可能需要整数 gcd、扩展欧几里得、CRT 或改写计数方式。类型可以帮你做
模运算，却不能替你证明逆元存在。

### 17.19 动态序列：翻转、赋值、连续段查询与搬运

**题目原型：**维护一个可插入、删除和搬运的整数序列，支持：

```text
reverse(l,r)        翻转 [l,r)
assign(l,r,x)       把 [l,r) 全部赋成 x
query(l,r)          查询 [l,r) 内最长相同值连续段
splice(l,r,at)      剪下 [l,r)，插到删除后的坐标 at
```

先装暴力台架：用 `vector<int>` 保存序列，翻转和赋值直接操作迭代区间，查询逐项扫描，
搬运则“复制区间、erase、在删除后的 `at` insert”。这份暴力每次最坏 `O(n)`，不适合大量
操作，却给出了最可靠的语义 oracle，尤其能钉死 `splice` 的坐标含义。

**为什么不是有序 FHQ：**`nset_fhq` 的中序顺序必须始终符合 key 比较器；这里中序顺序
就是题目的位置顺序，翻转后相邻关系会变化，但根本没有“仍按 key 排序”的要求。真正的
机关是：隐式 FHQ 可以按子树长度把序列切成 `A | B | C`，只修改代表 `B` 的一棵 AST，
再把三段合回去。随机优先级只负责让 split/merge 的期望高度为 `O(log n)`。

下面给出完整策略。`run_info` 没有保存一份“反向答案”；它只保存本题证明所需的六个量。
翻转时首尾、前后缀的角色互换，最长段 `best` 不变，这些字段已经足以原地变换摘要。

```cpp
struct run_info {
    int len = 0;
    int first = 0, last = 0;
    int prefix = 0, suffix = 0, best = 0;
};

run_info join_runs(run_info a, run_info b) {
    if (!a.len) return b;
    if (!b.len) return a;

    run_info c;
    c.len = a.len + b.len;
    c.first = a.first;
    c.last = b.last;
    c.prefix = a.prefix;
    c.suffix = b.suffix;
    c.best = max(a.best, b.best);

    if (a.last == b.first) {
        if (a.prefix == a.len) c.prefix = a.len + b.prefix;
        if (b.suffix == b.len) c.suffix = b.len + a.suffix;
        c.best = max(c.best, a.suffix + b.prefix);
    }
    return c;
}

struct reverse_tag {};
struct assign_tag { int value; };

struct run_policy {
    using info_type = run_info;
    struct state_type {
        bool assigned = false;
        int value = 0;
        bool reversed = false;
    };

    info_type id() const { return {}; }
    info_type leaf(int x) const { return {1, x, x, 1, 1, 1}; }
    state_type state_id() const { return {}; }

    void pull(auto node) const {
        run_info left = node.left() ? node.left().info() : id();
        run_info self = leaf(node.val());
        run_info right = node.right() ? node.right().info() : id();
        node.info() = join_runs(join_runs(left, self), right);
    }

    void apply(auto node, assign_tag tag) const {
        node.val() = tag.value;
        int n = node.len();
        node.info() = {n, tag.value, tag.value, n, n, n};
        node.state().assigned = true;
        node.state().value = tag.value;
    }

    void apply(auto node, reverse_tag) const {
        node.exchange_children();
        swap(node.info().first, node.info().last);
        swap(node.info().prefix, node.info().suffix);
        node.state().reversed = !node.state().reversed;
    }

    void push(auto node) const {
        state_type pending = node.state();
        if (pending.reversed) {
            if (node.left()) node.left().apply(reverse_tag{});
            if (node.right()) node.right().apply(reverse_tag{});
        }
        if (pending.assigned) {
            if (node.left()) node.left().apply(assign_tag{pending.value});
            if (node.right()) node.right().apply(assign_tag{pending.value});
        }
        node.state() = state_id();
    }
};
```

`pull` 的合并顺序严格是“左子序列、当前值、右子序列”。`apply` 必须在不进入孩子的
情况下，立刻让当前 `value/info` 表示操作后的逻辑序列；`state` 只记录以后进入孩子时
仍需传播的工作。`push` 取得一份 pending 快照、给孩子调用同一策略的 `apply`，最后清空
当前状态。

这个例子中 assign 与 reverse 对元素值的效果可交换：整段赋值后翻转仍是同一均匀段；
孩子拓扑的翻转已经在 `apply(reverse_tag)` 当场完成。因此 `push` 固定先传播 reverse、
再传播 assign 是正确表示。换成仿射、截断、位置加权或左右孩子收到不同参数的 tag 时，
不能照抄这个顺序；应把“旧操作后接新操作”的组合逐项写成状态转移并造两次覆盖反例。

```cpp
nimplicit_fhq<int, run_policy> seq{0, 1, 1, 0, 1, 0};

seq.apply(1, 5, reverse_tag{});       // 0 1 0 1 1 0
seq.apply(2, 5, assign_tag{0});       // 0 1 0 0 0 0
int longest = seq.fold().best;        // 4

seq.splice(1, 4, 3);                  // at 属于删除后的长度 3 序列
seq.rotate(1, 4, 6);                  // 交换 [1,4) 与 [4,6)
seq.ins(2, 1);
seq.del(3);
```

**正确性闭环：**

1. `join_runs(a,b)` 保存总长、首尾值、同值前后缀和最长段。跨边界的新连续段只可能在
   `a.last == b.first` 时出现，长度为 `a.suffix + b.prefix`，所以没有漏掉候选。
2. `split(root,k)` 按左子树长度递归，恰好分出中序前 `k` 项；`merge(a,b)` 的调用前提是
   `a` 的全部项先于 `b`，随机 priority 只选根，不改变拼接次序。
3. assign 直接把整个摘要变成长度为 `n` 的均匀段；reverse 交换孩子及方向相关字段，
   所以二者施加后无需下推，当前根摘要已经正确。
4. split/merge 在沿路径递归前先 `push`，随后由孩子信息 `pull`，因此结构操作不会把尚未
   传播的逻辑状态遗失。

插入、单点删除/读写、fold/apply、splice 和 rotate 的结构成本期望 `O(log n)`；删除长度
为 `k` 的区间还要释放其中节点，总成本 `O(log n+k)`。枚举整个序列为 `O(n)`。pool 会
复用删除槽位，存储量是 `O(历史最大同时存活节点数)`，而不是累计插入次数。随机树高给的
是期望保证，不是对恶意或极端随机序列的确定最坏保证。

**河童式对拍：**固定种子生成小序列，随机执行插入、删除、reverse、assign 和 splice；
每一步都枚举 `seq` 与 `vector` 全量比较，再枚举所有短区间比较 `fold(l,r).best`。只检查
最后一次整树答案会漏掉 tag 组合、删除后坐标和局部摘要错误。持有的 `nnode` 快照还必须
在一次操作内用完；split/merge、`fold(l,r)` 或修改之后重新从 `root()` 取得。

**迁移信号：**当题目同时出现“位置顺序会改变、需要按段施加自定义语义、摘要不能被
统一 lazy action 描述”时，使用自由 `nimplicit_fhq`。如果顺序固定，普通/lazy 线段树
通常常数更小；如果对象按 key 排序，使用 ordered FHQ；如果只需块搬运而没有摘要，先
确认 `vector/deque` 的线性操作是否已经能过，不为模板感支付随机树常数。

---

## 18. 调试、测试与提交工作流

### 18.1 诊断顺序

```text
题意/数学模型
→ API 前提
→ 区间 [l,r)
→ view 生命周期和引用类别
→ 合并性质/作用组合顺序
→ 下标、初始化、溢出
→ 算法复杂度
→ profile 差异
```

常见错误：

- 把 STL `.size/.push_back` 写法带入 Nitori。
- 把 `nfor` 当成宏技巧而忘记它必须保持单层 `for` 的 `break/continue` 语义。
- 在 owner 扩容后继续使用旧 view。
- projection 返回值而不是 `T&`，随后期待 `nsort` 修改 owner。
- 混淆离散函数的枚举位置 `i`、语义 key `f.key(i)` 与直接求值 `f(x)`。
- 复制 view/离散函数后误以为已经复制元素；真正实例化应使用 `ncollect/ntabulate`。
- Fenwick 区间 fold 使用了没有 inverse 的操作。
- 对非交换 operation 改变左右合并顺序。
- lazy `compose` 把 newer/older 写反。
- 把按 key 的 `nset_fhq` 当作按位置序列，翻转后仍期待 BST 顺序成立。
- 自由 FHQ 的 `apply` 只记录 state，却没有立即把当前 `value/info` 更新到逻辑结果。
- 自由 FHQ 的 `push` 按错误时间顺序传播多个 tag，或传播后忘记清空当前 state。
- 调用 `exchange_children()` 后，没有同步方向相关摘要或策略状态。
- 持有旧 `nnode` 跨过 split/merge、`fold(l,r)` 或修改，再继续下降。
- 把 `splice(l,r,at)` 的 `at` 当成删除前坐标；它属于删去 `[l,r)` 后的序列。
- Dijkstra 输入负边。
- reroot 输入不是双向树。
- 模合数下直接除以不可逆元素。
- NTT 长度不整除 `mod-1`。
- unsafe 中依赖 checked 的错误恢复；unsafe 没有恢复语义。

### 18.2 Nitori X 测试命令

```bash
cd /home/tnuzy/NitoriSTL

# 确认生成头没有漂移
python3 tools/amalgamate.py --check

# 最窄双 profile 测试
python3 tools/test.py seq poly matching

# 全量 checked + unsafe
python3 tools/test.py

# 单 profile sanitizer
python3 tools/test.py --profile checked --sanitize
python3 tools/test.py --profile unsafe --sanitize

# 完整发布门禁
python3 tools/audit.py

# 确定性微基准（数值依机器而变）
python3 tools/bench.py
```

测试编译到 Linux `memfd`，不会在仓库留下二进制。

本地开发不要把全量门禁当成每次改一个字符的第一步。先按失败面选择最窄证据：

```text
隐式 FHQ 策略/API       → python3 tools/test.py implicit_fhq
线段树域与 merge        → python3 tools/test.py property_segment_merge property_segment_domain
动态线段树              → python3 tools/test.py property_dynamic property_segment_domain
Wavelet/Mo              → python3 tools/test.py property_wavelet offline
树与 reroot             → python3 tools/test.py tree_compat property_reroot
字符串/自动机           → python3 tools/test.py string automata
```

一个复杂 lazy/AST 错误的高价值诊断顺序是：固定最小样例，逐操作枚举整棵序列与暴力比较；
再检查所有小区间摘要；最后才扩大随机步数。记录随机种子和第一份失败输入，修复后把它
固化成 regression。只比较最终答案会让错误 tag 在后续操作中被偶然覆盖。

### 18.3 生成规则

正式语义模块列于：

```text
/home/tnuzy/NitoriSTL/src/manifest.txt
```

修改模块后：

```bash
python3 tools/amalgamate.py
python3 tools/amalgamate.py --check
python3 tools/test.py <相关测试>
python3 tools/audit.py
```

禁止直接编辑 `Nitori.h` 或 `Nitori_unsafe.h`。

---

## 19. 扩展 Nitori 的规则

### 19.1 先找真实泛型维度

只为真实数学差异泛型化：值类型、比较器、合并操作、作用、存储后端、静态容量、
持久化/回滚性质。不要为可能永远不会替换的内部细节制造策略森林。

### 19.2 算法依赖能力，不依赖容器名

新增序列算法先判断最小能力：

```text
只读索引？
真实 lvalue reference？
可交换？
连续？
可 resize？
只可枚举而不可索引？
```

不要为了复用 STL 算法给 `ndeque` 或 view 补 iterator。直接实现 Nitori 能力版本。

线段树后端也遵循同一原则：实现固定、lazy、持久化或动态存储时，优先提供
`nseg_node<S>` 所需的 `aggregate/child/interval/epoch` 能力，再让 `nseg_walk` 等调度器
依赖这个最小协议；不要为每个后端复制一套“按节点下降”的算法。

### 19.3 离散函数先写清 position/key/value

新增离散函数适配器必须明确三件事：输入序列中的整数代表源枚举位置还是语义 key，
结果是否保留原 key，value 是引用还是值。位置选择使用 `nselect_positions`，只改变
枚举 support 使用 `nredomain`，调用时也要成员检查才使用 `nrestrict`。还必须写清构造、
求值和额外状态复杂度：
需要按 key 查询就建立正确 locator/index，不允许用隐藏线性扫描伪装轻量；只需位置拓扑
就退回 `nview`，不要在热点路径批量建立或保存高阶函数对象。运行时增删绑定属于
`nmap/npartial`，不塞进有限 `nfunc` 的职责。

### 19.4 保护 view 生命周期

- 借用 API 接受左值 owner。
- 不在 view 中拥有元素副本。
- accessor 返回真实引用时明确写 `-> T&`/`-> const T&`；公共适配器不得返回 `T&&`。
- 中间值为临时时，组合器不得让从它产生的引用逃逸；使用稳定调用边界并保留 sanitizer
  回归测试。
- 对临时 owner 的危险入口显式删除。
- 测试 owner 修改和非连续访问。

### 19.5 操作和作用必须写出定律

新增操作包必须在注释中写明单位元、结合/交换/逆元和溢出边界，且与运算一致。新增
lazy action 必须测试非交换 tag 组合；新增 segment 聚合必须用字符串拼接等非交换对象
测试顺序。

所有带隐形约束的 class/struct/算法都在定义附近保留本地契约注释，不依赖读者跳到另一章
才能知道它做什么。按适用项说明：功能、所需接口、数学/生命周期性质、失效边界与复杂度。
简单结构可用连续 `//`，承重结构使用 `/** ... */`；不为仅能检查语法的包装 concept
重复维护一份“证明登记表”。

### 19.6 自由隐式 FHQ 策略的维护规则

`nimplicit_fhq` 是确有位置序列需求时的低层逃生舱，不是让全库每个结构都变成 policy
forest 的理由。引擎只拥有 split/merge、priority、size、parent、pool 与 epoch；题目语义
必须完整留在一个局部策略中。禁止为了某个单题向核心添加 `reverse_subtree`、固定双向
聚合或 tag 名单。

自定义策略逐条保护下面的不变量：

1. `apply(node,tag)` 返回时，当前节点的 `val/info` 已经表示 tag 施加后的**逻辑子序列**；
   state 只是未向孩子解释的剩余工作，不能让根查询依赖先 push 才正确。
2. `push(node)` 只把 state 派生并发送给现有孩子，再把当前 state 还原到 `state_id()`；
   tag 有时间顺序时，先把 `older -> newer` 的实际效果手算清楚。
3. `pull(node)` 按“左子序列、当前值、右子序列”重建 info。引擎调用它时当前节点的 pending
   state 已经下推；策略不得在 pull 中猜测或重复施加旧 tag。
4. `exchange_children()` 只交换拓扑。首尾、前后缀、方向哈希、括号状态等摘要怎样变化，
   以及 reverse 标记怎样组合，都由同一个 `apply` 同步完成。
5. 位置相关 tag 可以在 `push` 中根据 `left.len()/right.len()` 派生两个不同 tag；核心不要求
   uniform action。若派生还依赖全局 offset，策略 state 必须保存足够坐标信息。
6. `mutate` 返回时必须留下同一节点集合的一份合法 lazy 表示；它不是绕过 `apply/push`
   约束的后门。优先把可复用变换写成普通 tag。
7. `nnode` 与 `node_editor` 都不是可长期保存的稳定指针：前者受 epoch 约束，后者只在收到
   它的策略/回调期间有效。

证据至少包含一个固定的 tag 组合反例，以及与 `vector` 的固定种子随机对拍：每步检查
枚举序列、整树 info 和随机/全部短区间 info。涉及 splice 时 oracle 也必须使用删除后的
`at` 坐标。只用交换性区间和无法证明顺序、组合和反转字段正确。

### 19.7 每个新算法的证据包

至少包含：

1. 公共契约与失败边界；
2. 正确性不变量或证明；
3. 复杂度；
4. 固定例子；
5. 随机暴力/性质测试；
6. checked 与 unsafe 有效输入一致性；
7. sanitizer；
8. 文档和公共符号索引同步。

### 19.8 Nitori v3 图论拓扑施工路线

这一节是 v3 的长期施工合同。目标不是把所有后端改成同一个容器，而是像建塔一样把
“位置/函数 → 资源/身份 → 可变结构 → 拓扑投影 → 图树算法”逐层装配。**已有 public API、
后端、算法和能力不得删除**；新层先以兼容适配器存在，只有在独立证据证明语义、复杂度和
常数都不退化后，才可以让旧入口改为委托实现。

#### 19.8.1 不可退让的设计决定

```text
统一资源身份和失效协议，不统一所有物理表示。
复用 nview / nfunc / nobject_holder / nindexed / nenumerable，不新增图论专用的
nview 或 nfunc。
继续走自由策略路线；不建立 concept/trait 森林。
所有数学、所有权、顺序、生命周期和复杂度前提写在依赖它们的局部注释中。
图算法仍面向 ngraph_like；不能因为有资源后端就把 dense vertex id API 换掉。
```

这里有一个必须说清的边界：`nnode_domain<T>` 的 `T` 是资源记录类型，
`same_domain` 只表示同一份可变资源池，不能伪装成跨类型的万能句柄。FHQ 节点、动态线段
树节点和图记录的物理类型可以不同；它们通过 `nnode_identity` 和 owner 级交易协议协作，
而不是互相强转 handle。跨类型组合若需要共同失效，增加的只能是一个极小的共享 epoch
token（暂名 `nnode_scope`），它只负责生命周期，不负责存储、算法或类型约束。

#### 19.8.2 拓扑高塔

```text
L0  nview / nfunc / nindexed / nenumerable / nfor
    └─ 只表达位置、映射和借用；不拥有图节点

L1  nresource_pool<T> / nnode_domain<T> / nnode_identity / nnode_view<S>
    └─ handle、generation、domain、epoch；不解释 T 是树节点还是图记录

L2  既有结构引擎
    ├─ nimplicit_fhq：同 domain 的 destructive split/merge、自由 policy
    ├─ nset_fhq / nset_splay：有序树的分割、合法有序 join、增强信息
    ├─ nseg / nlazyseg：定长叶的 pointwise merge
    ├─ npersistent_seg：append-only version/path-copy/merge
    └─ ndynamic_seg / ndynamic_lazyseg：稀疏 domain 的 destructive merge

L3  资源化拓扑
    ├─ graph topology record：vertex/arc 共用一个带标签的资源记录类型
    ├─ owning topology：稳定 node identity、head/next/prev 和 topology epoch
    ├─ borrowed topology projection：借用左值 owner，沿旧枚举协议访问
    └─ immutable compact projection：CSR 保持独立的缓存友好表示

L4  树/森林投影
    ├─ ntree_layout：旧字段和旧 dense API 的兼容物化层
    ├─ rooted tree/forest node owner：parent、children、component、Euler 顺序
    ├─ nview/nfunc：vertex ↔ position ↔ payload 的投影
    └─ FHQ/segment family：组件拆并、序列搬运、路径/子树数据维护

L5  算法
    └─ BFS/DFS、最短路、topo/SCC、LCA、HLD、reroot、MST、flow、matching 以及后续算法
```

`nnode_view<S>` 是 L1 的统一借用结构视图。它只承诺身份、当前性、`val()` 和 owner
提供的资源字段；树的 `left/right/parent`、lazy 的 `tag`、区间边界等是分层特化，不能
把图的邻接链硬解释成二叉树。必要时给 `nnode_view` 增加可选的局部访问器，但不复制一
份 `ngraph_view`/`ngraph_func`。`nseg_node<S>` 保留现有公共类型和区间 carry 语义，内部
只应与 `nnode_view` 共享身份/epoch 底座，不应再出现第三套 lifetime 检查。

#### 19.8.3 图资源层的候选形态

先实现一个资源化 owning topology（候选名 `ngraph_topology<W,V>`，名称和字段在测试后
冻结），其资源记录至少包含：

```text
kind(vertex/arc)、from/to、next/prev、public id、edge weight、可选 vertex payload
```

顶点和弧使用同一个 `nnode_domain<record>`，所以一个 node view 可以表达两者；记录的
`kind` 是数据，不是新的 view 类型。dense vertex id 和现有 edge id 仍由 owner 的
`nvector<int>` 映射维护，用户不会被迫改用资源 handle。所有 `make/erase/rewire` 在一个
公共操作中完成，恢复 head/next/prev、弧计数和 owner 根集合后只推进一次 topology epoch。

后端的职责明确分开：

| 后端 | 所有权/拓扑 | v3 处理 |
|---|---|---|
| `ngraph_forward` | owning、可追加、低常数 | 第一迁移目标；委托资源层但保留 `add/find/weight/set/arcs` 和 edge id |
| `ngraph_list` | owning、朴素参考 | 保留为最小正确性 oracle；不为统一而牺牲可读性 |
| `ngraph_csr` | owning、拓扑不可变、紧凑 | 保留独立 CSR；它是 immutable projection，不伪装成可删除资源池 |
| `ngraph_view` | borrowed、按需邻接 | 原样复用 `nview`/枚举和 owner 生命周期契约 |
| `ngraph_where_view` | borrowed、过滤 projection | 继续只保存左值 graph + predicate；不复制资源 |

`narcs`、`nvertices`、`ngraph_where` 和所有算法继续通过 `nfor`/`nenumerate` 消除后端
差异。资源节点访问只增加 `vertex_node/arc_node` 之类的 owner 入口并返回现有
`nnode_view<Owner>`；不公开第二套图节点对象。图的 `nnode_view` 不承诺 `left/right`，邻接
遍历仍是 enumerable cursor，避免把“资源链接”误当成“树结构”。

#### 19.8.4 树与既有 DS 的接合

`ntree_layout` 不能被删除或偷偷改成另一种字段语义。它继续是从任意 `ngraph_like` 得到
的 dense、可检查、可读 layout；资源化树 owner 作为上层新增能力，必要时导出同样的
`adjacency/parent/order`，让 `nlca`、`nhld`、`nreroot` 先零改动接入。

组件拆并时严格按下面的矩阵选择操作，禁止直接复制裸 handle：

| 结构 | 共享条件 | 正确交易 | 旧视图 |
|---|---|---|---|
| FHQ/splay | 同一节点记录 domain；root 不重叠 | `split_at/split_by` 或有序 `merge_from` | 共享 epoch 后失效 |
| fixed/lazy seg | 长度和操作/action 语义相同 | 逐叶 `merge_from`，不是 root aggregate 合并 | 两 owner 旧 view 失效 |
| persistent seg | 旧节点不可变 | version `merge`/path-copy；不 destructive erase | 旧版本保持 current |
| dynamic seg | 同 domain、同 bounds、root 不重叠 | union-of-materialized-nodes 的 destructive merge | shared domain 旧 view 失效 |
| graph/tree topology | 同资源 domain 或显式迁移 | 重连资源链并更新 owner root 集合；跨类型 DS 调其自身交易 | topology transaction 后统一失效 |

树节点记录只保存能证明拓扑不变量所需的 parent/child/component/顺序字段；路径摘要、
区间信息和 lazy tag 留在现有 FHQ/segment policy 中。vertex 到 Euler/HLD position、
position 到 vertex、vertex 到 payload 的映射分别用 `nview`/`nfunc` 表达，不能用一个高阶
graph facade 把 position、key、value 混成一个类型。

跨多个 typed domain 时，未来的 `nnode_scope`（若窄测试证明确有必要）只能提供共享
epoch：`same_domain` 仍然必须为真才可转移资源，scope 不能授权跨池 merge。V3-1 暂不
引入一个脱离 owner 的空壳 token；当前所有已存在的跨 owner 交易都在同一 typed domain
内，`ni::nnode_stamp` 已足以统一身份、generation 和 epoch 检查。等图记录与树节点真的
需要同一笔跨 typed domain transaction 时，再把 scope 接入 owner 并为它补独立失效测试。
一个 component transaction 要么调用各子结构的 `merge_from/split`，要么 clone/rebuild；
任何只复制 `int handle` 的实现都是错误的。

#### 19.8.5 分阶段施工与回滚点

每阶段独立提交，前一阶段的全套 checked/unsafe/property/death/sanitizer 证据必须保留：

| 阶段 | 施工内容 | 必须新增的证据 |
|---|---|---|
| V3-0 | 已完成基线：node domain、FHQ domain、五类 segment merge | 现有全库、audit、两 profile sanitizer |
| V3-1 | 抽出 `nnode_view`/`nseg_node` 共用的 identity/epoch/generation 检查；评估 scope token，暂不引入未接入 owner 的空壳 | 固定 stale/ABA/move/copy 测试；无公共签名回归 |
| V3-2 | 资源化 owning graph topology；先做 node identity、邻接链和只读枚举 | graph topology fixed/property/death；list/forward/CSR differential |
| V3-3 | 让 `ngraph_forward` 以兼容 facade 接入资源层；锁定 edge id、顺序、权值修改语义 | 旧 graph/compat 全测；随机 add/weight/reverse/narcs 对拍；ASan/UBSan |
| V3-4 | 把 `ntree_layout` 变成兼容投影并增加 rooted tree/forest owner | 连通/无环/对称/root/order death；LCA/HLD/reroot differential |
| V3-5 | 组件拆并与动态森林底座：FHQ Euler 序列 + 既有 segment family；先实现可验证 link/cut 子集 | vector oracle、字符串非交换聚合、跨组件 merge/split、stale view death |
| V3-6 | 算法层适配：遍历/最短路/topo/SCC/MST、LCA/HLD/reroot、flow/matching 的资源入口 | 各后端结果等价；流/匹配独立 oracle；递归深度与容量边界测试 |
| V3-7 | 性能和提交体验收口；只在 benchmark 证明不退化时替换更多后端 | deterministic benchmark、checksum、内存/节点计数、两 profile valid-input 等价 |

阶段中止条件是“接口能实例化”之外的任何失败：若复杂度、edge 顺序、旧字段、view
失效时机或非交换聚合无法对拍，就保留兼容后端，回滚当前适配器，不删旧实现。每个新类
都必须在局部注释写出功能、前提、生命周期、顺序和复杂度；没有必要的语法能力不新增
concept。

#### 19.8.6 测试和发布门禁

每个拓扑阶段至少补一组 fixed、一组 property/differential、一组 checked death；涉及
资源或递归时再加 sanitizer 和 benchmark。固定种子必须覆盖空图、单点、平行弧、自环、
不连通、父子单向/双向、极深链、星形树、非交换字符串聚合、跨 domain、移动后 owner、
删除后 generation 复用和 pending lazy tag。

阶段门禁固定为：

```bash
python3 tools/amalgamate.py
python3 tools/amalgamate.py --check
python3 tools/test.py <窄测试>
python3 tools/test.py
python3 tools/test.py --profile checked --sanitize
python3 tools/test.py --profile unsafe --sanitize
python3 tools/audit.py
python3 tools/audit_authority.py
python3 tools/bench.py
```

未实际执行的命令不得写成“已通过”。`Nitori.h` 和 `Nitori_unsafe.h` 始终由
`tools/amalgamate.py` 生成；源码、测试和本文是唯一需要审查的变更，v3 施工不建立第二
份文档、第二份头文件或隐藏兼容实现。

---

## 20. 旧版到 Nitori X 的迁移桥梁

Nitori X 不以“删掉旧能力换一个小而美的壳”为目标。它复用旧版竞赛经验，但把每个
能力重新放进统一的所有权、view、枚举、合并操作和后端层级。迁移原则是：有真实替代就
给出明确路径；没有等价替代就保留名字或单独后端，禁止静默消失。

### 20.1 已恢复的实现族

| 旧版/习惯入口 | Nitori X 主入口 | 后端或说明 |
|---|---|---|
| `nvector` | `nvector` | 自主 owner；`nvector_stl` 为同实现别名 |
| `ndeque` | `ndeque` | 默认真正环形 `ndeque_ring`；`ndeque_stl` 为参考后端 |
| `nheap` | `nheap` | 默认真正二叉堆 `nheap_binary` |
| `nset` / `nbag` | 同名 | 默认 FHQ；另有 `nset_splay/nset_stl` |
| `nmap` | `nmap` | 默认开放寻址 `nmap_flat`；另有 `nmap_hash/nmap_stl` |
| `nrel` | `nrel` | 当前线性关系后端 `nrel_scan` |
| 稀疏函数/部分函数 | `nfunc_hash` / `npartial` | 哈希绑定表，不与新 `nfunc` 混义 |
| `nbije` / `ninj` | 同名 | 双向哈希实现 `nbije_hash` |
| rank 双射/压缩 | `nbije_rank` / `ncompress` | STL 输入另用 `ncompress_stl` |
| 可扩展有序树 AST | `nnode` + augmentation 对象 | FHQ/splay 都实现，带 epoch 快照诊断 |
| 隐式 FHQ / rope | `nimplicit_fhq` / `nseq_fhq` | 中序即位置；自由策略拥有 info/state/tag/push/pull |
| `npool` | `npool` | 1-based 可删除复用 handle；与 `narena` 分离 |
| `nsparse` | `nsparse` | 升级为任意有单位元结合操作的 disjoint sparse table |
| 势能并查集 | `npotential_dsu` | 差值约束与一致性检查 |
| `ngraph` | `ngraph` | 默认 `ngraph_forward`；另有 `ngraph_csr/ngraph_list` |
| 图过滤/遍历 | `ngraph_where/nvertices/narcs` | 零复制 capability view |
| BFS/Dijkstra 路径 | `nbfs_path/ndijkstra_path` | `npath_result` 统一恢复路径 |
| topo/SCC | `ntopo`、`nscc_*` | 默认迭代 Kosaraju，另有 Tarjan |
| HLD/LCA | `nhld`、`nlca_binary` | 严格树入口仍为 `nlca` |
| MST | `nprim/nkruskal` | 连通树或最小生成森林 |
| `nflow` | `nflow_dinic` / `nflow` | 可 reset、限流、查边流量；push-relabel 保留为 `nmaxflow` |
| `nbimatch` | `nbimatch_hopcroft` / `nbimatch` | 状态式加边、pairs、最小点覆盖 |
| `nmod` / `ndmod` | 同名类型 | 静态/动态模整数，核心名 `nmodint/nmod_dynamic` |
| 因数分解 | `npollard/nfactor` | uint64 Pollard-Rho；trial/miller 后端也可显式选 |
| 概率/博弈 | `nprob/nnim/nxorbasis/nsg` | 各自保持清楚数学前提 |
| `nmat` | `nmat` | 建立在现代 `nmatrix` owner/view 存储上 |
| `npoly` | `npoly` | owner facade；卷积/FPS 函数接口仍可单独使用 |
| 离散 Li Chao | `nlichao_static` | 坐标压缩；动态整数域入口仍为 `nlichao` |
| 字符串短名 | `nprefix/nzfunc/nkmp` | 严格核心名仍保留 |
| 几何短名 | `nonseg/nline2/nline_intersect` | epsilon 与精确入口并存 |

### 20.2 七个有意的语义拆分

1. **`nspan` / `nview`**：Nitori X 只保留 `nview<T,Accessor>`。连续性由 accessor 是否暴露
   `data()` 静态决定；`nview<T>` 就是原连续借用的零开销入口，不再维护两套切片体系。
2. **`nfunc`**：Nitori X 的有限函数是 `nview` 上层 keyed 高阶抽象；结果策略拆为
   `nfunc_value/nfunc_ref/nfunc_eval`，按枚举绑定使用 `nfunc_bind`，分流拆为
   `nbranch_value/nbranch_ref`。`nredomain`、`nrestrict`、`nselect_positions` 分别表达
   改 support、成员限制和源位置选择。旧 `nfunc/nbranch/ngather` 只保留 deprecated
   迁移入口；同时 indexed + invocable 的旧 `nfunc` 参数会直接要求调用者选明契约。
   可动态增删绑定的部分函数仍使用 `nfunc_hash/npartial`。
3. **`nmod`**：`nmod<M>` 恢复为模整数类型；标量数学余数必须写 `nmodulo(x,m)`。
4. **`nmatrix` / `nmat`**：前者负责存储、行列/对角 view 和显式 semiring 算法；后者
   绑定 `Add/Mul`，提供 `+/*/pow/trans/eye` 的竞赛短桥。
5. **`narena` / `npool`**：前者 0-based 连续 bump + rollback，后者 1-based 独立删除和
   槽位复用。不能再用同一名字掩盖两种互斥生命周期。
6. **`nlichao` / `nlichao_static`**：前者在线整数域动态开点，后者预知坐标、排序去重并
   只允许查询离散点。
7. **ordered FHQ / implicit FHQ**：`nset_fhq` 维护比较器定义的 key 顺序，tag 必须保持
   BST 有序；`nimplicit_fhq/nseq_fhq` 以子树长度定义位置，允许策略交换孩子、派生不同
   子 tag 和重排自定义摘要。二者共享随机 split/merge 思想，不共享排序不变量。

### 20.3 循环宏迁移

```cpp
nfor(x, sequence)          // 单层循环
nfori(i, x, sequence)      // 单层循环 + 枚举编号
nforkv(k, v, sequence)     // 单层循环 + semantic key/value
nrep(i, count)
nrrep(i, count)
```

Nitori X 的宏只展开成一个 range-for。`break` 退出整个宏循环，`continue` 前进一项，序列/
次数只求值一次。旧内部辅助名 `nfor0/nfor1/nfori0/nfori1/nforkv0/nforkv1` 和
`nrep0/nrep1/nrrep0/nrrep1` 不再是公共 API；它们的存在本来只是宏实现细节。

### 20.4 不迁移实现内部名字

旧 cursor/view 的实现名（例如 `nrange_cursor`、`nspan_cursor`、`nzip_cursor`、
`nproduct_cursor`）和模块标签（例如 `nassoc`、`nfinite`、`nlinear`、`ngeom`）不构成
用户能力，Nitori X 不承诺逐字保留。公开替代分别是 `nenumerate/nenumerator_t`、对应 view
构造器和本章索引中的真实算法类型。若旧代码直接依赖这些内部名字，应改写到能力接口，
而不是再造一套冻结内部布局的兼容壳。

旧实现、`Nitori_naive`、迁移报告和构建产物已经移出仓库。checked 与 unsafe 来自
同一组 `src` 语义模块，不维护第三份“naive”实现；参考验证由独立暴力/property tests
承担。

### 20.5 迁移验证顺序

```text
先以 Nitori.h 编译
→ 修正名字拆分和 [l,r)
→ 检查 owner/view 与 position/key/value
→ 运行旧样例和 Nitori X 独立测试
→ 用暴力对拍关键算法
→ 最后换 Nitori_unsafe.h 再编译运行
```

不要直接全局替换类型名后切 unsafe；checked 的契约失败正是迁移期要保留的诊断台架。

---

## 21. 完整公共符号索引

本索引列出面向用户的稳定搜索入口，不代替各章节前提。`ni` 命名空间及 `ni_n*`
宏支撑是实现细节；头文件中仅为拼装模板而暴露的 holder/cursor 类型也不是独立能力。

### 配置、基础、检查与随机

```text
npre nassert nversion nunsafe
npos nwide_t ninf nninf nmaybe
nchmin nchmax nlen nbitceil
nless ngreater nequal nidentity
nrng nseed_value nrng_global nhash_seed nseed nhash
```

### 操作对象

```text
nadd nmul nxor nmin nmax naddsum_action npow
```

### 引用、枚举与组合 view

```text
nview nindexed nindex_reference_t nindex_value_t
nreference_indexed nswappable_indexed ncontiguous_indexed nresizable
nrange_object nview_object nviewable_indexed
nall nsub nstride nrow ncolumn ndiagonal
nrange_t nrange nenumerator_t nenumerable nenumerate
nkeyvalue_enumerable nfor nfori nforkv nrep nrrep
nreverse nproject nzip nproduct nwindows
ncollect ntabulate
```

### 离散函数

```text
nkeyed_indexed ndiscrete_function
nfunction_key_reference_t nfunction_key_t nevaluated_function
nfunc_value nfunc_ref nfunc_eval nfunc_bind nfunc
nkeys nvalues nentries nredomain_function nredomain nrestrict nanchors
nbranch_value nbranch_ref nbranch nruns
ncomposed_function ncompose nmap_values nselected_positions_function
nselect_positions ngather nsubfunc nblock nblocks
```

### 拥有型序列与算法

```text
nvector nvector_stl ndeque_ring ndeque_stl ndeque
nheap_binary nheap narray
nfill nassign nswap_ranges
nfind_if ncontains ncount ncount_if nall_of nany_of nnone_of nsame
nargmin nargmax nsort nreverse_inplace nfind nlower nupper nfind_sorted nfold
nunique_compact nunique nsort_unique
```

### 机制、内存、有限对象、关联对象与 AST

```text
nscan nsuffix_scan nfirst_true nlast_true nrollback
nscratch narena npool_dynamic npool
nresource_pool nnode_domain nnode_pool nnode_identity
npartition npart npart_dense nperm
nbranch nnode_view nnode nwalk nempty_augment nempty_tag nfirst_prefix nlast_suffix
nfhq_policy nimplicit_fhq nseq_fhq
nseg_node nseg_walk
nset_fhq nset_splay nset_stl nset nbag
nmap_flat nmap_hash nmap_stl nmap
nrel_scan nrel npartial_hash nfunc_hash npartial
nbije_hash nbije_rank nbije ninj ncompress ncompress_stl
```

### 数据结构与离线算法

```text
nfenwick nseg nseg_iter nlazyseg nlazy_addsum
ndynamic_seg ndynamic_lazyseg ndynamic_addsum
nqueue_agg ndsu nrollback_dsu ndsu_rollback npotential_dsu
npersistent_seg nwavelet nsparse
ninterval_query nmo_order nrun_mo
```

### 图、树、流与匹配

```text
narc nedge nedge_to nedge_weight ncapadd
ngraph_view ngraph_like ngraph_list ngraph_forward ngraph_csr ngraph
nvertices ngraph_arcs_view narcs ngraph_where_view ngraph_where
npath_result nbfs_path ndijkstra_path nbfs ndijkstra n01bfs
ntoposort ntopo nscc nscc_kosaraju nscc_tarjan
nlca nhld_segment nhld nlca_binary nreroot
nmst_result nprim nkruskal
nmaxflow nflow_dinic nflow
nbipartite_matching nhopcroft_karp nbicover nbimatch_hopcroft nbimatch
```

### 整数、模运算、组合与博弈

```text
ninteger nmag nabs ngcd_euclid ngcd_binary ngcd nlcm
nfloor_div nceil_div nmodulo nextgcd_result nextgcd
nmulmod npowmod nisprime nisprime_trial nisprime_miller nprimes npollard nfactor nfactor_rho
nfrac ncongruence ncrt nprime_table
nmodint nmod_static nmod nmod_dynamic ndmod ncomb
nsubmask_range nsubmasks
nzeta_subset nmobius_subset nzeta_superset nmobius_superset
nfwht_xor nconv_or nconv_and nconv_xor
nxorbasis nprob nexpect nnim nsg_dag nsg
```

### 矩阵、线性代数与多项式

```text
nmatrix nmatrix_like nmatrix_identity nmatmul nmatpow nmat
nrref ndeterminant ndet ninverse nlinear_solution nlinear_solve ngauss
nntt_info nconv_naive nconv_ntt nconv_auto nconv
npoly_derivative npoly_integral npoly_evaluate nfps_inverse npoly
nberlekamp nrec_nth
```

### 字符串与自动机

```text
nprefix_function nprefix nz_function nzfunc nkmp_find nkmp
npalindrome_index nmanacher nmanacher_result
nsuffix_array nlcp_array
nmatch ntrie nac
```

### 几何、优化与 I/O

```text
npoint ndot ncross norient ndist2 nsgn_eps nsign
non_segment nonseg nsegment_intersect
nline2 nline_intersect nline_intersection
nconvex_hull npolygon_area2 npoint_in_poly nconvex_diameter2
nline_function nlichao nline nlichao_static nunimodal_arg nternary_min
ninput noutput nin nout nread nprint nprintln
```

---

## 权威维护声明

Nitori X 的公共事实只允许存在于：

1. `/home/tnuzy/NitoriSTL/Nitori.h`：checked 公共实现；
2. `/home/tnuzy/NitoriSTL/NITORI_DOCUMENT.md`：本文。

全局 Codex skill 只能引用这两个路径并给出使用流程，不得保存 header、API 表、recipes
或 diagnosis 的副本。比赛目录中的拷贝只是临时提交材料，不是权威源。任何缓存、旧 skill、
旧参考文档或资产快照都不得覆盖这两件原稿。
