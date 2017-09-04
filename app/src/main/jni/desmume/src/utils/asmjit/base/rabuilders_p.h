// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_RABUILDERS_P_H
#define _ASMJIT_BASE_RABUILDERS_P_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/rapass_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_ra
//! \{

// ============================================================================
// [asmjit::RACFGBuilder]
// ============================================================================

template<typename This>
class RACFGBuilder {
public:
  ASMJIT_INLINE RACFGBuilder(RAPass* pass) noexcept : _pass(pass) {}

  Error run() noexcept {
    ASMJIT_RA_LOG_INIT(_pass->getLogger());
    ASMJIT_RA_LOG_FORMAT("[RAPass::ConstructCFG]\n");

    CodeCompiler* cc = _pass->cc();
    CCFunc* func = _pass->getFunc();
    CBNode* node = func;

    bool hasCode = false;
    uint32_t position = 0;
    uint32_t exitLabelId = func->getExitNode()->getId();

    // Create the first (entry) block.
    RABlock* currentBlock = _pass->newBlock();
    if (ASMJIT_UNLIKELY(!currentBlock))
      return DebugUtils::errored(kErrorNoHeapMemory);
    ASMJIT_PROPAGATE(_pass->addBlock(currentBlock));

    RARegsStats blockRegStats;
    blockRegStats.reset();

#if !defined(ASMJIT_DISABLE_LOGGING)
    StringBuilderTmp<512> sb;
    RABlock* lastPrintedBlock = nullptr;
    uint32_t loggerOptions = Logger::kOptionNodePosition;

    if (logger) {
      loggerOptions |= logger->getOptions();

      Logging::formatNode(sb, loggerOptions, cc, node);
      logger->logf("  %s\n", sb.getData());

      lastPrintedBlock = currentBlock;
      logger->logf("  {#%u}\n", lastPrintedBlock->getBlockId());
    }
#endif

    node = node->getNext();
    if (ASMJIT_UNLIKELY(!node))
      return DebugUtils::errored(kErrorInvalidState);

    currentBlock->setFirst(node);
    currentBlock->setLast(node);

    for (;;) {
      ASMJIT_ASSERT(!node->hasPosition());
      // Unlikely: Assume there is more instructions than labels by default.
      if (ASMJIT_UNLIKELY(node->getType() == CBNode::kNodeLabel)) {
        if (!currentBlock) {
          // If the current code is unreachable the label makes it reachable again.
          currentBlock = node->getPassData<RABlock>();
          if (currentBlock) {
            // If the label has a block assigned we can either continue with
            // it or skip it if the block has been constructed already.
            if (currentBlock->isConstructed())
              break;
          }
          else {
            // No block assigned, to create a new one, and assign it.
            currentBlock = _pass->newBlock(node);
            if (ASMJIT_UNLIKELY(!currentBlock))
              return DebugUtils::errored(kErrorNoHeapMemory);

            node->setPassData<RABlock>(currentBlock);
            hasCode = false;
            blockRegStats.reset();
          }

          ASMJIT_PROPAGATE(_pass->addBlock(currentBlock));
        }
        else {
          if (node->hasPassData()) {
            RABlock* consecutive = node->getPassData<RABlock>();
            if (currentBlock == consecutive) {
              // The label currently processed is part of the current block. This
              // is only possible for multiple labels that are right next to each
              // other, or are separated by non-code nodes (.align,  comments).
              if (ASMJIT_UNLIKELY(hasCode))
                return DebugUtils::errored(kErrorInvalidState);
            }
            else {
              // Label makes the current block constructed. There is a chance that the
              // Label is not used, but we don't know that at this point. In the worst
              // case there would be two blocks next to each other, it's just fine.
              ASMJIT_ASSERT(currentBlock->getLast() != node);
              currentBlock->setLast(node->getPrev());
              currentBlock->addFlags(RABlock::kFlagHasConsecutive);
              currentBlock->makeConstructed(position + 1, blockRegStats);

              ASMJIT_PROPAGATE(currentBlock->appendSuccessor(consecutive));
              ASMJIT_PROPAGATE(_pass->addBlock(consecutive));

              currentBlock = consecutive;
              hasCode = false;
              blockRegStats.reset();
            }
          }
          else {
            // First time we see this label.
            if (hasCode) {
              // Cannot continue the current block if it already contains some
              // code. We need to create a new block and make it a successor.
              ASMJIT_ASSERT(currentBlock->getLast() != node);
              currentBlock->setLast(node->getPrev());
              currentBlock->addFlags(RABlock::kFlagHasConsecutive);
              currentBlock->makeConstructed(position + 1, blockRegStats);

              RABlock* consecutive = _pass->newBlock(node);
              if (ASMJIT_UNLIKELY(!consecutive))
                return DebugUtils::errored(kErrorNoHeapMemory);

              ASMJIT_PROPAGATE(currentBlock->appendSuccessor(consecutive));
              ASMJIT_PROPAGATE(_pass->addBlock(consecutive));

              currentBlock = consecutive;
              hasCode = false;
              blockRegStats.reset();
            }

            node->setPassData<RABlock>(currentBlock);
          }
        }

#if !defined(ASMJIT_DISABLE_LOGGING)
        if (logger) {
          if (currentBlock && currentBlock != lastPrintedBlock) {
            lastPrintedBlock = currentBlock;
            logger->logf("  {#%u}\n", lastPrintedBlock->getBlockId());
          }

          sb.clear();
          Logging::formatNode(sb, loggerOptions, cc, node);
          logger->logf("  %s\n", sb.getData());
        }
#endif

        // Unlikely: Assume that the exit label is reached only once per function.
        if (ASMJIT_UNLIKELY(node->as<CBLabel>()->getId() == exitLabelId)) {
          currentBlock->setLast(node);
          currentBlock->addFlags(RABlock::kFlagIsFuncExit);
          currentBlock->makeConstructed(position + 1, blockRegStats);
          ASMJIT_PROPAGATE(_pass->_exits.append(_pass->getAllocator(), currentBlock));

          currentBlock = nullptr;
        }
      }
      else {
        if (node->actsAsInst()) {
          if (ASMJIT_UNLIKELY(!currentBlock)) {
            // If this code is unreachable then it has to be removed.
#if !defined(ASMJIT_DISABLE_LOGGING)
            if (logger) {
              sb.clear();
              Logging::formatNode(sb, loggerOptions, cc, node);
              logger->logf("  <Removed> %s\n", sb.getData());
            }
#endif

            CBNode* next = node->getNext();
            cc->removeNode(node);
            node = next;

            continue;
          }
          else {
            // Handle `CBInst`, `CCFuncCall`, and `CCFuncRet`. All of
            // these share the `CBInst` interface and contain operands.
            position += 2;
            node->setPosition(position);

            if (!hasCode) {
              hasCode = true;
              currentBlock->_firstPosition = position;
            }

#if !defined(ASMJIT_DISABLE_LOGGING)
            if (logger) {
              sb.clear();
              Logging::formatNode(sb, loggerOptions, cc, node);
              logger->logf("    %s\n", sb.getData());
            }
#endif

            CBInst* inst = node->as<CBInst>();
            uint32_t jumpType = Inst::kJumpTypeNone;

            static_cast<This*>(this)->onInst(inst, currentBlock, jumpType, blockRegStats);
            if (jumpType != Inst::kJumpTypeNone) {
              // Support for conditional and unconditional jumps.
              if (jumpType == Inst::kJumpTypeDirect || jumpType == Inst::kJumpTypeConditional) {
                // Jmp/Jcc/Call/Loop/etc...
                uint32_t opCount = inst->getOpCount();
                const Operand* opArray = inst->getOpArray();

                // The last operand must be label (this supports also instructions
                // like jecx in explicit form).
                if (ASMJIT_UNLIKELY(opCount == 0 || !opArray[opCount - 1].isLabel()))
                  return DebugUtils::errored(kErrorInvalidState);

                CBLabel* cbLabel;
                ASMJIT_PROPAGATE(cc->getLabelNode(&cbLabel, opArray[opCount - 1].as<Label>()));

                RABlock* targetBlock = _pass->newBlockOrExistingAt(cbLabel);
                if (ASMJIT_UNLIKELY(!targetBlock))
                  return DebugUtils::errored(kErrorNoHeapMemory);

                currentBlock->setLast(node);
                currentBlock->addFlags(RABlock::kFlagHasTerminator);
                currentBlock->makeConstructed(position + 1, blockRegStats);
                ASMJIT_PROPAGATE(currentBlock->appendSuccessor(targetBlock));

                if (jumpType == Inst::kJumpTypeDirect) {
                  // Unconditional jump makes the code after the jump unreachable,
                  // which will be removed instantly during the CFG construction;
                  // as we cannot allocate registers for instructions that are not
                  // part of any block. Of course we can leave these instructions
                  // as they are, however, that would only postpone the problem as
                  // assemblers can't encode instructions that use virtual registers.
                  currentBlock = nullptr;
                }
                else {
                  node = node->getNext();
                  if (ASMJIT_UNLIKELY(!node))
                    return DebugUtils::errored(kErrorInvalidState);

                  RABlock* consecutiveBlock;
                  if (node->getType() == CBNode::kNodeLabel) {
                    if (node->hasPassData()) {
                      consecutiveBlock = node->getPassData<RABlock>();
                    }
                    else {
                      consecutiveBlock = _pass->newBlock(node);
                      if (ASMJIT_UNLIKELY(!consecutiveBlock))
                        return DebugUtils::errored(kErrorNoHeapMemory);
                      node->setPassData<RABlock>(consecutiveBlock);
                    }
                  }
                  else {
                    consecutiveBlock = _pass->newBlock(node);
                    if (ASMJIT_UNLIKELY(!consecutiveBlock))
                      return DebugUtils::errored(kErrorNoHeapMemory);
                  }

                  currentBlock->addFlags(RABlock::kFlagHasConsecutive);
                  ASMJIT_PROPAGATE(currentBlock->prependSuccessor(consecutiveBlock));

                  currentBlock = consecutiveBlock;
                  hasCode = false;
                  blockRegStats.reset();

                  if (currentBlock->isConstructed())
                    break;
                  ASMJIT_PROPAGATE(_pass->addBlock(consecutiveBlock));

#if !defined(ASMJIT_DISABLE_LOGGING)
                  lastPrintedBlock = currentBlock;
                  ASMJIT_RA_LOG_FORMAT("  {#%u}\n", lastPrintedBlock->getBlockId());
#endif

                  continue;
                }
              }

              if (jumpType == Inst::kJumpTypeReturn) {
                currentBlock->setLast(node);
                currentBlock->makeConstructed(position + 1, blockRegStats);
                ASMJIT_PROPAGATE(_pass->_exits.append(_pass->getAllocator(), currentBlock));

                currentBlock = nullptr;
              }
            }
          }
        }
        else {
#if !defined(ASMJIT_DISABLE_LOGGING)
          if (logger) {
            sb.clear();
            Logging::formatNode(sb, loggerOptions, cc, node);
            logger->logf("    %s\n", sb.getData());
          }
#endif

          if (node->getType() == CBNode::kNodeSentinel) {
            if (node == func->getEnd()) {
              // Make sure we didn't flow here if this is the end of the function sentinel.
              if (ASMJIT_UNLIKELY(currentBlock))
                return DebugUtils::errored(kErrorInvalidState);
              break;
            }
          }
          else if (node->getType() == CBNode::kNodeFunc) {
            // RAPass can only compile a single function at a time. If we
            // encountered a function it must be the current one, bail if not.
            if (ASMJIT_UNLIKELY(node != func))
              return DebugUtils::errored(kErrorInvalidState);
            // PASS if this is the first node.
          }
          else {
            // PASS if this is a non-interesting or unknown node.
          }
        }
      }

      // Advance to the next node.
      node = node->getNext();

      // NOTE: We cannot encounter a NULL node, because every function must be
      // terminated by a sentinel (`stop`) node. If we encountered a NULL node it
      // means that something went wrong and this node list is corrupted; bail in
      // such case.
      if (ASMJIT_UNLIKELY(!node))
        return DebugUtils::errored(kErrorInvalidState);
    }

    if (_pass->hasDanglingBlocks())
      return DebugUtils::errored(kErrorInvalidState);

    return kErrorOk;
  }

  RAPass* _pass;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_RABUILDERS_P_H
