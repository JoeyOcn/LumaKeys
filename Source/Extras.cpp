#include "Extras.h"

using namespace juce;

static void styleEditor (TextEditor& e, const Skin& pal, Font f)
{
    e.setFont (f);
    e.setColour (TextEditor::backgroundColourId, pal.night ? pal.plate.brighter (0.1f) : Colours::white);
    e.setColour (TextEditor::textColourId, pal.text);
    e.setColour (TextEditor::outlineColourId, pal.edge);
    e.setColour (TextEditor::focusedOutlineColourId, pal.accent);
    e.setColour (TextEditor::highlightColourId, pal.accent.withAlpha (0.3f));
    e.setColour (CaretComponent::caretColourId, pal.accent);
    e.setIndents (12, 0);
    e.setJustification (Justification::centredLeft);
}

static void label (Graphics& g, UiContext& ctx, const String& t, Rectangle<float> r, bool strong = true)
{
    g.setColour (strong ? ctx.vis.pal.dim : ctx.vis.pal.dim.withAlpha (0.8f));
    g.setFont (ctx.fonts.ui (14.0f, strong ? 2 : 1));
    g.drawText (t, r, Justification::centredLeft, false);
}

static String timeText (double s)
{
    const int t = (int) std::floor (s + 0.5);
    return String (t / 60) + ":" + String (t % 60).paddedLeft ('0', 2);
}

//==============================================================================
// Nombre
//==============================================================================
NamePanel::NamePanel (UiContext& c, const String& p, const String& suggested)
    : ctx (c), prompt (p), saveBtn (c, "save", "Guardar", "Guardar")
{
    styleEditor (editor, ctx.vis.pal, ctx.fonts.ui (17.0f, 1));
    editor.setText (suggested, false);
    editor.selectAll();
    editor.onReturnKey = [this] { commit(); };
    saveBtn.setActive (true);
    saveBtn.onClick = [this] { commit(); };
    addAndMakeVisible (editor);
    addAndMakeVisible (saveBtn);
    setSize (460, 112);
}

void NamePanel::visibilityChanged()
{
    if (isShowing())
    {
        SafePointer<NamePanel> safe (this);
        MessageManager::callAsync ([safe] { if (auto* s = safe.getComponent()) { s->editor.grabKeyboardFocus(); s->editor.selectAll(); } });
    }
}

void NamePanel::commit()
{
    const auto t = editor.getText().trim();
    if (t.isEmpty()) { editor.grabKeyboardFocus(); return; }
    if (onSave) onSave (t);
}

void NamePanel::paint (Graphics& g)
{
    label (g, ctx, prompt, { 0.0f, 0.0f, (float) getWidth(), 20.0f });
}

void NamePanel::resized()
{
    editor.setBounds (0, 30, getWidth() - 150, 46);
    saveBtn.setBounds (getWidth() - 138, 30, 138, 46);
}

//==============================================================================
// Metrónomo
//==============================================================================
void BpmDisplay::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto r = getLocalBounds().toFloat();
    draw::screen (g, r, pal, 0.8f);
    auto in = r.reduced (14.0f, 8.0f);
    g.setColour (pal.scrText);
    g.setFont (pal.pixelScreen ? ctx.fonts.pixel (46.0f) : ctx.fonts.ui (40.0f, 2));
    const auto txt = std::abs (bpm - std::round (bpm)) < 0.05f ? String (roundToInt (bpm)) : String (bpm, 1);
    g.drawText (txt, in.withTrimmedRight (70.0f), Justification::centredRight, false);
    g.setColour (pal.scrDim);
    g.setFont (ctx.fonts.ui (15.0f, 1));
    g.drawText (host ? "BPM (DAW)" : "BPM", in.withTrimmedLeft (in.getWidth() - 64.0f), Justification::centredLeft, false);
}

void BpmDisplay::mouseDown (const MouseEvent&) { dragStart = bpm; }

void BpmDisplay::mouseDrag (const MouseEvent& e)
{
    if (host) return;
    const float v = jlimit (20.0f, 300.0f, std::round (dragStart - (float) e.getDistanceFromDragStartY() * 0.35f));
    if (onChange) onChange (v);
}

void BpmDisplay::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    if (host) return;
    const float step = w.deltaY > 0 ? 1.0f : (w.deltaY < 0 ? -1.0f : 0.0f);
    if (onChange && step != 0.0f) onChange (jlimit (20.0f, 300.0f, std::round (bpm) + step));
}

void BpmDisplay::mouseDoubleClick (const MouseEvent&)
{
    if (! host && onChange) onChange (100.0f);
}

MetronomePanel::MetronomePanel (UiContext& c, Prefs& p, MetronomeSettings& s, Metronome& m, bool daw, std::function<void()> ch)
    : ctx (c), prefs (p), settings (s), metro (m), inDaw (daw), changed (std::move (ch)),
      onSw (c), followSw (c), bpm (c),
      minusBtn (c, "minus", "Menos tempo"), plusBtn (c, "plus", U8 ("Más tempo")),
      tapBtn (c, "tap", U8 ("Toca varias veces al ritmo para poner el tempo"), "Tap"),
      beats (c, { "2", "3", "4", "5", "6", "7" })
{
    for (Component* comp : std::initializer_list<Component*> { &onSw, &bpm, &minusBtn, &plusBtn, &tapBtn, &beats, &volume })
        addAndMakeVisible (comp);
    if (inDaw) addAndMakeVisible (followSw);

    onSw.setOn (settings.on.load());
    onSw.onChange = [this] (bool v) { settings.on.store (v); if (changed) changed(); };
    followSw.setOn (prefs.metroFollowHost);
    followSw.onChange = [this] (bool v) { prefs.metroFollowHost = v; prefs.save (false); if (changed) changed(); };

    bpm.onChange = [this] (float v) { setBpm (v); };
    minusBtn.onClick = [this] { setBpm (std::round (prefs.metroBpm) - 1.0f); };
    plusBtn.onClick  = [this] { setBpm (std::round (prefs.metroBpm) + 1.0f); };
    tapBtn.onClick = [this]
    {
        const double now = Time::getMillisecondCounterHiRes();
        if (! taps.isEmpty() && now - taps.getLast() > 2000.0) taps.clear();
        taps.add (now);
        while (taps.size() > 6) taps.remove (0);
        if (taps.size() >= 2)
        {
            const double avg = (taps.getLast() - taps.getFirst()) / (double) (taps.size() - 1);
            setBpm ((float) std::round (60000.0 / avg));
        }
    };

    beats.setSelected (jlimit (0, 5, prefs.metroBeats - 2));
    beats.onChange = [this] (int i) { prefs.metroBeats = i + 2; prefs.save (false); if (changed) changed(); repaint(); };

    const auto names = metronomeSoundNames();
    for (int i = 0; i < names.size(); ++i)
    {
        auto* b = soundBtns.add (new PushButton (c, {}, U8 ("Escuchar y usar este sonido"), names[i]));
        b->setActive (i == prefs.metroSound);
        b->onClick = [this, i]
        {
            prefs.metroSound = i;
            prefs.save (false);
            if (changed) changed();
            metro.previewSound (i);
            for (int j = 0; j < soundBtns.size(); ++j) soundBtns[j]->setActive (j == i);
        };
        addAndMakeVisible (b);
    }

    volume.setSliderStyle (Slider::LinearHorizontal);
    volume.setTextBoxStyle (Slider::TextBoxRight, false, 78, 26);
    volume.setRange (-40.0, 6.0, 0.5);
    volume.setTextValueSuffix (" dB");
    volume.setValue (prefs.metroVolume, dontSendNotification);
    volume.setDoubleClickReturnValue (true, -8.0);
    volume.setColour (Slider::trackColourId, ctx.vis.pal.accent);
    volume.setColour (Slider::thumbColourId, ctx.vis.pal.accent);
    volume.setColour (Slider::backgroundColourId, ctx.vis.pal.night ? ctx.vis.pal.plate.brighter (0.15f) : ctx.vis.pal.plate.darker (0.1f));
    volume.onValueChange = [this] { prefs.metroVolume = (float) volume.getValue(); prefs.save (false); if (changed) changed(); };
    volume.onDragEnd = [this] { metro.previewSound (prefs.metroSound); };

    setSize (560, inDaw ? 420 : 380);
    lastCount = metro.uiBeatCount.load();
    startTimerHz (30);
}

MetronomePanel::~MetronomePanel() { stopTimer(); }

void MetronomePanel::setBpm (float b)
{
    prefs.metroBpm = jlimit (20.0f, 300.0f, b);
    prefs.save (false);
    if (changed) changed();
    bpm.setBpm (prefs.metroBpm, false);
}

void MetronomePanel::timerCallback()
{
    const bool host = inDaw && prefs.metroFollowHost && metro.uiHostTempo.load();
    bpm.setBpm (host ? metro.uiTempo.load() : prefs.metroBpm, host);
    for (auto* b : { &minusBtn, &plusBtn, &tapBtn }) b->setAlpha (host ? 0.4f : 1.0f);
    onSw.setOn (settings.on.load());

    const auto count = metro.uiBeatCount.load();
    if (count != lastCount && settings.on.load())
    {
        lastCount = count;
        beatShown = metro.uiBeat.load();
        beatFlash = 1.0f;
    }
    if (beatFlash > 0.0f) { beatFlash = jmax (0.0f, beatFlash - 0.09f); repaint (0, 0, getWidth(), 50); }
}

void MetronomePanel::resized()
{
    onSw.setBounds (150, 9, 58, 30);
    bpm.setBounds (0, 82, 250, 70);
    minusBtn.setBounds (262, 90, 54, 54);
    plusBtn.setBounds (324, 90, 54, 54);
    tapBtn.setBounds (390, 90, 170, 54);
    beats.setBounds (0, 196, 560, 38);
    int y = 274;
    for (int i = 0; i < soundBtns.size(); ++i)
        soundBtns[i]->setBounds ((i % 6) * 94, y, 86, 44);
    y += 66;
    volume.setBounds (90, y, 470, 30);
    if (inDaw) followSw.setBounds (getWidth() - 60, y + 46, 58, 30);
}

void MetronomePanel::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (16.0f, 2));
    g.drawText ("Encendido", Rectangle<float> (0.0f, 8.0f, 140.0f, 32.0f), Justification::centredLeft, false);

    // Lucecitas de los tiempos
    const int n = jlimit (1, 12, metro.beatsPerBar());
    const float d = 18.0f, gap = 10.0f;
    float x = (float) getWidth() - (float) n * (d + gap) + gap;
    for (int i = 0; i < n; ++i)
    {
        const bool lit = settings.on.load() && i == beatShown;
        auto r = Rectangle<float> (x, 15.0f, d, d);
        g.setColour ((i == 0 ? pal.accent : pal.btnText).withAlpha (lit ? 0.35f + 0.65f * beatFlash : 0.18f));
        g.fillEllipse (r);
        x += d + gap;
    }

    label (g, ctx, "Tempo", { 0.0f, 56.0f, 300.0f, 20.0f });
    label (g, ctx, U8 ("Tiempos por compás"), { 0.0f, 170.0f, 300.0f, 20.0f });
    label (g, ctx, "Sonido", { 0.0f, 248.0f, 300.0f, 20.0f });
    label (g, ctx, "Volumen", { 0.0f, 340.0f, 90.0f, 30.0f });
    if (inDaw)
    {
        g.setColour (pal.text);
        g.setFont (ctx.fonts.ui (15.0f, 1));
        g.drawText (U8 ("Usar el tempo y el compás del DAW"), Rectangle<float> (0.0f, 386.0f, 420.0f, 30.0f), Justification::centredLeft, false);
    }
}

//==============================================================================
// Grabación: notas y fuerza
//==============================================================================
void ClipView::setFile (const File& f)
{
    loaded = f.existsAsFile() && clip.load (f);
    lowNote = 127; highNote = 0;
    for (auto& n : clip.notes) { lowNote = jmin (lowNote, n.note); highNote = jmax (highNote, n.note); }
    if (! loaded || clip.notes.empty()) { lowNote = 48; highNote = 72; }
    const int span = highNote - lowNote;
    if (span < 12) { lowNote -= (12 - span) / 2; highNote = lowNote + 12; }
    lowNote = jmax (0, lowNote - 1);
    highNote = jmin (127, highNote + 1);
    repaint();
}

Rectangle<float> ClipView::rollArea() const
{
    auto r = getLocalBounds().toFloat();
    return r.removeFromTop (r.getHeight() * 0.6f).reduced (10.0f, 10.0f);
}

Rectangle<float> ClipView::laneArea() const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromTop (r.getHeight() * 0.6f);
    return r.reduced (10.0f, 0.0f).withTrimmedTop (22.0f).withTrimmedBottom (10.0f);
}

float ClipView::xOf (double beat) const
{
    const auto r = rollArea();
    const double len = jmax (1.0, std::ceil (clip.lengthBeats / 4.0) * 4.0);
    return r.getX() + (float) (beat / len) * r.getWidth();
}

void ClipView::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    const auto ink = pal.ink;
    auto all = getLocalBounds().toFloat();
    draw::card (g, all, pal, 14.0f);

    const auto roll = rollArea();
    const auto lane = laneArea();
    if (! loaded)
    {
        g.setColour (ink.withAlpha (0.6f));
        g.setFont (ctx.fonts.ui (15.0f, 1));
        g.drawFittedText (U8 ("Elige una grabación para ver sus notas.\nAbajo puedes cambiar la fuerza (velocity) arrastrando, como en FL Studio."),
                          all.reduced (30.0f).toNearestInt(), Justification::centred, 3);
        return;
    }

    // Rejilla: compases y tiempos
    const double len = jmax (1.0, std::ceil (clip.lengthBeats / 4.0) * 4.0);
    for (int b = 0; b <= (int) len; ++b)
    {
        const float x = xOf (b);
        const bool bar = b % 4 == 0;
        g.setColour (ink.withAlpha (bar ? 0.16f : 0.06f));
        g.drawVerticalLine (roundToInt (x), roll.getY(), roll.getBottom());
        g.drawVerticalLine (roundToInt (x), lane.getY(), lane.getBottom());
        if (bar && b < (int) len)
        {
            g.setFont (ctx.fonts.ui (12.0f, 1));
            g.setColour (ink.withAlpha (0.6f));
            g.drawText (String (b / 4 + 1), Rectangle<float> (x + 3.0f, roll.getY() - 2.0f, 30.0f, 14.0f), Justification::centredLeft, false);
        }
    }

    // Notas
    const float rowH = roll.getHeight() / (float) jmax (1, highNote - lowNote + 1);
    for (auto& n : clip.notes)
    {
        const float x1 = xOf (n.start), x2 = jmax (x1 + 3.0f, xOf (n.end));
        const float y = roll.getBottom() - (float) (n.note - lowNote + 1) * rowH;
        auto r = Rectangle<float> (x1, y + 1.0f, x2 - x1 - 1.0f, jmax (2.0f, rowH - 2.0f));
        const float a = 0.45f + 0.55f * (float) n.velocity / 127.0f;
        g.setColour ((n.note == hoverNote ? ink : pal.accent).withAlpha (a));
        g.fillRoundedRectangle (r, jmin (3.0f, r.getHeight() * 0.4f));
    }

    // Fuerza: palitos con bolita (estilo FL)
    g.setColour (ink.withAlpha (0.6f));
    g.setFont (ctx.fonts.ui (13.0f, 2));
    g.drawText ("Fuerza (velocity)", Rectangle<float> (lane.getX(), lane.getY() - 20.0f, 300.0f, 18.0f), Justification::centredLeft, false);
    g.setColour (ink.withAlpha (0.12f));
    g.drawHorizontalLine (roundToInt (lane.getBottom()), lane.getX(), lane.getRight());
    for (auto& n : clip.notes)
    {
        const float x = xOf (n.start) + 1.5f;
        const float top = lane.getBottom() - lane.getHeight() * (float) n.velocity / 127.0f;
        const auto col = pal.accent.withAlpha (0.4f + 0.6f * (float) n.velocity / 127.0f);
        g.setColour (col);
        g.fillRect (x - 1.0f, top, 2.0f, lane.getBottom() - top);
        g.fillEllipse (x - 4.5f, top - 4.5f, 9.0f, 9.0f);
    }
}

void ClipView::paintVelocity (Point<float> a, Point<float> b)
{
    const auto lane = laneArea();
    const float x1 = jmin (a.x, b.x) - 4.0f, x2 = jmax (a.x, b.x) + 4.0f;
    for (auto& n : clip.notes)
    {
        const float x = xOf (n.start) + 1.5f;
        if (x < x1 || x > x2) continue;
        // a lo largo de la línea arrastrada (para "dibujar" curvas de fuerza)
        const float t = std::abs (b.x - a.x) < 0.5f ? 1.0f : jlimit (0.0f, 1.0f, (x - a.x) / (b.x - a.x));
        const float y = a.y + (b.y - a.y) * t;
        n.velocity = jlimit (1, 127, roundToInt (127.0f * (lane.getBottom() - y) / jmax (1.0f, lane.getHeight())));
        edited = true;
    }
    repaint();
}

void ClipView::mouseDown (const MouseEvent& e)
{
    if (loaded && rollArea().contains (e.position)) { if (onOpen) onOpen(); return; }
    if (! loaded || ! laneArea().expanded (0.0f, 8.0f).contains (e.position)) return;
    lastDrag = e.position;
    paintVelocity (e.position, e.position);
}

void ClipView::mouseDrag (const MouseEvent& e)
{
    if (! loaded || lastDrag == Point<float>()) return;
    paintVelocity (lastDrag, e.position);
    lastDrag = e.position;
}

void ClipView::mouseUp (const MouseEvent&)
{
    lastDrag = {};
    if (edited)
    {
        edited = false;
        clip.saveVelocities();
        if (onEdited) onEdited();
    }
}

void ClipView::mouseMove (const MouseEvent& e)
{
    setMouseCursor (loaded && laneArea().expanded (0.0f, 8.0f).contains (e.position) ? MouseCursor::CrosshairCursor
                    : (loaded && rollArea().contains (e.position) ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor));
}

//==============================================================================
// Botón para arrastrar un archivo
//==============================================================================
FileDragChip::FileDragChip (UiContext& c, String t, String i) : text (std::move (t)), icon (std::move (i)), ctx (c)
{
    setMouseCursor (MouseCursor::DraggingHandCursor);
}

void FileDragChip::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    const bool has = getFile && getFile().existsAsFile();
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    auto c = b;
    if (compact)
    {
        // Minimalista: pastilla plana (llena si ya se puede arrastrar)
        const float rad = b.getHeight() * shapedRadius (pal, 0.5f);
        if (has) { g.setColour (pal.accent.withMultipliedBrightness (down ? 0.9f : 1.0f)); g.fillRoundedRectangle (b, rad); }
        else
        {
            g.setColour (pal.text.withAlpha (down ? 0.12f : 0.06f));
            g.fillRoundedRectangle (b, rad);
            g.setColour (pal.text.withAlpha (0.2f));
            g.drawRoundedRectangle (b.reduced (0.5f), rad, 1.0f);
        }
        c = b.reduced (6.0f, 0.0f);
    }
    else
    {
        draw::button (g, b, pal, b.getHeight() * 0.3f, down ? 1.0f : 0.0f, has, 0.0f);
        c = b.withTrimmedBottom (2.5f).translated (0.0f, down ? 2.5f : 0.0f);
    }
    const auto col = has ? pal.accentInk : (compact ? pal.text : pal.btnText);
    if (progress >= 0.0f)
    {
        auto bar = c.reduced (10.0f, 0.0f).withSizeKeepingCentre (c.getWidth() - 20.0f, 6.0f).translated (0.0f, c.getHeight() * 0.28f);
        g.setColour (col.withAlpha (0.2f));
        g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (pal.accent);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * jlimit (0.02f, 1.0f, progress)), 3.0f);
        c = c.withTrimmedBottom (c.getHeight() * 0.3f);
    }
    auto ir = c.removeFromLeft (jmin (c.getHeight(), 40.0f));
    draw::icon (g, has ? icon : "wave", ir.reduced (ir.getWidth() * 0.27f), col, 1.6f);
    const auto t = progress >= 0.0f ? String (roundToInt (progress * 100.0f)) + " %" : (has ? text : textWithoutFile);
    g.setColour (col);
    g.setFont (draw::fitUi (ctx.fonts, t, 14.0f, c.getWidth() - 6.0f, 2));
    g.drawText (t, c.withTrimmedRight (4.0f), Justification::centredLeft, false);
}

void FileDragChip::mouseDown (const MouseEvent&) { down = true; dragging = false; repaint(); }

void FileDragChip::mouseDrag (const MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 4 || ! getFile) return;
    const auto f = getFile();
    if (! f.existsAsFile()) return;
    dragging = true;
    SafePointer<FileDragChip> safe (this);
    DragAndDropContainer::performExternalDragDropOfFiles ({ MidiRecorder::dragCopy (f).getFullPathName() }, false, this,
        [safe] { if (auto* c = safe.getComponent()) { c->down = false; c->dragging = false; c->repaint(); } });
}

void FileDragChip::mouseUp (const MouseEvent& e)
{
    down = false;
    repaint();
    if (dragging || ! getLocalBounds().contains (e.getPosition())) { dragging = false; return; }
    const bool has = getFile && getFile().existsAsFile();
    if (! has && onClickWithoutFile) onClickWithoutFile();
}

//==============================================================================
// Lista de grabaciones: cada una se arrastra como MIDI o como audio
//==============================================================================
class RecordingsPanel::List : public Component
{
public:
    List (RecordingsPanel& p) : owner (p) {}
    static constexpr int rowH = 62;
    enum Zone { rowZone, playZone, trashZone, editZone, midiZone, audioZone };

    struct Rects { Rectangle<float> text, midi, audio, edit, play, trash; };
    Rects rectsFor (int i) const
    {
        Rects r;
        auto in = Rectangle<float> (0.0f, (float) (i * rowH), (float) getWidth(), (float) rowH - 6.0f).withTrimmedBottom (2.5f);
        r.trash = in.removeFromRight (36.0f);
        r.play = in.removeFromRight (36.0f);
        r.edit = in.removeFromRight (36.0f);
        in.removeFromRight (4.0f);
        r.audio = in.removeFromRight (70.0f).reduced (0.0f, 12.0f);
        in.removeFromRight (6.0f);
        r.midi = in.removeFromRight (62.0f).reduced (0.0f, 12.0f);
        in.removeFromRight (8.0f);
        r.text = in.withTrimmedLeft (14.0f);
        return r;
    }

    void paint (Graphics& g) override
    {
        auto& ctx = owner.ctx;
        const auto& pal = ctx.vis.pal;
        const bool bouncing = owner.rec.isBouncing();
        for (int i = 0; i < (int) owner.rows.size(); ++i)
        {
            const auto& row = owner.rows[(size_t) i];
            auto r = Rectangle<float> (0.0f, (float) (i * rowH), (float) getWidth(), (float) rowH - 6.0f);
            const bool sel = i == owner.selected;
            draw::button (g, r, pal, 10.0f, 0.0f, false, i == hover ? 0.6f : 0.0f);
            if (sel)
            {
                g.setColour (pal.accent);
                g.drawRoundedRectangle (r.withTrimmedBottom (2.5f).reduced (1.0f), 10.0f, 2.0f);
            }
            const auto rc = rectsFor (i);
            const auto txt = pal.btnText;

            // Chips para arrastrar
            auto chip = [&] (Rectangle<float> cr, const String& label, bool filled)
            {
                g.setColour (filled ? pal.accent : txt.withAlpha (0.08f));
                g.fillRoundedRectangle (cr, cr.getHeight() * 0.5f);
                if (! filled) { g.setColour (txt.withAlpha (0.35f)); g.drawRoundedRectangle (cr.reduced (0.5f), cr.getHeight() * 0.5f, 1.0f); }
                g.setColour (filled ? pal.accentInk : txt.withAlpha (0.8f));
                g.setFont (ctx.fonts.ui (12.5f, 2));
                g.drawText (label, cr, Justification::centred, false);
            };
            chip (rc.midi, "MIDI", true);
            const bool thisBouncing = bouncing && owner.rec.bouncingFile() == MidiRecorder::audioFileFor (row.file);
            if (thisBouncing) chip (rc.audio, String (roundToInt (owner.rec.bounceProgress() * 100.0f)) + " %", false);
            else chip (rc.audio, row.hasAudio ? "Audio" : U8 ("→ Audio"), row.hasAudio);

            draw::icon (g, "pencil", rc.edit.reduced (9.0f, 15.0f), txt.withAlpha (0.85f), 1.5f);
            const bool playingThis = owner.isPlaying && owner.isPlaying() && playingRow == i;
            draw::icon (g, playingThis ? "stop" : "play", rc.play.reduced (10.0f, 16.0f), txt.withAlpha (0.85f));
            draw::icon (g, "trash", rc.trash.reduced (9.0f, 15.0f), (confirmRow == i ? Colour (0xffe5383b) : txt.withAlpha (0.7f)), 1.5f);

            auto tr = rc.text;
            g.setColour (sel ? pal.accent : txt);
            g.setFont (ctx.fonts.ui (14.5f, 2));
            g.drawText (confirmRow == i ? U8 ("¿Borrar? Toca otra vez") : row.title,
                        tr.removeFromTop (tr.getHeight() * 0.55f), Justification::bottomLeft, true);
            g.setColour (txt.withAlpha (0.65f));
            g.setFont (ctx.fonts.ui (12.5f, 1));
            g.drawText (row.info, tr, Justification::topLeft, true);
        }
    }

    int rowAt (Point<int> p) const { const int i = p.y / rowH; return i >= 0 && i < (int) owner.rows.size() ? i : -1; }
    Zone zoneAt (int i, Point<int> p) const
    {
        const auto rc = rectsFor (i);
        const auto pf = p.toFloat();
        if (rc.trash.contains (pf)) return trashZone;
        if (rc.play.contains (pf)) return playZone;
        if (rc.edit.contains (pf)) return editZone;
        if (rc.audio.expanded (2.0f, 12.0f).contains (pf)) return audioZone;
        if (rc.midi.expanded (2.0f, 12.0f).contains (pf)) return midiZone;
        return rowZone;
    }

    void mouseMove (const MouseEvent& e) override
    {
        const int h = rowAt (e.getPosition());
        if (h != hover) { hover = h; repaint(); }
        MouseCursor c = MouseCursor::NormalCursor;
        if (h >= 0)
        {
            const auto z = zoneAt (h, e.getPosition());
            const bool audioReady = owner.rows[(size_t) h].hasAudio;
            c = (z == midiZone || z == rowZone || (z == audioZone && audioReady)) ? MouseCursor::DraggingHandCursor : MouseCursor::PointingHandCursor;
        }
        setMouseCursor (c);
    }
    void mouseExit (const MouseEvent&) override { hover = -1; repaint(); }
    void mouseDoubleClick (const MouseEvent& e) override
    {
        const int i = rowAt (e.getPosition());
        if (i >= 0 && zoneAt (i, e.getPosition()) == rowZone && owner.onEdit) owner.onEdit (owner.rows[(size_t) i].file);
    }

    void mouseDown (const MouseEvent& e) override
    {
        dragging = false;
        downRow = rowAt (e.getPosition());
        downZone = downRow >= 0 ? zoneAt (downRow, e.getPosition()) : rowZone;
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (dragging || downRow < 0 || e.getDistanceFromDragStart() < 5) return;
        // Arrastrar: la fila o el chip MIDI llevan el .mid; el chip Audio lleva el .wav
        File f = owner.rows[(size_t) downRow].file;
        if (downZone == audioZone) f = MidiRecorder::freshAudioFor (f);
        else if (downZone != midiZone && downZone != rowZone) return;
        if (! f.existsAsFile()) return;
        dragging = true;
        DragAndDropContainer::performExternalDragDropOfFiles ({ MidiRecorder::dragCopy (f).getFullPathName() }, false, this);
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (dragging) { dragging = false; return; }
        const int i = rowAt (e.getPosition());
        if (i < 0 || i != downRow) return;
        const auto z = zoneAt (i, e.getPosition());
        const auto file = owner.rows[(size_t) i].file;
        if (z == trashZone)
        {
            if (confirmRow == i)
            {
                file.moveToTrash();   // a la papelera (se puede recuperar)
                MidiRecorder::audioFileFor (file).moveToTrash();
                confirmRow = -1;
                owner.refreshList();
            }
            else { confirmRow = i; confirmAt = Time::getMillisecondCounter(); repaint(); }
            return;
        }
        confirmRow = -1;
        owner.select (i);
        if (z == editZone && owner.onEdit) { owner.onEdit (file); return; }
        if (z == audioZone && ! owner.rows[(size_t) i].hasAudio) { owner.toAudio (file); repaint(); return; }
        if (z == midiZone || z == audioZone) { owner.statusText = U8 ("Arrastra el chip a Ableton o FL Studio"); owner.repaint(); return; }
        if (z == playZone)
        {
            if (owner.isPlaying && owner.isPlaying() && playingRow == i) { if (owner.onStopPlay) owner.onStopPlay(); playingRow = -1; }
            else if (owner.onPlay) { owner.onPlay (file); playingRow = i; }
            repaint();
        }
    }

    void tick()
    {
        if (confirmRow >= 0 && Time::getMillisecondCounter() - confirmAt > 3500) { confirmRow = -1; repaint(); }
        if (playingRow >= 0 && owner.isPlaying && ! owner.isPlaying()) { playingRow = -1; repaint(); }
        if (owner.rec.isBouncing()) repaint();
    }

    RecordingsPanel& owner;
    int hover = -1, downRow = -1, confirmRow = -1, playingRow = -1;
    Zone downZone = rowZone;
    uint32 confirmAt = 0;
    bool dragging = false;
};

RecordingsPanel::RecordingsPanel (UiContext& c, Prefs& p, MidiRecorder& r, MetronomeSettings& ms, std::function<void()> ch)
    : ctx (c), prefs (p), rec (r), metro (ms), changed (std::move (ch)),
      recBtn (c, "rec", U8 ("Grabar o detener"), "Grabar"),
      folderBtn (c, "folder", U8 ("Abrir la carpeta de grabaciones"), "Abrir carpeta"),
      newBtn (c, "pencil", U8 ("Hacer un MIDI nuevo desde cero en el piano roll"), "Nuevo MIDI"),
      editBtn (c, "pencil", U8 ("Abrir esta grabación en el piano roll"), "Editar piano"),
      countSw (c), mouseSw (c), audioSw (c),
      midiChip (c, U8 ("Arrastra MIDI"), "export"),
      audioChip (c, U8 ("Arrastra audio"), "wave"),
      clipView (c)
{
    list = std::make_unique<List> (*this);
    viewport.setViewedComponent (list.get(), false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);
    for (Component* comp : std::initializer_list<Component*> { &recBtn, &folderBtn, &newBtn, &editBtn, &countSw, &mouseSw, &midiChip, &audioChip, &viewport, &clipView })
        addAndMakeVisible (comp);
    addChildComponent (audioSw);   // grabar audio ya no se ofrece
    midiChip.compact = audioChip.compact = true;
    midiChip.text = "MIDI";
    audioChip.text = "Audio";

    recBtn.activeColour = Colour (0xffe5383b);
    recBtn.iconColour = Colour (0xffe5383b);
    recBtn.onClick = [this] { rec.toggle(); timerCallback(); };
    folderBtn.onClick = [] { auto f = MidiRecorder::folder(); f.createDirectory(); f.startAsProcess(); };
    newBtn.onClick = [this] { if (onNew) onNew(); };
    editBtn.onClick = [this] { if (selectedFile().existsAsFile() && onEdit) onEdit (selectedFile()); };
    clipView.onOpen = [this] { if (selectedFile().existsAsFile() && onEdit) onEdit (selectedFile()); };
    clipView.onEdited = [this] { refreshList(); };

    countSw.setOn (prefs.countIn);
    countSw.onChange = [this] (bool v) { prefs.countIn = v; prefs.save (false); if (changed) changed(); };
    mouseSw.setOn (prefs.recordMouse);
    mouseSw.onChange = [this] (bool v) { prefs.recordMouse = v; prefs.save (false); if (changed) changed(); };

    midiChip.getFile = [this] { return selectedFile(); };
    midiChip.setTooltip (U8 ("Arrástralo a tu DAW (Ableton, FL Studio, Logic…). Lleva la tonalidad en el nombre"));
    audioChip.getFile = [this] { return MidiRecorder::freshAudioFor (selectedFile()); };
    audioChip.textWithoutFile = U8 ("Pasar a audio");
    audioChip.onClickWithoutFile = [this] { toAudio (selectedFile()); };
    audioChip.setTooltip (U8 ("El audio (WAV) con el sonido elegido, para usarlo como sample"));

    setSize (1000, 560);
    refreshList (true);
    startTimerHz (10);
}

RecordingsPanel::~RecordingsPanel() { stopTimer(); }

File RecordingsPanel::selectedFile() const
{
    return selected >= 0 && selected < (int) rows.size() ? rows[(size_t) selected].file : File();
}

void RecordingsPanel::toAudio (const File& midi)
{
    if (rec.isBouncing())
    {
        rec.cancelBounce();
        statusText = U8 ("Se canceló el paso a audio");
    }
    else if (rec.state() != 0) statusText = U8 ("Primero termina de grabar");
    else if (rec.startBounce (midi)) statusText = U8 ("Pasando a audio con el sonido elegido (se oye mientras)…");
    else statusText = U8 ("No se pudo pasar a audio");
    repaint();
}

void RecordingsPanel::refreshList (bool selectNewest)
{
    const auto keep = selectedFile();
    rows.clear();
    for (auto& f : MidiRecorder::recordings())
    {
        Row row;
        row.file = f;
        row.title = f.getFileNameWithoutExtension().replace ("MidiCat ", "").replace (U8 ("Midi Cat "), "");
        row.hasAudio = MidiRecorder::freshAudioFor (f) != File();
        MidiClip clip;
        if (clip.load (f))
            row.info = timeText (clip.seconds) + U8 ("  ·  ") + String ((int) clip.notes.size()) + " notas  ·  " + String (roundToInt (clip.bpm)) + " BPM";
        rows.push_back (row);
    }
    selected = -1;
    for (int i = 0; i < (int) rows.size(); ++i) if (rows[(size_t) i].file == keep) selected = i;
    if ((selectNewest || selected < 0) && ! rows.empty()) selected = 0;
    lastCount = (int) rows.size();
    lastFolderTime = MidiRecorder::folder().getLastModificationTime();
    list->setSize (jmax (10, viewport.getWidth() - 10), jmax (10, (int) rows.size() * List::rowH));
    if (selected >= 0 && rows[(size_t) selected].file != clipView.getFile()) clipView.setFile (rows[(size_t) selected].file);
    if (selected < 0) clipView.setFile ({});
    for (Component* comp : std::initializer_list<Component*> { &editBtn, &midiChip, &audioChip }) comp->setVisible (! rows.empty());
    list->repaint();
    midiChip.repaint();
    audioChip.repaint();
    repaint();
}

void RecordingsPanel::select (int i)
{
    selected = i;
    clipView.setFile (rows[(size_t) i].file);
    list->repaint();
    midiChip.repaint();
    audioChip.repaint();
}

void RecordingsPanel::timerCallback()
{
    const int s = rec.state();
    recBtn.setActive (s != 0);
    recBtn.setIcon (s != 0 ? "stop" : "rec");
    recBtn.setLabel (s == 0 ? "Grabar" : "Detener");
    String st;
    const bool bouncing = rec.isBouncing();
    if (s == 1) st = U8 ("Cuenta: ") + String (rec.beatsLeftInCountIn()) + U8 ("…");
    else if (s == 2) st = U8 ("Grabando  ") + timeText (rec.elapsedSeconds());
    else if (bouncing) st = U8 ("Pasando a audio… ") + String (roundToInt (rec.bounceProgress() * 100.0f)) + " %";
    else if (wasBouncing) st = U8 ("Listo: el audio quedó junto al MIDI");
    else if (statusText.startsWith ("Arrastra") || statusText.startsWith ("Se cancel") || statusText.startsWith ("No se") || statusText.startsWith ("Primero")) st = statusText;
    else st = metro.on.load() ? U8 ("Listo · cae justo en el compás") : U8 ("Listo · empieza en tu primera nota");
    if (bouncing != wasBouncing) { wasBouncing = bouncing; if (! bouncing) refreshList(); }
    audioChip.progress = bouncing && rec.bouncingFile() == MidiRecorder::audioFileFor (selectedFile()) ? rec.bounceProgress() : -1.0f;
    audioChip.repaint();
    if (st != statusText) { statusText = st; repaint (0, 0, getWidth(), 60); }

    const auto ft = MidiRecorder::folder().getLastModificationTime();
    if (ft != lastFolderTime || (int) MidiRecorder::recordings().size() != lastCount)
        refreshList (! bouncing);
    list->tick();
}

void RecordingsPanel::resized()
{
    recBtn.setBounds (0, 0, 170, 52);
    folderBtn.setBounds (getWidth() - 180, 0, 180, 52);
    newBtn.setBounds (getWidth() - 180 - 10 - 160, 0, 160, 52);

    // Dos opciones en fila
    const int y = 76;
    countSw.setBounds (240, y, 58, 30);
    mouseSw.setBounds (690, y, 58, 30);

    auto body = getLocalBounds().withTrimmedTop (128);
    auto left = body.removeFromLeft (440);
    viewport.setBounds (left.withTrimmedBottom (30));
    list->setSize (jmax (10, viewport.getWidth() - 10), jmax (10, (int) rows.size() * List::rowH));
    body.removeFromLeft (16);
    // Arriba de la vista: arrastrar MIDI (a la mano), audio y editar
    auto top = body.removeFromTop (40);
    midiChip.setBounds (top.removeFromLeft (124));
    top.removeFromLeft (8);
    audioChip.setBounds (top.removeFromLeft (150));
    editBtn.setBounds (top.removeFromRight (160));
    body.removeFromTop (10);
    clipView.setBounds (body);
}

void RecordingsPanel::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.setColour (rec.state() != 0 ? Colour (0xffe5383b) : pal.text);
    g.setFont (ctx.fonts.ui (16.0f, 2));
    g.drawText (statusText, Rectangle<float> (186.0f, 0.0f, (float) getWidth() - 186.0f - 360.0f, 52.0f), Justification::centredLeft, true);

    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (14.0f, 1));
    g.drawText (U8 ("Cuenta de 1 compás"), Rectangle<float> (0.0f, 76.0f, 228.0f, 30.0f), Justification::centredRight, false);
    g.drawText (U8 ("Grabar el mouse (teclas y grados)"), Rectangle<float> (330.0f, 76.0f, 348.0f, 30.0f), Justification::centredRight, false);

    if (rows.empty())
    {
        g.setColour (pal.dim);
        g.setFont (ctx.fonts.ui (15.0f, 1));
        g.drawFittedText (U8 ("Todavía no hay grabaciones.\nDale al botón rojo y toca algo."),
                          Rectangle<int> (0, 150, 440, 120), Justification::centred, 3);
    }
    label (g, ctx, U8 ("Arrastra MIDI o Audio directo a tu DAW (lleva la tonalidad en el nombre)"), Rectangle<float> (0.0f, (float) getHeight() - 24.0f, 440.0f, 22.0f), false);
}

//==============================================================================
// Tus instrumentos
//==============================================================================
class PluginsPanel::List : public Component
{
public:
    List (PluginsPanel& p) : owner (p) {}
    static constexpr int rowH = 52;

    void rebuild()
    {
        items.clear();
        const auto f = owner.filter.getText().trim();
        for (auto& d : owner.host.instruments())
            if (f.isEmpty() || d.name.containsIgnoreCase (f) || d.manufacturerName.containsIgnoreCase (f))
                items.add (d);
        setSize (jmax (10, getWidth()), jmax (10, items.size() * rowH));
        repaint();
    }

    void paint (Graphics& g) override
    {
        auto& ctx = owner.ctx;
        const auto& pal = ctx.vis.pal;
        for (int i = 0; i < items.size(); ++i)
        {
            const auto& d = items.getReference (i);
            const bool cur = owner.host.hasInstrument() && owner.host.description().fileOrIdentifier == d.fileOrIdentifier && owner.host.description().uniqueId == d.uniqueId;
            auto r = Rectangle<float> (0.0f, (float) (i * rowH), (float) getWidth(), (float) rowH - 6.0f);
            draw::button (g, r, pal, 10.0f, 0.0f, cur, i == hover ? 0.6f : 0.0f);
            auto in = r.withTrimmedBottom (2.5f).reduced (14.0f, 0.0f);
            const auto txt = cur ? pal.accentInk : pal.btnText;
            auto fmt = in.removeFromRight (110.0f);
            g.setColour (txt);
            g.setFont (ctx.fonts.ui (15.5f, 2));
            g.drawText (d.name, in.removeFromLeft (in.getWidth() * 0.58f), Justification::centredLeft, true);
            g.setColour (txt.withAlpha (0.65f));
            g.setFont (ctx.fonts.ui (13.5f, 1));
            g.drawText (d.manufacturerName, in, Justification::centredLeft, true);
            g.drawText (d.pluginFormatName == "AudioUnit" ? "AU" : d.pluginFormatName, fmt, Justification::centredRight, false);
        }
    }

    void mouseMove (const MouseEvent& e) override { const int h = e.y / rowH; if (h != hover) { hover = h < items.size() ? h : -1; repaint(); } }
    void mouseExit (const MouseEvent&) override { hover = -1; repaint(); }
    void mouseUp (const MouseEvent& e) override
    {
        const int i = e.y / rowH;
        if (i < 0 || i >= items.size() || owner.host.isScanning()) return;
        const auto d = items[i];
        owner.message = U8 ("Abriendo ") + d.name + U8 ("…");
        owner.repaint();
        SafePointer<PluginsPanel> safe (&owner);
        owner.host.load (d, [safe, d] (const String& err)
        {
            if (auto* p = safe.getComponent())
            {
                p->message = err.isEmpty() ? U8 ("Ahora suena ") + d.name + "." : U8 ("No se pudo abrir: ") + err;
                p->refresh();
            }
        });
    }

    PluginsPanel& owner;
    Array<PluginDescription> items;
    int hover = -1;
};

PluginsPanel::PluginsPanel (UiContext& c, PluginHost& h, std::function<void()> open, std::function<void()> back)
    : ctx (c), host (h), openWindow (std::move (open)), backToPiano (std::move (back)),
      scanBtn (c, "plug", U8 ("Buscar tus instrumentos VST3 y Audio Units"), "Buscar instrumentos"),
      openBtn (c, "windows", U8 ("Abrir la ventana del instrumento"), "Abrir ventana"),
      pianoBtn (c, "keys", U8 ("Volver a los pianos de MidiCat"), "Pianos de MidiCat")
{
    list = std::make_unique<List> (*this);
    viewport.setViewedComponent (list.get(), false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);
    styleEditor (filter, ctx.vis.pal, ctx.fonts.ui (15.0f, 1));
    filter.setTextToShowWhenEmpty (U8 ("Buscar por nombre…"), ctx.vis.pal.dim);
    filter.onTextChange = [this] { list->rebuild(); };
    for (Component* comp : std::initializer_list<Component*> { &scanBtn, &openBtn, &pianoBtn, &filter, &viewport })
        addAndMakeVisible (comp);
    scanBtn.setActive (true);
    scanBtn.onClick = [this] { host.startScan(); scanRequested = true; message = U8 ("Buscando… (puede tardar un poco la primera vez)"); refresh(); };
    openBtn.onClick = [this] { if (openWindow) openWindow(); };
    pianoBtn.onClick = [this] { if (backToPiano) backToPiano(); message = U8 ("Ahora suenan los pianos de MidiCat."); refresh(); };
    setSize (680, 500);
    refresh();
    startTimerHz (8);
}

PluginsPanel::~PluginsPanel() { stopTimer(); }

void PluginsPanel::refresh()
{
    openBtn.setVisible (host.hasInstrument());
    pianoBtn.setVisible (host.hasInstrument());
    list->rebuild();
    repaint();
}

void PluginsPanel::timerCallback()
{
    // Mientras busca: solo se mueve la barrita; la lista se rehace cuando cambia (y a lo mucho 2 veces por segundo)
    const bool s = host.isScanning();
    if (s)
    {
        repaint (0, 56, getWidth(), 44);
        if (++tick % 4 == 0 && host.instruments().size() != list->items.size()) list->rebuild();
    }
    if (s != wasScanning || (scanRequested && ! s))
    {
        if (! s && (wasScanning || scanRequested))
        {
            message = String (host.instruments().size()) + U8 (" instrumentos encontrados.");
            scanRequested = false;
        }
        wasScanning = s;
        scanBtn.setAlpha (s ? 0.5f : 1.0f);
        refresh();
    }
}

void PluginsPanel::resized()
{
    scanBtn.setBounds (0, 0, 230, 50);
    filter.setBounds (getWidth() - 240, 4, 240, 42);
    auto bottom = getLocalBounds().removeFromBottom (50);
    openBtn.setBounds (bottom.removeFromLeft (190));
    bottom.removeFromLeft (10);
    pianoBtn.setBounds (bottom.removeFromLeft (220));
    viewport.setBounds (0, 104, getWidth(), getHeight() - 104 - 62);
    list->setSize (viewport.getWidth() - 10, jmax (10, list->items.size() * List::rowH));
}

void PluginsPanel::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto info = Rectangle<float> (0.0f, 60.0f, (float) getWidth(), 34.0f);
    if (host.isScanning())
    {
        auto bar = info.removeFromLeft (220.0f).withSizeKeepingCentre (220.0f, 10.0f);
        g.setColour (pal.text.withAlpha (0.12f));
        g.fillRoundedRectangle (bar, 5.0f);
        g.setColour (pal.accent);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * jlimit (0.03f, 1.0f, host.scanProgress())), 5.0f);
        info.removeFromLeft (14.0f);
        g.setColour (pal.text);
        g.setFont (ctx.fonts.ui (14.5f, 1));
        g.drawText (U8 ("Revisando: ") + host.scanningName(), info, Justification::centredLeft, true);
    }
    else
    {
        g.setColour (pal.text);
        g.setFont (ctx.fonts.ui (14.5f, 1));
        auto t = message;
        if (t.isEmpty())
            t = host.hasInstrument() ? U8 ("Suena: ") + host.instrumentName()
                                     : (list->items.isEmpty() ? U8 ("Dale a \"Buscar instrumentos\" para ver los VST3 y AU de tu compu.")
                                                              : U8 ("Elige un instrumento para tocar con él."));
        g.drawText (t, info, Justification::centredLeft, true);
    }
    if (! host.isScanning() && list->items.isEmpty())
    {
        g.setColour (pal.dim);
        g.setFont (ctx.fonts.ui (14.0f, 1));
        g.drawFittedText (U8 ("MidiCat busca en las carpetas normales de plugins.\nSi un plugin hace que se cierre, la próxima vez lo salta."),
                          Rectangle<int> (0, 150, getWidth(), 80), Justification::centred, 3);
    }
}
