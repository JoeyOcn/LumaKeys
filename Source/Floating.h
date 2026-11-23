#pragma once

#include "Sections.h"

//==============================================================================
// El marco de una sección separada: barra para moverla y botones
// (minimizar, unir, restablecer todo, tamaño normal, siempre encima).
class FloatingShell : public juce::Component, private juce::Timer
{
public:
    FloatingShell (UiContext& c, Section& s);
    ~FloatingShell() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void setPinned (bool p) { pinBtn.setActive (p); }
    void setCollapsed (bool c);
    bool isCollapsed() const { return collapsed; }
    void releaseSection();

    Section& section;
    PushButton minBtn, dockBtn, restoreAllBtn, resetBtn, pinBtn;
    std::function<void()> onMinimize, onDock, onRestoreAll, onReset, onTogglePin;

    static constexpr int headerHeight = 40;
    static constexpr int margin = 10;
    static juce::Point<int> windowSizeFor (const Section& s) { return { s.natural.x + margin * 2, s.natural.y + headerHeight + margin }; }

private:
    void timerCallback() override;
    bool transparent() const { return ctx.vis.transparent; }
    juce::Rectangle<int> header() const;
    float scaleFactor() const;

    UiContext& ctx;
    juce::ComponentDragger dragger;
    juce::Image background;
    juce::String backgroundKey;
    float headerAlpha = 1.0f;
    bool released = false, collapsed = false;
};

//==============================================================================
class FloatingWindow : public juce::ResizableWindow
{
public:
    FloatingWindow (UiContext& c, Section& s, bool transparent, bool onTop, juce::Rectangle<int> screenBounds);
    ~FloatingWindow() override;

    int getDesktopWindowStyleFlags() const override;
    juce::BorderSize<int> getBorderThickness() const override { return {}; }
    juce::BorderSize<int> getContentComponentBorder() const override { return {}; }
    void moved() override;
    void resized() override;

    void releaseSection();
    FloatingShell& shell() { return *shellPtr; }
    void setCollapsed (bool c);

    std::function<void (juce::Rectangle<int>)> onBoundsChanged;

private:
    bool isTransparent;
    FloatingShell* shellPtr = nullptr;
    int fullHeight = 0;
    bool collapsing = false;
};
