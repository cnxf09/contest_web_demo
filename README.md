# ZK-AgentAuth 委托凭证 Demo

基于零知识证明的 AI Agent 委托授权系统演示。用户（Alice）可以将凭证中的部分属性委托给 Agent，Agent 向验证者出示 ZK 证明，验证者在无法识别 Alice 身份的情况下确认授权合法性。

## 方案概述

```
Issuer ──颁发凭证──▶ Alice ──委托──▶ Agent ──ZK证明──▶ Verifier
```

| 角色 | 模块 | 说明 |
|------|------|------|
| Issuer（颁发方） | A | 颁发 ISO 18013-5 mDL 格式数字凭证 |
| Alice（委托人） | B | 生成委托策略，用设备私钥签名授权 Agent |
| Agent（代理人） | C | 验证策略，调用 ZK 证明器生成证明 |
| Verifier（验证者） | D | 生成验证请求，验证 ZK 证明及委托合法性 |

**ZK 约束**：约束①-⑩ 在委托版电路内验证；约束⑪（撤销）暂不实现。当前仍使用原论文实现中的 ECDSA/P-256 与 SHA-256，后续再替换为 SM2/SM3。

---

## 目录结构

```
lib/examples/delegation_demo/
├── shared/               # 公共工具库
│   ├── types.h           # Policy 数据结构
│   ├── delegation_crypto.h/cc   # 规范化编码、签名、验签
│   └── delegation_files.h/cc    # 目录读写、delegation_token.json
├── issuer/main.cc        # Module A CLI
├── alice/                # Module B
│   ├── delegate.h/cc     # 委托逻辑
│   └── main.cc           # CLI
├── agent/                # Module C
│   ├── present.h/cc      # 代理出示逻辑
│   └── main.cc           # CLI
├── verifier/             # Module D
│   ├── verify.h/cc       # 请求生成 + 验证逻辑
│   └── main.cc           # CLI（request / verify 两个子命令）
└── CMakeLists.txt

web_demo/
├── app.py                # Flask 后端（API）
├── requirements.txt
└── static/index.html     # 单页前端
```

---

## 编译

```bash
cd lib
mkdir build-delegation && cd build-delegation
cmake .
cmake --build . --target \
  delegation_demo_issuer \
  delegation_demo_alice \
  delegation_demo_agent \
  delegation_demo_verifier
```

编译产物位于 `build-delegation/examples/delegation_demo/`。

---

## CLI 使用

以下命令在 `build-delegation/examples/delegation_demo/` 目录下执行，输出统一写入 `run/demo/`。

### Step 1：颁发凭证（Module A）

```bash
./delegation_demo_issuer issue --example 3 --out run/demo/issue
```

输出：
- `run/demo/issue/holder/` — Alice 的私有凭证材料（设备密钥、device_response.cbor 等）
- `run/demo/issue/issuer_public/` — 颁发方公开信息（issuer_pkx/pky、doc_type 等）

### Step 2：生成委托（Module B）

```bash
./delegation_demo_alice delegate \
  --holder run/demo/issue/holder \
  --claim age_over_18 \
  --expires 2027-01-01T00:00:00Z \
  --agent-id bookstore-agent \
  --out run/demo/delegation
```

`--claim` 可多次指定。输出 `run/demo/delegation/`，包含 Agent 临时密钥、policy.json、委托签名等。

### Step 3：生成验证请求（Module D-1）

```bash
./delegation_demo_verifier request \
  --issuer-public run/demo/issue/issuer_public \
  --claim age_over_18 \
  --out run/demo/request
```

输出 `run/demo/request/`，包含 ZK 电路（circuit.bin）、session_transcript.cbor 等。

### Step 4：代理出示（Module C）

```bash
./delegation_demo_agent present \
  --delegation run/demo/delegation \
  --issuer-public run/demo/issue/issuer_public \
  --request run/demo/request \
  --out run/demo/presentation
```

Agent 在此步骤执行：
1. 生成 Agent 会话签名（约束⑩）
2. 调用委托版 ZK 证明器生成证明（约束①-⑩）

输出 `run/demo/presentation/`，包含 proof.bin 和 delegation_token.json。

### Step 5：验证（Module D-2）

```bash
./delegation_demo_verifier verify \
  --issuer-public run/demo/issue/issuer_public \
  --request run/demo/request \
  --presentation run/demo/presentation
```

输出示例：

```
=== Delegation Verification ===
ZK proof:        PASS    # 约束①-⑩
Delegation sig:  PASS    # 约束⑦（电路内）
Policy claims:   PASS    # 约束⑧（电路内）
Policy expiry:   PASS    # 约束⑨（电路内）
Overall:         ACCEPT
```

退出码：`0` = ACCEPT，`1` = REJECT。

---

## Web Demo

提供浏览器可操作的 4 步演示界面。

### 启动

```bash
pip install flask
cd web_demo
python3 app.py
# 访问 http://localhost:5001
```

### API 接口

| 端点 | 方法 | 说明 |
|------|------|------|
| `GET /` | GET | 前端页面 |
| `POST /api/issue` | POST | 颁发凭证 |
| `POST /api/delegate` | POST | 生成委托 |
| `POST /api/present` | POST | 生成验证请求 + ZK 证明 |
| `POST /api/verify` | POST | 验证证明，返回逐项检查结果 |
| `POST /api/reset` | POST | 清除会话 |

---

## 文件格式约定

所有模块通过文件系统目录交换数据，格式规范如下：

| 格式 | 规则 |
|------|------|
| hex 字符串 | 小写，含 `0x` 前缀，无换行，64 字符（私钥/公钥坐标）或 128 字符（签名） |
| 时间 | ISO 8601 UTC，如 `2027-01-01T00:00:00Z` |
| 公钥 | 分为 `_pkx.txt` 和 `_pky.txt` 两个文件 |
| 二进制 | `.cbor` / `.bin` 原始字节 |
| JSON | UTF-8，2 空格缩进，snake_case 键名 |

**delegation_token.json**（Module C 输出，Module D 读入；展示公开委托材料）：

```json
{
  "agent_pkx": "0x...",
  "agent_pky": "0x...",
  "delegation_msg": "0x...",
  "delegation_sig": "0x...",
  "agent_sig": "0x...",
  "device_pkx": "0x...",
  "device_pky": "0x...",
  "policy": {
    "agent_id": "bookstore-agent",
    "allowed_claims": ["age_over_18"],
    "created": "2026-04-01T10:00:00Z",
    "expires": "2027-01-01T00:00:00Z"
  }
}
```

---

## 安全说明

- **分层方案折衷**：当前 demo 中 Alice 将 `sk_d` 传入 `delegation/` 目录供 Agent 使用（ZK 电路需要设备签名）。完整方案中 `sk_d` 不应离开安全硬件。
- **委托签名验证**：当前委托版电路在 ZK 内验证 `σ_del`，通过 MAC 桥接把隐藏的设备公钥与策略摘要绑定。
- **可撤销性**：当前版本不实现。完整方案通过 SMT（稀疏 Merkle 树）实现约束⑪。

---

## 密码学参数

| 参数 | 值 |
|------|-----|
| 签名算法 | ECDSA over NIST P-256 |
| 哈希函数 | SHA-256 |
| ZK 证明系统 | Ligero v2（Sumcheck + Ligero）|
| ZK 电路 | Fp256（ECDSA 验签）+ GF(2^128)（SHA-256 属性哈希）|
| 证明大小 | ~350 KB（1 个属性） |
| 证明生成时间 | ~5s（Apple M1）|
