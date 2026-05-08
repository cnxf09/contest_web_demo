// Module A: Issuer
// 直接调用现有的 RunMdocIssueCommand，无需修改现有代码

#include <filesystem>
#include <iostream>
#include <string>

#include "examples/delegation_demo/shared/revocation.h"
#include "examples/mdoc_anoncred/shared/crypto.h"
#include "examples/mdoc_anoncred/shared/files.h"
#include "examples/mdoc_anoncred/issuer/issue.h"

namespace {

void Usage() {
  std::cerr << "usage:\n"
            << "  delegation_demo_issuer issue\n"
            << "    --example <id>   mdoc 样本 ID（默认 3）\n"
            << "    --revoked        生成已撤销状态（用于负向测试）\n"
            << "    --out <dir>      输出根目录（生成 holder/ 和 issuer_public/）\n"
            << "\n示例：\n"
            << "  delegation_demo_issuer issue --example 3 --out run/demo/issue\n";
}

bool HasFlag(int argc, char* argv[], const std::string& name) {
  for (int i = 0; i < argc; ++i) {
    if (std::string(argv[i]) == name) return true;
  }
  return false;
}

const char* GetFlag(int argc, char* argv[], const std::string& name) {
  for (int i = 0; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == name) {
      return argv[i + 1];
    }
  }
  return nullptr;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2 || std::string(argv[1]) != "issue") {
    Usage();
    return 2;
  }

  const char* out_c = GetFlag(argc, argv, "--out");
  if (out_c == nullptr) {
    std::cerr << "error: --out is required\n\n";
    Usage();
    return 2;
  }

  const char* example_c = GetFlag(argc, argv, "--example");
  const uint32_t example_id = (example_c != nullptr)
                                   ? static_cast<uint32_t>(std::stoul(example_c))
                                   : 3u;
  const bool revoked = HasFlag(argc, argv, "--revoked");

  std::string err;
  if (!proofs::RunMdocIssueCommand(example_id, std::filesystem::path(out_c), &err)) {
    std::cerr << "issue failed: " << err << "\n";
    return 1;
  }

  proofs::HolderMdoc holder;
  proofs::MdocIssuerPublicBundle issuer_public;
  if (!proofs::ReadHolderMdocDir(std::filesystem::path(out_c) / "holder",
                                 &holder, &err) ||
      !proofs::ReadMdocIssuerPublicDir(
          std::filesystem::path(out_c) / "issuer_public", &issuer_public,
          &err)) {
    std::cerr << "issue failed: " << err << "\n";
    return 1;
  }
  std::string rev_sk;
  if (!proofs::GenerateP256KeyPair(&rev_sk, &issuer_public.revocation_pkx_hex,
                                   &issuer_public.revocation_pky_hex, &err) ||
      !proofs::CreateRevocationStatus(
          rev_sk, holder.device_pkx_hex, holder.device_pky_hex,
          /*epoch=*/1, "2027-01-01T00:00:00Z", revoked,
          &holder.revocation_status, &err) ||
      !proofs::WriteRevocationStatusJson(
          std::filesystem::path(out_c) / "holder" / "revocation_status.json",
          holder.revocation_status, &err) ||
      !proofs::WriteStringFile(
          std::filesystem::path(out_c) / "issuer_public" / "revocation_pkx.txt",
          issuer_public.revocation_pkx_hex, &err) ||
      !proofs::WriteStringFile(
          std::filesystem::path(out_c) / "issuer_public" / "revocation_pky.txt",
          issuer_public.revocation_pky_hex, &err)) {
    std::cerr << "issue failed: " << err << "\n";
    return 1;
  }

  std::cout << "holder mdoc written to "
            << std::filesystem::path(out_c) / "holder" << "\n";
  std::cout << "issuer public bundle written to "
            << std::filesystem::path(out_c) / "issuer_public" << "\n";
  std::cout << "revocation status: " << (revoked ? "REVOKED" : "VALID")
            << "\n";
  return 0;
}
