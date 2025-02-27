/*
 * Copyright (C) 2016-2017 Apple Inc. All rights reserved.
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
#include "RenderTreeUpdater.h"

#include "AXObjectCache.h"
#include "CSSAnimationController.h"
#include "ComposedTreeAncestorIterator.h"
#include "ComposedTreeIterator.h"
#include "Document.h"
#include "Element.h"
#include "HTMLParserIdioms.h"
#include "HTMLSlotElement.h"
#include "InspectorInstrumentation.h"
#include "NodeRenderStyle.h"
#include "PseudoElement.h"
#include "RenderDescendantIterator.h"
#include "RenderFullScreen.h"
#include "RenderInline.h"
#include "RenderListItem.h"
#include "RenderTreeUpdaterFirstLetter.h"
#include "RenderTreeUpdaterGeneratedContent.h"
#include "RenderTreeUpdaterListItem.h"
#include "RenderTreeUpdaterMultiColumn.h"
#include "StyleResolver.h"
#include "StyleTreeResolver.h"
#include <wtf/SystemTracing.h>

#if PLATFORM(IOS)
#include "WKContentObservation.h"
#include "WKContentObservationInternal.h"
#endif

namespace WebCore {

#if PLATFORM(IOS)
class CheckForVisibilityChange {
public:
    CheckForVisibilityChange(const Element&);
    ~CheckForVisibilityChange();

private:
    const Element& m_element;
    EDisplay m_previousDisplay;
    EVisibility m_previousVisibility;
    EVisibility m_previousImplicitVisibility;
};
#endif // PLATFORM(IOS)

RenderTreeUpdater::Parent::Parent(ContainerNode& root)
    : element(is<Document>(root) ? nullptr : downcast<Element>(&root))
    , renderTreePosition(RenderTreePosition(*root.renderer()))
{
}

RenderTreeUpdater::Parent::Parent(Element& element, const Style::ElementUpdates* updates)
    : element(&element)
    , updates(updates)
    , renderTreePosition(element.renderer() ? std::make_optional(RenderTreePosition(*element.renderer())) : std::nullopt)
{
}

RenderTreeUpdater::RenderTreeUpdater(Document& document)
    : m_document(document)
    , m_generatedContent(std::make_unique<GeneratedContent>(*this))
{
}

RenderTreeUpdater::~RenderTreeUpdater() = default;

static ContainerNode* findRenderingRoot(ContainerNode& node)
{
    if (node.renderer())
        return &node;
    for (auto& ancestor : composedTreeAncestors(node)) {
        if (ancestor.renderer())
            return &ancestor;
        if (!ancestor.hasDisplayContents())
            return nullptr;
    }
    return &node.document();
}

static ListHashSet<ContainerNode*> findRenderingRoots(const Style::Update& update)
{
    ListHashSet<ContainerNode*> renderingRoots;
    for (auto* root : update.roots()) {
        auto* renderingRoot = findRenderingRoot(*root);
        if (!renderingRoot)
            continue;
        renderingRoots.add(renderingRoot);
    }
    return renderingRoots;
}

void RenderTreeUpdater::commit(std::unique_ptr<const Style::Update> styleUpdate)
{
    ASSERT(&m_document == &styleUpdate->document());

    if (!m_document.shouldCreateRenderers() || !m_document.renderView())
        return;
    
    TraceScope scope(RenderTreeBuildStart, RenderTreeBuildEnd);

    Style::PostResolutionCallbackDisabler callbackDisabler(m_document);

    m_styleUpdate = WTFMove(styleUpdate);

    for (auto* root : findRenderingRoots(*m_styleUpdate))
        updateRenderTree(*root);

    generatedContent().updateRemainingQuotes();

    MultiColumn::update(renderView());

    m_styleUpdate = nullptr;
}

static bool shouldCreateRenderer(const Element& element, const RenderElement& parentRenderer)
{
    if (!parentRenderer.canHaveChildren() && !(element.isPseudoElement() && parentRenderer.canHaveGeneratedChildren()))
        return false;
    if (parentRenderer.element() && !parentRenderer.element()->childShouldCreateRenderer(element))
        return false;
    return true;
}

void RenderTreeUpdater::updateRenderTree(ContainerNode& root)
{
    ASSERT(root.renderer());
    ASSERT(m_parentStack.isEmpty());

    m_parentStack.append(Parent(root));

    auto descendants = composedTreeDescendants(root);
    auto it = descendants.begin();
    auto end = descendants.end();

    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=156172
    it.dropAssertions();

    while (it != end) {
        popParentsToDepth(it.depth());

        auto& node = *it;

        if (auto* renderer = node.renderer())
            renderTreePosition().invalidateNextSibling(*renderer);
        else if (is<Element>(node) && downcast<Element>(node).hasDisplayContents())
            renderTreePosition().invalidateNextSibling();

        if (is<Text>(node)) {
            auto& text = downcast<Text>(node);
            auto* textUpdate = m_styleUpdate->textUpdate(text);
            bool didCreateParent = parent().updates && parent().updates->update.change == Style::Detach;
            bool mayNeedUpdateWhitespaceOnlyRenderer = renderingParent().didCreateOrDestroyChildRenderer && text.data().isAllSpecialCharacters<isHTMLSpace>();
            if (didCreateParent || textUpdate || mayNeedUpdateWhitespaceOnlyRenderer)
                updateTextRenderer(text, textUpdate);

            storePreviousRenderer(text);
            it.traverseNextSkippingChildren();
            continue;
        }

        auto& element = downcast<Element>(node);

        auto* elementUpdates = m_styleUpdate->elementUpdates(element);

        // We hop through display: contents elements in findRenderingRoot, so
        // there may be other updates down the tree.
        if (!elementUpdates && !element.hasDisplayContents()) {
            storePreviousRenderer(element);
            it.traverseNextSkippingChildren();
            continue;
        }

        if (elementUpdates)
            updateElementRenderer(element, elementUpdates->update);

        storePreviousRenderer(element);

        bool mayHaveRenderedDescendants = element.renderer() || (element.hasDisplayContents() && shouldCreateRenderer(element, renderTreePosition().parent()));
        if (!mayHaveRenderedDescendants) {
            it.traverseNextSkippingChildren();
            continue;
        }

        pushParent(element, elementUpdates);

        it.traverseNext();
    }

    popParentsToDepth(0);
}

auto RenderTreeUpdater::renderingParent() -> Parent&
{
    for (unsigned i = m_parentStack.size(); i--;) {
        if (m_parentStack[i].renderTreePosition)
            return m_parentStack[i];
    }
    ASSERT_NOT_REACHED();
    return m_parentStack.last();
}

RenderTreePosition& RenderTreeUpdater::renderTreePosition()
{
    return *renderingParent().renderTreePosition;
}

void RenderTreeUpdater::pushParent(Element& element, const Style::ElementUpdates* updates)
{
    m_parentStack.append(Parent(element, updates));

    updateBeforeDescendants(element, updates);
}

void RenderTreeUpdater::popParent()
{
    auto& parent = m_parentStack.last();
    if (parent.element)
        updateAfterDescendants(*parent.element, parent.updates);

    m_parentStack.removeLast();
}

void RenderTreeUpdater::popParentsToDepth(unsigned depth)
{
    ASSERT(m_parentStack.size() >= depth);

    while (m_parentStack.size() > depth)
        popParent();
}

void RenderTreeUpdater::updateBeforeDescendants(Element& element, const Style::ElementUpdates* updates)
{
    if (updates)
        generatedContent().updatePseudoElement(element, updates->beforePseudoElementUpdate, BEFORE);
}

void RenderTreeUpdater::updateAfterDescendants(Element& element, const Style::ElementUpdates* updates)
{
    if (updates)
        generatedContent().updatePseudoElement(element, updates->afterPseudoElementUpdate, AFTER);

    auto* renderer = element.renderer();
    if (!renderer)
        return;

    // These functions do render tree mutations that require descendant renderers.
    if (is<RenderBlock>(*renderer))
        FirstLetter::update(downcast<RenderBlock>(*renderer));
    if (is<RenderListItem>(*renderer))
        ListItem::updateMarker(downcast<RenderListItem>(*renderer));
    if (is<RenderBlockFlow>(*renderer))
        MultiColumn::update(downcast<RenderBlockFlow>(*renderer));

    if (element.hasCustomStyleResolveCallbacks() && updates && updates->update.change == Style::Detach)
        element.didAttachRenderers();
}

static bool pseudoStyleCacheIsInvalid(RenderElement* renderer, RenderStyle* newStyle)
{
    const RenderStyle& currentStyle = renderer->style();

    const PseudoStyleCache* pseudoStyleCache = currentStyle.cachedPseudoStyles();
    if (!pseudoStyleCache)
        return false;

    for (auto& cache : *pseudoStyleCache) {
        PseudoId pseudoId = cache->styleType();
        std::unique_ptr<RenderStyle> newPseudoStyle = renderer->getUncachedPseudoStyle(PseudoStyleRequest(pseudoId), newStyle, newStyle);
        if (!newPseudoStyle)
            return true;
        if (*newPseudoStyle != *cache) {
            newStyle->addCachedPseudoStyle(WTFMove(newPseudoStyle));
            return true;
        }
    }
    return false;
}

void RenderTreeUpdater::updateElementRenderer(Element& element, const Style::ElementUpdate& update)
{
#if PLATFORM(IOS)
    CheckForVisibilityChange checkForVisibilityChange(element);
#endif

    bool shouldTearDownRenderers = update.change == Style::Detach && (element.renderer() || element.hasDisplayContents());
    if (shouldTearDownRenderers) {
        if (!element.renderer()) {
            // We may be tearing down a descendant renderer cached in renderTreePosition.
            renderTreePosition().invalidateNextSibling();
        }

        // display:none cancels animations.
        auto teardownType = update.style->display() == NONE ? TeardownType::RendererUpdateCancelingAnimations : TeardownType::RendererUpdate;
        tearDownRenderers(element, teardownType);

        renderingParent().didCreateOrDestroyChildRenderer = true;
    }

    bool hasDisplayContents = update.style->display() == CONTENTS;
    if (hasDisplayContents)
        element.storeDisplayContentsStyle(RenderStyle::clonePtr(*update.style));
    else
        element.resetComputedStyle();

    bool shouldCreateNewRenderer = !element.renderer() && !hasDisplayContents;
    if (shouldCreateNewRenderer) {
        if (element.hasCustomStyleResolveCallbacks())
            element.willAttachRenderers();
        createRenderer(element, RenderStyle::clone(*update.style));

        renderingParent().didCreateOrDestroyChildRenderer = true;
        return;
    }

    if (!element.renderer())
        return;
    auto& renderer = *element.renderer();

    if (update.recompositeLayer) {
        renderer.setStyle(RenderStyle::clone(*update.style), StyleDifferenceRecompositeLayer);
        return;
    }

    if (update.change == Style::NoChange) {
        if (pseudoStyleCacheIsInvalid(&renderer, update.style.get())) {
            renderer.setStyle(RenderStyle::clone(*update.style), StyleDifferenceEqual);
            return;
        }
        return;
    }

    renderer.setStyle(RenderStyle::clone(*update.style), StyleDifferenceEqual);
}

void RenderTreeUpdater::createRenderer(Element& element, RenderStyle&& style)
{
    auto computeInsertionPosition = [this, &element] () {
        renderTreePosition().computeNextSibling(element);
        return renderTreePosition();
    };
    
    if (!shouldCreateRenderer(element, renderTreePosition().parent()))
        return;

    if (!element.rendererIsNeeded(style))
        return;

    RenderTreePosition insertionPosition = computeInsertionPosition();
    auto newRenderer = element.createElementRenderer(WTFMove(style), insertionPosition);
    if (!newRenderer)
        return;
    if (!insertionPosition.canInsert(*newRenderer))
        return;

    element.setRenderer(newRenderer.get());

    newRenderer->initializeStyle();

#if ENABLE(FULLSCREEN_API)
    if (m_document.webkitIsFullScreen() && m_document.webkitCurrentFullScreenElement() == &element) {
        newRenderer = RenderFullScreen::wrapNewRenderer(WTFMove(newRenderer), insertionPosition.parent(), m_document);
        if (!newRenderer)
            return;
    }
#endif

    insertionPosition.insert(WTFMove(newRenderer));

    if (AXObjectCache* cache = m_document.axObjectCache())
        cache->updateCacheAfterNodeIsAttached(&element);
}

bool RenderTreeUpdater::textRendererIsNeeded(const Text& textNode)
{
    auto& renderingParent = this->renderingParent();
    auto& parentRenderer = renderingParent.renderTreePosition->parent();
    if (!parentRenderer.canHaveChildren())
        return false;
    if (parentRenderer.element() && !parentRenderer.element()->childShouldCreateRenderer(textNode))
        return false;
    if (textNode.isEditingText())
        return true;
    if (!textNode.length())
        return false;
    if (!textNode.data().isAllSpecialCharacters<isHTMLSpace>())
        return true;
    if (is<RenderText>(renderingParent.previousChildRenderer))
        return true;
    // This text node has nothing but white space. We may still need a renderer in some cases.
    if (parentRenderer.isTable() || parentRenderer.isTableRow() || parentRenderer.isTableSection() || parentRenderer.isRenderTableCol() || parentRenderer.isFrameSet())
        return false;
    if (parentRenderer.isFlexibleBox() && !parentRenderer.isRenderButton())
        return false;
    if (parentRenderer.style().preserveNewline()) // pre/pre-wrap/pre-line always make renderers.
        return true;

    auto* previousRenderer = renderingParent.previousChildRenderer;
    if (previousRenderer && previousRenderer->isBR()) // <span><br/> <br/></span>
        return false;

    if (parentRenderer.isRenderInline()) {
        // <span><div/> <div/></span>
        if (previousRenderer && !previousRenderer->isInline())
            return false;
    } else {
        if (parentRenderer.isRenderBlock() && !parentRenderer.childrenInline() && (!previousRenderer || !previousRenderer->isInline()))
            return false;
        
        RenderObject* first = parentRenderer.firstChild();
        while (first && first->isFloatingOrOutOfFlowPositioned())
            first = first->nextSibling();
        RenderObject* nextRenderer = textNode.renderer() ? textNode.renderer() :  renderTreePosition().nextSiblingRenderer(textNode);
        if (!first || nextRenderer == first) {
            // Whitespace at the start of a block just goes away. Don't even make a render object for this text.
            return false;
        }
    }
    return true;
}

static void createTextRenderer(Text& textNode, RenderTreePosition& renderTreePosition, const Style::TextUpdate* textUpdate)
{
    ASSERT(!textNode.renderer());

    auto textRenderer = textNode.createTextRenderer(renderTreePosition.parent().style());

    renderTreePosition.computeNextSibling(textNode);

    if (!renderTreePosition.canInsert(*textRenderer))
        return;

    textNode.setRenderer(textRenderer.get());

    if (textUpdate && textUpdate->inheritedDisplayContentsStyle && *textUpdate->inheritedDisplayContentsStyle) {
        // Wrap text renderer into anonymous inline so we can give it a style.
        // This is to support "<div style='display:contents;color:green'>text</div>" type cases
        auto newDisplayContentsAnonymousWrapper = createRenderer<RenderInline>(textNode.document(), RenderStyle::clone(**textUpdate->inheritedDisplayContentsStyle));
        newDisplayContentsAnonymousWrapper->initializeStyle();
        auto& displayContentsAnonymousWrapper = *newDisplayContentsAnonymousWrapper;
        renderTreePosition.insert(WTFMove(newDisplayContentsAnonymousWrapper));

        textRenderer->setInlineWrapperForDisplayContents(&displayContentsAnonymousWrapper);
        displayContentsAnonymousWrapper.addChild(WTFMove(textRenderer));
        return;
    }

    renderTreePosition.insert(WTFMove(textRenderer));
}

void RenderTreeUpdater::updateTextRenderer(Text& text, const Style::TextUpdate* textUpdate)
{
    auto* existingRenderer = text.renderer();
    bool needsRenderer = textRendererIsNeeded(text);

    if (existingRenderer && textUpdate && textUpdate->inheritedDisplayContentsStyle) {
        if (existingRenderer->inlineWrapperForDisplayContents() || *textUpdate->inheritedDisplayContentsStyle) {
            // FIXME: We could update without teardown.
            tearDownRenderer(text);
            existingRenderer = nullptr;
        }
    }

    if (existingRenderer) {
        if (needsRenderer) {
            if (textUpdate)
                existingRenderer->setTextWithOffset(text.data(), textUpdate->offset, textUpdate->length);
            return;
        }
        tearDownRenderer(text);
        renderingParent().didCreateOrDestroyChildRenderer = true;
        return;
    }
    if (!needsRenderer)
        return;
    createTextRenderer(text, renderTreePosition(), textUpdate);
    renderingParent().didCreateOrDestroyChildRenderer = true;
}

void RenderTreeUpdater::storePreviousRenderer(Node& node)
{
    auto* renderer = node.renderer();
    if (!renderer)
        return;
    ASSERT(renderingParent().previousChildRenderer != renderer);
    renderingParent().previousChildRenderer = renderer;
}

void RenderTreeUpdater::tearDownRenderers(Element& root)
{
    tearDownRenderers(root, TeardownType::Full);
}

void RenderTreeUpdater::tearDownRenderers(Element& root, TeardownType teardownType)
{
    WidgetHierarchyUpdatesSuspensionScope suspendWidgetHierarchyUpdates;

    Vector<Element*, 30> teardownStack;

    auto push = [&] (Element& element) {
        if (element.hasCustomStyleResolveCallbacks())
            element.willDetachRenderers();
        teardownStack.append(&element);
    };

    auto& animationController = root.document().frame()->animation();

    auto pop = [&] (unsigned depth) {
        while (teardownStack.size() > depth) {
            auto& element = *teardownStack.takeLast();

            if (teardownType == TeardownType::Full || teardownType == TeardownType::RendererUpdateCancelingAnimations)
                animationController.cancelAnimations(element);

            if (teardownType == TeardownType::Full)
                element.clearHoverAndActiveStatusBeforeDetachingRenderer();

            element.clearStyleDerivedDataBeforeDetachingRenderer();

            if (auto* renderer = element.renderer()) {
                renderer->removeFromParentAndDestroyCleaningUpAnonymousWrappers();
                element.setRenderer(nullptr);
            }
            if (element.hasCustomStyleResolveCallbacks())
                element.didDetachRenderers();
        }
    };

    push(root);

    auto descendants = composedTreeDescendants(root);
    for (auto it = descendants.begin(), end = descendants.end(); it != end; ++it) {
        pop(it.depth());

        if (is<Text>(*it)) {
            tearDownRenderer(downcast<Text>(*it));
            continue;
        }

        push(downcast<Element>(*it));
    }

    pop(0);
}

void RenderTreeUpdater::tearDownRenderer(Text& text)
{
    auto* renderer = text.renderer();
    if (!renderer)
        return;
    renderer->removeFromParentAndDestroyCleaningUpAnonymousWrappers();
    text.setRenderer(nullptr);
}

RenderView& RenderTreeUpdater::renderView()
{
    return *m_document.renderView();
}

#if PLATFORM(IOS)
static EVisibility elementImplicitVisibility(const Element& element)
{
    auto* renderer = element.renderer();
    if (!renderer)
        return VISIBLE;

    auto& style = renderer->style();

    auto width = style.width();
    auto height = style.height();
    if ((width.isFixed() && width.value() <= 0) || (height.isFixed() && height.value() <= 0))
        return HIDDEN;

    auto top = style.top();
    auto left = style.left();
    if (left.isFixed() && width.isFixed() && -left.value() >= width.value())
        return HIDDEN;

    if (top.isFixed() && height.isFixed() && -top.value() >= height.value())
        return HIDDEN;
    return VISIBLE;
}

CheckForVisibilityChange::CheckForVisibilityChange(const Element& element)
    : m_element(element)
    , m_previousDisplay(element.renderStyle() ? element.renderStyle()->display() : NONE)
    , m_previousVisibility(element.renderStyle() ? element.renderStyle()->visibility() : HIDDEN)
    , m_previousImplicitVisibility(WKObservingContentChanges() && WKObservedContentChange() != WKContentVisibilityChange ? elementImplicitVisibility(element) : VISIBLE)
{
}

CheckForVisibilityChange::~CheckForVisibilityChange()
{
    if (!WKObservingContentChanges())
        return;
    if (m_element.isInUserAgentShadowTree())
        return;
    auto* style = m_element.renderStyle();
    if (!style)
        return;
    if ((m_previousDisplay == NONE && style->display() != NONE) || (m_previousVisibility == HIDDEN && style->visibility() != HIDDEN)
        || (m_previousImplicitVisibility == HIDDEN && elementImplicitVisibility(m_element) == VISIBLE))
        WKSetObservedContentChange(WKContentVisibilityChange);
}
#endif

}
