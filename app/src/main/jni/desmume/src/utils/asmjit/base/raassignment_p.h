// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_RAASSIGNMENT_P_H
#define _ASMJIT_BASE_RAASSIGNMENT_P_H

#include "../asmjit_build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/radefs_p.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_ra
//! \{

// ============================================================================
// [asmjit::RAAssignment]
// ============================================================================

class RAAssignment {
  ASMJIT_NONCOPYABLE(RAAssignment)

public:
  ASMJIT_ENUM(Ids) {
    kPhysNone = 0xFF,
    kWorkNone = RAWorkReg::kIdNone
  };

  ASMJIT_ENUM(DirtyBit) {
    kClean = 0,
    kDirty = 1
  };

  // --------------------------------------------------------------------------
  // [Layout]
  // --------------------------------------------------------------------------

  struct Layout {
    ASMJIT_INLINE void reset() noexcept {
      physIndex.reset();
      physCount.reset();
      physTotal = 0;
      workCount = 0;
      workRegs = nullptr;
    }

    RARegIndex physIndex;                //!< Index of architecture registers per group.
    RARegCount physCount;                //!< Count of architecture registers per group.
    uint32_t physTotal;                  //!< Count of physical registers of all groups.
    uint32_t workCount;                  //!< Count of work registers.
    const RAWorkRegs* workRegs;          //!< WorkRegs data (vector).
  };

  // --------------------------------------------------------------------------
  // [PhysToWorkMap]
  // --------------------------------------------------------------------------

  struct PhysToWorkMap {
    static ASMJIT_INLINE size_t sizeOf(uint32_t count) noexcept {
      return sizeof(PhysToWorkMap) - sizeof(uint32_t) + static_cast<size_t>(count) * sizeof(uint32_t);
    }

    ASMJIT_INLINE void reset(uint32_t count) noexcept {
      assigned.reset();
      dirty.reset();

      for (uint32_t i = 0; i < count; i++)
        workIds[i] = kWorkNone;
    }

    ASMJIT_INLINE void copyFrom(const PhysToWorkMap* other, uint32_t count) noexcept {
      size_t size = sizeOf(count);
      ::memcpy(this, other, size);
    }

    RARegMask assigned;                  //!< Assigned registers (each bit represents one physical reg).
    RARegMask dirty;                     //!< Dirty registers (spill slot out of sync or no spill slot).
    uint32_t workIds[1 /* ... */];       //!< PhysReg to WorkReg mapping.
  };

  // --------------------------------------------------------------------------
  // [WorkToPhysMap]
  // --------------------------------------------------------------------------

  struct WorkToPhysMap {
    static ASMJIT_INLINE size_t sizeOf(uint32_t count) noexcept {
      return static_cast<size_t>(count) * sizeof(uint8_t);
    }

    ASMJIT_INLINE void reset(uint32_t count) noexcept {
      for (uint32_t i = 0; i < count; i++)
        physIds[i] = kPhysNone;
    }

    ASMJIT_INLINE void copyFrom(const WorkToPhysMap* other, uint32_t count) noexcept {
      size_t size = sizeOf(count);
      if (ASMJIT_LIKELY(size))
        ::memcpy(this, other, size);
    }

    uint8_t physIds[1 /* ... */];        //!< WorkReg to PhysReg mapping
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE RAAssignment() noexcept {
    _layout.reset();
    resetMaps();
  }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void initLayout(const RARegCount& physCount, const RAWorkRegs& workRegs) noexcept {
    // Layout must be initialized before data.
    ASMJIT_ASSERT(_physToWorkMap == nullptr);
    ASMJIT_ASSERT(_workToPhysMap == nullptr);

    _layout.physIndex.buildIndexes(physCount);
    _layout.physCount = physCount;
    _layout.physTotal = uint32_t(_layout.physIndex[Reg::kGroupVirt - 1]) +
                        uint32_t(_layout.physCount[Reg::kGroupVirt - 1]) ;
    _layout.workCount = workRegs.getLength();
    _layout.workRegs = &workRegs;
  }

  ASMJIT_INLINE void initMaps(PhysToWorkMap* physToWorkMap, WorkToPhysMap* workToPhysMap) noexcept {
    _physToWorkMap = physToWorkMap;
    _workToPhysMap = workToPhysMap;
    for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
      _physToWorkIds[group] = physToWorkMap->workIds + _layout.physIndex.get(group);
  }

  ASMJIT_INLINE void resetMaps() noexcept {
    _physToWorkMap = nullptr;
    _workToPhysMap = nullptr;
    for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
      _physToWorkIds[group] = nullptr;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE PhysToWorkMap* getPhysToWorkMap() const noexcept { return _physToWorkMap; }
  ASMJIT_INLINE WorkToPhysMap* getWorkToPhysMap() const noexcept { return _workToPhysMap; }

  ASMJIT_INLINE RARegMask& getAssigned() noexcept { return _physToWorkMap->assigned; }
  ASMJIT_INLINE const RARegMask& getAssigned() const noexcept { return _physToWorkMap->assigned; }
  ASMJIT_INLINE uint32_t getAssigned(uint32_t group) const noexcept { return _physToWorkMap->assigned[group]; }

  ASMJIT_INLINE RARegMask& getDirty() noexcept { return _physToWorkMap->dirty; }
  ASMJIT_INLINE const RARegMask& getDirty() const noexcept { return _physToWorkMap->dirty; }
  ASMJIT_INLINE uint32_t getDirty(uint32_t group) const noexcept { return _physToWorkMap->dirty[group]; }

  ASMJIT_INLINE uint32_t workToPhysId(uint32_t group, uint32_t workId) const noexcept {
    ASMJIT_UNUSED(group);
    ASMJIT_ASSERT(workId != kWorkNone);
    ASMJIT_ASSERT(workId < _layout.workCount);
    return _workToPhysMap->physIds[workId];
  }

  ASMJIT_INLINE uint32_t physToWorkId(uint32_t group, uint32_t physId) const noexcept {
    ASMJIT_ASSERT(physId < Globals::kMaxPhysRegs);
    return _physToWorkIds[group][physId];
  }

  ASMJIT_INLINE bool isPhysAssigned(uint32_t group, uint32_t physId) const noexcept {
    ASMJIT_ASSERT(physId < Globals::kMaxPhysRegs);
    uint32_t regMask = IntUtils::mask(physId);
    return (_physToWorkMap->assigned[group] & regMask) != 0;
  }

  ASMJIT_INLINE bool isPhysDirty(uint32_t group, uint32_t physId) const noexcept {
    ASMJIT_ASSERT(physId < Globals::kMaxPhysRegs);
    uint32_t regMask = IntUtils::mask(physId);
    return (_physToWorkMap->dirty[group] & regMask) != 0;
  }

  // --------------------------------------------------------------------------
  // [Assignment]
  // --------------------------------------------------------------------------

  // These are low-level allocation helpers that are used to update the current
  // mappings between physical and virt/work registers and also to update masks
  // that represent allocated and dirty registers. These functions don't emit
  // any code; they are only used to update and keep all mappings in sync.

  //! Assign [VirtReg/WorkReg] to a physical register.
  ASMJIT_INLINE void assign(uint32_t group, uint32_t workId, uint32_t physId, uint32_t dirty) noexcept {
    ASMJIT_ASSERT(workToPhysId(group, workId) == kPhysNone);
    ASMJIT_ASSERT(physToWorkId(group, physId) == kWorkNone);
    ASMJIT_ASSERT(!isPhysAssigned(group, physId));
    ASMJIT_ASSERT(!isPhysDirty(group, physId));

    _workToPhysMap->physIds[workId] = IntUtils::toUInt8(physId);
    _physToWorkIds[group][physId] = workId;

    uint32_t regMask = IntUtils::mask(physId);
    _physToWorkMap->assigned[group] |= regMask;
    _physToWorkMap->dirty[group] |= regMask & IntUtils::maskFromBool<uint32_t>(dirty);

    verify();
  }

  //! Reassign [VirtReg/WorkReg] to `dstPhysId` from `srcPhysId`.
  ASMJIT_INLINE void reassign(uint32_t group, uint32_t workId, uint32_t dstPhysId, uint32_t srcPhysId) noexcept {
    ASMJIT_ASSERT(dstPhysId != srcPhysId);
    ASMJIT_ASSERT(workToPhysId(group, workId) == srcPhysId);
    ASMJIT_ASSERT(physToWorkId(group, srcPhysId) == workId);
    ASMJIT_ASSERT(isPhysAssigned(group, srcPhysId) == true);
    ASMJIT_ASSERT(isPhysAssigned(group, dstPhysId) == false);

    _workToPhysMap->physIds[workId] = IntUtils::toUInt8(dstPhysId);
    _physToWorkIds[group][srcPhysId] = kWorkNone;
    _physToWorkIds[group][dstPhysId] = workId;

    uint32_t srcMask = IntUtils::mask(srcPhysId);
    uint32_t dstMask = IntUtils::mask(dstPhysId);

    uint32_t dirty = (_physToWorkMap->dirty[group] & srcMask) != 0;
    uint32_t regMask = dstMask | srcMask;

    _physToWorkMap->assigned[group] ^= regMask;
    _physToWorkMap->dirty[group] ^= regMask & IntUtils::maskFromBool<uint32_t>(dirty);

    verify();
  }

  ASMJIT_INLINE void swap(uint32_t group, uint32_t aWorkId, uint32_t aPhysId, uint32_t bWorkId, uint32_t bPhysId) noexcept {
    ASMJIT_ASSERT(aPhysId != bPhysId);
    ASMJIT_ASSERT(workToPhysId(group, aWorkId) == aPhysId);
    ASMJIT_ASSERT(workToPhysId(group, bWorkId) == bPhysId);
    ASMJIT_ASSERT(physToWorkId(group, aPhysId) == aWorkId);
    ASMJIT_ASSERT(physToWorkId(group, bPhysId) == bWorkId);
    ASMJIT_ASSERT(isPhysAssigned(group, aPhysId));
    ASMJIT_ASSERT(isPhysAssigned(group, bPhysId));

    _workToPhysMap->physIds[aWorkId] = IntUtils::toUInt8(bPhysId);
    _workToPhysMap->physIds[bWorkId] = IntUtils::toUInt8(aPhysId);
    _physToWorkIds[group][aPhysId] = bWorkId;
    _physToWorkIds[group][bPhysId] = aWorkId;

    uint32_t aMask = IntUtils::mask(aPhysId);
    uint32_t bMask = IntUtils::mask(bPhysId);

    uint32_t flipMask = IntUtils::maskFromBool<uint32_t>(
      ((_physToWorkMap->dirty[group] & aMask) != 0) ^
      ((_physToWorkMap->dirty[group] & bMask) != 0));

    uint32_t regMask = aMask | bMask;
    _physToWorkMap->dirty[group] ^= regMask & flipMask;

    verify();
  }

  //! Unassign [VirtReg/WorkReg] from a physical register.
  ASMJIT_INLINE void unassign(uint32_t group, uint32_t workId, uint32_t physId) noexcept {
    ASMJIT_ASSERT(physId < Globals::kMaxPhysRegs);
    ASMJIT_ASSERT(workToPhysId(group, workId) == physId);
    ASMJIT_ASSERT(physToWorkId(group, physId) == workId);
    ASMJIT_ASSERT(isPhysAssigned(group, physId));

    _workToPhysMap->physIds[workId] = kPhysNone;
    _physToWorkIds[group][physId] = kWorkNone;

    uint32_t regMask = IntUtils::mask(physId);
    _physToWorkMap->assigned[group] &= ~regMask;
    _physToWorkMap->dirty[group] &= ~regMask;

    verify();
  }

  ASMJIT_INLINE void makeClean(uint32_t group, uint32_t workId, uint32_t physId) noexcept {
    ASMJIT_UNUSED(workId);

    uint32_t regMask = IntUtils::mask(physId);
    _physToWorkMap->dirty[group] &= ~regMask;
  }

  ASMJIT_INLINE void makeDirty(uint32_t group, uint32_t workId, uint32_t physId) noexcept {
    ASMJIT_UNUSED(workId);

    uint32_t regMask = IntUtils::mask(physId);
    _physToWorkMap->dirty[group] |= regMask;
  }

  // --------------------------------------------------------------------------
  // [Copy / Swap]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void copyFrom(const PhysToWorkMap* physToWorkMap, const WorkToPhysMap* workToPhysMap) noexcept {
    ::memcpy(_physToWorkMap, physToWorkMap, PhysToWorkMap::sizeOf(_layout.physTotal));
    ::memcpy(_workToPhysMap, workToPhysMap, WorkToPhysMap::sizeOf(_layout.workCount));
  }

  ASMJIT_INLINE void copyFrom(const RAAssignment& other) noexcept {
    copyFrom(other.getPhysToWorkMap(), other.getWorkToPhysMap());
  }

  ASMJIT_INLINE void swapWith(RAAssignment& other) noexcept {
    std::swap(_workToPhysMap, other._workToPhysMap);
    std::swap(_physToWorkMap, other._physToWorkMap);

    for (uint32_t group = 0; group < Reg::kGroupVirt; group++)
      std::swap(_physToWorkIds[group], other._physToWorkIds[group]);
  }

  // --------------------------------------------------------------------------
  // [Verify]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_DEBUG)
  ASMJIT_NOINLINE void verify() noexcept {
    uint32_t workCount = _layout.workCount;

    // Verify WorkToPhysMap.
    {
      for (uint32_t workId = 0; workId < workCount; workId++) {
        uint32_t physId = _workToPhysMap->physIds[workId];
        if (physId != kPhysNone) {
          const RAWorkReg* workReg = _layout.workRegs->getAt(workId);
          uint32_t group = workReg->getGroup();
          ASMJIT_ASSERT(_physToWorkIds[group][physId] == workId);
        }
      }
    }

    // Verify PhysToWorkMap.
    {
      for (uint32_t group = 0; group < Reg::kGroupVirt; group++) {
        uint32_t physCount = _layout.physCount[group];
        for (uint32_t physId = 0; physId < physCount; physId++) {
          uint32_t workId = _physToWorkIds[group][physId];
          if (workId != kWorkNone) {
            ASMJIT_ASSERT(_workToPhysMap->physIds[workId] == physId);
          }
        }
      }
    }
  }
#else
  ASMJIT_INLINE void verify() noexcept {}
#endif

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Layout _layout;                        //!< Physical registers layout.
  WorkToPhysMap* _workToPhysMap;         //!< WorkReg to PhysReg mapping.
  PhysToWorkMap* _physToWorkMap;         //!< PhysReg to WorkReg mapping and assigned/dirty bits.
  uint32_t* _physToWorkIds[Reg::kGroupVirt]; //!< Optimization to translate PhysRegs to WorkRegs faster.
};
//! \}

} // asmjit namespace

// [Api-End]
#include "../asmjit_apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_RAASSIGNMENT_P_H
