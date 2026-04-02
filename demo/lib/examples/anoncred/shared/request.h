#ifndef PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_REQUEST_H_
#define PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_REQUEST_H_

#include <array>
#include <string>
#include <vector>

#include "circuits/tests/anoncred/small_witness.h"
#include "examples/anoncred/shared/types.h"

namespace proofs {

inline constexpr char kAgeOver18Claim[] = "age_over_18";
inline constexpr char kAgeOver21Claim[] = "age_over_21";
inline constexpr char kDateOfBirthClaim[] = "date_of_birth";
inline constexpr char kAgeRangePolicy[] = "age_range";
inline constexpr char kAgeThresholdsPolicy[] = "age_thresholds";

struct CompiledPresentationPolicy {
  size_t attr_index = 0;
  size_t attr_len = 0;
  uint8_t type = 0;
  uint8_t cutoff_count = 0;
  std::array<std::string, kMaxAgeThresholds> cutoff_dates;
};

bool ParseLegacyClaimPolicy(const std::string& claim_name,
                            PresentationPolicy* policy, std::string* err);

bool ParsePolicyType(const std::string& policy_name,
                     PresentationPolicyType* policy_type, std::string* err);

std::string PolicyDisplayName(const PresentationPolicy& policy);

bool ValidatePresentationPolicy(const PresentationPolicy& policy,
                                std::string* err);

bool CompilePresentationPolicy(const PresentationRequest& request,
                               CompiledPresentationPolicy* out,
                               std::string* err);

bool BuildPresentationRequest(const PresentationPolicy& policy,
                              const std::string& now_yyyymmdd,
                              const std::vector<uint8_t>& transcript,
                              PresentationRequest* out,
                              std::string* err);

bool BuildPresentationRequest(const std::string& claim_name,
                              const std::string& now_yyyymmdd,
                              const std::vector<uint8_t>& transcript,
                              PresentationRequest* out,
                              std::string* err);

bool BuildFreshPresentationRequest(const PresentationPolicy& policy,
                                   const std::string& now_yyyymmdd,
                                   PresentationRequest* out,
                                   std::string* err);

bool BuildFreshPresentationRequest(const std::string& claim_name,
                                   const std::string& now_yyyymmdd,
                                   PresentationRequest* out,
                                   std::string* err);

bool BuildOpenedAttributeForValue(const PresentationPolicy& policy,
                                  const std::vector<uint8_t>& disclosed_value,
                                  SmallOpenedAttribute* attr,
                                  std::string* err);

bool BuildOpenedAttributeForValue(const std::string& claim_name,
                                  const std::vector<uint8_t>& disclosed_value,
                                  SmallOpenedAttribute* attr,
                                  std::string* err);

bool ExtractDisclosedValueForRequest(const HolderCredential& credential,
                                     const PresentationRequest& request,
                                     std::vector<uint8_t>* value,
                                     std::string* err);

std::string FormatDisclosedValue(const PresentationPolicy& policy,
                                 const std::vector<uint8_t>& value);

std::string FormatDisclosedValue(const std::string& claim_name,
                                 const std::vector<uint8_t>& value);

}  // namespace proofs

#endif  // PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_REQUEST_H_
