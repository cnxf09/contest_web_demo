#include "examples/delegation_demo/shared/revocation.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

#include "examples/delegation_demo/shared/delegation_crypto.h"
#include "examples/mdoc_anoncred/shared/crypto.h"
#include "examples/mdoc_anoncred/shared/files.h"
#include "util/crypto.h"

namespace proofs {
namespace {

static constexpr uint8_t kRevIdDomain[] = {
    'Z', 'K', 'R', 'E', 'V', 'I', 'D', '1'};
static constexpr uint8_t kRevStatusDomain[] = {
    'Z', 'K', 'R', 'E', 'V', 'S', 'T', '1'};

std::string Trim(std::string s) {
  while (!s.empty() && (s.back() == '\n' || s.back() == '\r' ||
                        s.back() == ' ' || s.back() == '\t')) {
    s.pop_back();
  }
  return s;
}

std::string ExtractJsonString(const std::string& json,
                              const std::string& key) {
  const std::string token = "\"" + key + "\"";
  const size_t kpos = json.find(token);
  if (kpos == std::string::npos) return "";
  size_t cur = kpos + token.size();
  while (cur < json.size() &&
         (json[cur] == ' ' || json[cur] == ':' || json[cur] == '\n' ||
          json[cur] == '\r' || json[cur] == '\t')) {
    ++cur;
  }
  if (cur >= json.size() || json[cur] != '"') return "";
  const size_t start = cur + 1;
  const size_t end = json.find('"', start);
  if (end == std::string::npos) return "";
  return json.substr(start, end - start);
}

bool ExtractJsonBool(const std::string& json, const std::string& key,
                     bool* out) {
  const std::string token = "\"" + key + "\"";
  const size_t kpos = json.find(token);
  if (kpos == std::string::npos) return false;
  size_t cur = json.find(':', kpos + token.size());
  if (cur == std::string::npos) return false;
  ++cur;
  while (cur < json.size() &&
         (json[cur] == ' ' || json[cur] == '\n' || json[cur] == '\r' ||
          json[cur] == '\t')) {
    ++cur;
  }
  if (json.compare(cur, 4, "true") == 0) {
    *out = true;
    return true;
  }
  if (json.compare(cur, 5, "false") == 0) {
    *out = false;
    return true;
  }
  return false;
}

uint64_t ExtractJsonU64(const std::string& json, const std::string& key) {
  const std::string token = "\"" + key + "\"";
  const size_t kpos = json.find(token);
  if (kpos == std::string::npos) return 0;
  size_t cur = json.find(':', kpos + token.size());
  if (cur == std::string::npos) return 0;
  ++cur;
  while (cur < json.size() &&
         (json[cur] == ' ' || json[cur] == '\n' || json[cur] == '\r' ||
          json[cur] == '\t')) {
    ++cur;
  }
  size_t end = cur;
  while (end < json.size() && json[end] >= '0' && json[end] <= '9') ++end;
  if (end == cur) return 0;
  return static_cast<uint64_t>(std::stoull(json.substr(cur, end - cur)));
}

bool BuildRevocationStatusDigest(const RevocationStatus& status,
                                 std::vector<uint8_t>* digest,
                                 std::string* err) {
  std::vector<uint8_t> rev_id;
  if (!HexToBytes(status.rev_id_hex, &rev_id, err)) return false;
  if (rev_id.size() != 32) {
    if (err != nullptr) *err = "revocation id must be 32 bytes";
    return false;
  }
  if (status.expires.size() != 20) {
    if (err != nullptr) *err = "revocation status expires must be 20 bytes";
    return false;
  }
  std::vector<uint8_t> msg;
  msg.insert(msg.end(), std::begin(kRevStatusDomain),
             std::end(kRevStatusDomain));
  msg.insert(msg.end(), rev_id.begin(), rev_id.end());
  for (int i = 7; i >= 0; --i) {
    msg.push_back(static_cast<uint8_t>(status.epoch >> (8 * i)));
  }
  msg.insert(msg.end(), status.expires.begin(), status.expires.end());
  msg.push_back(status.revoked ? 1 : 0);
  return Sha256Digest(msg.data(), msg.size(), digest);
}

}  // namespace

std::string ComputeRevocationIdHex(const std::string& device_pkx_hex,
                                   const std::string& device_pky_hex,
                                   std::string* err) {
  std::vector<uint8_t> pkx;
  std::vector<uint8_t> pky;
  if (!HexToBytes(Trim(device_pkx_hex), &pkx, err) ||
      !HexToBytes(Trim(device_pky_hex), &pky, err)) {
    return "";
  }
  if (pkx.size() != 32 || pky.size() != 32) {
    if (err != nullptr) *err = "device public key coordinates must be 32 bytes";
    return "";
  }
  std::vector<uint8_t> msg;
  msg.insert(msg.end(), std::begin(kRevIdDomain), std::end(kRevIdDomain));
  msg.insert(msg.end(), pkx.begin(), pkx.end());
  msg.insert(msg.end(), pky.begin(), pky.end());
  std::vector<uint8_t> digest;
  Sha256Digest(msg.data(), msg.size(), &digest);
  return HexPrefixed(digest.data(), digest.size());
}

bool CreateRevocationStatus(const std::string& revocation_sk_hex,
                            const std::string& device_pkx_hex,
                            const std::string& device_pky_hex,
                            uint64_t epoch, const std::string& expires,
                            bool revoked, RevocationStatus* status,
                            std::string* err) {
  status->present = true;
  status->rev_id_hex =
      ComputeRevocationIdHex(device_pkx_hex, device_pky_hex, err);
  if (status->rev_id_hex.empty()) return false;
  status->epoch = epoch;
  status->expires = expires;
  status->revoked = revoked;
  std::vector<uint8_t> digest;
  if (!BuildRevocationStatusDigest(*status, &digest, err)) return false;
  std::vector<uint8_t> sig;
  if (!SignSha256DigestP256(revocation_sk_hex, digest, &sig, err)) {
    return false;
  }
  status->sig_hex = HexPrefixed(sig.data(), sig.size());
  return true;
}

bool VerifyRevocationStatus(const RevocationStatus& status,
                            const std::string& revocation_pkx_hex,
                            const std::string& revocation_pky_hex,
                            const std::string& device_pkx_hex,
                            const std::string& device_pky_hex,
                            const std::string& now_iso8601,
                            std::string* err) {
  if (!status.present) {
    if (err != nullptr) *err = "revocation status missing";
    return false;
  }
  if (revocation_pkx_hex.empty() || revocation_pky_hex.empty()) {
    if (err != nullptr) *err = "revocation public key missing";
    return false;
  }
  const std::string expected_id =
      ComputeRevocationIdHex(device_pkx_hex, device_pky_hex, err);
  if (expected_id.empty()) return false;
  if (status.rev_id_hex != expected_id) {
    if (err != nullptr) *err = "revocation id does not match credential";
    return false;
  }
  if (status.revoked) {
    if (err != nullptr) *err = "credential is revoked";
    return false;
  }
  if (status.expires <= now_iso8601) {
    if (err != nullptr) *err = "revocation status expired";
    return false;
  }
  std::vector<uint8_t> digest;
  if (!BuildRevocationStatusDigest(status, &digest, err)) return false;
  const std::string digest_hex = HexPrefixed(digest.data(), digest.size());
  if (!VerifyDelegationSig(revocation_pkx_hex, revocation_pky_hex, digest_hex,
                           status.sig_hex, err)) {
    if (err != nullptr) *err = "revocation status signature invalid: " + *err;
    return false;
  }
  return true;
}

bool WriteRevocationStatusJson(const std::filesystem::path& path,
                               const RevocationStatus& status,
                               std::string* err) {
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"rev_id\": \"" << status.rev_id_hex << "\",\n";
  oss << "  \"epoch\": " << status.epoch << ",\n";
  oss << "  \"expires\": \"" << status.expires << "\",\n";
  oss << "  \"revoked\": " << (status.revoked ? "true" : "false") << ",\n";
  oss << "  \"sig\": \"" << status.sig_hex << "\"\n";
  oss << "}\n";
  return WriteStringFile(path, oss.str(), err);
}

bool ReadRevocationStatusJson(const std::filesystem::path& path,
                              RevocationStatus* status, std::string* err) {
  std::ifstream in(path);
  if (!in) {
    if (err != nullptr) *err = "failed to open: " + path.string();
    return false;
  }
  const std::string json((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
  status->present = true;
  status->rev_id_hex = ExtractJsonString(json, "rev_id");
  status->epoch = ExtractJsonU64(json, "epoch");
  status->expires = ExtractJsonString(json, "expires");
  status->sig_hex = ExtractJsonString(json, "sig");
  if (!ExtractJsonBool(json, "revoked", &status->revoked)) {
    if (err != nullptr) *err = "revocation_status.json missing revoked";
    return false;
  }
  if (status->rev_id_hex.empty() || status->expires.empty() ||
      status->sig_hex.empty()) {
    if (err != nullptr) *err = "revocation_status.json missing required fields";
    return false;
  }
  return true;
}

}  // namespace proofs
