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

#include "MarkedAllocator.h"
#include "Subspace.h"

namespace JSC {

class IsoAlignedMemoryAllocator;

class IsoSubspace : public Subspace {
public:
    JS_EXPORT_PRIVATE IsoSubspace(CString name, Heap&, HeapCellType*, size_t size);
    JS_EXPORT_PRIVATE ~IsoSubspace();

    size_t size() const { return m_size; }

    MarkedAllocator* allocatorFor(size_t, AllocatorForMode) override;
    MarkedAllocator* allocatorForNonVirtual(size_t size, AllocatorForMode);

    void* allocate(size_t, GCDeferralContext*, AllocationFailureMode) override;
    JS_EXPORT_PRIVATE void* allocateNonVirtual(size_t size, GCDeferralContext* deferralContext, AllocationFailureMode failureMode);

private:
    size_t m_size;
    MarkedAllocator m_allocator;
    std::unique_ptr<IsoAlignedMemoryAllocator> m_isoAlignedMemoryAllocator;
};

inline MarkedAllocator* IsoSubspace::allocatorForNonVirtual(size_t size, AllocatorForMode)
{
    RELEASE_ASSERT(size == this->size());
    return &m_allocator;
}

#define ISO_SUBSPACE_INIT(heap, heapCellType, type) ("Isolated " #type " Space", (heap), (heapCellType), sizeof(type))

} // namespace JSC

