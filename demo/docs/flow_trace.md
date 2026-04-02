# 最小调用链梳理

本文基于 `docs/project_map.md`，只追踪“最短、最能跑通”的调用链，不展开所有辅助分支。

## 1. 生产 mdoc 链路：最小 proving / verifying 路径

### 1.1 输入数据从哪里进入

#### Prover 侧

最小入口是 C API：

- `lib/circuits/mdoc/mdoc_zk.h`
  - `run_mdoc_prover(...)`

它直接接收：

- 压缩后的 circuit bytes：`bcp`, `bcsz`
- 完整凭证 CBOR：`mdoc`, `mdoc_len`
- issuer 公钥：`pkx`, `pky`
- session transcript：`transcript`, `tr_len`
- 要证明的声明列表：`attrs`, `attrs_len`
- 当前时间：`now`

仓库内最直接调用它的完整测试是：

- `lib/circuits/mdoc/mdoc_zk_test.cc`
  - `MdocZKTest::run_test(...)`
  - `TEST_F(MdocZKTest, one_claim)`
  - `TEST_F(MdocZKTest, two_claims)`

#### Verifier 侧

如果走 HTTP 参考服务，输入首先进入：

- `reference/verifier-service/server/handler.go`
  - `func (s *Server) handleZKVerify(...)`

这里接收 JSON 请求体：

- `Transcript`
- `ZKDeviceResponseCBOR`

随后调用：

- `reference/verifier-service/server/zk/cbor.go`
  - `ProcessDeviceResponse(...)`

把外层 CBOR 转成内部 `VerifyRequest`，再调用：

- `reference/verifier-service/server/zk/proofs.go`
  - `VerifyProofRequest(...)`

最终通过 cgo 进入 C++：

- `lib/circuits/mdoc/mdoc_zk.h`
  - `run_mdoc_verifier(...)`

### 1.2 凭证数据在哪里解析

#### Verifier service 外层响应解析

在 Go 层先把钱包输出的 `ZKDeviceResponseCBOR` 拆开：

- `reference/verifier-service/server/zk/cbor.go`
  - `ProcessDeviceResponse(...)`
  - `ProcessDeviceResponseISO(...)`
  - `ProcessDeviceResponseOriginal(...)`
  - `extractAttributesIso(...)`
  - `extractAttributes(...)`
  - `buildAttributeLists(...)`
  - `validateIssuerKey(...)`

这一层的职责是：

- 解析外层 ZK response 封装
- 验证 issuer 证书链
- 提取 claims / docType / circuitHash / proof / timestamp
- 组装成 `VerifyRequest`

#### Prover / verifier 共用的 mdoc 内部解析

真正把 `DeviceResponse` 里的 mdoc 结构拆成 witness 索引和属性偏移的是：

- `lib/circuits/mdoc/mdoc_witness.h`
  - `class ParsedMdoc`
  - `MdocProverErrorCode ParsedMdoc::parse_device_response(...)`

它负责定位：

- `documents[0]`
- `issuerSigned`
- `issuerAuth`
- `tagged mso`
- `deviceSigned`
- `deviceSignature`
- `nameSpaces` / `elementIdentifier` / `elementValue` / `digestID` / `random`

在上层，它被两个 witness 构造函数调用：

- `lib/circuits/mdoc/mdoc_witness.h`
  - `MdocProverErrorCode MdocSignatureWitness::compute_witness(...)`
  - `MdocProverErrorCode MdocHashWitness::compute_witness(...)`

而这两个函数又由：

- `lib/circuits/mdoc/mdoc_zk.cc`
  - `MdocProverErrorCode fill_witness(...)`

统一调起。

### 1.3 证明是在哪一层构造

证明构造分三层看最清楚。

#### A. mdoc 业务装配层

- `lib/circuits/mdoc/mdoc_zk.cc`
  - `run_mdoc_prover(...)`
  - `fill_witness(...)`
  - `fill_attributes(...)`
  - `fill_signature_inputs(...)`
  - `compute_macs(...)`
  - `update_macs(...)`

这一层做的事情：

- 解压并反序列化两条电路：`sig` 和 `hash`
- 构造 `W_sig` / `W_hash`
- 从 mdoc 生成 witness
- 生成 MAC 和公共输入
- 为两条电路分别调用 ZK prover

#### B. 通用 ZK 封装层

- `lib/zk/zk_prover.h`
  - `void ZkProver::commit(...)`
  - `bool ZkProver::prove(...)`

在 `run_mdoc_prover(...)` 里被直接调用：

- `hash_p.commit(h_zk, W_hash, tp, rng)`
- `sig_p.commit(sig_zk, W_sig, tp, rng)`
- `hash_p.prove(h_zk, W_hash, tp)`
- `sig_p.prove(sig_zk, W_sig, tp)`

这一层负责：

- 提交 witness + random pad
- 运行 sumcheck transcript
- 派生 verifier constraints
- 调用 Ligero 对 commitment 做证明

#### C. 更底层证明内核

- `lib/sumcheck/prover.h`
  - `void Prover::prove(...)`
- `lib/sumcheck/prover_layers.h`
  - 分层 sumcheck 证明逻辑
- `lib/ligero/ligero_prover.h`
  - `void LigeroProver::commit(...)`
  - `void LigeroProver::prove(...)`
- `lib/zk/zk_proof.h`
  - `void ZkProof::write(...)`

结论：

- “业务证明装配”发生在 `mdoc_zk.cc`
- “真正的通用证明构造”发生在 `lib/zk/zk_prover.h`
- “底层承诺与约束证明”落在 `sumcheck` + `ligero`

### 1.4 验证是在哪一层执行

同样分三层。

#### A. HTTP / Go 组装层

- `reference/verifier-service/server/handler.go`
  - `handleZKVerify(...)`
- `reference/verifier-service/server/zk/proofs.go`
  - `VerifyProofRequest(...)`

这一层负责：

- 取 circuit
- 校验属性数和 spec
- 把 Go 结构转成 C 结构
- 调 `C.run_mdoc_verifier(...)`

#### B. mdoc 业务验证层

- `lib/circuits/mdoc/mdoc_zk.cc`
  - `run_mdoc_verifier(...)`
  - `fill_public_inputs(...)`
  - `fill_attributes(...)`

这一层负责：

- 解压并解析电路
- 反序列化 hash proof / sig proof
- 从 `attrs + transcript + pk + now + docType + macs` 重建 public inputs
- 创建 hash/sig 两个 verifier

#### C. 通用 ZK 验证层

- `lib/zk/zk_verifier.h`
  - `void ZkVerifier::recv_commitment(...)`
  - `bool ZkVerifier::verify(...)`
- `lib/ligero/ligero_verifier.h`
  - `bool LigeroVerifier::verify(...)`
- `lib/sumcheck/verifier.h`
  - `static bool Verifier::verify(...)`
- `lib/sumcheck/verifier_layers.h`
  - 分层 claims 验证逻辑

在 `run_mdoc_verifier(...)` 里直接发生的是：

- `hash_v.recv_commitment(pr_hash, tv)`
- `sig_v.recv_commitment(pr_sig, tv)`
- `hash_v.verify(pr_hash, pub_hash, tv)`
- `sig_v.verify(pr_sig, pub_sig, tv)`

结论：

- “业务级验证入口”是 `run_mdoc_verifier(...)`
- “真正执行证明校验”的层是 `lib/zk/zk_verifier.h`
- 更底层仍然是 `LigeroVerifier` + `sumcheck verifier`

## 2. 按文件 + 函数名串起来的最小调用链

### 2.1 生产 prover 最小链路

1. `lib/circuits/mdoc/mdoc_zk_test.cc`
   - `MdocZKTest::run_test(...)`
2. `lib/circuits/mdoc/mdoc_zk.cc`
   - `run_mdoc_prover(...)`
3. `lib/circuits/mdoc/mdoc_zk.cc`
   - `fill_witness(...)`
4. `lib/circuits/mdoc/mdoc_witness.h`
   - `MdocHashWitness::compute_witness(...)`
5. `lib/circuits/mdoc/mdoc_witness.h`
   - `MdocSignatureWitness::compute_witness(...)`
6. `lib/circuits/mdoc/mdoc_witness.h`
   - `ParsedMdoc::parse_device_response(...)`
7. `lib/zk/zk_prover.h`
   - `ZkProver::commit(...)`
8. `lib/zk/zk_prover.h`
   - `ZkProver::prove(...)`
9. `lib/zk/zk_proof.h`
   - `ZkProof::write(...)`

### 2.2 生产 verifier 最小链路

如果走 HTTP service：

1. `reference/verifier-service/server/handler.go`
   - `handleZKVerify(...)`
2. `reference/verifier-service/server/zk/cbor.go`
   - `ProcessDeviceResponse(...)`
3. `reference/verifier-service/server/zk/cbor.go`
   - `ProcessDeviceResponseISO(...)` 或 `ProcessDeviceResponseOriginal(...)`
4. `reference/verifier-service/server/zk/proofs.go`
   - `VerifyProofRequest(...)`
5. `lib/circuits/mdoc/mdoc_zk.cc`
   - `run_mdoc_verifier(...)`
6. `lib/circuits/mdoc/mdoc_zk.cc`
   - `fill_public_inputs(...)`
7. `lib/zk/zk_verifier.h`
   - `ZkVerifier::recv_commitment(...)`
8. `lib/zk/zk_verifier.h`
   - `ZkVerifier::verify(...)`
9. `lib/ligero/ligero_verifier.h`
   - `LigeroVerifier::verify(...)`

如果不走 HTTP，而是直接在 C/C++ 层验证，则最短入口直接从第 5 步开始。

## 3. 匿名凭证实验链路：仓库内最短完整样例

这条链不是生产 mdoc API，但它是最短、最纯的“本地构电路 -> 造 witness -> prove -> verify”样例。

### 3.1 输入从哪里进入

- `lib/circuits/tests/anoncred/small_test.cc`
  - `make_circuit()`
  - `fill_witness(Dense<Fp256Base>& W, Dense<Fp256Base>& pub)`

样例数据来自：

- `lib/circuits/tests/anoncred/small_examples.h`
  - `mdoc_small_tests[]`

### 3.2 凭证数据在哪里解析

- `lib/circuits/tests/anoncred/small_witness.h`
  - `bool SmallWitness::compute_witness(...)`

这一步把样例 credential bytes、transcript、签名参数转成：

- issuer 签名 witness
- device key 签名 witness
- SHA witness
- `now_`

### 3.3 证明在哪里构造

- `lib/circuits/tests/anoncred/small_test.cc`
  - `TEST(mdoc, mdoc_small_test)` -> `run2_test_zk(...)`
- `lib/zk/zk_testing.h`
  - `run2_test_zk(...)`
- `lib/zk/zk_prover.h`
  - `ZkProver::commit(...)`
  - `ZkProver::prove(...)`

### 3.4 验证在哪里执行

- `lib/zk/zk_testing.h`
  - `run2_test_zk(...)`
- `lib/zk/zk_verifier.h`
  - `ZkVerifier::recv_commitment(...)`
  - `ZkVerifier::verify(...)`

### 3.5 为什么它重要

因为它是仓库里最小的匿名凭证风格全流程样例：

- 自己构造电路
- 自己填 witness
- 自己序列化 proof
- 自己重读 proof
- 自己做 verifier

没有 HTTP、没有 cgo、没有 mdoc 生产兼容层，适合快速理解核心证明路径。

## 4. 哪些测试最能代表完整流程

### 4.1 最能代表生产 mdoc 全流程的测试

首选：

- `lib/circuits/mdoc/mdoc_zk_test.cc`
  - `MdocZKTest::run_test(...)`
  - `TEST_F(MdocZKTest, one_claim)`
  - `TEST_F(MdocZKTest, two_claims)`

原因：

- 真实调用 `generate_circuit(...)`
- 真实调用 `run_mdoc_prover(...)`
- 真实调用 `run_mdoc_verifier(...)`
- 覆盖 1 claim / 2 claims
- 覆盖不同 mdoc 样本

如果只挑一个最具代表性的函数，不是单个 `TEST_*`，而是它们共同复用的：

- `lib/circuits/mdoc/mdoc_zk_test.cc`
  - `MdocZKTest::run_test(...)`

它是最标准的“完整流程模板”。

### 4.2 最能代表匿名凭证 demo 全流程的测试

- `lib/circuits/tests/anoncred/small_test.cc`
  - `TEST(mdoc, mdoc_small_test)`

原因：

- 是匿名凭证实验目录里的完整路径测试
- 走完了 `make_circuit -> fill_witness -> run2_test_zk -> prover -> verifier`

### 4.3 只覆盖服务外壳、不能代表完整 C++ 流程的测试

- `reference/verifier-service/server/zk/proofs_test.go`
  - `TestVerifyProofRequest(...)`

这个测试明确写了限制：

- 不能真正调用 C 代码
- 只能测 Go 侧参数准备

所以它能代表“服务壳层”，但不能代表“完整证明/验证流程”。

## 5. 一句话总结

如果只追一条“最小但真实”的生产链路，优先看：

- Prover: `mdoc_zk_test.cc::MdocZKTest::run_test -> mdoc_zk.cc::run_mdoc_prover -> mdoc_witness.h::ParsedMdoc::parse_device_response -> zk_prover.h::ZkProver::{commit,prove}`
- Verifier: `handler.go::handleZKVerify -> cbor.go::ProcessDeviceResponse -> proofs.go::VerifyProofRequest -> mdoc_zk.cc::run_mdoc_verifier -> zk_verifier.h::ZkVerifier::{recv_commitment,verify}`

如果只想看最短的“本地完整证明样例”，优先看：

- `lib/circuits/tests/anoncred/small_test.cc::TEST(mdoc, mdoc_small_test)`
