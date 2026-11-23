#include "Floating.h"

using namespace juce;

//==============================================================================
FloatingShell::FloatingShell (UiContext& c, Section& s)
    : section (s),
      dockBtn (c, "dock", "Unir a la ventana principal"),
      restoreAllBtn (c, "restore", U8 ("Regresar todo a su lugar")),
      resetBtn (c, "reset", U8 ("Tamaño normal")),
      pinBtn (c, "pin", "Siempre encima"),
      ctx (c)
{
    for (auto* b : { &dockBtn, &restoreAllBtn, &resetBtn, &pinBtn })
    {
        b->framed = false;
        addAndMakeVisible (b);
    }
    dockBtn.label = "Unir";
    restoreAllBtn.label = "Restablecer";
    dockBtn.onClick  = [this] { if (onDock) onDock(); };
    restoreAllBtn.onClick = [this] { if (onRestoreAll) onRestoreAll(); };
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
        for (auto* b : { &dockBtn, &restoreAllBtn, &resetBtn, &pinBtn })
        {
            b->setAlpha (headerAlpha);
            b->setVisible (headerAlpha > 0.05f);
        }
        repaint (header());
    }
}

float FloatingShell::scaleFactor() const
{
    return jlimit (1.0f, 2.0f, Component::getApproximateScaleFactorForComponent (const_cast<FloatingShell*> (this)));
}

void FloatingShell::paint (Graphics& g)
{
    if (transparent()) return;
    const auto& pal = ctx.vis.pal;
    const float sc = scaleFactor();
    const String key = pal.id + String (pal.themeIdx) + (pal.night ? "n" : "d") + String (getWidth()) + "x" + String (getHeight()) + "@" + String (sc);
    if (key != backgroundKey || ! background.isValid())
    {
        background = Image (Image::ARGB, jmax (1, roundToInt ((float) getWidth() * sc)), jmax (1, roundToInt ((float) getHeight() * sc)), true);
        Graphics bg (background);
        bg.addTransform (AffineTransform::scale (sc));
        draw::chassis (bg, getLocalBounds().toFloat(), pal, 0.75f, ctx.tex, false);
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
        g.fillRoundedRectangle (h.reduced (2.0f), 8.0f);
    }
    else
    {
        {
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (draw::chassisPath (getLocalBounds().toFloat().reduced (0.5f), pal, 0.75f));
            themeCache.draw (g, getLocalBounds().toFloat(), pal, scaleFactor());
        }
        g.setColour (pal.railBorder);
        g.fillRect (h.getX() + 4.0f, h.getBottom() + 1.0f, h.getWidth() - 8.0f, 1.0f);
    }
    const auto col = transparent() ? Colours::white.withAlpha (headerAlpha) : pal.text;
    draw::icon (g, "grip", h.withWidth (22.0f).reduced (4.0f), col.withMultipliedAlpha (0.6f));
    g.setColour (col);
    g.setFont (ctx.fonts.label (pal.uiFont, 12.5f, 1));
    const float buttonsW = (float) (getWidth() - dockBtn.getX()) + 4.0f;
    g.drawText (section.title, h.withTrimmedLeft (24.0f).withTrimmedRight (buttonsW), Justification::centredLeft, true);
}

void FloatingShell::resized()
{
    auto h = header();
    const bool wide = getWidth() >= 380;
    h.removeFromRight (2);
    const int bh = h.getHeight();
    pinBtn.setBounds (h.removeFromRight (bh).reduced (2));
    resetBtn.setBounds (h.removeFromRight (bh).reduced (2));
    restoreAllBtn.setBounds (h.removeFromRight (wide ? 112 : bh).reduced (2));
    dockBtn.setBounds (h.removeFromRight (wide ? 66 : bh).reduced (2));
    for (auto* btn : { &dockBtn, &restoreAllBtn, &resetBtn, &pinBtn }) btn->lightOnDark = transparent();

    Rectangle<int> b;
    if (transparent()) b = getLocalBounds().withTrimmedTop (14);   // la barra flota encima
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
    setResizeLimits (jmax (180, s.natural.x / 3), jmax (100, s.natural.y / 3), 4000, 3000);
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
    if (onBoundsChanged) onBoundsChanged (getBounds());
}
