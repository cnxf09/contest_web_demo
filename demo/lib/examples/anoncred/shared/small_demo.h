#ifndef PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_SMALL_DEMO_H_
#define PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_SMALL_DEMO_H_

#include <memory>
#include <string>

#include "algebra/convolution.h"
#include "algebra/fp2.h"
#include "algebra/reed_solomon.h"
#include "arrays/dense.h"
#include "ec/p256.h"
#include "examples/anoncred/shared/types.h"
#include "sumcheck/circuit.h"
#include "zk/zk_proof.h"

namespace proofs {

std::unique_ptr<Circuit<Fp256Base>> BuildSmallDemoCircuit();

bool MaterializeExampleBundleFromExample(uint32_t example_id,
                                         HolderCredential* credential,
                                         IssuerPublicBundle* issuer_public,
                                         std::string* err);

bool ProveSmallDemo(const HolderCredential& credential,
                    const HolderKeyMaterial& holder_key,
                    const PresentationRequest& request,
                    PresentationProof* out, std::string* err);

VerificationResult VerifySmallDemo(const IssuerPublicBundle& issuer_public,
                                   const PresentationRequest& request,
                                   const PresentationProof& presentation);

}  // namespace proofs

#endif  // PRIVACY_PROOFS_ZK_LIB_EXAMPLES_ANONCRED_SHARED_SMALL_DEMO_H_
