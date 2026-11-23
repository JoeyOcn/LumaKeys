#include "PianoRoll.h"

using namespace juce;

std::vector<PianoRoll::Note> PianoRoll::clipboard;

static constexpr float tbH = 60.0f, rulerH = 26.0f, keysW = 74.0f, laneH = 112.0f, sbW = 12.0f, gapH = 6.0f;
static const char* pcNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static bool isBlack (int n) { const int pc = n % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }

struct SnapOption { const char* name; double beats; };
static const SnapOption snapOptions[] = {
    { "Sin ajuste", 0.0 },
    { "1/32", 0.125 },
    { "1/16 (paso)", 0.25 },
    { "1/8", 0.5 },
    { "1/4 (tiempo)", 1.0 },
    { "1/2", 2.0 },
    { "Compás", -1.0 },   // un compás completo
    { "1/16 tresillo", 1.0 / 6.0 },
    { "1/8 tresillo", 1.0 / 3.0 },
};
static constexpr int numSnaps = (int) (sizeof (snapOptions) / sizeof (snapOptions[0]));

//==============================================================================
class PianoRoll::DragOut : public Component, public SettableTooltipClient
{
public:
    explicit DragOut (PianoRoll& o) : owner (o)
    {
        setMouseCursor (MouseCursor::DraggingHandCursor);
        setTooltip (U8 ("Arrastra este MIDI a la vista Arrangement de Ableton o a la Playlist de FL Studio"));
    }
    void paint (Graphics& g) override
    {
        const auto& pal = owner.ctx.vis.pal;
        auto b = getLocalBounds().toFloat().reduced (1.0f);
        draw::button (g, b, pal, b.getHeight() * 0.3f, down ? 1.0f : 0.0f, true, 0.0f);
        auto c = b.withTrimmedBottom (2.5f).translated (0.0f, 2.5f * (down ? 1.0f : 0.0f));
        auto ir = c.removeFromLeft (c.getHeight());
        draw::icon (g, "export", ir.reduced (ir.getWidth() * 0.26f), pal.accentInk, 1.6f);
        g.setColour (pal.accentInk);
        g.setFont (draw::fitUi (owner.ctx.fonts, U8 ("Arrastra al DAW"), 14.5f, c.getWidth() - 8.0f, 2));
        g.drawText (U8 ("Arrastra al DAW"), c, Justification::centredLeft, false);
    }
    void mouseDown (const MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const MouseEvent&) override { down = false; repaint(); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (started || e.getDistanceFromDragStart() < 5) return;
        started = true;
        if (owner.file.existsAsFile())
            DragAndDropContainer::performExternalDragDropOfFiles ({ owner.file.getFullPathName() }, false, this, [this] { started = false; down = false; repaint(); });
        else started = false;
    }
private:
    PianoRoll& owner;
    bool down = false, started = false;
};

//==============================================================================
PianoRoll::PianoRoll (UiContext& c, MidiKeyboardState& k)
    : ctx (c), kb (k),
      drawBtn (c, "pencil", U8 ("Lápiz (P): clic para poner notas, arrastra para moverlas, la orilla derecha las alarga")),
      selectBtn (c, "cursor", U8 ("Seleccionar (E): arrastra un cuadro para elegir varias notas")),
      sliceBtn (c, "knife", U8 ("Cortar (C): clic o arrastra hacia abajo donde quieras partir las notas")),
      eraseBtn (c, "eraser", U8 ("Borrar (D): clic o arrastra sobre las notas (el clic derecho también borra)")),
      snapBtn (c, "magnet", U8 ("Ajuste a la rejilla")),
      quantBtn (c, "quant", U8 ("Cuantizar (Ctrl+Q): acomoda el inicio de las notas a la rejilla"), "Cuantizar"),
      undoBtn (c, "undo", U8 ("Deshacer (Ctrl+Z)")),
      redoBtn (c, "redo", U8 ("Rehacer (Ctrl+Y)")),
      playBtn (c, "play", U8 ("Reproducir / detener (barra espaciadora)"))
{
    setWantsKeyboardFocus (true);
    dragOut = std::make_unique<DragOut> (*this);
    for (Component* comp : std::initializer_list<Component*> { &drawBtn, &selectBtn, &sliceBtn, &eraseBtn, &snapBtn, &quantBtn, &undoBtn, &redoBtn, &playBtn, dragOut.get(), &hbar, &vbar })
        addAndMakeVisible (comp);
    for (auto* b : { &drawBtn, &selectBtn, &sliceBtn, &eraseBtn, &snapBtn, &quantBtn, &undoBtn, &redoBtn, &playBtn })
        b->setWantsKeyboardFocus (false);

    drawBtn.onClick   = [this] { setTool (draw); };
    selectBtn.onClick = [this] { setTool (select); };
    sliceBtn.onClick  = [this] { setTool (slice); };
    eraseBtn.onClick  = [this] { setTool (erase); };
    snapBtn.onClick   = [this] { showSnapMenu(); };
    quantBtn.onClick  = [this] { quantize(); };
    undoBtn.onClick   = [this] { undo(); };
    redoBtn.onClick   = [this] { redo(); };
    playBtn.onClick   = [this] { togglePlay(); };
    snapBtn.setLabel (snapName());
    setTool (draw);

    hbar.addListener (this);
    vbar.addListener (this);
    hbar.setAutoHide (false);
    vbar.setAutoHide (false);
    startTimerHz (30);
}

PianoRoll::~PianoRoll()
{
    stopPlay();
    Timer::stopTimer();
    if (previewNote >= 0) kb.noteOff (1, previewNote, 0.0f);
}

//==============================================================================
// Archivo
//==============================================================================
bool PianoRoll::loadFile (const File& f)
{
    stopPlay();
    file = f;
    notes.clear();
    others.clear();
    undoStack.clear();
    redoStack.clear();
    bpm = 120.0;
    beatsPerBar = 4;

    MidiFile mf;
    FileInputStream in (f);
    if (in.openedOk() && mf.readFrom (in) && mf.getTimeFormat() > 0)
    {
        const double tpq = (double) mf.getTimeFormat();
        bool tempoSet = false;
        for (int t = 0; t < mf.getNumTracks(); ++t)
        {
            MidiMessageSequence seq (*mf.getTrack (t));
            seq.updateMatchedPairs();
            for (int i = 0; i < seq.getNumEvents(); ++i)
            {
                auto* e = seq.getEventPointer (i);
                const auto& m = e->message;
                const double beat = m.getTimeStamp() / tpq;
                if (m.isTempoMetaEvent()) { if (! tempoSet) { bpm = 60.0 / jmax (0.0001, m.getTempoSecondsPerQuarterNote()); tempoSet = true; } }
                else if (m.isTimeSignatureMetaEvent()) { int num = 4, den = 4; m.getTimeSignatureInfo (num, den); beatsPerBar = jlimit (1, 16, num); }
                else if (m.isNoteOn())
                {
                    Note n;
                    n.note = m.getNoteNumber();
                    n.velocity = m.getVelocity();
                    n.start = beat;
                    const double end = e->noteOffObject != nullptr ? e->noteOffObject->message.getTimeStamp() / tpq : beat + 0.25;
                    n.length = jmax (1.0 / 64.0, end - beat);
                    notes.push_back (n);
                }
                else if (! m.isNoteOff() && ! m.isMetaEvent() && ! m.isSysEx())
                    others.push_back ({ beat, m });
            }
        }
    }
    std::sort (notes.begin(), notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });

    // Vista: que se vean todas las notas (si caben)
    int hi = 0, lo = 127;
    for (auto& n : notes) { hi = jmax (hi, n.note); lo = jmin (lo, n.note); }
    if (notes.empty()) { hi = 79; lo = 48; }
    const float gh = jmax (200.0f, gridArea().getHeight());
    rowH = jlimit (9.0f, 22.0f, gh / (float) (hi - lo + 7));
    topNote = jlimit (24, 127, hi + 3);
    scrollBeat = 0.0;
    const float gw = jmax (300.0f, gridArea().getWidth());
    pxPerBeat = jlimit (18.0f, 140.0f, gw / (float) jmax (8.0, totalBeats() - (double) beatsPerBar));
    cursorBeat = 0.0;
    status = notes.empty() ? U8 ("Vacío: dibuja notas con el lápiz") : String ((int) notes.size()) + " notas";
    updateScrollBars();
    repaint();
    return true;
}

void PianoRoll::save()
{
    if (file == File()) return;
    const int tpq = 960;
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::tempoMetaEvent ((int) std::llround (60000000.0 / bpm)), 0.0);
    seq.addEvent (MidiMessage::timeSignatureMetaEvent (beatsPerBar, 4), 0.0);
    seq.addEvent (MidiMessage::textMetaEvent (3, "Midi Cat"), 0.0);
    for (auto& o : others)
    {
        auto m = o.msg;
        m.setTimeStamp (std::round (o.beat * tpq));
        seq.addEvent (m);
    }
    for (auto& n : notes)
    {
        const double s = std::round (n.start * tpq);
        const double e = jmax (s + 1.0, std::round ((n.start + n.length) * tpq));
        seq.addEvent (MidiMessage::noteOn (1, n.note, (uint8) jlimit (1, 127, n.velocity)), s);
        seq.addEvent (MidiMessage::noteOff (1, n.note), e);
    }
    seq.sort();
    seq.updateMatchedPairs();
    MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (seq);

    TemporaryFile tmp (file);
    bool ok = false;
    {
        FileOutputStream os (tmp.getFile());
        if (os.openedOk())
        {
            mf.writeTo (os, 1);
            os.flush();
            ok = os.getStatus().wasOk();
        }
    }
    ok = ok && tmp.overwriteTargetFileWithTemporary();
    status = ok ? U8 ("Guardado  ·  ") + String ((int) notes.size()) + " notas" : U8 ("No se pudo guardar");
    if (ok && onSaved) onSaved();
    repaint (0, 0, getWidth(), (int) tbH);
}

void PianoRoll::pushUndo()
{
    undoStack.push_back (notes);
    if (undoStack.size() > 200) undoStack.erase (undoStack.begin());
    redoStack.clear();
}

void PianoRoll::undo()
{
    if (undoStack.empty()) return;
    redoStack.push_back (notes);
    notes = undoStack.back();
    undoStack.pop_back();
    save();
    repaint();
}

void PianoRoll::redo()
{
    if (redoStack.empty()) return;
    undoStack.push_back (notes);
    notes = redoStack.back();
    redoStack.pop_back();
    save();
    repaint();
}

//==============================================================================
// Edición
//==============================================================================
void PianoRoll::deleteSelected()
{
    bool any = false;
    for (auto& n : notes) any = any || n.selected;
    if (! any) return;
    pushUndo();
    notes.erase (std::remove_if (notes.begin(), notes.end(), [] (const Note& n) { return n.selected; }), notes.end());
    save();
    repaint();
}

void PianoRoll::selectAll (bool sel)
{
    for (auto& n : notes) n.selected = sel;
    repaint();
}

void PianoRoll::copySelected (bool cut)
{
    clipboard.clear();
    double first = 1.0e9;
    for (auto& n : notes) if (n.selected) { clipboard.push_back (n); first = jmin (first, n.start); }
    for (auto& n : clipboard) { n.start -= first; n.selected = false; }
    if (cut) deleteSelected();
    else status = String ((int) clipboard.size()) + " notas copiadas";
    repaint (0, 0, getWidth(), (int) tbH);
}

void PianoRoll::paste()
{
    if (clipboard.empty()) return;
    pushUndo();
    selectAll (false);
    for (auto n : clipboard)
    {
        n.start += cursorBeat;
        n.selected = true;
        notes.push_back (n);
    }
    double end = cursorBeat;
    for (auto& n : clipboard) end = jmax (end, cursorBeat + n.start + n.length);
    cursorBeat = snapRound (end);
    save();
    updateScrollBars();
    repaint();
}

void PianoRoll::duplicate()
{
    // Como Ctrl+B en FL: copia lo elegido justo después
    double first = 1.0e9, last = 0.0;
    std::vector<Note> sel;
    for (auto& n : notes) if (n.selected) { sel.push_back (n); first = jmin (first, n.start); last = jmax (last, n.start + n.length); }
    if (sel.empty()) return;
    pushUndo();
    const double bar = (double) beatsPerBar;
    double span = last - first;
    span = std::ceil (span / jmax (0.0625, snapValue() > 0.0 ? snapValue() : 0.25) - 1.0e-6) * jmax (0.0625, snapValue() > 0.0 ? snapValue() : 0.25);
    if (span >= bar * 0.75) span = std::ceil (span / bar - 1.0e-6) * bar;
    for (auto& n : notes) n.selected = false;
    for (auto n : sel) { n.start += span; n.selected = true; notes.push_back (n); }
    save();
    updateScrollBars();
    repaint();
}

void PianoRoll::quantize()
{
    const double q = snapValue() > 0.0 ? snapValue() : 0.25;
    bool anySel = false;
    for (auto& n : notes) anySel = anySel || n.selected;
    pushUndo();
    for (auto& n : notes)
        if (! anySel || n.selected)
        {
            const double end = n.start + n.length;
            n.start = std::round (n.start / q) * q;
            n.length = jmax (q, std::round (end / q) * q - n.start);
        }
    status = U8 ("Cuantizado a ") + snapName();
    save();
    repaint();
}

void PianoRoll::transpose (int semis)
{
    bool any = false;
    for (auto& n : notes) any = any || n.selected;
    if (! any) return;
    pushUndo();
    for (auto& n : notes) if (n.selected) n.note = jlimit (0, 127, n.note + semis);
    for (auto& n : notes) if (n.selected) { previewPitch (n.note, n.velocity); break; }
    save();
    repaint();
}

void PianoRoll::nudge (int steps)
{
    bool any = false;
    for (auto& n : notes) any = any || n.selected;
    if (! any) return;
    const double q = snapValue() > 0.0 ? snapValue() : 0.25;
    pushUndo();
    for (auto& n : notes) if (n.selected) n.start = jmax (0.0, n.start + q * steps);
    save();
    repaint();
}

void PianoRoll::setTool (Tool t)
{
    tool = t;
    drawBtn.setActive (t == draw);
    selectBtn.setActive (t == select);
    sliceBtn.setActive (t == slice);
    eraseBtn.setActive (t == erase);
}

double PianoRoll::snapValue() const
{
    const double v = snapOptions[jlimit (0, numSnaps - 1, snapIndex)].beats;
    return v < 0.0 ? (double) beatsPerBar : v;
}

String PianoRoll::snapName() const { return String::fromUTF8 (snapOptions[jlimit (0, numSnaps - 1, snapIndex)].name); }

double PianoRoll::snapRound (double b) const
{
    const double q = snapValue();
    return q > 0.0 ? std::round (b / q) * q : b;
}

double PianoRoll::snapFloor (double b) const
{
    const double q = snapValue();
    return q > 0.0 ? std::floor (b / q + 1.0e-6) * q : b;
}

void PianoRoll::showSnapMenu()
{
    PopupMenu m;
    m.addSectionHeader ("Ajuste a la rejilla");
    for (int i = 0; i < numSnaps; ++i)
        m.addItem (1 + i, String::fromUTF8 (snapOptions[i].name), true, i == snapIndex);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&snapBtn).withMinimumWidth (snapBtn.getWidth()),
                     [this] (int r)
                     {
                         if (r <= 0) return;
                         snapIndex = r - 1;
                         snapBtn.setLabel (snapName());
                         if (snapValue() > 0.0) lastLength = jmax (snapValue(), std::round (lastLength / snapValue()) * snapValue());
                         repaint();
                     });
}

double PianoRoll::totalBeats() const
{
    double end = 0.0;
    for (auto& n : notes) end = jmax (end, n.start + n.length);
    const double bar = (double) beatsPerBar;
    return jmax (bar * 8.0, std::ceil (end / bar) * bar + bar * 4.0);
}

void PianoRoll::previewPitch (int note, int vel)
{
    if (previewNote >= 0) kb.noteOff (1, previewNote, 0.0f);
    previewNote = note;
    kb.noteOn (1, note, jlimit (0.1f, 1.0f, (float) vel / 127.0f));
    previewOffAt = Time::getMillisecondCounterHiRes() + 260.0;
}

//==============================================================================
// Geometría
//==============================================================================
Rectangle<float> PianoRoll::gridArea() const
{
    const float w = (float) getWidth(), h = (float) getHeight();
    return { keysW, tbH + rulerH, jmax (10.0f, w - keysW - sbW), jmax (10.0f, h - tbH - rulerH - laneH - gapH - sbW) };
}
Rectangle<float> PianoRoll::keysArea() const  { const auto g = gridArea(); return { 0.0f, g.getY(), keysW, g.getHeight() }; }
Rectangle<float> PianoRoll::rulerArea() const { const auto g = gridArea(); return { g.getX(), tbH, g.getWidth(), rulerH }; }
Rectangle<float> PianoRoll::laneArea() const  { const auto g = gridArea(); return { g.getX(), g.getBottom() + gapH, g.getWidth(), laneH }; }
float PianoRoll::xOf (double beat) const { return gridArea().getX() + (float) ((beat - scrollBeat) * pxPerBeat); }
double PianoRoll::beatOf (float x) const { return scrollBeat + (double) (x - gridArea().getX()) / pxPerBeat; }
float PianoRoll::yOf (int note) const { return gridArea().getY() + (float) (topNote - note) * rowH; }
int PianoRoll::noteOfY (float y) const { return jlimit (0, 127, topNote - (int) std::floor ((y - gridArea().getY()) / rowH)); }

int PianoRoll::noteAt (Point<float> p, bool& nearEnd) const
{
    nearEnd = false;
    for (int i = (int) notes.size() - 1; i >= 0; --i)
    {
        const auto& n = notes[(size_t) i];
        const float x1 = xOf (n.start), x2 = xOf (n.start + n.length), y = yOf (n.note);
        if (p.y >= y && p.y < y + rowH && p.x >= x1 && p.x <= x2 + 2.0f)
        {
            nearEnd = p.x >= x2 - jmin (7.0f, (x2 - x1) * 0.35f);
            return i;
        }
    }
    return -1;
}

void PianoRoll::updateScrollBars()
{
    const auto g = gridArea();
    const double visBeats = g.getWidth() / pxPerBeat;
    const double total = jmax (totalBeats(), scrollBeat + visBeats);
    hbar.setRangeLimits (0.0, total, dontSendNotification);
    hbar.setCurrentRange (scrollBeat, visBeats, dontSendNotification);
    const double visRows = g.getHeight() / rowH;
    topNote = jlimit ((int) std::ceil (visRows) - 1, 127, topNote);
    vbar.setRangeLimits (0.0, 128.0, dontSendNotification);
    vbar.setCurrentRange (127.0 - topNote, visRows, dontSendNotification);
}

void PianoRoll::scrollBarMoved (ScrollBar* bar, double start)
{
    if (bar == &hbar) scrollBeat = jmax (0.0, start);
    else topNote = jlimit (0, 127, 127 - (int) std::round (start));
    repaint();
}

void PianoRoll::resized()
{
    const float w = (float) getWidth();
    int x = 10;
    const int y = 10, h = 42;
    auto place = [&] (Component& c, int width) { c.setBounds (x, y, width, h); x += width + 6; };
    place (drawBtn, 46); place (selectBtn, 46); place (sliceBtn, 46); place (eraseBtn, 46);
    x += 10;
    place (snapBtn, 170);
    place (quantBtn, 136);
    x += 10;
    place (undoBtn, 46); place (redoBtn, 46);
    x += 10;
    place (playBtn, 46);
    dragOut->setBounds ((int) w - 10 - 190, y, 190, h);

    const auto g = gridArea();
    hbar.setBounds ((int) g.getX(), getHeight() - (int) sbW, (int) g.getWidth(), (int) sbW);
    vbar.setBounds ((int) g.getRight(), (int) g.getY(), (int) sbW, (int) g.getHeight());
    updateScrollBars();
}

//==============================================================================
// Dibujo
//==============================================================================
void PianoRoll::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.fillAll (pal.body2);
    const auto grid = gridArea(), keys = keysArea(), ruler = rulerArea(), lane = laneArea();
    const auto ink = pal.ink;
    const double bar = (double) beatsPerBar;

    // Estado (arriba)
    {
        const float sx = (float) playBtn.getRight() + 16.0f;
        auto sr = Rectangle<float> (sx, 10.0f, (float) dragOut->getX() - sx - 12.0f, 42.0f);
        g.setColour (pal.dim);
        g.setFont (ctx.fonts.ui (14.0f, 1));
        g.drawText (status, sr, Justification::centredLeft, true);
    }

    // Rejilla
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (grid.toNearestInt());
        g.setColour (pal.card);
        g.fillRect (grid);

        const int firstRow = jmax (0, topNote - (int) (grid.getHeight() / rowH) - 1);
        for (int n = topNote; n >= firstRow; --n)
        {
            const float y = yOf (n);
            if (isBlack (n)) { g.setColour (ink.withAlpha (0.055f)); g.fillRect (grid.getX(), y, grid.getWidth(), rowH); }
            g.setColour (ink.withAlpha (n % 12 == 0 ? 0.14f : 0.045f));
            g.drawHorizontalLine (roundToInt (y + rowH), grid.getX(), grid.getRight());
        }

        // Compases alternados y líneas
        const double b0 = std::floor (beatOf (grid.getX()) / bar) * bar;
        const double b1 = beatOf (grid.getRight());
        for (double b = b0; b < b1; b += bar)
            if (((int) std::llround (b / bar)) % 2 == 1)
            {
                g.setColour (ink.withAlpha (0.025f));
                g.fillRect (Rectangle<float> (xOf (b), grid.getY(), (float) (bar * pxPerBeat), grid.getHeight()));
            }
        const double q = snapValue() > 0.0 ? snapValue() : 0.25;
        const bool showSteps = q * pxPerBeat >= 7.0;
        const double step = showSteps ? q : (pxPerBeat >= 7.0f ? 1.0 : bar);
        for (double b = std::floor (beatOf (grid.getX()) / step) * step; b < b1; b += step)
        {
            const bool isBar = std::abs (b / bar - std::round (b / bar)) < 1.0e-6;
            const bool isBeat = std::abs (b - std::round (b)) < 1.0e-6;
            g.setColour (ink.withAlpha (isBar ? 0.28f : (isBeat ? 0.12f : 0.05f)));
            g.drawVerticalLine (roundToInt (xOf (b)), grid.getY(), grid.getBottom());
        }

        // Notas (si hay elegidas, las demás se ven más apagadas, como en FL)
        bool anySelected = false;
        for (auto& n : notes) anySelected = anySelected || n.selected;
        for (auto& n : notes)
        {
            const float x1 = xOf (n.start), x2 = xOf (n.start + n.length);
            if (x2 < grid.getX() || x1 > grid.getRight()) continue;
            const float y = yOf (n.note);
            if (y + rowH < grid.getY() || y > grid.getBottom()) continue;
            auto r = Rectangle<float> (x1 + 0.5f, y + 1.0f, jmax (3.0f, x2 - x1 - 1.0f), jmax (3.0f, rowH - 2.0f));
            const float v = (float) n.velocity / 127.0f;
            auto col = pal.accent.withMultipliedSaturation (0.6f + 0.4f * v).withMultipliedBrightness (0.82f + 0.18f * v);
            if (anySelected && ! n.selected) col = col.withMultipliedSaturation (0.35f).withAlpha (0.7f);
            g.setColour (col);
            g.fillRoundedRectangle (r, jmin (3.0f, r.getHeight() * 0.35f));
            g.setColour (n.selected ? (pal.night ? Colours::white : pal.ink) : col.darker (0.45f));
            g.drawRoundedRectangle (r.reduced (0.5f), jmin (3.0f, r.getHeight() * 0.35f), n.selected ? 2.0f : 1.0f);
            if (r.getWidth() > 30.0f && rowH >= 9.5f)
            {
                g.setColour (pal.accentInk.withAlpha (0.9f));
                g.setFont (ctx.fonts.ui (jmin (13.0f, rowH * 0.72f), 1));
                g.drawText (String (pcNames[n.note % 12]) + String (n.note / 12 - 1), r.reduced (4.0f, 0.0f), Justification::centredLeft, false);
            }
        }

        // Cortar: línea de corte
        if (drag == slicing)
        {
            const float x = xOf (snapRound (beatOf (downPos.x)));
            g.setColour (Colour (0xffe5383b));
            g.drawLine (x, jmin (downPos.y, lastPos.y) - rowH, x, jmax (downPos.y, lastPos.y) + rowH, 2.0f);
        }
        if (drag == marquee)
        {
            g.setColour (pal.accent.withAlpha (0.12f));
            g.fillRect (marqueeRect);
            g.setColour (pal.accent);
            g.drawRect (marqueeRect, 1.2f);
        }

        // Cabezal
        const double headBeat = playing.load() ? playFromBeat + (Time::getMillisecondCounterHiRes() - playStartMs.load()) / 1000.0 * bpm / 60.0 : cursorBeat;
        g.setColour (playing.load() ? pal.accent : ink.withAlpha (0.4f));
        g.drawLine (xOf (headBeat), grid.getY(), xOf (headBeat), grid.getBottom(), playing.load() ? 2.0f : 1.0f);

        if (notes.empty())
        {
            g.setColour (ink.withAlpha (0.45f));
            g.setFont (ctx.fonts.ui (16.0f, 1));
            g.drawFittedText (U8 ("Clic con el lápiz para poner notas · arrastra para moverlas · la orilla derecha las alarga\n"
                                  "Clic derecho borra · Ctrl+Z deshace · Ctrl+rueda hace zoom"),
                              grid.reduced (40.0f).toNearestInt(), Justification::centred, 3);
        }
    }

    // Regla (compases)
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (ruler.toNearestInt());
        g.setColour (pal.body1);
        g.fillRect (ruler);
        g.setFont (ctx.fonts.ui (13.0f, 2));
        const double b0 = std::floor (beatOf (ruler.getX()) / bar) * bar;
        for (double b = b0; b < beatOf (ruler.getRight()); b += 1.0)
        {
            const float x = xOf (b);
            const bool isBar = std::abs (b / bar - std::round (b / bar)) < 1.0e-6;
            g.setColour (pal.text.withAlpha (isBar ? 0.6f : 0.2f));
            g.drawVerticalLine (roundToInt (x), ruler.getBottom() - (isBar ? 12.0f : 6.0f), ruler.getBottom());
            if (isBar)
                g.drawText (String ((int) std::llround (b / bar) + 1), Rectangle<float> (x + 4.0f, ruler.getY(), 40.0f, ruler.getHeight() - 4.0f), Justification::centredLeft, false);
        }
        const float cx = xOf (cursorBeat);
        Path tri;
        tri.addTriangle (cx - 6.0f, ruler.getY() + 2.0f, cx + 6.0f, ruler.getY() + 2.0f, cx, ruler.getY() + 11.0f);
        g.setColour (pal.accent);
        g.fillPath (tri);
    }

    // Teclado de lado
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (keys.toNearestInt());
        const int firstRow = jmax (0, topNote - (int) (keys.getHeight() / rowH) - 1);
        // Teclado de lado siempre claro (como en FL), para que se lean bien las notas
        const Colour whiteKey (0xffeeeeec), blackKey (0xff1c1c1f);
        g.setColour (whiteKey);
        g.fillRect (keys);
        for (int n = topNote; n >= firstRow; --n)
        {
            const float y = yOf (n);
            if (isBlack (n))
            {
                g.setColour (n == previewNote ? pal.accent : blackKey);
                g.fillRoundedRectangle (keys.getX(), y + 0.5f, keys.getWidth() * 0.62f, rowH - 1.0f, 2.5f);
            }
            else
            {
                if (n == previewNote) { g.setColour (pal.accent); g.fillRect (keys.getX(), y, keys.getWidth(), rowH); }
                const int pc = n % 12;
                if (pc == 4 || pc == 11) { g.setColour (Colours::black.withAlpha (0.18f)); g.drawHorizontalLine (roundToInt (y), keys.getX(), keys.getRight()); }
                if (pc == 0)
                {
                    g.setColour (Colours::black.withAlpha (0.3f));
                    g.drawHorizontalLine (roundToInt (y + rowH), keys.getX(), keys.getRight());
                    g.setColour (Colours::black.withAlpha (0.6f));
                    g.setFont (ctx.fonts.ui (jmin (12.0f, rowH * 0.8f), 2));
                    g.drawText ("C" + String (n / 12 - 1), Rectangle<float> (keys.getRight() - 34.0f, y, 30.0f, rowH), Justification::centredRight, false);
                }
            }
        }
        g.setColour (ink.withAlpha (0.25f));
        g.drawVerticalLine ((int) keys.getRight() - 1, keys.getY(), keys.getBottom());
    }

    // Carril de fuerza (velocity), como en FL
    {
        draw::recess (g, lane.withX (0.0f).withRight (lane.getRight()), pal, 8.0f, pal.night ? pal.plate.darker (0.25f) : pal.card);
        g.setColour (pal.dim);
        g.setFont (ctx.fonts.ui (12.0f, 2));
        g.drawFittedText (U8 ("Fuerza"), Rectangle<int> (4, (int) lane.getY() + 6, (int) keysW - 8, 36), Justification::topLeft, 2);
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (lane.toNearestInt());
        bool anySel = false;
        for (auto& n : notes) anySel = anySel || n.selected;
        const auto in = lane.reduced (0.0f, 8.0f);
        for (auto& n : notes)
        {
            const float x = xOf (n.start) + 2.0f;
            if (x < lane.getX() - 6.0f || x > lane.getRight() + 6.0f) continue;
            const float top = in.getBottom() - in.getHeight() * (float) n.velocity / 127.0f;
            const bool dim = anySel && ! n.selected;
            g.setColour (pal.accent.withAlpha (dim ? 0.3f : 0.95f));
            g.fillRect (x - 1.0f, top, 2.0f, in.getBottom() - top);
            g.fillEllipse (x - 4.5f, top - 4.5f, 9.0f, 9.0f);
        }
    }
}

//==============================================================================
// Mouse
//==============================================================================
void PianoRoll::mouseDown (const MouseEvent& e)
{
    grabKeyboardFocus();
    const auto p = e.position;
    downPos = lastPos = p;
    changed = false;
    preEdit = notes;
    drag = none;

    if (keysArea().contains (p)) { drag = keysPreview; dragNote = noteOfY (p.y); previewPitch (dragNote, 100); repaint(); return; }
    if (rulerArea().contains (p)) { cursorBeat = jmax (0.0, snapFloor (beatOf (p.x))); playFromBeat = cursorBeat; repaint(); return; }
    if (laneArea().contains (p)) { drag = velocity; mouseDrag (e); return; }
    if (! gridArea().contains (p)) return;

    if (e.mods.isMiddleButtonDown()) { drag = panning; return; }

    bool nearEnd = false;
    const int idx = noteAt (p, nearEnd);
    if (e.mods.isPopupMenu() || tool == erase)
    {
        drag = erasing;
        if (idx >= 0) { notes.erase (notes.begin() + idx); changed = true; repaint(); }
        return;
    }
    if (tool == slice) { drag = slicing; repaint(); return; }

    if (idx >= 0)
    {
        if (! notes[(size_t) idx].selected)
        {
            if (! e.mods.isShiftDown() && ! e.mods.isCommandDown()) for (auto& n : notes) n.selected = false;
            notes[(size_t) idx].selected = true;
        }
        else if (e.mods.isCommandDown()) { notes[(size_t) idx].selected = false; repaint(); return; }

        if (e.mods.isShiftDown() && ! nearEnd)
        {
            // Shift + arrastrar: copia las notas (como en FL)
            std::vector<Note> copies;
            for (auto& n : notes) if (n.selected) { auto c = n; n.selected = false; copies.push_back (c); }
            const size_t base = notes.size();
            for (auto& c : copies) notes.push_back (c);
            // la nota tomada es la copia de la que se hizo clic
            for (size_t i = base; i < notes.size(); ++i)
                if (notes[i].note == preEdit[(size_t) idx].note && std::abs (notes[i].start - preEdit[(size_t) idx].start) < 1.0e-9) { dragNote = (int) i; break; }
            changed = true;
        }
        else dragNote = idx;

        drag = nearEnd ? resizing : moving;
        dragOrig = notes;
        lastLength = notes[(size_t) dragNote].length;
        lastVelocity = notes[(size_t) dragNote].velocity;
        cursorBeat = snapFloor (notes[(size_t) dragNote].start);
        previewPitch (notes[(size_t) dragNote].note, notes[(size_t) dragNote].velocity);
        repaint();
        return;
    }

    if (tool == draw)
    {
        for (auto& n : notes) n.selected = false;
        Note n;
        n.note = noteOfY (p.y);
        n.start = jmax (0.0, snapFloor (beatOf (p.x)));
        n.length = lastLength;
        n.velocity = lastVelocity;
        n.selected = true;
        notes.push_back (n);
        dragNote = (int) notes.size() - 1;
        dragOrig = notes;
        drag = moving;
        changed = true;
        cursorBeat = n.start;
        previewPitch (n.note, n.velocity);
        repaint();
        return;
    }

    // Seleccionar: cuadro
    if (! e.mods.isShiftDown()) for (auto& n : notes) n.selected = false;
    dragOrig = notes;
    drag = marquee;
    marqueeRect = {};
    cursorBeat = jmax (0.0, snapFloor (beatOf (p.x)));
    repaint();
}

void PianoRoll::mouseDrag (const MouseEvent& e)
{
    const auto p = e.position;
    switch (drag)
    {
        case moving:
        {
            const auto& a = dragOrig[(size_t) dragNote];
            const double dx = beatOf (p.x) - beatOf (downPos.x);
            const int dy = noteOfY (p.y) - noteOfY (downPos.y);
            double newStart = jmax (0.0, snapRound (a.start + dx));
            double dBeat = newStart - a.start;
            double minStart = 1.0e9;
            for (auto& n : dragOrig) if (n.selected) minStart = jmin (minStart, n.start);
            dBeat = jmax (dBeat, -minStart);
            int lo = 127, hi = 0;
            for (auto& n : dragOrig) if (n.selected) { lo = jmin (lo, n.note); hi = jmax (hi, n.note); }
            const int dyc = jlimit (-lo, 127 - hi, dy);
            const int before = notes[(size_t) dragNote].note;
            for (size_t i = 0; i < notes.size(); ++i)
                if (dragOrig[i].selected)
                {
                    notes[i].start = dragOrig[i].start + dBeat;
                    notes[i].note = dragOrig[i].note + dyc;
                }
            if (notes[(size_t) dragNote].note != before) previewPitch (notes[(size_t) dragNote].note, notes[(size_t) dragNote].velocity);
            if (std::abs (dBeat) > 1.0e-9 || dyc != 0) changed = true;
            cursorBeat = notes[(size_t) dragNote].start;
            break;
        }
        case resizing:
        {
            const auto& a = dragOrig[(size_t) dragNote];
            const double dx = beatOf (p.x) - beatOf (downPos.x);
            const double newEnd = snapRound (a.start + a.length + dx);
            const double dLen = newEnd - (a.start + a.length);
            const double minLen = snapValue() > 0.0 ? snapValue() : 1.0 / 64.0;
            for (size_t i = 0; i < notes.size(); ++i)
                if (dragOrig[i].selected) notes[i].length = jmax (minLen, dragOrig[i].length + dLen);
            lastLength = notes[(size_t) dragNote].length;
            changed = std::abs (dLen) > 1.0e-9 || changed;
            break;
        }
        case marquee:
        {
            marqueeRect = Rectangle<float> (downPos, p);
            for (size_t i = 0; i < notes.size(); ++i)
            {
                const auto& n = notes[i];
                auto r = Rectangle<float> (xOf (n.start), yOf (n.note), (float) (n.length * pxPerBeat), rowH);
                notes[i].selected = dragOrig[i].selected || r.intersects (marqueeRect);
            }
            break;
        }
        case erasing:
        {
            bool nearEnd = false;
            const int idx = noteAt (p, nearEnd);
            if (idx >= 0) { notes.erase (notes.begin() + idx); changed = true; }
            break;
        }
        case velocity:
        {
            const auto in = laneArea().reduced (0.0f, 8.0f);
            bool anySel = false;
            for (auto& n : notes) anySel = anySel || n.selected;
            const float x1 = jmin (lastPos.x, p.x) - 4.0f, x2 = jmax (lastPos.x, p.x) + 4.0f;
            for (auto& n : notes)
            {
                if (anySel && ! n.selected) continue;
                const float x = xOf (n.start) + 2.0f;
                if (x < x1 || x > x2) continue;
                const float t = std::abs (p.x - lastPos.x) < 0.5f ? 1.0f : jlimit (0.0f, 1.0f, (x - lastPos.x) / (p.x - lastPos.x));
                const float y = lastPos.y + (p.y - lastPos.y) * t;
                n.velocity = jlimit (1, 127, roundToInt (127.0f * (in.getBottom() - y) / jmax (1.0f, in.getHeight())));
                lastVelocity = n.velocity;
                changed = true;
            }
            break;
        }
        case panning:
        {
            scrollBeat = jmax (0.0, scrollBeat - (double) (p.x - lastPos.x) / pxPerBeat);
            const int rows = (int) std::round ((p.y - lastPos.y) / rowH);
            if (rows != 0) topNote = jlimit (0, 127, topNote + rows);
            else { lastPos.x = p.x; repaint(); updateScrollBars(); return; }
            updateScrollBars();
            break;
        }
        case keysPreview:
        {
            const int n = noteOfY (p.y);
            if (n != previewNote && keysArea().contains (p)) previewPitch (n, 100);
            break;
        }
        default: break;
    }
    lastPos = p;
    repaint();
}

void PianoRoll::mouseUp (const MouseEvent& e)
{
    if (drag == slicing)
    {
        const double t = snapRound (beatOf (downPos.x));
        const int nA = noteOfY (downPos.y), nB = noteOfY (e.position.y);
        const int lo = jmin (nA, nB), hi = jmax (nA, nB);
        std::vector<Note> added;
        for (auto& n : notes)
            if (n.note >= lo && n.note <= hi && t > n.start + 1.0e-6 && t < n.start + n.length - 1.0e-6)
            {
                Note right = n;
                right.start = t;
                right.length = n.start + n.length - t;
                n.length = t - n.start;
                added.push_back (right);
                changed = true;
            }
        for (auto& a : added) notes.push_back (a);
        if (changed) status = String ((int) added.size()) + (added.size() == 1 ? " nota cortada" : " notas cortadas");
    }
    if (changed)
    {
        undoStack.push_back (preEdit);
        if (undoStack.size() > 200) undoStack.erase (undoStack.begin());
        redoStack.clear();
        std::sort (notes.begin(), notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });
        save();
        updateScrollBars();
    }
    drag = none;
    dragNote = -1;
    changed = false;
    marqueeRect = {};
    repaint();
}

void PianoRoll::mouseMove (const MouseEvent& e)
{
    const auto p = e.position;
    MouseCursor c = MouseCursor::NormalCursor;
    if (gridArea().contains (p))
    {
        bool nearEnd = false;
        const int idx = noteAt (p, nearEnd);
        if (tool == erase) c = MouseCursor::CrosshairCursor;
        else if (tool == slice) c = MouseCursor::IBeamCursor;
        else if (idx >= 0) c = nearEnd ? MouseCursor::LeftRightResizeCursor : MouseCursor::DraggingHandCursor;
        else if (tool == draw) c = MouseCursor::CrosshairCursor;
    }
    else if (laneArea().contains (p)) c = MouseCursor::UpDownResizeCursor;
    else if (keysArea().contains (p) || rulerArea().contains (p)) c = MouseCursor::PointingHandCursor;
    setMouseCursor (c);
}

void PianoRoll::mouseDoubleClick (const MouseEvent& e)
{
    // Doble clic en una nota con la flecha: la borra (rápido para limpiar)
    if (tool != select || ! gridArea().contains (e.position)) return;
    bool nearEnd = false;
    const int idx = noteAt (e.position, nearEnd);
    if (idx < 0) return;
    pushUndo();
    notes.erase (notes.begin() + idx);
    save();
    repaint();
}

void PianoRoll::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const float d = (std::abs (w.deltaY) > std::abs (w.deltaX) ? w.deltaY : w.deltaX) * (w.isReversed ? -1.0f : 1.0f);
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        // Zoom horizontal en donde está el mouse
        const double anchor = beatOf (e.position.x);
        pxPerBeat = jlimit (8.0f, 600.0f, pxPerBeat * (d > 0 ? 1.15f : 1.0f / 1.15f));
        scrollBeat = jmax (0.0, anchor - (double) (e.position.x - gridArea().getX()) / pxPerBeat);
    }
    else if (e.mods.isAltDown())
    {
        rowH = jlimit (7.0f, 40.0f, rowH * (d > 0 ? 1.12f : 1.0f / 1.12f));
    }
    else if (e.mods.isShiftDown() || std::abs (w.deltaX) > std::abs (w.deltaY))
    {
        scrollBeat = jmax (0.0, scrollBeat - (double) d * 4.0);
    }
    else
    {
        topNote = jlimit (0, 127, topNote + (d > 0 ? 3 : -3));
    }
    updateScrollBars();
    repaint();
}

bool PianoRoll::keyPressed (const KeyPress& key)
{
    const auto mods = key.getModifiers();
    const bool cmd = mods.isCommandDown();
    const int code = key.getKeyCode();
    auto is = [code] (char c) { return code == c || code == (c - 'A' + 'a'); };

    if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey) { deleteSelected(); return true; }
    if (key == KeyPress::spaceKey) { togglePlay(); return true; }
    if (key == KeyPress::upKey)    { transpose (mods.isShiftDown() ? 12 : 1); return true; }
    if (key == KeyPress::downKey)  { transpose (mods.isShiftDown() ? -12 : -1); return true; }
    if (key == KeyPress::leftKey)  { nudge (-1); return true; }
    if (key == KeyPress::rightKey) { nudge (1); return true; }
    if (cmd)
    {
        if (is ('A')) { selectAll (true); return true; }
        if (is ('D')) { selectAll (false); return true; }
        if (is ('C')) { copySelected (false); return true; }
        if (is ('X')) { copySelected (true); return true; }
        if (is ('V')) { paste(); return true; }
        if (is ('B')) { duplicate(); return true; }
        if (is ('Q')) { quantize(); return true; }
        if (is ('Z')) { if (mods.isShiftDown()) redo(); else undo(); return true; }
        if (is ('Y')) { redo(); return true; }
        return false;
    }
    if (is ('P')) { setTool (draw); return true; }
    if (is ('E')) { setTool (select); return true; }
    if (is ('C')) { setTool (slice); return true; }
    if (is ('D')) { setTool (erase); return true; }
    return false;
}

//==============================================================================
// Reproducir
//==============================================================================
void PianoRoll::togglePlay()
{
    if (playing.load()) { stopPlay(); return; }
    const ScopedLock sl (playLock);
    playEvents.clear();
    const double spb = 60.0 / jmax (1.0, bpm);
    for (auto& n : notes)
    {
        if (n.start + 1.0e-6 < playFromBeat) continue;
        playEvents.push_back ({ (n.start - playFromBeat) * spb, n.note, (float) n.velocity / 127.0f, true });
        playEvents.push_back ({ (n.start + n.length - playFromBeat) * spb - 0.002, n.note, 0.0f, false });
    }
    std::sort (playEvents.begin(), playEvents.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && ! a.on && b.on); });
    playNext = 0;
    held.fill (0);
    playStartMs.store (Time::getMillisecondCounterHiRes() + 40.0);
    playing.store (true);
    playBtn.setIcon ("stop");
    playBtn.setActive (true);
    HighResolutionTimer::startTimer (2);
}

void PianoRoll::stopPlay()
{
    HighResolutionTimer::stopTimer();
    const ScopedLock sl (playLock);
    for (int n = 0; n < 128; ++n)
        while (held[(size_t) n] > 0) { kb.noteOff (1, n, 0.0f); --held[(size_t) n]; }
    playing.store (false);
    playBtn.setIcon ("play");
    playBtn.setActive (false);
}

void PianoRoll::hiResTimerCallback()
{
    const ScopedLock sl (playLock);
    const double t = (Time::getMillisecondCounterHiRes() - playStartMs.load()) / 1000.0;
    while (playNext < playEvents.size() && playEvents[playNext].t <= t)
    {
        const auto& e = playEvents[playNext++];
        if (e.on) { kb.noteOn (1, e.note, jmax (0.05f, e.vel)); ++held[(size_t) e.note]; }
        else if (held[(size_t) e.note] > 0) { kb.noteOff (1, e.note, 0.0f); --held[(size_t) e.note]; }
    }
    if (playNext >= playEvents.size())
    {
        playing.store (false);
        HighResolutionTimer::stopTimer();
    }
}

void PianoRoll::timerCallback()
{
    if (previewNote >= 0 && Time::getMillisecondCounterHiRes() > previewOffAt && drag != keysPreview)
    {
        kb.noteOff (1, previewNote, 0.0f);
        previewNote = -1;
        repaint (keysArea().toNearestInt());
    }
    if (playing.load())
    {
        // seguir el cabezal si se sale de la vista
        const double head = playFromBeat + (Time::getMillisecondCounterHiRes() - playStartMs.load()) / 1000.0 * bpm / 60.0;
        const auto g = gridArea();
        if (xOf (head) > g.getRight() - 20.0f) { scrollBeat = head - 1.0; updateScrollBars(); }
        repaint();
    }
    else if (playBtn.isActive())
    {
        playBtn.setIcon ("play");
        playBtn.setActive (false);
        repaint();
    }
}

//==============================================================================
PianoRollWindow::PianoRollWindow (UiContext& c, MidiKeyboardState& kb, LookAndFeel& lf)
    : DocumentWindow ("Piano roll", c.vis.pal.body2, DocumentWindow::allButtons)
{
    setLookAndFeel (&lf);
    setUsingNativeTitleBar (true);
    rollComp = new PianoRoll (c, kb);
    rollComp->setSize (1180, 700);
    setContentOwned (rollComp, true);
    setResizable (true, false);
    setResizeLimits (760, 440, 6000, 4000);
    centreWithSize (getWidth(), getHeight());
}

PianoRollWindow::~PianoRollWindow()
{
    clearContentComponent();
    setLookAndFeel (nullptr);
}

void PianoRollWindow::open (const File& f)
{
    rollComp->loadFile (f);
    setName (U8 ("Piano roll · ") + f.getFileNameWithoutExtension());
    setVisible (true);
    toFront (true);
    rollComp->grabKeyboardFocus();
}
