#include "examples/anoncred/holder/prove.h"

#include "examples/anoncred/shared/files.h"
#include "examples/anoncred/shared/small_demo.h"

namespace proofs {

bool RunProveCommand(const std::filesystem::path& credential_dir,
                     const std::filesystem::path& holder_key_dir,
                     const std::filesystem::path& request_dir,
                     const std::filesystem::path& out_dir, std::string* err) {
  HolderCredential credential;
  HolderKeyMaterial holder_key;
  PresentationRequest request;
  PresentationProof proof;
  if (!ReadHolderCredentialDir(credential_dir, &credential, err) ||
      !ReadHolderKeyMaterialDir(holder_key_dir, &holder_key, err) ||
      !ReadPresentationRequestDir(request_dir, &request, err) ||
      !ProveSmallDemo(credential, holder_key, request, &proof, err)) {
    return false;
  }
  return WritePresentationProofDir(out_dir, proof, err);
}

}  // namespace proofs
