#ifndef PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_TYPES_H_
#define PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_TYPES_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace proofs {

enum class PresentationPolicyType : uint8_t {
  kRevealDateOfBirth = 1,
  kAgeRange = 2,
  kAgeThresholds = 3,
};

inline constexpr size_t kMaxAgeThresholds = 3;

struct PresentationPolicy {
  PresentationPolicyType type = PresentationPolicyType::kRevealDateOfBirth;
  uint8_t min_age = 0;
  uint8_t max_age = 0;
  std::vector<uint8_t> age_thresholds;
};

struct HolderCredential {
  std::vector<uint8_t> credential_bytes;
  std::string issuer_pkx_hex;
  std::string issuer_pky_hex;
  std::string issuer_sig_r_hex;
  std::string issuer_sig_s_hex;
};

struct IssuerPublicBundle {
  uint32_t example_id = 0;
  std::string issuer_pkx_hex;
  std::string issuer_pky_hex;
};

struct HolderKeyMaterial {
  std::string device_sk_hex;
  std::string device_pkx_hex;
  std::string device_pky_hex;
};

struct PresentationRequest {
  PresentationPolicy policy;
  std::vector<uint8_t> transcript_bytes;
  std::string now_yyyymmdd;
};

struct PresentationProof {
  std::vector<uint8_t> proof_bytes;
  std::string claim_name;
  std::vector<uint8_t> disclosed_value;
};

struct VerificationResult {
  bool ok = false;
  std::string message;
  std::string claim_name;
  std::vector<uint8_t> disclosed_value;
};

}  // namespace proofs

#endif  // PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_TYPES_H_
