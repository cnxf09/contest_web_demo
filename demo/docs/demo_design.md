# 最小匿名凭证 Demo 设计

本文给出一个“先跑通 issuance -> prove -> verify 命令行流程”的最小设计。
目标不是一次做成产品，而是用最少新增代码，把仓库里已经存在的匿名凭证实验能力包装成一个清晰、可复用、可演示的 CLI demo。

## 设计目标

1. 尽量复用现有模块
2. 新增代码最少
3. 第一阶段只跑通命令行流程：`issue -> prove -> verify`
4. 不先引入 HTTP、数据库、复杂配置或新的密码学协议

## 结论先行

最合适的第一版 demo 目录放在：

- `lib/examples/anoncred/`

原因：

- 语义上它是 demo / example，不再继续塞进 `tests/`
- 仍然处于 `lib/` 主构建树里，接入 `CMake` 最简单
- 可以直接复用 `lib/circuits/tests/anoncred/` 里的 `small` 凭证格式和样例，不需要先抽大重构

第一版 demo 的关键策略是：

- `issuer` 不做真正的在线签发服务，而是把 `small_examples.h` 中的样例凭证物化成一个 issued bundle
- `holder` 读取 issued bundle，构造 witness，生成 proof
- `verifier` 读取 proof 和 request，重建相同电路与公共输入，执行验证
- challenge / transcript 先采用最小实现：由 verifier CLI 生成一个简单 request 文件，holder 和 verifier 都使用它

这样可以保留角色边界，同时避免第一版就引入新的签名发行逻辑。

## 建议目录结构

```text
lib/examples/anoncred/
  CMakeLists.txt
  README.md

  shared/
    types.h
    files.h
    files.cc
    small_demo.h
    small_demo.cc
    request.h
    request.cc

  issuer/
    main.cc
    issue.h
    issue.cc

  holder/
    main.cc
    prove.h
    prove.cc

  verifier/
    main.cc
    verify.h
    verify.cc
```

## 每个目录的职责

### `shared/`

这是第一版里最重要的新代码位置。

职责：

- 定义 issuer / holder / verifier 之间共享的数据结构
- 负责最小文件读写
- 负责把现有 `small_test.cc` 里的核心逻辑抽成可复用函数
- 避免三端都重复拷贝 `make_circuit()`、`fill_witness()`、proof 序列化逻辑

### `issuer/`

职责：

- 生成一个最小 issued credential bundle
- MVP 阶段不做真正随机签发，而是从 `small_examples.h` 读取样例数据并写到磁盘
- 输出 holder 后续 proving 所需的全部材料

### `holder/`

职责：

- 读取 issued credential bundle 和 verifier request
- 调用共享层构建电路、填 witness、生成 proof
- 输出 presentation bundle

### `verifier/`

职责：

- 生成 request/challenge 文件
- 读取 request + presentation
- 重建相同 circuit / public inputs
- 调用 `ZkVerifier` 完成验证，并输出结果

## 共享类型设计

第一版只定义最小类型，不做通用 credential 框架。

### `shared/types.h`

建议包含以下结构：

```cpp
struct IssuedCredential {
  std::vector<uint8_t> credential_bytes;
  std::string issuer_pkx_hex;
  std::string issuer_pky_hex;
  std::string issuer_sig_r_hex;
  std::string issuer_sig_s_hex;

  // MVP 为了最小改动，先直接复用样例里的设备绑定签名。
  std::string device_sig_r_hex;
  std::string device_sig_s_hex;

  std::string default_now_yyyymmdd;
  uint32_t example_id;
};

struct PresentationRequest {
  std::string claim_name;
  std::vector<uint8_t> transcript_bytes;
  std::string now_yyyymmdd;
};

struct PresentationProof {
  std::vector<uint8_t> proof_bytes;
  std::string claim_name;
  std::vector<uint8_t> disclosed_value;
};

struct VerificationResult {
  bool ok;
  std::string message;
  std::string claim_name;
  std::vector<uint8_t> disclosed_value;
};
```

### 为什么这样定义

- `IssuedCredential` 只包含当前 `small_witness.h` 真正需要的字段
- `PresentationRequest` 把 transcript/challenge 和 claim 绑定起来
- `PresentationProof` 只保留 verifier 真正需要的最小信息
- 不先引入 JSON schema、CBOR schema 或复杂多 claim 结构

## 建议的文件格式

为了最小代码量，第一版不要引入 JSON 解析器。

建议使用“目录 + 简单文件”的方式交换数据：

```text
run/demo/
  issued/
    credential.bin
    issuer_pkx.txt
    issuer_pky.txt
    issuer_sig_r.txt
    issuer_sig_s.txt
    device_sig_r.txt
    device_sig_s.txt
    now.txt
    example_id.txt

  request/
    claim.txt
    transcript.bin
    now.txt

  presentation/
    proof.bin
    claim.txt
    disclosed_value.bin
```

这样做的原因：

- 读写代码最少
- 不需要额外三方库
- 容易 `cat` / `xxd` / `diff`
- 失败时最容易调试

## 最小复用策略

## 1. 直接复用的现有模块

### 凭证和 witness

- `lib/circuits/tests/anoncred/small.h`
- `lib/circuits/tests/anoncred/small_witness.h`
- `lib/circuits/tests/anoncred/small_examples.h`
- `lib/circuits/tests/anoncred/small_io.h`

用途：

- 凭证布局
- opened attribute 语义
- witness 生成
- 内置样例 issuance 数据

### 证明与验证

- `lib/zk/zk_prover.h`
- `lib/zk/zk_verifier.h`
- `lib/zk/zk_proof.h`
- `lib/random/transcript.h`
- `lib/proto/circuit.h`（可选）

用途：

- 证明构造
- 证明验证
- proof 序列化
- transcript / challenge

### 辅助基础设施

- `lib/circuits/tests/anoncred/CMakeLists.txt` 中已有的依赖关系可作为参考
- `lib/util/crypto.h` 中已有 hex 工具可复用
- `lib/algebra/static_string.h` / `Nat(StaticString)` 可复用现有 hex 输入风格

## 2. 明确不复用的部分

第一版不建议依赖：

- `reference/verifier-service/server/*`
- `lib/circuits/mdoc/mdoc_zk.cc`

原因：

- 它们偏 `mdoc` 产品化集成层，不是匿名凭证 demo 的最短路径
- 把 `small` demo 绑到 Go service 或 mdoc C API 上，只会增加包袱

## 共享适配层设计

`shared/small_demo.{h,cc}` 应该承担第一版里最核心的复用抽象。

建议提供下面这些函数：

```cpp
std::unique_ptr<Circuit<Fp256Base>> BuildSmallDemoCircuit();

bool MaterializeIssuedCredentialFromExample(uint32_t example_id,
                                            IssuedCredential* out);

bool BuildPresentationRequest(const std::string& claim_name,
                              const std::string& now_yyyymmdd,
                              const std::vector<uint8_t>& transcript,
                              PresentationRequest* out);

bool ProveSmallDemo(const IssuedCredential& issued,
                    const PresentationRequest& request,
                    PresentationProof* out);

VerificationResult VerifySmallDemo(const IssuedCredential& issued,
                                   const PresentationRequest& request,
                                   const PresentationProof& presentation);
```

## 为什么要有这一层

如果没有 `small_demo.cc`，以下逻辑会在三端重复：

- `make_circuit()`
- witness 填充
- claim 到 `SmallOpenedAttribute` 的映射
- `ZkProof::write/read`
- transcript 初始化方式

这个共享适配层是“最少新增代码”和“避免重复复制粘贴”之间的最佳平衡点。

## claim 设计

第一版不要做通用 claim DSL。

只支持一个内建 claim：

- `age_over_18`

映射方式：

- `shared/request.cc` 把字符串 `age_over_18` 映射成 `SmallOpenedAttribute`
- 先固定为 `small_test.cc` 已经使用过的字段位置和值

原因：

- 这是最短路径
- 先证明 demo 结构可用，比先支持多 claim 更重要
- 后续再扩到 `birthdate` / `age_over_21` / 多属性披露都不晚

## transcript / challenge 设计

第一版保留 verifier 角色，但不追求复杂协议。

### MVP 方式

- verifier CLI 生成一个 request 目录
- request 中包含：
  - `claim.txt`
  - `transcript.bin`
  - `now.txt`
- holder prove 时读取 request
- verifier verify 时读取同一个 request

### transcript 来源

第一版可以这样做：

- 默认使用 `small_examples.h` 里已有 transcript
- verifier `request` 子命令也可以支持 `--use-example-transcript`
- 后续再升级为真实随机 transcript 生成和设备签名绑定

### 为什么这样设计

- 仓库当前 `small` witness 直接消费 transcript 字节和设备绑定签名
- 如果第一版就引入真实设备私钥签名，会新增明显更多代码
- 先把 request 文件边界做出来，更利于后续平滑升级

## issuance 设计

### MVP issuance 的真实含义

第一版 `issuer issue` 不做真正在线签发，而是：

- 从 `small_examples.h` 选择一个样例
- 将其物化为 `IssuedCredential` 目录

也就是说，第一版 issuance 更准确地说是：

- `sample credential materialization`

而不是：

- `fresh cryptographic issuance`

### 为什么接受这个取舍

- 这是当前仓库最少代码的路径
- 已经能完整演示角色边界：issuer -> holder -> verifier
- 后续如果需要真实签发，可再引入 OpenSSL ECDSA signing 逻辑
- 仓库里已经有 `verify_external_test.cc` 可作为未来生成签名的参考

## 三个 CLI 的建议接口

## `issuer`

建议命令：

```bash
./anoncred_issuer issue --example 0 --out run/demo/issued
```

行为：

- 读取 `small_examples.h` 中第 `0` 个样例
- 写出 issued credential bundle
- 输出摘要信息，例如：
  - credential size
  - issuer pk
  - default claim set

## `holder`

建议命令：

```bash
./anoncred_holder prove \
  --issued run/demo/issued \
  --request run/demo/request \
  --out run/demo/presentation
```

行为：

- 读取 issued credential bundle
- 读取 request
- 构造 circuit 和 witness
- 调用 `ZkProver`
- 输出 proof bytes 和 disclosed value

## `verifier`

建议提供两个子命令。

### 生成 request

```bash
./anoncred_verifier request \
  --claim age_over_18 \
  --use-example-transcript 0 \
  --out run/demo/request
```

### 验证 presentation

```bash
./anoncred_verifier verify \
  --issued run/demo/issued \
  --request run/demo/request \
  --presentation run/demo/presentation
```

行为：

- `request`：生成 request 目录
- `verify`：读取 request + presentation，重建公共输入并调用 `ZkVerifier`

## 为什么 verifier 需要 `issued`

第一版 verifier 读取 `issued` 目录，是为了最少新增代码。

原因：

- verifier 需要 issuer 公钥和 credential 相关公共材料来重建公共输入
- 如果一开始就把这些字段复制进 `presentation`，会先引入新的 envelope 设计
- 第一版先让 verifier 直接读取 `issued` bundle，代码更短

后续如果要把 verifier 做成更真实的外部方，可以再把必要公共数据从 `issued` 提炼到 `presentation` 或 `request` 中。

## 推荐命令行流程

第一版推荐的最小演示流程是 4 条命令：

```bash
./build/examples/anoncred/issuer/anoncred_issuer issue \
  --example 0 \
  --out run/demo/issued

./build/examples/anoncred/verifier/anoncred_verifier request \
  --claim age_over_18 \
  --use-example-transcript 0 \
  --out run/demo/request

./build/examples/anoncred/holder/anoncred_holder prove \
  --issued run/demo/issued \
  --request run/demo/request \
  --out run/demo/presentation

./build/examples/anoncred/verifier/anoncred_verifier verify \
  --issued run/demo/issued \
  --request run/demo/request \
  --presentation run/demo/presentation
```

如果一定要压缩成“看起来只有 3 阶段”的流程，可以把 `request` 生成视为 `verify` 角色的准备步骤。

## 构建设计

建议在 `lib/CMakeLists.txt` 中新增：

```cmake
add_subdirectory(examples/anoncred)
```

`lib/examples/anoncred/CMakeLists.txt` 建议定义三个可执行文件：

- `anoncred_issuer`
- `anoncred_holder`
- `anoncred_verifier`

### 链接策略

优先采用和 `small_test` 一致的依赖思路：

- 直接链接 `mdoc`
- 再补 `ec`、`algebra`、`util` 等必要库
- 不依赖 `gtest` / `benchmark`

## 为什么不直接改 `small_test.cc`

不建议把 `small_test.cc` 直接扩成 CLI。

原因：

- 它是测试文件，混合了 benchmark、gtest 和 demo 逻辑
- 继续在这个文件上叠 CLI，会让边界更乱
- 更好的做法是把里面的核心逻辑提到 `shared/small_demo.cc`
- `small_test.cc` 后续可以继续只做测试

## 为什么不先做真实签发

真实签发意味着至少要新增：

- issuer 私钥载入/生成
- 对 credential bytes 做 ECDSA 签名
- 设备绑定签名的密钥管理
- request 级 transcript 重新签名

这会立刻把第一版 demo 从“薄包装”变成“新增一套签名工作流”。

对当前目标来说，这不是最短路径。

第一版应该先接受下面这个现实：

- issuance 使用样例 fixture 物化
- prove/verify 使用真实 ZK 流程

这样最符合“先跑通，再迭代”的要求。

## 第二阶段可升级点

第一版跑通之后，再考虑这些增强：

1. `issuer issue` 改成真实签发，而不是物化 sample
2. `verifier request` 改成真实随机 transcript
3. `holder prove` 改成对 request 做实时设备签名，而不是复用样例签名
4. `claim` 从单内建值扩展到多属性
5. 文件格式从目录文件升级为 JSON/CBOR envelope
6. 再视需要增加 HTTP 层

## 最终判断

如果目标是“在现有仓库上用最少新增代码做一个最小匿名凭证 demo”，最合适的设计是：

- 新建 `lib/examples/anoncred/`
- 下面分成 `issuer/`、`holder/`、`verifier/`、`shared/`
- 复用 `lib/circuits/tests/anoncred/` 的 `small` 凭证与 witness
- 用 `shared/small_demo.cc` 吸收 `small_test.cc` 中的核心逻辑
- 第一版接受 sample-based issuance，只把 prove/verify 路径做成真正的 CLI

这条路径新增代码最少，角色边界清晰，也最容易在下一步真正实现成功。
