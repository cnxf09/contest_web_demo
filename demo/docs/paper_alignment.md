# 当前 Demo 与论文方案的对齐差距

本文对照仓库 README 中指向的论文目标“Anonymous credentials from ECDSA”，评估当前 `lib/examples/anoncred/` 最小 CLI demo 与论文方案之间的差距。

判断原则：

- 论文目标以仓库 README 的描述为准，即“针对 legacy identity verification standards 构造零知识匿名展示协议”。
- demo 现状以当前 `small` anoncred CLI 实现为准。
- 下面凡是“论文目标”的表述，若仓库内未给出完整形式化定义，按 README 和当前代码能力做保守推断。

## 结论先行

当前 demo 已经实现了论文路线里最关键的一段：

- 把 ECDSA issuer 签名验证、holder 对 verifier challenge 的实时签名、有效期检查、以及基于 DOB 的 selective disclosure / predicate proof 一起放进零知识证明里。

但它仍然不是论文意义上的完整匿名凭证方案。主要差距在五类：

1. 安全性假设还停留在研究型 demo
2. 凭证结构仍是实验格式 `small`，不是标准凭证生态格式
3. 证明语句还较窄，claim / policy 空间有限
4. 零知识性主要来自底层证明系统，本 demo 还没有系统级安全论证
5. 匿名性边界虽已明显改善，但还没有达到完整论文级匿名边界

## 1. 安全性假设

### 论文目标

根据 README，Longfellow 的目标是：

- 在既有 ECDSA 身份凭证生态上构造匿名展示协议
- 让 prover / verifier 的安全性建立在标准零知识与 Fiat-Shamir 等密码学假设上
- 面向真实身份标准，如 ISO MDOC、JWT、VC

来源：

- [README.md](/home/cat/longfellow-zk/README.md:8)

### 当前 demo 已做到的部分

- issuer 对 credential 的签名在电路中验证：[small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:124)
- holder 对 verifier transcript 的实时 device 签名在电路中验证：[small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:125)
- request transcript 由 verifier 每次现场随机生成，而不是静态样例：[verify.cc](/home/cat/longfellow-zk/lib/examples/anoncred/verifier/verify.cc:9), [request.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/request.cc:282)
- verifier 不再读取 holder 完整凭证，只读取 `issuer_public + request + presentation`：[verify.cc](/home/cat/longfellow-zk/lib/examples/anoncred/verifier/verify.cc:20)

### 与论文方案的差距

- 仍然没有 issuer PKI、证书链、撤销、长期信任根管理。
- 当前 issuer 是 demo 本地生成 / materialize 的，不是现实世界可审计 issuer。
- 安全性仍显著依赖“底层 ZK 系统正确且零知识”的库级假设，而不是 demo 自己单独论证完毕。
- 没有 side-channel、proof encoding、metadata leakage 的系统性评估。
- 没有恶意 verifier、恶意 issuer、多展示跨会话链接性的正式安全证明。

### 判断

- 当前 demo 达到了“研究型原型的最小安全假设闭环”。
- 还没有达到“论文方案面向真实凭证生态部署”的完整安全假设层级。

## 2. 凭证结构

### 论文目标

README 明确指向 legacy identity verification standards：

- ISO MDOC
- JWT
- W3C Verifiable Credentials

来源：

- [README.md](/home/cat/longfellow-zk/README.md:8)

### 当前 demo 已做到的部分

- demo credential 已经包含发行者签名、holder device 公钥、有效期、属性字段等匿名展示所需骨架。
- holder 的 device public key 被写入 credential，并在电路中与实时签名一致性绑定：[small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:137)

### 与论文方案的差距

- 当前使用的是 `small` 实验格式，不是 mdoc/JWT/VC 的真实编码。
- credential 布局是固定偏移的 183-byte toy format：[small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:29)
- 没有 schema、namespace、typed claim、标准编码规则、标准 canonicalization。
- selective disclosure 仍依赖“固定位置字段 + 固定 policy 编译”，不是通用 schema 驱动。
- 虽然仓库主线已有 `mdoc` 支持，但当前 demo 还没有迁到那条真实格式链路。

### 判断

- 当前 demo 的凭证结构足够说明论文核心机制“能工作”。
- 但它不能代表论文对真实凭证生态兼容性的完整目标。

## 3. 证明语句

### 当前 demo 实际证明的语句

当前 `small` 电路证明的是：

1. credential 上存在有效 issuer ECDSA 签名：[small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:124)
2. credential 中嵌入的 holder device public key 与本次 transcript 的实时签名一致：[small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:125), [small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:137)
3. `valid_from <= now <= valid_until`：[small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:130)
4. disclosure policy 与公开值匹配：
   - `date_of_birth` 直接披露：[small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:160)
   - `age_range` 由 DOB 与两个 cutoff 推导：[small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:177)
   - `age_thresholds` 由 DOB 与多个 cutoff 的合取推导：[small.h](/home/cat/longfellow-zk/lib/circuits/tests/anoncred/small.h:182)

### 论文目标

论文方案通常希望证明更一般的语句：

- “我持有由合法 issuer 签发的一份真实凭证”
- “这份凭证满足 verifier 请求的 disclosure / predicate policy”
- “presentation 被绑定到 verifier challenge / 会话”
- “除 policy 允许披露的信息外不泄露其余属性”

### 与论文方案的差距

- 当前 policy 空间仍很窄，只支持：
  - 直接披露 DOB
  - 单区间年龄谓词
  - 最多 3 个阈值的年龄合取
- 不支持通用布尔组合、范围证明族、跨字段关系证明、可扩展 claim schema。
- `kNumAttr = 1`，当前 presentation 仍然是单属性 / 单 policy 展示，而非多 disclosure bundle：[small_demo.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/small_demo.cc:30)
- policy 是应用层编译成固定公共输入，不是更完整的通用 policy language。
- 仍没有覆盖 mdoc/JWT/VC 场景下的真实 claim 语义。

### 判断

- 当前 demo 已实现论文核心语句的一个最小子集。
- 距离“论文方案支持的一般匿名凭证语句”仍有明显差距。

## 4. 零知识性来源

### 当前 demo 的零知识性来源

零知识性主要来自 Longfellow / Ligero 证明系统本身，而不是应用层壳代码：

- prover 使用 `ZkProver`：[small_demo.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/small_demo.cc:348)
- verifier 使用 `ZkVerifier`：[small_demo.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/small_demo.cc:390)
- 证明对象包括 issuer 签名、holder 签名、credential bytes、DOB 等私有 witness
- verifier 公共输入只包括 issuer 公钥、challenge hash、公开 policy、公开值、`now` 等：[small_demo.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/small_demo.cc:146)

### 相比旧版本的改进

- verifier 已经不再读取完整 `issued/holder` 凭证目录：[verify.cc](/home/cat/longfellow-zk/lib/examples/anoncred/verifier/verify.cc:20)
- request 中不再直接序列化 `predicate_date.txt` 这种辅助值，而是序列化 formal policy，再在 prove/verify 端编译：[files.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/files.cc:184), [request.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/request.cc:199)

### 与论文方案的差距

- 当前对“零知识性”的判断主要来自底层证明系统假设，没有单独给出面向该 demo 的安全论证。
- 没有分析 proof encoding、长度、运行时日志、错误分支等实现层侧信道是否额外暴露信息。
- 没有给出模拟器级或形式化语义，说明 presentation 除公开 policy 与 disclosed value 外不泄露其他信息。

### 判断

- 当前 demo 在“技术实现”上已经是真正的零知识 proving demo，不是伪装出来的匿名层。
- 但在“论文级安全说明”上，还缺一份面向该 demo 的隐私论证与侧信道边界说明。

## 5. 匿名性边界

### 当前 demo 已做到的边界

当前 verifier 明确能看到的只有：

- issuer 公钥：[small_demo.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/small_demo.cc:123)
- request transcript / challenge：[small_demo.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/small_demo.cc:142)
- `now`：[small_demo.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/small_demo.cc:159)
- request policy 及其编译后的 cutoff：[small_demo.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/small_demo.cc:163)
- disclosed value：[small_demo.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/small_demo.cc:152)
- ZK proof bytes：[files.cc](/home/cat/longfellow-zk/lib/examples/anoncred/shared/files.cc:243)

当前 verifier 明确看不到：

- credential 原文字节
- issuer signature 原文 witness
- holder private key
- 未披露的 first name / family name / gender / issuer id 等字段

### 与论文方案的差距

- `date_of_birth` policy 本身就是直接披露 DOB，因此匿名边界仍取决于 verifier 请求了什么；当前还没有更强的最小披露策略层。
- policy 本身是公开的，因此 verifier 知道自己请求的年龄区间、阈值、`now`，这不是漏洞，但意味着“匿名性边界”仍是 policy-aware 的。
- 还没有多 presentation 间 unlinkability 的专门分析。
- 还没有研究恶意 verifier 通过构造细粒度 policy 反复询问来挤出更多信息的边界。
- 还没有 issuer / holder / verifier 跨多会话、多 issuer 的链接性实验或形式化说明。

### 判断

- 相比早期版本，当前 demo 已经明显更接近论文想要的匿名展示边界。
- 但它更准确地说是“单次 selective disclosure / predicate proof demo”，还不是“完整匿名凭证系统边界已经被严格收束”的状态。

## 总结

如果按论文目标来评价，当前 demo 的位置可以概括为：

- 已经完成：
  - challenge freshness
  - holder possession 绑定
  - verifier 不直接读取完整 credential
  - 基于 DOB 的 predicate proof
  - formal policy 输入的最小收束
- 尚未完成：
  - 真实标准凭证结构
  - 更通用的证明语句与多 claim policy
  - 面向 demo 的系统级安全论证
  - 多会话 unlinkability / 恶意 verifier 边界分析
  - 真实发行生态中的 PKI / 撤销 / 证书链前提

因此，当前 demo 最适合的定位是：

- 论文核心机制的研究型实现样例
- 已接近匿名凭证协议最小闭环
- 但还不是论文方案的完整系统化落地版本
