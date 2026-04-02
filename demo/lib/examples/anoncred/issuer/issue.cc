#include "examples/anoncred/issuer/issue.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "examples/anoncred/shared/device_key.h"
#include "examples/anoncred/shared/files.h"
#include "examples/anoncred/shared/small_demo.h"
#include "openssl/bn.h"
#include "openssl/ec.h"
#include "openssl/ecdsa.h"
#include "util/crypto.h"

namespace proofs {
namespace {

constexpr size_t kFieldLen = 32;
constexpr size_t kDateLenLocal = 8;
constexpr size_t kSmallFirstNamePos = 0;
constexpr size_t kSmallFamilyNamePos = 32;
constexpr size_t kSmallDateOfBirthPos = 64;
constexpr size_t kSmallValidFromPos = 84;
constexpr size_t kSmallValidUntilPos = 92;
constexpr size_t kSmallDevicePkXPos = 100;
constexpr size_t kSmallDevicePkYPos = 132;

bool IsDate(const std::string& s) {
  if (s.size() != kDateLenLocal) {
    return false;
  }
  for (char c : s) {
    if (c < '0' || c > '9') {
      return false;
    }
  }
  return true;
}

bool HexToFixedBytes(const std::string& hex, uint8_t* out, size_t out_len,
                     std::string* err) {
  const size_t prefix = hex.rfind("0x", 0) == 0 ? 2 : 0;
  if (hex.size() != prefix + 2 * out_len) {
    if (err != nullptr) {
      *err = "unexpected hex length";
    }
    return false;
  }
  auto nibble = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  for (size_t i = 0; i < out_len; ++i) {
    int hi = nibble(hex[prefix + 2 * i]);
    int lo = nibble(hex[prefix + 2 * i + 1]);
    if (hi < 0 || lo < 0) {
      if (err != nullptr) {
        *err = "invalid hex string";
      }
      return false;
    }
    out[i] = static_cast<uint8_t>((hi << 4) | lo);
  }
  return true;
}

void WritePaddedField(std::vector<uint8_t>* doc, size_t pos, size_t len,
                      const std::string& value) {
  std::fill(doc->begin() + static_cast<std::ptrdiff_t>(pos),
            doc->begin() + static_cast<std::ptrdiff_t>(pos + len), 0);
  const size_t n = std::min(len, value.size());
  std::memcpy(doc->data() + pos, value.data(), n);
}

std::string ReadAsciiField(const std::vector<uint8_t>& doc, size_t pos,
                           size_t len) {
  return std::string(reinterpret_cast<const char*>(doc.data() + pos), len);
}

std::string HexPrefixed(const uint8_t* bytes, size_t n) {
  std::vector<char> buf(2 * n + 3, '\0');
  buf[0] = '0';
  buf[1] = 'x';
  hex_to_str(buf.data() + 2, bytes, n);
  return std::string(buf.data());
}

bool GenerateIssuerKeyAndSign(const std::vector<uint8_t>& credential_bytes,
                              HolderCredential* credential,
                              IssuerPublicBundle* issuer_public,
                              std::string* err) {
  EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
  EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
  if (eckey == nullptr || group == nullptr) {
    if (err != nullptr) {
      *err = "failed to initialize EC key";
    }
    if (eckey != nullptr) EC_KEY_free(eckey);
    if (group != nullptr) EC_GROUP_free(group);
    return false;
  }

  bool ok = false;
  do {
    if (!EC_KEY_generate_key(eckey)) {
      if (err != nullptr) {
        *err = "failed to generate issuer key";
      }
      break;
    }

    uint8_t hash[kSHA256DigestSize];
    proofs::SHA256 sha;
    sha.Update(credential_bytes.data(), credential_bytes.size());
    sha.DigestData(hash);

    ECDSA_SIG* sig = ECDSA_do_sign(hash, sizeof(hash), eckey);
    if (sig == nullptr) {
      if (err != nullptr) {
        *err = "failed to sign credential";
      }
      break;
    }

    const BIGNUM* br = ECDSA_SIG_get0_r(sig);
    const BIGNUM* bs = ECDSA_SIG_get0_s(sig);
    std::array<uint8_t, 32> r_bytes{};
    std::array<uint8_t, 32> s_bytes{};
    if (BN_bn2binpad(br, r_bytes.data(), r_bytes.size()) !=
            static_cast<int>(r_bytes.size()) ||
        BN_bn2binpad(bs, s_bytes.data(), s_bytes.size()) !=
            static_cast<int>(s_bytes.size())) {
      if (err != nullptr) {
        *err = "failed to serialize signature";
      }
      ECDSA_SIG_free(sig);
      break;
    }

    const EC_POINT* pub_key = EC_KEY_get0_public_key(eckey);
    uint8_t pub[65];
    if (EC_POINT_point2oct(group, pub_key, POINT_CONVERSION_UNCOMPRESSED, pub,
                           sizeof(pub), nullptr) != sizeof(pub)) {
      if (err != nullptr) {
        *err = "failed to serialize issuer public key";
      }
      ECDSA_SIG_free(sig);
      break;
    }

    credential->issuer_pkx_hex = HexPrefixed(&pub[1], 32);
    credential->issuer_pky_hex = HexPrefixed(&pub[33], 32);
    credential->issuer_sig_r_hex = HexPrefixed(r_bytes.data(), r_bytes.size());
    credential->issuer_sig_s_hex = HexPrefixed(s_bytes.data(), s_bytes.size());
    issuer_public->issuer_pkx_hex = credential->issuer_pkx_hex;
    issuer_public->issuer_pky_hex = credential->issuer_pky_hex;
    ECDSA_SIG_free(sig);
    ok = true;
  } while (false);

  EC_KEY_free(eckey);
  EC_GROUP_free(group);
  return ok;
}

bool ApplyHolderPublicKey(HolderCredential* credential,
                          const HolderKeyMaterial& holder_key,
                          std::string* err) {
  std::array<uint8_t, 32> pkx{};
  std::array<uint8_t, 32> pky{};
  if (!HexToFixedBytes(holder_key.device_pkx_hex, pkx.data(), pkx.size(), err) ||
      !HexToFixedBytes(holder_key.device_pky_hex, pky.data(), pky.size(), err)) {
    return false;
  }
  std::memcpy(credential->credential_bytes.data() + kSmallDevicePkXPos,
              pkx.data(), pkx.size());
  std::memcpy(credential->credential_bytes.data() + kSmallDevicePkYPos,
              pky.data(), pky.size());
  return true;
}

bool ApplyIssueOptions(HolderCredential* credential,
                       IssuerPublicBundle* issuer_public,
                       const IssueOptions& options, std::string* err) {
  if (!options.first_name.empty() && options.first_name.size() > kFieldLen) {
    if (err != nullptr) *err = "first_name must be at most 32 bytes";
    return false;
  }
  if (!options.family_name.empty() && options.family_name.size() > kFieldLen) {
    if (err != nullptr) *err = "family_name must be at most 32 bytes";
    return false;
  }
  if (!options.date_of_birth_yyyymmdd.empty() &&
      !IsDate(options.date_of_birth_yyyymmdd)) {
    if (err != nullptr) *err = "date_of_birth must be YYYYMMDD";
    return false;
  }
  if (!options.valid_from_yyyymmdd.empty() &&
      !IsDate(options.valid_from_yyyymmdd)) {
    if (err != nullptr) *err = "valid_from must be YYYYMMDD";
    return false;
  }
  if (!options.valid_until_yyyymmdd.empty() &&
      !IsDate(options.valid_until_yyyymmdd)) {
    if (err != nullptr) *err = "valid_until must be YYYYMMDD";
    return false;
  }
  if (!options.first_name.empty()) {
    WritePaddedField(&credential->credential_bytes, kSmallFirstNamePos,
                     kFieldLen, options.first_name);
  }
  if (!options.family_name.empty()) {
    WritePaddedField(&credential->credential_bytes, kSmallFamilyNamePos,
                     kFieldLen, options.family_name);
  }
  if (!options.date_of_birth_yyyymmdd.empty()) {
    WritePaddedField(&credential->credential_bytes, kSmallDateOfBirthPos,
                     kDateLenLocal, options.date_of_birth_yyyymmdd);
  }
  if (!options.valid_from_yyyymmdd.empty()) {
    WritePaddedField(&credential->credential_bytes, kSmallValidFromPos,
                     kDateLenLocal, options.valid_from_yyyymmdd);
  }
  if (!options.valid_until_yyyymmdd.empty()) {
    WritePaddedField(&credential->credential_bytes, kSmallValidUntilPos,
                     kDateLenLocal, options.valid_until_yyyymmdd);
  }
  return GenerateIssuerKeyAndSign(credential->credential_bytes, credential,
                                  issuer_public, err);
}

}  // namespace

bool RunIssueCommand(uint32_t example_id, const std::filesystem::path& out_dir,
                     const IssueOptions& options, std::string* err) {
  if (options.holder_key_dir.empty()) {
    if (err != nullptr) {
      *err = "holder_key_dir is required";
    }
    return false;
  }

  HolderCredential credential;
  IssuerPublicBundle issuer_public;
  HolderKeyMaterial holder_key;
  if (!MaterializeExampleBundleFromExample(example_id, &credential,
                                           &issuer_public, err) ||
      !ReadHolderKeyMaterialDir(std::filesystem::path(options.holder_key_dir),
                                &holder_key, err) ||
      !ValidateHolderKeyMaterial(holder_key, err) ||
      !ApplyHolderPublicKey(&credential, holder_key, err) ||
      !ApplyIssueOptions(&credential, &issuer_public, options, err)) {
    return false;
  }
  return WriteHolderCredentialDir(out_dir / "holder", credential, err) &&
         WriteIssuerPublicBundleDir(out_dir / "issuer_public", issuer_public,
                                    err);
}

}  // namespace proofs
