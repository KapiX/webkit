/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

WI.TabBrowser = class TabBrowser extends WI.View
{
    constructor(element, tabBar, navigationSidebar, detailsSidebar)
    {
        console.assert(tabBar, "Must provide a TabBar.");

        super(element);

        this.element.classList.add("tab-browser");

        this._tabBar = tabBar;
        this._navigationSidebar = navigationSidebar || null;
        this._detailsSidebar = detailsSidebar || null;

        if (this._navigationSidebar) {
            this._navigationSidebar.addEventListener(WI.Sidebar.Event.CollapsedStateDidChange, this._sidebarCollapsedStateDidChange, this);
            this._navigationSidebar.addEventListener(WI.Sidebar.Event.WidthDidChange, this._sidebarWidthDidChange, this);
        }

        if (this._detailsSidebar) {
            this._detailsSidebar.addEventListener(WI.Sidebar.Event.CollapsedStateDidChange, this._sidebarCollapsedStateDidChange, this);
            this._detailsSidebar.addEventListener(WI.Sidebar.Event.SidebarPanelSelected, this._sidebarPanelSelected, this);
            this._detailsSidebar.addEventListener(WI.Sidebar.Event.WidthDidChange, this._sidebarWidthDidChange, this);
        }

        this._contentViewContainer = new WI.ContentViewContainer;
        this.addSubview(this._contentViewContainer);

        let showNextTab = () => { this._showNextTab(); };
        let showPreviousTab = () => { this._showPreviousTab(); };

        let isRTL = WI.resolvedLayoutDirection() === WI.LayoutDirection.RTL;

        let nextKey1 = isRTL ? WI.KeyboardShortcut.Key.LeftCurlyBrace : WI.KeyboardShortcut.Key.RightCurlyBrace;
        let previousKey1 = isRTL ? WI.KeyboardShortcut.Key.RightCurlyBrace : WI.KeyboardShortcut.Key.LeftCurlyBrace;

        this._showNextTabKeyboardShortcut1 = new WI.KeyboardShortcut(WI.KeyboardShortcut.Modifier.CommandOrControl | WI.KeyboardShortcut.Modifier.Shift, nextKey1, showNextTab);
        this._showPreviousTabKeyboardShortcut1 = new WI.KeyboardShortcut(WI.KeyboardShortcut.Modifier.CommandOrControl | WI.KeyboardShortcut.Modifier.Shift, previousKey1, showPreviousTab);

        let nextModifier2 = isRTL ? WI.KeyboardShortcut.Modifier.Shift : 0;
        let previousModifier2 = isRTL ? 0 : WI.KeyboardShortcut.Modifier.Shift;

        this._showNextTabKeyboardShortcut2 = new WI.KeyboardShortcut(WI.KeyboardShortcut.Modifier.Control | nextModifier2, WI.KeyboardShortcut.Key.Tab, showNextTab);
        this._showPreviousTabKeyboardShortcut2 = new WI.KeyboardShortcut(WI.KeyboardShortcut.Modifier.Control | previousModifier2, WI.KeyboardShortcut.Key.Tab, showPreviousTab);

        let previousTabKey = isRTL ? WI.KeyboardShortcut.Key.Right : WI.KeyboardShortcut.Key.Left;
        let nextTabKey = isRTL ? WI.KeyboardShortcut.Key.Left : WI.KeyboardShortcut.Key.Right;
        this._previousTabKeyboardShortcut = new WI.KeyboardShortcut(WI.KeyboardShortcut.Modifier.CommandOrControl | WI.KeyboardShortcut.Modifier.Shift, previousTabKey, this._showPreviousTabCheckingForEditableField.bind(this));
        this._previousTabKeyboardShortcut.implicitlyPreventsDefault = false;
        this._nextTabKeyboardShortcut = new WI.KeyboardShortcut(WI.KeyboardShortcut.Modifier.CommandOrControl | WI.KeyboardShortcut.Modifier.Shift, nextTabKey, this._showNextTabCheckingForEditableField.bind(this));
        this._nextTabKeyboardShortcut.implicitlyPreventsDefault = false;

        this._tabBar.addEventListener(WI.TabBar.Event.TabBarItemSelected, this._tabBarItemSelected, this);
        this._tabBar.addEventListener(WI.TabBar.Event.TabBarItemAdded, this._tabBarItemAdded, this);
        this._tabBar.addEventListener(WI.TabBar.Event.TabBarItemRemoved, this._tabBarItemRemoved, this);
        this._tabBar.newTabTabBarItem.addEventListener(WI.PinnedTabBarItem.Event.ContextMenu, this._handleNewTabContextMenu, this);

        this._recentTabContentViews = [];
        this._closedTabClasses = new Set;
    }

    // Public

    get tabBar()
    {
        return this._tabBar;
    }

    get navigationSidebar()
    {
        return this._navigationSidebar;
    }

    get detailsSidebar()
    {
        return this._detailsSidebar;
    }

    get selectedTabContentView()
    {
        return this._contentViewContainer.currentContentView;
    }

    bestTabContentViewForClass(constructor)
    {
        console.assert(!this.selectedTabContentView || this.selectedTabContentView === this._recentTabContentViews[0]);

        for (var tabContentView of this._recentTabContentViews) {
            if (tabContentView instanceof constructor)
                return tabContentView;
        }

        return null;
    }

    bestTabContentViewForRepresentedObject(representedObject, options = {})
    {
        console.assert(!this.selectedTabContentView || this.selectedTabContentView === this._recentTabContentViews[0]);

        for (var tabContentView of this._recentTabContentViews) {
            if (options.ignoreSearchTab && tabContentView instanceof WI.SearchTabContentView)
                continue;

            if (options.ignoreNetworkTab && tabContentView instanceof WI.NetworkTabContentView)
                continue;

            if (options.ignoreResourcesTab && tabContentView instanceof WI.ResourcesTabContentView)
                continue;

            if (options.ignoreDebuggerTab && tabContentView instanceof WI.DebuggerTabContentView)
                continue;

            if (tabContentView.canShowRepresentedObject(representedObject))
                return tabContentView;
        }

        return null;
    }

    addTabForContentView(tabContentView, options = {})
    {
        console.assert(tabContentView instanceof WI.TabContentView);
        if (!(tabContentView instanceof WI.TabContentView))
            return false;

        let tabBarItem = tabContentView.tabBarItem;

        console.assert(tabBarItem instanceof WI.TabBarItem);
        if (!(tabBarItem instanceof WI.TabBarItem))
            return false;

        if (tabBarItem.representedObject !== tabContentView)
            tabBarItem.representedObject = tabContentView;

        tabContentView.parentTabBrowser = this;

        if (tabBarItem.parentTabBar === this._tabBar)
            return true;

        // Add the tab after the first tab content view, since the first
        // tab content view is the currently selected one.
        if (this._recentTabContentViews.length && this.selectedTabContentView)
            this._recentTabContentViews.splice(1, 0, tabContentView);
        else
            this._recentTabContentViews.push(tabContentView);

        if (typeof options.insertionIndex === "number")
            this._tabBar.insertTabBarItem(tabBarItem, options.insertionIndex, options);
        else
            this._tabBar.addTabBarItem(tabBarItem, options);

        console.assert(this._recentTabContentViews.length === this._tabBar.normalTabCount);
        console.assert(!this.selectedTabContentView || this.selectedTabContentView === this._recentTabContentViews[0]);

        return true;
    }

    showTabForContentView(tabContentView, options = {})
    {
        if (!this.addTabForContentView(tabContentView, options))
            return false;

        if (!options.suppressSelection)
            this._tabBar.selectedTabBarItem = tabContentView.tabBarItem;

        // FIXME: this is a workaround for <https://webkit.org/b/151876>.
        // Without this extra call, we might never lay out the child tab
        // if it has already marked itself as dirty in the same run loop
        // as it is attached. It will schedule a layout, but when the rAF
        // fires the parent will abort the layout because the counter is
        // out of sync.
        this.needsLayout();
        return true;
    }

    closeTabForContentView(tabContentView, options = {})
    {
        console.assert(tabContentView instanceof WI.TabContentView);
        if (!(tabContentView instanceof WI.TabContentView))
            return false;

        console.assert(tabContentView.tabBarItem instanceof WI.TabBarItem);
        if (!(tabContentView.tabBarItem instanceof WI.TabBarItem))
            return false;

        if (tabContentView.tabBarItem.parentTabBar !== this._tabBar)
            return false;

        this._tabBar.removeTabBarItem(tabContentView.tabBarItem, options);

        console.assert(this._recentTabContentViews.length === this._tabBar.normalTabCount);
        console.assert(!this.selectedTabContentView || this.selectedTabContentView === this._recentTabContentViews[0]);

        return true;
    }

    // Protected

    layout()
    {
        if (this.layoutReason !== WI.View.LayoutReason.Resize)
            return;

        for (let tabContentView of this._recentTabContentViews)
            tabContentView[WI.TabBrowser.NeedsResizeLayoutSymbol] = tabContentView !== this.selectedTabContentView;
    }

    // Private

    _tabBarItemSelected(event)
    {
        let tabContentView = this._tabBar.selectedTabBarItem ? this._tabBar.selectedTabBarItem.representedObject : null;

        if (tabContentView) {
            let isSettingsTab = tabContentView instanceof WI.SettingsTabContentView;
            if (!isSettingsTab) {
                this._recentTabContentViews.remove(tabContentView);
                this._recentTabContentViews.unshift(tabContentView);
            }

            this._contentViewContainer.showContentView(tabContentView);

            console.assert(this.selectedTabContentView);
            console.assert(this._recentTabContentViews.length === this._tabBar.normalTabCount);
            console.assert(this.selectedTabContentView === this._recentTabContentViews[0] || isSettingsTab);
        } else {
            this._contentViewContainer.closeAllContentViews();

            console.assert(!this.selectedTabContentView);
        }

        this._showNavigationSidebarPanelForTabContentView(tabContentView);
        this._showDetailsSidebarPanelsForTabContentView(tabContentView);

        // If the tab browser was resized prior to showing the tab, the new tab needs to perform a resize layout.
        if (tabContentView && tabContentView[WI.TabBrowser.NeedsResizeLayoutSymbol]) {
            tabContentView[WI.TabBrowser.NeedsResizeLayoutSymbol] = false;
            tabContentView.updateLayout(WI.View.LayoutReason.Resize);
        }

        this.dispatchEventToListeners(WI.TabBrowser.Event.SelectedTabContentViewDidChange);
    }

    _tabBarItemAdded(event)
    {
        let tabContentView = event.data.tabBarItem.representedObject;

        console.assert(tabContentView);
        if (!tabContentView)
            return;

        this._closedTabClasses.delete(tabContentView.constructor);
    }

    _tabBarItemRemoved(event)
    {
        let tabContentView = event.data.tabBarItem.representedObject;

        console.assert(tabContentView);
        if (!tabContentView)
            return;

        this._recentTabContentViews.remove(tabContentView);

        if (!tabContentView.constructor.isEphemeral())
            this._closedTabClasses.add(tabContentView.constructor);

        this._contentViewContainer.closeContentView(tabContentView);

        tabContentView.parentTabBrowser = null;

        console.assert(this._recentTabContentViews.length === this._tabBar.normalTabCount);
        console.assert(!this.selectedTabContentView || this.selectedTabContentView === this._recentTabContentViews[0]);
    }

    _handleNewTabContextMenu(event)
    {
        // The array must be reversed because Sets insert into the end, and we want to display the
        // most recently closed item first (which is the last item added to the set).
        let closedTabClasses = Array.from(this._closedTabClasses).reverse();
        let allTabClasses = Array.from(WI.knownTabClasses());
        let tabClassesToDisplay = closedTabClasses.concat(allTabClasses.filter((tabClass) => {
            if (closedTabClasses.includes(tabClass))
                return false;

            if (tabClass.isEphemeral())
                return false;

            return WI.isNewTabWithTypeAllowed(tabClass.Type);
        }));
        if (!tabClassesToDisplay.length)
            return;

        let contextMenu = event.data.contextMenu;

        contextMenu.appendItem(WI.UIString("Recently Closed Tabs"), null, true);

        for (let tabClass of tabClassesToDisplay) {
            contextMenu.appendItem(tabClass.tabInfo().title, () => {
                WI.createNewTabWithType(tabClass.Type, {shouldShowNewTab: true});
            });
        }
    }

    _sidebarPanelSelected(event)
    {
        if (this._ignoreSidebarEvents)
            return;

        var tabContentView = this.selectedTabContentView;
        if (!tabContentView)
            return;

        console.assert(event.target === this._detailsSidebar);

        if (tabContentView.managesDetailsSidebarPanels)
            return;

        var selectedSidebarPanel = this._detailsSidebar.selectedSidebarPanel;
        tabContentView.detailsSidebarSelectedPanelSetting.value = selectedSidebarPanel ? selectedSidebarPanel.identifier : null;
    }

    _sidebarCollapsedStateDidChange(event)
    {
        if (this._ignoreSidebarEvents)
            return;

        var tabContentView = this.selectedTabContentView;
        if (!tabContentView)
            return;

        if (event.target === this._navigationSidebar)
            tabContentView.navigationSidebarCollapsedSetting.value = this._navigationSidebar.collapsed;
        else if (event.target === this._detailsSidebar && !tabContentView.managesDetailsSidebarPanels)
            tabContentView.detailsSidebarCollapsedSetting.value = this._detailsSidebar.collapsed;
    }

    _sidebarWidthDidChange(event)
    {
        if (this._ignoreSidebarEvents || !event.data)
            return;

        let tabContentView = this.selectedTabContentView;
        if (!tabContentView)
            return;

        switch (event.target) {
        case this._navigationSidebar:
            tabContentView.navigationSidebarWidthSetting.value = event.data.newWidth;
            break;

        case this._detailsSidebar:
            tabContentView.detailsSidebarWidthSetting.value = event.data.newWidth;
            break;
        }
    }

    _showNavigationSidebarPanelForTabContentView(tabContentView)
    {
        if (!this._navigationSidebar)
            return;

        this._ignoreSidebarEvents = true;

        this._navigationSidebar.removeSidebarPanel(0);

        console.assert(!this._navigationSidebar.sidebarPanels.length);

        if (!tabContentView) {
            this._ignoreSidebarEvents = false;
            return;
        }

        if (tabContentView.navigationSidebarWidthSetting.value)
            this._navigationSidebar.width = tabContentView.navigationSidebarWidthSetting.value;

        var navigationSidebarPanel = tabContentView.navigationSidebarPanel;
        if (!navigationSidebarPanel) {
            this._navigationSidebar.collapsed = true;
            this._ignoreSidebarEvents = false;
            return;
        }

        this._navigationSidebar.addSidebarPanel(navigationSidebarPanel);
        this._navigationSidebar.selectedSidebarPanel = navigationSidebarPanel;

        this._navigationSidebar.collapsed = tabContentView.navigationSidebarCollapsedSetting.value;

        this._ignoreSidebarEvents = false;
    }

    _showDetailsSidebarPanelsForTabContentView(tabContentView)
    {
        if (!this._detailsSidebar)
            return;

        this._ignoreSidebarEvents = true;

        for (var i = this._detailsSidebar.sidebarPanels.length - 1; i >= 0; --i)
            this._detailsSidebar.removeSidebarPanel(i);

        console.assert(!this._detailsSidebar.sidebarPanels.length);

        if (!tabContentView) {
            this._ignoreSidebarEvents = false;
            return;
        }

        if (tabContentView.detailsSidebarWidthSetting.value)
            this._detailsSidebar.width = tabContentView.detailsSidebarWidthSetting.value;

        if (tabContentView.managesDetailsSidebarPanels) {
            tabContentView.showDetailsSidebarPanels();
            this._ignoreSidebarEvents = false;
            return;
        }

        var detailsSidebarPanels = tabContentView.detailsSidebarPanels;
        if (!detailsSidebarPanels) {
            this._detailsSidebar.collapsed = true;
            this._ignoreSidebarEvents = false;
            return;
        }

        for (var detailsSidebarPanel of detailsSidebarPanels)
            this._detailsSidebar.addSidebarPanel(detailsSidebarPanel);

        this._detailsSidebar.selectedSidebarPanel = tabContentView.detailsSidebarSelectedPanelSetting.value || detailsSidebarPanels[0];

        this._detailsSidebar.collapsed = tabContentView.detailsSidebarCollapsedSetting.value || !detailsSidebarPanels.length;

        this._ignoreSidebarEvents = false;
    }

    _showPreviousTab(event)
    {
        this._tabBar.selectPreviousTab();
    }

    _showNextTab(event)
    {
        this._tabBar.selectNextTab();
    }

    _showNextTabCheckingForEditableField(event)
    {
        if (WI.isEventTargetAnEditableField(event))
            return;

        this._showNextTab(event);

        event.preventDefault();
    }

    _showPreviousTabCheckingForEditableField(event)
    {
        if (WI.isEventTargetAnEditableField(event))
            return;

        this._showPreviousTab(event);

        event.preventDefault();
    }
};

WI.TabBrowser.NeedsResizeLayoutSymbol = Symbol("needs-resize-layout");

WI.TabBrowser.Event = {
    SelectedTabContentViewDidChange: "tab-browser-selected-tab-content-view-did-change"
};
