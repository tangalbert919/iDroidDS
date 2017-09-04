// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_RALOCAL_P_H
#define _ASMJIT_BASE_RALOCAL_P_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/intutils.h"
#include "../base/raassignment_p.h"
#include "../base/radefs_p.h"
#include "../base/rapass_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_ra
//! \{

// ============================================================================
// [asmjit::RALocalAllocator]
// ============================================================================

//! Local register allocator.
class RALocalAllocator {
public:
  ASMJIT_NONCOPYABLE(RALocalAllocator)

  typedef RAAssignment::PhysToWorkMap PhysToWorkMap;
  typedef RAAssignment::WorkToPhysMap WorkToPhysMap;

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RALocalAllocator(RAPass* pass) noexcept
    : _pass(pass),
      _cc(pass->cc()),
      _archTraits(pass->_archTraits),
      _availableRegs(pass->_availableRegs),
      _clobberedRegs(),
      _assignment(),
      _block(nullptr),
      _cbInst(nullptr),
      _raInst(nullptr),
      _tiedTotal(),
      _tiedCount() {}

  Error init() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RAWorkReg* getWorkReg(uint32_t workId) const noexcept { return _pass->getWorkReg(workId); }
  ASMJIT_INLINE PhysToWorkMap* getPhysToWorkMap() const noexcept { return _assignment.getPhysToWorkMap(); }
  ASMJIT_INLINE WorkToPhysMap* getWorkToPhysMap() const noexcept { return _assignment.getWorkToPhysMap(); }

  // --------------------------------------------------------------------------
  // [Block]
  // --------------------------------------------------------------------------

  //! Get the currently processed block.
  ASMJIT_INLINE RABlock* getBlock() const noexcept { return _block; }
  //! Set the currently processed block.
  ASMJIT_INLINE void setBlock(RABlock* block) noexcept { _block = block; }

  // --------------------------------------------------------------------------
  // [Instruction]
  // --------------------------------------------------------------------------

  //! Get the currently processed `CBInst`.
  ASMJIT_INLINE CBInst* getCBInst() const noexcept { return _cbInst; }
  //! Get the currently processed `RAInst`.
  ASMJIT_INLINE RAInst* getRAInst() const noexcept { return _raInst; }

  //! Get all tied regs.
  ASMJIT_INLINE RATiedReg* getTiedRegs() const noexcept { return _raInst->getTiedRegs(); }
  //! Get grouped tied regs.
  ASMJIT_INLINE RATiedReg* getTiedRegs(uint32_t group) const noexcept { return _raInst->getTiedRegs(group); }

  //! Get TiedReg count (all).
  ASMJIT_INLINE uint32_t getTiedCount() const noexcept { return _tiedTotal; }
  //! Get TiedReg count (per class).
  ASMJIT_INLINE uint32_t getTiedCount(uint32_t group) const noexcept { return _tiedCount.get(group); }

  ASMJIT_INLINE bool isGroupUsed(uint32_t group) const noexcept { return _tiedCount[group] != 0; }

  // --------------------------------------------------------------------------
  // [Allocation - Run]
  // --------------------------------------------------------------------------

  Error makeInitialAssignment() noexcept;

  Error allocInst(CBInst* cbInst) noexcept;
  Error allocBranch(CBInst* cbInst, RABlock* target, RABlock* cont) noexcept;

  Error replaceAssignment(
    const PhysToWorkMap* physToWorkMap,
    const WorkToPhysMap* workToPhysMap) noexcept;

  //! Switch to the given assignment by reassigning all register and emitting
  //! code that reassigns them. This is always used to switch to a previously
  //! stored assignment.
  //!
  //! If `tryMode` is true then the final assignment doesn't have to be exactly
  //! same as specified by `dstPhysToWorkMap` and `dstWorkToPhysMap`. This mode
  //! is only used before conditional jumps that already have assignment to
  //! generate a code sequence that is always executed regardless of the flow.
  Error switchToAssignment(
    PhysToWorkMap* dstPhysToWorkMap,
    WorkToPhysMap* dstWorkToPhysMap,
    const ZoneBitVector& liveIn,
    bool dstReadOnly,
    bool tryMode) noexcept;

  ASMJIT_INLINE void onBeforeRun(CBInst* cbInst, RAInst* raInst) noexcept;
  ASMJIT_INLINE void onAfterRun(CBInst* cbInst) noexcept;
  ASMJIT_INLINE Error runOnGroup(uint32_t group) noexcept;

  // --------------------------------------------------------------------------
  // [Allocation - Decision Making]
  // --------------------------------------------------------------------------

  enum CostModel {
    kCostOfFrequency = 1048576,
    kCostOfDirtyFlag = kCostOfFrequency / 4
  };

  ASMJIT_INLINE uint32_t costByFrequency(float freq) const noexcept {
    return static_cast<uint32_t>(int32_t(freq * float(kCostOfFrequency)));
  }

  ASMJIT_INLINE uint32_t calculateSpillCost(uint32_t group, uint32_t workId, uint32_t assignedId) const noexcept {
    RAWorkReg* workReg = getWorkReg(workId);
    uint32_t cost = costByFrequency(workReg->getLiveStats().getFreq());

    if (_assignment.isPhysDirty(group, assignedId))
      cost += kCostOfDirtyFlag;

    return cost;
  }

  //! Decide on register assignment.
  uint32_t decideOnAssignment(uint32_t group, uint32_t workId, uint32_t assignedId, uint32_t allocableRegs) const noexcept;

  //! Decide on whether to MOVE or SPILL the given WorkReg.
  //!
  //! The function must return either `RAAssignment::kPhysNone`, which means that
  //! the WorkReg should be spilled, or a valid physical register ID, which means
  //! that the register should be moved to that physical register instead.
  uint32_t decideOnUnassignment(uint32_t group, uint32_t workId, uint32_t assignedId, uint32_t allocableRegs) const noexcept;

  //! Decide on best spill given a register mask `spillableRegs`
  uint32_t decideOnBestSpill(uint32_t group, uint32_t spillableRegs, uint32_t* outWorkId) const noexcept;

  // --------------------------------------------------------------------------
  // [Allocation - Emit]
  // --------------------------------------------------------------------------

  //! Emit a move between a destination and source register, and fix the register assignment.
  ASMJIT_INLINE Error onMoveReg(uint32_t group, uint32_t workId, uint32_t dstPhysId, uint32_t srcPhysId) noexcept {
    if (dstPhysId == srcPhysId) return kErrorOk;
    _assignment.reassign(group, workId, dstPhysId, srcPhysId);
    return _pass->onEmitMove(workId, dstPhysId, srcPhysId);
  }

  //! Emit a swap between two physical registers and fix their assignment.
  //!
  //! NOTE: Target must support this operation otherwise this would ASSERT.
  ASMJIT_INLINE Error onSwapReg(uint32_t group, uint32_t aWorkId, uint32_t aPhysId, uint32_t bWorkId, uint32_t bPhysId) noexcept {
    _assignment.swap(group, aWorkId, aPhysId, bWorkId, bPhysId);
    return _pass->onEmitSwap(aWorkId, aPhysId, bWorkId, bPhysId);
  }

  //! Emit a load from [VirtReg/WorkReg]'s spill slot to a physical register and make it assigned and clean.
  ASMJIT_INLINE Error onLoadReg(uint32_t group, uint32_t workId, uint32_t physId) noexcept {
    _assignment.assign(group, workId, physId, RAAssignment::kClean);
    return _pass->onEmitLoad(workId, physId);
  }

  //! Emit a save a physical register to a [VirtReg/WorkReg]'s spill slot, keep it assigned, and make it clean.
  ASMJIT_INLINE Error onSaveReg(uint32_t group, uint32_t workId, uint32_t physId) noexcept {
    ASMJIT_ASSERT(_assignment.workToPhysId(group, workId) == physId);
    ASMJIT_ASSERT(_assignment.physToWorkId(group, physId) == workId);

    _assignment.makeClean(group, workId, physId);
    return _pass->onEmitSave(workId, physId);
  }

  //! Assign a register, the content of it is undefined at this point.
  ASMJIT_INLINE Error onAssignReg(uint32_t group, uint32_t workId, uint32_t physId, uint32_t dirty) noexcept {
    _assignment.assign(group, workId, physId, dirty);
    return kErrorOk;
  }

  //! Spill variable/register, saves the content to the memory-home if modified.
  ASMJIT_INLINE Error onSpillReg(uint32_t group, uint32_t workId, uint32_t physId) noexcept {
    if (_assignment.isPhysDirty(group, physId))
      ASMJIT_PROPAGATE(onSaveReg(group, workId, physId));
    return onKillReg(group, workId, physId);
  }

  ASMJIT_INLINE Error onDirtyReg(uint32_t group, uint32_t workId, uint32_t physId) noexcept {
    _assignment.makeDirty(group, workId, physId);
    return kErrorOk;
  }

  ASMJIT_INLINE Error onKillReg(uint32_t group, uint32_t workId, uint32_t physId) noexcept {
    _assignment.unassign(group, workId, physId);
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  RAPass* _pass;                         //!< Link to `RAPass`.
  CodeCompiler* _cc;                     //!< Link to `CodeCompiler`.

  RAArchTraits _archTraits;              //!< Architecture traits.
  RARegMask _availableRegs;              //!< Registers available to the allocator.
  RARegMask _clobberedRegs;              //!< Registers clobbered by the allocator.

  RAAssignment _assignment;              //!< Register assignment (current).
  RAAssignment _tmpAssignment;           //!< Register assignment used temporarily during assignment switches.

  RABlock* _block;                       //!< Link to the current `RABlock`.
  CBInst* _cbInst;                       //!< CB instruction.
  RAInst* _raInst;                       //!< RA instruction.

  uint32_t _tiedTotal;                   //!< Count of all TiedReg's.
  RARegCount _tiedCount;                 //!< TiedReg's total counter.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_RALOCAL_P_H
