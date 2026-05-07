# ZK-AgentAuth 修改记录与用法

本文记录当前项目在原论文 `Anonymous Credentials from ECDSA` 实现基础上的扩展、已验证流程和常用命令。

## 当前完成内容

当前保留原 ECDSA/P-256/SHA-256 实现，暂未替换为 SM2/SM3。已经完成方案中撤销机制以外的主要委托能力：

- 约束 7：在签名电路中验证 Alice/device 对委托消息的 ECDSA 签名。
- 约束 8：在 hash 电路中检查请求 claim 被 Alice 委托策略覆盖。
- 约束 9：在 hash 电路中检查 `now < policy.expires`。
- 约束 10：在签名电路中验证 Agent 对本次 session transcript/docType 摘要的 ECDSA 签名。
- 保留原始 `run_mdoc_prover/verifier/generate_circuit`，新增 delegated 版本，避免破坏原 demo。
- 增加通用谓词策略层，支持通过参数组合 `DISCLOSE / EQ / IN_SET / GE / LE`。

## 委托消息

委托消息采用固定宽度编码，然后做 SHA-256：

```text
"ZKDELG1\0"
+ agent_pkx
+ agent_pky
+ allowed_claim_count
+ allowed_claim_hash[4]
+ expires[20]
+ policy_context_hash
```

其中 `policy_context_hash = SHA256(agent_id || "|" || canonical_predicates)`。

注意：Agent 公钥坐标在委托消息中使用大端 32 字节编码；库内部字段字节是小端，代码里已经做了转换。

## 通用谓词

目前支持的谓词：

```text
DISCLOSE
EQ
IN_SET
GE
LE
```

多个谓词按 AND 组合，也就是全部满足才通过。

示例：

```text
age_over_18:EQ:true
height:GE:170
nationality:IN_SET:CN,SG
amount:LE:300
ip_region:IN_SET:campus_net,home_net
```

当前 demo 中实际可直接跑通的 claim 包括：

```text
age_over_18
height
family_name
birth_date
```

其中 `height` 是数值型 CBOR，当前 example 3 中值为 `175`。

## 构建

在项目根目录执行：

```bash
cmake --build /tmp/contest_web_demo_e2e --target delegation_demo_issuer delegation_demo_alice delegation_demo_verifier delegation_demo_agent
```

如果还没有 CMake build 目录，先执行：

```bash
cmake -S demo/lib -B /tmp/contest_web_demo_e2e
```

依赖：

```bash
sudo apt install libzstd-dev libbenchmark-dev
```

目前 benchmark 不是必须跑，测试/benchmark 已做成可选。

## 基础委托验证

完整流程：

```bash
rm -rf /tmp/contest_web_demo_run

/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_issuer issue \
  --example 3 \
  --out /tmp/contest_web_demo_run/issue

/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_alice delegate \
  --holder /tmp/contest_web_demo_run/issue/holder \
  --claim age_over_18 \
  --expires 2027-01-01T00:00:00Z \
  --agent-id bookstore-agent \
  --out /tmp/contest_web_demo_run/delegation

/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_verifier request \
  --issuer-public /tmp/contest_web_demo_run/issue/issuer_public \
  --claim age_over_18 \
  --out /tmp/contest_web_demo_run/request

/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_agent present \
  --delegation /tmp/contest_web_demo_run/delegation \
  --issuer-public /tmp/contest_web_demo_run/issue/issuer_public \
  --request /tmp/contest_web_demo_run/request \
  --out /tmp/contest_web_demo_run/presentation

/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_verifier verify \
  --issuer-public /tmp/contest_web_demo_run/issue/issuer_public \
  --request /tmp/contest_web_demo_run/request \
  --presentation /tmp/contest_web_demo_run/presentation
```

预期输出：

```text
ZK proof: PASS
Delegation sig: PASS
Policy claims: PASS
Policy predicates: PASS
Policy expiry: PASS
Overall: ACCEPT
```

## 通用谓词验证

示例：同时检查 `age_over_18 == true` 且 `height >= 170`。

```bash
rm -rf /tmp/contest_web_demo_predicate_run

/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_issuer issue \
  --example 3 \
  --out /tmp/contest_web_demo_predicate_run/issue

/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_alice delegate \
  --holder /tmp/contest_web_demo_predicate_run/issue/holder \
  --predicate age_over_18:EQ:true \
  --predicate height:GE:170 \
  --expires 2027-01-01T00:00:00Z \
  --agent-id bookstore-agent \
  --out /tmp/contest_web_demo_predicate_run/delegation

/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_verifier request \
  --issuer-public /tmp/contest_web_demo_predicate_run/issue/issuer_public \
  --predicate age_over_18:EQ:true \
  --predicate height:GE:170 \
  --out /tmp/contest_web_demo_predicate_run/request

/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_agent present \
  --delegation /tmp/contest_web_demo_predicate_run/delegation \
  --issuer-public /tmp/contest_web_demo_predicate_run/issue/issuer_public \
  --request /tmp/contest_web_demo_predicate_run/request \
  --out /tmp/contest_web_demo_predicate_run/presentation

/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_verifier verify \
  --issuer-public /tmp/contest_web_demo_predicate_run/issue/issuer_public \
  --request /tmp/contest_web_demo_predicate_run/request \
  --presentation /tmp/contest_web_demo_predicate_run/presentation
```

已验证通过：

```text
ZK proof: PASS
Delegation sig: PASS
Policy claims: PASS
Policy predicates: PASS
Policy expiry: PASS
Overall: ACCEPT
```

负向例子：

```bash
/tmp/contest_web_demo_e2e/examples/delegation_demo/delegation_demo_alice delegate \
  --holder /tmp/contest_web_demo_predicate_fail/issue/holder \
  --predicate height:GE:180 \
  --expires 2027-01-01T00:00:00Z \
  --agent-id bookstore-agent \
  --out /tmp/contest_web_demo_predicate_fail/delegation
```

因为 example 3 中 `height = 175`，所以 Agent 会拒绝生成证明：

```text
present failed: policy predicate check failed: height numeric predicate failed
```

## 当前限制

- 当前 demo 的 ZK spec 只支持 1 或 2 个 claim，所以最多组合两个谓词。要支持更多条件，需要增加对应 `num_attributes` 的电路规格。
- 通用谓词目前在应用层检查，电路仍负责证明 claim 值来自 mdoc 且被委托授权。谓词内容被并入委托签名承诺，防止被篡改。
- `GE/LE` 当前只支持可解析为整数的 CBOR 值。
- 真实 IP/CIDR 不建议直接放进第一版电路。推荐先抽象成 `ip_region`、`network_zone`、`country_by_ip` 等离散 claim，再用 `IN_SET`。
- 撤销方案尚未实现。
- 加密算法仍是 P-256 ECDSA + SHA-256，后续再替换为 SM2/SM3。

## 关键修改文件

主要修改集中在：

```text
demo/lib/circuits/mdoc/mdoc_zk.*
demo/lib/circuits/mdoc/mdoc_signature.h
demo/lib/circuits/mdoc/mdoc_hash.h
demo/lib/circuits/mdoc/mdoc_generate_circuit.cc
demo/lib/examples/delegation_demo/*
demo/lib/examples/mdoc_anoncred/shared/mdoc_demo.*
demo/lib/CMake/proofs.cmake
```

通用谓词相关主要在：

```text
demo/lib/examples/delegation_demo/shared/types.h
demo/lib/examples/delegation_demo/shared/delegation_crypto.*
demo/lib/examples/delegation_demo/shared/delegation_files.*
demo/lib/examples/delegation_demo/alice/*
demo/lib/examples/delegation_demo/agent/*
demo/lib/examples/delegation_demo/verifier/*
```
