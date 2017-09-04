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
#include "../base/rastack_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::RAStackAllocator - Slots]
// ============================================================================

RAStackSlot* RAStackAllocator::newSlot(uint32_t size, uint32_t alignment, uint32_t flags) noexcept {
  if (ASMJIT_UNLIKELY(_slots.willGrow(getAllocator(), 1) != kErrorOk))
    return nullptr;

  RAStackSlot* slot = getAllocator()->allocT<RAStackSlot>();
  if (ASMJIT_UNLIKELY(!slot))
    return nullptr;

  slot->size = size;
  slot->alignment = std::max<uint32_t>(alignment, 1);
  slot->flags = flags;
  slot->usage = 0;
  slot->weight = 0;
  slot->offset = 0;

  _alignment = std::max<uint32_t>(_alignment, alignment);
  _slots.appendUnsafe(slot);
  return slot;
}

// ============================================================================
// [asmjit::RAStackAllocator - Calculation]
// ============================================================================

struct RAStackGap {
  ASMJIT_INLINE RAStackGap() noexcept
    : offset(0),
      size(0) {}

  ASMJIT_INLINE RAStackGap(uint32_t offset, uint32_t size) noexcept
    : offset(offset),
      size(size) {}

  ASMJIT_INLINE RAStackGap(const RAStackGap& other) noexcept
    : offset(other.offset),
      size(other.size) {}

  uint32_t offset;
  uint32_t size;
};

struct RAStackSlot_CompareByWeight : public AsmJitInternal::CompareByValue<RAStackSlot_CompareByWeight, false> {
  static ASMJIT_INLINE uint32_t getValue(const RAStackSlot* item) noexcept { return item->weight; }
};

Error RAStackAllocator::calculateStackFrame() noexcept {
  // Base weight added to all registers regardless of their size and alignment.
  uint32_t kBaseRegWeight = 16;

  uint32_t i;
  RAStackSlots& slots = getSlots();
  uint32_t numSlots = slots.getLength();

  // STEP 1:
  //
  // Update usage based on the size of the slot. We boost smaller slots in a way
  // that 32-bit register has higher priority than a 128-bit register, however,
  // if one 128-bit register is used 4 times more than some other 32-bit register
  // it will overweight it.
  for (i = 0; i < numSlots; i++) {
    RAStackSlot* slot = _slots[i];

    uint32_t alignment = slot->alignment;
    ASMJIT_ASSERT(alignment > 0);

    uint32_t power = IntUtils::ctz(alignment);
    uint64_t weight;

    if (slot->isRegHome()) {
      weight = kBaseRegWeight + (static_cast<uint64_t>(slot->usage) * (7 - power));
    }
    else {
      weight = power;
    }

    // If overflown, which has less chance of winning a lottery, just use max
    // possible weight. In such case it probably doesn't matter at all.
    if (weight > 0xFFFFFFFFU) weight = 0xFFFFFFFFU;

    slot->usage = static_cast<uint32_t>(weight);
  }

  // STEP 2:
  //
  // Sort stack slots based on their newly calculated weight (in descending order).
  slots.sort(RAStackSlot_CompareByWeight());

  // STEP 3:
  //
  // Calculate offset of each slot. We start from the slot that has the highest
  // weight and advance to slots with lower weight. It could look that offsets
  // start from the first slot in our list and then simply increase, but it's
  // not always the case as we also try to fill all gaps introduced by the fact
  // that slots are sorted by weight and not by size & alignment, so when we need
  // to align some slot we distribute the gap caused by the alignment to `gaps`.
  uint32_t offset = 0;

  ZoneAllocator* allocator = getAllocator();
  ZoneVector<RAStackGap> gaps[kSizeCount - 1];

  for (i = 0; i < numSlots; i++) {
    RAStackSlot* slot = _slots[i];

    uint32_t slotAlignment = slot->alignment;
    uint32_t alignedOffset = IntUtils::alignTo(offset, slotAlignment);

    // Try to find a slot within gaps first, before advancing the `offset`.
    bool foundGap = false;

    uint32_t gapSize = 0;
    uint32_t gapOffset = 0;

    // Try to find a slot within gaps first.
    {
      uint32_t slotSize = slot->size;
      if (slotSize < (1U << static_cast<uint32_t>(ASMJIT_ARRAY_SIZE(gaps)))) {
        // Iterate from the lowest to the highest possible.
        uint32_t index = IntUtils::ctz(slotSize);
        do {
          if (!gaps[index].isEmpty()) {
            RAStackGap gap = gaps[index].pop();

            ASMJIT_ASSERT(IntUtils::isAligned(gap.offset, slotAlignment));
            slot->offset = static_cast<int32_t>(gap.offset);

            gapSize = gap.size - slotSize;
            gapOffset = gap.offset - slotSize;

            foundGap = true;
            break;
          }
        } while (++index < static_cast<uint32_t>(ASMJIT_ARRAY_SIZE(gaps)));
      }
    }

    // No gap found, we may create a new one(s) if the current offset is not aligned.
    if (!foundGap && offset != alignedOffset) {
      gapSize = alignedOffset - offset;
      gapOffset = alignedOffset;

      offset = alignedOffset;
    }

    // True if we have found a gap and not filled all of it or we aligned the current offset.
    if (gapSize) {
      uint32_t slotSize = IntUtils::alignToPowerOf2(gapSize + 1) / 2;
      do {
        if (gapSize >= slotSize) {
          gapSize -= slotSize;
          gapOffset -= slotSize;

          uint32_t index = IntUtils::ctz(slotSize);
          ASMJIT_PROPAGATE(gaps[index].append(allocator, RAStackGap(gapOffset, slotSize)));
        }
        slotSize >>= 1;
      } while (gapSize);
    }

    if (!foundGap) {
      ASMJIT_ASSERT(IntUtils::isAligned(offset, slotAlignment));
      slot->offset = static_cast<int32_t>(offset);
      offset += slot->size;
    }
  }

  _stackSize = IntUtils::alignTo(offset, _alignment);
  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
