#include "InterfaceEditor.h"

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
        syncFields();
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

void PrecisePicker::syncFields()
{
    const auto& pal = ctx.vis.pal;
    hex.setColour (TextEditor::backgroundColourId, pal.night ? pal.plate.brighter (0.12f) : pal.plate);
    hex.setColour (TextEditor::outlineColourId, pal.text.withAlpha (0.12f));
    hex.setColour (TextEditor::focusedOutlineColourId, pal.accent);
    hex.setColour (TextEditor::textColourId, pal.text);
    hex.setColour (CaretComponent::caretColourId, pal.accent);
    if (! hex.hasKeyboardFocus (false))
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
// Piezas internas del panel
//==============================================================================
namespace
{
    // Los estilos, con una muestra chiquita de cada uno
    class LookTiles : public Component, public HeightForWidth
    {
    public:
        LookTiles (UiContext& c, Prefs& p) : ctx (c), prefs (p) {}
        int heightFor (int width) override
        {
            const int cols = jmax (1, width / 112);
            const int n = (int) looks().size();
            return ((n + cols - 1) / cols) * 98;
        }
        Rectangle<float> tile (int i) const
        {
            const int cols = jmax (1, getWidth() / 112);
            const float w = ((float) getWidth() - 10.0f * (float) (cols - 1)) / (float) cols;
            return { (float) (i % cols) * (w + 10.0f), (float) (i / cols) * 98.0f, w, 90.0f };
        }
        void paint (Graphics& g) override
        {
            const auto& pal = ctx.vis.pal;
            const auto cur = lookIndex (prefs.look);
            for (int i = 0; i < (int) looks().size(); ++i)
            {
                auto r = tile (i);
                const Skin s = prefs.palette (looks()[(size_t) i].id);
                auto dev = r.withTrimmedBottom (22.0f).reduced (3.0f, 3.0f);
                draw::device (g, dev, s, 0.35f, false);
                auto scr = dev.reduced (dev.getWidth() * 0.26f, dev.getHeight() * 0.2f).withTrimmedBottom (dev.getHeight() * 0.22f);
                g.setColour (s.screen);
                g.fillRoundedRectangle (scr, 4.0f);
                g.setColour (s.scrText);
                g.setFont (s.pixelScreen ? ctx.fonts.pixel (17.0f) : ctx.fonts.ui (15.0f, 2));
                g.drawText ("Dm", scr, Justification::centred, false);
                auto keys = dev.reduced (8.0f, 0.0f).withTrimmedTop (dev.getHeight() * 0.74f).withTrimmedBottom (6.0f);
                const float kw = keys.getWidth() / 8.0f;
                for (int kk = 0; kk < 8; ++kk)
                {
                    g.setColour (kk == 2 || kk == 5 ? s.lit : s.keyW);
                    g.fillRoundedRectangle (keys.getX() + (float) kk * kw + 1.0f, keys.getY(), kw - 2.0f, keys.getHeight(), 2.0f);
                }
                g.setColour (i == cur ? pal.accent : pal.text.withAlpha (i == hover ? 0.4f : 0.12f));
                g.drawRoundedRectangle (dev.expanded (2.5f), 11.0f, i == cur ? 2.5f : 1.0f);
                g.setColour (i == cur ? pal.text : pal.dim);
                g.setFont (ctx.fonts.ui (13.5f, i == cur ? 2 : 1));
                g.drawText (looks()[(size_t) i].name, r.withTop (r.getBottom() - 20.0f), Justification::centred, false);
            }
        }
        void mouseMove (const MouseEvent& e) override
        {
            int h = -1;
            for (int i = 0; i < (int) looks().size(); ++i) if (tile (i).contains (e.position)) h = i;
            if (h != hover) { hover = h; repaint(); }
            if (h >= 0) setTooltip (looks()[(size_t) h].description);
        }
        void mouseUp (const MouseEvent& e) override
        {
            for (int i = 0; i < (int) looks().size(); ++i)
                if (tile (i).contains (e.position))
                {
                    prefs.look = looks()[(size_t) i].id;
                    prefs.keyStyle = -1;   // las teclas del estilo
                    prefs.save();
                    repaint();
                    return;
                }
        }
        void setTooltip (const String& t) { tip = t; }
    private:
        UiContext& ctx;
        Prefs& prefs;
        int hover = -1;
        String tip;
    };

    // Tus temas guardados (se aplican con un clic) y el botón para guardar uno nuevo
    class ThemesBox : public Component, public HeightForWidth
    {
    public:
        ThemesBox (UiContext& c, Prefs& p, std::function<void()> applied)
            : ctx (c), prefs (p), onApplied (std::move (applied)),
              saveBtn (c, "save", U8 ("Guarda estilo, colores, formas y movimiento como un tema tuyo"), "Guardar tema"),
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
                auto t = prefs.captureTheme (name);
                bool replaced = false;
                for (auto& th : prefs.themes) if (th.name == name) { th = t; replaced = true; }
                if (! replaced) prefs.themes.push_back (t);
                prefs.save (false);
                setNaming (false);
            };
            nameEditor.onReturnKey = commit;
            okBtn.onClick = commit;
            cancelBtn.onClick = [this] { setNaming (false); };
            saveBtn.onClick = [this] { setNaming (true); };
        }
        void setNaming (bool n)
        {
            naming = n;
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
            const float w = 150.0f, gap = 8.0f;
            const int perRow = jmax (1, (int) (((float) getWidth() + gap) / (w + gap)));
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
            saveBtn.setBounds (0, 0, 190, 40);
            nameEditor.setBounds (0, 0, getWidth() - 160, 40);
            okBtn.setBounds (getWidth() - 152, 0, 110, 40);
            cancelBtn.setBounds (getWidth() - 40, 0, 40, 40);
        }
        void paint (Graphics& g) override
        {
            const auto& pal = ctx.vis.pal;
            if (prefs.themes.empty())
            {
                g.setColour (pal.dim);
                g.setFont (ctx.fonts.ui (13.5f, 1));
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
                const auto s = prefs.palette (t.look);
                const auto body = t.uiTint.isTransparent() ? s.body2 : t.uiTint;
                const auto acc = t.uiAccent.isTransparent() ? s.accent : t.uiAccent;
                auto dots = in.removeFromLeft (28.0f);
                g.setColour (body); g.fillEllipse (dots.getX(), dots.getCentreY() - 7.0f, 14.0f, 14.0f);
                g.setColour (acc);  g.fillEllipse (dots.getX() + 10.0f, dots.getCentreY() - 7.0f, 14.0f, 14.0f);
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
                prefs.applyTheme (prefs.themes[(size_t) i]);
                if (onApplied) onApplied();
                return;
            }
            if (confirmDelete >= 0) { confirmDelete = -1; repaint(); }
        }
    private:
        UiContext& ctx;
        Prefs& prefs;
        std::function<void()> onApplied;
        PushButton saveBtn, okBtn, cancelBtn;
        TextEditor nameEditor;
        bool naming = false;
        int confirmDelete = -1;
    };

    // Muestra de cómo se ven las teclas
    class KeysPreview : public Component
    {
    public:
        KeysPreview (UiContext& c, Prefs& p) : ctx (c), prefs (p) {}
        void paint (Graphics& g) override
        {
            const auto& pal = ctx.vis.pal;
            auto demo = getLocalBounds().toFloat();
            draw::recess (g, demo, pal, 14.0f, pal.keyBed);
            auto kr = demo.reduced (12.0f);
            const int whites = 8;
            const float ww = kr.getWidth() / (float) whites;
            const Skin s = prefs.palette();
            for (int i = 0; i < whites; ++i)
            {
                const bool lit = i == 0 || i == 2 || i == 4;
                auto r = Rectangle<float> (kr.getX() + (float) i * ww + 2.0f, kr.getY() + kr.getHeight() * 0.32f, ww - 4.0f, kr.getHeight() * 0.68f);
                g.setColour (lit ? s.noteColour (60 + i * 2) : s.keyW);
                g.fillRoundedRectangle (r, jmin (ww * 0.42f, s.keyStyle == keysClasica ? 3.0f : 12.0f));
            }
            for (int i : { 1, 2, 4, 5, 6 })
            {
                const float x = kr.getX() + (float) i * ww;
                g.setColour (i == 4 ? s.noteColour (66) : s.keyB);
                if (s.keyStyle == keysBotones) g.fillEllipse (x - ww * 0.3f, kr.getY() + 4.0f, ww * 0.6f, ww * 0.6f);
                else g.fillRoundedRectangle (x - ww * 0.28f, kr.getY() + 2.0f, ww * 0.56f, kr.getHeight() * 0.5f, 4.0f);
            }
        }
    private:
        UiContext& ctx;
        Prefs& prefs;
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
        void resized() override { btn.setBounds (getLocalBounds().withWidth (jmin (getWidth(), 300))); }
        PushButton btn;
    };

    // Página: opciones y, debajo, sus colores (todo en una columna que se desplaza)
    class Page : public Component
    {
    public:
        Page (UiContext& c, std::unique_ptr<OptionsList> l, std::unique_ptr<ColourEditor> col)
            : ctx (c), list (std::move (l)), colours (std::move (col))
        {
            stack.addAndMakeVisible (*list);
            if (colours != nullptr) stack.addAndMakeVisible (*colours);
            view.setViewedComponent (&stack, false);
            view.setScrollBarsShown (true, false);
            view.setScrollBarThickness (8);
            addAndMakeVisible (view);
        }
        void resized() override
        {
            view.setBounds (getLocalBounds());
            const int w = getWidth() - 14;
            int h = list->layout (w);
            list->setTopLeftPosition (0, 0);
            if (colours != nullptr)
            {
                h += 6;
                colourTop = h;
                colours->setBounds (0, h + 30, w, 394);
                h += 30 + 394 + 10;
            }
            stack.setSize (w, h);
        }
    private:
        struct Stack : Component
        {
            Page* owner = nullptr;
            void paint (Graphics& g) override
            {
                if (owner == nullptr || owner->colours == nullptr) return;
                const auto& pal = owner->ctx.vis.pal;
                g.setColour (pal.text.withAlpha (0.10f));
                g.fillRect (0, owner->colourTop, getWidth(), 1);
                g.setColour (pal.dim);
                g.setFont (owner->ctx.fonts.ui (14.0f, 2));
                g.drawText ("Colores", 0, owner->colourTop + 6, getWidth(), 20, Justification::centredLeft, false);
            }
        };
        UiContext& ctx;
        std::unique_ptr<OptionsList> list;
        std::unique_ptr<ColourEditor> colours;
        Stack stack;
        Viewport view;
        int colourTop = 0;
        friend struct Stack;
    public:
        void parentHierarchyChanged() override { stack.owner = this; }
    };

    String pct (float v) { return String (roundToInt (v * 100.0f)) + " %"; }
}

//==============================================================================
// El panel
//==============================================================================
StringArray CustomizePanel::tabNames()
{
    return { "Estilo", "Teclas", "Pantalla", "Grados", U8 ("Círculo"), "Pentagrama", "Movimiento" };
}

static int lastCustomizeTab = 0;
void CustomizePanel::setStartTab (int t) { lastCustomizeTab = jlimit (0, tabNames().size() - 1, t); }

CustomizePanel::CustomizePanel (UiContext& c, Prefs& p, std::function<void()> openDegreeInput)
    : ctx (c), prefs (p), openDegrees (std::move (openDegreeInput)), tabs (c, tabNames())
{
    addAndMakeVisible (tabs);
    tabs.onChange = [this] (int i) { showTab (i); };
    setSize (620, 530);
    tabs.setSelected (lastCustomizeTab);
    showTab (lastCustomizeTab);
}

void SideTabs::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    for (int i = 0; i < names.size(); ++i)
    {
        auto r = item (i);
        const float rad = r.getHeight() * shapedRadius (pal, 0.3f);
        if (i == selected)
        {
            g.setColour (pal.accent);
            g.fillRoundedRectangle (r, rad);
        }
        else if (i == hover)
        {
            g.setColour (pal.text.withAlpha (0.07f));
            g.fillRoundedRectangle (r, rad);
        }
        g.setColour (i == selected ? pal.accentInk : pal.text.withAlpha (0.85f));
        g.setFont (ctx.fonts.ui (14.5f, i == selected ? 2 : 1));
        g.drawText (names[i], r.reduced (14.0f, 0.0f), Justification::centredLeft, false);
    }
    // Línea que separa las pestañas del contenido
    g.setColour (pal.text.withAlpha (0.08f));
    g.fillRect ((float) getWidth() - 1.0f, 0.0f, 1.0f, (float) getHeight());
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

void CustomizePanel::showTab (int i)
{
    lastCustomizeTab = i;
    build (i);
    resized();
    if (onTab) onTab (i);
}

void CustomizePanel::resized()
{
    tabs.setBounds (0, 0, 134, getHeight());
    if (page != nullptr) page->setBounds (150, 0, getWidth() - 150, getHeight());
}

void CustomizePanel::paint (Graphics&) {}

void CustomizePanel::build (int tab)
{
    if (page != nullptr) removeChildComponent (page.get());
    page.reset();
    auto list = std::make_unique<OptionsList> (ctx);
    std::unique_ptr<ColourEditor> colours;
    auto& P = prefs;
    auto& pc = prefs.pieces;
    auto save = [&P] { P.save(); };
    SafePointer<CustomizePanel> safe (this);
    auto rebuild = [safe] { MessageManager::callAsync ([safe] { if (safe != nullptr) safe->showTab (safe->tabs.getSelected()); }); };

    switch (tab)
    {
        case 0:   // Estilo
        {
            auto tiles = std::make_unique<LookTiles> (ctx, P);
            const int th = tiles->heightFor (456);
            list->custom ("Estilo", std::move (tiles), th);
            list->chips (U8 ("Modo día o noche"), { U8 ("Día"), "Noche", "Auto" }, P.dayNight, [&P] (int i) { P.dayNight = i; P.save(); });
            {
                const auto& qp = quickPalettes();
                StringArray names;
                for (auto& q : qp) names.add (q.name);
                int sel = -1;
                for (int i = 0; i < (int) qp.size(); ++i)
                    if ((i == 0 && P.uiTint.isTransparent() && P.uiAccent.isTransparent()) || (i > 0 && P.uiTint == qp[(size_t) i].tint && P.uiAccent == qp[(size_t) i].accent)) sel = i;
                auto& cg = list->chips (U8 ("Paletas rápidas"), names, sel, [&P, rebuild] (int i)
                {
                    const auto& q = quickPalettes()[(size_t) i];
                    P.uiTint = q.tint; P.uiAccent = q.accent; P.save();
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
            list->chips ("Forma de los botones", buttonShapeNames(), pc.btnShape, [&P] (int i) { P.pieces.btnShape = i; P.save(); });
            auto themes = std::make_unique<ThemesBox> (ctx, P, rebuild);
            const int tH = themes->heightFor (456);
            list->custom ("Mis temas (guardan todo: estilo, colores, formas y movimiento)", std::move (themes), tH);
            colours = std::make_unique<ColourEditor> (ctx, std::vector<ColourTarget> {
                { "Tono del cuerpo", [&P] { return P.uiTint; }, [this] { return ctx.vis.pal.body2; }, [&P] (Colour c) { P.uiTint = c; P.save(); } },
                { "Acento", [&P] { return P.uiAccent; }, [this] { return ctx.vis.pal.accent; }, [&P] (Colour c) { P.uiAccent = c; P.save(); } } },
                "Volver al original", [&P] { P.uiTint = P.uiAccent = Colour(); P.save(); });
            break;
        }
        case 1:   // Teclas
        {
            list->chips ("Forma de las teclas", keyStyleNames(), P.effectiveKeyStyle(), [&P] (int i) { P.keyStyle = i; P.save(); });
            list->toggle (U8 ("Arcoíris: cada nota de un color"), P.rainbow, [&P] (bool on) { P.rainbow = on; P.save(); });
            list->custom ("Muestra", std::make_unique<KeysPreview> (ctx, P), 150);
            colours = std::make_unique<ColourEditor> (ctx, std::vector<ColourTarget> {
                { "Encendidas", [&P] { return P.keyLit; }, [this] { return ctx.vis.pal.lit; }, [&P] (Colour c) { P.keyLit = c; P.rainbow = false; P.save(); } },
                { "Blancas", [&P] { return P.keyWhite; }, [this] { return ctx.vis.pal.keyW; }, [&P] (Colour c) { P.keyWhite = c; P.save(); } },
                { "Negras", [&P] { return P.keyBlack; }, [this] { return ctx.vis.pal.keyB; }, [&P] (Colour c) { P.keyBlack = c; P.save(); } } },
                U8 ("Colores de fábrica"), [&P, rebuild] { P.keyLit = P.keyWhite = P.keyBlack = Colour(); P.rainbow = false; P.save(); rebuild(); });
            break;
        }
        case 2:   // Pantalla
        {
            list->chips (U8 ("Diseño"), screenSkinNames(), pc.scrSkin, [&P, rebuild] (int i) { P.pieces.scrSkin = i; P.save(); rebuild(); });
            auto& letters = list->chips ("Letra", screenFontNames(), pc.scrFont, [&P] (int i) { P.pieces.scrFont = i; P.save(); });
            auto* ctxp = &ctx;
            letters.fontFor = [ctxp] (int i, float h) { return i == 0 ? ctxp->fonts.ui (h * 0.93f, 1) : ctxp->fonts.display (i, false, h * 1.05f); };
            list->chips ("Forma", screenShapeNames(), pc.scrShape, [&P] (int i) { P.pieces.scrShape = i; P.save(); });
            list->chips ("Textura", screenTextureNames(), pc.scrTexture, [&P] (int i) { P.pieces.scrTexture = i; P.save(); });
            list->slider ("Luces (brillo de las letras)", 0.0f, 1.0f, 0.0f, pieces::screenLook (ctx.vis.pal).glow, pct,
                          [&P] (float v) { P.pieces.scrGlow = v; P.save(); });
            list->chips (U8 ("Movimiento al cambiar de acorde"), screenMotionNames(), pc.scrMotion, [&P] (int i) { P.pieces.scrMotion = i; P.save(); });
            colours = std::make_unique<ColourEditor> (ctx, std::vector<ColourTarget> {
                { "Letras", [&P] { return P.pieces.scrText; }, [this] { return pieces::screenLook (ctx.vis.pal).text; }, [&P] (Colour c) { P.pieces.scrText = c; P.save(); } },
                { "Fondo", [&P] { return P.pieces.scrBack; }, [this] { return pieces::screenLook (ctx.vis.pal).bg; }, [&P] (Colour c) { P.pieces.scrBack = c; P.save(); } } },
                "Quitar mis colores", [&P] { P.pieces.scrText = P.pieces.scrBack = Colour(); P.save(); });
            break;
        }
        case 3:   // Grados
        {
            list->chips (U8 ("Diseño de los botones"), degreeSkinNames(), pc.degSkin, [&P] (int i) { P.pieces.degSkin = i; P.save(); });
            list->chips ("Forma", degreeShapeNames(), pc.degShape, [&P] (int i) { P.pieces.degShape = i; P.save(); });
            auto* od = &openDegrees;
            list->custom ("Tocar y grabar los grados con teclas de la compu o pads MIDI",
                          std::make_unique<ButtonRow> (ctx, "keys", U8 ("Teclas y pads…"), [od] { if (*od) (*od)(); }), 42);
            colours = std::make_unique<ColourEditor> (ctx, std::vector<ColourTarget> {
                { U8 ("Luz del grado que suena"), [&P] { return P.pieces.degColour; }, [this] { return ctx.vis.pal.accent; }, [&P] (Colour c) { P.pieces.degColour = c; P.save(); } } },
                "Quitar mi color", [&P] { P.pieces.degColour = Colour(); P.save(); });
            break;
        }
        case 4:   // Círculo
        {
            list->chips (U8 ("Diseño"), circleSkinNames(), pc.cirSkin, [&P] (int i) { P.pieces.cirSkin = i; P.save(); });
            list->chips (U8 ("Líneas entre las notas"), circleRouteNames(), pc.cirRoute, [&P] (int i) { P.pieces.cirRoute = i; P.save(); });
            list->slider (U8 ("Grosor de las líneas"), 0.5f, 2.5f, 1.0f, pc.cirLineW, [] (float v) { return String (v, 1) + "x"; },
                          [&P] (float v) { P.pieces.cirLineW = v; P.save(); });
            list->slider (U8 ("Tamaño del botón del centro"), 0.5f, 1.15f, 1.0f, pc.cirKnob, pct,
                          [&P] (float v) { P.pieces.cirKnob = v; P.save(); });
            list->chips ("Puntos de las notas", circleNodeNames(), pc.cirNodes, [&P] (int i) { P.pieces.cirNodes = i; P.save(); });
            list->chips ("Textura", circleTextureNames(), pc.cirTexture, [&P] (int i) { P.pieces.cirTexture = i; P.save(); });
            list->chips ("Placa", plateShapeNames(), pc.cirShape, [&P] (int i) { P.pieces.cirShape = i; P.save(); });
            list->toggle (U8 ("Luz que recorre las líneas"), pc.cirFlow, [&P] (bool on) { P.pieces.cirFlow = on; P.save(); });
            colours = std::make_unique<ColourEditor> (ctx, std::vector<ColourTarget> {
                { U8 ("Color de la tonalidad y las líneas"), [&P] { return P.pieces.cirColour; }, [this] { return ctx.vis.pal.accent; }, [&P] (Colour c) { P.pieces.cirColour = c; P.save(); } } },
                "Quitar mi color", [&P] { P.pieces.cirColour = Colour(); P.save(); });
            break;
        }
        case 5:   // Pentagrama
        {
            list->chips (U8 ("Diseño"), staffSkinNames(), pc.stfSkin, [&P] (int i) { P.pieces.stfSkin = i; P.save(); });
            list->chips ("Textura", staffTextureNames(), pc.stfTexture, [&P] (int i) { P.pieces.stfTexture = i; P.save(); });
            list->chips ("Marco", frameShapeNames(), pc.stfShape, [&P] (int i) { P.pieces.stfShape = i; P.save(); });
            colours = std::make_unique<ColourEditor> (ctx, std::vector<ColourTarget> {
                { "Tinta", [&P] { return P.pieces.stfInk; }, [this] { return pieces::staffLook (ctx.vis.pal).ink; }, [&P] (Colour c) { P.pieces.stfInk = c; P.save(); } },
                { "Papel", [&P] { return P.pieces.stfPaper; }, [this] { return pieces::staffLook (ctx.vis.pal).paper; }, [&P] (Colour c) { P.pieces.stfPaper = c; P.save(); } } },
                "Quitar mis colores", [&P] { P.pieces.stfInk = P.pieces.stfPaper = Colour(); P.save(); });
            break;
        }
        default:  // Movimiento
        {
            list->toggle ("Animaciones", pc.anim, [&P] (bool on) { P.pieces.anim = on; P.save(); });
            list->slider ("Velocidad de las animaciones", 0.4f, 2.5f, 1.0f, pc.animSpeed, [] (float v) { return String (v, 1) + "x"; },
                          [&P] (float v) { P.pieces.animSpeed = v; P.save(); });
            list->chips ("Teclas al tocar", keyMotionNames(), pc.keyMotion, [&P] (int i) { P.pieces.keyMotion = i; P.save(); });
            list->slider ("Destello de las teclas", 0.0f, 1.0f, 0.5f, pc.keyGlow, pct, [&P] (float v) { P.pieces.keyGlow = v; P.save(); });
            list->chips (U8 ("Pantalla al cambiar de acorde"), screenMotionNames(), pc.scrMotion, [&P] (int i) { P.pieces.scrMotion = i; P.save(); });
            list->toggle (U8 ("Luz que recorre las líneas del círculo"), pc.cirFlow, [&P] (bool on) { P.pieces.cirFlow = on; P.save(); });
            list->toggle ("Paneles con transiciones suaves", pc.panelMotion, [&P] (bool on) { P.pieces.panelMotion = on; P.save(); });
            break;
        }
    }
    (void) save;
    page = std::make_unique<Page> (ctx, std::move (list), std::move (colours));
    addAndMakeVisible (*page);
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
