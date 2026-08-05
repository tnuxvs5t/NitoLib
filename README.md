# NitoLib

> Nitori X 的公开仓库：用纯泛型与 `struct` 思维，挑战 STL 和现代 C++20/23 的死板边界。

NitoLib 是面向个人工程架构与算法竞赛的 GNU C++20 单头文件库。它不追求把 STL 的
每个名字机械地改成 `n`，而是尝试把**能力、所有权、代数结构和算法**拆开，让同一套
短代码可以作用于拥有型容器、真实引用 view、投影序列、图结构和数学对象。

仓库名是 **NitoLib**；库的实现与 API 仍以 **Nitori X** 为准，公共 checked 头文件是
[`Nitori.h`](./Nitori.h)，完整语义文档是 [`NITORI_DOCUMENT.md`](./NITORI_DOCUMENT.md)。

## 设计宣言

- **挑战死板，而不是重复造轮子**：重新检查 STL 与现代 C++20/23 惯用法背后的假设，
  让抽象服务于算法和不变量。
- **纯泛型 + `struct` 优先**：用真实的值类型、能力约束、代数操作和小型数据结构装配
  组件，避免为了抽象而抽象的类层级与运行时多态。
- **零成本抽象**：view、projection、泛型算法和竞赛组件以编译期组合为目标，不引入
  不必要的运行时分派；关键成本由测试与 benchmark 验证，而不是口号保证。
- **短，但不黑箱**：默认使用有符号 `int` 下标、半开区间 `[l,r)`，显式处理生命周期、
  溢出、不可达状态、代数定律和复杂度。
- **checked 与 unsafe 分离**：checked 版本优先暴露错误前提；unsafe 版本只在合法输入
  和前提已经验证后，用于优化器假设。

## 适用场景

1. 个人项目中的轻量泛型组件与数据结构装配；
2. 算法竞赛中的单文件、低样板、可检查提交；
3. 研究 `Range / View / Projection`、代数数据结构与零成本泛型接口；
4. 用固定测试、属性测试、对拍和 benchmark 验证抽象是否真的值得。

NitoLib 不是 STL 的 drop-in replacement，也不是企业级框架。它更像一间持续试验的
河童工坊：接口会围绕语义、不变量和竞赛实战继续收紧。

## 最小示例

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

## 核心方向

```text
contract/base → algebra → borrowed reference topology → owning storage
→ generic mechanisms → data structures/domain algorithms → I/O
```

当前实现覆盖的方向包括：

- `nvector`、`ndeque`、`narray` 与统一的序列算法；
- `nview`、切片、步长、projection、离散函数与 `ncollect`；
- Fenwick、线段树、lazy、持久化、wavelet、并查集、堆与离线算法；
- 图、最短路、树、流、匹配与相关图算法；
- 整数、模运算、组合、线性代数、多项式、博弈、字符串和几何；
- checked / unsafe 双 profile，以及独立的固定、death、属性和 benchmark 工具链。

算法依赖最小能力，不依赖某个具体容器后端；拥有者负责存储，view 只映射位置到真实
引用。非交换 fold 与 lazy action 的顺序也属于公开语义，不能靠“看起来一样”替代证明。

## 使用

### 直接包含

```bash
g++ -std=gnu++20 -O2 -Wall -Wextra \
    -I/path/to/NitoLib solution.cpp
```

训练与调试默认使用：

```cpp
#include "Nitori.h"
```

只有在 checked 版本、边界测试和相关前提都已经通过后，才考虑使用：

```cpp
#include "Nitori_unsafe.h"
```

### 生成与验证

`Nitori.h` 和 `Nitori_unsafe.h` 都由同一组 `src/*.hpp` 按
[`src/manifest.txt`](./src/manifest.txt) 生成；不要手工编辑生成头文件。

```bash
python3 tools/amalgamate.py
python3 tools/amalgamate.py --check
python3 tools/test.py                 # 全部测试
python3 tools/audit.py                # 公共符号与权威性审查
```

开发时可以先运行单个测试：

```bash
python3 tools/test.py zero_cost
python3 tools/test.py property_seq
```

完整 API 契约、前置条件、复杂度、迁移规则和维护流程请以
[`NITORI_DOCUMENT.md`](./NITORI_DOCUMENT.md) 为准。

## 仓库结构

```text
Nitori.h             checked 生成头文件（公共实现）
Nitori_unsafe.h       unsafe 生成头文件
src/                  按 manifest 排序的语义模块
test/                 固定、death、属性与兼容性测试
bench/                维护中的性能基准
tools/                amalgamation、测试与审计脚本
NITORI_DOCUMENT.md    唯一权威用户文档
```

Abel 是独立项目，不属于 NitoLib；本仓库不会收录 Abel 源码、配置或构建产物。

## 状态与边界

这是个人工程与竞赛库，优先级是：

```text
语义明确 > 可验证 > 可组合 > 低常数 > 样板少
```

API 与实现仍可能演进。提交问题或改动时，请同时说明：输入边界、所有权 / 生命周期、
代数定律、复杂度和可复现测试；不要只报告“能不能像 STL 一样写”。

## License

当前仓库尚未单独声明许可证。若要在个人项目或比赛之外分发，请先确认仓库后续公布的
许可证条款。
