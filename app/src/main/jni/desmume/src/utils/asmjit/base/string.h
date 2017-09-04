// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_STRING_H
#define _ASMJIT_BASE_STRING_H

// [Dependencies]
#include "../base/zone.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::StringUtils]
// ============================================================================

namespace StringUtils {
  template<typename T>
  static ASMJIT_INLINE T toLower(T c) noexcept { return c ^ (static_cast<T>(c >= T('A') && c <= T('Z')) << 5); }

  template<typename T>
  static ASMJIT_INLINE T toUpper(T c) noexcept { return c ^ (static_cast<T>(c >= T('a') && c <= T('z')) << 5); }

  static ASMJIT_INLINE size_t strLen(const char* s, size_t maxlen) noexcept {
    size_t i;
    for (i = 0; i < maxlen; i++)
      if (!s[i])
        break;
    return i;
  }

  static ASMJIT_INLINE const char* findPackedString(const char* p, uint32_t id) noexcept {
    uint32_t i = 0;
    while (i < id) {
      while (p[0])
        p++;
      p++;
      i++;
    }
    return p;
  }

  //! \internal
  //!
  //! Compare two instruction names.
  //!
  //! `a` is a null terminated instruction name from `???InstDB::nameData[]` table.
  //! `b` is a non-null terminated instruction name passed to `???Inst::getIdByName()`.
  static ASMJIT_INLINE int cmpInstName(const char* a, const char* b, size_t len) noexcept {
    for (size_t i = 0; i < len; i++) {
      int c = static_cast<int>(static_cast<uint8_t>(a[i])) -
              static_cast<int>(static_cast<uint8_t>(b[i])) ;
      if (c != 0) return c;
    }

    return static_cast<int>(a[len]);
  }
} // StringUtils namespace

// ============================================================================
// [asmjit::SmallStringBase]
// ============================================================================

struct SmallStringBase {
  ASMJIT_INLINE void reset() noexcept {
    _dummy = nullptr;
    _external = nullptr;
  }

  ASMJIT_API Error setData(Zone* zone, uint32_t maxEmbeddedLength, const char* str, size_t len) noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      uint32_t _length;
      char _embedded[sizeof(void*) * 2 - 4];
    };
    struct {
      void* _dummy;
      char* _external;
    };
  };
};

// ============================================================================
// [asmjit::SmallString<N>]
// ============================================================================

//! Small string is a template that helps to create strings that can be either
//! statically allocated if they are small, or externally allocated in case
//! their length exceeds the limit. The `N` represents the size of the whole
//! `SmallString` structure, based on that size the maximum size of the internal
//! buffer is determined.
template<size_t N>
class SmallString {
public:
  enum {
    kWholeSize = N > sizeof(SmallStringBase) ? static_cast<unsigned int>(N)
                                             : static_cast<unsigned int>(sizeof(SmallStringBase)),
    kMaxEmbeddedLength = kWholeSize - 5
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE SmallString() noexcept { reset(); }
  ASMJIT_INLINE void reset() noexcept { _base.reset(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isEmpty() const noexcept { return _base._length == 0; }
  ASMJIT_INLINE bool isEmbedded() const noexcept { return _base._length <= kMaxEmbeddedLength; }

  ASMJIT_INLINE uint32_t getLength() const noexcept { return _base._length; }
  ASMJIT_INLINE const char* getData() const noexcept { return _base._length <= kMaxEmbeddedLength ? _base._embedded : _base._external; }

  ASMJIT_INLINE Error setData(Zone* zone, const char* data, size_t len) noexcept {
    return _base.setData(zone, kMaxEmbeddedLength, data, len);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    SmallStringBase _base;
    char _wholeData[kWholeSize];
  };
};

// ============================================================================
// [asmjit::StringBuilder]
// ============================================================================

//! String builder.
//!
//! String builder was designed to be able to build a string using append like
//! operation to append numbers, other strings, or signle characters. It can
//! allocate it's own buffer or use a buffer created on the stack.
//!
//! String builder contains method specific to AsmJit functionality, used for
//! logging or HTML output.
class StringBuilder {
public:
  ASMJIT_NONCOPYABLE(StringBuilder)

  //! \internal
  //!
  //! String operation.
  ASMJIT_ENUM(OpType) {
    kStringOpSet = 0,                    //!< Replace the current string by a given content.
    kStringOpAppend = 1                  //!< Append a given content to the current string.
  };

  //! \internal
  //!
  //! String format flags.
  ASMJIT_ENUM(StringFormatFlags) {
    kStringFormatShowSign  = 0x00000001,
    kStringFormatShowSpace = 0x00000002,
    kStringFormatAlternate = 0x00000004,
    kStringFormatSigned    = 0x80000000
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API StringBuilder() noexcept;
  ASMJIT_API ~StringBuilder() noexcept;

  ASMJIT_INLINE StringBuilder(const _NoInit&) noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get if the string is empty.
  ASMJIT_INLINE bool isEmpty() const noexcept { return _length == 0; }

  //! Get string builder capacity.
  ASMJIT_INLINE size_t getCapacity() const noexcept { return _capacity; }
  //! Get length.
  ASMJIT_INLINE size_t getLength() const noexcept { return _length; }

  //! Get null-terminated string data.
  ASMJIT_INLINE char* getData() noexcept { return _data; }
  //! Get null-terminated string data (const).
  ASMJIT_INLINE const char* getData() const noexcept { return _data; }

  // --------------------------------------------------------------------------
  // [Prepare / Reserve]
  // --------------------------------------------------------------------------

  //! Prepare to set/append.
  ASMJIT_API char* prepare(uint32_t op, size_t len) noexcept;

  //! Reserve `to` bytes in string builder.
  ASMJIT_API Error reserve(size_t to) noexcept;

  // --------------------------------------------------------------------------
  // [Clear]
  // --------------------------------------------------------------------------

  //! Clear the content in String builder.
  ASMJIT_API void clear() noexcept;

  // --------------------------------------------------------------------------
  // [Op]
  // --------------------------------------------------------------------------

  ASMJIT_API Error _opString(uint32_t op, const char* str, size_t len = Globals::kNullTerminated) noexcept;
  ASMJIT_API Error _opVFormat(uint32_t op, const char* fmt, va_list ap) noexcept;
  ASMJIT_API Error _opChar(uint32_t op, char c) noexcept;
  ASMJIT_API Error _opChars(uint32_t op, char c, size_t n) noexcept;
  ASMJIT_API Error _opNumber(uint32_t op, uint64_t i, uint32_t base = 0, size_t width = 0, uint32_t flags = 0) noexcept;
  ASMJIT_API Error _opHex(uint32_t op, const void* data, size_t len) noexcept;

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  //! Replace the current string with `str` having `len` characters (or possibly null terminated).
  ASMJIT_INLINE Error setString(const char* str, size_t len = Globals::kNullTerminated) noexcept { return _opString(kStringOpSet, str, len); }
  //! Replace the current content by a formatted string `fmt`.
  ASMJIT_API Error setFormat(const char* fmt, ...) noexcept;
  //! Replace the current content by a formatted string `fmt` (va_list version).
  ASMJIT_INLINE Error setFormatVA(const char* fmt, va_list ap) noexcept { return _opVFormat(kStringOpSet, fmt, ap); }

  //! Replace the current content by a single `c` character.
  ASMJIT_INLINE Error setChar(char c) noexcept { return _opChar(kStringOpSet, c); }
  //! Replace the current content by `c` character `n` times.
  ASMJIT_INLINE Error setChars(char c, size_t n) noexcept { return _opChars(kStringOpSet, c, n); }

  //! Replace the current content by a formatted integer `i` (signed).
  ASMJIT_INLINE Error setInt(uint64_t i, uint32_t base = 0, size_t width = 0, uint32_t flags = 0) noexcept {
    return _opNumber(kStringOpSet, i, base, width, flags | kStringFormatSigned);
  }

  //! Replace the current content by a formatted integer `i` (unsigned).
  ASMJIT_INLINE Error setUInt(uint64_t i, uint32_t base = 0, size_t width = 0, uint32_t flags = 0) noexcept {
    return _opNumber(kStringOpSet, i, base, width, flags);
  }

  //! Replace the current content by the given `data` converted to a HEX string.
  ASMJIT_INLINE Error setHex(const void* data, size_t len) noexcept {
    return _opHex(kStringOpSet, data, len);
  }

  // --------------------------------------------------------------------------
  // [Append]
  // --------------------------------------------------------------------------

  //! Append string `str` having `len` characters (or possibly null terminated).
  ASMJIT_INLINE Error appendString(const char* str, size_t len = Globals::kNullTerminated) noexcept { return _opString(kStringOpAppend, str, len); }
  //! Append a formatted string `fmt`.
  ASMJIT_API Error appendFormat(const char* fmt, ...) noexcept;
  //! Append a formatted string `fmt` (va_list version).
  ASMJIT_INLINE Error appendFormatVA(const char* fmt, va_list ap) noexcept { return _opVFormat(kStringOpAppend, fmt, ap); }

  //! Append a single `c` character.
  ASMJIT_INLINE Error appendChar(char c) noexcept { return _opChar(kStringOpAppend, c); }
  //! Append `c` character `n` times.
  ASMJIT_INLINE Error appendChars(char c, size_t n) noexcept { return _opChars(kStringOpAppend, c, n); }

  ASMJIT_API Error padEnd(size_t n, char c = ' ') noexcept;

  //! Append `i`.
  ASMJIT_INLINE Error appendInt(int64_t i, uint32_t base = 0, size_t width = 0, uint32_t flags = 0) noexcept {
    return _opNumber(kStringOpAppend, static_cast<uint64_t>(i), base, width, flags | kStringFormatSigned);
  }

  //! Append `i`.
  ASMJIT_INLINE Error appendUInt(uint64_t i, uint32_t base = 0, size_t width = 0, uint32_t flags = 0) noexcept {
    return _opNumber(kStringOpAppend, i, base, width, flags);
  }

  //! Append the given `data` converted to a HEX string.
  ASMJIT_INLINE Error appendHex(const void* data, size_t len) noexcept {
    return _opHex(kStringOpAppend, data, len);
  }

  // --------------------------------------------------------------------------
  // [Eq]
  // --------------------------------------------------------------------------

  //! Check for equality with other `str` of length `len`.
  ASMJIT_API bool eq(const char* str, size_t len = Globals::kNullTerminated) const noexcept;
  //! Check for equality with `other`.
  ASMJIT_INLINE bool eq(const StringBuilder& other) const noexcept { return eq(other._data, other._length); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool operator==(const StringBuilder& other) const noexcept { return  eq(other); }
  ASMJIT_INLINE bool operator!=(const StringBuilder& other) const noexcept { return !eq(other); }

  ASMJIT_INLINE bool operator==(const char* str) const noexcept { return  eq(str); }
  ASMJIT_INLINE bool operator!=(const char* str) const noexcept { return !eq(str); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  char* _data;                           //!< String data.
  size_t _length;                        //!< String length.
  size_t _capacity;                      //!< String capacity.
  size_t _canFree;                       //!< If the string data can be freed.
};

// ============================================================================
// [asmjit::StringBuilderTmp]
// ============================================================================

//! Temporary string builder, has statically allocated `N` bytes.
template<size_t N>
class StringBuilderTmp : public StringBuilder {
public:
  ASMJIT_NONCOPYABLE(StringBuilderTmp<N>)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE StringBuilderTmp() noexcept : StringBuilder(NoInit) {
    _data = _embeddedData;
    _data[0] = 0;

    _length = 0;
    _capacity = N;
    _canFree = false;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Embedded data.
  char _embeddedData[static_cast<size_t>(N + 1 + sizeof(intptr_t)) & ~static_cast<size_t>(sizeof(intptr_t) - 1)];
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_STRING_H
