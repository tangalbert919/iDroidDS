// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_ARM_ARMASSEMBLER_H
#define _ASMJIT_ARM_ARMASSEMBLER_H

// [Dependencies]
#include "../base/assembler.h"
#include "../base/utils.h"
#include "../arm/armemitter.h"
#include "../arm/armoperand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_arm
//! \{

// ============================================================================
// [asmjit::A32Assembler]
// ============================================================================

//! ARM32 assembler.
class ASMJIT_VIRTAPI A32Assembler
  : public Assembler,
    public A32EmitterT<A32Assembler> {

public:
  typedef Assembler Base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API A32Assembler(CodeHolder* code = nullptr) noexcept;
  ASMJIT_API virtual ~A32Assembler() noexcept;

  // --------------------------------------------------------------------------
  // [Compatibility]
  // --------------------------------------------------------------------------

  //! Explicit cast to `A32Emitter`.
  ASMJIT_INLINE A32Emitter* asEmitter() noexcept { return reinterpret_cast<A32Emitter*>(this); }
  //! Explicit cast to `A32Emitter` (const).
  ASMJIT_INLINE const A32Emitter* asEmitter() const noexcept { return reinterpret_cast<const A32Emitter*>(this); }

  //! Implicit cast to `A32Emitter`.
  ASMJIT_INLINE operator A32Emitter&() noexcept { return *asEmitter(); }
  //! Implicit cast to `A32Emitter` (const).
  ASMJIT_INLINE operator const A32Emitter&() const noexcept { return *asEmitter(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get if the current ARM mode is THUMB (only available on A32).
  ASMJIT_INLINE bool isInThumbMode() const noexcept {
    return _archInfo.getSubType() == ArchInfo::kSubTypeA32_Thumb;
  }

  //! Get the current code alignment of the current mode (ARM vs THUMB).
  ASMJIT_INLINE uint32_t getCodeAlignment() const noexcept {
    return isInThumbMode() ? 2 : 4;
  }

  // --------------------------------------------------------------------------
  // [Code-Generation]
  // --------------------------------------------------------------------------

  ASMJIT_API Error _emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) override;
  ASMJIT_API Error align(uint32_t mode, uint32_t alignment) override;

  // --------------------------------------------------------------------------
  // [Events]
  // --------------------------------------------------------------------------

  ASMJIT_API Error onAttach(CodeHolder* code) noexcept override;
  ASMJIT_API Error onDetach(CodeHolder* code) noexcept override;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_ARM_ARMASSEMBLER_H
