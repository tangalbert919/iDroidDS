// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_FUNC_H
#define _ASMJIT_BASE_FUNC_H

// [Dependencies]
#include "../base/arch.h"
#include "../base/intutils.h"
#include "../base/operand.h"
#include "../base/utils.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class CodeEmitter;

// ============================================================================
// [asmjit::CallConv]
// ============================================================================

//! Function calling convention.
//!
//! Function calling convention is a scheme that defines how function parameters
//! are passed and how function returns its result. AsmJit defines a variety of
//! architecture and OS specific calling conventions and also provides a compile
//! time detection to make the code-generation easier.
struct CallConv {
  //! Calling convention id.
  ASMJIT_ENUM(Id) {
    //! None or invalid (can't be used).
    kIdNone = 0,

    // ------------------------------------------------------------------------
    // [Universal]
    // ------------------------------------------------------------------------

    // TODO: To make this possible we need to know target ARCH and ABI.

    /*

    // Universal calling conventions are applicable to any target and are
    // converted to target dependent conventions at runtime. The purpose of
    // these conventions is to make using functions less target dependent.

    kIdCDecl = 1,
    kIdStdCall = 2,
    kIdFastCall = 3,

    //! AsmJit specific calling convention designed for calling functions
    //! inside a multimedia code like that don't use many registers internally,
    //! but are long enough to be called and not inlined. These functions are
    //! usually used to calculate trigonometric functions, logarithms, etc...
    kIdFastEval2 = 10,
    kIdFastEval3 = 11,
    kIdFastEval4 = 12,
    */

    // ------------------------------------------------------------------------
    // [X86]
    // ------------------------------------------------------------------------

    //! X86 `__cdecl` calling convention (used by C runtime and libraries).
    kIdX86CDecl = 16,
    //! X86 `__stdcall` calling convention (used mostly by WinAPI).
    kIdX86StdCall = 17,
    //! X86 `__thiscall` calling convention (MSVC/Intel).
    kIdX86MsThisCall = 18,
    //! X86 `__fastcall` convention (MSVC/Intel).
    kIdX86MsFastCall = 19,
    //! X86 `__fastcall` convention (GCC and Clang).
    kIdX86GccFastCall = 20,
    //! X86 `regparm(1)` convention (GCC and Clang).
    kIdX86GccRegParm1 = 21,
    //! X86 `regparm(2)` convention (GCC and Clang).
    kIdX86GccRegParm2 = 22,
    //! X86 `regparm(3)` convention (GCC and Clang).
    kIdX86GccRegParm3 = 23,

    kIdX86FastEval2 = 29,
    kIdX86FastEval3 = 30,
    kIdX86FastEval4 = 31,

    //! X64 calling convention - WIN64-ABI.
    kIdX86Win64 = 32,
    //! X64 calling convention - SystemV / AMD64-ABI.
    kIdX86SysV64 = 33,

    kIdX64FastEval2 = 45,
    kIdX64FastEval3 = 46,
    kIdX64FastEval4 = 47,

    // ------------------------------------------------------------------------
    // [ARM]
    // ------------------------------------------------------------------------

    //! Legacy calling convention, floating point arguments are passed via GP registers.
    kIdArm32SoftFP = 48,
    //! Modern calling convention, uses VFP registers to pass floating point arguments.
    kIdArm32HardFP = 49,

    // ------------------------------------------------------------------------
    // [Internal]
    // ------------------------------------------------------------------------

    _kIdX86Start = 16,  //!< \internal
    _kIdX86End = 31,    //!< \internal

    _kIdX64Start = 32,  //!< \internal
    _kIdX64End = 47,    //!< \internal

    _kIdArmStart = 48,  //!< \internal
    _kIdArmEnd = 49,    //!< \internal

    // ------------------------------------------------------------------------
    // [Host]
    // ------------------------------------------------------------------------

#if defined(ASMJIT_DOCGEN)
    //! Default calling convention based on the current C++ compiler's settings.
    //!
    //! NOTE: This should be always the same as `kIdHostCDecl`, but some
    //! compilers allow to override the default calling convention. Overriding
    //! is not detected at the moment.
    kIdHost          = DETECTED_AT_COMPILE_TIME,

    //! Default CDECL calling convention based on the current C++ compiler's settings.
    kIdHostCDecl     = DETECTED_AT_COMPILE_TIME,

    //! Default STDCALL calling convention based on the current C++ compiler's settings.
    //!
    //! NOTE: If not defined by the host then it's the same as `kIdHostCDecl`.
    kIdHostStdCall   = DETECTED_AT_COMPILE_TIME,

    //! Compatibility for `__fastcall` calling convention.
    //!
    //! NOTE: If not defined by the host then it's the same as `kIdHostCDecl`.
    kIdHostFastCall  = DETECTED_AT_COMPILE_TIME
#elif ASMJIT_ARCH_X86
    kIdHost          = kIdX86CDecl,
    kIdHostCDecl     = kIdX86CDecl,
    kIdHostStdCall   = kIdX86StdCall,
    kIdHostFastCall  = ASMJIT_CC_MSC   ? kIdX86MsFastCall  :
                       ASMJIT_CC_GCC   ? kIdX86GccFastCall :
                       ASMJIT_CC_CLANG ? kIdX86GccFastCall : kIdNone,
    kIdHostFastEval2 = kIdX86FastEval2,
    kIdHostFastEval3 = kIdX86FastEval3,
    kIdHostFastEval4 = kIdX86FastEval4
#elif ASMJIT_ARCH_X64
    kIdHost          = ASMJIT_OS_WINDOWS ? kIdX86Win64 : kIdX86SysV64,
    kIdHostCDecl     = kIdHost, // Doesn't exist, redirected to host.
    kIdHostStdCall   = kIdHost, // Doesn't exist, redirected to host.
    kIdHostFastCall  = kIdHost, // Doesn't exist, redirected to host.
    kIdHostFastEval2 = kIdX64FastEval2,
    kIdHostFastEval3 = kIdX64FastEval3,
    kIdHostFastEval4 = kIdX64FastEval4
#elif ASMJIT_ARCH_ARM32
# if defined(__SOFTFP__)
    kIdHost          = kIdArm32SoftFP,
# else
    kIdHost          = kIdArm32HardFP,
# endif
    // These don't exist on ARM.
    kIdHostCDecl     = kIdHost, // Doesn't exist, redirected to host.
    kIdHostStdCall   = kIdHost, // Doesn't exist, redirected to host.
    kIdHostFastCall  = kIdHost  // Doesn't exist, redirected to host.
#else
# error "[asmjit] Couldn't determine the target's calling convention."
#endif
  };

  //! Strategy used to assign registers to function arguments.
  //!
  //! This is AsmJit specific. It basically describes how should AsmJit convert
  //! the function arguments defined by `FuncSignature` into register IDs or
  //! stack offsets. The default strategy assigns registers and then stack.
  //! The Win64 strategy does register shadowing as defined by `WIN64` calling
  //! convention - it applies to 64-bit calling conventions only.
  ASMJIT_ENUM(Strategy) {
    kStrategyDefault     = 0,            //!< Default register assignment strategy.
    kStrategyWin64       = 1             //!< WIN64 specific register assignment strategy.
  };

  //! Calling convention flags.
  ASMJIT_ENUM(Flags) {
    kFlagCalleePopsStack = 0x01,         //!< Callee is responsible for cleaning up the stack.
    kFlagPassFloatsByVec = 0x02,         //!< Pass F32 and F64 arguments by VEC128 register.
    kFlagVectorCall      = 0x04,         //!< This is a '__vectorcall' calling convention.
    kFlagIndirectVecArgs = 0x08          //!< Pass vector arguments indirectly (as a pointer).
  };

  //! Internal limits of AsmJit/CallConv.
  ASMJIT_ENUM(Limits) {
    kMaxRegArgsPerGroup  = 16
  };

  //! Passed registers' order.
  union RegOrder {
    uint8_t id[kMaxRegArgsPerGroup];     //!< Passed registers, ordered.
    uint32_t packed[(kMaxRegArgsPerGroup + 3) / 4];
  };

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  static ASMJIT_INLINE bool isX86Family(uint32_t ccId) noexcept { return ccId >= _kIdX86Start && ccId <= _kIdX64End; }
  static ASMJIT_INLINE bool isArmFamily(uint32_t ccId) noexcept { return ccId >= _kIdArmStart && ccId <= _kIdArmEnd; }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_API Error init(uint32_t ccId) noexcept;

  ASMJIT_INLINE void reset() noexcept {
    ::memset(this, 0, sizeof(*this));
    ::memset(_passedOrder, 0xFF, sizeof(_passedOrder));
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get calling convention id, see \ref Id.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _id; }
  //! Set calling convention id, see \ref Id.
  ASMJIT_INLINE void setId(uint32_t id) noexcept { _id = IntUtils::toUInt8(id); }

  //! Get architecture type.
  ASMJIT_INLINE uint32_t getArchType() const noexcept { return _archType; }
  //! Set architecture type.
  ASMJIT_INLINE void setArchType(uint32_t archType) noexcept { _archType = IntUtils::toUInt8(archType); }

  //! Get a strategy used to assign registers to arguments, see \ref Strategy.
  ASMJIT_INLINE uint32_t getStrategy() const noexcept { return _strategy; }
  //! Set a strategy used to assign registers to arguments, see \ref Strategy.
  ASMJIT_INLINE void setStrategy(uint32_t strategy) noexcept { _strategy = IntUtils::toUInt8(strategy); }

  //! Get if the calling convention has the given `flag` set.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) const noexcept { return (static_cast<uint32_t>(_flags) & flag) != 0; }
  //! Get calling convention flags, see \ref Flags.
  ASMJIT_INLINE uint32_t getFlags() const noexcept { return static_cast<uint32_t>(_flags); }
  //! Add calling convention flags, see \ref Flags.
  ASMJIT_INLINE void setFlags(uint32_t flag) noexcept { _flags = IntUtils::toUInt8(flag); };
  //! Add calling convention flags, see \ref Flags.
  ASMJIT_INLINE void addFlags(uint32_t flags) noexcept { _flags = IntUtils::toUInt8(_flags | flags); };

  //! Get if this calling convention specifies 'RedZone'.
  ASMJIT_INLINE bool hasRedZone() const noexcept { return _redZoneSize != 0; }
  //! Get size of 'RedZone'.
  ASMJIT_INLINE uint32_t getRedZoneSize() const noexcept { return _redZoneSize; }
  //! Set size of 'RedZone'.
  ASMJIT_INLINE void setRedZoneSize(uint32_t size) noexcept { _redZoneSize = IntUtils::toUInt8(size); }

  //! Get if this calling convention specifies 'SpillZone'.
  ASMJIT_INLINE bool hasSpillZone() const noexcept { return _spillZoneSize != 0; }
  //! Get size of 'SpillZone'.
  ASMJIT_INLINE uint32_t getSpillZoneSize() const noexcept { return _spillZoneSize; }
  //! Set size of 'SpillZone'.
  ASMJIT_INLINE void setSpillZoneSize(uint32_t size) noexcept { _spillZoneSize = IntUtils::toUInt8(size); }

  //! Get a natural stack alignment.
  ASMJIT_INLINE uint32_t getNaturalStackAlignment() const noexcept { return _naturalStackAlignment; }
  //! Set a natural stack alignment.
  //!
  //! This function can be used to override the default stack alignment in case
  //! that you know that it's alignment is different. For example it allows to
  //! implement custom calling conventions that guarantee higher stack alignment.
  ASMJIT_INLINE void setNaturalStackAlignment(uint32_t value) noexcept { _naturalStackAlignment = IntUtils::toUInt8(value); }

  ASMJIT_INLINE const uint8_t* getPassedOrder(uint32_t group) const noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    return _passedOrder[group].id;
  }

  ASMJIT_INLINE uint32_t getPassedRegs(uint32_t group) const noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    return _passedRegs[group];
  }

  ASMJIT_INLINE void _setPassedPacked(uint32_t group, uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);

    _passedOrder[group].packed[0] = p0;
    _passedOrder[group].packed[1] = p1;
    _passedOrder[group].packed[2] = p2;
    _passedOrder[group].packed[3] = p3;
  }

  ASMJIT_INLINE void setPassedToNone(uint32_t group) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);

    _setPassedPacked(group, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU);
    _passedRegs[group] = 0U;
  }

  ASMJIT_INLINE void setPassedOrder(uint32_t group, uint32_t a0, uint32_t a1 = 0xFF, uint32_t a2 = 0xFF, uint32_t a3 = 0xFF, uint32_t a4 = 0xFF, uint32_t a5 = 0xFF, uint32_t a6 = 0xFF, uint32_t a7 = 0xFF) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);

    // NOTE: This should always be called with all arguments known at compile time,
    // so even if it looks scary it should be translated into few instructions.
    _setPassedPacked(group, ASMJIT_PACK32_4x8(a0, a1, a2, a3),
                            ASMJIT_PACK32_4x8(a4, a5, a6, a7),
                            0xFFFFFFFFU,
                            0xFFFFFFFFU);

    _passedRegs[group] = (a0 != 0xFF ? 1U << a0 : 0U) |
                         (a1 != 0xFF ? 1U << a1 : 0U) |
                         (a2 != 0xFF ? 1U << a2 : 0U) |
                         (a3 != 0xFF ? 1U << a3 : 0U) |
                         (a4 != 0xFF ? 1U << a4 : 0U) |
                         (a5 != 0xFF ? 1U << a5 : 0U) |
                         (a6 != 0xFF ? 1U << a6 : 0U) |
                         (a7 != 0xFF ? 1U << a7 : 0U) ;
  }

  ASMJIT_INLINE uint32_t getPreservedRegs(uint32_t group) const noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    return _preservedRegs[group];
  }

  ASMJIT_INLINE void setPreservedRegs(uint32_t group, uint32_t regs) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    _preservedRegs[group] = regs;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _id;                           //!< Calling convention id, see \ref Id.
  uint8_t _archType;                     //!< Architecture type (see \ref ArchInfo::Type).
  uint8_t _strategy;                     //!< Register assignment strategy.
  uint8_t _flags;                        //!< Flags.

  uint8_t _redZoneSize;                  //!< Red zone size (AMD64 == 128 bytes).
  uint8_t _spillZoneSize;                //!< Spill zone size (WIN64 == 32 bytes).
  uint8_t _naturalStackAlignment;        //!< Natural stack alignment as defined by OS/ABI.
  uint8_t _reserved[1];

  uint32_t _passedRegs[Reg::kGroupVirt];    //!< Mask of all passed registers, per group.
  uint32_t _preservedRegs[Reg::kGroupVirt]; //!< Mask of all preserved registers, per group.
  RegOrder _passedOrder[Reg::kGroupVirt];   //!< Passed registers' order, per group.
};

// ============================================================================
// [asmjit::FuncArgIndex]
// ============================================================================

//! Function argument index (lo/hi).
ASMJIT_ENUM(FuncArgIndex) {
  //! Maximum number of function arguments supported by AsmJit.
  kFuncArgCount = 16,
  //! Extended maximum number of arguments (used internally).
  kFuncArgCountLoHi = kFuncArgCount * 2,

  //! Index to the LO part of function argument (default).
  //!
  //! This value is typically omitted and added only if there is HI argument
  //! accessed.
  kFuncArgLo = 0,

  //! Index to the HI part of function argument.
  //!
  //! HI part of function argument depends on target architecture. On x86 it's
  //! typically used to transfer 64-bit integers (they form a pair of 32-bit
  //! integers).
  kFuncArgHi = kFuncArgCount
};

// ============================================================================
// [asmjit::FuncSignature]
// ============================================================================

//! Function signature.
//!
//! Contains information about function return type, count of arguments and
//! their TypeIds. Function signature is a low level structure which doesn't
//! contain platform specific or calling convention specific information.
struct FuncSignature {
  enum {
    //! Doesn't have variable number of arguments (`...`).
    kNoVarArgs = 0xFF
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  //! Initialize the function signature.
  ASMJIT_INLINE void init(uint32_t ccId, uint32_t ret, const uint8_t* args, uint32_t argCount) noexcept {
    ASMJIT_ASSERT(ccId <= 0xFF);
    ASMJIT_ASSERT(argCount <= 0xFF);

    _callConv = IntUtils::toUInt8(ccId);
    _argCount = IntUtils::toUInt8(argCount);
    _vaIndex = kNoVarArgs;
    _ret = IntUtils::toUInt8(ret);
    _args = args;
  }

  ASMJIT_INLINE void reset() noexcept { ::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the function's calling convention.
  ASMJIT_INLINE uint32_t getCallConv() const noexcept { return _callConv; }

  //! Get if the function has variable number of arguments (...).
  ASMJIT_INLINE bool hasVarArgs() const noexcept { return _vaIndex != kNoVarArgs; }
  //! Get the variable arguments (...) index, `kNoVarArgs` if none.
  ASMJIT_INLINE uint32_t getVAIndex() const noexcept { return _vaIndex; }

  //! Get the number of function arguments.
  ASMJIT_INLINE uint32_t getArgCount() const noexcept { return _argCount; }

  ASMJIT_INLINE bool hasRet() const noexcept { return _ret != TypeId::kVoid; }
  //! Get the return value type.
  ASMJIT_INLINE uint32_t getRet() const noexcept { return _ret; }

  //! Get the type of the argument at index `i`.
  ASMJIT_INLINE uint32_t getArg(uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < _argCount);
    return _args[i];
  }
  //! Get the array of function arguments' types.
  ASMJIT_INLINE const uint8_t* getArgs() const noexcept { return _args; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _callConv;                     //!< Calling convention id.
  uint8_t _argCount;                     //!< Count of arguments.
  uint8_t _vaIndex;                      //!< Index of a first VA or `kNoVarArgs`.
  uint8_t _ret;                          //!< Return value TypeId.
  const uint8_t* _args;                  //!< Function arguments TypeIds.
};

// ============================================================================
// [asmjit::FuncSignatureT]
// ============================================================================

//! \internal
#define T(TYPE) TypeIdOf<TYPE>::kTypeId

//! Static function signature (no arguments).
template<typename RET>
class FuncSignature0 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature0(uint32_t ccId = CallConv::kIdHost) noexcept {
    init(ccId, T(RET), nullptr, 0);
  }
};

//! Static function signature (1 argument).
template<typename RET, typename A0>
class FuncSignature1 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature1(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0) };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Static function signature (2 arguments).
template<typename RET, typename A0, typename A1>
class FuncSignature2 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature2(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1) };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Static function signature (3 arguments).
template<typename RET, typename A0, typename A1, typename A2>
class FuncSignature3 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature3(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2) };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Static function signature (4 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3>
class FuncSignature4 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature4(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3) };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Static function signature (5 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4>
class FuncSignature5 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature5(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4) };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Static function signature (6 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
class FuncSignature6 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature6(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5) };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Static function signature (7 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
class FuncSignature7 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature7(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6) };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Static function signature (8 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
class FuncSignature8 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature8(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6), T(A7) };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Static function signature (9 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
class FuncSignature9 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature9(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6), T(A7), T(A8) };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Static function signature (10 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
class FuncSignature10 : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignature10(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6), T(A7), T(A8), T(A9) };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

#if ASMJIT_CC_HAS_VARIADIC_TEMPLATES
//! Static function signature (variadic template).
template<typename RET, typename... ARGS>
class FuncSignatureT : public FuncSignature {
public:
  ASMJIT_INLINE FuncSignatureT(uint32_t ccId = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { (T(ARGS))... };
    init(ccId, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};
#endif

#undef T

// ============================================================================
// [asmjit::FuncSignatureX]
// ============================================================================

//! Dynamic function signature.
class FuncSignatureX : public FuncSignature {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE FuncSignatureX(uint32_t ccId = CallConv::kIdHost) noexcept {
    init(ccId, TypeId::kVoid, _builderArgList, 0);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void setCallConv(uint32_t ccId) noexcept {
    ASMJIT_ASSERT(ccId <= 0xFF);
    _callConv = IntUtils::toUInt8(ccId);
  }

  //! Set the return type to `retType`.
  ASMJIT_INLINE void setRet(uint32_t retType) noexcept { _ret = IntUtils::toUInt8(retType); }
  //! Set the return type based on `T`.
  template<typename T>
  ASMJIT_INLINE void setRetT() noexcept { setRet(TypeIdOf<T>::kTypeId); }

  //! Set the argument at index `i` to `argType`.
  ASMJIT_INLINE void setArg(uint32_t i, uint32_t argType) noexcept {
    ASMJIT_ASSERT(i < _argCount);
    _builderArgList[i] = IntUtils::toUInt8(argType);
  }
  //! Set the argument at index `i` to the type based on `T`.
  template<typename T>
  ASMJIT_INLINE void setArgT(uint32_t i) noexcept { setArg(i, TypeIdOf<T>::kTypeId); }

  //! Append an argument of `type` to the function prototype.
  ASMJIT_INLINE void addArg(uint32_t type) noexcept {
    ASMJIT_ASSERT(_argCount < kFuncArgCount);
    _builderArgList[_argCount++] = IntUtils::toUInt8(type);
  }
  //! Append an argument of type based on `T` to the function prototype.
  template<typename T>
  ASMJIT_INLINE void addArgT() noexcept { addArg(TypeIdOf<T>::kTypeId); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _builderArgList[kFuncArgCount];
};

// ============================================================================
// [asmjit::FuncValue]
// ============================================================================

//! Argument or return value as defined by `FuncSignature`, but with register
//! or stack address (and other metadata) assigned to it.
struct FuncValue {
  ASMJIT_ENUM(Parts) {
    kStackOffsetShift = 0,
    kStackOffsetMask  = 0x0000FFFFU,

    kRegIdShift       = 0,
    kRegIdMask        = 0x000000FFU,

    kRegTypeShift     = 8,
    kRegTypeMask      = 0x0000FF00U,

    kIsReg            = 0x00010000U,   //!< Passed by register.
    kIsStack          = 0x00020000U,   //!< Passed by stack.
    kIsIndirect       = 0x00040000U,   //!< Passed indirectly by reference (internally a pointer).
    kIsDone           = 0x00080000U,   //!< Used internally by arguments allocator.

    kTypeIdShift      = 24,
    kTypeIdMask       = 0xFF000000U
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  //! Initialize this in/out by a given `typeId`.
  ASMJIT_INLINE void init(uint32_t typeId) noexcept { _data = typeId << kTypeIdShift; }

  ASMJIT_INLINE void initReg(uint32_t regType, uint32_t regId, uint32_t typeId, uint32_t flags = 0) noexcept {
    _data = (regType << kRegTypeShift) | (regId << kRegIdShift) | (typeId << kTypeIdShift) | kIsReg | flags;
  }

  ASMJIT_INLINE void initStack(uint32_t offset, uint32_t typeId) noexcept {
    _data = (offset << kStackOffsetShift) | (typeId << kTypeIdShift) | kIsStack;
  }

  //! Reset the value to its uninitialized and unassigned state.
  ASMJIT_INLINE void reset() noexcept { _data = 0; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool hasFlag(uint32_t flag) const noexcept { return (_data & flag) != 0; }
  ASMJIT_INLINE void addFlags(uint32_t flags) noexcept { _data |= flags; }
  ASMJIT_INLINE void clearFlags(uint32_t flags) noexcept { _data &= ~flags; }

  //! Get if this value is initialized (i.e. contains a valid data).
  ASMJIT_INLINE bool isInitialized() const noexcept { return _data != 0; }
  //! Get if this argument is passed by register.
  ASMJIT_INLINE bool isReg() const noexcept { return hasFlag(kIsReg); }
  //! Get if this argument is passed by stack.
  ASMJIT_INLINE bool isStack() const noexcept { return hasFlag(kIsStack); }
  //! Get if this argument is passed by register.
  ASMJIT_INLINE bool isAssigned() const noexcept { return hasFlag(kIsReg | kIsStack); }
  //! Get if this argument is passed through a pointer (used by WIN64 to pass XMM|YMM|ZMM).
  ASMJIT_INLINE bool isIndirect() const noexcept { return hasFlag(kIsIndirect); }

  ASMJIT_INLINE bool isDone() const noexcept { return hasFlag(kIsDone); }

  //! Get a register type of the register used to pass the argument or return the value.
  ASMJIT_INLINE uint32_t getRegType() const noexcept { return (_data & kRegTypeMask) >> kRegTypeShift; }
  ASMJIT_INLINE void setRegType(uint32_t regType) noexcept { _data = (_data & ~kRegTypeMask) | (regType << kRegTypeShift); }

  //! Get a physical id of the register used to pass the argument or return the value.
  ASMJIT_INLINE uint32_t getRegId() const noexcept { return (_data & kRegIdMask) >> kRegIdShift; }
  ASMJIT_INLINE void setRegId(uint32_t regId) noexcept { _data = (_data & ~kRegIdMask) | (regId << kRegIdShift); }

  ASMJIT_INLINE void addRegData(uint32_t type, uint32_t id) noexcept {
    ASMJIT_ASSERT((_data & (kRegTypeMask | kRegIdMask)) == 0);
    _data |= (type << kRegTypeShift) | (id << kRegIdShift) | kIsReg;
  }

  //! Get a stack offset of this argument (always zero or positive).
  ASMJIT_INLINE uint32_t getStackOffset() const noexcept {
    return (_data & kStackOffsetMask) >> kStackOffsetShift;
  }

  // TODO: INVALID NAME, AMBIGUOUS.
  ASMJIT_INLINE void addStackOffset(uint32_t offset) noexcept {
    ASMJIT_ASSERT((_data & kStackOffsetMask) == 0);
    _data |= (offset << kStackOffsetShift) | kIsStack;
  }

  //! Get virtual type of this argument or return value.
  ASMJIT_INLINE uint32_t getTypeId() const noexcept {
    return _data >> kTypeIdShift;
  }

  ASMJIT_INLINE void setTypeId(uint32_t typeId) noexcept {
    _data = (_data & ~(kTypeIdMask)) | (typeId << kTypeIdShift);
  }

  ASMJIT_INLINE void addTypeId(uint32_t typeId) noexcept {
    ASMJIT_ASSERT((_data & kTypeIdMask) != 0);
    _data |= (typeId << kTypeIdShift);
  }


  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _data;
};

// ============================================================================
// [asmjit::FuncDetail]
// ============================================================================

//! Function detail - CallConv and expanded FuncSignature.
//!
//! Function details is architecture and OS dependent representation of function.
//! It contains calling convention and expanded function signature so all
//! arguments have assigned either register type & id or stack address.
class FuncDetail {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE FuncDetail() noexcept { reset(); }
  ASMJIT_INLINE FuncDetail(const FuncDetail& other) noexcept { ::memcpy(this, &other, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  //! Initialize this `FuncDetail` to the given signature.
  ASMJIT_API Error init(const FuncSignature& sign);
  ASMJIT_INLINE void reset() noexcept { ::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Accessors - Calling Convention]
  // --------------------------------------------------------------------------

  //! Get the function's calling convention, see `CallConv`.
  ASMJIT_INLINE const CallConv& getCallConv() const noexcept { return _callConv; }

  //! Get CallConv flags, see \ref CallConv::Flags.
  ASMJIT_INLINE uint32_t getFlags() const noexcept { return _callConv.getFlags(); }
  //! Check if a CallConv `flag` is set, see \ref CallConv::Flags.
  ASMJIT_INLINE bool hasFlag(uint32_t ccFlag) const noexcept { return _callConv.hasFlag(ccFlag); }

  // --------------------------------------------------------------------------
  // [Accessors - Arguments and Return]
  // --------------------------------------------------------------------------

  //! Get count of function return values.
  ASMJIT_INLINE uint32_t getRetCount() const noexcept { return _retCount; }
  //! Get the number of function arguments.
  ASMJIT_INLINE uint32_t getArgCount() const noexcept { return _argCount; }

  //! Get whether the function has a return value.
  ASMJIT_INLINE bool hasRet() const noexcept { return _retCount != 0; }
  //! Get function return value.
  ASMJIT_INLINE FuncValue& getRet(uint32_t index = 0) noexcept {
    ASMJIT_ASSERT(index < ASMJIT_ARRAY_SIZE(_rets));
    return _rets[index];
  }
  //! Get function return value (const).
  ASMJIT_INLINE const FuncValue& getRet(uint32_t index = 0) const noexcept {
    ASMJIT_ASSERT(index < ASMJIT_ARRAY_SIZE(_rets));
    return _rets[index];
  }

  //! Get function arguments array.
  ASMJIT_INLINE FuncValue* getArgs() noexcept { return _args; }
  //! Get function arguments array (const).
  ASMJIT_INLINE const FuncValue* getArgs() const noexcept { return _args; }

  ASMJIT_INLINE bool hasArg(uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _args[index].isInitialized();
  }

  //! Get function argument at index `index`.
  ASMJIT_INLINE FuncValue& getArg(uint32_t index) noexcept {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _args[index];
  }

  //! Get function argument at index `index`.
  ASMJIT_INLINE const FuncValue& getArg(uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _args[index];
  }

  ASMJIT_INLINE void resetArg(uint32_t index) noexcept {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    _args[index].reset();
  }

  //! Get if the function passes one or more argument by stack.
  ASMJIT_INLINE bool hasStackArgs() const noexcept { return _argStackSize != 0; }
  //! Get stack size needed for function arguments passed on the stack.
  ASMJIT_INLINE uint32_t getArgStackSize() const noexcept { return _argStackSize; }

  ASMJIT_INLINE uint32_t getRedZoneSize() const noexcept { return _callConv.getRedZoneSize(); }
  ASMJIT_INLINE uint32_t getSpillZoneSize() const noexcept { return _callConv.getSpillZoneSize(); }
  ASMJIT_INLINE uint32_t getNaturalStackAlignment() const noexcept { return _callConv.getNaturalStackAlignment(); }

  ASMJIT_INLINE uint32_t getPassedRegs(uint32_t group) const noexcept { return _callConv.getPassedRegs(group); }
  ASMJIT_INLINE uint32_t getPreservedRegs(uint32_t group) const noexcept { return _callConv.getPreservedRegs(group); }

  ASMJIT_INLINE uint32_t getUsedRegs(uint32_t group) const noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    return _usedRegs[group];
  }

  ASMJIT_INLINE void addUsedRegs(uint32_t group, uint32_t regs) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    _usedRegs[group] |= regs;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CallConv _callConv;                    //!< Calling convention.
  uint8_t _argCount;                     //!< Number of function arguments.
  uint8_t _retCount;                     //!< Number of function return values.
  uint32_t _usedRegs[Reg::kGroupVirt];   //!< Registers that contains arguments.
  uint32_t _argStackSize;                //!< Size of arguments passed by stack.
  FuncValue _rets[2];                    //!< Function return values.
  FuncValue _args[kFuncArgCountLoHi];    //!< Function arguments.
};

// ============================================================================
// [asmjit::FuncFrame]
// ============================================================================

//! Function frame.
//!
//! Function frame is used directly by prolog and epilog insertion (PEI) utils.
//! It provides information necessary to insert a proper and ABI comforming
//! prolog and epilog. Function frame calculation is based on \ref CallConv and
//! other function attributes.
//!
//! Function Frame Structure
//! ------------------------
//!
//! Various properties can contribute to the size and structure of the function
//! frame. The function frame in most cases won't use all of the properties
//! illustrated (for example Spill Zone and Red Zone are never used together).
//!
//!   +-----------------------------+
//!   | Arguments Passed by Stack   |
//!   +-----------------------------+
//!   | Spill Zone                  |
//!   +-----------------------------+ <- Stack offset (args) starts from here.
//!   | Return Address if Pushed    |
//!   +-----------------------------+ <- Stack pointer (SP) upon entry.
//!   | Save/Restore Stack.         |
//!   +-----------------------------+-----------------------------+
//!   | Local Stack                 |                             |
//!   +-----------------------------+          Final Stack        |
//!   | Call Stack                  |                             |
//!   +-----------------------------+-----------------------------+
//!   | Red Zone                    |
//!   +-----------------------------+
class FuncFrame {
public:
  ASMJIT_ENUM(Group) {
    kGroupVirt = Reg::kGroupVirt
  };

  ASMJIT_ENUM(Tag) {
    kTagInvalidOffset     = 0xFFFFFFFFU  //!< Tag used to inform that some offset is invalid.
  };

  //! Attributes are designed in a way that all are initially false, and user
  //! or FuncFrame finalizer adds them when necessary.
  ASMJIT_ENUM(Attributes) {
    kAttrHasPreservedFP   = 0x00000001U, //!< Preserve frame pointer (don't omit FP).
    kAttrHasFuncCalls     = 0x00000002U, //!< Function calls other functions (is not leaf).

    kAttrX86AvxEnabled    = 0x00010000U, //!< Use AVX instead of SSE for all operations (X86).
    kAttrX86AvxCleanup    = 0x00020000U, //!< Emit VZEROUPPER instruction in epilog (X86).
    kAttrX86MmxCleanup    = 0x00040000U, //!< Emit EMMS instruction in epilog (X86).

    kAttrAlignedVecSR     = 0x40000000U, //!< Function has aligned save/restore of vector registers.
    kAttrIsFinalized      = 0x80000000U  //!< FuncFrame is finalized and can be used by PEI.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE FuncFrame() noexcept { reset(); }
  ASMJIT_INLINE FuncFrame(const FuncFrame& other) noexcept { ::memcpy(this, &other, sizeof(FuncFrame)); }

  // --------------------------------------------------------------------------
  // [Init / Reset / Finalize]
  // --------------------------------------------------------------------------

  ASMJIT_API Error init(const FuncDetail& func) noexcept;
  ASMJIT_API Error finalize() noexcept;

  ASMJIT_INLINE void reset() noexcept {
    ::memset(this, 0, sizeof(FuncFrame));
    _spRegId = Reg::kIdBad;
    _saRegId = Reg::kIdBad;
    _daOffset = kTagInvalidOffset;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the target architecture of the function frame.
  ASMJIT_INLINE uint32_t getArchType() const noexcept { return _archType; }

  //! Get FuncFrame attributes, see \ref Attributes.
  ASMJIT_INLINE uint32_t getAttributes() const noexcept { return _attributes; }
  //! Check if the FuncFame contains an attribute `attr`.
  ASMJIT_INLINE bool hasAttribute(uint32_t attr) const noexcept { return (_attributes & attr) != 0; }
  //! Add attributes `attrs` to the FuncFrame.
  ASMJIT_INLINE void addAttributes(uint32_t attrs) noexcept { _attributes |= attrs; }
  //! Clear attributes `attrs` from the FrameFrame.
  ASMJIT_INLINE void clearAttributes(uint32_t attrs) noexcept { _attributes &= ~attrs; }

  //! Get if the function preserves frame pointer (EBP|ESP on X86).
  ASMJIT_INLINE bool hasPreservedFP() const noexcept { return hasAttribute(kAttrHasPreservedFP); }
  //! Enable preserved frame pointer.
  ASMJIT_INLINE void setPreservedFP() noexcept { addAttributes(kAttrHasPreservedFP); }
  //! Disable preserved frame pointer.
  ASMJIT_INLINE void resetPreservedFP() noexcept { clearAttributes(kAttrHasPreservedFP); }

  //! Get if the function calls other functions.
  ASMJIT_INLINE bool hasFuncCalls() const noexcept { return hasAttribute(kAttrHasFuncCalls); }
  //! Set `kFlagHasCalls` to true.
  ASMJIT_INLINE void enableFuncCalls() noexcept { addAttributes(kAttrHasFuncCalls); }
  //! Set `kFlagHasCalls` to false.
  ASMJIT_INLINE void disableFuncCalls() noexcept { clearAttributes(kAttrHasFuncCalls); }

  //! Get if the function contains AVX cleanup - 'vzeroupper' instruction in epilog.
  ASMJIT_INLINE bool hasAvxCleanup() const noexcept { return hasAttribute(kAttrX86AvxCleanup); }
  //! Enable AVX cleanup.
  ASMJIT_INLINE void setAvxCleanup() noexcept { addAttributes(kAttrX86AvxCleanup); }
  //! Disable AVX cleanup.
  ASMJIT_INLINE void resetAvxCleanup() noexcept { clearAttributes(kAttrX86AvxCleanup); }

  //! Get if the function contains AVX cleanup - 'vzeroupper' instruction in epilog.
  ASMJIT_INLINE bool isAvxEnabled() const noexcept { return hasAttribute(kAttrX86AvxEnabled); }
  //! Enable AVX cleanup.
  ASMJIT_INLINE void setAvxEnabled() noexcept { addAttributes(kAttrX86AvxEnabled); }
  //! Disable AVX cleanup.
  ASMJIT_INLINE void resetAvxEnabled() noexcept { clearAttributes(kAttrX86AvxEnabled); }

  //! Get if the function contains MMX cleanup - 'emms' instruction in epilog.
  ASMJIT_INLINE bool hasMmxCleanup() const noexcept { return hasAttribute(kAttrX86MmxCleanup); }
  //! Enable MMX cleanup.
  ASMJIT_INLINE void setMmxCleanup() noexcept { addAttributes(kAttrX86MmxCleanup); }
  //! Disable MMX cleanup.
  ASMJIT_INLINE void resetMmxCleanup() noexcept { clearAttributes(kAttrX86MmxCleanup); }

  //! Get if the function uses call stack.
  ASMJIT_INLINE bool hasCallStack() const noexcept { return _callStackSize != 0; }
  //! Get if the function uses local stack.
  ASMJIT_INLINE bool hasLocalStack() const noexcept { return _localStackSize != 0; }
  //! Get if vector registers can be saved and restored by using aligned writes and reads.
  ASMJIT_INLINE bool hasAlignedVecSR() const noexcept { return hasAttribute(kAttrAlignedVecSR); }
  //! Get if the function has to align stack dynamically.
  ASMJIT_INLINE bool hasDynamicAlignment() const noexcept { return _finalStackAlignment >= _minimumDynamicAlignment; }

  //! Get if this calling convention specifies 'RedZone'.
  ASMJIT_INLINE bool hasRedZone() const noexcept { return _redZoneSize != 0; }
  //! Get if this calling convention specifies 'SpillZone'.
  ASMJIT_INLINE bool hasSpillZone() const noexcept { return _spillZoneSize != 0; }

  //! Get size of 'RedZone'.
  ASMJIT_INLINE uint32_t getRedZoneSize() const noexcept { return _redZoneSize; }
  //! Get size of 'SpillZone'.
  ASMJIT_INLINE uint32_t getSpillZoneSize() const noexcept { return _spillZoneSize; }
  //! Get natural stack alignment (guaranteed stack alignment upon entry).
  ASMJIT_INLINE uint32_t getNaturalStackAlignment() const noexcept { return _naturalStackAlignment; }
  //! Get natural stack alignment (guaranteed stack alignment upon entry).
  ASMJIT_INLINE uint32_t getMinimumDynamicAlignment() const noexcept { return _minimumDynamicAlignment; }

  //! Get if the callee must adjust SP before returning (X86-STDCALL only)
  ASMJIT_INLINE bool hasCalleeStackCleanup() const noexcept { return _calleeStackCleanup != 0; }
  //! Get home many bytes of the stack the the callee must adjust before returning (X86-STDCALL only)
  ASMJIT_INLINE uint32_t getCalleeStackCleanup() const noexcept { return _calleeStackCleanup; }

  //! Get call stack alignment.
  ASMJIT_INLINE uint32_t getCallStackAlignment() const noexcept { return _callStackAlignment; }
  //! Get local stack alignment.
  ASMJIT_INLINE uint32_t getLocalStackAlignment() const noexcept { return _localStackAlignment; }
  //! Get final stack alignment (the maximum value of call, local, and natural stack alignments).
  ASMJIT_INLINE uint32_t getFinalStackAlignment() const noexcept { return _finalStackAlignment; }

  //! Set call stack alignment.
  //!
  //! NOTE: This also updates the final stack alignment.
  ASMJIT_INLINE void setCallStackAlignment(uint32_t alignment) noexcept {
    _callStackAlignment = IntUtils::toUInt8(alignment);
    _finalStackAlignment = std::max(_naturalStackAlignment, std::max(_callStackAlignment, _localStackAlignment));
  }

  //! Set local stack alignment.
  //!
  //! NOTE: This also updates the final stack alignment.
  ASMJIT_INLINE void setLocalStackAlignment(uint32_t value) noexcept {
    _localStackAlignment = IntUtils::toUInt8(value);
    _finalStackAlignment = std::max(_naturalStackAlignment, std::max(_callStackAlignment, _localStackAlignment));
  }

  //! Combine call stack alignment with `alignment`, updating it to the greater value.
  //!
  //! NOTE: This also updates the final stack alignment.
  ASMJIT_INLINE void updateCallStackAlignment(uint32_t alignment) noexcept {
    _callStackAlignment = IntUtils::toUInt8(std::max<uint32_t>(_callStackAlignment, alignment));
    _finalStackAlignment = std::max(_finalStackAlignment, _callStackAlignment);
  }

  //! Combine local stack alignment with `alignment`, updating it to the greater value.
  //!
  //! NOTE: This also updates the final stack alignment.
  ASMJIT_INLINE void updateLocalStackAlignment(uint32_t alignment) noexcept {
    _localStackAlignment = IntUtils::toUInt8(std::max<uint32_t>(_localStackAlignment, alignment));
    _finalStackAlignment = std::max(_finalStackAlignment, _localStackAlignment);
  }

  //! Get call stack size.
  ASMJIT_INLINE uint32_t getCallStackSize() const noexcept { return _callStackSize; }
  //! Get local stack size.
  ASMJIT_INLINE uint32_t getLocalStackSize() const noexcept { return _localStackSize; }

  //! Set call stack size.
  ASMJIT_INLINE void setCallStackSize(uint32_t size) noexcept { _callStackSize = size; }
  //! Set local stack size.
  ASMJIT_INLINE void setLocalStackSize(uint32_t size) noexcept { _localStackSize = size; }

  //! Combine call stack size with `size`, updating it to the greater value.
  ASMJIT_INLINE void updateCallStackSize(uint32_t size) noexcept { _callStackSize = std::max(_callStackSize, size); }
  //! Combine local stack size with `size`, updating it to the greater value.
  ASMJIT_INLINE void updateLocalStackSize(uint32_t size) noexcept { _localStackSize = std::max(_localStackSize, size); }

  //! Get final stack size (only valid after the FuncFrame is finalized).
  ASMJIT_INLINE uint32_t getFinalStackSize() const noexcept { return _finalStackSize; }

  //! Get an offset to access the local stack (non-zero only if call stack is used).
  ASMJIT_INLINE uint32_t getLocalStackOffset() const noexcept { return _localStackOffset; }

  //! Get if the function prolog/epilog requires a memory slot for storing unaligned SP.
  ASMJIT_INLINE bool hasDAOffset() const noexcept { return _daOffset != kTagInvalidOffset; }
  //! Get a memory offset used to store DA (dynamic alignment) slot (relative to SP).
  ASMJIT_INLINE uint32_t getDAOffset() const noexcept { return _daOffset; }

  ASMJIT_INLINE uint32_t getSAOffset(uint32_t regId) const noexcept {
    return regId == _spRegId ? getSAOffsetFromSP() : getSAOffsetFromSA();
  }

  ASMJIT_INLINE uint32_t getSAOffsetFromSP() const noexcept { return _saOffsetFromSP; }
  ASMJIT_INLINE uint32_t getSAOffsetFromSA() const noexcept { return _saOffsetFromSA; }

  //! Get which registers (by `group`) are saved/restored in prolog/epilog, respectively.
  ASMJIT_INLINE uint32_t getDirtyRegs(uint32_t group) const noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    return _dirtyRegs[group];
  }

  //! Set which registers (by `group`) are saved/restored in prolog/epilog, respectively.
  ASMJIT_INLINE void setDirtyRegs(uint32_t group, uint32_t regs) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    _dirtyRegs[group] = regs;
  }

  //! Add registers (by `group`) to saved/restored registers.
  ASMJIT_INLINE void addDirtyRegs(uint32_t group, uint32_t regs) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    _dirtyRegs[group] |= regs;
  }

  ASMJIT_INLINE void setAllDirty() noexcept {
    _dirtyRegs[0] = 0xFFFFFFFFU;
    _dirtyRegs[1] = 0xFFFFFFFFU;
    _dirtyRegs[2] = 0xFFFFFFFFU;
    _dirtyRegs[3] = 0xFFFFFFFFU;
  }

  ASMJIT_INLINE void setAllDirty(uint32_t group) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    _dirtyRegs[group] = 0xFFFFFFFFU;
  }

  ASMJIT_INLINE uint32_t getSavedRegs(uint32_t group) const noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    return _dirtyRegs[group] & _preservedRegs[group];
  }

  ASMJIT_INLINE uint32_t getPreservedRegs(uint32_t group) const noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    return _preservedRegs[group];
  }

  ASMJIT_INLINE bool hasSARegId() const noexcept { return _saRegId != Reg::kIdBad; }
  ASMJIT_INLINE uint32_t getSARegId() const noexcept { return _saRegId; }
  ASMJIT_INLINE void setSARegId(uint32_t regId) { _saRegId = IntUtils::toUInt8(regId); }
  ASMJIT_INLINE void resetSARegId() { setSARegId(Reg::kIdBad); }

  //! Get stack size required to save GP registers.
  ASMJIT_INLINE uint32_t getGpSaveSize() const noexcept { return _gpSaveSize; }
  //! Get stack size required to save other than GP registers (MM, XMM|YMM|ZMM, K, VFP, etc...).
  ASMJIT_INLINE uint32_t getNonGpSaveSize() const noexcept { return _nonGpSaveSize; }

  ASMJIT_INLINE uint32_t getGpSaveOffset() const noexcept { return _gpSaveOffset; }
  ASMJIT_INLINE uint32_t getNonGpSaveOffset() const noexcept { return _nonGpSaveOffset; }

  ASMJIT_INLINE bool hasStackAdjustment() const noexcept { return _stackAdjustment != 0; }
  ASMJIT_INLINE uint32_t getStackAdjustment() const noexcept { return _stackAdjustment; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _attributes;                  //!< Function attributes.

  uint8_t _archType;                     //!< Architecture.
  uint8_t _spRegId;                      //!< SP register ID (to access call stack and local stack).
  uint8_t _saRegId;                      //!< SA register ID (to access stack arguments).

  uint8_t _redZoneSize;                  //!< Red zone size (copied from CallConv).
  uint8_t _spillZoneSize;                //!< Spill zone size (copied from CallConv).
  uint8_t _naturalStackAlignment;        //!< Natural stack alignment (copied from CallConv).
  uint8_t _minimumDynamicAlignment;      //!< Minimum stack alignment to turn on dynamic alignment.

  uint8_t _callStackAlignment;           //!< Call stack alignment.
  uint8_t _localStackAlignment;          //!< Local stack alignment.
  uint8_t _finalStackAlignment;          //!< Final stack alignment.

  uint16_t _calleeStackCleanup;          //!< Adjustment of the stack before returning (X86-STDCALL).

  uint32_t _callStackSize;               //!< Call stack size.
  uint32_t _localStackSize;              //!< Local stack size.
  uint32_t _finalStackSize;              //!< Final stack size (sum of call stack and local stack).

  uint32_t _localStackOffset;            //!< Local stack offset (non-zero only call stack is used).
  uint32_t _daOffset;                    //!< Offset relative to SP that contains previous SP (before alignment).
  uint32_t _saOffsetFromSP;              //!< Offset of the first stack argument relative to SP.
  uint32_t _saOffsetFromSA;              //!< Offset of the first stack argument relative to SA (_saRegId or FP).

  uint32_t _stackAdjustment;             //!< Local stack adjustment in prolog/epilog.

  uint32_t _dirtyRegs[Reg::kGroupVirt];     //!< Registers that are dirty.
  uint32_t _preservedRegs[Reg::kGroupVirt]; //!< Registers that must be preserved (copied from CallConv).

  uint16_t _gpSaveSize;                  //!< Final stack size required to save GP regs.
  uint16_t _nonGpSaveSize;               //!< Final Stack size required to save other than GP regs.
  uint32_t _gpSaveOffset;                //!< Final offset where saved GP regs are stored.
  uint32_t _nonGpSaveOffset;             //!< Final offset where saved other than GP regs are stored.
};

// ============================================================================
// [asmjit::FuncArgsAssignment]
// ============================================================================

//! A helper class that can be used to assign a physical register for each
//! function argument. Use with `CodeEmitter::emitArgsAssignment()`.
class FuncArgsAssignment {
public:
  enum {
    kArgCount = kFuncArgCountLoHi
  };

  explicit ASMJIT_INLINE FuncArgsAssignment(const FuncDetail* fd = nullptr) noexcept { reset(fd); }

  ASMJIT_INLINE FuncArgsAssignment(const FuncArgsAssignment& other) noexcept {
    ::memcpy(this, &other, sizeof(*this));
  }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset(const FuncDetail* fd = nullptr) noexcept {
    _funcDetail = fd;
    _saRegId = static_cast<uint8_t>(Reg::kIdBad);
    ::memset(_reserved, 0, sizeof(_reserved));
    ::memset(_args, 0, sizeof(_args));
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE const FuncDetail* getFuncDetail() const noexcept { return _funcDetail; }
  ASMJIT_INLINE void setFuncDetail(const FuncDetail* fd) noexcept { _funcDetail = fd; }

  ASMJIT_INLINE bool hasSARegId() const noexcept { return _saRegId != Reg::kIdBad; }
  ASMJIT_INLINE uint32_t getSARegId() const noexcept { return _saRegId; }
  ASMJIT_INLINE void setSARegId(uint32_t regId) { _saRegId = IntUtils::toUInt8(regId); }
  ASMJIT_INLINE void resetSARegId() { _saRegId = static_cast<uint8_t>(Reg::kIdBad); }

  ASMJIT_INLINE FuncValue& getArg(uint32_t index) noexcept {
    ASMJIT_ASSERT(index < ASMJIT_ARRAY_SIZE(_args));
    return _args[index];
  }
  ASMJIT_INLINE const FuncValue& getArg(uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < ASMJIT_ARRAY_SIZE(_args));
    return _args[index];
  }

  ASMJIT_INLINE bool isAssigned(uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < ASMJIT_ARRAY_SIZE(_args));
    return _args[index].isAssigned();
  }

  ASMJIT_INLINE void assignReg(uint32_t index, const Reg& reg, uint32_t typeId = TypeId::kVoid) noexcept {
    ASMJIT_ASSERT(index < ASMJIT_ARRAY_SIZE(_args));
    ASMJIT_ASSERT(reg.isPhysReg());
    _args[index].initReg(reg.getType(), reg.getId(), typeId);
  }

  ASMJIT_INLINE void assignReg(uint32_t index, uint32_t regType, uint32_t regId, uint32_t typeId = TypeId::kVoid) noexcept {
    ASMJIT_ASSERT(index < ASMJIT_ARRAY_SIZE(_args));
    _args[index].initReg(regType, regId, typeId);
  }

  // NOTE: All `assignAll()` methods are shortcuts to assign all arguments at
  // once, however, since registers are passed all at once these initializers
  // don't provide any way to pass TypeId and/or to keep any argument between
  // the arguments passed uninitialized.
  ASMJIT_INLINE void assignAll(const Reg& a0) noexcept {
    assignReg(0, a0);
  }

  ASMJIT_INLINE void assignAll(const Reg& a0, const Reg& a1) noexcept {
    assignReg(0, a0);
    assignReg(1, a1);
  }

  ASMJIT_INLINE void assignAll(const Reg& a0, const Reg& a1, const Reg& a2) noexcept {
    assignReg(0, a0);
    assignReg(1, a1);
    assignReg(2, a2);
  }

  ASMJIT_INLINE void assignAll(const Reg& a0, const Reg& a1, const Reg& a2, const Reg& a3) noexcept {
    assignReg(0, a0);
    assignReg(1, a1);
    assignReg(2, a2);
    assignReg(3, a3);
  }

  ASMJIT_INLINE void assignAll(const Reg& a0, const Reg& a1, const Reg& a2, const Reg& a3, const Reg& a4) noexcept {
    assignReg(0, a0);
    assignReg(1, a1);
    assignReg(2, a2);
    assignReg(3, a3);
    assignReg(4, a4);
  }

  ASMJIT_INLINE void assignAll(const Reg& a0, const Reg& a1, const Reg& a2, const Reg& a3, const Reg& a4, const Reg& a5) noexcept {
    assignReg(0, a0);
    assignReg(1, a1);
    assignReg(2, a2);
    assignReg(3, a3);
    assignReg(4, a4);
    assignReg(5, a5);
  }

  ASMJIT_INLINE void assignAll(const Reg& a0, const Reg& a1, const Reg& a2, const Reg& a3, const Reg& a4, const Reg& a5, const Reg& a6) noexcept {
    assignReg(0, a0);
    assignReg(1, a1);
    assignReg(2, a2);
    assignReg(3, a3);
    assignReg(4, a4);
    assignReg(5, a5);
    assignReg(6, a6);
  }

  ASMJIT_INLINE void assignAll(const Reg& a0, const Reg& a1, const Reg& a2, const Reg& a3, const Reg& a4, const Reg& a5, const Reg& a6, const Reg& a7) noexcept {
    assignReg(0, a0);
    assignReg(1, a1);
    assignReg(2, a2);
    assignReg(3, a3);
    assignReg(4, a4);
    assignReg(5, a5);
    assignReg(6, a6);
    assignReg(7, a7);
  }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  //! Update `FuncFrame` based on function's arguments assignment.
  //!
  //! NOTE: You MUST call this in orher to use `CodeEmitter::emitArgsAssignment()`,
  //! otherwise the FuncFrame would not contain the information necessary to
  //! assign all arguments into the registers and/or stack specified.
  ASMJIT_API Error updateFuncFrame(FuncFrame& frame) const noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const FuncDetail* _funcDetail;         //!< Function detail.
  uint8_t _saRegId;                      //!< Register that can be used to access arguments passed by stack.
  uint8_t _reserved[3];                  //!< \internal
  FuncValue _args[kArgCount];            //!< Mapping of each function argument.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_FUNC_H
