#include <filesystem>
#include <iostream>
#include <string>

#include "examples/anoncred/holder/keygen.h"
#include "examples/anoncred/holder/prove.h"

namespace {

const char* GetFlag(int argc, char* argv[], const std::string& name) {
  for (int i = 0; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == name) {
      return argv[i + 1];
    }
  }
  return nullptr;
}

void Usage() {
  std::cerr << "usage:\n"
            << "  anoncred_holder keygen --out <dir>\n"
            << "  anoncred_holder prove --credential <dir> --holder-key <dir> --request <dir> --out <dir>\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2) {
    Usage();
    return 2;
  }
  const std::string cmd(argv[1]);
  std::string err;
  if (cmd == "keygen") {
    const char* out = GetFlag(argc, argv, "--out");
    if (out == nullptr) {
      Usage();
      return 2;
    }
    if (!proofs::RunKeygenCommand(std::filesystem::path(out), &err)) {
      std::cerr << "keygen failed: " << err << "\n";
      return 1;
    }
    std::cout << "holder key written to " << out << "\n";
    return 0;
  }

  if (cmd == "prove") {
    const char* credential = GetFlag(argc, argv, "--credential");
    const char* holder_key = GetFlag(argc, argv, "--holder-key");
    const char* request = GetFlag(argc, argv, "--request");
    const char* out = GetFlag(argc, argv, "--out");
    if (credential == nullptr || holder_key == nullptr || request == nullptr ||
        out == nullptr) {
      Usage();
      return 2;
    }
    if (!proofs::RunProveCommand(std::filesystem::path(credential),
                                 std::filesystem::path(holder_key),
                                 std::filesystem::path(request),
                                 std::filesystem::path(out), &err)) {
      std::cerr << "prove failed: " << err << "\n";
      return 1;
    }
    std::cout << "presentation written to " << out << "\n";
    return 0;
  }

  Usage();
  return 2;
}
