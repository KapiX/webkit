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

#include "FreeListInlines.h"
#include "MarkedAllocator.h"

namespace JSC {

inline bool MarkedAllocator::isFreeListedCell(const void* target) const
{
    return m_freeList.contains(bitwise_cast<HeapCell*>(target));
}

ALWAYS_INLINE void* MarkedAllocator::allocate(GCDeferralContext* deferralContext, AllocationFailureMode failureMode)
{
    return m_freeList.allocate(
        [&] () -> HeapCell* {
            sanitizeStackForVM(heap()->vm());
            return static_cast<HeapCell*>(allocateSlowCase(deferralContext, failureMode));
        });
}

template <typename Functor> inline void MarkedAllocator::forEachBlock(const Functor& functor)
{
    m_live.forEachSetBit(
        [&] (size_t index) {
            functor(m_blocks[index]);
        });
}

template <typename Functor> inline void MarkedAllocator::forEachNotEmptyBlock(const Functor& functor)
{
    m_markingNotEmpty.forEachSetBit(
        [&] (size_t index) {
            functor(m_blocks[index]);
        });
}

} // namespace JSC

