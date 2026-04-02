#ifndef PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_VERIFIER_VERIFY_H_
#define PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_VERIFIER_VERIFY_H_

#include <filesystem>
#include <string>

#include "examples/anoncred/shared/types.h"

namespace proofs {

bool RunRequestCommand(const PresentationPolicy& policy,
                       const std::string& now_yyyymmdd,
                       const std::filesystem::path& out_dir,
                       std::string* err);

bool RunVerifyCommand(const std::filesystem::path& issuer_public_dir,
                      const std::filesystem::path& request_dir,
                      const std::filesystem::path& presentation_dir,
                      bool* verified, std::string* err);

}  // namespace proofs

#endif  // PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_VERIFIER_VERIFY_H_
