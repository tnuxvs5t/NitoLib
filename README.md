# Nitori v3

Nitori v3 是面向算法竞赛的 C++23 泛型库重建工程。当前改革目标是：

```text
结构化复用改革 + 自由度革命
```

V3 不复用 V2 的实现、测试、checked/unsafe 双体系或单头文件组织。代码从 `src-v3/`
重新生长，模板只要求实际使用的表达式，数学、生命周期和失效限制写在局部注释中。

## 当前入口

- 综合教程与公共契约：[`v3-Tutorial-Comprehensive.md`](./v3-Tutorial-Comprehensive.md)
- 语义源码：[`src-v3/`](./src-v3/)
- 独立测试：[`test-v3/`](./test-v3/)
- deterministic benchmark：[`bench-v3/`](./bench-v3/)
- 已验证装配：[`examples-v3/`](./examples-v3/)

V3 暂时没有统一 `Nitori.h`。直接包含需要的模块：

```cpp
#include "src-v3/view.hpp"
#include "src-v3/discrete.hpp"
#include "src-v3/segment.hpp"
```

```bash
g++ -std=c++23 -O2 -I/path/to/NitoriSTL solution.cpp
```

`discrete.hpp` 用位置计划统一装配 `nview/nfunc`，并提供结构排序、值排序、序列折叠与
区间键 chunks；它不引入 holder、locator 或 concept/trait 登记层。

## 施工原则

- 128 KiB 语义源码预算；注释和布局空白不计入。
- 有符号 `int` 位置，半开区间 `[left,right)`。
- 不建立 concept/trait/npre 森林。
- 算法依赖最小端口，不依赖默认 owner。
- 一个 kernel 可以承载多个普通整数根；merge/split 的共享与 destructive 契约显式书写。
- 不用统一身份 token 混淆 vertex、Euler position、segment node 等不同语义。
- 新结构必须有固定反例、独立随机 oracle 和 sanitizer 证据。

## 验证

```bash
python3 test-v3/run.py
python3 test-v3/run.py composition graph_store_property
python3 test-v3/audit.py
python3 bench-v3/run.py
python3 test-v3/measure.py
```

测试会分别编译运行 debug、`-O2` 和 ASan+UBSan 模式，并启用严格 warnings。

## V2 状态

V2/X、v2-c 与 v2-nano 已从活动工作树移除，以确定性压缩包保存。完整性、
来源和恢复边界见 [`archive/README.md`](./archive/README.md)。它只用于用户明确要求的历史
考古，普通 V3 开发、调试和迁移不得搜索或解包该归档。

## License

当前仓库尚未单独声明许可证。比赛之外分发前请先确认后续许可证条款。
