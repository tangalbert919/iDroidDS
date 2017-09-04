// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_UTILS_H
#define _ASMJIT_BASE_UTILS_H

// [Dependencies]
#include "../base/intutils.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::AsmJitInternal::QSortT<Impl>]
// ============================================================================

namespace AsmJitInternal {

template<class Parent, bool Ascending>
struct CompareByValue {
  template<typename T>
  static ASMJIT_INLINE bool lt(const T& a, const T& b) noexcept {
    return Ascending ? Parent::getValue(a) < Parent::getValue(b)
                     : Parent::getValue(a) > Parent::getValue(b);
  }

  template<typename T>
  static ASMJIT_INLINE bool le(const T& a, const T& b) noexcept {
    return Ascending ? Parent::getValue(a) <= Parent::getValue(b)
                     : Parent::getValue(a) >= Parent::getValue(b);
  }

  template<typename T>
  static ASMJIT_INLINE bool eq(const T& a, const T& b) noexcept { return Parent::getValue(a) == Parent::getValue(b); }
};

template<typename T, typename Compare>
struct QSortT {
  static ASMJIT_INLINE T* med3(T* a, T* b, T* c, const Compare& cmp) noexcept {
    return cmp.lt(*a, *b) ? ( cmp.lt(*b, *c) ? b : (cmp.lt(*a, *c) ? c : a))
                          : (!cmp.le(*b, *c) ? b : (cmp.lt(*a, *c) ? a : c));
  }

  static ASMJIT_NOINLINE void sort(T* base, size_t len, const Compare& cmp) noexcept {
    T* pa; T* pb; T* pc; T* pd;
    T* pl; T* pm; T* pn;
    unsigned int swapFlag;

_Repeat:
    swapFlag = 0;

    // Insertion sort.
    if (len < 7) {
      for (pm = base + 1; pm < base + len; pm++)
        for (pl = pm; pl > base && !cmp.lt(pl[-1], pl[0]); pl--)
          std::swap(pl[0], pl[-1]);
      return;
    }

    pm = base + (len >> 1);
    if (len > 7) {
      pl = base;
      pn = base + (len - 1);

      if (len > 40) {
        size_t i = len / 8;
        pl = med3(pl       , pl + i, pl + i * 2, cmp);
        pm = med3(pm -i    , pm    , pm + i    , cmp);
        pn = med3(pn -i * 2, pn - i, pn        , cmp);
      }

      pm = med3(pl, pm, pn, cmp);
    }

    std::swap(*base, *pm);
    pa = pb = base + 1;
    pc = pd = base + len - 1;

    for (;;) {
      while (pb <= pc && cmp.le(*pb, *base)) {
        if (cmp.eq(*pd, *base)) {
          swapFlag = 1;
          std::swap(*pa, *pb);
          pa++;
        }
        pb++;
      }

      while (pb <= pc && !cmp.lt(*pc, *base)) {
        if (cmp.eq(*pc, *base)) {
          swapFlag = 1;
          std::swap(*pc, *pd);
          pd--;
        }
        pc--;
      }

      if (pb > pc)
        break;

      swapFlag = 1;
      std::swap(*pb, *pc);

      pb++;
      pc--;
    }

    if (swapFlag) {
      size_t i;
      T* swpA;
      T* swpB;

      pn = base + len;

      // Step 1.
      i = std::min((size_t)(pa - base), (size_t)(pb - pa));
      swpA = base;
      swpB = pb - i;

      for (;;) {
        while (i > 0) {
          std::swap(swpA, swpB);
          swpA++;
          swpB++;
          i--;
        }

        if (swapFlag == 0)
          break;
        swapFlag = 0;

        // Step 2.
        i = std::min((size_t)(pd - pc), (size_t)(pn - pd) - 1);
        swpA = pb;
        swpB = pn - i;
      }

      // Recurse.
      if ((i = (size_t)(pb - pa)) > 1) {
        sort(base, i, cmp);
      }

      // Iterate.
      if ((i = (size_t)(pd - pc)) > 1) {
        base = pn - i;
        len = i;
        goto _Repeat;
      }
    }
    else {
      // Insertion sort.
      for (pm = base + 1; pm < base + len; pm++)
        for (pl = pm; pl > base && !cmp.le(pl[-1], pl[0]); pl--)
          std::swap(pl[0], pl[-1]);
    }
  }
};

} // AsmJitInternal namespace

// ============================================================================
// [asmjit::Utils]
// ============================================================================

//! AsmJit utilities - integer, string, etc...
namespace Utils {
  // --------------------------------------------------------------------------
  // [Hash]
  // --------------------------------------------------------------------------

  // \internal
  static ASMJIT_INLINE uint32_t hashRound(uint32_t hash, uint32_t c) noexcept { return hash * 65599 + c; }

  // Get a hash of the given string `str` of `len` length. Length must be valid
  // as this function doesn't check for a null terminator and allows it in the
  // middle of the string.
  static ASMJIT_INLINE uint32_t hashString(const char* str, size_t len) noexcept {
    uint32_t hVal = 0;
    for (uint32_t i = 0; i < len; i++)
      hVal = hashRound(hVal, static_cast<uint8_t>(str[i]));
    return hVal;
  }

  // --------------------------------------------------------------------------
  // [ReadMem]
  // --------------------------------------------------------------------------

  template<typename Type, size_t Alignment>
  struct AlignedInt {};

  template<> struct AlignedInt<uint16_t, 1> { typedef uint16_t ASMJIT_ALIGN_TYPE(T, 1); };
  template<> struct AlignedInt<uint16_t, 2> { typedef uint16_t T; };
  template<> struct AlignedInt<uint32_t, 1> { typedef uint32_t ASMJIT_ALIGN_TYPE(T, 1); };
  template<> struct AlignedInt<uint32_t, 2> { typedef uint32_t ASMJIT_ALIGN_TYPE(T, 2); };
  template<> struct AlignedInt<uint32_t, 4> { typedef uint32_t T; };
  template<> struct AlignedInt<uint64_t, 1> { typedef uint64_t ASMJIT_ALIGN_TYPE(T, 1); };
  template<> struct AlignedInt<uint64_t, 2> { typedef uint64_t ASMJIT_ALIGN_TYPE(T, 2); };
  template<> struct AlignedInt<uint64_t, 4> { typedef uint64_t ASMJIT_ALIGN_TYPE(T, 4); };
  template<> struct AlignedInt<uint64_t, 8> { typedef uint64_t T; };

  static ASMJIT_INLINE uint32_t readU8(const void* p) noexcept {
    return static_cast<uint32_t>(static_cast<const uint8_t*>(p)[0]);
  }

  static ASMJIT_INLINE int32_t readI8(const void* p) noexcept {
    return static_cast<int32_t>(static_cast<const int8_t*>(p)[0]);
  }

  template<size_t Alignment>
  static ASMJIT_INLINE uint32_t readU16xLE(const void* p) noexcept {
    if (ASMJIT_ARCH_LE && (ASMJIT_ARCH_UNALIGNED_16 || Alignment >= 2)) {
      typedef typename AlignedInt<uint16_t, Alignment>::T U16AlignedToN;
      return static_cast<uint32_t>(static_cast<const U16AlignedToN*>(p)[0]);
    }
    else {
      uint32_t x = static_cast<uint32_t>(static_cast<const uint8_t*>(p)[0]);
      uint32_t y = static_cast<uint32_t>(static_cast<const uint8_t*>(p)[1]);
      return x + (y << 8);
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE uint32_t readU16xBE(const void* p) noexcept {
    if (ASMJIT_ARCH_BE && (ASMJIT_ARCH_UNALIGNED_16 || Alignment >= 2)) {
      typedef typename AlignedInt<uint16_t, Alignment>::T U16AlignedToN;
      return static_cast<uint32_t>(static_cast<const U16AlignedToN*>(p)[0]);
    }
    else {
      uint32_t x = static_cast<uint32_t>(static_cast<const uint8_t*>(p)[0]);
      uint32_t y = static_cast<uint32_t>(static_cast<const uint8_t*>(p)[1]);
      return (x << 8) + y;
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE uint32_t readU16x(const void* p) noexcept {
    return ASMJIT_ARCH_LE ? readU16xLE<Alignment>(p) : readU16xBE<Alignment>(p);
  }

  template<size_t Alignment>
  static ASMJIT_INLINE int32_t readI16xLE(const void* p) noexcept {
    if (ASMJIT_ARCH_LE && (ASMJIT_ARCH_UNALIGNED_16 || Alignment >= 2)) {
      typedef typename AlignedInt<uint16_t, Alignment>::T U16AlignedToN;
      return static_cast<int32_t>(
        static_cast<int16_t>(
          static_cast<const U16AlignedToN*>(p)[0]));
    }
    else {
      int32_t x = static_cast<int32_t>(static_cast<const uint8_t*>(p)[0]);
      int32_t y = static_cast<int32_t>(static_cast<const int8_t*>(p)[1]);
      return x + (y << 8);
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE int32_t readI16xBE(const void* p) noexcept {
    if (ASMJIT_ARCH_BE && (ASMJIT_ARCH_UNALIGNED_16 || Alignment >= 2)) {
      typedef typename AlignedInt<uint16_t, Alignment>::T U16AlignedToN;
      return static_cast<int32_t>(
        static_cast<int16_t>(
          static_cast<const U16AlignedToN*>(p)[0]));
    }
    else {
      int32_t x = static_cast<int32_t>(static_cast<const int8_t*>(p)[0]);
      int32_t y = static_cast<int32_t>(static_cast<const uint8_t*>(p)[1]);
      return (x << 8) + y;
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE int32_t readI16x(const void* p) noexcept {
    return ASMJIT_ARCH_LE ? readI16xLE<Alignment>(p) : readI16xBE<Alignment>(p);
  }

  static ASMJIT_INLINE uint32_t readU16aLE(const void* p) noexcept { return readU16xLE<2>(p); }
  static ASMJIT_INLINE uint32_t readU16uLE(const void* p) noexcept { return readU16xLE<1>(p); }

  static ASMJIT_INLINE uint32_t readU16aBE(const void* p) noexcept { return readU16xBE<2>(p); }
  static ASMJIT_INLINE uint32_t readU16uBE(const void* p) noexcept { return readU16xBE<1>(p); }

  static ASMJIT_INLINE uint32_t readU16a(const void* p) noexcept { return readU16x<2>(p); }
  static ASMJIT_INLINE uint32_t readU16u(const void* p) noexcept { return readU16x<1>(p); }

  static ASMJIT_INLINE int32_t readI16aLE(const void* p) noexcept { return readI16xLE<2>(p); }
  static ASMJIT_INLINE int32_t readI16uLE(const void* p) noexcept { return readI16xLE<1>(p); }

  static ASMJIT_INLINE int32_t readI16aBE(const void* p) noexcept { return readI16xBE<2>(p); }
  static ASMJIT_INLINE int32_t readI16uBE(const void* p) noexcept { return readI16xBE<1>(p); }

  static ASMJIT_INLINE int32_t readI16a(const void* p) noexcept { return readI16x<2>(p); }
  static ASMJIT_INLINE int32_t readI16u(const void* p) noexcept { return readI16x<1>(p); }

  template<size_t Alignment>
  static ASMJIT_INLINE uint32_t readU32xLE(const void* p) noexcept {
    if (ASMJIT_ARCH_UNALIGNED_32 || Alignment >= 4) {
      typedef typename AlignedInt<uint32_t, Alignment>::T U32AlignedToN;
      uint32_t x = static_cast<const U32AlignedToN*>(p)[0];
      return ASMJIT_ARCH_LE ? x : IntUtils::byteswap32(x);
    }
    else {
      uint32_t x = readU16xLE<Alignment < 4 ? Alignment : size_t(2)>(static_cast<const uint8_t*>(p) + 0);
      uint32_t y = readU16xLE<Alignment < 4 ? Alignment : size_t(2)>(static_cast<const uint8_t*>(p) + 2);
      return x + (y << 16);
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE uint32_t readU32xBE(const void* p) noexcept {
    if (ASMJIT_ARCH_UNALIGNED_32 || Alignment >= 4) {
      typedef typename AlignedInt<uint32_t, Alignment>::T U32AlignedToN;
      uint32_t x = static_cast<const U32AlignedToN*>(p)[0];
      return ASMJIT_ARCH_BE ? x : IntUtils::byteswap32(x);
    }
    else {
      uint32_t x = readU16xBE<Alignment < 4 ? Alignment : size_t(2)>(static_cast<const uint8_t*>(p) + 0);
      uint32_t y = readU16xBE<Alignment < 4 ? Alignment : size_t(2)>(static_cast<const uint8_t*>(p) + 2);
      return (x << 16) + y;
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE uint32_t readU32x(const void* p) noexcept {
    return ASMJIT_ARCH_LE ? readU32xLE<Alignment>(p) : readU32xBE<Alignment>(p);
  }

  template<size_t Alignment>
  static ASMJIT_INLINE int32_t readI32xLE(const void* p) noexcept {
    return static_cast<int32_t>(readU32xLE<Alignment>(p));
  }

  template<size_t Alignment>
  static ASMJIT_INLINE int32_t readI32xBE(const void* p) noexcept {
    return static_cast<int32_t>(readU32xBE<Alignment>(p));
  }

  template<size_t Alignment>
  static ASMJIT_INLINE int32_t readI32x(const void* p) noexcept {
    return ASMJIT_ARCH_LE ? readI32xLE<Alignment>(p) : readI32xBE<Alignment>(p);
  }

  static ASMJIT_INLINE uint32_t readU32a(const void* p) noexcept { return readU32x<4>(p); }
  static ASMJIT_INLINE uint32_t readU32u(const void* p) noexcept { return readU32x<1>(p); }

  static ASMJIT_INLINE uint32_t readU32aLE(const void* p) noexcept { return readU32xLE<4>(p); }
  static ASMJIT_INLINE uint32_t readU32uLE(const void* p) noexcept { return readU32xLE<1>(p); }

  static ASMJIT_INLINE uint32_t readU32aBE(const void* p) noexcept { return readU32xBE<4>(p); }
  static ASMJIT_INLINE uint32_t readU32uBE(const void* p) noexcept { return readU32xBE<1>(p); }

  static ASMJIT_INLINE int32_t readI32a(const void* p) noexcept { return readI32x<4>(p); }
  static ASMJIT_INLINE int32_t readI32u(const void* p) noexcept { return readI32x<1>(p); }

  static ASMJIT_INLINE int32_t readI32aLE(const void* p) noexcept { return readI32xLE<4>(p); }
  static ASMJIT_INLINE int32_t readI32uLE(const void* p) noexcept { return readI32xLE<1>(p); }

  static ASMJIT_INLINE int32_t readI32aBE(const void* p) noexcept { return readI32xBE<4>(p); }
  static ASMJIT_INLINE int32_t readI32uBE(const void* p) noexcept { return readI32xBE<1>(p); }

  template<size_t Alignment>
  static ASMJIT_INLINE uint64_t readU64xLE(const void* p) noexcept {
    if (ASMJIT_ARCH_LE && (ASMJIT_ARCH_UNALIGNED_64 || Alignment >= 8)) {
      typedef typename AlignedInt<uint64_t, Alignment>::T U64AlignedToN;
      return static_cast<const U64AlignedToN*>(p)[0];
    }
    else {
      uint32_t x = readU32xLE<Alignment < 8 ? Alignment : size_t(4)>(static_cast<const uint8_t*>(p) + 0);
      uint32_t y = readU32xLE<Alignment < 8 ? Alignment : size_t(4)>(static_cast<const uint8_t*>(p) + 4);
      return static_cast<uint64_t>(x) + (static_cast<uint64_t>(y) << 32);
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE uint64_t readU64xBE(const void* p) noexcept {
    if (ASMJIT_ARCH_BE && (ASMJIT_ARCH_UNALIGNED_64 || Alignment >= 8)) {
      typedef typename AlignedInt<uint64_t, Alignment>::T U64AlignedToN;
      return static_cast<const U64AlignedToN*>(p)[0];
    }
    else {
      uint32_t x = readU32xLE<Alignment < 8 ? Alignment : size_t(4)>(static_cast<const uint8_t*>(p) + 0);
      uint32_t y = readU32xLE<Alignment < 8 ? Alignment : size_t(4)>(static_cast<const uint8_t*>(p) + 4);
      return (static_cast<uint64_t>(x) << 32) + static_cast<uint64_t>(y);
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE uint64_t readU64x(const void* p) noexcept {
    return ASMJIT_ARCH_LE ? readU64xLE<Alignment>(p) : readU64xBE<Alignment>(p);
  }

  template<size_t Alignment>
  static ASMJIT_INLINE int64_t readI64xLE(const void* p) noexcept {
    return static_cast<int64_t>(readU64xLE<Alignment>(p));
  }

  template<size_t Alignment>
  static ASMJIT_INLINE int64_t readI64xBE(const void* p) noexcept {
    return static_cast<int64_t>(readU64xBE<Alignment>(p));
  }

  template<size_t Alignment>
  static ASMJIT_INLINE int64_t readI64x(const void* p) noexcept {
    return ASMJIT_ARCH_LE ? readI64xLE<Alignment>(p) : readI64xBE<Alignment>(p);
  }

  static ASMJIT_INLINE uint64_t readU64a(const void* p) noexcept { return readU64x<8>(p); }
  static ASMJIT_INLINE uint64_t readU64u(const void* p) noexcept { return readU64x<1>(p); }

  static ASMJIT_INLINE uint64_t readU64aLE(const void* p) noexcept { return readU64xLE<8>(p); }
  static ASMJIT_INLINE uint64_t readU64uLE(const void* p) noexcept { return readU64xLE<1>(p); }

  static ASMJIT_INLINE uint64_t readU64aBE(const void* p) noexcept { return readU64xBE<8>(p); }
  static ASMJIT_INLINE uint64_t readU64uBE(const void* p) noexcept { return readU64xBE<1>(p); }

  static ASMJIT_INLINE int64_t readI64a(const void* p) noexcept { return readI64x<8>(p); }
  static ASMJIT_INLINE int64_t readI64u(const void* p) noexcept { return readI64x<1>(p); }

  static ASMJIT_INLINE int64_t readI64aLE(const void* p) noexcept { return readI64xLE<8>(p); }
  static ASMJIT_INLINE int64_t readI64uLE(const void* p) noexcept { return readI64xLE<1>(p); }

  static ASMJIT_INLINE int64_t readI64aBE(const void* p) noexcept { return readI64xBE<8>(p); }
  static ASMJIT_INLINE int64_t readI64uBE(const void* p) noexcept { return readI64xBE<1>(p); }

  // --------------------------------------------------------------------------
  // [WriteMem]
  // --------------------------------------------------------------------------

  static ASMJIT_INLINE void writeU8(void* p, uint32_t x) noexcept {
    static_cast<uint8_t*>(p)[0] = static_cast<uint8_t>(x & 0xFFU);
  }

  static ASMJIT_INLINE void writeI8(void* p, int32_t x) noexcept {
    static_cast<uint8_t*>(p)[0] = static_cast<uint8_t>(x & 0xFF);
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeU16xLE(void* p, uint32_t x) noexcept {
    if (ASMJIT_ARCH_LE && (ASMJIT_ARCH_UNALIGNED_16 || Alignment >= 2)) {
      typedef typename AlignedInt<uint16_t, Alignment>::T U16AlignedToN;
      static_cast<U16AlignedToN*>(p)[0] = static_cast<uint16_t>(x & 0xFFFFU);
    }
    else {
      static_cast<uint8_t*>(p)[0] = static_cast<uint8_t>((x     ) & 0xFFU);
      static_cast<uint8_t*>(p)[1] = static_cast<uint8_t>((x >> 8) & 0xFFU);
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeU16xBE(void* p, uint32_t x) noexcept {
    if (ASMJIT_ARCH_BE && (ASMJIT_ARCH_UNALIGNED_16 || Alignment >= 2)) {
      typedef typename AlignedInt<uint16_t, Alignment>::T U16AlignedToN;
      static_cast<U16AlignedToN*>(p)[0] = static_cast<uint16_t>(x & 0xFFFFU);
    }
    else {
      static_cast<uint8_t*>(p)[0] = static_cast<uint8_t>((x >> 8) & 0xFFU);
      static_cast<uint8_t*>(p)[1] = static_cast<uint8_t>((x     ) & 0xFFU);
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeU16x(void* p, uint32_t x) noexcept {
    if (ASMJIT_ARCH_LE)
      writeU16xLE<Alignment>(p, x);
    else
      writeU16xBE<Alignment>(p, x);
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeI16xLE(void* p, int32_t x) noexcept {
    writeU16xLE<Alignment>(p, static_cast<uint32_t>(x));
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeI16xBE(void* p, int32_t x) noexcept {
    writeU16xBE<Alignment>(p, static_cast<uint32_t>(x));
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeI16x(void* p, int32_t x) noexcept {
    writeU16x<Alignment>(p, static_cast<uint32_t>(x));
  }

  static ASMJIT_INLINE void writeU16aLE(void* p, uint32_t x) noexcept { writeU16xLE<2>(p, x); }
  static ASMJIT_INLINE void writeU16uLE(void* p, uint32_t x) noexcept { writeU16xLE<1>(p, x); }

  static ASMJIT_INLINE void writeU16aBE(void* p, uint32_t x) noexcept { writeU16xBE<2>(p, x); }
  static ASMJIT_INLINE void writeU16uBE(void* p, uint32_t x) noexcept { writeU16xBE<1>(p, x); }

  static ASMJIT_INLINE void writeU16a(void* p, uint32_t x) noexcept { writeU16x<2>(p, x); }
  static ASMJIT_INLINE void writeU16u(void* p, uint32_t x) noexcept { writeU16x<1>(p, x); }

  static ASMJIT_INLINE void writeI16aLE(void* p, int32_t x) noexcept { writeI16xLE<2>(p, x); }
  static ASMJIT_INLINE void writeI16uLE(void* p, int32_t x) noexcept { writeI16xLE<1>(p, x); }

  static ASMJIT_INLINE void writeI16aBE(void* p, int32_t x) noexcept { writeI16xBE<2>(p, x); }
  static ASMJIT_INLINE void writeI16uBE(void* p, int32_t x) noexcept { writeI16xBE<1>(p, x); }

  static ASMJIT_INLINE void writeI16a(void* p, int32_t x) noexcept { writeI16x<2>(p, x); }
  static ASMJIT_INLINE void writeI16u(void* p, int32_t x) noexcept { writeI16x<1>(p, x); }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeU32xLE(void* p, uint32_t x) noexcept {
    if (ASMJIT_ARCH_UNALIGNED_32 || Alignment >= 4) {
      typedef typename AlignedInt<uint32_t, Alignment>::T U32AlignedToN;
      static_cast<U32AlignedToN*>(p)[0] = ASMJIT_ARCH_LE ? x : IntUtils::byteswap32(x);
    }
    else {
      writeU16xLE<Alignment < 4 ? Alignment : size_t(2)>(static_cast<uint8_t*>(p) + 0, x >> 16);
      writeU16xLE<Alignment < 4 ? Alignment : size_t(2)>(static_cast<uint8_t*>(p) + 2, x);
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeU32xBE(void* p, uint32_t x) noexcept {
    if (ASMJIT_ARCH_UNALIGNED_32 || Alignment >= 4) {
      typedef typename AlignedInt<uint32_t, Alignment>::T U32AlignedToN;
      static_cast<U32AlignedToN*>(p)[0] = ASMJIT_ARCH_BE ? x : IntUtils::byteswap32(x);
    }
    else {
      writeU16xBE<Alignment < 4 ? Alignment : size_t(2)>(static_cast<uint8_t*>(p) + 0, x);
      writeU16xBE<Alignment < 4 ? Alignment : size_t(2)>(static_cast<uint8_t*>(p) + 2, x >> 16);
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeU32x(void* p, uint32_t x) noexcept {
    if (ASMJIT_ARCH_LE)
      writeU32xLE<Alignment>(p, x);
    else
      writeU32xBE<Alignment>(p, x);
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeI32xLE(void* p, int32_t x) noexcept {
    writeU32xLE<Alignment>(p, static_cast<uint32_t>(x));
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeI32xBE(void* p, int32_t x) noexcept {
    writeU32xBE<Alignment>(p, static_cast<uint32_t>(x));
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeI32x(void* p, int32_t x) noexcept {
    writeU32x<Alignment>(p, static_cast<uint32_t>(x));
  }

  static ASMJIT_INLINE void writeU32aLE(void* p, uint32_t x) noexcept { writeU32xLE<4>(p, x); }
  static ASMJIT_INLINE void writeU32uLE(void* p, uint32_t x) noexcept { writeU32xLE<1>(p, x); }

  static ASMJIT_INLINE void writeU32aBE(void* p, uint32_t x) noexcept { writeU32xBE<4>(p, x); }
  static ASMJIT_INLINE void writeU32uBE(void* p, uint32_t x) noexcept { writeU32xBE<1>(p, x); }

  static ASMJIT_INLINE void writeU32a(void* p, uint32_t x) noexcept { writeU32x<4>(p, x); }
  static ASMJIT_INLINE void writeU32u(void* p, uint32_t x) noexcept { writeU32x<1>(p, x); }

  static ASMJIT_INLINE void writeI32aLE(void* p, int32_t x) noexcept { writeI32xLE<4>(p, x); }
  static ASMJIT_INLINE void writeI32uLE(void* p, int32_t x) noexcept { writeI32xLE<1>(p, x); }

  static ASMJIT_INLINE void writeI32aBE(void* p, int32_t x) noexcept { writeI32xBE<4>(p, x); }
  static ASMJIT_INLINE void writeI32uBE(void* p, int32_t x) noexcept { writeI32xBE<1>(p, x); }

  static ASMJIT_INLINE void writeI32a(void* p, int32_t x) noexcept { writeI32x<4>(p, x); }
  static ASMJIT_INLINE void writeI32u(void* p, int32_t x) noexcept { writeI32x<1>(p, x); }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeU64xLE(void* p, uint64_t x) noexcept {
    if (ASMJIT_ARCH_LE && (ASMJIT_ARCH_UNALIGNED_64 || Alignment >= 8)) {
      typedef typename AlignedInt<uint64_t, Alignment>::T U64AlignedToN;
      static_cast<U64AlignedToN*>(p)[0] = x;
    }
    else {
      writeU32xLE<Alignment < 8 ? Alignment : size_t(4)>(static_cast<uint8_t*>(p) + 0, static_cast<uint32_t>(x >> 32));
      writeU32xLE<Alignment < 8 ? Alignment : size_t(4)>(static_cast<uint8_t*>(p) + 4, static_cast<uint32_t>(x & 0xFFFFFFFFU));
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeU64xBE(void* p, uint64_t x) noexcept {
    if (ASMJIT_ARCH_BE && (ASMJIT_ARCH_UNALIGNED_64 || Alignment >= 8)) {
      typedef typename AlignedInt<uint64_t, Alignment>::T U64AlignedToN;
      static_cast<U64AlignedToN*>(p)[0] = x;
    }
    else {
      writeU32xBE<Alignment < 8 ? Alignment : size_t(4)>(static_cast<uint8_t*>(p) + 0, static_cast<uint32_t>(x & 0xFFFFFFFFU));
      writeU32xBE<Alignment < 8 ? Alignment : size_t(4)>(static_cast<uint8_t*>(p) + 4, static_cast<uint32_t>(x >> 32));
    }
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeU64x(void* p, uint64_t x) noexcept {
    if (ASMJIT_ARCH_LE)
      writeU64xLE<Alignment>(p, x);
    else
      writeU64xBE<Alignment>(p, x);
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeI64xLE(void* p, int64_t x) noexcept {
    writeU64xLE<Alignment>(p, static_cast<uint64_t>(x));
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeI64xBE(void* p, int64_t x) noexcept {
    writeU64xBE<Alignment>(p, static_cast<uint64_t>(x));
  }

  template<size_t Alignment>
  static ASMJIT_INLINE void writeI64x(void* p, int64_t x) noexcept {
    writeU64x<Alignment>(p, static_cast<uint64_t>(x));
  }

  static ASMJIT_INLINE void writeU64aLE(void* p, uint64_t x) noexcept { writeU64xLE<8>(p, x); }
  static ASMJIT_INLINE void writeU64uLE(void* p, uint64_t x) noexcept { writeU64xLE<1>(p, x); }

  static ASMJIT_INLINE void writeU64aBE(void* p, uint64_t x) noexcept { writeU64xBE<8>(p, x); }
  static ASMJIT_INLINE void writeU64uBE(void* p, uint64_t x) noexcept { writeU64xBE<1>(p, x); }

  static ASMJIT_INLINE void writeU64a(void* p, uint64_t x) noexcept { writeU64x<8>(p, x); }
  static ASMJIT_INLINE void writeU64u(void* p, uint64_t x) noexcept { writeU64x<1>(p, x); }

  static ASMJIT_INLINE void writeI64aLE(void* p, int64_t x) noexcept { writeI64xLE<8>(p, x); }
  static ASMJIT_INLINE void writeI64uLE(void* p, int64_t x) noexcept { writeI64xLE<1>(p, x); }

  static ASMJIT_INLINE void writeI64aBE(void* p, int64_t x) noexcept { writeI64xBE<8>(p, x); }
  static ASMJIT_INLINE void writeI64uBE(void* p, int64_t x) noexcept { writeI64xBE<1>(p, x); }

  static ASMJIT_INLINE void writeI64a(void* p, int64_t x) noexcept { writeI64x<8>(p, x); }
  static ASMJIT_INLINE void writeI64u(void* p, int64_t x) noexcept { writeI64x<1>(p, x); }
} // Utils namespace

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_UTILS_H
