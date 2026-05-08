#ifndef EXAMPLES_DELEGATION_DEMO_SHARED_REVOCATION_H_
#define EXAMPLES_DELEGATION_DEMO_SHARED_REVOCATION_H_

#include <filesystem>
#include <string>

#include "examples/mdoc_anoncred/shared/types.h"

namespace proofs {

std::string ComputeRevocationIdHex(const std::string& device_pkx_hex,
                                   const std::string& device_pky_hex,
                                   std::string* err);

bool CreateRevocationStatus(const std::string& revocation_sk_hex,
                            const std::string& device_pkx_hex,
                            const std::string& device_pky_hex,
                            uint64_t epoch, const std::string& expires,
                            bool revoked, RevocationStatus* status,
                            std::string* err);

bool VerifyRevocationStatus(const RevocationStatus& status,
                            const std::string& revocation_pkx_hex,
                            const std::string& revocation_pky_hex,
                            const std::string& device_pkx_hex,
                            const std::string& device_pky_hex,
                            const std::string& now_iso8601,
                            std::string* err);

bool WriteRevocationStatusJson(const std::filesystem::path& path,
                               const RevocationStatus& status,
                               std::string* err);

bool ReadRevocationStatusJson(const std::filesystem::path& path,
                              RevocationStatus* status, std::string* err);

}  // namespace proofs

#endif  // EXAMPLES_DELEGATION_DEMO_SHARED_REVOCATION_H_
