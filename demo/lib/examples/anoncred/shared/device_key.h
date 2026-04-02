#ifndef PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_DEVICE_KEY_H_
#define PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_DEVICE_KEY_H_

#include <string>
#include <vector>

#include "examples/anoncred/shared/types.h"

namespace proofs {

bool GenerateHolderKeyMaterial(HolderKeyMaterial* holder_key, std::string* err);

bool ValidateHolderKeyMaterial(const HolderKeyMaterial& holder_key,
                               std::string* err);

bool SignTranscriptWithHolderKey(const HolderKeyMaterial& holder_key,
                                 const std::vector<uint8_t>& transcript,
                                 std::string* sig_r_hex,
                                 std::string* sig_s_hex, std::string* err);

}  // namespace proofs

#endif  // PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_DEVICE_KEY_H_
