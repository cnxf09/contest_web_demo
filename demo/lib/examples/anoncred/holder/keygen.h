#ifndef PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_HOLDER_KEYGEN_H_
#define PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_HOLDER_KEYGEN_H_

#include <filesystem>
#include <string>

namespace proofs {

bool RunKeygenCommand(const std::filesystem::path& out_dir, std::string* err);

}  // namespace proofs

#endif  // PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_HOLDER_KEYGEN_H_
