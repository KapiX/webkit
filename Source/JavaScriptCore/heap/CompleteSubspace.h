/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "Subspace.h"

namespace JSC {

class CompleteSubspace : public Subspace {
public:
    JS_EXPORT_PRIVATE CompleteSubspace(CString name, Heap&, HeapCellType*, AlignedMemoryAllocator*);
    JS_EXPORT_PRIVATE ~CompleteSubspace();

    // In some code paths, we need it to be a compile error to call the virtual version of one of
    // these functions. That's why we do final methods the old school way.
    
    MarkedAllocator* allocatorFor(size_t, AllocatorForMode) override;
    MarkedAllocator* allocatorForNonVirtual(size_t, AllocatorForMode);
    
    void* allocate(size_t, GCDeferralContext*, AllocationFailureMode) override;
    JS_EXPORT_PRIVATE void* allocateNonVirtual(size_t, GCDeferralContext*, AllocationFailureMode);
    
    static ptrdiff_t offsetOfAllocatorForSizeStep() { return OBJECT_OFFSETOF(CompleteSubspace, m_allocatorForSizeStep); }
    
    MarkedAllocator** allocatorForSizeStep() { return &m_allocatorForSizeStep[0]; }

private:
    MarkedAllocator* allocatorForSlow(size_t);
    
    // These slow paths are concerned with large allocations and allocator creation.
    void* allocateSlow(size_t, GCDeferralContext*, AllocationFailureMode);
    void* tryAllocateSlow(size_t, GCDeferralContext*);
    
    std::array<MarkedAllocator*, MarkedSpace::numSizeClasses> m_allocatorForSizeStep;
    Vector<std::unique_ptr<MarkedAllocator>> m_allocators;
};

ALWAYS_INLINE MarkedAllocator* CompleteSubspace::allocatorForNonVirtual(size_t size, AllocatorForMode mode)
{
    if (size <= MarkedSpace::largeCutoff) {
        MarkedAllocator* result = m_allocatorForSizeStep[MarkedSpace::sizeClassToIndex(size)];
        switch (mode) {
        case AllocatorForMode::MustAlreadyHaveAllocator:
            RELEASE_ASSERT(result);
            break;
        case AllocatorForMode::EnsureAllocator:
            if (!result)
                return allocatorForSlow(size);
            break;
        case AllocatorForMode::AllocatorIfExists:
            break;
        }
        return result;
    }
    RELEASE_ASSERT(mode != AllocatorForMode::MustAlreadyHaveAllocator);
    return nullptr;
}

} // namespace JSC

