#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "examples/anoncred/issuer/issue.h"

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
  std::cerr << "usage: anoncred_issuer issue --example <id> --out <dir> "
               "--holder-key <dir> "
               "[--first-name <name>] [--family-name <name>] "
               "[--date-of-birth <YYYYMMDD>] [--valid-from <YYYYMMDD>] "
               "[--valid-until <YYYYMMDD>]\n";
  std::cerr << "The output directory will contain holder/ and issuer_public/.\n";
  std::cerr << "If an optional field is omitted, the CLI will prompt for it.\n";
  std::cerr << "Press Enter at a prompt to keep the example default.\n";
}

void PromptIfEmpty(const char* label, std::string* value) {
  if (!value->empty()) {
    return;
  }
  std::cout << label << " [press Enter to keep default]: " << std::flush;
  std::string line;
  if (std::getline(std::cin, line) && !line.empty()) {
    *value = line;
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2 || std::string(argv[1]) != "issue") {
    Usage();
    return 2;
  }
  const char* example = GetFlag(argc, argv, "--example");
  const char* out = GetFlag(argc, argv, "--out");
  const char* holder_key = GetFlag(argc, argv, "--holder-key");
  if (example == nullptr || out == nullptr || holder_key == nullptr) {
    Usage();
    return 2;
  }

  proofs::IssueOptions options;
  options.holder_key_dir = holder_key;
  if (const char* first = GetFlag(argc, argv, "--first-name"); first != nullptr) {
    options.first_name = first;
  }
  if (const char* family = GetFlag(argc, argv, "--family-name");
      family != nullptr) {
    options.family_name = family;
  }
  if (const char* dob = GetFlag(argc, argv, "--date-of-birth"); dob != nullptr) {
    options.date_of_birth_yyyymmdd = dob;
  }
  if (const char* valid_from = GetFlag(argc, argv, "--valid-from");
      valid_from != nullptr) {
    options.valid_from_yyyymmdd = valid_from;
  }
  if (const char* valid_until = GetFlag(argc, argv, "--valid-until");
      valid_until != nullptr) {
    options.valid_until_yyyymmdd = valid_until;
  }
  PromptIfEmpty("first_name", &options.first_name);
  PromptIfEmpty("family_name", &options.family_name);
  PromptIfEmpty("date_of_birth (YYYYMMDD)", &options.date_of_birth_yyyymmdd);
  PromptIfEmpty("valid_from (YYYYMMDD)", &options.valid_from_yyyymmdd);
  PromptIfEmpty("valid_until (YYYYMMDD)", &options.valid_until_yyyymmdd);

  std::string err;
  if (!proofs::RunIssueCommand(
          static_cast<uint32_t>(std::strtoul(example, nullptr, 10)),
          std::filesystem::path(out), options, &err)) {
    std::cerr << "issue failed: " << err << "\n";
    return 1;
  }
  std::cout << "holder credential written to " << out << "/holder\n";
  std::cout << "issuer public bundle written to " << out << "/issuer_public\n";
  return 0;
}
