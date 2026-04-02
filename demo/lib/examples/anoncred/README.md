# Anonymous Credential CLI Demo

This is a minimal command-line demo built on top of the existing `small`
credential example and the shared Longfellow ZK prover/verifier stack.

## What it does

- `anoncred_holder keygen` generates a holder device key pair
- `anoncred_issuer issue` materializes an example credential bundle and binds it to the holder device public key
- `anoncred_verifier request` creates a fresh verifier request with a random transcript, explicit `now`, and a structured disclosure policy
- `anoncred_holder prove` signs the request transcript with the holder device key and creates a ZK proof
- `anoncred_verifier verify` verifies the proof from the request, presentation, and issuer public bundle

## Supported policies

Legacy request names still work:

- `age_over_18`
- `age_over_21`
- `date_of_birth`

The verifier can also request structured policies directly:

- `--policy date_of_birth`
- `--policy age_range --min-age 18 --max-age 21`
- `--policy age_thresholds --thresholds 18,21`

All age predicates are derived from the credential's `date_of_birth` inside the
proof statement. They no longer rely on the legacy pre-encoded age flag bytes in
the sample credential.

## Build

```bash
CXX=clang++ cmake -D CMAKE_BUILD_TYPE=Release -S lib -B build
cmake --build build -j 16 --target anoncred_issuer anoncred_holder anoncred_verifier anoncred_demo_test
```

## Directory layout

`holder keygen --out <dir>` writes:

- `<dir>/device_sk.txt`
- `<dir>/device_pkx.txt`
- `<dir>/device_pky.txt`

`issuer issue --out <dir>` writes:

- `<dir>/holder`: holder credential material used by `prove`
- `<dir>/issuer_public`: verifier-visible public material used by `verify`

`verifier request --out <dir>` writes:

- `policy_name.txt`
- `policy_type.txt`
- `policy_min_age.txt`
- `policy_max_age.txt`
- `policy_thresholds.txt`
- `transcript.bin`
- `now.txt`

The serialized policy is now explicit. The helper cutoff dates used inside the
proof are derived from this policy at prove/verify time and are no longer stored
as request files.

## Run

Example 1: prove `age_over_18`

```bash
mkdir -p run/demo

./build/examples/anoncred/anoncred_holder keygen \
  --out run/demo/holder_key

./build/examples/anoncred/anoncred_issuer issue \
  --example 0 \
  --holder-key run/demo/holder_key \
  --first-name Alice \
  --family-name Researcher \
  --date-of-birth 19991231 \
  --valid-from 20240101 \
  --valid-until 20271231 \
  --out run/demo/issue

./build/examples/anoncred/anoncred_verifier request \
  --claim age_over_18 \
  --now 20241005 \
  --out run/demo/request

./build/examples/anoncred/anoncred_holder prove \
  --credential run/demo/issue/holder \
  --holder-key run/demo/holder_key \
  --request run/demo/request \
  --out run/demo/presentation

./build/examples/anoncred/anoncred_verifier verify \
  --issuer-public run/demo/issue/issuer_public \
  --request run/demo/request \
  --presentation run/demo/presentation
```

Expected output ends with:

```text
verification ok: age_over_18=true
```

Example 2: prove `age_range 18..21`

```bash
./build/examples/anoncred/anoncred_verifier request \
  --policy age_range \
  --min-age 18 \
  --max-age 21 \
  --now 20241005 \
  --out run/demo/request_range

./build/examples/anoncred/anoncred_holder prove \
  --credential run/demo/issue/holder \
  --holder-key run/demo/holder_key \
  --request run/demo/request_range \
  --out run/demo/presentation_range

./build/examples/anoncred/anoncred_verifier verify \
  --issuer-public run/demo/issue/issuer_public \
  --request run/demo/request_range \
  --presentation run/demo/presentation_range
```

Expected output ends with:

```text
verification ok: age_in_range_18_21=true
```

Example 3: prove threshold conjunction `18,21`

```bash
./build/examples/anoncred/anoncred_verifier request \
  --policy age_thresholds \
  --thresholds 18,21 \
  --now 20241005 \
  --out run/demo/request_thresholds

./build/examples/anoncred/anoncred_holder prove \
  --credential run/demo/issue/holder \
  --holder-key run/demo/holder_key \
  --request run/demo/request_thresholds \
  --out run/demo/presentation_thresholds

./build/examples/anoncred/anoncred_verifier verify \
  --issuer-public run/demo/issue/issuer_public \
  --request run/demo/request_thresholds \
  --presentation run/demo/presentation_thresholds
```

Example 4: disclose `date_of_birth`

```bash
./build/examples/anoncred/anoncred_verifier request \
  --claim date_of_birth \
  --now 20241005 \
  --out run/demo/request_dob

./build/examples/anoncred/anoncred_holder prove \
  --credential run/demo/issue/holder \
  --holder-key run/demo/holder_key \
  --request run/demo/request_dob \
  --out run/demo/presentation_dob

./build/examples/anoncred/anoncred_verifier verify \
  --issuer-public run/demo/issue/issuer_public \
  --request run/demo/request_dob \
  --presentation run/demo/presentation_dob
```

## Behavior checks

Two requests for the same legacy claim still produce different transcripts:

```bash
./build/examples/anoncred/anoncred_verifier request --claim age_over_18 --now 20241005 --out run/demo/request1
./build/examples/anoncred/anoncred_verifier request --claim age_over_18 --now 20241005 --out run/demo/request2
cmp -s run/demo/request1/transcript.bin run/demo/request2/transcript.bin; echo $?
```

Expected result: `1`

## Failure examples

Tamper with the proof:

```bash
cp -r run/demo/presentation run/demo/presentation_bad
printf '\xff' | dd of=run/demo/presentation_bad/proof.bin bs=1 seek=0 count=1 conv=notrunc

./build/examples/anoncred/anoncred_verifier verify \
  --issuer-public run/demo/issue/issuer_public \
  --request run/demo/request \
  --presentation run/demo/presentation_bad
```

Tamper with the disclosed value:

```bash
cp -r run/demo/presentation_dob run/demo/presentation_dob_bad
printf '2' | dd of=run/demo/presentation_dob_bad/disclosed_value.bin bs=1 seek=0 count=1 conv=notrunc

./build/examples/anoncred/anoncred_verifier verify \
  --issuer-public run/demo/issue/issuer_public \
  --request run/demo/request_dob \
  --presentation run/demo/presentation_dob_bad
```

## Test

```bash
ctest --test-dir build -R '^AnoncredDemo\.' --output-on-failure
```
