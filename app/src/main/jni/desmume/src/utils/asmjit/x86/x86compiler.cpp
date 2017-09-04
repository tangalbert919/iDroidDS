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
#include "../base/utils.h"
#include "../x86/x86assembler.h"
#include "../x86/x86compiler.h"
#include "../x86/x86rapass_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86Compiler - Construction / Destruction]
// ============================================================================

X86Compiler::X86Compiler(CodeHolder* code) noexcept : CodeCompiler() {
  if (code)
    code->attach(this);
}
X86Compiler::~X86Compiler() noexcept {}

// ============================================================================
// [asmjit::X86Compiler - Finalize]
// ============================================================================

Error X86Compiler::finalize() {
  if (ASMJIT_UNLIKELY(_lastError))
    return _lastError;

  // Flush the global constant pool.
  if (_globalConstPool) {
    addNode(_globalConstPool);
    _globalConstPool = nullptr;
  }

  ASMJIT_PROPAGATE(runPasses());

  X86Assembler a(_code);
  return serialize(&a);
}

// ============================================================================
// [asmjit::X86Compiler - Events]
// ============================================================================

Error X86Compiler::onAttach(CodeHolder* code) noexcept {
  uint32_t archType = code->getArchType();
  if (!ArchInfo::isX86Family(archType))
    return DebugUtils::errored(kErrorInvalidArch);

  ASMJIT_PROPAGATE(_passes.willGrow(&_allocator));
  ASMJIT_PROPAGATE(Base::onAttach(code));

  if (archType == ArchInfo::kTypeX86)
    _gpRegInfo.setSignature(x86OpData.gpd[0].getSignature());
  else
    _gpRegInfo.setSignature(x86OpData.gpq[0].getSignature());

  return addPassT<X86RAPass>();
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 && !ASMJIT_DISABLE_COMPILER
