// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_ARM_ARMOPERAND_H
#define _ASMJIT_ARM_ARMOPERAND_H

// [Dependencies]
#include "../base/arch.h"
#include "../base/operand.h"
#include "../base/utils.h"
#include "../arm/armglobals.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

class ArmMem;
class ArmReg;
class ArmGp;
class ArmGpW;
class ArmGpX;
class ArmVec;
class ArmVecS;
class ArmVecD;
class ArmVecV;

typedef ArmGpW ArmGpR;

//! \addtogroup asmjit_arm
//! \{

// ============================================================================
// [asmjit::ArmMem]
// ============================================================================

//! Memory operand (ARM/AArch64).
class ArmMem : public Mem {
public:
  //! Additional bits of operand's signature used by `ArmMem`.
  ASMJIT_ENUM(AdditionalBits) {
    kSignatureMemShiftShift   = 16,
    kSignatureMemShiftBits    = 0x1FU,
    kSignatureMemShiftMask    = kSignatureMemShiftBits << kSignatureMemShiftShift,

    kSignatureMemModeShift    = 21,
    kSignatureMemModeBits     = 0x03U,
    kSignatureMemModeMask     = kSignatureMemSegmentBits << kSignatureMemSegmentShift,
  };

  //! Memory addressing mode.
  ASMJIT_ENUM(Mode) {
    kModeOffset               = 0,       //!< Address + offset "[BASE, #Offset]".
    kModePreInc               = 1,       //!< Pre-increment    "[BASE, #Offset]!".
    kModePostInc              = 2        //!< Post-increment   "[BASE], #Offset".
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Construct a default `ArmMem` operand, that points to [0].
  ASMJIT_INLINE ArmMem() noexcept : Mem(NoInit) { reset(); }
  ASMJIT_INLINE ArmMem(const ArmMem& other) noexcept : Mem(other) {}
  explicit ASMJIT_INLINE ArmMem(const _NoInit&) noexcept : Mem(NoInit) {}

  // --------------------------------------------------------------------------
  // [ArmMem]
  // --------------------------------------------------------------------------

  //! Clone the memory operand.
  ASMJIT_INLINE ArmMem clone() const noexcept { return ArmMem(*this); }

  using Mem::setIndex;

  ASMJIT_INLINE void setIndex(const Reg& index, uint32_t shift) noexcept {
    setIndex(index);
    setShift(shift);
  }

  //! Get if the memory operand has shift (aka scale) constant.
  ASMJIT_INLINE bool hasShift() const noexcept { return _hasSignatureData(kSignatureMemShiftMask); }
  //! Get the memory operand's shift (aka scale) constant.
  ASMJIT_INLINE uint32_t getShift() const noexcept { return _getSignatureData(kSignatureMemShiftBits, kSignatureMemShiftShift); }
  //! Set the memory operand's shift (aka scale) constant.
  ASMJIT_INLINE void setShift(uint32_t shift) noexcept { _setSignatureData(shift, kSignatureMemShiftBits, kSignatureMemShiftShift); }
  //! Reset the memory operand's shift (aka scale) constant to zero.
  ASMJIT_INLINE void resetShift() noexcept { _signature &= ~kSignatureMemShiftMask; }

  //! Get the addressing mode, see \ref ArmMem::Mode.
  ASMJIT_INLINE uint32_t getMode() const noexcept { return _getSignatureData(kSignatureMemModeBits, kSignatureMemModeShift); }
  //! Set the addressing mode, see \ref ArmMem::Mode.
  ASMJIT_INLINE void setMode(uint32_t mode) noexcept { _setSignatureData(mode, kSignatureMemModeBits, kSignatureMemModeShift); }
  //! Reset the addressing mode to \ref ArmMem::kModeOffset.
  ASMJIT_INLINE void resetMode() noexcept { _signature &= ~kSignatureMemModeMask; }

  ASMJIT_INLINE bool isOffsetMode() const noexcept { return getMode() == kModeOffset; }
  ASMJIT_INLINE bool isPreIncMode() const noexcept { return getMode() == kModePreInc; }
  ASMJIT_INLINE bool isPostIncMode() const noexcept { return getMode() == kModePostInc; }

  ASMJIT_INLINE ArmMem pre() const noexcept {
    ArmMem result(*this);
    result.setMode(kModePreInc);
    return result;
  }

  ASMJIT_INLINE ArmMem pre(int64_t off) const noexcept {
    ArmMem result(*this);
    result.setMode(kModePreInc);
    result.addOffset(off);
    return result;
  }

  ASMJIT_INLINE ArmMem post() const noexcept {
    ArmMem result(*this);
    result.setMode(kModePreInc);
    return result;
  }

  ASMJIT_INLINE ArmMem post(int64_t off) const noexcept {
    ArmMem result(*this);
    result.setMode(kModePostInc);
    result.addOffset(off);
    return result;
  }

  //! Get new memory operand adjusted by `off`.
  ASMJIT_INLINE ArmMem adjusted(int64_t off) const noexcept {
    ArmMem result(*this);
    result.addOffset(off);
    return result;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE ArmMem& operator=(const ArmMem& other) noexcept { copyFrom(other); return *this; }
};

// ============================================================================
// [asmjit::ArmReg]
// ============================================================================

//! Register traits (ARM/AArch64).
template<uint32_t RegType>
struct ArmRegTraits {
  enum {
    kValid     = 0,                      // RegType is not valid by default.
    kTypeId    = TypeId::kVoid,          // Everything is void by default.

    kType      = 0,                      // Zero type by default.
    kGroup     = 0,                      // Zero group by default.
    kSize      = 0,                      // No size by default.
    kSignature = 0                       // No signature by default.
  };
};

//! Register (ARM/AArch64).
class ArmReg : public Reg {
public:
  //! Register type.
  //!
  //! Don't change these constants; they are essential to some built-in tables.
  ASMJIT_ENUM(RegType) {
    kRegNone      = Reg::kRegNone,       //!< No register type or invalid register.
    kRegGpR       = Reg::kRegGp32,       //!< 32-bit general purpose register (R or W).
    kRegGpW       = Reg::kRegGp32,       //!< 32-bit general purpose register (R or W).
    kRegGpX       = Reg::kRegGp64,       //!< 64-bit general purpose register (X).
    kRegVecS      = Reg::kRegVec32,      //!< 32-bit view of VFP/ASIMD register (S).
    kRegVecD      = Reg::kRegVec64,      //!< 64-bit view of VFP/ASIMD register (D).
    kRegVecV      = Reg::kRegVec128,     //!< 128-bit view of VFP/ASIMD register (Q|V).
    kRegIP        = Reg::kRegIP,         //!< Instruction pointer (A64).
    kRegCount                            //!< Count of register types.
  };

  //! Register group.
  ASMJIT_ENUM(Group) {
    kGroupGp       = Reg::kGroupGp,      //!< General purpose register group.
    kGroupVec      = Reg::kGroupVec,     //!< Vector (VFP/ASIMD) register group.
    kGroupCount                          //!< Count of all ARM register groups.
  };

  ASMJIT_DEFINE_ABSTRACT_REG(ArmReg, Reg)

  //! Get if the register is a GP register (any size).
  ASMJIT_INLINE bool isGp() const noexcept { return _reg.group == kGroupGp; }
  ASMJIT_INLINE bool isGpR() const noexcept { return hasSignature(signatureOf(kRegGpR)); }
  ASMJIT_INLINE bool isGpW() const noexcept { return hasSignature(signatureOf(kRegGpW)); }
  ASMJIT_INLINE bool isGpX() const noexcept { return hasSignature(signatureOf(kRegGpX)); }

  //! Get if the register is a VFP/ASIMD register.
  ASMJIT_INLINE bool isVec() const noexcept { return _reg.group == kGroupVec; }
  ASMJIT_INLINE bool isVecS() const noexcept { return hasSignature(signatureOf(kRegVecS)); }
  ASMJIT_INLINE bool isVecD() const noexcept { return hasSignature(signatureOf(kRegVecD)); }
  ASMJIT_INLINE bool isVecV() const noexcept { return hasSignature(signatureOf(kRegVecV)); }

  template<uint32_t Type>
  ASMJIT_INLINE void setArmRegT(uint32_t id) noexcept {
    setSignature(ArmRegTraits<Type>::kSignature);
    setId(id);
  }

  ASMJIT_INLINE void setTypeAndId(uint32_t regType, uint32_t id) noexcept {
    ASMJIT_ASSERT(regType < kRegCount);
    setSignature(signatureOf(regType));
    setId(id);
  }

  static ASMJIT_INLINE uint32_t groupOf(uint32_t regType) noexcept;
  template<uint32_t Type>
  static ASMJIT_INLINE uint32_t groupOfT() noexcept { return ArmRegTraits<Type>::kGroup; }

  static ASMJIT_INLINE uint32_t signatureOf(uint32_t regType) noexcept;
  template<uint32_t Type>
  static ASMJIT_INLINE uint32_t signatureOfT() noexcept { return ArmRegTraits<Type>::kSignature; }

  static ASMJIT_INLINE bool isGpR(const Operand_& op) noexcept { return op.as<ArmReg>().isGpR(); }
  static ASMJIT_INLINE bool isGpW(const Operand_& op) noexcept { return op.as<ArmReg>().isGpW(); }
  static ASMJIT_INLINE bool isGpX(const Operand_& op) noexcept { return op.as<ArmReg>().isGpX(); }
  static ASMJIT_INLINE bool isGpR(const Operand_& op, uint32_t id) noexcept { return isGpR(op) & (op.getId() == id); }
  static ASMJIT_INLINE bool isGpW(const Operand_& op, uint32_t id) noexcept { return isGpW(op) & (op.getId() == id); }
  static ASMJIT_INLINE bool isGpX(const Operand_& op, uint32_t id) noexcept { return isGpX(op) & (op.getId() == id); }

  static ASMJIT_INLINE bool isVecS(const Operand_& op) noexcept { return op.as<ArmReg>().isVecS(); }
  static ASMJIT_INLINE bool isVecD(const Operand_& op) noexcept { return op.as<ArmReg>().isVecD(); }
  static ASMJIT_INLINE bool isVecV(const Operand_& op) noexcept { return op.as<ArmReg>().isVecV(); }
  static ASMJIT_INLINE bool isVecS(const Operand_& op, uint32_t id) noexcept { return isVecS(op) & (op.getId() == id); }
  static ASMJIT_INLINE bool isVecD(const Operand_& op, uint32_t id) noexcept { return isVecD(op) & (op.getId() == id); }
  static ASMJIT_INLINE bool isVecV(const Operand_& op, uint32_t id) noexcept { return isVecV(op) & (op.getId() == id); }
};

ASMJIT_DEFINE_REG_TRAITS(ArmRegTraits, ArmGpW , ArmReg::kRegGpW , ArmReg::kGroupGp , 4 , 32, TypeId::kI32  );
ASMJIT_DEFINE_REG_TRAITS(ArmRegTraits, ArmGpX , ArmReg::kRegGpX , ArmReg::kGroupGp , 8 , 32, TypeId::kI64  );
ASMJIT_DEFINE_REG_TRAITS(ArmRegTraits, ArmVecS, ArmReg::kRegVecS, ArmReg::kGroupVec, 4 , 32, TypeId::kF32x1);
ASMJIT_DEFINE_REG_TRAITS(ArmRegTraits, ArmVecD, ArmReg::kRegVecD, ArmReg::kGroupVec, 8 , 32, TypeId::kF64x2);
ASMJIT_DEFINE_REG_TRAITS(ArmRegTraits, ArmVecV, ArmReg::kRegVecV, ArmReg::kGroupVec, 16, 32, TypeId::kI32x4);

//! General purpose register (ARM/AArch64).
class ArmGp : public ArmReg {
public:
  ASMJIT_DEFINE_ABSTRACT_REG(ArmGp, ArmReg)

  //! Cast this register to a 32-bit GPR (ARM/AArch32 terminology) and/or GPW (AArch64 terminology).
  ASMJIT_INLINE ArmGpR r() const noexcept;
  //! Cast this register to a 32-bit GPR (ARM/AArch32 terminology) and/or GPW (AArch64 terminology).
  ASMJIT_INLINE ArmGpW w() const noexcept;
  //! Cast this register to a 64-bit GPX (AArch64 only).
  ASMJIT_INLINE ArmGpX x() const noexcept;
};

//! Vector register (ARM/AArch64).
class ArmVec : public ArmReg {
public:
  ASMJIT_DEFINE_ABSTRACT_REG(ArmVec, ArmReg)

  //! Cast this register to a 32-bit S register.
  ASMJIT_INLINE ArmVecS s() const noexcept;
  //! Cast this register to a 64-bit D register.
  ASMJIT_INLINE ArmVecD d() const noexcept;
  //! Cast this register to a 128-bit V register.
  ASMJIT_INLINE ArmVecV v() const noexcept;
};

//! 32-bit GPW (AArch64) and/or GPR (ARM/AArch32) register.
class ArmGpW : public ArmGp { ASMJIT_DEFINE_FINAL_REG(ArmGpW, ArmGp, ArmRegTraits<kRegGpW>) };
//! 64-bit GPX (AArch64) register.
class ArmGpX : public ArmGp { ASMJIT_DEFINE_FINAL_REG(ArmGpX, ArmGp, ArmRegTraits<kRegGpX>) };

//! 32-bit view (S) of VFP/SIMD register.
class ArmVecS : public ArmVec { ASMJIT_DEFINE_FINAL_REG(ArmVecS, ArmVec, ArmRegTraits<kRegVecS>) };
//! 64-bit view (D) of VFP/SIMD register.
class ArmVecD : public ArmVec { ASMJIT_DEFINE_FINAL_REG(ArmVecD, ArmVec, ArmRegTraits<kRegVecD>) };
//! 128-bit vector register (Q or V).
class ArmVecV : public ArmVec { ASMJIT_DEFINE_FINAL_REG(ArmVecV, ArmVec, ArmRegTraits<kRegVecV>) };

ASMJIT_INLINE ArmGpR ArmGp::r() const noexcept { return ArmGpR(getId()); }
ASMJIT_INLINE ArmGpW ArmGp::w() const noexcept { return ArmGpW(getId()); }
ASMJIT_INLINE ArmGpX ArmGp::x() const noexcept { return ArmGpX(getId()); }

ASMJIT_INLINE ArmVecS ArmVec::s() const noexcept { return ArmVecS(getId()); }
ASMJIT_INLINE ArmVecD ArmVec::d() const noexcept { return ArmVecD(getId()); }
ASMJIT_INLINE ArmVecV ArmVec::v() const noexcept { return ArmVecV(getId()); }

ASMJIT_DEFINE_TYPE_ID(ArmGpW, TypeId::kI32);
ASMJIT_DEFINE_TYPE_ID(ArmGpX, TypeId::kI64);
ASMJIT_DEFINE_TYPE_ID(ArmVecS, TypeId::kF32x1);
ASMJIT_DEFINE_TYPE_ID(ArmVecD, TypeId::kF64x1);
ASMJIT_DEFINE_TYPE_ID(ArmVecV, TypeId::kI32x4);

// ============================================================================
// [asmjit::ArmOpData]
// ============================================================================

struct ArmOpData {
  // --------------------------------------------------------------------------
  // [Signatures]
  // --------------------------------------------------------------------------

  //! Information about all architecture registers.
  ArchRegs archRegs;

  // --------------------------------------------------------------------------
  // [Operands]
  // --------------------------------------------------------------------------

  // Prevent calling constructors of these registers when exporting.
#if defined(ASMJIT_EXPORTS_ARM_OPERAND)
# define ASMJIT_ARM_REG_DATA(REG) Operand_
#else
# define ASMJIT_ARM_REG_DATA(REG) REG
#endif
  ASMJIT_ARM_REG_DATA(ArmGpW) gpw[32];
  ASMJIT_ARM_REG_DATA(ArmGpX) gpx[32];
#undef ASMJIT_ARM_REG_DATA
};
ASMJIT_VARAPI const ArmOpData armOpData;

// ... ArmReg methods that require `armOpData`.
ASMJIT_INLINE uint32_t ArmReg::signatureOf(uint32_t regType) noexcept {
  ASMJIT_ASSERT(regType <= Reg::kRegMax);
  return armOpData.archRegs.regInfo[regType].signature;
}

ASMJIT_INLINE uint32_t ArmReg::groupOf(uint32_t regType) noexcept {
  ASMJIT_ASSERT(regType <= Reg::kRegMax);
  return armOpData.archRegs.regInfo[regType].group;
}

// ============================================================================
// [asmjit::arm]
// ============================================================================

namespace arm {

// ============================================================================
// [asmjit::arm - Reg]
// ============================================================================

#if !defined(ASMJIT_EXPORTS_ARM_OPERAND)
namespace {
#define ASMJIT_ARM_PHYS_REG(TYPE, NAME, PROPERTY) \
  static const TYPE& NAME = armOpData.PROPERTY

ASMJIT_ARM_PHYS_REG(ArmGpW , w0   , gpw[0]);    //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w1   , gpw[1]);    //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w2   , gpw[2]);    //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w3   , gpw[3]);    //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w4   , gpw[4]);    //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w5   , gpw[5]);    //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w6   , gpw[6]);    //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w7   , gpw[7]);    //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w8   , gpw[8]);    //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w9   , gpw[9]);    //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w10  , gpw[10]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w11  , gpw[11]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w12  , gpw[12]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w13  , gpw[13]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w14  , gpw[14]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w15  , gpw[15]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w16  , gpw[16]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w17  , gpw[17]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w18  , gpw[18]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w19  , gpw[19]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w20  , gpw[20]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w21  , gpw[21]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w22  , gpw[22]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w23  , gpw[23]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w24  , gpw[24]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w25  , gpw[25]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w26  , gpw[26]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w27  , gpw[27]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w28  , gpw[28]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w29  , gpw[29]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w30  , gpw[30]);   //!< 32-bit GPR/GPW.
ASMJIT_ARM_PHYS_REG(ArmGpW , w31  , gpw[31]);   //!< 32-bit GPR/GPW.

ASMJIT_ARM_PHYS_REG(ArmGpX , x0   , gpx[0]);    //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x1   , gpx[1]);    //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x2   , gpx[2]);    //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x3   , gpx[3]);    //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x4   , gpx[4]);    //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x5   , gpx[5]);    //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x6   , gpx[6]);    //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x7   , gpx[7]);    //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x8   , gpx[8]);    //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x9   , gpx[9]);    //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x10  , gpx[10]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x11  , gpx[11]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x12  , gpx[12]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x13  , gpx[13]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x14  , gpx[14]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x15  , gpx[15]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x16  , gpx[16]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x17  , gpx[17]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x18  , gpx[18]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x19  , gpx[19]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x20  , gpx[20]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x21  , gpx[21]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x22  , gpx[22]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x23  , gpx[23]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x24  , gpx[24]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x25  , gpx[25]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x26  , gpx[26]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x27  , gpx[27]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x28  , gpx[28]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x29  , gpx[29]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x30  , gpx[30]);   //!< 64-bit GPX.
ASMJIT_ARM_PHYS_REG(ArmGpX , x31  , gpx[31]);   //!< 64-bit GPX.

#undef ASMJIT_ARM_PHYS_REG
} // anonymous namespace
#endif // !ASMJIT_EXPORTS_ARM_OPERAND

//! Create a 32-bit W register operand (ARM/AArch64).
static ASMJIT_INLINE ArmGpW w(uint32_t id) noexcept { return ArmGpW(id); }
//! Create a 64-bit X register operand (AArch64).
static ASMJIT_INLINE ArmGpX x(uint32_t id) noexcept { return ArmGpX(id); }

//! Create a 32-bit S register operand (ARM/AArch64).
static ASMJIT_INLINE ArmVecS s(uint32_t id) noexcept { return ArmVecS(id); }
//! Create a 64-bit D register operand (ARM/AArch64).
static ASMJIT_INLINE ArmVecD d(uint32_t id) noexcept { return ArmVecD(id); }
//! Create a 1282-bit V register operand (ARM/AArch64).
static ASMJIT_INLINE ArmVecV v(uint32_t id) noexcept { return ArmVecV(id); }

// ============================================================================
// [asmjit::arm - Ptr (Reg)]
// ============================================================================

} // arm namespace

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_ARM_ARMOPERAND_H
