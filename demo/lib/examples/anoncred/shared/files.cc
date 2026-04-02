#include "examples/anoncred/shared/files.h"

#include <fstream>
#include <iterator>
#include <sstream>

#include "examples/anoncred/shared/request.h"

namespace proofs {
namespace {

bool EnsureDir(const std::filesystem::path& dir, std::string* err) {
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    if (err != nullptr) {
      *err = "failed to create directory: " + dir.string() + ": " +
             ec.message();
    }
    return false;
  }
  return true;
}

}  // namespace

bool WriteBytesFile(const std::filesystem::path& path,
                    const std::vector<uint8_t>& bytes, std::string* err) {
  if (!EnsureDir(path.parent_path(), err)) {
    return false;
  }
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (err != nullptr) {
      *err = "failed to open file for write: " + path.string();
    }
    return false;
  }
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  if (!out.good()) {
    if (err != nullptr) {
      *err = "failed to write file: " + path.string();
    }
    return false;
  }
  return true;
}

bool ReadBytesFile(const std::filesystem::path& path, std::vector<uint8_t>* out,
                   std::string* err) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (err != nullptr) {
      *err = "failed to open file for read: " + path.string();
    }
    return false;
  }
  out->assign(std::istreambuf_iterator<char>(in),
              std::istreambuf_iterator<char>());
  if (!in.good() && !in.eof()) {
    if (err != nullptr) {
      *err = "failed to read file: " + path.string();
    }
    return false;
  }
  return true;
}

bool WriteStringFile(const std::filesystem::path& path, const std::string& text,
                     std::string* err) {
  std::vector<uint8_t> bytes(text.begin(), text.end());
  return WriteBytesFile(path, bytes, err);
}

bool ReadStringFile(const std::filesystem::path& path, std::string* out,
                    std::string* err) {
  std::vector<uint8_t> bytes;
  if (!ReadBytesFile(path, &bytes, err)) {
    return false;
  }
  out->assign(bytes.begin(), bytes.end());
  return true;
}

bool WriteHolderCredentialDir(const std::filesystem::path& dir,
                              const HolderCredential& credential,
                              std::string* err) {
  if (!EnsureDir(dir, err)) {
    return false;
  }
  return WriteBytesFile(dir / "credential.bin", credential.credential_bytes,
                        err) &&
         WriteStringFile(dir / "issuer_pkx.txt", credential.issuer_pkx_hex,
                         err) &&
         WriteStringFile(dir / "issuer_pky.txt", credential.issuer_pky_hex,
                         err) &&
         WriteStringFile(dir / "issuer_sig_r.txt", credential.issuer_sig_r_hex,
                         err) &&
         WriteStringFile(dir / "issuer_sig_s.txt", credential.issuer_sig_s_hex,
                         err);
}

bool ReadHolderCredentialDir(const std::filesystem::path& dir,
                             HolderCredential* credential, std::string* err) {
  return ReadBytesFile(dir / "credential.bin", &credential->credential_bytes,
                       err) &&
         ReadStringFile(dir / "issuer_pkx.txt", &credential->issuer_pkx_hex,
                        err) &&
         ReadStringFile(dir / "issuer_pky.txt", &credential->issuer_pky_hex,
                        err) &&
         ReadStringFile(dir / "issuer_sig_r.txt",
                        &credential->issuer_sig_r_hex, err) &&
         ReadStringFile(dir / "issuer_sig_s.txt",
                        &credential->issuer_sig_s_hex, err);
}

bool WriteIssuerPublicBundleDir(const std::filesystem::path& dir,
                                const IssuerPublicBundle& issuer_public,
                                std::string* err) {
  if (!EnsureDir(dir, err)) {
    return false;
  }
  return WriteStringFile(dir / "issuer_pkx.txt", issuer_public.issuer_pkx_hex,
                         err) &&
         WriteStringFile(dir / "issuer_pky.txt", issuer_public.issuer_pky_hex,
                         err) &&
         WriteStringFile(dir / "example_id.txt",
                         std::to_string(issuer_public.example_id), err);
}

bool ReadIssuerPublicBundleDir(const std::filesystem::path& dir,
                               IssuerPublicBundle* issuer_public,
                               std::string* err) {
  std::string example_id;
  if (!ReadStringFile(dir / "issuer_pkx.txt", &issuer_public->issuer_pkx_hex,
                      err) ||
      !ReadStringFile(dir / "issuer_pky.txt", &issuer_public->issuer_pky_hex,
                      err) ||
      !ReadStringFile(dir / "example_id.txt", &example_id, err)) {
    return false;
  }
  issuer_public->example_id = static_cast<uint32_t>(std::stoul(example_id));
  return true;
}

bool WriteHolderKeyMaterialDir(const std::filesystem::path& dir,
                               const HolderKeyMaterial& holder_key,
                               std::string* err) {
  if (!EnsureDir(dir, err)) {
    return false;
  }
  return WriteStringFile(dir / "device_sk.txt", holder_key.device_sk_hex, err) &&
         WriteStringFile(dir / "device_pkx.txt", holder_key.device_pkx_hex,
                         err) &&
         WriteStringFile(dir / "device_pky.txt", holder_key.device_pky_hex,
                         err);
}

bool ReadHolderKeyMaterialDir(const std::filesystem::path& dir,
                              HolderKeyMaterial* holder_key,
                              std::string* err) {
  return ReadStringFile(dir / "device_sk.txt", &holder_key->device_sk_hex,
                        err) &&
         ReadStringFile(dir / "device_pkx.txt", &holder_key->device_pkx_hex,
                        err) &&
         ReadStringFile(dir / "device_pky.txt", &holder_key->device_pky_hex,
                        err);
}

bool WritePresentationRequestDir(const std::filesystem::path& dir,
                                 const PresentationRequest& request,
                                 std::string* err) {
  if (!EnsureDir(dir, err)) {
    return false;
  }
  std::ostringstream thresholds;
  for (size_t i = 0; i < request.policy.age_thresholds.size(); ++i) {
    if (i != 0) {
      thresholds << ",";
    }
    thresholds << static_cast<unsigned>(request.policy.age_thresholds[i]);
  }
  return WriteStringFile(dir / "policy_name.txt",
                         PolicyDisplayName(request.policy), err) &&
         WriteStringFile(
             dir / "policy_type.txt",
             request.policy.type == PresentationPolicyType::kRevealDateOfBirth
                 ? kDateOfBirthClaim
                 : (request.policy.type == PresentationPolicyType::kAgeRange
                        ? kAgeRangePolicy
                        : kAgeThresholdsPolicy),
             err) &&
         WriteBytesFile(dir / "transcript.bin", request.transcript_bytes, err) &&
         WriteStringFile(dir / "now.txt", request.now_yyyymmdd, err) &&
         WriteStringFile(dir / "policy_min_age.txt",
                         std::to_string(request.policy.min_age), err) &&
         WriteStringFile(dir / "policy_max_age.txt",
                         std::to_string(request.policy.max_age), err) &&
         WriteStringFile(dir / "policy_thresholds.txt", thresholds.str(), err);
}

bool ReadPresentationRequestDir(const std::filesystem::path& dir,
                                PresentationRequest* request,
                                std::string* err) {
  std::string policy_type;
  std::string min_age;
  std::string max_age;
  std::string thresholds;
  if (!ReadStringFile(dir / "policy_type.txt", &policy_type, err) ||
      !ReadBytesFile(dir / "transcript.bin", &request->transcript_bytes, err) ||
      !ReadStringFile(dir / "now.txt", &request->now_yyyymmdd, err) ||
      !ReadStringFile(dir / "policy_min_age.txt", &min_age, err) ||
      !ReadStringFile(dir / "policy_max_age.txt", &max_age, err) ||
      !ReadStringFile(dir / "policy_thresholds.txt", &thresholds, err)) {
    return false;
  }
  if (!ParsePolicyType(policy_type, &request->policy.type, err)) {
    return false;
  }
  request->policy.min_age =
      static_cast<uint8_t>(min_age.empty() ? 0 : std::stoul(min_age));
  request->policy.max_age =
      static_cast<uint8_t>(max_age.empty() ? 0 : std::stoul(max_age));
  request->policy.age_thresholds.clear();
  if (!thresholds.empty()) {
    std::stringstream ss(thresholds);
    std::string item;
    while (std::getline(ss, item, ',')) {
      request->policy.age_thresholds.push_back(
          static_cast<uint8_t>(std::stoul(item)));
    }
  }
  return true;
}

bool WritePresentationProofDir(const std::filesystem::path& dir,
                               const PresentationProof& proof,
                               std::string* err) {
  if (!EnsureDir(dir, err)) {
    return false;
  }
  return WriteBytesFile(dir / "proof.bin", proof.proof_bytes, err) &&
         WriteStringFile(dir / "claim.txt", proof.claim_name, err) &&
         WriteBytesFile(dir / "disclosed_value.bin", proof.disclosed_value,
                        err);
}

bool ReadPresentationProofDir(const std::filesystem::path& dir,
                              PresentationProof* proof, std::string* err) {
  return ReadBytesFile(dir / "proof.bin", &proof->proof_bytes, err) &&
         ReadStringFile(dir / "claim.txt", &proof->claim_name, err) &&
         ReadBytesFile(dir / "disclosed_value.bin", &proof->disclosed_value,
                       err);
}

}  // namespace proofs
