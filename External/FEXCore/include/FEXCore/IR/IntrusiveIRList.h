#pragma once

#include "FEXCore/IR/IR.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <tuple>
#include <vector>

namespace FEXCore::IR {
/**
 * @brief This is purely an intrusive allocator
 * This doesn't support any form of ordering at all
 * Just provides a chunk of memory for allocating IR nodes from
 *
 * Can potentially support reallocation if we are smart and make sure to invalidate anything holding a true pointer
 */
class IntrusiveAllocator final {
  public:
    IntrusiveAllocator() = delete;
    IntrusiveAllocator(IntrusiveAllocator &&) = delete;
    IntrusiveAllocator(size_t Size)
      : MemorySize {Size} {
      Data = reinterpret_cast<uintptr_t>(malloc(Size));
    }

    ~IntrusiveAllocator() {
      free(reinterpret_cast<void*>(Data));
    }

    bool CheckSize(size_t Size) {
      size_t NewOffset = CurrentOffset + Size;
      return NewOffset <= MemorySize;
    }

    void *Allocate(size_t Size) {
      assert(CheckSize(Size) &&
        "Ran out of space in IntrusiveAllocator during allocation");
      size_t NewOffset = CurrentOffset + Size;
      uintptr_t NewPointer = Data + CurrentOffset;
      CurrentOffset = NewOffset;
      return reinterpret_cast<void*>(NewPointer);
    }

    size_t Size() const { return CurrentOffset; }
    size_t BackingSize() const { return MemorySize; }

    uintptr_t const Begin() const { return Data; }

    void Reset() { CurrentOffset = 0; }

    void CopyData(IntrusiveAllocator const &rhs) {
      CurrentOffset = rhs.CurrentOffset;
      memcpy(reinterpret_cast<void*>(Data), reinterpret_cast<void*>(rhs.Data), CurrentOffset);
    }

  private:
    size_t CurrentOffset {0};
    size_t MemorySize;
    uintptr_t Data;
};

template<bool Copy>
class IRListView final {
public:
  IRListView() = delete;
  IRListView(IRListView<Copy> &&) = delete;

  IRListView(IntrusiveAllocator *Data, IntrusiveAllocator *List) {
    DataSize = Data->Size();
    ListSize = List->Size();

    if (Copy) {
      IRData = malloc(DataSize + ListSize);
      ListData = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(IRData) + DataSize);
      memcpy(IRData, reinterpret_cast<void*>(Data->Begin()), DataSize);
      memcpy(ListData, reinterpret_cast<void*>(List->Begin()), ListSize);
    }
    else {
      // We are just pointing to the data
      IRData = reinterpret_cast<void*>(Data->Begin());
      ListData = reinterpret_cast<void*>(List->Begin());
    }
  }

  ~IRListView() {
    if (Copy) {
      free (IRData);
      // ListData is just offset from IRData
    }
  }

  uintptr_t const GetData() const { return reinterpret_cast<uintptr_t>(IRData); }
  uintptr_t const GetListData() const { return reinterpret_cast<uintptr_t>(ListData); }

  size_t GetDataSize() const { return DataSize; }
  size_t GetListSize() const { return ListSize; }
  size_t GetSSACount() const { return ListSize / sizeof(OrderedNode); }

  using iterator = NodeWrapperIterator;

  iterator begin() const noexcept
  {
    OrderedNodeWrapper Wrapped;
    Wrapped.NodeOffset = sizeof(OrderedNode);
    return iterator(reinterpret_cast<uintptr_t>(ListData), Wrapped);
  }

  /**
   * @brief This is not an iterator that you can reverse iterator through!
   *
   * @return Our iterator sentinal to ensure ending correctly
   */
  iterator end() const noexcept
  {
    OrderedNodeWrapper Wrapped;
    Wrapped.NodeOffset = 0;
    return iterator(reinterpret_cast<uintptr_t>(ListData), Wrapped);
  }

  /**
   * @brief Convert a OrderedNodeWrapper to an interator that we can iterate over
   * @return Iterator for this op
   */
  iterator at(OrderedNodeWrapper Node) const noexcept {
    return iterator(reinterpret_cast<uintptr_t>(ListData), Node);
  }

private:
  void *IRData;
  void *ListData;
  size_t DataSize;
  size_t ListSize;
};
}

