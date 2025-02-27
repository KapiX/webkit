/*
 * Copyright (C) 2012-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#include "AllocationFailureMode.h"
#include "AllocatorAttributes.h"
#include "FreeList.h"
#include "MarkedBlock.h"
#include <wtf/DataLog.h>
#include <wtf/FastBitVector.h>
#include <wtf/SharedTask.h>
#include <wtf/Vector.h>

namespace JSC {

class GCDeferralContext;
class Heap;
class MarkedSpace;
class LLIntOffsetsExtractor;

#define FOR_EACH_MARKED_ALLOCATOR_BIT(macro) \
    macro(live, Live) /* The set of block indices that have actual blocks. */\
    macro(empty, Empty) /* The set of all blocks that have no live objects. */ \
    macro(allocated, Allocated) /* The set of all blocks that are full of live objects. */\
    macro(canAllocateButNotEmpty, CanAllocateButNotEmpty) /* The set of all blocks are neither empty nor retired (i.e. are more than minMarkedBlockUtilization full). */ \
    macro(destructible, Destructible) /* The set of all blocks that may have destructors to run. */\
    macro(eden, Eden) /* The set of all blocks that have new objects since the last GC. */\
    macro(unswept, Unswept) /* The set of all blocks that could be swept by the incremental sweeper. */\
    \
    /* These are computed during marking. */\
    macro(markingNotEmpty, MarkingNotEmpty) /* The set of all blocks that are not empty. */ \
    macro(markingRetired, MarkingRetired) /* The set of all blocks that are retired. */

// FIXME: We defined canAllocateButNotEmpty and empty to be exclusive:
//
//     canAllocateButNotEmpty & empty == 0
//
// Instead of calling it canAllocate and making it inclusive:
//
//     canAllocate & empty == empty
//
// The latter is probably better. I'll leave it to a future bug to fix that, since breathing on
// this code leads to regressions for days, and it's not clear that making this change would
// improve perf since it would not change the collector's behavior, and either way the allocator
// has to look at both bitvectors.
// https://bugs.webkit.org/show_bug.cgi?id=162121

class MarkedAllocator {
    WTF_MAKE_NONCOPYABLE(MarkedAllocator);
    WTF_MAKE_FAST_ALLOCATED;
    
    friend class LLIntOffsetsExtractor;

public:
    static ptrdiff_t offsetOfFreeList();
    static ptrdiff_t offsetOfCellSize();

    MarkedAllocator(Heap*, size_t cellSize);
    void setSubspace(Subspace*);
    void lastChanceToFinalize();
    void prepareForAllocation();
    void stopAllocating();
    void resumeAllocating();
    void beginMarkingForFullCollection();
    void endMarking();
    void snapshotUnsweptForEdenCollection();
    void snapshotUnsweptForFullCollection();
    void sweep();
    void shrink();
    void assertNoUnswept();
    size_t cellSize() const { return m_cellSize; }
    const AllocatorAttributes& attributes() const { return m_attributes; }
    bool needsDestruction() const { return m_attributes.destruction == NeedsDestruction; }
    DestructionMode destruction() const { return m_attributes.destruction; }
    HeapCell::Kind cellKind() const { return m_attributes.cellKind; }
    void* allocate(GCDeferralContext*, AllocationFailureMode);
    Heap* heap() { return m_heap; }

    bool isFreeListedCell(const void* target) const;

    template<typename Functor> void forEachBlock(const Functor&);
    template<typename Functor> void forEachNotEmptyBlock(const Functor&);
    
    RefPtr<SharedTask<MarkedBlock::Handle*()>> parallelNotEmptyBlockSource();
    
    void addBlock(MarkedBlock::Handle*);
    void removeBlock(MarkedBlock::Handle*);

    bool isPagedOut(double deadline);
    
    static size_t blockSizeForBytes(size_t);
    
    Lock& bitvectorLock() { return m_bitvectorLock; }
   
#define MARKED_ALLOCATOR_BIT_ACCESSORS(lowerBitName, capitalBitName)     \
    bool is ## capitalBitName(const AbstractLocker&, size_t index) const { return m_ ## lowerBitName[index]; } \
    bool is ## capitalBitName(const AbstractLocker& locker, MarkedBlock::Handle* block) const { return is ## capitalBitName(locker, block->index()); } \
    void setIs ## capitalBitName(const AbstractLocker&, size_t index, bool value) { m_ ## lowerBitName[index] = value; } \
    void setIs ## capitalBitName(const AbstractLocker& locker, MarkedBlock::Handle* block, bool value) { setIs ## capitalBitName(locker, block->index(), value); }
    FOR_EACH_MARKED_ALLOCATOR_BIT(MARKED_ALLOCATOR_BIT_ACCESSORS)
#undef MARKED_ALLOCATOR_BIT_ACCESSORS

    template<typename Func>
    void forEachBitVector(const AbstractLocker&, const Func& func)
    {
#define MARKED_ALLOCATOR_BIT_CALLBACK(lowerBitName, capitalBitName) \
        func(m_ ## lowerBitName);
        FOR_EACH_MARKED_ALLOCATOR_BIT(MARKED_ALLOCATOR_BIT_CALLBACK);
#undef MARKED_ALLOCATOR_BIT_CALLBACK
    }
    
    template<typename Func>
    void forEachBitVectorWithName(const AbstractLocker&, const Func& func)
    {
#define MARKED_ALLOCATOR_BIT_CALLBACK(lowerBitName, capitalBitName) \
        func(m_ ## lowerBitName, #capitalBitName);
        FOR_EACH_MARKED_ALLOCATOR_BIT(MARKED_ALLOCATOR_BIT_CALLBACK);
#undef MARKED_ALLOCATOR_BIT_CALLBACK
    }
    
    MarkedAllocator* nextAllocator() const { return m_nextAllocator; }
    MarkedAllocator* nextAllocatorInSubspace() const { return m_nextAllocatorInSubspace; }
    MarkedAllocator* nextAllocatorInAlignedMemoryAllocator() const { return m_nextAllocatorInAlignedMemoryAllocator; }
    
    void setNextAllocator(MarkedAllocator* allocator) { m_nextAllocator = allocator; }
    void setNextAllocatorInSubspace(MarkedAllocator* allocator) { m_nextAllocatorInSubspace = allocator; }
    void setNextAllocatorInAlignedMemoryAllocator(MarkedAllocator* allocator) { m_nextAllocatorInAlignedMemoryAllocator = allocator; }
    
    MarkedBlock::Handle* findEmptyBlockToSteal();
    
    MarkedBlock::Handle* findBlockToSweep();
    
    Subspace* subspace() const { return m_subspace; }
    MarkedSpace& markedSpace() const;
    
    const FreeList& freeList() const { return m_freeList; }
    
    void dump(PrintStream&) const;
    void dumpBits(PrintStream& = WTF::dataFile());
    
private:
    friend class MarkedBlock;
    
    JS_EXPORT_PRIVATE void* allocateSlowCase(GCDeferralContext*, AllocationFailureMode failureMode);
    void didConsumeFreeList();
    void* tryAllocateWithoutCollecting();
    MarkedBlock::Handle* tryAllocateBlock();
    void* tryAllocateIn(MarkedBlock::Handle*);
    void* allocateIn(MarkedBlock::Handle*);
    ALWAYS_INLINE void doTestCollectionsIfNeeded(GCDeferralContext*);
    
    FreeList m_freeList;
    
    Vector<MarkedBlock::Handle*> m_blocks;
    Vector<unsigned> m_freeBlockIndices;

    // Mutator uses this to guard resizing the bitvectors. Those things in the GC that may run
    // concurrently to the mutator must lock this when accessing the bitvectors.
    Lock m_bitvectorLock;
#define MARKED_ALLOCATOR_BIT_DECLARATION(lowerBitName, capitalBitName) \
    FastBitVector m_ ## lowerBitName;
    FOR_EACH_MARKED_ALLOCATOR_BIT(MARKED_ALLOCATOR_BIT_DECLARATION)
#undef MARKED_ALLOCATOR_BIT_DECLARATION
    
    // After you do something to a block based on one of these cursors, you clear the bit in the
    // corresponding bitvector and leave the cursor where it was.
    size_t m_allocationCursor { 0 }; // Points to the next block that is a candidate for allocation.
    size_t m_emptyCursor { 0 }; // Points to the next block that is a candidate for empty allocation (allocating in empty blocks).
    size_t m_unsweptCursor { 0 }; // Points to the next block that is a candidate for incremental sweeping.
    
    MarkedBlock::Handle* m_currentBlock;
    MarkedBlock::Handle* m_lastActiveBlock;

    Lock m_lock;
    unsigned m_cellSize;
    AllocatorAttributes m_attributes;
    // FIXME: All of these should probably be references.
    // https://bugs.webkit.org/show_bug.cgi?id=166988
    Heap* m_heap { nullptr };
    Subspace* m_subspace { nullptr };
    MarkedAllocator* m_nextAllocator { nullptr };
    MarkedAllocator* m_nextAllocatorInSubspace { nullptr };
    MarkedAllocator* m_nextAllocatorInAlignedMemoryAllocator { nullptr };
};

inline ptrdiff_t MarkedAllocator::offsetOfFreeList()
{
    return OBJECT_OFFSETOF(MarkedAllocator, m_freeList);
}

inline ptrdiff_t MarkedAllocator::offsetOfCellSize()
{
    return OBJECT_OFFSETOF(MarkedAllocator, m_cellSize);
}

} // namespace JSC
