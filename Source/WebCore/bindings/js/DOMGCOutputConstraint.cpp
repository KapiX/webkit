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

#include "config.h"
#include "DOMGCOutputConstraint.h"

#include "WebCoreJSClientData.h"
#include <heap/HeapInlines.h>
#include <heap/MarkedAllocatorInlines.h>
#include <heap/MarkedBlockInlines.h>
#include <heap/SubspaceInlines.h>
#include <runtime/VM.h>

namespace WebCore {

using namespace JSC;

DOMGCOutputConstraint::DOMGCOutputConstraint(VM& vm, JSVMClientData& clientData)
    : MarkingConstraint("Domo", "DOM Output", ConstraintVolatility::SeldomGreyed, ConstraintConcurrency::Concurrent, ConstraintParallelism::Parallel)
    , m_vm(vm)
    , m_clientData(clientData)
    , m_lastExecutionVersion(vm.heap.mutatorExecutionVersion())
{
}

DOMGCOutputConstraint::~DOMGCOutputConstraint()
{
}

ConstraintParallelism DOMGCOutputConstraint::executeImpl(SlotVisitor&)
{
    Heap& heap = m_vm.heap;
    
    if (heap.mutatorExecutionVersion() == m_lastExecutionVersion)
        return ConstraintParallelism::Sequential;
    
    m_lastExecutionVersion = heap.mutatorExecutionVersion();
    
    RELEASE_ASSERT(m_tasks.isEmpty());
    m_clientData.forEachOutputConstraintSpace(
        [&] (Subspace& subspace) {
            auto func = [] (SlotVisitor& visitor, HeapCell* heapCell, HeapCell::Kind) {
                JSCell* cell = static_cast<JSCell*>(heapCell);
                cell->methodTable(visitor.vm())->visitOutputConstraints(cell, visitor);
            };
            
            m_tasks.append(subspace.forEachMarkedCellInParallel(func));
        });
    
    return ConstraintParallelism::Parallel;
}
        
void DOMGCOutputConstraint::doParallelWorkImpl(SlotVisitor& visitor)
{
    for (auto& task : m_tasks)
        task->run(visitor);
}
        
void DOMGCOutputConstraint::finishParallelWorkImpl(SlotVisitor&)
{
    m_tasks.clear();
}

} // namespace WebCore

