#include "examples/anoncred/shared/small_demo.h"

#include <memory>

#include "algebra/convolution.h"
#include "algebra/fp2.h"
#include "algebra/reed_solomon.h"
#include "arrays/dense.h"
#include "circuits/compiler/circuit_dump.h"
#include "circuits/compiler/compiler.h"
#include "circuits/logic/compiler_backend.h"
#include "circuits/logic/logic.h"
#include "circuits/mdoc/mdoc_witness.h"
#include "circuits/tests/anoncred/small.h"
#include "circuits/tests/anoncred/small_examples.h"
#include "circuits/tests/anoncred/small_witness.h"
#include "ec/p256.h"
#include "examples/anoncred/shared/device_key.h"
#include "examples/anoncred/shared/request.h"
#include "random/secure_random_engine.h"
#include "random/transcript.h"
#include "sumcheck/circuit.h"
#include "util/log.h"
#include "util/readbuffer.h"
#include "zk/zk_proof.h"
#include "zk/zk_prover.h"
#include "zk/zk_verifier.h"

namespace proofs {
namespace {

using Sw = SmallWitness<P256, Fp256Base, Fp256Scalar>;
using FftExtConvolutionFactory =
    FFTExtConvolutionFactory<Fp256Base, Fp2<Fp256Base>>;
using RSFactory = ReedSolomonFactory<Fp256Base, FftExtConvolutionFactory>;
static constexpr size_t kNumAttr = 1;
static constexpr size_t kLigeroRate = 4;
static constexpr size_t kLigeroNreq = 189;
static constexpr size_t kTranscriptVersion = 4;

bool ParseElt(const std::string& s, Fp256Base::Elt* out, std::string* err) {
  auto maybe = p256_base.of_untrusted_string(s.c_str());
  if (!maybe.has_value()) {
    if (err != nullptr) {
      *err = "invalid field element: " + s;
    }
    return false;
  }
  *out = maybe.value();
  return true;
}

bool ParseNat(const std::string& s, Fp256Base::N* out, std::string* err) {
  auto maybe = Fp256Base::N::of_untrusted_string(s.c_str());
  if (!maybe.has_value()) {
    if (err != nullptr) {
      *err = "invalid scalar: " + s;
    }
    return false;
  }
  *out = maybe.value();
  return true;
}

bool FillSmallWitness(const HolderCredential& credential,
                      const HolderKeyMaterial& holder_key,
                      const PresentationRequest& request, Sw* sw,
                      Fp256Base::Elt* pk_x, Fp256Base::Elt* pk_y,
                      std::string* err) {
  if (!ParseElt(credential.issuer_pkx_hex, pk_x, err) ||
      !ParseElt(credential.issuer_pky_hex, pk_y, err)) {
    return false;
  }
  std::string sig_r_hex;
  std::string sig_s_hex;
  if (!SignTranscriptWithHolderKey(holder_key, request.transcript_bytes,
                                   &sig_r_hex, &sig_s_hex, err)) {
    return false;
  }

  Fp256Base::N nr, ns, nr2, ns2;
  if (!ParseNat(credential.issuer_sig_r_hex, &nr, err) ||
      !ParseNat(credential.issuer_sig_s_hex, &ns, err) ||
      !ParseNat(sig_r_hex, &nr2, err) || !ParseNat(sig_s_hex, &ns2, err)) {
    return false;
  }

  using Nat = Fp256Base::N;
  Nat ne = nat_from_hash<Nat>(credential.credential_bytes.data(),
                              credential.credential_bytes.size());
  sw->e_ = p256_base.to_montgomery(ne);
  sw->ew_.compute_witness(*pk_x, *pk_y, ne, nr, ns);

  Nat ne2 = nat_from_hash<Nat>(request.transcript_bytes.data(),
                               request.transcript_bytes.size());
  sw->dpkx_ =
      p256_base.to_montgomery(nat_from_be<Nat>(&credential.credential_bytes[100]));
  sw->dpky_ =
      p256_base.to_montgomery(nat_from_be<Nat>(&credential.credential_bytes[132]));
  sw->e2_ = p256_base.to_montgomery(ne2);
  sw->dkw_.compute_witness(sw->dpkx_, sw->dpky_, ne2, nr2, ns2);

  FlatSHA256Witness::transform_and_witness_message(
      credential.credential_bytes.size(), credential.credential_bytes.data(), 7,
      sw->numb_, sw->signed_bytes_, sw->bw_);

  if (request.now_yyyymmdd.size() != kDateLen) {
    if (err != nullptr) {
      *err = "request now must be YYYYMMDD";
    }
    return false;
  }
  for (size_t i = 0; i < kDateLen; ++i) {
    sw->now_[i] = static_cast<uint8_t>(request.now_yyyymmdd[i]);
  }
  return true;
}

bool BuildPublicInputs(const IssuerPublicBundle& issuer_public,
                       const PresentationRequest& request,
                       const std::vector<uint8_t>& disclosed_value,
                       Dense<Fp256Base>* pub, std::string* err) {
  Fp256Base::Elt pk_x, pk_y;
  if (!ParseElt(issuer_public.issuer_pkx_hex, &pk_x, err) ||
      !ParseElt(issuer_public.issuer_pky_hex, &pk_y, err)) {
    return false;
  }

  CompiledPresentationPolicy compiled_policy;
  if (!CompilePresentationPolicy(request, &compiled_policy, err)) {
    return false;
  }

  auto attr =
      SmallOpenedAttribute(0, 0, reinterpret_cast<const uint8_t*>(""), 0);
  if (!BuildOpenedAttributeForValue(request.policy, disclosed_value, &attr,
                                    err)) {
    return false;
  }

  using Nat = Fp256Base::N;
  Nat ne2 = nat_from_hash<Nat>(request.transcript_bytes.data(),
                               request.transcript_bytes.size());
  auto e2 = p256_base.to_montgomery(ne2);

  DenseFiller<Fp256Base> pub_filler(*pub);
  pub_filler.push_back(p256_base.one());
  pub_filler.push_back(pk_x);
  pub_filler.push_back(pk_y);
  pub_filler.push_back(e2);

  pub_filler.push_back(attr.ind_, 8, p256_base);
  pub_filler.push_back(attr.len_, 8, p256_base);
  for (size_t i = 0; i < 32; ++i) {
    uint8_t v = attr.value_.size() > i ? attr.value_[i] : 0;
    pub_filler.push_back(v, 8, p256_base);
  }

  for (size_t i = 0; i < kDateLen; ++i) {
    pub_filler.push_back(static_cast<uint8_t>(request.now_yyyymmdd[i]), 8,
                         p256_base);
  }
  pub_filler.push_back(compiled_policy.type, 8, p256_base);
  pub_filler.push_back(compiled_policy.cutoff_count, 8, p256_base);
  for (size_t cutoff = 0; cutoff < kMaxAgeThresholds; ++cutoff) {
    for (size_t i = 0; i < kDateLen; ++i) {
      const uint8_t byte =
          compiled_policy.cutoff_dates[cutoff].size() == kDateLen
              ? static_cast<uint8_t>(compiled_policy.cutoff_dates[cutoff][i])
              : 0;
      pub_filler.push_back(byte, 8, p256_base);
    }
  }
  return true;
}

bool BuildWitnessAndPublicInputs(const HolderCredential& credential,
                                 const HolderKeyMaterial& holder_key,
                                 const PresentationRequest& request,
                                 const Circuit<Fp256Base>& circuit,
                                 Dense<Fp256Base>* witness,
                                 Dense<Fp256Base>* pub,
                                 std::vector<uint8_t>* disclosed_value,
                                 std::string* err) {
  Fp256Base::Elt pk_x, pk_y;
  Sw sw(p256, p256_scalar);
  if (!FillSmallWitness(credential, holder_key, request, &sw, &pk_x, &pk_y,
                        err)) {
    return false;
  }
  if (!ExtractDisclosedValueForRequest(credential, request, disclosed_value,
                                       err)) {
    return false;
  }

  CompiledPresentationPolicy compiled_policy;
  if (!CompilePresentationPolicy(request, &compiled_policy, err)) {
    return false;
  }

  auto attr =
      SmallOpenedAttribute(0, 0, reinterpret_cast<const uint8_t*>(""), 0);
  if (!BuildOpenedAttributeForValue(request.policy, *disclosed_value, &attr,
                                    err)) {
    return false;
  }

  DenseFiller<Fp256Base> filler(*witness);
  DenseFiller<Fp256Base> pub_filler(*pub);

  filler.push_back(p256_base.one());
  pub_filler.push_back(p256_base.one());
  filler.push_back(pk_x);
  pub_filler.push_back(pk_x);
  filler.push_back(pk_y);
  pub_filler.push_back(pk_y);
  filler.push_back(sw.e2_);
  pub_filler.push_back(sw.e2_);

  filler.push_back(attr.ind_, 8, p256_base);
  pub_filler.push_back(attr.ind_, 8, p256_base);
  filler.push_back(attr.len_, 8, p256_base);
  pub_filler.push_back(attr.len_, 8, p256_base);
  for (size_t i = 0; i < 32; ++i) {
    uint8_t v = attr.value_.size() > i ? attr.value_[i] : 0;
    filler.push_back(v, 8, p256_base);
    pub_filler.push_back(v, 8, p256_base);
  }

  for (size_t i = 0; i < kDateLen; ++i) {
    uint8_t byte = static_cast<uint8_t>(request.now_yyyymmdd[i]);
    filler.push_back(byte, 8, p256_base);
    pub_filler.push_back(byte, 8, p256_base);
  }
  filler.push_back(compiled_policy.type, 8, p256_base);
  pub_filler.push_back(compiled_policy.type, 8, p256_base);
  filler.push_back(compiled_policy.cutoff_count, 8, p256_base);
  pub_filler.push_back(compiled_policy.cutoff_count, 8, p256_base);
  for (size_t cutoff = 0; cutoff < kMaxAgeThresholds; ++cutoff) {
    for (size_t i = 0; i < kDateLen; ++i) {
      const uint8_t byte =
          compiled_policy.cutoff_dates[cutoff].size() == kDateLen
              ? static_cast<uint8_t>(compiled_policy.cutoff_dates[cutoff][i])
              : 0;
      filler.push_back(byte, 8, p256_base);
      pub_filler.push_back(byte, 8, p256_base);
    }
  }

  sw.fill_witness(filler, true);
  return witness->n1_ == circuit.ninputs && pub->n1_ == circuit.npub_in;
}

}  // namespace

std::unique_ptr<Circuit<Fp256Base>> BuildSmallDemoCircuit() {
  using CompilerBackendT = CompilerBackend<Fp256Base>;
  using LogicCircuit = Logic<Fp256Base, CompilerBackendT>;
  using v8 = typename LogicCircuit::v8;
  using EltW = typename LogicCircuit::EltW;
  using SmallT = Small<LogicCircuit, Fp256Base, P256, kNumAttr>;

  QuadCircuit<Fp256Base> q(p256_base);
  const CompilerBackendT cbk(&q);
  const LogicCircuit lc(&cbk, p256_base);
  SmallT small(lc, p256, n256_order);

  EltW pk_x = lc.eltw_input();
  EltW pk_y = lc.eltw_input();
  EltW htr = lc.eltw_input();
  typename SmallT::OpenedAttribute attrs[kNumAttr];
  for (size_t i = 0; i < kNumAttr; ++i) {
    attrs[i].input(lc);
  }

  v8 now[kDateLen];
  for (size_t i = 0; i < kDateLen; ++i) {
    now[i] = lc.template vinput<8>();
  }
  v8 policy_type = lc.template vinput<8>();
  v8 policy_cutoff_count = lc.template vinput<8>();
  v8 policy_cutoffs[kMaxAgeThresholds][kDateLen];
  for (size_t cutoff = 0; cutoff < kMaxAgeThresholds; ++cutoff) {
    for (size_t i = 0; i < kDateLen; ++i) {
      policy_cutoffs[cutoff][i] = lc.template vinput<8>();
    }
  }

  q.private_input();
  typename SmallT::Witness witness;
  witness.input(lc);
  small.assert_credential(pk_x, pk_y, htr, attrs, now, policy_type,
                          policy_cutoff_count, policy_cutoffs,
                          witness);

  auto circuit = q.mkcircuit(/*nc=*/1);
  dump_info("anoncred_demo", q);
  return circuit;
}

bool MaterializeExampleBundleFromExample(uint32_t example_id,
                                         HolderCredential* credential,
                                         IssuerPublicBundle* issuer_public,
                                         std::string* err) {
  if (example_id >= sizeof(mdoc_small_tests) / sizeof(mdoc_small_tests[0])) {
    if (err != nullptr) {
      *err = "unknown example id: " + std::to_string(example_id);
    }
    return false;
  }
  const SmallTest& test = mdoc_small_tests[example_id];
  credential->credential_bytes.assign(test.mdoc, test.mdoc + test.mdoc_size);
  credential->issuer_pkx_hex = test.pkx.as_pointer;
  credential->issuer_pky_hex = test.pky.as_pointer;
  credential->issuer_sig_r_hex = test.sigr.as_pointer;
  credential->issuer_sig_s_hex = test.sigs.as_pointer;

  issuer_public->issuer_pkx_hex = test.pkx.as_pointer;
  issuer_public->issuer_pky_hex = test.pky.as_pointer;
  issuer_public->example_id = example_id;
  return true;
}

bool ProveSmallDemo(const HolderCredential& credential,
                    const HolderKeyMaterial& holder_key,
                    const PresentationRequest& request,
                    PresentationProof* out, std::string* err) {
  auto circuit = BuildSmallDemoCircuit();
  Dense<Fp256Base> witness(1, circuit->ninputs);
  Dense<Fp256Base> pub(1, circuit->npub_in);
  std::vector<uint8_t> disclosed_value;
  if (!BuildWitnessAndPublicInputs(credential, holder_key, request, *circuit,
                                   &witness, &pub, &disclosed_value, err)) {
    return false;
  }

  const Fp2<Fp256Base> p256_2(p256_base);
  const auto omega = p256_2.of_string(
      "112649224146410281873500457609690258373018840430489408729223714171582664680802",
      "84087994358540907695740461427818660560182168997182378749313018254450460212908");
  const FftExtConvolutionFactory fft_factory(p256_base, p256_2, omega,
                                             1ull << 31);
  const RSFactory rsf(fft_factory, p256_base);
  ZkProof<Fp256Base> proof(*circuit, kLigeroRate, kLigeroNreq);
  Transcript tp(reinterpret_cast<const uint8_t*>("zk_test"), 7,
                kTranscriptVersion);
  SecureRandomEngine rng;
  ZkProver<Fp256Base, RSFactory> prover(*circuit, p256_base, rsf);
  prover.commit(proof, witness, tp, rng);
  if (!prover.prove(proof, witness, tp)) {
    if (err != nullptr) {
      *err = "proof generation failed";
    }
    return false;
  }

  out->proof_bytes.clear();
  proof.write(out->proof_bytes, p256_base);
  out->claim_name = PolicyDisplayName(request.policy);
  out->disclosed_value = disclosed_value;
  return true;
}

VerificationResult VerifySmallDemo(const IssuerPublicBundle& issuer_public,
                                   const PresentationRequest& request,
                                   const PresentationProof& presentation) {
  VerificationResult result;
  result.claim_name = presentation.claim_name;
  result.disclosed_value = presentation.disclosed_value;

  std::string err;
  if (presentation.claim_name != PolicyDisplayName(request.policy)) {
    result.message = "policy name mismatch";
    return result;
  }

  auto circuit = BuildSmallDemoCircuit();
  Dense<Fp256Base> pub(1, circuit->npub_in);
  if (!BuildPublicInputs(issuer_public, request, presentation.disclosed_value,
                         &pub, &err)) {
    result.message = err;
    return result;
  }

  const Fp2<Fp256Base> p256_2(p256_base);
  const auto omega = p256_2.of_string(
      "112649224146410281873500457609690258373018840430489408729223714171582664680802",
      "84087994358540907695740461427818660560182168997182378749313018254450460212908");
  const FftExtConvolutionFactory fft_factory(p256_base, p256_2, omega,
                                             1ull << 31);
  const RSFactory rsf(fft_factory, p256_base);
  ZkProof<Fp256Base> proof(*circuit, kLigeroRate, kLigeroNreq);
  ReadBuffer rb(presentation.proof_bytes);
  if (!proof.read(rb, p256_base)) {
    result.message = "invalid proof encoding";
    return result;
  }

  Transcript tv(reinterpret_cast<const uint8_t*>("zk_test"), 7,
                kTranscriptVersion);
  ZkVerifier<Fp256Base, RSFactory> verifier(*circuit, rsf, kLigeroRate,
                                            kLigeroNreq, p256_base);
  verifier.recv_commitment(proof, tv);
  result.ok = verifier.verify(proof, pub, tv);
  result.message = result.ok ? "ok" : "proof verification failed";
  return result;
}

}  // namespace proofs
