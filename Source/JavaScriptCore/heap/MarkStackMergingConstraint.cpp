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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MarkStackMergingConstraint.h"

namespace JSC {

MarkStackMergingConstraint::MarkStackMergingConstraint(Heap& heap)
    : MarkingConstraint("Msm", "Mark Stack Merging", ConstraintVolatility::GreyedByExecution)
    , m_heap(heap)
{
}

MarkStackMergingConstraint::~MarkStackMergingConstraint()
{
}

double MarkStackMergingConstraint::quickWorkEstimate(SlotVisitor&)
{
    return m_heap.m_mutatorMarkStack->size() + m_heap.m_raceMarkStack->size();
}

void MarkStackMergingConstraint::prepareToExecuteImpl(const AbstractLocker&, SlotVisitor& visitor)
{
    // Logging the work here ensures that the constraint solver knows that it doesn't need to produce
    // anymore work.
    size_t size = m_heap.m_mutatorMarkStack->size() + m_heap.m_raceMarkStack->size();
    visitor.addToVisitCount(size);
    
    if (Options::logGC())
        dataLog("(", size, ")");
}

ConstraintParallelism MarkStackMergingConstraint::executeImpl(SlotVisitor& visitor)
{
    m_heap.m_mutatorMarkStack->transferTo(visitor.mutatorMarkStack());
    m_heap.m_raceMarkStack->transferTo(visitor.mutatorMarkStack());
    return ConstraintParallelism::Sequential;
}

} // namespace JSC

