#ifndef PRIVACY_PROOFS_ZK_LIB_EXAMPLES_MDOC_ANONCRED_SHARED_MDOC_DEMO_H_
#define PRIVACY_PROOFS_ZK_LIB_EXAMPLES_MDOC_ANONCRED_SHARED_MDOC_DEMO_H_

#include <string>
#include <vector>

#include "examples/mdoc_anoncred/shared/types.h"

namespace proofs {

bool MaterializeMdocExample(uint32_t example_id, HolderMdoc* holder,
                            MdocIssuerPublicBundle* issuer_public,
                            std::string* err);

bool BuildReaderClaim(const std::string& claim_alias, ReaderClaim* claim,
                      std::string* err);

bool BuildReaderClaimForIssuer(const MdocIssuerPublicBundle& issuer_public,
                               const std::string& claim_alias, ReaderClaim* claim,
                               std::string* err);

bool BuildReaderRequest(const MdocIssuerPublicBundle& issuer_public,
                        const std::vector<std::string>& claim_aliases,
                        ReaderRequest* request, std::string* err);

bool ProveMdocPresentation(const HolderMdoc& holder,
                           const MdocIssuerPublicBundle& issuer_public,
                           const ReaderRequest& request,
                           MdocPresentation* presentation,
                           std::string* err);

MdocVerificationResult VerifyMdocPresentation(
    const MdocIssuerPublicBundle& issuer_public, const ReaderRequest& request,
    const MdocPresentation& presentation);

}  // namespace proofs

#endif  // PRIVACY_PROOFS_ZK_LIB_EXAMPLES_MDOC_ANONCRED_SHARED_MDOC_DEMO_H_
