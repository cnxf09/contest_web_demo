// Copyright 2026 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PRIVACY_PROOFS_ZK_LIB_CIRCUITS_TESTS_ANONCRED_SMALL_H_
#define PRIVACY_PROOFS_ZK_LIB_CIRCUITS_TESTS_ANONCRED_SMALL_H_

#include <cstddef>
#include <vector>

#include "circuits/ecdsa/verify_circuit.h"
#include "circuits/logic/bit_plucker.h"
#include "circuits/logic/memcmp.h"
#include "circuits/sha/flatsha256_circuit.h"
#include "circuits/tests/anoncred/small_io.h"

namespace proofs {

// This class creates a circuit to verify the signatures in a "small" MDOC.
// A small credential is a 183-byte document formatted as:
//      first_name    32 0
//      family_name   32 32
//      date_of_birth YYYYMMDD 64
//      gender        B 72
//      age_over_X.   BBBBBBB 73    [16, 18, 21, 25, 62, 65, 67]
//      issuerid   BBBB 80
//      validfrom  YYYYMMDD 84
//      validuntil YYYYMMDD 92
//      DPKX  32x 100
//      DPKY  32x 132
//      <arbitrary bytes of information>
template <class LogicCircuit, class Field, class EC, size_t kNumAttr>
class Small {
  using EltW = typename LogicCircuit::EltW;
  using Elt = typename LogicCircuit::Elt;
  using Nat = typename Field::N;
  using Ecdsa = VerifyCircuit<LogicCircuit, Field, EC>;
  using EcdsaWitness = typename Ecdsa::Witness;

  using v8 = typename LogicCircuit::v8;
  using v32 = typename LogicCircuit::v32;
  static constexpr size_t kIndexBits = 5;
  static constexpr size_t kMaxSHABlocks = 7;
  static constexpr size_t kMaxPolicyCutoffs = 3;
  static constexpr uint8_t kPolicyRevealDateOfBirth = 1;
  static constexpr uint8_t kPolicyAgeRange = 2;
  static constexpr uint8_t kPolicyAgeThresholds = 3;
  using Flatsha = FlatSHA256Circuit<LogicCircuit, BitPlucker<LogicCircuit, 3>>;
  using ShaBlockWitness = typename Flatsha::BlockWitness;
  using sha_packed_v32 = typename Flatsha::packed_v32;

  const LogicCircuit& lc_;
  const EC& ec_;
  const Nat& order_;

 public:
  class Witness {
   public:
    EltW e_;
    EltW dpkx_, dpky_;

    EcdsaWitness sig_;
    EcdsaWitness dpk_sig_;

    v8 in_[64 * kMaxSHABlocks]; /* input bytes, 64 * MAX */
    v8 nb_; /* index of sha block that contains the real hash  */
    ShaBlockWitness sig_sha_[kMaxSHABlocks];

    void input(const LogicCircuit& lc) {
      e_ = lc.eltw_input();
      dpkx_ = lc.eltw_input();
      dpky_ = lc.eltw_input();

      sig_.input(lc);
      dpk_sig_.input(lc);

      nb_ = lc.template vinput<8>();

      // sha input init =========================
      for (size_t i = 0; i < 64 * kMaxSHABlocks; ++i) {
        in_[i] = lc.template vinput<8>();
      }
      for (size_t j = 0; j < kMaxSHABlocks; j++) {
        sig_sha_[j].input(lc);
      }
    }
  };

  struct OpenedAttribute {
    v8 ind;    /* index of attribute */
    v8 len;    /* length of attribute, 1--32 */
    v8 v1[32]; /* attribute value */
    void input(const LogicCircuit& lc) {
      ind = lc.template vinput<8>();
      len = lc.template vinput<8>();
      for (size_t j = 0; j < 32; ++j) {
        v1[j] = lc.template vinput<8>();
      }
    }
  };

  explicit Small(const LogicCircuit& lc, const EC& ec, const Nat& order)
      : lc_(lc), ec_(ec), order_(order), sha_(lc) {}

  void assert_credential(EltW pkX, EltW pkY, EltW hash_tr,
                         OpenedAttribute oa[/* NUM_ATTR */],
                         const v8 now[/*kDateLen*/],
                         const v8& policy_type,
                         const v8& policy_cutoff_count,
                         const v8 policy_cutoffs[kMaxPolicyCutoffs][kDateLen],
                         const Witness& vw) const {
    Ecdsa ecc(lc_, ec_, order_);

    ecc.verify_signature3(pkX, pkY, vw.e_, vw.sig_);
    ecc.verify_signature3(vw.dpkx_, vw.dpky_, hash_tr, vw.dpk_sig_);

    sha_.assert_message(kMaxSHABlocks, vw.nb_, vw.in_, vw.sig_sha_);
    assert_hash(vw.e_, vw);

    const Memcmp<LogicCircuit> CMP(lc_);
    // // validFrom <= now
    lc_.assert1(CMP.leq(kDateLen, &vw.in_[84], &now[0]));

    // // now <= validUntil
    lc_.assert1(CMP.leq(kDateLen, &now[0], &vw.in_[92]));

    // // DPK_{x,y}
    EltW dpkx = repack(vw.in_, 100);
    EltW dpky = repack(vw.in_, 132);
    lc_.assert_eq(&dpkx, vw.dpkx_);
    lc_.assert_eq(&dpky, vw.dpky_);

    for (size_t ai = 0; ai < kNumAttr; ++ai) {
      auto is_dob = lc_.veq(oa[ai].ind, 64);
      auto is_age_claim = lc_.veq(oa[ai].ind, 74);
      auto policy_is_dob = lc_.veq(policy_type, kPolicyRevealDateOfBirth);
      auto policy_is_range = lc_.veq(policy_type, kPolicyAgeRange);
      auto policy_is_thresholds = lc_.veq(policy_type, kPolicyAgeThresholds);
      auto age_policy = lc_.lor(&policy_is_range, policy_is_thresholds);
      auto dob_supported = lc_.land(&is_dob, policy_is_dob);
      auto age_supported = lc_.land(&is_age_claim, age_policy);
      auto supported = lc_.lor(&dob_supported, age_supported);
      lc_.assert1(supported);

      auto len_is_8 = lc_.veq(oa[ai].len, 8);
      auto len_is_1 = lc_.veq(oa[ai].len, 1);
      lc_.assert_implies(&dob_supported, len_is_8);
      lc_.assert_implies(&age_supported, len_is_1);

      auto dob_check = lc_.land(&dob_supported, len_is_8);
      for (size_t j = 0; j < kDateLen; ++j) {
        auto same = lc_.veq(&vw.in_[64 + j], oa[ai].v1[j]);
        lc_.assert_implies(&dob_check, same);
      }

      auto want_true = lc_.veq(oa[ai].v1[0], 0xf5);
      auto want_false = lc_.veq(oa[ai].v1[0], 0xf4);
      auto age_check = lc_.land(&age_supported, len_is_1);

      auto count_is_1 = lc_.veq(policy_cutoff_count, 1);
      auto count_is_2 = lc_.veq(policy_cutoff_count, 2);
      auto count_is_3 = lc_.veq(policy_cutoff_count, 3);
      auto threshold_count_ok = lc_.lor(&count_is_1, lc_.lor(&count_is_2, count_is_3));
      lc_.assert_implies(&policy_is_thresholds, threshold_count_ok);
      lc_.assert_implies(&policy_is_range, count_is_2);

      auto lower_ok = CMP.leq(kDateLen, &vw.in_[64], &policy_cutoffs[0][0]);
      auto upper_too_old =
          CMP.leq(kDateLen, &vw.in_[64], &policy_cutoffs[1][0]);
      auto range_ok = lc_.land(&lower_ok, lc_.lnot(upper_too_old));

      auto thresholds_ok = lc_.bit(1);
      for (size_t i = 0; i < kMaxPolicyCutoffs; ++i) {
        auto active = lc_.vlt(i, policy_cutoff_count);
        auto cutoff_ok = CMP.leq(kDateLen, &vw.in_[64], &policy_cutoffs[i][0]);
        auto maybe_ok = lc_.mux(&active, &cutoff_ok, lc_.bit(1));
        thresholds_ok = lc_.land(&thresholds_ok, maybe_ok);
      }

      auto age_ok = lc_.mux(&policy_is_range, &range_ok, thresholds_ok);
      auto disclosed_matches = lc_.mux(&age_ok, &want_true, want_false);
      lc_.assert_implies(&age_check, disclosed_matches);
    }
  }

 private:
  // Checks that an attribute id or attribute value is as expected.
  // The len parameter holds the byte length of the expected id or value.
  void assert_attribute(size_t max, const v8& vlen, const v8 got[/*max*/],
                        const v8 want[/*max*/]) const {
    for (size_t j = 0; j < max; ++j) {
      auto ll = lc_.vlt(j, vlen);
      auto cmp = lc_.veq(&got[j], want[j]);
      lc_.assert_implies(&ll, cmp);
    }
  }

  // Assert that the hash of the mdoc is equal to e.
  // The hash is encoded in the SHA witness, and thus the correct block
  // must be muxed for the comparison. Thus method first muxes the "packed"
  // encoding of the SHA witness, then unpacks it and compares it to e to
  // save a lot of work in the bit plucker.
  void assert_hash(const EltW& e, const Witness& vw) const {
    sha_packed_v32 x[8];
    for (size_t b = 0; b < kMaxSHABlocks; ++b) {
      auto bt = lc_.veq(vw.nb_, b + 1); /* b is zero-indexed */
      auto ebt = lc_.eval(bt);
      for (size_t i = 0; i < 8; ++i) {
        for (size_t k = 0; k < sha_.bp_.kNv32Elts; ++k) {
          if (b == 0) {
            x[i][k] = lc_.mul(&ebt, vw.sig_sha_[b].h1[i][k]);
          } else {
            auto maybe_sha = lc_.mul(&ebt, vw.sig_sha_[b].h1[i][k]);
            x[i][k] = lc_.add(&x[i][k], maybe_sha);
          }
        }
      }
    }

    EltW h = repack32(x);
    lc_.assert_eq(&h, e);
  }

  EltW repack(const v8 in[], size_t ind) const {
    EltW h = lc_.konst(0);
    EltW base = lc_.konst(0x2);
    for (size_t i = 0; i < 32; ++i) {
      for (size_t j = 0; j < 8; ++j) {
        auto t = lc_.mul(&h, base);
        auto tin = lc_.eval(in[ind + i][7 - j]);
        h = lc_.add(&tin, t);
      }
    }
    return h;
  }

  EltW repack32(const sha_packed_v32 H[]) const {
    EltW h = lc_.konst(0);
    Elt twok = lc_.one();
    for (size_t j = 8; j-- > 0;) {
      auto hj = sha_.bp_.unpack_v32(H[j]);
      for (size_t k = 0; k < 32; ++k) {
        h = lc_.axpy(&h, twok, lc_.eval(hj[k]));
        lc_.f_.add(twok, twok);
      }
    }
    return h;
  }
  Flatsha sha_;
};
}  // namespace proofs

#endif  // PRIVACY_PROOFS_ZK_LIB_CIRCUITS_TESTS_ANONCRED_SMALL_H_
