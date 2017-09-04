// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/intutils.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::IntUtils - Unit]
// ============================================================================

#if defined(ASMJIT_TEST)
UNIT(base_intutils) {
  uint32_t i;

  INFO("IntUtils::IntTraits<>");
  EXPECT(IntUtils::IntTraits<signed char>::kIsSigned);
  EXPECT(IntUtils::IntTraits<short      >::kIsSigned);
  EXPECT(IntUtils::IntTraits<int        >::kIsSigned);
  EXPECT(IntUtils::IntTraits<long       >::kIsSigned);
  EXPECT(IntUtils::IntTraits<intptr_t   >::kIsSigned);

  EXPECT(IntUtils::IntTraits<unsigned char >::kIsUnsigned);
  EXPECT(IntUtils::IntTraits<unsigned short>::kIsUnsigned);
  EXPECT(IntUtils::IntTraits<unsigned int  >::kIsUnsigned);
  EXPECT(IntUtils::IntTraits<unsigned long >::kIsUnsigned);
  EXPECT(IntUtils::IntTraits<uintptr_t     >::kIsUnsigned);

  EXPECT(IntUtils::IntTraits<intptr_t >::kIsIntPtr);
  EXPECT(IntUtils::IntTraits<uintptr_t>::kIsIntPtr);

  INFO("IntUtils::minValue()");
  EXPECT(IntUtils::minValue<int8_t>() == -128);
  EXPECT(IntUtils::minValue<int16_t>() == -32768);
  EXPECT(IntUtils::minValue<int32_t>() == -2147483647 - 1);
  EXPECT(IntUtils::minValue<uint8_t>() == 0U);
  EXPECT(IntUtils::minValue<uint16_t>() == 0U);
  EXPECT(IntUtils::minValue<uint32_t>() == 0U);

  INFO("IntUtils::maxValue()");
  EXPECT(IntUtils::maxValue<int8_t>() == 127);
  EXPECT(IntUtils::maxValue<int16_t>() == 32767);
  EXPECT(IntUtils::maxValue<int32_t>() == 2147483647);
  EXPECT(IntUtils::maxValue<uint8_t>() == 255U);
  EXPECT(IntUtils::maxValue<uint16_t>() == 65535U);
  EXPECT(IntUtils::maxValue<uint32_t>() == 4294967295U);

  INFO("IntUtils::blsi()");
  for (i = 0; i < 32; i++) EXPECT(IntUtils::blsi(uint32_t(1) << i) == uint32_t(1) << i);
  for (i = 0; i < 31; i++) EXPECT(IntUtils::blsi(uint32_t(3) << i) == uint32_t(1) << i);
  for (i = 0; i < 64; i++) EXPECT(IntUtils::blsi(uint64_t(1) << i) == uint64_t(1) << i);
  for (i = 0; i < 63; i++) EXPECT(IntUtils::blsi(uint64_t(3) << i) == uint64_t(1) << i);

  INFO("IntUtils::ctz()");
  for (i = 0; i < 32; i++) EXPECT(IntUtils::ctz(uint32_t(1) << i) == i);
  for (i = 0; i < 64; i++) EXPECT(IntUtils::ctz(uint64_t(1) << i) == i);
  for (i = 0; i < 32; i++) EXPECT(IntUtils::_ctzGeneric(uint32_t(1) << i) == i);
  for (i = 0; i < 64; i++) EXPECT(IntUtils::_ctzGeneric(uint64_t(1) << i) == i);

  INFO("IntUtils::popcnt()");
  for (i = 0; i < 32; i++) EXPECT(IntUtils::popcnt((uint32_t(1) << i)) == 1);
  for (i = 0; i < 64; i++) EXPECT(IntUtils::popcnt((uint64_t(1) << i)) == 1);
  EXPECT(IntUtils::popcnt(0x000000F0) ==  4);
  EXPECT(IntUtils::popcnt(0x10101010) ==  4);
  EXPECT(IntUtils::popcnt(0xFF000000) ==  8);
  EXPECT(IntUtils::popcnt(0xFFFFFFF7) == 31);
  EXPECT(IntUtils::popcnt(0x7FFFFFFF) == 31);

  INFO("IntUtils::isBetween()");
  EXPECT(IntUtils::isBetween<int>(11 , 10, 20) == true);
  EXPECT(IntUtils::isBetween<int>(101, 10, 20) == false);

  INFO("IntUtils::isInt8()");
  EXPECT(IntUtils::isInt8(-128) == true);
  EXPECT(IntUtils::isInt8( 127) == true);
  EXPECT(IntUtils::isInt8(-129) == false);
  EXPECT(IntUtils::isInt8( 128) == false);

  INFO("IntUtils::isInt16()");
  EXPECT(IntUtils::isInt16(-32768) == true);
  EXPECT(IntUtils::isInt16( 32767) == true);
  EXPECT(IntUtils::isInt16(-32769) == false);
  EXPECT(IntUtils::isInt16( 32768) == false);

  INFO("IntUtils::isInt32()");
  EXPECT(IntUtils::isInt32( 2147483647    ) == true);
  EXPECT(IntUtils::isInt32(-2147483647 - 1) == true);
  EXPECT(IntUtils::isInt32(ASMJIT_UINT64_C(2147483648)) == false);
  EXPECT(IntUtils::isInt32(ASMJIT_UINT64_C(0xFFFFFFFF)) == false);
  EXPECT(IntUtils::isInt32(ASMJIT_UINT64_C(0xFFFFFFFF) + 1) == false);

  INFO("IntUtils::isUInt8()");
  EXPECT(IntUtils::isUInt8(0)   == true);
  EXPECT(IntUtils::isUInt8(255) == true);
  EXPECT(IntUtils::isUInt8(256) == false);
  EXPECT(IntUtils::isUInt8(-1)  == false);

  INFO("IntUtils::isUInt12()");
  EXPECT(IntUtils::isUInt12(0)    == true);
  EXPECT(IntUtils::isUInt12(4095) == true);
  EXPECT(IntUtils::isUInt12(4096) == false);
  EXPECT(IntUtils::isUInt12(-1)   == false);

  INFO("IntUtils::isUInt16()");
  EXPECT(IntUtils::isUInt16(0)     == true);
  EXPECT(IntUtils::isUInt16(65535) == true);
  EXPECT(IntUtils::isUInt16(65536) == false);
  EXPECT(IntUtils::isUInt16(-1)    == false);

  INFO("IntUtils::isUInt32()");
  EXPECT(IntUtils::isUInt32(ASMJIT_UINT64_C(0xFFFFFFFF)) == true);
  EXPECT(IntUtils::isUInt32(ASMJIT_UINT64_C(0xFFFFFFFF) + 1) == false);
  EXPECT(IntUtils::isUInt32(-1) == false);

  INFO("IntUtils::isAligned()");
  EXPECT(IntUtils::isAligned<size_t>(0xFFFF,  4) == false);
  EXPECT(IntUtils::isAligned<size_t>(0xFFF4,  4) == true);
  EXPECT(IntUtils::isAligned<size_t>(0xFFF8,  8) == true);
  EXPECT(IntUtils::isAligned<size_t>(0xFFF0, 16) == true);

  INFO("IntUtils::alignTo()");
  EXPECT(IntUtils::alignTo<size_t>(0xFFFF,  4) == 0x10000);
  EXPECT(IntUtils::alignTo<size_t>(0xFFF4,  4) == 0x0FFF4);
  EXPECT(IntUtils::alignTo<size_t>(0xFFF8,  8) == 0x0FFF8);
  EXPECT(IntUtils::alignTo<size_t>(0xFFF0, 16) == 0x0FFF0);
  EXPECT(IntUtils::alignTo<size_t>(0xFFF0, 32) == 0x10000);

  INFO("IntUtils::alignDiff()");
  EXPECT(IntUtils::alignDiff<size_t>(0xFFFF,  4) == 1);
  EXPECT(IntUtils::alignDiff<size_t>(0xFFF4,  4) == 0);
  EXPECT(IntUtils::alignDiff<size_t>(0xFFF8,  8) == 0);
  EXPECT(IntUtils::alignDiff<size_t>(0xFFF0, 16) == 0);
  EXPECT(IntUtils::alignDiff<size_t>(0xFFF0, 32) == 16);

  INFO("IntUtils::isPowerOf2()");
  for (i = 0; i < 64; i++) {
    EXPECT(IntUtils::isPowerOf2(static_cast<uint64_t>(1) << i) == true);
    EXPECT(IntUtils::isPowerOf2((static_cast<uint64_t>(1) << i) ^ 0x001101) == false);
  }

  INFO("IntUtils::alignToPowerOf2()");
  EXPECT(IntUtils::alignToPowerOf2<size_t>(0xFFFF) == 0x10000);
  EXPECT(IntUtils::alignToPowerOf2<size_t>(0xF123) == 0x10000);
  EXPECT(IntUtils::alignToPowerOf2<size_t>(0x0F00) == 0x01000);
  EXPECT(IntUtils::alignToPowerOf2<size_t>(0x0100) == 0x00100);
  EXPECT(IntUtils::alignToPowerOf2<size_t>(0x1001) == 0x02000);

  INFO("IntUtils::mask()");
  for (i = 0; i < 32; i++) {
    EXPECT(IntUtils::mask(i) == (1U << i));
  }

  INFO("IntUtils::bits()");
  for (i = 0; i < 32; i++) {
    uint32_t expectedBits = 0;
    for (uint32_t b = 0; b < i; b++)
      expectedBits |= static_cast<uint32_t>(1) << b;
    EXPECT(IntUtils::bits(i) == expectedBits, "IntUtils::bits(%u) should return %X", i, expectedBits);
  }

  INFO("IntUtils::hasBit()");
  for (i = 0; i < 32; i++) {
    EXPECT(IntUtils::hasBit((1 << i), i) == true, "IntUtils::hasBit(%X, %u) should return true", (1 << i), i);
  }

  INFO("IntUtils::BitWordIterator<uint32_t>");
  {
    IntUtils::BitWordIterator<uint32_t> it(0x80000F01U);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 0);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 8);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 9);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 10);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 11);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 31);
    EXPECT(!it.hasNext());

    // No bits set.
    it.init(0x00000000U);
    ASMJIT_ASSERT(!it.hasNext());

    // Only first bit set.
    it.init(0x00000001U);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 0);
    ASMJIT_ASSERT(!it.hasNext());

    // Only last bit set (special case).
    it.init(0x80000000U);
    ASMJIT_ASSERT(it.hasNext());
    ASMJIT_ASSERT(it.next() == 31);
    ASMJIT_ASSERT(!it.hasNext());
  }

  INFO("IntUtils::BitWordIterator<uint64_t>");
  {
    IntUtils::BitWordIterator<uint64_t> it(uint64_t(1) << 63);
    ASMJIT_ASSERT(it.hasNext());
    ASMJIT_ASSERT(it.next() == 63);
    ASMJIT_ASSERT(!it.hasNext());
  }

  INFO("IntUtils::BitArrayIterator<uint32_t>");
  {
    static const uint32_t bits1[] = { 0x80000008U, 0x80000001U, 0x00000000U, 0x80000000U, 0x00000000U, 0x00000000U, 0x00003000U };
    IntUtils::BitArrayIterator<uint32_t> it(bits1, static_cast<uint32_t>(ASMJIT_ARRAY_SIZE(bits1)));

    EXPECT(it.hasNext());
    EXPECT(it.next() == 3);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 31);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 32);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 63);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 127);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 204);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 205);
    EXPECT(!it.hasNext());

    static const uint32_t bits2[] = { 0x80000000U, 0x80000000U, 0x00000000U, 0x80000000U };
    it.init(bits2, static_cast<uint32_t>(ASMJIT_ARRAY_SIZE(bits2)));

    EXPECT(it.hasNext());
    EXPECT(it.next() == 31);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 63);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 127);
    EXPECT(!it.hasNext());
  }

  INFO("IntUtils::BitArrayIterator<uint64_t>");
  {
    static const uint64_t bits[] = { 0x80000000U, 0x80000000U, 0x00000000U, 0x80000000U };
    IntUtils::BitArrayIterator<uint64_t> it(bits, static_cast<uint32_t>(ASMJIT_ARRAY_SIZE(bits)));

    EXPECT(it.hasNext());
    EXPECT(it.next() == 31);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 95);
    EXPECT(it.hasNext());
    EXPECT(it.next() == 223);
    EXPECT(!it.hasNext());
  }
}
#endif

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
