#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "examples/anoncred/shared/files.h"
#include "examples/anoncred/shared/request.h"
#include "examples/anoncred/verifier/verify.h"

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
            << "  anoncred_verifier request --claim <name> --now <YYYYMMDD> --out <dir>\n"
            << "  anoncred_verifier request --policy date_of_birth --now <YYYYMMDD> --out <dir>\n"
            << "  anoncred_verifier request --policy age_range --min-age <n> --max-age <n> --now <YYYYMMDD> --out <dir>\n"
            << "  anoncred_verifier request --policy age_thresholds --thresholds <a,b,...> --now <YYYYMMDD> --out <dir>\n"
            << "  anoncred_verifier verify --issuer-public <dir> --request <dir> --presentation <dir>\n";
}

bool ParseAgeList(const std::string& text, std::vector<uint8_t>* out,
                  std::string* err) {
  out->clear();
  std::stringstream ss(text);
  std::string item;
  while (std::getline(ss, item, ',')) {
    if (item.empty()) {
      if (err != nullptr) {
        *err = "thresholds must not contain empty entries";
      }
      return false;
    }
    for (char c : item) {
      if (c < '0' || c > '9') {
        if (err != nullptr) {
          *err = "thresholds must be numeric";
        }
        return false;
      }
    }
    const int age = std::stoi(item);
    if (age <= 0 || age > 99) {
      if (err != nullptr) {
        *err = "threshold values must be between 1 and 99";
      }
      return false;
    }
    out->push_back(static_cast<uint8_t>(age));
  }
  return true;
}

bool ParseSingleAge(const char* text, uint8_t* out, std::string* err) {
  if (text == nullptr || *text == '\0') {
    if (err != nullptr) {
      *err = "age value must not be empty";
    }
    return false;
  }
  for (const char* p = text; *p != '\0'; ++p) {
    if (*p < '0' || *p > '9') {
      if (err != nullptr) {
        *err = "age value must be numeric";
      }
      return false;
    }
  }
  const int age = std::stoi(text);
  if (age <= 0 || age > 99) {
    if (err != nullptr) {
      *err = "age value must be between 1 and 99";
    }
    return false;
  }
  *out = static_cast<uint8_t>(age);
  return true;
}

bool BuildPolicyFromFlags(int argc, char* argv[], proofs::PresentationPolicy* policy,
                          std::string* err) {
  const char* claim = GetFlag(argc, argv, "--claim");
  const char* policy_name = GetFlag(argc, argv, "--policy");
  if (claim != nullptr) {
    return proofs::ParseLegacyClaimPolicy(claim, policy, err);
  }
  if (policy_name == nullptr) {
    if (err != nullptr) {
      *err = "missing --claim or --policy";
    }
    return false;
  }

  if (!proofs::ParsePolicyType(policy_name, &policy->type, err)) {
    return false;
  }
  if (policy->type == proofs::PresentationPolicyType::kRevealDateOfBirth) {
    return proofs::ValidatePresentationPolicy(*policy, err);
  }
  if (policy->type == proofs::PresentationPolicyType::kAgeRange) {
    const char* min_age = GetFlag(argc, argv, "--min-age");
    const char* max_age = GetFlag(argc, argv, "--max-age");
    if (min_age == nullptr || max_age == nullptr) {
      if (err != nullptr) {
        *err = "age_range requires --min-age and --max-age";
      }
      return false;
    }
    if (!ParseSingleAge(min_age, &policy->min_age, err) ||
        !ParseSingleAge(max_age, &policy->max_age, err)) {
      return false;
    }
    return proofs::ValidatePresentationPolicy(*policy, err);
  }

  const char* thresholds = GetFlag(argc, argv, "--thresholds");
  if (thresholds == nullptr) {
    if (err != nullptr) {
      *err = "age_thresholds requires --thresholds";
    }
    return false;
  }
  if (!ParseAgeList(thresholds, &policy->age_thresholds, err)) {
    return false;
  }
  return proofs::ValidatePresentationPolicy(*policy, err);
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2) {
    Usage();
    return 2;
  }

  const std::string cmd(argv[1]);
  std::string err;
  if (cmd == "request") {
    const char* now = GetFlag(argc, argv, "--now");
    const char* out = GetFlag(argc, argv, "--out");
    if (now == nullptr || out == nullptr) {
      Usage();
      return 2;
    }
    proofs::PresentationPolicy policy;
    if (!BuildPolicyFromFlags(argc, argv, &policy, &err)) {
      std::cerr << "request failed: " << err << "\n";
      return 1;
    }
    if (!proofs::RunRequestCommand(policy, now, std::filesystem::path(out),
                                   &err)) {
      std::cerr << "request failed: " << err << "\n";
      return 1;
    }
    std::cout << "request written to " << out << "\n";
    return 0;
  }

  if (cmd == "verify") {
    const char* issuer_public = GetFlag(argc, argv, "--issuer-public");
    const char* request = GetFlag(argc, argv, "--request");
    const char* presentation = GetFlag(argc, argv, "--presentation");
    if (issuer_public == nullptr || request == nullptr ||
        presentation == nullptr) {
      Usage();
      return 2;
    }
    bool verified = false;
    if (!proofs::RunVerifyCommand(std::filesystem::path(issuer_public),
                                  std::filesystem::path(request),
                                  std::filesystem::path(presentation),
                                  &verified, &err)) {
      std::cerr << "verify failed: " << err << "\n";
      return 1;
    }
    if (!verified) {
      std::cerr << "verification failed: " << err << "\n";
      return 1;
    }
    proofs::PresentationProof proof;
    if (!proofs::ReadPresentationProofDir(std::filesystem::path(presentation),
                                          &proof, &err)) {
      std::cerr << "verify failed: " << err << "\n";
      return 1;
    }
    proofs::PresentationRequest request_policy;
    if (!proofs::ReadPresentationRequestDir(std::filesystem::path(request),
                                            &request_policy, &err)) {
      std::cerr << "verify failed: " << err << "\n";
      return 1;
    }
    std::cout << "verification ok: " << proof.claim_name
              << "="
              << proofs::FormatDisclosedValue(request_policy.policy,
                                              proof.disclosed_value)
              << "\n";
    return 0;
  }

  Usage();
  return 2;
}
