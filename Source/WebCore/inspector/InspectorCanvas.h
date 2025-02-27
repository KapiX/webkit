/*
 * Copyright (C) 2017 Apple Inc.  All rights reserved.
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

#include "CallTracerTypes.h"
#include <inspector/InspectorProtocolObjects.h>
#include <inspector/ScriptCallFrame.h>
#include <wtf/HashMap.h>
#include <wtf/Ref.h>
#include <wtf/RefPtr.h>
#include <wtf/Variant.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CanvasGradient;
class CanvasPattern;
class HTMLCanvasElement;
class HTMLImageElement;
class HTMLVideoElement;
class ImageBitmap;
class ImageData;
class InstrumentingAgents;

typedef String ErrorString;

class InspectorCanvas final : public RefCounted<InspectorCanvas> {
public:
    static Ref<InspectorCanvas> create(HTMLCanvasElement&, const String& cssCanvasName);

    const String& identifier() { return m_identifier; }
    HTMLCanvasElement& canvas() { return m_canvas; }
    const String& cssCanvasName() { return m_cssCanvasName; }

    void resetRecordingData();
    bool hasRecordingData() const;
    bool currentFrameHasData() const;
    void recordAction(const String&, Vector<RecordCanvasActionVariant>&& = { });

    RefPtr<Inspector::Protocol::Recording::InitialState>&& releaseInitialState();
    RefPtr<JSON::ArrayOf<Inspector::Protocol::Recording::Frame>>&& releaseFrames();
    RefPtr<JSON::ArrayOf<JSON::Value>>&& releaseData();

    void finalizeFrame();
    void markCurrentFrameIncomplete();

    const String& recordingName() const { return m_recordingName; }
    void setRecordingName(const String& name) { m_recordingName = name; }

    void setBufferLimit(long);
    bool hasBufferSpace() const;
    long bufferUsed() const { return m_bufferUsed; }

    bool singleFrame() const { return m_singleFrame; }
    void setSingleFrame(bool singleFrame) { m_singleFrame = singleFrame; }

    Ref<Inspector::Protocol::Canvas::Canvas> buildObjectForCanvas(InstrumentingAgents&, bool captureBacktrace);

    ~InspectorCanvas();

private:
    InspectorCanvas(HTMLCanvasElement&, const String& cssCanvasName);
    void appendActionSnapshotIfNeeded();
    String getCanvasContentAsDataURL();

    typedef Variant<
        CanvasGradient*,
        CanvasPattern*,
        HTMLCanvasElement*,
        HTMLImageElement*,
#if ENABLE(VIDEO)
        HTMLVideoElement*,
#endif
        ImageData*,
        ImageBitmap*,
        Inspector::ScriptCallFrame,
        String
    > DuplicateDataVariant;

    int indexForData(DuplicateDataVariant);
    RefPtr<Inspector::Protocol::Recording::InitialState> buildInitialState();
    RefPtr<JSON::ArrayOf<JSON::Value>> buildAction(const String&, Vector<RecordCanvasActionVariant>&& = { });
    RefPtr<JSON::ArrayOf<JSON::Value>> buildArrayForCanvasGradient(const CanvasGradient&);
    RefPtr<JSON::ArrayOf<JSON::Value>> buildArrayForCanvasPattern(const CanvasPattern&);
    RefPtr<JSON::ArrayOf<JSON::Value>> buildArrayForImageData(const ImageData&);
    RefPtr<JSON::ArrayOf<JSON::Value>> buildArrayForImageBitmap(const ImageBitmap&);

    String m_identifier;
    HTMLCanvasElement& m_canvas;
    String m_cssCanvasName;

    RefPtr<Inspector::Protocol::Recording::InitialState> m_initialState;
    RefPtr<JSON::ArrayOf<Inspector::Protocol::Recording::Frame>> m_frames;
    RefPtr<JSON::ArrayOf<JSON::Value>> m_currentActions;
    RefPtr<JSON::ArrayOf<JSON::Value>> m_actionNeedingSnapshot;
    RefPtr<JSON::ArrayOf<JSON::Value>> m_serializedDuplicateData;
    Vector<DuplicateDataVariant> m_indexedDuplicateData;

    String m_recordingName;
    double m_currentFrameStartTime { NAN };
    size_t m_bufferLimit { 100 * 1024 * 1024 };
    size_t m_bufferUsed { 0 };
    bool m_singleFrame { true };
};

} // namespace WebCore

