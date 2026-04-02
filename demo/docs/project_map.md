# 项目地图

## 1. 顶层目录作用

- `.devcontainer/`
  - 开发容器配置，用于快速拉起可编译、可测试环境。
- `.github/`
  - CI / workflow 配置。
- `docs/`
  - 文档站源码与静态产物。
  - `docs/content/` 是 Hugo 文档内容；`docs/hugo.yaml` 是站点配置；`docs/static/reference/cpp/` 是已生成的 C++ API 文档；`docs/specs/` 是规格草案与辅助代码。
- `lib/`
  - 主体 C++ 代码库，也是主构建入口。
  - 包含通用 ZK 证明栈、密码学基础模块、各类 credential / circuit 实现、测试与实验代码。
- `reference/`
  - 参考集成实现。
  - 当前最重要的是 `reference/verifier-service/`，它提供一个 Go 写的 ZK mdoc verifier service。

根目录下的主要文件：

- `README.md`
  - 项目总览、依赖、手工构建方式；明确写了主构建命令是 `cmake -S lib -B ...`。
- `Doxyfile`
  - C++ API 文档生成配置。
- `main.pdf`
  - 仓库外部论文/说明材料的导出文件，不是构建入口。
- `LICENSE`
  - 许可证。

## 2. 与零知识证明、凭证、声明、验证相关的核心文件

### 2.1 通用 ZK 证明内核

- `lib/zk/zk_prover.h`
  - 顶层 ZK prover；把 sumcheck transcript + Ligero commitment/proof 组合起来。
- `lib/zk/zk_verifier.h`
  - 顶层 ZK verifier；重放 Fiat-Shamir transcript，生成约束并验证 commitment proof。
- `lib/zk/zk_proof.h`
  - ZK proof 数据结构与序列化格式。
- `lib/zk/zk_common.h`
  - prover / verifier 共享的约束生成与公共逻辑。
- `lib/sumcheck/prover.h`
  - sumcheck prover 封装。
- `lib/sumcheck/verifier.h`
  - sumcheck verifier 封装。
- `lib/sumcheck/prover_layers.h`
  - 按层生成 sumcheck 证明。
- `lib/sumcheck/verifier_layers.h`
  - 按层验证并维护 claims。
- `lib/ligero/ligero_prover.h`
  - Ligero commitment/proof 生成。
- `lib/ligero/ligero_verifier.h`
  - Ligero proof 验证。
- `lib/merkle/merkle_commitment.h`
  - commitment 的 Merkle 封装。
- `lib/proto/circuit.h`
  - 电路序列化/反序列化；也是 circuit bytes 的核心格式入口。

### 2.2 凭证/声明/验证的生产主线：mdoc

这一支是当前最“产品化”的路径。

- `lib/circuits/mdoc/mdoc_zk.h`
  - 对外 C 接口；定义 `run_mdoc_prover`、`run_mdoc_verifier`、`generate_circuit`、`find_zk_spec`。
- `lib/circuits/mdoc/mdoc_zk.cc`
  - mdoc prover / verifier 主实现；把 witness 构造、MAC、公共输入填充、ZK proof 调用串起来。
- `lib/circuits/mdoc/mdoc_witness.h`
  - 解析 device response / issuerSigned / deviceSigned，提取属性、签名、digest、随机盐等 witness。
- `lib/circuits/mdoc/mdoc_hash.h`
  - mdoc 属性、有效期、digest、选择性披露相关 hash 电路/约束。
- `lib/circuits/mdoc/mdoc_signature.h`
  - mdoc 签名验证相关电路。
- `lib/circuits/mdoc/mdoc_generate_circuit.cc`
  - 生成压缩后的 circuit bytes。
- `lib/circuits/mdoc/mdoc_circuit_id.cc`
  - 计算/校验 circuit id。
- `lib/circuits/mdoc/zk_spec.cc`
  - 硬编码支持的 ZK spec 版本与 circuit hash 列表。
- `lib/circuits/mdoc/circuits/`
  - 已生成并签入仓库的 mdoc 电路 bundle；reference verifier service 会直接加载这里的文件。

### 2.3 凭证相关的底层子电路

- `lib/circuits/ecdsa/verify_circuit.h`
  - ECDSA 验签电路。
- `lib/circuits/ecdsa/verify_witness.h`
  - ECDSA witness 构造。
- `lib/circuits/sha/flatsha256_circuit.h`
  - SHA-256 电路。
- `lib/circuits/sha/flatsha256_witness.h`
  - SHA witness 构造。
- `lib/circuits/cbor_parser/*.h`
  - CBOR 解析相关电路与 witness 结构，服务于 mdoc/CBOR 类凭证。
- `lib/circuits/logic/routing.h`
  - 在字节流中按索引抽取字段，适合“打开某个属性”的声明场景。
- `lib/circuits/logic/memcmp.h`
  - 字节序比较，用于有效期等约束。
- `lib/circuits/logic/bit_plucker.h`
  - bit/word 打包拆包，SHA/签名/字段转换常用。
- `lib/circuits/compiler/compiler.h`
  - 把逻辑电路编译成底层 quad circuit。

### 2.4 参考验证服务

- `reference/verifier-service/server/main.go`
  - HTTP 服务入口，加载 circuits、CA 证书、VICAL，并暴露 `/zkverify`、`/specs`。
- `reference/verifier-service/server/zk/proofs.go`
  - Go 通过 cgo 调 `mdoc_zk.h` 的核心桥接层。
- `reference/verifier-service/server/zk/circuits.go`
  - 读取/缓存 circuit 文件。
- `reference/verifier-service/server/zk/roots.go`
  - issuer root CA 处理。
- `reference/verifier-service/server/zk/vical.go`
  - VICAL 相关处理。
- `reference/verifier-service/server/handler.go`
  - HTTP 请求解析与返回。

## 3. demo / test / example 在哪里

### 3.1 匿名凭证 / 实验 demo

- `lib/circuits/tests/anoncred/`
  - 仓库里最接近“匿名凭证 demo”的位置。
- 关键文件：
  - `small_test.cc`
    - 可运行测试 + benchmark；名字里虽然是 test，但实际上已经把“构建电路 -> 填 witness -> 跑 ZK prover”整条链路串起来了。
  - `small.h`
    - 一个简化 credential 格式的电路定义。
  - `ptrcred.h`
    - pointer-based credential 格式实验。
  - `small_witness.h`
    - `small` 凭证 witness 构造。
  - `small_examples.h`
    - 内置样例数据。
  - `README.md`
    - 明确写了这是 sample credential format，用来展示 ZK-optimized credential format 的性能特征。

### 3.2 mdoc 相关测试

- `lib/circuits/mdoc/mdoc_zk_test.cc`
- `lib/circuits/mdoc/mdoc_signature_test.cc`
- `lib/circuits/mdoc/zk_spec_test.cc`（当前在 CMake 里注释掉）
- `lib/circuits/tests/mdoc/`
  - 额外的实验性 mdoc 测试，例如 revocation、`mdoc_1f`。

### 3.3 其他通用测试

- `lib/circuits/tests/base64/`
- `lib/circuits/tests/jwt/`
- `lib/circuits/tests/sha3/`
- `lib/circuits/tests/ripemd/`
- `lib/*/*_test.cc`
  - `ecdsa`、`logic`、`merkle`、`sumcheck`、`zk` 等模块都有单测。
- `reference/verifier-service/server/zk/*_test.go`
  - Go verifier service 的单元测试。

### 3.4 参考请求样例

- `reference/verifier-service/server/examples/post1.json`
- `reference/verifier-service/server/examples/post2.json`
- `reference/verifier-service/server/examples/post3.json`
  - 这是现成的 verifier service 请求样例。

### 3.5 文档中的教程

- `docs/content/en/docs/tutorials/`
  - 是文档教程，不是源码 demo。

## 4. 构建入口在哪里

### 4.1 主 C++ 构建入口

- 主入口：`lib/CMakeLists.txt`
- 仓库根 `README.md` 给出的标准命令也是：

```bash
CXX=clang++ cmake -D CMAKE_BUILD_TYPE=Release -S lib -B clang-build-release --install-prefix ${PWD}/install
cd clang-build-release && make -j 16 && ctest -j 16
```

结论：这个仓库不是从根目录 `CMakeLists.txt` 开始构建，而是明确从 `lib/` 作为 source dir 开始。

### 4.2 mdoc 可安装库入口

- `lib/circuits/mdoc/CMakeLists.txt`
  - 定义了 `mdoc` / `mdoc_static`。
  - 对外安装 `mdoc_zk.h` 和静态库，供 reference verifier service 链接。

### 4.3 文档站构建入口

- `docs/package.json`
  - Node/Hugo scripts 入口，如 `npm run build`、`npm run serve`。
- `docs/hugo.yaml`
  - Hugo 站点配置。
- `docs/go.mod`
  - Hugo module 依赖（Docsy theme）。

### 4.4 参考 verifier service 构建入口

- `reference/verifier-service/server/main.go`
  - Go server 程序入口。
- `reference/verifier-service/Dockerfile`
  - 容器化构建入口；先编 C++ 库，再编 server。
- `reference/verifier-service/server/README.md`
  - 本地构建说明：先 `cmake -S lib ... make install`，再在 `server/` 下 `go build`。

## 5. 我推测“匿名凭证 demo”最可能复用哪些模块

如果你要在这个仓库里做一个“匿名凭证 demo”，我认为最可能复用的是下面这条链路。

### 5.1 直接复用的 demo/实验层

- `lib/circuits/tests/anoncred/small.h`
- `lib/circuits/tests/anoncred/small_witness.h`
- `lib/circuits/tests/anoncred/small_examples.h`
- `lib/circuits/tests/anoncred/small_test.cc`
- 可选：`lib/circuits/tests/anoncred/ptrcred.h`

原因：这是仓库里唯一明确以“sample credential format”形式出现、并且已经把 credential 电路、witness、样例数据、benchmark 串起来的目录。

### 5.2 一定会复用的底层密码学/电路模块

- `lib/circuits/ecdsa/verify_circuit.h`
- `lib/circuits/ecdsa/verify_witness.h`
- `lib/circuits/sha/flatsha256_circuit.h`
- `lib/circuits/sha/flatsha256_witness.h`
- `lib/circuits/logic/routing.h`
- `lib/circuits/logic/memcmp.h`
- `lib/circuits/logic/bit_plucker.h`
- `lib/circuits/compiler/*`
- `lib/arrays/dense.h`

原因：`small.h` / `ptrcred.h` 已经直接包含并使用这些模块来完成签名验证、消息哈希、字段选择、时间比较和电路编译。

### 5.3 一定会复用的证明系统主栈

- `lib/zk/*`
- `lib/sumcheck/*`
- `lib/ligero/*`
- `lib/proto/circuit.h`
- `lib/merkle/*`

原因：无论是 `small_test.cc` 还是 `mdoc_zk.cc`，最终都要落到同一套 Longfellow proof stack。

### 5.4 可能部分复用的 mdoc 模块

- `lib/circuits/mdoc/mdoc_witness.h`
- `lib/circuits/mdoc/CMakeLists.txt` 中的 `mdoc` 静态库

原因：

- `anoncred/small_witness.h` 已经直接包含了 `circuits/mdoc/mdoc_witness.h`。
- `anoncred/CMakeLists.txt` 里 `small_test` 也直接链接了 `mdoc` 库。
- 这说明匿名凭证实验并不是完全独立的一套实现，而是在复用部分 mdoc witness / 工具链能力。

### 5.5 不太像会直接复用的部分

- `reference/verifier-service/`

原因：它明显是面向 mdoc HTTP verifier 的参考服务，不是匿名凭证 demo 本身。不过如果你后面想把 demo 包一层 HTTP API，这里会是最自然的服务端骨架。

## 总结

这个仓库的结构可以粗略理解为：

- `lib/zk + lib/sumcheck + lib/ligero`：通用证明引擎
- `lib/circuits/*`：面向 credential/签名/hash/CBOR 的具体电路
- `lib/circuits/mdoc/*`：当前最完整、最接近生产的 mdoc ZK 接口
- `lib/circuits/tests/anoncred/*`：最像“匿名凭证 demo”的实验目录
- `reference/verifier-service/*`：把 mdoc verifier 暴露成 HTTP 服务的参考集成
