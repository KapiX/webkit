/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2015 Igalia S.L.
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
#include "BitmapTexturePool.h"

#if USE(TEXTURE_MAPPER_GL)
#include "BitmapTextureGL.h"
#else
#include "BitmapTextureImageBuffer.h"
#endif

namespace WebCore {

static const double releaseUnusedSecondsTolerance = 3;
static const Seconds releaseUnusedTexturesTimerInterval { 500_ms };

#if USE(TEXTURE_MAPPER_GL)
BitmapTexturePool::BitmapTexturePool(const TextureMapperContextAttributes& contextAttributes)
    : m_contextAttributes(contextAttributes)
    , m_releaseUnusedTexturesTimer(RunLoop::current(), this, &BitmapTexturePool::releaseUnusedTexturesTimerFired)
{
}
#else
BitmapTexturePool::BitmapTexturePool()
    : m_releaseUnusedTexturesTimer(RunLoop::current(), this, &BitmapTexturePool::releaseUnusedTexturesTimerFired)
{
}
#endif

RefPtr<BitmapTexture> BitmapTexturePool::acquireTexture(const IntSize& size, const BitmapTexture::Flags flags)
{
    Vector<Entry>& list = flags & BitmapTexture::FBOAttachment ? m_attachmentTextures : m_textures;

    Entry* selectedEntry = std::find_if(list.begin(), list.end(),
        [&size](Entry& entry) { return entry.m_texture->refCount() == 1 && entry.m_texture->size() == size; });

    if (selectedEntry == list.end()) {
        list.append(Entry(createTexture(flags)));
        selectedEntry = &list.last();
    }

    scheduleReleaseUnusedTextures();
    selectedEntry->markIsInUse();
    return selectedEntry->m_texture.copyRef();
}

void BitmapTexturePool::scheduleReleaseUnusedTextures()
{
    if (m_releaseUnusedTexturesTimer.isActive())
        return;

    m_releaseUnusedTexturesTimer.startOneShot(releaseUnusedTexturesTimerInterval);
}

void BitmapTexturePool::releaseUnusedTexturesTimerFired()
{
    // Delete entries, which have been unused in releaseUnusedSecondsTolerance.
    double minUsedTime = monotonicallyIncreasingTime() - releaseUnusedSecondsTolerance;

    if (!m_textures.isEmpty()) {
        m_textures.removeAllMatching([&minUsedTime](const Entry& entry) {
            return entry.canBeReleased(minUsedTime);
        });
    }

    if (!m_attachmentTextures.isEmpty()) {
        m_attachmentTextures.removeAllMatching([&minUsedTime](const Entry& entry) {
            return entry.canBeReleased(minUsedTime);
        });
    }

    if (!m_textures.isEmpty() || !m_attachmentTextures.isEmpty())
        scheduleReleaseUnusedTextures();
}

RefPtr<BitmapTexture> BitmapTexturePool::createTexture(const BitmapTexture::Flags flags)
{
#if USE(TEXTURE_MAPPER_GL)
    return BitmapTextureGL::create(m_contextAttributes, flags);
#else
    return BitmapTextureImageBuffer::create();
#endif
}

} // namespace WebCore
