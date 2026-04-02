#include "examples/delegation_demo/verifier/verify.h"

#include <sstream>
#include <string>

#include "examples/delegation_demo/shared/delegation_crypto.h"
#include "examples/delegation_demo/shared/delegation_files.h"
#include "examples/delegation_demo/shared/types.h"
#include "examples/mdoc_anoncred/shared/files.h"
#include "examples/mdoc_anoncred/verifier/verify.h"

namespace proofs {

// ----------------------------------------------------------------
// D-1: 生成验证请求
// ----------------------------------------------------------------

bool RunDelegationRequestCommand(
    const std::filesystem::path& issuer_public_dir,
    const std::vector<std::string>& claim_aliases,
    const std::filesystem::path& out_dir,
    std::string* err) {
  return RunMdocRequestCommand(issuer_public_dir, claim_aliases, out_dir, err);
}

// ----------------------------------------------------------------
// D-2: 验证展示
// ----------------------------------------------------------------

bool RunDelegationVerifyCommand(
    const std::filesystem::path& issuer_public_dir,
    const std::filesystem::path& request_dir,
    const std::filesystem::path& presentation_dir,
    DelegationVerificationResult* result,
    std::string* err) {
  // 1. ZK 证明验证（约束①-⑥）
  bool zk_ok = false;
  std::string zk_err;
  if (!RunMdocVerifyCommand(issuer_public_dir, request_dir, presentation_dir,
                            &zk_ok, &zk_err)) {
    if (err != nullptr) *err = "ZK verify error: " + zk_err;
    return false;
  }
  result->zk_proof_ok = zk_ok;

  // 2. 读取 delegation_token.json
  std::string agent_pkx, agent_pky, del_msg, del_sig, device_pkx, device_pky;
  Policy policy;
  if (!ReadDelegationTokenJson(presentation_dir / "delegation_token.json",
                               &agent_pkx, &agent_pky, &del_msg, &del_sig,
                               &device_pkx, &device_pky, &policy, err)) {
    return false;
  }

  // 3. 委托签名验证（约束⑦）
  std::string sig_err;
  result->delegation_sig_ok =
      VerifyDelegationSig(device_pkx, device_pky, del_msg, del_sig, &sig_err);

  // 4. 重新计算 del_msg 确保 policy 未被篡改
  if (result->delegation_sig_ok) {
    std::string recomputed_msg;
    std::string compute_err;
    if (ComputeDelegationMsg(agent_pkx, agent_pky, policy, &recomputed_msg,
                             &compute_err)) {
      if (recomputed_msg != del_msg) {
        result->delegation_sig_ok = false;
      }
    } else {
      result->delegation_sig_ok = false;
    }
  }

  // 5. 策略 claim 覆盖检查（约束⑧）
  ReaderRequest request;
  if (!ReadReaderRequestDir(request_dir, &request, err)) {
    return false;
  }

  result->policy_claims_ok = true;
  for (const auto& claim : request.claims) {
    if (!PolicyAllowsClaim(policy, claim.alias)) {
      result->policy_claims_ok = false;
      break;
    }
  }

  // 6. 策略过期检查（约束⑨）
  result->policy_not_expired = !PolicyExpired(policy, request.now_iso8601);

  // 7. 综合结果
  result->overall_ok = result->zk_proof_ok && result->delegation_sig_ok &&
                       result->policy_claims_ok && result->policy_not_expired;

  std::ostringstream msg;
  msg << "ZK proof: " << (result->zk_proof_ok ? "PASS" : "FAIL") << "\n";
  msg << "Delegation sig: "
      << (result->delegation_sig_ok ? "PASS" : "FAIL") << "\n";
  msg << "Policy claims: "
      << (result->policy_claims_ok ? "PASS" : "FAIL") << "\n";
  msg << "Policy expiry: "
      << (result->policy_not_expired ? "PASS" : "FAIL") << "\n";
  msg << "Overall: " << (result->overall_ok ? "ACCEPT" : "REJECT");
  result->message = msg.str();

  return true;
}

}  // namespace proofs
