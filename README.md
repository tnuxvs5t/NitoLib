# Nitori v3.2

Nitori v3 是面向算法竞赛的 C++23 泛型库重建工程。当前改革目标是：

```text
结构化复用改革 + 自由度革命
```

当前清理方向是直接使用 STL 容器与 span，公开接口保持全局 `n*` 名字。普通算法不要求
先包装 `nall`；语义投影需要时再引入 `nview/nfunc`。规则多维数据用独立的 `nmdview`
组合切片、换轴、反向和 bias，不复制元素、不建 hash。

本版新增 pooled doubly-linked chains 内核 `nlist`；它以共享节点池、整数 handle 和轻量链根
支持多链组合，并非 `std::list` 替代。`nruns` 的自定义 Operation 接受候选段
`[left,right)` 边界，自行捕获原 view 并维护增量摘要。直接 HLD 构造已使用迭代遍历，
支持 `nhld(n,root,next)`；`nroot/nreroot` 等仍按各自契约使用递归。

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
#include "src-v3/io.hpp"
#include "src-v3/segment.hpp"
```

`io.hpp` 提供直接复用现有 `istream/ostream` 缓冲区的十进制泛整数 I/O，支持
`__int128_t/__uint128_t`，并可与普通 `cin/cout` 操作交叉使用；它不扩张为浮点、容器或格式
DSL 包装层。

```bash
g++ -std=c++23 -O2 -I/path/to/NitoriSTL solution.cpp
```

`nview` 可选提供 `inverse(key)->position`：range/sub/reverse/product 等结构组合直接传播
代数 inverse，任意离散 key 可由 `hash.hpp` 的 `ninvert` 一次性附加紧凑静态 hash
fallback。`nanchors(keys,values)` 优先复用结构 inverse，否则自动建表，图与树算法因此
不再携带平行的 index 参数。

`discrete.hpp` 用位置计划统一装配 `nview/nfunc`，并提供结构排序、值排序、序列折叠与
区间键 chunks；`nassign/nfill/ncopy/ntransform` 让重排、切片或函数 value 投影也能直接
作为写入目标。它不引入 holder、locator protocol 或 concept/trait 登记层。

## 施工原则

- 162 KiB（128 + 24 + 4 + 6 KiB）语义源码预算；24 KiB 专用于数学与多项式，4 KiB 专用于图论，另有 6 KiB 供图论与 DS 共享，注释和布局空白不计入。
- 有符号 `nidx_t` 位置，半开区间 `[left,right)`；默认 `int`，全程序定义
  `NITORI_INDEX_64` 时切换为 `long long`。
- 不建立 concept/trait/npre 森林。
- 算法依赖最小端口，不依赖默认 owner。
- 一个 kernel 可以承载多个普通整数根；merge/split 的共享与 destructive 契约显式书写。
- 不用统一身份 token 混淆 vertex、Euler position、segment node 等不同语义。
- 新结构必须有固定反例、独立随机 oracle 和 sanitizer 证据。

## 验证

**严禁跑全量测试。** 只选择与本次改动直接相关的最小测试集；不得枚举全部测试名，
或通过分片、并行、分批执行变相跑全量。阶段收尾也不例外。整库头文件编译审计和完整
benchmark runner 不作为默认替代检查；只检查受影响的头文件及确有必要的 benchmark。
纯文档修改不运行 C++ 测试，相关验证通过后停止，不为形式上的完整性重复运行。

```bash
python3 test-v3/run.py reftree_property
python3 test-v3/measure.py
```

上面的测试名是定向验证示例，应按实际改动选择；`measure.py` 用于源码或预算变化。
选中的测试会分别编译运行 debug、`-O2` 和 ASan+UBSan 模式，并启用严格 warnings。

## V2 状态

V2/X、v2-c 与 v2-nano 已从活动工作树移除，以确定性压缩包保存。完整性、
来源和恢复边界见 [`archive/README.md`](./archive/README.md)。它只用于用户明确要求的历史
考古，普通 V3 开发、调试和迁移不得搜索或解包该归档。

## License

当前仓库尚未单独声明许可证。比赛之外分发前请先确认后续许可证条款。
