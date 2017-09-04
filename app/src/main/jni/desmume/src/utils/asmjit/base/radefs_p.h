// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_RADEFS_P_H
#define _ASMJIT_BASE_RADEFS_P_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/codecompiler.h"
#include "../base/intutils.h"
#include "../base/logging.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_ra
//! \{

// ============================================================================
// [Logging]
// ============================================================================

#if !defined(ASMJIT_DISABLE_LOGGING)
# define ASMJIT_RA_LOG_INIT(LOGGER) \
  Logger* logger = LOGGER;
# define ASMJIT_RA_LOG_FORMAT(...)  \
  do {                              \
    if (logger)                     \
      logger->logf(__VA_ARGS__);    \
  } while (0)
# define ASMJIT_RA_LOG_COMPLEX(...) \
  do {                              \
    if (logger) {                   \
      __VA_ARGS__                   \
    }                               \
  } while (0)
#else
# define ASMJIT_RA_LOG_INIT(LOGGER) ASMJIT_NOP
# define ASMJIT_RA_LOG_FORMAT(...) ASMJIT_NOP
# define ASMJIT_RA_LOG_COMPLEX(...) ASMJIT_NOP
#endif

// ============================================================================
// [Forward Declarations]
// ============================================================================

class RAPass;
class RABlock;
struct RAStackSlot;

typedef ZoneVector<RABlock*> RABlocks;
typedef ZoneVector<RAWorkReg*> RAWorkRegs;

// ============================================================================
// [asmjit::RAArchTraits]
// ============================================================================

//! Traits.
struct RAArchTraits {
  ASMJIT_ENUM(Flags) {
    //! Registers can be swapped by a single instruction.
    kHasSwap = 0x01U
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RAArchTraits() noexcept { reset(); }
  ASMJIT_INLINE void reset() noexcept { ::memset(_flags, 0, sizeof(_flags)); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool hasFlag(uint32_t group, uint32_t flag) const noexcept { return (_flags[group] & flag) != 0; }
  ASMJIT_INLINE bool hasSwap(uint32_t group) const noexcept { return hasFlag(group, kHasSwap); }

  ASMJIT_INLINE uint8_t& operator[](uint32_t group) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    return _flags[group];
  }

  ASMJIT_INLINE const uint8_t& operator[](uint32_t group) const noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    return _flags[group];
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _flags[Reg::kGroupVirt];
};

// ============================================================================
// [asmjit::RARegCount]
// ============================================================================

//! Count of virtual or physical registers per group.
//!
//! NOTE: This class uses 8-bit integers to represent counters, it's only used
//! in places where this is sufficient - for example total count of machine's
//! physical registers, count of virtual registers per instruction, etc. There
//! is also `RALiveCount`, which uses 32-bit integers and is indeed much safer.
struct RARegCount {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset all counters to zero.
  ASMJIT_INLINE void reset() noexcept { _packed = 0; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RARegCount& operator=(const RARegCount& other) noexcept {
    _packed = other._packed;
    return *this;
  }

  ASMJIT_INLINE uint8_t& operator[](uint32_t index) noexcept {
    ASMJIT_ASSERT(index < Reg::kGroupVirt);
    return _regs[index];
  }

  ASMJIT_INLINE const uint8_t& operator[](uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < Reg::kGroupVirt);
    return _regs[index];
  }

  // --------------------------------------------------------------------------
  // [Operators]
  // --------------------------------------------------------------------------

  //! Get register count by a register `group`.
  ASMJIT_INLINE uint32_t get(uint32_t group) const noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);

    uint32_t shift = IntUtils::byteShiftOfDWordStruct(group);
    return (_packed >> shift) & static_cast<uint32_t>(0xFF);
  }

  //! Set register count by a register `group`.
  ASMJIT_INLINE void set(uint32_t group, uint32_t n) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    ASMJIT_ASSERT(n <= 0xFF);

    uint32_t shift = IntUtils::byteShiftOfDWordStruct(group);
    _packed = (_packed & ~static_cast<uint32_t>(0xFF << shift)) + (n << shift);
  }

  //! Add register count by a register `group`.
  ASMJIT_INLINE void add(uint32_t group, uint32_t n = 1) noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    ASMJIT_ASSERT(0xFF - static_cast<uint32_t>(_regs[group]) >= n);

    uint32_t shift = IntUtils::byteShiftOfDWordStruct(group);
    _packed += n << shift;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    uint8_t _regs[4];
    uint32_t _packed;
  };
};

struct RARegIndex : public RARegCount {
  //! Build register indexes based on the given `count` of registers.
  ASMJIT_INLINE void buildIndexes(const RARegCount& count) noexcept {
    uint32_t x = static_cast<uint32_t>(count._regs[0]);
    uint32_t y = static_cast<uint32_t>(count._regs[1]) + x;
    uint32_t z = static_cast<uint32_t>(count._regs[2]) + y;

    ASMJIT_ASSERT(y <= 0xFF);
    ASMJIT_ASSERT(z <= 0xFF);
    _packed = IntUtils::pack32_4x8(0, x, y, z);
  }
};

// ============================================================================
// [asmjit::RARegMask]
// ============================================================================

//! Registers mask.
struct RARegMask {
  // --------------------------------------------------------------------------
  // [Consturction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void init(const RARegMask& other) noexcept {
    for (uint32_t i = 0; i < Reg::kGroupVirt; i++)
      _masks[i] = other._masks[i];
  }

  //! Reset all register masks to zero.
  ASMJIT_INLINE void reset() noexcept {
    for (uint32_t i = 0; i < Reg::kGroupVirt; i++)
      _masks[i] = 0;
  }

  // --------------------------------------------------------------------------
  // [IsEmpty / Has]
  // --------------------------------------------------------------------------

  //! Get whether all register masks are zero (empty).
  ASMJIT_INLINE bool isEmpty() const noexcept {
    uint32_t m = 0;
    for (uint32_t i = 0; i < Reg::kGroupVirt; i++)
      m |= _masks[i];
    return m == 0;
  }

  ASMJIT_INLINE bool has(uint32_t group, uint32_t mask = 0xFFFFFFFFU) const noexcept {
    ASMJIT_ASSERT(group < Reg::kGroupVirt);
    return (_masks[group] & mask) != 0;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RARegMask& operator=(const RARegMask& other) noexcept {
    init(other);
    return *this;
  }

  ASMJIT_INLINE uint32_t& operator[](uint32_t index) noexcept {
    ASMJIT_ASSERT(index < Reg::kGroupVirt);
    return _masks[index];
  }

  ASMJIT_INLINE const uint32_t& operator[](uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < Reg::kGroupVirt);
    return _masks[index];
  }

  // --------------------------------------------------------------------------
  // [Operators]
  // --------------------------------------------------------------------------

  template<class Operator>
  ASMJIT_INLINE void op(const RARegMask& other) noexcept {
    for (uint32_t i = 0; i < Reg::kGroupVirt; i++)
      _masks[i] = Operator::op(_masks[i], other._masks[i]);
  }

  template<class Operator>
  ASMJIT_INLINE void op(uint32_t group, uint32_t input) noexcept {
    _masks[group] = Operator::op(_masks[group], input);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _masks[Reg::kGroupVirt];
};

// ============================================================================
// [asmjit::RARegsStats]
// ============================================================================

//! Information associated with each instruction, propagated to blocks, loops,
//! and the whole function. This information can be used to do minor decisions
//! before the register allocator tries to do its job. For example to use fast
//! register allocation inside a block or loop it cannot have clobbered and/or
//! fixed registers, etc...
struct RARegsStats {
  ASMJIT_ENUM(Index) {
    kIndexUsed       = 0,
    kIndexFixed      = 8,
    kIndexClobbered  = 16
  };

  ASMJIT_ENUM(Mask) {
    kMaskUsed        = 0xFF << kIndexUsed,
    kMaskFixed       = 0xFF << kIndexFixed,
    kMaskClobbered   = 0xFF << kIndexClobbered
  };

  ASMJIT_INLINE void reset() noexcept { _packed = 0; }
  ASMJIT_INLINE void combineWith(const RARegsStats& other) noexcept { _packed |= other._packed; }

  ASMJIT_INLINE bool hasUsed() const noexcept { return (_packed & kMaskUsed) != 0U; }
  ASMJIT_INLINE bool hasUsed(uint32_t group) const noexcept { return (_packed & IntUtils::mask(kIndexUsed + group)) != 0U; }
  ASMJIT_INLINE void makeUsed(uint32_t group) noexcept { _packed |= IntUtils::mask(kIndexUsed + group); }

  ASMJIT_INLINE bool hasFixed() const noexcept { return (_packed & kMaskFixed) != 0U; }
  ASMJIT_INLINE bool hasFixed(uint32_t group) const noexcept { return (_packed & IntUtils::mask(kIndexFixed + group)) != 0U; }
  ASMJIT_INLINE void makeFixed(uint32_t group) noexcept { _packed |= IntUtils::mask(kIndexFixed + group); }

  ASMJIT_INLINE bool hasClobbered() const noexcept { return (_packed & kMaskClobbered) != 0U; }
  ASMJIT_INLINE bool hasClobbered(uint32_t group) const noexcept { return (_packed & IntUtils::mask(kIndexClobbered + group)) != 0U; }
  ASMJIT_INLINE void makeClobbered(uint32_t group) noexcept { _packed |= IntUtils::mask(kIndexClobbered + group); }

  uint32_t _packed;
};

// ============================================================================
// [asmjit::RALiveCount]
// ============================================================================

//! Count of live register, per group.
class RALiveCount {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RALiveCount() noexcept { reset(); }
  ASMJIT_INLINE RALiveCount(const RALiveCount& other) noexcept { init(other); }

  ASMJIT_INLINE void init(const RALiveCount& other) noexcept {
    for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
      n[group] = other.n[group];
  }

  ASMJIT_INLINE void reset() noexcept {
    for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
      n[group] = 0;
  }

  // --------------------------------------------------------------------------
  // [Ops]
  // --------------------------------------------------------------------------

  template<class Operator>
  ASMJIT_INLINE void op(const RALiveCount& other) noexcept {
    for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
      n[group] = Operator::op(n[group], other.n[group]);
  }

  ASMJIT_INLINE RALiveCount& operator=(const RALiveCount& other) noexcept { init(other); return *this; }
  ASMJIT_INLINE uint32_t& operator[](uint32_t group) noexcept { return n[group]; }
  ASMJIT_INLINE const uint32_t& operator[](uint32_t group) const noexcept { return n[group]; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t n[Reg::kGroupVirt];
};

// ============================================================================
// [asmjit::LiveInterval]
// ============================================================================

struct LiveInterval {
  ASMJIT_ENUM(Misc) {
    kNaN = 0,
    kInf = 0xFFFFFFFFU
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE LiveInterval() noexcept : a(0), b(0) {}
  ASMJIT_INLINE LiveInterval(uint32_t a, uint32_t b) noexcept : a(a), b(b) {}
  ASMJIT_INLINE LiveInterval(const LiveInterval& other) noexcept : a(other.a), b(other.b) {}

  ASMJIT_INLINE void init(uint32_t a, uint32_t b) noexcept {
    this->a = a;
    this->b = b;
  }
  ASMJIT_INLINE void init(const LiveInterval& other) noexcept { init(other.a, other.b); }
  ASMJIT_INLINE void reset() noexcept { init(0, 0); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isValid() const noexcept { return a < b; }
  ASMJIT_INLINE uint32_t getWidth() const noexcept { return b - a; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t a, b;
};

// ============================================================================
// [asmjit::RALiveSpan<T>]
// ============================================================================

template<typename T>
class RALiveSpan : public LiveInterval, public T {
public:
  typedef T DataType;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RALiveSpan() noexcept : LiveInterval(), T() {}
  ASMJIT_INLINE RALiveSpan(const RALiveSpan<T>& other) noexcept : LiveInterval(other), T() {}
  ASMJIT_INLINE RALiveSpan(const LiveInterval& interval, const T& data) noexcept : LiveInterval(interval), T(data) {}
  ASMJIT_INLINE RALiveSpan(uint32_t a, uint32_t b) noexcept : LiveInterval(a, b), T() {}
  ASMJIT_INLINE RALiveSpan(uint32_t a, uint32_t b, const T& data) noexcept : LiveInterval(a, b), T(data) {}

  ASMJIT_INLINE void init(const RALiveSpan<T>& other) noexcept {
    LiveInterval::init(static_cast<const LiveInterval&>(other));
    T::init(static_cast<const T&>(other));
  }

  ASMJIT_INLINE void init(const RALiveSpan<T>& span, const T& data) noexcept {
    LiveInterval::init(static_cast<const LiveInterval&>(span));
    T::init(data);
  }

  ASMJIT_INLINE void init(const LiveInterval& interval, const T& data) noexcept {
    LiveInterval::init(interval);
    T::init(data);
  }
};

// ============================================================================
// [asmjit::RALiveSpans<T>]
// ============================================================================

template<typename T>
class RALiveSpans {
public:
  ASMJIT_NONCOPYABLE(RALiveSpans<T>)

  typedef typename T::DataType DataType;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RALiveSpans() noexcept : _data() {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset() noexcept { _data.reset(); }
  ASMJIT_INLINE void release(ZoneAllocator* allocator) noexcept { _data.release(allocator); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isEmpty() const noexcept { return _data.isEmpty(); }
  ASMJIT_INLINE uint32_t getLength() const noexcept { return _data.getLength(); }

  ASMJIT_INLINE T* getData() noexcept { return _data.getData(); }
  ASMJIT_INLINE const T* getData() const noexcept { return _data.getData(); }

  ASMJIT_INLINE void swap(RALiveSpans<T>& other) noexcept { _data.swap(other._data); }

  ASMJIT_INLINE bool isOpen() const noexcept {
    uint32_t len = _data.getLength();
    return len > 0 && _data[len - 1].b == LiveInterval::kInf;
  }

  //! Open the current live span.
  ASMJIT_INLINE Error openAt(ZoneAllocator* allocator, uint32_t start, uint32_t end) noexcept {
    bool wasOpen;
    return openAt(allocator, start, end, wasOpen);
  }

  ASMJIT_INLINE Error openAt(ZoneAllocator* allocator, uint32_t start, uint32_t end, bool& wasOpen) noexcept {
    uint32_t len = _data.getLength();
    wasOpen = false;

    if (len > 0) {
      T& last = _data[len - 1];
      if (last.b >= start) {
        wasOpen = last.b > start;
        last.b = end;
        return kErrorOk;
      }
    }

    return _data.append(allocator, T(start, end));
  }

  ASMJIT_INLINE void closeAt(uint32_t end) noexcept {
    ASMJIT_ASSERT(!isEmpty());

    uint32_t len = _data.getLength();
    _data[len - 1].b = end;
  }

  //! Returns the sum of width of all spans.
  //!
  //! NOTE: Don't overuse, this iterates over all spans so it's O(N).
  //! It should be only called once and then cached.
  ASMJIT_INLINE uint32_t calcWidth() const noexcept {
    uint32_t width = 0;
    const T* spans = _data.getData();
    for (uint32_t i = 0, len = _data.getLength(); i < len; i++)
      width += spans[i].getWidth();
    return width;
  }

  ASMJIT_INLINE T& operator[](uint32_t index) noexcept { return _data[index]; }
  ASMJIT_INLINE const T& operator[](uint32_t index) const noexcept { return _data[index]; }

  ASMJIT_INLINE bool intersects(const RALiveSpans<T>& other) const noexcept {
    return intersects(*this, other);
  }

  ASMJIT_INLINE Error nonOverlappingUnionOf(ZoneAllocator* allocator, const RALiveSpans<T>& x, const RALiveSpans<T>& y, const DataType& yData) noexcept {
    uint32_t finalLength = x.getLength() + y.getLength();
    ASMJIT_PROPAGATE(_data.reserve(allocator, finalLength));

    T* dstPtr = _data.getData();

    const T* xSpan = x.getData();
    const T* ySpan = y.getData();

    const T* xEnd = xSpan + x.getLength();
    const T* yEnd = ySpan + y.getLength();

    // Loop until we have intersection or either `xSpan == xEnd` or `ySpan == yEnd`,
    // which means that there is no intersection. We advance either `xSpan` or `ySpan`
    // depending on their ranges.
    if (xSpan != xEnd && ySpan != yEnd) {
      uint32_t xa, ya;
      xa = xSpan->a;
      for (;;) {
        while (ySpan->b <= xa) {
          dstPtr->init(*ySpan, yData);
          dstPtr++;
          if (++ySpan == yEnd)
            goto Done;
        }

        ya = ySpan->a;
        while (xSpan->b <= ya) {
          *dstPtr++ = *xSpan;
          if (++xSpan == xEnd)
            goto Done;
        }

        // We know that `xSpan->b > ySpan->a`, so check if `ySpan->b > xSpan->a`.
        xa = xSpan->a;
        if (ySpan->b > xa)
          return 0xFFFFFFFFU;
      }
    }

  Done:
    while (xSpan != xEnd) {
      *dstPtr++ = *xSpan++;
    }

    while (ySpan != yEnd) {
      dstPtr->init(*ySpan, yData);
      dstPtr++;
      ySpan++;
    }

    _data._setEndPtr(dstPtr);
    return kErrorOk;
  }

  static ASMJIT_INLINE bool intersects(const RALiveSpans<T>& x, const RALiveSpans<T>& y) noexcept {
    const T* xSpan = x.getData();
    const T* ySpan = y.getData();

    const T* xEnd = xSpan + x.getLength();
    const T* yEnd = ySpan + y.getLength();

    // Loop until we have intersection or either `xSpan == xEnd` or `ySpan == yEnd`,
    // which means that there is no intersection. We advance either `xSpan` or `ySpan`
    // depending on their end positions.
    if (xSpan == xEnd || ySpan == yEnd)
      return false;

    uint32_t xa, ya;
    xa = xSpan->a;

    for (;;) {
      while (ySpan->b <= xa)
        if (++ySpan == yEnd)
          return false;

      ya = ySpan->a;
      while (xSpan->b <= ya)
        if (++xSpan == xEnd)
          return false;

      // We know that `xSpan->b > ySpan->a`, so check if `ySpan->b > xSpan->a`.
      xa = xSpan->a;
      if (ySpan->b > xa)
        return true;
    }
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ZoneVector<T> _data;
};

// ============================================================================
// [asmjit::RALiveStats]
// ============================================================================

//! Statistics about a register liveness.
class RALiveStats {
public:
  ASMJIT_INLINE RALiveStats()
    : _width(0),
      _freq(0.0f) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t getWidth() const noexcept { return _width; }
  ASMJIT_INLINE float getFreq() const noexcept { return _freq; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _width;
  float _freq;
};

// ============================================================================
// [asmjit::LiveRegData]
// ============================================================================

struct LiveRegData {
  ASMJIT_INLINE LiveRegData() noexcept : id(Reg::kIdBad) {}
  explicit ASMJIT_INLINE LiveRegData(uint32_t id) noexcept : id(id) {}
  explicit ASMJIT_INLINE LiveRegData(const LiveRegData& other) noexcept : id(other.id) {}

  ASMJIT_INLINE void init(const LiveRegData& other) noexcept { id = other.id; }

  ASMJIT_INLINE bool operator==(const LiveRegData& other) const noexcept { return id == other.id; }
  ASMJIT_INLINE bool operator!=(const LiveRegData& other) const noexcept { return id != other.id; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t id;
};

typedef RALiveSpan<LiveRegData> LiveRegSpan;
typedef RALiveSpans<LiveRegSpan> LiveRegSpans;

// ============================================================================
// [asmjit::RATiedReg]
// ============================================================================

//! Tied register merges one ore more register operand into a single entity. It
//! contains information about its access (Read|Write) and allocation slots
//! (Use|Out) that are used by the register allocator and liveness analysis.
struct RATiedReg {
  //! Flags.
  //!
  //! Register access information is encoded in 4 flags in total:
  //!
  //!   - `kRead`  - Register is Read    (ReadWrite if combined with `kWrite`).
  //!   - `kWrite` - Register is Written (ReadWrite if combined with `kRead`).
  //!   - `kUse`   - Encoded as Read or ReadWrite.
  //!   - `kOut`   - Encoded as WriteOnly.
  //!
  //! Let's describe all of these on two X86 instructions:
  //!
  //!   - ADD x{R|W|Use},  x{R|Use}              -> {x:R|W|Use            }
  //!   - LEA x{  W|Out}, [x{R|Use} + x{R|Out}]  -> {x:R|W|Use|Out        }
  //!   - ADD x{R|W|Use},  y{R|Use}              -> {x:R|W|Use     y:R|Use}
  //!   - LEA x{  W|Out}, [x{R|Use} + y{R|Out}]  -> {x:R|W|Use|Out y:R|Use}
  //!
  //! It should be obvious from the example above how these flags get created.
  //! Each operand contains READ/WRITE information, which is then merged to
  //! RATiedReg's flags. However, we also need to represent the possitility to
  //! use see the operation as two independent operations - USE and OUT, because
  //! the register allocator will first allocate USE registers, and then assign
  //! OUT registers independently of USE registers.
  ASMJIT_ENUM(Flags) {
    kRead        = OpInfo::kRead,        //!< Register is read.
    kWrite       = OpInfo::kWrite,       //!< Register is written.
    kRW          = OpInfo::kRW,          //!< Register read and written.
    kUse         = OpInfo::kUse,         //!< Regsiter has a USE slot (Read/ReadWrite).
    kOut         = OpInfo::kOut,         //!< Register has an OUT slot (WriteOnly).
    kUseFixed    = OpInfo::kUseFixed,    //!< Register has a fixed USE slot.
    kOutFixed    = OpInfo::kOutFixed,    //!< Register has a fixed OUT slot.

    // TODO: Maybe we don't need these at all.
    kUseCall     = 0x00000040U,          //!< Function-call register argument (USE).
    kOutCall     = 0x00000080U,          //!< Function-call register return (OUT).

    kUseDone     = 0x00000100U,          //!< Register USE slot has been allocated.
    kOutDone     = 0x00000200U,          //!< Register OUT slot has been allocated

    kLast        = 0x00000400U,          //!< Last occurence of this VirtReg in basic block.
    kKill        = 0x00000800U,          //!< Kill this VirtReg after use.

    // Architecture specific flags are used during RATiedReg building to ensure
    // that architecture-specific constraints are handled properly. These flags
    // are not really needed after RATiedReg[] is built and copied to `RAInst`.

    kX86Gpb      = 0x00001000U           //!< This tied references GPB-LO or GPB-HI.
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void init(uint32_t workId, uint32_t flags, uint32_t allocableRegs, uint32_t useId, uint32_t outId) noexcept {
    this->workId = workId;
    this->flags = flags;
    this->allocableRegs = allocableRegs;
    this->refCount = 1;
    this->useId = IntUtils::toUInt8(useId);
    this->outId = IntUtils::toUInt8(outId);
    this->reserved = 0;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t getWorkId() const noexcept { return workId; }

  //! Check if the given `flag` is set, see \ref Flags.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) const noexcept { return (this->flags & flag) != 0; }

  //! Get tied register flags, see \ref Flags.
  ASMJIT_INLINE uint32_t getFlags() const noexcept { return flags; }
  //! Add tied register flags, see \ref Flags.
  ASMJIT_INLINE void addFlags(uint32_t flags) noexcept { this->flags |= flags; }

  //! Get if the register is read (writes `true` also if it's Read/Write).
  ASMJIT_INLINE bool isRead() const noexcept { return hasFlag(kRead); }
  //! Get if the register is written (writes `true` also if it's Read/Write).
  ASMJIT_INLINE bool isWrite() const noexcept { return hasFlag(kWrite); }
  //! Get if the register is read only.
  ASMJIT_INLINE bool isReadOnly() const noexcept { return (flags & kRW) == kRead; }
  //! Get if the register is write only.
  ASMJIT_INLINE bool isWriteOnly() const noexcept { return (flags & kRW) == kWrite; }
  //! Get if the register is read and written.
  ASMJIT_INLINE bool isReadWrite() const noexcept { return (flags & kRW) == kRW; }

  //! Get if the tied register has use operand (Read/ReadWrite).
  ASMJIT_INLINE bool isUse() const noexcept { return hasFlag(kUse); }
  //! Get if the tied register has out operand (Write).
  ASMJIT_INLINE bool isOut() const noexcept { return hasFlag(kOut); }

  ASMJIT_INLINE void makeReadOnly() noexcept { flags = (flags & ~(kOut | kWrite)) | kUse; }
  ASMJIT_INLINE void makeWriteOnly() noexcept { flags = (flags & ~(kUse | kRead)) | kOut; }

  //! Get if this register (and the instruction it's part of) appears last in the basic block.
  ASMJIT_INLINE bool isLast() const noexcept { return hasFlag(kLast); }
  //! Get if this register should be killed after USEd and/or OUTed.
  ASMJIT_INLINE bool isKill() const noexcept { return hasFlag(kKill); }

  //! Get if this register is OUT or KILL (used internally by local register allocator).
  ASMJIT_INLINE bool isOutOrKill() const noexcept { return hasFlag(kOut | kKill); }

  //! Get if the register must be allocated to a fixed physical register before it's used.
  ASMJIT_INLINE bool hasUseId() const noexcept { return useId != Reg::kIdBad; }
  //! Get if the register must be allocated to a fixed physical register before it's written.
  ASMJIT_INLINE bool hasOutId() const noexcept { return outId != Reg::kIdBad; }

  //! Get a physical register used for 'use' operation.
  ASMJIT_INLINE uint32_t getUseId() const noexcept { return useId; }
  //! Get a physical register used for 'out' operation.
  ASMJIT_INLINE uint32_t getOutId() const noexcept { return outId; }

  //! Set a physical register used for 'use' operation.
  ASMJIT_INLINE void setUseId(uint32_t index) noexcept { useId = IntUtils::toUInt8(index); }
  //! Set a physical register used for 'out' operation.
  ASMJIT_INLINE void setOutId(uint32_t index) noexcept { outId = IntUtils::toUInt8(index); }

  ASMJIT_INLINE bool isUseDone() const noexcept { return hasFlag(kUseDone); }
  ASMJIT_INLINE bool isOutDone() const noexcept { return hasFlag(kUseDone); }

  ASMJIT_INLINE void markUseDone() noexcept { addFlags(kUseDone); }
  ASMJIT_INLINE void markOutDone() noexcept { addFlags(kUseDone); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RATiedReg& operator=(const RATiedReg& other) noexcept {
    ::memcpy(this, &other, sizeof(RATiedReg));
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t workId;                       //!< WorkReg id.
  uint32_t flags;                        //!< Allocation flags.
  uint32_t allocableRegs;                //!< Registers where input {R|X} can be allocated to.
  union {
    struct {
      uint8_t refCount;                  //!< How many times the VirtReg is referenced in all operands.
      uint8_t useId;                     //!< Physical register for use operation (ReadOnly / ReadWrite).
      uint8_t outId;                     //!< Physical register for out operation (WriteOnly).
      uint8_t reserved;                  //!< Index of OUT operand or 0xFF if none.
    };
    uint32_t packed;                     //!< Packed data.
  };
};

// ============================================================================
// [asmjit::RAWorkReg]
// ============================================================================

class RAWorkReg {
public:
  ASMJIT_NONCOPYABLE(RAWorkReg)

  ASMJIT_ENUM(Ids) {
    kIdNone               = 0xFFFFFFFFU
  };

  ASMJIT_ENUM(Flags) {
    kFlagCoalesced        = 0x00000001U,
    kFlagStackUsed        = 0x00000010U, //!< Stack slot has to be allocated.

    // TODO: Used?
    kFlagDirtyStats       = 0x80000000U
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RAWorkReg(VirtReg* vReg, uint32_t workId) noexcept
    : _workId(workId),
      _virtId(vReg->getId()),
      _info(vReg->getInfo()),
      _flags(kFlagDirtyStats),
      _virtReg(vReg),
      _tiedReg(nullptr),
      _stackSlot(nullptr),
      _tiedPosition(0xFFFFFFFFU),
      _homeId(Reg::kIdBad),
      _liveSpans(),
      _liveStats(),
      _refs() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t getWorkId() const noexcept { return _workId; }
  ASMJIT_INLINE uint32_t getVirtId() const noexcept { return _virtId; }

  ASMJIT_INLINE const char* getName() const noexcept { return _virtReg->getName(); }
  ASMJIT_INLINE uint32_t getNameLength() const noexcept { return _virtReg->getNameLength(); }

  ASMJIT_INLINE uint32_t getTypeId() const noexcept { return _virtReg->getTypeId(); }

  ASMJIT_INLINE bool hasFlag(uint32_t flag) const noexcept { return (_flags & flag) != 0; }

  ASMJIT_INLINE uint32_t getFlags() const noexcept { return _flags; }
  ASMJIT_INLINE void addFlags(uint32_t flags) noexcept { _flags |= flags; }

  ASMJIT_INLINE bool isStackUsed() const noexcept { return hasFlag(kFlagStackUsed); }
  ASMJIT_INLINE void markStackUsed() noexcept { addFlags(kFlagStackUsed); }

  //! Get if this RAWorkReg has been coalesced with another one (cannot be used anymore).
  ASMJIT_INLINE bool isCoalesced() const noexcept { return hasFlag(kFlagCoalesced); }

  ASMJIT_INLINE const RegInfo& getInfo() const noexcept { return _info; }
  ASMJIT_INLINE uint32_t getGroup() const noexcept { return _info.getGroup(); }

  ASMJIT_INLINE VirtReg* getVirtReg() const noexcept { return _virtReg; }

  ASMJIT_INLINE bool hasTiedReg() const noexcept { return _tiedReg != nullptr; }
  ASMJIT_INLINE RATiedReg* getTiedReg() const noexcept { return _tiedReg; }
  ASMJIT_INLINE void setTiedReg(RATiedReg* tiedReg) noexcept { _tiedReg = tiedReg; }
  ASMJIT_INLINE void resetTiedReg() noexcept { _tiedReg = nullptr; }

  ASMJIT_INLINE bool hasStackSlot() const noexcept { return _stackSlot != nullptr; }
  ASMJIT_INLINE RAStackSlot* getStackSlot() const noexcept { return _stackSlot; }

  ASMJIT_INLINE LiveRegSpans& getLiveSpans() noexcept { return _liveSpans; }
  ASMJIT_INLINE const LiveRegSpans& getLiveSpans() const noexcept { return _liveSpans; }

  ASMJIT_INLINE RALiveStats& getLiveStats() noexcept { return _liveStats; }
  ASMJIT_INLINE const RALiveStats& getLiveStats() const noexcept { return _liveStats; }

  ASMJIT_INLINE bool hasHomeId() const noexcept { return _homeId != Reg::kIdBad; }
  ASMJIT_INLINE uint32_t getHomeId() const noexcept { return _homeId; }
  ASMJIT_INLINE void setHomeId(uint32_t physId) noexcept { _homeId = IntUtils::toUInt8(physId); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _workId;                      //!< RAPass specific ID used during analysis and allocation.
  uint32_t _virtId;                      //!< Copy of ID used by `VirtReg`.

  RegInfo _info;                         //!< Copy of a signature used by `VirtReg`.
  uint32_t _flags;                       //!< RAPass specific flags used during analysis and allocation.

  VirtReg* _virtReg;                     //!< Permanent association with `VirtReg`.
  RATiedReg* _tiedReg;                   //!< Temporary association with `RATiedReg`.
  RAStackSlot* _stackSlot;               //!< Stack slot associated with the register.

  uint32_t _tiedPosition;                //!< Position of last associated `RATiedReg`.
  uint8_t _homeId;                       //!< ID of a physical home register (if any).

  LiveRegSpans _liveSpans;               //!< Live spans of the `VirtReg`.
  RALiveStats _liveStats;                //!< Live statistics.

  ZoneVector<CBNode*> _refs;             //!< All nodes that read/write this VirtReg/WorkReg.
  ZoneVector<CBNode*> _writes;           //!< All nodes that write to this VirtReg/WorkReg.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_RADEFS_P_H
