# 匿名凭证 Demo Gap Analysis

本文只回答一个问题：基于仓库当前状态，如果目标是做一个“匿名凭证 demo”，哪些能力已经具备，哪些还缺，以及最短实现路径是什么。

## 结论先行

结论很明确：

- 这个仓库已经具备匿名凭证 demo 所需的大部分底层能力，尤其是 `prover`、`verifier`、`Fiat-Shamir transcript/challenge`、`proof/circuit serialization` 和最小 example harness。
- 当前真正缺的不是证明系统，而是“把这些能力收束成一个独立、稳定、可复用的匿名凭证 demo 边界”。
- 如果只追求最短可演示路径，最应该复用的是 `lib/circuits/tests/anoncred/` 这一支，而不是先改造 `mdoc` 生产链路或 Go verifier service。

## 1. 已有能力

| 能力 | 当前状态 | 证据文件 | 判断 |
|---|---|---|---|
| 凭证表示 | 已具备 ，但分为两层 | `lib/circuits/tests/anoncred/small.h`、`lib/circuits/tests/anoncred/small_examples.h`、`lib/circuits/mdoc/mdoc_witness.h` | 有实验性的 `small` 凭证格式，也有生产级 `mdoc` 表示与解析能力 |
| claim / selective disclosure | 已具备 | `lib/circuits/tests/anoncred/small.h`、`lib/circuits/mdoc/mdoc_zk.h`、`lib/circuits/mdoc/mdoc_witness.h` | `small` 里有 `OpenedAttribute`；`mdoc` 里有 `RequestedAttribute` 和按属性披露的 witness 生成 |
| prover | 已具备 | `lib/zk/zk_prover.h`、`lib/circuits/mdoc/mdoc_zk.h`、`lib/circuits/tests/anoncred/small_test.cc` | 通用 `ZkProver` 已成型，`mdoc` 和 `small` 都能实际跑 proof |
| verifier | 已具备 | `lib/zk/zk_verifier.h`、`lib/circuits/mdoc/mdoc_zk.h`、`reference/verifier-service/server/zk/proofs.go`、`lib/circuits/tests/anoncred/small_test.cc` | 通用 verifier 成型，`mdoc` 有产品化 verifier，`small` 在测试里已经完成验证闭环 |
| transcript / challenge | 已具备 | `lib/random/transcript.h`、`lib/sumcheck/transcript_sumcheck.h`、`lib/zk/zk_common.h` | Fiat-Shamir transcript、sumcheck transcript 和 challenge 派生都已经存在 |
| serialization | 已具备底层能力 | `lib/zk/zk_proof.h`、`lib/proto/circuit.h`、`lib/circuits/mdoc/mdoc_generate_circuit.cc` | proof 可 `write/read`，circuit 可序列化/反序列化，`mdoc` 还有已生成 bundle |
| example harness | 已具备最小闭环 | `lib/circuits/tests/anoncred/small_test.cc`、`lib/zk/zk_testing.h`、`lib/circuits/mdoc/mdoc_zk_test.cc`、`reference/verifier-service/server/examples/post1.json` | 已有最小 C++ harness，也有 `mdoc` 端到端测试和服务端样例 |

### 1.1 凭证表示：已经有两套可复用起点

#### A. 实验型匿名凭证表示：`small`

- `lib/circuits/tests/anoncred/small.h`
  - 定义了一个固定布局的“小凭证”格式：姓名、生日、性别、`age_over_X`、有效期、设备公钥等字段。
- `lib/circuits/tests/anoncred/small_examples.h`
  - 给出内置样例凭证字节串、issuer 公钥、issuer/device 签名、session transcript。
- `lib/circuits/tests/anoncred/small_witness.h`
  - 负责把这些字节串转成签名 witness 和 SHA witness。

判断：
- 这已经足够支撑 demo。
- 对匿名凭证 demo 来说，`small` 比 `mdoc` 更适合作为第一版表示，因为结构简单、阅读和调试成本低。

#### B. 生产型凭证表示：`mdoc`

- `lib/circuits/mdoc/mdoc_witness.h`
  - `ParsedMdoc::parse_device_response(...)` 已经可以从 `DeviceResponse` 中抽取 issuerSigned、deviceSigned、attributes、digest、random。

判断：
- 如果目标是“和真实钱包 / mdoc 对接”，仓库已经有成熟入口。
- 但如果目标只是“做匿名凭证 demo”，直接基于 `mdoc` 起步反而更重。

### 1.2 claim / selective disclosure：仓库已经不缺“能证明某个属性”的能力

#### `small` 路径

- `lib/circuits/tests/anoncred/small.h`
  - `OpenedAttribute` 以 `ind + len + value` 形式描述被打开的属性。
  - `assert_credential(...)` 把“属性位置、属性值、时间约束、签名验证”组合进电路。
- `lib/circuits/tests/anoncred/small_test.cc`
  - `fill_witness(...)` 已经实际构造了 `show` 列表，例如披露年龄属性。

#### `mdoc` 路径

- `lib/circuits/mdoc/mdoc_zk.h`
  - `RequestedAttribute` 允许 verifier 指定 namespace、id 和 CBOR 值。
- `lib/circuits/mdoc/mdoc_witness.h`
  - 能在原始 mdoc 中定位并生成按属性披露所需的 witness。

判断：
- 仓库已经具备“claim + selective disclosure”的核心机制。
- 缺的不是能力，而是一个统一、面向 demo 的 disclosure API。

### 1.3 prover：已经有通用内核，也有业务层封装

#### 通用层

- `lib/zk/zk_prover.h`
  - `commit(...)`
  - `prove(...)`

#### 业务层

- `lib/circuits/mdoc/mdoc_zk.h`
  - `run_mdoc_prover(...)`
- `lib/circuits/tests/anoncred/small_test.cc`
  - 直接通过 `run2_test_zk(...)` 调通 `ZkProver`

判断：
- `prover` 不是 gap。
- 如果做匿名凭证 demo，完全不需要另写新的证明系统。

### 1.4 verifier：已经有通用 verifier，也有产品化验证入口

#### 通用层

- `lib/zk/zk_verifier.h`
  - `recv_commitment(...)`
  - `verify(...)`

#### 产品化层

- `lib/circuits/mdoc/mdoc_zk.h`
  - `run_mdoc_verifier(...)`
- `reference/verifier-service/server/zk/proofs.go`
  - 通过 cgo 调 `run_mdoc_verifier(...)`

#### 实验层

- `lib/circuits/tests/anoncred/small_test.cc`
  - `run2_test_zk(...)` 已完成 prover/verifier 最小闭环

判断：
- verifier 也不是 gap。
- 真正缺的是“把 `small` 的 verifier 从 test harness 提升成 demo API / CLI / JSON 边界”。

### 1.5 transcript / challenge：核心协议部件已经成型

- `lib/random/transcript.h`
  - `Transcript` 实现 Fiat-Shamir transcript
- `lib/sumcheck/transcript_sumcheck.h`
  - sumcheck transcript 写入与 challenge 派生
- `lib/zk/zk_common.h`
  - `initialize_sumcheck_fiat_shamir(...)`

判断：
- challenge 生成机制已经成熟。
- demo 不需要重新设计这层。

### 1.6 serialization：proof 和 circuit 都已有底层格式

- `lib/zk/zk_proof.h`
  - `write(...)`
  - `read(...)`
- `lib/proto/circuit.h`
  - circuit `to_bytes(...)` / `from_bytes(...)`
- `lib/circuits/mdoc/mdoc_generate_circuit.cc`
  - 说明业务层已经在使用 circuit bytes 作为交付边界

判断：
- 底层 proof/circuit serialization 已经足够 demo 复用。
- 缺的是匿名凭证 demo 自己的“封装格式”，不是底层序列化能力。

### 1.7 example harness：已经有可以直接借壳的最小样例

- `lib/circuits/tests/anoncred/small_test.cc`
  - 最短的“构建电路 -> 填 witness -> prover -> verifier”闭环
- `lib/zk/zk_testing.h`
  - `run2_test_zk(...)` 提供最小测试 harness
- `lib/circuits/mdoc/mdoc_zk_test.cc`
  - 生产链路的完整 harness

判断：
- 仓库已经有 example harness。
- 要做 demo，最短路径不是从零写 harness，而是把 `small_test.cc` 抽薄成一个可调用 demo 入口。

## 2. 缺失能力

这里的“缺失”不是指密码学内核没有，而是指“要把它做成一个独立匿名凭证 demo，还差哪些产品化边界”。

| 缺失项 | 当前现状 | 为什么算缺失 |
|---|---|---|
| 统一的匿名凭证 demo API | `small` 逻辑主要还在 `small_test.cc` 中 | 现在它是测试，不是稳定接口 |
| 独立的 proof envelope | 有 `ZkProof::write/read`，但没有 `anoncred` 自己的 request/response 封装 | demo 需要一个清晰的输入输出格式 |
| 独立 verifier 边界 | `small` verifier 只存在于测试流程里 | 还没有像 `run_mdoc_verifier(...)` 这样的匿名凭证对外入口 |
| demo 级凭证发行流程 | 样例 credential 是静态内置字节串 | 没有“issue credential -> present proof”的显式流程接口 |
| demo 级 challenge/session schema | `Transcript` 已有，但应用层消息格式未定义 | 还没有面向 demo 的 challenge 约定 |
| 可运行的用户接口 | 没有 CLI / HTTP / JSON demo 入口直接包装 `small` | 新研究者很难不读源码就跑匿名凭证演示 |
| 多属性/多策略配置层 | `small` 示例是固定 `kNumAttr = 1` 且 disclosure 写死在测试里 | demo 若要展示不同声明，需要一层参数化包装 |
| 与真实生态的桥接 | `small` 是实验格式，`mdoc` 是真实格式 | 缺一个明确说明：demo 是实验格式还是 mdoc 兼容模式 |

### 2.1 最关键的 gap 不是 prover/verifier，而是“边界”

当前仓库的匿名凭证相关代码有一个明显特点：

- 底层能力很强
- 测试样例也已经能跑
- 但对外边界仍然是“研究代码 + test harness”

所以如果问“还缺什么”，最准确的回答不是：

- 缺证明系统
- 缺验证系统

而是：

- 缺一个最薄但稳定的匿名凭证 demo 封装层

### 2.2 对“匿名凭证 demo”来说，当前不建议先补的东西

以下能力并不是第一版 demo 的必要前提：

- 新的证明协议
- 新的 transcript 机制
- 新的 verifier service 框架
- 泛化到所有 credential format 的统一抽象
- 先把 `small` 完全升级成生产代码

原因：
- 这些都不是最短路径。
- 第一版 demo 的目标应该是“把现有闭环显式化”，而不是“把实验目录产品化到完美”。

## 3. 实现匿名凭证 demo 的最短路径

## 3.1 最短路径判断

最短路径是：

- 直接复用 `lib/circuits/tests/anoncred/` 的 `small` 凭证格式
- 直接复用 `lib/zk/*` 的通用 prover/verifier
- 直接复用 `ZkProof::write/read` 作为 proof bytes 边界
- 只增加一个很薄的 demo 包装层

不建议的最短路径外方案：

- 不要先从 `reference/verifier-service/` 起步，因为它是 `mdoc` 专用服务层，不是匿名凭证 demo 的最短实现点。
- 不要先把 `mdoc_zk.cc` 改造成通用 anoncred API，因为那会把问题复杂化。

## 3.2 具体最短实现步骤

### 第一步：冻结 demo 所用凭证格式

直接采用：

- `lib/circuits/tests/anoncred/small.h`
- `lib/circuits/tests/anoncred/small_examples.h`
- `lib/circuits/tests/anoncred/small_witness.h`

理由：
- 已有样例数据
- 已有 witness 生成
- 已有属性披露语义
- 已有签名和时间校验逻辑

### 第二步：把 `small_test.cc` 的核心逻辑抽成 demo 入口

最应该复用的函数片段：

- `make_circuit()`
- `fill_witness(...)`
- `run2_test_zk(...)` 对应的 prover/verifier 闭环

目标不是大改，而是把它们整理成三步接口：

1. `build_demo_circuit()`
2. `prove_demo(...)`
3. `verify_demo(...)`

### 第三步：用 `ZkProof::write/read` 定义 proof 交换边界

直接复用：

- `lib/zk/zk_proof.h`

最短做法：

- prover 端输出 proof bytes
- verifier 端读取 proof bytes
- 不必先设计复杂 JSON/CBOR
- 第一版 demo 甚至可以直接用二进制文件或 hex/base64

### 第四步：固定一个最小 claim 场景

建议第一版只做一个 claim：

- 例如 `age_over_18`
- 或 `birthdate before X`

理由：
- 仓库现有 `small_test.cc` 就是沿着单属性 disclosure 走的
- 单 claim 最容易把 demo 跑通
- 跑通后再扩展到多个 claims

### 第五步：应用层 transcript 先复用静态样例

直接复用：

- `small_examples.h` 中现有 `transcript`
- `lib/random/transcript.h` 中现有 `Transcript`

理由：
- 第一版 demo 不需要先设计完整会话协商协议
- 只要证明 prover/verifier 对同一 transcript 达成一致即可

### 第六步：最后再决定 demo 外壳

可选外壳按成本排序：

1. 最短：C++ CLI
2. 次短：单进程本地 demo 程序
3. 更重：HTTP service

建议：
- 第一版先做 CLI
- 不要先做 HTTP service

## 3.3 最值得复用的模块清单

### 必复用

- `lib/circuits/tests/anoncred/small.h`
- `lib/circuits/tests/anoncred/small_witness.h`
- `lib/circuits/tests/anoncred/small_examples.h`
- `lib/zk/zk_prover.h`
- `lib/zk/zk_verifier.h`
- `lib/zk/zk_proof.h`
- `lib/random/transcript.h`
- `lib/zk/zk_testing.h`

### 可选复用

- `lib/proto/circuit.h`
- `lib/circuits/mdoc/mdoc_witness.h`
  - 仅当你想继续借用 mdoc 的部分 witness/解析工具时再用

### 第一版不建议依赖

- `reference/verifier-service/server/*`
- `lib/circuits/mdoc/mdoc_zk.cc`

原因：
- 它们更偏 `mdoc` 产品集成层，不是匿名凭证 demo 的最短路径。

## 3.4 最终判断

如果目标是“证明这个仓库能不能快速做出匿名凭证 demo”，答案是：能。

而且最短路径相当明确：

- 不需要新证明系统
- 不需要新 verifier 内核
- 不需要新 transcript 机制
- 不需要新 serialization 内核

真正要做的只是：

- 把 `lib/circuits/tests/anoncred/small_test.cc` 从“测试”提升成“最薄 demo 封装”
- 为它补一个清晰的输入输出边界
- 固定一个最小 claim 场景

所以从 gap analysis 的角度，仓库当前状态更像：

- `crypto/proof substrate`: 已经够
- `anoncred demo packaging`: 还差最后一层
