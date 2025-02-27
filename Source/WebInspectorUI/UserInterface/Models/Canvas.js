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

WI.Canvas = class Canvas extends WI.Object
{
    constructor(identifier, contextType, frame, {domNode, cssCanvasName, contextAttributes, memoryCost, backtrace} = {})
    {
        super();

        console.assert(identifier);
        console.assert(contextType);
        console.assert(frame instanceof WI.Frame);

        this._identifier = identifier;
        this._contextType = contextType;
        this._frame = frame;
        this._domNode = domNode || null;
        this._cssCanvasName = cssCanvasName || "";
        this._contextAttributes = contextAttributes || {};
        this._extensions = new Set;
        this._memoryCost = memoryCost || NaN;
        this._backtrace = backtrace || [];

        this._cssCanvasClientNodes = null;
        this._shaderProgramCollection = new WI.ShaderProgramCollection;
        this._recordingCollection = new WI.RecordingCollection;

        this._nextShaderProgramDisplayNumber = 1;

        this._requestNodePromise = null;
    }

    // Static

    static fromPayload(payload)
    {
        let contextType = null;
        switch (payload.contextType) {
        case CanvasAgent.ContextType.Canvas2D:
            contextType = WI.Canvas.ContextType.Canvas2D;
            break;
        case CanvasAgent.ContextType.WebGL:
            contextType = WI.Canvas.ContextType.WebGL;
            break;
        case CanvasAgent.ContextType.WebGL2:
            contextType = WI.Canvas.ContextType.WebGL2;
            break;
        case CanvasAgent.ContextType.WebGPU:
            contextType = WI.Canvas.ContextType.WebGPU;
            break;
        default:
            console.error("Invalid canvas context type", payload.contextType);
        }

        let frame = WI.frameResourceManager.frameForIdentifier(payload.frameId);
        return new WI.Canvas(payload.canvasId, contextType, frame, {
            domNode: payload.nodeId ? WI.domTreeManager.nodeForId(payload.nodeId) : null,
            cssCanvasName: payload.cssCanvasName,
            contextAttributes: payload.contextAttributes,
            memoryCost: payload.memoryCost,
            backtrace: Array.isArray(payload.backtrace) ? payload.backtrace.map((item) => WI.CallFrame.fromPayload(WI.mainTarget, item)) : [],
        });
    }

    static displayNameForContextType(contextType)
    {
        switch (contextType) {
        case WI.Canvas.ContextType.Canvas2D:
            return WI.UIString("2D");
        case WI.Canvas.ContextType.WebGL:
            return WI.unlocalizedString("WebGL");
        case WI.Canvas.ContextType.WebGL2:
            return WI.unlocalizedString("WebGL2");
        case WI.Canvas.ContextType.WebGPU:
            return WI.unlocalizedString("WebGPU");
        default:
            console.error("Invalid canvas context type", contextType);
        }
    }

    static resetUniqueDisplayNameNumbers()
    {
        WI.Canvas._nextUniqueDisplayNameNumber = 1;
    }

    // Public

    get identifier() { return this._identifier; }
    get contextType() { return this._contextType; }
    get frame() { return this._frame; }
    get cssCanvasName() { return this._cssCanvasName; }
    get contextAttributes() { return this._contextAttributes; }
    get extensions() { return this._extensions; }
    get backtrace() { return this._backtrace; }
    get shaderProgramCollection() { return this._shaderProgramCollection; }
    get recordingCollection() { return this._recordingCollection; }

    get isRecording()
    {
        return WI.canvasManager.recordingCanvas === this;
    }

    get memoryCost()
    {
        return this._memoryCost;
    }

    set memoryCost(memoryCost)
    {
        if (memoryCost === this._memoryCost)
            return;

        this._memoryCost = memoryCost;

        this.dispatchEventToListeners(WI.Canvas.Event.MemoryChanged);
    }

    get displayName()
    {
        if (this._cssCanvasName)
            return WI.UIString("CSS canvas “%s”").format(this._cssCanvasName);

        if (this._domNode) {
            let idSelector = this._domNode.escapedIdSelector;
            if (idSelector)
                return WI.UIString("Canvas %s").format(idSelector);
        }

        if (!this._uniqueDisplayNameNumber)
            this._uniqueDisplayNameNumber = this.constructor._nextUniqueDisplayNameNumber++;
        return WI.UIString("Canvas %d").format(this._uniqueDisplayNameNumber);
    }

    requestNode()
    {
        if (!this._requestNodePromise) {
            this._requestNodePromise = new Promise((resolve, reject) => {
                WI.domTreeManager.ensureDocument();

                CanvasAgent.requestNode(this._identifier).then((result) => {
                    this._domNode = WI.domTreeManager.nodeForId(result.nodeId);
                    if (!this._domNode) {
                        reject(`No DOM node for identifier: ${result.nodeId}.`);
                        return;
                    }
                    resolve(this._domNode);
                }).catch(reject);
            });
        }

        return this._requestNodePromise;
    }

    requestContent()
    {
        return CanvasAgent.requestContent(this._identifier).then((result) => result.content).catch((error) => console.error(error));
    }

    requestCSSCanvasClientNodes(callback)
    {
        if (!this._cssCanvasName) {
            callback([]);
            return;
        }

        if (this._cssCanvasClientNodes) {
            callback(this._cssCanvasClientNodes);
            return;
        }

        WI.domTreeManager.ensureDocument();

        CanvasAgent.requestCSSCanvasClientNodes(this._identifier, (error, clientNodeIds) => {
            if (error) {
                callback([]);
                return;
            }

            clientNodeIds = Array.isArray(clientNodeIds) ? clientNodeIds : [];
            this._cssCanvasClientNodes = clientNodeIds.map((clientNodeId) => WI.domTreeManager.nodeForId(clientNodeId));
            callback(this._cssCanvasClientNodes);
        });
    }

    requestSize()
    {
        function calculateSize(domNode) {
            function getAttributeValue(name) {
                let value = Number(domNode.getAttribute(name));
                if (!Number.isInteger(value) || value < 0)
                    return NaN;
                return value;
            }

            return {
                width: getAttributeValue("width"),
                height: getAttributeValue("height")
            };
        }

        function getPropertyValue(remoteObject, name) {
            return new Promise((resolve, reject) => {
                remoteObject.getProperty(name, (error, result) => {
                    if (error) {
                        reject(error);
                        return;
                    }
                    resolve(result);
                });
            });
        }

        return this.requestNode().then((domNode) => {
            let size = calculateSize(domNode);
            if (!isNaN(size.width) && !isNaN(size.height))
                return size;

            // Since the "width" and "height" properties of canvas elements are more than just
            // attributes, we need to invoke the getter for each to get the actual value.
            //  - https://html.spec.whatwg.org/multipage/canvas.html#attr-canvas-width
            //  - https://html.spec.whatwg.org/multipage/canvas.html#attr-canvas-height
            let remoteObject = null;
            return WI.RemoteObject.resolveNode(domNode).then((object) => {
                remoteObject = object;
                return Promise.all([getPropertyValue(object, "width"), getPropertyValue(object, "height")]);
            }).then((values) => {
                let width = values[0].value;
                let height = values[1].value;
                values[0].release();
                values[1].release();
                remoteObject.release();
                return {width, height};
            });
        });
    }

    saveIdentityToCookie(cookie)
    {
        cookie[WI.Canvas.FrameURLCookieKey] = this._frame.url.hash;

        if (this._cssCanvasName)
            cookie[WI.Canvas.CSSCanvasNameCookieKey] = this._cssCanvasName;
        else if (this._domNode)
            cookie[WI.Canvas.NodePathCookieKey] = this._domNode.path;

    }

    enableExtension(extension)
    {
        // Called from WI.CanvasManager.

        this._extensions.add(extension);

        this.dispatchEventToListeners(WI.Canvas.Event.ExtensionEnabled, {extension});
    }

    cssCanvasClientNodesChanged()
    {
        // Called from WI.CanvasManager.

        if (!this._cssCanvasName)
            return;

        this._cssCanvasClientNodes = null;

        this.dispatchEventToListeners(WI.Canvas.Event.CSSCanvasClientNodesChanged);
    }

    nextShaderProgramDisplayNumber()
    {
        // Called from WI.ShaderProgram.

        return this._nextShaderProgramDisplayNumber++;
    }
};

WI.Canvas._nextUniqueDisplayNameNumber = 1;

WI.Canvas.FrameURLCookieKey = "canvas-frame-url";
WI.Canvas.CSSCanvasNameCookieKey = "canvas-css-canvas-name";

WI.Canvas.ContextType = {
    Canvas2D: "canvas-2d",
    WebGL: "webgl",
    WebGL2: "webgl2",
    WebGPU: "webgpu",
};

WI.Canvas.Event = {
    MemoryChanged: "canvas-memory-changed",
    ExtensionEnabled: "canvas-extension-enabled",
    CSSCanvasClientNodesChanged: "canvas-css-canvas-client-nodes-changed",
};
