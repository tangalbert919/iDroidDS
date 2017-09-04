// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/assembler.h"
#include "../base/codecompiler.h"
#include "../base/cpuinfo.h"
#include "../base/intutils.h"
#include "../base/logging.h"
#include "../base/rapass_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::CCFuncCall - Arg / Ret]
// ============================================================================

bool CCFuncCall::_setArg(uint32_t i, const Operand_& op) noexcept {
  if ((i & ~kFuncArgHi) >= _funcDetail.getArgCount())
    return false;

  _args[i] = op;
  return true;
}

bool CCFuncCall::_setRet(uint32_t i, const Operand_& op) noexcept {
  if (i >= 2)
    return false;

  _ret[i] = op;
  return true;
}

// ============================================================================
// [asmjit::CodeCompiler - Construction / Destruction]
// ============================================================================

CodeCompiler::CodeCompiler() noexcept
  : CodeBuilder(),
    _func(nullptr),
    _vRegZone(4096 - Zone::kZoneOverhead),
    _vRegArray(),
    _localConstPool(nullptr),
    _globalConstPool(nullptr) {

  _type = kTypeCompiler;
}
CodeCompiler::~CodeCompiler() noexcept {}

// ============================================================================
// [asmjit::CodeCompiler - Func]
// ============================================================================

CCFunc* CodeCompiler::newFunc(const FuncSignature& sign) noexcept {
  Error err;

  CCFunc* func = newNodeT<CCFunc>();
  if (!func) goto _NoMemory;

  err = registerLabelNode(func);
  if (ASMJIT_UNLIKELY(err)) {
    // TODO: Calls setLastError, maybe rethink noexcept?
    setLastError(err);
    return nullptr;
  }

  // Create helper nodes.
  func->_exitNode = newLabelNode();
  func->_end = newNodeT<CBSentinel>(CBSentinel::kSentinelFuncEnd);

  if (!func->_exitNode || !func->_end)
    goto _NoMemory;

  // Initialize the function info.
  err = func->getDetail().init(sign);
  if (ASMJIT_UNLIKELY(err)) {
    setLastError(err);
    return nullptr;
  }

  // Override the natural stack alignment of the calling convention to what's
  // specified by CodeInfo.
  func->_funcDetail._callConv.setNaturalStackAlignment(_codeInfo.getStackAlignment());

  // Initialize the function frame.
  err = func->_frame.init(func->_funcDetail);
  if (ASMJIT_UNLIKELY(err)) {
    setLastError(err);
    return nullptr;
  }

  // Allocate space for function arguments.
  func->_args = nullptr;
  if (func->getArgCount() != 0) {
    func->_args = _allocator.allocT<VirtReg*>(func->getArgCount() * sizeof(VirtReg*));
    if (!func->_args) goto _NoMemory;

    ::memset(func->_args, 0, func->getArgCount() * sizeof(VirtReg*));
  }

  return func;

_NoMemory:
  setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  return nullptr;
}

CCFunc* CodeCompiler::addFunc(CCFunc* func) {
  ASMJIT_ASSERT(_func == nullptr);
  _func = func;

  addNode(func);                 // Function node.
  CBNode* cursor = getCursor();  // {CURSOR}.
  addNode(func->getExitNode());  // Function exit label.
  addNode(func->getEnd());       // Function end marker.

  _setCursor(cursor);
  return func;
}

CCFunc* CodeCompiler::addFunc(const FuncSignature& sign) {
  CCFunc* func = newFunc(sign);

  if (!func) {
    setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    return nullptr;
  }

  return addFunc(func);
}

CBSentinel* CodeCompiler::endFunc() {
  CCFunc* func = getFunc();
  if (!func) {
    // TODO:
    return nullptr;
  }

  // Add the local constant pool at the end of the function (if exists).
  if (_localConstPool) {
    setCursor(func->getEnd()->getPrev());
    addNode(_localConstPool);
    _localConstPool = nullptr;
  }

  // Mark as finished.
  func->_isFinished = true;
  _func = nullptr;

  CBSentinel* end = func->getEnd();
  setCursor(end);
  return end;
}

// ============================================================================
// [asmjit::CodeCompiler - Ret]
// ============================================================================

CCFuncRet* CodeCompiler::newRet(const Operand_& o0, const Operand_& o1) noexcept {
  CCFuncRet* node = newNodeT<CCFuncRet>();
  if (!node) {
    setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    return nullptr;
  }

  node->setOp(0, o0);
  node->setOp(1, o1);
  node->setOpCount(!o1.isNone() ? 2 : !o0.isNone() ? 1 : 0);

  return node;
}

CCFuncRet* CodeCompiler::addRet(const Operand_& o0, const Operand_& o1) noexcept {
  CCFuncRet* node = newRet(o0, o1);
  if (!node) return nullptr;
  return addNode(node)->as<CCFuncRet>();
}

// ============================================================================
// [asmjit::CodeCompiler - Call]
// ============================================================================

CCFuncCall* CodeCompiler::newCall(uint32_t instId, const Operand_& o0, const FuncSignature& sign) noexcept {
  CCFuncCall* node = _allocator.allocT<CCFuncCall>(sizeof(CCFuncCall));
  if (ASMJIT_UNLIKELY(!node)) {
    setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    return nullptr;
  }

  node = new(node) CCFuncCall(this, instId, 0);
  node->setOpCount(1);
  node->setOp(0, o0);
  node->resetOp(1);
  node->resetOp(2);
  node->resetOp(3);

  Error err = node->getDetail().init(sign);
  if (ASMJIT_UNLIKELY(err)) {
    setLastError(err);
    return nullptr;
  }

  // If there are no arguments skip the allocation.
  uint32_t nArgs = sign.getArgCount();
  if (!nArgs) return node;

  node->_args = static_cast<Operand*>(_allocator.alloc(nArgs * sizeof(Operand)));
  if (!node->_args) {
    setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    return nullptr;
  }

  ::memset(node->_args, 0, nArgs * sizeof(Operand));
  return node;
}

CCFuncCall* CodeCompiler::addCall(uint32_t instId, const Operand_& o0, const FuncSignature& sign) noexcept {
  CCFuncCall* node = newCall(instId, o0, sign);
  if (!node) return nullptr;
  return addNode(node)->as<CCFuncCall>();
}

// ============================================================================
// [asmjit::CodeCompiler - Vars]
// ============================================================================

Error CodeCompiler::setArg(uint32_t argIndex, const Reg& r) {
  CCFunc* func = getFunc();

  if (ASMJIT_UNLIKELY(!func))
    return setLastError(DebugUtils::errored(kErrorInvalidState));

  if (ASMJIT_UNLIKELY(!isVirtRegValid(r)))
    return setLastError(DebugUtils::errored(kErrorInvalidVirtId));

  VirtReg* vReg = getVirtReg(r);
  func->setArg(argIndex, vReg);

  return kErrorOk;
}

// ============================================================================
// [asmjit::CodeCompiler - Vars]
// ============================================================================

static void CodeCompiler_assignGenericName(CodeCompiler* self, VirtReg* vReg) {
  uint32_t index = static_cast<unsigned int>(Operand::unpackId(vReg->_id));

  char buf[64];
  int len = snprintf(buf, ASMJIT_ARRAY_SIZE(buf), "%%%u", index);

  ASMJIT_ASSERT(len > 0 && len < static_cast<int>(ASMJIT_ARRAY_SIZE(buf)));
  vReg->_name.setData(&self->_dataZone, buf, static_cast<unsigned int>(len));
}

VirtReg* CodeCompiler::newVirtReg(uint32_t typeId, uint32_t signature, const char* name) noexcept {
  uint32_t index = _vRegArray.getLength();
  if (ASMJIT_UNLIKELY(index >= static_cast<uint32_t>(Operand::kPackedIdCount)))
    return nullptr;

  VirtReg* vReg;
  if (_vRegArray.willGrow(&_allocator) != kErrorOk || !(vReg = _vRegZone.allocZeroedT<VirtReg>()))
    return nullptr;

  uint32_t size = TypeId::sizeOf(typeId);
  uint32_t alignment = std::min<uint32_t>(size, 64);

  vReg = new(vReg) VirtReg(Operand::packId(index), signature, size, alignment, typeId);

#if !defined(ASMJIT_DISABLE_LOGGING)
  if (name && name[0] != '\0')
    vReg->_name.setData(&_dataZone, name, Globals::kNullTerminated);
  else
    CodeCompiler_assignGenericName(this, vReg);
#endif

  _vRegArray.appendUnsafe(vReg);
  return vReg;
}

Error CodeCompiler::_newReg(Reg& out, uint32_t typeId, const char* name) {
  RegInfo regInfo;

  Error err = ArchUtils::typeIdToRegInfo(getArchType(), typeId, regInfo);
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  VirtReg* vReg = newVirtReg(typeId, regInfo.getSignature(), name);
  if (ASMJIT_UNLIKELY(!vReg)) {
    out.reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  out._initReg(regInfo.getSignature(), vReg->getId());
  return kErrorOk;
}

Error CodeCompiler::_newReg(Reg& out, uint32_t typeId, const char* nameFmt, va_list ap) {
  StringBuilderTmp<256> sb;
  sb.appendFormatVA(nameFmt, ap);
  return _newReg(out, typeId, sb.getData());
}

Error CodeCompiler::_newReg(Reg& out, const Reg& ref, const char* name) {
  RegInfo regInfo;
  uint32_t typeId;

  if (isVirtRegValid(ref)) {
    VirtReg* vRef = getVirtReg(ref);
    typeId = vRef->getTypeId();

    // NOTE: It's possible to cast one register type to another if it's the
    // same register group. However, VirtReg always contains the TypeId that
    // was used to create the register. This means that in some cases we may
    // end up having different size of `ref` and `vRef`. In such case we
    // adjust the TypeId to match the `ref` register type instead of the
    // original register type, which should be the expected behavior.
    uint32_t typeSize = TypeId::sizeOf(typeId);
    uint32_t refSize = ref.getSize();

    if (typeSize != refSize) {
      if (TypeId::isInt(typeId)) {
        // GP register - change TypeId to match `ref`, but keep sign of `vRef`.
        switch (refSize) {
          case  1: typeId = TypeId::kI8  | (typeId & 1); break;
          case  2: typeId = TypeId::kI16 | (typeId & 1); break;
          case  4: typeId = TypeId::kI32 | (typeId & 1); break;
          case  8: typeId = TypeId::kI64 | (typeId & 1); break;
          default: typeId = TypeId::kVoid; break;
        }
      }
      else if (TypeId::isMmx(typeId)) {
        // MMX register - always use 64-bit.
        typeId = TypeId::kMmx64;
      }
      else if (TypeId::isMask(typeId)) {
        // Mask register - change TypeId to match `ref` size.
        switch (refSize) {
          case  1: typeId = TypeId::kMask8; break;
          case  2: typeId = TypeId::kMask16; break;
          case  4: typeId = TypeId::kMask32; break;
          case  8: typeId = TypeId::kMask64; break;
          default: typeId = TypeId::kVoid; break;
        }
      }
      else {
        // VEC register - change TypeId to match `ref` size, keep vector metadata.
        uint32_t elementTypeId = TypeId::elementOf(typeId);

        switch (refSize) {
          case 16: typeId = TypeId::_kVec128Start + (elementTypeId - TypeId::kI8); break;
          case 32: typeId = TypeId::_kVec256Start + (elementTypeId - TypeId::kI8); break;
          case 64: typeId = TypeId::_kVec512Start + (elementTypeId - TypeId::kI8); break;
          default: typeId = TypeId::kVoid; break;
        }
      }

      if (typeId == TypeId::kVoid)
        return setLastError(DebugUtils::errored(kErrorInvalidState));
    }
  }
  else {
    typeId = ref.getType();
  }

  Error err = ArchUtils::typeIdToRegInfo(getArchType(), typeId, regInfo);
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  VirtReg* vReg = newVirtReg(typeId, regInfo.getSignature(), name);
  if (ASMJIT_UNLIKELY(!vReg)) {
    out.reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  out._initReg(regInfo.getSignature(), vReg->getId());
  return kErrorOk;
}

Error CodeCompiler::_newReg(Reg& out, const Reg& ref, const char* nameFmt, va_list ap) {
  StringBuilderTmp<256> sb;
  sb.appendFormatVA(nameFmt, ap);
  return _newReg(out, ref, sb.getData());
}

Error CodeCompiler::_newStack(Mem& out, uint32_t size, uint32_t alignment, const char* name) {
  if (size == 0)
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (alignment == 0) alignment = 1;
  if (!IntUtils::isPowerOf2(alignment))
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (alignment > 64) alignment = 64;

  VirtReg* vReg = newVirtReg(0, 0, name);
  if (ASMJIT_UNLIKELY(!vReg)) {
    out.reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  vReg->_virtSize = size;
  vReg->_isStack = true;
  vReg->_alignment = IntUtils::toUInt8(alignment);

  // Set the memory operand to GPD/GPQ and its id to VirtReg.
  out = Mem(Init, _gpRegInfo.getType(), vReg->getId(), Reg::kRegNone, 0, 0, 0, Mem::kSignatureMemRegHomeFlag);
  return kErrorOk;
}

Error CodeCompiler::_newConst(Mem& out, uint32_t scope, const void* data, size_t size) {
  CBConstPool** pPool;
  if (scope == kConstScopeLocal)
    pPool = &_localConstPool;
  else if (scope == kConstScopeGlobal)
    pPool = &_globalConstPool;
  else
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (!*pPool && !(*pPool = newConstPoolNode()))
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  CBConstPool* pool = *pPool;
  size_t off;

  Error err = pool->add(data, size, off);
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  out = Mem(Init,
    Label::kLabelTag,             // Base type.
    pool->getId(),                // Base id.
    0,                            // Index type.
    0,                            // Index id.
    static_cast<int32_t>(off),    // Offset.
    static_cast<uint32_t>(size),  // Size.
    0);                           // Flags.
  return kErrorOk;
}

void CodeCompiler::rename(Reg& reg, const char* fmt, ...) {
  if (!reg.isVirtReg()) return;

  VirtReg* vReg = getVirtRegById(reg.getId());
  if (!vReg) return;

  if (fmt && fmt[0] != '\0') {
    char buf[128];

    va_list ap;
    va_start(ap, fmt);

    vsnprintf(buf, ASMJIT_ARRAY_SIZE(buf), fmt, ap);
    buf[ASMJIT_ARRAY_SIZE(buf) - 1] = '\0';

    vReg->_name.setData(&_dataZone, buf, Globals::kNullTerminated);
    va_end(ap);
  }
  else {
    CodeCompiler_assignGenericName(this, vReg);
  }
}

// ============================================================================
// [asmjit::CodeCompiler - Events]
// ============================================================================

Error CodeCompiler::onAttach(CodeHolder* code) noexcept {
  return Base::onAttach(code);
}

Error CodeCompiler::onDetach(CodeHolder* code) noexcept {
  _func = nullptr;
  _localConstPool = nullptr;
  _globalConstPool = nullptr;

  _vRegArray.reset();
  _vRegZone.reset(false);

  return Base::onDetach(code);
}

// ============================================================================
// [asmjit::CCFuncPass - Construction / Destruction]
// ============================================================================

CCFuncPass::CCFuncPass(const char* name) noexcept
  : CBPass(name) {}

// ============================================================================
// [asmjit::CCFuncPass - Run]
// ============================================================================

Error CCFuncPass::run(Zone* zone, Logger* logger) noexcept {
  CBNode* node = cb()->getFirstNode();
  if (!node) return kErrorOk;

  do {
    if (node->getType() == CBNode::kNodeFunc) {
      CCFunc* func = node->as<CCFunc>();
      node = func->getEnd();
      ASMJIT_PROPAGATE(runOnFunction(zone, logger, func));
    }

    // Find a function by skipping all nodes that are not `kNodeFunc`.
    do {
      node = node->getNext();
    } while (node && node->getType() != CBNode::kNodeFunc);
  } while (node);

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
