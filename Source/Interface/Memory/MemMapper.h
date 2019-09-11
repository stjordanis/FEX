#pragma once
#include "Interface/Memory/SharedMem.h"
#include <FEXCore/Memory/MemMapper.h>
#include <stdint.h>
#include <vector>

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::Memory {

  class MemMapper final {
  friend struct FEXCore::Context::Context;
  public:
    void SetBaseRegion(FEXCore::SHM::SHMObject *NewSHM) {
      SHM = reinterpret_cast<FEXCore::SHM::InternalSHMObject*>(NewSHM);
    }

    void *MapRegion(uint64_t Offset, size_t Size, bool Fixed = true);
    void *MapRegion(uint64_t Offset, size_t Size, uint32_t Flags, bool Fixed = true);
    void *ChangeMappedRegion(uint64_t Offset, size_t Size, uint32_t Flags, bool Fixed = true);

    void UnmapRegion(void *Ptr, size_t Size);

    void *GetMemoryBase() { return SHM->Object.Ptr; }

    void *GetPointer(uint64_t Offset);
    template<typename T>
    T GetPointer(uint64_t Offset) {
      return reinterpret_cast<T>(GetPointer(Offset));
    }

    template<typename T>
    T GetBaseOffset(uint64_t Offset) {
      return reinterpret_cast<T>((reinterpret_cast<uintptr_t>(GetMemoryBase()) + Offset));
    }

  private:
    FEXCore::SHM::InternalSHMObject *SHM;
    std::vector<FEXCore::Memory::MemRegion> MappedRegions{};
  };
}
