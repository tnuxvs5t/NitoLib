# Nitori v3.2 Tutorial Comprehensive

> 状态：从零重建中。本文只描述 `src-v3/` 当前已经存在并经过独立测试的能力。
> V2/X 及更早代码已离开活动工作树；V3 暂时没有统一头文件。
> 日常算法直接接受 STL 容器和 span；公开名字保持全局 `n*`，不要求命名空间。
> `nview/nfunc` 用于确有语义映射需求的组合，规则多维布局单独使用 `nmdview`。

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

当前源码预算为 **167 KiB（128 + 5 + 24 + 4 + 6 KiB）语义代码**；新增 5 KiB 归入通用额度，
24 KiB 专用于数学与多项式，4 KiB 专用于图论，另有 6 KiB 供图论与数据结构共享。
注释和排版空白不计入预算，因此应当用注释解释
承重契约，而不是用复杂类型系统把用户锁进唯一组合方式。

## 1. 使用方式与头文件关系

V3 使用 C++23，直接包含需要的模块：

```cpp
#include "src-v3/segment.hpp"

vector<long long> a{2, 3, 5};
nseg<long long> seg(a);
```

位置宽度由一个全程序开关决定。默认模式保持竞赛常用的 32 位位置；如果有限对象、节点池
或稠密编号可能超过 `INT_MAX`，在**每个**包含 V3 头文件的翻译单元中提前定义
`NITORI_INDEX_64`，或统一加入编译参数：

```bash
g++ -std=c++23 -DNITORI_INDEX_64 solution.cpp
```

此时 `nidx_t` 为 `long long`；否则为 `int`。`nuidx_t` 是对应无符号类型。两种模式不能在
同一程序的不同翻译单元中混用，否则公共模板和布局会违反 ODR。V3 不保存平行的
`int`/`long long` 实现，所有长度、位置、稠密顶点编号、节点 handle 与根都沿同一个
`nidx_t` 类型传播。

模块可以正常依赖其他模块，用户不需要手工补齐传递依赖。当前依赖骨架是：

```text
core
├── io / sequence / mdview / string / automata / wavelet / geom
├── ds ── flow（同时依赖 sequence）/ permutation
├── graph ── graph_algo
│         ├─ graph_store（同时依赖 view）
│         └─ rooted（同时依赖 func）── tree
├── view ── hash ── func ── discrete（同时依赖 sequence）── vec_bag
│                       └─ debug（同时依赖 io）
├── arena ── segment ── link_cut
│         ├─ fhq（同时依赖 view）── bag / dynamic_tree
│         └─ opt / reftree
├── math ── poly / linear / recurrence
└── list / number / divisor / bitmath / frac

dynamic_tree 同时依赖 segment。
```

不存在 `Nitori-v3.h`，也不要求形成它。这样每个模块可以先独立成熟，避免为了 amalgamate
顺序产生虚假的底层抽象。

## 2. 全局约定

- 长度、位置、稠密顶点编号、节点 handle 与根使用 `nidx_t`。
- `nidx_t` 默认是 `int`；全程序定义 `NITORI_INDEX_64` 后是 `long long`。
- 32 位模式下，`ntabulate/nrange/nsub/nslice/nstride` 拒绝更宽的整数参数，避免把本应开启
  64 位模式的范围静默截断；其他由容器或 descriptor 导出的长度仍由调用者保证可表示。
- 区间统一为半开区间 `[left,right)`。
- `len()` 是 V3 有限对象的长度接口；`nlen(x)` 也能读取普通容器的 `size()`。
- `nchmin(target,candidate)` / `nchmax(target,candidate)` 在严格序更优时原地赋值，并返回是否发生更新。
- V3 默认不做边界检查、溢出检查和契约恢复。
- 直接 HLD 构造使用迭代遍历，调用深度为常数。`nroot/nreroot`、流与部分动态结构仍有递归；按各模块契约评估栈深度。
- view、func、graph descriptor 都可能只是借用；owner 移动、销毁或结构修改后，旧投影可能失效。
- `nall(lvalue)` 保留原对象可用的 inverse，仍然只借用；descriptor 可能持有位置数组或 hash，复制成本不一定是常数。
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

### 2.1 `io.hpp`：与 iostream 共用缓冲区的泛整数 I/O

`io.hpp` 不替换 `cin/cout`，也不安装新的全局 `streambuf`。它直接使用调用者现有
`istream/ostream` 的缓冲区和状态，因此普通 `>>/<<` 与 Nitori 整数 I/O 可以按调用顺序
任意交叉：

```cpp
#include "src-v3/io.hpp"

ios::sync_with_stdio(false);
cin.tie(nullptr);

int n;
__int128_t limit;
cin >> n;
nread(limit);                 // 默认读取 cin

long long a, b;
nscan(a, b);                  // 空格分隔的多个十进制整数
cout << "answer ";
nprintln(limit, a + b);       // 默认写入 cout，值之间一个空格并换行
```

显式流版本适用于文件和字符串台架：

```cpp
stringstream data("12 -34");
int x, y;
nscan(data, x, y);
nprintln(cerr, x, y);
```

```text
nread([in,] value)            读一个整数；成功返回 true
nscan([in,] values...)        共用一次 stream sentry，全部成功才返回 true
nwrite([out,] value)          写一个十进制整数
nprint([out,] values...)      单空格分隔，不换行
nprintln([out,] values...)    单空格分隔并换行
```

输入支持可选 `+/-`，逐位检查目标类型的上下界，因此覆盖普通整数以及
`__int128_t/__uint128_t`，不会借道 `long long`。越界、无数字或流错误设置 `failbit`，并保持
目标值不变；完整 token 恰好在 EOF 结束时读取成功并设置 `eofbit`。输出也直接处理最小有
符号值，不先取可能溢出的绝对值。

这是十进制整数端口，不服从流的 `hex/oct/showbase/width/locale` 数值格式状态；字符串、
浮点、容器和格式 DSL 继续使用标准流或题内代码。若使用默认 `cin/cout` 入口，通常仍应在
`main` 开头关闭同步并解除 `cin` 的 tie。

### 2.2 `debug.hpp`：递归、即时的 Python 风格观察

`debug.hpp` 默认把值写入 `cerr`，参数之间一个空格，结尾换行并立即 flush；也可以把
`ostream` 作为第一个参数，便于字符串台架或文件记录：

```cpp
#include "src-v3/debug.hpp"

vector<pair<int, tuple<string, vector<int>>>> a{
    {1, {"north", {2, 3}}},
    {4, {"south", {5}}}
};
ndebug("a =", a);

ostringstream out;
ndebug(out, "state =", a);
```

```text
a = [(1, ("north", [2, 3])), (4, ("south", [5]))]
```

顶层字符串像 Python `print` 一样原样输出，因此可直接充当标签；嵌套字符串与 `char` 带
引号并转义。整数（包括 128 位）、浮点、布尔、空指针以及已有 `ostream << value` 的
叶子类型均可输出。`vector/deque` 使用 `[a, b]`，`pair/tuple` 使用 `(a, b)`；单元素 tuple 保留
尾逗号。它们可以任意嵌套。

用户类型可以在类型所在的命名空间提供 ADL 调试接口，不需要加入全局注册表：

```cpp
namespace app {

struct point {
    int x, y;
};

void ndebug_repr(ndebug_writer& out, const point& value) {
    out.object("point", [&](auto field) {
        field("x", value.x);
        field("y", value.y);
    });
}

}

app::point p{2, 3};
ndebug("p =", p);
// p = point{x=2, y=3}
```

`ndebug_writer::value(x)` 重新进入递归渲染，因此字段可以继续使用 `vector`、`tuple`、
`nview`、`nfunc` 或其他自定义类型的格式。`object(type, fields)` 负责输出
`type{field=value, ...}` 的标点；`raw(text)` 原样写出，只应用于固定标点和格式标签。
自定义接口优先于 `operator<<`，没有自定义接口时原有的 `operator<<` 叶子行为保持不变。
自定义函数应接受 `const T&`，不得依赖 writer 在回调结束后继续存在。

`nview` 显示为 `nview[...]`，强调它是惰性位置投影；`nfunc` 显示为按 position 排列的
`nfunc[(key, value), ...]`。后者故意不伪装成字典，因为 domain 可以有重复 key，而且顺序
属于语义的一部分：

```cpp
auto square = nfunc{nrange(3), [](nidx_t key) { return key * key; }};
ndebug("square =", square);
// square = nfunc[(0, 0), (1, 1), (2, 4)]
```

观察惰性对象会真实求值：`nview` 从左到右访问每个 position 一次；`nfunc` 对每个 position
求 key 一次，再用同一个 key 调 evaluator 一次。有状态 accessor/evaluator 因此可能推进
状态。接口不复制 descriptor，也不延长 `nall(owner)` 的借用寿命。完整输出的时间为
`O(输出节点数 + 字符数)`，不做隐藏截断；大对象应先用 `nsub/nslice` 缩小观察范围。

模块不读取 `ONLINE_JUDGE` 或 `NDEBUG`，也不定义全局调试宏。若提交时需要彻底移除求值，
由调用处显式使用 `#ifdef LOCAL` 包围 `ndebug`。

### 2.3 STL 输入与显式位置计划

`ds/segment/string/automata/wavelet/geom/linear/poly/flow` 的序列输入直接接受
`vector`、`array`、`span`、`string` 或提供长度与下标的 descriptor。算法借用输入，
拥有数据的结构在构造时读取元素；都不会先复制整个输入容器。需要投影、重排或语义 key
时再引入 `nall/nview/nfunc`。这些名字均在全局作用域。

`sequence.hpp` 只依赖 core，提供：

```cpp
#include "src-v3/sequence.hpp"
vector<int> a{3, 1, 1, 2};
auto order = nargsort(a);                         // 拥有位置数组；不搬动 a
auto sorted = ngather(span{a}, span{order});        // 借用 a 和 order
auto bounds = nrun_bounds(sorted);                // 相邻相等的 [left,right) 快照
```

`nargsort` 为 `O(n log n)`；支持 compare/projection，相等元素的次序不保证稳定。
`ngather` 返回 `nindexed_span<T>`，构造和复制不分配，下标访问 `O(1)`；重复位置别名。
两个 span 的 owner 必须存活且存储地址稳定。`nrun_bounds` 为 `O(n)`，结果只保存边界，
不保存源或共享 owner；之后修改源不会更新这份快照。复杂的惰性 chunks 仍在 discrete 中。

重复名字已合并：`nrestrict` 改用 `nredomain`，`nselect_positions` 改用 discrete 中的
`nselect`，Fenwick 的 `lower` 与有序多重集的 `order_of_key` 改用 `lower_bound`，
`nmake_fhq<T>(ops)` 改为 `nfhq<T,Ops>(ops)`。需要 `nroot` 时显式包含 `rooted.hpp`。

### 2.4 多维布局与 bias 的组合

`mdview.hpp` 的 `nmdview<T,D>` 借用一段连续存储，以 shape、bias、stride 分别表示
每轴长度、语义坐标起点和物理步长。三者分开后，换轴和重标坐标无需搬动元素。

```cpp
#include "src-v3/mdview.hpp"
array<int, 6> a{0, 1, 2, 3, 4, 5};
nmdview grid(span{a}, array<nidx_t, 2>{2, 3}, array<long long, 2>{-2, 10});
auto part = grid.sub({-1, 11}, {0, 13})
                .permute({1, 0}).reverse(0).rebase({100, -7});
part(101, -7) = 41;                       // 写入 a[4]
auto coordinate = part.coordinate(0);    // {100,-7}
auto position = part.position(coordinate); // 0，逻辑行优先位置
```

- `sub(lower,upper)` 按每轴语义半开区间切片，保留坐标标签。
- `permute(axes)` 的 `axes[new_axis] = old_axis`；参数必须是维度排列。
- `reverse(axis)` 反转该轴元素次序，保留坐标标签。
- `rebase(origin)` 只改标签，保留形状与元素次序。
- `operator()(key)` 或 `operator()(i,j,...)` 按坐标访问；`operator[](p)` 按当前
  shape 的逻辑行优先顺序访问，换轴后不再保证物理连续。
- `position(key)` 和 `coordinate(p)` 在有效逻辑域内互逆；它们不等于物理地址偏移。

坐标始终为 `long long`，因此默认 32 位位置模式也允许 `10^18` bias。长度乘积必须能由
`nidx_t` 表示，步长及偏移运算必须能由 `ptrdiff_t` 表示，`bias+shape` 必须能由
`long long` 表示。初始 span 足以容纳 shape 的元素数；坐标和子范围必须合法。
空域允许继续变换，不允许访问或反求坐标。访问和变换均为 `O(D)`，不分配、不建 hash。

派生视图共享底层元素；owner 的销毁或存储迁移使它们全部失效。constness 是浅的：
只读元素使用 `span<const T>` 构造。不要把变换后的视图误当连续 span。
规则多维数据用此布局；离散、不规则的 key 仍使用原有 `nproduct/nfunc` 组合：

```cpp
// 另外包含 func.hpp。
auto keys = nproduct(nrange(-2, 0), nrange(10, 13));
auto f = nfunc{keys, [grid](auto key) -> int& {
    return grid(get<0>(key), get<1>(key));
}};
auto values = nmap_values(f, [](int& x) -> int& { return x; });
```

`nproduct` 负责枚举 key，`nmdview` 负责元素地址，`nmap_values` 负责 value 变换。

## 3. `nview`：有限位置投影与可选 inverse

头文件：`src-v3/view.hpp`

```cpp
template<class Access>
struct nview {
    nidx_t length;
    Access access;
    nidx_t len() const;
    decltype(auto) operator[](nidx_t position) const;
    // 仅当 accessor 提供该表达式时存在
    nidx_t inverse(Key&& key) const;
};
```

`nview` 拥有 accessor，但通常不拥有 accessor 引用的数据。const 是浅 const：如果 accessor
返回 `T&`，那么 const view 仍然能修改底层对象。普通 view 只需要 `access(position)`；若
accessor 还提供 `inverse(key)`，则该投影声明自己单射，并满足：

```text
view.inverse(view[position]) == position
```

inverse 只对 view 的像内 key 有定义，不做 membership 或边界恢复。key 的值、顺序或 owner
结构变化后，依赖旧 key 的 inverse 可能失效。

### 3.1 创建和切片

```cpp
vector<int> a{10, 20, 30, 40};
auto all = nall(a);             // 只接受 lvalue，借用 a
auto ids = nrange(2, 6);        // 2,3,4,5
auto sq = ntabulate(5, [](nidx_t i) { return i * i; });
auto mid = nsub(all, 1, 3);     // a[1],a[2]
auto rev = nreverse(all);
```

`nall(vector<int>{...})` 故意不能编译，因为临时 owner 会立刻销毁。`nsub` 不复制元素，
也不检查区间。`ntabulate` 每次访问都会重新调用函数。

### 3.2 结构 inverse

`nrange` 自带 O(1) inverse；`nsub`、`nreverse` 在 source 可逆时传播 inverse：

```cpp
auto ids = nreverse(nsub(nrange(10, 30), 3, 9));
assert(ids.inverse(ids[2]) == 2);
```

自定义投影可以同时给出两个方向：

```cpp
auto arithmetic = ntabulate(
    6,
    [](nidx_t position) { return 10 + 3 * position; },
    [](int key) { return (key - 10) / 3; }
);
```

`nlocate(view)` 借用一个可逆 lvalue view，返回 `key -> position` callable；它不延长 view
寿命。`ninvert(view)` 保留已有结构 inverse；若 view 不可逆，则一次性建立并拥有静态 hash
inverse。后者位于 `hash.hpp`，构造期望 O(n)、查询期望 O(1)。

### 3.3 projection 与 materialization

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

### 3.4 组合器

```cpp
auto picked = ngather(nall(a), ntabulate(2, [](nidx_t i) { return 1 - i; }));
auto zipped = nzip(nrange(3), ntabulate(3, [](nidx_t i) { return i * i; }));
auto cells = nproduct(nrange(2), nrange(3));
auto cube = nproduct(nrange(2), nrange(3), nrange(4));
```

- `ngather(values,positions)` 按位置重排或重复访问。
- `nzip` 取最短输入长度，tuple 元素保留引用。
- `nproduct` 是左主序笛卡尔积，最后一维变化最快，长度乘积必须能放进 `nidx_t`。
  二维结果保持 `pair`，三维及以上结果是 tuple；tuple 元素保留输入 view 的返回类别。

`nproduct` 在所有轴可逆时同时提供坐标到 flat position 的 inverse，不再要求调用者另外
重复 shape。`ngather` 在 source 与 position plan 都可逆时传播 inverse；`nmap/nproject/nzip`
默认不猜测逆映射。`nfilter/norder` 也不会为了保留 inverse 偷偷再分配 O(n) 反向计划。

`nview` 还提供随机访问 iterator，因此可以 range-for，也能交给真正接受随机访问 iterator
的标准算法。迭代器内部借用 view 本身，不能比 view 活得更久。

## 4. `nfunc`：把位置、key 和 value 分开

头文件：`src-v3/func.hpp`

```cpp
template<class Domain, class Eval>
struct nfunc {
    Domain domain;
    Eval eval;
    nidx_t len() const;
    decltype(auto) key(nidx_t position) const;
    decltype(auto) operator[](nidx_t position) const;
    decltype(auto) operator()(Key&& key) const;
};
```

`domain[position]` 给语义 key，`eval(key)` 给 value：

```cpp
vector<string> names{"alice", "bob"};
vector<int> score{80, 95};

auto f = nanchors(nall(names), nall(score));

assert(f.key(1) == "bob");
assert(f[1] == 95);             // 按 position
assert(f("alice") == 80);      // 按 key
```

`operator()` 支持把多个实参自动打包成结构化 key：两个实参打包成 `pair`，三个及以上实参
打包成 `tuple`。因此可以直接用坐标调用笛卡尔积函数：

```cpp
auto f = nfunc{
    nproduct(nrange(2), nrange(3)),
    [](const pair<nidx_t, nidx_t>& cell) { return 3 * cell.first + cell.second; }
};
assert(f[5] == 5);
assert(f(1, 2) == f(pair{1, 2}));
```

单个实参仍按原样传递；多参数调用与手动构造对应的 `pair`/`tuple` 等价。

稠密下标是常见特例：

```cpp
auto dense = nanchors(nall(score));  // key 就是 0..n-1
```

`nfunc` 本身不要求 domain key 唯一。一般 evaluator 可以在重复 domain 上枚举同一个 key
多次；只有把两组按 position 对齐的 keys/values 绑定起来时，才需要 key 唯一。

### 4.2 `nanchors`：结构 inverse 优先，hash fallback 兜底

两参数 `nanchors` 表示 `keys[position]` 与 `values[position]` 对齐。它专门把按 position
对齐的 value 锚定到 semantic key；直接写 `nfunc{domain, evaluator}` 仍然表示自定义求值，
两者不是同一个入口：

```cpp
auto grid = nproduct(nrange(10, 13), nrange(-2, 2));
auto cells = nanchors(grid, nrange(12));
assert(cells(12, 1) == 11);       // 使用 nproduct 的结构 inverse

vector<string> names{"alice", "bob"};
vector<int> score{80, 95};
auto scores = nanchors(nall(names), nall(score));
assert(scores("bob") == 95);     // nall 无结构 inverse，构造一次 hash fallback
```

选择顺序是：

```text
可复制且具有 inverse 的 keys      -> 复制 descriptor，复用已有定位
其他 keys                           -> 物化静态 hash inverse
```

hash fallback 把 owned key 紧凑存放；每个 slot 保存 32 位 fingerprint 和一个 `nidx_t`
position，负 position 表示空槽。由于对齐，slot 在 32 位模式下为 8 bytes、64 位模式下通常
为 16 bytes。它是一次构建、只查询的静态 inverse，不提供 erase、节点迭代器或增量
rehash。构造期望 O(n)，查询期望 O(1)，额外空间 O(n)。`nhash` 对普通叶子使用
`std::hash`，递归支持 `pair/tuple`，并给每张默认表独立 salt。

自定义 hash/equality 使用四参数 overload，并强制走 hash fallback：

```cpp
auto f = nanchors(nall(keys), nall(values), key_hash, key_equal);
```

若题目已经有坐标压缩数组或其他更紧凑 locator，继续使用三参数 overload：

```cpp
auto f = nanchors(keys, values, locate);
```

所有 `nanchors` 形式都要求两组序列等长、key 唯一且查询 key 存在。hash/equality 必须一致；借用
owner 必须存活，建表后 key 的长度、次序和值保持稳定，payload value 可以修改。

已有 hash inverse 的 descriptor 本身可能拥有整张表。需要复用同一张表时，先保存
`auto keys = ninvert(nall(names));`，再用 `nanchors(nall(keys), nall(score))`；这里
`nall(keys)` 保留 inverse，只复制借用描述，不复制 hash。`keys` 必须比绑定结果活得更久。
lookup 的复杂度继承实际 inverse；自定义 inverse 不必是常数时间。

### 4.3 组合接口

```text
nkeys(f)                       domain
nvalues(f)                     按 domain 枚举 value 的 view
nentries(f)                    (key,value) view
nredomain(f,new_domain)        只更换枚举域
nmap_values(f,transform)       保持 key，变换 value
ncompose(outer,inner)          outer(inner(key))
nselect(f,positions)           按原 position 选择新 domain（discrete.hpp）
```

`nkeys/nvalues/nentries` 对左值 `nfunc` 只借用，对右值 `nfunc` 则移入并由返回的
descriptor 拥有。因此 move-only 或内部有状态的 evaluator 不会因为遍历左值而被暗中
复制。借用结果不得比原 `nfunc` 活得更久。

`nredomain` 不检查新 key 是否能被 evaluator 处理。`nmap_values` 保留 transform
的返回类别；这也是 V3 保留该名字而不把它降级成普通 `nmap` 的原因。

`nproduct` 与 `nmap_values` 是稳定的公共名字，不会因为底层实现缩小而消失。

### 4.4 `discrete.hpp`：用位置计划打通 view 与 func

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
auto picked = nselect(nall(a), vector<nidx_t>{3, 1, 3}); // 20,10,20
auto odd = nfilter(nall(a), [](int x) { return x & 1; });
auto sorted_view = norder(nall(a));                  // 懒重排，a 未变
auto materialized = ncollect(sorted_view);           // vector<int>{10,20,30,40}
```

`vector<nidx_t>` 位置计划会被移入返回的 descriptor，所以 `nfilter/norder` 不会借用已销毁的
局部下标容器。重复位置是合法的；若 source 产生左值，它们会别名到同一元素。
`ncollect` 是明确物化边界，会递归移除 `nzip/nproduct` 所产生 pair/tuple 内部的引用。

`npositions(source,predicate)` 返回可复用的命中位置数组，`nfilter` 复用该计划生成内核。
同一计划可通过 `nselect(nall(a), nall(plan))` 应用于多组对齐数据；其顺序稳定，每个位置
只调用一次 predicate。`ncollect/nprefix/nsuffix/naccumulate/neach`、查找、计数、极值和
二分算法只在调用期间借用 source，不复制它拥有的位置数组或 hash，也接受 move-only 左值。
有状态 accessor 的调用会作用于原 descriptor；这些终结算法不延长输入寿命。

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
npositions(source,predicate)          vector<nidx_t> 筛选计划，可供多个序列复用
nunique(source,together={})           稳定保留每个相邻归并段的首位置
nindexed(source)                      (position,value) 惰性 view
nargsort(source,compare,projection)   vector<nidx_t> 排序计划
norder(source,compare,projection)     应用计划，不移动值
```

值操作与基础序列内核：

```text
nassign(destination,value_at)        destination[i] = value_at(i)
nfill(destination,value)             用稳定 value 填充全部目标位置
ncopy(source,destination)             按 position 左到右复制
ntransform(source,destination,op)     一元位置变换并写入
ntransform(a,b,destination,op)        二元位置变换并写入
nprefix(source,identity={},operation=plus<>)  含 ID 的左扫描
nsuffix(source,identity={},operation=plus<>)  含 ID 的右扫描
nsort(source,compare,projection)      原地排序 source[position] 左值
nreverse_inplace(source)              原地反转值
naccumulate(source,initial,operation) 从左到右折叠
neach(source,action)                  按序调用并返回 action
nfind_if / ncontains / ncount_if      返回位置 / 是否存在匹配值 / 数量
nall_of / nany_of / nnone_of         量词
nargmin / nargmax                     极值位置（空序列为 len）
nlower / nupper                       已排序位置序列的插入位置
```

`nassign` 是写入内核，参数不是含糊的“值或 range”联合体，而始终是
`value_at(position)`：

```cpp
vector<int> a(6), b(6);
nassign(nall(a), [](nidx_t i) { return i * i; });
nfill(nstride(nall(a), 2), -1);            // 只填偶数 position
ncopy(nall(a), nreverse(nall(b)));         // 复制到重排后的目标
ntransform(nall(a), nall(b), [](int x) { return x + 1; });
```

因此目标既可以是普通 `nview`，也可以是 `nfunc` 的 value 投影；domain/key 不会被写入。
`ncopy/ntransform` 要求目标至少与输入等长，二元 transform 的两个输入等长。所有调用严格
从左到右执行，不隐藏分配；若输入与目标重叠，后续读取会观察到前面已经完成的写入。需要
快照语义时先显式 `ncollect`。四者均为 O(写入位置数)，目标必须产生可赋值左值。

V3 不提供统一的“极值元素物化”包装：`nzip` 会返回嵌套引用，`vector<bool>` 会返回代理，
单靠 `remove_cvref` 不能可靠推出调用者需要的拥有类型。需要位置时使用 `nargmin/nargmax`；
需要显式值类型和空源兜底时，先判断返回位置，再写 `T(source[position])`。高频的纯数值归约
可以在同时引入 `segment.hpp` 后直接复用 merge：

```cpp
T minimum = naccumulate(source, nmin<T>{}.id(), nmin<T>{});
T maximum = naccumulate(source, nmax<T>{}.id(), nmax<T>{});
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

`nunique(source,together)` 不移动值，也不做全局集合去重；它按原位置顺序比较每一对相邻
元素，并保留第一个位置以及所有满足 `!together(previous,current)` 的位置。默认
`together` 是 `equal_to<>`，所以常见的 `nunique(norder(source))` 会得到排序后的不同值视图。
返回结果仍别名到各段的首元素；构造时保存的位置计划不会因后续值修改而重算。对 `nfunc`
比较的是枚举到的 value，保留下来的 domain key 与原 evaluator 关系不变。

`ncontains(source,target,compare,projection)` 是线性值查询，按位置检查
`compare(projection(source[i]),target)`，命中后立即停止；默认使用 `equal_to<>` 和
`identity`。它检查 source 枚举到的 value，而不是 `nfunc` 的 semantic key；空序列返回
`false`，时间复杂度为 `O(n)`、额外空间为 `O(1)`。需要已排序序列上的对数查询时使用
`nlower/nupper`，不要把 `ncontains` 当成二分查找。

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
nruns(source)                  相邻相等值成段
nruns(source,operation)        operation(left,right) 判定候选段 [left,right)
```

`nruns` 按顺序贪心生成非空块。Operation 自行捕获原 view，并接收扩展后的候选段
`[left,right)`，不接收源对象或元素。首次及每次切段后先调用单元素段，随后 right
每次加一；拒绝 `[left,right)` 后输出 `[left,right-1)`，立即调用
`operation(right-1,right)` 初始化新段。每个单元素必须可成段，空输入不调用 Operation。

```cpp
// Operation 自己增量维护极值；拒绝后，新的 left 会触发重新初始化。
auto groups = nruns(nall(a), [&, start = nidx_t(-1), low = 0LL, high = 0LL]
                   (nidx_t left, nidx_t right) mutable {
    long long x = a[right - 1];
    if (left != start) start = left, low = high = x;
    else low = min(low, x), high = max(high, x);
    return high - low <= tolerance;
});
```

非空输入调用次数为 `n+块数-1`，没有结束通知。Operation 可以先更新自己的摘要再
判断，拒绝后的摘要在下一次单元素调用时重置；库不需要摘要类型、回滚或 emit 协议。
源结构不能改变；捕获的 view/owner 必须在调用期间有效，不要捕获随后被移空的 descriptor。
边界是快照，结果仍为原来源的 chunks。自定义双参数谓词现在表示位置边界，旧版的
相邻值谓词应改为捕获源并比较 `source[right-2]` 与 `source[right-1]`，单元素返回 true。

规则保证首次拒绝即切段；有负数时 sum 超限后可能恢复，不能据此宣称最长合法前缀。
数值差、和由调用者保证可表示。任意区间统计可以直接调用外部 fold；预先计算或回溯
得出的区间计划仍直接交给 `nchunks`，不强制经过 nruns。

chunk 构造时把 source descriptor 移入一次共享存储，子块保留该存储，因此可以脱离外层
chunk function；每次取子块只复制常数大小的描述，不分配、不复制整个位置计划或 hash。
move-only source 也能分块。子块共享 accessor 的可变状态；底层外部 owner 的借用寿命仍
不会延长。普通容器传 `nall(owner)`，source 为左值时构造分块仍按值复制一次。

例如 `nblocks(norder(nall(a)), 1)` 只保存一份排序计划，取完所有子块的描述成本为 `O(n)`。
原来的子块脱离能力保留，但带内部可变状态的 accessor 从“每个子块独立复制”改为共享。
这类共享分块在运行期构造，不再支持常量求值；`nblock` 的单个普通切片仍可用于常量求值。

以下均另计 descriptor 捕获成本和源访问成本：已有借用计划的 select/slice/stride 构造 `O(1)`；filter/unique 构造 `O(n)`；
runs 构造为 `O(n + 所有谓词调用的总成本)`，空间与块数成正比；
argsort/order/sort 为 `O(n log n)`；blocks/windows 的 interval domain 是惰性
`O(1)` 描述，分块额外分配一次共享源；枚举全部子块与子块数成正比。

### 4.5 `permutation.hpp`：原地旋转与完整排列的 rank/unrank

```cpp
#include "src-v3/permutation.hpp"
vector<nidx_t> a{0, 1, 2, 3, 4};
nrotate(a, 2);                        // {2,3,4,0,1}，原地左旋两位
nrotate(a, 1, 3, 5);                  // 只将 [1,5) 的 [1,3) 移到末尾
auto rank = npermutation_rank(array{1, 0, 2}); // uint64_t(2)
auto p = npermutation_unrank(3, rank);          // {1,0,2}
```

`nrotate(source,middle)` 作用于整个序列，`nrotate(source,left,middle,right)` 只作用于
`[left,right)`；均与 `std::rotate` 的方向一致，原地移动值、不修改 `nfunc` 的 domain 结构。
若 domain 的 key 存储与这些值别名，key 仍可能随之改变并使旧定位表失效。
边界要求 `0<=left<=middle<=right<=nlen(source)`，各位置必须是互不别名的可交换左值。
空段和 `middle==left/right` 合法。三次区间反转给出 `O(right-left)` 次交换、`O(1)` 库内额外空间；
外部 owner 和 descriptor 在调用期间必须有效。

`npermutation_rank(source)` 把恰好含有 `0..n-1` 各一次的**完整排列**映射到零基字典序名次；
`npermutation_unrank(n,rank)` 反向返回 `vector<nidx_t>`，要求 `n>=0` 且
`0<=rank<n!`。空排列的名次为 0。默认名次类型为 `uint64_t`，若结果可能超出其范围，
调用 `npermutation_rank<更宽整数类型>(source)`；名次类型须支持精确非负乘加、除法与取余，
且名次和中间值可表示。反查函数从参数推导名次类型，不要求 `n!` 本身能放入该类型：
例如 `uint64_t` 下仍可反查 21 元排列的名次 0 或 1。

设第 `i` 位选出的数在剩余数中有 `d_i` 个更小者，则按字典序扫描时
`R_{i+1}=R_i*(n-i)+d_i`；反查从末位依次以 `n-i` 为基数取余，得到这些 `d_i`。
Fenwick 维护未选元素的计数，求 `d_i` 和寻找第 `d_i+1` 个未选元素均为 `O(log n)`；
两方向均做 `O(n log n)` Fenwick 工作和 `O(n)` 次名次运算，位置存储 `O(n)`。
若名次运算为常数时间，总时间就是 `O(n log n)`；大整数需另计位复杂度。
输入排列的合法性与整数溢出由调用者保证，
实现不执行排列验证或溢出恢复。头文件依赖 `ds.hpp`，不加重基础的 `sequence.hpp`。

## 5. `narena` 与根代数

头文件：`src-v3/arena.hpp`

`narena<T>` 是 append-only `vector<T>`，`make(...)` 返回 `nidx_t` handle。handle 在 vector
扩容后仍有效，引用和指针不保证有效。它没有 generation、epoch、owner、自动回收或
跨类型身份协议。

这是有意的：需要删除复用、陈旧 handle 检测或事务的结构，可以在这块最小地基之上
单独实现，而不是让所有竞赛结构为这些能力付费。

## 6. `nfhq`：一个 kernel，多棵根

头文件：`src-v3/fhq.hpp`

`nfhq<T,Ops>` 把节点池、随机优先级和一个策略对象放在一个 kernel 中。`-1` 是空根；
同一 kernel 可以同时持有任意多棵互不相交的树。

```cpp
nfhq<int> q;
vector<int> a{1, 2, 3, 4};
nidx_t root = q.build(nall(a));
auto [left, right] = q.split(root, 2);
root = q.merge(right, left);        // 3,4,1,2
nidx_t handle = q.kth(root, 1);
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
edit(root,left,right,fn)     隔离区间，将 fn(q,middle) 返回的根拼回
apply(root,command)          尝试整棵子树更新，失败则下钻
walk(root,visit,element)     自定义剪枝与中序访问
```

承重契约：

- merge 的两棵树必须来自同一个 kernel 且节点集合不相交。
- split/merge 消耗旧的根语义；不要把输入 root 继续当独立树使用。
- `split_by` 的谓词沿中序必须先真后假。
- 可选的 `ops.pull(q,handle)` / `ops.push(q,handle)` 不能跨分配保存节点引用。
- 单次 split/merge/kth/rank 的复杂度是随机优先级下期望 `O(log n)`，假定每次策略调用 `O(1)`。
  当前 build 连续 merge，保守界为期望 `O(n log n)`。apply/walk 另按实际访问量计费。

这解决了 V2 merge/split 卡手的根因：交易对象是同一 kernel 中的普通整数根，不再由
每棵树的 owner/domain 类型阻止组合。安全边界放在清楚的 destructive contract 中。

#### 先学区间交易，再写策略

`edit` 相当于 split 两次、处理片段、merge 两次。回调接收 `q` 和隔离后的普通整数根，
**必须返回要拼回的根**，外面也必须保存返回值。可以修改、替换、删除或重新拼接片段。

```cpp
// q 和 root 接上面的例子。删除位置 [1,3)，得到 3,2。
root = q.edit(root, 1, 3, [](auto&, nidx_t) { return nidx_t(-1); });
// 空区间也调用回调，因此可以在位置 1 插入新节点，得到 3,9,2。
root = q.edit(root, 1, 1, [](auto& tree, nidx_t middle) {
    assert(middle == -1);
    return tree.make(9);
});
```

旧 root 已被消费。返回的片段不能与保留的左右两侧共享节点；被丢弃的节点仍占 arena
空间。回调可以扩容节点池，整数 handle 不失效，但引用/指针可能失效。`edit` 没有异常回滚。
这里的区间是调用时的秩区间，要求 `0 <= left <= right <= size(root)`。

#### 一个策略如何维护区间加与和

以下是可独立编译的完整例子。先只看 `pull/push`：`up(h)` 先重算结构 size，再调用
`ops.pull`；`down(h)` 调用 `ops.push`。两者都可省略，默认 `nfhq<T>` 就不维护额外摘要。

```cpp
#include "src-v3/fhq.hpp"

struct sum_item { long long x, sum, add = 0; };
struct sum_ops {
    void pull(auto& q, nidx_t h) const {
        auto& node = q[h];
        node.value.sum = node.value.x;
        if (node.left >= 0) node.value.sum += q[node.left].value.sum;
        if (node.right >= 0) node.value.sum += q[node.right].value.sum;
    }
    bool try_apply(auto& q, nidx_t h, long long delta) const {
        auto& s = q[h].value;
        s.x += delta;
        s.sum += delta * q.size(h);
        s.add += delta;
        return true;
    }
    void push(auto& q, nidx_t h) const {
        long long delta = q[h].value.add;
        if (!delta) return;
        for (nidx_t child : {q[h].left, q[h].right})
            if (child >= 0) try_apply(q, child, delta);
        q[h].value.add = 0;
    }
    void apply_one(auto& q, nidx_t h, long long delta) const {
        q[h].value.x += delta;
    }
};

int main() {
    nfhq<sum_item, sum_ops> q(sum_ops{});
    vector<sum_item> a{{1, 1}, {2, 2}, {3, 3}, {4, 4}};
    nidx_t root = q.build(nall(a));
    root = q.edit(root, 1, 4, [](auto& tree, nidx_t middle) {
        tree.apply(middle, 5LL);
        return middle;
    });
    assert(q[root].value.sum == 25);
    nidx_t h = q.kth(root, 2); // kth 已经 push 到目标节点
    q[h].value.x = 10;
    q.rebuild(h);
    assert(q[root].value.sum == 27);
}
```

策略对象实际存放在 `q.ops`，可以携带运行期参数和 move-only 状态。`T` 仍显式指定，
以免把 proxy/reference 初值误推导成节点拥有的值。策略类型直接写作 `nfhq<T,Ops>`，不再保留平行工厂名。
如果修改的是早先保存的 handle，先 `expose(h)` 下推祖先，再改自身值，再 `rebuild(h)`。
不要在尚有未下传状态的节点上直接 pull，否则孩子的旧摘要会覆盖父节点的新摘要。

#### 不能整段完成时发生什么

`apply(root,command)` 只把 command 当作当前操作，不要求它具有 `tag_id/compose`：

```text
空根：结束
ops.try_apply(q,h,command) == true：结束
否则：down(h) → 更新左子树 → ops.apply_one(q,h,command)
      → 更新右子树 → up(h)
```

成功必须同时维护摘要、自身元素和延迟表示；失败必须保持语义状态不变，合法命令必须
在叶子成功。`apply_one` 只更新当前节点自身元素，不重复更新左右子树，不记录整段 tag；
随后由 `up` 重建摘要。上面的加法总是成功，所以不会执行 fallback，但 `apply` 实例化时
仍需提供 `apply_one` 表达式。对 chmin/取模，这个钩子会实际执行。

固定线段树的内部节点一般只合并两个孩子；FHQ 的每个内部节点还拥有一个元素。
因此共享的是 command 的数学含义，不是节点布局。两种树共用 chmin/取模 command 的
完整对拍见 [conditional_tree_property.cpp](./test-v3/conditional_tree_property.cpp)。

`walk(root,visit,element)` 不要求 command。`visit(q,h)` 返回 true 就剪掉子树；返回 false
则 `down → 左子树 → element(q,h) → 右子树 → up`。可用摘要剪枝，在 element 中收集或修改
自身元素。与 segment 的 walk 不同，FHQ 的叶子可以返回 false，再交给 element 处理。
回调不能改变当前遍历的拓扑；需要选择不同分支顺序或联动多棵根时，直接使用
`q[h].left/right`、`down/up` 写递归。剪枝不会自动终止其他分支，找到答案后可捕获一个
found 标志，让后续 visit 返回 true。

#### 反转、位置标记与摊还界

反转会改变中序，是结构操作。策略需要配合 `swap_children(h)`，维护反向摘要或一个
已证明正确的摘要反转变换，并下传反转状态。交换聚合（例如 sum）不必保存双份摘要；
非交换聚合则不能只交换孩子、保留原摘要。核心不会替策略实现反转。

位置相关标记还必须变换坐标。长度 `L` 的子树上加 `p*i+b`，反转后延迟标记变成
`(-p)*i + b+p*(L-1)`；向右孩子下传时，截距还要加 `p*(left_size+1)`。
先反转再更新与先更新再反转一般不同。可参考
[fhq_position_property.cpp](./test-v3/fhq_position_property.cpp)：它测试仿射复合、位置偏移、
反转、切分重排和非交换摘要，逐项与 vector 对拍。

按秩维护序列可以自由改值；按 key 排序的根若用于 split_by/有序搜索，改值后必须保持
比较器下的中序顺序，否则应重新插入或重建。parent 指针也意味着这仍是 destructive
结构，不能直接共享节点做持久化。

`apply/walk` 的成本是访问节点数乘局部操作成本，递归栈深度等于树高。
固定线段树的 Beats 势能证明依赖固定分组；FHQ 的 split/merge/reorder 会改变分组，
不能仅因为对拍通过，就照搬其摊还界。`edit` 只保证自身期望 `O(log n)` 的结构成本。

### 6.1 `nbag`：用一个 FHQ 根装配有序多重集

头文件：`src-v3/bag.hpp`

`nbag<T,C>` 是 `nfhq<T>` 的薄适配器：`T` 必须显式给出，不能依靠 CTAD 从 source 推断。
这是所有权边界，不是语法限制：`nzip` 的元素可能是嵌套引用，`vector<bool>` 的元素是代理；
source 构造器会把每个元素显式转成 `T` 后再保存。`C` 必须是稳定的严格弱序，等价值仍作为
不同节点保留，并插入到已有等价值之后。它提供：

```text
insert / emplace                 插入并返回 nfhq handle
erase_one / erase_all            按值删一个 / 删除全部并返回数量
erase_at / erase_handle          按位置 / 当前有效 handle 删除
lower_bound / upper_bound        返回位置；可传兼容的 compare / projection
equal_range / count / find       返回 [left,right) / 数量 / 位置（未找到为 len）
contains                        存在性；小于 key 的数量直接用 lower_bound
kth / front / back               只读元素访问
sequence                         只读 nview
```

边界查询还有与 `nlower/nupper` 对齐的重载：

```cpp
bag.lower_bound(key, order, projection);
bag.upper_bound(key, order, projection);
```

它们分别寻找第一个 `!order(projection(value),key)` 与第一个
`order(key,projection(value))` 的位置，允许异构 key，也避免为结构体字段手造哨兵值。
承重前提是投影后的中序序列已经按 `order` 排好；不能拿与建树顺序无关的任意比较器搜索。
例如默认按 `pair` 字典序保存时，中序对 `pair::first` 仍然单调，因此可以直接按第一维 bound。
这两个重载直接沿树下降，仍是期望 `O(log n)`；对 `bag.sequence()` 调 `nlower/nupper` 则因
每次 positional access 本身为期望 `O(log n)`，总计期望 `O(log^2 n)`。

插入和删除复用 destructive split/merge；边界与排名查询只沿 BST 和子树大小下降，不拆根。
因此插入、删除和值查询均为期望 `O(log n)`，包括 `kth` 与 `sequence` 的每次访问；纯查询
不会改变 root 或树形，已有 `sequence` 仍有效。`insert/emplace`、任一 erase 和 `clear()` 是
结构修改，会使旧 `sequence` 失效。handle 在对应节点被删除前保持有效，arena 重分配不影响
整数 handle，但会使既有元素引用失效；`clear()` 还会使所有 handle 失效，并按已分配节点数
析构存储。arena 在删除后不会自动回收，节点存储保留到 `clear()`；不同 kernel 的根不能 merge。

这个适配器只复用 FHQ 真正有用的机关：arena、随机平衡、父指针、子树大小与 destructive
根代数。它不增加 owner/domain 外壳；同时也不为了“复用 split”而让只读查询物理改树。

### 6.2 `nvec_bag`：vector 后端的朴素基线

头文件：`src-v3/vec_bag.hpp`

`nvec_bag<T,C>` 用一个 `vector<T>` 保存同样的有序多重集语义，主要面向启发式/局部搜索中
复制与随机访问远多于插入、且需要连续内存和缓存友好的场景；也适合作为题解、对拍和
benchmark 中的朴素参照。它仍要求显式的 `T`、稳定严格弱序，并把 source 的每个元素显式
递归 owning 后物化为 `T`；`T` 本身应是 owning value type。source 构造先稳定排序，因此等价值保留 source 顺序。动态插入使用 `nupper`，
边界查询使用 `nlower/nupper`：

```text
insert / emplace                 插入并返回当时的位置
erase_one / erase_all / erase_at 按值删除 / 按位置删除
lower_bound / upper_bound        返回位置；可传 compare / projection
equal_range / count / find       返回 [left,right) / 数量 / 位置
contains                        存在性；小于 key 的数量直接用 lower_bound
kth / front / back / sequence    只读访问 / 只读 nview
```

vector 的位置会被插入和删除移动，因此这里没有 `erase_handle` 或 `nodes`：`insert/emplace`
返回的是当时的位置，不是可跨修改保存的 handle。`insert`、任一 erase 和 `clear()` 会使旧
sequence 失效；`reserve` 可能使已经取得的元素引用失效。`kth/front/back/operator[]` 使用
`decltype(auto)`，所以普通 `T` 返回 `const T&`，`T = bool` 则返回安全的 `vector<bool>` 值。
复制 `nvec_bag` 本身会复制其 `vector<T>`，因此两个 bag 的元素存储互不共享；但
`sequence()` 是借用的只读 view，不是脱离 bag 的副本。

边界查询的投影序列必须已经按传入 order 排好，和 `nbag` 的 bounds 重载具有相同前提。
`lower_bound/upper_bound/find/contains` 是 `O(log n)`，`kth` 与 sequence 每次
访问是 `O(1)`；vector 插入、删除需要移动后缀，均为 `O(n)`。source 构造的稳定排序为
`O(n log n)`，存储只保留当前元素，删除会立即释放被删除元素但 vector capacity 可能保留。

这是故意保留的基线，不是通用 `nvector` 或 `nbag` 的 drop-in 替换：需要稳定 handle、
期望对数插入/删除或 FHQ 根操作时应使用 `nbag`。

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

nsegment_trace(base, position, [&](nidx_t node) {
    tags[node].insert(value);               // 点插入写入所有祖先
});

nsegment_cover(base, left, right, [&](nidx_t node) {
    answer += query(tags[node]);             // 区间查询规范分解
});
```

动态开点只需更换 child callable，同一份 walk 不变：

```cpp
using outer_tree = nsparse_seg<multiset<int>, monostate>;
outer_tree outer(lo, hi);
nidx_t root = outer.make(multiset<int>{});

auto open_child = [&](nidx_t node, nidx_t side) {
    nidx_t next = side ? outer[node].right : outer[node].left;
    if (next < 0) {
        next = outer.make(multiset<int>{});
        if (side) outer[node].right = next;
        else outer[node].left = next;
    }
    return next;
};

nsegment_trace(root, lo, hi, x, open_child, [&](nidx_t node) {
    outer[node].aggregate.insert(y);
});
```

内层并不需要是 STL 容器。对于**强制在线**的动态区间次序统计，应把两个维度反过来：
`nsparse_seg<nidx_t,monostate>` 划分固定值域，每个 aggregate 保存一棵维护数组位置的 FHQ
root，所有内层根共同使用一个 `nfhq<nidx_t>` kernel：

```cpp
nidx_t insert_position(nidx_t root, nidx_t position) {
    auto [a, b] = positions.split_by(root, [&](nidx_t y) { return y < position; });
    return positions.merge(positions.merge(a, positions.make(position)), b);
}

nsegment_trace(root, value_lo, value_hi, value, open_child, [&](nidx_t node) {
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

`segment.hpp` 还提供数值 merge 适配器：`nadd<T>` 使用 `T{}` 加法。对 `pair` / `tuple`，
`nadd` 递归逐分量相加；`nmin` / `nmax` 保持标准字典序比较，并递归构造每个数值分量的
上界 / 下界单位元。标量 `nmin<T>` / `nmax<T>` 在 `numeric_limits<T>::has_infinity` 时分别
以正 / 负无穷为单位元，否则使用 `max()` / `lowest()`。数值叶子的边界必须可用，且 `<`
构成严格顺序；任一浮点分量都必须排除 NaN。

```cpp
vector<long long> a{1, 2, 3, 4};
nseg<long long> seg(nall(a));
assert(seg.fold(1, 4) == 9);
seg.set(2, 10);
```

`pointwise(other)` 对同位置叶子逐点 destructive merge，再重建内部节点。两棵树必须长度
相同，且操作对象具有同样的数学意义。复杂度 `O(n)`，不是伪装成 `O(log n)` 的根合并。

`max_right(left,predicate)` 返回最大的 `right`，使 `[left,right)` 的聚合满足 predicate；
`min_left(right,predicate)` 对偶地返回最小的 `left`。参数可取端点 `0` 与 `len()`，返回值
也在 `[0,len()]`。predicate 必须满足 `predicate(M.id()) == true`，并且聚合按搜索方向扩展
后只能从 true 变成 false；框架不尝试证明单调性。搜索严格保持左到右 merge 顺序，因此
也适用于非交换幺半群。在 predicate 与 merge 为 `O(1)` 时每次搜索为 `O(log n)`。

```cpp
nidx_t right = seg.max_right(left, [](long long sum) { return sum <= limit; });
nidx_t left = seg.min_left(right, [](long long sum) { return sum <= limit; });
```

### 7.3 `nlazyseg`：从普通 lazy 到条件下钻

先按需求选入口，不必一开始就编写所有策略钩子：

| 需求 | 入口 |
| --- | --- |
| 点改与区间结合聚合 | 保留更紧凑的迭代 `nseg` |
| 区间加、区间和 | `nlazy_addsum<T>` |
| 普通可复合 lazy 动作 | `nlazyseg(source, nlazy_ops{merge,action})` |
| Beats、取模、自定义节点不变量 | `nlazyseg(source, ops)` |
| 单调聚合边界搜索 | `max_right/min_left` |
| 复杂剪枝、临时修改叶子 | `walk`；更特殊的递归用公开 `push/pull` |

#### 普通 lazy：一个策略参数

```cpp
vector<long long> a{1, 2, 3, 4};
nlazy_addsum<long long> seg(nall(a));
seg.apply(1, 4, 5);
assert(seg.fold(0, 4) == 25);
seg.set(2, 10);
assert(seg.get(2) == 10);
```

`nlazyseg<Node,Ops>` 是底层形式，普通使用由初值和策略推导 Node 与 Ops。
`nlazy_ops{merge,action}` 是常规 lazy 的装配器，自动从 `merge.id()`、`action.tag_id()`
推导聚合与 tag 类型，并管理内部节点的待下传状态。

```text
Merge.id()
Merge(left,right)                    结合、保序
Action.tag_id()
Action.compose(newer,older)          先执行 older，再执行 newer
Action.apply(aggregate,tag,length)   对区间聚合施加 tag
```

`Merge` 满足结合律和单位元律，按左到右合并，允许非交换。Action 的恒等动作不改值；
动作需对区间合并可分配，且 `apply(apply(s,older),newer)` 与 compose 的结果相同。
下面是可独立编译的区间仿射和示例，系数顺序刻意写全：

```cpp
#include "src-v3/segment.hpp"

struct affine_tag { long long a = 1, b = 0; };
struct affine_sum {
    affine_tag tag_id() const { return {}; }
    affine_tag compose(affine_tag newer, affine_tag older) const {
        return {newer.a * older.a, newer.a * older.b + newer.b};
    }
    long long apply(long long sum, affine_tag f, nidx_t length) const {
        return f.a * sum + f.b * length;
    }
};

int main() {
    vector<long long> a{1, 2, 3, 4};
    nlazyseg seg(nall(a), nlazy_ops{nadd<long long>{}, affine_sum{}});
    seg.apply(0, 4, affine_tag{2, 1}); // 3,5,7,9
    seg.apply(1, 3, affine_tag{3, 4}); // 3,19,25,9
    assert(seg.fold() == 56);
    seg.set(2, 7);
    assert(seg.fold(1, 3) == 26);
}
```

数值范围必须保证计算不溢出，也可使用满足这些定律的模数类型。`nlazy_ops` 原样将同一
tag 传给两个孩子，适合逐元素同构动作；依赖子段相对位置的动作需要自定义 push/偏移。

旧的四参数 `nlazyseg<S,F,M,A>(source,m,a)` 已替换为
`nlazyseg(source,nlazy_ops{m,a})`，不保留第二套递归实现。`nlazy_addsum<T>` 的常见写法不变。
旧 FHQ 的两个回调则迁入一个 Ops 的 `pull/push` 方法；策略状态经 `.ops` 访问。

#### Node、Command、Tag 各自是什么

Node 是当前区间的信息；Command 是这次要求做的事；Tag 是留给孩子以后兑现的状态。
三者不必同型。普通仿射可以把 command 本身复合成 tag；chmin 可以用父节点最大值
隐式保存约束；取模只能剪枝或下钻，不需要保存“取模历史”。

自定义策略只在实际调用对应操作时检查表达式，没有统一 traits 或继承基类：

| 策略表达式 | 用途 |
| --- | --- |
| `ops.identity()` | 空区间和补齐叶子的 Node；join 的单位元 |
| `ops.make(value)` | 将初值或 set 的值构造为一个叶子 Node |
| `ops.join(left,right)` | 左右区间保序合并；fold 使用它 |
| `ops.init(q)`，可省略 | 分配策略自身所需的状态；建叶之前调用 |
| `ops.pull(q,h)`，可省略 | 从孩子重建节点；省略时使用 join |
| `ops.push(q,h,lo,hi)`，可省略 | 将父节点的延迟状态兑现到两个孩子 |
| `ops.try_apply(q,h,lo,hi,cmd)` | 尝试完成本次整段更新，返回 bool |

`q[h]` 直接访问 Node，根为 1、孩子为 `2*h` 与 `2*h+1`，根覆盖 `[0,q.base)`，
`base` 是至少为 1 的二次幂。真实区间只有 `[0,q.len())`，其余叶子是 identity；
必须正确合并它们，不能给 padding 应用真实元素的更新。`fold()` 返回根 Node，
`fold(l,r)` 返回按左到右合并的 Node，用户从中读取 sum/max 等字段。

节点存储是一个 vector；不用 tag 的策略不会产生任何通用 lazy 数组。常规装配器则
只为内部节点保存 tag/pending。Ops 可持有运行期状态或 move-only 成员，不能在移动树后
继续依赖旧树地址。当前构造需要可复制的 identity Node；move-only 策略不等于 move-only
Node 支持。没有 source 的显式 `nlazyseg<Node,Ops>(n,ops)` 创建 n 个 identity 叶子，
仅在 identity 也能表示合法元素时使用，否则提供 source。

`set` 调用 make 替换叶子；叶子的延迟语义必须随 payload 一起重置，不能在外部另藏旧
叶子 tag。默认装配器不保存叶子 tag，因此自动满足这一条。

#### 一个没有 Tag 的例子：区间取模

下面完整程序维护非负整数的区间取模、区间和与点赋值。这里的 `long long mod`
就是 Command，没有 compose，也没有 push：

```cpp
#include "src-v3/segment.hpp"

struct mod_info { long long sum = 0, maximum = 0; };
struct mod_ops {
    mod_info identity() const { return {}; }
    mod_info make(long long x) const { return {x, x}; }
    mod_info join(mod_info a, mod_info b) const {
        return {a.sum + b.sum, max(a.maximum, b.maximum)};
    }
    bool try_apply(auto& q, nidx_t h, nidx_t lo, nidx_t hi, long long mod) const {
        if (q[h].maximum < mod) return true;
        if (hi - lo != 1) return false;
        q[h] = make(q[h].sum % mod);
        return true;
    }
};

int main() {
    vector<long long> a{17, 8, 23, 4};
    nlazyseg seg(nall(a), mod_ops{});
    seg.apply(0, 3, 7LL); // 3,1,2,4
    assert(seg.fold().sum == 10);
    seg.set(1, 20LL);
    seg.apply(1, 4, 6LL); // 3,2,2,4
    assert(seg.fold(1, 4).sum == 8);
}
```

要求值非负、模数正。一次真正改变 `x` 的取模使它小于原值的一半：若 `m <= x/2`，
余数小于 m；否则余数为 `x-m < x/2`。势能取各元素 `log2(x+1)` 之和，每次有效改变
消耗常数量级势能，点赋值重新注入势能。设初值与赋值都不超过 V，s 次点赋值、q 次操作，
总时间可界为 `O(n + (q + (n+s) log(V+1)) log n)`，而非每次修改都 `O(log n)`。

#### try_apply 的三条承重约定

1. 返回 true：整个节点已经正确，包括摘要、叶子值和延迟表示；也可以只是无须修改。
2. 返回 false：语义状态必须保持不变。框架随后 push、下钻、pull，不能留下半次更新。
3. 合法命令必须在真实叶子成功。debug 有断言辅助发现错误，但违反契约不承诺恢复。

`apply(l,r,cmd)` 只对**完全覆盖**的节点调用 try_apply。部分覆盖会继续下钻，不会因
父区间碰巧满足整段更新条件就改掉区间外的元素。时间是实际访问节点数乘局部代价。
push 还要有自己的正确性证明：父节点成功存下的约束必须能在孩子上兑现。

chmin 的常见节点为 `sum/max1/max2/count_max1`：`max1 <= cap` 无须修改；
`max2 < cap < max1` 时只降低最大值组，否则返回 false。第二大值必须是**严格次大值**，
判断也必须是严格不等号。更新 sum 后还要保存对孩子的约束，可直接用父节点 max1
隐式下传；若无元素则按 identity 处理。完整可运行实现见
[conditional_tree_property.cpp](./test-v3/conditional_tree_property.cpp)。

仅含区间 chmin、点赋值、sum/max 查询的固定树，可用所有节点中不同值个数之和作势能，
初始 `O(n log n)`。一次失败的完整覆盖会合并至少两个值层；部分覆盖的边界节点与点赋值
每次最多在 `O(log n)` 个祖先上增加值层。由此总成本为 `O((n+q) log n)`，假定节点操作
常数时间。这一证明没有覆盖区间加、chmax 或任意其他新命令，也没有覆盖任意 FHQ 重排。

#### walk：查询剪枝与 adhoc 下钻

`walk(l,r,visit)` 只访问相交节点，调用 `visit(q,h,lo,hi,full)`；true 表示当前子树已处理
或应跳过，false 表示由框架 push 后从左到右递归并 pull。叶子必须返回 true。
`full` 明确区分部分覆盖：允许根据整个节点 maximum 判断“这一段里肯定没有答案”并剪枝，
但只有完整覆盖时才能修改整个节点。

例如在上面的 mod_ops 树中找 `[left,right)` 内第一个大于 threshold 的位置，未找到返回 right：

```cpp
nidx_t found = right;
seg.walk(left, right, [&](auto& q, nidx_t h, nidx_t lo, nidx_t hi, bool) {
    if (found < right || q[h].maximum <= threshold) return true;
    if (hi - lo != 1) return false;
    found = lo;
    return true;
});
```

这个查询只在可能有答案的分支下降，找到后停止后续分支；在常数时间摘要操作下为
`O(log n)`。它没有伪装成一个“查询 tag”。需要从右向左选分支、多棵同域树联动、
回溯自定义信息时，可以直接操作 `push(h,lo,hi)`、孩子索引和 `pull(h)`；读取孩子前
先 push，修改后再 pull。walk 本身不能改变拓扑、base 或存储长度。

`apply/fold/set` 复用同一 walk；查询也会 push/pull，因此物理上修改内部状态。
`max_right/min_left` 与 `nseg` 具有相同的 predicate、返回值和保序契约，也可能 push/pull；
它们处理标准单调聚合搜索，`walk` 继续承担 Beats 剪枝、多树联动等自定义下钻。
常规常数时间 lazy 的区间更新、fold、set 为 `O(log n)`，根聚合 `fold()` 为 `O(1)`；
存储 `O(n)`，递归栈 `O(log n)`。若 Node 是字符串，复制、join、pull 的实际代价必须另算，
不能只数树高。空区间 apply 不调用策略，空区间 fold 返回 identity。

### 7.4 `nsparse_seg`

一个 append-only kernel 同时承载 destructive 与 persistent 根：

```cpp
nsparse_seg<long long> seg(0, 1LL << 60);
nidx_t a = -1;
a = seg.set(a, 100, 7);          // destructive
nidx_t b = seg.set_copy(a, 200, 9); // persistent path-copy
nidx_t c = seg.merge_copy(a, b);    // 保留旧根
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

### 7.5 `nreftree`：不可变模式与逻辑拆合

头文件：`src-v3/reftree.hpp`。`nreftree<Info,Ops>` 是可直接调用的持久化模式内核。
它维护有序二叉展开、不可变引用及对齐块替换；用户决定节点信息和查询所需的判断。
默认 `nreftree<>` 使用空信息 `monostate` 与 `nreftree_noop`，只维护结构。

#### 根表示模式，观察高度决定长度

完整有限对象是 `(root,H)`，表示 `[0,2^H)` 上的终端序列，满足
`0 <= q[root].height <= H <= 60`。节点的 `height` 是存储高度；没有存储的上层
表示**重复**。同一个 `01` 根在 `H=1` 展开为 `01`，在 `H=3` 展开为 `01010101`。

根、孩子和高度使用 `nidx_t`；逻辑地址及搜索结果使用 `uint64_t`，与物理节点编号宽度
分开。这与稀疏线段树使用独立的大坐标域类似：默认索引模式也能观察 `2^60` 个逻辑位置，
但节点池数量仍须能由 `nidx_t` 表示。`NITORI_INDEX_64` 扩宽物理 handle，不改变重复语义。

```text
leaf(args...)              用 Info(args...) 创建高度零终端
join(H,left,right)         拼接两个在 H-1 下观察的模式
split(root,H)              返回在 H-1 下观察的逻辑左右半
block(root,x,k)            提取从 x 开始、长度 2^k 的对齐块
paste(root,H,x,k,source)   持久化替换该块；source 较矮时重复到 k
set(root,H,x,terminal)     用高度零终端替换单点
leaf_at(root,x)            返回该地址的终端 handle
find_first(root,H,x,has)   第一个 >=x 的合格位置；不存在返回 2^H
kth(root,H,k,count)        第 k 个合格位置，k 从零开始
operator[](root) const    读取 value、left、right、height
nodes() / reserve(n)       已分配节点数 / 预留空间
```

所有根属于同一个 kernel。没有空根，也没有预留的“全零/全一”编号。`leaf` 每次创建不同
终端，哪怕信息相等；应用自行保存、重用所需终端。`set` 的最后一个参数是终端 handle，
不是一个待转换的 `Info` 值。

`split(p,H)` 要求 `height(p)<=H` 且 `H>0`；存储高度等于 H 时取物理孩子，否则返回
`(p,p)`。`join(H,l,r)` 要求 `height(l),height(r)<H`；`l==r` 时直接返回 l，
否则追加新分支。两者均不消费旧根。不要把物理 `left/right` 当成省略层的逻辑孩子，
也不要用一次 `visited[root]` 去重来统计所有逻辑出现。

`paste` 要求 `0<=k<=H`、`height(source)<=k`、`x<2^H` 且 x 被 `2^k` 整除；
`block` 同样要求对齐，且整块位于调用者选择的合法观察域内。`leaf_at` 的 x 也须位于
合法观察域；这两个接口不保存或推测 H。它们不提供任意偏移切片或任意长度拼接。

#### 自定义信息：只要求构造新分支的表达式

```cpp
Info ops.join(const auto& q, nidx_t H, nidx_t left, nidx_t right);
```

策略看到只读内核、父高度及孩子根，自行把孩子信息解释到 `H-1` 再生成父信息。
`q[p].value` 只对应节点**自身的存储高度**。框架不强制提供 `id/compose/lift`，
也不比较或哈希 Info。非空信息若需要创建分支，必须提供相应 Ops。
Info 不必默认构造、复制或赋值；移动构造即可支持 move-only 信息。
策略保存在公开的 `q.ops` 中，也可以携带 move-only 状态。

`join(H,p,p)` 消层时不调用策略。策略的信息解释必须与这种重复一致；不能把物理创建
次数、当前绝对位置或某次出现的独有状态作为共享内容。策略可维护缓存/计数器，但不能
修改旧节点、在该构造回调中递归向同一 q 分配，或改变既有信息的含义。返回的信息不能
借用会随 pool 扩容失效的地址，应保存稳定 handle 或自己拥有的值。

下面是可独立编译的计数与搜索例子：

```cpp
#include "src-v3/reftree.hpp"

struct count_ops {
    uint64_t count(const auto& q, nidx_t p, nidx_t H) const {
        return q[p].value << (H - q[p].height);
    }
    uint64_t join(const auto& q, nidx_t H, nidx_t l, nidx_t r) const {
        return count(q, l, H - 1) + count(q, r, H - 1);
    }
};

int main() {
    nreftree<uint64_t, count_ops> q;
    auto zero = q.leaf(0), one = q.leaf(1);
    auto p = q.join(1, zero, one);
    auto r = q.paste(p, 3, 4, 1, zero);
    // (p,3) 是 01010101；(r,3) 是 01010001。
    auto count = [&](nidx_t root, nidx_t H) { return q.ops.count(q, root, H); };
    auto has = [&](nidx_t root, nidx_t H) { return count(root, H) != 0; };
    cout << count(p, 3) << ' ' << count(r, 3) << '\n'; // 4 3
    cout << q.find_first(r, 3, 4, has) << '\n';       // 7
    cout << q.kth(r, 3, 1, count) << '\n';            // 3
    cout << (q.leaf_at(p, 5) == one) << '\n';         // 1，旧根保持不变
}
```

无摘要的结构使用者也可将信息放在外部表中，通过只读节点访问做物理 DAG 动态规划。
需要双树递归或带绝对位置的查询时，直接使用 `split/join` 写递归，不需要注册策略族。

#### 搜索需要的承诺与信息提升

`has(root,h)` 必须准确回答在该观察高度是否存在合格位置，并与逻辑拆分一致；它不能
依赖未传入的绝对出现位置。`count(root,h)` 返回合格逻辑位置数，叶上为 0 或 1，
等于两个逻辑孩子计数之和。`kth` 要求 `k<count(root,H)`。回调按引用使用，允许
move-only callable；查询期间不能修改 q。`find_first` 接受 x 超过域末端，仍返回 `2^H`。

只有“可能存在”的保守剪枝不能满足 `find_first` 的对数调用界；这种算法应自行遍历，
按实际访问量计费。这里也不提供把任意逐叶谓词自动变成快速搜索的承诺。

计数的提升是乘重复次数，但其他摘要未必如此。例如 FESTIVAL 双底色费用
`(a,b)` 分别表示从全零/全一起步的最少涂写次数，满足 `|a-b|<=1`。重复 d 层后：

```text
m = 2^d * min(a,b)
lift(a,b,d) = (m + [a>b], m + [b>a])
```

父节点先把两子信息提升到 `H-1`，令两种底色的子费用和分别为 A、B，得到
`(min(A,1+B), min(B,1+A))`。例如 `1110` 的费用为 `(2,1)`，重复两次为 `(3,2)`，
不能把两个分量都简单翻倍。这个提升可完全写在 Ops 中，无须修改 nreftree。
左右有序也不要求交换律；字符串/哈希等摘要可保留拼接顺序。

#### 生命周期、成本与独立验证

已发布节点仅能只读访问，更新只追加节点；复制根为 `O(1)`，复制可复制的整个 kernel
则复制节点池及策略。节点引用可能因 `leaf/join/paste/set/reserve` 失效，整数 handle
保持有效。没有父指针、自动回收、全局驻留或内容判等；不同根可以表示相同内容。

`split` 为 `O(1)`；`join/leaf` 的追加为摊还 `O(1)`，另计 Info/策略成本。
`block/leaf_at` 为 `O(H+1)`；`paste` 为摊还 `O(H-k+1)` 结构工作，至多分配 `H-k`
个节点，`set` 至多 H 个。递归栈最多 `O(H+1)`；vector 扩容还须计入 Info 的移动/复制成本。
查询不分配树节点，`find_first` 为 `O(H+1)` 次 has 调用与栈空间，`kth` 为 `O(H)` 次
count 调用、常数额外空间。用户策略若物化字符串或在内部循环提升，要另外计算实际费用。

[`reftree_property.cpp`](./test-v3/reftree_property.cpp) 包含小布尔函数及全部对齐块穷举、
随机跨版本取块/重复粘贴、搜索/选择与朴素数组对拍、`H=60` 边界、非交换字符串、
双底色费用与实际涂写状态 BFS 对拍，以及不要求默认构造/复制/赋值的 move-only 装配。
[`reftree_bench.cpp`](./bench-v3/reftree_bench.cpp) 在高度 60 下执行 2 万次持久化更新、
5 万组历史搜索/选择，检查独立答案、节点分配量和两种索引模式下的 deterministic checksum。

## 8. 基础数据结构

头文件：`src-v3/ds.hpp`

### `nlist<T>`：节点池与双向链根

头文件：`src-v3/list.hpp`。一个池可以承载多条链，每条链只有 `{first,last}` 两个
端点；节点用 `nidx_t` handle 标识，`-1` 表示无邻居或尾后边界。handle 是节点身份，
不是第几个元素。链根不缓存长度，不提供 iterator 或位置下标；这是竞赛用 pooled-chain
kernel，不是 `std::list` 的替代品。

```cpp
nlist<string> q;
nlist<string>::root a, b;
auto river = q.insert(a, -1, "river");  // 尾插，返回 handle
q.insert(a, river, "gear");            // 插到 river 前面
auto part = q.cut(a, river, -1);       // 截出 [river,尾后)
q.splice(b, -1, part);                // 尾接，part 变空
q[river] += " lab";
for (auto h = b.first; h >= 0; h = q.next(h)) cout << q[h] << '\n';
q.erase(b, river);                    // 析构载荷、回收槽位，返回后继
```

```text
root.first / root.last / root.empty()
q[handle] / prev(handle) / next(handle)
insert(root,position,args...)         在 position 前构造，-1 表示尾插
erase(root,handle) / clear(root)      删除单节点 / 清空链
cut(root,first,last)                  截出沿 next 的半开段，返回独立链根
splice(root,position,part)            将 part 插入 root，消费 part
reverse(root) / reserve(node_count)
```

给定节点边界后，`cut/splice` 都为 `O(1)`，跨链也不扫描段长，不分配、不移动载荷。
`erase` 为 `O(1)` 加载荷析构；`insert` 摊还 `O(1)` 加构造与池扩容的搬迁成本。
`clear/reverse` 为 `O(k)`。要搬动同链的一段，先 cut，再向剩余链的边界 splice。
空段可截取，空链可拼接；非空段的 last 必须沿 next 可达，`-1` 表示包含链尾。

根是轻量描述，不是独立 owner：复制根会产生别名，不能把别名当作两条独占链操作。
参与操作的根和 handle 必须属于同一个池，splice 两侧节点集合必须不相交。
所有活节点的 handle 在扩容、截段、拼接、反转后稳定；节点引用和载荷引用可能因
`insert/reserve` 扩容失效，插入参数也不得借用可能搬迁的本池对象。
`erase` 后 handle 失效，即使这个编号随后被复用也不代表原节点复活。

池复用空槽，槽位数由同时存活节点数的峰值决定；删除立即析构 T，池容量保留。
T 要满足 vector 的搬迁要求，不再承诺不可移动载荷或元素地址稳定。
复制 kernel 是复制整个池；移动 kernel 后原有根属于目标，源需重新初始化才能继续使用。
`pool[h].prev/next` 公开拓扑，`pool[h].value` 用 optional 管理槽位生命期；直接修改时由
调用者维护双向链接、根端点及空槽链不变量。普通载荷访问使用 `q[h]` 即可。

#### 组合成块状链表

块是普通载荷，块内直接用已有 view、离散操作和自行维护的统计量。下面在节点 h
的块内位置 k 拆分，要求 `0 < k < nlen(q[h])`：

```cpp
nlist<vector<long long>> q;
nlist<vector<long long>>::root blocks;
auto h = q.insert(blocks, -1, vector<long long>{1, 2, 3, 4});
nidx_t k = 2;
auto tail = ncollect(nsub(nall(q[h]), k, nlen(q[h])));
q[h].resize(k);
auto right = q.insert(blocks, q.next(h), move(tail));
// 此时 h 和 right 两块依次为 [1,2]、[3,4]。
```

需要段和、最值或 lazy 时，T 就写成 `{data,sum,min,max,lazy,...}`，拆块前 push，
拆块后 pull。整块区间的移动只操作链根；统计量随载荷留在原节点，不必重建或交给
额外的框架。链上找位置需自行扫描块长或组合索引，块大小的平衡策略也由算法决定。
不能把 `nall(q)` 误当链序；块内 `nall(q[h].data)` 或自己的 handle view 可以直接复用。

`test-v3/list_blocks_property.cpp` 给出完整组合：拆块/合块、区间加、区间和、跨链搬段
以及区间反转，并与平坦 vector 对拍。示例为验证组合而扫描合并相邻小块，未承诺每次
修改都为平方根复杂度。

### `nfenwick<T,Group>`

要求 Abel 群：结合、交换、单位元、逆元。`add`、`prefix`、`fold`、`get`、`set` 均为
`O(log n)`。`lower_bound(target)` 返回第一个使 `prefix(i+1) >= target` 的元素位置 i，
`upper_bound(target)` 返回第一个使 `prefix(i+1) > target` 的位置；比较关系由可选 less
定义，找不到时都返回 `len()`，返回范围为 `[0,len()]`。统一使用 `lower_bound` 名字。

两个 bound 都要求前缀序列在 less 下单调；任意 Abel 群本身并不保证这一点。普通加法的
典型合法场景是所有元素和增量非负，使前缀不下降。若更新会破坏此前缀单调性，fold 仍然
有效，但 bound 搜索没有正确性保证。

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

### `ndeque_agg<T,M>`

双向聚合队列以两个有方向的聚合栈表示
`reverse(left) + right`。`push_front/push_back/pop_front/pop_back/front/back/fold` 均摊
`O(1)`，但一次端点访问或弹出可能触发 `O(n)` 的半分重建；空间为 `O(n)`。`M` 只要求
单位元与结合律，聚合严格保持从队首到队尾的非交换顺序。

`operator[](position)` 按队列顺序提供 `O(1)` 只读访问，因此可以借用 `nall(deque)` 做限制性
枚举。由 `nall` 捕获的长度在 push/pop 后失效；重建或修改也可能使先前取得的元素引用失效。
`front/back/pop_front/pop_back` 要求队列非空。

### `nsparse_table<T,O>`

要求结合、幂等，查询必须非空，合并保留从左到右顺序，不要求交换律。
重叠部分的聚合连续出现两次，由幂等律消去。预处理 `O(n log n)`，查询 `O(1)`。

### `ntopk<T,K>`：带证据的前 k 摘要

头文件：`src-v3/topk.hpp`。`ntopk<T,K>` 是固定容量、best-first 的候选摘要；候选的
值、来源、前驱等证据都保存在 T 中并整体移动，不要求字段名或额外 owner。空摘要是单位元。

```cpp
struct choice { long long score; int source, previous; };
struct better { bool operator()(choice a, choice b) const { return a.score > b.score; } };
using top4_merge = ntopk_merge<choice, 4, better>;

top4_merge merge;
auto state = merge.single({10, 3, 7});
merge.update(state, {12, 5, 9});  // 提交候选，证据随分数一起保留
```

状态提供 `len()/empty()/operator[]/front()`，元素访问只读；下标为 `[0,len())`，
`front()` 要求非空。操作提供 `id()/single(candidate)/update(state,candidate)` 和
`operator()(left,right)`；`update` 返回新候选是否被保留。所有摘要须使用同一套稳定的
比较/key 规则，按 key 合并只接受 key 唯一的摘要。更新可能移动候选，不能把槽位引用当作证据身份。

`ntopk_merge<T,K,Better>` 合并两个已排序摘要并截断到 K，单位元为 `{}`，单次提交和
合并都是 `O(K)`，空间为 `O(K)`。相同排名时左摘要优先；因此普通版结合但一般不交换，
若需要交换必须在 `Better` 中写入稳定的来源/证据 tie-break。普通版不幂等，不能用于
依赖幂等性的重叠区间 sparse table。

`ntopk_by_merge<T,K,Better,KeyOf>` 先让每个 key 只保留最佳候选，再取前 K；它可直接
作为 `nseg` 或 `nqueue_agg` 的合并对象。key 检查是线性扫描，合并上界为 `O(K^2)`、
单次提交为 `O(K)`，空间为 `O(K)`。为了保持结合律，`Better` 必须能确定性比较不同 key
的代表；值相同时应加入来源或 key 的 tie-break。按 key 版幂等，适合需要幂等摘要的组合。

`update` 只表示提交新候选或对同 key 做更优松弛，不提供任意覆盖和删除。仅保存前 K 项时，
删除某项后无法知道被截掉的第 K+1 项；需要真正点修改时，把摘要作为 `nseg` 叶子，由外层
完整保存叶子并调用 `set` 重建。`K=0` 合法但所有候选都会被丢弃。T 需要默认构造、复制和移动。

### `nwavelet<T>`

头文件：`src-v3/wavelet.hpp`

静态 Wavelet Matrix 先把任意可排序值压缩成 rank，因此不要求整数值域：

```cpp
vector<long long> a{7, -2, 7, 4, 9};
nwavelet wave(nall(a));
auto x = wave.kth(1, 5, 2);          // 子数组第 2 小，0-based
nidx_t y = wave.less(0, 5, 7);          // 严格小于 7 的数量
nidx_t z = wave.count(0, 5, 4LL, 9LL);  // 值域 [4,9)
```

还提供 `access`、单值 `count`、`next(>=lower)` 和 `previous(<upper)`。构造
`O(n log n+n log sigma)`，空间 `O(n log sigma)`，查询 `O(log sigma)`；所有区间合法，
`kth` 的 order 必须落在子数组内。

## 9. 图不是容器：`ngraph` 拓扑与边身份端口

头文件：`src-v3/graph.hpp`

```cpp
ngraph graph{
    vertices,                       // 有限 key descriptor
    [&](Key vertex) { return adjacency(vertex); },
    [&](const Edge& edge) { return edge.to; },
    [&](const Edge& edge) { return edge.id; }
};
```

算法按需使用以下表达式，不依赖具体存储：

```text
graph.vertices
graph.edges(vertex)
graph.target(edge)
graph.edge_id(edge)     // 需要边身份的算法使用
```

邻接结果只需能被 range-for。整数点数可直接构造 `ngraph{n,next}`；它使用无分配的
`nvertices` 域，不引入 view/hash。自定义 `vertices` 只须提供 `len()`、`operator[]` 和
`vertices.inverse(key)->[0,n)` 把语义顶点映到算法内部 position。连续整数、切片、反转和
笛卡尔积通常自带结构 inverse；任意离散 key 可在装图前用 `ninvert` 一次性附加静态 hash
fallback。没有额外 index 参数、`ngraph_like` concept 或默认 edge trait。

第四个构造参数 `id` 是边身份投影。`edge_id(edge)` 返回非负 `nidx_t`：不同逻辑边
ID 不同，同一条无向边的两个方向共享一个 ID，重边即使端点相同也必须使用不同 ID。
它不是邻接下标、CSR 存储位置、目标顶点或半边 handle；不规定 `id ^ 1` 为反向边。
ID 在算法调用中须稳定，允许稀疏或保留过滤前的原图编号，`-1` 保留为“没有边”。
ID 的有效范围须能用 `nidx_t` 表示；不能把超范围整数交给投影再依赖隐式窄化。
需要 `used[id]` 等数组时，调用方须另行提供匹配的下标域或显式压缩，不能静默重编号。

`ngraph{vertices,next}` 和 `ngraph{vertices,next,to}` 仍用于纯拓扑图，没有可调用的
`edge_id` 端口，不会伪造编号。BFS、距离算法等仍可运行在无 ID 的隐式图上；
lowlink／圆方树则要求真正的边身份，不保留按父顶点猜边的 fallback。
这些构造形式也都接受整数点数。邻接 record 的形状自由，边记录和 callable 均可为
move-only；算法只保存数值 ID，不保存可能失效的边地址。

### 9.1 任意后端示例

```cpp
vector<vector<nidx_t>> adjacency{{1, 2}, {2}, {}};
auto graph = ngraph{
    3,
    [&](nidx_t vertex) -> const vector<nidx_t>& { return adjacency[vertex]; }
};
auto distance = nbfs(graph, 0);
```

只要端口表达式成立，同一个 BFS 也可以运行在隐式状态图、forward-star、CSR 或用户自定义
压缩结构上。

非整数顶点也沿同一条端口装配：

```cpp
vector<string> names{"source", "middle", "sink"};
auto vertices = ninvert(nall(names));
auto graph = ngraph{
    move(vertices),
    [&](const string& vertex) { return adjacency(vertex); }
};
auto distance = nbfs(graph, string{"source"});
```

hash inverse 由 vertex descriptor 自己拥有；其借用的 `names` 与邻接后端仍须活过算法调用。

显式无向重边可以直接装配，无需公共 edge 基类：

```cpp
struct arc { nidx_t to, id; };
vector<vector<arc>> adjacency{{{1, 7}, {1, 12}}, {{0, 12}, {0, 7}}};
auto graph = ngraph{2,
    [&](nidx_t v) -> const auto& { return adjacency[v]; },
    [](const arc& e) { return e.to; },
    [](const arc& e) { return e.id; }};
// 两条平行边；双向邻接顺序不需要一致。
```

### 9.2 `ncsr`

头文件：`src-v3/graph_store.hpp`

```cpp
struct edge { nidx_t from, to, id; long long weight; };
vector<edge> edges = ...;
auto graph = nmake_csr(n, nall(edges),
                       [](const edge& e) { return e.from; },
                       [](const edge& e) { return e.to; },
                       [](const edge& e) { return e.id; });
```

构造 `O(V+E)`，每个 source 桶内保持输入顺序，edge record 不要求默认构造。`ncsr` 自己
已经满足图端口，也可用 `.view()` 得到借用它的轻量 `ngraph`。`.view()` 在 owner 移动或
销毁后失效。const 与非 const `.view()` 都借用原 owner 的投影，因此支持 move-only
投影；边 ID 不因 CSR 分桶重排而改变。省略 `id` 的 `nmake_csr(n,edges,from,to)`
仍构造纯拓扑 CSR，本体和 view 都不会凭存储位置生成 ID。

如果输入每条无向边只存一次，使用 `nmake_undirected_csr`：

```cpp
struct input_edge { nidx_t u, v; long long weight; };
vector<input_edge> input{{2, 0, 7}, {0, 2, 3}, {1, 1, 11}};
auto graph = nmake_undirected_csr(4, input,
    [](const input_edge& e) { return e.u; },
    [](const input_edge& e) { return e.v; });
// graph 的 record 是 ncsr_edge{from,to,id}，id == input 中的位置。
// 每条边展开两次，自环也有两个同 ID 的邻接项。
auto distance = ndijkstra(graph, 2,
    [&](const auto& e) { return input[graph.edge_id(e)].weight; }, 1LL << 60);
```

该构造器只拥有端点与 ID，不复制或借用输入 payload，输入 record 可为 move-only。
查询权值等外部 payload 时，调用方自行维持其寿命与 ID 映射。顶点是 `[0,n)`，
构造时间／空间 `O(V+E)`，两倍输入长度须能用 `nidx_t` 表示。
这里明确以**本次输入枚举的位置**编号；若需要保留既有的稀疏／过滤前 ID，使用
带 ID 投影的 `nmake_csr`，并自行提供共享 ID 的双向记录，不要把已展开的双向项
再传给 `nmake_undirected_csr`。

## 10. 图算法

头文件：`graph.hpp`、`graph_algo.hpp`、`flow.hpp`

```text
nbfs / nbfs_many     O(V+E)，返回按稠密 position 存储的距离
n01bfs                O(V+E)，边权严格为 0/1，不可达为 -1
ndijkstra            O((V+E)log V)，边权非负
nbellman_ford         标准全边扫描，返回距离或以 nullopt 表示源点可达负环
nbellman_ford_closure 最短路闭包，区分 +∞、有限距离与负环影响的 −∞
ntoposort             O(V+E)，返回稠密 position；长度不足表示有环
nscc                  O(V+E)，Kosaraju，需要正图和反图
nlowlink              O(V+E)，Tarjan 无向图割点与桥，支持重边、自环及非连通图
nblockcut             O(V+E)，点双圆方森林及各块的原图边 ID 列表
neuler                DFS/Hierholzer 构造有向或无向 Euler 迹
nkruskal              O(E log E)，返回最小生成森林的权值和输入 edge positions
ndenseprim            O(V²)，按需取权的稠密 Prim，返回最小生成森林的权值与 parent
ndinic                Dinic 最大流与残量 cut
nhopcroft_karp        二分图最大匹配
```

`neuler` 直接位于 `graph_algo.hpp`，使用图的边身份端口：

```cpp
auto trail = neuler(graph, m);                          // 无向，自动选起点
auto from = neuler(graph, source, m);                   // 指定语义顶点 key
auto directed = neuler(graph, m, neuler_kind::directed);
// neuler_result：vertices / edges 是起点一侧的遍历结果；complete 只报告覆盖数量
```

`m` 是调用者给定的全图逻辑边数；边 ID 可稀疏，相同 ID 被视为同一条边。调用者保证
有向边各有一个邻接项，无向边（自环也一样）各有两个共享 ID、端点相反的邻接项，
并保证所选起点可走出所在部分的一条欧拉迹。`neuler` **不验证**这些前提，也不要求覆盖
其他不连通分量：它保留从起点得到的局部结果。`complete` 仅表示全图不同 ID 数及本次
走过的 ID 数都等于 `m`，**不是**轨迹合法性证明；输入或起点不满足欧拉前提时，
`vertices[i] --edges[i]--> vertices[i+1]` 不保证成立，检查由调用者负责。
自动入口优先选首个奇度／出入差为 `+1` 的点，否则选首个有邻接的点；要指定分量，
显式传 `source`。无边时非空顶点域返回 position 0 的单点结果，空图返回空结果；
两者在 `m == 0` 时 `complete == true`。
结果顶点是稠密 position，边是原 ID；在欧拉前提下，`complete == false` 仍可是一条
有效的起点分量欧拉迹。

先把邻接读成轻量 `(to,id)`，然后 DFS 中按 ID 标记、回溯时写入顶点与边，最后反转。
这使 range-for 后端无需在递归期间保持迭代器有效，也不复制 edge record。Euler 前提保证
起点所在部分的回溯段能拼接成一条迹。端口操作为常数时，期望时间／空间均为
`O(V+A)`，`A` 为邻接项数；递归深度最多为不同边 ID 数加一，深图须留足栈。
不同 ID 数、各点度数及出入差须可由 `nidx_t` 表示。

`ndenseprim` 位于 `graph_algo.hpp`，直接接收顶点数和取权 callable，适合矩阵及隐式稠密图：

```cpp
auto forest = ndenseprim(n, [&](nidx_t u, nidx_t v) { return matrix[u][v]; }, infinity);
// ndenseprim_result<W>：weight 总权值、parent 父节点数组、components 连通分量数。
// parent[v] == -1 表示该分量的根；否则选中无向边 (parent[v],v)。
```

顶点为 `[0,n)`，`n>=0`；`weight(u,v)` 须稳定、对称，缺边返回 `infinity`，有限边权
严格小于它。允许负权、零权和不连通图；自环忽略。空图返回零权、空 parent 和零个分量。
每步选取尚未加入森林且连接代价最小的顶点，再用其边权更新其余顶点；若没有连接边，
就开始一个新分量。连接代价是跨割的单条边权，不是到根的路径长度。
算法额外空间 `O(V)`、时间 `O(V²)`，每对不同顶点恰好取权一次；不要求预先存储全部边。
这些复杂度假定取权与权值运算为常数时间。`W` 由 `infinity` 推导，取权结果转换为 W；
`W{}` 表示零，比较与加法须精确，累计权值不得溢出，但总权值可以超过 `infinity`。
需要宽累加时，显式传入 `__int128_t` 等宽类型的哨兵。相同最优权值不承诺唯一的 parent。

两种 Bellman–Ford 均位于 `graph_algo.hpp`，只依赖既有图端口，不包含 SPFA：

```cpp
auto result = nbellman_ford(graph, source, cost, infinity); // optional<vector<W>>
auto distance = nbellman_ford_closure(graph, source, cost,
                                    infinity, negative_infinity); // vector<W>
```

`source` 是语义顶点 key；输出按 `graph.vertices` 的稠密 position 存储。
标准版存在源点可达负环时返回 `nullopt`，否则返回距离；不可达点为 `infinity`。
这里“闭包”指最短游走的三态结果：不可达为 `infinity`，下界有限为最短距离，
可从源点到达某个负环、再从该负环到达的点为 `negative_infinity`。
不受负环影响的点仍保留正确距离；不可达负环不影响结果。

共享内核最多进行 `V-1` 轮原地全边松弛，一整轮不变就提前退出。
每轮至少覆盖再多一条边的路径；无可达负环时最短路可去环成为至多 `V-1` 条边的简单路。
之后只读扫描仍可松弛的边：标准版据此判负环，闭包版收集这些边的终点，
再做一次向前可达遍历。可达负环上不可能所有边都满足距离不再下降的约束，
因此必有种子；种子的后继恰是应标记的负环影响集合。
这个遍历只传播标记，每点最多入队一次，不进行队列式距离松弛。

内核先记录邻接非空的顶点，每轮只扫描这些顶点，避免大量孤立点拖成 `O(V²)`。
时间上界为 `O(V+VE)`（通常略去初始化写作 `O(VE)`），额外空间 `O(V)`；
闭包标记另需 `O(V+E)` 时间。上述界假定端口及权值操作为常数时间。
图须非空、源点合法，邻接、顶点映射和 `cost(edge)` 的值在调用期间保持稳定。
`W{}` 表示零，加法与比较须精确；**所有实际计算的有限候选值**都须可表示且严格小于
`infinity`，包括负环存在时的中间游走权值，而不只是最终最短路。
闭包版还要求两个哨兵互异并位于全部有限计算值之外。
不会对无穷哨兵做加法，也不自动饱和或选择更宽类型；大权值可显式使用 `__int128_t` 的 `W`。

`nlowlink(graph)` 位于 `graph_algo.hpp`，使用边身份端口，一次 Tarjan DFS 求全部割点与桥：

```cpp
auto cut = nlowlink(graph); // nlowlink_result
// cut.articulation[pos]：按稠密 position 存储的 0/1 割点标记。
// cut.bridges：vector<nidx_t>，每条桥的原图逻辑边 ID。
// 若使用 nmake_undirected_csr，input[id] 就是被选中的原始输入边。
```

输入须为稳定的无向多重图：每条非自环边在两个端点各出现一次，两项共享一个非负
`nidx_t` ID，不同逻辑边的 ID 不同；自环可记录一次或两次。ID 可以稀疏，
不按最大 ID 分配数组，也不要求边记录可复制；空图返回两个空数组。
割点/桥的定义是删除该点（及其关联边）/单条边后，整张图的连通分量数增加。
算法逐个启动尚未访问的连通分量；重边和自环不会误报为桥。

`dfn[v]` 是发现时间，`low[v]` 记录 DFS 子树通过树边和至多一条回边能到达的最早发现时间。
递归携带进入边的 ID；遍历时只跳过这个 ID，自环忽略，其余平行边仍降低 `low`。
对子节点 `v`，`low[v] > dfn[u]` 表示无法绕过树边 `(u,v)`，故该边是桥；
非根节点 `u` 若有子节点满足 `low[v] >= dfn[u]`，删去 `u` 就会分离该子树，故为割点。
DFS 根没有父侧，仅当 DFS 子节点数大于 1 时才是割点，不能用邻接度数代替。
桥按 DFS 回溯完成顺序输出，每条一次；顺序依赖顶点及邻接枚举，ID 不被重排或重编号。
端口操作为常数时间时，总时间 `O(V+E)`、额外空间 `O(V)`（含结果和递归调用栈）。
极深图需要调用者评估栈容量；外层邻接 range/迭代器须在递归访问其他顶点时仍有效。
有向图、不成对的非自环邻接、不同逻辑边复用 ID 等输入不满足契约，不自动补边或检查。
这是一次有意的接口迁移：旧的端点对 `bridges` 改为边 ID；无 ID 的 lowlink 输入须
显式补身份端口，或从“一条记录一条无向边”的输入构造 `nmake_undirected_csr`。

`nblockcut(graph)` 同样位于 `graph_algo.hpp`，接收与 `nlowlink` 相同的稳定无向多重图，
返回保留全部原顶点的点双圆方森林（不是删桥后缩点的边双森林）：

```cpp
auto blocks = nblockcut(graph); // nblockcut_result
// blocks.original_size == graph.vertices.len()
// [0, original_size)：原图顶点的稠密 position，圆点。
// [original_size, adjacency.size())：点双块，方点。
// adjacency[v]：圆点所属的方点编号；adjacency[b]：方点包含的原顶点 position。
// edges[b-original_size]：方点 b 内每条非自环原边的 ID，各出现一次。
auto forest = ngraph{nidx_t(blocks.adjacency.size()),
    [&](nidx_t v) -> const auto& { return blocks.adjacency[v]; }};
```

关联边双向存储，每个块与其不同成员各连一次。桥形成二成员块；平行边也可能形成
二成员块，因此不能仅凭块大小判桥；该块的边列表若只有一条边，则它是桥。
自环明确从森林和 `edges` 列表中排除，不另建块；忽略自环后孤立的原顶点
获得一个单点块，其边列表为空。空图返回 `original_size == 0`、空邻接及空边列表。
每个原连通分量对应森林中的一棵树；原顶点 `v` 是割点当且仅当
`blocks.adjacency[v].size() > 1`。
块编号、成员和边列表顺序依赖 DFS 枚举，不承诺规范顺序。结果只拥有整数 position、
原边 ID 和关联数组，不借用原图、不复制原始 edge record；解释原顶点 key 时仍使用
原来的顶点映射。
示例中的 `forest` 则借用 `blocks`，该对象须在使用期间存活，且不能被移动。

一次 DFS 维护顶点栈与边 ID 栈：树边下降时入边栈，回边仅在从后代指向祖先的一侧
入栈，避免同一条无向边压入两次；父边按 ID 跳过，平行的其他边仍入栈。
子节点 `v` 返回 `u` 后，若
`low[v] >= dfn[u]`，弹出栈顶直到并包括 `v`，这些顶点与 `u` 组成一个块。
同时弹出边栈直到并包括树边 `(u,v)` 的 ID，得到这个块的原边列表。
`u` 本身不弹出，因为它还可能属于别的块；即使 `u` 是 DFS 根，也使用同一个出块条件。
根回溯结束时清理自身栈项，没有 DFS 子节点的根补单点块。
两种入口共享私有 lowlink 内核；`nlowlink` 不为顶点栈、边栈或块邻接分配存储，
`nblockcut` 不先求割点和桥。每条非自环边恰好属于一个块；需要按 ID 反查块时，
可以扫描 `edges` 一次建立外部映射，稀疏 ID 不需要被压缩进算法内部。

端口操作为常数时间时，构造时间 `O(V+E)`，额外空间 `O(V+E)`，包括边栈、结果
及递归栈，且不依赖最大 ID。圆方森林本身仍只有 `O(V)` 大小。
若原图有 `c` 个连通分量、输出有 `b` 个块，则森林有 `V+b` 个节点、`V+b-c` 条无向边，
上述约定下 `b <= V`；输出总节点数须能用 `nidx_t` 表示。深图仍需评估递归栈容量，
外层邻接 range/迭代器在嵌套调用期间须保持有效。

对于互异的原顶点 `u,v,x`，且 `u,v` 原本连通，删除 `x` 后两点不连通，当且仅当
圆点 `x` 位于森林的 `u—v` 唯一路径上。因此可把森林交给已有 rooted/HLD 工具；
非连通输入应为每棵树提供一个根，跨组件不能调用 LCA/path。动态维护及查询层
不包含在此接口内。

`nscc(forward,reverse)` 要求两张图表示相同的 key 集合，但 vertex 枚举顺序可以不同。
第二遍以正图的 key 访问反图邻接，并以正图 inverse 解释反图 edge target；结果按正图
position 存储。V3 不替用户偷偷构造反图，因为反图
的存储策略本来就是自由度的一部分。

SCC 两遍使用递归 lambda DFS，辅助调用栈最坏 `O(V)`。BFS 的队列、Dijkstra 的堆
仍是算法所需结构。SCC 组件标签按第二遍发现顺序给出，不承诺旧版本的具体标签数值。
SCC、nroot、nreroot 及 HLD 构造会嵌套访问邻接/children，外层 range 和迭代器必须在
内层调用期间有效；返回独立临时容器可以，返回每次调用都会覆写的共享 scratch 不可以。

`ndinic<C>` 的容量类型要支持零值、比较、加减与 `min`。DFS 是递归实现，极深层次图需要
由调用者评估栈深度。

## 11. 静态 rooted projection 与 HLD

### 11.1 `nroot`

头文件：`src-v3/rooted.hpp`。`graph.hpp` 不再间接提供 rooted/function 接口。

`nroot(graph,roots)` 对给定根按邻接枚举顺序做递归 DFS first-discovery，返回 `nrooted`：

```text
parents() subtree_sizes() depths() components() positions()
order() roots() children(vertex)
```

这些语义入口大量使用 `nfunc`，从而保留“语义 key 与内部 position 不相等”的自由。
`parent[root]==root`。没有被 roots 覆盖的点保留 unseen metadata。
这些点 parent/depth/component 为 `-1`、subtree 为 `0`，不出现在 order 中；将内部
position 翻译成 key 的 parents/components 只允许查询覆盖点。

`nroot` 可以接受有向图甚至有环图；它只生成遍历森林，不声称原图本身是树。需要树语义
的算法必须由调用者保证输入是森林。
DFS 在返回时累计 subtree，调用栈为 `O(height)`；有环图的发现父边与旧手工栈版本
可能不同，返回值始终是当前 DFS 的遍历森林。

### 11.2 `nhld`

头文件：`src-v3/tree.hpp`

核心构造端口是：

```cpp
auto layout = nhld(vertices, roots, children);
// 连通无向树的整数入口：
auto dense = nhld(n, root, [&](nidx_t v) -> auto& { return adjacency[v]; });
```

其中 `vertices` 自身必须提供 inverse；核心入口没有独立 `index` 参数。

`nhld(rooted)` 直接复用已经计算的 parent/depth/subtree 数值构造布局；左值入口复制这些
数组，顶点 descriptor 仍借用 rooted。`nhld(move(rooted))` 转移这些数组与原顶点
descriptor，结果不再借用 rooted 本身，所以 `nhld(nroot(graph, roots))` 也有效；原
descriptor 借用的外部 owner 仍须存活。两种入口保留未覆盖顶点的原始 metadata。

```text
parents() depths() subtree_sizes() heads() positions()
order() lca(a,b) path(a,b) visit_path(a,b,visitor)
```

`path(a,b)` 返回按 a 到 b 的遍历顺序排列的 `npath_piece{left,right,reverse}`。reverse 为真
表示该 HLD 基区间要从右向左读取。这一位不能在字符串拼接、矩阵乘法等非交换路径聚合中
丢掉。

`visit_path(a,b,visitor)` 以相同顺序直接发出这些 piece，使用有界局部缓冲，不进行堆分配。
visitor 只消费结果，不得修改或销毁正在遍历的布局。`path` 是在该访问内核上收集 vector
的便利入口。

HLD 需要 roots/children 描述 rooted forest，或传每条边双向出现的无向森林；根不重复且每个覆盖组件只有一个根。`lca/path`
的两个顶点必须在同一组件。构造 `O(n)`，LCA 和分段数 `O(log n)`。
直接构造用迭代发现、逆序累计子树、重链展开，工作内存 `O(n)`，调用深度为常数。
`nhld(nroot(...))` 中的 `nroot` 仍需递归栈；深树优先使用直接构造。`len()` 是全 key 域大小，`order().len()` 只计覆盖点。未覆盖点
position/head 为 `-1`，parent/depth/subtree 为 `-1/-1/0`；heads/parents/lca/path
等要求合法树位置的操作只接受覆盖点。空根集的 order 为空，不会填入伪顶点。

### 11.3 `nreroot`

```cpp
auto answer = nreroot(graph, base, lift, merge);
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
nfloor_sum            带符号参数的类欧几里得 floor 求和，返回 __int128_t
nisqrt                整个 uint64_t 域上的精确向下整数平方根
nquotient_blocks      单商/双商同时恒定的半开区间枚举
npow                 泛型快速幂
next_gcd             扩展 gcd，返回 {gcd,x,y}
ninv_mod             可选模逆
ncrt                 两同余合并，不相容返回 nullopt
nmodint<MOD>         静态模整数
nmod_norm/add/sub/mul/neg  运行时模数的规范化与基本运算
ncomb<Mint>          阶乘/逆阶乘、排列数和组合数
nchoose_small<Mint>  大 n 小 k 的乘法组合数，不建到 n 的表
ninverse_batch       O(n) 乘法、一次除法求一组可逆元素的逆
nsieve               线性筛、最小质因子、范围内分解与 phi
```

`ndiv_floor(a,b)` / `ndiv_ceil(a,b)` 要求 `b != 0`，返回数学意义上的
`floor(a / b)` / `ceil(a / b)`，同时处理负数除数和无符号整数。模板参数 `I` 是同一个
内建整数类型；带符号最小值除以 `-1` 不在契约内，因为普通 C++ 除法本身无法表示该商。

模数宽度保持可检查：运行时 `nmod_*` 的模数与规范剩余为正 `long long`；`nmodint<MOD>`
使用 `auto` 非类型模板参数，`MOD` 要求为正整数且可表示为 signed `long long`，因此不会把
模数强制压成 `int`。`ninv_mod/ncrt` 的模数与结果仍为 `long long`。源值可以比模数更宽：
`nmod_norm`、`nmodint` 构造、`ninv_mod` 和 `ncrt` 都先在源类型中取余，再窄化规范剩余，
因此可直接接收 `__int128_t/__uint128_t`。源类型只需使 `value % modulus` 的结果能够转成
`long long`，不依赖 `std::integral`。

`nmodint::pow` 的非负指数同样保留调用者类型，只要求支持按位判奇和右移；直接流输入按
带可选正负号的十进制 token 逐位取模，不要求整段数字先放进 `long long`。

运行时 `nmod_add/sub/mul/neg` 的规范内核接收 `[0,modulus)` 内的 `long long` 剩余；传入
`int/long long/__int128_t/__uint128_t` 等源值时，同名重载会先分别调用 `nmod_norm`，再进入
该 `long long` 内核。加法使用无溢出的标准无符号算术；乘法使用 GCC/Clang 的
`__int128_t` 宽乘法保护中间结果，因此规范模运算保持 `O(1)`。使用 `deploy` 时选择
compiler-profile 可移植性策略；调用底层二元内核时，调用者仍须保证操作数已在
`[0,modulus)` 内。

`ncrt` 要求两个模数为正且最终 lcm 放进 `long long`。`nmodint::inv()` 和除法要求逆元
存在；实现不会把不可逆除法改写成别的运算。`ncomb` 要求阶乘中用到的每个分母可逆。
`next_gcd(a,b)` 返回非负 gcd 及满足 `a*x+b*y=gcd` 的系数。内部使用 128 位余数和系数，
支持 `LLONG_MIN` 输入及最终未使用系数的宽更新；返回的 gcd 必须放进 `long long`，
所以 gcd 等于 `2^63` 的输入组合不在契约内。`next_gcd(0,0)` 返回 `{0,1,0}`。

`nfloor_sum(n,m,a,b)` 返回 `sum_{i=0}^{n-1} floor((a*i+b)/m)`，参数为 `long long`，
要求 `n>=0,m>0`，允许 `a,b` 为负数乃至 `LLONG_MIN`。先做数学向下整除归一化，
再做 Euclid 递降，复杂度 `O(1+log m)`；返回值及中间和必须放进 signed `__int128_t`。
`nisqrt(uint64_t)` 只做整数运算，32 步二分，支持 `UINT64_MAX`；完全平方判定可直接比较
`r*r==x`，其中 `r=nisqrt(x)`，这个乘积不会溢出 `uint64_t`。

```cpp
__int128_t sum = nfloor_sum(1000000000LL, 97, -13, 42);
nquotient_blocks(100LL, [&](long long left, long long right, long long quotient) {
    // [left,right) 内每个 i 都满足 100/i == quotient。
});
nquotient_blocks(100LL, 80LL,
    [&](long long left, long long right, long long qa, long long qb) {
        // 同时保持 100/i == qa、80/i == qb。
    });
```

单商枚举覆盖 `[1,n+1)`，要求 `0<=n<LLONG_MAX`，回调数 `O(sqrt(n))`。
双商枚举覆盖 `[1,min(a,b)+1)`，要求非负参数且 `min(a,b)<LLONG_MAX`，
回调数 `O(sqrt(a)+sqrt(b))`；不包含较小参数范围外的零商尾段。空范围不调用回调。
两者均不分配内存；回调的累加类型与溢出上界由使用者决定。

`nsieve(n)` 的 `n>=0` 且 `n+1` 可表示为 `nidx_t`，构建时间和存储为 `O(n)`。
`prime/factor/phi` 查询不得越过筛范围，`factor/phi` 要求正数；`factor(1)` 为空，
`phi(1)=1`。单次分解和 phi 为 `O(log x)`，后者不分配临时数组。
`phi_table()` 和 `mu_table()` 各做一次 `O(n)` SPF 递推，独立返回 `vector<nidx_t>` 和
`vector<int8_t>`；位置 0 为占位零，存在位置 1 时为 1。默认筛不保存这些额外表，
每次调用都重新构建；多次查询应由调用者保存结果。前缀和通常使用 `long long` 或更宽类型。

`nchoose_small<M>(n,k)` 保留同一整数类型的 `n/k`，支持 128 位大 `n`；`n>=0`，
非法 `k` 返回零。时间 `O(min(k,n-k))`，一次求逆，仍要求这些分母都可逆。
`ncomb<M>::lucas(n,k)` 要求 `M::mod()` 为素数且阶乘表完整覆盖 `[0,mod)`，
`n,k` 非负，逐位查表 `O(1+log_mod(n))`。只适合能建完整表的小素数，不是任意模组合数。
`ninverse_batch(values)` 接受 STL、span 或 positional descriptor；元素乘法交换且全部可逆，
返回独立 vector，`O(n)` 时间/空间，仅一次 `M(1)/product`，空输入不求逆。
乘积等中间量必须在系数类型中保持可表示、可逆；它不是防止浮点溢出/下溢的稳定化算法。

### 13.2 64 位素数与分解

头文件：`src-v3/number.hpp`

```text
naddmod64 / nmulmod64 / npowmod64   无溢出模加、128 位保护的模乘与模幂
nisprime                对整个 uint64_t 确定性的 Miller-Rabin
npollard                Brent 风格 Pollard-Rho，输入为合数
nfactor                 升序返回带重数的质因子
```

`nfactor` 要求输入至少为 1，期望复杂度依赖质因子形状；它不是小范围筛法的替代品。
`naddmod64` 要求两个操作数已位于 `[0,modulus)`；`nmulmod64` 接受任意 `uint64_t`
操作数。这两个函数都要求模数非零，不能把加法误当作带内部归一化的入口。

#### `divisor.hpp`：约数与整除变换

```text
nfactor_powers(sorted_factors)   带重数的升序质因子 -> (prime,exponent)
ndivisors(prime_powers)          从不同质数及正指数枚举全部约数，无序
ndivisor_zeta / ndivisor_mobius  约数求和及其逆
nmultiple_zeta / nmultiple_mobius 倍数求和及其逆
```

`nfactor_powers` 保留质数值类型，指数为 `nidx_t`，线性扫描；`ndivisors` 接受它的结果或
`nsieve::factor` 的结果，时间/空间 `O(tau(n))`。所表示的整数必须放进质数值类型，约数数量
必须放进 `nidx_t`；质数不能重复，指数必须为正。空分解代表 1，约数为 `{1}`。不隐式排序。

四种整除变换原地作用于 `values[1..n]`，位置 0 原样保留，空输入也可。
`ndivisor_zeta` 得到 `out[x]=sum_{d|x} in[d]`；`nmultiple_zeta` 得到
`out[x]=sum_{x|m,m<=n} in[m]`。对应 Möbius 操作是各自的逆。
要求可独立写入、不互相别名的位置以及交换加法群，数值运算不能溢出，
不要求乘法或除法。当前采用显式调和枚举，`O(n log n)` 时间、`O(1)` 额外空间，
不是宣称使用按素数推进的 `O(n log log n)` 实现。

例如正整数输入的频次表 `freq`，求 gcd 恰好为每个值的无序下标对数量：

```cpp
vector<long long> exact = freq; // freq[0]=0，计数和乘积必须可表示。
nmultiple_zeta(exact);         // exact[d] = 被 d 整除的输入数量。
for (nidx_t d = 1; d < nlen(exact); ++d)
    exact[d] = exact[d] * (exact[d] - 1) / 2;
nmultiple_mobius(exact);       // exact[g] = gcd 恰好为 g 的对数。
```

### 13.3 `poly.hpp`

```text
nntt<MOD,ROOT>
nconvolution<MOD,ROOT>
npoly_derivative
npoly_integral
npoly_inverse<MOD,ROOT>
nlagrange_consecutive(values,point,combinations)
```

`nntt` 的 MOD 必须为素数，长度必须是非零二次幂并整除 `MOD-1`，ROOT 必须是原根。卷积在很小规模时自动
使用朴素算法，否则使用 NTT。FPS inverse 要求常数项可逆。

`npoly_integral` 的每个整数分母必须可逆，保留零积分常数。精确系数复用批量逆元，
使用 `O(n)` 乘法和一次除法，要求中间乘积可表示。内建 `float/double/long double` 改用
逐项除法，避免先形成阶乘造成无必要的溢出/下溢；这一路线为 `O(n)` 次除法。
两条路线都不强制系数拥有 `.inv()` 成员。
`nlagrange_consecutive` 用已知的 `f(0)..f(n-1)` 计算 `f(point)`，要求 `n>=1`、
多项式次数 `<n`、这些点在系数域内互异，传入的 `ncomb` 至少预处理到 `n-1`。
单次查询 `O(n)` 时间/空间、无额外求逆；采样点直接返回对应值。

```cpp
using mint = nmodint<998244353>;
ncomb<mint> combinations(3);
vector<mint> samples{0, 1, 8, 27}; // f(x)=x^3。
mint value = nlagrange_consecutive(samples, mint(1000000000LL), combinations);
```

### 13.4 矩阵与线性代数

头文件：`src-v3/linear.hpp`

`nmatrix<T>` 是紧凑 row-major owner，提供 `(row,column)` 和 `operator[](row)` 返回的
行 span；const 矩阵返回 `span<const T>`。消元等按值接收的工作矩阵仍会复制。当前线性代数面向
精确域：零比较必须可靠，每个非零 pivot 可除。浮点 eps 不会被默认藏入实现。

```text
nmatmul / nmatpow       矩阵乘法与非负整数幂
nrref                    Gauss-Jordan；可限制允许成为 pivot 的列
ndeterminant             方阵行列式，空矩阵为 1
ninverse                 可逆时返回逆矩阵，否则 nullopt
nlinear_solve            particular + nullspace basis，或 inconsistent
```

`nmatpow` 的非负指数保留调用者类型，只要求支持按位判奇和右移，因此可以直接使用
`__int128_t` 指数。朴素乘法 `O(rmk)`；RREF、行列式和求逆为标准三次复杂度。维度相容、
方阵条件和右端长度属于调用者契约。

#### 位压缩 GF(2) 消元与方程组

```text
ngf2_rref(packed, columns, pivot_columns=-1)
ngf2_solve(coefficients, variables, right)
ngf2_solution { consistent, rank, particular, basis }
```

复用 `nmatrix<uint64_t>` 保存每行的机器字，不额外引入 bit-matrix owner。
**`matrix.columns` 是机器字数量，参数 `columns/variables` 才是逻辑位数。**
逻辑位 `j` 位于 `(row,j/64)` 的第 `j%64` 位，最低位优先。输入必须恰好有
`ceil(columns/64)` 字/行；未使用的尾部 padding 可以非零，实现会清除。

`ngf2_rref` 按值接收工作矩阵，可 `move` 避免复制。返回 `nrref_result<uint64_t>`，
其中 `matrix` 仍位压缩，`pivot` 存逻辑列号。默认全部逻辑列可成为主元；显式限制时要求
`0<=pivot_columns<=columns`，主元之外的增广列也参与整行异或。
`m` 行、`n` 逻辑列、秩 `r` 时，为 `O(m*n + m*r*ceil(n/64))` 次字操作，
存储含工作矩阵为 `O(m*ceil(n/64)+r)`。不能把查找主元的成本也误称为缩小 64 倍。

`ngf2_solve` 的系数矩阵有 `ceil(variables/64)` 字/行，`right` 提供相同的行数个
0/1 值，可为 vector、span 或 positional descriptor；原始输入不修改。
要求 `variables>=0` 且 `variables+1` 放进 `nidx_t`，内部右端列不能成为主元。
返回的 `rank` 始终是系数矩阵的秩。无解时 `consistent=false`、特解与基为空。
有解时特解是 `vector<uint64_t>`，`basis` 每行是一个位压缩的零空间方向，
所有 padding 为零；解集为特解异或任意方向子集，共 `2^(variables-rank)` 个解。
不直接用机器整数计算这个数量，避免满秩/高维时移位或计数溢出。

求解在消元外还需 `O(variables*rank)` 位提取和
`O((variables-rank)*ceil(variables/64))` 输出初始化；输出完整零空间不是免费的。
支持零方程、零变量；例如零方程时零空间基为所有单位向量，而不是空基。

```cpp
// x0 XOR x1 = 1; x1 XOR x2 = 0。
nmatrix<uint64_t> a(2, 1); // 3 个变量，每行只需 1 字。
a(0, 0) = 0b011;
a(1, 0) = 0b110;
vector<unsigned char> b{1, 0};
auto solution = ngf2_solve(a, 3, b);
// rank=2; particular={0b001}; basis 的唯一一行为 {0b111}。
```

### 13.5 `bitmath.hpp`：线性基与位运算变换

`nxor_basis<U=uint64_t>` 存储无符号整数的 GF(2) 线性空间，完整支持最高位。
`insert(x)` 返回是否增秩；`contains(x)` 判断可表示；`rank()` 返回秩；
`maximize(seed=0)` 最大化 `seed XOR v`；`merge(other)` 合并空间。
插入/查询 `O(W)`，合并 `O(W^2)`，固定存储 `O(W)`，`W` 为 U 的位数。

`ordered()` 以低主元到高主元返回约化后的基，代价 `O(W^2)`；
用 `k` 的二进制位选中这些向量并异或，就得到升序第 `k` 个不同值。
`kth(k)` 是零基序号，包含零，超出范围返回 `nullopt`，每次调用重新约化，代价 `O(W^2)`。
大量第 k 小查询应保存一次 `ordered()` 的结果。满 64 位秩不计算 `1ULL<<64`。
线性基本身不保存插入次数或子集重数，不能把“零可表示”理解为“有非空零异或子集”。

```text
nsubset_zeta(values,inverse=false)    out[S] = sum_{T subset S} in[T]
nsuperset_zeta(values,inverse=false)  out[S] = sum_{T superset S} in[T]
nxor_transform(values,inverse=false) XOR Walsh-Hadamard
nbit_convolution<nbit_operation::bit_and/bit_or/bit_xor>(left,right)
```

变换要求非零二次幂长度，空数组为 no-op；位置独立且可变。原地 `O(n log n)` 时间，
`O(1)` 额外空间。subset/superset 的逆只要求减法，不要求除法。
XOR 的逆要求长度在系数环里可逆，不能直接对普通整数系数使用截断除法。
位卷积要求两个输入等长，返回独立 vector；`out[k]=sum_{i OP j=k} a[i]*b[j]`，
时间 `O(n log n)`，额外空间 `O(n)`。算子在模板参数选择，因此 AND/OR 不实例化 XOR
的除法要求，能接受无除法的交换环。XOR 需要 2 可逆（长度 1 不需要）。

### 13.6 `recurrence.hpp`：线性递推求项与 BM

统一系数顺序：长度 `k` 的 `coefficients` 表示
`a[t] = coefficients[0]*a[t-1] + ... + coefficients[k-1]*a[t-k]`，从 `t>=k` 起成立。
不是按最早初始项到最近项排列；BM 输出可直接交给求项函数。

`nlinear_recurrence(initial,coefficients,index)` 返回从 0 起算的第 `index` 项。
`initial` 至少提供前 `k` 项，多余条目忽略；`index` 为支持判奇/右移的非负整数，
包括 signed/unsigned 128 位。空系数表示恒零序列，不读取初值。长度要求 `2*k` 可表示为
`nidx_t`。数据源可以是 STL、span 或 positional descriptor，调用期间借用，不修改。

实现计算 `x^index` 对 `x^k-c[0]x^(k-1)-...-c[k-1]` 的余式，然后与初始项做点积。
多项式乘法与降次复用 `npow` 的自定义乘法入口；只用乘法、加法和 0/1，不要求逆元，
因此合数模也可用。复杂度 `O(k^2 log(index+1)+k)` 次系数运算、`O(k)` 空间。
不是矩阵的 `O(k^3 log index)`，也没有声称达到 NTT 加速递推的复杂度。

`nberlekamp_massey(values)` 返回与有限前缀相容的最短递推，复杂度 `O(n^2)` 次域运算、
`O(n)` 空间，`n+1` 必须放进 `nidx_t`。要求精确域：非零差异可除、零比较精确；不能在
一般合数模或浮点近似零比较下照搬。空/全零序列返回空系数，**末尾零系数不能删掉**：
前缀 `{1,0}` 的结果为 `{0}`，表示一阶递推，不是恒零序列。

BM 只保证 `k<=t<n` 内的递推成立，不提供未观测项的证明。若已独立证明整个序列满足
阶数不超过 `K`、且从相应阶数起成立的递推，前 `2*K` 项足够恢复一个可继续求项的递推。
只有题目数据前缀而没有此保证时，不得把外推结果当成确定答案。

```cpp
using mint = nmodint<998244353>;
vector<mint> initial{0, 1}, coefficients{1, 1};
mint f10 = nlinear_recurrence(initial, coefficients, 10); // 55
vector<mint> prefix{0, 1, 1, 2, 3, 5, 8, 13};
auto recovered = nberlekamp_massey(prefix);               // {1,1}
mint f100 = nlinear_recurrence(prefix, recovered, 100);   // Fibonacci 递推由题意保证。
```

### 13.7 `frac.hpp`：精确分数 `nfrac`

`nfrac<I = long long>` 是独立模块中的 `struct`，直接包含 `src-v3/frac.hpp`。
`I` 为至多 64 位的内建有符号整数，不随 `nidx_t` 切换。默认值是 `0/1`；构造时约分、
将负号移到分子，并始终保持分母为正。内部表示不允许直接改写，使用
`numerator()` / `denominator()` 读取，修改整个值使用赋值或复合运算。

```cpp
#include "src-v3/frac.hpp"

using Q = nfrac<>;
Q a(2, -6), b(5, 4);  // -1/3, 5/4
Q c = a + b;           // 11/12
c *= 6;                // 11/2；两侧都支持可表示的整数混合运算
bool less = a < b;     // 精确比较，不转浮点
cout << c << '\n';     // 11/2；流输出固定为 numerator/denominator
auto inverse = c.inv(); // 2/11
```

提供一元 `+/-`、四则运算及 `+=`、`-=`、`*=`、`/=`、完整比较、
`inv()`、流输出及显式 `static_cast<long double>(value)`。浮点转换只用于近似观察，
不参与内部运算。没有隐式浮点构造、跨 `nfrac<I>` 类型转换或分数字符串输入；
输入时分别读取两个整数再构造即可。整数构造接受至多 64 位的有符号/无符号源类型，
先约分再检查 `I` 范围，因此 `nfrac<int8_t>(1000,2000)` 可得到 `1/2`；
超过 64 位的源类型和 `nfrac<__int128_t>` 在编译期拒绝，不会先静默窄化。
混合整数运算会先将整数构造成同一 `nfrac<I>`，该整数本身必须可表示。

**溢出是明确失败边界，不是取消分数功能的理由。** 每个二元运算先在 signed 128 位中
完成精确计算、约分，再检查规范分子和正分母是否装得进 `I`。由于存储分母至多为
`LLONG_MAX`，两个交叉乘积的和/差仍能装进 signed 128 位；比较的交叉乘积也不会溢出。
所以不会因“未约分的乘积超过 64 位”而误报，`Q(LLONG_MAX,2)*Q(2,LLONG_MAX)` 返回 `1/1`。
减法不先对右操作数取负，除法不先形成其倒数，故 `Q(LLONG_MIN)-Q(LLONG_MIN)` 为零，
`Q(LLONG_MIN)/Q(LLONG_MIN)` 为一，尽管单独取负或取倒数不可表示。

- 规范结果装不下：抛出 `std::overflow_error`，错误消息以 `nfrac:` 开头。
- 构造零分母、除以零分数、零的倒数：抛出 `std::domain_error`。
- 抛异常时复合赋值保持左操作数原值，不依赖 debug assertion，优化构建行为相同。
- `Q(0,LLONG_MIN)` 规范为 `0/1`；`Q(1,LLONG_MIN)` 因正分母不可表示而报溢出。
- 每一次运算都必须可表示；不承诺跨表达式消去溢出，也不是任意精度有理数。

构造和四则运算做 `O(log M)` 次 Euclid 步骤，`M` 为宽中间整数的量级；比较为 `O(1)`
次宽整数操作，存储和辅助空间均为 `O(1)`。依赖 GCC/Clang 的 `__int128_t`。
可直接作为 `nmatrix/nlinear_solve/ninverse` 的精确系数，也可传给 `npow`；这些算法中的
**每个中间分数**同样必须可表示，否则传播异常，不自动退化到浮点。

测试：`python3 test-v3/run.py frac_property` 覆盖两种索引宽度与三种构建模式；
`python3 test-v3/frac_oracle.py` 额外用 Python 标准库 `fractions.Fraction` 的任意精度结果
核对全宽构造、四则运算、比较及溢出判定，不依赖 Boost。

### 13.8 数学增强施工规划（待实施项不是 API）

面向 ICPC 金牌线与 CF 2400 以下的常用工作流，新增 24 KiB 按下面顺序使用。
分批预算是规划额度，不强制填满；每批必须先通过独立暴力对拍，再进入已实现章节。

| 批次 | 内容 | 新增语义预算 | 状态 |
| --- | --- | --- | --- |
| A | SPF 的 phi/mu 整表、约数枚举、整除分块与变换、floor_sum、整数平方根、线性基、SOS/位卷积、插值与组合数补强 | 10 KiB | 已完成 |
| B1 | 位压缩 GF(2)、已知递推求项与 BM | 与 B2/B3 共用 10 KiB | 已完成并通过门禁 |
| B2/B3 | 浮点消元、整数/任意模卷积、运行时模整数 | 与 B1 共用 10 KiB | 待实施 |
| C | 边界修复、实测优化与按题型选择的 FPS 扩展 | 4 KiB | 预留 |

批次 A 的验证模型：逐数试除与约数扫描、逐项 floor 求和、枚举子集异或与掩码对、
Pascal 三角和直接多项式求值。危险边界包括 0/1、重复素因子、负参数、64 位最高位、
满秩线性基、模数非素数时的不可逆条件、空数组和半开区间端点。
批次 B 不把有限前缀的 BM 结果当作递推证明，也不把无系数界的三模 CRT 当作任意模卷积。

#### 2026-09-15：A 批审计与 B1 验证记录

本次审计范围为已实现的数学、数论、位运算、多项式与精确线性代数，
不是对图、树、字符串等全库运行时行为重新背书。发现并修复两处问题：

- **浮点积分回归**：A 批无条件将积分分母批量求逆，先形成阶乘会溢出，
  尽管每个 `1/(i+1)` 都能表示。新增 3000 项的 `float/double/long double` 固定回归，
  保留精确系数的批量算法，内建浮点恢复逐项除法。
- **extgcd 极值溢出**：原实现的余数/系数更新可能在最终答案可表示时溢出 `long long`，
  已用 UBSan 在包含 `LLONG_MIN` 的输入上复现。内部改为 signed 128 位，公开结果类型不变；
  新增极值笛卡尔积与全宽随机 Bézout 恒等式核对，明确排除 gcd 为 `2^63` 的不可表示结果。

B1 使用独立 oracle 而不只做往返测试：GF(2) 枚举全部 3×3 二元系数矩阵及右端，
比较全部解集；较大矩阵与逐字节消元核对，并覆盖 63/64/65、127/128/129 列及非零 padding。
BM 穷举长度不超过 9 的二元前缀，排除所有更低阶的二元递推；一般素数域用独立方程组
求解检验最短阶数。递推求项对照朴素生成与伴随矩阵，覆盖 128 位下标、合数模和无除法类型。

A 批完成时数学合计 `23390` 语义字节；本次两项修复与 B1 实现后合计 `27287`，
新增 `3897`。相对原始基线的扩展使用 `10803 / 24576` 字节，数学专用剩余 `13773`。
该批完成时总计 `141801 / 155648` 字节；后续图论扩展另增 4 KiB，当前预算以
`test-v3/measure.py` 扫描结果为准。

本次最终验证结果（不是只根据中途输出判断）：

- 12 组数学测试 × 2 种索引宽度 × 3 种构建模式，合计 72 次构建运行全部通过。
  覆盖 `arithmetic/divisor/bitmath/comb_poly/math/number/poly/mod/division/linear/gf2/recurrence_property`。
- 结构审计通过：36 个头文件 × 2 种宽度独立编译；扫描 67 个测试文件。
  67 是结构审计扫描数量，不表示本轮重跑了全库 67 组运行时测试。
- 完整 benchmark runner 通过；8 组数学 workload 在两种宽度下各运行三次，
  每组的六个 checksum 一致。GF(2) workload 的累计 checksum 为 `6255089516811532963`，
  递推 workload 的累计 checksum 为 `6255089517296092905`。
- 预算门禁与 `git diff --check` 通过。没有启动 B2/B3，也没有将修改自动提交。

## 14. 字符串

头文件：`src-v3/string.hpp`

字符串算法直接接受 `string`、`string_view`、vector 或位置 descriptor，不复制输入对象。
字符字面量请用 `string_view("aba")`：原始字符数组的 size 包含末尾 NUL。

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
vector<nidx_t> terminal;
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
nidx_t root = -1;
root = tree.add(root, {2, 7});
root = tree.add_segment(root, -10, 20, {-3, 5});
long long answer = tree.query(root, x);
```

同一 kernel 可持有多棵互不共享节点的普通整数根，更新是 destructive。整条函数插入与
单点查询为 `O(log(hi-lo))`，线段函数插入为 `O(log^2(hi-lo))`；空根返回构造时给出的 infinity。

## 16. 跨结构装配

### 16.1 CSR → rooted projection → HLD → 区间结构

```cpp
auto graph = nmake_csr(n, edges, from, to);
auto rooted = nroot(graph.view(), roots);
auto hld = nhld(rooted);

vector<long long> base(n);
for (nidx_t p = 0; p < n; ++p)
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
位置投影：nview、range/sub/reverse/project/map/gather/zip/product、结构 inverse
离散函数：nfunc、anchors/keys/values/entries/redomain/restrict/map_values/compose、hash fallback
离散算法：select/slice/stride/filter/collect/order/sort、assign/fill/copy/transform、序列折叠、区间键 chunks/blocks/windows/runs
节点内核：narena、隐式 FHQ、多根 destructive split/merge
区间结构：Fenwick、迭代/懒/稀疏/持久根线段树、聚合队列、Sparse Table、Wavelet Matrix
并查集：普通、带势能与 rollback
图端口：隐式/任意邻接 descriptor、CSR owner
图算法：BFS、Dijkstra、标准 Bellman–Ford、负环影响闭包 Bellman–Ford、拓扑、SCC、Tarjan 割点/桥、Kruskal、Dinic、Hopcroft-Karp
树：rooted projection、HLD/LCA、Euler Tour 动态森林
数学：快速幂、exgcd、CRT、静态模数、精确分数 nfrac、组合表、线性筛、NTT/FPS inverse
字符串：prefix/Z/KMP/Manacher/SA/LCP、AC 自动机内核
几何：精确二维基础、线段、凸包、面积、直线交点
```

### 17.2 当前明确不搬运

```text
checked/unsafe 双实现与 npre 恢复链
nmaybe、nvector、ndeque、nheap 等 STL 同义包装
覆盖整个 STL 的 nreplace/nremove 等同义改名层（`nrotate` 是单独提供的原地排列操作）
nenumerable 游标森林与循环宏
nresource_pool/nnode_domain/generation/epoch 的全局身份体系
nset/nmap/nbije/nrel 等大规模关联容器包装族
统一图 edge trait、统一 owner facade、自动反图和自动纠错
覆盖浮点、字符串、容器和格式 DSL 的大而全竞赛 I/O 包装
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

V3 不复用 V2 测试，历史归档也不参与 include、搜索或测试发现。

**2026-09-20 用户指令：以后严禁跑全量测试。** 只运行与本次改动直接相关的最小测试集。
禁止无测试名运行 `test-v3/run.py`，也禁止枚举所有测试名、分片、并行或分批重组全量测试。
“阶段闸”“收尾”及旧技能中的全量验证建议均不能作为例外。纯文档收尾不运行 C++ 测试。
相关检查通过后停止；只有新的修改、实际失败或尚未解决的具体问题才应触发补充验证。

整库头文件编译审计与完整 benchmark runner 也不作为默认替代检查。需要时仅编译受影响
的头文件、执行对应 benchmark；源码或预算变化使用轻量的 `measure.py`。下列为定向入口：

```bash
cd /home/tnuzy/NitoriSTL
python3 test-v3/run.py reftree_property
python3 test-v3/measure.py
```

示例中的测试名必须按实际改动选择。`run.py` 仅对选中的测试分别在默认 32 位与
`NITORI_INDEX_64` 两种位置模式下执行：

```text
debug + _GLIBCXX_ASSERTIONS
-O2 + NDEBUG
ASan + UBSan
```

即每个测试共经过 `2 × 3` 个构建运行，并启用
`-Wall -Wextra -Wpedantic -Wshadow -Werror`。`index_mode_contract` 另外固定覆盖
`10^18` 起点 re-domain、`5×10^9` 惰性 range、超过 `INT_MAX` 的笛卡尔积长度与 stride。
性质测试使用独立朴素模型、随机操作、
非交换操作、空结构、重复值、多组件、深链、星形、根共享和 destructive/persistent 混用
边界。测试通过只证明已经覆盖的契约内行为，不等于所有模板实例都天然正确。

`measure.py` 通过词法扫描排除注释和布局空白，同时保留字符串/raw string 内的内容，并有
自测试保护计量规则。总预算上限是 `171008` 语义字节；`discrete.hpp` 另有
`10240` 语义字节的局部闸门，防止它再长成包装森林。

数学扩展以 2026-09-15 的 `math/number/poly/linear` 合计 `16484` 字节为基线，
新增额度 `24576` 字节；`measure.py` 单独计量这些模块以及 `divisor/bitmath/recurrence/frac`。
图论扩展以 Bellman–Ford 加入前的 `graph/graph_algo/graph_store/flow` 合计 `9396` 字节
为基线，另增 `4096` 字节。数学部分上限 `41060`，图论原有额度为 `13492`。
2026-09-20 另获图论与 DS 共享的 `6144` 字节；DS 基线为加入 nreftree 前的 `62960`
字节，覆盖 `arena/list/ds/fhq/bag/vec_bag/segment/wavelet/dynamic_tree/link_cut/opt/hash/tree/rooted`，
新 `reftree` 归入同组。共享额度消耗为 `max(0,graph-13492)+max(0,DS-62960)`；
超出 `6144` 的部分转由通用额度承担，不通过另建头文件规避计量。
其余模块（包括 `permutation.hpp`）的基础通用上限由 `42232` 增至 `47352` 字节；
通用实际消耗再加上上述图／DS 超额。
总额度 `171008`；数学专款、图论原专款与图论/DS 共享款
分别计量，未花掉的专款不能用于无关模块。
这不是要求用满预算，也不把额度挪给无关包装层。

以下说明已有审计/benchmark 工具的覆盖范围，不是运行全量工具的指令。
`audit.py` 检查旧源码是否回流到活动路径、历史归档校验和是否被解包、V2 include
泄漏、会在 `NDEBUG` 消失的 assert 测试、concept/domain 压力回流、绕过 `nidx_t` 的结构
`int`、文档本地链接，以及每个头文件能否在两种位置模式下独立 include。

benchmark 的结构、hash 与 math workload 会分别构建 32/64 位位置模式；I/O workload 与位置宽度
无关，只构建一次。它是 deterministic workload，用于观察直接排序与投影排序、结构 order/runs、
静态 hash inverse 与 `unordered_map`、FHQ 节点大小、split/merge、`nbag` 与 `nvec_bag` 查询与删除重插、
vector 基线与 FHQ 后端的差异、线段树、Wavelet Matrix、
LCT 路径、直接邻接与 graph port、CSR 构造/BFS、rooted projection、稀疏节点数和峰值
RSS。数学 workload 覆盖百万 SPF/phi/mu 表、整除变换、floor_sum、满秩线性基、
`2^18` XOR 卷积、连续点插值、512 变量位压缩 GF(2) 方程组，以及从 512 项恢复
64 阶递推并求第 `10^18` 项，保留结果不变量检查和 deterministic checksum。
时间值受机器波动影响，checksum 与规模必须稳定；跨运行的单个毫秒值不能代替同一
workload 下的结构、内存和 checksum 对照。
统一入口还运行 `tree_protocol_bench.cpp`，覆盖 lazy 布局、FHQ edit 及 pooled 双向链根的
稳定 handle splice。直接 HLD/BFS 的 30 万点深链在 8 MiB 栈下测试；benchmark 中的
`nroot` 仍有 20 万层递归，需要充足栈，例如在独立 shell 中先执行 `ulimit -s 262144`。
ASan 同样计入栈成本，直接 HLD 的栈保证不延伸到 `nroot/nreroot` 或流算法。

`composition_cost_property` 额外检查排序后分块的逐块零分配、终结算法不复制持有计划的
descriptor、move-only 源的子块脱离、共享 accessor 状态、已有 hash inverse 的显式借用，
以及 HLD visitor 的零堆分配。benchmark 也包含完整的排序后分块遍历与连续 HLD 路径访问。

## 19. 扩展 V3 的自检流程

新增能力前：

1. 从题目需求写出最小操作集合和暴力基线。
2. 判断它是新的数学结构，还是现有 root/view/graph port 的一个策略。
3. 只模板化真实自由维度，不为“以后也许”增加 trait。
4. 把合法输入、代数定律、生命周期、失效条件和复杂度写在实现旁。
5. 先写固定反例，再写独立随机 oracle。
6. 只运行最小相关测试集，严禁全量测试；按需要执行对应 benchmark 与轻量 size audit，
   不在阶段闸或收尾时扩大到整库。
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
io:         nread nscan nwrite nprint nprintln
debug:      ndebug
            ndebug_writer ndebug_repr
sequence:   nargsort ngather nindexed_span nrun_bounds
permutation: nrotate npermutation_rank npermutation_unrank
mdview:     nmdview
view:       nview nall ntabulate nrange nsub nreverse nproject nmap ngather nzip nproduct nlocate
hash:       nhash nhash_inverse nmake_hash_inverse ninvert
func:       nfunc nkeys nvalues nentries nredomain nmap_values ncompose nanchors
discrete:   nselect nslice nstride nfilter nunique nindexed ncollect nprefix nsuffix
            npositions nassign nfill ncopy ntransform naccumulate neach nfind_if ncontains ncount_if
            nall_of nany_of nnone_of
            nargmin nargmax nlower nupper nargsort norder nsort nreverse_inplace
            nchunks nblock nblocks nwindows nruns
memory:     narena
reftree:    nreftree nreftree_noop
list:       nlist
fhq:        nfhq nfhq_noop
segment:    nsegment_trace nsegment_cover nadd nmin nmax nseg nlazyseg nlazy_ops naddsum_action
            nlazy_addsum nsparse_seg
bag:        nbag nvec_bag
ds:         nsum_group nfenwick ndsu npotential_dsu nrollback_dsu nqueue_agg ndeque_agg
            nsparse_table ntopk ntopk_merge ntopk_by_merge nwavelet
graph:      nvertices ngraph nto_self nbfs nbfs_many
rooted:     nrooted nroot
graph store: ncsr nmake_csr ncsr_edge nmake_undirected_csr
graph alg:  n01bfs ndijkstra ntoposort nscc nscc_result
            nlowlink nlowlink_result nblockcut nblockcut_result
            nbellman_ford nbellman_ford_closure ndenseprim ndenseprim_result
            neuler neuler_kind neuler_result
tree:       npath_piece nhld_layout nhld nreroot nett_forest nlct
flow:       ndinic nmatching nhopcroft_karp nmst_result nkruskal
math:       ndiv_floor ndiv_ceil nfloor_sum nisqrt nquotient_blocks
            npow negcd_result next_gcd ninv_mod ncrt nmod_norm nmod_add nmod_sub nmod_mul nmod_neg
            nmodint ncomb nchoose_small ninverse_batch nsieve
frac:       nfrac
number:     naddmod64 nmulmod64 npowmod64 nisprime nsplitmix64 npollard nfactor
divisor:    nfactor_powers ndivisors ndivisor_zeta ndivisor_mobius nmultiple_zeta nmultiple_mobius
bitmath:    nxor_basis nsubset_zeta nsuperset_zeta nxor_transform nbit_operation nbit_convolution
poly:       nntt nconvolution npoly_derivative npoly_integral npoly_inverse nlagrange_consecutive
recurrence: nlinear_recurrence nberlekamp_massey
linear:     nmatrix nmatmul nmatpow nrref_result nrref ndeterminant ninverse
            nlinear_solution nlinear_solve ngf2_rref ngf2_solution ngf2_solve
string:     nprefix_function nz nkmp npalindrome_radii nmanacher nsuffix_array nlcp
automata:   nlowercase nac
geom:       npoint ndot ncross non_segment nsegment_intersect npolygon_area2
            nconvex_hull nline_intersection
opt:        nline nline_eval nlichao
```

这个索引只帮助搜索。真正的前提以 `src-v3/*.hpp` 中紧邻实现的注释和本文对应章节为准。
