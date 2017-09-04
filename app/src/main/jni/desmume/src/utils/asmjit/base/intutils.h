// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_INTUTILS_H
#define _ASMJIT_BASE_INTUTILS_H

// [Dependencies]
#include "../base/globals.h"

#if ASMJIT_CC_MSC_GE(14, 0, 0)
# include <intrin.h>
#endif

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::IntUtils]
// ============================================================================

//! Utilities related to integers and bitwords.
namespace IntUtils {
  // --------------------------------------------------------------------------
  // [IntTraits]
  // --------------------------------------------------------------------------

  template<size_t Size, int IsSigned>
  struct IntTraitsImpl {}; // Would fail if not specialized.

  template<> struct IntTraitsImpl<1, 0> { typedef int     IntType; typedef uint32_t UIntType; typedef int8_t  SignedType; typedef uint8_t  UnsignedType; };
  template<> struct IntTraitsImpl<1, 1> { typedef int     IntType; typedef uint32_t UIntType; typedef int8_t  SignedType; typedef uint8_t  UnsignedType; };
  template<> struct IntTraitsImpl<2, 0> { typedef int     IntType; typedef uint32_t UIntType; typedef int16_t SignedType; typedef uint16_t UnsignedType; };
  template<> struct IntTraitsImpl<2, 1> { typedef int     IntType; typedef uint32_t UIntType; typedef int16_t SignedType; typedef uint16_t UnsignedType; };
  template<> struct IntTraitsImpl<4, 0> { typedef int64_t IntType; typedef uint32_t UIntType; typedef int32_t SignedType; typedef uint32_t UnsignedType; };
  template<> struct IntTraitsImpl<4, 1> { typedef int     IntType; typedef uint32_t UIntType; typedef int32_t SignedType; typedef uint32_t UnsignedType; };
  template<> struct IntTraitsImpl<8, 0> { typedef int64_t IntType; typedef uint64_t UIntType; typedef int64_t SignedType; typedef uint64_t UnsignedType; };
  template<> struct IntTraitsImpl<8, 1> { typedef int64_t IntType; typedef uint64_t UIntType; typedef int64_t SignedType; typedef uint64_t UnsignedType; };

  template<typename T>
  struct IntTraits {
    enum {
      kIsSigned   = static_cast<T>(~static_cast<T>(0)) < static_cast<T>(0),
      kIsUnsigned = !kIsSigned,
      kIs8Bit     = sizeof(T) == 1,
      kIs16Bit    = sizeof(T) == 2,
      kIs32Bit    = sizeof(T) == 4,
      kIs64Bit    = sizeof(T) == 8,
      kIsIntPtr   = sizeof(T) == sizeof(intptr_t)
    };

    typedef typename IntTraitsImpl<sizeof(T), kIsSigned>::IntType IntType;
    typedef typename IntTraitsImpl<sizeof(T), kIsSigned>::UIntType UIntType;
    typedef typename IntTraitsImpl<sizeof(T), kIsSigned>::SignedType SignedType;
    typedef typename IntTraitsImpl<sizeof(T), kIsSigned>::UnsignedType UnsignedType;
  };

  // --------------------------------------------------------------------------
  // [Float <-> Int]
  // --------------------------------------------------------------------------

  //! \internal
  union FloatBits {
    int32_t i;
    float f;
  };

  //! \internal
  union DoubleBits {
    int64_t i;
    double d;
  };

  //! Bit-cast `float` to a 32-bit integer.
  static ASMJIT_INLINE int32_t floatAsInt(float f) noexcept { FloatBits m; m.f = f; return m.i; }
  //! Bit-cast 32-bit integer to `float`.
  static ASMJIT_INLINE float intAsFloat(int32_t i) noexcept { FloatBits m; m.i = i; return m.f; }

  //! Bit-cast `double` to a 64-bit integer.
  static ASMJIT_INLINE int64_t doubleAsInt(double d) noexcept { DoubleBits m; m.d = d; return m.i; }
  //! Bit-cast 64-bit integer to `double`.
  static ASMJIT_INLINE double intAsDouble(int64_t i) noexcept { DoubleBits m; m.i = i; return m.d; }

  // --------------------------------------------------------------------------
  // [FastUInt8]
  // --------------------------------------------------------------------------

#if ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64
  typedef uint8_t FastUInt8;
#else
  typedef unsigned int FastUInt8;
#endif

  // --------------------------------------------------------------------------
  // [AsInt / AsUInt]
  // --------------------------------------------------------------------------

  //! Map an integer `x` of type `T` to `int` or `int64_t` depending on the
  //! type. Used internally by AsmJit to dispatch arguments that can be of
  //! arbitrary integer type into a function argument that is either `int` or
  //! `int64_t`.
  template<typename T>
  static ASMJIT_INLINE typename IntTraits<T>::IntType asInt(T x) noexcept {
    return static_cast<typename IntTraits<T>::IntType>(x);
  }

  template<typename T>
  static ASMJIT_INLINE typename IntTraits<T>::UIntType asUInt(T x) noexcept {
    return static_cast<typename IntTraits<T>::UIntType>(x);
  }

  // --------------------------------------------------------------------------
  // [ToUInt8]
  // --------------------------------------------------------------------------

  template<typename T>
  static ASMJIT_INLINE uint8_t toUInt8(T x) noexcept {
    ASMJIT_ASSERT(x <= T(0xFF));
    return static_cast<uint8_t>(x);
  }

  template<typename T>
  static ASMJIT_INLINE uint8_t toUInt16(T x) noexcept {
    ASMJIT_ASSERT(x <= T(0xFFFF));
    return static_cast<uint8_t>(x);
  }

  // --------------------------------------------------------------------------
  // [Pack / Unpack]
  // --------------------------------------------------------------------------

  //! Pack four 8-bit integer into a 32-bit integer as it is an array of `{b0,b1,b2,b3}`.
  static ASMJIT_INLINE uint32_t pack32_4x8(uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3) noexcept {
    return ASMJIT_PACK32_4x8(b0, b1, b2, b3);
  }

  //! Pack two 32-bit integer into a 64-bit integer as it is an array of `{u0,u1}`.
  static ASMJIT_INLINE uint64_t pack64_2x32(uint32_t u0, uint32_t u1) noexcept {
    return ASMJIT_ARCH_LE ? (static_cast<uint64_t>(u1) << 32) + u0
                          : (static_cast<uint64_t>(u0) << 32) + u1;
  }

  // --------------------------------------------------------------------------
  // [Position of byte (in bit-shift)]
  // --------------------------------------------------------------------------

  static ASMJIT_INLINE uint32_t byteShiftOfDWordStruct(uint32_t index) noexcept {
    if (ASMJIT_ARCH_LE)
      return index * 8;
    else
      return (static_cast<uint32_t>(sizeof(uint32_t)) - 1 - index) * 8;
  }

  // --------------------------------------------------------------------------
  // [MinValue / MaxValue]
  // --------------------------------------------------------------------------

  //! Get a minimum value of `T`.
  template<typename T>
  static ASMJIT_INLINE T minValue() noexcept {
    typedef typename IntTraits<T>::UnsignedType UnsignedType;
    return IntTraits<T>::kIsUnsigned ? T(0) : T((UnsignedType(~UnsignedType(0)) >> 1) + UnsignedType(1));
  }

  //! Get a maximum value of `T`.
  template<typename T>
  static ASMJIT_INLINE T maxValue() noexcept {
    typedef typename IntTraits<T>::UnsignedType UnsignedType;
    return IntTraits<T>::kIsUnsigned ? T(~UnsignedType(0)) : T(UnsignedType(~UnsignedType(0)) >> 1);
  }

  // --------------------------------------------------------------------------
  // [BLSI]
  // --------------------------------------------------------------------------

  //! \internal
  template<typename T>
  static ASMJIT_INLINE T _blsiImpl(T x) noexcept { return x & (T(0) - x); }

  //! Extract lowest set isolated bit, like BLSI instruction, but portable.
  template<typename T>
  static ASMJIT_INLINE T blsi(T x) noexcept { return static_cast<T>(_blsiImpl(asUInt(x))); }

  // --------------------------------------------------------------------------
  // [CTZ]
  // --------------------------------------------------------------------------

  //! \internal
  static ASMJIT_INLINE uint32_t _ctzGeneric(uint32_t x) noexcept {
    x &= uint32_t(0) - x;

    uint32_t c = 31;
    if (x & 0x0000FFFFU) c -= 16;
    if (x & 0x00FF00FFU) c -= 8;
    if (x & 0x0F0F0F0FU) c -= 4;
    if (x & 0x33333333U) c -= 2;
    if (x & 0x55555555U) c -= 1;

    return c;
  }

  //! \internal
  static ASMJIT_INLINE uint32_t _ctzGeneric(uint64_t x) noexcept {
    x &= uint64_t(0) - x;

    uint32_t c = 63;
    if (x & ASMJIT_UINT64_C(0x00000000FFFFFFFF)) c -= 32;
    if (x & ASMJIT_UINT64_C(0x0000FFFF0000FFFF)) c -= 16;
    if (x & ASMJIT_UINT64_C(0x00FF00FF00FF00FF)) c -= 8;
    if (x & ASMJIT_UINT64_C(0x0F0F0F0F0F0F0F0F)) c -= 4;
    if (x & ASMJIT_UINT64_C(0x3333333333333333)) c -= 2;
    if (x & ASMJIT_UINT64_C(0x5555555555555555)) c -= 1;

    return c;
  }

  //! \internal
  static ASMJIT_INLINE uint32_t _ctzImpl(uint32_t x) noexcept {
#if ASMJIT_CC_MSC_GE(14, 0, 0) && (ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64 || ASMJIT_ARCH_ARM32 || ASMJIT_ARCH_ARM64)
    unsigned long i;
    _BitScanForward(&i, x);
    return static_cast<uint32_t>(i);
#elif ASMJIT_CC_GCC_GE(3, 4, 6) || ASMJIT_CC_CLANG
    return static_cast<uint32_t>(__builtin_ctz(x));
#else
    return _ctzGeneric(x);
#endif
  }

  //! \internal
  static ASMJIT_INLINE uint32_t _ctzImpl(uint64_t x) noexcept {
#if ASMJIT_CC_MSC_GE(14, 0, 0) && (ASMJIT_ARCH_X64 || ASMJIT_ARCH_ARM64)
    unsigned long i;
    _BitScanForward64(&i, x);
    return static_cast<uint32_t>(i);
#elif ASMJIT_CC_GCC_GE(3, 4, 6) || ASMJIT_CC_CLANG
    return static_cast<uint32_t>(__builtin_ctzll(x));
#else
    return _ctzGeneric(x);
#endif
  }

  //! Count trailing zeros in `x` (returns a position of a first bit set in `x`).
  //!
  //! NOTE: The input MUST NOT be zero, otherwise the result is undefined.
  template<typename T>
  static ASMJIT_INLINE uint32_t ctz(T x) noexcept { return _ctzImpl(asUInt(x)); }

  template<uint64_t N>
  struct StaticCtzImpl {
    enum {
      _kTmp1 = 0      + (((N            ) & uint64_t(0xFFFFFFFFU)) == 0 ? 32 : 0),
      _kTmp2 = _kTmp1 + (((N >> (_kTmp1)) & uint64_t(0x0000FFFFU)) == 0 ? 16 : 0),
      _kTmp3 = _kTmp2 + (((N >> (_kTmp2)) & uint64_t(0x000000FFU)) == 0 ?  8 : 0),
      _kTmp4 = _kTmp3 + (((N >> (_kTmp3)) & uint64_t(0x0000000FU)) == 0 ?  4 : 0),
      _kTmp5 = _kTmp4 + (((N >> (_kTmp4)) & uint64_t(0x00000003U)) == 0 ?  2 : 0),
      kValue = _kTmp5 + (((N >> (_kTmp5)) & uint64_t(0x00000001U)) == 0 ?  1 : 0)
    };
  };

  template<>
  struct StaticCtzImpl<0> {}; // Undefined.

  template<uint64_t N>
  static ASMJIT_INLINE uint32_t staticCtz() noexcept { return StaticCtzImpl<N>::kValue; }

  // --------------------------------------------------------------------------
  // [Popcnt]
  // --------------------------------------------------------------------------

  // Based on the following resource:
  //   http://graphics.stanford.edu/~seander/bithacks.html
  //
  // Alternatively, for a very small number of bits in `x`:
  //   uint32_t n = 0;
  //   while (x) {
  //     x &= x - 1;
  //     n++;
  //   }
  //   return n;

  //! \internal
  static ASMJIT_INLINE uint32_t _popcntGeneric(uint32_t x) noexcept {
    x = x - ((x >> 1) & 0x55555555U);
    x = (x & 0x33333333U) + ((x >> 2) & 0x33333333U);
    return (((x + (x >> 4)) & 0x0F0F0F0FU) * 0x01010101U) >> 24;
  }

  //! \internal
  static ASMJIT_INLINE uint32_t _popcntGeneric(uint64_t x) noexcept {
    if (ASMJIT_ARCH_64BIT) {
      x = x - ((x >> 1) & ASMJIT_UINT64_C(0x5555555555555555));
      x = (x & ASMJIT_UINT64_C(0x3333333333333333)) + ((x >> 2) & ASMJIT_UINT64_C(0x3333333333333333));
      return static_cast<uint32_t>(
        (((x + (x >> 4)) & ASMJIT_UINT64_C(0x0F0F0F0F0F0F0F0F)) * ASMJIT_UINT64_C(0x0101010101010101)) >> 56);
    }
    else {
      return _popcntGeneric(static_cast<uint32_t>(x >> 32)) +
             _popcntGeneric(static_cast<uint32_t>(x & 0xFFFFFFFFU));
    }
  }

  //! \internal
  static ASMJIT_INLINE uint32_t _popcntImpl(uint32_t x) noexcept {
#if ASMJIT_CC_GCC || ASMJIT_CC_CLANG
    return static_cast<uint32_t>(__builtin_popcount(x));
#else
    return _popcntGeneric(asUInt(x));
#endif
  }

  //! \internal
  static ASMJIT_INLINE uint32_t _popcntImpl(uint64_t x) noexcept {
#if ASMJIT_CC_GCC || ASMJIT_CC_CLANG
    return static_cast<uint32_t>(__builtin_popcountll(x));
#else
    return _popcntGeneric(asUInt(x));
#endif
  }

  //! Get count of bits in `x`.
  template<typename T>
  static ASMJIT_INLINE uint32_t popcnt(T x) noexcept { return _popcntImpl(asUInt(x)); }

  // --------------------------------------------------------------------------
  // [IsBetween]
  // --------------------------------------------------------------------------

  //! Get whether `x` is greater than or equal to `a` and lesses than or equal to `b`.
  template<typename T>
  static ASMJIT_INLINE bool isBetween(T x, T a, T b) noexcept { return x >= a && x <= b; }

  // --------------------------------------------------------------------------
  // [IsInt / IsUInt]
  // --------------------------------------------------------------------------

  //! Get whether the given integer `x` can be casted to a 4-bit signed integer.
  template<typename T>
  static ASMJIT_INLINE bool isInt4(T x) noexcept {
    typedef typename IntTraits<T>::SignedType SignedType;
    typedef typename IntTraits<T>::UnsignedType UnsignedType;

    if (IntTraits<T>::kIsSigned)
      return isBetween<SignedType>(SignedType(x), -8, 7);
    else
      return UnsignedType(x) <= UnsignedType(7U);
  }

  //! Get whether the given integer `x` can be casted to an 8-bit signed integer.
  template<typename T>
  static ASMJIT_INLINE bool isInt8(T x) noexcept {
    typedef typename IntTraits<T>::SignedType SignedType;
    typedef typename IntTraits<T>::UnsignedType UnsignedType;

    if (IntTraits<T>::kIsSigned)
      return sizeof(T) <= 1 || isBetween<SignedType>(SignedType(x), -128, 127);
    else
      return UnsignedType(x) <= UnsignedType(127U);
  }

  //! Get whether the given integer `x` can be casted to a 16-bit signed integer.
  template<typename T>
  static ASMJIT_INLINE bool isInt16(T x) noexcept {
    typedef typename IntTraits<T>::SignedType SignedType;
    typedef typename IntTraits<T>::UnsignedType UnsignedType;

    if (IntTraits<T>::kIsSigned)
      return sizeof(T) <= 2 || isBetween<SignedType>(SignedType(x), -32768, 32767);
    else
      return sizeof(T) <= 1 || UnsignedType(x) <= UnsignedType(32767U);
  }

  //! Get whether the given integer `x` can be casted to a 32-bit signed integer.
  template<typename T>
  static ASMJIT_INLINE bool isInt32(T x) noexcept {
    typedef typename IntTraits<T>::SignedType SignedType;
    typedef typename IntTraits<T>::UnsignedType UnsignedType;

    if (IntTraits<T>::kIsSigned)
      return sizeof(T) <= 4 || isBetween<SignedType>(SignedType(x), -2147483647 - 1, 2147483647);
    else
      return sizeof(T) <= 2 || UnsignedType(x) <= UnsignedType(2147483647U);
  }

  //! Get whether the given integer `x` can be casted to a 4-bit unsigned integer.
  template<typename T>
  static ASMJIT_INLINE bool isUInt4(T x) noexcept {
    typedef typename IntTraits<T>::UnsignedType UnsignedType;

    if (IntTraits<T>::kIsSigned)
      return x >= T(0) && x <= T(15);
    else
      return UnsignedType(x) <= UnsignedType(15U);
  }

  //! Get whether the given integer `x` can be casted to an 8-bit unsigned integer.
  template<typename T>
  static ASMJIT_INLINE bool isUInt8(T x) noexcept {
    typedef typename IntTraits<T>::UnsignedType UnsignedType;

    if (IntTraits<T>::kIsSigned)
      return x >= T(0) && (sizeof(T) <= 1 ? true : x <= T(255));
    else
      return sizeof(T) <= 1 || UnsignedType(x) <= UnsignedType(255U);
  }

  //! Get whether the given integer `x` can be casted to a 12-bit unsigned integer (ARM specific).
  template<typename T>
  static ASMJIT_INLINE bool isUInt12(T x) noexcept {
    typedef typename IntTraits<T>::UnsignedType UnsignedType;

    if (IntTraits<T>::kIsSigned)
      return x >= T(0) && (sizeof(T) <= 1 ? true : x <= T(4095));
    else
      return sizeof(T) <= 1 || UnsignedType(x) <= UnsignedType(4095U);
  }

  //! Get whether the given integer `x` can be casted to a 16-bit unsigned integer.
  template<typename T>
  static ASMJIT_INLINE bool isUInt16(T x) noexcept {
    typedef typename IntTraits<T>::UnsignedType UnsignedType;

    if (IntTraits<T>::kIsSigned)
      return x >= T(0) && (sizeof(T) <= 2 ? true : x <= T(65535));
    else
      return sizeof(T) <= 2 || UnsignedType(x) <= UnsignedType(65535U);
  }

  //! Get whether the given integer `x` can be casted to a 32-bit unsigned integer.
  template<typename T>
  static ASMJIT_INLINE bool isUInt32(T x) noexcept {
    typedef typename IntTraits<T>::UnsignedType UnsignedType;

    if (IntTraits<T>::kIsSigned)
      return x >= T(0) && (sizeof(T) <= 4 ? true : x <= T(4294967295U));
    else
      return sizeof(T) <= 4 || UnsignedType(x) <= UnsignedType(4294967295U);
  }

  // --------------------------------------------------------------------------
  // [Align]
  // --------------------------------------------------------------------------

  template<typename X, typename Y>
  static ASMJIT_INLINE bool isAligned(X base, Y alignment) noexcept {
    typedef typename IntTraitsImpl<sizeof(X), 0>::UnsignedType U;
    return ((U)base % (U)alignment) == 0;
  }

  template<typename X, typename Y>
  static ASMJIT_INLINE X alignTo(X x, Y alignment) noexcept {
    typedef typename IntTraitsImpl<sizeof(X), 0>::UnsignedType U;
    return (X)( ((U)x + (U)(alignment - 1)) & ~((U)(alignment - 1)) );
  }

  //! Get delta required to align `base` to `alignment`.
  template<typename X, typename Y>
  static ASMJIT_INLINE X alignDiff(X base, Y alignment) noexcept {
    return alignTo(base, alignment) - base;
  }

  // --------------------------------------------------------------------------
  // [IsPowerOf2 / AlignToPowerOf2]
  // --------------------------------------------------------------------------

  //! Get whether the `x` is a power of two (only one bit is set).
  template<typename T>
  static ASMJIT_INLINE bool isPowerOf2(T x) noexcept { return x != 0 && (x & (x - 1)) == 0; }

  template<typename T>
  static ASMJIT_INLINE T alignToPowerOf2(T base) noexcept {
    // Implementation is from "Hacker's Delight" by Henry S. Warren, Jr.
    base -= 1;

#if ASMJIT_CC_MSC
# pragma warning(push)
# pragma warning(disable: 4293)
#endif

    base = base | (base >> 1);
    base = base | (base >> 2);
    base = base | (base >> 4);

    // 8/16/32 constants are multiplied by the condition to prevent a compiler
    // complaining about the 'shift count >= type width' (GCC).
    if (sizeof(T) >= 2) base = base | (base >> ( 8 * (sizeof(T) >= 2))); // Base >>  8.
    if (sizeof(T) >= 4) base = base | (base >> (16 * (sizeof(T) >= 4))); // Base >> 16.
    if (sizeof(T) >= 8) base = base | (base >> (32 * (sizeof(T) >= 8))); // Base >> 32.

#if ASMJIT_CC_MSC
# pragma warning(pop)
#endif

    return base + 1;
  }

  // --------------------------------------------------------------------------
  // [Mask]
  // --------------------------------------------------------------------------

  //! Generate a bit-mask that has `x` bit set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x) noexcept {
    ASMJIT_ASSERT(x < 32);
    return static_cast<uint32_t>(1) << x;
  }

  //! Generate a bit-mask that has `x0` and `x1` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1) noexcept {
    return mask(x0) | mask(x1);
  }

  //! Generate a bit-mask that has `x0`, `x1` and `x2` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2) noexcept {
    return mask(x0, x1) | mask(x2);
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2` and `x3` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3) noexcept {
    return mask(x0, x1) | mask(x2, x3);
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3` and `x4` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4) noexcept {
    return mask(x0, x1) | mask(x2, x3) | mask(x4);
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3`, `x4` and `x5` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5) noexcept {
    return mask(x0, x1) | mask(x2, x3) | mask(x4, x5);
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3`, `x4`, `x5` and `x6` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5, uint32_t x6) noexcept {
    return mask(x0, x1) | mask(x2, x3) | mask(x4, x5) | mask(x6);
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3`, `x4`, `x5`, `x6` and `x7` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5, uint32_t x6, uint32_t x7) noexcept {
    return mask(x0, x1) | mask(x2, x3) | mask(x4, x5) | mask(x6, x7);
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3`, `x4`, `x5`, `x6`, `x7` and `x8` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5, uint32_t x6, uint32_t x7, uint32_t x8) noexcept {
    return mask(x0, x1) | mask(x2, x3) | mask(x4, x5) | mask(x6, x7) | mask(x8);
  }

  //! Generate a bit-mask that has `x0`, `x1`, `x2`, `x3`, `x4`, `x5`, `x6`, `x7`, `x8` and `x9` bits set.
  static ASMJIT_INLINE uint32_t mask(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5, uint32_t x6, uint32_t x7, uint32_t x8, uint32_t x9) noexcept {
    return mask(x0, x1) | mask(x2, x3) | mask(x4, x5) | mask(x6, x7) | mask(x8, x9);
  }

  // --------------------------------------------------------------------------
  // [Bits]
  // --------------------------------------------------------------------------

  //! Generate a bit-mask that has `x` least significant bits set.
  static ASMJIT_INLINE uint32_t bits(uint32_t x) noexcept {
    // Shifting more bits than the type has results in undefined behavior. In
    // such case asmjit trashes the result by ORing with `overflow` mask, which
    // discards the undefined value returned by the shift.
    uint32_t overflow = uint32_t(0) - uint32_t(x >= sizeof(uint32_t) * 8);
    return ((uint32_t(1) << x) - uint32_t(1)) | overflow;
  }

  //! Convert a boolean value `b` to zero or full mask (all bits set).
  template<typename Dst, typename Src>
  static ASMJIT_INLINE Dst maskFromBool(Src b) noexcept {
    typedef typename IntTraits<Dst>::UnsignedType UnsignedType;
    return static_cast<Dst>(UnsignedType(0) - static_cast<UnsignedType>(b));
  }

  // --------------------------------------------------------------------------
  // [HasBit]
  // --------------------------------------------------------------------------

  //! Get whether `x` has bit `n` set.
  template<typename T, typename Index>
  static ASMJIT_INLINE bool hasBit(T x, Index n) noexcept {
    return (x & (static_cast<T>(1) << n)) != 0;
  }

  // --------------------------------------------------------------------------
  // [ByteSwap]
  // --------------------------------------------------------------------------

  static ASMJIT_INLINE uint32_t byteswap32(uint32_t x) noexcept {
#if ASMJIT_CC_MSC
    return static_cast<uint32_t>(_byteswap_ulong(x));
#elif ASMJIT_CC_GCC_GE(4, 3, 0) || ASMJIT_CC_CLANG_GE(2, 6, 0)
    return __builtin_bswap32(x);
#else
    uint32_t y = x & 0x00FFFF00U;
    x = (x << 24) + (x >> 24);
    y = (y <<  8) + (y >>  8);
    return x + (y & 0x00FFFF00U);
#endif
  }

  // --------------------------------------------------------------------------
  // [Operators]
  // --------------------------------------------------------------------------

  struct And    { template<typename T> static ASMJIT_INLINE T op(T x, T y) noexcept { return  x &  y; } };
  struct AndNot { template<typename T> static ASMJIT_INLINE T op(T x, T y) noexcept { return  x & ~y; } };
  struct NotAnd { template<typename T> static ASMJIT_INLINE T op(T x, T y) noexcept { return ~x &  y; } };
  struct Or     { template<typename T> static ASMJIT_INLINE T op(T x, T y) noexcept { return  x |  y; } };
  struct Xor    { template<typename T> static ASMJIT_INLINE T op(T x, T y) noexcept { return  x ^  y; } };
  struct Add    { template<typename T> static ASMJIT_INLINE T op(T x, T y) noexcept { return  x +  y; } };
  struct Sub    { template<typename T> static ASMJIT_INLINE T op(T x, T y) noexcept { return  x -  y; } };
  struct Min    { template<typename T> static ASMJIT_INLINE T op(T x, T y) noexcept { return std::min<T>(x, y); } };
  struct Max    { template<typename T> static ASMJIT_INLINE T op(T x, T y) noexcept { return std::max<T>(x, y); } };

  // --------------------------------------------------------------------------
  // [Iterators]
  // --------------------------------------------------------------------------

  template<typename BitWordT>
  ASMJIT_INLINE uint32_t _ctzPlusOneAndShift(BitWordT& bitWord) {
    uint32_t x = ctz(bitWord);

    if (sizeof(BitWordT) < sizeof(Globals::BitWord)) {
      // One instruction less on most architectures, no undefined behavior.
      bitWord = static_cast<BitWordT>(static_cast<Globals::BitWord>(bitWord) >> ++x);
    }
    else {
      bitWord >>= x++;
      bitWord >>= 1U;
    }

    return x;
  }

  //! Iterates over each bit in a number which is set to 1.
  //!
  //! Example of use:
  //!
  //! ```
  //! uint32_t bitsToIterate = 0x110F;
  //! IntUtils::BitWordIterator<uint32_t> it(bitsToIterate);
  //!
  //! while (it.hasNext()) {
  //!   uint32_t bitIndex = it.next();
  //!   printf("Bit at %u is set\n", bitIndex);
  //! }
  //! ```
  template<typename BitWordT>
  class BitWordIterator {
  public:
    explicit ASMJIT_INLINE BitWordIterator(BitWordT bitWord) noexcept { init(bitWord); }

    ASMJIT_INLINE void init(BitWordT bitWord) noexcept {
      _bitWord = bitWord;
      _index = ~uint32_t(0);
    }

    ASMJIT_INLINE bool hasNext() const noexcept { return _bitWord != 0; }
    ASMJIT_INLINE uint32_t next() noexcept {
      ASMJIT_ASSERT(_bitWord != 0);
      _index += _ctzPlusOneAndShift(_bitWord);
      return _index;
    }

    BitWordT _bitWord;
    uint32_t _index;
  };

  template<typename BitWordT>
  class BitArrayIterator {
  public:
    enum { kBitWordSizeInBits = static_cast<int>(sizeof(BitWordT)) * 8 };

    ASMJIT_INLINE BitArrayIterator(const BitWordT* data, uint32_t count) noexcept {
      init(data, count);
    }

    ASMJIT_INLINE void init(const BitWordT* data, uint32_t count) noexcept {
      const BitWordT* ptr = data;
      const BitWordT* end = data + count;

      BitWordT bitWord = BitWordT(0);
      uint32_t bitIndex = ~uint32_t(0);

      while (ptr != end) {
        bitWord = *ptr++;
        if (bitWord)
          break;
        bitIndex += kBitWordSizeInBits;
      }

      _ptr = ptr;
      _end = end;
      _current = bitWord;
      _bitIndex = bitIndex;
    }

    ASMJIT_INLINE bool hasNext() noexcept {
      return _current != BitWordT(0);
    }

    ASMJIT_INLINE uint32_t next() noexcept {
      BitWordT bitWord = _current;
      uint32_t bitIndex = _bitIndex;
      ASMJIT_ASSERT(bitWord != BitWordT(0));

      bitIndex += _ctzPlusOneAndShift(bitWord);
      uint32_t retIndex = bitIndex;

      if (!bitWord) {
        bitIndex |= uint32_t(kBitWordSizeInBits - 1);
        while (_ptr != _end) {
          bitWord = *_ptr++;
          if (bitWord)
            break;
          bitIndex += kBitWordSizeInBits;
        }
      }

      _current = bitWord;
      _bitIndex = bitIndex;
      return retIndex;
    }

    const BitWordT* _ptr;
    const BitWordT* _end;
    BitWordT _current;
    uint32_t _bitIndex;
  };

  template<typename BitWordT, class Operator>
  class BitArrayOpIterator {
  public:
    enum { kBitWordSizeInBits = static_cast<int>(sizeof(BitWordT)) * 8 };

    ASMJIT_INLINE BitArrayOpIterator(const BitWordT* aData, const BitWordT* bData, uint32_t count) noexcept {
      init(aData, bData, count);
    }

    ASMJIT_INLINE void init(const BitWordT* aData, const BitWordT* bData, uint32_t count) noexcept {
      const BitWordT* aPtr = aData;
      const BitWordT* bPtr = bData;
      const BitWordT* aEnd = aData + count;

      BitWordT bitWord = BitWordT(0);
      uint32_t bitIndex = ~uint32_t(0);

      while (aPtr != aEnd) {
        bitWord = Operator::op(*aPtr++, *bPtr++);
        if (bitWord)
          break;
        bitIndex += kBitWordSizeInBits;
      }

      _aPtr = aPtr;
      _bPtr = bPtr;
      _aEnd = aEnd;
      _current = bitWord;
      _bitIndex = bitIndex;
    }

    ASMJIT_INLINE bool hasNext() noexcept {
      return _current != BitWordT(0);
    }

    ASMJIT_INLINE uint32_t next() noexcept {
      BitWordT bitWord = _current;
      uint32_t bitIndex = _bitIndex;
      ASMJIT_ASSERT(bitWord != BitWordT(0));

      bitIndex += _ctzPlusOneAndShift(bitWord);
      uint32_t retIndex = bitIndex;

      if (!bitWord) {
        bitIndex |= uint32_t(kBitWordSizeInBits - 1);
        while (_aPtr != _aEnd) {
          bitWord = Operator::op(*_aPtr++, *_bPtr++);
          if (bitWord)
            break;
          bitIndex += kBitWordSizeInBits;
        }
      }

      _current = bitWord;
      _bitIndex = bitIndex;
      return retIndex;
    }

    const BitWordT* _aPtr;
    const BitWordT* _bPtr;
    const BitWordT* _aEnd;
    BitWordT _current;
    uint32_t _bitIndex;
  };
} // IntUtils namespace

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_INTUTILS_H
