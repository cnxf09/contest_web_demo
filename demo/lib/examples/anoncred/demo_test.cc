#include <filesystem>
#include <string>
#include <vector>
#include <unistd.h>

#include "examples/anoncred/holder/keygen.h"
#include "examples/anoncred/issuer/issue.h"
#include "examples/anoncred/shared/files.h"
#include "examples/anoncred/shared/request.h"
#include "examples/anoncred/shared/small_demo.h"
#include "gtest/gtest.h"

namespace proofs {
namespace {

constexpr char kRequestNow[] = "20241005";

std::filesystem::path MakeTempDir(const char* suffix) {
  auto dir = std::filesystem::temp_directory_path() /
             (std::string("anoncred_demo_") + suffix + "_" +
              std::to_string(::getpid()));
  std::filesystem::create_directories(dir);
  return dir;
}

struct DemoSetup {
  std::filesystem::path root;
  std::filesystem::path holder_key_dir;
  std::filesystem::path issue_root;
  std::filesystem::path holder_dir;
  std::filesystem::path issuer_public_dir;
  HolderCredential credential;
  HolderKeyMaterial holder_key;
  IssuerPublicBundle issuer_public;
};

bool SetupDemo(const char* suffix, const IssueOptions& options, DemoSetup* out,
               std::string* err) {
  out->root = MakeTempDir(suffix);
  out->holder_key_dir = out->root / "holder_key";
  out->issue_root = out->root / "issue";
  out->holder_dir = out->issue_root / "holder";
  out->issuer_public_dir = out->issue_root / "issuer_public";
  if (!RunKeygenCommand(out->holder_key_dir, err)) {
    return false;
  }
  if (!ReadHolderKeyMaterialDir(out->holder_key_dir, &out->holder_key, err)) {
    return false;
  }
  IssueOptions opts = options;
  opts.holder_key_dir = out->holder_key_dir.string();
  if (!RunIssueCommand(0, out->issue_root, opts, err)) {
    return false;
  }
  return ReadHolderCredentialDir(out->holder_dir, &out->credential, err) &&
         ReadIssuerPublicBundleDir(out->issuer_public_dir, &out->issuer_public,
                                   err);
}

void ExpectRoundTripSuccess(const std::string& claim_name) {
  std::string err;
  DemoSetup setup;
  ASSERT_TRUE(SetupDemo(claim_name.c_str(), IssueOptions{}, &setup, &err)) << err;

  PresentationRequest request;
  ASSERT_TRUE(
      BuildFreshPresentationRequest(claim_name, kRequestNow, &request, &err))
      << err;

  PresentationProof proof;
  ASSERT_TRUE(ProveSmallDemo(setup.credential, setup.holder_key, request, &proof,
                             &err))
      << err;

  VerificationResult result =
      VerifySmallDemo(setup.issuer_public, request, proof);
  EXPECT_TRUE(result.ok) << result.message;
}

void ExpectRoundTripSuccess(const PresentationPolicy& policy,
                            const char* suffix) {
  std::string err;
  DemoSetup setup;
  ASSERT_TRUE(SetupDemo(suffix, IssueOptions{}, &setup, &err)) << err;

  PresentationRequest request;
  ASSERT_TRUE(BuildFreshPresentationRequest(policy, kRequestNow, &request, &err))
      << err;

  PresentationProof proof;
  ASSERT_TRUE(ProveSmallDemo(setup.credential, setup.holder_key, request, &proof,
                             &err))
      << err;

  VerificationResult result =
      VerifySmallDemo(setup.issuer_public, request, proof);
  EXPECT_TRUE(result.ok) << result.message;
}

TEST(AnoncredDemo, RequestTranscriptIsFresh) {
  std::string err;
  DemoSetup setup;
  ASSERT_TRUE(SetupDemo("fresh_request", IssueOptions{}, &setup, &err)) << err;

  PresentationRequest r1;
  PresentationRequest r2;
  ASSERT_TRUE(BuildFreshPresentationRequest(kAgeOver18Claim, kRequestNow, &r1,
                                            &err))
      << err;
  ASSERT_TRUE(BuildFreshPresentationRequest(kAgeOver18Claim, kRequestNow, &r2,
                                            &err))
      << err;
  EXPECT_NE(r1.transcript_bytes, r2.transcript_bytes);
}

TEST(AnoncredDemo, RoundTripAgeOver18) { ExpectRoundTripSuccess(kAgeOver18Claim); }

TEST(AnoncredDemo, RoundTripAgeOver21) { ExpectRoundTripSuccess(kAgeOver21Claim); }

TEST(AnoncredDemo, RoundTripDateOfBirth) {
  ExpectRoundTripSuccess(kDateOfBirthClaim);
}

TEST(AnoncredDemo, RoundTripAgeRange18To21) {
  PresentationPolicy policy;
  policy.type = PresentationPolicyType::kAgeRange;
  policy.min_age = 18;
  policy.max_age = 21;
  ExpectRoundTripSuccess(policy, "age_range_18_21");
}

TEST(AnoncredDemo, RoundTripAgeThresholds18And21) {
  PresentationPolicy policy;
  policy.type = PresentationPolicyType::kAgeThresholds;
  policy.age_thresholds = {18, 21};
  ExpectRoundTripSuccess(policy, "age_thresholds_18_21");
}

TEST(AnoncredDemo, CustomIssuedCredentialRoundTrip) {
  std::string err;
  IssueOptions options;
  options.first_name = "Alice";
  options.family_name = "Researcher";
  options.date_of_birth_yyyymmdd = "19991231";
  options.valid_from_yyyymmdd = "20240101";
  options.valid_until_yyyymmdd = "20271231";

  DemoSetup setup;
  ASSERT_TRUE(SetupDemo("custom_issue", options, &setup, &err)) << err;
  EXPECT_EQ(std::string(
                reinterpret_cast<const char*>(setup.credential.credential_bytes.data()),
                5),
            "Alice");
  EXPECT_EQ(std::string(reinterpret_cast<const char*>(
                            setup.credential.credential_bytes.data() + 32),
                        10),
            "Researcher");
  EXPECT_EQ(std::string(reinterpret_cast<const char*>(
                            setup.credential.credential_bytes.data() + 64),
                        8),
            "19991231");
  EXPECT_FALSE(setup.credential.issuer_pkx_hex.empty());
  EXPECT_FALSE(setup.credential.issuer_sig_r_hex.empty());

  PresentationRequest request;
  ASSERT_TRUE(BuildFreshPresentationRequest(kDateOfBirthClaim, kRequestNow,
                                            &request, &err))
      << err;
  PresentationProof proof;
  ASSERT_TRUE(ProveSmallDemo(setup.credential, setup.holder_key, request, &proof,
                             &err))
      << err;

  VerificationResult result =
      VerifySmallDemo(setup.issuer_public, request, proof);
  EXPECT_TRUE(result.ok) << result.message;
  EXPECT_EQ(std::string(result.disclosed_value.begin(), result.disclosed_value.end()),
            "19991231");
}

TEST(AnoncredDemo, AgeClaimsAreDerivedFromDob) {
  std::string err;
  IssueOptions options;
  options.date_of_birth_yyyymmdd = "20100101";

  DemoSetup setup;
  ASSERT_TRUE(SetupDemo("age_from_dob", options, &setup, &err)) << err;

  PresentationRequest request18;
  ASSERT_TRUE(
      BuildFreshPresentationRequest(kAgeOver18Claim, kRequestNow, &request18, &err))
      << err;
  PresentationProof proof18;
  ASSERT_TRUE(ProveSmallDemo(setup.credential, setup.holder_key, request18,
                             &proof18, &err))
      << err;
  VerificationResult result18 =
      VerifySmallDemo(setup.issuer_public, request18, proof18);
  ASSERT_TRUE(result18.ok) << result18.message;
  ASSERT_EQ(proof18.disclosed_value.size(), 1u);
  EXPECT_EQ(proof18.disclosed_value[0], 0xf4);

  PresentationRequest request21;
  ASSERT_TRUE(
      BuildFreshPresentationRequest(kAgeOver21Claim, kRequestNow, &request21, &err))
      << err;
  PresentationProof proof21;
  ASSERT_TRUE(ProveSmallDemo(setup.credential, setup.holder_key, request21,
                             &proof21, &err))
      << err;
  VerificationResult result21 =
      VerifySmallDemo(setup.issuer_public, request21, proof21);
  ASSERT_TRUE(result21.ok) << result21.message;
  ASSERT_EQ(proof21.disclosed_value.size(), 1u);
  EXPECT_EQ(proof21.disclosed_value[0], 0xf4);
}

TEST(AnoncredDemo, AgeRangePolicyUsesDob) {
  std::string err;
  IssueOptions options;
  options.date_of_birth_yyyymmdd = "20051231";

  DemoSetup setup;
  ASSERT_TRUE(SetupDemo("age_range_from_dob", options, &setup, &err)) << err;

  PresentationPolicy policy;
  policy.type = PresentationPolicyType::kAgeRange;
  policy.min_age = 18;
  policy.max_age = 21;

  PresentationRequest request;
  ASSERT_TRUE(BuildFreshPresentationRequest(policy, kRequestNow, &request, &err))
      << err;
  PresentationProof proof;
  ASSERT_TRUE(ProveSmallDemo(setup.credential, setup.holder_key, request, &proof,
                             &err))
      << err;
  VerificationResult result =
      VerifySmallDemo(setup.issuer_public, request, proof);
  ASSERT_TRUE(result.ok) << result.message;
  ASSERT_EQ(proof.disclosed_value.size(), 1u);
  EXPECT_EQ(proof.disclosed_value[0], 0xf5);
}

TEST(AnoncredDemo, AgeThresholdsPolicyUsesDob) {
  std::string err;
  IssueOptions options;
  options.date_of_birth_yyyymmdd = "20060101";

  DemoSetup setup;
  ASSERT_TRUE(SetupDemo("age_thresholds_from_dob", options, &setup, &err))
      << err;

  PresentationPolicy policy;
  policy.type = PresentationPolicyType::kAgeThresholds;
  policy.age_thresholds = {18, 21};

  PresentationRequest request;
  ASSERT_TRUE(BuildFreshPresentationRequest(policy, kRequestNow, &request, &err))
      << err;
  PresentationProof proof;
  ASSERT_TRUE(ProveSmallDemo(setup.credential, setup.holder_key, request, &proof,
                             &err))
      << err;
  VerificationResult result =
      VerifySmallDemo(setup.issuer_public, request, proof);
  ASSERT_TRUE(result.ok) << result.message;
  ASSERT_EQ(proof.disclosed_value.size(), 1u);
  EXPECT_EQ(proof.disclosed_value[0], 0xf4);
}

TEST(AnoncredDemo, ReplayAcrossRequestsFails) {
  std::string err;
  DemoSetup setup;
  ASSERT_TRUE(SetupDemo("replay", IssueOptions{}, &setup, &err)) << err;

  PresentationRequest request1;
  PresentationRequest request2;
  ASSERT_TRUE(BuildFreshPresentationRequest(kAgeOver18Claim, kRequestNow,
                                            &request1, &err))
      << err;
  ASSERT_TRUE(BuildFreshPresentationRequest(kAgeOver18Claim, kRequestNow,
                                            &request2, &err))
      << err;

  PresentationProof proof1;
  ASSERT_TRUE(ProveSmallDemo(setup.credential, setup.holder_key, request1,
                             &proof1, &err))
      << err;

  VerificationResult result =
      VerifySmallDemo(setup.issuer_public, request2, proof1);
  EXPECT_FALSE(result.ok);
}

TEST(AnoncredDemo, WrongHolderKeyFails) {
  std::string err;
  DemoSetup setup;
  ASSERT_TRUE(SetupDemo("wrong_key", IssueOptions{}, &setup, &err)) << err;

  auto wrong_key_dir = setup.root / "wrong_holder_key";
  ASSERT_TRUE(RunKeygenCommand(wrong_key_dir, &err)) << err;
  HolderKeyMaterial wrong_key;
  ASSERT_TRUE(ReadHolderKeyMaterialDir(wrong_key_dir, &wrong_key, &err)) << err;

  PresentationRequest request;
  ASSERT_TRUE(BuildFreshPresentationRequest(kAgeOver18Claim, kRequestNow,
                                            &request, &err))
      << err;

  PresentationProof proof;
  EXPECT_FALSE(
      ProveSmallDemo(setup.credential, wrong_key, request, &proof, &err));
  EXPECT_EQ(err, "proof generation failed");
}

TEST(AnoncredDemo, TamperedProofFails) {
  std::string err;
  DemoSetup setup;
  ASSERT_TRUE(SetupDemo("tamper_proof", IssueOptions{}, &setup, &err)) << err;

  PresentationRequest request;
  ASSERT_TRUE(BuildFreshPresentationRequest(kAgeOver18Claim, kRequestNow,
                                            &request, &err))
      << err;

  PresentationProof proof;
  ASSERT_TRUE(ProveSmallDemo(setup.credential, setup.holder_key, request, &proof,
                             &err))
      << err;
  ASSERT_FALSE(proof.proof_bytes.empty());
  proof.proof_bytes[0] ^= 0x01;

  VerificationResult result =
      VerifySmallDemo(setup.issuer_public, request, proof);
  EXPECT_FALSE(result.ok);
}

TEST(AnoncredDemo, TamperedDisclosedValueFails) {
  std::string err;
  DemoSetup setup;
  ASSERT_TRUE(SetupDemo("tamper_disclosed", IssueOptions{}, &setup, &err))
      << err;

  PresentationRequest request;
  ASSERT_TRUE(BuildFreshPresentationRequest(kDateOfBirthClaim, kRequestNow,
                                            &request, &err))
      << err;

  PresentationProof proof;
  ASSERT_TRUE(ProveSmallDemo(setup.credential, setup.holder_key, request, &proof,
                             &err))
      << err;
  ASSERT_EQ(proof.disclosed_value.size(), 8u);
  proof.disclosed_value[0] = '2';

  VerificationResult result =
      VerifySmallDemo(setup.issuer_public, request, proof);
  EXPECT_FALSE(result.ok);
}

}  // namespace
}  // namespace proofs
