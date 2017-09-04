// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_ZONE_H
#define _ASMJIT_BASE_ZONE_H

// [Dependencies]
#include "../base/intutils.h"
#include "../base/utils.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::Zone]
// ============================================================================

//! Zone memory.
//!
//! Zone is an incremental memory allocator that allocates memory by simply
//! incrementing a pointer. It allocates blocks of memory by using C's `malloc()`,
//! but divides these blocks into smaller segments requested by calling
//! `Zone::alloc()` and friends.
//!
//! Zone has no function to release the allocated memory. It has to be released
//! all at once by calling `reset()`. If you need a more friendly allocator that
//! also supports `release()`, consider using \ref Zone with \ref ZoneAllocator.
class Zone {
public:
  //! \internal
  //!
  //! A single block of memory.
  struct Block {
    Block* prev;                         //!< Link to the previous block.
    Block* next;                         //!< Link to the next block.
    size_t size;                         //!< Size of the block.
    uint8_t data[sizeof(void*)];         //!< Data.
  };

  enum {
    //! Zone allocator overhead.
    kZoneOverhead = Globals::kAllocOverhead + static_cast<int>(sizeof(Block))
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new instance of `Zone` allocator.
  //!
  //! The `blockSize` parameter describes the default size of the block. If the
  //! `size` parameter passed to `alloc()` is greater than the default size
  //! `Zone` will allocate and use a larger block, but it will not change the
  //! default `blockSize`.
  //!
  //! It's not required, but it's good practice to set `blockSize` to a
  //! reasonable value that depends on the usage of `Zone`. Greater block sizes
  //! are generally safer and perform better than unreasonably low values.
  ASMJIT_API Zone(uint32_t blockSize, uint32_t blockAlignment = 0) noexcept;

  //! Destroy the `Zone` instance.
  //!
  //! This will destroy the `Zone` instance and release all blocks of memory
  //! allocated by it. It performs implicit `reset(true)`.
  ASMJIT_API ~Zone() noexcept;

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset the `Zone` invalidating all blocks allocated.
  //!
  //! If `releaseMemory` is true all buffers will be released to the system.
  ASMJIT_API void reset(bool releaseMemory = false) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the default block size.
  ASMJIT_INLINE uint32_t getBlockSize() const noexcept { return _blockSize; }
  //! Get the default block alignment.
  ASMJIT_INLINE uint32_t getBlockAlignment() const noexcept { return (uint32_t)1 << _blockAlignmentShift; }
  //! Get remaining size of the current block.
  ASMJIT_INLINE size_t getRemainingSize() const noexcept { return (size_t)(_end - _ptr); }

  //! Get the current zone cursor (dangerous).
  //!
  //! This is a function that can be used to get exclusive access to the current
  //! block's memory buffer.
  ASMJIT_INLINE uint8_t* getCursor() noexcept { return _ptr; }
  //! Get the end of the current zone block, only useful if you use `getCursor()`.
  ASMJIT_INLINE uint8_t* getEnd() noexcept { return _end; }

  //! Set the current zone cursor to `p` (must match the current block).
  ASMJIT_INLINE void setCursor(uint8_t* p) noexcept {
    ASMJIT_ASSERT(p >= _ptr && p <= _end);
    _ptr = p;
  }

  // --------------------------------------------------------------------------
  // [Align the current pointer to `alignment` and return it]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint8_t* align(size_t alignment) noexcept {
    _ptr = std::min<uint8_t*>(IntUtils::alignTo(_ptr, alignment), _end);
    ASMJIT_ASSERT(_ptr >= _block->data && _ptr <= _end);
    return _ptr;
  }

  // --------------------------------------------------------------------------
  // [Alloc]
  // --------------------------------------------------------------------------

  //! Allocate `size` bytes of memory.
  //!
  //! Pointer returned is valid until the `Zone` instance is destroyed or reset
  //! by calling `reset()`. If you plan to make an instance of C++ from the
  //! given pointer use placement `new` and `delete` operators:
  //!
  //! ~~~
  //! using namespace asmjit;
  //!
  //! class Object { ... };
  //!
  //! // Create Zone with default block size of approximately 65536 bytes.
  //! Zone zone(65536 - Zone::kZoneOverhead);
  //!
  //! // Create your objects using zone object allocating, for example:
  //! Object* obj = static_cast<Object*>( zone.alloc(sizeof(Object)) );
  //
  //! if (!obj) {
  //!   // Handle out of memory error.
  //! }
  //!
  //! // Placement `new` and `delete` operators can be used to instantiate it.
  //! new(obj) Object();
  //!
  //! // ... lifetime of your objects ...
  //!
  //! // To destroy the instance (if required).
  //! obj->~Object();
  //!
  //! // Reset or destroy `Zone`.
  //! zone.reset();
  //! ~~~
  ASMJIT_INLINE void* alloc(size_t size) noexcept {
    uint8_t* ptr = _ptr;
    size_t remainingBytes = (size_t)(_end - ptr);

    if (ASMJIT_UNLIKELY(remainingBytes < size))
      return _alloc(size);

    _ptr += size;
    ASMJIT_ASSERT(_ptr <= _end);

    return static_cast<void*>(ptr);
  }

  //! Allocate `size` bytes of memory aligned to the given `alignment`.
  ASMJIT_INLINE void* allocAligned(size_t size, size_t alignment) noexcept {
    align(alignment);
    return alloc(size);
  }

  //! Allocate `size` bytes without any checks.
  //!
  //! Can only be called if `getRemainingSize()` returns size at least equal
  //! to `size`.
  ASMJIT_INLINE void* allocNoCheck(size_t size) noexcept {
    ASMJIT_ASSERT((size_t)(_end - _ptr) >= size);

    uint8_t* ptr = _ptr;
    _ptr += size;
    return static_cast<void*>(ptr);
  }

  //! Allocate `size` bytes of zeroed memory.
  //!
  //! See \ref alloc() for more details.
  ASMJIT_API void* allocZeroed(size_t size) noexcept;

  //! Like `alloc()`, but the return pointer is casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* allocT(size_t size = sizeof(T)) noexcept {
    return static_cast<T*>(alloc(size));
  }

  //! Like `alloc()`, but the return pointer is casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* allocAlignedT(size_t size, size_t alignment) noexcept {
    return static_cast<T*>(allocAligned(size, alignment));
  }

  //! Like `allocNoCheck()`, but the return pointer is casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* allocNoCheckT(size_t size = sizeof(T)) noexcept {
    return static_cast<T*>(allocNoCheck(size));
  }

  //! Like `allocZeroed()`, but the return pointer is casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* allocZeroedT(size_t size = sizeof(T)) noexcept {
    return static_cast<T*>(allocZeroed(size));
  }

  //! Like `new(std::nothrow) T(...)`, but allocated by `Zone`.
  template<typename T>
  ASMJIT_INLINE T* newT() noexcept {
    void* p = alloc(sizeof(T));
    if (ASMJIT_UNLIKELY(!p))
      return nullptr;
    return new(p) T();
  }
  //! Like `new(std::nothrow) T(...)`, but allocated by `Zone`.
  template<typename T, typename P1>
  ASMJIT_INLINE T* newT(P1 p1) noexcept {
    void* p = alloc(sizeof(T));
    if (ASMJIT_UNLIKELY(!p))
      return nullptr;
    return new(p) T(p1);
  }

  //! \internal
  ASMJIT_API void* _alloc(size_t size) noexcept;

  //! Helper to duplicate data.
  ASMJIT_API void* dup(const void* data, size_t size, bool nullTerminate = false) noexcept;

  //! Helper to duplicate a formatted string, maximum length is 256 bytes.
  ASMJIT_API char* sformat(const char* str, ...) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t* _ptr;                         //!< Pointer in the current block's buffer.
  uint8_t* _end;                         //!< End of the current block's buffer.
  Block* _block;                         //!< Current block.

#if ASMJIT_ARCH_64BIT
  uint32_t _blockSize;                   //!< Default size of a newly allocated block.
  uint32_t _blockAlignmentShift;         //!< Minimum alignment of each block.
#else
  uint32_t _blockSize : 29;              //!< Default size of a newly allocated block.
  uint32_t _blockAlignmentShift : 3;     //!< Minimum alignment of each block.
#endif
};

// ============================================================================
// [asmjit::ZoneAllocator]
// ============================================================================

//! Zone-based memory allocator that uses an existing \ref Zone and provides
//! a `release()` functionality on top of it. It uses \ref Zone only for chunks
//! that can be pooled, and uses libc `malloc()` for chunks that are large.
//!
//! The advantage of ZoneAllocator is that it can allocate small chunks of memory
//! really fast, and these chunks, when released, will be reused by consecutive
//! calls to `alloc()`. Also, since ZoneAllocator uses \ref Zone, you can turn
//! any \ref Zone into a \ref ZoneAllocator, and use it in your \ref Pass when
//! necessary.
//!
//! ZoneAllocator is used by AsmJit containers to make containers having only
//! few elements fast (and lightweight) and to allow them to grow and use
//! dynamic blocks when require more storage.
class ZoneAllocator {
public:
  ASMJIT_NONCOPYABLE(ZoneAllocator)

  enum {
    // In short, we pool chunks of these sizes:
    //   [32, 64, 96, 128, 192, 256, 320, 384, 448, 512]

    //! How many bytes per a low granularity pool (has to be at least 16).
    kLoGranularity = 32,
    //! Number of slots of a low granularity pool.
    kLoCount = 4,
    //! Maximum size of a block that can be allocated in a low granularity pool.
    kLoMaxSize = kLoGranularity * kLoCount,

    //! How many bytes per a high granularity pool.
    kHiGranularity = 64,
    //! Number of slots of a high granularity pool.
    kHiCount = 6,
    //! Maximum size of a block that can be allocated in a high granularity pool.
    kHiMaxSize = kLoMaxSize + kHiGranularity * kHiCount,

    //! Alignment of every pointer returned by `alloc()`.
    kBlockAlignment = kLoGranularity
  };

  //! Single-linked list used to store unused chunks.
  struct Slot {
    //! Link to a next slot in a single-linked list.
    Slot* next;
  };

  //! A block of memory that has been allocated dynamically and is not part of
  //! block-list used by the allocator. This is used to keep track of all these
  //! blocks so they can be freed by `reset()` if not freed explicitly.
  struct DynamicBlock {
    DynamicBlock* prev;
    DynamicBlock* next;
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `ZoneAllocator`.
  //!
  //! NOTE: To use it, you must first `init()` it.
  ASMJIT_INLINE ZoneAllocator() noexcept {
    ::memset(this, 0, sizeof(*this));
  }
  //! Create a new `ZoneAllocator` initialized to use `zone`.
  explicit ASMJIT_INLINE ZoneAllocator(Zone* zone) noexcept {
    ::memset(this, 0, sizeof(*this));
    _zone = zone;
  }
  //! Destroy the `ZoneAllocator`.
  ASMJIT_INLINE ~ZoneAllocator() noexcept { reset(); }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  //! Get if the `ZoneAllocator` is initialized (i.e. has `Zone`).
  ASMJIT_INLINE bool isInitialized() const noexcept { return _zone != nullptr; }

  //! Convenience method to initialize the `ZoneAllocator` with `zone`.
  //!
  //! It's the same as calling `reset(zone)`.
  ASMJIT_INLINE void init(Zone* zone) noexcept { reset(zone); }

  //! Reset this `ZoneAllocator` and also forget about the current `Zone` which
  //! is attached (if any). Reset optionally attaches a new `zone` passed, or
  //! keeps the `ZoneAllocator` in an uninitialized state, if `zone` is null.
  ASMJIT_API void reset(Zone* zone = nullptr) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the `Zone` the allocator is using, or null if it's not initialized.
  ASMJIT_INLINE Zone* getZone() const noexcept { return _zone; }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Get the slot index to be used for `size`. Returns `true` if a valid slot
  //! has been written to `slot` and `allocatedSize` has been filled with slot
  //! exact size (`allocatedSize` can be equal or slightly greater than `size`).
  static ASMJIT_INLINE bool _getSlotIndex(size_t size, uint32_t& slot) noexcept {
    ASMJIT_ASSERT(size > 0);
    if (size > kHiMaxSize)
      return false;

    if (size <= kLoMaxSize)
      slot = static_cast<uint32_t>((size - 1) / kLoGranularity);
    else
      slot = static_cast<uint32_t>((size - kLoMaxSize - 1) / kHiGranularity) + kLoCount;

    return true;
  }

  //! \overload
  static ASMJIT_INLINE bool _getSlotIndex(size_t size, uint32_t& slot, size_t& allocatedSize) noexcept {
    ASMJIT_ASSERT(size > 0);
    if (size > kHiMaxSize)
      return false;

    if (size <= kLoMaxSize) {
      slot = static_cast<uint32_t>((size - 1) / kLoGranularity);
      allocatedSize = IntUtils::alignTo(size, kLoGranularity);
    }
    else {
      slot = static_cast<uint32_t>((size - kLoMaxSize - 1) / kHiGranularity) + kLoCount;
      allocatedSize = IntUtils::alignTo(size, kHiGranularity);
    }

    return true;
  }

  // --------------------------------------------------------------------------
  // [Alloc / Release]
  // --------------------------------------------------------------------------

  ASMJIT_API void* _alloc(size_t size, size_t& allocatedSize) noexcept;
  ASMJIT_API void* _allocZeroed(size_t size, size_t& allocatedSize) noexcept;
  ASMJIT_API void _releaseDynamic(void* p, size_t size) noexcept;

  //! Allocate `size` bytes of memory, ideally from an available pool.
  //!
  //! NOTE: `size` can't be zero, it will assert in debug mode in such case.
  ASMJIT_INLINE void* alloc(size_t size) noexcept {
    ASMJIT_ASSERT(isInitialized());
    size_t allocatedSize;
    return _alloc(size, allocatedSize);
  }

  //! Like `alloc(size)`, but provides a second argument `allocatedSize` that
  //! provides a way to know how big the block returned actually is. This is
  //! useful for containers to prevent growing too early.
  ASMJIT_INLINE void* alloc(size_t size, size_t& allocatedSize) noexcept {
    ASMJIT_ASSERT(isInitialized());
    return _alloc(size, allocatedSize);
  }

  //! Like `alloc()`, but the return pointer is casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* allocT(size_t size = sizeof(T)) noexcept {
    return static_cast<T*>(alloc(size));
  }

  //! Like `alloc(size)`, but returns zeroed memory.
  ASMJIT_INLINE void* allocZeroed(size_t size) noexcept {
    ASMJIT_ASSERT(isInitialized());

    size_t allocatedSize;
    return _allocZeroed(size, allocatedSize);
  }

  //! Like `alloc(size, allocatedSize)`, but returns zeroed memory.
  ASMJIT_INLINE void* allocZeroed(size_t size, size_t& allocatedSize) noexcept {
    ASMJIT_ASSERT(isInitialized());

    return _allocZeroed(size, allocatedSize);
  }

  //! Like `allocZeroed()`, but the return pointer is casted to `T*`.
  template<typename T>
  ASMJIT_INLINE T* allocZeroedT(size_t size = sizeof(T)) noexcept {
    return static_cast<T*>(allocZeroed(size));
  }

  //! Release the memory previously allocated by `alloc()`. The `size` argument
  //! has to be the same as used to call `alloc()` or `allocatedSize` returned
  //! by `alloc()`.
  ASMJIT_INLINE void release(void* p, size_t size) noexcept {
    ASMJIT_ASSERT(isInitialized());

    ASMJIT_ASSERT(p != nullptr);
    ASMJIT_ASSERT(size != 0);

    uint32_t slot;
    if (_getSlotIndex(size, slot)) {
      //printf("RELEASING %p of size %d (SLOT %u)\n", p, int(size), slot);
      static_cast<Slot*>(p)->next = static_cast<Slot*>(_slots[slot]);
      _slots[slot] = static_cast<Slot*>(p);
    }
    else {
      _releaseDynamic(p, size);
    }
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Zone* _zone;                           //!< Zone used to allocate memory that fits into slots.
  Slot* _slots[kLoCount + kHiCount];     //!< Indexed slots containing released memory.
  DynamicBlock* _dynamicBlocks;          //!< Dynamic blocks for larger allocations (no slots).
};

// ============================================================================
// [asmjit::ZoneList<T>]
// ============================================================================

//! \internal
template <typename T>
class ZoneList {
public:
  ASMJIT_NONCOPYABLE(ZoneList<T>)

  // --------------------------------------------------------------------------
  // [Link]
  // --------------------------------------------------------------------------

  //! ZoneList node.
  struct Link {
    //! Get next node.
    ASMJIT_INLINE Link* getNext() const noexcept { return _next; }
    //! Get value.
    ASMJIT_INLINE T getValue() const noexcept { return _value; }
    //! Set value to `value`.
    ASMJIT_INLINE void setValue(const T& value) noexcept { _value = value; }

    Link* _next;
    T _value;
  };

  // --------------------------------------------------------------------------
  // [Appender]
  // --------------------------------------------------------------------------

  //! Specialized appender that takes advantage of ZoneList structure. You must
  //! initialize it and then call done().
  struct Appender {
    ASMJIT_INLINE Appender(ZoneList<T>& list) noexcept { init(list); }

    ASMJIT_INLINE void init(ZoneList<T>& list) noexcept {
      pPrev = &list._first;
    }

    ASMJIT_INLINE void done(ZoneList<T>& list) noexcept {
      list._last = *pPrev;
      *pPrev = nullptr;
    }

    ASMJIT_INLINE void append(Link* node) noexcept {
      *pPrev = node;
      pPrev = &node->_next;
    }

    Link** pPrev;
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE ZoneList() noexcept : _first(nullptr), _last(nullptr) {}
  ASMJIT_INLINE ~ZoneList() noexcept {}

  // --------------------------------------------------------------------------
  // [Data]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isEmpty() const noexcept { return _first != nullptr; }
  ASMJIT_INLINE Link* getFirst() const noexcept { return _first; }
  ASMJIT_INLINE Link* getLast() const noexcept { return _last; }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset() noexcept {
    _first = nullptr;
    _last = nullptr;
  }

  ASMJIT_INLINE void prepend(Link* link) noexcept {
    link->_next = _first;
    if (!_first) _last = link;
    _first = link;
  }

  ASMJIT_INLINE void append(Link* link) noexcept {
    link->_next = nullptr;
    if (!_first)
      _first = link;
    else
      _last->_next = link;
    _last = link;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Link* _first;
  Link* _last;
};

// ============================================================================
// [asmjit::ZoneBitVector]
// ============================================================================

class ZoneBitVector {
public:
  ASMJIT_NONCOPYABLE(ZoneBitVector)

  typedef Globals::BitWord BitWord;
  enum { kBitWordSize = Globals::kBitWordSize };

  static ASMJIT_INLINE uint32_t _wordsPerBits(uint32_t nBits) noexcept {
    return ((nBits + kBitWordSize - 1) / kBitWordSize);
  }

  // Return all bits zero if 0 and all bits set if 1.
  static ASMJIT_INLINE BitWord _patternFromBit(bool bit) noexcept {
    BitWord bitAsWord = static_cast<BitWord>(bit);
    ASMJIT_ASSERT(bitAsWord == 0 || bitAsWord == 1);
    return static_cast<BitWord>(0) - bitAsWord;
  }

  static ASMJIT_INLINE void _zeroBits(BitWord* dst, uint32_t nBitWords) noexcept {
    for (uint32_t i = 0; i < nBitWords; i++)
      dst[i] = 0;
  }

  static ASMJIT_INLINE void _copyBits(BitWord* dst, const BitWord* src, uint32_t nBitWords) noexcept {
    for (uint32_t i = 0; i < nBitWords; i++)
      dst[i] = src[i];
  }

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  explicit ASMJIT_INLINE ZoneBitVector() noexcept
    : _data(nullptr),
      _length(0),
      _capacity(0) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get if the bit-vector is empty (has no bits).
  ASMJIT_INLINE bool isEmpty() const noexcept { return _length == 0; }
  //! Get a length of this bit-vector (in bits).
  ASMJIT_INLINE uint32_t getLength() const noexcept { return _length; }
  //! Get a capacity of this bit-vector (in bits).
  ASMJIT_INLINE uint32_t getCapacity() const noexcept { return _capacity; }

  //! Get a count of `BitWord[]` array need to store all bits.
  ASMJIT_INLINE uint32_t getBitWordLength() const noexcept { return _wordsPerBits(_length); }
  //! Get a count of `BitWord[]` array need to store all bits.
  ASMJIT_INLINE uint32_t getBitWordCapacity() const noexcept { return _wordsPerBits(_capacity); }

  //! Get data.
  ASMJIT_INLINE BitWord* getData() noexcept { return _data; }
  //! \overload
  ASMJIT_INLINE const BitWord* getData() const noexcept { return _data; }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void clear() noexcept {
    _length = 0;
  }

  ASMJIT_INLINE void reset() noexcept {
    _data = nullptr;
    _length = 0;
    _capacity = 0;
  }

  ASMJIT_INLINE void truncate(uint32_t newLength) noexcept {
    _length = std::min(_length, newLength);
    _clearUnusedBits();
  }

  ASMJIT_INLINE bool getAt(uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < _length);

    uint32_t idx = index / kBitWordSize;
    uint32_t bit = index % kBitWordSize;
    return static_cast<bool>((_data[idx] >> bit) & 1);
  }

  ASMJIT_INLINE void setAt(uint32_t index, bool value) noexcept {
    ASMJIT_ASSERT(index < _length);

    uint32_t idx = index / kBitWordSize;
    uint32_t bit = index % kBitWordSize;
    if (value)
      _data[idx] |= static_cast<BitWord>(1) << bit;
    else
      _data[idx] &= ~(static_cast<BitWord>(1) << bit);
  }

  ASMJIT_INLINE void toggleAt(uint32_t index) noexcept {
    ASMJIT_ASSERT(index < _length);

    uint32_t idx = index / kBitWordSize;
    uint32_t bit = index % kBitWordSize;
    _data[idx] ^= static_cast<BitWord>(1) << bit;
  }

  ASMJIT_INLINE Error append(ZoneAllocator* allocator, bool value) noexcept {
    uint32_t index = _length;
    if (ASMJIT_UNLIKELY(index >= _capacity))
      return _append(allocator, value);

    uint32_t idx = index / kBitWordSize;
    uint32_t bit = index % kBitWordSize;

    if (bit == 0)
      _data[idx] = static_cast<BitWord>(value) << bit;
    else
      _data[idx] |= static_cast<BitWord>(value) << bit;

    _length++;
    return kErrorOk;
  }

  ASMJIT_API Error copyFrom(ZoneAllocator* allocator, const ZoneBitVector& other) noexcept;
  ASMJIT_API Error fill(uint32_t fromIndex, uint32_t toIndex, bool value) noexcept;
  ASMJIT_INLINE void zero() noexcept { _zeroBits(_data, _wordsPerBits(_length)); }

  ASMJIT_INLINE void and_(const ZoneBitVector& other) noexcept {
    BitWord* dst = _data;
    const BitWord* src = other._data;

    uint32_t numWords = (std::min(_length, other._length) + kBitWordSize - 1) / kBitWordSize;
    for (uint32_t i = 0; i < numWords; i++)
      dst[i] = dst[i] & src[i];
    _clearUnusedBits();
  }

  ASMJIT_INLINE void andNot(const ZoneBitVector& other) noexcept {
    BitWord* dst = _data;
    const BitWord* src = other._data;

    uint32_t numWords = _wordsPerBits(std::min(_length, other._length));
    for (uint32_t i = 0; i < numWords; i++)
      dst[i] = dst[i] & ~src[i];
    _clearUnusedBits();
  }

  ASMJIT_INLINE void or_(const ZoneBitVector& other) noexcept {
    BitWord* dst = _data;
    const BitWord* src = other._data;

    uint32_t numWords = _wordsPerBits(std::min(_length, other._length));
    for (uint32_t i = 0; i < numWords; i++)
      dst[i] = dst[i] | src[i];
    _clearUnusedBits();
  }

  ASMJIT_INLINE void _clearUnusedBits() noexcept {
    uint32_t idx = _length / kBitWordSize;
    uint32_t bit = _length % kBitWordSize;

    if (!bit) return;
    _data[idx] &= (static_cast<BitWord>(1) << bit) - 1U;
  }

  ASMJIT_INLINE bool eq(const ZoneBitVector& other) const noexcept {
    uint32_t len = _length;

    if (len != other._length)
      return false;

    const BitWord* aData = _data;
    const BitWord* bData = other._data;

    uint32_t numBitWords = _wordsPerBits(len);
    for (uint32_t i = 0; i < numBitWords; i++) {
      if (aData[i] != bData[i])
        return false;
    }
    return true;
  }

  ASMJIT_INLINE bool operator==(const ZoneBitVector& other) const noexcept { return  eq(other); }
  ASMJIT_INLINE bool operator!=(const ZoneBitVector& other) const noexcept { return !eq(other); }

  // --------------------------------------------------------------------------
  // [Memory Management]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void release(ZoneAllocator* allocator) noexcept {
    if (!_data)
      return;
    allocator->release(_data, _capacity / 8);
    reset();
  }

  ASMJIT_INLINE Error resize(ZoneAllocator* allocator, uint32_t newLength, bool newBitsValue = false) noexcept {
    return _resize(allocator, newLength, newLength, newBitsValue);
  }

  ASMJIT_API Error _resize(ZoneAllocator* allocator, uint32_t newLength, uint32_t idealCapacity, bool newBitsValue) noexcept;
  ASMJIT_API Error _append(ZoneAllocator* allocator, bool value) noexcept;

  // --------------------------------------------------------------------------
  // [Iterators]
  // --------------------------------------------------------------------------

  class ForEachBitSet : public IntUtils::BitArrayIterator<BitWord> {
  public:
    explicit ASMJIT_INLINE ForEachBitSet(const ZoneBitVector& bitVector) noexcept
      : IntUtils::BitArrayIterator<BitWord>(bitVector.getData(), bitVector.getBitWordLength()) {}
  };

  template<class Operator>
  class ForEachBitOp : public IntUtils::BitArrayOpIterator<BitWord, Operator> {
  public:
    ASMJIT_INLINE ForEachBitOp(const ZoneBitVector& a, const ZoneBitVector& b) noexcept
      : IntUtils::BitArrayOpIterator<BitWord, Operator>(a.getData(), b.getData(), a.getBitWordLength()) {
      ASMJIT_ASSERT(a.getLength() == b.getLength());
    }
  };

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  BitWord* _data;                        //!< Bits.
  uint32_t _length;                      //!< Length of the bit-vector (in bits).
  uint32_t _capacity;                    //!< Capacity of the bit-vector (in bits).
};

// ============================================================================
// [asmjit::ZoneVectorBase]
// ============================================================================

//! \internal
class ZoneVectorBase {
public:
  ASMJIT_NONCOPYABLE(ZoneVectorBase)

protected:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new instance of `ZoneVectorBase`.
  explicit ASMJIT_INLINE ZoneVectorBase() noexcept
    : _data(nullptr),
      _length(0),
      _capacity(0) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

public:
  //! Get if the vector is empty.
  ASMJIT_INLINE bool isEmpty() const noexcept { return _length == 0; }
  //! Get vector length.
  ASMJIT_INLINE uint32_t getLength() const noexcept { return _length; }
  //! Get vector capacity.
  ASMJIT_INLINE uint32_t getCapacity() const noexcept { return _capacity; }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  //! Makes the vector empty (won't change the capacity or data pointer).
  ASMJIT_INLINE void clear() noexcept { _length = 0; }
  //! Reset the vector data and set its `length` to zero.
  ASMJIT_INLINE void reset() noexcept {
    _data = nullptr;
    _length = 0;
    _capacity = 0;
  }

  //! Truncate the vector to at most `n` items.
  ASMJIT_INLINE void truncate(uint32_t n) noexcept {
    _length = std::min(_length, n);
  }

  //! Set length of the vector to `n`. Used internally by some algorithms.
  ASMJIT_INLINE void _setLength(uint32_t n) noexcept {
    ASMJIT_ASSERT(n <= _capacity);
    _length = n;
  }

  // --------------------------------------------------------------------------
  // [Memory Management]
  // --------------------------------------------------------------------------

protected:
  ASMJIT_INLINE void _release(ZoneAllocator* allocator, uint32_t sizeOfT) noexcept {
    if (_data != nullptr) {
      allocator->release(_data, _capacity * sizeOfT);
      reset();
    }
  }

  ASMJIT_API Error _grow(ZoneAllocator* allocator, uint32_t sizeOfT, uint32_t n) noexcept;
  ASMJIT_API Error _resize(ZoneAllocator* allocator, uint32_t sizeOfT, uint32_t n) noexcept;
  ASMJIT_API Error _reserve(ZoneAllocator* allocator, uint32_t sizeOfT, uint32_t n) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

public:
  void* _data;                           //!< Vector data (untyped).
  uint32_t _length;                      //!< Length of the vector.
  uint32_t _capacity;                    //!< Capacity of the vector.
};

// ============================================================================
// [asmjit::ZoneVector<T>]
// ============================================================================

//! Template used to store and manage array of Zone allocated data.
//!
//! This template has these advantages over other std::vector<>:
//! - Always non-copyable (designed to be non-copyable, we want it).
//! - No copy-on-write (some implementations of STL can use it).
//! - Optimized for working only with POD types.
//! - Uses ZoneAllocator, thus small vectors are basically for free.
template <typename T>
class ZoneVector : public ZoneVectorBase {
public:
  ASMJIT_NONCOPYABLE(ZoneVector<T>)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new instance of `ZoneVector<T>`.
  ASMJIT_INLINE ZoneVector() noexcept : ZoneVectorBase() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get data.
  ASMJIT_INLINE T* getData() noexcept { return static_cast<T*>(_data); }
  //! \overload
  ASMJIT_INLINE const T* getData() const noexcept { return static_cast<const T*>(_data); }

  //! Get item at index `i` (const).
  ASMJIT_INLINE const T& getAt(uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < _length);
    return getData()[i];
  }

  ASMJIT_INLINE void _setEndPtr(T* p) noexcept {
    ASMJIT_ASSERT(p >= getData() && p <= getData() + _capacity);
    _setLength(static_cast<uint32_t>((uintptr_t)(p - getData())));
  }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  //! Prepend `item` to the vector.
  Error prepend(ZoneAllocator* allocator, const T& item) noexcept {
    if (ASMJIT_UNLIKELY(_length == _capacity))
      ASMJIT_PROPAGATE(grow(allocator, 1));

    ::memmove(static_cast<T*>(_data) + 1, _data, static_cast<size_t>(_length) * sizeof(T));
    ::memcpy(_data, &item, sizeof(T));

    _length++;
    return kErrorOk;
  }

  //! Insert an `item` at the specified `index`.
  Error insert(ZoneAllocator* allocator, uint32_t index, const T& item) noexcept {
    ASMJIT_ASSERT(index <= _length);

    if (ASMJIT_UNLIKELY(_length == _capacity))
      ASMJIT_PROPAGATE(grow(allocator, 1));

    T* dst = static_cast<T*>(_data) + index;
    ::memmove(dst + 1, dst, static_cast<size_t>(_length - index) * sizeof(T));
    ::memcpy(dst, &item, sizeof(T));
    _length++;

    return kErrorOk;
  }

  //! Append `item` to the vector.
  Error append(ZoneAllocator* allocator, const T& item) noexcept {
    if (ASMJIT_UNLIKELY(_length == _capacity))
      ASMJIT_PROPAGATE(grow(allocator, 1));

    ::memcpy(static_cast<T*>(_data) + _length, &item, sizeof(T));
    _length++;

    return kErrorOk;
  }

  Error concat(ZoneAllocator* allocator, const ZoneVector<T>& other) noexcept {
    uint32_t count = other._length;
    if (_capacity - _length < count)
      ASMJIT_PROPAGATE(grow(allocator, count));

    if (count) {
      ::memcpy(static_cast<T*>(_data) + _length, other._data, static_cast<size_t>(count) * sizeof(T));
      _length += count;
    }

    return kErrorOk;
  }

  //! Prepend `item` to the vector (unsafe case).
  //!
  //! Can only be used together with `willGrow()`. If `willGrow(N)` returns
  //! `kErrorOk` then N elements can be added to the vector without checking
  //! if there is a place for them. Used mostly internally.
  ASMJIT_INLINE void prependUnsafe(const T& item) noexcept {
    ASMJIT_ASSERT(_length < _capacity);
    T* data = static_cast<T*>(_data);

    if (_length)
      ::memmove(data + 1, data, static_cast<size_t>(_length) * sizeof(T));

    ::memcpy(data, &item, sizeof(T));
    _length++;
  }

  //! Append `item` to the vector (unsafe case).
  //!
  //! Can only be used together with `willGrow()`. If `willGrow(N)` returns
  //! `kErrorOk` then N elements can be added to the vector without checking
  //! if there is a place for them. Used mostly internally.
  ASMJIT_INLINE void appendUnsafe(const T& item) noexcept {
    ASMJIT_ASSERT(_length < _capacity);

    ::memcpy(static_cast<T*>(_data) + _length, &item, sizeof(T));
    _length++;
  }

  //! Concatenate all items of `other` at the end of the vector.
  ASMJIT_INLINE void concatUnsafe(const ZoneVector<T>& other) noexcept {
    uint32_t count = other._length;
    ASMJIT_ASSERT(_capacity - _length >= count);

    if (count) {
      ::memcpy(static_cast<T*>(_data) + _length, other._data, static_cast<size_t>(count) * sizeof(T));
      _length += count;
    }
  }

  //! Get index of `val` or `Globals::kNotFound` if not found.
  ASMJIT_INLINE uint32_t indexOf(const T& val) const noexcept {
    const T* data = static_cast<const T*>(_data);
    uint32_t length = _length;

    for (uint32_t i = 0; i < length; i++)
      if (data[i] == val)
        return i;

    return Globals::kNotFound;
  }

  //! Get whether the vector contains `val`.
  ASMJIT_INLINE bool contains(const T& val) const noexcept {
    return indexOf(val) != Globals::kNotFound;
  }

  //! Remove item at index `i`.
  ASMJIT_INLINE void removeAt(uint32_t i) noexcept {
    ASMJIT_ASSERT(i < _length);

    T* data = static_cast<T*>(_data) + i;
    uint32_t count = --_length - i;
    if (count) ::memmove(data, data + 1, static_cast<size_t>(count) * sizeof(T));
  }

  ASMJIT_INLINE T pop() noexcept {
    ASMJIT_ASSERT(_length > 0);

    uint32_t index = --_length;
    return getData()[index];
  }

  //! Swap this pod-vector with `other`.
  ASMJIT_INLINE void swap(ZoneVector<T>& other) noexcept {
    std::swap(_length, other._length);
    std::swap(_capacity, other._capacity);
    std::swap(_data, other._data);
  }

  template<typename Compare>
  ASMJIT_INLINE void sort(const Compare& cmp) noexcept {
    AsmJitInternal::QSortT<T, Compare>::sort(getData(), getLength(), cmp);
  }

  //! Get item at index `i`.
  ASMJIT_INLINE T& operator[](uint32_t i) noexcept {
    ASMJIT_ASSERT(i < _length);
    return getData()[i];
  }

  //! Get item at index `i`.
  ASMJIT_INLINE const T& operator[](uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < _length);
    return getData()[i];
  }

  ASMJIT_INLINE T& getFirst() noexcept { return operator[](0); }
  ASMJIT_INLINE const T& getFirst() const noexcept { return operator[](0); }

  ASMJIT_INLINE T& getLast() noexcept { return operator[](_length - 1); }
  ASMJIT_INLINE const T& getLast() const noexcept { return operator[](_length - 1); }

  // --------------------------------------------------------------------------
  // [Memory Management]
  // --------------------------------------------------------------------------

  //! Release the memory held by `ZoneVector<T>` back to the `allocator`.
  ASMJIT_INLINE void release(ZoneAllocator* allocator) noexcept { _release(allocator, sizeof(T)); }

  //! Called to grow the buffer to fit at least `n` elements more.
  ASMJIT_INLINE Error grow(ZoneAllocator* allocator, uint32_t n) noexcept { return ZoneVectorBase::_grow(allocator, sizeof(T), n); }

  //! Resize the vector to hold `n` elements.
  //!
  //! If `n` is greater than the current length then the additional elements'
  //! content will be initialized to zero. If `n` is less than the current
  //! length then the vector will be truncated to exactly `n` elements.
  ASMJIT_INLINE Error resize(ZoneAllocator* allocator, uint32_t n) noexcept { return ZoneVectorBase::_resize(allocator, sizeof(T), n); }

  //! Realloc internal array to fit at least `n` items.
  ASMJIT_INLINE Error reserve(ZoneAllocator* allocator, uint32_t n) noexcept {
    return n > _capacity ? ZoneVectorBase::_reserve(allocator, sizeof(T), n) : static_cast<Error>(kErrorOk);
  }

  ASMJIT_INLINE Error willGrow(ZoneAllocator* allocator, uint32_t n = 1) noexcept {
    return _capacity - _length < n ? grow(allocator, n) : static_cast<Error>(kErrorOk);
  }
};

// ============================================================================
// [asmjit::ZoneStackBase]
// ============================================================================

class ZoneStackBase {
public:
  enum Side {
    kSideLeft  = 0,
    kSideRight = 1
  };

  enum {
    kBlockSize = ZoneAllocator::kHiMaxSize
  };

  struct Block {
    ASMJIT_INLINE Block* getPrev() const noexcept { return _link[kSideLeft]; }
    ASMJIT_INLINE Block* getNext() const noexcept { return _link[kSideRight]; }

    ASMJIT_INLINE void setPrev(Block* block) noexcept { _link[kSideLeft] = block; }
    ASMJIT_INLINE void setNext(Block* block) noexcept { _link[kSideRight] = block; }

    template<typename T>
    ASMJIT_INLINE T* getStart() const noexcept { return static_cast<T*>(_start); }
    template<typename T>
    ASMJIT_INLINE void setStart(T* start) noexcept { _start = static_cast<void*>(start); }

    template<typename T>
    ASMJIT_INLINE T* getEnd() const noexcept { return static_cast<T*>(_end); }
    template<typename T>
    ASMJIT_INLINE void setEnd(T* end) noexcept { _end = static_cast<void*>(end); }

    ASMJIT_INLINE bool isEmpty() const noexcept { return _start == _end; }

    template<typename T>
    ASMJIT_INLINE T* getData() const noexcept {
      return static_cast<T*>(static_cast<void*>((uint8_t*)this + sizeof(Block)));
    }

    template<typename T>
    ASMJIT_INLINE bool canPrepend() const noexcept {
      return _start > getData<void>();
    }

    template<typename T>
    ASMJIT_INLINE bool canAppend() const noexcept {
      size_t kNumBlockItems = (kBlockSize - sizeof(Block)) / sizeof(T);
      size_t kBlockEnd = sizeof(Block) + kNumBlockItems * sizeof(T);
      return (uintptr_t)_end - (uintptr_t)this < kBlockEnd;
    }

    Block* _link[2];                     //!< Next and previous blocks.
    void* _start;                        //!< Pointer to the start of the array.
    void* _end;                          //!< Pointer to the end of the array.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE ZoneStackBase() noexcept {
    _allocator = nullptr;
    _block[0] = nullptr;
    _block[1] = nullptr;
  }
  ASMJIT_INLINE ~ZoneStackBase() noexcept { reset(); }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isInitialized() const noexcept { return _allocator != nullptr; }
  ASMJIT_API Error _init(ZoneAllocator* allocator, size_t middleIndex) noexcept;
  ASMJIT_INLINE Error reset() noexcept { return _init(nullptr, 0); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get a `ZoneAllocator` attached to this container.
  ASMJIT_INLINE ZoneAllocator* getAllocator() const noexcept { return _allocator; }

  ASMJIT_INLINE bool isEmpty() const noexcept {
    ASMJIT_ASSERT(isInitialized());
    return _block[0] == _block[1] && _block[0]->isEmpty();
  }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  ASMJIT_API Error _prepareBlock(uint32_t side, size_t initialIndex) noexcept;
  ASMJIT_API void _cleanupBlock(uint32_t side, size_t middleIndex) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ZoneAllocator* _allocator;             //!< Allocator used to allocate data.
  Block* _block[2];                      //!< First and last blocks.
};

// ============================================================================
// [asmjit::ZoneStack<T>]
// ============================================================================

template<typename T>
class ZoneStack : public ZoneStackBase {
public:
  enum {
    kNumBlockItems   = static_cast<int>((kBlockSize - sizeof(Block)) / sizeof(T)),
    kStartBlockIndex = static_cast<int>(sizeof(Block)),
    kMidBlockIndex   = static_cast<int>(kStartBlockIndex + (kNumBlockItems / 2) * sizeof(T)),
    kEndBlockIndex   = static_cast<int>(kStartBlockIndex + kNumBlockItems * sizeof(T))
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE ZoneStack() noexcept {}
  ASMJIT_INLINE ~ZoneStack() noexcept {}

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Error init(ZoneAllocator* allocator) noexcept { return _init(allocator, kMidBlockIndex); }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Error prepend(T item) noexcept {
    ASMJIT_ASSERT(isInitialized());
    Block* block = _block[kSideLeft];

    if (!block->canPrepend<T>()) {
      ASMJIT_PROPAGATE(_prepareBlock(kSideLeft, kEndBlockIndex));
      block = _block[kSideLeft];
    }

    T* ptr = block->getStart<T>() - 1;
    ASMJIT_ASSERT(ptr >= block->getData<T>() && ptr < block->getData<T>() + kNumBlockItems);
    *ptr = item;
    block->setStart<T>(ptr);
    return kErrorOk;
  }

  ASMJIT_INLINE Error append(T item) noexcept {
    ASMJIT_ASSERT(isInitialized());
    Block* block = _block[kSideRight];

    if (!block->canAppend<T>()) {
      ASMJIT_PROPAGATE(_prepareBlock(kSideRight, kStartBlockIndex));
      block = _block[kSideRight];
    }

    T* ptr = block->getEnd<T>();
    ASMJIT_ASSERT(ptr >= block->getData<T>() && ptr < block->getData<T>() + kNumBlockItems);

    *ptr++ = item;
    block->setEnd(ptr);
    return kErrorOk;
  }

  ASMJIT_INLINE T popFirst() noexcept {
    ASMJIT_ASSERT(isInitialized());
    ASMJIT_ASSERT(!isEmpty());

    Block* block = _block[kSideLeft];
    ASMJIT_ASSERT(!block->isEmpty());

    T* ptr = block->getStart<T>();
    T item = *ptr++;

    block->setStart(ptr);
    if (block->isEmpty())
      _cleanupBlock(kSideLeft, kMidBlockIndex);

    return item;
  }

  ASMJIT_INLINE T pop() noexcept {
    ASMJIT_ASSERT(isInitialized());
    ASMJIT_ASSERT(!isEmpty());

    Block* block = _block[kSideRight];
    ASMJIT_ASSERT(!block->isEmpty());

    T* ptr = block->getEnd<T>();
    T item = *--ptr;

    block->setEnd(ptr);
    if (block->isEmpty())
      _cleanupBlock(kSideRight, kMidBlockIndex);

    return item;
  }
};

// ============================================================================
// [asmjit::ZoneHashNode]
// ============================================================================

//! Node used by \ref ZoneHash<> template.
//!
//! You must provide function `bool eq(const Key& key)` in order to make
//! `ZoneHash::get()` working.
class ZoneHashNode {
public:
  ASMJIT_INLINE ZoneHashNode(uint32_t hVal = 0) noexcept
    : _hashNext(nullptr),
      _hVal(hVal) {}

  ZoneHashNode* _hashNext;               //!< Next node in the chain, null if it terminates the chain.
  uint32_t _hVal;                        //!< Key hash value.
  uint32_t _customData;                  //!< Padding, can be reused by any Node that inherits `ZoneHashNode`.
};

// ============================================================================
// [asmjit::ZoneHashBase]
// ============================================================================

class ZoneHashBase {
public:
  ASMJIT_NONCOPYABLE(ZoneHashBase)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE ZoneHashBase(ZoneAllocator* allocator) noexcept {
    _allocator = allocator;
    _size = 0;
    _bucketsCount = 1;
    _bucketsGrow = 1;
    _data = _embedded;
    _embedded[0] = nullptr;
  }
  ASMJIT_INLINE ~ZoneHashBase() noexcept { reset(nullptr); }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isInitialized() const noexcept { return _allocator != nullptr; }
  ASMJIT_API void reset(ZoneAllocator* allocator) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get a `ZoneAllocator` attached to this container.
  ASMJIT_INLINE ZoneAllocator* getAllocator() const noexcept { return _allocator; }

  ASMJIT_INLINE size_t getSize() const noexcept { return _size; }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  ASMJIT_API void _rehash(uint32_t newCount) noexcept;
  ASMJIT_API ZoneHashNode* _put(ZoneHashNode* node) noexcept;
  ASMJIT_API ZoneHashNode* _del(ZoneHashNode* node) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ZoneAllocator* _allocator;             //!< Zone allocator.
  size_t _size;                          //!< Count of records inserted into the hash table.
  uint32_t _bucketsCount;                //!< Count of hash buckets.
  uint32_t _bucketsGrow;                 //!< When buckets array should grow.

  ZoneHashNode** _data;                  //!< Buckets data.
  ZoneHashNode* _embedded[1];            //!< Embedded data, used by empty hash tables.
};

// ============================================================================
// [asmjit::ZoneHash<Key, Node>]
// ============================================================================

//! Low-level hash table specialized for storing string keys and POD values.
//!
//! This hash table allows duplicates to be inserted (the API is so low
//! level that it's up to you if you allow it or not, as you should first
//! `get()` the node and then modify it or insert a new node by using `put()`,
//! depending on the intention).
template<typename Node>
class ZoneHash : public ZoneHashBase {
public:
  explicit ASMJIT_INLINE ZoneHash(ZoneAllocator* allocator = nullptr) noexcept : ZoneHashBase(allocator) {}
  ASMJIT_INLINE ~ZoneHash() noexcept {}

  template<typename Key>
  ASMJIT_INLINE Node* get(const Key& key) const noexcept {
    uint32_t hMod = key.hVal % _bucketsCount;
    Node* node = static_cast<Node*>(_data[hMod]);

    while (node && !key.matches(node))
      node = static_cast<Node*>(node->_hashNext);
    return node;
  }

  ASMJIT_INLINE Node* put(Node* node) noexcept { return static_cast<Node*>(_put(node)); }
  ASMJIT_INLINE Node* del(Node* node) noexcept { return static_cast<Node*>(_del(node)); }
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_ZONE_H
