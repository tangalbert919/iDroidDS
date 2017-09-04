// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if defined(ASMJIT_BUILD_X86)

// [Dependencies]
#include "../base/intutils.h"
#include "../x86/x86internal_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86Internal - CallConv]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::initCallConv(CallConv& cc, uint32_t ccId) noexcept {
  const uint32_t kGroupGp  = X86Reg::kGroupGp;
  const uint32_t kGroupVec = X86Reg::kGroupVec;
  const uint32_t kGroupMm  = X86Reg::kGroupMm;
  const uint32_t kGroupK   = X86Reg::kGroupK;

  const uint32_t kZax = X86Gp::kIdAx;
  const uint32_t kZbx = X86Gp::kIdBx;
  const uint32_t kZcx = X86Gp::kIdCx;
  const uint32_t kZdx = X86Gp::kIdDx;
  const uint32_t kZsp = X86Gp::kIdSp;
  const uint32_t kZbp = X86Gp::kIdBp;
  const uint32_t kZsi = X86Gp::kIdSi;
  const uint32_t kZdi = X86Gp::kIdDi;

  switch (ccId) {
    case CallConv::kIdX86StdCall:
      cc.setFlags(CallConv::kFlagCalleePopsStack);
      goto X86CallConv;

    case CallConv::kIdX86MsThisCall:
      cc.setFlags(CallConv::kFlagCalleePopsStack);
      cc.setPassedOrder(kGroupGp, kZcx);
      goto X86CallConv;

    case CallConv::kIdX86MsFastCall:
    case CallConv::kIdX86GccFastCall:
      cc.setFlags(CallConv::kFlagCalleePopsStack);
      cc.setPassedOrder(kGroupGp, kZcx, kZdx);
      goto X86CallConv;

    case CallConv::kIdX86GccRegParm1:
      cc.setPassedOrder(kGroupGp, kZax);
      goto X86CallConv;

    case CallConv::kIdX86GccRegParm2:
      cc.setPassedOrder(kGroupGp, kZax, kZdx);
      goto X86CallConv;

    case CallConv::kIdX86GccRegParm3:
      cc.setPassedOrder(kGroupGp, kZax, kZdx, kZcx);
      goto X86CallConv;

    case CallConv::kIdX86CDecl:
X86CallConv:
      cc.setNaturalStackAlignment(4);
      cc.setArchType(ArchInfo::kTypeX86);
      cc.setPreservedRegs(kGroupGp, IntUtils::mask(kZbx, kZsp, kZbp, kZsi, kZdi));
      break;

    case CallConv::kIdX86Win64:
      cc.setArchType(ArchInfo::kTypeX64);
      cc.setStrategy(CallConv::kStrategyWin64);
      cc.setFlags(CallConv::kFlagPassFloatsByVec | CallConv::kFlagIndirectVecArgs);
      cc.setNaturalStackAlignment(16);
      cc.setSpillZoneSize(32);
      cc.setPassedOrder(kGroupGp, kZcx, kZdx, 8, 9);
      cc.setPassedOrder(kGroupVec, 0, 1, 2, 3);
      cc.setPreservedRegs(kGroupGp, IntUtils::mask(kZbx, kZsp, kZbp, kZsi, kZdi, 12, 13, 14, 15));
      cc.setPreservedRegs(kGroupVec, IntUtils::mask(6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
      break;

    case CallConv::kIdX86SysV64:
      cc.setArchType(ArchInfo::kTypeX64);
      cc.setFlags(CallConv::kFlagPassFloatsByVec);
      cc.setNaturalStackAlignment(16);
      cc.setRedZoneSize(128);
      cc.setPassedOrder(kGroupGp, kZdi, kZsi, kZdx, kZcx, 8, 9);
      cc.setPassedOrder(kGroupVec, 0, 1, 2, 3, 4, 5, 6, 7);
      cc.setPreservedRegs(kGroupGp, IntUtils::mask(kZbx, kZsp, kZbp, 12, 13, 14, 15));
      break;

    case CallConv::kIdX86FastEval2:
    case CallConv::kIdX86FastEval3:
    case CallConv::kIdX86FastEval4: {
      uint32_t n = ccId - CallConv::kIdX86FastEval2;

      cc.setArchType(ArchInfo::kTypeX86);
      cc.setFlags(CallConv::kFlagPassFloatsByVec);
      cc.setNaturalStackAlignment(16);
      cc.setPassedOrder(kGroupGp, kZax, kZdx, kZcx, kZsi, kZdi);
      cc.setPassedOrder(kGroupMm, 0, 1, 2, 3, 4, 5, 6, 7);
      cc.setPassedOrder(kGroupVec, 0, 1, 2, 3, 4, 5, 6, 7);

      cc.setPreservedRegs(kGroupGp , IntUtils::bits(8));
      cc.setPreservedRegs(kGroupVec, IntUtils::bits(8) & ~IntUtils::bits(n));
      cc.setPreservedRegs(kGroupMm , IntUtils::bits(8));
      cc.setPreservedRegs(kGroupK  , IntUtils::bits(8));
      break;
    }

    case CallConv::kIdX64FastEval2:
    case CallConv::kIdX64FastEval3:
    case CallConv::kIdX64FastEval4: {
      uint32_t n = ccId - CallConv::kIdX64FastEval2;

      cc.setArchType(ArchInfo::kTypeX64);
      cc.setFlags(CallConv::kFlagPassFloatsByVec);
      cc.setNaturalStackAlignment(16);
      cc.setPassedOrder(kGroupGp, kZax, kZdx, kZcx, kZsi, kZdi);
      cc.setPassedOrder(kGroupMm, 0, 1, 2, 3, 4, 5, 6, 7);
      cc.setPassedOrder(kGroupVec, 0, 1, 2, 3, 4, 5, 6, 7);

      cc.setPreservedRegs(kGroupGp , IntUtils::bits(16));
      cc.setPreservedRegs(kGroupVec,~IntUtils::bits(n));
      cc.setPreservedRegs(kGroupMm , IntUtils::bits(8));
      cc.setPreservedRegs(kGroupK  , IntUtils::bits(8));
      break;
    }

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  cc.setId(ccId);
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - Helpers]
// ============================================================================

static ASMJIT_INLINE uint32_t x86GetXmmMovInst(const FuncFrame& frame) {
  bool avx = frame.isAvxEnabled();
  bool aligned = frame.hasAlignedVecSR();

  return aligned ? (avx ? X86Inst::kIdVmovaps : X86Inst::kIdMovaps)
                 : (avx ? X86Inst::kIdVmovups : X86Inst::kIdMovups);
}

static ASMJIT_INLINE uint32_t x86VecTypeIdToRegType(uint32_t typeId) noexcept {
  return typeId <= TypeId::_kVec128End ? X86Reg::kRegXmm :
         typeId <= TypeId::_kVec256End ? X86Reg::kRegYmm : X86Reg::kRegZmm;
}

// ============================================================================
// [asmjit::X86Internal - FuncDetail]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::initFuncDetail(FuncDetail& func, const FuncSignature& sign, uint32_t gpSize) noexcept {
  ASMJIT_UNUSED(sign);

  const CallConv& cc = func.getCallConv();
  uint32_t archType = cc.getArchType();
  uint32_t stackOffset = cc._spillZoneSize;

  uint32_t i;
  uint32_t argCount = func.getArgCount();

  if (func.getRetCount() != 0) {
    uint32_t typeId = func._rets[0].getTypeId();
    switch (typeId) {
      case TypeId::kI64:
      case TypeId::kU64: {
        if (archType == ArchInfo::kTypeX86) {
          // Convert a 64-bit return value to two 32-bit return values.
          func._retCount = 2;
          typeId -= 2;

          // 64-bit value is returned in EDX:EAX on X86.
          func._rets[0].initReg(X86Gp::kRegGpd, X86Gp::kIdAx, typeId);
          func._rets[1].initReg(X86Gp::kRegGpd, X86Gp::kIdDx, typeId);
          break;
        }
        else {
          func._rets[0].initReg(X86Gp::kRegGpq, X86Gp::kIdAx, typeId);
        }
        break;
      }

      case TypeId::kI8:
      case TypeId::kI16:
      case TypeId::kI32: {
        func._rets[0].initReg(X86Gp::kRegGpd, X86Gp::kIdAx, TypeId::kI32);
        break;
      }

      case TypeId::kU8:
      case TypeId::kU16:
      case TypeId::kU32: {
        func._rets[0].initReg(X86Gp::kRegGpd, X86Gp::kIdAx, TypeId::kU32);
        break;
      }

      case TypeId::kF32:
      case TypeId::kF64: {
        uint32_t regType = (archType == ArchInfo::kTypeX86) ? X86Reg::kRegFp : X86Reg::kRegXmm;
        func._rets[0].initReg(regType, 0, typeId);
        break;
      }

      case TypeId::kF80: {
        // 80-bit floats are always returned by FP0.
        func._rets[0].initReg(X86Reg::kRegFp, 0, typeId);
        break;
      }

      case TypeId::kMmx32:
      case TypeId::kMmx64: {
        // MM registers are returned through XMM or GPQ (Win64).
        uint32_t regType = X86Reg::kRegMm;
        if (archType != ArchInfo::kTypeX86)
          regType = cc.getStrategy() == CallConv::kStrategyDefault ? X86Reg::kRegXmm : X86Reg::kRegGpq;

        func._rets[0].initReg(regType, 0, typeId);
        break;
      }

      default: {
        func._rets[0].initReg(x86VecTypeIdToRegType(typeId), 0, typeId);
        break;
      }
    }
  }

  if (cc.getStrategy() == CallConv::kStrategyDefault) {
    uint32_t gpzPos = 0;
    uint32_t vecPos = 0;

    for (i = 0; i < argCount; i++) {
      FuncValue& arg = func._args[i];
      uint32_t typeId = arg.getTypeId();

      if (TypeId::isInt(typeId)) {
        uint32_t regId = gpzPos < CallConv::kMaxRegArgsPerGroup ? cc._passedOrder[X86Reg::kGroupGp].id[gpzPos] : uint8_t(Reg::kIdBad);
        if (regId != Reg::kIdBad) {
          uint32_t regType = (typeId <= TypeId::kU32) ? X86Reg::kRegGpd : X86Reg::kRegGpq;
          arg.addRegData(regType, regId);
          func.addUsedRegs(X86Reg::kGroupGp, IntUtils::mask(regId));
          gpzPos++;
        }
        else {
          uint32_t size = std::max<uint32_t>(TypeId::sizeOf(typeId), gpSize);
          arg.addStackOffset(stackOffset);
          stackOffset += size;
        }
        continue;
      }

      if (TypeId::isFloat(typeId) || TypeId::isVec(typeId)) {
        uint32_t regId = vecPos < CallConv::kMaxRegArgsPerGroup ? cc._passedOrder[X86Reg::kGroupVec].id[vecPos] : uint8_t(Reg::kIdBad);

        // If this is a float, but `floatByVec` is false, we have to pass by stack.
        if (TypeId::isFloat(typeId) && !cc.hasFlag(CallConv::kFlagPassFloatsByVec))
          regId = Reg::kIdBad;

        if (regId != Reg::kIdBad) {
          arg.init(typeId);
          arg.addRegData(x86VecTypeIdToRegType(typeId), regId);
          func.addUsedRegs(X86Reg::kGroupVec, IntUtils::mask(regId));
          vecPos++;
        }
        else {
          uint32_t size = TypeId::sizeOf(typeId);
          arg.addStackOffset(stackOffset);
          stackOffset += size;
        }
        continue;
      }
    }
  }

  if (cc.getStrategy() == CallConv::kStrategyWin64) {
    for (i = 0; i < argCount; i++) {
      FuncValue& arg = func._args[i];

      uint32_t typeId = arg.getTypeId();
      uint32_t size = TypeId::sizeOf(typeId);

      if (TypeId::isInt(typeId) || TypeId::isMmx(typeId)) {
        uint32_t regId = i < CallConv::kMaxRegArgsPerGroup ? cc._passedOrder[X86Reg::kGroupGp].id[i] : uint8_t(Reg::kIdBad);
        if (regId != Reg::kIdBad) {
          uint32_t regType = (size <= 4 && !TypeId::isMmx(typeId)) ? X86Reg::kRegGpd : X86Reg::kRegGpq;
          arg.addRegData(regType, regId);
          func.addUsedRegs(X86Reg::kGroupGp, IntUtils::mask(regId));
        }
        else {
          arg.addStackOffset(stackOffset);
          stackOffset += gpSize;
        }
        continue;
      }

      if (TypeId::isFloat(typeId) || TypeId::isVec(typeId)) {
        uint32_t regId = i < CallConv::kMaxRegArgsPerGroup ? cc._passedOrder[X86Reg::kGroupVec].id[i] : uint8_t(Reg::kIdBad);
        if (regId != Reg::kIdBad && (TypeId::isFloat(typeId) || cc.hasFlag(CallConv::kFlagVectorCall))) {
          uint32_t regType = x86VecTypeIdToRegType(typeId);
          uint32_t regId = cc._passedOrder[X86Reg::kGroupVec].id[i];

          arg.addRegData(regType, regId);
          func.addUsedRegs(X86Reg::kGroupVec, IntUtils::mask(regId));
        }
        else {
          arg.addStackOffset(stackOffset);
          stackOffset += 8; // Always 8 bytes (float/double).
        }
        continue;
      }
    }
  }

  func._argStackSize = stackOffset;
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86FuncArgsContext]
// ============================================================================

// Used by both `argsToFuncFrame()` and `emitArgsAssignment()`.
class X86FuncArgsContext {
public:
  ASMJIT_ENUM(VarId) {
    kVarIdNone = 0xFF
  };

  //! Contains information about a single argument or SA register that may need shuffling.
  struct Var {
    ASMJIT_INLINE void init(const FuncValue& cur_, const FuncValue& out_) noexcept {
      cur = cur_;
      out = out_;
    }

    //! Reset the value to its unassigned state.
    ASMJIT_INLINE void reset() noexcept {
      cur.reset();
      out.reset();
    }

    FuncValue cur;
    FuncValue out;
  };

  struct WorkData {
    ASMJIT_INLINE void reset() noexcept {
      archRegs = 0;
      workRegs = 0;
      usedRegs = 0;
      liveRegs = 0;
      dstRegs = 0;
      dstShuf = 0;
      numSwaps = 0;
      numStackArgs = 0;
      ::memset(reserved, 0, sizeof(reserved));
      ::memset(physToVarId, kVarIdNone, 32);
    }

    ASMJIT_INLINE bool isAssigned(uint32_t regId) const noexcept {
      ASMJIT_ASSERT(regId < 32);
      return physToVarId[regId] != kVarIdNone;
    }

    ASMJIT_INLINE void assign(uint32_t regId, uint32_t varId) noexcept {
      ASMJIT_ASSERT((liveRegs & IntUtils::mask(regId)) == 0);

      physToVarId[regId] = IntUtils::toUInt8(varId);
      liveRegs ^= IntUtils::mask(regId);
    }

    ASMJIT_INLINE void reassign(uint32_t newId, uint32_t oldId, uint32_t varId) noexcept {
      ASMJIT_ASSERT((liveRegs & IntUtils::mask(newId)) == 0);
      ASMJIT_ASSERT((liveRegs & IntUtils::mask(oldId)) != 0);

      physToVarId[oldId] = static_cast<uint8_t>(kVarIdNone);
      physToVarId[newId] = IntUtils::toUInt8(varId);
      liveRegs ^= IntUtils::mask(newId) ^ IntUtils::mask(oldId);
    }

    ASMJIT_INLINE void swap(uint32_t aRegId, uint32_t aVarId, uint32_t bRegId, uint32_t bVarId) noexcept {
      physToVarId[aRegId] = IntUtils::toUInt8(bVarId);
      physToVarId[bRegId] = IntUtils::toUInt8(aVarId);
    }

    ASMJIT_INLINE void unassign(uint32_t regId) noexcept {
      ASMJIT_ASSERT((liveRegs & IntUtils::mask(regId)) != 0);

      physToVarId[regId] = static_cast<uint8_t>(kVarIdNone);
      liveRegs ^= IntUtils::mask(regId);
    }

    uint32_t archRegs;                   //!< All allocable registers provided by the architecture.
    uint32_t workRegs;                   //!< All registers that can be used by the shuffler.
    uint32_t usedRegs;                   //!< Registers actually used by the shuffler.
    uint32_t liveRegs;                   //!< Registers currently alive.
    uint32_t dstRegs;                    //!< Destination registers assigned to arguments or SA.
    uint32_t dstShuf;                    //!< Destination registers that require shuffling.
    uint8_t numSwaps;                    //!< Number of register swaps.
    uint8_t numStackArgs;                //!< Number of stack loads.
    uint8_t reserved[6];                 //!< Reserved (only used as padding).
    uint8_t physToVarId[32];             //!< Physical ID to variable ID mapping.
  };

  X86FuncArgsContext() noexcept;

  Error initWorkData(const FuncFrame& frame, const FuncArgsAssignment& args) noexcept;
  Error markRegsForSwaps(FuncFrame& frame) noexcept;
  Error markDstRegsDirty(FuncFrame& frame) noexcept;
  Error markStackArgsReg(FuncFrame& frame) noexcept;

  ASMJIT_INLINE uint32_t indexOf(const Var* var) const noexcept { return static_cast<uint32_t>((size_t)(var - _vars)); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _varCount;
  bool _hasRegSwaps;
  bool _hasStackArgs;
  bool _hasPreservedFP;
  uint8_t _saVarId;
  WorkData _workData[Reg::kGroupVirt];
  Var _vars[kFuncArgCountLoHi + 1];
};

X86FuncArgsContext::X86FuncArgsContext() noexcept {
  _varCount = 0;
  _hasRegSwaps = false;
  _hasStackArgs = false;
  _hasPreservedFP = false;
  _saVarId = kVarIdNone;

  for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
    _workData[group].reset();
}

ASMJIT_FAVOR_SIZE Error X86FuncArgsContext::initWorkData(const FuncFrame& frame, const FuncArgsAssignment& args) noexcept {
  // The code has to be updated if this changes.
  ASMJIT_ASSERT(Reg::kGroupVirt == 4);

  uint32_t i;
  const FuncDetail& func = *args.getFuncDetail();

  uint32_t archType = func.getCallConv().getArchType();
  uint32_t count = (archType == ArchInfo::kTypeX86) ? 8 : 16;
  uint32_t varId = 0;

  // Initialize WorkData::archRegs.
  _workData[X86Reg::kGroupGp ].archRegs = IntUtils::bits(count) & ~IntUtils::mask(X86Gp::kIdSp);
  _workData[X86Reg::kGroupVec].archRegs = IntUtils::bits(count);
  _workData[X86Reg::kGroupMm ].archRegs = IntUtils::bits(8);
  _workData[X86Reg::kGroupK  ].archRegs = IntUtils::bits(8);

  if (frame.hasPreservedFP())
    _workData[X86Reg::kGroupGp].archRegs &= ~IntUtils::mask(X86Gp::kIdBp);

  // Extract information from all function arguments/assignments and build Var[] array.
  for (i = 0; i < kFuncArgCountLoHi; i++) {
    const FuncValue& dstArg = args.getArg(i);
    if (!dstArg.isAssigned()) continue;

    const FuncValue& srcArg = func.getArg(i);
    if (ASMJIT_UNLIKELY(!srcArg.isAssigned()))
      return DebugUtils::errored(kErrorInvalidState);

    uint32_t dstType = dstArg.getRegType();
    if (ASMJIT_UNLIKELY(dstType >= X86Reg::kRegCount))
      return DebugUtils::errored(kErrorInvalidRegType);

    uint32_t dstGroup = X86Reg::groupOf(dstType);
    if (ASMJIT_UNLIKELY(dstGroup >= Reg::kGroupVirt))
      return DebugUtils::errored(kErrorInvalidRegGroup);

    WorkData& wd = _workData[dstGroup];
    uint32_t dstId = dstArg.getRegId();
    if (ASMJIT_UNLIKELY(dstId >= 32 || !(wd.archRegs & IntUtils::mask(dstId))))
      return DebugUtils::errored(kErrorInvalidPhysId);

    uint32_t dstMask = IntUtils::mask(dstId);
    if (ASMJIT_UNLIKELY(wd.dstRegs & dstMask))
      return DebugUtils::errored(kErrorOverlappedRegs);

    Var& var = _vars[varId];
    var.init(srcArg, dstArg);

    wd.dstRegs |= dstMask;
    wd.usedRegs |= dstMask;

    if (srcArg.isReg()) {
      uint32_t srcId = srcArg.getRegId();
      uint32_t srcGroup = X86Reg::groupOf(srcArg.getRegType());

      if (dstGroup == srcGroup) {
        wd.assign(srcId, varId);
        // The best case, register is allocated where it is expected to be.
        if (dstId == srcId) continue;
      }
      else {
        if (ASMJIT_UNLIKELY(srcGroup >= Reg::kGroupVirt))
          return DebugUtils::errored(kErrorInvalidState);

        WorkData& srcData = _workData[srcGroup];
        srcData.assign(srcId, varId);
      }
    }
    else {
      wd.numStackArgs++;
      _hasStackArgs = true;
    }

    wd.dstShuf |= dstMask;
    varId++;
  }

  // Initialize WorkData::workRegs.
  for (i = 0; i < Reg::kGroupVirt; i++)
    _workData[i].workRegs = (_workData[i].archRegs & (frame.getDirtyRegs(i) | ~frame.getPreservedRegs(i))) | _workData[i].dstRegs | _workData[i].liveRegs;

  // Create a variable that represents `SARegId` if necessary.
  bool saRegRequired = _hasStackArgs && frame.hasDynamicAlignment() && !frame.hasPreservedFP();

  WorkData& gpRegs = _workData[Reg::kGroupGp];
  uint32_t saCurRegId = frame.getSARegId();
  uint32_t saOutRegId = args.getSARegId();

  if (saCurRegId != Reg::kIdBad) {
    // Check if the provided `SARegId` doesn't collide with input registers.
    if (ASMJIT_UNLIKELY((gpRegs.liveRegs & IntUtils::mask(saCurRegId)) != 0))
      return DebugUtils::errored(kErrorOverlappedRegs);
  }

  if (saOutRegId != Reg::kIdBad) {
    // Check if the provided `SARegId` doesn't collide with argument assignments.
    if (ASMJIT_UNLIKELY((gpRegs.dstRegs & IntUtils::mask(saOutRegId)) != 0))
      return DebugUtils::errored(kErrorOverlappedRegs);
    saRegRequired = true;
  }

  if (saRegRequired) {
    uint32_t ptrTypeId = (archType == ArchInfo::kTypeX86) ? TypeId::kU32 : TypeId::kU64;
    uint32_t ptrRegType = (archType == ArchInfo::kTypeX86) ? Reg::kRegGp32 : Reg::kRegGp64;

    _saVarId = IntUtils::toUInt8(varId);
    _hasPreservedFP = frame.hasPreservedFP();

    Var& var = _vars[varId];
    var.reset();

    if (saCurRegId == Reg::kIdBad) {
      if (saOutRegId != Reg::kIdBad && (gpRegs.liveRegs & IntUtils::mask(saOutRegId)) == 0) {
        saCurRegId = saOutRegId;
      }
      else {
        uint32_t availableRegs = gpRegs.workRegs & ~(gpRegs.liveRegs);
        if (!availableRegs) availableRegs = gpRegs.archRegs & ~gpRegs.workRegs;
        if (ASMJIT_UNLIKELY(!availableRegs))
          return DebugUtils::errored(kErrorNoMorePhysRegs);

        saCurRegId = IntUtils::ctz(availableRegs);
      }
    }

    var.cur.initReg(ptrRegType, saCurRegId, ptrTypeId);
    gpRegs.assign(saCurRegId, varId);
    gpRegs.workRegs |= IntUtils::mask(saCurRegId);

    if (saOutRegId != Reg::kIdBad) {
      var.out.initReg(ptrRegType, saOutRegId, ptrTypeId);
      gpRegs.dstRegs |= IntUtils::mask(saOutRegId);
      gpRegs.workRegs |= IntUtils::mask(saOutRegId);
    }
    else {
      var.cur.addFlags(FuncValue::kIsDone);
    }

    varId++;
  }

  _varCount = varId;

  // Detect register swaps.
  for (varId = 0; varId < _varCount; varId++) {
    Var& var = _vars[varId];
    if (var.cur.isReg()) {
      uint32_t regId = var.cur.getRegId();
      uint32_t regGroup = X86Reg::groupOf(var.cur.getRegType());

      WorkData& set = _workData[regGroup];
      if (set.isAssigned(regId)) {
        Var& other = _vars[set.physToVarId[regId]];
        if (X86Reg::groupOf(other.out.getRegType()) == regGroup && other.out.getRegId() == regId) {
          set.numSwaps++;
          _hasRegSwaps = true;
        }
      }
    }
  }

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error X86FuncArgsContext::markDstRegsDirty(FuncFrame& frame) noexcept {
  for (uint32_t i = 0; i < Reg::kGroupVirt; i++) {
    WorkData& wd = _workData[i];
    uint32_t regs = wd.usedRegs | wd.dstShuf;

    wd.workRegs |= regs;
    frame.addDirtyRegs(i, regs);
  }

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error X86FuncArgsContext::markRegsForSwaps(FuncFrame& frame) noexcept {
  if (!_hasRegSwaps)
    return kErrorOk;

  // If some registers require swapping then select one dirty register that
  // can be used as a temporary. We can do it also without it (by using xors),
  // but using temporary is always safer and also faster approach.
  for (uint32_t i = 0; i < Reg::kGroupVirt; i++) {
    // Skip all register groups where swapping is natively supported (GP regs).
    if (i == X86Reg::kGroupGp) continue;

    // Skip all register groups that don't require swapping.
    WorkData& wd = _workData[i];
    if (!wd.numSwaps) continue;

    // Initially, pick some clobbered or dirty register.
    uint32_t workRegs = wd.workRegs;
    uint32_t regs = workRegs & ~(wd.usedRegs | wd.dstShuf);

    // If that didn't work out pick some register which is not in 'used'.
    if (!regs) regs = workRegs & ~wd.usedRegs;

    // If that didn't work out pick any other register that is allocable.
    // This last resort case will, however, result in marking one more
    // register dirty.
    if (!regs) regs = wd.archRegs & ~workRegs;

    // If that didn't work out we will have to use XORs instead of MOVs.
    if (!regs) continue;

    uint32_t regMask = IntUtils::blsi(regs);
    wd.workRegs |= regMask;
    frame.addDirtyRegs(i, regMask);
  }

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error X86FuncArgsContext::markStackArgsReg(FuncFrame& frame) noexcept {
  // TODO: Validate, improve...
  if (_saVarId != kVarIdNone) {
    const Var& var = _vars[_saVarId];
    frame.setSARegId(var.cur.getRegId());
  }
  else if (frame.hasPreservedFP()) {
    // Always EBP|RBP if the frame-pointer isn't omitted.
    frame.setSARegId(X86Gp::kIdBp);
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - FrameLayout]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::initFuncFrame(FuncFrame& frame, const FuncDetail& func) noexcept {
  uint32_t archType = func.getCallConv().getArchType();

  // Initializing FuncFrame means making a copy of some properties of `func`.
  // Properties like `_localStackSize` will be set by the user before the frame
  // is finalized.
  frame.reset();

  frame._archType = IntUtils::toUInt8(archType);
  frame._spRegId = X86Gp::kIdSp;
  frame._saRegId = X86Gp::kIdBad;

  uint32_t naturalStackAlignment = func.getCallConv().getNaturalStackAlignment();
  uint32_t minimumDynamicAlignment = std::max<uint32_t>(naturalStackAlignment, 16);

  if (minimumDynamicAlignment == naturalStackAlignment)
    minimumDynamicAlignment <<= 1;

  frame._naturalStackAlignment = IntUtils::toUInt8(naturalStackAlignment);
  frame._minimumDynamicAlignment = IntUtils::toUInt8(minimumDynamicAlignment);
  frame._redZoneSize = IntUtils::toUInt8(func.getRedZoneSize());
  frame._spillZoneSize = IntUtils::toUInt8(func.getSpillZoneSize());
  frame._finalStackAlignment = IntUtils::toUInt8(frame._naturalStackAlignment);

  if (func.hasFlag(CallConv::kFlagCalleePopsStack)) {
    frame._calleeStackCleanup = IntUtils::toUInt16(func.getArgStackSize());
  }

  // Initial masks of dirty and preserved registers.
  for (uint32_t group = 0; group < Reg::kGroupVirt; group++) {
    frame._dirtyRegs[group] = func.getPassedRegs(group);
    frame._preservedRegs[group] = func.getPreservedRegs(group);
  }

  // Exclude ESP/RSP - this register is never included in saved GP regs.
  frame._preservedRegs[Reg::kGroupGp] &= ~IntUtils::mask(X86Gp::kIdSp);

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error X86Internal::finalizeFuncFrame(FuncFrame& frame) noexcept {
  uint32_t gpSize = frame.getArchType() == ArchInfo::kTypeX86 ? 4 : 8;

  // The final stack alignment must be updated accordingly to call and local stack alignments.
  uint32_t stackAlignment = frame._finalStackAlignment;
  ASMJIT_ASSERT(stackAlignment == std::max(frame._naturalStackAlignment,
                                  std::max(frame._callStackAlignment,
                                           frame._localStackAlignment)));

  // TODO: Must be configurable.
  uint32_t vecSize = 16;

  bool hasFP = frame.hasPreservedFP();
  bool hasDA = frame.hasDynamicAlignment();

  // Include EBP|RBP if the function preserves the frame-pointer.
  if (hasFP) frame._dirtyRegs[X86Reg::kGroupGp] |= IntUtils::mask(X86Gp::kIdBp);

  // These two are identical if the function doesn't align its stack dynamically.
  uint32_t saRegId = frame.getSARegId();
  if (saRegId == Reg::kIdBad) saRegId = X86Gp::kIdSp;

  // Fix stack arguments base-register from ESP|RSP to EBP|RBP in case it was
  // not picked before and the function performs dynamic stack alignment.
  if (hasDA && saRegId == X86Gp::kIdSp) saRegId = X86Gp::kIdBp;

  // Mark as dirty any register but ESP|RSP if used as SA pointer.
  if (saRegId != X86Gp::kIdSp)
    frame._dirtyRegs[X86Reg::kGroupGp] |= IntUtils::mask(saRegId);

  frame._spRegId = X86Gp::kIdSp;
  frame._saRegId = IntUtils::toUInt8(saRegId);

  // Setup stack size used to save preserved registers.
  frame._gpSaveSize    = IntUtils::toUInt16(IntUtils::popcnt(frame.getSavedRegs(X86Reg::kGroupGp )) * gpSize);
  frame._nonGpSaveSize = IntUtils::toUInt16(IntUtils::popcnt(frame.getSavedRegs(X86Reg::kGroupVec)) * vecSize +
                                            IntUtils::popcnt(frame.getSavedRegs(X86Reg::kGroupMm )) * 8 +
                                            IntUtils::popcnt(frame.getSavedRegs(X86Reg::kGroupK  )) * 8);

  uint32_t v = 0;                             // The beginning of the stack frame relative to SP after prolog.
  v += frame.getCallStackSize();              // Count 'callStackSize'    <- This is used to call functions.
  v  = IntUtils::alignTo(v, stackAlignment);  // Align to function's stack alignment.

  frame._localStackOffset = v;                // Store 'localStackOffset' <- Function's local stack starts here..
  v += frame.getLocalStackSize();             // Count 'localStackSize'   <- Function's local stack ends here.

  // If the function is aligned, calculate the alignment necessary to store
  // vector registers, and set `FuncFrame::kAttrAlignedVecSR` to inform
  // PEI that it can use instructions to perform aligned stores/loads.
  if (stackAlignment >= vecSize && frame._nonGpSaveSize) {
    frame.addAttributes(FuncFrame::kAttrAlignedVecSR);
    v = IntUtils::alignTo(v, vecSize);        // Align '_nonGpSaveOffset'.
  }

  frame._nonGpSaveOffset = v;                 // Store '_nonGpSaveOffset' <- Non-GP Save/Restore starts here.
  v += frame._nonGpSaveSize;                  // Count '_nonGpSaveSize'   <- Non-GP Save/Restore ends here.

  // Calculate if dynamic alignment (DA) slot (stored as offset relative to SP) is required and its offset of it.
  if (hasDA && !hasFP) {
    frame._daOffset = v;                      // Store 'daOffset'         <- DA pointer would be stored here.
    v += gpSize;                              // Count 'daOffset'.
  }
  else {
    frame._daOffset = FuncFrame::kTagInvalidOffset;
  }

  // The return address should be stored after GP save/restore regs. It has
  // the same size as `gpSize` (basically the native register/pointer size).
  // We don't adjust it now as `v` now contains the exact size that the
  // function requires to adjust (call frame + stack frame, vec stack size).
  // The stack (if we consider this size) is misaligned now, as it's always
  // aligned before the function call - when `call()` is executed it pushes
  // the current EIP|RIP onto the stack, and misaligns it by 12 or 8 bytes
  // (depending on the architecture). So count number of bytes needed to align
  // it up to the function's CallFrame (the beginning).
  if (v || frame.hasFuncCalls())
    v += IntUtils::alignDiff(v + frame.getGpSaveSize() + gpSize, stackAlignment);

  frame._gpSaveOffset = v;                    // Store 'gpSaveOffset'     <- Function's GP Save/Restore starts here.
  frame._stackAdjustment = v;                 // Store 'stackAdjustment'  <- SA used by 'add zsp, SA' and 'sub zsp, SA'.

  v += frame._gpSaveSize;                     // Count 'gpSaveSize'       <- Function's GP Save/Restore ends here.
  v += gpSize;                                // Count 'ReturnAddress'    <- As CALL pushes onto stack.
  v += frame.getSpillZoneSize();              // Count 'SpillZoneSize'    <- WIN64 or custom calling convention only.

  // If the function performs dynamic stack alignment then the stack-adjustment must be aligned.
  if (hasDA) frame._stackAdjustment = IntUtils::alignTo(frame._stackAdjustment, stackAlignment);

  uint32_t saInvOff = FuncFrame::kTagInvalidOffset;
  uint32_t saTmpOff = gpSize + frame._gpSaveSize;

  // Calculate where the function arguments start relative to SP.
  frame._saOffsetFromSP = hasDA ? saInvOff : v;

  // Calculate where the function arguments start relative to FP or user-provided register.
  frame._saOffsetFromSA = hasFP ? gpSize * 2  // Return address + frame pointer.
                                : saTmpOff;   // Return address + all saved GP regs.

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - ArgsToFrameInfo]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::argsToFuncFrame(const FuncArgsAssignment& args, FuncFrame& frame) noexcept {
  X86FuncArgsContext ctx;
  ASMJIT_PROPAGATE(ctx.initWorkData(frame, args));
  ASMJIT_PROPAGATE(ctx.markDstRegsDirty(frame));
  ASMJIT_PROPAGATE(ctx.markRegsForSwaps(frame));
  ASMJIT_PROPAGATE(ctx.markStackArgsReg(frame));
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - Emit Helpers]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::emitRegMove(X86Emitter* emitter,
  const Operand_& dst_,
  const Operand_& src_, uint32_t typeId, bool avxEnabled, const char* comment) {

  // Invalid or abstract TypeIds are not allowed.
  ASMJIT_ASSERT(TypeId::isValid(typeId) && !TypeId::isAbstract(typeId));

  Operand dst(dst_);
  Operand src(src_);

  uint32_t instId = Inst::kIdNone;
  uint32_t memFlags = 0;

  enum MemFlags {
    kDstMem = 0x1,
    kSrcMem = 0x2
  };

  // Detect memory operands and patch them to have the same size as the register.
  // CodeCompiler always sets memory size of allocs and spills, so it shouldn't
  // be really necessary, however, after this function was separated from Compiler
  // it's better to make sure that the size is always specified, as we can use
  // 'movzx' and 'movsx' that rely on it.
  if (dst.isMem()) { memFlags |= kDstMem; dst.as<X86Mem>().setSize(src.getSize()); }
  if (src.isMem()) { memFlags |= kSrcMem; src.as<X86Mem>().setSize(dst.getSize()); }

  switch (typeId) {
    case TypeId::kI8:
    case TypeId::kU8:
    case TypeId::kI16:
    case TypeId::kU16:
      // Special case - 'movzx' load.
      if (memFlags & kSrcMem) {
        instId = X86Inst::kIdMovzx;
        dst.setSignature(X86RegTraits<X86Reg::kRegGpd>::kSignature);
      }
      else if (!memFlags) {
        // Change both destination and source registers to GPD (safer, no dependencies).
        dst.setSignature(X86RegTraits<X86Reg::kRegGpd>::kSignature);
        src.setSignature(X86RegTraits<X86Reg::kRegGpd>::kSignature);
      }
      ASMJIT_FALLTHROUGH;

    case TypeId::kI32:
    case TypeId::kU32:
    case TypeId::kI64:
    case TypeId::kU64:
      instId = X86Inst::kIdMov;
      break;

    case TypeId::kMmx32:
      instId = X86Inst::kIdMovd;
      if (memFlags) break;
      ASMJIT_FALLTHROUGH;

    case TypeId::kMmx64 : instId = X86Inst::kIdMovq ; break;
    case TypeId::kMask8 : instId = X86Inst::kIdKmovb; break;
    case TypeId::kMask16: instId = X86Inst::kIdKmovw; break;
    case TypeId::kMask32: instId = X86Inst::kIdKmovd; break;
    case TypeId::kMask64: instId = X86Inst::kIdKmovq; break;

    default: {
      uint32_t elementTypeId = TypeId::elementOf(typeId);
      if (TypeId::isVec32(typeId) && memFlags) {
        if (elementTypeId == TypeId::kF32)
          instId = avxEnabled ? X86Inst::kIdVmovss : X86Inst::kIdMovss;
        else
          instId = avxEnabled ? X86Inst::kIdVmovd : X86Inst::kIdMovd;
        break;
      }

      if (TypeId::isVec64(typeId) && memFlags) {
        if (elementTypeId == TypeId::kF64)
          instId = avxEnabled ? X86Inst::kIdVmovsd : X86Inst::kIdMovsd;
        else
          instId = avxEnabled ? X86Inst::kIdVmovq : X86Inst::kIdMovq;
        break;
      }

      if (elementTypeId == TypeId::kF32)
        instId = avxEnabled ? X86Inst::kIdVmovaps : X86Inst::kIdMovaps;
      else if (elementTypeId == TypeId::kF64)
        instId = avxEnabled ? X86Inst::kIdVmovapd : X86Inst::kIdMovapd;
      else if (typeId <= TypeId::_kVec256End)
        instId = avxEnabled ? X86Inst::kIdVmovdqa : X86Inst::kIdMovdqa;
      else if (elementTypeId <= TypeId::kU32)
        instId = X86Inst::kIdVmovdqa32;
      else
        instId = X86Inst::kIdVmovdqa64;
      break;
    }
  }

  if (!instId)
    return DebugUtils::errored(kErrorInvalidState);

  emitter->setInlineComment(comment);
  return emitter->emit(instId, dst, src);
}

ASMJIT_FAVOR_SIZE Error X86Internal::emitArgMove(X86Emitter* emitter,
  const X86Reg& dst_, uint32_t dstTypeId,
  const Operand_& src_, uint32_t srcTypeId, bool avxEnabled, const char* comment) {

  // Deduce optional `dstTypeId`, which may be `TypeId::kVoid` in some cases.
  if (!dstTypeId) dstTypeId = x86OpData.archRegs.regTypeToTypeId[dst_.getType()];

  // Invalid or abstract TypeIds are not allowed.
  ASMJIT_ASSERT(TypeId::isValid(dstTypeId) && !TypeId::isAbstract(dstTypeId));
  ASMJIT_ASSERT(TypeId::isValid(srcTypeId) && !TypeId::isAbstract(srcTypeId));

  X86Reg dst(dst_);
  Operand src(src_);

  uint32_t dstSize = TypeId::sizeOf(dstTypeId);
  uint32_t srcSize = TypeId::sizeOf(srcTypeId);

  uint32_t instId = Inst::kIdNone;

  // Not a real loop, just 'break' is nicer than 'goto'.
  for (;;) {
    if (TypeId::isInt(dstTypeId)) {
      if (TypeId::isInt(srcTypeId)) {
        instId = X86Inst::kIdMovsx;
        uint32_t typeOp = (dstTypeId << 8) | srcTypeId;

        // Sign extend by using 'movsx'.
        if (typeOp == ((TypeId::kI16 << 8) | TypeId::kI8 ) ||
            typeOp == ((TypeId::kI32 << 8) | TypeId::kI8 ) ||
            typeOp == ((TypeId::kI32 << 8) | TypeId::kI16) ||
            typeOp == ((TypeId::kI64 << 8) | TypeId::kI8 ) ||
            typeOp == ((TypeId::kI64 << 8) | TypeId::kI16)) break;

        // Sign extend by using 'movsxd'.
        instId = X86Inst::kIdMovsxd;
        if (typeOp == ((TypeId::kI64 << 8) | TypeId::kI32)) break;
      }

      if (TypeId::isInt(srcTypeId) || src_.isMem()) {
        // Zero extend by using 'movzx' or 'mov'.
        if (dstSize <= 4 && srcSize < 4) {
          instId = X86Inst::kIdMovzx;
          dst.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
        }
        else {
          // We should have caught all possibilities where `srcSize` is less
          // than 4, so we don't have to worry about 'movzx' anymore. Minimum
          // size is enough to determine if we want 32-bit or 64-bit move.
          instId = X86Inst::kIdMov;
          srcSize = std::min(srcSize, dstSize);

          dst.setSignature(srcSize == 4 ? X86Reg::signatureOfT<X86Reg::kRegGpd>()
                                        : X86Reg::signatureOfT<X86Reg::kRegGpq>());
          if (src.isReg()) src.setSignature(dst.getSignature());
        }
        break;
      }

      // NOTE: The previous branch caught all memory sources, from here it's
      // always register to register conversion, so catch the remaining cases.
      srcSize = std::min(srcSize, dstSize);

      if (TypeId::isMmx(srcTypeId)) {
        // 64-bit move.
        instId = X86Inst::kIdMovq;
        if (srcSize == 8) break;

        // 32-bit move.
        instId = X86Inst::kIdMovd;
        dst.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
        break;
      }

      if (TypeId::isMask(srcTypeId)) {
        instId = X86Inst::kmovIdFromSize(srcSize);
        dst.setSignature(srcSize <= 4 ? X86Reg::signatureOfT<X86Reg::kRegGpd>()
                                      : X86Reg::signatureOfT<X86Reg::kRegGpq>());
        break;
      }

      if (TypeId::isVec(srcTypeId)) {
        // 64-bit move.
        instId = avxEnabled ? X86Inst::kIdVmovq : X86Inst::kIdMovq;
        if (srcSize == 8) break;

        // 32-bit move.
        instId = avxEnabled ? X86Inst::kIdVmovd : X86Inst::kIdMovd;
        dst.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
        break;
      }
    }

    if (TypeId::isMmx(dstTypeId)) {
      instId = X86Inst::kIdMovq;
      srcSize = std::min(srcSize, dstSize);

      if (TypeId::isInt(srcTypeId) || src.isMem()) {
        // 64-bit move.
        if (srcSize == 8) break;

        // 32-bit move.
        instId = X86Inst::kIdMovd;
        if (src.isReg()) src.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
        break;
      }

      if (TypeId::isMmx(srcTypeId)) break;

      // This will hurt if `avxEnabled`.
      instId = X86Inst::kIdMovdq2q;
      if (TypeId::isVec(srcTypeId)) break;
    }

    if (TypeId::isMask(dstTypeId)) {
      srcSize = std::min(srcSize, dstSize);

      if (TypeId::isInt(srcTypeId) || TypeId::isMask(srcTypeId) || src.isMem()) {
        instId = X86Inst::kmovIdFromSize(srcSize);
        if (X86Reg::isGp(src) && srcSize <= 4) src.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
        break;
      }
    }

    if (TypeId::isVec(dstTypeId)) {
      // By default set destination to XMM, will be set to YMM|ZMM if needed.
      dst.setSignature(X86Reg::signatureOfT<X86Reg::kRegXmm>());

      // This will hurt if `avxEnabled`.
      if (X86Reg::isMm(src)) {
        // 64-bit move.
        instId = X86Inst::kIdMovq2dq;
        break;
      }

      // Argument conversion.
      uint32_t dstElement = TypeId::elementOf(dstTypeId);
      uint32_t srcElement = TypeId::elementOf(srcTypeId);

      if (dstElement == TypeId::kF32 && srcElement == TypeId::kF64) {
        srcSize = std::min(dstSize * 2, srcSize);
        dstSize = srcSize / 2;

        if (srcSize <= 8)
          instId = avxEnabled ? X86Inst::kIdVcvtss2sd : X86Inst::kIdCvtss2sd;
        else
          instId = avxEnabled ? X86Inst::kIdVcvtps2pd : X86Inst::kIdCvtps2pd;

        if (dstSize == 32)
          dst.setSignature(X86Reg::signatureOfT<X86Reg::kRegYmm>());
        if (src.isReg())
          src.setSignature(X86Reg::signatureOfVecBySize(srcSize));
        break;
      }

      if (dstElement == TypeId::kF64 && srcElement == TypeId::kF32) {
        srcSize = std::min(dstSize, srcSize * 2) / 2;
        dstSize = srcSize * 2;

        if (srcSize <= 4)
          instId = avxEnabled ? X86Inst::kIdVcvtsd2ss : X86Inst::kIdCvtsd2ss;
        else
          instId = avxEnabled ? X86Inst::kIdVcvtpd2ps : X86Inst::kIdCvtpd2ps;

        dst.setSignature(X86Reg::signatureOfVecBySize(dstSize));
        if (src.isReg() && srcSize >= 32)
          src.setSignature(X86Reg::signatureOfT<X86Reg::kRegYmm>());
        break;
      }

      srcSize = std::min(srcSize, dstSize);
      if (X86Reg::isGp(src) || src.isMem()) {
        // 32-bit move.
        if (srcSize <= 4) {
          instId = avxEnabled ? X86Inst::kIdVmovd : X86Inst::kIdMovd;
          if (src.isReg()) src.setSignature(X86Reg::signatureOfT<X86Reg::kRegGpd>());
          break;
        }

        // 64-bit move.
        if (srcSize == 8) {
          instId = avxEnabled ? X86Inst::kIdVmovq : X86Inst::kIdMovq;
          break;
        }
      }

      if (X86Reg::isVec(src) || src.isMem()) {
        instId = avxEnabled ? X86Inst::kIdVmovaps : X86Inst::kIdMovaps;
        uint32_t sign = X86Reg::signatureOfVecBySize(srcSize);

        dst.setSignature(sign);
        if (src.isReg()) src.setSignature(sign);
        break;
      }
    }

    return DebugUtils::errored(kErrorInvalidState);
  }

  if (src.isMem())
    src.as<X86Mem>().setSize(srcSize);

  emitter->setInlineComment(comment);
  return emitter->emit(instId, dst, src);
}

// ============================================================================
// [asmjit::X86Internal - Emit Prolog & Epilog]
// ============================================================================

static ASMJIT_INLINE void X86Internal_setupSaveRestoreInfo(uint32_t group, const FuncFrame& frame, X86Reg& xReg, uint32_t& xInst, uint32_t& xSize) noexcept {
  switch (group) {
    case X86Reg::kGroupVec:
      xReg = x86::xmm(0);
      xInst = x86GetXmmMovInst(frame);
      xSize = xReg.getSize();
      break;
    case X86Reg::kGroupMm:
      xReg = x86::mm(0);
      xInst = X86Inst::kIdMovq;
      xSize = xReg.getSize();
      break;
    case X86Reg::kGroupK:
      xReg = x86::k(0);
      xInst = X86Inst::kIdKmovq;
      xSize = xReg.getSize();
      break;
    default:
      ASMJIT_NOT_REACHED();
  }
}

ASMJIT_FAVOR_SIZE Error X86Internal::emitProlog(X86Emitter* emitter, const FuncFrame& frame) {
  uint32_t gpSaved = frame.getSavedRegs(X86Reg::kGroupGp);

  X86Gp zsp = emitter->zsp();   // ESP|RSP register.
  X86Gp zbp = emitter->zsp();   // EBP|RBP register.
  zbp.setId(X86Gp::kIdBp);

  X86Gp gpReg = emitter->zsp(); // General purpose register (temporary).
  X86Gp saReg = emitter->zsp(); // Stack-arguments base pointer.

  // Emit: 'push zbp'
  //       'mov  zbp, zsp'.
  if (frame.hasPreservedFP()) {
    gpSaved &= ~IntUtils::mask(X86Gp::kIdBp);
    ASMJIT_PROPAGATE(emitter->push(zbp));
    ASMJIT_PROPAGATE(emitter->mov(zbp, zsp));
  }

  // Emit: 'push gp' sequence.
  {
    IntUtils::BitWordIterator<uint32_t> it(gpSaved);
    while (it.hasNext()) {
      gpReg.setId(it.next());
      ASMJIT_PROPAGATE(emitter->push(gpReg));
    }
  }

  // Emit: 'mov saReg, zsp'.
  uint32_t saRegId = frame.getSARegId();
  if (saRegId != Reg::kIdBad && saRegId != X86Gp::kIdSp) {
    saReg.setId(saRegId);
    if (frame.hasPreservedFP()) {
      if (saRegId != X86Gp::kIdBp)
        ASMJIT_PROPAGATE(emitter->mov(saReg, zbp));
    }
    else {
      ASMJIT_PROPAGATE(emitter->mov(saReg, zsp));
    }
  }

  // Emit: 'and zsp, StackAlignment'.
  if (frame.hasDynamicAlignment()) {
    ASMJIT_PROPAGATE(emitter->and_(zsp, -static_cast<int32_t>(frame.getFinalStackAlignment())));
  }

  // Emit: 'sub zsp, StackAdjustment'.
  if (frame.hasStackAdjustment()) {
    ASMJIT_PROPAGATE(emitter->sub(zsp, frame.getStackAdjustment()));
  }

  // Emit: 'mov [zsp + DAOffset], saReg'.
  if (frame.hasDynamicAlignment() && frame.hasDAOffset()) {
    X86Mem saMem = x86::ptr(zsp, static_cast<int32_t>(frame.getDAOffset()));
    ASMJIT_PROPAGATE(emitter->mov(saMem, saReg));
  }

  // Emit 'movxxx [zsp + X], {[x|y|z]mm, k}'.
  {
    X86Reg xReg;
    X86Mem xBase = x86::ptr(zsp, static_cast<int32_t>(frame.getNonGpSaveOffset()));

    uint32_t xInst;
    uint32_t xSize;

    for (uint32_t group = 1; group < Reg::kGroupVirt; group++) {
      IntUtils::BitWordIterator<uint32_t> it(frame.getSavedRegs(group));
      if (it.hasNext()) {
        X86Internal_setupSaveRestoreInfo(group, frame, xReg, xInst, xSize);
        do {
          xReg.setId(it.next());
          ASMJIT_PROPAGATE(emitter->emit(xInst, xBase, xReg));
          xBase.addOffsetLo32(static_cast<int32_t>(xSize));
        } while (it.hasNext());
      }
    }
  }

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error X86Internal::emitEpilog(X86Emitter* emitter, const FuncFrame& frame) {
  uint32_t i;
  uint32_t regId;

  uint32_t gpSize = emitter->getGpSize();
  uint32_t gpSaved = frame.getSavedRegs(X86Reg::kGroupGp);

  X86Gp zsp = emitter->zsp();   // ESP|RSP register.
  X86Gp zbp = emitter->zsp();   // EBP|RBP register.
  zbp.setId(X86Gp::kIdBp);

  X86Gp gpReg = emitter->zsp(); // General purpose register (temporary).

  // Don't emit 'pop zbp' in the pop sequence, this case is handled separately.
  if (frame.hasPreservedFP())
    gpSaved &= ~IntUtils::mask(X86Gp::kIdBp);

  // Emit 'movxxx {[x|y|z]mm, k}, [zsp + X]'.
  {
    X86Reg xReg;
    X86Mem xBase = x86::ptr(zsp, static_cast<int32_t>(frame.getNonGpSaveOffset()));

    uint32_t xInst;
    uint32_t xSize;

    for (uint32_t group = 1; group < Reg::kGroupVirt; group++) {
      IntUtils::BitWordIterator<uint32_t> it(frame.getSavedRegs(group));
      if (it.hasNext()) {
        X86Internal_setupSaveRestoreInfo(group, frame, xReg, xInst, xSize);
        do {
          xReg.setId(it.next());
          ASMJIT_PROPAGATE(emitter->emit(xInst, xReg, xBase));
          xBase.addOffsetLo32(static_cast<int32_t>(xSize));
        } while (it.hasNext());
      }
    }
  }

  // Emit 'emms' and/or 'vzeroupper'.
  if (frame.hasMmxCleanup()) ASMJIT_PROPAGATE(emitter->emms());
  if (frame.hasAvxCleanup()) ASMJIT_PROPAGATE(emitter->vzeroupper());

  if (frame.hasPreservedFP()) {
    // Emit 'mov zsp, zbp' or 'lea zsp, [zbp - x]'
    int32_t count = static_cast<int32_t>(frame.getGpSaveSize() - gpSize);
    if (!count)
      ASMJIT_PROPAGATE(emitter->mov(zsp, zbp));
    else
      ASMJIT_PROPAGATE(emitter->lea(zsp, x86::ptr(zbp, -count)));
  }
  else {
    if (frame.hasDynamicAlignment() && frame.hasDAOffset()) {
      // Emit 'mov zsp, [zsp + DsaSlot]'.
      X86Mem saMem = x86::ptr(zsp, static_cast<int32_t>(frame.getDAOffset()));
      ASMJIT_PROPAGATE(emitter->mov(zsp, saMem));
    }
    else if (frame.hasStackAdjustment()) {
      // Emit 'add zsp, StackAdjustment'.
      ASMJIT_PROPAGATE(emitter->add(zsp, static_cast<int32_t>(frame.getStackAdjustment())));
    }
  }

  // Emit 'pop gp' sequence.
  if (gpSaved) {
    i = gpSaved;
    regId = 16;

    do {
      regId--;
      if (i & 0x8000) {
        gpReg.setId(regId);
        ASMJIT_PROPAGATE(emitter->pop(gpReg));
      }
      i <<= 1;
    } while (regId != 0);
  }

  // Emit 'pop zbp'.
  if (frame.hasPreservedFP())
    ASMJIT_PROPAGATE(emitter->pop(zbp));

  // Emit 'ret' or 'ret x'.
  if (frame.hasCalleeStackCleanup())
    ASMJIT_PROPAGATE(emitter->emit(X86Inst::kIdRet, static_cast<int>(frame.getCalleeStackCleanup())));
  else
    ASMJIT_PROPAGATE(emitter->emit(X86Inst::kIdRet));

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Internal - Emit Arguments Assignment]
// ============================================================================

ASMJIT_FAVOR_SIZE Error X86Internal::emitArgsAssignment(X86Emitter* emitter, const FuncFrame& frame, const FuncArgsAssignment& args) {
  typedef X86FuncArgsContext::Var Var;
  typedef X86FuncArgsContext::WorkData WorkData;

  enum WorkFlags {
    kHasWork   = 0x01,
    kDidWork   = 0x02,
    kPostponed = 0x04
  };

  X86FuncArgsContext ctx;
  ASMJIT_PROPAGATE(ctx.initWorkData(frame, args));

  uint32_t varCount = ctx._varCount;
  WorkData* workData = ctx._workData;

  // Use AVX if it's enabled.
  bool avxEnabled = frame.isAvxEnabled();

  // Shuffle all registers that are currently assigned as specified by the assignment.
  for (;;) {
    uint32_t flags = 0;

    for (uint32_t varId = 0; varId < varCount; varId++) {
      Var& var = ctx._vars[varId];

      if (var.cur.isDone() || !var.cur.isReg())
        continue;

      uint32_t curType = var.cur.getRegType();
      uint32_t outType = var.out.getRegType();

      uint32_t curGroup = X86Reg::groupOf(curType);
      uint32_t outGroup = X86Reg::groupOf(outType);

      uint32_t curId = var.cur.getRegId();
      uint32_t outId = var.out.getRegId();

      if (curGroup != outGroup) {
        ASMJIT_ASSERT(!"IMPLEMENTED");

        // Requires a conversion between two register groups.
        if (workData[outGroup].numSwaps) {
          // TODO: Postponed
          flags |= kHasWork;
        }
        else {
          // TODO:
          flags |= kHasWork;
        }
      }
      else {
        WorkData& wd = workData[outGroup];
        if (!wd.isAssigned(outId)) {
EmitMove:
          ASMJIT_PROPAGATE(
            emitArgMove(emitter,
              X86Reg::fromTypeAndId(outType, outId), var.out.getTypeId(),
              X86Reg::fromTypeAndId(curType, curId), var.cur.getTypeId(), avxEnabled));

          wd.reassign(outId, curId, varId);
          var.cur.initReg(outType, outId, var.out.getTypeId());

          if (outId == var.out.getRegId())
            var.cur.addFlags(FuncValue::kIsDone);
          flags |= kDidWork;
        }
        else {
          uint32_t altId = wd.physToVarId[outId];
          Var& altVar = ctx._vars[altId];

          if (!altVar.out.isInitialized() || altVar.out.getRegId() == curId) {
            // Swap operation is possible only between two GP registers.
            if (curGroup == X86Reg::kGroupGp) {
              uint32_t highestType = std::max(var.cur.getRegType(), altVar.cur.getRegType());
              uint32_t signature = highestType == X86Reg::kRegGpq ? uint32_t(X86RegTraits<X86Reg::kRegGpq>::kSignature)
                                                                  : uint32_t(X86RegTraits<X86Reg::kRegGpd>::kSignature);
              ASMJIT_PROPAGATE(
                emitter->emit(X86Inst::kIdXchg,
                  X86Reg::fromSignature(signature, outId),
                  X86Reg::fromSignature(signature, curId)));

              wd.swap(curId, varId, outId, altId);
              var.cur.setRegId(outId);
              var.cur.addFlags(FuncValue::kIsDone);
              altVar.cur.setRegId(curId);

              if (altVar.out.isInitialized())
                altVar.cur.addFlags(FuncValue::kIsDone);
              flags |= kDidWork;
            }
            else {
              // If there is a scratch register it can be used to do perform the swap.
              uint32_t availableRegs = (wd.liveRegs & wd.workRegs);
              if (availableRegs) {
                uint32_t inOutRegs = wd.dstRegs;
                if (availableRegs & inOutRegs) availableRegs &= inOutRegs;

                outId = IntUtils::ctz(availableRegs);
                goto EmitMove;
              }
              else {
                flags |= kHasWork;
              }
            }
          }
          else {
            flags |= kHasWork;
          }
        }
      }
    }

    if (!(flags & kHasWork))
      break;

    if (!(flags & kDidWork))
      return DebugUtils::errored(kErrorInvalidState);
  }

  // Load arguments passed by stack into registers. This is pretty simple and
  // it never requires multiple iterations like the previous phase.
  if (ctx._hasStackArgs) {
    uint32_t iterCount = 1;

    uint32_t saVarId = ctx._saVarId;
    uint32_t saRegId = X86Gp::kIdSp;

    if (frame.hasDynamicAlignment()) {
      if (frame.hasPreservedFP())
        saRegId = X86Gp::kIdBp;
      else
        saRegId = saVarId < varCount ? ctx._vars[saVarId].cur.getRegId() : frame.getSARegId();
    }

    // Base address of all arguments passed by stack.
    X86Mem saMem = x86::ptr(emitter->gpz(saRegId), static_cast<int32_t>(frame.getSAOffset(saRegId)));

    for (uint32_t iter = 0; iter < iterCount; iter++) {
      for (uint32_t varId = 0; varId < varCount; varId++) {
        Var& var = ctx._vars[varId];
        if (var.cur.isStack() && !var.cur.isDone()) {
          uint32_t outId = var.out.getRegId();
          uint32_t outType = var.out.getRegType();

          uint32_t group = X86Reg::groupOf(outType);
          WorkData& wd = ctx._workData[group];

          if (outId == saRegId && group == Reg::kGroupGp) {
            if (iterCount == 1) {
              iterCount++;
              continue;
            }
            wd.unassign(outId);
          }

          X86Reg dstReg = X86Reg::fromTypeAndId(outType, outId);
          X86Mem srcMem = saMem.adjusted(var.cur.getStackOffset());

          ASMJIT_PROPAGATE(
            emitArgMove(emitter,
              dstReg, var.out.getTypeId(),
              srcMem, var.cur.getTypeId(), avxEnabled));

          wd.assign(outId, varId);
          var.cur.initReg(outType, outId, var.cur.getTypeId(), FuncValue::kIsDone);
        }
      }
    }
  }

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86
