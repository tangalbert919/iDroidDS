 // [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_CODEBUILDER_H
#define _ASMJIT_BASE_CODEBUILDER_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_BUILDER)

// [Dependencies]
#include "../base/assembler.h"
#include "../base/codeholder.h"
#include "../base/constpool.h"
#include "../base/inst.h"
#include "../base/intutils.h"
#include "../base/operand.h"
#include "../base/utils.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

class CBNode;
class CBAlign;
class CBComment;
class CBConstPool;
class CBData;
class CBInst;
class CBLabel;
class CBLabelData;
class CBSentinel;
class CBPass;

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::CodeBuilder]
// ============================================================================

class ASMJIT_VIRTAPI CodeBuilder : public CodeEmitter {
public:
  ASMJIT_NONCOPYABLE(CodeBuilder)
  typedef CodeEmitter Base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CodeBuilder` instance.
  ASMJIT_API CodeBuilder() noexcept;
  //! Destroy the `CodeBuilder` instance.
  ASMJIT_API virtual ~CodeBuilder() noexcept;

  // --------------------------------------------------------------------------
  // [Node Management]
  // --------------------------------------------------------------------------

  //! Get the first node.
  ASMJIT_INLINE CBNode* getFirstNode() const noexcept { return _firstNode; }
  //! Get the last node.
  ASMJIT_INLINE CBNode* getLastNode() const noexcept { return _lastNode; }

  //! \internal
  template<typename T>
  ASMJIT_INLINE T* newNodeT() noexcept { return new(_allocator.alloc(sizeof(T))) T(this); }
  //! \internal
  template<typename T, typename P0>
  ASMJIT_INLINE T* newNodeT(P0 p0) noexcept { return new(_allocator.alloc(sizeof(T))) T(this, p0); }
  //! \internal
  template<typename T, typename P0, typename P1>
  ASMJIT_INLINE T* newNodeT(P0 p0, P1 p1) noexcept { return new(_allocator.alloc(sizeof(T))) T(this, p0, p1); }
  //! \internal
  template<typename T, typename P0, typename P1, typename P2>
  ASMJIT_INLINE T* newNodeT(P0 p0, P1 p1, P2 p2) noexcept { return new(_allocator.alloc(sizeof(T))) T(this, p0, p1, p2); }

  //! Create a new \ref CBLabel node.
  ASMJIT_API CBLabel* newLabelNode() noexcept;
  //! Create a new \ref CBAlign node.
  ASMJIT_API CBAlign* newAlignNode(uint32_t mode, uint32_t alignment) noexcept;
  //! Create a new \ref CBData node.
  ASMJIT_API CBData* newDataNode(const void* data, uint32_t size) noexcept;
  //! Create a new \ref CBConstPool node.
  ASMJIT_API CBConstPool* newConstPoolNode() noexcept;
  //! Create a new \ref CBComment node.
  ASMJIT_API CBComment* newCommentNode(const char* s, size_t len) noexcept;

  //! Add `node` after the current and set current to `node`.
  ASMJIT_API CBNode* addNode(CBNode* node) noexcept;
  //! Insert `node` after `ref`.
  ASMJIT_API CBNode* addAfter(CBNode* node, CBNode* ref) noexcept;
  //! Insert `node` before `ref`.
  ASMJIT_API CBNode* addBefore(CBNode* node, CBNode* ref) noexcept;
  //! Remove `node`.
  ASMJIT_API CBNode* removeNode(CBNode* node) noexcept;
  //! Remove multiple nodes.
  ASMJIT_API void removeNodes(CBNode* first, CBNode* last) noexcept;

  //! Get current node.
  //!
  //! \note If this method returns null it means that nothing has been
  //! emitted yet.
  ASMJIT_INLINE CBNode* getCursor() const noexcept { return _cursor; }
  //! Set the current node without returning the previous node.
  ASMJIT_INLINE void _setCursor(CBNode* node) noexcept { _cursor = node; }
  //! Set the current node to `node` and return the previous one.
  ASMJIT_API CBNode* setCursor(CBNode* node) noexcept;

  // --------------------------------------------------------------------------
  // [Label Management]
  // --------------------------------------------------------------------------

  //! Get a vector of CBLabel nodes.
  //!
  //! NOTE: If a label of some index is not associated with `CodeBuilder` it
  //! would be null, so always check for nulls if you iterate over the vector.
  ASMJIT_INLINE const ZoneVector<CBLabel*>& getLabelNodes() const noexcept { return _labelNodes; }

  //! Get `CBLabel` by `id`.
  ASMJIT_API Error getLabelNode(CBLabel** pOut, uint32_t id) noexcept;
  //! Get `CBLabel` by `label`.
  ASMJIT_INLINE Error getLabelNode(CBLabel** pOut, const Label& label) noexcept { return getLabelNode(pOut, label.getId()); }
  //! \internal
  ASMJIT_API Error registerLabelNode(CBLabel* node) noexcept;

  ASMJIT_API Label newLabel() override;
  ASMJIT_API Label newNamedLabel(
    const char* name,
    size_t nameLength = Globals::kNullTerminated,
    uint32_t type = Label::kTypeGlobal,
    uint32_t parentId = 0) override;
  ASMJIT_API Error bind(const Label& label) override;

  // --------------------------------------------------------------------------
  // [Pass Management]
  // --------------------------------------------------------------------------

  //! Get a vector of Pass objects that will be executed by `runPasses()`.
  ASMJIT_INLINE const ZoneVector<CBPass*>& getPasses() const noexcept { return _passes; }

  template<typename T>
  ASMJIT_INLINE T* newPassT() noexcept { return new(_codeZone.alloc(sizeof(T))) T(); }
  template<typename T, typename P0>
  ASMJIT_INLINE T* newPassT(P0 p0) noexcept { return new(_codeZone.alloc(sizeof(T))) T(p0); }
  template<typename T, typename P0, typename P1>
  ASMJIT_INLINE T* newPassT(P0 p0, P1 p1) noexcept { return new(_codeZone.alloc(sizeof(T))) T(p0, p1); }

  template<typename T>
  ASMJIT_INLINE Error addPassT() noexcept { return addPass(newPassT<T>()); }
  template<typename T, typename P0>
  ASMJIT_INLINE Error addPassT(P0 p0) noexcept { return addPass(newPassT<P0>(p0)); }
  template<typename T, typename P0, typename P1>
  ASMJIT_INLINE Error addPassT(P0 p0, P1 p1) noexcept { return addPass(newPassT<P0, P1>(p0, p1)); }

  //! Get a `CBPass` by name.
  ASMJIT_API CBPass* getPassByName(const char* name) const noexcept;
  //! Add `pass` to the list of passes.
  ASMJIT_API Error addPass(CBPass* pass) noexcept;
  //! Remove `pass` from the list of passes and delete it.
  ASMJIT_API Error deletePass(CBPass* pass) noexcept;

  //! Run all passes in order.
  ASMJIT_API Error runPasses();

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  ASMJIT_API Error _emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) override;
  ASMJIT_API Error _emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, const Operand_& o4, const Operand_& o5) override;

  // --------------------------------------------------------------------------
  // [Align]
  // --------------------------------------------------------------------------

  ASMJIT_API Error align(uint32_t mode, uint32_t alignment) override;

  // --------------------------------------------------------------------------
  // [Embed]
  // --------------------------------------------------------------------------

  ASMJIT_API Error embed(const void* data, uint32_t size) override;
  ASMJIT_API Error embedLabel(const Label& label) override;
  ASMJIT_API Error embedConstPool(const Label& label, const ConstPool& pool) override;

  // --------------------------------------------------------------------------
  // [Comment]
  // --------------------------------------------------------------------------

  ASMJIT_API Error comment(const char* s, size_t len = Globals::kNullTerminated) override;

  // --------------------------------------------------------------------------
  // [Serialize]
  // --------------------------------------------------------------------------

  //! Serialize everything theis `CodeBuilder` or `::CodeCompiler` contains to
  //! another `CodeEmitter`, which is usually an `Assembler` instance.
  ASMJIT_API Error serialize(CodeEmitter* dst);

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_DISABLE_LOGGING)
  ASMJIT_API Error dump(StringBuilder& sb, uint32_t logOptions = 0) const noexcept;
#endif

  // --------------------------------------------------------------------------
  // [Events]
  // --------------------------------------------------------------------------

  ASMJIT_API Error onAttach(CodeHolder* code) noexcept override;
  ASMJIT_API Error onDetach(CodeHolder* code) noexcept override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Zone _codeZone;                        //!< Base zone used to allocate nodes and `CBPass`.
  Zone _dataZone;                        //!< Data zone used to allocate data and names.
  Zone _passZone;                        //!< Pass zone, passed to `Pass::run()`.
  ZoneAllocator _allocator;              //!< Allocator that uses `_codeZone`.

  ZoneVector<CBPass*> _passes;           //!< Array of `CBPass` objects.
  ZoneVector<CBLabel*> _labelNodes;      //!< Maps label indexes to `CBLabel` nodes.

  CBNode* _firstNode;                    //!< First node of the current section.
  CBNode* _lastNode;                     //!< Last node of the current section.
  CBNode* _cursor;                       //!< Current node (cursor).

  uint32_t _nodeFlags;                   //!< Flags assigned to each new node.
};

// ============================================================================
// [asmjit::CBNode]
// ============================================================================

//! Node (CodeBuilder).
//!
//! Every node represents a building-block used by \ref CodeBuilder. It can be
//! instruction, data, label, comment, directive, or any other high-level
//! representation that can be transformed to the building blocks mentioned.
//! Every class that inherits \ref CodeBuilder can define its own nodes that it
//! can lower to basic nodes.
class CBNode {
public:
  ASMJIT_NONCOPYABLE(CBNode)

  //! Type of link, used to index `_link[2]` data.
  ASMJIT_ENUM(LinkType) {
    kLinkPrev       = 0,                 //!< Previous node in a double-linked list.
    kLinkNext       = 1,                 //!< Next node in a double-linked list.
    kLinkCount      = 2                  //!< Count of node links (must be 2 as it's a double-linked list)
  };

  //! Type of \ref CBNode.
  ASMJIT_ENUM(NodeType) {
    kNodeNone       = 0,                 //!< Invalid node (internal, don't use).

    // [CodeBuilder]
    kNodeInst       = 1,                 //!< Node is \ref CBInst or \ref CBInstExtended.
    kNodeData       = 2,                 //!< Node is \ref CBData.
    kNodeAlign      = 3,                 //!< Node is \ref CBAlign.
    kNodeLabel      = 4,                 //!< Node is \ref CBLabel.
    kNodeLabelData  = 5,                 //!< Node is \ref CBLabelData.
    kNodeConstPool  = 6,                 //!< Node is \ref CBConstPool.
    kNodeComment    = 7,                 //!< Node is \ref CBComment.
    kNodeSentinel   = 8,                 //!< Node is \ref CBSentinel.

    // [CodeCompiler]
    kNodeFunc       = 16,                //!< Node is \ref CCFunc     (acts as CBLabel).
    kNodeFuncRet    = 17,                //!< Node is \ref CCFuncRet  (acts as CBNode).
    kNodeFuncCall   = 18,                //!< Node is \ref CCFuncCall (acts as CBInst).

    // [UserDefined]
    kNodeUser       = 32                 //!< First id of a user-defined node.
  };

  //! Node flags, specify what the node is and/or does.
  ASMJIT_ENUM(Flags) {
    kFlagIsCode          = 0x01U,        //!< Node is code that can be executed (instruction, label, align, etc...).
    kFlagIsData          = 0x02U,        //!< Node is data that cannot be executed (data, const-pool, etc...).
    kFlagIsInformative   = 0x04U,        //!< Node is informative, can be removed and ignored.
    kFlagIsRemovable     = 0x08U,        //!< Node can be safely removed if unreachable.
    kFlagHasNoEffect     = 0x20U,        //!< Node does nothing when executed (label, align, explicit nop).

    kFlagActsAsInst      = 0x40U,        //!< Node is an instruction or acts as it.
    kFlagActsAsLabel     = 0x80U         //!< Node is a label or acts as it.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new \ref CBNode - always use \ref CodeBuilder to allocate nodes.
  ASMJIT_INLINE CBNode(CodeBuilder* cb, uint32_t type, uint32_t flags = 0) noexcept {
    _link[kLinkPrev] = nullptr;
    _link[kLinkNext] = nullptr;
    _any._nodeType = static_cast<uint8_t>(type);
    _any._nodeFlags = static_cast<uint8_t>(flags | cb->_nodeFlags);
    _any._reserved0 = 0;
    _any._reserved1 = 1;
    _position = 0;
    _passData = nullptr;
    _inlineComment = nullptr;
  }
  //! Destroy the `CBNode` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBNode() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE CBNode* getLink(size_t link) const noexcept {
    ASMJIT_ASSERT(link < kLinkCount);
    return _link[link];
  }

  //! Get previous node.
  ASMJIT_INLINE CBNode* getPrev() const noexcept { return _link[kLinkPrev]; }
  //! Get next node.
  ASMJIT_INLINE CBNode* getNext() const noexcept { return _link[kLinkNext]; }

  //! Set previous node (internal, use public CodeBuilder API to manage nodes).
  ASMJIT_INLINE void _setPrev(CBNode* node) noexcept { _link[kLinkPrev] = node; }
  //! Set next node (internal, use public CodeBuilder API to do manage nodes).
  ASMJIT_INLINE void _setNext(CBNode* node) noexcept { _link[kLinkNext] = node; }

  //! Get the node type, see \ref Type.
  ASMJIT_INLINE uint32_t getType() const noexcept { return _any._nodeType; }
  //! Set the node type, see \ref Type (internal).
  ASMJIT_INLINE void setType(uint32_t type) noexcept { _any._nodeType = IntUtils::toUInt8(type); }

  //! Get the node flags.
  ASMJIT_INLINE uint32_t getFlags() const noexcept { return _any._nodeFlags; }
  //! Get whether the instruction has flag `flag`.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) const noexcept { return (static_cast<uint32_t>(_any._nodeFlags) & flag) != 0; }
  //! Set node flags to `flags`.
  ASMJIT_INLINE void setFlags(uint32_t flags) noexcept { _any._nodeFlags = IntUtils::toUInt8(flags); }
  //! Add instruction `flags`.
  ASMJIT_INLINE void addFlags(uint32_t flags) noexcept { _any._nodeFlags = static_cast<uint8_t>(_any._nodeFlags | flags); }
  //! Clear instruction `flags`.
  ASMJIT_INLINE void clearFlags(uint32_t flags) noexcept { _any._nodeFlags = static_cast<uint8_t>(_any._nodeFlags & (flags ^ 0xFF)); }

  //! Get if the node is code that can be executed.
  ASMJIT_INLINE bool isCode() const noexcept { return hasFlag(kFlagIsCode); }
  //! Get if the node is data that cannot be executed.
  ASMJIT_INLINE bool isData() const noexcept { return hasFlag(kFlagIsData); }
  //! Get if the node is informative only (is never encoded like comment, etc...).
  ASMJIT_INLINE bool isInformative() const noexcept { return hasFlag(kFlagIsInformative); }
  //! Get if the node is removable if it's in an unreachable code block.
  ASMJIT_INLINE bool isRemovable() const noexcept { return hasFlag(kFlagIsRemovable); }

  //! The node has no effect when executed (label, .align, nop, ...).
  ASMJIT_INLINE bool hasNoEffect() const noexcept { return hasFlag(kFlagHasNoEffect); }

  //! Get if the node acts as `CBInst`.
  //!
  //! Returns true if the node is `CBInst` or if the node is of a different type
  //! that inherits `CBInst`.
  ASMJIT_INLINE bool actsAsInst() const noexcept { return hasFlag(kFlagActsAsInst); }

  //! Get if the node acts as `CBLabel`.
  //!
  //! Returns true if the node is `CBLabel` or if the node is of a different type
  //! that inherits `CBLabel`.
  ASMJIT_INLINE bool actsAsLabel() const noexcept { return hasFlag(kFlagActsAsLabel); }

  //! Get if the node has a position.
  ASMJIT_INLINE bool hasPosition() const noexcept { return _position != 0; }
  //! Get node position.
  ASMJIT_INLINE uint32_t getPosition() const noexcept { return _position; }
  //! Set node position.
  ASMJIT_INLINE void setPosition(uint32_t position) noexcept { _position = position; }

  //! Get if the node has associated work-data.
  ASMJIT_INLINE bool hasPassData() const noexcept { return _passData != nullptr; }
  //! Get work-data - data used during processing & transformations.
  template<typename T>
  ASMJIT_INLINE T* getPassData() const noexcept { return (T*)_passData; }
  //! Set work-data to `data`.
  template<typename T>
  ASMJIT_INLINE void setPassData(T* data) noexcept { _passData = (void*)data; }
  //! Reset work-data to null.
  ASMJIT_INLINE void resetPassData() noexcept { _passData = nullptr; }

  //! Get if the node has an inline comment.
  ASMJIT_INLINE bool hasInlineComment() const noexcept { return _inlineComment != nullptr; }
  //! Get an inline comment string.
  ASMJIT_INLINE const char* getInlineComment() const noexcept { return _inlineComment; }
  //! Set an inline comment string to `s`.
  ASMJIT_INLINE void setInlineComment(const char* s) noexcept { _inlineComment = s; }
  //! Set an inline comment string to null.
  ASMJIT_INLINE void resetInlineComment() noexcept { _inlineComment = nullptr; }

  //! Cast this node to `T*`.
  template<typename T>
  ASMJIT_INLINE T* as() noexcept { return static_cast<T*>(this); }
  //! Cast this node to `const T*`.
  template<typename T>
  ASMJIT_INLINE const T* as() const noexcept { return static_cast<const T*>(this); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CBNode* _link[2];                      //!< Links (previous and next nodes).

  struct AnyData {
    uint8_t _nodeType;                   //!< Node type, see \ref NodeType.
    uint8_t _nodeFlags;                  //!< Node flags.
    uint8_t _reserved0;
    uint8_t _reserved1;
  };

  struct InstData {
    uint8_t _nodeType;                   //!< Node type, see \ref NodeType.
    uint8_t _nodeFlags;                  //!< Node flags.
    uint8_t _opCount;                    //!< Number of operands.
    uint8_t _opCapacity;                 //!< Maximum number of operands (capacity).
  };

  struct SentinelData {
    uint8_t _nodeType;                   //!< Node type, see \ref NodeType.
    uint8_t _nodeFlags;                  //!< Node flags.
    uint8_t _sentinelType;               //!< Sentinel type.
    uint8_t _reserved0;
  };

  union {
    AnyData _any;
    InstData _inst;
    SentinelData _sentinel;
  };

  uint32_t _position;                    //!< Node position in code (should be unique).
  void* _passData;                       //!< Data used exclusively by the current `CBPass`.
  const char* _inlineComment;            //!< Inline comment or null if not used.
};

// ============================================================================
// [asmjit::CBInst]
// ============================================================================

//! Instruction (CodeBuilder).
//!
//! Wraps an instruction with its options and operands.
class CBInst : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBInst)

  enum {
    //! Count of embedded operands per `CBInst` that are always allocated as
    //! a part of the instruction. Minimum embedded operands is 4, but in 32-bit
    //! more pointers are smaller and we can embed 5. The rest (up to 6 operands)
    //! is always stored in `CBInstEx`.
    kBaseOpCapacity = static_cast<int>((128 - sizeof(CBNode) - sizeof(Inst::Detail)) / sizeof(Operand_)),
    kExtendedOpCapacity = 6
  };

  static ASMJIT_INLINE uint32_t capacityOfOpCount(uint32_t opCount) noexcept {
    return opCount <= kBaseOpCapacity ? kBaseOpCapacity : kExtendedOpCapacity;
  }

  static ASMJIT_INLINE size_t nodeSizeOfOpCapacity(uint32_t opCapacity) noexcept {
    size_t base = sizeof(CBInst) - kBaseOpCapacity * sizeof(Operand);
    return base + opCapacity * sizeof(Operand);
  }

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBInst` instance.
  ASMJIT_INLINE CBInst(CodeBuilder* cb, uint32_t instId, uint32_t options, uint32_t opCapacity = kBaseOpCapacity) noexcept
    : CBNode(cb, kNodeInst, kFlagIsCode | kFlagIsRemovable | kFlagActsAsInst),
      _instDetail(instId, options) {
    _inst._opCapacity = IntUtils::toUInt8(opCapacity);
    _inst._opCount = 0;
  }

  //! Destroy the `CBInst` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBInst() noexcept {}

  // --------------------------------------------------------------------------
  // [Init]
  // --------------------------------------------------------------------------

  //! Reset all built-in operands, including `extraReg`.
  ASMJIT_INLINE void _resetOps() noexcept {
    _instDetail.resetExtraReg();
    for (uint32_t i = 0, count = getOpCapacity(); i < count; i++)
      _opArray[i].reset();
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Inst::Detail& getInstDetail() noexcept { return _instDetail; }
  ASMJIT_INLINE const Inst::Detail& getInstDetail() const noexcept { return _instDetail; }

  //! Get the instruction id, see \ref Inst::Id.
  ASMJIT_INLINE uint32_t getInstId() const noexcept { return _instDetail.instId; }
  //! Set the instruction id to `instId`, see \ref Inst::Id.
  ASMJIT_INLINE void setInstId(uint32_t instId) noexcept { _instDetail.instId = instId; }

  //! Get instruction options.
  ASMJIT_INLINE uint32_t getInstOptions() const noexcept { return _instDetail.options; }
  //! Set instruction options.
  ASMJIT_INLINE void setInstOptions(uint32_t options) noexcept { _instDetail.options = options; }
  //! Add instruction options.
  ASMJIT_INLINE void addInstOptions(uint32_t options) noexcept { _instDetail.options |= options; }
  //! Mask instruction options.
  ASMJIT_INLINE void andInstOptions(uint32_t options) noexcept { _instDetail.options &= options; }
  //! Clear instruction options.
  ASMJIT_INLINE void clearInstOptions(uint32_t options) noexcept { _instDetail.options &= ~options; }

  //! Get if the node has an extra register operand.
  ASMJIT_INLINE bool hasExtraReg() const noexcept { return _instDetail.hasExtraReg(); }
  //! Get extra register operand.
  ASMJIT_INLINE RegOnly& getExtraReg() noexcept { return _instDetail.extraReg; }
  //! \overload
  ASMJIT_INLINE const RegOnly& getExtraReg() const noexcept { return _instDetail.extraReg; }
  //! Set extra register operand to `reg`.
  ASMJIT_INLINE void setExtraReg(const Reg& reg) noexcept { _instDetail.extraReg.init(reg); }
  //! Set extra register operand to `reg`.
  ASMJIT_INLINE void setExtraReg(const RegOnly& reg) noexcept { _instDetail.extraReg.init(reg); }
  //! Reset extra register operand.
  ASMJIT_INLINE void resetExtraReg() noexcept { _instDetail.extraReg.reset(); }

  //! Get operands capacity.
  ASMJIT_INLINE uint32_t getOpCapacity() const noexcept { return _inst._opCapacity; }
  //! Get operands count.
  ASMJIT_INLINE uint32_t getOpCount() const noexcept { return _inst._opCount; }
  //! Set operands count.
  ASMJIT_INLINE void setOpCount(uint32_t opCount) noexcept { _inst._opCount = IntUtils::toUInt8(opCount); }

  //! Get operands array.
  ASMJIT_INLINE Operand* getOpArray() noexcept { return (Operand*)_opArray; }
  //! Get operands array (const).
  ASMJIT_INLINE const Operand* getOpArray() const noexcept { return (const Operand*)_opArray; }

  ASMJIT_INLINE Operand& getOp(uint32_t index) noexcept {
    ASMJIT_ASSERT(index < getOpCapacity());
    return _opArray[index].as<Operand>();
  }

  ASMJIT_INLINE const Operand& getOp(uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < getOpCapacity());
    return _opArray[index].as<Operand>();
  }

  ASMJIT_INLINE void setOp(uint32_t index, const Operand_& op) noexcept {
    ASMJIT_ASSERT(index < getOpCapacity());
    _opArray[index].copyFrom(op);
  }

  ASMJIT_INLINE void resetOp(uint32_t index) noexcept {
    ASMJIT_ASSERT(index < getOpCapacity());
    _opArray[index].reset();
  }

  // --------------------------------------------------------------------------
  // [Utilities]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool hasOpType(uint32_t opType) const noexcept {
    for (uint32_t i = 0, count = getOpCount(); i < count; i++)
      if (_opArray[i].getOp() == opType)
        return true;
    return false;
  }

  ASMJIT_INLINE bool hasRegOp() const noexcept { return hasOpType(Operand::kOpReg); }
  ASMJIT_INLINE bool hasMemOp() const noexcept { return hasOpType(Operand::kOpMem); }
  ASMJIT_INLINE bool hasImmOp() const noexcept { return hasOpType(Operand::kOpImm); }
  ASMJIT_INLINE bool hasLabelOp() const noexcept { return hasOpType(Operand::kOpLabel); }

  ASMJIT_INLINE uint32_t indexOfOpType(uint32_t opType) const noexcept {
    uint32_t i = 0;
    uint32_t count = getOpCount();

    while (i < count) {
      if (_opArray[i].getOp() == opType)
        break;
      i++;
    }

    return i;
  }

  ASMJIT_INLINE uint32_t indexOfMemOp() const noexcept { return indexOfOpType(Operand::kOpMem); }
  ASMJIT_INLINE uint32_t indexOfImmOp() const noexcept { return indexOfOpType(Operand::kOpImm); }
  ASMJIT_INLINE uint32_t indexOfLabelOp() const noexcept { return indexOfOpType(Operand::kOpLabel); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Inst::Detail _instDetail;              //!< Instruction id, options, and extra register.
  Operand_ _opArray[kBaseOpCapacity];    //!< First 4 or 5 operands (indexed from 0).
};

// ============================================================================
// [asmjit::CBInstEx]
// ============================================================================

class CBInstEx : public CBInst {
public:
  ASMJIT_NONCOPYABLE(CBInstEx)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBInstEx` instance.
  ASMJIT_INLINE CBInstEx(CodeBuilder* cb, uint32_t instId, uint32_t options, uint32_t opCapacity = 6) noexcept
    : CBInst(cb, instId, options, opCapacity) {}

  //! Destroy the `CBInstEx` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBInstEx() noexcept {}

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Continued `_opArray[]` to hold up to 6 (maximum) operands.
  Operand_ _opArrayEx[kExtendedOpCapacity - kBaseOpCapacity];
};

// ============================================================================
// [asmjit::CBData]
// ============================================================================

//! Data (CodeBuilder).
//!
//! Wraps `.data` directive. The node contains data that will be placed at the
//! node's position in the assembler stream. The data is considered to be RAW;
//! no analysis nor byte-order conversion is performed on RAW data.
class CBData : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBData)

  enum {
    kInlineBufferSize = static_cast<int>(64 - sizeof(CBNode) - 4)
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBData` instance.
  ASMJIT_INLINE CBData(CodeBuilder* cb, void* data, uint32_t size) noexcept
    : CBNode(cb, kNodeData, kFlagIsData) {

    if (size <= kInlineBufferSize) {
      if (data) ::memcpy(_buf, data, size);
    }
    else {
      _externalPtr = static_cast<uint8_t*>(data);
    }
    _size = size;
  }

  //! Destroy the `CBData` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBData() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get size of the data.
  ASMJIT_INLINE uint32_t getSize() const noexcept { return _size; }
  //! Get pointer to the data.
  ASMJIT_INLINE uint8_t* getData() const noexcept { return _size <= kInlineBufferSize ? const_cast<uint8_t*>(_buf) : _externalPtr; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      uint8_t _buf[kInlineBufferSize];   //!< Embedded data buffer.
      uint32_t _size;                    //!< Size of the data.
    };
    struct {
      uint8_t* _externalPtr;             //!< Pointer to external data.
    };
  };
};

// ============================================================================
// [asmjit::CBAlign]
// ============================================================================

//! Align directive (CodeBuilder).
//!
//! Wraps `.align` directive.
class CBAlign : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBAlign)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBAlign` instance.
  ASMJIT_INLINE CBAlign(CodeBuilder* cb, uint32_t mode, uint32_t alignment) noexcept
    : CBNode(cb, kNodeAlign, kFlagIsCode | kFlagHasNoEffect),
      _mode(mode),
      _alignment(alignment) {}
  //! Destroy the `CBAlign` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBAlign() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get align mode.
  ASMJIT_INLINE uint32_t getMode() const noexcept { return _mode; }
  //! Set align mode.
  ASMJIT_INLINE void setMode(uint32_t mode) noexcept { _mode = mode; }

  //! Get align offset in bytes.
  ASMJIT_INLINE uint32_t getAlignment() const noexcept { return _alignment; }
  //! Set align offset in bytes to `offset`.
  ASMJIT_INLINE void setAlignment(uint32_t alignment) noexcept { _alignment = alignment; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _mode;                        //!< Align mode, see \ref AlignMode.
  uint32_t _alignment;                   //!< Alignment (in bytes).
};

// ============================================================================
// [asmjit::CBLabel]
// ============================================================================

//! Label (CodeBuilder).
class CBLabel : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBLabel)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBLabel` instance.
  ASMJIT_INLINE CBLabel(CodeBuilder* cb, uint32_t id = 0) noexcept
    : CBNode(cb, kNodeLabel, kFlagHasNoEffect | kFlagActsAsLabel),
      _id(id) {}
  //! Destroy the `CBLabel` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBLabel() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the label id.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _id; }
  //! Get the label as `Label` operand.
  ASMJIT_INLINE Label getLabel() const noexcept { return Label(_id); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _id;
};

// ============================================================================
// [asmjit::CBLabelData]
// ============================================================================

class CBLabelData : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBLabelData)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBLabelData` instance.
  ASMJIT_INLINE CBLabelData(CodeBuilder* cb, uint32_t id = 0) noexcept
    : CBNode(cb, kNodeLabelData, kFlagIsData),
      _id(id) {}

  //! Destroy the `CBLabelData` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBLabelData() noexcept {}

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Get the label id.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _id; }
  //! Set the label id (use with caution, improper use can break a lot of things).
  ASMJIT_INLINE void setId(uint32_t id) noexcept { _id = id; }

  //! Get the label as `Label` operand.
  ASMJIT_INLINE Label getLabel() const noexcept { return Label(_id); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _id;
};

// ============================================================================
// [asmjit::CBConstPool]
// ============================================================================

class CBConstPool : public CBLabel {
public:
  ASMJIT_NONCOPYABLE(CBConstPool)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBConstPool` instance.
  ASMJIT_INLINE CBConstPool(CodeBuilder* cb, uint32_t id = 0) noexcept
    : CBLabel(cb, id),
      _constPool(&cb->_codeZone) {

    setType(kNodeConstPool);
    addFlags(kFlagIsData);
    clearFlags(kFlagIsCode | kFlagHasNoEffect);
  }

  //! Destroy the `CBConstPool` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBConstPool() noexcept {}

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE ConstPool& getConstPool() noexcept { return _constPool; }
  ASMJIT_INLINE const ConstPool& getConstPool() const noexcept { return _constPool; }

  //! Get whether the constant-pool is empty.
  ASMJIT_INLINE bool isEmpty() const noexcept { return _constPool.isEmpty(); }
  //! Get the size of the constant-pool in bytes.
  ASMJIT_INLINE size_t getSize() const noexcept { return _constPool.getSize(); }
  //! Get minimum alignment.
  ASMJIT_INLINE size_t getAlignment() const noexcept { return _constPool.getAlignment(); }

  //! See \ref ConstPool::add().
  ASMJIT_INLINE Error add(const void* data, size_t size, size_t& dstOffset) noexcept {
    return _constPool.add(data, size, dstOffset);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ConstPool _constPool;
};

// ============================================================================
// [asmjit::CBComment]
// ============================================================================

//! Comment (CodeBuilder).
class CBComment : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBComment)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBComment` instance.
  ASMJIT_INLINE CBComment(CodeBuilder* cb, const char* comment) noexcept
    : CBNode(cb, kNodeComment, kFlagIsInformative | kFlagHasNoEffect | kFlagIsRemovable) {
    _inlineComment = comment;
  }

  //! Destroy the `CBComment` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBComment() noexcept {}
};

// ============================================================================
// [asmjit::CBSentinel]
// ============================================================================

//! Sentinel (CodeBuilder).
//!
//! Sentinel is a marker that is completely ignored by the code builder. It's
//! used to remember a position in a code as it never gets removed by any pass.
class CBSentinel : public CBNode {
public:
  ASMJIT_NONCOPYABLE(CBSentinel)

  //! Type of the sentinel (purery informative purpose).
  ASMJIT_ENUM(SentinelType) {
    kSentinelUnknown = 0U,
    kSentinelFuncEnd = 1U
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CBSentinel` instance.
  ASMJIT_INLINE CBSentinel(CodeBuilder* cb, uint32_t sentinelType = kSentinelUnknown) noexcept
    : CBNode(cb, kNodeSentinel, kFlagIsInformative | kFlagHasNoEffect) {

    _sentinel._sentinelType = IntUtils::toUInt8(sentinelType);
  }

  //! Destroy the `CBSentinel` instance (NEVER CALLED).
  ASMJIT_INLINE ~CBSentinel() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t getSentinelType() const noexcept { return _sentinel._sentinelType; }
  ASMJIT_INLINE void setSentinelType(uint32_t type) noexcept { _sentinel._sentinelType = IntUtils::toUInt8(type); }
};

// ============================================================================
// [asmjit::CBPass]
// ============================================================================

//! `CodeBuilder` pass is used to implement code transformations, analysis,
//! and lowering.
class ASMJIT_VIRTAPI CBPass {
public:
  ASMJIT_NONCOPYABLE(CBPass);

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API CBPass(const char* name) noexcept;
  ASMJIT_API virtual ~CBPass() noexcept;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Process the code stored in CodeBuilder.
  //!
  //! This is the only function that is called by the `CodeBuilder` to process
  //! the code. It passes `zone`, which will be reset after the `run()` returns
  //! by `CodeBuilder`.
  virtual Error run(Zone* zone, Logger* logger) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE const CodeBuilder* cb() const noexcept { return _cb; }
  ASMJIT_INLINE const char* getName() const noexcept { return _name; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CodeBuilder* _cb;                      //!< CodeBuilder this pass is assigned to.
  const char* _name;                     //!< Name of the pass.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_BUILDER
#endif // _ASMJIT_BASE_CODEBUILDER_H
