# Build And Run

这份说明按“主仓库 C++ 证明库优先，参考服务与文档站次之”的顺序整理。

## 先说结论：这个仓库实际使用了什么构建方式

### 主构建方式

- `CMake + Make + CTest`
- 主入口是 `lib/CMakeLists.txt`
- 根目录 `README.md` 给出的标准命令也是从 `lib/` 作为 source dir 开始：`cmake -S lib -B ...`

### 次级构建方式

- `Go modules + go build/go test`
  - 位置：`reference/verifier-service/server`
  - 用于参考 verifier service
- `npm + Hugo`
  - 位置：`docs/`
  - 用于文档站
- `Docker`
  - 位置：`reference/verifier-service/Dockerfile`
  - 用于一次性构建 C++ 库和 Go verifier service

### 没看到的主构建方式

- 没有 `Bazel`
- 没有 `Cargo`
- 没有 `pytest` 作为主测试入口

## 1. 安装依赖命令

### 1.1 主仓库 C++ 依赖

`README.md` 明确给出的 Ubuntu / Debian 依赖：

```bash
sudo apt install -y clang cmake libssl-dev libzstd-dev libgtest-dev libbenchmark-dev zlib1g-dev
```

如果你还要编参考 Go 服务和文档站，通常还需要：

```bash
sudo apt install -y golang-go npm
```

### 1.2 参考 Go verifier service 依赖

Go 依赖在 `reference/verifier-service/server` 下由模块系统处理：

```bash
cd reference/verifier-service/server
go mod download
```

### 1.3 文档站依赖

`docs/package.json` 已经固定了 Node 侧依赖，常规安装方式是：

```bash
cd docs
npm ci
```

## 2. 构建命令

### 2.1 主仓库 C++ 构建

推荐直接用仓库 README 的主线命令：

```bash
CXX=clang++ cmake -D CMAKE_BUILD_TYPE=Release -S lib -B build --install-prefix ${PWD}/install
cmake --build build -j 16
```

如果你要同时跑完整测试：

```bash
ctest --test-dir build -j 16
```

### 2.2 参考 Go verifier service 本地构建

这个服务不是独立仓库；它依赖先把 C++ `mdoc` 库安装到 `reference/verifier-service/install`。

```bash
CXX=clang++ cmake -D CMAKE_BUILD_TYPE=Release -S lib -B build-verifier --install-prefix ${PWD}/reference/verifier-service/install
cmake --build build-verifier -j 16 --target install
cd reference/verifier-service/server
go build
```

### 2.3 文档站构建

```bash
cd docs
npm ci
npm run build
```

### 2.4 Docker 构建 verifier service

这是 `reference/verifier-service/server/README.md` 给出的容器化方式：

```bash
cd reference/verifier-service
docker build -t zk -f Dockerfile ../..
```

## 3. 运行最小测试 / 样例的命令

### 3.1 最小 C++ 测试样例：匿名凭证实验测试

这是我认为最适合作为“仓库最小可运行样例”的命令，因为它最短地串起了电路、witness、prover、verifier：

```bash
CXX=clang++ cmake -D CMAKE_BUILD_TYPE=Release -S lib -B build --install-prefix ${PWD}/install
cmake --build build -j 16 --target small_test
./build/circuits/tests/anoncred/small_test --gtest_filter=mdoc.mdoc_small_test
```

如果你更喜欢走 `ctest`：

```bash
ctest --test-dir build -R '^mdoc.mdoc_small_test$' --output-on-failure
```

### 3.2 最小生产主线测试：mdoc ZK 流程

如果你想直接验证生产 `mdoc` 调用链：

```bash
cmake --build build -j 16 --target mdoc_zk_test
./build/circuits/mdoc/mdoc_zk_test --gtest_filter=MdocZKTest.one_claim
```

### 3.3 最小参考服务样例：启动 verifier service 并发送样例请求

先构建并启动：

```bash
cd reference/verifier-service/server
./server -circuit_dir ../../../lib/circuits/mdoc/circuits
```

另开一个终端，发送仓库自带样例：

```bash
curl -X POST -H "Content-Type: application/json" \
  --data-binary @reference/verifier-service/server/examples/post1.json \
  http://localhost:8888/zkverify
```

### 3.4 最小文档站样例

```bash
cd docs
npm ci
npm run serve
```

## 4. 可能失败的环境点

### 4.1 最容易踩坑的主仓库问题

- 这个仓库不是从根目录 `CMakeLists.txt` 构建，而是必须用 `cmake -S lib -B ...`。
- 需要 `clang`、`OpenSSL`、`zstd`、`GTest`、`benchmark` 都能被 CMake 找到；缺任何一个都可能在配置或链接时报错。
- `lib/CMakeLists.txt` 会根据架构注入 `-mpclmul`、`-march=armv8-a+crypto` 等编译参数；在非常规平台上可能没有专门优化参数。
- `circuit_maker` 依赖 `absl`，没有安装时不会构建，但这不影响主库和大多数测试。

### 4.2 Go verifier service 的常见问题

- `go build` 之前必须先把 C++ 库 `install` 到 `reference/verifier-service/install`，因为 `reference/verifier-service/server/zk/proofs.go` 的 `cgo` 配置写死了：
  - `-L../../install/lib`
  - `-I../../install/include`
- 如果只编了 C++ 但没 `make install` 或 `cmake --build ... --target install`，Go 链接阶段会失败。
- 运行服务时需要正确的 `-circuit_dir`，否则它找不到 `lib/circuits/mdoc/circuits` 中的电路文件。
- 服务默认还依赖证书文件和内置样例环境；如果你脱离仓库目录运行，常见问题是相对路径失效。

### 4.3 文档站的常见问题

- `docs/hugo.yaml` 要求 `Hugo extended`，并且最小版本是 `0.146.0`；`docs/package.json` 当前固定的是 `hugo-extended 0.147.9`。
- 如果本机没有 `npm` 或无法安装 Node 依赖，`docs/` 子系统无法构建。
- `npm run build` 之前通常需要先 `npm ci`，否则本地没有 `hugo-extended` 可执行文件。

### 4.4 Docker 方式的常见问题

- `docker build -f Dockerfile ../..` 必须从 `reference/verifier-service/` 目录执行，因为它依赖整个仓库作为 build context。
- 如果构建上下文给错，Dockerfile 里的 `COPY lib ...` 和 `COPY /reference/verifier-service/server ...` 会直接失败。

## 推荐的最短上手路径

如果你的目标只是确认仓库“能编、能跑、能看到一条最小 ZK 流程”，建议只做这三步：

```bash
sudo apt install -y clang cmake libssl-dev libzstd-dev libgtest-dev libbenchmark-dev zlib1g-dev
CXX=clang++ cmake -D CMAKE_BUILD_TYPE=Release -S lib -B build --install-prefix ${PWD}/install
cmake --build build -j 16 --target small_test && ./build/circuits/tests/anoncred/small_test --gtest_filter=mdoc.mdoc_small_test
```

这条路径最短，也最能验证主仓库的核心 C++ 构建链路是否正常。
