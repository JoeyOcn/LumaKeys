#include "Floating.h"

using namespace juce;

//==============================================================================
FloatingShell::FloatingShell (UiContext& c, Section& s)
    : section (s),
      minBtn (c, "minimize", "Minimizar (solo la barra)"),
      dockBtn (c, "dock", "Unir a la ventana principal", "Unir"),
      restoreAllBtn (c, "restore", U8 ("Regresar todo a su lugar"), "Restablecer"),
      resetBtn (c, "reset", U8 ("Tamaño normal")),
      pinBtn (c, "pin", "Siempre encima"),
      ctx (c)
{
    for (auto* b : { &minBtn, &dockBtn, &restoreAllBtn, &resetBtn, &pinBtn })
    {
        b->framed = false;
        addAndMakeVisible (b);
    }
    minBtn.onClick        = [this] { if (onMinimize) onMinimize(); };
    dockBtn.onClick       = [this] { if (onDock) onDock(); };
    restoreAllBtn.onClick = [this] { if (onRestoreAll) onRestoreAll(); };
    resetBtn.onClick      = [this] { if (onReset) onReset(); };
    pinBtn.onClick        = [this] { if (onTogglePin) onTogglePin(); };

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
        section.setVisible (true);
        released = true;
    }
}

void FloatingShell::setCollapsed (bool c)
{
    collapsed = c;
    minBtn.setIcon (c ? "expand" : "minimize");
    minBtn.setTooltip (c ? "Mostrar de nuevo" : "Minimizar (solo la barra)");
    section.setVisible (! c);
    resized();
    repaint();
}

Rectangle<int> FloatingShell::header() const
{
    if (transparent()) return getLocalBounds().removeFromTop (headerHeight - 4);
    return getLocalBounds().reduced (margin, 0).withTrimmedTop (4).removeFromTop (headerHeight - 6);
}

float FloatingShell::scaleFactor() const
{
    return jlimit (1.0f, 2.0f, Component::getApproximateScaleFactorForComponent (const_cast<FloatingShell*> (this)));
}

void FloatingShell::timerCallback()
{
    // En modo transparente la barra aparece solo cuando pasas el mouse
    const float target = (! transparent() || collapsed || isMouseOverOrDragging (true)) ? 1.0f : 0.0f;
    if (std::abs (target - headerAlpha) > 0.01f)
    {
        headerAlpha += (target - headerAlpha) * 0.35f;
        for (auto* b : { &minBtn, &dockBtn, &restoreAllBtn, &resetBtn, &pinBtn })
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
    const float sc = scaleFactor();
    const String key = String (pal.look) + (pal.night ? "n" : "d") + pal.body1.toString() + String (getWidth()) + "x" + String (getHeight()) + "@" + String (sc);
    if (key != backgroundKey || ! background.isValid())
    {
        background = Image (Image::ARGB, jmax (1, roundToInt ((float) getWidth() * sc)), jmax (1, roundToInt ((float) getHeight() * sc)), true);
        Graphics bg (background);
        bg.addTransform (AffineTransform::scale (sc));
        draw::device (bg, getLocalBounds().toFloat(), pal, 0.6f, false);
        backgroundKey = key;
    }
    g.drawImage (background, getLocalBounds().toFloat());
}

void FloatingShell::paintOverChildren (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto h = header().toFloat();
    if (transparent())
    {
        if (headerAlpha < 0.05f) return;
        g.setColour (Colours::black.withAlpha (0.55f * headerAlpha));
        g.fillRoundedRectangle (h.reduced (2.0f), 10.0f);
    }
    else if (! collapsed)
    {
        g.setColour (pal.edge.withAlpha (0.6f));
        g.fillRect (h.getX() + 4.0f, h.getBottom() + 1.0f, h.getWidth() - 8.0f, 1.0f);
    }
    const auto col = transparent() ? Colours::white.withAlpha (headerAlpha) : pal.text;
    draw::icon (g, "grip", h.withWidth (26.0f).reduced (5.0f), col.withMultipliedAlpha (0.6f));
    g.setColour (col);
    g.setFont (ctx.fonts.ui (14.5f, 2));
    const float buttonsW = (float) (getWidth() - minBtn.getX()) + 6.0f;
    g.drawText (section.title, h.withTrimmedLeft (28.0f).withTrimmedRight (buttonsW), Justification::centredLeft, true);
}

void FloatingShell::resized()
{
    auto h = header();
    const bool wide = getWidth() >= 440;
    h.removeFromRight (2);
    const int bh = h.getHeight();
    pinBtn.setBounds (h.removeFromRight (bh).reduced (3));
    resetBtn.setBounds (h.removeFromRight (bh).reduced (3));
    restoreAllBtn.setBounds (h.removeFromRight (wide ? 128 : bh).reduced (3));
    dockBtn.setBounds (h.removeFromRight (wide ? 76 : bh).reduced (3));
    minBtn.setBounds (h.removeFromRight (bh).reduced (3));
    for (auto* btn : { &minBtn, &dockBtn, &restoreAllBtn, &resetBtn, &pinBtn }) btn->lightOnDark = transparent();

    if (collapsed) return;
    Rectangle<int> b;
    if (transparent()) b = getLocalBounds().withTrimmedTop (16);   // la barra flota encima
    else               b = getLocalBounds().reduced (margin, 0).withTrimmedTop (headerHeight).withTrimmedBottom (margin);

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

void FloatingShell::mouseDoubleClick (const MouseEvent& e)
{
    if (header().contains (e.getPosition()) && onMinimize) onMinimize();
}

//==============================================================================
FloatingWindow::FloatingWindow (UiContext& c, Section& s, bool transparent, bool onTop, Rectangle<int> screenBounds)
    : ResizableWindow (s.title, Colours::transparentBlack, false),
      isTransparent (transparent)
{
    setOpaque (false);
   #if JUCE_MAC
    setDropShadowEnabled (! transparent);   // macOS dibuja la sombra siguiendo las esquinas redondas
   #else
    setDropShadowEnabled (false);
   #endif
    auto* sh = new FloatingShell (c, s);
    shellPtr = sh;
    setContentOwned (sh, false);
    setResizable (true, true);
    setResizeLimits (jmax (220, s.natural.x / 3), FloatingShell::headerHeight + 6, 4000, 3000);
    setAlwaysOnTop (onTop);
    sh->setPinned (onTop);
    setBounds (screenBounds);
    fullHeight = screenBounds.getHeight();
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

void FloatingWindow::setCollapsed (bool c)
{
    if (shellPtr == nullptr || c == shellPtr->isCollapsed()) return;
    collapsing = true;
    if (c)
    {
        fullHeight = getHeight();
        shellPtr->setCollapsed (true);
        setSize (getWidth(), FloatingShell::headerHeight + 6);
    }
    else
    {
        shellPtr->setCollapsed (false);
        const auto natural = FloatingShell::windowSizeFor (shellPtr->section);
        setSize (getWidth(), fullHeight > FloatingShell::headerHeight + 20 ? fullHeight : natural.y);
    }
    collapsing = false;
}

int FloatingWindow::getDesktopWindowStyleFlags() const
{
    int f = ResizableWindow::getDesktopWindowStyleFlags();
    f &= ~ComponentPeer::windowHasTitleBar;
    f |= ComponentPeer::windowAppearsOnTaskbar;
    f |= ComponentPeer::windowIsSemiTransparent;
    if (isTransparent)
        f &= ~ComponentPeer::windowHasDropShadow;
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
    if (! collapsing && shellPtr != nullptr && ! shellPtr->isCollapsed())
        fullHeight = getHeight();
    if (onBoundsChanged) onBoundsChanged (shellPtr != nullptr && shellPtr->isCollapsed()
                                              ? getBounds().withHeight (fullHeight) : getBounds());
}
