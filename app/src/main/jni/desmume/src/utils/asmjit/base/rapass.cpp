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
#include "../base/rapass_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::RABlock - Control Flow]
// ============================================================================

Error RABlock::appendSuccessor(RABlock* successor) noexcept {
  RABlock* predecessor = this;

  if (predecessor->_successors.contains(successor))
    return kErrorOk;
  ASMJIT_ASSERT(!successor->_predecessors.contains(predecessor));

  ZoneAllocator* allocator = getAllocator();
  ASMJIT_PROPAGATE(successor->_predecessors.willGrow(allocator));
  ASMJIT_PROPAGATE(predecessor->_successors.willGrow(allocator));

  predecessor->_successors.appendUnsafe(successor);
  successor->_predecessors.appendUnsafe(predecessor);

  return kErrorOk;
}

Error RABlock::prependSuccessor(RABlock* successor) noexcept {
  RABlock* predecessor = this;

  if (predecessor->_successors.contains(successor))
    return kErrorOk;
  ASMJIT_ASSERT(!successor->_predecessors.contains(predecessor));

  ZoneAllocator* allocator = getAllocator();
  ASMJIT_PROPAGATE(successor->_predecessors.willGrow(allocator));
  ASMJIT_PROPAGATE(predecessor->_successors.willGrow(allocator));

  predecessor->_successors.prependUnsafe(successor);
  successor->_predecessors.prependUnsafe(predecessor);

  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - Construction / Destruction]
// ============================================================================

RAPass::RAPass() noexcept
  : CCFuncPass("RAPass"),
    _allocator(),
    _logger(nullptr),
    _loggerOptions(0),
    _func(nullptr),
    _stop(nullptr),
    _extraBlock(nullptr),
    _blocks(),
    _exits(),
    _pov(),
    _instructionCount(0),
    _createdBlockCount(0),
    _lastTimestamp(0),
    _archTraits(),
    _physRegIndex(),
    _physRegCount(),
    _physRegTotal(0),
    _availableRegs(),
    _availableRegCount(),
    _clobberedRegs(),
    _sp(),
    _fp(),
    _stackAllocator(),
    _argsAssignment(),
    _maxWorkRegNameLength(0) {}
RAPass::~RAPass() noexcept {}

// ============================================================================
// [asmjit::RAPass - RunOnFunction]
// ============================================================================

static void RAPass_reset(RAPass* self, FuncDetail* funcDetail) noexcept {
  self->_blocks.reset();
  self->_exits.reset();
  self->_pov.reset();
  self->_workRegs.reset();
  self->_instructionCount = 0;
  self->_createdBlockCount = 0;
  self->_lastTimestamp = 0;

  self->_archTraits.reset();
  self->_physRegIndex.reset();
  self->_physRegCount.reset();
  self->_physRegTotal = 0;

  self->_availableRegs.reset();
  self->_availableRegCount.reset();
  self->_clobberedRegs.reset();

  self->_workRegs.reset();
  for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
    self->_workRegsOfGroup[group].reset();

  self->_stackAllocator.reset(self->getAllocator());
  self->_argsAssignment.reset(funcDetail);
  self->_maxWorkRegNameLength = 0;
}

static void RAPass_resetVirtRegData(RAPass* self) noexcept {
  RAWorkRegs& workRegs = self->_workRegs;
  uint32_t count = workRegs.getLength();

  for (uint32_t i = 0; i < count; i++) {
    RAWorkReg* wReg = workRegs[i];
    VirtReg* vReg = wReg->getVirtReg();

    // Zero everything so it cannot be used by accident.
    vReg->_workReg = nullptr;
  }
}

Error RAPass::runOnFunction(Zone* zone, Logger* logger, CCFunc* func) noexcept {
  _allocator.reset(zone);

#if !defined(ASMJIT_DISABLE_LOGGING)
  _logger = logger;
  if (logger)
    _loggerOptions = logger->getOptions();
#endif

  // Initialize all core structures to use `zone` and `func`.
  CBNode* end = func->getEnd();
  _func = func;
  _stop = end->getNext();
  _extraBlock = end;
  RAPass_reset(this, &_func->_funcDetail);

  onInit();                        // Initialize architecture-specific members.
  Error err = onPerformAllSteps(); // Perform all allocation steps required.
  onDone();                        // Must be called regardless of the allocation status.

  // TODO: I don't like this...
  RAPass_resetVirtRegData(this);   // Reset possible connections introduced by the register allocator.

  // Reset all core structures and everything that depends on the passed `Zone`.
  RAPass_reset(this, nullptr);
  _allocator.reset(nullptr);

#if !defined(ASMJIT_DISABLE_LOGGING)
  _logger = nullptr;
  _loggerOptions = 0;
#endif

  _func = nullptr;
  _stop = nullptr;
  _extraBlock = nullptr;

  // Reset `Zone` as nothing should persist between `runOnFunction()` calls.
  zone->reset();

  // We alter the compiler cursor, because it doesn't make sense to reference
  // it after the compilation - some nodes may disappear and the old cursor
  // can go out anyway.
  cc()->_setCursor(cc()->getLastNode());

  return err;
}

Error RAPass::onPerformAllSteps() noexcept {
  ASMJIT_PROPAGATE(onBuildCFG());
  ASMJIT_PROPAGATE(buildViews());
  ASMJIT_PROPAGATE(removeUnreachableBlocks());

  ASMJIT_PROPAGATE(buildDominators());
  ASMJIT_PROPAGATE(buildLiveness());

#if !defined(ASMJIT_DISABLE_LOGGING)
  if (hasLogger() && getLogger()->hasOption(Logger::kOptionAnnotate))
    ASMJIT_PROPAGATE(annotateCode());
#endif

  ASMJIT_PROPAGATE(runGlobalAllocator());
  ASMJIT_PROPAGATE(runLocalAllocator());

  ASMJIT_PROPAGATE(updateStackFrame());
  ASMJIT_PROPAGATE(insertPrologEpilog());

  ASMJIT_PROPAGATE(rewrite());

  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - CFG - Basic Block Management]
// ============================================================================

RABlock* RAPass::newBlock(CBNode* initialNode) noexcept {
  RABlock* block = getZone()->allocT<RABlock>();
  if (ASMJIT_UNLIKELY(!block)) return nullptr;

  block = new(block) RABlock(this);
  block->setFirst(initialNode);
  block->setLast(initialNode);

  _createdBlockCount++;
  return block;
}

RABlock* RAPass::newBlockOrExistingAt(CBLabel* cbLabel) noexcept {
  if (cbLabel->hasPassData())
    return cbLabel->getPassData<RABlock>();

  CBNode* node = cbLabel->getPrev();
  RABlock* block = nullptr;

  // Try to find some label, but terminate the loop on any code. We try hard to
  // coalesce code that contains two consecutive labels or a combination of
  // non-code nodes between 2 or more labels.
  //
  // Possible cases that would share the same basic block:
  //
  //   1. Two or more consecutive labels:
  //     Label1:
  //     Label2:
  //
  //   2. Two or more labels separated by non-code nodes:
  //     Label1:
  //     ; Some comment...
  //     .align 16
  //     Label2:
  uint32_t exitLabelId = getFunc()->getExitNode()->getId();
  size_t nPendingLabels = 0;

  while (node) {
    if (node->getType() == CBNode::kNodeLabel) {
      if (node->as<CBLabel>()->getId() == exitLabelId)
        break;

      block = node->getPassData<RABlock>();
      if (block)
        break;

      nPendingLabels++;
    }
    else if (node->getType() == CBNode::kNodeAlign) {
      // Align node is fine.
    }
    else {
      break;
    }

    node = node->getPrev();
  }

  if (!block) {
    block = newBlock();
    if (ASMJIT_UNLIKELY(!block))
      return nullptr;
  }

  cbLabel->setPassData<RABlock>(block);
  node = cbLabel;

  while (nPendingLabels) {
    node = node->getPrev();
    for (;;) {
      if (node->getType() == CBNode::kNodeLabel) {
        node->setPassData<RABlock>(block);
        nPendingLabels--;
        break;
      }

      node = node->getPrev();
      ASMJIT_ASSERT(node != nullptr);
    }
  }

  if (!block->getFirst()) {
    block->setFirst(node);
    block->setLast(cbLabel);
  }

  return block;
}

Error RAPass::addBlock(RABlock* block) noexcept {
  ASMJIT_PROPAGATE(_blocks.willGrow(getAllocator()));

  block->_blockId = getBlockCount();
  _blocks.appendUnsafe(block);
  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - CFG - Views Order]
// ============================================================================

class RABlockVisitItem {
public:
  ASMJIT_INLINE RABlockVisitItem(RABlock* block, uint32_t index) noexcept
    : _block(block),
      _index(index) {}

  ASMJIT_INLINE RABlockVisitItem(const RABlockVisitItem& other) noexcept
    : _block(other._block),
      _index(other._index) {}

  ASMJIT_INLINE RABlock* getBlock() const noexcept { return _block; }
  ASMJIT_INLINE uint32_t getIndex() const noexcept { return _index; }

  RABlock* _block;
  uint32_t _index;
};

Error RAPass::buildViews() noexcept {
  ASMJIT_RA_LOG_INIT(getLogger());
  ASMJIT_RA_LOG_FORMAT("[RAPass::BuildViews]\n");

  uint32_t count = getBlockCount();
  if (ASMJIT_UNLIKELY(!count)) return kErrorOk;

  ASMJIT_PROPAGATE(_pov.reserve(getAllocator(), count));

  ZoneStack<RABlockVisitItem> stack;
  ASMJIT_PROPAGATE(stack.init(getAllocator()));

  ZoneBitVector visited;
  ASMJIT_PROPAGATE(visited.resize(getAllocator(), count));

  RABlock* current = _blocks[0];
  uint32_t i = 0;

  for (;;) {
    for (;;) {
      if (i >= current->getSuccessors().getLength())
        break;

      // Skip if already visited.
      RABlock* child = current->getSuccessors().getAt(i++);
      if (visited.getAt(child->getBlockId()))
        continue;

      // Mark as visited to prevent visiting the same block multiple times.
      visited.setAt(child->getBlockId(), true);

      // Add the current block on the stack, we will get back to it later.
      ASMJIT_PROPAGATE(stack.append(RABlockVisitItem(current, i)));
      current = child;
      i = 0;
    }

    current->makeReachable();
    current->_povOrder = _pov.getLength();
    _pov.appendUnsafe(current);

    if (stack.isEmpty())
      break;

    RABlockVisitItem top = stack.pop();
    current = top.getBlock();
    i = top.getIndex();
  }

  visited.release(getAllocator());
  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - CFG - Dominators]
// ============================================================================

static ASMJIT_INLINE RABlock* intersectBlocks(RABlock* b1, RABlock* b2) noexcept {
  while (b1 != b2) {
    while (b2->getPovOrder() > b1->getPovOrder()) b1 = b1->getIDom();
    while (b1->getPovOrder() > b2->getPovOrder()) b2 = b2->getIDom();
  }
  return b1;
}

Error RAPass::buildDominators() noexcept {
  // Based on "A Simple, Fast Dominance Algorithm".
  ASMJIT_RA_LOG_INIT(getLogger());
  ASMJIT_RA_LOG_FORMAT("[RAPass::BuildDominators]\n");

  if (_blocks.isEmpty())
    return kErrorOk;

  RABlock* entryBlock = getEntryBlock();
  entryBlock->setIDom(entryBlock);

  bool changed = true;
  uint32_t nIters = 0;

  while (changed) {
    nIters++;
    changed = false;

    uint32_t i = _pov.getLength();
    while (i) {
      RABlock* block = _pov[--i];
      if (block == entryBlock)
        continue;

      RABlock* iDom = nullptr;
      const RABlocks& preds = block->getPredecessors();

      uint32_t j = preds.getLength();
      while (j) {
        RABlock* p = preds[--j];
        if (!p->hasIDom())
          continue;
        iDom = !iDom ? p : intersectBlocks(iDom, p);
      }

      if (block->getIDom() != iDom) {
        ASMJIT_RA_LOG_FORMAT("  IDom of #%u -> #%u\n", block->getBlockId(), iDom->getBlockId());
        block->setIDom(iDom);
        changed = true;
      }
    }
  }

  ASMJIT_RA_LOG_FORMAT("  Done (%u iterations)\n", static_cast<unsigned int>(nIters));
  return kErrorOk;
}

bool RAPass::_strictlyDominates(const RABlock* a, const RABlock* b) const noexcept {
  ASMJIT_ASSERT(a != nullptr); // There must be at least one block if this function is
  ASMJIT_ASSERT(b != nullptr); // called, as both `a` and `b` must be valid blocks.
  ASMJIT_ASSERT(a != b);       // Checked by `dominates()` and `strictlyDominates()`.

  // Nothing strictly dominates the entry block.
  const RABlock* entryBlock = getEntryBlock();
  if (a == entryBlock)
    return false;

  const RABlock* iDom = b->getIDom();
  while (iDom != a && iDom != entryBlock)
    iDom = iDom->getIDom();

  return iDom != entryBlock;
}

const RABlock* RAPass::_nearestCommonDominator(const RABlock* a, const RABlock* b) const noexcept {
  ASMJIT_ASSERT(a != nullptr); // There must be at least one block if this function is
  ASMJIT_ASSERT(b != nullptr); // called, as both `a` and `b` must be valid blocks.
  ASMJIT_ASSERT(a != b);       // Checked by `dominates()` and `properlyDominates()`.

  if (a == b)
    return a;

  // If `a` strictly dominates `b` then `a` is the nearest common dominator.
  if (_strictlyDominates(a, b))
    return a;

  // If `b` strictly dominates `a` then `b` is the nearest common dominator.
  if (_strictlyDominates(b, a))
    return b;

  const RABlock* entryBlock = getEntryBlock();
  uint64_t timestamp = nextTimestamp();

  // Mark all A's dominators.
  const RABlock* block = a->getIDom();
  while (block != entryBlock) {
    block->setTimestamp(timestamp);
    block = block->getIDom();
  }

  // Check all B's dominators against marked dominators of A.
  block = b->getIDom();
  while (block != entryBlock) {
    if (block->hasTimestamp(timestamp))
      return block;
    block = block->getIDom();
  }

  return entryBlock;
}

// ============================================================================
// [asmjit::RAPass - CFG - Utilities]
// ============================================================================

Error RAPass::removeUnreachableBlocks() noexcept {
  uint32_t numAllBlocks = getBlockCount();
  uint32_t numReachableBlocks = getReachableBlockCount();

  // All reachable -> nothing to do.
  if (numAllBlocks == numReachableBlocks)
    return kErrorOk;

  ASMJIT_RA_LOG_INIT(getLogger());
  ASMJIT_RA_LOG_FORMAT("[RAPass::RemoveUnreachableBlocks (%u of %u unreachable)]\n", numAllBlocks - numReachableBlocks, numAllBlocks);

  for (uint32_t i = 0; i < numAllBlocks; i++) {
    RABlock* block = _blocks[i];
    if (block->isReachable())
      continue;

    ASMJIT_RA_LOG_FORMAT("  Removing block {%u}\n", i);
    CBNode* first = block->getFirst();
    CBNode* last = block->getLast();

    CBNode* beforeFirst = first->getPrev();
    CBNode* afterLast = last->getNext();

    // Skip labels as they can be used as reference points.
    while (first->actsAsLabel() && first != afterLast)
      first = first->getNext();

    // Just to control flow.
    for (;;) {
      if (first == afterLast)
        break;

      // Don't know a better way atm, .align nodes before labels should be preserved.
      if (last->getType() == CBNode::kNodeAlign) {
        if (first == last)
          break;
        last = last->getPrev();
      }

      bool wholeBlockGone = (first == block->getFirst() && last == block->getLast());
      cc()->removeNodes(first, last);

      if (wholeBlockGone) {
        block->setFirst(nullptr);
        block->setLast(nullptr);
      }
      else {
        block->setFirst(beforeFirst->getNext());
        block->setLast(afterLast->getPrev());
      }
      break;
    }
  }

  return kErrorOk;
}

CBNode* RAPass::findSuccessorStartingAt(CBNode* node) noexcept {
  while (node && (node->isInformative() || node->hasNoEffect()))
    node = node->getNext();
  return node;
}

bool RAPass::isNextTo(CBNode* node, CBNode* target) noexcept {
  for (;;) {
    node = node->getNext();
    if (node == target)
      return true;

    if (!node)
      return false;

    if (node->isCode() || node->isData())
      return false;
  }
}

// ============================================================================
// [asmjit::RAPass - ]
// ============================================================================

Error RAPass::_asWorkReg(VirtReg* vReg, RAWorkReg** out) noexcept {
  // Checked by `asWorkReg()` - must be true.
  ASMJIT_ASSERT(vReg->_workReg == nullptr);

  uint32_t group = vReg->getGroup();
  ASMJIT_ASSERT(group < Reg::kGroupVirt);

  RAWorkRegs& workRegs = getWorkRegs();
  RAWorkRegs& workRegsByGroup = getWorkRegs(group);

  ASMJIT_PROPAGATE(workRegs.willGrow(getAllocator()));
  ASMJIT_PROPAGATE(workRegsByGroup.willGrow(getAllocator()));

  RAWorkReg* wReg = getZone()->allocT<RAWorkReg>();
  if (ASMJIT_UNLIKELY(!wReg))
    return DebugUtils::errored(kErrorNoHeapMemory);

  wReg = new(wReg) RAWorkReg(vReg, workRegs.getLength());
  vReg->setWorkReg(wReg);

  workRegs.appendUnsafe(wReg);
  workRegsByGroup.appendUnsafe(wReg);

  // Only used by RA logging.
  _maxWorkRegNameLength = std::max(_maxWorkRegNameLength, vReg->getNameLength());

  *out = wReg;
  return kErrorOk;
}

RAAssignment::WorkToPhysMap* RAPass::newWorkToPhysMap() noexcept {
  uint32_t count = getWorkRegCount();
  size_t size = WorkToPhysMap::sizeOf(count);

  // If no registers are used it could be zero, in that case return a dummy
  // map instead of NULL.
  if (ASMJIT_UNLIKELY(!size)) {
    static const RAAssignment::WorkToPhysMap nullMap = {{ 0 }};
    return const_cast<RAAssignment::WorkToPhysMap*>(&nullMap);
  }

  WorkToPhysMap* map = getZone()->allocT<WorkToPhysMap>(size);
  if (ASMJIT_UNLIKELY(!map))
    return nullptr;

  map->reset(count);
  return map;
}

RAAssignment::PhysToWorkMap* RAPass::newPhysToWorkMap() noexcept {
  uint32_t count = getPhysRegTotal();
  size_t size = PhysToWorkMap::sizeOf(count);

  PhysToWorkMap* map = getZone()->allocAlignedT<PhysToWorkMap>(size, sizeof(uint32_t));
  if (ASMJIT_UNLIKELY(!map))
    return nullptr;

  map->reset(count);
  return map;
}

// ============================================================================
// [asmjit::RAPass - Registers - Liveness Analysis and Statistics]
// ============================================================================

namespace LiveOps {
  typedef ZoneBitVector::BitWord BitWord;

  struct In {
    static ASMJIT_INLINE BitWord op(BitWord dst, BitWord out, BitWord gen, BitWord kill) noexcept {
      ASMJIT_UNUSED(dst);
      return (out | gen) & ~kill;
    }
  };

  template<typename Operator>
  static ASMJIT_INLINE bool op(BitWord* dst, const BitWord* a, uint32_t n) noexcept {
    BitWord changed = 0;

    for (uint32_t i = 0; i < n; i++) {
      BitWord before = dst[i];
      BitWord after = Operator::op(before, a[i]);

      dst[i] = after;
      changed |= (before ^ after);
    }

    return changed != 0;
  }

  template<typename Operator>
  static ASMJIT_INLINE bool op(BitWord* dst, const BitWord* a, const BitWord* b, uint32_t n) noexcept {
    BitWord changed = 0;

    for (uint32_t i = 0; i < n; i++) {
      BitWord before = dst[i];
      BitWord after = Operator::op(before, a[i], b[i]);

      dst[i] = after;
      changed |= (before ^ after);
    }

    return changed != 0;
  }

  template<typename Operator>
  static ASMJIT_INLINE bool op(BitWord* dst, const BitWord* a, const BitWord* b, const BitWord* c, uint32_t n) noexcept {
    BitWord changed = 0;

    for (uint32_t i = 0; i < n; i++) {
      BitWord before = dst[i];
      BitWord after = Operator::op(before, a[i], b[i], c[i]);

      dst[i] = after;
      changed |= (before ^ after);
    }

    return changed != 0;
  }

  static ASMJIT_INLINE bool recalcInOut(RABlock* block, uint32_t numBitWords, bool initial = false) noexcept {
    bool changed = initial;

    const RABlocks& successors = block->getSuccessors();
    uint32_t numSuccessors = successors.getLength();

    // Calculate `OUT` based on `IN` of all successors.
    for (uint32_t i = 0; i < numSuccessors; i++)
      changed |= op<IntUtils::Or>(block->getLiveOut().getData(), successors[i]->getLiveIn().getData(), numBitWords);

    // Calculate `IN` based on `OUT`, `GEN`, and `KILL` bits.
    if (changed)
      changed = op<In>(block->getLiveIn().getData(), block->getLiveOut().getData(), block->getGen().getData(), block->getKill().getData(), numBitWords);

    return changed;
  }
}

Error RAPass::buildLiveness() noexcept {
  ASMJIT_RA_LOG_INIT(getLogger());
  ASMJIT_RA_LOG_FORMAT("[RAPass::BuildLiveness]\n");

  ZoneAllocator* allocator = getAllocator();
  uint32_t i;

  uint32_t numAllBlocks = getBlockCount();
  uint32_t numReachableBlocks = getReachableBlockCount();

  uint32_t numVisits = numReachableBlocks;
  uint32_t numWorkRegs = getWorkRegCount();
  uint32_t numBitWords = ZoneBitVector::_wordsPerBits(numWorkRegs);

  if (!numWorkRegs) {
    ASMJIT_RA_LOG_FORMAT("  Done (no virtual registers)\n");
    return kErrorOk;
  }

  ZoneVector<uint32_t> nUsesPerWorkReg;
  ZoneVector<uint32_t> nOutsPerWorkReg;

  ASMJIT_PROPAGATE(nUsesPerWorkReg.resize(allocator, numWorkRegs));
  ASMJIT_PROPAGATE(nOutsPerWorkReg.resize(allocator, numWorkRegs));

  // String builder used to format debugging information.
#if !defined(ASMJIT_DISABLE_LOGGING)
  StringBuilderTmp<512> sb;
#endif

  // --------------------------------------------------------------------------
  // Calculate GEN/KILL of each block.
  // --------------------------------------------------------------------------

  {
    for (i = 0; i < numReachableBlocks; i++) {
      RABlock* block = _pov[i];
      ASMJIT_PROPAGATE(block->resizeLiveBits(numWorkRegs));

      CBNode* node = block->getLast();
      CBNode* stop = block->getFirst();

      for (;;) {
        if (node->actsAsInst()) {
          CBInst* cbInst = node->as<CBInst>();
          RAInst* raInst = cbInst->getPassData<RAInst>();
          ASMJIT_ASSERT(raInst != nullptr);

          RATiedReg* tiedRegs = raInst->getTiedRegs();
          uint32_t count = raInst->getTiedCount();

          for (uint32_t j = 0; j < count; j++) {
            RATiedReg* tiedReg = &tiedRegs[j];
            uint32_t workId = tiedReg->getWorkId();

            // Create refs and writes.
            nUsesPerWorkReg[workId] += 1;
            nOutsPerWorkReg[workId] += (tiedReg->flags & RATiedReg::kWrite) != 0;

            // Mark as:
            //   KILL - if this VirtReg is killed afterwards.
            //   LAST - if this VirtReg is last in this basic block.
            if (block->getKill().getAt(workId))
              tiedReg->addFlags(RATiedReg::kKill);
            else if (!block->getGen().getAt(workId))
              tiedReg->addFlags(RATiedReg::kLast);

            if (tiedReg->isWriteOnly()) {
              // KILL.
              block->getKill().setAt(workId, true);
            }
            else {
              // GEN.
              block->getKill().setAt(workId, false);
              block->getGen().setAt(workId, true);
            }
          }
        }

        if (node == stop)
          break;

        node = node->getPrev();
        ASMJIT_ASSERT(node != nullptr);
      }
    }
  }

  // --------------------------------------------------------------------------
  // Calculate IN/OUT of each block.
  // --------------------------------------------------------------------------

  {
    ZoneStack<RABlock*> workList;
    ZoneBitVector workBits;

    ASMJIT_PROPAGATE(workList.init(allocator));
    ASMJIT_PROPAGATE(workBits.resize(allocator, getBlockCount(), true));

    for (i = 0; i < numReachableBlocks; i++) {
      RABlock* block = _pov[i];
      LiveOps::recalcInOut(block, numBitWords, true);
      ASMJIT_PROPAGATE(workList.append(block));
    }

    while (!workList.isEmpty()) {
      RABlock* block = workList.popFirst();
      uint32_t blockId = block->getBlockId();

      workBits.setAt(blockId, false);
      if (LiveOps::recalcInOut(block, numBitWords)) {
        const RABlocks& predecessors = block->getPredecessors();
        uint32_t numPredecessors = predecessors.getLength();

        for (uint32_t j = 0; j < numPredecessors; j++) {
          RABlock* pred = predecessors[j];
          if (!workBits.getAt(pred->getBlockId())) {
            workBits.setAt(pred->getBlockId(), true);
            ASMJIT_PROPAGATE(workList.append(pred));
          }
        }
      }
      numVisits++;
    }

    workList.reset();
    workBits.release(allocator);
  }

  ASMJIT_RA_LOG_COMPLEX({
    StringBuilderTmp<512> sb;
    logger->logf("  LiveIn/Out Done (%u visits)\n", static_cast<unsigned int>(numVisits));

    for (i = 0; i < numAllBlocks; i++) {
      RABlock* block = _blocks[i];

      ASMJIT_PROPAGATE(sb.setFormat("  {#%u}\n", static_cast<unsigned int>(block->getBlockId())));
      ASMJIT_PROPAGATE(_dumpBlockLiveness(sb, block));

      logger->log(sb);
    }
  });

  // --------------------------------------------------------------------------
  // Reserve the space in each `RAWorkReg` for references.
  // --------------------------------------------------------------------------

  for (i = 0; i < numWorkRegs; i++) {
    RAWorkReg* workReg = getWorkReg(i);

    ASMJIT_PROPAGATE(workReg->_refs.reserve(allocator, nUsesPerWorkReg[i]));
    ASMJIT_PROPAGATE(workReg->_writes.reserve(allocator, nOutsPerWorkReg[i]));
  }

  // --------------------------------------------------------------------------
  // This calculation expects the following:
  //   1. Blocks are ordered based on their start positions (CFG builder does
  //      this with no cost).
  //   2. Blocks must contain proper LIVE-IN information, and each `RATiedReg`
  //      must contain Kill/Last.
  // --------------------------------------------------------------------------

  for (i = 0; i < numAllBlocks; i++) {
    RABlock* block = _blocks[i];
    if (!block->isReachable())
      continue;

    CBNode* node = block->getFirst();
    CBNode* stop = block->getLast();

    uint32_t curPosition = block->getFirstPosition();
    uint32_t endPosition = block->getEndPosition() + 2;

    RALiveCount curLiveCount;
    RALiveCount maxLiveCount;

    // Process LIVE-IN.
    ZoneBitVector::ForEachBitSet it(block->getLiveIn());
    while (it.hasNext()) {
      RAWorkReg* workReg = _workRegs[it.next()];
      curLiveCount[workReg->getGroup()]++;
      ASMJIT_PROPAGATE(workReg->getLiveSpans().openAt(allocator, curPosition, endPosition));
    }

    for (;;) {
      if (node->actsAsInst()) {
        CBInst* cbInst = node->as<CBInst>();
        RAInst* raInst = cbInst->getPassData<RAInst>();
        ASMJIT_ASSERT(raInst != nullptr);

        RATiedReg* tiedRegs = raInst->getTiedRegs();
        uint32_t count = raInst->getTiedCount();

        curPosition = cbInst->getPosition();
        raInst->_liveCount = curLiveCount;

        for (uint32_t j = 0; j < count; j++) {
          RATiedReg* tiedReg = &tiedRegs[j];
          uint32_t workId = tiedReg->getWorkId();

          // Create refs and writes.
          RAWorkReg* workReg = getWorkReg(workId);
          workReg->_refs.appendUnsafe(node);
          if (tiedReg->flags & RATiedReg::kWrite)
            workReg->_writes.appendUnsafe(node);

          // We couldn't calculate this in previous steps, but since we know all LIVE-OUT
          // at this point it becomes trivial. If this is the last instruction that uses
          // this `workReg` and it's not LIVE-OUT then it is KILLed here.
          if (tiedReg->isLast() && !block->getLiveOut().getAt(workId))
            tiedReg->addFlags(RATiedReg::kKill);

          LiveRegSpans& liveSpans = workReg->getLiveSpans();
          bool wasOpen;
          ASMJIT_PROPAGATE(liveSpans.openAt(allocator, curPosition + !tiedReg->isRead(), endPosition, wasOpen));

          uint32_t group = workReg->getGroup();
          if (!wasOpen) {
            curLiveCount[group]++;
            raInst->_liveCount[group]++;
          }

          if (tiedReg->isKill()) {
            liveSpans.closeAt(curPosition + !tiedReg->isRead() + 1);
            curLiveCount[group]--;
          }
        }

        maxLiveCount.op<IntUtils::Max>(raInst->_liveCount);
      }

      if (node == stop)
        break;

      node = node->getNext();
      ASMJIT_ASSERT(node != nullptr);
    }

    block->_maxLiveCount = maxLiveCount;
  }

  // --------------------------------------------------------------------------
  // Calculate WorkReg statistics.
  // --------------------------------------------------------------------------

  for (i = 0; i < numWorkRegs; i++) {
    RAWorkReg* workReg = _workRegs[i];

    LiveRegSpans& spans = workReg->getLiveSpans();
    uint32_t width = spans.calcWidth();
    float freq = static_cast<float>(double(workReg->_refs.getLength()) / double(width));

    RALiveStats& stats = workReg->getLiveStats();
    stats._width = width;
    stats._freq = freq;
  }

  ASMJIT_RA_LOG_COMPLEX({
    StringBuilderTmp<1024> sb;
    _dumpLiveSpans(sb);
    logger->log(sb);
  });

  nUsesPerWorkReg.release(allocator);
  nOutsPerWorkReg.release(allocator);

  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - Allocation - Global]
// ============================================================================

struct CompareByFrequency : public AsmJitInternal::CompareByValue<CompareByFrequency, false> {
  static ASMJIT_INLINE float getValue(const RAWorkReg* item) noexcept { return item->getLiveStats().getFreq(); }
};

Error RAPass::runGlobalAllocator() noexcept {
  for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
    binPack(group);

  return kErrorOk;
}

static void dumpSpans(StringBuilder& sb, uint32_t index, const LiveRegSpans& liveSpans) {
  sb.appendFormat("  %02u: ", index);

  for (uint32_t i = 0; i < liveSpans.getLength(); i++) {
    const LiveRegSpan& liveSpan = liveSpans[i];
    if (i) sb.appendString(", ");
    sb.appendFormat("[%u:%u@%u]", liveSpan.a, liveSpan.b, liveSpan.id);
  }

  sb.appendChar('\n');
}

Error RAPass::binPack(uint32_t group) noexcept {
  if (getWorkRegCount(group) == 0)
    return kErrorOk;

  ASMJIT_RA_LOG_INIT(getLogger());
  ASMJIT_RA_LOG_FORMAT("[RAPass::BinPack] Available=%u (0x%08X) Count=%u\n",
    static_cast<unsigned int>(IntUtils::popcnt(_availableRegs[group])),
    static_cast<unsigned int>(_availableRegs[group]),
    static_cast<unsigned int>(getWorkRegCount(group)));

  ZoneAllocator* allocator = getAllocator();
  RAWorkRegs workRegs;

  ASMJIT_PROPAGATE(workRegs.concat(allocator, getWorkRegs(group)));
  workRegs.sort(CompareByFrequency());

  LiveRegSpans tmp;

#if !defined(ASMJIT_DISABLE_LOGGING)
  StringBuilderTmp<512> sb;
#endif

  IntUtils::BitWordIterator<uint32_t> it(_availableRegs[group]);
  while (it.hasNext() && !workRegs.isEmpty()) {
    uint32_t physId = it.next();
    LiveRegSpans live;

    uint32_t dstIndex = 0;
    uint32_t srcIndex = 0;

    uint32_t numWorkRegs = workRegs.getLength();
    RAWorkReg** workRegsArray = workRegs.getData();

    for (srcIndex = 0; srcIndex < numWorkRegs; srcIndex++) {
      RAWorkReg* workReg = workRegsArray[srcIndex];
      Error err = tmp.nonOverlappingUnionOf(allocator, live, workReg->getLiveSpans(), LiveRegData(workReg->getVirtId()));

      if (err == kErrorOk) {
        live.swap(tmp);
        continue;
      }

      if (err != 0xFFFFFFFFU)
        return err;
      workRegsArray[dstIndex++] = workReg;
    }
    workRegs._setLength(dstIndex);

    ASMJIT_RA_LOG_COMPLEX({
      sb.clear();
      dumpSpans(sb, physId, live);
      logger->log(sb);
    });
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - Allocation - Local]
// ============================================================================

Error RAPass::runLocalAllocator() noexcept {
  RALocalAllocator lra(this);
  ASMJIT_PROPAGATE(lra.init());

  uint32_t blockCount = getBlockCount();
  if (!blockCount)
    return kErrorOk;

  // The allocation is done when this reaches zero.
  uint32_t blocksRemaining = getReachableBlockCount();

  // Current block.
  uint32_t blockId = 0;
  RABlock* block = _blocks[blockId];

  // The first block (entry) must always be reachable.
  ASMJIT_ASSERT(block->isReachable());

  // Assign function arguments for the initial block. The `lra` is valid now.
  lra.makeInitialAssignment();
  ASMJIT_PROPAGATE(setBlockEntryAssignment(block, block, lra._assignment));

  // The loop starts from the first block and iterates blocks in order, however,
  // the algorithm also allows to jump to any other block when finished if it's
  // a jump target. In-order iteration just makes sure that all blocks are visited.
  for (;;) {
    CBNode* first = block->getFirst();
    CBNode* last = block->getLast();
    CBNode* terminator = block->hasTerminator() ? last : nullptr;

    CBNode* beforeFirst = first->getPrev();
    CBNode* afterLast = last->getNext();

    bool unconditionalJump = false;
    RABlock* consecutive = nullptr;

    if (block->hasSuccessors())
      consecutive = block->getSuccessors()[0];

    lra.setBlock(block);
    for (CBNode* node = first; node != afterLast; node = node->getNext()) {
      if (node->actsAsInst()) {
        CBInst* cbInst = node->as<CBInst>();

        if (ASMJIT_UNLIKELY(cbInst == terminator)) {
          const RABlocks& successors = block->getSuccessors();
          if (block->hasConsecutive()) {
            ASMJIT_PROPAGATE(lra.allocBranch(cbInst, successors.getLast(), successors.getFirst()));
            continue;
          }
          else if (successors.getLength() > 1) {
            // TODO: Jump table.
            ASMJIT_ASSERT(!"IMPLEMENTED");
          }
          else {
            // Otherwise this is an unconditional jump, special handling isn't required.
            unconditionalJump = true;
          }
        }

        ASMJIT_PROPAGATE(lra.allocInst(cbInst));
      }
    }
    block->makeAllocated();

    if (consecutive) {
      if (consecutive->hasEntryAssignment()) {
        CBNode* prev = afterLast ? afterLast->getPrev() : cc()->getLastNode();
        cc()->_setCursor(unconditionalJump ? prev->getPrev() : prev);

        ASMJIT_PROPAGATE(
          lra.switchToAssignment(
            consecutive->getEntryPhysToWorkMap(),
            consecutive->getEntryWorkToPhysMap(),
            consecutive->getLiveIn(),
            consecutive->isAllocated(),
            false));
      }
      else {
        ASMJIT_PROPAGATE(setBlockEntryAssignment(consecutive, block, lra._assignment));
      }
    }

    // Important as the local allocator can insert instructions before
    // and after any instruction within the basic block.
    block->setFirst(beforeFirst->getNext());
    block->setLast(afterLast ? afterLast->getPrev() : cc()->getLastNode());

    if (--blocksRemaining == 0)
      break;

    // Switch to the next consecutive block, if any.
    if (consecutive) {
      block = consecutive;
      if (!block->isAllocated())
        continue;
    }

    // Get the next block.
    for (;;) {
      if (++blockId >= blockCount)
        blockId = 0;

      block = _blocks[blockId];
      if (!block->isReachable() || block->isAllocated() || !block->hasEntryAssignment())
        continue;

      break;
    }

    // If we switched to some block we have to update `lra`.
    lra.replaceAssignment(block->getEntryPhysToWorkMap(), block->getEntryWorkToPhysMap());
  }

  _clobberedRegs.op<IntUtils::Or>(lra._clobberedRegs);
  return kErrorOk;
}

Error RAPass::setBlockEntryAssignment(RABlock* block, const RABlock* fromBlock, const RAAssignment& fromAssignment) noexcept {
  PhysToWorkMap* physToWorkMap = clonePhysToWorkMap(fromAssignment.getPhysToWorkMap());
  WorkToPhysMap* workToPhysMap = cloneWorkToPhysMap(fromAssignment.getWorkToPhysMap());

  if (ASMJIT_UNLIKELY(!physToWorkMap || !workToPhysMap))
    return DebugUtils::errored(kErrorNoHeapMemory);

  block->setEntryAssignment(physToWorkMap, workToPhysMap);

  // Must be first block, otherwise impossible.
  if (block == fromBlock)
    return kErrorOk;

  const ZoneBitVector& liveOut = fromBlock->getLiveOut();
  const ZoneBitVector& liveIn  = block->getLiveIn();

  RAAssignment as;
  as.initLayout(_physRegCount, getWorkRegs());
  as.initMaps(physToWorkMap, workToPhysMap);

  ZoneBitVector::ForEachBitOp<IntUtils::AndNot> it(liveOut, liveIn);
  while (it.hasNext()) {
    uint32_t workId = it.next();
    RAWorkReg* workReg = getWorkReg(workId);

    uint32_t group = workReg->getGroup();
    uint32_t physId = as.workToPhysId(group, workId);

    if (physId != RAAssignment::kPhysNone)
      as.unassign(group, workId, physId);
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - Allocation - Prolog / Epilog]
// ============================================================================

Error RAPass::updateStackFrame() noexcept {
  // Calculate offsets of all stack slots.
  ASMJIT_PROPAGATE(_stackAllocator.calculateStackFrame());

  // Update function frame information to reflect the calculated values.
  FuncFrame& frame = getFunc()->getFrame();
  frame.setLocalStackSize(_stackAllocator.getStackSize());
  frame.setLocalStackAlignment(_stackAllocator.getAlignment());

  for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
    frame.addDirtyRegs(group, _clobberedRegs[group]);

  return kErrorOk;
}

Error RAPass::insertPrologEpilog() noexcept {
  FuncFrame& frame = _func->getFrame();

  ASMJIT_PROPAGATE(_argsAssignment.updateFuncFrame(frame));
  ASMJIT_PROPAGATE(frame.finalize());

  cc()->_setCursor(getFunc());
  ASMJIT_PROPAGATE(cc()->emitProlog(frame));
  ASMJIT_PROPAGATE(cc()->emitArgsAssignment(frame, _argsAssignment));

  cc()->_setCursor(getFunc()->getExitNode());
  ASMJIT_PROPAGATE(cc()->emitEpilog(frame));

  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - Allocation - Rewrite]
// ============================================================================

Error RAPass::rewrite() noexcept {
  ASMJIT_RA_LOG_INIT(getLogger());
  ASMJIT_RA_LOG_FORMAT("[RAPass::Rewrite]\n");

  return rewrite(_func, _stop);
}

Error RAPass::rewrite(CBNode* first, CBNode* stop) noexcept {
  uint32_t virtCount = cc()->_vRegArray.getLength();

  CBNode* node = first;
  while (node != stop) {
    if (node->actsAsInst()) {
      CBInst* cbInst = node->as<CBInst>();
      RAInst* raInst = node->getPassData<RAInst>();

      uint32_t i;
      Operand* operands = cbInst->getOpArray();
      uint32_t opCount = cbInst->getOpCount();

      if (ASMJIT_LIKELY(raInst)) {
        // If the instruction contains pass data (raInst) then it was a subject
        // for register allocation and must be rewritten to use physical regs.
        const OpInfo* opInfo = raInst->getOpInfo();

        RATiedReg* tiedRegs = raInst->getTiedRegs();
        uint32_t tiedCount = raInst->getTiedCount();

        for (i = 0; i < tiedCount; i++) {
          RATiedReg* tiedReg = &tiedRegs[i];
          RAWorkReg* workReg = getWorkReg(tiedReg->getWorkId());

          workReg->setTiedReg(tiedReg);
        }

        // Rewriting does the following:
        //   1. Get `VirtReg` instance,
        //   2. Get a linked `RAWorkReg` instance,
        //   3. Get a linked `RATiedReg` instance,
        //   4. Rewrite RegId from `VirtId` to `PhysId` based on `RATiedReg` content.
        if (cbInst->hasExtraReg()) {
          uint32_t virtIndex = Operand::unpackId(cbInst->getExtraReg().getId());

          // Only patch virtual registers.
          if (virtIndex < virtCount) {
            VirtReg* virtReg = cc()->getVirtRegAt(virtIndex);
            RAWorkReg* workReg = virtReg->getWorkReg();
            ASMJIT_ASSERT(workReg != nullptr);

            RATiedReg* tiedReg = workReg->getTiedReg();
            ASMJIT_ASSERT(tiedReg != nullptr);

            uint32_t physId = tiedReg->getUseId();
            ASMJIT_ASSERT(physId != Reg::kIdBad);

            cbInst->getExtraReg().setId(physId);
          }
        }

        for (i = 0; i < opCount; i++) {
          Operand& op = operands[i];
          if (op.isReg()) {
            Reg& reg = op.as<Reg>();
            uint32_t virtIndex = Operand::unpackId(reg.getId());

            // Only patch virtual registers.
            if (virtIndex < virtCount) {
              VirtReg* virtReg = cc()->getVirtRegAt(virtIndex);
              RAWorkReg* workReg = virtReg->getWorkReg();
              ASMJIT_ASSERT(workReg != nullptr);

              RATiedReg* tiedReg = workReg->getTiedReg();
              ASMJIT_ASSERT(tiedReg != nullptr);

              uint32_t physId = opInfo[i].isUse() ? tiedReg->getUseId() : tiedReg->getOutId();
              ASMJIT_ASSERT(physId < Reg::kIdBad);

              reg.setId(physId);
            }
          }
          else if (op.isMem()) {
            Mem& mem = op.as<Mem>();
            if (mem.isRegHome()) {
              uint32_t virtIndex = Operand::unpackId(mem.getBaseId());
              if (ASMJIT_UNLIKELY(virtIndex >= virtCount))
                return DebugUtils::errored(kErrorInvalidVirtId);

              VirtReg* virtReg = cc()->getVirtRegAt(virtIndex);
              RAWorkReg* workReg = virtReg->getWorkReg();
              ASMJIT_ASSERT(workReg != nullptr);

              RAStackSlot* slot = workReg->getStackSlot();
              int32_t offset = slot->offset;

              mem.setBase(_sp);
              mem.clearRegHome();
              mem.addOffsetLo32(offset);
            }
            else if (mem.hasBaseReg()) {
              uint32_t virtIndex = Operand::unpackId(mem.getBaseId());
              if (virtIndex < virtCount) {
                VirtReg* virtReg = cc()->getVirtRegAt(virtIndex);
                RAWorkReg* workReg = virtReg->getWorkReg();
                ASMJIT_ASSERT(workReg != nullptr);

                RATiedReg* tiedReg = workReg->getTiedReg();
                ASMJIT_ASSERT(tiedReg != nullptr);

                uint32_t physId = tiedReg->getUseId();
                ASMJIT_ASSERT(physId < Reg::kIdBad);

                mem.setBaseId(physId);
              }
            }

            if (mem.hasIndexReg()) {
              uint32_t virtIndex = Operand::unpackId(mem.getIndexId());
              if (virtIndex < virtCount) {
                VirtReg* virtReg = cc()->getVirtRegAt(virtIndex);
                RAWorkReg* workReg = virtReg->getWorkReg();
                ASMJIT_ASSERT(workReg != nullptr);

                RATiedReg* tiedReg = workReg->getTiedReg();
                ASMJIT_ASSERT(tiedReg != nullptr);

                uint32_t physId = tiedReg->getUseId();
                ASMJIT_ASSERT(physId < Reg::kIdBad);

                mem.setIndexId(physId);
              }
            }
          }
        }

        // This data is allocated by Zone passed to `runOnFunction()`, which
        // will be reset after the RA pass finishes. So reset this data to
        // prevent having a dead pointer after RA pass is complete.
        node->resetPassData();
      }
      else {
        // Even if the instruction doesn't have pass data it's possible that
        // it was added by register allocator and references a HOME register.
        for (i = 0; i < opCount; i++) {
          Operand& op = operands[i];
          if (op.isMem()) {
            Mem& mem = op.as<Mem>();
            if (mem.isRegHome()) {
              uint32_t virtIndex = Operand::unpackId(mem.getBaseId());
              if (ASMJIT_UNLIKELY(virtIndex >= virtCount))
                return DebugUtils::errored(kErrorInvalidVirtId);

              VirtReg* virtReg = cc()->getVirtRegAt(virtIndex);
              RAWorkReg* workReg = virtReg->getWorkReg();
              ASMJIT_ASSERT(workReg != nullptr);

              RAStackSlot* slot = workReg->getStackSlot();
              int32_t offset = slot->offset;

              mem.setBase(_sp);
              mem.clearRegHome();
              mem.addOffsetLo32(offset);
            }
          }
        }
      }

      if (ASMJIT_UNLIKELY(node->getType() != CBNode::kNodeInst)) {
        // FuncRet terminates the flow, it must either be removed if the exit
        // label is next to it (optimization) or patched to an architecture
        // dependent jump instruction that jumps to the function's exit before
        // the epilog.
        if (node->getType() == CBNode::kNodeFuncRet) {
          RABlock* block = raInst->getBlock();
          CBNode* next = node->getNext();

          if (!isNextTo(node, _func->getExitNode())) {
            cc()->_setCursor(node->getPrev());
            ASMJIT_PROPAGATE(onEmitJump(_func->getExitNode()->getLabel()));
          }

          CBNode* prev = node->getPrev();
          cc()->removeNode(node);
          block->setLast(prev);

          node = next;
          continue;
        }
      }
    }

    node = node->getNext();
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::RAPass - Logging]
// ============================================================================

#if !defined(ASMJIT_DISABLE_LOGGING)
ASMJIT_FAVOR_SIZE Error RAPass::annotateCode() noexcept {
  const RABlocks& blocks = _blocks;
  StringBuilderTmp<1024> sb;

  for (uint32_t i = 0, len = blocks.getLength(); i < len; i++) {
    const RABlock* block = blocks[i];
    CBNode* node = block->getFirst();

    if (!node)
      continue;

    CBNode* last = block->getLast();
    for (;;) {
      sb.clear();
      Logging::formatNode(sb, _loggerOptions, cc(), node);

      node->setInlineComment(
        static_cast<char*>(
          cc()->_dataZone.dup(sb.getData(), sb.getLength(), true)));

      if (node == last)
        break;
      node = node->getNext();
    }
  }

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error RAPass::_logBlockIds(const RABlocks& blocks) noexcept {
  // Can only be called if the `Logger` is present.
  ASMJIT_ASSERT(hasLogger());

  StringBuilderTmp<1024> sb;
  sb.appendString("  [Succ] {");

  for (uint32_t i = 0, len = blocks.getLength(); i < len; i++) {
    const RABlock* block = blocks[i];
    if (i != 0)
      sb.appendString(", ");
    sb.appendFormat("#%u", static_cast<unsigned int>(block->getBlockId()));
  }

  sb.appendString("}\n");
  return getLogger()->log(sb.getData(), sb.getLength());
}

ASMJIT_FAVOR_SIZE Error RAPass::_dumpBlockLiveness(StringBuilder& sb, const RABlock* block) noexcept {
  for (uint32_t liveType = 0; liveType < RABlock::kLiveCount; liveType++) {
    const char* bitsName = liveType == RABlock::kLiveIn  ? "IN  " :
                           liveType == RABlock::kLiveOut ? "OUT " :
                           liveType == RABlock::kLiveGen ? "GEN " : "KILL";

    const ZoneBitVector& bits = block->_liveBits[liveType];
    uint32_t len = bits.getLength();
    ASMJIT_ASSERT(len <= getWorkRegCount());

    uint32_t n = 0;
    for (uint32_t workId = 0; workId < len; workId++) {
      if (bits.getAt(workId)) {
        RAWorkReg* wReg = getWorkReg(workId);

        if (!n)
          sb.appendFormat("    %s [", bitsName);
        else
          sb.appendString(", ");

        sb.appendString(wReg->getName());
        n++;
      }
    }

    if (n)
      sb.appendString("]\n");
  }

  return kErrorOk;
}

ASMJIT_FAVOR_SIZE Error RAPass::_dumpLiveSpans(StringBuilder& sb) noexcept {
  uint32_t numWorkRegs = _workRegs.getLength();
  uint32_t maxLen = _maxWorkRegNameLength;

  for (uint32_t workId = 0; workId < numWorkRegs; workId++) {
    RAWorkReg* workReg = _workRegs[workId];

    sb.appendString("  ");

    size_t oldLen = sb.getLength();
    sb.appendString(workReg->getName());
    sb.padEnd(oldLen + maxLen);

    RALiveStats& stats = workReg->getLiveStats();

    sb.appendFormat(" {id:%04u width: %-4u freq: %0.4f}", workReg->getVirtId(), stats.getWidth(), stats.getFreq());
    sb.appendString(": ");

    LiveRegSpans& liveSpans = workReg->getLiveSpans();
    for (uint32_t x = 0; x < liveSpans.getLength(); x++) {
      const LiveRegSpan& liveSpan = liveSpans[x];
      if (x) sb.appendString(", ");
      sb.appendFormat("[%u:%u]", liveSpan.a, liveSpan.b);
    }

    sb.appendChar('\n');
  }

  return kErrorOk;
}
#endif

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
