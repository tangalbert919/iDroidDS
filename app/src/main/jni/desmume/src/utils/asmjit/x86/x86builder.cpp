// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if defined(ASMJIT_BUILD_X86) && !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../x86/x86assembler.h"
#include "../x86/x86builder.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86Builder - Construction / Destruction]
// ============================================================================

X86Builder::X86Builder(CodeHolder* code) noexcept : CodeBuilder() {
  if (code)
    code->attach(this);
}
X86Builder::~X86Builder() noexcept {}

// ============================================================================
// [asmjit::X86Builder - Finalize]
// ============================================================================

Error X86Builder::finalize() {
  ASMJIT_PROPAGATE(runPasses());

  X86Assembler a(_code);
  return serialize(&a);
}

// ============================================================================
// [asmjit::X86Builder - Events]
// ============================================================================

Error X86Builder::onAttach(CodeHolder* code) noexcept {
  uint32_t archType = code->getArchType();
  if (!ArchInfo::isX86Family(archType))
    return DebugUtils::errored(kErrorInvalidArch);

  ASMJIT_PROPAGATE(Base::onAttach(code));

  if (archType == ArchInfo::kTypeX86)
    _gpRegInfo.setSignature(x86OpData.gpd[0].getSignature());
  else
    _gpRegInfo.setSignature(x86OpData.gpq[0].getSignature());

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 && !ASMJIT_DISABLE_COMPILER
