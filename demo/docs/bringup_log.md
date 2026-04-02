# Bring-up Log

这份日志记录了我在当前 Ubuntu 环境里，按照 [docs/build_and_run.md](/home/cat/longfellow-zk/docs/build_and_run.md) 的“最短上手路径”实际尝试构建并运行最小样例的过程。

## 目标

验证以下最小链路是否能在当前环境中跑通：

1. 用 `CMake` 从 `lib/` 配置主工程
2. 只构建最小样例目标 `small_test`
3. 运行 `mdoc.mdoc_small_test`
4. 再用 `ctest` 验证测试发现和执行链路

## 本次采用的最小策略

- 不做大规模重构
- 不改业务代码
- 只在仓库根目录新增一个独立构建目录：`build-bringup`
- 只构建最小目标：`small_test`
- 只运行一个最小测试：`mdoc.mdoc_small_test`

## 环境探测

### 命令

```bash
date -Iseconds
pwd
uname -a
clang --version | head -n 1
cmake --version | head -n 1
go version
node --version
npm --version
ccache --version | head -n 1
```

### 结果

- 时间：`2026-03-09T14:06:21+08:00`
- 工作目录：`/home/cat/longfellow-zk`
- 系统：`Linux ... x86_64 GNU/Linux`
- `clang`：`Ubuntu clang version 18.1.3 (1ubuntu1)`
- `cmake`：`3.28.3`
- `node`：`v20.20.0`
- `npm`：`10.8.2`

### 观察到的环境缺口

- `go` 不存在：`/bin/bash: line 7: go: command not found`
- `ccache` 不存在：`/bin/bash: line 10: ccache: command not found`

### 结论

- 对这次目标来说，`clang` 和 `cmake` 已具备，足以尝试主仓库 C++ 最小样例。
- `go` 缺失会影响 `reference/verifier-service/server`，但不影响本次最小 C++ bring-up。
- `ccache` 缺失只影响构建速度，不影响正确性。

## 步骤 1：CMake 配置

### 命令

```bash
CXX=clang++ cmake -D CMAKE_BUILD_TYPE=Release -S lib -B build-bringup --install-prefix ${PWD}/install-bringup
```

### 关键输出

```text
-- Found GTest: /usr/lib/x86_64-linux-gnu/cmake/GTest/GTestConfig.cmake (found version "1.14.0")
-- Compiler flags added for architecture x86_64: -mpclmul
-- ABSL not found. circuit_maker will not be built.
-- Configuring done
-- Generating done
-- Build files have been written to: /home/cat/longfellow-zk/build-bringup
```

### 结果

- 配置成功
- `GTest` 已找到
- `benchmark` 也已满足，否则配置阶段会在 `find_package(benchmark REQUIRED)` 失败
- `absl` 缺失，但只导致 `circuit_maker` 不构建，不影响最小样例

### 是否需要修复

- 不需要
- 这是可接受的可选组件缺失，不是主链路失败

## 步骤 2：构建最小样例目标

### 命令

```bash
cmake --build build-bringup -j 16 --target small_test
```

### 关键输出

```text
[ 90%] Built target mdoc
[100%] Building CXX object circuits/tests/anoncred/CMakeFiles/small_test.dir/small_test.cc.o
[100%] Linking CXX executable small_test
[100%] Built target small_test
```

### 结果

- `small_test` 构建成功
- 没有出现链接错误
- 没有出现缺失系统库错误

### 是否需要修复

- 不需要

## 步骤 3：直接运行最小样例

### 命令

```bash
./build-bringup/circuits/tests/anoncred/small_test --gtest_filter=mdoc.mdoc_small_test
```

### 关键输出

```text
[ RUN      ] mdoc.mdoc_small_test
[INFO] Compiled circuit: mdocsmall
[INFO] Witness done
[INFO] Fill done
[INFO] ZK Commit start
[INFO] ZK Commitment done
[INFO] ZK sumcheck done
[INFO] ZK constraints done
[INFO] ZK Prover done
[INFO] verifier: recv commit
[INFO] verifier: verify
[INFO] verify done: ok
[       OK ] mdoc.mdoc_small_test (1791 ms)
[  PASSED  ] 1 test.
```

### 结果

- 最小样例运行成功
- 证明构造与验证都完成
- 总耗时约 `1.79s`

### 是否需要修复

- 不需要

## 步骤 4：用 CTest 再验证一次测试入口

### 命令

```bash
ctest --test-dir build-bringup -R '^mdoc.mdoc_small_test$' --output-on-failure
```

### 输出

```text
Test project /home/cat/longfellow-zk/build-bringup
    Start 53: mdoc.mdoc_small_test
1/1 Test #53: mdoc.mdoc_small_test .............   Passed    1.90 sec

100% tests passed, 0 tests failed out of 1
```

### 结果

- `ctest` 入口正常
- 文档里的最小测试命令可执行

## 报错汇总

### 实际阻塞错误

本次最小样例 bring-up 过程中，没有遇到阻塞构建或运行的错误。

### 非阻塞环境问题

1. 缺少 `go`

```text
/bin/bash: line 7: go: command not found
```

影响：
- 不能在当前环境里构建 `reference/verifier-service/server`
- 不影响本次 `small_test` 最小 C++ 样例

2. 缺少 `ccache`

```text
/bin/bash: line 10: ccache: command not found
```

影响：
- 只影响增量构建速度
- 不影响正确性

3. 缺少 `absl`

```text
-- ABSL not found. circuit_maker will not be built.
```

影响：
- `circuit_maker` 不会生成
- 不影响主库和 `small_test`

## 本次没有做的事

- 没有运行 `sudo apt install ...`，因为主仓库最小 C++ 样例所需依赖在当前环境中已经满足
- 没有尝试 Go verifier service，因为当前环境没有 `go`
- 没有尝试 `docs/` 的 `npm run build`，因为这不属于“最小样例”链路
- 没有修改任何业务代码

## 结论

当前 Ubuntu 环境下，主仓库的最小 C++ 样例已经跑通。

最小可复现命令如下：

```bash
CXX=clang++ cmake -D CMAKE_BUILD_TYPE=Release -S lib -B build-bringup --install-prefix ${PWD}/install-bringup
cmake --build build-bringup -j 16 --target small_test
./build-bringup/circuits/tests/anoncred/small_test --gtest_filter=mdoc.mdoc_small_test
```

如果只关心“这个仓库能不能在当前环境里编起来并跑通一条 ZK 链路”，答案是：可以。

## Go verifier service bring-up

在确认最小 C++ 样例跑通后，我继续尝试把 `reference/verifier-service/server` bring up 到“可编译状态”。

### 目标

1. 提供 Go 1.24 工具链
2. 按服务端约定安装 C++ `mdoc` 静态库
3. 执行 `go mod download`
4. 执行 `go build`
5. 做最小启动验证
6. 记录所有环境级失败点

### 步骤 G1：尝试系统安装 Go

#### 命令 1

```bash
sudo apt-get update && sudo apt-get install -y golang-go
```

#### 报错

```text
sudo: a terminal is required to read the password; either use the -S option to read from standard input or configure an askpass helper
sudo: a password is required
```

#### 结论

- 这条路径在当前环境不可行，因为没有交互式 `sudo` 密码输入能力。

#### 命令 2

```bash
apt-get update && apt-get install -y golang-go
```

#### 报错

```text
E: Could not open lock file /var/lib/apt/lists/lock - open (13: Permission denied)
E: Unable to lock directory /var/lib/apt/lists/
```

#### 结论

- 当前执行环境不能直接写系统级 `apt` 目录。
- 这不是仓库问题，而是执行权限限制。

### 步骤 G2：最小修复，下载本地 Go 工具链

因为系统安装不可行，我采用了不碰系统目录的最小修复：把 Go 工具链下载到 `/tmp`。

#### 版本依据

`reference/verifier-service/server/go.mod` 声明：

```text
go 1.24
```

#### 命令

```bash
mkdir -p /tmp/go-toolchain && \
curl -fL https://go.dev/dl/go1.24.0.linux-amd64.tar.gz -o /tmp/go1.24.0.linux-amd64.tar.gz && \
rm -rf /tmp/go-toolchain/go && \
tar -C /tmp/go-toolchain -xzf /tmp/go1.24.0.linux-amd64.tar.gz && \
/tmp/go-toolchain/go/bin/go version
```

#### 结果

```text
go version go1.24.0 linux/amd64
```

#### 结论

- 本地 Go 1.24.0 工具链可用。
- 这是本次唯一做的“修复”，而且是环境级最小修复，没有改仓库代码。

### 步骤 G3：安装 C++ `mdoc` 库到服务端预期位置

`reference/verifier-service/server/zk/proofs.go` 的 `cgo` 配置写死了：

- `-L../../install/lib`
- `-I../../install/include`

因此必须先把 C++ 安装到 `reference/verifier-service/install`。

#### 命令

```bash
CXX=clang++ cmake -D CMAKE_BUILD_TYPE=Release -S lib -B build-verifier --install-prefix ${PWD}/reference/verifier-service/install && \
cmake --build build-verifier -j 16 --target install
```

#### 关键输出

```text
-- Installing: /home/cat/longfellow-zk/reference/verifier-service/install/lib/libmdoc_static.a
-- Installing: /home/cat/longfellow-zk/reference/verifier-service/install/include/mdoc_zk.h
```

#### 结果

- 服务端依赖的静态库和头文件已经安装到正确位置。

### 步骤 G4：下载 Go 模块并构建服务端

为了避免写入受限的用户默认缓存目录，我把 Go 缓存都放到 `/tmp`。

#### 命令

```bash
PATH=/tmp/go-toolchain/go/bin:$PATH \
GOCACHE=/tmp/go-cache \
GOMODCACHE=/tmp/go-modcache \
GOPATH=/tmp/go \
go mod download && \
PATH=/tmp/go-toolchain/go/bin:$PATH \
GOCACHE=/tmp/go-cache \
GOMODCACHE=/tmp/go-modcache \
GOPATH=/tmp/go \
go build
```

#### 结果

- `go mod download` 成功
- `go build` 成功
- 当前仓库已经达到 `reference/verifier-service/server` 的“可编译状态”

### 步骤 G5：最小启动验证

#### 命令

```bash
timeout 15s ./server -circuit_dir ../../../lib/circuits/mdoc/circuits -port :8890
```

#### 关键输出

```text
2026/03/09 14:11:30 Reading from dir ../../../lib/circuits/mdoc/circuits
...
2026/03/09 14:11:47 Fetching VICAL from https://vical.dts.aamva.org/vical/vc
{"time":"2026-03-09T14:11:47.227925557+08:00","level":"ERROR","msg":"could not load VICAL","url":"https://vical.dts.aamva.org/vical/vc","err":"failed to fetch VICAL: Get \"https://vical.dts.aamva.org/vical/vc\": proxyconnect tcp: dial tcp 172.24.144.1:7890: socket: operation not permitted"}
{"time":"2026-03-09T14:11:47.228225501+08:00","level":"INFO","msg":"Starting server","addr":":8890"}
{"time":"2026-03-09T14:11:47.228270019+08:00","level":"ERROR","msg":"server failed to start","err":"listen tcp :8890: socket: operation not permitted"}
```

#### 结果

- 服务二进制可以启动到“读取 circuits、加载证书、尝试抓取 VICAL”这一步。
- 失败点不在编译，而在当前环境限制：
  1. 外网访问 VICAL 被代理/网络策略拦截
  2. 当前沙箱不允许监听本地端口

#### 结论

- 从二进制健康度看，服务已经不是“编不过”，而是“运行环境不允许完整启动 HTTP server”。

### 步骤 G6：执行 Go 单测

#### 命令

```bash
PATH=/tmp/go-toolchain/go/bin:$PATH \
GOCACHE=/tmp/go-cache \
GOMODCACHE=/tmp/go-modcache \
GOPATH=/tmp/go \
go test -v ./...
```

#### 结果摘要

- `proofs/server/v2` 包：无测试文件
- `proofs/server/v2/zk` 包：大多数测试通过
- 失败项：`TestLoadVICAL`

#### 失败详情

```text
panic: httptest: failed to listen on a port: listen tcp6 [::1]:0: socket: operation not permitted
```

#### 结论

- 失败原因仍然是当前环境禁止本地监听端口。
- 这不是源码编译错误，也不是依赖缺失错误。

## Go bring-up 总结

### 已完成

- 用本地 Go 1.24.0 工具链替代系统安装
- 成功安装 `mdoc` 静态库到 `reference/verifier-service/install`
- 成功执行 `go mod download`
- 成功执行 `go build`
- 成功把服务启动到“读 circuits / 读证书 / 尝试抓 VICAL”阶段

### 仍受环境限制的点

1. 不能用 `apt-get` 安装系统级 Go
2. 不能访问外网抓取 VICAL
3. 不能监听本地端口
4. 依赖 `httptest.NewServer` 的 Go 测试会失败

### 最终判断

如果标准是“让 Go verifier service 达到可编译状态”，本次已经达成。

如果标准是“在当前沙箱环境里完整启动 HTTP 服务并通过所有 Go 测试”，当前限制主要来自执行环境，而不是仓库源码。
