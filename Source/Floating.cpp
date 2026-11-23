#include "Floating.h"

using namespace juce;

//==============================================================================
FloatingShell::FloatingShell (UiContext& c, Section& s)
    : section (s),
      dockBtn (c, "dock", "Unir a la ventana principal"),
      resetBtn (c, "reset", U8 ("Tamaño normal")),
      pinBtn (c, "pin", "Siempre encima"),
      ctx (c)
{
    for (auto* b : { &dockBtn, &resetBtn, &pinBtn })
    {
        b->framed = false;
        addAndMakeVisible (b);
    }
    dockBtn.onClick  = [this] { if (onDock) onDock(); };
    resetBtn.onClick = [this] { if (onReset) onReset(); };
    pinBtn.onClick   = [this] { if (onTogglePin) onTogglePin(); };

    section.floating = true;
    addAndMakeVisible (section);
    startTimerHz (30);
}

FloatingShell::~FloatingShell()
{
    releaseSection();
}

void FloatingShell::releaseSection()
{
    if (! released)
    {
        removeChildComponent (&section);
        section.floating = false;
        released = true;
    }
}

void FloatingShell::timerCallback()
{
    // En modo transparente la barra aparece solo cuando pasas el mouse
    const float target = (! transparent() || isMouseOverOrDragging (true)) ? 1.0f : 0.0f;
    if (std::abs (target - headerAlpha) > 0.01f)
    {
        headerAlpha += (target - headerAlpha) * 0.35f;
        for (auto* b : { &dockBtn, &resetBtn, &pinBtn })
        {
            b->setAlpha (headerAlpha);
            b->setVisible (headerAlpha > 0.05f);
        }
        repaint (header());
    }
}

void FloatingShell::paint (Graphics& g)
{
    if (transparent()) return;
    const auto& pal = ctx.vis.pal;
    draw::body (g, getLocalBounds().toFloat(), pal, ctx.tex);
}

void FloatingShell::paintOverChildren (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto h = header().toFloat();
    if (transparent())
    {
        if (headerAlpha < 0.05f) return;
        g.setColour (Colours::black.withAlpha (0.55f * headerAlpha));
        g.fillRoundedRectangle (h.reduced (2.0f), 6.0f);
    }
    else
    {
        g.setColour (pal.rail);
        g.fillRect (h);
        g.setColour (pal.railBorder);
        g.fillRect (h.removeFromBottom (1.0f));
        h = header().toFloat();
    }
    const auto col = transparent() ? Colours::white.withAlpha (headerAlpha) : pal.text;
    draw::icon (g, "grip", h.withWidth (22.0f).reduced (4.0f), col.withMultipliedAlpha (0.6f));
    g.setColour (col);
    g.setFont (ctx.fonts.label (pal.uiFont, 12.0f, 1));
    g.drawText (section.title, h.withTrimmedLeft (24.0f).withTrimmedRight (90.0f), Justification::centredLeft, true);
}

void FloatingShell::resized()
{
    auto b = getLocalBounds();
    auto h = b.removeFromTop (headerHeight);
    if (transparent()) b = getLocalBounds().withTrimmedTop (headerHeight / 2);   // la barra flota encima

    h.removeFromRight (4);
    pinBtn.setBounds (h.removeFromRight (headerHeight).reduced (2));
    resetBtn.setBounds (h.removeFromRight (headerHeight).reduced (2));
    dockBtn.setBounds (h.removeFromRight (headerHeight).reduced (2));
    for (auto* btn : { &dockBtn, &resetBtn, &pinBtn }) btn->lightOnDark = transparent();

    section.k = jlimit (0.45f, 3.0f, jmin ((float) b.getWidth() / (float) section.natural.x, (float) b.getHeight() / (float) section.natural.y));
    section.setBounds (b);
    section.toBack();
}

void FloatingShell::mouseDown (const MouseEvent& e)
{
    if (auto* w = getTopLevelComponent())
        dragger.startDraggingComponent (w, e.getEventRelativeTo (w));
}

void FloatingShell::mouseDrag (const MouseEvent& e)
{
    if (auto* w = getTopLevelComponent())
        dragger.dragComponent (w, e.getEventRelativeTo (w), nullptr);
}

//==============================================================================
FloatingWindow::FloatingWindow (UiContext& c, Section& s, bool transparent, bool onTop, Rectangle<int> screenBounds)
    : ResizableWindow (s.title, transparent ? Colours::transparentBlack : c.vis.pal.bodyColour, false),
      isTransparent (transparent)
{
    setOpaque (! transparent);
    setDropShadowEnabled (! transparent);
    auto* sh = new FloatingShell (c, s);
    shellPtr = sh;
    setContentOwned (sh, false);
    setResizable (true, true);
    setResizeLimits (jmax (160, s.natural.x / 3), jmax (90, s.natural.y / 3), 4000, 3000);
    setAlwaysOnTop (onTop);
    sh->setPinned (onTop);
    setBounds (screenBounds);
    addToDesktop (getDesktopWindowStyleFlags());
    setVisible (true);
}

FloatingWindow::~FloatingWindow()
{
    releaseSection();
    clearContentComponent();
}

void FloatingWindow::releaseSection()
{
    if (shellPtr != nullptr)
        shellPtr->releaseSection();
}

int FloatingWindow::getDesktopWindowStyleFlags() const
{
    int f = ResizableWindow::getDesktopWindowStyleFlags();
    f &= ~ComponentPeer::windowHasTitleBar;
    f |= ComponentPeer::windowAppearsOnTaskbar;
    if (isTransparent)
    {
        f |= ComponentPeer::windowIsSemiTransparent;
        f &= ~ComponentPeer::windowHasDropShadow;
    }
    return f;
}

void FloatingWindow::moved()
{
    ResizableWindow::moved();
    if (onBoundsChanged) onBoundsChanged (getBounds());
}

void FloatingWindow::resized()
{
    ResizableWindow::resized();
    if (onBoundsChanged) onBoundsChanged (getBounds());
}
