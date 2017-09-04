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
#include "../base/cpuinfo.h"
#include "../base/intutils.h"
#include "../x86/x86assembler.h"
#include "../x86/x86compiler.h"
#include "../x86/x86internal_p.h"
#include "../x86/x86rapass_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86OpInfo]
// ============================================================================

namespace X86OpInfo {
  enum {
    Any = Reg::kIdBad,
    Zax = X86Gp::kIdAx,
    Zbx = X86Gp::kIdBx,
    Zcx = X86Gp::kIdCx,
    Zdx = X86Gp::kIdDx,
    Zsi = X86Gp::kIdSi,
    Zdi = X86Gp::kIdDi,
  };

#define R(IDX) { uint16_t(OpInfo::kUse | OpInfo::kRead  | (IDX == Any ? uint32_t(0) : uint32_t(OpInfo::kUseFixed))), uint8_t(IDX), uint8_t(0) }
#define W(IDX) { uint16_t(OpInfo::kOut | OpInfo::kWrite | (IDX == Any ? uint32_t(0) : uint32_t(OpInfo::kOutFixed))), uint8_t(IDX), uint8_t(0) }
#define X(IDX) { uint16_t(OpInfo::kUse | OpInfo::kRW    | (IDX == Any ? uint32_t(0) : uint32_t(OpInfo::kUseFixed))), uint8_t(IDX), uint8_t(0) }
#define NONE() { uint16_t(0), uint8_t(Any), uint8_t(0) }
#define DEFINE_OPS(NAME, ...) static const OpInfo NAME[6] = { __VA_ARGS__ }
#define RETURN_OPS(...) do { DEFINE_OPS(ops, __VA_ARGS__); return ops; } while(0)

  // Common cases.
  DEFINE_OPS(op_r    , R(Any), R(Any), R(Any), R(Any), R(Any), R(Any));
  DEFINE_OPS(op_w    , W(Any), R(Any), R(Any), R(Any), R(Any), R(Any));
  DEFINE_OPS(op_x    , X(Any), R(Any), R(Any), R(Any), R(Any), R(Any));
  DEFINE_OPS(op_xx   , X(Any), X(Any), R(Any), R(Any), R(Any), R(Any));
  DEFINE_OPS(op_w_all, W(Any), W(Any), W(Any), W(Any), W(Any), W(Any));

  static ASMJIT_INLINE const OpInfo* get(uint32_t instId, const X86Inst& instData, const Operand* opArray, uint32_t opCount) noexcept {
    const X86Inst::CommonData& commonData = instData.getCommonData();
    if (!commonData.hasFixedRM()) {
      if (commonData.isUseXX()) return op_xx;
      if (commonData.isUseX()) return op_x;
      if (commonData.isUseW()) return op_w;
      if (commonData.isUseR()) return op_r;
    }
    else {
      switch (instId) {
        case X86Inst::kIdAaa:
        case X86Inst::kIdAad:
        case X86Inst::kIdAam:
        case X86Inst::kIdAas:
        case X86Inst::kIdDaa:
        case X86Inst::kIdDas:
          RETURN_OPS(X(Zax));

        case X86Inst::kIdCpuid:
          RETURN_OPS(X(Zax), W(Zbx), X(Zcx), W(Zdx));

        case X86Inst::kIdCbw:
        case X86Inst::kIdCdqe:
        case X86Inst::kIdCwde:
          RETURN_OPS(X(Zax));

        case X86Inst::kIdCdq:
        case X86Inst::kIdCwd:
        case X86Inst::kIdCqo:
          RETURN_OPS(W(Zdx), R(Zax));

        case X86Inst::kIdCmpxchg:
          RETURN_OPS(X(Any), R(Any), X(Zax));

        case X86Inst::kIdCmpxchg8b:
        case X86Inst::kIdCmpxchg16b:
          RETURN_OPS(NONE(), X(Zdx), X(Zax), R(Zcx), R(Zbx));

        case X86Inst::kIdDiv:
        case X86Inst::kIdIdiv:
          if (opCount == 2)
            RETURN_OPS(X(Zax), R(Any));
          else
            RETURN_OPS(X(Zdx), X(Zax), R(Any));

        case X86Inst::kIdImul:
          if (opCount == 2) {
            if (X86Reg::isGpw(opArray[0]) && opArray[1].getSize() == 1)
              RETURN_OPS(W(Zax), R(Any)); // imul ax, r8/m8
            else
              RETURN_OPS(X(Any), R(Any)); // imul r?, r?/m?
          }

          if (opCount == 3) {
            if (opArray[2].isImm())
              return op_w;
            else
              RETURN_OPS(W(Zdx), X(Zax), R(Any));
          }
          break;

        case X86Inst::kIdMul:
          if (opCount == 2)
            RETURN_OPS(X(Zax), R(Any));
          else
            RETURN_OPS(W(Zdx), X(Zax), R(Any));

        case X86Inst::kIdMulx:
          RETURN_OPS(W(Any), W(Any), R(Any), R(Zdx));

        case X86Inst::kIdJecxz:
        case X86Inst::kIdLoop:
        case X86Inst::kIdLoope:
        case X86Inst::kIdLoopne:
          RETURN_OPS(R(Zcx));

        case X86Inst::kIdLahf: RETURN_OPS(W(Zax));
        case X86Inst::kIdSahf: RETURN_OPS(R(Zax));

        case X86Inst::kIdRet: break;
        case X86Inst::kIdEnter: break;
        case X86Inst::kIdLeave: break;

        case X86Inst::kIdMonitor    : RETURN_OPS(R(Zax), R(Zcx), R(Zdx));
        case X86Inst::kIdMwait      : RETURN_OPS(R(Zax), R(Zcx));

        case X86Inst::kIdPush       : return op_r;
        case X86Inst::kIdPop        : return op_w;

        case X86Inst::kIdRcl:
        case X86Inst::kIdRcr:
        case X86Inst::kIdRol:
        case X86Inst::kIdRor:
        case X86Inst::kIdSal:
        case X86Inst::kIdSar:
        case X86Inst::kIdShl:
        case X86Inst::kIdShr:
          RETURN_OPS(X(Any), R(Zcx));

        case X86Inst::kIdShld:
        case X86Inst::kIdShrd:
          RETURN_OPS(X(Any), R(Any), R(Zcx));

        case X86Inst::kIdRdtsc:
        case X86Inst::kIdRdtscp:
          RETURN_OPS(W(Zdx), W(Zax), W(Zcx));

        case X86Inst::kIdXrstor:
        case X86Inst::kIdXrstor64:
        case X86Inst::kIdXsave:
        case X86Inst::kIdXsave64:
        case X86Inst::kIdXsaveopt:
        case X86Inst::kIdXsaveopt64:
          RETURN_OPS(W(Any), R(Zdx), R(Zax));

        case X86Inst::kIdXgetbv:
          RETURN_OPS(W(Zdx), W(Zax), R(Zcx));

        case X86Inst::kIdXsetbv:
          RETURN_OPS(R(Zdx), R(Zax), R(Zcx));

        case X86Inst::kIdIn  : RETURN_OPS(W(Zax), R(Zdx));
        case X86Inst::kIdIns : RETURN_OPS(X(Zdi), R(Zdx));
        case X86Inst::kIdOut : RETURN_OPS(R(Zdx), R(Zax));
        case X86Inst::kIdOuts: RETURN_OPS(R(Zdx), X(Zsi));

        case X86Inst::kIdCmps: RETURN_OPS(X(Zsi), X(Zdi));
        case X86Inst::kIdLods: RETURN_OPS(W(Zax), X(Zsi));
        case X86Inst::kIdMovs: RETURN_OPS(X(Zdi), X(Zsi));
        case X86Inst::kIdScas: RETURN_OPS(X(Zdi), R(Zax));
        case X86Inst::kIdStos: RETURN_OPS(X(Zdi), R(Zax));

        case X86Inst::kIdMaskmovq:
        case X86Inst::kIdMaskmovdqu:
        case X86Inst::kIdVmaskmovdqu:
          RETURN_OPS(R(Any), R(Any), R(Zdi));

        case X86Inst::kIdBlendvpd:
        case X86Inst::kIdBlendvps:
        case X86Inst::kIdPblendvb:
        case X86Inst::kIdSha256rnds2:
          RETURN_OPS(W(Any), R(Any), R(0));

        case X86Inst::kIdPcmpestri  :
        case X86Inst::kIdVpcmpestri : RETURN_OPS(R(Any), R(Any), NONE(), W(Zcx));
        case X86Inst::kIdPcmpistri  :
        case X86Inst::kIdVpcmpistri : RETURN_OPS(R(Any), R(Any), NONE(), W(Zcx), R(Zax), R(Zdx));
        case X86Inst::kIdPcmpestrm  :
        case X86Inst::kIdVpcmpestrm : RETURN_OPS(R(Any), R(Any), NONE(), W(0));
        case X86Inst::kIdPcmpistrm  :
        case X86Inst::kIdVpcmpistrm : RETURN_OPS(R(Any), R(Any), NONE(), W(0)  , R(Zax), R(Zdx));
      }
    }

    return op_x;
  }

#undef RETURN_OPS
#undef DEFINE_OPS
#undef NONE
#undef X
#undef W
#undef R
} // X86OpInfo namespace

// ============================================================================
// [asmjit::X86RAPass - Construction / Destruction]
// ============================================================================

X86RAPass::X86RAPass() noexcept
  : RAPass(),
    _avxEnabled(false) {}
X86RAPass::~X86RAPass() noexcept {}

// ============================================================================
// [asmjit::X86RAPass - OnInit / OnDone]
// ============================================================================

void X86RAPass::onInit() noexcept {
  uint32_t archType = cc()->getArchType();
  uint32_t baseRegCount = archType == ArchInfo::kTypeX86 ? 8 : 16;

  _archTraits[X86Reg::kGroupGp] |= RAArchTraits::kHasSwap;

  _physRegCount.set(X86Reg::kGroupGp , baseRegCount);
  _physRegCount.set(X86Reg::kGroupVec, baseRegCount);
  _physRegCount.set(X86Reg::kGroupMm , 8);
  _physRegCount.set(X86Reg::kGroupK  , 8);
  _buildPhysIndex();

  _availableRegCount = _physRegCount;
  _availableRegs[X86Reg::kGroupGp ] = IntUtils::bits(_physRegCount.get(X86Reg::kGroupGp ));
  _availableRegs[X86Reg::kGroupVec] = IntUtils::bits(_physRegCount.get(X86Reg::kGroupVec));
  _availableRegs[X86Reg::kGroupMm ] = IntUtils::bits(_physRegCount.get(X86Reg::kGroupMm ));
  _availableRegs[X86Reg::kGroupK  ] = IntUtils::bits(_physRegCount.get(X86Reg::kGroupK  ));

  // The architecture specific setup makes implicitly all registers available. So
  // make unavailable all registers that are special and cannot be used in general.
  bool hasFP = _func->getFrame().hasPreservedFP();

  makeUnavailable(X86Reg::kGroupGp, X86Gp::kIdSp);            // ESP|RSP used as a stack-pointer (SP).
  if (hasFP) makeUnavailable(X86Reg::kGroupGp, X86Gp::kIdBp); // EBP|RBP used as a frame-pointer (FP).

  _sp = cc()->zsp();
  _fp = cc()->zbp();
  _avxEnabled = false;
}

void X86RAPass::onDone() noexcept {}

// ============================================================================
// [asmjit::X86RAPass - CFG - Build CFG]
// ============================================================================

static ASMJIT_INLINE uint64_t immMaskFromSize(uint32_t size) noexcept {
  ASMJIT_ASSERT(size > 0 && size < 256);
  static uint64_t masks[] = {
    ASMJIT_UINT64_C(0x00000000000000FF), //   1
    ASMJIT_UINT64_C(0x000000000000FFFF), //   2
    ASMJIT_UINT64_C(0x00000000FFFFFFFF), //   4
    ASMJIT_UINT64_C(0xFFFFFFFFFFFFFFFF), //   8
    ASMJIT_UINT64_C(0x0000000000000000), //  16
    ASMJIT_UINT64_C(0x0000000000000000), //  32
    ASMJIT_UINT64_C(0x0000000000000000), //  64
    ASMJIT_UINT64_C(0x0000000000000000), // 128
    ASMJIT_UINT64_C(0x0000000000000000)  // 256
  };
  return masks[IntUtils::ctz(size)];
}

class X86RACFGBuilder : public RACFGBuilder<X86RACFGBuilder> {
public:
  ASMJIT_INLINE X86RACFGBuilder(X86RAPass* pass) noexcept
    : RACFGBuilder<X86RACFGBuilder>(pass),
      _is64Bit(pass->getGpSize() == 8) {}

  ASMJIT_INLINE Error onInst(CBInst* inst, RABlock* block, uint32_t& jumpType, RARegsStats& blockRegStats) noexcept {
    X86Compiler* cc = static_cast<X86RAPass*>(_pass)->cc();
    uint32_t numVirtRegs = cc->getVirtRegArray().getLength();

    uint32_t instId = inst->getInstId();
    uint32_t opCount = inst->getOpCount();

    RAInstBuilder ib;
    const OpInfo* opInfo = nullptr;

    if (X86Inst::isDefinedId(instId)) {
      const X86Inst& instData = X86Inst::getInst(instId);
      const X86Inst::CommonData& commonData = instData.getCommonData();

      bool hasGpbHiConstraint = false;
      uint32_t singleRegOps = 0;

      if (opCount) {
        const Operand* opArray = inst->getOpArray();
        opInfo = X86OpInfo::get(instId, instData, opArray, opCount);

        for (uint32_t i = 0; i < opCount; i++) {
          const Operand& op = opArray[i];
          if (op.isReg()) {
            // Register operand.
            const X86Reg& reg = op.as<X86Reg>();
            uint32_t vIndex = Operand::unpackId(reg.getId());

            uint32_t flags = opInfo[i].getFlags();
            uint32_t allowedRegs = 0xFFFFFFFFU;

            // X86-specific constraints related to LO|HI general purpose registers.
            if (reg.isGpb()) {
              flags |= RATiedReg::kX86Gpb;
              if (!_is64Bit) {
                // Restrict to first four - AL|AH|BL|BH|CL|CH|DL|DH. In 32-bit mode
                // it's not possible to access SIL|DIL, etc, so this is just enough.
                allowedRegs = 0x0FU;
              }
              else {
                // If we encountered GPB-HI register the situation is much more
                // complicated than in 32-bit mode. We need to patch all registers
                // to not use ID higher than 7 and all GPB-LO registers to not use
                // index higher than 3. Instead of doing the patching here we just
                // set a flag and will do it later, to not complicate this loop.
                if (reg.isGpbHi()) {
                  hasGpbHiConstraint = true;
                  allowedRegs = 0x0FU;
                }
              }
            }

            if (vIndex < Operand::kPackedIdCount) {
              if (ASMJIT_UNLIKELY(vIndex >= numVirtRegs))
                return DebugUtils::errored(kErrorInvalidVirtId);

              VirtReg* virtReg = cc->getVirtRegAt(vIndex);
              RAWorkReg* workReg;
              ASMJIT_PROPAGATE(_pass->asWorkReg(virtReg, &workReg));

              uint32_t group = workReg->getGroup();
              uint32_t allocable = _pass->_availableRegs[group] & allowedRegs;

              uint32_t useId = Reg::kIdBad;
              uint32_t outId = Reg::kIdBad;

              if (opInfo[i].isUse())
                useId = opInfo[i].getPhysId();
              else
                outId = opInfo[i].getPhysId();

              ASMJIT_PROPAGATE(ib.add(workReg, flags, allocable, useId, outId));

              if (singleRegOps == i)
                singleRegOps++;
            }
          }
          else if (op.isMem()) {
            // Memory operand.
            const X86Mem& mem = op.as<X86Mem>();
            if (mem.isRegHome()) {
              uint32_t vIndex = Operand::unpackId(mem.getBaseId());
              if (ASMJIT_UNLIKELY(vIndex >= numVirtRegs))
                return DebugUtils::errored(kErrorInvalidVirtId);

              VirtReg* virtReg = cc->getVirtRegAt(vIndex);
              RAWorkReg* workReg;
              ASMJIT_PROPAGATE(_pass->asWorkReg(virtReg, &workReg));
              _pass->markStackUsed(workReg);
            }
            else if (mem.hasBaseReg()) {
              uint32_t vIndex = Operand::unpackId(mem.getBaseId());
              if (vIndex < Operand::kPackedIdCount) {
                if (ASMJIT_UNLIKELY(vIndex >= numVirtRegs))
                  return DebugUtils::errored(kErrorInvalidVirtId);

                VirtReg* virtReg = cc->getVirtRegAt(vIndex);
                RAWorkReg* workReg;
                ASMJIT_PROPAGATE(_pass->asWorkReg(virtReg, &workReg));

                uint32_t group = workReg->getGroup();
                uint32_t allocable = _pass->_availableRegs[group];
                ASMJIT_PROPAGATE(ib.add(workReg, RATiedReg::kUse | RATiedReg::kRead, allocable, Reg::kIdBad, Reg::kIdBad));
              }
            }

            if (mem.hasIndexReg()) {
              uint32_t vIndex = Operand::unpackId(mem.getIndexId());
              if (vIndex < Operand::kPackedIdCount) {
                if (ASMJIT_UNLIKELY(vIndex >= numVirtRegs))
                  return DebugUtils::errored(kErrorInvalidVirtId);

                VirtReg* virtReg = cc->getVirtRegAt(vIndex);
                RAWorkReg* workReg;
                ASMJIT_PROPAGATE(_pass->asWorkReg(virtReg, &workReg));

                uint32_t group = workReg->getGroup();
                uint32_t allocable = _pass->_availableRegs[group];
                ASMJIT_PROPAGATE(ib.add(workReg, RATiedReg::kUse | RATiedReg::kRead, allocable, Reg::kIdBad, Reg::kIdBad));
              }
            }
          }
        }
      }

      // Handle extra operand (either REP {cx|ecx|rcx} or AVX-512 {k} selector).
      if (inst->hasExtraReg()) {
        uint32_t vIndex = Operand::unpackId(inst->getExtraReg().getId());
        if (vIndex < Operand::kPackedIdCount) {
          if (ASMJIT_UNLIKELY(vIndex >= numVirtRegs))
            return DebugUtils::errored(kErrorInvalidVirtId);

          VirtReg* virtReg = cc->getVirtRegAt(vIndex);
          RAWorkReg* workReg;
          ASMJIT_PROPAGATE(_pass->asWorkReg(virtReg, &workReg));

          uint32_t group = virtReg->getGroup();
          if (group == X86Gp::kGroupK) {
            // AVX-512 mask selector {k} register - read-only, allocable to any register except {k0}.
            uint32_t allocableRegs= _pass->_availableRegs[group] & ~IntUtils::mask(0);
            ASMJIT_PROPAGATE(ib.add(workReg, RATiedReg::kUse | RATiedReg::kRead, allocableRegs, Reg::kIdBad, Reg::kIdBad));
            singleRegOps = 0;
          }
          else {
            // REP {cx|ecx|rcx} register - read & write, allocable to {cx|ecx|rcx} only.
            ASMJIT_PROPAGATE(ib.add(workReg, RATiedReg::kUse | RATiedReg::kUseFixed | RATiedReg::kRW, 0, X86Gp::kIdCx, X86Gp::kIdBad));
          }
        }
        else {
          uint32_t group = inst->getExtraReg().getGroup();
          if (group == X86Gp::kGroupK && inst->getExtraReg().getId() != 0)
            singleRegOps = 0;
        }
      }

      // Handle X86 constraints.
      if (hasGpbHiConstraint) {
        for (uint32_t i = 0; i < ib.getTiedRegCount(); i++) {
          RATiedReg* tiedReg = ib[i];
          tiedReg->allocableRegs &= tiedReg->hasFlag(RATiedReg::kX86Gpb) ? 0x0FU : 0xFFU;
        }
      }

      if (ib.getTiedRegCount() == 1) {
        // Handle special cases of some instructions where all operands share the same
        // register. In such case the single operand becomes read-only or write-only.
        uint32_t singleRegCase = X86Inst::kSingleRegNone;
        if (singleRegOps == opCount) {
          singleRegCase = commonData.getSingleRegCase();
        }
        else if (opCount == 2 && inst->getOp(1).isImm()) {
          // Handle some tricks used by X86 asm.
          const Reg& reg = inst->getOp(0).as<Reg>();
          const Imm& imm = inst->getOp(1).as<Imm>();

          const RAWorkReg* workReg = _pass->getWorkReg(ib[0]->workId);
          uint32_t workRegSize = workReg->getInfo().getSize();

          switch (inst->getInstId()) {
            case X86Inst::kIdOr: {
              // Sets the value of the destination register to -1, previous content unused.
              if (reg.getSize() >= 4 || reg.getSize() >= workRegSize) {
                if (imm.getInt64() == -1 || imm.getUInt64() == immMaskFromSize(reg.getSize()))
                  singleRegCase = X86Inst::kSingleRegWO;
              }
              ASMJIT_FALLTHROUGH;
            }

            case X86Inst::kIdAdd:
            case X86Inst::kIdAnd:
            case X86Inst::kIdRol:
            case X86Inst::kIdRor:
            case X86Inst::kIdSar:
            case X86Inst::kIdShl:
            case X86Inst::kIdShr:
            case X86Inst::kIdSub:
            case X86Inst::kIdXor: {
              // Updates [E|R]FLAGS without changing the content.
              if (reg.getSize() != 4 || reg.getSize() >= workRegSize) {
                if (imm.getUInt64() == 0)
                  singleRegCase = X86Inst::kSingleRegRO;
              }
              break;
            }
          }
        }

        switch (singleRegCase) {
          case X86Inst::kSingleRegNone:
            break;
          case X86Inst::kSingleRegRO:
            ib[0]->makeReadOnly();
            opInfo = X86OpInfo::op_r;
            break;
          case X86Inst::kSingleRegWO:
            ib[0]->makeWriteOnly();
            opInfo = X86OpInfo::op_w_all;
            break;
        }
      }

      jumpType = commonData.getJumpType();
    }

    // Handle `CCFuncCall` and `CCFuncRet` constructs.
    uint32_t nodeType = inst->getType();
    if (nodeType != CBNode::kNodeInst) {
      if (nodeType == CBNode::kNodeFuncCall) {
        // TODO:
        ASMJIT_ASSERT(!"IMPLEMENTED");
      }
      else if (nodeType == CBNode::kNodeFuncRet) {
        opInfo = X86OpInfo::op_r;

        const FuncDetail& funcDetail = _pass->getFunc()->getDetail();
        const Operand* opArray = inst->getOpArray();

        for (uint32_t i = 0; i < opCount; i++) {
          const Operand& op = opArray[i];
          if (op.isNone()) continue;

          const FuncValue& ret = funcDetail.getRet(i);
          if (ASMJIT_UNLIKELY(!ret.isReg()))
            return DebugUtils::errored(kErrorInvalidState);

          if (op.isReg()) {
            // Register return value.
            const X86Reg& reg = op.as<X86Reg>();
            uint32_t vIndex = Operand::unpackId(reg.getId());

            if (vIndex < Operand::kPackedIdCount) {
              if (ASMJIT_UNLIKELY(vIndex >= numVirtRegs))
                return DebugUtils::errored(kErrorInvalidVirtId);

              VirtReg* virtReg = cc->getVirtRegAt(vIndex);
              RAWorkReg* workReg;
              ASMJIT_PROPAGATE(_pass->asWorkReg(virtReg, &workReg));

              uint32_t group = workReg->getGroup();
              uint32_t allocable = _pass->_availableRegs[group];
              ASMJIT_PROPAGATE(ib.add(workReg, RATiedReg::kUse | RATiedReg::kUseFixed | RATiedReg::kRead, allocable, ret.getRegId(), Reg::kIdBad));
            }
          }
          else {
            return DebugUtils::errored(kErrorInvalidState);
          }
        }

        jumpType = Inst::kJumpTypeReturn;
      }
      else {
        return DebugUtils::errored(kErrorInvalidInstruction);
      }
    }

    ASMJIT_PROPAGATE(_pass->assignRAInst(inst, block, opInfo, ib));
    blockRegStats.combineWith(ib._stats);
    return kErrorOk;
  }

  bool _is64Bit;
};

Error X86RAPass::onBuildCFG() noexcept {
  return X86RACFGBuilder(this).run();
}

// ============================================================================
// [asmjit::X86RAPass - Allocation - Emit]
// ============================================================================

Error X86RAPass::onEmitMove(uint32_t workId, uint32_t dstPhysId, uint32_t srcPhysId) noexcept {
  RAWorkReg* wReg = getWorkReg(workId);
  Reg dst(X86Reg::fromSignature(wReg->getInfo().getSignature(), dstPhysId));
  Reg src(X86Reg::fromSignature(wReg->getInfo().getSignature(), srcPhysId));

  const char* comment = nullptr;
#if !defined(ASMJIT_DISABLE_LOGGING)
  if (_loggerOptions & Logger::kOptionAnnotate) {
    _tmpString.setFormat("MOVE %s", getWorkReg(workId)->getName());
    comment = _tmpString.getData();
  }
#endif

  return X86Internal::emitRegMove(cc()->asEmitter(), dst, src, wReg->getTypeId(), _avxEnabled, comment);
}

Error X86RAPass::onEmitSwap(uint32_t aWorkId, uint32_t aPhysId, uint32_t bWorkId, uint32_t bPhysId) noexcept {
  RAWorkReg* waReg = getWorkReg(aWorkId);
  RAWorkReg* wbReg = getWorkReg(bWorkId);

  uint32_t is64 = std::max(waReg->getTypeId(), wbReg->getTypeId()) >= TypeId::kI64;
  uint32_t sign = is64 ? uint32_t(X86RegTraits<X86Reg::kRegGpq>::kSignature)
                       : uint32_t(X86RegTraits<X86Reg::kRegGpd>::kSignature);

#if !defined(ASMJIT_DISABLE_LOGGING)
  if (_loggerOptions & Logger::kOptionAnnotate) {
    _tmpString.setFormat("SWAP %s, %s", waReg->getName(), wbReg->getName());
    cc()->setInlineComment(_tmpString.getData());
  }
#endif

  return cc()->emit(X86Inst::kIdXchg, X86Reg::fromSignature(sign, aPhysId),
                                      X86Reg::fromSignature(sign, bPhysId));
}

Error X86RAPass::onEmitLoad(uint32_t workId, uint32_t dstPhysId) noexcept {
  RAWorkReg* wReg = getWorkReg(workId);
  Reg dstReg(X86Reg::fromSignature(wReg->getInfo().getSignature(), dstPhysId));
  Mem srcMem(workRegAsMem(wReg));

  const char* comment = nullptr;
#if !defined(ASMJIT_DISABLE_LOGGING)
  if (_loggerOptions & Logger::kOptionAnnotate) {
    _tmpString.setFormat("LOAD %s", getWorkReg(workId)->getName());
    comment = _tmpString.getData();
  }
#endif

  return X86Internal::emitRegMove(cc()->asEmitter(), dstReg, srcMem, wReg->getTypeId(), _avxEnabled, comment);
}

Error X86RAPass::onEmitSave(uint32_t workId, uint32_t srcPhysId) noexcept {
  RAWorkReg* wReg = getWorkReg(workId);
  Mem dstMem(workRegAsMem(wReg));
  Reg srcReg(X86Reg::fromSignature(wReg->getInfo().getSignature(), srcPhysId));

  const char* comment = nullptr;
#if !defined(ASMJIT_DISABLE_LOGGING)
  if (_loggerOptions & Logger::kOptionAnnotate) {
    _tmpString.setFormat("SAVE %s", getWorkReg(workId)->getName());
    comment = _tmpString.getData();
  }
#endif

  return X86Internal::emitRegMove(cc()->asEmitter(), dstMem, srcReg, wReg->getTypeId(), _avxEnabled, comment);
}

Error X86RAPass::onEmitJump(const Label& label) noexcept {
  return cc()->jmp(label);
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86 && !ASMJIT_DISABLE_COMPILER
