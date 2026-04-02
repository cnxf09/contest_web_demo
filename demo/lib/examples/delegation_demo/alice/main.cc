#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "examples/delegation_demo/alice/delegate.h"

namespace {

void Usage() {
  std::cerr << "usage:\n"
            << "  delegation_demo_alice delegate\n"
            << "    --holder <dir>        Alice 的 holder/ 目录\n"
            << "    --claim  <alias>      允许的 claim alias（可多次指定）\n"
            << "    --expires <iso8601>   委托过期时间，如 2027-01-01T00:00:00Z\n"
            << "    --agent-id <id>       Agent 标识（可选，默认 'agent'）\n"
            << "    --out <dir>           输出 delegation/ 目录\n"
            << "\n示例：\n"
            << "  delegation_demo_alice delegate \\\n"
            << "    --holder run/demo/issue/holder \\\n"
            << "    --claim age_over_18 \\\n"
            << "    --expires 2027-01-01T00:00:00Z \\\n"
            << "    --agent-id bookstore-agent \\\n"
            << "    --out run/demo/delegation\n";
}

const char* GetFlag(int argc, char* argv[], const std::string& name) {
  for (int i = 0; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == name) {
      return argv[i + 1];
    }
  }
  return nullptr;
}

// 收集所有 --claim 参数（可多次）
std::vector<std::string> GetFlagAll(int argc, char* argv[], const std::string& name) {
  std::vector<std::string> result;
  for (int i = 0; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == name) {
      result.push_back(argv[i + 1]);
    }
  }
  return result;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2 || std::string(argv[1]) != "delegate") {
    Usage();
    return 2;
  }

  const char* holder_c  = GetFlag(argc, argv, "--holder");
  const char* expires_c = GetFlag(argc, argv, "--expires");
  const char* out_c     = GetFlag(argc, argv, "--out");

  if (holder_c == nullptr || expires_c == nullptr || out_c == nullptr) {
    std::cerr << "error: --holder, --expires, --out are required\n\n";
    Usage();
    return 2;
  }

  const std::vector<std::string> claims = GetFlagAll(argc, argv, "--claim");
  if (claims.empty()) {
    std::cerr << "error: at least one --claim is required\n\n";
    Usage();
    return 2;
  }

  const char* agent_id_c = GetFlag(argc, argv, "--agent-id");
  const std::string agent_id = (agent_id_c != nullptr) ? agent_id_c : "agent";

  std::string err;
  if (!proofs::RunDelegateCommand(
          std::filesystem::path(holder_c),
          claims,
          std::string(expires_c),
          agent_id,
          std::filesystem::path(out_c),
          &err)) {
    std::cerr << "delegate failed: " << err << "\n";
    return 1;
  }

  std::cout << "delegation written to " << out_c << "\n";
  std::cout << "  allowed claims:";
  for (const auto& c : claims) std::cout << " " << c;
  std::cout << "\n";
  std::cout << "  expires: " << expires_c << "\n";
  std::cout << "  agent-id: " << agent_id << "\n";
  return 0;
}
