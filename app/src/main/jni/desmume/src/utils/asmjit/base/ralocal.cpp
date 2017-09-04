// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/intutils.h"
#include "../base/ralocal_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::RALocalAllocator - Init / Reset]
// ============================================================================

Error RALocalAllocator::init() noexcept {
  PhysToWorkMap* physToWorkMap;
  WorkToPhysMap* workToPhysMap;

  physToWorkMap = _pass->newPhysToWorkMap();
  workToPhysMap = _pass->newWorkToPhysMap();
  if (!physToWorkMap || !workToPhysMap)
    return DebugUtils::errored(kErrorNoHeapMemory);

  _assignment.initLayout(_pass->_physRegCount, _pass->getWorkRegs());
  _assignment.initMaps(physToWorkMap, workToPhysMap);

  physToWorkMap = _pass->newPhysToWorkMap();
  workToPhysMap = _pass->newWorkToPhysMap();
  if (!physToWorkMap || !workToPhysMap)
    return DebugUtils::errored(kErrorNoHeapMemory);

  _tmpAssignment.initLayout(_pass->_physRegCount, _pass->getWorkRegs());
  _tmpAssignment.initMaps(physToWorkMap, workToPhysMap);

  return kErrorOk;
}

// ============================================================================
// [asmjit::RALocalAllocator - Run]
// ============================================================================

Error RALocalAllocator::makeInitialAssignment() noexcept {
  CCFunc* func = _pass->getFunc();
  RABlock* entry = _pass->getEntryBlock();

  ZoneBitVector& liveIn = entry->getLiveIn();
  uint32_t argCount = func->getArgCount();

  for (uint32_t iter = 0; iter < 2; iter++) {
    for (uint32_t i = 0; i < argCount; i++) {
      VirtReg* virtReg = func->getArg(i);

      // Unassigned argument.
      if (!virtReg)
        continue;

      RAWorkReg* workReg = virtReg->getWorkReg();

      // Unused argument.
      if (!workReg)
        continue;

      uint32_t workId = workReg->getWorkId();
      if (liveIn.getAt(workId)) {
        uint32_t group = workReg->getGroup();
        if (_assignment.workToPhysId(group, workId) != RAAssignment::kPhysNone)
          continue;

        uint32_t allocableRegs = _availableRegs[group] & ~_assignment.getAssigned(group);
        if (workReg->hasHomeId()) {
          uint32_t physId = workReg->getHomeId();
          if (allocableRegs & IntUtils::mask(physId)) {
            _assignment.assign(group, workId, physId, true);
            _pass->_argsAssignment.assignReg(i, workReg->getInfo().getType(), physId, workReg->getTypeId());
            continue;
          }
        }

        if (iter > 0) {
          uint32_t physId = IntUtils::ctz(allocableRegs);
          _assignment.assign(group, workId, physId, true);
          _pass->_argsAssignment.assignReg(i, workReg->getInfo().getType(), physId, workReg->getTypeId());
        }
      }
    }
  }

  return kErrorOk;
}

Error RALocalAllocator::allocInst(CBInst* cbInst) noexcept {
  RAInst* raInst = cbInst->getPassData<RAInst>();

#if defined(ASMJIT_DEBUG_LRA)
  {
    StringBuilderTmp<256> sb;
    Logging::formatNode(sb, 0, _cc, cbInst);

    const RATiedReg* tiedRegs = raInst->getTiedRegs();
    uint32_t tiedCount = raInst->getTiedCount();
    if (tiedCount) {
      sb.padEnd(40);
      sb.appendString(" <- ");

      for (uint32_t i = 0; i < tiedCount; i++) {
        const RATiedReg& tiedReg = tiedRegs[i];
        if (i) sb.appendChar(' ');

        sb.appendFormat("%s{", _pass->getWorkReg(tiedReg.getWorkId())->getName());
        sb.appendChar(tiedReg.isReadWrite() ? 'X' :
                      tiedReg.isRead()      ? 'R' :
                      tiedReg.isWrite()     ? 'W' : '?');

        if (tiedReg.hasUseId())
          sb.appendFormat(" Use=%u", tiedReg.getUseId());
        else if (tiedReg.isUse())
          sb.appendString(" Use");

        if (tiedReg.hasOutId())
          sb.appendFormat(" Out=%u", tiedReg.getOutId());
        else if (tiedReg.isOut())
          sb.appendString(" Out");

        if (tiedReg.isLast()) sb.appendString(" Last");
        if (tiedReg.isKill()) sb.appendString(" Kill");

        sb.appendString("}");
      }
    }

    printf("  LRA: %s\n", sb.getData());
  }
#endif

  onBeforeRun(cbInst, raInst);
  if (raInst->_tiedTotal != 0) {
    for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
      ASMJIT_PROPAGATE(runOnGroup(group));
  }
  onAfterRun(cbInst);

  return kErrorOk;
}

Error RALocalAllocator::allocBranch(CBInst* cbInst, RABlock* target, RABlock* cont) noexcept {
  // The cursor must point to the previous instruction for a possible instruction insertion.
  _cc->_setCursor(cbInst->getPrev());

  // Use TryMode of `switchToAssignment()` if possible.
  if (target->hasEntryAssignment()) {
    ASMJIT_PROPAGATE(switchToAssignment(
      target->getEntryPhysToWorkMap(),
      target->getEntryWorkToPhysMap(),
      target->getLiveIn(),
      target->isAllocated(),
      true));
  }

  ASMJIT_PROPAGATE(allocInst(cbInst));

  if (target->hasEntryAssignment()) {
    CBNode* injectionPoint = _pass->getExtraBlock()->getPrev();
    CBNode* prevCursor = _cc->setCursor(injectionPoint);

    _tmpAssignment.copyFrom(_assignment);
    ASMJIT_PROPAGATE(switchToAssignment(
      target->getEntryPhysToWorkMap(),
      target->getEntryWorkToPhysMap(),
      target->getLiveIn(),
      target->isAllocated(),
      false));

    CBNode* curCursor = _cc->getCursor();
    if (curCursor != injectionPoint) {
      // Additional instructions emitted to switch from the current state to
      // the `target`s state. This means that we have to move these instructions
      // into an independent code block and patch the jump location.
      Operand& target(cbInst->getOp(cbInst->getOpCount() - 1));
      if (ASMJIT_UNLIKELY(!target.isLabel()))
        return DebugUtils::errored(kErrorInvalidState);

      Label trampoline = _cc->newLabel();
      Label savedTarget = target.as<Label>();

      // Patch `target` to point to the `trampoline` we just created.
      target = trampoline;

      // Clear a possible SHORT form as we have no clue now if the SHORT form would
      // be encodable after patching the target to `trampoline` (X86 specific).
      cbInst->clearInstOptions(Inst::kOptionShortForm);

      // Finalize the switch assignment sequence.
      ASMJIT_PROPAGATE(_pass->onEmitJump(savedTarget));
      _cc->_setCursor(injectionPoint);
      _cc->bind(trampoline);
    }

    _cc->_setCursor(prevCursor);
    _assignment.swapWith(_tmpAssignment);
  }
  else {
    ASMJIT_PROPAGATE(_pass->setBlockEntryAssignment(target, getBlock(), _assignment));
  }

  return kErrorOk;
}

Error RALocalAllocator::replaceAssignment(
  const PhysToWorkMap* physToWorkMap,
  const WorkToPhysMap* workToPhysMap) noexcept {

  _assignment.copyFrom(physToWorkMap, workToPhysMap);
  return kErrorOk;
}

Error RALocalAllocator::switchToAssignment(
  PhysToWorkMap* dstPhysToWorkMap,
  WorkToPhysMap* dstWorkToPhysMap,
  const ZoneBitVector& liveIn,
  bool dstReadOnly,
  bool tryMode) noexcept {

  RAAssignment dst;
  RAAssignment& cur = _assignment;

  dst.initLayout(_pass->_physRegCount, _pass->getWorkRegs());
  dst.initMaps(dstPhysToWorkMap, dstWorkToPhysMap);

  for (uint32_t group = 0; group < Reg::kGroupVirt; group++) {
    // ------------------------------------------------------------------------
    // STEP 1:
    //   - KILL all registers that are not live at `dst`,
    //   - SPILL all registers that are not assigned at `dst`.
    // ------------------------------------------------------------------------

    if (!tryMode) {
      IntUtils::BitWordIterator<uint32_t> it(cur.getAssigned(group));
      while (it.hasNext()) {
        uint32_t physId = it.next();
        uint32_t workId = cur.physToWorkId(group, physId);

        // Must be true as we iterate over assigned registers.
        ASMJIT_ASSERT(workId != RAAssignment::kWorkNone);

        // KILL if it's not live on entry.
        if (!liveIn.getAt(workId)) {
          onKillReg(group, workId, physId);
          continue;
        }

        // SPILL if it's not assigned on entry.
        uint32_t altId = dst.workToPhysId(group, workId);
        if (altId == RAAssignment::kPhysNone) {
          ASMJIT_PROPAGATE(onSpillReg(group, workId, physId));
        }
      }
    }

    // ------------------------------------------------------------------------
    // STEP 2:
    //   - MOVE and SWAP registers from their current assignments into their
    //     DST assignments.
    //   - Build `willLoadRegs` mask of registers scheduled for `onLoadReg()`.
    // ------------------------------------------------------------------------

    int32_t runId = -1;                             // Current run-id (1 means more aggressive decisions).
    uint32_t willLoadRegs = 0;                      // Remaining registers scheduled for `onLoadReg()`.
    uint32_t affectedRegs = dst.getAssigned(group); // Remaining registers to be allocated in this loop.

    while (affectedRegs) {
      IntUtils::BitWordIterator<uint32_t> it(affectedRegs);

      if (++runId == 2) {
        if (!tryMode)
          return DebugUtils::errored(kErrorInvalidState);

        // Stop in `tryMode` if we haven't done anything in last two rounds.
        break;
      }

      while (it.hasNext()) {
        uint32_t physId = it.next();
        uint32_t physMask = IntUtils::mask(physId);

        uint32_t curWorkId = cur.physToWorkId(group, physId);
        uint32_t dstWorkId = dst.physToWorkId(group, physId);

        // The register must have assigned `dstWorkId` as we only iterate over assigned regs.
        ASMJIT_ASSERT(dstWorkId != RAAssignment::kWorkNone);

        if (curWorkId != RAAssignment::kWorkNone) {
          // Both assigned.
          if (curWorkId != dstWorkId) {
            // Wait a bit if this is the first run, we may avoid this if `curWorkId` moves out.
            if (runId <= 0)
              continue;

            uint32_t altPhysId = cur.workToPhysId(group, dstWorkId);
            if (altPhysId == RAAssignment::kPhysNone)
              continue;

            // Reset as we will do some changes to the current assignment.
            runId = -1;

            if (_archTraits.hasSwap(group)) {
              ASMJIT_PROPAGATE(onSwapReg(group, curWorkId, physId, dstWorkId, altPhysId));
            }
            else {
              // SPILL the reg if it's not dirty in DST, otherwise try to MOVE.
              if (!cur.isPhysDirty(group, physId)) {
                ASMJIT_PROPAGATE(onKillReg(group, curWorkId, physId));
              }
              else {
                uint32_t allocableRegs = _pass->_availableRegs[group] & ~cur.getAssigned(group);

                // If possible don't conflict with assigned regs at DST.
                if (allocableRegs & ~dst.getAssigned(group))
                  allocableRegs &= ~dst.getAssigned(group);

                if (allocableRegs) {
                  // MOVE is possible, thus preferred.
                  uint32_t tmpPhysId = IntUtils::ctz(allocableRegs);

                  ASMJIT_PROPAGATE(onMoveReg(group, curWorkId, tmpPhysId, physId));
                  _pass->_clobberedRegs[group] |= IntUtils::mask(tmpPhysId);
                }
                else {
                  // MOVE is impossible, must SPILL.
                  ASMJIT_PROPAGATE(onSpillReg(group, curWorkId, physId));
                }
              }

              goto Cleared;
            }
          }
        }
        else {
Cleared:
          // DST assigned, CUR unassigned.
          uint32_t altPhysId = cur.workToPhysId(group, dstWorkId);
          if (altPhysId == RAAssignment::kPhysNone) {
            if (liveIn.getAt(dstWorkId))
              willLoadRegs |=  physMask; // Scheduled for `onLoadReg()`.
            affectedRegs &= ~physMask;   // Unaffected from now.
            continue;
          }
          ASMJIT_PROPAGATE(onMoveReg(group, dstWorkId, physId, altPhysId));
        }

        // Both DST and CUR assigned to the same reg or CUR just moved to DST.
        if ((dst.getDirty(group) & physMask) == 0 && (cur.getDirty(group) & physMask) != 0) {
          // If `dstReadOnly` is true it means that that block was already
          // processed and we cannot change from CLEAN to DIRTY. In that case
          // the register has to be saved as it cannot enter the block DIRTY.
          if (dstReadOnly)
            ASMJIT_PROPAGATE(onSaveReg(group, dstWorkId, physId));
          else
            dst.makeDirty(group, dstWorkId, physId);
        }

        runId = -1;
        affectedRegs &= ~physMask;
      }
    }

    // ------------------------------------------------------------------------
    // STEP 3:
    //   - Load registers specified by `willLoadRegs`.
    // ------------------------------------------------------------------------

    {
      IntUtils::BitWordIterator<uint32_t> it(willLoadRegs);
      while (it.hasNext()) {
        uint32_t physId = it.next();

        if ((cur.getAssigned(group) & IntUtils::mask(physId)) == 0) {
          uint32_t workId = dst.physToWorkId(group, physId);

          // The algorithm is broken if it tries to load register that is not in LIVE-IN.
          ASMJIT_ASSERT(liveIn.getAt(workId) == true);

          ASMJIT_PROPAGATE(onLoadReg(group, workId, physId));
        }
        else {
          // Not possible otherwise.
          ASMJIT_ASSERT(tryMode == true);
        }
      }
    }
  }

  return kErrorOk;
}

ASMJIT_INLINE void RALocalAllocator::onBeforeRun(CBInst* cbInst, RAInst* raInst) noexcept {
  // The cursor must point to the previous instruction for a possible instruction insertion.
  _cc->_setCursor(cbInst->getPrev());

  _cbInst = cbInst;
  _raInst = raInst;

  _tiedTotal = raInst->_tiedTotal;
  _tiedCount = raInst->_tiedCount;
}

ASMJIT_INLINE void RALocalAllocator::onAfterRun(CBInst* cbInst) noexcept {
}

ASMJIT_INLINE uint32_t RALocalAllocator::runOnGroup(uint32_t group) noexcept {
  uint32_t willUse = _raInst->_usedRegs[group];
  uint32_t willOut = _raInst->_clobberedRegs[group];
  uint32_t willFree = 0;

  uint32_t i, count = getTiedCount(group);
  RATiedReg* tiedRegs = getTiedRegs(group);

  uint32_t usePending = count;
  uint32_t outPending = 0;

  // --------------------------------------------------------------------------
  // STEP 1:
  //
  // Calculate `willUse` and `willFree` masks based on tied registers we have.
  //
  // We don't do any assignment decisions at this stage as we just need to collect some
  // information first. Then, after we populate all masks needed we can finally make some
  // decisions in the second loop. The main reason for this is that we really need `willFree`
  // to make assignment decisions for `willUse`, because if we mark some registers that will
  // be freed, we can consider them in decision making afterwards.
  // --------------------------------------------------------------------------

  for (i = 0; i < count; i++) {
    RATiedReg* tiedReg = &tiedRegs[i];

    // Add OUT and KILL to `outPending` for CLOBBERing and/or OUT assignment.
    outPending += static_cast<uint32_t>(tiedReg->isOutOrKill());

    if (!tiedReg->isUse()) {
      tiedReg->markUseDone();
      usePending--;
      continue;
    }

    uint32_t workId = tiedReg->getWorkId();
    uint32_t assignedId = _assignment.workToPhysId(group, workId);

    if (tiedReg->hasUseId()) {
      // If the register has `useId` it means it can only be allocated in that register.
      uint32_t useMask = IntUtils::mask(tiedReg->getUseId());

      // RAInstBuilder must have collected `usedRegs` on-the-fly.
      ASMJIT_ASSERT((willUse & useMask) != 0);

      if (assignedId == tiedReg->getUseId()) {
        // If the register is already allocated in this one, mark it done and continue.
        tiedReg->markUseDone();
        if (tiedReg->isWrite())
          _assignment.makeDirty(group, workId, assignedId);
        usePending--;
        willUse |= useMask;
      }
      else {
        willFree |= (useMask & _assignment.getAssigned(group));
      }
    }
    else {
      // Check if the register must be moved to `allocableRegs`.
      uint32_t allocableRegs = tiedReg->allocableRegs;
      if (assignedId != RAAssignment::kPhysNone) {
        uint32_t assignedMask = IntUtils::mask(assignedId);
        if ((allocableRegs & ~willUse) & assignedMask) {
          tiedReg->setUseId(assignedId);
          tiedReg->markUseDone();
          if (tiedReg->isWrite())
            _assignment.makeDirty(group, workId, assignedId);
          usePending--;
          willUse |= assignedMask;
        }
        else {
          willFree |= assignedMask;
        }
      }
    }
  }

  // --------------------------------------------------------------------------
  // STEP 2:
  //
  // Do some decision making to find the best candidates of registers that
  // need to be assigned, moved, and/or spilled. Only USE registers are
  // considered here, OUT will be decided later after all CLOBBERed and OUT
  // registers are unassigned.
  // --------------------------------------------------------------------------

  if (usePending) {
    // TODO: Not sure `liveRegs` should be used, maybe willUse and willFree would be enough and much more clear.

    // All registers that are currently alive without registers that will be freed.
    uint32_t liveRegs = _assignment.getAssigned(group) & ~willFree;

    for (i = 0; i < count; i++) {
      RATiedReg* tiedReg = &tiedRegs[i];
      if (tiedReg->isUseDone()) continue;

      uint32_t workId = tiedReg->getWorkId();
      uint32_t assignedId = _assignment.workToPhysId(group, workId);

      if (!tiedReg->hasUseId()) {
        uint32_t allocableRegs = tiedReg->allocableRegs & ~(willFree | willUse);

        // DECIDE where to assign the register (USE).
        uint32_t useId = decideOnAssignment(group, workId, assignedId, allocableRegs);
        uint32_t useMask = IntUtils::mask(useId);

        willUse |= useMask;
        willFree |= useMask & liveRegs;
        tiedReg->setUseId(useId);

        if (assignedId != RAAssignment::kPhysNone) {
          uint32_t assignedMask = IntUtils::mask(assignedId);

          willFree |= assignedMask;
          liveRegs &= ~assignedMask;

          // OPTIMIZATION: Assign the USE register here if it's possible.
          if (!(liveRegs & useMask)) {
            ASMJIT_PROPAGATE(onMoveReg(group, workId, useId, assignedId));
            tiedReg->markUseDone();
            if (tiedReg->isWrite())
              _assignment.makeDirty(group, workId, useId);
            usePending--;
          }
        }
        else {
          // OPTIMIZATION: Assign the USE register here if it's possible.
          if (!(liveRegs & useMask)) {
            ASMJIT_PROPAGATE(onLoadReg(group, workId, useId));
            tiedReg->markUseDone();
            if (tiedReg->isWrite())
              _assignment.makeDirty(group, workId, useId);
            usePending--;
          }
        }

        liveRegs |= useMask;
      }
    }
  }

  // Initially all used regs will be marked clobbered.
  uint32_t clobberedByInst = willUse | willOut;

  // --------------------------------------------------------------------------
  // STEP 3:
  //
  // Free all registers that we marked as `willFree`. Only registers that are not
  // USEd by the instruction are considered as we don't want to free regs we need.
  // --------------------------------------------------------------------------

  if (willFree) {
    uint32_t allocableRegs = _availableRegs[group] & ~(_assignment.getAssigned(group) | willFree | willUse | willOut);
    IntUtils::BitWordIterator<uint32_t> it(willFree);

    do {
      uint32_t assignedId = it.next();
      uint32_t workId = _assignment.physToWorkId(group, assignedId);

      // DECIDE whether to MOVE or SPILL.
      if (allocableRegs) {
        uint32_t reassignedId = decideOnUnassignment(group, workId, assignedId, allocableRegs);
        if (reassignedId != RAAssignment::kPhysNone) {
          ASMJIT_PROPAGATE(onMoveReg(group, workId, reassignedId, assignedId));
          allocableRegs ^= IntUtils::mask(reassignedId);
          continue;
        }
      }

      ASMJIT_PROPAGATE(onSpillReg(group, workId, assignedId));
    } while (it.hasNext());
  }

  // --------------------------------------------------------------------------
  // STEP 4:
  //
  // ALLOCATE / SHUFFLE all registers that we marked as `willUse` and weren't
  // allocated yet. This is a bit complicated as the allocation is iterative.
  // In some cases we have to wait before allocating a particual physical
  // register as it's still occupied by some other one, which we need to move
  // before we can use it. In this case we skip it and allocate another some
  // other instead (making it free for another iteration).
  //
  // NOTE: Iterations are mostly important for complicated allocations like
  // function calls, where there can be up to N registers used at once. Asm
  // instructions won't run the loop more than once in 99.9% of cases as they
  // use 2..3 registers in average.
  // --------------------------------------------------------------------------

  if (usePending) {
    bool mustSwap = false;
    do {
      uint32_t oldPending = usePending;

      for (i = 0; i < count; i++) {
        RATiedReg* thisTiedReg = &tiedRegs[i];
        if (thisTiedReg->isUseDone()) continue;

        uint32_t thisWorkId = thisTiedReg->getWorkId();
        uint32_t thisPhysId = _assignment.workToPhysId(group, thisWorkId);

        // This would be a bug, fatal one!
        uint32_t targetPhysId = thisTiedReg->getUseId();
        ASMJIT_ASSERT(targetPhysId != thisPhysId);

        uint32_t targetWorkId = _assignment.physToWorkId(group, targetPhysId);
        if (targetWorkId != RAAssignment::kWorkNone) {
          RAWorkReg* targetWorkReg = getWorkReg(targetWorkId);

          // Swapping two registers can solve two allocation tasks by emitting
          // just a single instruction. However, swap is only available on few
          // architectures and it's definitely not available for each register
          // group. Calling `onSwapReg()` before checking these would be fatal.
          if (_archTraits.hasSwap(group) && thisPhysId != RAAssignment::kPhysNone) {
            ASMJIT_PROPAGATE(onSwapReg(group, thisWorkId, thisPhysId, targetWorkId, targetPhysId));

            thisTiedReg->markUseDone();
            if (thisTiedReg->isWrite())
              _assignment.makeDirty(group, thisWorkId, targetPhysId);
            usePending--;

            // Double-hit.
            RATiedReg* targetTiedReg = targetWorkReg->getTiedReg();
            if (targetTiedReg && targetTiedReg->getUseId() == thisPhysId) {
              targetTiedReg->markUseDone();
              if (targetTiedReg->isWrite())
                _assignment.makeDirty(group, targetWorkId, thisPhysId);
              usePending--;
            }
            continue;
          }

          if (!mustSwap)
            continue;

          // Only branched here if the previous iteration did nothing. This is
          // essentially a SWAP operation without having a dedicated instruction
          // for that purpose (vector registers, etc). The simplest way to
          // handle such case is to SPILL the target register.
          ASMJIT_PROPAGATE(onSpillReg(group, targetWorkId, targetPhysId));
        }

        if (thisPhysId != RAAssignment::kPhysNone) {
          ASMJIT_PROPAGATE(onMoveReg(group, thisWorkId, targetPhysId, thisPhysId));

          thisTiedReg->markUseDone();
          if (thisTiedReg->isWrite())
            _assignment.makeDirty(group, thisWorkId, targetPhysId);
          usePending--;
        }
        else {
          ASMJIT_PROPAGATE(onLoadReg(group, thisWorkId, targetPhysId));

          thisTiedReg->markUseDone();
          if (thisTiedReg->isWrite())
            _assignment.makeDirty(group, thisWorkId, targetPhysId);
          usePending--;
        }
      }

      mustSwap = (oldPending == usePending);
    } while (usePending);
  }

  // --------------------------------------------------------------------------
  // STEP 5:
  //
  // KILL registers marked as KILL/OUT.
  // --------------------------------------------------------------------------

  if (outPending) {
    for (i = 0; i < count; i++) {
      RATiedReg* tiedReg = &tiedRegs[i];
      if (!tiedReg->isOutOrKill()) continue;

      uint32_t workId = tiedReg->getWorkId();
      uint32_t physId = _assignment.workToPhysId(group, workId);

      // Must check if it's allocated as KILL can be related to OUT (like KILL
      // immediately after OUT, which could mean the register is not assigned).
      if (physId != RAAssignment::kPhysNone) {
        ASMJIT_PROPAGATE(onKillReg(group, workId, physId));
        willOut &= ~IntUtils::mask(physId);
      }

      // We still maintain number of pending registers for OUT assignment.
      // So, if this is only KILL, not OUT, we can safely decrement it.
      outPending -= !tiedReg->isOut();
    }
  }

  // --------------------------------------------------------------------------
  // STEP 6:
  //
  // SPILL registers that will be CLOBBERed. Since OUT and KILL were
  // already processed this is used mostly to handle function CALLs.
  // --------------------------------------------------------------------------

  if (willOut) {
    IntUtils::BitWordIterator<uint32_t> it(willOut);
    do {
      uint32_t physId = it.next();
      uint32_t workId = _assignment.physToWorkId(group, physId);

      if (workId == RAAssignment::kWorkNone)
        continue;

      ASMJIT_PROPAGATE(onSpillReg(group, workId, physId));
    } while (it.hasNext());
  }

  // --------------------------------------------------------------------------
  // STEP 7:
  //
  // Assign OUT registers.
  // --------------------------------------------------------------------------

  if (outPending) {
    // Must avoid as they have been already OUTed (added during the loop).
    uint32_t outRegs = 0;

    // Must avoid as they collide with already allocated ones.
    uint32_t avoidRegs = willUse & ~clobberedByInst;

    for (i = 0; i < count; i++) {
      RATiedReg* tiedReg = &tiedRegs[i];
      if (!tiedReg->isOut()) continue;

      uint32_t workId = tiedReg->getWorkId();
      uint32_t assignedId = _assignment.workToPhysId(group, workId);

      if (assignedId != RAAssignment::kPhysNone)
        ASMJIT_PROPAGATE(onKillReg(group, workId, assignedId));

      uint32_t physId = tiedReg->getOutId();
      if (physId == RAAssignment::kPhysNone) {
        uint32_t liveRegs = _assignment.getAssigned(group);
        uint32_t allocableRegs = _availableRegs[group] & ~(outRegs | avoidRegs);

        if (!(allocableRegs & ~liveRegs)) {
          // There are no more registers, decide which one to spill.
          uint32_t spillWorkId;
          physId = decideOnBestSpill(group, allocableRegs & liveRegs, &spillWorkId);
          ASMJIT_PROPAGATE(onSpillReg(group, spillWorkId, physId));
        }
        else {
          physId = decideOnAssignment(group, workId, RAAssignment::kPhysNone, allocableRegs & ~liveRegs);
        }
      }

      ASMJIT_ASSERT(!(_assignment.getAssigned(group) & IntUtils::mask(physId))); // OUTs are CLOBBERed.
      ASMJIT_PROPAGATE(onAssignReg(group, workId, physId, true));

      tiedReg->setOutId(physId);
      tiedReg->markOutDone();

      outRegs |= IntUtils::mask(physId);
      outPending--;
    }

    clobberedByInst |= outRegs;
    ASMJIT_ASSERT(outPending == 0);
  }

  _clobberedRegs[group] |= clobberedByInst;
  return kErrorOk;
}

// ============================================================================
// [asmjit::RALocalAllocator - Decision Making]
// ============================================================================

uint32_t RALocalAllocator::decideOnAssignment(uint32_t group, uint32_t workId, uint32_t physId, uint32_t allocableRegs) const noexcept {
  ASMJIT_UNUSED(physId);
  ASMJIT_ASSERT(allocableRegs != 0);

  RAWorkReg* workReg = getWorkReg(workId);

  // Home id is at the moment the highest priority.
  if (workReg->hasHomeId()) {
    uint32_t homeId = workReg->getHomeId();
    uint32_t homeMask = IntUtils::mask(homeId);

    if (allocableRegs & homeMask)
      return homeId;
  }

  // TODO: This is not finished.
  return IntUtils::ctz(allocableRegs);
}

uint32_t RALocalAllocator::decideOnUnassignment(uint32_t group, uint32_t workId, uint32_t physId, uint32_t allocableRegs) const noexcept {
  ASMJIT_ASSERT(allocableRegs != 0);

  // TODO:
  // if (!_assignment.isPhysDirty(group, physId)) {
  // }

  // Decided to SPILL.
  return RAAssignment::kPhysNone;
}

uint32_t RALocalAllocator::decideOnBestSpill(uint32_t group, uint32_t spillableRegs, uint32_t* outWorkId) const noexcept {
  IntUtils::BitWordIterator<uint32_t> it(spillableRegs);

  uint32_t bestPhysId = it.next();
  uint32_t bestWorkId = _assignment.physToWorkId(group, bestPhysId);

  // Avoid calculating the cost model if there is only one spillable register.
  if (it.hasNext()) {
    uint32_t bestCost = calculateSpillCost(group, bestWorkId, bestPhysId);
    do {
      uint32_t physId = it.next();
      uint32_t workId = _assignment.physToWorkId(group, physId);
      uint32_t cost = calculateSpillCost(group, workId, physId);

      if (cost < bestCost) {
        bestCost = cost;
        bestPhysId = physId;
        bestWorkId = workId;
      }
    } while (it.hasNext());
  }

  *outWorkId = bestWorkId;
  return bestPhysId;
}

#if 0
ASMJIT_INLINE void X86CallAlloc::allocImmsOnStack() {
  CCFuncCall* node = getNode();
  FuncDetail& fd = node->getDetail();

  uint32_t argCount = fd.getArgCount();
  Operand_* args = node->_args;

  for (uint32_t i = 0; i < argCount; i++) {
    Operand_& op = args[i];
    if (!op.isImm()) continue;

    const Imm& imm = static_cast<const Imm&>(op);
    const FuncDetail::Value& arg = fd.getArg(i);
    uint32_t varType = arg.getTypeId();

    if (arg.isReg()) {
      _context->emitImmToReg(varType, arg.getRegId(), &imm);
    }
    else {
      X86Mem dst = x86::ptr(_context->_zsp, -static_cast<int>(_context->getGpSize()) + arg.getStackOffset());
      _context->emitImmToStack(varType, &dst, &imm);
    }
  }
}

template<int C>
ASMJIT_INLINE void X86CallAlloc::duplicate() {
  TiedReg* tiedRegs = getTiedRegsByGroup(C);
  uint32_t tiedCount = getTiedCountByGroup(C);

  for (uint32_t i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedRegs[i];
    if ((tied->flags & TiedReg::kRReg) == 0) continue;

    uint32_t inRegs = tied->inRegs;
    if (!inRegs) continue;

    VirtReg* vreg = tied->vreg;
    uint32_t physId = vreg->getPhysId();

    ASMJIT_ASSERT(physId != Reg::kIdBad);

    inRegs &= ~IntUtils::mask(physId);
    if (!inRegs) continue;

    for (uint32_t dupIndex = 0; inRegs != 0; dupIndex++, inRegs >>= 1) {
      if (inRegs & 0x1) {
        _context->emitMove(vreg, dupIndex, physId, "Duplicate");
        _context->_clobberedRegs.or_(C, IntUtils::mask(dupIndex));
      }
    }
  }
}

template<int C>
ASMJIT_INLINE uint32_t X86CallAlloc::decideToAlloc(VirtReg* vreg, uint32_t allocableRegs) {
  ASMJIT_ASSERT(allocableRegs != 0);

  // Stop now if there is only one bit (register) set in 'allocableRegs' mask.
  if (IntUtils::isPowerOf2(allocableRegs))
    return allocableRegs;

  uint32_t i;
  uint32_t safeRegs = allocableRegs;
  uint32_t maxLookAhead = kCompilerDefaultLookAhead;

  // Look ahead and calculate mask of special registers on both - input/output.
  CBNode* node = _node;
  for (i = 0; i < maxLookAhead; i++) {
    // Stop on `CCFuncRet` and `CBSentinel`.
    if (node->hasFlag(CBNode::kFlagIsRet))
      break;

    // Stop on conditional jump, we don't follow them.
    if (node->hasFlag(CBNode::kFlagIsJcc))
      break;

    // Advance on non-conditional jump.
    if (node->hasFlag(CBNode::kFlagIsJmp)) {
      node = static_cast<CBJump*>(node)->getTarget();
      // Stop on jump that is not followed.
      if (!node) break;
    }

    node = node->getNext();
    ASMJIT_ASSERT(node != nullptr);

    X86RAData* raData = node->getPassData<X86RAData>();
    if (raData) {
      TiedReg* tied = raData->findTiedByGroup(C, vreg);
      if (tied) {
        uint32_t inRegs = tied->inRegs;
        if (inRegs != 0) {
          safeRegs = allocableRegs;
          allocableRegs &= inRegs;

          if (allocableRegs == 0)
            goto _UseSafeRegs;
          else
            return allocableRegs;
        }
      }

      safeRegs = allocableRegs;
      allocableRegs &= ~(raData->inRegs.get(C) | raData->outRegs.get(C) | raData->clobberedRegs.get(C));

      if (allocableRegs == 0)
        break;
    }
  }

_UseSafeRegs:
  return safeRegs;
}

template<int C>
ASMJIT_INLINE void X86CallAlloc::save() {
  X86RAState* state = getState();
  VirtReg** sVars = state->getListByGroup(C);

  uint32_t i;
  uint32_t affected = _raData->clobberedRegs.get(C) & state->_occupied.get(C) & state->_modified.get(C);

  for (i = 0; affected != 0; i++, affected >>= 1) {
    if (affected & 0x1) {
      VirtReg* vreg = sVars[i];
      ASMJIT_ASSERT(vreg != nullptr);
      ASMJIT_ASSERT(vreg->isModified());

      TiedReg* tied = vreg->_tied;
      if (!tied || (tied->flags & (TiedReg::kWReg | TiedReg::kUnuse)) == 0)
        _context->save<C>(vreg);
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc - Ret]
// ============================================================================

ASMJIT_INLINE void X86CallAlloc::ret() {
  CCFuncCall* node = getNode();
  FuncDetail& fd = node->getDetail();
  Operand_* rets = node->_ret;

  for (uint32_t i = 0; i < 2; i++) {
    const FuncDetail::Value& ret = fd.getRet(i);
    Operand_* op = &rets[i];

    if (!ret.isReg() || !op->isVirtReg())
      continue;

    VirtReg* vreg = _cc->getVirtRegById(op->getId());
    uint32_t regId = ret.getRegId();

    switch (vreg->getGroup()) {
      case X86Reg::kGroupGp:
        _context->unuse<X86Reg::kGroupGp>(vreg);
        _context->attach<X86Reg::kGroupGp>(vreg, regId, true);
        break;

      case X86Reg::kGroupMm:
        _context->unuse<X86Reg::kGroupMm>(vreg);
        _context->attach<X86Reg::kGroupMm>(vreg, regId, true);
        break;

      case X86Reg::kGroupVec:
        if (X86Reg::groupOf(ret.getRegType()) == X86Reg::kGroupVec) {
          _context->unuse<X86Reg::kGroupVec>(vreg);
          _context->attach<X86Reg::kGroupVec>(vreg, regId, true);
        }
        else {
          uint32_t elementId = TypeId::elementOf(vreg->getTypeId());
          uint32_t size = (elementId == TypeId::kF32) ? 4 : 8;

          X86Mem m = _context->getVarMem(vreg);
          m.setSize(size);

          _context->unuse<X86Reg::kGroupVec>(vreg, VirtReg::kStateMem);
          _cc->fstp(m);
        }
        break;
    }
  }
}

// ============================================================================
// [asmjit::X86RAPass - Translate - Jump]
// ============================================================================

//! \internal
static void X86RAPass_translateJump(X86RAPass* self, CBJump* jNode, CBLabel* jTarget) {
  X86Compiler* cc = self->cc();

  CBNode* injectRef = self->getFunc()->getEnd()->getPrev();
  CBNode* prevCursor = cc->setCursor(injectRef);

  self->switchState(jTarget->getPassData<RAData>()->state);

  // Any code necessary to `switchState()` will be added at the end of the function.
  if (cc->getCursor() != injectRef) {
    // TODO: Can fail.
    CBLabel* injectLabel = cc->newLabelNode();

    // Add the jump to the target.
    cc->jmp(jTarget->getLabel());

    // Inject the label.
    cc->_setCursor(injectRef);
    cc->addNode(injectLabel);

    // Finally, patch `jNode` target.
    ASMJIT_ASSERT(jNode->getOpCount() > 0);
    jNode->_opArray[jNode->getOpCount() - 1] = injectLabel->getLabel();
    jNode->_target = injectLabel;
    // If we injected any code it may not satisfy short form anymore.
    jNode->delOptions(X86Inst::kOptionShortForm);
  }

  cc->_setCursor(prevCursor);
  self->loadState(jNode->getPassData<RAData>()->state);
}

// ============================================================================
// [asmjit::X86RAPass - Translate - Ret]
// ============================================================================

static Error X86RAPass_translateRet(X86RAPass* self, CCFuncRet* rNode, CBLabel* exitTarget) {
  X86Compiler* cc = self->cc();
  CBNode* node = rNode->getNext();

  // 32-bit mode requires to push floating point return value(s), handle it
  // here as it's a special case.
  X86RAData* raData = rNode->getPassData<X86RAData>();
  if (raData) {
    TiedReg* tiedRegs = raData->tiedRegs;
    uint32_t tiedTotal = raData->tiedTotal;

    for (uint32_t i = 0; i < tiedTotal; i++) {
      TiedReg* tied = &tiedRegs[i];
      if (tied->flags & (TiedReg::kX86Fld4 | TiedReg::kX86Fld8)) {
        VirtReg* vreg = tied->vreg;
        X86Mem m(self->getVarMem(vreg));

        uint32_t elementId = TypeId::elementOf(vreg->getTypeId());
        m.setSize(elementId == TypeId::kF32 ? 4 :
                  elementId == TypeId::kF64 ? 8 :
                  (tied->flags & TiedReg::kX86Fld4) ? 4 : 8);

        cc->fld(m);
      }
    }
  }

  // Decide whether to `jmp` or not in case we are next to the return label.
  while (node) {
    switch (node->getType()) {
      // If we have found an exit label we just return, there is no need to
      // emit jump to that.
      case CBNode::kNodeLabel:
        if (static_cast<CBLabel*>(node) == exitTarget)
          return kErrorOk;
        goto _EmitRet;

      case CBNode::kNodeData:
      case CBNode::kNodeInst:
      case CBNode::kNodeFuncCall:
      case CBNode::kNodeFuncExit:
        goto _EmitRet;

      // Continue iterating.
      case CBNode::kNodeComment:
      case CBNode::kNodeAlign:
      case CBNode::kNodeHint:
        break;

      // Invalid node to be here.
      case CBNode::kNodeFunc:
        return DebugUtils::errored(kErrorInvalidState);

      // We can't go forward from here.
      case CBNode::kNodeSentinel:
        return kErrorOk;
    }

    node = node->getNext();
  }

_EmitRet:
  {
    cc->_setCursor(rNode);
    cc->jmp(exitTarget->getLabel());
  }
  return kErrorOk;
}
#endif

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
