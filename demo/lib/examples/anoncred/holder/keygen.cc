#include "examples/anoncred/holder/keygen.h"

#include "examples/anoncred/shared/device_key.h"
#include "examples/anoncred/shared/files.h"

namespace proofs {

bool RunKeygenCommand(const std::filesystem::path& out_dir, std::string* err) {
  HolderKeyMaterial holder_key;
  if (!GenerateHolderKeyMaterial(&holder_key, err)) {
    return false;
  }
  return WriteHolderKeyMaterialDir(out_dir, holder_key, err);
}

}  // namespace proofs
