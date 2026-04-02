# 新研究者阅读顺序

下面这条顺序以“先建立整体心智模型，再看最短可运行样例，最后进入最核心实现”为原则。
我刻意避开了过早进入 `sumcheck`、`ligero`、`compiler` 等深层细节，因为那样会让第一次阅读的人在还没理解系统边界时就陷入证明细节。

## 建议顺序

### 1. `docs/project_map.md`

为什么先读：
- 这是整个仓库最短的结构化导航，能先回答“仓库里哪些是生产主线，哪些是实验目录，哪些是服务封装”。
- 先建立模块边界，后面读代码时才不会把 `mdoc`、`anoncred`、`zk` 三层混在一起。
- 对新研究者来说，这一步的价值是先知道该忽略什么。

### 2. `README.md`

为什么第二个读：
- 这里给出项目目标、支持的凭证类型、论文链接和标准构建方式。
- 它把仓库定位从“一个 C++ 代码库”提升为“一个围绕 legacy identity verification 的 ZK 系统”。
- 在读实现前先看 README，可以先理解作者想解决的问题，而不是只看到局部函数。

### 3. `lib/CMakeLists.txt`

为什么第三个读：
- 这是最直接的模块装配图，能看出哪些目录属于“production”，哪些是“experiments and tests”。
- 只看这一份文件，就能快速知道主构建入口、依赖关系的大致层次，以及 `mdoc` 和 `anoncred` 在工程里的真实地位。
- 对研究者来说，它比盲目 `rg` 全仓库更高效，因为它定义了系统的编译边界。

### 4. `lib/circuits/tests/anoncred/small_test.cc`

为什么现在读：
- 这是仓库里最容易一口气看完、同时又能串起“构建电路 -> 填 witness -> 跑 prover/verifier”的样例。
- 它比 `mdoc_zk.cc` 更小、更像教程，适合作为第一份“可执行的 mental model”。
- 先看它，可以先弄清楚 Longfellow 的基本使用姿势，再进入更复杂的真实凭证链路。

### 5. `lib/zk/zk_testing.h`

为什么接着读：
- `small_test.cc` 里真正把证明和验证跑起来的，是这里的 `run2_test_zk(...)`。
- 这份文件很适合新手，因为它把通用 ZK 主流程压缩成一个短函数：`commit -> prove -> serialize -> recv_commitment -> verify`。
- 先在测试辅助层看到最小闭环，再去看正式 prover/verifier 类，会容易很多。

### 6. `lib/circuits/mdoc/mdoc_zk_test.cc`

为什么第六个读：
- 到这里再看生产主线测试，难度会比直接看 `mdoc_zk.cc` 低很多，因为你已经知道了最小 ZK 调用链长什么样。
- 这个测试把 `generate_circuit(...)`、`run_mdoc_prover(...)`、`run_mdoc_verifier(...)` 串起来，是“真实凭证流程”的最佳入口。
- 它还能帮助你先认识外部接口，再去读接口背后的实现。

### 7. `lib/circuits/mdoc/mdoc_zk.h`

为什么在测试之后读：
- 这是 `mdoc` 主线的公共 API 定义，包含 `run_mdoc_prover`、`run_mdoc_verifier`、`generate_circuit` 和错误码。
- 先通过测试知道这些接口怎么被调用，再回来看头文件，会更容易理解参数分层：哪些是电路输入，哪些是凭证输入，哪些是声明输入，哪些是 ZK spec。
- 对研究者来说，它也是最适合建立“系统边界”的正式接口文档。

### 8. `lib/circuits/mdoc/mdoc_witness.h`

为什么这里读：
- 真正理解 `mdoc` 的关键，不是先看证明器，而是先看凭证数据怎样被解析成 witness。
- 这个文件里有 `ParsedMdoc::parse_device_response(...)` 和多种 witness 构造逻辑，回答了“凭证数据从哪里进入系统、怎样变成电路输入”这个核心问题。
- 如果在没看懂 witness 结构前就读 `mdoc_zk.cc`，会很容易迷失在填充数组和 MAC 细节里。

### 9. `lib/circuits/mdoc/mdoc_zk.cc`

为什么接近最后再读：
- 这是 `mdoc` 生产路径的主实现，也是把 witness、公共输入、MAC、proof 序列化、verification glue 全部串起来的核心文件。
- 它信息密度很高，直接从这里开始阅读成本太大；但在前面 8 个文件之后再读，就能把它识别为“组装层”，而不是一团难以切分的复杂逻辑。
- 新研究者此时应该重点看三个问题：`fill_witness(...)` 怎么接 witness 层，`run_mdoc_prover(...)` 怎么接 ZK 层，`run_mdoc_verifier(...)` 怎么接 verifier 层。

### 10. `lib/zk/zk_prover.h`

为什么先读 prover 再读 verifier：
- 这里是 Longfellow 通用证明系统的核心抽象，解释了“先 commitment，再跑 sumcheck transcript，再用 Ligero 证明约束成立”的总体设计。
- 在已经理解业务层调用链后再读它，才能把注意力放在协议结构上，而不是被业务字段分散注意力。
- 先看 prover，能先理解系统是怎样构造证明的，之后 verifier 就会变成对称的另一半。

### 11. `lib/zk/zk_verifier.h`

为什么最后读：
- verifier 逻辑相对更短，但它默认读者已经知道 proof 里装了什么、transcript 是怎样形成的、约束从哪里来。
- 把它放在最后，最容易把完整闭环真正看明白：业务层准备公共输入，ZK 层重放 transcript，最后交给 Ligero 验证约束。
- 读完这里，新研究者通常就已经具备继续下钻 `sumcheck/*`、`ligero/*`、`zk_common.h` 的条件。

## 为什么没有把 Go verifier service 放进前 11

我没有把 `reference/verifier-service/server/handler.go`、`reference/verifier-service/server/zk/cbor.go`、`reference/verifier-service/server/zk/proofs.go` 放进主阅读顺序，原因是：

- 它们更像“系统集成层”，而不是 Longfellow 证明系统本体。
- 对第一次进入仓库的人，先理解 C++ 主线比先理解 HTTP/cgo 包装更重要。
- 如果你的研究目标偏向部署或接口联调，再把它们作为第二阶段阅读会更合适。

## 读完这 11 个文件后再看什么

建议的下一层顺序：

1. `lib/zk/zk_common.h`
2. `lib/sumcheck/prover.h`
3. `lib/sumcheck/verifier.h`
4. `lib/ligero/ligero_prover.h`
5. `lib/ligero/ligero_verifier.h`
6. `lib/circuits/ecdsa/verify_circuit.h`
7. `lib/circuits/sha/flatsha256_circuit.h`

这样可以继续从“协议公共逻辑 -> 底层证明协议 -> 具体密码子电路”逐层下钻，而不会打乱前面建立起来的整体模型。
