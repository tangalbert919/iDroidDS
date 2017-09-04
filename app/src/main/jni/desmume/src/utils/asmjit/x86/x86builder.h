// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86BUILDER_H
#define _ASMJIT_X86_X86BUILDER_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_BUILDER)

// [Dependencies]
#include "../base/codebuilder.h"
#include "../base/simdtypes.h"
#include "../x86/x86emitter.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_x86
//! \{

// ============================================================================
// [asmjit::CodeBuilder]
// ============================================================================

//! Architecture-dependent \ref CodeBuilder targeting X86 and X64.
class ASMJIT_VIRTAPI X86Builder
  : public CodeBuilder,
    public X86EmitterImplicitT<X86Builder> {

public:
  ASMJIT_NONCOPYABLE(X86Builder)
  typedef CodeBuilder Base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a `X86Builder` instance.
  ASMJIT_API X86Builder(CodeHolder* code = nullptr) noexcept;
  //! Destroy the `X86Builder` instance.
  ASMJIT_API ~X86Builder() noexcept;

  // --------------------------------------------------------------------------
  // [X86Emitter]
  // --------------------------------------------------------------------------

  //! Explicit cast to `X86Emitter`.
  ASMJIT_INLINE X86Emitter* asEmitter() noexcept { return reinterpret_cast<X86Emitter*>(this); }
  //! Explicit cast to `X86Emitter` (const).
  ASMJIT_INLINE const X86Emitter* asEmitter() const noexcept { return reinterpret_cast<const X86Emitter*>(this); }

  //! Implicit cast to `X86Emitter&`.
  ASMJIT_INLINE operator X86Emitter&() noexcept { return *asEmitter(); }
  //! Implicit cast to `X86Emitter&` (const).
  ASMJIT_INLINE operator const X86Emitter&() const noexcept { return *asEmitter(); }

  // --------------------------------------------------------------------------
  // [Finalize]
  // --------------------------------------------------------------------------

  ASMJIT_API Error finalize() override;

  // --------------------------------------------------------------------------
  // [Events]
  // --------------------------------------------------------------------------

  ASMJIT_API Error onAttach(CodeHolder* code) noexcept override;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_BUILDER
#endif // _ASMJIT_X86_X86BUILDER_H
