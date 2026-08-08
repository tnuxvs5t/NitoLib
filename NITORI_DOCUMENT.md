# Nitori X

> 从第一份可提交程序，到零成本 view、离散函数与竞赛算法装配的唯一权威文档
>
> 适用版本：`nversion == 20000`
>
> checked 头文件：`/home/tnuzy/NitoriSTL/Nitori.h`
>
> unsafe 头文件：`/home/tnuzy/NitoriSTL/Nitori_unsafe.h`

本文同时承担两件事：前半部分让第一次接触 Nitori 的选手尽快写题，后半部分提供
可检索、可验证的完整 API 契约。不要先背完整符号表；先掌握下面这条主线：

```text
输入输出 → nvector 与循环 → 通用算法 → nview → projection
→ 明确物化 ncollect → 数据结构/图论/数学 → nfunc 与自定义扩展
```

Nitori X 是面向算法竞赛的 GNU C++20 单头文件系统。它不是给 STL 名字机械加 `n`，
也不是要求选手先学一套工程框架。它要解决的是：让一份算法能够直接作用在 vector、
deque、矩阵行列、步长序列、lambda 映射位置和离散函数上，同时保持代码短、引用真实、
复杂度明确，并在训练阶段尽早抓住越界和错误前提。

本文是 Nitori X 唯一的用户文档原稿。不得在 skill、比赛目录、assets 或其他
reference 中复制本文或头文件。公共签名与真实行为以 checked 头文件为最终事实；
数学前提、使用语义、复杂度和工作流以本文为最终说明。若二者不一致，停止扩散，
以实现和测试重建结论并立即修正文档。

---

## 怎样阅读本文

### 第一次使用：只读这些

1. 第 1 章：写出第一份完整程序；
2. 第 2 章：记住 `int` 下标、`[l,r)` 和 owner/view 生命周期；
3. 第 6 章：先理解 `nview`、projection 与 `ncollect`；只有需要语义 key 时再读 6.8；
4. 第 8 章：按表查容器和序列算法；
5. 第 16 章：查 I/O 边界。

### 正在做题：按题型跳转

| 题目特征 | 先看 |
|---|---|
| 区间查询、区间修改、滑动窗口、并查集 | 第 10 章 |
| BFS、最短路、树、流、匹配 | 第 11 章 |
| gcd、模运算、组合、博弈、异或基 | 第 12 章 |
| 矩阵、线性方程、卷积、多项式 | 第 13 章 |
| KMP、后缀数组、Trie、AC 自动机 | 第 14 章 |
| 几何、Li Chao、单峰优化 | 第 15 章 |
| 状态 key、分块对应、惰性分流、连续段组合 | 第 6.8 节 |
| 想把多个组件装成短代码 | 第 17 章 |

### 开发 Nitori：再读这些

第 3、5、9、18、19、20、21 章面向 profile、操作契约、AST、测试、扩展、迁移和
公共符号审计。普通做题不需要先读完。

---

## 目录

1. [十分钟快速开始](#1-十分钟快速开始)
2. [不可违反的全局约定](#2-不可违反的全局约定)
3. [checked 与 unsafe](#3-checked-与-unsafe)
4. [基础值、哨兵和可选结果](#4-基础值哨兵和可选结果)
5. [操作对象与隐形契约](#5-操作对象与隐形契约)
6. [统一 Range/View/Projection 与离散函数](#6-统一-rangeviewprojection-与离散函数)
7. [枚举协议与组合视图](#7-枚举协议与组合视图)
8. [拥有型序列与通用算法](#8-拥有型序列与通用算法)
9. [机制、内存、关联对象与 AST](#9-机制内存关联对象与-ast)
10. [代数数据结构](#10-代数数据结构)
11. [图、树、流与匹配](#11-图树流与匹配)
12. [整数、模运算与组合数学](#12-整数模运算与组合数学)
13. [矩阵、线性代数与多项式](#13-矩阵线性代数与多项式)
14. [字符串、Trie 与 AC 自动机](#14-字符串trie-与-ac-自动机)
15. [几何与优化](#15-几何与优化)
16. [竞赛 I/O](#16-竞赛-io)
17. [典型装配配方](#17-典型装配配方)
18. [调试、测试与提交工作流](#18-调试测试与提交工作流)
19. [扩展 Nitori 的规则](#19-扩展-nitori-的规则)
20. [旧版到 Nitori X 的迁移桥梁](#20-旧版到-nitori-x-的迁移桥梁)
21. [完整公共符号索引](#21-完整公共符号索引)

---

## 1. 十分钟快速开始

这一章不要求理解模板、concept、游标或代数定律。目标只有一个：十分钟后，你能用
Nitori 读入数据、保存序列、遍历、排序、切片、输出，并知道何时会产生副本。

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

static_assert(nversion == 20000);
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

### 1.7 `nview`：引用一组位置，不复制元素

这是 Nitori 最重要的抽象。先看最小例子：

```cpp
nvector<int> a{9, 4, 7, 1, 8};

auto middle = nsub(a, 1, 4); // 引用 a 的 [1,4)
nsort(middle);
// a == {9,1,4,7,8}
```

`middle` 不是新数组。它只保存“第 `i` 项应该访问 `a[1+i]`”这条规则，因此排序会写回
`a`。其他常用 view：

```cpp
auto all = nall(a);
auto backwards = nreverse(a);
auto every_second = nstride(a, 0, a.len(), 2);
auto pairs = nzip(a, backwards);
auto windows = nwindows(a, 3);
```

lambda 也能定义位置映射，而且返回真实引用时可以直接修改和排序：

```cpp
auto even_positions = nview((a.len() + 1) / 2,
    [&](int i) -> int& { return a[2 * i]; });

nsort(even_positions);
```

二维对象仍然使用同一套算法：

```cpp
nmatrix<int> matrix(4, 4, 0);
nfill(matrix.row(1), -1);
nsort(matrix.column(2));
nsort(matrix.diagonal());
```

### 1.8 view 的复制、物化与写入

这是新人最容易混淆的地方：

```cpp
auto view = nreverse(nsub(a, 1, 4));
auto alias = view;           // O(1)：复制访问描述，仍引用 a
auto copy = ncollect(view);  // O(n)：建立独立 nvector
```

记忆表：

| 操作 | 是否复制元素 | 修改后是否写回 owner |
|---|---:|---:|
| `auto b = view` | 否 | 是 |
| `nall/nsub/nreverse/nstride/nproject` | 否 | 是 |
| `ncollect(view)` | 是 | 否 |
| `nassign(destination,view)` | 写入既有目标 | 取决于 destination |

矩阵 DP 不需要 `.data()` 或 `std::copy`：

```cpp
nmatrix<long long> rows(blocks, width, 0);
nvector<long long> dp(width, 0);

nassign(rows.row(block), dp);
nfill(rows.row(block), nninf<long long>);
```

生命周期规则只有一句话：**view 不拥有最底层 owner；owner 必须活着，而且不能发生使
引用失效的扩容或结构修改。**

### 1.9 两种 projection，作用完全不同

假设：

```cpp
struct item {
    int key;
    int payload;
};

nvector<item> items{{3, 30}, {1, 10}, {2, 20}};
```

算法 projection 只决定“按什么观察和比较”，交换的仍是完整记录：

```cpp
nsort(items, nless<>{}, &item::key);
// (key,payload) 关系保持，顺序变成 (1,10),(2,20),(3,30)
```

`nproject` 则建立字段引用 view，排序时只交换字段：

```cpp
auto keys = nproject(items, &item::key);
nsort(keys); // 只重新排列 key；payload 留在原位置
```

大多数“按字段排序记录”的题目需要第一种。只有确实想单独操作某一列/字段时才使用
`nproject`。

### 1.10 一个真正体现 Nitori 的 0/1 背包转移

普通写法用倒序下标保证每件物品只使用一次。Nitori 可以把这个依赖方向直接表达为
两个倒序 view：

```cpp
auto relax = [&](auto&& dp, int weight, long long value) {
    const int width = nlen(dp);
    auto destination = nreverse(nsub(dp, weight, width));
    auto source = nreverse(nsub(dp, 0, width - weight));

    nfor(cell, nzip(destination, source)) {
        auto&& [to, from] = cell;
        if (from != nninf<long long>)
            nchmax(to, from + value);
    }
};
```

这里没有 iterator、裸指针或容器特判。只要 `dp` 是可写 indexed range，同一段代码就能
作用于 `nvector`、矩阵行、切片或 lambda view。倒序不是装饰：它保护“读取旧层状态”
这个 0/1 背包不变量。

### 1.11 checked 调试，unsafe 提交

训练时默认 checked。常见错误会在离问题最近的位置中止：

- 下标越界；
- 非法 `[l,r)`；
- `nassign` 两侧长度不等；
- 空堆取 top；
- 非法图顶点；
- 读入溢出；
- 违反已编码的结构前提。

推荐最小流程：

```text
checked 编译
→ 跑样例
→ 跑空、单点、极值边界
→ 小规模暴力对拍
→ 再生成 unsafe 提交
```

unsafe 不会修复错误，也不是“忽略 assert 继续跑”。其中 `npre(condition)` 会把
condition 当作优化器可以相信的事实；条件为假时行为未定义。

### 1.12 十分钟检查点

如果你已经能回答下面七个问题，就可以停止阅读教程，直接做题并按需查后文：

1. 为什么长度使用 `a.len()` 而不是 `.size()`？
2. `nrep`、`nrrep`、`nfor` 分别适合什么循环？
3. 为什么所有区间都写成 `[l,r)`？
4. `nsub(a,l,r)` 会不会复制元素？
5. 怎样得到 view 的独立副本？
6. `nsort(items,cmp,projection)` 与 `nsort(nproject(items,projection))` 有什么区别？
7. 为什么训练时应使用 checked？

如果第 4～6 题仍不确定，重读 1.7～1.9；那是 Nitori 与普通容器模板最关键的区别。

---

## 2. 不可违反的全局约定

### 2.1 索引和区间

- 长度、位置、顶点、版本号统一使用 `int`。
- 不存在的位置使用 `npos == -1`。
- 区间统一使用半开区间 `[l,r)`。
- 合法空区间满足 `l == r`。
- 越界不是可恢复查询，而是前置条件错误。

### 2.2 所有权

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
- 会立即悬垂的临时 owning container 被 concept 拒绝，例如 `nall(nvector<int>{...})`；
  临时 view 则可以安全进入下一层组合器。

### 2.3 命名

- 公共全局名字以小写 `n` 开头。
- 内部实现位于 `ni`，用户不得依赖。
- 成员已由对象作用域隔离，不重复加 `n`：`a.len()`、`f.fold(l,r)`、
  `g.add(u,v)`。

### 2.4 数学前提

C++ concept 只能验证语法，不能证明结合律、单位元、交换律、可逆性、单调性、
树结构或非负边权。调用者必须保证真实数学条件成立。

### 2.5 整数与溢出

- 几何整数积和直线函数求值通过 `nwide_t<T>` 扩为 `__int128_t`。
- `nlcm`、输入解析和若干尺寸计算检查表示范围。
- 普通用户表达式和权值相加不会自动无限扩宽；选对 `long long`/`__int128_t`。
- `ninf<T>` 是算法哨兵，不是数值类型的真正无穷。

---

## 3. checked 与 unsafe

| 项目 | checked | unsafe |
|---|---|---|
| 文件 | `Nitori.h` | `Nitori_unsafe.h` |
| `nunsafe` | `false` | `true` |
| `npre(false)` | 打印表达式、文件、行号后 `abort()` | `__builtin_unreachable()` |
| 合法输入语义 | 与 unsafe 相同 | 与 checked 相同 |
| 用途 | 训练、开发、调试、性质测试 | 已验证的竞赛提交 |

两个头文件由 `src/manifest.txt` 的同一语义源生成。任何手改生成头都会被
freshness 审计拒绝。

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
| `nversion` | 当前为 `20000` |
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

通用幺半群快速幂：

```cpp
auto x = npow(base, exponent, operation);
```

非负指数只要求幺半群；负指数要求 `operation` 另外声明并实现群逆元。复杂度
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

## 6. 统一 Range/View/Projection 与离散函数

### 6.1 三层协议，不建立 iterator/trait 森林

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

### 6.3 最小能力概念

| Concept/trait | 含义 |
|---|---|
| `nindexed<A>` | `nlen(a)` 与 const/non-const `a[i]` 存在 |
| `nindex_reference_t<A>` | `a[i]` 的引用类型 |
| `nindex_value_t<A>` | 去 cv/ref 后的元素类型 |
| `nreference_indexed<A>` | 索引返回 lvalue reference |
| `nswappable_indexed<A>` | 元素是可写且可交换的 lvalue |
| `ncontiguous_indexed<A>` | 另外提供 contiguous `data()` |
| `nresizable<A>` | 提供 `resize(int)` |
| `nrange_object<A>` | 可安全按值接管的 range 描述符 |
| `nview_object<A>` | 真正的 `nview<T,Accessor>` 描述符 |
| `nviewable_indexed<A>` | indexed 左值 owner，或可复制/移动进组合器的 range 描述符 |

算法只要求实际需要的能力。例如 `nfind` 只需要索引读取，`nsort` 需要可交换引用，
`nunique` 还需要 resize。

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
唯一、有序或可哈希；旧 `ndiscrete` 暂时保留为 `nkeyed_indexed` 的兼容别名，新代码
必须写出准确能力。需要运行时绑定/解绑键值时使用 `nfunc_hash`/`npartial`。

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

---

## 7. 枚举协议与组合视图

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
| `nfold(a,l,r,op,proj)` | 投影值类型上的 op 为幺半群 | `O(r-l)`，保持顺序 |
| `nunique_compact(a,equal,proj)` | 相邻投影 key 等价元素压缩 | `O(n)`，返回保留长度但不 resize |
| `nunique(a,equal,proj)` | 另需 `resize` | `O(n)`，并缩短容器 |
| `nsort_unique(a,cmp,equal,proj)` | 可排序且可 resize | `O(n log n)`，排序后去重 |

`nunique` 只压缩相邻等价项；通常先用相同 projection 执行 `nsort`。

---

## 9. 机制、内存、关联对象与 AST

### 9.1 扫描

```cpp
auto prefix = nscan(a, op);          // 长度 n+1，prefix[0] = id
auto suffix = nsuffix_scan(a, op);   // 长度 n+1，suffix[n] = id
```

两者保持操作顺序，因此支持非交换幺半群。

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

节点快照携带 epoch。任何可能改变拓扑的操作都会令旧快照 `current()==false`；尤其
splay 的 `has/get/rank/...` 也可能旋转，不能把 `nnode` 跨下一次树操作保存。FHQ 的纯
查询当前不改拓扑，但公共安全规则仍是“用完即弃”。`nempty_augment<T>` 是默认空信息，
`nwalk`、`nfirst_prefix` 和 `nlast_suffix` 直接按调用点所需接口实例化，不再额外注册 AST
concept；因此自定义 owner 可以自由调度节点，但必须自己满足上述接口和聚合单调性。

---

## 10. 代数数据结构

线段树家族共享结构视图 `nseg_node<S>`。`nseg`、`nlazyseg`、`npersistent_seg`、
`ndynamic_seg` 与 `ndynamic_lazyseg` 都提供 `root()`；持久化树另有 `root(version)`：

```cpp
auto node = seg.root();
node.aggregate(); node.info();
node.left_bound(); node.right_bound(); node.width(); node.leaf();
node.left(); node.right(); node.handle();

auto found = nseg_walk(seg, decide);             // 从 root() 调度
auto old_found = nseg_walk(p.root(version), decide); // 从指定节点调度
```

`decide(node)` 返回 `nbranch::left/take/right`。固定树和持久化树的视图区间是内部
`[0,bit_ceil(n))`，`n` 以后的叶子为单位元；
动态树的视图区间就是构造时给出的坐标域。普通/lazy/dynamic 树修改后旧视图 epoch 失效；
持久化节点不可变，所以旧版本视图保持有效。带 lazy 的节点额外有 `tag()`，它返回当前
节点尚未下推的表示 tag，而不是从根到节点的累计作用；沿 `left()/right()` 取得的子视图会
携带祖先未下推作用，因此 `aggregate()` 仍是该区间的逻辑聚合，且不会为动态树的只读视图
补开缺失节点。

### 10.1 Fenwick：`nfenwick<T,O>`

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

### 10.2 迭代线段树：`nseg<T,O>`

要求有序幺半群，不要求交换。

```cpp
nseg<string, nconcat> s(source, nconcat{});
s.set(i, value);
s.get(i);
s.fold(l, r);
s.fold();
s.clear();
```

单点与区间操作 `O(log n)`；整体 fold `O(1)`。别名 `nseg_iter`。

### 10.3 Lazy 线段树：`nlazyseg<S,F,M,A>`

- `S`：聚合类型。
- `F`：tag 类型。
- `M`：`S` 上的有序幺半群。
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

### 10.4 聚合队列：`nqueue_agg<T,O>`

保持队列顺序的双栈幺半群聚合：`push`、`front`、`pop`、`pop(fallback)`、
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

### 10.7 动态开点线段树

`ndynamic_seg<T,O>` 在 `long long` 坐标域 `[lo,hi)` 上只为写入路径开点；未开节点表示
幺半群单位元：

```cpp
ndynamic_seg<long long> seg(-1'000'000'000'000LL, 1'000'000'000'000LL);
seg.set(x, value);
seg.combine(x, delta);       // leaf = op(old, delta)，顺序不可交换
seg.get(x); seg.fold(l, r); seg.fold();
seg.nodes(); seg.reserve_nodes(capacity); seg.clear();
```

设坐标域宽度为 `W`：点修改和区间查询为 `O(log W)`，每个首次写入的点至多增加
`O(log W)` 节点；只读查询不分配节点。坐标域宽度必须能装入 `long long`。

`ndynamic_lazyseg<S,F,M,A>` 使用与 `nlazyseg` 相同的 monoid/action 协议，支持动态开点
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

### 10.10 Disjoint Sparse Table：`nsparse<T,O>`

`nsparse` 的名字兼容经典 sparse table，但实现是 disjoint sparse table：只要求有序
幺半群，不要求幂等，也不要求交换。因此字符串拼接等操作也能 `O(1)` 查询：

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

---

## 11. 图、树、流与匹配

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

---

## 12. 整数、模运算与组合数学

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

### 12.3 阶乘组合表 `ncomb<Mint>`

```cpp
ncomb<mint> c(max_n);
c.factorial(n);
c.choose(n, k);   // k 越界返回 0
c.permute(n, k);  // k 越界返回 0
```

构造要求 `factorial(max_n)` 可逆。素数模下常见充分条件是 `max_n < modulus`。
预处理 `O(n)`，查询 `O(1)`。

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

---

## 13. 矩阵、线性代数与多项式

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

`nntt_info<Mint>` 是 primitive-root 快速入口。内建为 `998244353`、`1004535809`、
`469762049` 提供根 3；其他适用 `nmodint<M>` 会在首次使用时自动分解 `M-1` 搜根。
若为另一个常用 `nmodint<M>` 补 trait，必须保证：

```cpp
template<> struct nntt_info<nmodint<M>> {
    static constexpr bool ok = true;
    static constexpr uint64_t root = ...; // 真正的本原根
};
```

错误 root 会静默产生错误卷积，测试必须与 `nconv_naive` 随机对拍。

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

---

## 14. 字符串、Trie 与 AC 自动机

字符串算法接受任意 `nindexed` 符号序列，不要求 `std::string`。

### 14.1 线性字符串算法

```cpp
nprefix_function(sequence);  // prefix[i]：结尾 i 的最长真 border
nz_function(sequence);       // z[0]=n
nkmp_find(text, pattern);    // 返回全部起点；空 pattern 匹配 0..n
```

兼容短名分别是 `nprefix`、`nzfunc`、`nkmp`。均为线性复杂度。

### 14.2 Manacher

```cpp
auto pal = nmanacher(sequence);
pal.odd_radius(center);
pal.even_radius(right_center);
pal.pal(l, r);               // [l,r) 是否回文，空串为 true
```

奇半径包含中心；偶半径中心位于 `right_center-1` 与 `right_center` 之间。
结果类型名为 `npalindrome_index`，兼容别名 `nmanacher_result`。

### 14.3 后缀数组与 LCP

```cpp
auto sa = nsuffix_array(sequence, compare);
auto lcp = nlcp_array(sequence, sa);
```

`lcp[i]` 是 `sa[i-1]` 与 `sa[i]` 的 LCP，`lcp[0]=0`。当前 suffix array 使用倍增排序，
复杂度 `O(n log^2 n)`（每轮比较排序 `O(n log n)`，轮数 `O(log n)`）；LCP 为 `O(n)`。
传给 `nlcp_array` 的 suffix 必须是 `[0,n)` 的排列，范围与重复项都会验证。

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

### 14.5 `nac<Alphabet>`

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

---

## 15. 几何与优化

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

### 15.3 直线交点

```cpp
auto point = nline_intersection(a,b,c,d);

nline2<long double> x{origin, direction};
auto same = nline_intersect(x, y, epsilon);
```

`nline_intersection(a,b,c,d)` 以两组点表示无限直线，平行或重合返回空；该旧入口当前
使用 `denominator == 0`。`nline2<T>{p,v}` 明确使用点加方向向量，`nline_intersect`
接受 epsilon。两者成功时都返回 `npoint<long double>`，浮点输入的误差策略仍由调用者负责。

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

---

## 16. 竞赛 I/O

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

---

## 17. 典型装配配方

### 17.1 矩阵主对角线排序

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

### 17.2 排序 deque 或 lambda 映射位置

```cpp
ndeque<int> q{4,1,3};
nsort(q); // 非连续原地 heapsort

auto selected = nview(k, [&](int i) -> int& { return storage[index[i]]; });
nsort(selected);
```

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

```cpp
nqueue_agg<int, nmin<int>> q;
for (int i = 0; i < n; ++i) {
    q.push(a[i]);
    if (q.len() > width) q.pop();
    if (i + 1 >= width) answer.push(q.fold());
}
```

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

---

## 18. 调试、测试与提交工作流

### 18.1 诊断顺序

```text
题意/数学模型
→ API 前提
→ 区间 [l,r)
→ view 生命周期和引用类别
→ 代数定律/作用组合顺序
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

只为真实数学差异泛型化：值类型、比较器、代数运算、作用、存储后端、静态容量、
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

### 19.5 代数结构必须写出定律

新增操作包必须在注释中写明单位元、结合/交换/逆元和溢出边界，且与运算一致。新增
lazy action 必须测试非交换 tag 组合；新增 segment 聚合必须用字符串拼接等非交换对象
测试顺序。

### 19.6 每个新算法的证据包

至少包含：

1. 公共契约与失败边界；
2. 正确性不变量或证明；
3. 复杂度；
4. 固定例子；
5. 随机暴力/性质测试；
6. checked 与 unsafe 有效输入一致性；
7. sanitizer；
8. 文档和公共符号索引同步。

---

## 20. 旧版到 Nitori X 的迁移桥梁

Nitori X 不以“删掉旧能力换一个小而美的壳”为目标。它复用旧版竞赛经验，但把每个
能力重新放进统一的所有权、view、枚举、代数和后端层级。迁移原则是：有真实替代就
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
| `npool` | `npool` | 1-based 可删除复用 handle；与 `narena` 分离 |
| `nsparse` | `nsparse` | 升级为任意有序幺半群的 disjoint sparse table |
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

### 20.2 六个有意的语义拆分

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
nkeyed_indexed ndiscrete_function ndiscrete nstable_function_result
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
npartition npart npart_dense nperm
nbranch nnode nwalk nempty_augment nempty_tag nfirst_prefix nlast_suffix
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
