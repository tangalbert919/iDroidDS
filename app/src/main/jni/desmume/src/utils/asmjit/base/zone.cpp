// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/intutils.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// Zero size block used by `Zone` that doesn't have any memory allocated.
// Should be allocated in read-only memory and should never be modified.
static const Zone::Block Zone_zeroBlock = { nullptr, nullptr, 0, { 0 } };

static ASMJIT_INLINE uint32_t Zone_getAlignmentOffsetFromAlignment(uint32_t x) noexcept {
  switch (x) {
    default: return 0;
    case 0 : return 0;
    case 1 : return 0;
    case 2 : return 1;
    case 4 : return 2;
    case 8 : return 3;
    case 16: return 4;
    case 32: return 5;
    case 64: return 6;
  }
}

// ============================================================================
// [asmjit::Zone - Construction / Destruction]
// ============================================================================

Zone::Zone(uint32_t blockSize, uint32_t blockAlignment) noexcept {
  Block* empty = const_cast<Zone::Block*>(&Zone_zeroBlock);
  _ptr = empty->data;
  _end = empty->data;
  _block = empty;
  _blockSize = blockSize;
  _blockAlignmentShift = Zone_getAlignmentOffsetFromAlignment(blockAlignment);
}

Zone::~Zone() noexcept {
  reset(true);
}

// ============================================================================
// [asmjit::Zone - Reset]
// ============================================================================

void Zone::reset(bool releaseMemory) noexcept {
  Block* cur = _block;

  // Can't be altered.
  if (cur == &Zone_zeroBlock)
    return;

  if (releaseMemory) {
    // Since `cur` can be in the middle of the double-linked list, we have to
    // iterate in both directions `prev` and `next` separately.
    Block* next = cur->next;
    do {
      Block* prev = cur->prev;
      AsmJitInternal::releaseMemory(cur);
      cur = prev;
    } while (cur);

    cur = next;
    while (cur) {
      next = cur->next;
      AsmJitInternal::releaseMemory(cur);
      cur = next;
    }

    Block* empty = const_cast<Zone::Block*>(&Zone_zeroBlock);
    _ptr = empty->data;
    _end = empty->data;
    _block = empty;
  }
  else {
    while (cur->prev)
      cur = cur->prev;

    _ptr = cur->data;
    _end = _ptr + cur->size;
    _block = cur;
  }
}

// ============================================================================
// [asmjit::Zone - Alloc]
// ============================================================================

void* Zone::_alloc(size_t size) noexcept {
  Block* curBlock = _block;
  uint8_t* p;

  size_t blockSize = std::max<size_t>(_blockSize, size);
  size_t blockAlignment = getBlockAlignment();

  // The `_alloc()` method can only be called if there is not enough space
  // in the current block, see `alloc()` implementation for more details.
  ASMJIT_ASSERT(curBlock == &Zone_zeroBlock || getRemainingSize() < size);

  // If the `Zone` has been cleared the current block doesn't have to be the
  // last one. Check if there is a block that can be used instead of allocating
  // a new one. If there is a `next` block it's completely unused, we don't have
  // to check for remaining bytes.
  Block* next = curBlock->next;
  if (next && next->size >= size) {
    p = IntUtils::alignTo(next->data, blockAlignment);

    _block = next;
    _ptr = p + size;
    _end = next->data + next->size;

    return static_cast<void*>(p);
  }

  // Prevent arithmetic overflow.
  const size_t kBaseBlockSize = sizeof(Block) - sizeof(void*);
  if (ASMJIT_UNLIKELY(blockSize > (~static_cast<size_t>(0) - kBaseBlockSize - blockAlignment)))
    return nullptr;

  blockSize += blockAlignment;
  Block* newBlock = static_cast<Block*>(AsmJitInternal::allocMemory(kBaseBlockSize + blockSize));

  if (ASMJIT_UNLIKELY(!newBlock))
    return nullptr;

  // Align the pointer to `blockAlignment` and adjust the size of this block
  // accordingly. It's the same as using `blockAlignment - IntUtils::alignDiff()`,
  // just written differently.
  p = IntUtils::alignTo(newBlock->data, blockAlignment);
  newBlock->prev = nullptr;
  newBlock->next = nullptr;
  newBlock->size = blockSize;

  if (curBlock != &Zone_zeroBlock) {
    newBlock->prev = curBlock;
    curBlock->next = newBlock;

    // Does only happen if there is a next block, but the requested memory
    // can't fit into it. In this case a new buffer is allocated and inserted
    // between the current block and the next one.
    if (next) {
      newBlock->next = next;
      next->prev = newBlock;
    }
  }

  _block = newBlock;
  _ptr = p + size;
  _end = newBlock->data + blockSize;

  return static_cast<void*>(p);
}

void* Zone::allocZeroed(size_t size) noexcept {
  void* p = alloc(size);
  if (ASMJIT_UNLIKELY(!p))
    return p;
  return ::memset(p, 0, size);
}

void* Zone::dup(const void* data, size_t size, bool nullTerminate) noexcept {
  if (ASMJIT_UNLIKELY(!data || !size))
    return nullptr;

  ASMJIT_ASSERT(size != IntUtils::maxValue<size_t>());
  uint8_t* m = allocT<uint8_t>(size + nullTerminate);
  if (ASMJIT_UNLIKELY(!m)) return nullptr;

  ::memcpy(m, data, size);
  if (nullTerminate) m[size] = '\0';

  return static_cast<void*>(m);
}

char* Zone::sformat(const char* fmt, ...) noexcept {
  if (ASMJIT_UNLIKELY(!fmt))
    return nullptr;

  char buf[512];
  size_t len;

  va_list ap;
  va_start(ap, fmt);

  len = vsnprintf(buf, ASMJIT_ARRAY_SIZE(buf) - 1, fmt, ap);
  buf[len++] = 0;

  va_end(ap);
  return static_cast<char*>(dup(buf, len));
}

// ============================================================================
// [asmjit::ZoneAllocator - Helpers]
// ============================================================================

static bool ZoneAllocator_hasDynamicBlock(ZoneAllocator* self, ZoneAllocator::DynamicBlock* block) noexcept {
  ZoneAllocator::DynamicBlock* cur = self->_dynamicBlocks;
  while (cur) {
    if (cur == block)
      return true;
    cur = cur->next;
  }
  return false;
}

// ============================================================================
// [asmjit::ZoneAllocator - Init / Reset]
// ============================================================================

void ZoneAllocator::reset(Zone* zone) noexcept {
  // Free dynamic blocks.
  DynamicBlock* block = _dynamicBlocks;
  while (block) {
    DynamicBlock* next = block->next;
    AsmJitInternal::releaseMemory(block);
    block = next;
  }

  // Zero the entire class and initialize to the given `zone`.
  ::memset(this, 0, sizeof(*this));
  _zone = zone;
}

// ============================================================================
// [asmjit::ZoneAllocator - Alloc / Release]
// ============================================================================

void* ZoneAllocator::_alloc(size_t size, size_t& allocatedSize) noexcept {
  ASMJIT_ASSERT(isInitialized());

  // We use our memory pool only if the requested block is of a reasonable size.
  uint32_t slot;
  if (_getSlotIndex(size, slot, allocatedSize)) {
    // Slot reuse.
    uint8_t* p = reinterpret_cast<uint8_t*>(_slots[slot]);
    size = allocatedSize;

    if (p) {
      _slots[slot] = reinterpret_cast<Slot*>(p)->next;
      //printf("ALLOCATED %p of size %d (SLOT %d)\n", p, int(size), slot);
      return p;
    }

    p = _zone->align(kBlockAlignment);
    size_t remain = (size_t)(_zone->getEnd() - p);

    if (ASMJIT_LIKELY(remain >= size)) {
      _zone->setCursor(p + size);
      //printf("ALLOCATED %p of size %d (SLOT %d)\n", p, int(size), slot);
      return p;
    }
    else {
      // Distribute the remaining memory to suitable slots, if possible.
      if (remain >= kLoGranularity) {
        do {
          size_t distSize = std::min<size_t>(remain, kLoMaxSize);
          uint32_t distSlot = static_cast<uint32_t>((distSize - kLoGranularity) / kLoGranularity);
          ASMJIT_ASSERT(distSlot < kLoCount);

          reinterpret_cast<Slot*>(p)->next = _slots[distSlot];
          _slots[distSlot] = reinterpret_cast<Slot*>(p);

          p += distSize;
          remain -= distSize;
        } while (remain >= kLoGranularity);
        _zone->setCursor(p);
      }

      p = static_cast<uint8_t*>(_zone->_alloc(size));
      if (ASMJIT_UNLIKELY(!p)) {
        allocatedSize = 0;
        return nullptr;
      }

      //printf("ALLOCATED %p of size %d (SLOT %d)\n", p, int(size), slot);
      return p;
    }
  }
  else {
    // Allocate a dynamic block.
    size_t overhead = sizeof(DynamicBlock) + sizeof(DynamicBlock*) + kBlockAlignment;

    // Handle a possible overflow.
    if (ASMJIT_UNLIKELY(overhead >= ~static_cast<size_t>(0) - size))
      return nullptr;

    void* p = AsmJitInternal::allocMemory(size + overhead);
    if (ASMJIT_UNLIKELY(!p)) {
      allocatedSize = 0;
      return nullptr;
    }

    // Link as first in `_dynamicBlocks` double-linked list.
    DynamicBlock* block = static_cast<DynamicBlock*>(p);
    DynamicBlock* next = _dynamicBlocks;

    if (next)
      next->prev = block;

    block->prev = nullptr;
    block->next = next;
    _dynamicBlocks = block;

    // Align the pointer to the guaranteed alignment and store `DynamicBlock`
    // at the end of the memory block, so `_releaseDynamic()` can find it.
    p = IntUtils::alignTo(static_cast<uint8_t*>(p) + sizeof(DynamicBlock) + sizeof(DynamicBlock*), kBlockAlignment);
    reinterpret_cast<DynamicBlock**>(p)[-1] = block;

    allocatedSize = size;
    //printf("ALLOCATED DYNAMIC %p of size %d\n", p, int(size));
    return p;
  }
}

void* ZoneAllocator::_allocZeroed(size_t size, size_t& allocatedSize) noexcept {
  ASMJIT_ASSERT(isInitialized());

  void* p = _alloc(size, allocatedSize);
  if (ASMJIT_UNLIKELY(!p)) return p;
  return ::memset(p, 0, allocatedSize);
}

void ZoneAllocator::_releaseDynamic(void* p, size_t size) noexcept {
  ASMJIT_UNUSED(size);
  ASMJIT_ASSERT(isInitialized());
  //printf("RELEASING DYNAMIC %p of size %d\n", p, int(size));

  // Pointer to `DynamicBlock` is stored at [-1].
  DynamicBlock* block = reinterpret_cast<DynamicBlock**>(p)[-1];
  ASMJIT_ASSERT(ZoneAllocator_hasDynamicBlock(this, block));

  // Unlink and free.
  DynamicBlock* prev = block->prev;
  DynamicBlock* next = block->next;

  if (prev)
    prev->next = next;
  else
    _dynamicBlocks = next;

  if (next)
    next->prev = prev;

  AsmJitInternal::releaseMemory(block);
}

// ============================================================================
// [asmjit::ZoneVectorBase - Helpers]
// ============================================================================

Error ZoneVectorBase::_grow(ZoneAllocator* allocator, uint32_t sizeOfT, uint32_t n) noexcept {
  uint32_t threshold = Globals::kAllocThreshold / sizeOfT;
  uint32_t capacity = _capacity;
  uint32_t after = _length;

  if (ASMJIT_UNLIKELY(IntUtils::maxValue<uint32_t>() - n < after))
    return DebugUtils::errored(kErrorNoHeapMemory);

  after += n;
  if (capacity >= after)
    return kErrorOk;

  // ZoneVector is used as an array to hold short-lived data structures used
  // during code generation. The growing strategy is simple - use small capacity
  // at the beginning (very good for ZoneAllocator) and then grow quicker to
  // prevent successive reallocations.
  if (capacity < 4)
    capacity = 4;
  else if (capacity < 8)
    capacity = 8;
  else if (capacity < 16)
    capacity = 16;
  else if (capacity < 64)
    capacity = 64;
  else if (capacity < 256)
    capacity = 256;

  while (capacity < after) {
    if (capacity < threshold)
      capacity *= 2;
    else
      capacity += threshold;
  }

  return _reserve(allocator, sizeOfT, capacity);
}

Error ZoneVectorBase::_reserve(ZoneAllocator* allocator, uint32_t sizeOfT, uint32_t n) noexcept {
  uint32_t oldCapacity = _capacity;
  if (oldCapacity >= n) return kErrorOk;

  uint32_t nBytes = n * sizeOfT;
  if (ASMJIT_UNLIKELY(nBytes < n))
    return DebugUtils::errored(kErrorNoHeapMemory);

  size_t allocatedBytes;
  uint8_t* newData = static_cast<uint8_t*>(allocator->alloc(nBytes, allocatedBytes));

  if (ASMJIT_UNLIKELY(!newData))
    return DebugUtils::errored(kErrorNoHeapMemory);

  void* oldData = _data;
  if (_length)
    ::memcpy(newData, oldData, static_cast<size_t>(_length) * sizeOfT);

  if (oldData)
    allocator->release(oldData, static_cast<size_t>(oldCapacity) * sizeOfT);

  _capacity = static_cast<uint32_t>(allocatedBytes / sizeOfT);
  ASMJIT_ASSERT(_capacity >= n);

  _data = newData;
  return kErrorOk;
}

Error ZoneVectorBase::_resize(ZoneAllocator* allocator, uint32_t sizeOfT, uint32_t n) noexcept {
  uint32_t length = _length;

  if (_capacity < n) {
    ASMJIT_PROPAGATE(_grow(allocator, sizeOfT, n - length));
    ASMJIT_ASSERT(_capacity >= n);
  }

  if (length < n)
    ::memset(static_cast<uint8_t*>(_data) + static_cast<size_t>(length) * sizeOfT, 0, static_cast<size_t>(n - length) * sizeOfT);

  _length = n;
  return kErrorOk;
}

// ============================================================================
// [asmjit::ZoneBitVector - Ops]
// ============================================================================

Error ZoneBitVector::copyFrom(ZoneAllocator* allocator, const ZoneBitVector& other) noexcept {
  BitWord* data = _data;
  uint32_t newLength = other.getLength();

  if (!newLength) {
    _length = 0;
    return kErrorOk;
  }

  if (newLength > _capacity) {
    // Realloc needed... Calculate the minimum capacity (in bytes) requied.
    uint32_t minimumCapacityInBits = IntUtils::alignTo<uint32_t>(newLength, kBitWordSize);
    if (ASMJIT_UNLIKELY(minimumCapacityInBits < newLength))
      return DebugUtils::errored(kErrorNoHeapMemory);

    // Normalize to bytes.
    uint32_t minimumCapacity = minimumCapacityInBits / 8;
    size_t allocatedCapacity;

    BitWord* newData = static_cast<BitWord*>(allocator->alloc(minimumCapacity, allocatedCapacity));
    if (ASMJIT_UNLIKELY(!newData))
      return DebugUtils::errored(kErrorNoHeapMemory);

    // `allocatedCapacity` now contains number in bytes, we need bits.
    size_t allocatedCapacityInBits = allocatedCapacity * 8;

    // Arithmetic overflow should normally not happen. If it happens we just
    // change the `allocatedCapacityInBits` to the `minimumCapacityInBits` as
    // this value is still safe to be used to call `_allocator->release(...)`.
    if (ASMJIT_UNLIKELY(allocatedCapacityInBits < allocatedCapacity))
      allocatedCapacityInBits = minimumCapacityInBits;

    if (data)
      allocator->release(data, _capacity / 8);
    data = newData;

    _data = data;
    _capacity = static_cast<uint32_t>(allocatedCapacityInBits);
  }

  _length = newLength;
  _copyBits(data, other.getData(), _wordsPerBits(newLength));

  return kErrorOk;
}

Error ZoneBitVector::_resize(ZoneAllocator* allocator, uint32_t newLength, uint32_t idealCapacity, bool newBitsValue) noexcept {
  ASMJIT_ASSERT(idealCapacity >= newLength);

  if (newLength <= _length) {
    // The size after the resize is lesser than or equal to the current length.
    uint32_t idx = newLength / kBitWordSize;
    uint32_t bit = newLength % kBitWordSize;

    // Just set all bits outside of the new length in the last word to zero.
    // There is a case that there are not bits to set if `bit` is zero. This
    // happens when `newLength` is a multiply of `kBitWordSize` like 64, 128,
    // and so on. In that case don't change anything as that would mean settings
    // bits outside of the `_length`.
    if (bit)
      _data[idx] &= (static_cast<uintptr_t>(1) << bit) - 1U;

    _length = newLength;
    return kErrorOk;
  }

  uint32_t oldLength = _length;
  BitWord* data = _data;

  if (newLength > _capacity) {
    // Realloc needed, calculate the minimum capacity (in bytes) requied.
    uint32_t minimumCapacityInBits = IntUtils::alignTo<uint32_t>(idealCapacity, kBitWordSize);

    if (ASMJIT_UNLIKELY(minimumCapacityInBits < newLength))
      return DebugUtils::errored(kErrorNoHeapMemory);

    // Normalize to bytes.
    uint32_t minimumCapacity = minimumCapacityInBits / 8;
    size_t allocatedCapacity;

    BitWord* newData = static_cast<BitWord*>(allocator->alloc(minimumCapacity, allocatedCapacity));
    if (ASMJIT_UNLIKELY(!newData))
      return DebugUtils::errored(kErrorNoHeapMemory);

    // `allocatedCapacity` now contains number in bytes, we need bits.
    size_t allocatedCapacityInBits = allocatedCapacity * 8;

    // Arithmetic overflow should normally not happen. If it happens we just
    // change the `allocatedCapacityInBits` to the `minimumCapacityInBits` as
    // this value is still safe to be used to call `_allocator->release(...)`.
    if (ASMJIT_UNLIKELY(allocatedCapacityInBits < allocatedCapacity))
      allocatedCapacityInBits = minimumCapacityInBits;

    _copyBits(newData, data, _wordsPerBits(oldLength));

    if (data)
      allocator->release(data, _capacity / 8);
    data = newData;

    _data = data;
    _capacity = static_cast<uint32_t>(allocatedCapacityInBits);
  }

  // Start (of the old length) and end (of the new length) bits
  uint32_t idx = oldLength / kBitWordSize;
  uint32_t startBit = oldLength % kBitWordSize;
  uint32_t endBit = newLength % kBitWordSize;

  // Set new bits to either 0 or 1. The `pattern` is used to set multiple
  // bits per bit-word and contains either all zeros or all ones.
  BitWord pattern = _patternFromBit(newBitsValue);

  // First initialize the last bit-word of the old length.
  if (startBit) {
    uint32_t nBits = 0;

    if (idx == (newLength / kBitWordSize)) {
      // The number of bit-words is the same after the resize. In that case
      // we need to set only bits necessary in the current last bit-word.
      ASMJIT_ASSERT(startBit < endBit);
      nBits = endBit - startBit;
    }
    else {
      // There is be more bit-words after the resize. In that case we don't
      // have to be extra careful about the last bit-word of the old length.
      nBits = kBitWordSize - startBit;
    }

    data[idx++] |= pattern << nBits;
  }

  // Initialize all bit-words after the last bit-word of the old length.
  uint32_t endIdx = _wordsPerBits(newLength);
  endIdx -= static_cast<uint32_t>(endIdx * kBitWordSize == newLength);

  while (idx <= endIdx)
    data[idx++] = pattern;

  // Clear unused bits of the last bit-word.
  if (endBit)
    data[endIdx] &= (static_cast<BitWord>(1) << endBit) - 1;

  _length = newLength;
  return kErrorOk;
}

Error ZoneBitVector::_append(ZoneAllocator* allocator, bool value) noexcept {
  uint32_t kThreshold = Globals::kAllocThreshold * 8;
  uint32_t newLength = _length + 1;
  uint32_t idealCapacity = _capacity;

  if (idealCapacity < 128)
    idealCapacity = 128;
  else if (idealCapacity <= kThreshold)
    idealCapacity *= 2;
  else
    idealCapacity += kThreshold;

  if (ASMJIT_UNLIKELY(idealCapacity < _capacity)) {
    if (ASMJIT_UNLIKELY(_length == IntUtils::maxValue<uint32_t>()))
      return DebugUtils::errored(kErrorNoHeapMemory);
    idealCapacity = newLength;
  }

  return _resize(allocator, newLength, idealCapacity, value);
}

Error ZoneBitVector::fill(uint32_t from, uint32_t to, bool value) noexcept {
  if (ASMJIT_UNLIKELY(from >= to)) {
    if (from > to)
      return DebugUtils::errored(kErrorInvalidArgument);
    else
      return kErrorOk;
  }

  ASMJIT_ASSERT(from <= _length);
  ASMJIT_ASSERT(to <= _length);

  // This is very similar to `ZoneBitVector::_fill()`, however, since we
  // actually set bits that are already part of the container we need to
  // special case filiing to zeros and ones.
  uint32_t idx = from / kBitWordSize;
  uint32_t startBit = from % kBitWordSize;

  uint32_t endIdx = to / kBitWordSize;
  uint32_t endBit = to % kBitWordSize;

  BitWord* data = _data;
  ASMJIT_ASSERT(data != nullptr);

  // Special case for non-zero `startBit`.
  if (startBit) {
    if (idx == endIdx) {
      ASMJIT_ASSERT(startBit < endBit);

      uint32_t nBits = endBit - startBit;
      BitWord mask = ((static_cast<BitWord>(1) << nBits) - 1) << startBit;

      if (value)
        data[idx] |= mask;
      else
        data[idx] &= ~mask;
      return kErrorOk;
    }
    else {
      BitWord mask = (static_cast<BitWord>(0) - 1) << startBit;

      if (value)
        data[idx++] |= mask;
      else
        data[idx++] &= ~mask;
    }
  }

  // Fill all bits in case there is a gap between the current `idx` and `endIdx`.
  if (idx < endIdx) {
    BitWord pattern = _patternFromBit(value);
    do {
      data[idx++] = pattern;
    } while (idx < endIdx);
  }

  // Special case for non-zero `endBit`.
  if (endBit) {
    BitWord mask = ((static_cast<BitWord>(1) << endBit) - 1);
    if (value)
      data[endIdx] |= mask;
    else
      data[endIdx] &= ~mask;
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::ZoneStackBase - Init / Reset]
// ============================================================================

Error ZoneStackBase::_init(ZoneAllocator* allocator, size_t middleIndex) noexcept {
  ZoneAllocator* oldAllocator = _allocator;

  if (oldAllocator) {
    Block* block = _block[kSideLeft];
    while (block) {
      Block* next = block->getNext();
      oldAllocator->release(block, kBlockSize);
      block = next;
    }

    _allocator = nullptr;
    _block[kSideLeft] = nullptr;
    _block[kSideRight] = nullptr;
  }

  if (allocator) {
    Block* block = static_cast<Block*>(allocator->alloc(kBlockSize));
    if (ASMJIT_UNLIKELY(!block))
      return DebugUtils::errored(kErrorNoHeapMemory);

    block->_link[kSideLeft] = nullptr;
    block->_link[kSideRight] = nullptr;
    block->_start = (uint8_t*)block + middleIndex;
    block->_end = (uint8_t*)block + middleIndex;

    _allocator = allocator;
    _block[kSideLeft] = block;
    _block[kSideRight] = block;
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::ZoneStackBase - Ops]
// ============================================================================

Error ZoneStackBase::_prepareBlock(uint32_t side, size_t initialIndex) noexcept {
  ASMJIT_ASSERT(isInitialized());

  Block* prev = _block[side];
  ASMJIT_ASSERT(!prev->isEmpty());

  Block* block = _allocator->allocT<Block>(kBlockSize);
  if (ASMJIT_UNLIKELY(!block))
    return DebugUtils::errored(kErrorNoHeapMemory);

  block->_link[ side] = nullptr;
  block->_link[!side] = prev;
  block->_start = (uint8_t*)block + initialIndex;
  block->_end = (uint8_t*)block + initialIndex;

  prev->_link[side] = block;
  _block[side] = block;

  return kErrorOk;
}

void ZoneStackBase::_cleanupBlock(uint32_t side, size_t middleIndex) noexcept {
  Block* block = _block[side];
  ASMJIT_ASSERT(block->isEmpty());

  Block* prev = block->_link[!side];
  if (prev) {
    ASMJIT_ASSERT(prev->_link[side] == block);
    _allocator->release(block, kBlockSize);

    prev->_link[side] = nullptr;
    _block[side] = prev;
  }
  else if (_block[!side] == prev && prev->isEmpty()) {
    // If the container becomes empty center both pointers in the remaining block.
    prev->_start = (uint8_t*)prev + middleIndex;
    prev->_end = (uint8_t*)prev + middleIndex;
  }
}

// ============================================================================
// [asmjit::ZoneHashBase - Utilities]
// ============================================================================

static uint32_t ZoneHash_getClosestPrime(uint32_t x) noexcept {
  static const uint32_t primeTable[] = {
    23, 53, 193, 389, 769, 1543, 3079, 6151, 12289, 24593
  };

  size_t i = 0;
  uint32_t p;

  do {
    if ((p = primeTable[i]) > x)
      break;
  } while (++i < ASMJIT_ARRAY_SIZE(primeTable));

  return p;
}

// ============================================================================
// [asmjit::ZoneHashBase - Reset]
// ============================================================================

void ZoneHashBase::reset(ZoneAllocator* allocator) noexcept {
  ZoneHashNode** oldData = _data;
  if (oldData != _embedded)
    _allocator->release(oldData, _bucketsCount * sizeof(ZoneHashNode*));

  _allocator = allocator;
  _size = 0;
  _bucketsCount = 1;
  _bucketsGrow = 1;
  _data = _embedded;
  _embedded[0] = nullptr;
}

// ============================================================================
// [asmjit::ZoneHashBase - Rehash]
// ============================================================================

void ZoneHashBase::_rehash(uint32_t newCount) noexcept {
  ASMJIT_ASSERT(isInitialized());

  ZoneHashNode** oldData = _data;
  ZoneHashNode** newData = reinterpret_cast<ZoneHashNode**>(
    _allocator->allocZeroed(static_cast<size_t>(newCount) * sizeof(ZoneHashNode*)));

  // We can still store nodes into the table, but it will degrade.
  if (ASMJIT_UNLIKELY(newData == nullptr))
    return;

  uint32_t i;
  uint32_t oldCount = _bucketsCount;

  for (i = 0; i < oldCount; i++) {
    ZoneHashNode* node = oldData[i];
    while (node) {
      ZoneHashNode* next = node->_hashNext;
      uint32_t hMod = node->_hVal % newCount;

      node->_hashNext = newData[hMod];
      newData[hMod] = node;

      node = next;
    }
  }

  // 90% is the maximum occupancy, can't overflow since the maximum capacity
  // is limited to the last prime number stored in the prime table.
  if (oldData != _embedded)
    _allocator->release(oldData, _bucketsCount * sizeof(ZoneHashNode*));

  _bucketsCount = newCount;
  _bucketsGrow = newCount * 9 / 10;

  _data = newData;
}

// ============================================================================
// [asmjit::ZoneHashBase - Ops]
// ============================================================================

ZoneHashNode* ZoneHashBase::_put(ZoneHashNode* node) noexcept {
  uint32_t hMod = node->_hVal % _bucketsCount;
  ZoneHashNode* next = _data[hMod];

  node->_hashNext = next;
  _data[hMod] = node;

  if (++_size >= _bucketsGrow && next) {
    uint32_t newCapacity = ZoneHash_getClosestPrime(_bucketsCount);
    if (newCapacity != _bucketsCount)
      _rehash(newCapacity);
  }

  return node;
}

ZoneHashNode* ZoneHashBase::_del(ZoneHashNode* node) noexcept {
  uint32_t hMod = node->_hVal % _bucketsCount;

  ZoneHashNode** pPrev = &_data[hMod];
  ZoneHashNode* p = *pPrev;

  while (p) {
    if (p == node) {
      *pPrev = p->_hashNext;
      return node;
    }

    pPrev = &p->_hashNext;
    p = *pPrev;
  }

  return nullptr;
}

// ============================================================================
// [asmjit::Zone - Test]
// ============================================================================

#if defined(ASMJIT_TEST)
UNIT(base_zone_bit_vector) {
  Zone zone(8096 - Zone::kZoneOverhead);
  ZoneAllocator allocator(&zone);

  uint32_t i, count;
  uint32_t kMaxCount = 100;

  ZoneBitVector vec;
  EXPECT(vec.isEmpty());
  EXPECT(vec.getLength() == 0);

  INFO("ZoneBitVector::resize()");
  for (count = 1; count < kMaxCount; count++) {
    vec.clear();
    EXPECT(vec.resize(&allocator, count, false) == kErrorOk);
    EXPECT(vec.getLength() == count);

    for (i = 0; i < count; i++)
      EXPECT(vec.getAt(i) == false);

    vec.clear();
    EXPECT(vec.resize(&allocator, count, true) == kErrorOk);
    EXPECT(vec.getLength() == count);

    for (i = 0; i < count; i++)
      EXPECT(vec.getAt(i) == true);
  }

  INFO("ZoneBitVector::fill()");
  for (count = 1; count < kMaxCount; count += 2) {
    vec.clear();
    EXPECT(vec.resize(&allocator, count) == kErrorOk);
    EXPECT(vec.getLength() == count);

    for (i = 0; i < (count + 1) / 2; i++) {
      bool value = static_cast<bool>(i & 1);
      EXPECT(vec.fill(i, count - i, value) == kErrorOk);
    }

    for (i = 0; i < count; i++) {
      EXPECT(vec.getAt(i) == static_cast<bool>(i & 1));
    }
  }
}

UNIT(base_zone_int_vector) {
  Zone zone(8096 - Zone::kZoneOverhead);
  ZoneAllocator allocator(&zone);

  int i;
  int kMax = 100000;

  ZoneVector<int> vec;

  INFO("ZoneVector<int> basic tests");
  EXPECT(vec.append(&allocator, 0) == kErrorOk);
  EXPECT(vec.isEmpty() == false);
  EXPECT(vec.getLength() == 1);
  EXPECT(vec.getCapacity() >= 1);
  EXPECT(vec.indexOf(0) == 0);
  EXPECT(vec.indexOf(-11) == Globals::kNotFound);

  vec.clear();
  EXPECT(vec.isEmpty());
  EXPECT(vec.getLength() == 0);
  EXPECT(vec.indexOf(0) == Globals::kNotFound);

  for (i = 0; i < kMax; i++) {
    EXPECT(vec.append(&allocator, i) == kErrorOk);
  }
  EXPECT(vec.isEmpty() == false);
  EXPECT(vec.getLength() == static_cast<uint32_t>(kMax));
  EXPECT(vec.indexOf(kMax - 1) == static_cast<uint32_t>(kMax - 1));
}

UNIT(base_zone_stack) {
  Zone zone(8096 - Zone::kZoneOverhead);
  ZoneAllocator allocator(&zone);
  ZoneStack<int> stack;

  INFO("ZoneStack<int> contains %d elements per one Block", ZoneStack<int>::kNumBlockItems);

  EXPECT(stack.init(&allocator) == kErrorOk);
  EXPECT(stack.isEmpty(), "Stack must be empty after `init()`");

  EXPECT(stack.append(42) == kErrorOk);
  EXPECT(!stack.isEmpty() , "Stack must not be empty after an item has been appended");
  EXPECT(stack.pop() == 42, "Stack.pop() must return the item that has been appended last");
  EXPECT(stack.isEmpty()  , "Stack must be empty after the last element has been removed");

  EXPECT(stack.prepend(43) == kErrorOk);
  EXPECT(!stack.isEmpty()      , "Stack must not be empty after an item has been prepended");
  EXPECT(stack.popFirst() == 43, "Stack.popFirst() must return the item that has been prepended last");
  EXPECT(stack.isEmpty()       , "Stack must be empty after the last element has been removed");

  int i;
  int iMin =-100;
  int iMax = 100000;

  INFO("Adding items from %d to %d to the stack", iMin, iMax);
  for (i = 1; i <= iMax; i++) stack.append(i);
  for (i = 0; i >= iMin; i--) stack.prepend(i);

  INFO("Validating popFirst()");
  for (i = iMin; i <= iMax; i++) {
    int item = stack.popFirst();
    EXPECT(i == item, "Item '%d' didn't match the item '%d' popped", i, item);
  }
  EXPECT(stack.isEmpty());

  INFO("Adding items from %d to %d to the stack", iMin, iMax);
  for (i = 0; i >= iMin; i--) stack.prepend(i);
  for (i = 1; i <= iMax; i++) stack.append(i);

  INFO("Validating pop()");
  for (i = iMax; i >= iMin; i--) {
    int item = stack.pop();
    EXPECT(i == item, "Item '%d' didn't match the item '%d' popped", i, item);
  }
  EXPECT(stack.isEmpty());
}
#endif

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
