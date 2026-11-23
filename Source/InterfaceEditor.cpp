#include "InterfaceEditor.h"
#include "Sections.h"

using namespace juce;

//==============================================================================
// Pastillas de opciones
//==============================================================================
void ChipGroup::layout (int width)
{
    rects.clear();
    const float h = 34.0f, gap = 7.0f;
    float x = 0.0f, y = 0.0f;
    for (int i = 0; i < names.size(); ++i)
    {
        const auto f = fontFor ? fontFor (i, 15.0f) : ctx.fonts.ui (14.0f, 1);
        float w = jmax (62.0f, draw::textWidth (f, names[i]) + 26.0f + decorateW);
        w = jmin (w, (float) width);
        if (x > 0.0f && x + w > (float) width) { x = 0.0f; y += h + gap; }
        rects.push_back ({ x, y, w, h });
        x += w + gap;
    }
}

int ChipGroup::heightFor (int width)
{
    layout (width);
    return rects.empty() ? 0 : (int) std::ceil (rects.back().getBottom()) + 2;
}

void ChipGroup::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    for (int i = 0; i < (int) rects.size(); ++i)
    {
        auto r = rects[(size_t) i].reduced (0.5f);
        const bool sel = i == selected;
        const float rad = r.getHeight() * shapedRadius (pal, 0.5f);
        if (sel)
        {
            g.setColour (pal.accent);
            g.fillRoundedRectangle (r, rad);
        }
        else
        {
            g.setColour (pal.night ? pal.plate.brighter (0.12f) : pal.plate);
            g.fillRoundedRectangle (r, rad);
            g.setColour (pal.text.withAlpha (i == hover ? 0.32f : 0.12f));
            g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
        }
        auto in = r.reduced (12.0f, 0.0f);
        if (decorate && decorateW > 0.0f)
            decorate (g, in.removeFromLeft (decorateW), i);
        g.setColour (sel ? pal.accentInk : pal.text);
        g.setFont (fontFor ? fontFor (i, 15.0f) : ctx.fonts.ui (14.0f, sel ? 2 : 1));
        g.drawFittedText (names[i], in.toNearestInt(), Justification::centred, 1, 0.8f);
    }
}

void ChipGroup::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < (int) rects.size(); ++i) if (rects[(size_t) i].contains (e.position)) h = i;
    if (h != hover) { hover = h; setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); }
}

void ChipGroup::mouseUp (const MouseEvent& e)
{
    for (int i = 0; i < (int) rects.size(); ++i)
        if (rects[(size_t) i].contains (e.position))
        {
            selected = i;
            repaint();
            if (onChange) onChange (i);
            return;
        }
}

//==============================================================================
// Deslizador
//==============================================================================
Rectangle<float> MiniSlider::track() const
{
    return getLocalBounds().toFloat().withTrimmedRight (64.0f).reduced (10.0f, 0.0f);
}

void MiniSlider::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto t = track();
    const float cy = t.getCentreY();
    const float pos = (value - minV) / jmax (0.0001f, maxV - minV);
    auto bar = Rectangle<float> (t.getX(), cy - 3.0f, t.getWidth(), 6.0f);
    g.setColour (pal.night ? pal.plate.brighter (0.18f) : pal.plate.darker (0.08f));
    g.fillRoundedRectangle (bar, 3.0f);
    g.setColour (pal.accent);
    g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * pos), 3.0f);
    const float x = t.getX() + t.getWidth() * pos;
    g.setColour (Colours::black.withAlpha (0.2f));
    g.fillEllipse (x - 9.0f, cy - 8.0f, 18.0f, 18.0f);
    g.setColour (pal.night ? pal.btn.brighter (0.25f) : Colours::white);
    g.fillEllipse (x - 9.0f, cy - 9.0f, 18.0f, 18.0f);
    g.setColour (pal.accent);
    g.drawEllipse (x - 9.0f, cy - 9.0f, 18.0f, 18.0f, 2.0f);
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (14.0f, 2));
    g.drawText (format ? format (value) : String (value, 2), getLocalBounds().removeFromRight (60), Justification::centredRight, false);
}

void MiniSlider::set (float v)
{
    v = jlimit (minV, maxV, v);
    if (std::abs (v - value) < 1.0e-5f) return;
    value = v;
    repaint();
    if (onChange) onChange (value);
}

void MiniSlider::mouseDown (const MouseEvent& e)
{
    dragStartV = value;
    if (! e.mods.isShiftDown())
    {
        const auto t = track();
        set (minV + (maxV - minV) * jlimit (0.0f, 1.0f, (e.position.x - t.getX()) / t.getWidth()));
        dragStartV = value;
    }
}

void MiniSlider::mouseDrag (const MouseEvent& e)
{
    const auto t = track();
    const float perPx = (maxV - minV) / t.getWidth() * (e.mods.isShiftDown() ? 0.15f : 1.0f);
    set (dragStartV + (float) e.getDistanceFromDragStartX() * perPx);
}

void MiniSlider::mouseDoubleClick (const MouseEvent&) { set (defV); }

void MiniSlider::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    set (value + (w.deltaY + w.deltaX) * (maxV - minV) * 0.15f);
}

//==============================================================================
// Número arrastrable
//==============================================================================
void NumberBox::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (pal.night ? pal.plate.brighter (0.12f) : pal.plate);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (pal.text.withAlpha (isMouseOverOrDragging() ? 0.35f : 0.12f));
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
    if (editor != nullptr) return;
    auto in = r.reduced (8.0f, 0.0f);
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (12.5f, 2));
    g.drawText (label, in.removeFromLeft (14.0f), Justification::centredLeft, false);
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (15.0f, 2));
    g.drawText (String (value), in, Justification::centredRight, false);
}

void NumberBox::set (int v)
{
    v = jlimit (minV, maxV, v);
    if (v == value) return;
    value = v;
    repaint();
    if (onChange) onChange (value);
}

void NumberBox::mouseDown (const MouseEvent&) { dragStartV = value; setMouseCursor (MouseCursor::UpDownResizeCursor); }

void NumberBox::mouseDrag (const MouseEvent& e)
{
    const float perPx = e.mods.isShiftDown() ? 0.125f : 0.5f;
    set (dragStartV + roundToInt ((float) -e.getDistanceFromDragStartY() * perPx));
}

void NumberBox::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    set (value + (w.deltaY > 0 ? 1 : (w.deltaY < 0 ? -1 : 0)));
}

void NumberBox::mouseDoubleClick (const MouseEvent&)
{
    editor = std::make_unique<TextEditor>();
    editor->setFont (ctx.fonts.ui (15.0f, 2));
    editor->setJustification (Justification::centred);
    editor->setInputRestrictions (3, "0123456789");
    editor->setText (String (value), false);
    editor->setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
    editor->setColour (TextEditor::outlineColourId, Colours::transparentBlack);
    editor->setColour (TextEditor::focusedOutlineColourId, ctx.vis.pal.accent);
    editor->setColour (TextEditor::textColourId, ctx.vis.pal.text);
    addAndMakeVisible (*editor);
    editor->setBounds (getLocalBounds());
    editor->selectAll();
    editor->grabKeyboardFocus();
    SafePointer<NumberBox> safe (this);
    auto done = [safe]
    {
        if (safe == nullptr || safe->editor == nullptr) return;
        const int v = safe->editor->getText().getIntValue();
        MessageManager::callAsync ([safe, v]
        {
            if (safe == nullptr) return;
            safe->editor.reset();
            safe->set (v);
            safe->repaint();
        });
    };
    editor->onReturnKey = done;
    editor->onFocusLost = done;
    editor->onEscapeKey = [safe] { MessageManager::callAsync ([safe] { if (safe != nullptr) { safe->editor.reset(); safe->repaint(); } }); };
}

//==============================================================================
// Selector de color preciso
//==============================================================================
PrecisePicker::PrecisePicker (UiContext& c)
    : ctx (c), rBox (c, "R", 0, 255), gBox (c, "G", 0, 255), bBox (c, "B", 0, 255)
{
    hex.setFont (ctx.fonts.ui (15.0f, 2));
    hex.setJustification (Justification::centred);
    hex.setInputRestrictions (7, "#0123456789abcdefABCDEF");
    hex.setTooltip (U8 ("Escribe el color (por ejemplo #FF5A1F) y presiona Enter"));
    auto applyHex = [this]
    {
        auto t = hex.getText().trim().removeCharacters ("#");
        if (t.length() == 3) t = String::charToString (t[0]) + t[0] + t[1] + t[1] + t[2] + t[2];
        if (t.length() != 6) { syncFields(); return; }
        const auto col = Colour::fromString ("ff" + t);
        hue = col.getHue(); sat = col.getSaturation(); val = col.getBrightness();
        current = col;
        repaint();
        syncFields (true);
        if (onColour) onColour (current);
    };
    hex.onReturnKey = applyHex;
    hex.onFocusLost = applyHex;
    hex.onTextChange = [this, applyHex] { if (hex.getText().removeCharacters ("#").length() == 6 && hex.hasKeyboardFocus (false)) applyHex(); };
    addAndMakeVisible (hex);
    for (auto* b : { &rBox, &gBox, &bBox })
        addAndMakeVisible (b);
    rBox.onChange = [this] (int v) { current = Colour ((uint8) v, current.getGreen(), current.getBlue()); hue = current.getHue(); sat = current.getSaturation(); val = current.getBrightness(); syncFields(); repaint(); if (onColour) onColour (current); };
    gBox.onChange = [this] (int v) { current = Colour (current.getRed(), (uint8) v, current.getBlue()); hue = current.getHue(); sat = current.getSaturation(); val = current.getBrightness(); syncFields(); repaint(); if (onColour) onColour (current); };
    bBox.onChange = [this] (int v) { current = Colour (current.getRed(), current.getGreen(), (uint8) v); hue = current.getHue(); sat = current.getSaturation(); val = current.getBrightness(); syncFields(); repaint(); if (onColour) onColour (current); };
    setSize (390, 292);
}

void PrecisePicker::setColour (Colour c)
{
    c = c.withAlpha (1.0f);
    current = original = c;
    hue = c.getHue(); sat = c.getSaturation(); val = c.getBrightness();
    syncFields();
    repaint();
}

void PrecisePicker::syncFields (bool fromHex)
{
    const auto& pal = ctx.vis.pal;
    hex.setColour (TextEditor::backgroundColourId, pal.night ? pal.plate.brighter (0.12f) : pal.plate);
    hex.setColour (TextEditor::outlineColourId, pal.text.withAlpha (0.12f));
    hex.setColour (TextEditor::focusedOutlineColourId, pal.accent);
    hex.setColour (TextEditor::textColourId, pal.text);
    hex.setColour (CaretComponent::caretColourId, pal.accent);
    // Si el cambio vino de otro lado (cuadro, muestras, RGB) el texto se actualiza siempre:
    // así al salir del campo no regresa el color viejo
    if (! fromHex || ! hex.hasKeyboardFocus (false))
        hex.setText ("#" + current.toDisplayString (false), false);
    rBox.setValue (current.getRed());
    gBox.setValue (current.getGreen());
    bBox.setValue (current.getBlue());
}

void PrecisePicker::fromHSV (bool notify)
{
    current = Colour::fromHSV (hue, sat, val, 1.0f);
    syncFields();
    repaint();
    if (notify && onColour) onColour (current);
}

Rectangle<float> PrecisePicker::swatchRect (int i) const
{
    const auto& sw = swatches();
    const float d = 22.0f;
    const float gap = ((float) getWidth() - d * (float) sw.size()) / (float) jmax (1, (int) sw.size() - 1);
    return { (float) i * (d + gap), 0.0f, d, d };
}

void PrecisePicker::resized()
{
    const float w = (float) getWidth();
    svArea = { 0.0f, 34.0f, w - 40.0f, 170.0f };
    hueArea = { w - 26.0f, 34.0f, 26.0f, 170.0f };
    prevArea = { 0.0f, 216.0f, 58.0f, 34.0f };
    hex.setBounds (66, 216, 104, 34);
    const int bw = (getWidth() - 178 - 12) / 3;
    rBox.setBounds (178, 216, bw, 34);
    gBox.setBounds (178 + bw + 6, 216, bw, 34);
    bBox.setBounds (178 + 2 * (bw + 6), 216, getWidth() - 178 - 2 * (bw + 6), 34);
}

void PrecisePicker::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    // Muestras rápidas
    const auto& sw = swatches();
    for (int i = 0; i < (int) sw.size(); ++i)
    {
        auto r = swatchRect (i);
        g.setColour (sw[(size_t) i]);
        g.fillEllipse (r);
        g.setColour (Colours::black.withAlpha (0.2f));
        g.drawEllipse (r.reduced (0.5f), 1.0f);
        if (current.getARGB() == sw[(size_t) i].getARGB())
        {
            g.setColour (pal.text);
            g.drawEllipse (r.expanded (2.5f), 1.8f);
        }
    }

    // Cuadro de saturación (a lo ancho) y brillo (a lo alto) para el tono actual
    if (std::abs (svImageHue - hue) > 0.0005f || ! svImage.isValid())
    {
        svImage = Image (Image::RGB, 96, 64, false);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 96; ++x)
                svImage.setPixelAt (x, y, Colour::fromHSV (hue, (float) x / 95.0f, 1.0f - (float) y / 63.0f, 1.0f));
        svImageHue = hue;
    }
    {
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (svArea, 10.0f);
        g.reduceClipRegion (clip);
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImage (svImage, svArea);
    }
    g.setColour (pal.text.withAlpha (0.15f));
    g.drawRoundedRectangle (svArea.reduced (0.5f), 10.0f, 1.0f);
    {
        const Point<float> p (svArea.getX() + sat * svArea.getWidth(), svArea.getY() + (1.0f - val) * svArea.getHeight());
        g.setColour (Colours::black.withAlpha (0.5f));
        g.drawEllipse (p.x - 8.0f, p.y - 8.0f, 16.0f, 16.0f, 3.0f);
        g.setColour (Colours::white);
        g.drawEllipse (p.x - 8.0f, p.y - 8.0f, 16.0f, 16.0f, 1.8f);
        // Cruz fina para saber exactamente dónde estás
        g.setColour (Colours::white.withAlpha (0.35f));
        g.fillRect (p.x - 0.5f, svArea.getY(), 1.0f, p.y - 9.0f - svArea.getY());
        g.fillRect (p.x - 0.5f, p.y + 9.0f, 1.0f, svArea.getBottom() - p.y - 9.0f);
    }

    // Tira de tono
    {
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (hueArea, 8.0f);
        g.reduceClipRegion (clip);
        for (int i = 0; i < (int) hueArea.getHeight(); ++i)
        {
            g.setColour (Colour::fromHSV ((float) i / hueArea.getHeight(), 1.0f, 1.0f, 1.0f));
            g.fillRect (hueArea.getX(), hueArea.getY() + (float) i, hueArea.getWidth(), 1.0f);
        }
    }
    {
        const float y = hueArea.getY() + hue * hueArea.getHeight();
        auto knob = Rectangle<float> (hueArea.getX() - 3.0f, y - 4.0f, hueArea.getWidth() + 6.0f, 8.0f);
        g.setColour (Colours::black.withAlpha (0.45f));
        g.drawRoundedRectangle (knob.expanded (1.0f), 4.0f, 2.0f);
        g.setColour (Colours::white);
        g.drawRoundedRectangle (knob, 4.0f, 2.0f);
    }

    // Antes y ahora (clic en "antes" para regresar)
    {
        auto pr = prevArea;
        auto left = pr.removeFromLeft (pr.getWidth() * 0.5f);
        g.setColour (original);
        g.fillRoundedRectangle (left.withTrimmedRight (-6.0f), 8.0f);
        g.setColour (current);
        g.fillRoundedRectangle (pr, 8.0f);
        g.fillRect (pr.withWidth (8.0f));
        g.setColour (pal.text.withAlpha (0.2f));
        g.drawRoundedRectangle (prevArea.reduced (0.5f), 8.0f, 1.0f);
    }
    g.setColour (pal.dim.withAlpha (0.85f));
    g.setFont (ctx.fonts.ui (12.5f, 1));
    g.drawText (U8 ("Shift: fino · Rueda: ajuste fino · Clic en el color viejo: regresar"),
                Rectangle<float> (0.0f, 258.0f, (float) getWidth(), 30.0f), Justification::centredLeft, true);
}

void PrecisePicker::mouseDown (const MouseEvent& e)
{
    dragStart = e.position;
    startH = hue; startS = sat; startV = val;
    dragging = svArea.expanded (6.0f).contains (e.position) ? 1 : (hueArea.expanded (6.0f, 6.0f).contains (e.position) ? 2 : 0);
    if (dragging == 1 && ! e.mods.isShiftDown())
    {
        sat = jlimit (0.0f, 1.0f, (e.position.x - svArea.getX()) / svArea.getWidth());
        val = jlimit (0.0f, 1.0f, 1.0f - (e.position.y - svArea.getY()) / svArea.getHeight());
        startS = sat; startV = val; dragStart = e.position;
        fromHSV (true);
    }
    else if (dragging == 2 && ! e.mods.isShiftDown())
    {
        hue = jlimit (0.0f, 0.9999f, (e.position.y - hueArea.getY()) / hueArea.getHeight());
        startH = hue; dragStart = e.position;
        fromHSV (true);
    }
    if (dragging != 0) setMouseCursor (MouseCursor::CrosshairCursor);
}

void PrecisePicker::mouseDrag (const MouseEvent& e)
{
    const float fine = e.mods.isShiftDown() ? 0.2f : 1.0f;
    const auto d = (e.position - dragStart) * fine;
    if (dragging == 1)
    {
        sat = jlimit (0.0f, 1.0f, startS + d.x / svArea.getWidth());
        val = jlimit (0.0f, 1.0f, startV - d.y / svArea.getHeight());
        fromHSV (true);
    }
    else if (dragging == 2)
    {
        hue = jlimit (0.0f, 0.9999f, startH + d.y / hueArea.getHeight());
        fromHSV (true);
    }
}

void PrecisePicker::mouseUp (const MouseEvent& e)
{
    setMouseCursor (MouseCursor::NormalCursor);
    if (dragging != 0) { dragging = 0; return; }
    const auto& sw = swatches();
    for (int i = 0; i < (int) sw.size(); ++i)
        if (swatchRect (i).expanded (2.0f).contains (e.position))
        {
            current = sw[(size_t) i];
            hue = current.getHue(); sat = current.getSaturation(); val = current.getBrightness();
            syncFields(); repaint();
            if (onColour) onColour (current);
            return;
        }
    if (prevArea.withWidth (prevArea.getWidth() * 0.5f).contains (e.position))
    {
        current = original;
        hue = current.getHue(); sat = current.getSaturation(); val = current.getBrightness();
        syncFields(); repaint();
        if (onColour) onColour (current);
    }
}

void PrecisePicker::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const float step = (w.deltaY != 0.0f ? w.deltaY : w.deltaX) * (e.mods.isShiftDown() ? 0.01f : 0.04f);
    if (hueArea.expanded (6.0f).contains (e.position)) hue = std::fmod (hue - step + 1.0f, 1.0f);
    else if (svArea.contains (e.position)) val = jlimit (0.0f, 1.0f, val + step);
    else return;
    fromHSV (true);
}

//==============================================================================
// Editor de colores con varios destinos
//==============================================================================
static StringArray targetNames (const std::vector<ColourTarget>& t)
{
    StringArray s;
    for (auto& x : t) s.add (x.name);
    return s;
}

ColourEditor::ColourEditor (UiContext& c, std::vector<ColourTarget> t, String resetText, std::function<void()> onReset)
    : ctx (c), targets (std::move (t)), tabs (c, targetNames (targets)), picker (c),
      resetBtn (c, "restore", resetText, resetText), resetFn (std::move (onReset))
{
    addAndMakeVisible (tabs);
    tabs.setVisible (targets.size() > 1);
    addAndMakeVisible (picker);
    addAndMakeVisible (resetBtn);
    tabs.onChange = [this] (int) { refresh(); };
    picker.onColour = [this] (Colour col)
    {
        const int i = jlimit (0, (int) targets.size() - 1, tabs.getSelected());
        targets[(size_t) i].set (col);
    };
    resetBtn.onClick = [this] { if (resetFn) resetFn(); refresh(); };
    refresh();
}

void ColourEditor::refresh()
{
    const int i = jlimit (0, (int) targets.size() - 1, tabs.getSelected());
    const auto own = targets[(size_t) i].get();
    picker.setColour (own.isTransparent() ? targets[(size_t) i].base() : own);
}

void ColourEditor::resized()
{
    auto b = getLocalBounds();
    tabs.setBounds (b.removeFromTop (38));
    b.removeFromTop (14);
    picker.setBounds (b.removeFromTop (picker.getHeight()));
    b.removeFromTop (8);
    resetBtn.setBounds (b.removeFromTop (42));
}

void ColourEditor::paint (Graphics& g)
{
    if (targets.size() == 1)
    {
        g.setColour (ctx.vis.pal.dim);
        g.setFont (ctx.fonts.ui (14.0f, 2));
        g.drawText (targets[0].name, Rectangle<int> (0, 0, getWidth(), 38), Justification::centredLeft, false);
    }
}

//==============================================================================
// Lista de opciones
//==============================================================================
ChipGroup& OptionsList::chips (const String& label, StringArray names, int selected, std::function<void (int)> fn)
{
    auto c = std::make_unique<ChipGroup> (ctx, std::move (names));
    c->setSelected (selected);
    c->onChange = std::move (fn);
    auto& ref = *c;
    addAndMakeVisible (*c);
    rows.push_back ({ label, std::move (c), 0, 0, {} });
    return ref;
}

MiniSlider& OptionsList::slider (const String& label, float lo, float hi, float def, float value,
                                 std::function<String (float)> fmt, std::function<void (float)> fn)
{
    auto s = std::make_unique<MiniSlider> (ctx, lo, hi, def, std::move (fmt));
    s->setValue (value);
    s->onChange = std::move (fn);
    auto& ref = *s;
    addAndMakeVisible (*s);
    rows.push_back ({ label, std::move (s), 30, 1, {} });
    return ref;
}

Switch& OptionsList::toggle (const String& label, bool on, std::function<void (bool)> fn)
{
    auto s = std::make_unique<Switch> (ctx);
    s->setOn (on);
    s->onChange = std::move (fn);
    auto& ref = *s;
    addAndMakeVisible (*s);
    rows.push_back ({ label, std::move (s), 30, 2, {} });
    return ref;
}

void OptionsList::custom (const String& label, std::unique_ptr<Component> comp, int height)
{
    addAndMakeVisible (*comp);
    rows.push_back ({ label, std::move (comp), height, 3, {} });
}

int OptionsList::layout (int width)
{
    int y = 0;
    for (auto& r : rows)
    {
        if (r.kind == 2)   // interruptor: título a la izquierda, switch a la derecha
        {
            r.labelArea = { 0, y, width - 70, 34 };
            r.comp->setBounds (width - 58, y + 2, 54, 30);
            y += 34 + 14;
            continue;
        }
        if (r.label.isNotEmpty()) { r.labelArea = { 0, y, width, 20 }; y += 24; }
        else r.labelArea = {};
        int h = r.height;
        if (auto* cg = dynamic_cast<ChipGroup*> (r.comp.get())) h = cg->heightFor (width);
        else if (auto* hw = dynamic_cast<HeightForWidth*> (r.comp.get())) h = hw->heightFor (width);
        r.comp->setBounds (0, y, width, h);
        y += h + 18;
    }
    setSize (width, y);
    return y;
}

void OptionsList::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    for (auto& r : rows)
    {
        if (r.labelArea.isEmpty()) continue;
        g.setColour (r.kind == 2 ? pal.text : pal.dim);
        g.setFont (ctx.fonts.ui (r.kind == 2 ? 15.0f : 14.0f, r.kind == 2 ? 1 : 2));
        g.drawText (r.label, r.labelArea, Justification::centredLeft, true);
    }
}

//==============================================================================
// Piezas internas de los paneles
//==============================================================================
namespace
{
    // Página: una lista de opciones que se desplaza si no cabe
    class Page : public Component
    {
    public:
        Page (UiContext& c, std::unique_ptr<OptionsList> l) : ctx (c), list (std::move (l))
        {
            view.setViewedComponent (list.get(), false);
            view.setScrollBarsShown (true, false);
            view.setScrollBarThickness (7);
            addAndMakeVisible (view);
        }
        void resized() override
        {
            view.setBounds (getLocalBounds());
            const int w = getWidth() - 12;
            const int h = list->layout (w);
            list->setTopLeftPosition (0, 0);
            if (onHeight && h != lastH) { lastH = h; onHeight (h); }
        }
        int naturalHeight (int width) { return list->layout (width - 12); }
        std::function<void (int)> onHeight;   // cuando crece o se encoge (por ejemplo al abrir el selector de color)
    private:
        UiContext& ctx;
        std::unique_ptr<OptionsList> list;
        Viewport view;
        int lastH = -1;
    };

    void relayoutPage (Component* c)
    {
        if (auto* pg = c->findParentComponentOfClass<Page>())
            pg->resized();
    }

    // Colores en un renglón: "Auto", las muestras y "+" para el selector preciso
    class SwatchRow : public Component, public HeightForWidth
    {
    public:
        SwatchRow (UiContext& c, std::function<Colour()> g, std::function<Colour()> b, std::function<void (Colour)> s, String autoText = "Auto")
            : ctx (c), get (std::move (g)), base (std::move (b)), set (std::move (s)), autoLabel (std::move (autoText)), picker (c)
        {
            addChildComponent (picker);
            picker.onColour = [this] (Colour col) { if (set) set (col); repaint(); };
        }
        int heightFor (int width) override
        {
            layoutDots (width);
            return dotsH + (open ? 12 + picker.getHeight() : 0);
        }
        void resized() override
        {
            layoutDots (getWidth());
            picker.setBounds (0, dotsH + 12, getWidth(), 292);
            picker.setVisible (open);
        }
        void paint (Graphics& g) override
        {
            const auto& pal = ctx.vis.pal;
            const auto cur = get ? get() : Colour();
            const auto& sw = swatches();
            for (int i = 0; i < (int) items.size(); ++i)
            {
                auto r = items[(size_t) i];
                const bool hov = i == hover;
                if (i == 0)
                {
                    // Auto: el color del diseño
                    const bool sel = cur.isTransparent();
                    g.setColour (sel ? pal.accent : (pal.night ? pal.plate.brighter (0.12f) : pal.plate));
                    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
                    if (! sel) { g.setColour (pal.text.withAlpha (hov ? 0.32f : 0.14f)); g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f); }
                    auto dot = r.reduced (5.0f).removeFromLeft (r.getHeight() - 10.0f);
                    g.setColour (base ? base() : pal.accent);
                    g.fillEllipse (dot);
                    g.setColour (sel ? pal.accentInk : pal.text);
                    g.setFont (ctx.fonts.ui (13.0f, sel ? 2 : 1));
                    g.drawText (autoLabel, r.withTrimmedLeft (r.getHeight() - 2.0f), Justification::centredLeft, false);
                    continue;
                }
                if (i == (int) items.size() - 1)
                {
                    // + : selector preciso
                    g.setColour (open ? pal.accent : (pal.night ? pal.plate.brighter (0.12f) : pal.plate));
                    g.fillEllipse (r);
                    g.setColour (open ? pal.accentInk : pal.text.withAlpha (hov ? 0.9f : 0.6f));
                    draw::icon (g, open ? "minus" : "plus", r.reduced (4.0f), open ? pal.accentInk : pal.text.withAlpha (0.75f), 1.6f);
                    continue;
                }
                const auto c = sw[(size_t) (i - 1)];
                g.setColour (c);
                g.fillEllipse (r.reduced (hov ? 0.0f : 1.0f));
                g.setColour (Colours::black.withAlpha (0.18f));
                g.drawEllipse (r.reduced (1.0f), 1.0f);
                if (! cur.isTransparent() && cur.getARGB() == c.getARGB())
                {
                    g.setColour (pal.text);
                    g.drawEllipse (r.expanded (2.5f), 1.8f);
                }
            }
            // El color propio que no es una muestra
            if (! cur.isTransparent() && std::none_of (sw.begin(), sw.end(), [&] (Colour c) { return c.getARGB() == cur.getARGB(); }) && ! items.empty())
            {
                auto r = items.back();
                g.setColour (cur);
                g.fillEllipse (r.withSizeKeepingCentre (8.0f, 8.0f).translated (r.getWidth() * 0.32f, -r.getHeight() * 0.32f));
            }
        }
        void mouseMove (const MouseEvent& e) override
        {
            int h = -1;
            for (int i = 0; i < (int) items.size(); ++i) if (items[(size_t) i].expanded (2.0f).contains (e.position)) h = i;
            if (h != hover) { hover = h; setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); }
        }
        void mouseExit (const MouseEvent&) override { hover = -1; repaint(); }
        void mouseUp (const MouseEvent& e) override
        {
            for (int i = 0; i < (int) items.size(); ++i)
                if (items[(size_t) i].expanded (2.0f).contains (e.position))
                {
                    if (i == 0) { if (set) set (Colour()); }
                    else if (i == (int) items.size() - 1)
                    {
                        open = ! open;
                        const auto cur = get ? get() : Colour();
                        picker.setColour (cur.isTransparent() && base ? base() : cur);
                        resized();
                        relayoutPage (this);
                    }
                    else if (set) set (swatches()[(size_t) (i - 1)]);
                    repaint();
                    return;
                }
        }
    private:
        void layoutDots (int width)
        {
            items.clear();
            const float d = 24.0f, gap = 7.0f;
            float x = 0.0f, y = 0.0f;
            auto put = [&] (float w)
            {
                if (x > 0.0f && x + w > (float) width) { x = 0.0f; y += d + 8.0f; }
                items.push_back ({ x, y, w, d });
                x += w + gap;
            };
            put (draw::textWidth (ctx.fonts.ui (13.0f, 2), autoLabel) + d + 12.0f);
            for (size_t i = 0; i < swatches().size(); ++i) put (d);
            put (d);
            dotsH = (int) std::ceil (y + d) + 2;
        }
        UiContext& ctx;
        std::function<Colour()> get, base;
        std::function<void (Colour)> set;
        String autoLabel;
        PrecisePicker picker;
        std::vector<Rectangle<float>> items;
        int dotsH = 26, hover = -1;
        bool open = false;
    };

    // Una nota de texto
    class NoteRow : public Component, public HeightForWidth
    {
    public:
        NoteRow (UiContext& c, String t) : ctx (c), text (std::move (t)) {}
        int heightFor (int width) override
        {
            AttributedString a; a.append (text, ctx.fonts.ui (13.0f, 1)); a.setWordWrap (AttributedString::byWord);
            TextLayout tl; tl.createLayout (a, (float) width);
            return (int) std::ceil (tl.getHeight()) + 2;
        }
        void paint (Graphics& g) override
        {
            g.setColour (ctx.vis.pal.dim);
            g.setFont (ctx.fonts.ui (13.0f, 1));
            g.drawFittedText (text, getLocalBounds(), Justification::topLeft, 4, 1.0f);
        }
    private:
        UiContext& ctx;
        String text;
    };

    // "Más ajustes": abre o cierra las opciones de menos uso
    class MoreRow : public Component
    {
    public:
        MoreRow (UiContext& c, bool isOpen, std::function<void()> fn) : ctx (c), open (isOpen), onClick (std::move (fn))
        { setMouseCursor (MouseCursor::PointingHandCursor); }
        void paint (Graphics& g) override
        {
            const auto& pal = ctx.vis.pal;
            g.setColour (pal.text.withAlpha (0.08f));
            g.fillRect (0, 0, getWidth(), 1);
            auto r = getLocalBounds().toFloat().withTrimmedTop (4.0f);
            g.setColour (hover ? pal.accent : pal.text.withAlpha (0.8f));
            g.setFont (ctx.fonts.ui (14.0f, 2));
            g.drawText (open ? U8 ("Menos ajustes") : U8 ("Más ajustes"), r, Justification::centredLeft, false);
            const float tw = draw::textWidth (ctx.fonts.ui (14.0f, 2), open ? U8 ("Menos ajustes") : U8 ("Más ajustes"));
            Graphics::ScopedSaveState ss (g);
            auto ir = Rectangle<float> (tw + 6.0f, r.getCentreY() - 8.0f, 16.0f, 16.0f);
            if (open) g.addTransform (AffineTransform::rotation (MathConstants<float>::pi, ir.getCentreX(), ir.getCentreY()));
            draw::icon (g, "chev", ir, hover ? pal.accent : pal.text.withAlpha (0.8f), 1.8f);
        }
        void mouseEnter (const MouseEvent&) override { hover = true; repaint(); }
        void mouseExit (const MouseEvent&) override { hover = false; repaint(); }
        void mouseUp (const MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
    private:
        UiContext& ctx;
        bool open, hover = false;
        std::function<void()> onClick;
    };

    // Un botón dentro de la lista
    class ButtonRow : public Component
    {
    public:
        ButtonRow (UiContext& c, String icon, String text, std::function<void()> fn) : btn (c, icon, text, text)
        {
            btn.onClick = std::move (fn);
            addAndMakeVisible (btn);
        }
        void resized() override { btn.setBounds (getLocalBounds().withWidth (jmin (getWidth(), 260))); }
        PushButton btn;
    };

    // Un aparato chiquito con los colores de un estilo o tema (para elegir viendo)
    void drawMiniDevice (Graphics& g, UiContext& ctx, Rectangle<float> dev, const Skin& s)
    {
        draw::device (g, dev, s, 0.3f, false);
        auto in = dev.reduced (dev.getWidth() * 0.08f, dev.getHeight() * 0.13f);
        auto top = in.removeFromTop (in.getHeight() * 0.6f);
        auto circ = top.removeFromRight (top.getHeight());
        top.removeFromRight (in.getWidth() * 0.05f);
        // Pantalla
        const auto sl = pieces::screenLook (s);
        const auto sp = draw::shapePath (top, s.pieces.scrShape == 3 ? 3 : (s.pieces.scrShape == 2 ? 2 : (s.pieces.scrShape == 1 ? 1 : 0)), 4.0f);
        g.setColour (sl.bg);
        g.fillPath (sp);
        const float th = top.getHeight() * 0.56f;
        g.setColour (sl.text);
        g.setFont (sl.font == 0 ? (s.pixelScreen ? ctx.fonts.pixel (th * 1.1f) : ctx.fonts.ui (th, 2)) : ctx.fonts.display (sl.font, s.pixelScreen, th));
        g.drawText (Fonts::displayText (sl.font, "Dm"), top, Justification::centred, false);
        // Círculo
        auto c = circ.reduced (1.0f);
        g.setColour (s.plate.darker (s.night ? 0.0f : 0.04f));
        g.fillEllipse (c);
        Path wedge;
        wedge.addPieSegment (c, -0.27f, 0.27f, 0.45f);
        g.setColour (pieces::circleColour (s));
        g.fillPath (wedge);
        g.setColour (s.caps[0]);
        g.fillEllipse (c.reduced (c.getWidth() * 0.29f));
        // Teclas
        auto keys = in.withTrimmedTop (in.getHeight() * 0.18f);
        const float kw = keys.getWidth() / 7.0f;
        for (int i = 0; i < 7; ++i)
        {
            g.setColour (i == 2 || i == 4 ? s.noteColour (i == 2 ? 64 : 67) : s.keyW);
            g.fillRoundedRectangle (keys.getX() + (float) i * kw + 0.8f, keys.getY(), kw - 1.6f, keys.getHeight(), 1.5f);
        }
        for (int i : { 1, 2, 4, 5, 6 })
        {
            g.setColour (s.keyB);
            g.fillRoundedRectangle (keys.getX() + (float) i * kw - kw * 0.28f, keys.getY(), kw * 0.56f, keys.getHeight() * 0.55f, 1.2f);
        }
    }

    // Los estilos, con una muestra chiquita de cada uno
    class LookTiles : public Component, public HeightForWidth
    {
    public:
        LookTiles (UiContext& c, Prefs& p, std::function<void()> changed) : ctx (c), prefs (p), onChanged (std::move (changed)) {}
        int heightFor (int width) override
        {
            const int cols = jmax (1, width / 100);
            const int n = (int) looks().size();
            return ((n + cols - 1) / cols) * 86;
        }
        Rectangle<float> tile (int i) const
        {
            const int cols = jmax (1, getWidth() / 100);
            const float w = ((float) getWidth() - 8.0f * (float) (cols - 1)) / (float) cols;
            return { (float) (i % cols) * (w + 8.0f), (float) (i / cols) * 86.0f, w, 80.0f };
        }
        void paint (Graphics& g) override
        {
            const auto& pal = ctx.vis.pal;
            const auto cur = lookIndex (prefs.look);
            for (int i = 0; i < (int) looks().size(); ++i)
            {
                auto r = tile (i);
                const Skin s = prefs.palette (looks()[(size_t) i].id);
                auto dev = r.withTrimmedBottom (20.0f).reduced (3.0f, 3.0f);
                drawMiniDevice (g, ctx, dev, s);
                g.setColour (i == cur ? pal.accent : pal.text.withAlpha (i == hover ? 0.4f : 0.12f));
                g.drawRoundedRectangle (dev.expanded (2.5f), 10.0f, i == cur ? 2.5f : 1.0f);
                g.setColour (i == cur ? pal.text : pal.dim);
                g.setFont (ctx.fonts.ui (13.0f, i == cur ? 2 : 1));
                g.drawText (looks()[(size_t) i].name, r.withTop (r.getBottom() - 18.0f), Justification::centred, false);
            }
        }
        void mouseMove (const MouseEvent& e) override
        {
            int h = -1;
            for (int i = 0; i < (int) looks().size(); ++i) if (tile (i).contains (e.position)) h = i;
            if (h != hover) { hover = h; setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); }
        }
        void mouseExit (const MouseEvent&) override { hover = -1; repaint(); }
        void mouseUp (const MouseEvent& e) override
        {
            for (int i = 0; i < (int) looks().size(); ++i)
                if (tile (i).contains (e.position))
                {
                    prefs.look = looks()[(size_t) i].id;
                    prefs.keyStyle = -1;   // las teclas del estilo
                    prefs.save();
                    repaint();
                    if (onChanged) onChanged();
                    return;
                }
        }
    private:
        UiContext& ctx;
        Prefs& prefs;
        std::function<void()> onChanged;
        int hover = -1;
    };

    // Temas listos de un grupo (un clic y se pone todo)
    class ThemeTiles : public Component, public HeightForWidth
    {
    public:
        ThemeTiles (UiContext& c, Prefs& p, std::vector<const ThemePreset*> t, std::function<void (const ThemePreset&)> fn)
            : ctx (c), prefs (p), items (std::move (t)), apply (std::move (fn)) {}
        int heightFor (int width) override
        {
            const int cols = jmax (2, width / 100);
            return (((int) items.size() + cols - 1) / cols) * 84;
        }
        Rectangle<float> tile (int i) const
        {
            const int cols = jmax (2, getWidth() / 100);
            const float w = ((float) getWidth() - 8.0f * (float) (cols - 1)) / (float) cols;
            return { (float) (i % cols) * (w + 8.0f), (float) (i / cols) * 84.0f, w, 78.0f };
        }
        void paint (Graphics& g) override
        {
            const auto& pal = ctx.vis.pal;
            for (int i = 0; i < (int) items.size(); ++i)
            {
                auto r = tile (i);
                const auto s = prefs.paletteFor (*items[(size_t) i]);
                auto dev = r.withTrimmedBottom (20.0f).reduced (3.0f);
                drawMiniDevice (g, ctx, dev, s);
                g.setColour (pal.text.withAlpha (i == hover ? 0.45f : 0.12f));
                g.drawRoundedRectangle (dev.expanded (2.5f), 10.0f, i == hover ? 2.0f : 1.0f);
                g.setColour (i == hover ? pal.text : pal.dim);
                g.setFont (ctx.fonts.ui (13.0f, i == hover ? 2 : 1));
                g.drawFittedText (items[(size_t) i]->name, r.withTop (r.getBottom() - 18.0f).toNearestInt(), Justification::centred, 1, 0.8f);
            }
        }
        void mouseMove (const MouseEvent& e) override
        {
            int h = -1;
            for (int i = 0; i < (int) items.size(); ++i) if (tile (i).contains (e.position)) h = i;
            if (h != hover) { hover = h; setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); }
        }
        void mouseExit (const MouseEvent&) override { hover = -1; repaint(); }
        void mouseUp (const MouseEvent& e) override
        {
            for (int i = 0; i < (int) items.size(); ++i)
                if (tile (i).contains (e.position)) { if (apply) apply (*items[(size_t) i]); return; }
        }
    private:
        UiContext& ctx;
        Prefs& prefs;
        std::vector<const ThemePreset*> items;
        std::function<void (const ThemePreset&)> apply;
        int hover = -1;
    };

    // Tus temas guardados (se aplican con un clic) y el botón para guardar uno nuevo
    class ThemesBox : public Component, public HeightForWidth
    {
    public:
        ThemesBox (UiContext& c, Prefs& p, PanelHooks h, std::function<void()> applied)
            : ctx (c), prefs (p), hooks (std::move (h)), onApplied (std::move (applied)),
              saveBtn (c, "save", U8 ("Guarda todo como un tema tuyo: estilo, colores, cada pieza, movimiento, teclas y acomodo"), "Guardar tema"),
              okBtn (c, "save", "Guardar", "Guardar"), cancelBtn (c, "close", "Cancelar")
        {
            addAndMakeVisible (saveBtn);
            addChildComponent (nameEditor);
            addChildComponent (okBtn);
            addChildComponent (cancelBtn);
            okBtn.setActive (true);
            nameEditor.setFont (ctx.fonts.ui (15.0f, 1));
            nameEditor.setIndents (10, 0);
            nameEditor.setJustification (Justification::centredLeft);
            auto commit = [this]
            {
                const auto name = nameEditor.getText().trim();
                if (name.isEmpty()) return;
                auto t = hooks.captureTheme ? hooks.captureTheme (name) : prefs.captureTheme (name);
                bool replaced = false;
                for (auto& th : prefs.themes) if (th.name == name) { th = t; replaced = true; }
                if (! replaced) prefs.themes.push_back (t);
                prefs.save (false);
                setNaming (false);
                if (onApplied) onApplied();
            };
            nameEditor.onReturnKey = commit;
            okBtn.onClick = commit;
            cancelBtn.onClick = [this] { setNaming (false); };
            saveBtn.onClick = [this] { setNaming (true); };
        }
        void setNaming (bool n)
        {
            nameEditor.setVisible (n); okBtn.setVisible (n); cancelBtn.setVisible (n); saveBtn.setVisible (! n);
            const auto& pal = ctx.vis.pal;
            nameEditor.setColour (TextEditor::backgroundColourId, pal.night ? pal.plate.brighter (0.1f) : Colours::white);
            nameEditor.setColour (TextEditor::textColourId, pal.text);
            nameEditor.setColour (TextEditor::outlineColourId, pal.edge);
            nameEditor.setColour (TextEditor::focusedOutlineColourId, pal.accent);
            if (n)
            {
                nameEditor.setText ("Mi tema " + String ((int) prefs.themes.size() + 1), false);
                nameEditor.grabKeyboardFocus();
                nameEditor.selectAll();
            }
            resized();
            repaint();
        }
        Rectangle<float> chip (int i) const
        {
            const float gap = 8.0f;
            const int perRow = jmax (1, (int) (((float) getWidth() + gap) / (140.0f + gap)));
            const float w = ((float) getWidth() - gap * (float) (perRow - 1)) / (float) perRow;
            return { (float) (i % perRow) * (w + gap), 50.0f + (float) (i / perRow) * 44.0f, w, 38.0f };
        }
        int heightFor (int width) override
        {
            setSize (width, 10);
            const int n = (int) prefs.themes.size();
            return n == 0 ? 76 : (int) chip (n - 1).getBottom() + 4;
        }
        void resized() override
        {
            saveBtn.setBounds (0, 0, jmin (getWidth(), 190), 40);
            nameEditor.setBounds (0, 0, getWidth() - 150, 40);
            okBtn.setBounds (getWidth() - 144, 0, 100, 40);
            cancelBtn.setBounds (getWidth() - 40, 0, 40, 40);
        }
        void paint (Graphics& g) override
        {
            const auto& pal = ctx.vis.pal;
            if (prefs.themes.empty())
            {
                g.setColour (pal.dim);
                g.setFont (ctx.fonts.ui (13.0f, 1));
                g.drawText (U8 ("Aquí aparecen los temas que guardes."), Rectangle<float> (0.0f, 48.0f, (float) getWidth(), 24.0f), Justification::centredLeft, true);
            }
            for (int i = 0; i < (int) prefs.themes.size(); ++i)
            {
                const auto& t = prefs.themes[(size_t) i];
                auto r = chip (i);
                g.setColour (pal.night ? pal.plate.brighter (0.12f) : pal.plate);
                g.fillRoundedRectangle (r, r.getHeight() * shapedRadius (pal, 0.3f));
                auto in = r.reduced (10.0f, 0.0f);
                auto x = in.removeFromRight (16.0f);
                const auto s = prefs.paletteFor (t);
                auto dots = in.removeFromLeft (30.0f);
                g.setColour (s.body2); g.fillEllipse (dots.getX(), dots.getCentreY() - 7.0f, 14.0f, 14.0f);
                g.setColour (s.accent); g.fillEllipse (dots.getX() + 10.0f, dots.getCentreY() - 7.0f, 14.0f, 14.0f);
                const bool ask = confirmDelete == i;
                g.setColour (ask ? Colour (0xffe5383b) : pal.text);
                g.setFont (ctx.fonts.ui (13.5f, 1));
                g.drawFittedText (ask ? U8 ("¿Borrar?") : t.name, in.toNearestInt(), Justification::centredLeft, 1, 0.8f);
                draw::icon (g, "close", x.withSizeKeepingCentre (13.0f, 13.0f), ask ? Colour (0xffe5383b) : pal.text.withAlpha (0.45f), 1.4f);
            }
        }
        void mouseUp (const MouseEvent& e) override
        {
            for (int i = 0; i < (int) prefs.themes.size(); ++i)
            {
                auto r = chip (i);
                if (! r.contains (e.position)) continue;
                if (e.position.x > r.getRight() - 28.0f)
                {
                    if (confirmDelete == i) { prefs.themes.erase (prefs.themes.begin() + i); prefs.save (false); confirmDelete = -1; if (onApplied) onApplied(); }
                    else confirmDelete = i;
                    repaint();
                    return;
                }
                confirmDelete = -1;
                const auto t = prefs.themes[(size_t) i];
                if (hooks.applyTheme) hooks.applyTheme (t); else prefs.applyTheme (t);
                if (onApplied) onApplied();
                return;
            }
            if (confirmDelete >= 0) { confirmDelete = -1; repaint(); }
        }
    private:
        UiContext& ctx;
        Prefs& prefs;
        PanelHooks hooks;
        std::function<void()> onApplied;
        PushButton saveBtn, okBtn, cancelBtn;
        TextEditor nameEditor;
        int confirmDelete = -1;
    };

    // Muestra de cómo se ven las teclas: se dibuja con el mismo código del teclado
    class KeysPreview : public Component
    {
    public:
        KeysPreview (UiContext& c, Prefs& p) : ctx (c), prefs (p) {}
        void paint (Graphics& g) override
        {
            KeyboardSection::paintPreview (g, ctx, prefs.palette(), getLocalBounds().toFloat(), 0.62f);
        }
    private:
        UiContext& ctx;
        Prefs& prefs;
    };

    String pct (float v) { return String (roundToInt (v * 100.0f)) + " %"; }

    // Un color global cambia todo: se quitan los colores propios de cada pieza
    void clearPieceColours (Prefs& P)
    {
        P.keyLit = Colour();
        P.pieces.degColour = Colour();
        P.pieces.cirColour = Colour();
    }
}

//==============================================================================
// Pestañas arriba
//==============================================================================
Rectangle<float> TopTabs::item (int i) const
{
    const float w = (float) getWidth() / (float) jmax (1, names.size());
    return { (float) i * w, 0.0f, w, (float) getHeight() };
}

void TopTabs::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto all = getLocalBounds().toFloat();
    g.setColour (pal.night ? pal.plate.brighter (0.08f) : pal.plate.withAlpha (0.85f));
    g.fillRoundedRectangle (all, all.getHeight() * shapedRadius (pal, 0.5f));
    for (int i = 0; i < names.size(); ++i)
    {
        auto r = item (i).reduced (3.0f);
        const float rad = r.getHeight() * shapedRadius (pal, 0.5f);
        if (i == selected) { g.setColour (pal.accent); g.fillRoundedRectangle (r, rad); }
        else if (i == hover) { g.setColour (pal.text.withAlpha (0.07f)); g.fillRoundedRectangle (r, rad); }
        g.setColour (i == selected ? pal.accentInk : pal.text.withAlpha (0.85f));
        g.setFont (ctx.fonts.ui (14.0f, i == selected ? 2 : 1));
        g.drawText (names[i], r, Justification::centred, false);
    }
}

void TopTabs::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < names.size(); ++i) if (item (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); }
}

void TopTabs::mouseUp (const MouseEvent& e)
{
    for (int i = 0; i < names.size(); ++i)
        if (item (i).contains (e.position) && i != selected)
        {
            selected = i;
            repaint();
            if (onChange) onChange (i);
            return;
        }
}

//==============================================================================
// Pestañas en columna (ya casi no se usan; se quedan por si acaso)
//==============================================================================
void SideTabs::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    for (int i = 0; i < names.size(); ++i)
    {
        auto r = item (i);
        const float rad = r.getHeight() * shapedRadius (pal, 0.3f);
        if (i == selected) { g.setColour (pal.accent); g.fillRoundedRectangle (r, rad); }
        else if (i == hover) { g.setColour (pal.text.withAlpha (0.07f)); g.fillRoundedRectangle (r, rad); }
        g.setColour (i == selected ? pal.accentInk : pal.text.withAlpha (0.85f));
        g.setFont (ctx.fonts.ui (14.5f, i == selected ? 2 : 1));
        g.drawText (names[i], r.reduced (14.0f, 0.0f), Justification::centredLeft, false);
    }
}

void SideTabs::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < names.size(); ++i) if (item (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); }
}

void SideTabs::mouseUp (const MouseEvent& e)
{
    for (int i = 0; i < names.size(); ++i)
        if (item (i).contains (e.position) && i != selected)
        {
            selected = i;
            repaint();
            if (onChange) onChange (i);
            return;
        }
}

//==============================================================================
// Panel general: estilo, temas y movimiento
//==============================================================================
StringArray CustomizePanel::tabNames() { return { "Estilo", "Temas", "Movimiento" }; }

static int lastCustomizeTab = 0;
void CustomizePanel::setStartTab (int t) { lastCustomizeTab = jlimit (0, tabNames().size() - 1, t); }

CustomizePanel::CustomizePanel (UiContext& c, Prefs& p, PanelHooks h)
    : ctx (c), prefs (p), hooks (std::move (h)), tabs (c, tabNames())
{
    addAndMakeVisible (tabs);
    tabs.onChange = [this] (int i) { showTab (i); };
    setSize (440, 500);
    tabs.setSelected (lastCustomizeTab);
    showTab (lastCustomizeTab);
}

void CustomizePanel::resized()
{
    tabs.setBounds (0, 0, getWidth(), 38);
    if (page != nullptr) page->setBounds (0, 50, getWidth(), getHeight() - 50);
}

void CustomizePanel::paint (Graphics&) {}

void CustomizePanel::showTab (int tab)
{
    lastCustomizeTab = tab;
    if (page != nullptr) removeChildComponent (page.get());
    page.reset();
    auto list = std::make_unique<OptionsList> (ctx);
    auto& P = prefs;
    SafePointer<CustomizePanel> safe (this);
    auto rebuild = [safe] { MessageManager::callAsync ([safe] { if (safe != nullptr) safe->showTab (safe->tabs.getSelected()); }); };
    const int w = 428;

    if (tab == 0)   // Estilo
    {
        auto tiles = std::make_unique<LookTiles> (ctx, P, rebuild);
        const int th = tiles->heightFor (w);
        list->custom ("Estilo", std::move (tiles), th);
        list->chips (U8 ("Día o noche"), { U8 ("Día"), "Noche", "Auto" }, P.dayNight, [&P] (int i) { P.dayNight = i; P.save(); });
        list->custom (U8 ("Color principal (cambia todo MidiCat)"), std::make_unique<SwatchRow> (ctx,
                          [&P] { return P.uiAccent; }, [&P] { return P.palette().accent; },
                          [&P] (Colour col) { P.uiAccent = col; clearPieceColours (P); P.save(); }), 0);
        list->custom (U8 ("Color del cuerpo"), std::make_unique<SwatchRow> (ctx,
                          [&P] { return P.uiTint; }, [&P] { return P.palette().body1; },
                          [&P] (Colour col) { P.uiTint = col; P.uiTintExact = ! col.isTransparent(); P.save(); }), 0);
        {
            const auto& qp = quickPalettes();
            StringArray names;
            for (auto& q : qp) names.add (q.name);
            int sel = -1;
            for (int i = 0; i < (int) qp.size(); ++i)
                if ((i == 0 && P.uiTint.isTransparent() && P.uiAccent.isTransparent()) || (i > 0 && ! P.uiTintExact && P.uiTint == qp[(size_t) i].tint && P.uiAccent == qp[(size_t) i].accent)) sel = i;
            auto& cg = list->chips (U8 ("Paletas rápidas"), names, sel, [&P, rebuild] (int i)
            {
                const auto& q = quickPalettes()[(size_t) i];
                P.uiTint = q.tint; P.uiAccent = q.accent; P.uiTintExact = false;
                clearPieceColours (P);
                P.save();
                rebuild();
            });
            cg.decorateW = 30.0f;
            auto* ctxp = &ctx;
            cg.decorate = [ctxp] (Graphics& g, Rectangle<float> r, int i)
            {
                const auto& q = quickPalettes()[(size_t) i];
                const auto& pal = ctxp->vis.pal;
                g.setColour (q.tint.isTransparent() ? pal.body2 : q.tint);
                g.fillEllipse (r.getX(), r.getCentreY() - 7.0f, 14.0f, 14.0f);
                g.setColour (q.accent.isTransparent() ? pal.accent : q.accent);
                g.fillEllipse (r.getX() + 11.0f, r.getCentreY() - 7.0f, 14.0f, 14.0f);
            };
        }
        list->slider (U8 ("Colores suaves (más grises, descansan la vista)"), 0.0f, 1.0f, 0.0f, P.soft, pct,
                      [&P] (float v) { P.soft = v; P.save(); });
        list->chips ("Forma de los botones", buttonShapeNames(), P.pieces.btnShape, [&P] (int i) { P.pieces.btnShape = i; P.save(); });
        list->custom ({}, std::make_unique<NoteRow> (ctx, U8 ("Para cambiar una sola pieza (pantalla, grados, círculo, pentagrama o teclas) usa el botoncito de ajustes que tiene en su esquina.")), 0);
    }
    else if (tab == 1)   // Temas
    {
        StringArray groups;
        for (auto& t : builtinThemes()) groups.addIfNotAlreadyThere (t.group);
        auto apply = [this, rebuild] (const ThemePreset& t)
        {
            if (hooks.applyTheme) hooks.applyTheme (t); else prefs.applyTheme (t);
            rebuild();
        };
        for (auto& gname : groups)
        {
            std::vector<const ThemePreset*> items;
            for (auto& t : builtinThemes()) if (t.group == gname) items.push_back (&t);
            auto tiles = std::make_unique<ThemeTiles> (ctx, P, items, apply);
            const int th = tiles->heightFor (w);
            list->custom ("Temas listos: " + gname, std::move (tiles), th);
        }
        auto themes = std::make_unique<ThemesBox> (ctx, P, hooks, rebuild);
        const int tH = themes->heightFor (w);
        list->custom ("Mis temas (guardan todo: estilo, colores, cada pieza, movimiento y acomodo)", std::move (themes), tH);
    }
    else   // Movimiento
    {
        const auto& pc = P.pieces;
        list->toggle ("Animaciones", pc.anim, [&P] (bool on) { P.pieces.anim = on; P.save(); });
        list->slider ("Velocidad de las animaciones", 0.4f, 2.5f, 1.0f, pc.animSpeed, [] (float v) { return String (v, 1) + "x"; },
                      [&P] (float v) { P.pieces.animSpeed = v; P.save(); });
        list->chips ("Teclas al tocar", keyMotionNames(), pc.keyMotion, [&P] (int i) { P.pieces.keyMotion = i; P.save(); });
        list->slider ("Destello de las teclas", 0.0f, 1.0f, 0.5f, pc.keyGlow, pct, [&P] (float v) { P.pieces.keyGlow = v; P.save(); });
        list->chips (U8 ("Pantalla al cambiar de acorde"), screenMotionNames(), pc.scrMotion, [&P] (int i) { P.pieces.scrMotion = i; P.save(); });
        list->toggle (U8 ("Luz que recorre las líneas del círculo"), pc.cirFlow, [&P] (bool on) { P.pieces.cirFlow = on; P.save(); });
        list->toggle ("Paneles con transiciones suaves", pc.panelMotion, [&P] (bool on) { P.pieces.panelMotion = on; P.save(); });
    }
    page = std::make_unique<Page> (ctx, std::move (list));
    addAndMakeVisible (*page);
    resized();
}

//==============================================================================
// Panel de una sola pieza
//==============================================================================
static std::array<bool, 5> pieceMoreOpen {};

String PiecePanel::titleFor (int piece)
{
    switch (piece)
    {
        case screenPiece:  return "Pantalla";
        case degreesPiece: return "Grados";
        case circlePiece:  return U8 ("Círculo de quintas");
        case staffPiece:   return "Pentagrama";
        default:           return "Teclas";
    }
}

PiecePanel::PiecePanel (UiContext& c, Prefs& p, int pc, PanelHooks h)
    : ctx (c), prefs (p), piece (jlimit (0, 4, pc)), hooks (std::move (h))
{
    setSize (340, 420);
    rebuild();
}

void PiecePanel::resized()
{
    if (page != nullptr) page->setBounds (getLocalBounds());
}

void PiecePanel::rebuild()
{
    if (page != nullptr) removeChildComponent (page.get());
    page.reset();
    auto list = std::make_unique<OptionsList> (ctx);
    auto& P = prefs;
    auto& pc = prefs.pieces;
    SafePointer<PiecePanel> safe (this);
    auto again = [safe] { MessageManager::callAsync ([safe] { if (safe != nullptr) safe->rebuild(); }); };
    const bool more = pieceMoreOpen[(size_t) piece];
    auto moreRow = [&]
    {
        const int pi = piece;
        list->custom ({}, std::make_unique<MoreRow> (ctx, more, [pi, again] { pieceMoreOpen[(size_t) pi] = ! pieceMoreOpen[(size_t) pi]; again(); }), 34);
    };
    auto swatch = [&] (const String& label, std::function<Colour()> get, std::function<Colour()> base, std::function<void (Colour)> set)
    {
        list->custom (label, std::make_unique<SwatchRow> (ctx, std::move (get), std::move (base), std::move (set)), 0);
    };
    std::function<void()> reset;

    switch (piece)
    {
        case screenPiece:
        {
            list->chips (U8 ("Diseño"), screenSkinNames(), pc.scrSkin, [&P, again] (int i) { P.pieces.scrSkin = i; P.save(); again(); });
            auto& letters = list->chips ("Letra", screenFontNames(), pc.scrFont, [&P] (int i) { P.pieces.scrFont = i; P.save(); });
            auto* ctxp = &ctx;
            letters.fontFor = [ctxp] (int i, float h) { return i == 0 ? ctxp->fonts.ui (h * 0.93f, 1) : ctxp->fonts.display (i, false, h * 1.05f); };
            list->chips ("Forma", screenShapeNames(), pc.scrShape, [&P] (int i) { P.pieces.scrShape = i; P.save(); });
            list->chips ("Sin tocar nada", screenIdleNames(), pc.scrIdle, [&P] (int i) { P.pieces.scrIdle = i; P.save(); });
            swatch ("Color de las letras", [&P] { return P.pieces.scrText; }, [&P] { return pieces::screenLook (P.palette()).text; },
                    [&P] (Colour c) { P.pieces.scrText = c; P.save(); });
            moreRow();
            if (more)
            {
                swatch ("Color del fondo", [&P] { return P.pieces.scrBack; }, [&P] { return pieces::screenLook (P.palette()).bg; },
                        [&P] (Colour c) { P.pieces.scrBack = c; P.save(); });
                list->chips ("Textura", screenTextureNames(), pc.scrTexture, [&P] (int i) { P.pieces.scrTexture = i; P.save(); });
                list->slider ("Luces (brillo de las letras)", 0.0f, 1.0f, 0.0f, pieces::screenLook (P.palette()).glow, pct,
                              [&P] (float v) { P.pieces.scrGlow = v; P.save(); });
                list->chips (U8 ("Al cambiar de acorde"), screenMotionNames(), pc.scrMotion, [&P] (int i) { P.pieces.scrMotion = i; P.save(); });
            }
            reset = [&P] { auto& q = P.pieces; q.scrSkin = q.scrFont = q.scrTexture = q.scrMotion = q.scrShape = q.scrIdle = 0;
                           q.scrText = q.scrBack = Colour(); q.scrGlow = -1.0f; P.save(); };
            break;
        }
        case degreesPiece:
        {
            list->chips ("Mostrar", degreeLabelNames(), pc.degLabels, [&P] (int i) { P.pieces.degLabels = i; P.save(); });
            list->chips (U8 ("Diseño"), degreeSkinNames(), pc.degSkin, [&P] (int i) { P.pieces.degSkin = i; P.save(); });
            list->chips ("Forma", degreeShapeNames(), pc.degShape, [&P] (int i) { P.pieces.degShape = i; P.save(); });
            swatch ("Luz del grado que suena", [&P] { return P.pieces.degColour; }, [&P] { return P.palette().accent; },
                    [&P] (Colour c) { P.pieces.degColour = c; P.save(); });
            moreRow();
            if (more)
            {
                auto* hk = &hooks;
                list->custom ("Tocar y grabar los grados con teclas de la compu o pads MIDI",
                              std::make_unique<ButtonRow> (ctx, "keys", U8 ("Teclas y pads…"), [hk] { if (hk->openDegreeInput) hk->openDegreeInput(); }), 40);
            }
            reset = [&P] { auto& q = P.pieces; q.degSkin = q.degShape = q.degLabels = 0; q.degColour = Colour(); P.save(); };
            break;
        }
        case circlePiece:
        {
            list->chips (U8 ("Diseño"), circleSkinNames(), pc.cirSkin, [&P] (int i) { P.pieces.cirSkin = i; P.save(); });
            list->chips (U8 ("Líneas entre las notas"), circleRouteNames(), pc.cirRoute, [&P] (int i) { P.pieces.cirRoute = i; P.save(); });
            list->slider (U8 ("Tamaño del centro"), 0.5f, 1.15f, 1.0f, pc.cirKnob, pct, [&P] (float v) { P.pieces.cirKnob = v; P.save(); });
            swatch (U8 ("Color de la tonalidad y las líneas"), [&P] { return P.pieces.cirColour; }, [&P] { return P.palette().accent; },
                    [&P] (Colour c) { P.pieces.cirColour = c; P.save(); });
            moreRow();
            if (more)
            {
                list->slider (U8 ("Grosor de las líneas"), 0.5f, 2.5f, 1.0f, pc.cirLineW, [] (float v) { return String (v, 1) + "x"; },
                              [&P] (float v) { P.pieces.cirLineW = v; P.save(); });
                list->chips ("Puntos de las notas", circleNodeNames(), pc.cirNodes, [&P] (int i) { P.pieces.cirNodes = i; P.save(); });
                list->chips ("Textura", circleTextureNames(), pc.cirTexture, [&P] (int i) { P.pieces.cirTexture = i; P.save(); });
                list->chips ("Placa", plateShapeNames(), pc.cirShape, [&P] (int i) { P.pieces.cirShape = i; P.save(); });
                list->toggle (U8 ("Luz que recorre las líneas"), pc.cirFlow, [&P] (bool on) { P.pieces.cirFlow = on; P.save(); });
            }
            reset = [&P] { auto& q = P.pieces; q.cirSkin = q.cirTexture = q.cirRoute = q.cirNodes = q.cirShape = 0;
                           q.cirLineW = q.cirKnob = 1.0f; q.cirColour = Colour(); q.cirFlow = true; P.save(); };
            break;
        }
        case staffPiece:
        {
            list->chips (U8 ("Diseño"), staffSkinNames(), pc.stfSkin, [&P, again] (int i) { P.pieces.stfSkin = i; P.save(); again(); });
            list->chips ("Marco", frameShapeNames(), pc.stfShape, [&P] (int i) { P.pieces.stfShape = i; P.save(); });
            swatch ("Tinta", [&P] { return P.pieces.stfInk; }, [&P] { return pieces::staffLook (P.palette()).ink; },
                    [&P] (Colour c) { P.pieces.stfInk = c; P.save(); });
            moreRow();
            if (more)
            {
                swatch ("Papel", [&P] { return P.pieces.stfPaper; }, [&P] { return pieces::staffLook (P.palette()).paper; },
                        [&P] (Colour c) { P.pieces.stfPaper = c; P.save(); });
                list->chips ("Textura", staffTextureNames(), pc.stfTexture, [&P] (int i) { P.pieces.stfTexture = i; P.save(); });
            }
            reset = [&P] { auto& q = P.pieces; q.stfSkin = q.stfTexture = q.stfShape = 0; q.stfInk = q.stfPaper = Colour(); P.save(); };
            break;
        }
        default:   // Teclas
        {
            list->chips ("Forma de las teclas", keyStyleNames(), P.effectiveKeyStyle(), [&P] (int i) { P.keyStyle = i; P.save(); });
            list->toggle (U8 ("Arcoíris: cada nota de un color"), P.rainbow, [&P] (bool on) { P.rainbow = on; P.save(); });
            list->custom ("Muestra", std::make_unique<KeysPreview> (ctx, P), 112);
            swatch ("Encendidas", [&P] { return P.keyLit; }, [&P] { return P.palette().accent; },
                    [&P, again] (Colour c) { P.keyLit = c; if (! c.isTransparent()) P.rainbow = false; P.save(); again(); });
            moreRow();
            if (more)
            {
                swatch ("Blancas", [&P] { return P.keyWhite; }, [&P] { auto s = P.palette(); return P.keyWhite.isTransparent() ? s.keyW : s.keyW; },
                        [&P] (Colour c) { P.keyWhite = c; P.save(); });
                swatch ("Negras", [&P] { return P.keyBlack; }, [&P] { return P.palette().keyB; },
                        [&P] (Colour c) { P.keyBlack = c; P.save(); });
                list->chips ("Al tocar", keyMotionNames(), pc.keyMotion, [&P] (int i) { P.pieces.keyMotion = i; P.save(); });
                list->slider ("Destello", 0.0f, 1.0f, 0.5f, pc.keyGlow, pct, [&P] (float v) { P.pieces.keyGlow = v; P.save(); });
            }
            reset = [&P] { P.keyStyle = -1; P.rainbow = false; P.keyLit = P.keyWhite = P.keyBlack = Colour();
                           P.pieces.keyMotion = 0; P.pieces.keyGlow = 0.5f; P.save(); };
            break;
        }
    }
    list->custom ({}, std::make_unique<ButtonRow> (ctx, "restore", "Volver al estilo", [reset, again] { if (reset) reset(); again(); }), 40);

    auto pg = std::make_unique<Page> (ctx, std::move (list));
    // El panel crece con lo que tiene (hasta un tope) y si no cabe, se desplaza
    const int natural = pg->naturalHeight (getWidth());
    pg->onHeight = [safe] (int h)
    {
        if (safe == nullptr) return;
        const int want = jlimit (160, 470, h + 4);
        if (want != safe->getHeight())
        {
            safe->setSize (safe->getWidth(), want);
            if (auto* p = safe->getParentComponent()) p->resized();
        }
    };
    page = std::move (pg);
    addAndMakeVisible (*page);
    setSize (getWidth(), jlimit (160, 470, natural + 4));
    resized();
    if (auto* p = getParentComponent()) { p->resized(); p->repaint(); }
}

//==============================================================================
// Teclas y pads para los grados
//==============================================================================
bool DegreeInputPanel::capturing = false;

DegreeInputPanel::DegreeInputPanel (UiContext& c, Prefs& p, std::function<int()> lp)
    : ctx (c), prefs (p), lastPad (std::move (lp)), keysOn (c), padsOn (c),
      learnAllBtn (c, "plus", U8 ("Toca tus pads en orden, del I al VII"), "Aprender los 7 pads"),
      clearBtn (c, "trash", "Quitar todos los pads", "Quitar pads"),
      defaultsBtn (c, "restore", "Teclas 1 a 7", "Teclas 1 a 7")
{
    setWantsKeyboardFocus (true);
    for (Component* comp : std::initializer_list<Component*> { &keysOn, &padsOn, &learnAllBtn, &clearBtn, &defaultsBtn })
        addAndMakeVisible (comp);
    keysOn.setOn (prefs.degreeKeysOn);
    padsOn.setOn (prefs.degreePadsOn);
    keysOn.onChange = [this] (bool on) { prefs.degreeKeysOn = on; prefs.save(); };
    padsOn.onChange = [this] (bool on) { prefs.degreePadsOn = on; prefs.save(); };
    learnAllBtn.onClick = [this] { learnAll = true; listen (0, 1); };
    clearBtn.onClick = [this] { for (auto& pd : prefs.degreePads) pd = -1; prefs.save(); listenRow = -1; learnAll = false; repaint(); };
    defaultsBtn.onClick = [this] { for (int i = 0; i < 7; ++i) prefs.degreeKeys[(size_t) i] = '1' + i; prefs.save(); repaint(); };
    lastSeenPad = lastPad ? lastPad() : 0;
    startTimerHz (30);
    setSize (560, 476);
}

DegreeInputPanel::~DegreeInputPanel() { capturing = false; }

Rectangle<float> DegreeInputPanel::slot (int row, int kind) const
{
    const float y = 96.0f + (float) row * 46.0f;
    return kind == 0 ? Rectangle<float> (70.0f, y, 150.0f, 38.0f) : Rectangle<float> (236.0f, y, (float) getWidth() - 236.0f, 38.0f);
}

void DegreeInputPanel::resized()
{
    keysOn.setBounds (184, 6, 54, 30);
    padsOn.setBounds (getWidth() - 56, 6, 54, 30);
    const int y = 96 + 7 * 46 + 8;
    learnAllBtn.setBounds (0, y, 220, 42);
    clearBtn.setBounds (228, y, 160, 42);
    defaultsBtn.setBounds (396, y, getWidth() - 396, 42);
}

void DegreeInputPanel::listen (int row, int kind)
{
    listenRow = row;
    listenKind = kind;
    capturing = row >= 0 && kind == 0;
    lastSeenPad = lastPad ? lastPad() : 0;
    if (capturing) grabKeyboardFocus();
    repaint();
}

void DegreeInputPanel::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (15.0f, 1));
    g.drawText (U8 ("Teclas de la compu"), Rectangle<float> (0.0f, 6.0f, 180.0f, 30.0f), Justification::centredLeft, false);
    g.drawText ("Pads MIDI", Rectangle<float> ((float) getWidth() - 160.0f, 6.0f, 100.0f, 30.0f), Justification::centredLeft, false);
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (13.5f, 1));
    g.drawFittedText (U8 ("Haz clic en una casilla y presiona la tecla (o toca el pad). Suenan y se graban como si tocaras el grado; en la interfaz no se ve nada."),
                      Rectangle<int> (0, 42, getWidth(), 44), Justification::topLeft, 2);

    static const char* romans[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
    for (int i = 0; i < 7; ++i)
    {
        const float y = 96.0f + (float) i * 46.0f;
        g.setColour (pal.text);
        g.setFont (ctx.fonts.ui (17.0f, 2));
        g.drawText (romans[i], Rectangle<float> (0.0f, y, 60.0f, 38.0f), Justification::centredLeft, false);
        if (i < (int) ctx.vis.diatonic.size())
        {
            g.setColour (pal.dim);
            g.setFont (ctx.fonts.ui (13.0f, 1));
            g.drawText (ctx.vis.diatonic[(size_t) i].symbol, Rectangle<float> (26.0f, y, 44.0f, 38.0f), Justification::centredLeft, true);
        }
        for (int kind = 0; kind < 2; ++kind)
        {
            auto r = slot (i, kind);
            const bool on = listenRow == i && listenKind == kind;
            g.setColour (on ? pal.accent.withAlpha (0.18f) : (pal.night ? pal.plate.brighter (0.12f) : pal.plate));
            g.fillRoundedRectangle (r, 9.0f);
            g.setColour (on ? pal.accent : pal.text.withAlpha (0.14f));
            g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, on ? 2.0f : 1.0f);
            const String t = on ? (kind == 0 ? U8 ("Presiona una tecla…") : U8 ("Toca un pad…"))
                                : (kind == 0 ? Prefs::keyName (prefs.degreeKeys[(size_t) i]) : Prefs::padName (prefs.degreePads[(size_t) i]));
            g.setColour (on ? pal.accent : pal.text);
            g.setFont (ctx.fonts.ui (14.5f, on ? 2 : 1));
            g.drawText (t, r.reduced (12.0f, 0.0f), Justification::centredLeft, true);
        }
    }
}

void DegreeInputPanel::mouseUp (const MouseEvent& e)
{
    for (int i = 0; i < 7; ++i)
        for (int kind = 0; kind < 2; ++kind)
            if (slot (i, kind).contains (e.position))
            {
                if (e.mods.isPopupMenu())   // clic derecho: quitar
                {
                    if (kind == 0) prefs.degreeKeys[(size_t) i] = 0; else prefs.degreePads[(size_t) i] = -1;
                    prefs.save();
                    repaint();
                    return;
                }
                learnAll = false;
                listen (listenRow == i && listenKind == kind ? -1 : i, kind);
                return;
            }
}

bool DegreeInputPanel::keyPressed (const KeyPress& key)
{
    if (listenRow < 0 || listenKind != 0) return false;
    if (key == KeyPress::escapeKey) { listen (-1, 0); return true; }
    int code = key.getKeyCode();
    if (code >= 'A' && code <= 'Z') code += 'a' - 'A';
    if (key == KeyPress::backspaceKey || key == KeyPress::deleteKey) code = 0;
    for (auto& k : prefs.degreeKeys) if (k == code && code != 0) k = 0;   // una tecla, un grado
    prefs.degreeKeys[(size_t) listenRow] = code;
    prefs.save();
    listen (-1, 0);
    return true;
}

void DegreeInputPanel::timerCallback()
{
    if (listenRow < 0 || listenKind != 1 || ! lastPad) return;
    const int ev = lastPad();
    if (ev == lastSeenPad) return;
    lastSeenPad = ev;
    const int code = ev & 0xffff;   // nota + 256 * canal
    for (auto& pd : prefs.degreePads) if (pd == code) pd = -1;
    prefs.degreePads[(size_t) listenRow] = code;
    prefs.save();
    if (learnAll && listenRow < 6) listen (listenRow + 1, 1);
    else { learnAll = false; listen (-1, 1); }
}
