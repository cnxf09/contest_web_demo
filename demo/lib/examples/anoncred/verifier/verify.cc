#include "examples/anoncred/verifier/verify.h"

#include "examples/anoncred/shared/files.h"
#include "examples/anoncred/shared/request.h"
#include "examples/anoncred/shared/small_demo.h"

namespace proofs {

bool RunRequestCommand(const PresentationPolicy& policy,
                       const std::string& now_yyyymmdd,
                       const std::filesystem::path& out_dir,
                       std::string* err) {
  PresentationRequest request;
  if (!BuildFreshPresentationRequest(policy, now_yyyymmdd, &request, err)) {
    return false;
  }
  return WritePresentationRequestDir(out_dir, request, err);
}

bool RunVerifyCommand(const std::filesystem::path& issuer_public_dir,
                      const std::filesystem::path& request_dir,
                      const std::filesystem::path& presentation_dir,
                      bool* verified, std::string* err) {
  IssuerPublicBundle issuer_public;
  PresentationRequest request;
  PresentationProof proof;
  if (!ReadIssuerPublicBundleDir(issuer_public_dir, &issuer_public, err) ||
      !ReadPresentationRequestDir(request_dir, &request, err) ||
      !ReadPresentationProofDir(presentation_dir, &proof, err)) {
    return false;
  }
  VerificationResult result = VerifySmallDemo(issuer_public, request, proof);
  *verified = result.ok;
  *err = result.message;
  return true;
}

}  // namespace proofs
