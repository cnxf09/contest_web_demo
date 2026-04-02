#ifndef PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_ISSUER_ISSUE_H_
#define PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_ISSUER_ISSUE_H_

#include <filesystem>
#include <string>

namespace proofs {

struct IssueOptions {
  std::string holder_key_dir;
  std::string first_name;
  std::string family_name;
  std::string date_of_birth_yyyymmdd;
  std::string valid_from_yyyymmdd;
  std::string valid_until_yyyymmdd;
};

bool RunIssueCommand(uint32_t example_id, const std::filesystem::path& out_dir,
                     const IssueOptions& options, std::string* err);

}  // namespace proofs

#endif  // PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_ISSUER_ISSUE_H_
