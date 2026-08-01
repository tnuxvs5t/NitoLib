# Nitori v2 权威文档

> 适用版本：`nversion == 20000`  
> 权威 checked 头文件：`/home/tnuzy/NitoriSTL/v2/Nitori.h`  
> 权威 unsafe 头文件：`/home/tnuzy/NitoriSTL/v2_unsafe/Nitori.h`

本文是 Nitori v2 唯一的用户文档原稿。不得在 skill、比赛目录、assets 或其他
reference 中复制本文或头文件。公共签名与真实行为以 checked 头文件为最终事实；
数学前提、使用语义、复杂度和工作流以本文为最终说明。若二者不一致，停止扩散，
以实现和测试重建结论并立即修正文档。

Nitori v2 是面向算法竞赛的 GNU C++20 单头文件系统。它不是 STL 兼容层，也不是
企业工程库。它使用统一的有限索引、借用引用、枚举游标、代数操作包和半开区间，
让同一算法自然作用于连续数组、deque、stride、投影、lambda 引用、矩阵列和对角线。

---

## 目录

1. [快速开始](#1-快速开始)
2. [不可违反的全局约定](#2-不可违反的全局约定)
3. [checked 与 unsafe](#3-checked-与-unsafe)
4. [基础值、哨兵和可选结果](#4-基础值哨兵和可选结果)
5. [代数操作包与定律](#5-代数操作包与定律)
6. [引用拓扑：nspan 与 nview](#6-引用拓扑nspan-与-nview)
7. [枚举协议与组合视图](#7-枚举协议与组合视图)
8. [拥有型序列与通用算法](#8-拥有型序列与通用算法)
9. [机制、scratch、arena 与有限对象](#9-机制scratcharena-与有限对象)
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
20. [完整公共符号索引](#20-完整公共符号索引)

---

## 1. 快速开始

### 1.1 训练、开发和调试

```cpp
#include "/home/tnuzy/NitoriSTL/v2/Nitori.h"

int main() {
    nvector<int> a{5, 1, 4, 1};
    nsort(a);
    nprintln(a[0], a[1], a[2], a[3]);
}
```

```bash
g++ -std=gnu++20 -O2 -Wall -Wextra solution.cpp
```

### 1.2 已验证后的比赛提交

把 include 改为：

```cpp
#include "/home/tnuzy/NitoriSTL/v2_unsafe/Nitori.h"
```

unsafe 不是“更宽松”的版本。它要求输入、下标、定律和所有前置条件已经成立，
否则行为未定义并可能被 `-O2` 激进优化。

### 1.3 最小心智模型

```text
容器/对象提供能力
→ view 把位置映射为真实引用
→ 算法只约束它需要的最小能力
→ 数据结构显式要求代数定律
→ checked 诊断错误，unsafe 把同一前提交给优化器
```

Nitori 公共 API 不提供 iterator。遍历使用 `nfor`/`nfori` 或显式游标；随机访问
使用 `len()` 与 `operator[]`；所有区间统一为 `[l,r)`。

---

## 2. 不可违反的全局约定

### 2.1 索引和区间

- 长度、位置、顶点、版本号统一使用 `int`。
- 不存在的位置使用 `npos == -1`。
- 区间统一使用半开区间 `[l,r)`。
- 合法空区间满足 `l == r`。
- 越界不是可恢复查询，而是前置条件错误。

### 2.2 所有权

- `nvector`、`ndeque`、`narray`、`nmatrix` 等拥有存储。
- `nspan`、`nview`、`nall`、`nsub`、`nstride`、`nproject`、`nzip`、
  `nproduct`、`nwindows` 和 `ngraph_view` 借用 owner。
- 组合器对左值 owner 只保存引用；对 `nspan/nview/组合 view` 则复制或移动 view 包装，
  让中间 view 可以安全嵌套，但仍不延长最底层 owner 的生命周期。
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
| 文件 | `v2/Nitori.h` | `v2_unsafe/Nitori.h` |
| `nunsafe` | `false` | `true` |
| `npre(false)` | 打印表达式、文件、行号后 `abort()` | `__builtin_unreachable()` |
| 合法输入语义 | 与 unsafe 相同 | 与 checked 相同 |
| 用途 | 训练、开发、调试、性质测试 | 已验证的竞赛提交 |

两个头文件由 `v2_src/manifest.txt` 的同一语义源生成。任何手改生成头都会被
freshness 审计拒绝。

```bash
cd /home/tnuzy/NitoriSTL
python3 tools/amalgamate_v2.py --check
python3 tools/audit_v2.py
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
```

比较器：`nless<>`、`ngreater<>`、`nequal<>`。

---

## 5. 代数操作包与定律

### 5.1 定律位

```cpp
enum class nlaw {
    none, associative, identity, inverse, commutative, idempotent
};
```

使用 `operator|` 组合，使用 `nhas_law(laws, law)` 查询。

概念：

| Concept | 必要接口与声明 |
|---|---|
| `ndeclares<O,Law>` | `O::laws` 包含 `Law` |
| `nsemigroup<O,T>` | 结合律；`op(T,const T&) -> T` |
| `nmonoid<O,T>` | 半群 + `op.id() -> T` |
| `ngroup<O,T>` | 幺半群 + `op.inv(T) -> T` |
| `ncommutative_monoid<O,T>` | 幺半群 + 交换律声明 |
| `nsemiring<Add,Mul,T>` | 两个接口合格的幺半群 + 显式 `nsemiring_laws` 声明 |
| `nexact_field_element<T>` | `nexact_field<T>` 显式为真，且具备精确域运算接口 |
| `naction<A,S,F>` | action 接口 + 显式 `naction_laws<A,S,F>` 声明 |

`nexact_field<T>` 默认是 `false`。素数模 `nmodint<M>` 自动声明为真；合数模和普通
整数不会冒充域。自定义有理数等精确域类型可显式特化：

```cpp
template<> inline constexpr bool nexact_field<rational> = true;
```

这仍是调用者的数学声明：concept 能检查运算接口，不能替你证明域公理。

`nsemiring_laws<Add,Mul,T>` 显式声明加法交换、乘法结合、分配律与加法零吸收。
内建非 bool 整数 `nadd/nmul`（在调用者保证不发生有符号溢出的域内）和所有
`nmodint<M>` 默认声明；浮点与自定义操作默认不声明。自定义 min-plus 等半环应特化：

```cpp
template<> inline constexpr bool nsemiring_laws<my_add, my_multiply, state> = true;
```

### 5.2 内建操作

| 操作包 | 单位元 | 声明的定律 |
|---|---|---|
| `nadd<T>` | `T{}` | 结合、单位；当 `nadd_group<T>` 为真时再声明逆与交换（`bool` 排除） |
| `nmul<T>` | `T{1}` | 结合、单位 |
| `nxor<T>` | `T{}` | 结合、单位、逆、交换 |
| `nmin<T>` | 类型的真实上界；浮点为 `+infinity` | 结合、单位、交换、幂等 |
| `nmax<T>` | 类型的真实下界；浮点为 `-infinity` | 结合、单位、交换、幂等 |

`ninf/nninf` 是为安全加减保留余量的算法哨兵，不能充当 `nmin/nmax` 的数学单位元；
两者故意是不同概念。

`naddsum_action<T>` 实现“区间加、区间和”：

```cpp
tag_id()                       // 0
compose(newer, older)          // older 后执行 newer
apply(sum, delta, length)      // sum + delta*length
```

内建 action 定律只为非 `bool` 整数和 `nmodint<M>` 声明；有符号整数仍要求调用者
保证运算不溢出。其他标量即使接口可编译，也必须由调用者显式声明
`naction_laws`，不能从运算符长相推断代数定律。

### 5.3 自定义非交换幺半群

```cpp
struct nconcat {
    static constexpr nlaw laws = nlaw::associative | nlaw::identity;
    string id() const { return {}; }
    string operator()(string a, const string& b) const { return a += b; }
};

nseg<string, nconcat> seg(nvector<string>{"a", "bc", "d"});
assert(seg.fold(0, 3) == "abcd");
```

`nseg` 和 `nfold` 保持顺序，不假设交换。谎报定律会使算法错误；concept 不会替你证明。

### 5.4 自定义 lazy action

作用协议固定为：

```cpp
struct action {
    F tag_id() const;
    F compose(const F& newer, const F& older) const;
    S apply(S aggregate, const F& tag, int length) const;
};

template<> inline constexpr bool naction_laws<action, S, F> = true;
```

`compose(newer, older)` 表示原有 `older` 后再追加 `newer`。顺序错误是 lazy segment
tree 最常见的隐蔽 WA。`naction_laws` 声明 tag 单位、compose 结合与单位，以及
`apply` 对聚合和区间拼接的兼容性；concept 只能检查声明和接口，不能证明这些等式。

---

## 6. 引用拓扑：nspan 与 nview

### 6.1 `nspan<T>`：连续借用

```cpp
int raw[] = {3, 1, 2};
nspan<int> s(raw);         // 数组构造
nspan<int> t(raw, 3);      // 指针 + 长度

s.len(); s.empty(); s.data();
s[i];
s.get(i);                 // 越界返回 nullptr
s.sub(l, r);              // [l,r)
s.sub(l);                 // [l,len)
```

构造要求长度非负，非空时指针非空。`nspan<T>` 可转换为兼容的 `nspan<const T>`。

### 6.2 `nview<F>`：访问器借用

`nview` 保存长度和访问器 `F`。访问器被调用为 `access(i)`，普通可写 view 应返回
真实 `T&`，而不是代理值。

```cpp
nvector<int> a{5, 4, 3, 2, 1};
auto odd_positions = nview(3, [&](int i) -> int& { return a[2 * i]; });
nsort(odd_positions);
// a == {1,4,3,2,5}
```

### 6.3 能力概念

| Concept/trait | 含义 |
|---|---|
| `nindexed<A>` | `nlen(a)` 与 const/non-const `a[i]` 存在 |
| `nindex_reference_t<A>` | `a[i]` 的引用类型 |
| `nindex_value_t<A>` | 去 cv/ref 后的元素类型 |
| `nreference_indexed<A>` | 索引返回 lvalue reference |
| `nswappable_indexed<A>` | 元素是可写且可交换的 lvalue |
| `ncontiguous_indexed<A>` | 另外提供 contiguous `data()` |
| `nresizable<A>` | 提供 `resize(int)` |
| `nview_object<A>` | 类型显式携带安全可传递的 view 包装 |
| `nviewable_indexed<A>` | indexed 左值 owner，或可复制/移动进组合器的 view |

算法只要求实际需要的能力。例如 `nfind` 只需要索引读取，`nsort` 需要可交换引用，
`nunique` 还需要 resize。

### 6.4 标准 view 组合器

```cpp
auto all = nall(a);                    // 全序列引用
auto middle = nsub(a, 2, 7);           // [2,7)
auto every_second = nstride(a, 0, 5, 2);
auto backwards = nreverse(a);
auto keys = nproject(items, [](item& x) -> int& { return x.key; });

auto nested = nreverse(nsub(a, 2, 7));
auto paired = nzip(nsub(a, 0, 3), nreverse(nsub(b, 1, 4)));
auto window = nwindows(nsub(a, 1, 8), 3)[1];
```

`nstride(a, first, count, step)` 的 `count` 是元素数，不是终点；`step` 可为负。
组合结果按值携带中间 view，因此上例不会借用已经销毁的包装对象；它们仍借用 `a/b`
本体，所以 owner 必须继续存活且索引拓扑稳定。

原地算法接受左值 owner，也接受临时 view，因此无需为每一层机械命名：

```cpp
nsort(nreverse(nsub(a, 2, 7)));
nreverse_inplace(nsub(a, 0, 4));
nzeta_subset(nsub(table, offset, offset + (1 << bits)));
```

同样的调用若把临时 owning container 放在最底层会在编译期拒绝。

### 6.5 矩阵列和对角线就是普通 view

```cpp
nmatrix<int> m{{9,2,7}, {6,8,3}, {1,0,5}};
auto column = m.column(1);
nsort(column);

auto diagonal = m.diagonal();
nsort(diagonal);

auto upper = m.diagonal(1);   // offset > 0：主对角线上方
```

这里没有 `sort_diagonal` 特例。`nsort` 只看到可交换索引引用。

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
```

两者都只建立一个具有普通 C++ 语义的循环：`break` 立即结束整个 `nfor` / `nfori`，
`continue` 进入下一次枚举，`sequence` 只求值一次。宏内部不得用额外循环模拟元素绑定。

### 7.3 `nrange`

```cpp
nrange(last)
nrange(first, last)
nrange(first, last, step)

nrep(i, count)   // i = 0, 1, ..., count-1
nrrep(i, count)  // i = count-1, ..., 1, 0
```

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
| `front/back` | 非空首尾元素 |

公开接口故意不是 STL 的 `.size()`/`.push_back()`。

### 8.2 `ndeque<T>`

提供 `len/empty`、索引与 `get`、`pushl/pushr`、`popl/popr`、带 fallback 的
pop、`front/back/clear`。它不连续，但满足普通 `nsort` 所需的可交换索引能力；
Nitori 对非连续路径使用原地 heapsort。

### 8.3 `narray<T,Rank>`

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

### 8.4 通用序列算法

| API | 前提 | 复杂度 |
|---|---|---|
| `nsort(a,cmp)` | viewable + `nswappable_indexed`，cmp 为严格弱序 | `O(n log n)`；连续走 `std::sort`，否则 heapsort |
| `nreverse_inplace(a,l,r)` | 可交换；默认全区间 | `O(r-l)` |
| `nfind(a,x,fallback)` | 可读 indexed | `O(n)` |
| `nlower/nupper(a,x,cmp)` | 已按 cmp 排序 | `O(log n)` |
| `nfold(a,l,r,op)` | op 为幺半群 | `O(r-l)`，保持顺序 |
| `nunique_compact(a,equal)` | 相邻等价元素压缩 | `O(n)`，返回保留长度但不 resize |
| `nunique(a,equal)` | 另需 `resize` | `O(n)`，并缩短容器 |

`nunique` 只压缩相邻等价项；通常先 `nsort(a)`。

---

## 9. 机制、scratch、arena 与有限对象

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

### 9.6 `npartition`

由任意整数 labels 构造，首次出现顺序被压成稠密类编号。

```cpp
npartition p(nvector<int>{8,8,3,5,3});
p.len(); p.classes(); p.empty();
p.classof(i, fallback);
p[i]; p.same(a,b);
auto groups = p.groups();
```

别名：`npart`。

### 9.7 `nperm`

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

---

## 10. 代数数据结构

### 10.1 Fenwick：`nfenwick<T,O>`

要求 `O` 声明交换幺半群；区间查询和单点读取另要求群。

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
p.versions(); p.nodes();
p.reserve_nodes(count);
```

点设与查询 `O(log n)`，每次 set 产生 `O(log n)` 新节点。

### 10.7 Wavelet Matrix：`nwavelet<T>`

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

### 10.8 Mo 调度

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
ngraph_list<long long> g(n);
g.add(u, v, w);       // 有向弧
g.add2(u, v, w);      // 两条反向弧
g.vertices(); g.arcs();
g.neighbors(u);       // nspan<const narc<W>>
```

隐式图：

```cpp
auto grid_graph = ngraph_view(rows * cols, [&](int v) {
    return neighbors_of(v); // 任意可枚举值/引用对象
});
```

算法依赖 `ngraph_like`，不依赖 `ngraph_list`。算法保留 `neighbors` 的返回类别：按引用
返回的邻接对象不会被隐式复制，按值生成的临时邻接对象则存活到本次遍历结束。

### 11.3 遍历与最短路

| API | 前提 | 返回 | 复杂度 |
|---|---|---|---|
| `nbfs(g,s)` | 无权图 | `npos` 表示不可达的距离 | `O(V+E)` |
| `n01bfs(g,s)` | 权仅 0/1 | `npos` 表示不可达 | `O(V+E)` |
| `ndijkstra<D>(g,s,inf)` | 权非负，距离严格小于 `inf` 且和不溢出 | 距离向量 | `O((V+E)logV)` |

`ndijkstra` 的默认 `inf` 使用 `D` 的真实顺序上界（浮点为正无穷），而不是保留余量的
`ninf<D>`。`inf` 本身必须非负且不能为 NaN；若合法最短路可能等于该哨兵，请改用
更宽的距离类型或显式策略。

### 11.4 DAG、SCC 与 LCA

```cpp
auto order = ntoposort(g); // 有环返回空 nmaybe
auto components = nscc(g); // npartition，迭代 Kosaraju
```

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

### 11.6 Prim MST

```cpp
auto mst = nprim<long long>(g, root);
if (mst) {
    auto weight = mst->weight;
    auto edges = mst->edges;
}
```

无连通生成树返回空。无向边必须以对称弧提供；边权转换和总权累加必须能由 `D`
表示，checked profile 会验证。复杂度 `O(E log E)`。

### 11.7 最大流

```cpp
nmaxflow<long long> flow(n);
flow.add(u, v, capacity);
auto value = flow.flow(source, sink);
auto side = flow.mincut(source); // 残量图中 source 可达标记
```

容量类型必须是排除 `bool` 的整数，容量非负，`source != sink`，顶点数不超过
`INT_MAX/2`。总流量与残量加法必须能由容量类型表示，checked profile 会在累加前验证。
对象只允许调用一次 `flow`，之后不能继续 `add`。实现为非递归 push-relabel；
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

---

## 12. 整数、模运算与组合数学

### 12.1 整数基础

`ninteger<T>` 表示排除 `bool` 的整数类型，并包含 GNU++20 下的 128-bit 整数。

| API | 语义 |
|---|---|
| `nmag/nabs` | 无符号绝对值，能表示有符号最低值的幅度 |
| `ngcd` | 非负无符号 gcd |
| `nlcm` | 检查无符号结果溢出 |
| `nfloor_div/nceil_div` | 数学向下/向上整除，除数不可零 |
| `nmod(value,modulus)` | 规范到 `[0,modulus)`，modulus > 0 |
| `nextgcd(a,b)` | 至多 64-bit 输入；`{gcd,x,y}` 满足 `ax+by=gcd`，系数为 `__int128_t` |
| `nmulmod` | uint64 乘法模，内部 `__uint128_t` |
| `npowmod` | uint64 模快速幂 |
| `nisprime` | 对全部 uint64 确定性的 Miller–Rabin bases |
| `nprimes(limit)` | 线性筛返回 `<= limit` 的所有素数 |

### 12.2 静态模整数 `nmodint<M>`

```cpp
using mint = nmodint<998244353>;
mint a = -3;
a.val();
a += b; a -= b; a *= b; a /= b;
auto p = a.pow(exponent);
auto inv = a.inverse();       // gcd(a,M)!=1 时为空
auto raw = mint::raw(value);  // 要求 value < M
mint::mod();
```

除法要求被除数存在乘法逆元；模数不必为质数，但 composite modulus 下并非每个非零值
都可逆。加法群定律已向 `nadd` 声明。

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
a.row(r);          // nspan
a.column(c);       // nview
a.diagonal(offset);// nview，正数向上，负数向下
```

row/column/diagonal 只允许从左值矩阵借用。

### 13.2 矩阵运算

```cpp
auto id = nmatrix_identity<T>(n, add, multiply);
auto c = nmatmul(a, b, add, multiply);
auto p = nmatpow(square, exponent, add, multiply);
```

默认使用 `nadd<T>` 与 `nmul<T>`。乘法要求维度相容；朴素乘法 `O(r*k*c)`，快速幂
为 `O(n^3 log exponent)`。操作包必须满足 `nsemiring`，包括显式的跨操作定律声明；
concept 仍不能替调用者证明声明为真。

### 13.3 RREF、行列式与线性方程

```cpp
nvector<int> pivots;
int rank = nrref(matrix, &pivots); // 原地化为 RREF
T det = ndeterminant(square);      // 按值复制输入
auto solution = nlinear_solve(A, b);
```

`nlinear_solve`：

- 无解：空 `nmaybe`。
- 有解：`particular` 为特解，`basis` 为齐次解空间基。
- 任意解形如 `particular + Σ c_i*basis[i]`。

这些算法要求 `nexact_field_element<T>`：`T{}` 是零、`T{1}` 是一、非零 pivot 可除。
素数模 `nmodint` 自动满足；普通整数、合数模和浮点数默认在编译期拒绝。整数行列式
不能把截断除法塞进高斯消元，应另用 Bareiss 等整环算法；浮点线性代数应另行设计
带 eps 与选主元策略的数值接口。复杂度为三次量级。

### 13.4 卷积

```cpp
auto c = nconv_naive(a, b); // O(nm)
auto c = nconv_ntt(a, b);   // 静态 nmodint，O(k log k)
auto c = nconv_auto(a, b);  // 根据真实 NTT 前提选择
auto c = nconv(a, b);       // 当前默认转发到 auto
```

NTT 前提（素数模由 `nexact_field` 在编译期约束）：

- 系数类型是 `nmodint<M>`。
- `M` 为不超过 `UINT32_MAX` 的质数。
- 变换长度为二次幂且整除 `M-1`。

`nconv_auto` 仅在两边长度至少 32 且上述条件成立时选择 NTT，否则使用朴素卷积。
输入为空时结果为空。

### 13.5 多项式/FPS 工具

系数按低次到高次排列：

```cpp
auto d = npoly_derivative(a);
auto i = npoly_integral(a);       // 常数项为 0，要求 i+1 可逆
auto y = npoly_evaluate(a, x);    // Horner
auto inv = nfps_inverse(a, terms);// a[0] 非零且可逆
```

`npoly_integral` 与 `nfps_inverse` 接受浮点或 `nexact_field_element`，不会让普通整数
静默执行截断除法；合数模即使个别元素可逆，也不冒充全域。
FPS 逆使用 Newton 迭代，复杂度由 `nconv` 后端决定；NTT 可用时约为 `O(M(n)log n)`。

---

## 14. 字符串、Trie 与 AC 自动机

字符串算法接受任意 `nindexed` 符号序列，不要求 `std::string`。

### 14.1 线性字符串算法

```cpp
nprefix_function(sequence);  // prefix[i]：结尾 i 的最长真 border
nz_function(sequence);       // z[0]=n
nkmp_find(text, pattern);    // 返回全部起点；空 pattern 匹配 0..n
```

均为线性复杂度。

### 14.2 Manacher

```cpp
auto pal = nmanacher(sequence);
pal.odd_radius(center);
pal.even_radius(right_center);
pal.pal(l, r);               // [l,r) 是否回文，空串为 true
```

奇半径包含中心；偶半径中心位于 `right_center-1` 与 `right_center` 之间。

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
int next_state = ac.step(state, symbol);
```

构建/扫描线性于节点与文本，另加实际报告匹配数。`match` 按引用转发 callback，
不会复制其内部状态，也支持不可复制 functor。

---

## 15. 几何与优化

### 15.1 点与精确整数谓词

```cpp
npoint<T> p{x,y};
p += q; p -= q; p *= scale;
p + q; p - q; p * scale;

ndot(a,b);
ncross(a,b);
norient(a,b,c);       // (b-a) x (c-a)
ndist2(a,b);
non_segment(p,a,b);   // 含端点
nsegment_intersect(a,b,c,d);
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
```

凸包去重并以逆时针顺序返回；默认移除边上中间共线点。全共线时，默认只返回两端。
凸包 `O(n log n)`，面积 `O(n)`，直径包含建 hull 为 `O(n log n)`。

### 15.3 直线交点

```cpp
auto point = nline_intersection(a,b,c,d);
```

输入表示两条无限直线。平行或重合返回空；否则返回 `npoint<long double>`。
当前用 `denominator == 0`，浮点输入需要调用者理解精度边界。

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

### 15.5 离散单峰搜索

```cpp
auto argmin = nunimodal_arg(first, last, function);
auto argmax = nunimodal_arg(first, last, function, ngreater<>{});
```

搜索整数 `[first,last)`，要求非空且目标相对比较器单峰。返回一个最优位置，约
`O(log range)` 次求值，尾部至多四点暴力。每轮固定先求左探针、再求右探针，
不依赖函数实参的未指定求值顺序。

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

### 17.3 滑动窗口最小值

```cpp
nqueue_agg<int, nmin<int>> q;
for (int i = 0; i < n; ++i) {
    q.push(a[i]);
    if (q.len() > width) q.pop();
    if (i + 1 >= width) answer.push(q.fold());
}
```

### 17.4 隐式网格 BFS

邻接 accessor 可以返回按需生成的 indexed/view 对象；算法不要求先构造所有边。

```cpp
auto graph = ngraph_view(rows * cols, [&](int v) {
    int r = v / cols, c = v % cols;
    // 返回当前点的合法邻居序列；其生命周期必须覆盖这次 neighbors 使用。
    return build_neighbors(r, c);
});
auto distance = nbfs(graph, source);
```

### 17.5 随机对拍台架

```text
小规模生成器
→ 直接暴力
→ Nitori 高效实现
→ 比较全部输出
→ 保存首个失败样例
→ 缩小并手算
```

v2 自身的 property tests 就采用这一模式；复杂结构不能靠样例一次通过证明正确。

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
- 在 owner 扩容后继续使用旧 view。
- projection 返回值而不是 `T&`，随后期待 `nsort` 修改 owner。
- Fenwick 区间 fold 使用了没有 inverse 的操作。
- 对非交换 operation 改变左右合并顺序。
- lazy `compose` 把 newer/older 写反。
- Dijkstra 输入负边。
- reroot 输入不是双向树。
- 模合数下直接除以不可逆元素。
- NTT 长度不整除 `mod-1`。
- unsafe 中依赖 checked 的错误恢复；unsafe 没有恢复语义。

### 18.2 v2 测试命令

```bash
cd /home/tnuzy/NitoriSTL

# 确认生成头没有漂移
python3 tools/amalgamate_v2.py --check

# 最窄双 profile 测试
python3 tools/test_v2.py seq poly matching

# 全量 checked + unsafe
python3 tools/test_v2.py

# 单 profile sanitizer
python3 tools/test_v2.py --profile checked --sanitize
python3 tools/test_v2.py --profile unsafe --sanitize

# 完整发布门禁
python3 tools/audit_v2.py

# 确定性微基准（数值依机器而变）
python3 tools/bench_v2.py
```

测试编译到 Linux `memfd`，不会在仓库留下二进制。

### 18.3 生成规则

正式语义模块列于：

```text
/home/tnuzy/NitoriSTL/v2_src/manifest.txt
```

修改模块后：

```bash
python3 tools/amalgamate_v2.py
python3 tools/amalgamate_v2.py --check
python3 tools/test_v2.py <相关测试>
python3 tools/audit_v2.py
```

禁止直接编辑 `v2/Nitori.h` 或 `v2_unsafe/Nitori.h`。

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

### 19.3 保护 view 生命周期

- 借用 API 接受左值 owner。
- 不在 view 中拥有元素副本。
- accessor 返回真实引用时明确写 `-> T&`/`-> decltype(auto)`。
- 对临时 owner 的危险入口显式删除。
- 测试 owner 修改和非连续访问。

### 19.4 代数结构必须写出定律

新增操作包写 `static constexpr nlaw laws`，单位元和逆元必须与运算一致。新增 lazy
action 必须测试非交换 tag 组合；新增 segment 聚合必须用字符串拼接等非交换对象测试顺序。

### 19.5 每个新算法的证据包

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

## 20. 完整公共符号索引

本索引用于搜索，不代替各章节的前提说明。

### 配置与基础

```text
npre nassert nversion nunsafe
npos nwide_t ninf nninf nmaybe
nchmin nchmax nlen nbitceil nless ngreater nequal
```

### 代数

```text
nlaw nhas_law ndeclares nsemigroup nmonoid ngroup ncommutative_monoid nsemiring_laws nsemiring
nexact_field nexact_field_element naction_laws naction nadd_group nadd nmul nxor nmin nmax naddsum_action
```

### 引用、枚举与序列

```text
nspan nview nindexed nindex_reference_t nindex_value_t
nreference_indexed nswappable_indexed ncontiguous_indexed nresizable nview_object nviewable_indexed
nall nsub nstride
nrange_t nrange nrep nrrep nenumerator_t nenumerable nenumerate nfor nfori nreverse nproject
nzip_view nzip nproduct_view nproduct nwindow_view nwindows
nvector ndeque narray
nsort nreverse_inplace nfind nlower nupper nfold nunique_compact nunique
```

### 机制、内存与有限对象

```text
nscan nsuffix_scan nfirst_true nlast_true nrollback
nscratch narena
npartition npart nperm
```

### 数据结构与离线

```text
nfenwick nseg nseg_iter nlazyseg nlazy_addsum
nqueue_agg ndsu nrollback_dsu ndsu_rollback
npersistent_seg nwavelet
ninterval_query nmo_order nrun_mo
```

### 图与树

```text
narc nedge_to nedge_weight ngraph_view ngraph_like ngraph_list
nbfs ndijkstra ntoposort nscc nlca nreroot n01bfs
nmst_result nprim nmaxflow
nbipartite_matching nhopcroft_karp
```

### 整数、组合、线性代数与多项式

```text
ninteger nmag nabs ngcd nlcm nfloor_div nceil_div nmod
nextgcd_result nextgcd nmulmod npowmod nisprime nprimes
nmodint ncomb
nsubmask_range nsubmasks
nzeta_subset nmobius_subset nzeta_superset nmobius_superset
nfwht_xor nconv_or nconv_and nconv_xor
nmatrix nmatrix_like nmatrix_identity nmatmul nmatpow
nrref ndeterminant nlinear_solution nlinear_solve
nconv_naive nconv_ntt nconv_auto nconv
npoly_derivative npoly_integral npoly_evaluate nfps_inverse
```

### 字符串、几何、优化与 I/O

```text
nprefix_function nz_function nkmp_find npalindrome_index nmanacher
nsuffix_array nlcp_array ntrie nac
npoint ndot ncross norient ndist2 non_segment nsegment_intersect
nconvex_hull npolygon_area2 nconvex_diameter2 nline_intersection
nline_function nlichao nunimodal_arg
ninput noutput nin nout nread nprint nprintln
```

---

## 权威维护声明

Nitori v2 的公共事实只允许存在于：

1. `/home/tnuzy/NitoriSTL/v2/Nitori.h`：checked 公共实现；
2. `/home/tnuzy/NitoriSTL/NITORI_DOCUMENT.md`：本文。

全局 Codex skill 只能引用这两个路径并给出使用流程，不得保存 header、API 表、recipes
或 diagnosis 的副本。比赛目录中的拷贝只是临时提交材料，不是权威源。任何缓存、旧 skill、
旧参考文档或资产快照都不得覆盖这两件原稿。
