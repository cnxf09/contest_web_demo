#include "examples/delegation_demo/agent/present.h"

#include <iostream>
#include <string>

#include "examples/delegation_demo/shared/delegation_crypto.h"
#include "examples/delegation_demo/shared/delegation_files.h"
#include "examples/delegation_demo/shared/types.h"
#include "examples/mdoc_anoncred/shared/files.h"
#include "examples/mdoc_anoncred/shared/mdoc_demo.h"
#include "examples/mdoc_anoncred/shared/types.h"

namespace proofs {

bool RunAgentPresentCommand(const std::filesystem::path& delegation_dir,
                            const std::filesystem::path& issuer_public_dir,
                            const std::filesystem::path& request_dir,
                            const std::filesystem::path& out_dir,
                            std::string* err) {
  // 1. 读取委托材料
  HolderMdoc holder;
  std::string agent_pkx, agent_pky, agent_sk, del_msg, del_sig;
  Policy policy;
  if (!ReadDelegationDir(delegation_dir, &holder, &agent_pkx, &agent_pky,
                         &agent_sk, &del_msg, &del_sig, &policy, err)) {
    return false;
  }

  // 2. 读取颁发方公开信息
  MdocIssuerPublicBundle issuer_public;
  if (!ReadMdocIssuerPublicDir(issuer_public_dir, &issuer_public, err)) {
    return false;
  }

  // 3. 读取验证请求
  ReaderRequest request;
  if (!ReadReaderRequestDir(request_dir, &request, err)) {
    return false;
  }

  // 4. 约束⑧：策略 claim 检查
  for (const auto& claim : request.claims) {
    if (!PolicyAllowsClaim(policy, claim.alias)) {
      if (err != nullptr) {
        *err = "policy does not allow claim: " + claim.alias;
      }
      return false;
    }
  }

  // 5. 约束⑨：过期检查
  if (PolicyExpired(policy, request.now_iso8601)) {
    if (err != nullptr) {
      *err = "delegation expired (policy.expires=" + policy.expires +
             ", now=" + request.now_iso8601 + ")";
    }
    return false;
  }

  // 6. 生成 ZK 证明
  MdocPresentation presentation;
  if (!ProveMdocPresentation(holder, issuer_public, request, &presentation,
                             err)) {
    return false;
  }

  // 7. 写出标准 presentation 目录
  if (!WriteMdocPresentationDir(out_dir, presentation, err)) {
    return false;
  }

  // 8. 写出 delegation_token.json（供 Verifier 读取）
  if (!WriteDelegationTokenJson(out_dir / "delegation_token.json",
                                agent_pkx, agent_pky, del_msg, del_sig,
                                holder.device_pkx_hex, holder.device_pky_hex,
                                policy, err)) {
    return false;
  }

  return true;
}

}  // namespace proofs
