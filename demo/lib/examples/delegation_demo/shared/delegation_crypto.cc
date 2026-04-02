#include "examples/delegation_demo/shared/delegation_crypto.h"

#include <algorithm>
#include <vector>

#include "examples/mdoc_anoncred/shared/crypto.h"
#include "util/crypto.h"
#include "openssl/bn.h"
#include "openssl/ec.h"
#include "openssl/ecdsa.h"

namespace proofs {

// ----------------------------------------------------------------
// 规范化 JSON 编码（键按字母序）
// ----------------------------------------------------------------
std::string CanonicalPolicyJson(const Policy& policy) {
  // 键按字母序：agent_id, allowed_claims, created, expires
  std::string j = "{";
  // agent_id
  j += "\"agent_id\":\"" + policy.agent_id + "\"";
  // allowed_claims
  j += ",\"allowed_claims\":[";
  for (size_t i = 0; i < policy.allowed_claims.size(); ++i) {
    if (i > 0) j += ",";
    j += "\"" + policy.allowed_claims[i] + "\"";
  }
  j += "]";
  // created
  j += ",\"created\":\"" + policy.created + "\"";
  // expires
  j += ",\"expires\":\"" + policy.expires + "\"";
  j += "}";
  return j;
}

// ----------------------------------------------------------------
// 委托消息计算
// ----------------------------------------------------------------
bool ComputeDelegationMsg(const std::string& agent_pkx_hex,
                          const std::string& agent_pky_hex,
                          const Policy& policy,
                          std::string* out_msg_hex,
                          std::string* err) {
  std::vector<uint8_t> pkx_bytes, pky_bytes;
  if (!HexToBytes(agent_pkx_hex, &pkx_bytes, err) ||
      !HexToBytes(agent_pky_hex, &pky_bytes, err)) {
    return false;
  }
  if (pkx_bytes.size() != 32 || pky_bytes.size() != 32) {
    if (err != nullptr) {
      *err = "agent public key coordinates must be 32 bytes each";
    }
    return false;
  }

  const std::string policy_json = CanonicalPolicyJson(policy);
  const std::vector<uint8_t> policy_bytes(policy_json.begin(), policy_json.end());

  // msg_data = pk_ag_x (32) || pk_ag_y (32) || policy_bytes
  std::vector<uint8_t> msg_data;
  msg_data.reserve(64 + policy_bytes.size());
  msg_data.insert(msg_data.end(), pkx_bytes.begin(), pkx_bytes.end());
  msg_data.insert(msg_data.end(), pky_bytes.begin(), pky_bytes.end());
  msg_data.insert(msg_data.end(), policy_bytes.begin(), policy_bytes.end());

  std::vector<uint8_t> digest;
  if (!Sha256Digest(msg_data.data(), msg_data.size(), &digest)) {
    if (err != nullptr) {
      *err = "SHA256 computation failed";
    }
    return false;
  }

  *out_msg_hex = HexPrefixed(digest.data(), digest.size());
  return true;
}

// ----------------------------------------------------------------
// 委托签名
// ----------------------------------------------------------------
bool SignDelegation(const std::string& sk_hex,
                    const std::string& msg_hex,
                    std::string* out_sig_hex,
                    std::string* err) {
  std::vector<uint8_t> msg_bytes;
  if (!HexToBytes(msg_hex, &msg_bytes, err)) {
    return false;
  }
  if (msg_bytes.size() != 32) {
    if (err != nullptr) {
      *err = "delegation message must be a 32-byte SHA256 digest";
    }
    return false;
  }

  std::vector<uint8_t> sig_rs;
  if (!SignSha256DigestP256(sk_hex, msg_bytes, &sig_rs, err)) {
    return false;
  }
  // sig_rs 是 64 字节 r(32)||s(32)
  *out_sig_hex = HexPrefixed(sig_rs.data(), sig_rs.size());
  return true;
}

// ----------------------------------------------------------------
// 委托签名验证（用于模块 B 自测）
// ----------------------------------------------------------------
bool VerifyDelegationSig(const std::string& pkx_hex,
                         const std::string& pky_hex,
                         const std::string& msg_hex,
                         const std::string& sig_hex,
                         std::string* err) {
  std::vector<uint8_t> pkx_bytes, pky_bytes, msg_bytes, sig_bytes;
  if (!HexToBytes(pkx_hex, &pkx_bytes, err) ||
      !HexToBytes(pky_hex, &pky_bytes, err) ||
      !HexToBytes(msg_hex, &msg_bytes, err) ||
      !HexToBytes(sig_hex, &sig_bytes, err)) {
    return false;
  }
  if (pkx_bytes.size() != 32 || pky_bytes.size() != 32) {
    if (err != nullptr) { *err = "invalid public key size"; }
    return false;
  }
  if (msg_bytes.size() != 32) {
    if (err != nullptr) { *err = "message must be 32 bytes"; }
    return false;
  }
  if (sig_bytes.size() != 64) {
    if (err != nullptr) { *err = "signature must be 64 bytes (r||s)"; }
    return false;
  }

  EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
  EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
  if (group == nullptr || eckey == nullptr) {
    if (err != nullptr) { *err = "failed to init EC key"; }
    if (group) EC_GROUP_free(group);
    if (eckey) EC_KEY_free(eckey);
    return false;
  }

  bool ok = false;
  BIGNUM* bx = BN_bin2bn(pkx_bytes.data(), 32, nullptr);
  BIGNUM* by = BN_bin2bn(pky_bytes.data(), 32, nullptr);
  EC_POINT* pub = EC_POINT_new(group);
  if (bx && by && pub) {
    if (EC_POINT_set_affine_coordinates_GFp(group, pub, bx, by, nullptr) == 1 &&
        EC_KEY_set_public_key(eckey, pub) == 1) {
      BIGNUM* br = BN_bin2bn(sig_bytes.data(), 32, nullptr);
      BIGNUM* bs = BN_bin2bn(sig_bytes.data() + 32, 32, nullptr);
      ECDSA_SIG* sig = ECDSA_SIG_new();
      if (br && bs && sig) {
        ECDSA_SIG_set0(sig, br, bs);  // sig takes ownership of br, bs
        br = nullptr; bs = nullptr;
        int ret = ECDSA_do_verify(msg_bytes.data(),
                                  static_cast<int>(msg_bytes.size()),
                                  sig, eckey);
        ok = (ret == 1);
        if (!ok && err != nullptr) {
          *err = (ret == 0) ? "signature verification failed"
                            : "ECDSA_do_verify error";
        }
        ECDSA_SIG_free(sig);
      }
      if (br) BN_free(br);
      if (bs) BN_free(bs);
    }
  }

  if (bx) BN_free(bx);
  if (by) BN_free(by);
  if (pub) EC_POINT_free(pub);
  EC_KEY_free(eckey);
  EC_GROUP_free(group);
  return ok;
}

// ----------------------------------------------------------------
// 策略检查
// ----------------------------------------------------------------
bool PolicyAllowsClaim(const Policy& policy, const std::string& alias) {
  return std::find(policy.allowed_claims.begin(),
                   policy.allowed_claims.end(), alias) != policy.allowed_claims.end();
}

bool PolicyExpired(const Policy& policy, const std::string& now_iso8601) {
  // ISO 8601 字符串的字典序即时间序，直接比较即可
  return policy.expires <= now_iso8601;
}

}  // namespace proofs
