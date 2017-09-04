// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#if defined(ASMJIT_API_SCOPE)
# undef ASMJIT_API_SCOPE
#else
# error "[asmjit] api-scope not active, forgot to include asmjit_apibegin.h?"
#endif

// ============================================================================
// [C++ Support]
// ============================================================================

// [NoExcept]
#if defined(ASMJIT_UNDEF_NOEXCEPT)
# undef noexcept
# undef ASMJIT_UNDEF_NOEXCEPT
#endif

// [NullPtr]
#if defined(ASMJIT_UNDEF_NULLPTR)
# undef nullptr
# undef ASMJIT_UNDEF_NULLPTR
#endif

// [Override]
#if defined(ASMJIT_UNDEF_OVERRIDE)
# undef override
# undef ASMJIT_UNDEF_OVERRIDE
#endif

// ============================================================================
// [Compiler Support]
// ============================================================================

// [Clang]
#if ASMJIT_CC_CLANG
# pragma clang diagnostic pop
#endif

// [GCC]
#if ASMJIT_CC_GCC
# pragma GCC diagnostic pop
#endif

// [MSC]
#if ASMJIT_CC_MSC
# pragma warning(pop)
# if _MSC_VER < 1900
#  if defined(ASMJIT_UNDEF_VSNPRINTF)
#   undef vsnprintf
#   undef ASMJIT_UNDEF_VSNPRINTF
#  endif
#  if defined(ASMJIT_UNDEF_SNPRINTF)
#   undef snprintf
#   undef ASMJIT_UNDEF_SNPRINTF
#  endif
# endif
#endif

// ============================================================================
// [Custom Macros]
// ============================================================================

// [ASMJIT_NON...]
#undef ASMJIT_NONCONSTRUCTIBLE
#undef ASMJIT_NONCOPYABLE

// [ASMJIT_ENUM]
#undef ASMJIT_ENUM
