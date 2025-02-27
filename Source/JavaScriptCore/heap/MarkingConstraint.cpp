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
#include "MarkingConstraint.h"

#include "JSCInlines.h"
#include "VisitCounter.h"

namespace JSC {

static constexpr bool verboseMarkingConstraint = false;

MarkingConstraint::MarkingConstraint(CString abbreviatedName, CString name, ConstraintVolatility volatility, ConstraintConcurrency concurrency, ConstraintParallelism parallelism)
    : m_abbreviatedName(abbreviatedName)
    , m_name(WTFMove(name))
    , m_volatility(volatility)
    , m_concurrency(concurrency)
    , m_parallelism(parallelism)
{
}

MarkingConstraint::~MarkingConstraint()
{
}

void MarkingConstraint::resetStats()
{
    m_lastVisitCount = 0;
}

ConstraintParallelism MarkingConstraint::execute(SlotVisitor& visitor)
{
    VisitCounter visitCounter(visitor);
    ConstraintParallelism result = executeImpl(visitor);
    m_lastVisitCount += visitCounter.visitCount();
    if (verboseMarkingConstraint && visitCounter.visitCount())
        dataLog("(", abbreviatedName(), " visited ", visitCounter.visitCount(), " in execute)");
    if (result == ConstraintParallelism::Parallel) {
        // It's illegal to produce parallel work if you haven't advertised it upfront because the solver
        // has optimizations for constraints that promise to never produce parallel work.
        RELEASE_ASSERT(m_parallelism == ConstraintParallelism::Parallel);
    }
    return result;
}

double MarkingConstraint::quickWorkEstimate(SlotVisitor&)
{
    return 0;
}

double MarkingConstraint::workEstimate(SlotVisitor& visitor)
{
    return lastVisitCount() + quickWorkEstimate(visitor);
}

void MarkingConstraint::prepareToExecute(const AbstractLocker& constraintSolvingLocker, SlotVisitor& visitor)
{
    if (Options::logGC())
        dataLog(abbreviatedName());
    VisitCounter visitCounter(visitor);
    prepareToExecuteImpl(constraintSolvingLocker, visitor);
    m_lastVisitCount = visitCounter.visitCount();
    if (verboseMarkingConstraint && visitCounter.visitCount())
        dataLog("(", abbreviatedName(), " visited ", visitCounter.visitCount(), " in prepareToExecute)");
}

void MarkingConstraint::doParallelWork(SlotVisitor& visitor)
{
    VisitCounter visitCounter(visitor);
    doParallelWorkImpl(visitor);
    if (verboseMarkingConstraint && visitCounter.visitCount())
        dataLog("(", abbreviatedName(), " visited ", visitCounter.visitCount(), " in doParallelWork)");
    {
        auto locker = holdLock(m_lock);
        m_lastVisitCount += visitCounter.visitCount();
    }
}

void MarkingConstraint::finishParallelWork(SlotVisitor& visitor)
{
    VisitCounter visitCounter(visitor);
    finishParallelWorkImpl(visitor);
    m_lastVisitCount += visitCounter.visitCount();
    if (verboseMarkingConstraint && visitCounter.visitCount())
        dataLog("(", abbreviatedName(), " visited ", visitCounter.visitCount(), " in finishParallelWork)");
}

void MarkingConstraint::prepareToExecuteImpl(const AbstractLocker&, SlotVisitor&)
{
}

void MarkingConstraint::doParallelWorkImpl(SlotVisitor&)
{
    UNREACHABLE_FOR_PLATFORM();
}

void MarkingConstraint::finishParallelWorkImpl(SlotVisitor&)
{
}

} // namespace JSC

