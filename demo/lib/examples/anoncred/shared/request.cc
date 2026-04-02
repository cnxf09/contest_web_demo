#include "examples/anoncred/shared/request.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <iomanip>
#include <sstream>

#include "util/crypto.h"

namespace proofs {
namespace {

constexpr size_t kRequestTranscriptSize = 32;
constexpr size_t kDateOfBirthAttrIndex = 64;
constexpr size_t kAgePredicateAttrIndex = 74;

bool IsDate(const std::string& s) {
  if (s.size() != kDateLen) {
    return false;
  }
  for (char c : s) {
    if (c < '0' || c > '9') {
      return false;
    }
  }
  return true;
}

std::string AddYears(const std::string& yyyymmdd, int years) {
  int year = std::stoi(yyyymmdd.substr(0, 4));
  year += years;
  char buf[9];
  std::snprintf(buf, sizeof(buf), "%04d%s", year, yyyymmdd.substr(4).c_str());
  return std::string(buf);
}

bool DateLessOrEqual(const std::string& lhs, const std::string& rhs) {
  return lhs <= rhs;
}

std::string JoinThresholds(const std::vector<uint8_t>& thresholds,
                           const std::string& sep) {
  std::ostringstream out;
  for (size_t i = 0; i < thresholds.size(); ++i) {
    if (i != 0) {
      out << sep;
    }
    out << static_cast<unsigned>(thresholds[i]);
  }
  return out.str();
}

std::string BoolByteToString(const std::vector<uint8_t>& value) {
  if (value.size() == 1) {
    if (value[0] == 0xf5) {
      return "true";
    }
    if (value[0] == 0xf4) {
      return "false";
    }
  }
  std::ostringstream out;
  out << "0x" << std::hex << std::setfill('0');
  for (uint8_t byte : value) {
    out << std::setw(2) << static_cast<unsigned>(byte);
  }
  return out.str();
}

}  // namespace

bool ParseLegacyClaimPolicy(const std::string& claim_name,
                            PresentationPolicy* policy, std::string* err) {
  if (claim_name == kDateOfBirthClaim) {
    policy->type = PresentationPolicyType::kRevealDateOfBirth;
    policy->min_age = 0;
    policy->max_age = 0;
    policy->age_thresholds.clear();
    return true;
  }
  if (claim_name == kAgeOver18Claim) {
    policy->type = PresentationPolicyType::kAgeThresholds;
    policy->min_age = 0;
    policy->max_age = 0;
    policy->age_thresholds = {18};
    return true;
  }
  if (claim_name == kAgeOver21Claim) {
    policy->type = PresentationPolicyType::kAgeThresholds;
    policy->min_age = 0;
    policy->max_age = 0;
    policy->age_thresholds = {21};
    return true;
  }
  if (err != nullptr) {
    *err = "unsupported claim: " + claim_name;
  }
  return false;
}

bool ParsePolicyType(const std::string& policy_name,
                     PresentationPolicyType* policy_type, std::string* err) {
  if (policy_name == kDateOfBirthClaim) {
    *policy_type = PresentationPolicyType::kRevealDateOfBirth;
    return true;
  }
  if (policy_name == kAgeRangePolicy) {
    *policy_type = PresentationPolicyType::kAgeRange;
    return true;
  }
  if (policy_name == kAgeThresholdsPolicy) {
    *policy_type = PresentationPolicyType::kAgeThresholds;
    return true;
  }
  if (err != nullptr) {
    *err = "unsupported policy type: " + policy_name;
  }
  return false;
}

std::string PolicyDisplayName(const PresentationPolicy& policy) {
  switch (policy.type) {
    case PresentationPolicyType::kRevealDateOfBirth:
      return kDateOfBirthClaim;
    case PresentationPolicyType::kAgeRange:
      return "age_in_range_" + std::to_string(policy.min_age) + "_" +
             std::to_string(policy.max_age);
    case PresentationPolicyType::kAgeThresholds:
      if (policy.age_thresholds.size() == 1) {
        return "age_over_" +
               std::to_string(static_cast<unsigned>(policy.age_thresholds[0]));
      }
      return "age_thresholds_over_" +
             JoinThresholds(policy.age_thresholds, "_");
  }
  return "unknown_policy";
}

bool ValidatePresentationPolicy(const PresentationPolicy& policy,
                                std::string* err) {
  switch (policy.type) {
    case PresentationPolicyType::kRevealDateOfBirth:
      if (policy.min_age != 0 || policy.max_age != 0 ||
          !policy.age_thresholds.empty()) {
        if (err != nullptr) {
          *err = "date_of_birth policy must not carry age parameters";
        }
        return false;
      }
      return true;
    case PresentationPolicyType::kAgeRange:
      if (!policy.age_thresholds.empty()) {
        if (err != nullptr) {
          *err = "age_range policy must not carry threshold list";
        }
        return false;
      }
      if (policy.min_age == 0 || policy.max_age == 0 ||
          policy.min_age >= policy.max_age) {
        if (err != nullptr) {
          *err = "age_range requires 0 < min_age < max_age";
        }
        return false;
      }
      return true;
    case PresentationPolicyType::kAgeThresholds:
      if (policy.min_age != 0 || policy.max_age != 0) {
        if (err != nullptr) {
          *err = "age_thresholds policy must not carry min_age/max_age";
        }
        return false;
      }
      if (policy.age_thresholds.empty() ||
          policy.age_thresholds.size() > kMaxAgeThresholds) {
        if (err != nullptr) {
          *err = "age_thresholds requires between 1 and 3 thresholds";
        }
        return false;
      }
      if (!std::is_sorted(policy.age_thresholds.begin(),
                          policy.age_thresholds.end()) ||
          std::adjacent_find(policy.age_thresholds.begin(),
                             policy.age_thresholds.end()) !=
              policy.age_thresholds.end()) {
        if (err != nullptr) {
          *err = "age_thresholds must be strictly increasing";
        }
        return false;
      }
      return true;
  }
  if (err != nullptr) {
    *err = "unknown policy type";
  }
  return false;
}

bool CompilePresentationPolicy(const PresentationRequest& request,
                               CompiledPresentationPolicy* out,
                               std::string* err) {
  if (!IsDate(request.now_yyyymmdd)) {
    if (err != nullptr) {
      *err = "now must be YYYYMMDD";
    }
    return false;
  }
  if (!ValidatePresentationPolicy(request.policy, err)) {
    return false;
  }

  out->cutoff_count = 0;
  for (std::string& cutoff : out->cutoff_dates) {
    cutoff.clear();
  }

  switch (request.policy.type) {
    case PresentationPolicyType::kRevealDateOfBirth:
      out->attr_index = kDateOfBirthAttrIndex;
      out->attr_len = 8;
      out->type = static_cast<uint8_t>(PresentationPolicyType::kRevealDateOfBirth);
      return true;
    case PresentationPolicyType::kAgeRange:
      out->attr_index = kAgePredicateAttrIndex;
      out->attr_len = 1;
      out->type = static_cast<uint8_t>(PresentationPolicyType::kAgeRange);
      out->cutoff_count = 2;
      out->cutoff_dates[0] = AddYears(request.now_yyyymmdd, -request.policy.min_age);
      out->cutoff_dates[1] = AddYears(request.now_yyyymmdd, -request.policy.max_age);
      return true;
    case PresentationPolicyType::kAgeThresholds:
      out->attr_index = kAgePredicateAttrIndex;
      out->attr_len = 1;
      out->type = static_cast<uint8_t>(PresentationPolicyType::kAgeThresholds);
      out->cutoff_count = static_cast<uint8_t>(request.policy.age_thresholds.size());
      for (size_t i = 0; i < request.policy.age_thresholds.size(); ++i) {
        out->cutoff_dates[i] =
            AddYears(request.now_yyyymmdd, -request.policy.age_thresholds[i]);
      }
      return true;
  }
  if (err != nullptr) {
    *err = "unknown policy type";
  }
  return false;
}

bool BuildPresentationRequest(const PresentationPolicy& policy,
                              const std::string& now_yyyymmdd,
                              const std::vector<uint8_t>& transcript,
                              PresentationRequest* out,
                              std::string* err) {
  if (!IsDate(now_yyyymmdd)) {
    if (err != nullptr) {
      *err = "now must be YYYYMMDD";
    }
    return false;
  }
  if (!ValidatePresentationPolicy(policy, err)) {
    return false;
  }
  out->policy = policy;
  out->transcript_bytes = transcript;
  out->now_yyyymmdd = now_yyyymmdd;

  CompiledPresentationPolicy compiled;
  return CompilePresentationPolicy(*out, &compiled, err);
}

bool BuildPresentationRequest(const std::string& claim_name,
                              const std::string& now_yyyymmdd,
                              const std::vector<uint8_t>& transcript,
                              PresentationRequest* out,
                              std::string* err) {
  PresentationPolicy policy;
  if (!ParseLegacyClaimPolicy(claim_name, &policy, err)) {
    return false;
  }
  return BuildPresentationRequest(policy, now_yyyymmdd, transcript, out, err);
}

bool BuildFreshPresentationRequest(const PresentationPolicy& policy,
                                   const std::string& now_yyyymmdd,
                                   PresentationRequest* out,
                                   std::string* err) {
  std::vector<uint8_t> transcript(kRequestTranscriptSize);
  rand_bytes(transcript.data(), transcript.size());
  return BuildPresentationRequest(policy, now_yyyymmdd, transcript, out, err);
}

bool BuildFreshPresentationRequest(const std::string& claim_name,
                                   const std::string& now_yyyymmdd,
                                   PresentationRequest* out,
                                   std::string* err) {
  std::vector<uint8_t> transcript(kRequestTranscriptSize);
  rand_bytes(transcript.data(), transcript.size());
  return BuildPresentationRequest(claim_name, now_yyyymmdd, transcript, out,
                                  err);
}

bool BuildOpenedAttributeForValue(const PresentationPolicy& policy,
                                  const std::vector<uint8_t>& disclosed_value,
                                  SmallOpenedAttribute* attr,
                                  std::string* err) {
  PresentationRequest request;
  request.policy = policy;
  request.now_yyyymmdd = "20000101";
  CompiledPresentationPolicy compiled;
  if (!CompilePresentationPolicy(request, &compiled, err)) {
    return false;
  }
  if (disclosed_value.size() != compiled.attr_len) {
    if (err != nullptr) {
      *err = "unexpected disclosed value length for policy: " +
             PolicyDisplayName(policy);
    }
    return false;
  }
  *attr = SmallOpenedAttribute(compiled.attr_index, compiled.attr_len,
                               disclosed_value.data(), disclosed_value.size());
  return true;
}

bool BuildOpenedAttributeForValue(const std::string& claim_name,
                                  const std::vector<uint8_t>& disclosed_value,
                                  SmallOpenedAttribute* attr,
                                  std::string* err) {
  PresentationPolicy policy;
  if (!ParseLegacyClaimPolicy(claim_name, &policy, err)) {
    return false;
  }
  return BuildOpenedAttributeForValue(policy, disclosed_value, attr, err);
}

bool ExtractDisclosedValueForRequest(const HolderCredential& credential,
                                     const PresentationRequest& request,
                                     std::vector<uint8_t>* value,
                                     std::string* err) {
  const size_t ind = 64;
  const size_t len = 8;
  if (credential.credential_bytes.size() < ind + len) {
    if (err != nullptr) {
      *err = "credential is too short for date_of_birth";
    }
    return false;
  }
  std::string dob(
      reinterpret_cast<const char*>(credential.credential_bytes.data() + ind),
      len);

  if (request.policy.type == PresentationPolicyType::kRevealDateOfBirth) {
    value->assign(credential.credential_bytes.begin() + ind,
                  credential.credential_bytes.begin() + ind + len);
    return true;
  }

  CompiledPresentationPolicy compiled;
  if (!CompilePresentationPolicy(request, &compiled, err)) {
    return false;
  }

  bool ok = false;
  if (request.policy.type == PresentationPolicyType::kAgeRange) {
    const bool lower_ok = DateLessOrEqual(dob, compiled.cutoff_dates[0]);
    const bool upper_too_old = DateLessOrEqual(dob, compiled.cutoff_dates[1]);
    ok = lower_ok && !upper_too_old;
  } else if (request.policy.type == PresentationPolicyType::kAgeThresholds) {
    ok = true;
    for (size_t i = 0; i < compiled.cutoff_count; ++i) {
      ok = ok && DateLessOrEqual(dob, compiled.cutoff_dates[i]);
    }
  } else {
    if (err != nullptr) {
      *err = "unsupported policy: " + PolicyDisplayName(request.policy);
    }
    return false;
  }

  value->assign(1, ok ? 0xf5 : 0xf4);
  return true;
}

std::string FormatDisclosedValue(const PresentationPolicy& policy,
                                 const std::vector<uint8_t>& value) {
  if (policy.type == PresentationPolicyType::kRevealDateOfBirth) {
    return std::string(value.begin(), value.end());
  }
  return BoolByteToString(value);
}

std::string FormatDisclosedValue(const std::string& claim_name,
                                 const std::vector<uint8_t>& value) {
  PresentationPolicy policy;
  std::string err;
  if (ParseLegacyClaimPolicy(claim_name, &policy, &err)) {
    return FormatDisclosedValue(policy, value);
  }
  return BoolByteToString(value);
}

}  // namespace proofs
