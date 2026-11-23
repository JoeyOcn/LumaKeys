#pragma once

#include "Sections.h"

//==============================================================================
// El marco de una sección separada: barra para moverla y botones (unir, tamaño normal, siempre encima).
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
    void setPinned (bool p) { pinBtn.setActive (p); }
    void releaseSection();

    Section& section;
    IconButton dockBtn, resetBtn, pinBtn;
    std::function<void()> onDock, onReset, onTogglePin;

    static constexpr int headerHeight = 26;

private:
    void timerCallback() override;
    bool transparent() const { return ctx.vis.transparent; }
    juce::Rectangle<int> header() const { return getLocalBounds().removeFromTop (headerHeight); }

    UiContext& ctx;
    juce::ComponentDragger dragger;
    float headerAlpha = 1.0f;
    bool released = false;
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

    std::function<void (juce::Rectangle<int>)> onBoundsChanged;

private:
    bool isTransparent;
    FloatingShell* shellPtr = nullptr;
};
