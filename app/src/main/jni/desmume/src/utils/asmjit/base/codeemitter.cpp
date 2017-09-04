// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/assembler.h"
#include "../base/intutils.h"
#include "../base/logging.h"
#include "../base/utils.h"
#include "../base/virtmem.h"

#if defined(ASMJIT_BUILD_X86)
# include "../x86/x86internal_p.h"
# include "../x86/x86inst.h"
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
# include "../arm/arminternal_p.h"
# include "../arm/arminst.h"
#endif // ASMJIT_BUILD_ARM

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::CodeEmitter - Construction / Destruction]
// ============================================================================

CodeEmitter::CodeEmitter(uint32_t type) noexcept
  : _type(IntUtils::toUInt8(type)),
    _reserved(0),
    _flags(0),
    _lastError(kErrorNotInitialized),
    _code(nullptr),
    _errorHandler(nullptr),
    _codeInfo(),
    _gpRegInfo(),
    _emitterOptions(0),
    _privateData(0),
    _instOptions(0),
    _globalInstOptions(Inst::kOptionReserved),
    _extraReg(),
    _inlineComment(nullptr),
    _none() {}

CodeEmitter::~CodeEmitter() noexcept {
  if (_code) {
    _addFlags(kFlagDestroyed);
    _code->detach(this);
  }
}

// ============================================================================
// [asmjit::CodeEmitter - Code-Generation]
// ============================================================================

Error CodeEmitter::_emitOpArray(uint32_t instId, const Operand_* operands, size_t count) {
  const Operand_* op = operands;
  switch (count) {
    case  0: return _emit(instId, _none, _none, _none, _none);
    case  1: return _emit(instId, op[0], _none, _none, _none);
    case  2: return _emit(instId, op[0], op[1], _none, _none);
    case  3: return _emit(instId, op[0], op[1], op[2], _none);
    case  4: return _emit(instId, op[0], op[1], op[2], op[3]);
    case  5: return _emit(instId, op[0], op[1], op[2], op[3], op[4], _none);
    case  6: return _emit(instId, op[0], op[1], op[2], op[3], op[4], op[5]);
    default: return DebugUtils::errored(kErrorInvalidArgument);
  }
}

// ============================================================================
// [asmjit::CodeEmitter - Finalize]
// ============================================================================

Label CodeEmitter::getLabelByName(const char* name, size_t nameLength, uint32_t parentId) noexcept {
  return Label(_code ? _code->getLabelIdByName(name, nameLength, parentId) : static_cast<uint32_t>(0));
}

// ============================================================================
// [asmjit::CodeEmitter - Finalize]
// ============================================================================

Error CodeEmitter::finalize() {
  // Does nothing by default, overridden by `CodeBuilder` and `CodeCompiler`.
  return kErrorOk;
}

// ============================================================================
// [asmjit::CodeEmitter - Error Handling]
// ============================================================================

Error CodeEmitter::setLastError(Error error, const char* message) {
  // Don't change anything if the emitter is not attached to `CodeHolder`.
  if (_code == nullptr)
    return error;

  // Special case used to reset the last error.
  if (error == kErrorOk) {
    _lastError = kErrorOk;

    // This may clear `Inst::kOptionReserved` bit in `_globalInstOptions`.
    onUpdateGlobalInstOptions();
    return kErrorOk;
  }

  if (!message)
    message = DebugUtils::errorAsString(error);

  // Logging is skipped if the error is handled by `ErrorHandler`.
  ErrorHandler* handler = getErrorHandler();
  if (!handler)
    handler = getCode()->getErrorHandler();

  if (handler && handler->handleError(error, message, this))
    return error;

  // The handler->handleError() function may throw an exception or longjmp()
  // to terminate the execution of `setLastError()`. This is the reason why
  // we have delayed changing the `_error` member until now.
  _lastError = error;

  // This is the same as calling `onUpdateGlobalInstOptions()`, since it only
  // updates the `Inst::kOptionReserved` bit we can do it manually here as we
  // know that we are in error state and this bit MUST BE set.
  _globalInstOptions |= Inst::kOptionReserved;

  return error;
}

// ============================================================================
// [asmjit::CodeEmitter - Label Management]
// ============================================================================

bool CodeEmitter::isLabelValid(uint32_t id) const noexcept {
  uint32_t index = Operand::unpackId(id);
  return _code && index < _code->getLabelCount();
}

// ============================================================================
// [asmjit::CodeEmitter - Emit (Low-Level)]
// ============================================================================

#define OP const Operand_&

Error CodeEmitter::emit(uint32_t instId) { return _emit(instId, _none, _none, _none, _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0) { return _emit(instId, o0, _none, _none, _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1) { return _emit(instId, o0, o1, _none, _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, OP o2) { return _emit(instId, o0, o1, o2, _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3) { return _emit(instId, o0, o1, o2, o3); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, OP o4) { return _emit(instId, o0, o1, o2, o3, o4, _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, OP o4, OP o5) { return _emit(instId, o0, o1, o2, o3, o4, o5); }

Error CodeEmitter::emit(uint32_t instId, int o0) { return _emit(instId, Imm(o0), _none, _none, _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, int o1) { return _emit(instId, o0, Imm(o1), _none, _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, int o2) { return _emit(instId, o0, o1, Imm(o2), _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, OP o2, int o3) { return _emit(instId, o0, o1, o2, Imm(o3)); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, int o4) { return _emit(instId, o0, o1, o2, o3, Imm(o4), _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, OP o4, int o5) { return _emit(instId, o0, o1, o2, o3, o4, Imm(o5)); }

Error CodeEmitter::emit(uint32_t instId, int64_t o0) { return _emit(instId, Imm(o0), _none, _none, _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, int64_t o1) { return _emit(instId, o0, Imm(o1), _none, _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, int64_t o2) { return _emit(instId, o0, o1, Imm(o2), _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, OP o2, int64_t o3) { return _emit(instId, o0, o1, o2, Imm(o3)); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, int64_t o4) { return _emit(instId, o0, o1, o2, o3, Imm(o4), _none); }
Error CodeEmitter::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, OP o4, int64_t o5) { return _emit(instId, o0, o1, o2, o3, o4, Imm(o5)); }

#undef OP

// ============================================================================
// [asmjit::CodeEmitter - Emit (High-Level)]
// ============================================================================

ASMJIT_FAVOR_SIZE Error CodeEmitter::emitProlog(const FuncFrame& frame) {
#if defined(ASMJIT_BUILD_X86)
  if (getArchInfo().isX86Family())
    return X86Internal::emitProlog(static_cast<X86Emitter*>(this), frame);
#endif

#if defined(ASMJIT_BUILD_ARM)
  if (getArchInfo().isArmFamily())
    return ArmInternal::emitProlog(static_cast<ArmEmitter*>(this), frame);
#endif

  return DebugUtils::errored(kErrorInvalidArch);
}

ASMJIT_FAVOR_SIZE Error CodeEmitter::emitEpilog(const FuncFrame& frame) {
#if defined(ASMJIT_BUILD_X86)
  if (getArchInfo().isX86Family())
    return X86Internal::emitEpilog(static_cast<X86Emitter*>(this), frame);
#endif

#if defined(ASMJIT_BUILD_ARM)
  if (getArchInfo().isArmFamily())
    return ArmInternal::emitEpilog(static_cast<ArmEmitter*>(this), frame);
#endif

  return DebugUtils::errored(kErrorInvalidArch);
}

ASMJIT_FAVOR_SIZE Error CodeEmitter::emitArgsAssignment(const FuncFrame& frame, const FuncArgsAssignment& args) {
#if defined(ASMJIT_BUILD_X86)
  if (getArchInfo().isX86Family())
    return X86Internal::emitArgsAssignment(static_cast<X86Emitter*>(this), frame, args);
#endif

#if defined(ASMJIT_BUILD_ARM)
  if (getArchInfo().isArmFamily())
    return ArmInternal::emitArgsAssignment(static_cast<ArmEmitter*>(this), frame, args);
#endif

  return DebugUtils::errored(kErrorInvalidArch);
}

// ============================================================================
// [asmjit::CodeEmitter - Comment]
// ============================================================================

Error CodeEmitter::commentf(const char* fmt, ...) {
  Error err = _lastError;
  if (ASMJIT_UNLIKELY(err))
    return err;

#if !defined(ASMJIT_DISABLE_LOGGING)
  if (hasEmitterOption(kOptionLoggingEnabled)) {
    va_list ap;
    va_start(ap, fmt);
    err = _code->_logger->logv(fmt, ap);
    va_end(ap);
  }
#else
  ASMJIT_UNUSED(fmt);
#endif

  return err;
}

Error CodeEmitter::commentv(const char* fmt, va_list ap) {
  Error err = _lastError;
  if (ASMJIT_UNLIKELY(err))
    return err;

#if !defined(ASMJIT_DISABLE_LOGGING)
  if (hasEmitterOption(kOptionLoggingEnabled))
    err = _code->_logger->logv(fmt, ap);
#else
  ASMJIT_UNUSED(fmt);
  ASMJIT_UNUSED(ap);
#endif

  return err;
}

// ============================================================================
// [asmjit::CodeEmitter - Events]
// ============================================================================

Error CodeEmitter::onAttach(CodeHolder* code) noexcept {
  _lastError = kErrorOk;
  _codeInfo = code->getCodeInfo();
  _emitterOptions = code->getEmitterOptions();

  onUpdateGlobalInstOptions();
  return kErrorOk;
}

Error CodeEmitter::onDetach(CodeHolder* code) noexcept {
  ASMJIT_UNUSED(code);

  _flags = 0;
  _lastError = kErrorNotInitialized;
  _errorHandler = nullptr;

  _codeInfo.reset();
  _gpRegInfo.reset();

  _emitterOptions = 0;
  _privateData = 0;

  _instOptions = 0;
  _globalInstOptions = Inst::kOptionReserved;
  _extraReg.reset();
  _inlineComment = nullptr;

  return kErrorOk;
}

void CodeEmitter::onUpdateGlobalInstOptions() noexcept {
  const uint32_t kCriticalEmitterOptions = kOptionLoggingEnabled   |
                                           kOptionStrictValidation ;

  _globalInstOptions &= ~Inst::kOptionReserved;
  if (_lastError != kErrorOk || (_emitterOptions & kCriticalEmitterOptions) != 0) {
    _globalInstOptions |= Inst::kOptionReserved;
  }
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"
