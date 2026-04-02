#ifndef PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_FILES_H_
#define PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_FILES_H_

#include <filesystem>
#include <string>
#include <vector>

#include "examples/anoncred/shared/types.h"

namespace proofs {

bool WriteHolderCredentialDir(const std::filesystem::path& dir,
                              const HolderCredential& credential,
                              std::string* err);
bool ReadHolderCredentialDir(const std::filesystem::path& dir,
                             HolderCredential* credential, std::string* err);

bool WriteIssuerPublicBundleDir(const std::filesystem::path& dir,
                                const IssuerPublicBundle& issuer_public,
                                std::string* err);
bool ReadIssuerPublicBundleDir(const std::filesystem::path& dir,
                               IssuerPublicBundle* issuer_public,
                               std::string* err);

bool WriteHolderKeyMaterialDir(const std::filesystem::path& dir,
                               const HolderKeyMaterial& holder_key,
                               std::string* err);
bool ReadHolderKeyMaterialDir(const std::filesystem::path& dir,
                              HolderKeyMaterial* holder_key,
                              std::string* err);

bool WritePresentationRequestDir(const std::filesystem::path& dir,
                                 const PresentationRequest& request,
                                 std::string* err);
bool ReadPresentationRequestDir(const std::filesystem::path& dir,
                                PresentationRequest* request,
                                std::string* err);

bool WritePresentationProofDir(const std::filesystem::path& dir,
                               const PresentationProof& proof,
                               std::string* err);
bool ReadPresentationProofDir(const std::filesystem::path& dir,
                              PresentationProof* proof, std::string* err);

bool WriteBytesFile(const std::filesystem::path& path,
                    const std::vector<uint8_t>& bytes, std::string* err);
bool ReadBytesFile(const std::filesystem::path& path, std::vector<uint8_t>* out,
                   std::string* err);
bool WriteStringFile(const std::filesystem::path& path, const std::string& text,
                     std::string* err);
bool ReadStringFile(const std::filesystem::path& path, std::string* out,
                    std::string* err);

}  // namespace proofs

#endif  // PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_FILES_H_
