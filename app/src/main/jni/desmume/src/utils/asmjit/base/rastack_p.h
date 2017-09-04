// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_RASTACK_P_H
#define _ASMJIT_BASE_RASTACK_P_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/radefs_p.h"
#include "../base/rastack_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_ra
//! \{

// ============================================================================
// [asmjit::RAStackSlot]
// ============================================================================

//! Stack slot.
struct RAStackSlot {
  ASMJIT_ENUM(Flags) {
    kIsRegHome = 0x01
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isRegHome() const noexcept { return (flags & kIsRegHome) != 0; }
  ASMJIT_INLINE void addUsage(uint32_t count = 1) noexcept { usage += count; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t size;                         //!< Size of memory required by the slot.
  uint32_t alignment;                    //!< Minimum alignment required by the slot.
  uint32_t flags;                        //!< Slot flags.
  uint32_t usage;                        //!< Usage counter (one unit equals one memory operation).

  uint32_t weight;                       //!< Weight of the slot (calculated by `calculateStackFrame()`).
  int32_t offset;                        //!< Stack offset (calculated by `calculateStackFrame()`).
};

typedef ZoneVector<RAStackSlot*> RAStackSlots;

// ============================================================================
// [asmjit::RAStackAllocator]
// ============================================================================

//! Stack allocator.
struct RAStackAllocator {
  enum Size {
    kSize1     = 0,
    kSize2     = 1,
    kSize4     = 2,
    kSize8     = 3,
    kSize16    = 4,
    kSize32    = 5,
    kSize64    = 6,
    kSizeCount = 7
  };

  ASMJIT_INLINE RAStackAllocator() noexcept
    : _allocator(nullptr),
      _bytesUsed(0),
      _stackSize(0),
      _alignment(1),
      _slots() {}

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset(ZoneAllocator* allocator) noexcept {
    _allocator = allocator;
    _bytesUsed = 0;
    _stackSize = 0;
    _alignment = 1;
    _slots.reset();
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE ZoneAllocator* getAllocator() const noexcept { return _allocator; }

  ASMJIT_INLINE uint32_t getBytesUsed() const noexcept { return _bytesUsed; }
  ASMJIT_INLINE uint32_t getStackSize() const noexcept { return _stackSize; }
  ASMJIT_INLINE uint32_t getAlignment() const noexcept { return _alignment; }

  ASMJIT_INLINE RAStackSlots& getSlots() noexcept { return _slots; }
  ASMJIT_INLINE const RAStackSlots& getSlots() const noexcept { return _slots; }

  ASMJIT_INLINE uint32_t getSlotCount() const noexcept { return _slots.getLength(); }

  // --------------------------------------------------------------------------
  // [Slots]
  // --------------------------------------------------------------------------

  RAStackSlot* newSlot(uint32_t size, uint32_t alignment, uint32_t flags = 0) noexcept;

  // --------------------------------------------------------------------------
  // [Calculation]
  // --------------------------------------------------------------------------

  Error calculateStackFrame() noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ZoneAllocator* _allocator;             //!< Allocator used to allocate internal data.
  uint32_t _bytesUsed;                   //!< Count of bytes used by all slots.
  uint32_t _stackSize;                   //!< Calculated stack size (can be a bit greater than `_bytesUsed`).
  uint32_t _alignment;                   //!< Minimum stack alignment.
  RAStackSlots _slots;                   //!< Stack slots vector.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_RASTACK_P_H
