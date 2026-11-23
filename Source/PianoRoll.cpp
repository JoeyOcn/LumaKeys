#include "PianoRoll.h"
#include "Recorder.h"

using namespace juce;

std::vector<PianoRoll::Note> PianoRoll::clipboard;

static constexpr float tbH = 60.0f, rulerH = 28.0f, keysW = 74.0f, laneH = 104.0f, sbW = 12.0f, gapH = 6.0f;
static const char* pcNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static bool isBlack (int n) { const int pc = n % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }

// Rejilla recta: 1/32, 1/16, 1/8, 1/4, 1/2, 1 compás (el último depende del compás)
static const double gridBeats[] = { 0.125, 0.25, 0.5, 1.0, 2.0, -1.0 };
static const char* gridNames[] = { "1/32", "1/16", "1/8", "1/4", "1/2", "Compás" };

// Como en Ableton: copiar con Alt (Mac) / Ctrl (Windows); fuerza y sin rejilla con Cmd (Mac) / Alt (Windows)
static bool copyMod (const ModifierKeys& m)
{
   #if JUCE_MAC
    return m.isAltDown();
   #else
    return m.isCtrlDown();
   #endif
}
static bool fineMod (const ModifierKeys& m)
{
   #if JUCE_MAC
    return m.isCommandDown();
   #else
    return m.isAltDown();
   #endif
}
static String cmdName()
{
   #if JUCE_MAC
    return "Cmd";
   #else
    return "Ctrl";
   #endif
}

//==============================================================================
class PianoRoll::DragOut : public Component, public SettableTooltipClient
{
public:
    // MIDI: siempre se puede arrastrar. Audio: clic lo pasa a audio; ya listo, se arrastra el WAV
    DragOut (PianoRoll& o, bool isAudio) : owner (o), audio (isAudio)
    {
        setMouseCursor (MouseCursor::DraggingHandCursor);
        setTooltip (audio ? U8 ("Clic: pasa este MIDI a audio (WAV) con el sonido elegido. Ya listo, arrástralo a Ableton o FL Studio como sample")
                          : U8 ("Arrastra este MIDI a la vista Arrangement de Ableton o a la Playlist de FL Studio"));
    }
    File target() const
    {
        if (! owner.file.existsAsFile()) return {};
        return audio ? MidiRecorder::freshAudioFor (owner.file) : owner.file;
    }
    float progress() const { return audio && owner.audioProgress ? owner.audioProgress() : -1.0f; }
    void paint (Graphics& g) override
    {
        const auto& pal = owner.ctx.vis.pal;
        const bool has = target().existsAsFile();
        const float prog = progress();
        auto b = getLocalBounds().toFloat().reduced (1.0f);
        draw::button (g, b, pal, b.getHeight() * 0.3f, down ? 1.0f : 0.0f, has, 0.0f);
        auto c = b.withTrimmedBottom (2.5f).translated (0.0f, 2.5f * (down ? 1.0f : 0.0f));
        const auto col = has ? pal.accentInk : pal.btnText;
        if (prog >= 0.0f)
        {
            auto bar = c.withSizeKeepingCentre (c.getWidth() - 20.0f, 5.0f).translated (0.0f, c.getHeight() * 0.3f);
            g.setColour (col.withAlpha (0.2f));
            g.fillRoundedRectangle (bar, 2.5f);
            g.setColour (pal.accent);
            g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * jlimit (0.02f, 1.0f, prog)), 2.5f);
            c = c.withTrimmedBottom (c.getHeight() * 0.3f);
        }
        auto ir = c.removeFromLeft (c.getHeight());
        draw::icon (g, audio ? "wave" : "export", ir.reduced (ir.getWidth() * 0.26f), col, 1.6f);
        const String t = ! audio ? U8 ("Arrastra MIDI")
                       : prog >= 0.0f ? String (roundToInt (prog * 100.0f)) + " %"
                       : has ? U8 ("Arrastra audio") : U8 ("A audio");
        g.setColour (col);
        g.setFont (draw::fitUi (owner.ctx.fonts, t, 14.5f, c.getWidth() - 8.0f, 2));
        g.drawText (t, c, Justification::centredLeft, false);
    }
    void mouseDown (const MouseEvent&) override { down = true; started = false; repaint(); }
    void mouseUp (const MouseEvent& e) override
    {
        down = false;
        repaint();
        if (started || ! getLocalBounds().contains (e.getPosition())) return;
        // Clic en el chip de audio: pasar a audio (o cancelar si ya se está pasando)
        if (audio && owner.onConvertToAudio && owner.file.existsAsFile() && (progress() >= 0.0f || ! target().existsAsFile()))
            owner.onConvertToAudio (owner.file);
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (started || e.getDistanceFromDragStart() < 4) return;
        const auto f = target();
        if (! f.existsAsFile()) return;
        started = true;
        SafePointer<DragOut> safe (this);
        DragAndDropContainer::performExternalDragDropOfFiles ({ MidiRecorder::dragCopy (f).getFullPathName() }, false, this,
            [safe] { if (auto* d = safe.getComponent()) { d->down = false; d->repaint(); } });
    }
private:
    PianoRoll& owner;
    const bool audio;
    bool down = false, started = false;
};

//==============================================================================
PianoRoll::PianoRoll (UiContext& c, MidiKeyboardState& k)
    : ctx (c), kb (k),
      drawBtn (c, "pencil", U8 ("Lápiz (B): clic o arrastra para poner notas; clic en una nota la quita")),
      gridBtn (c, "magnet", U8 ("Rejilla (") + cmdName() + U8 ("+1 más fina, +2 más ancha, +3 tresillos, +4 sin ajuste)")),
      quantBtn (c, "quant", U8 ("Cuantizar (") + cmdName() + "+U)"),
      splitBtn (c, "knife", U8 ("Dividir en el marcador (") + cmdName() + "+E)"),
      dupBtn (c, "dup", U8 ("Duplicar (") + cmdName() + "+D)"),
      undoBtn (c, "undo", U8 ("Deshacer (") + cmdName() + "+Z)"),
      redoBtn (c, "redo", U8 ("Rehacer (") + cmdName() + "+Shift+Z)"),
      playBtn (c, "play", U8 ("Reproducir / detener (barra espaciadora)")),
      metroBtn (c, "metro", U8 ("Metrónomo al reproducir (para guiarte con el tiempo)"))
{
    setOpaque (true);
    setWantsKeyboardFocus (true);
    dragOut = std::make_unique<DragOut> (*this, false);
    audioOut = std::make_unique<DragOut> (*this, true);
    for (Component* comp : std::initializer_list<Component*> { &drawBtn, &gridBtn, &quantBtn, &splitBtn, &dupBtn, &undoBtn, &redoBtn, &playBtn, &metroBtn, dragOut.get(), &hbar, &vbar })
        addAndMakeVisible (comp);
    addChildComponent (audioOut.get());   // pasar a audio ya no se ofrece
    for (auto* b : { &drawBtn, &gridBtn, &quantBtn, &splitBtn, &dupBtn, &undoBtn, &redoBtn, &playBtn, &metroBtn })
        b->setWantsKeyboardFocus (false);

    drawBtn.onClick  = [this] { setDraw (! drawMode); };
    gridBtn.onClick  = [this] { showSnapMenu(); };
    quantBtn.onClick = [this] { quantize(); };
    splitBtn.onClick = [this] { splitAt (insertBeat, anySelected()); };
    dupBtn.onClick   = [this] { duplicate(); };
    undoBtn.onClick  = [this] { undo(); };
    redoBtn.onClick  = [this] { redo(); };
    playBtn.onClick  = [this] { togglePlay(); };
    metroBtn.setActive (metroOn);
    metroBtn.onClick = [this] { metroOn = ! metroOn; metroBtn.setActive (metroOn); };
    gridBtn.setLabel (gridName());

    hbar.addListener (this);
    vbar.addListener (this);
    hbar.setAutoHide (false);
    vbar.setAutoHide (false);
    smallFont = ctx.fonts.ui (12.0f, 2);
    rulerFont = ctx.fonts.ui (13.0f, 2);
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
    keyRoot = -1;
    keyScale = 0;

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
                else if (m.isTextMetaEvent() && m.getMetaEventType() == 1 && m.getTextFromTextMetaEvent().startsWith ("MidiCat key="))
                {
                    const auto parts = StringArray::fromTokens (m.getTextFromTextMetaEvent().fromFirstOccurrenceOf ("=", false, false), ",", "");
                    if (parts.size() == 2) { keyRoot = parts[0].getIntValue(); keyScale = parts[1].getIntValue(); }
                }
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

    // Vista: que se vean todas las notas
    int hi = 0, lo = 127;
    for (auto& n : notes) { hi = jmax (hi, n.note); lo = jmin (lo, n.note); }
    if (notes.empty()) { hi = 79; lo = 48; }
    const auto g = gridArea();
    rowH = jlimit (9.0f, 22.0f, jmax (200.0f, g.getHeight()) / (float) (hi - lo + 7));
    topRow = jlimit (24.0, 127.0, (double) hi + 3.0);
    scrollBeat = 0.0;
    pxPerBeat = jlimit (14.0f, 140.0f, jmax (300.0f, g.getWidth()) / (float) jmax (8.0, totalBeats() - (double) beatsPerBar));
    insertBeat = 0.0;
    noteFont = ctx.fonts.ui (jmin (13.0f, rowH * 0.72f), 1);
    status = notes.empty() ? U8 ("Vacío: doble clic o lápiz (B) para poner notas") : String ((int) notes.size()) + " notas";
    clampView();
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
    if (keyRoot >= 0) MidiRecorder::addKeyInfo (seq, keyRoot, keyScale);   // la tonalidad se conserva
    else seq.addEvent (MidiMessage::textMetaEvent (3, "MidiCat"), 0.0);
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
}

void PianoRoll::commit()
{
    undoStack.push_back (preEdit);
    if (undoStack.size() > 200) undoStack.erase (undoStack.begin());
    redoStack.clear();
    std::sort (notes.begin(), notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });
    save();
    updateScrollBars();
    repaint();
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
bool PianoRoll::anySelected() const
{
    for (auto& n : notes) if (n.selected) return true;
    return false;
}

void PianoRoll::deleteSelected()
{
    if (! anySelected()) return;
    preEdit = notes;
    notes.erase (std::remove_if (notes.begin(), notes.end(), [] (const Note& n) { return n.selected; }), notes.end());
    commit();
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
    else { status = String ((int) clipboard.size()) + " notas copiadas"; repaint(); }
}

void PianoRoll::paste()
{
    if (clipboard.empty()) return;
    preEdit = notes;
    for (auto& n : notes) n.selected = false;
    double end = insertBeat;
    for (auto n : clipboard)
    {
        n.start += insertBeat;
        n.selected = true;
        end = jmax (end, n.start + n.length);
        notes.push_back (n);
    }
    insertBeat = end;
    commit();
}

void PianoRoll::duplicate()
{
    // Como Cmd+D en Ableton: copia lo elegido justo después
    double first = 1.0e9, last = 0.0;
    std::vector<Note> sel;
    for (auto& n : notes) if (n.selected) { sel.push_back (n); first = jmin (first, n.start); last = jmax (last, n.start + n.length); }
    if (sel.empty()) return;
    preEdit = notes;
    const double q = gridOrDefault();
    double span = std::ceil ((last - first) / q - 1.0e-6) * q;
    const double bar = (double) beatsPerBar;
    if (span >= bar * 0.75) span = std::ceil (span / bar - 1.0e-6) * bar;
    for (auto& n : notes) n.selected = false;
    for (auto n : sel) { n.start += span; n.selected = true; notes.push_back (n); }
    insertBeat = first + span * 2.0;
    commit();
}

void PianoRoll::quantize()
{
    // Como Cmd+U en Ableton: acomoda el inicio de las notas a la rejilla
    const double q = gridOrDefault();
    const bool sel = anySelected();
    preEdit = notes;
    for (auto& n : notes)
        if (! sel || n.selected)
            n.start = jmax (0.0, std::round (n.start / q) * q);
    commit();
    status = U8 ("Cuantizado a ") + gridName();
}

void PianoRoll::splitAt (double beat, bool onlySelected)
{
    preEdit = notes;
    std::vector<Note> added;
    for (auto& n : notes)
        if ((! onlySelected || n.selected) && beat > n.start + 1.0e-6 && beat < n.start + n.length - 1.0e-6)
        {
            Note right = n;
            right.start = beat;
            right.length = n.start + n.length - beat;
            n.length = beat - n.start;
            added.push_back (right);
        }
    if (added.empty()) { status = U8 ("Pon el marcador (clic) donde quieras dividir"); repaint(); return; }
    for (auto& a : added) notes.push_back (a);
    commit();
    status = String ((int) added.size()) + (added.size() == 1 ? " nota dividida" : " notas divididas");
}

void PianoRoll::transpose (int semis)
{
    if (! anySelected()) return;
    preEdit = notes;
    for (auto& n : notes) if (n.selected) n.note = jlimit (0, 127, n.note + semis);
    for (auto& n : notes) if (n.selected) { previewPitch (n.note, n.velocity); break; }
    commit();
}

void PianoRoll::nudge (int steps)
{
    if (! anySelected()) return;
    preEdit = notes;
    const double q = gridOrDefault();
    for (auto& n : notes) if (n.selected) n.start = jmax (0.0, n.start + q * steps);
    commit();
}

void PianoRoll::stretch (int steps)
{
    if (! anySelected()) return;
    preEdit = notes;
    const double q = gridOrDefault();
    for (auto& n : notes) if (n.selected) n.length = jmax (q, n.length + q * steps);
    commit();
}

void PianoRoll::setDraw (bool on)
{
    drawMode = on;
    drawBtn.setActive (on);
    status = on ? U8 ("Lápiz: clic o arrastra para poner notas (B para volver a la flecha)")
                : U8 ("Flecha: doble clic pone o quita notas, arrastra para elegir varias");
    repaint (0, 0, getWidth(), (int) tbH);
}

void PianoRoll::addNote (int pitch, double start)
{
    Note n;
    n.note = jlimit (0, 127, pitch);
    n.start = jmax (0.0, start);
    n.length = gridOrDefault();
    n.velocity = lastVelocity;
    n.selected = true;
    notes.push_back (n);
    previewPitch (n.note, n.velocity);
}

double PianoRoll::gridOrDefault() const
{
    double v = gridBeats[jlimit (0, 5, gridIndex)];
    if (v < 0.0) v = (double) beatsPerBar;
    if (triplets) v *= 2.0 / 3.0;
    return v;
}

double PianoRoll::grid() const { return snapOn ? gridOrDefault() : 0.0; }

String PianoRoll::gridName() const
{
    String n = String::fromUTF8 (gridNames[jlimit (0, 5, gridIndex)]);
    if (triplets) n << " tresillo";
    if (! snapOn) n = U8 ("Sin ajuste");
    return n;
}

double PianoRoll::snapRound (double b) const
{
    const double q = grid();
    return q > 0.0 ? std::round (b / q) * q : b;
}

double PianoRoll::snapFloor (double b) const
{
    const double q = grid();
    return q > 0.0 ? std::floor (b / q + 1.0e-6) * q : b;
}

void PianoRoll::showSnapMenu()
{
    PopupMenu m;
    m.addSectionHeader ("Rejilla");
    for (int i = 0; i < 6; ++i)
        m.addItem (1 + i, String::fromUTF8 (gridNames[i]), true, snapOn && i == gridIndex);
    m.addSeparator();
    m.addItem (20, "Tresillos   (" + cmdName() + "+3)", true, triplets);
    m.addItem (21, U8 ("Ajustar a la rejilla   (") + cmdName() + "+4)", true, snapOn);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&gridBtn).withMinimumWidth (gridBtn.getWidth()),
                     [this] (int r)
                     {
                         if (r >= 1 && r <= 6) { gridIndex = r - 1; snapOn = true; }
                         else if (r == 20) triplets = ! triplets;
                         else if (r == 21) snapOn = ! snapOn;
                         else return;
                         gridBtn.setLabel (gridName());
                         repaint();
                     });
}

void PianoRoll::showContextMenu (int idx, double beat)
{
    PopupMenu m;
    const auto c = cmdName();
    if (idx >= 0)
    {
        m.addItem (1, "Eliminar   (Supr)");
        m.addItem (2, "Duplicar   (" + c + "+D)");
        m.addItem (3, U8 ("Dividir aquí   (") + c + "+E)");
        m.addItem (4, "Cuantizar   (" + c + "+U)");
        m.addItem (5, "Copiar   (" + c + "+C)");
    }
    else
    {
        m.addItem (6, U8 ("Poner nota aquí   (doble clic)"));
        m.addItem (7, "Pegar   (" + c + "+V)", ! clipboard.empty());
    }
    m.addSeparator();
    m.addItem (8, "Seleccionar todo   (" + c + "+A)");
    const int pitch = noteOfY (lastPos.y);
    m.showMenuAsync (PopupMenu::Options().withMousePosition(), [this, beat, pitch] (int r)
    {
        switch (r)
        {
            case 1: deleteSelected(); break;
            case 2: duplicate(); break;
            case 3: splitAt (snapRound (beat), true); break;
            case 4: quantize(); break;
            case 5: copySelected (false); break;
            case 6: preEdit = notes; for (auto& n : notes) n.selected = false; addNote (pitch, snapFloor (beat)); commit(); break;
            case 7: insertBeat = snapFloor (beat); paste(); break;
            case 8: selectAll (true); break;
            default: break;
        }
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
    previewOffAt = Time::getMillisecondCounterHiRes() + 240.0;
}

//==============================================================================
// Geometría y vista
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
float PianoRoll::yOf (int note) const { return gridArea().getY() + (float) ((topRow - note) * rowH); }
int PianoRoll::noteOfY (float y) const { return jlimit (0, 127, (int) std::ceil (topRow - (double) (y - gridArea().getY()) / rowH - 1.0e-9)); }

int PianoRoll::noteAt (Point<float> p, int& edge) const
{
    edge = 0;
    for (int i = (int) notes.size() - 1; i >= 0; --i)
    {
        const auto& n = notes[(size_t) i];
        const float x1 = xOf (n.start), x2 = xOf (n.start + n.length), y = yOf (n.note);
        if (p.y >= y && p.y < y + rowH && p.x >= x1 - 1.0f && p.x <= x2 + 2.0f)
        {
            const float w = x2 - x1;
            const float e = jmin (7.0f, w * 0.3f);
            if (p.x >= x2 - e) edge = 1;
            else if (w > 16.0f && p.x <= x1 + e) edge = -1;
            return i;
        }
    }
    return -1;
}

void PianoRoll::clampView()
{
    const auto g = gridArea();
    rowH = jlimit (6.0f, 40.0f, rowH);
    pxPerBeat = jlimit (6.0f, 900.0f, pxPerBeat);
    const double visRows = g.getHeight() / rowH;
    topRow = jlimit (jmin (127.0, visRows - 1.0), 127.0, topRow);
    const double visBeats = g.getWidth() / pxPerBeat;
    scrollBeat = jlimit (0.0, jmax (0.0, totalBeats() + 64.0 - visBeats), scrollBeat);
    noteFont = ctx.fonts.ui (jmin (13.0f, rowH * 0.72f), 1);
}

void PianoRoll::updateScrollBars()
{
    const auto g = gridArea();
    const double visBeats = g.getWidth() / pxPerBeat;
    const double total = jmax (totalBeats(), scrollBeat + visBeats);
    hbar.setRangeLimits (0.0, total, dontSendNotification);
    hbar.setCurrentRange (scrollBeat, visBeats, dontSendNotification);
    const double visRows = g.getHeight() / rowH;
    vbar.setRangeLimits (0.0, 128.0, dontSendNotification);
    vbar.setCurrentRange (127.0 - topRow, visRows, dontSendNotification);
}

void PianoRoll::scrollBarMoved (ScrollBar* bar, double start)
{
    if (bar == &hbar) scrollBeat = jmax (0.0, start);
    else topRow = 127.0 - start;
    clampView();
    repaint();
}

void PianoRoll::resized()
{
    const float w = (float) getWidth();
    int x = 10;
    const int y = 10, h = 42;
    auto place = [&] (Component& c, int width) { c.setBounds (x, y, width, h); x += width + 6; };
    place (drawBtn, 46);
    x += 8;
    place (gridBtn, 150);
    place (quantBtn, 46); place (splitBtn, 46); place (dupBtn, 46);
    x += 8;
    place (undoBtn, 46); place (redoBtn, 46);
    x += 8;
    place (playBtn, 46);
    place (metroBtn, 46);
    dragOut->setBounds ((int) w - 10 - 170, y, 170, h);
    audioOut->setBounds (dragOut->getX(), y, 0, h);

    const auto g = gridArea();
    hbar.setBounds ((int) g.getX(), getHeight() - (int) sbW, (int) g.getWidth(), (int) sbW);
    vbar.setBounds ((int) g.getRight(), (int) g.getY(), (int) sbW, (int) g.getHeight());
    clampView();
    updateScrollBars();
}

//==============================================================================
// Dibujo
//==============================================================================
void PianoRoll::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.fillAll (pal.body2);
    const auto grid = gridArea(), keys = keysArea(), rulerR = rulerArea(), lane = laneArea();
    const auto ink = pal.ink;
    const double bar = (double) beatsPerBar;
    const double b0 = beatOf (grid.getX()), b1 = beatOf (grid.getRight());
    const int rowTop = jmin (127, (int) std::ceil (topRow) + 1);
    const int rowBottom = jmax (0, noteOfY (grid.getBottom()) - 1);

    // Estado
    {
        const float sx = (float) metroBtn.getRight() + 14.0f;
        auto sr = Rectangle<float> (sx, 10.0f, (float) dragOut->getX() - sx - 10.0f, 42.0f);
        g.setColour (pal.dim);
        g.setFont (smallFont);
        g.drawFittedText (status, sr.toNearestInt(), Justification::centredLeft, 2);
    }

    // Rejilla
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (grid.toNearestInt());
        g.setColour (pal.card);
        g.fillRect (grid);

        g.setColour (ink.withAlpha (0.055f));
        for (int n = rowBottom; n <= rowTop; ++n)
            if (isBlack (n)) g.fillRect (grid.getX(), yOf (n), grid.getWidth(), rowH);
        for (int n = rowBottom; n <= rowTop; ++n)
        {
            g.setColour (ink.withAlpha (n % 12 == 0 ? 0.16f : 0.045f));
            g.drawHorizontalLine (roundToInt (yOf (n) + rowH), grid.getX(), grid.getRight());
        }

        // Compases alternados
        g.setColour (ink.withAlpha (0.025f));
        for (double b = std::floor (b0 / bar) * bar; b < b1; b += bar)
            if (((int) std::llround (b / bar)) % 2 == 1)
                g.fillRect (Rectangle<float> (xOf (b), grid.getY(), (float) (bar * pxPerBeat), grid.getHeight()));

        // Líneas: rejilla, tiempos y compases
        const double q = gridOrDefault();
        double step = q;
        while (step * pxPerBeat < 7.0) step *= 2.0;
        for (double b = std::floor (b0 / step) * step; b < b1; b += step)
        {
            const bool isBar = std::abs (b / bar - std::round (b / bar)) < 1.0e-6;
            const bool isBeat = std::abs (b - std::round (b)) < 1.0e-6;
            g.setColour (ink.withAlpha (isBar ? 0.30f : (isBeat ? 0.13f : 0.055f)));
            g.drawVerticalLine (roundToInt (xOf (b)), grid.getY(), grid.getBottom());
        }

        // Notas: cuadradas y planas como en Ableton (sin bordes); la fuerza se ve en lo intenso del color
        const bool labels = rowH >= 9.5f;
        g.setFont (noteFont);
        const auto bgCol = pal.card;
        const auto selCol = pal.accent.interpolatedWith (pal.night ? Colours::white : Colour (0xff101012), 0.42f);
        for (auto& n : notes)
        {
            if (n.note < rowBottom || n.note > rowTop) continue;
            const float x1 = std::round (xOf (n.start)), x2 = std::round (xOf (n.start + n.length));
            if (x2 < grid.getX() || x1 > grid.getRight()) continue;
            const float y1 = std::round (yOf (n.note)), y2 = std::round (yOf (n.note) + rowH);
            // 1 pixel libre a la derecha y abajo: las notas pegadas se distinguen sin necesidad de borde
            auto r = Rectangle<float> (x1, y1 + 1.0f, jmax (2.0f, x2 - x1 - 1.0f), jmax (2.0f, y2 - y1 - 1.0f));
            const float v = (float) n.velocity / 127.0f;
            auto col = n.selected ? selCol : pal.accent.interpolatedWith (bgCol, 0.55f * (1.0f - v));
            g.setColour (col);
            g.fillRect (r);
            if (labels && r.getWidth() > 26.0f)
            {
                g.setColour ((col.getPerceivedBrightness() > 0.6f ? Colour (0xff111111) : Colours::white).withAlpha (0.85f));
                g.drawText (String (pcNames[n.note % 12]) + String (n.note / 12 - 1), r.reduced (4.0f, 0.0f), Justification::centredLeft, false);
            }
        }

        // Lápiz: muestra exactamente dónde caerá la nota
        if (drawMode && ghostPitch >= 0 && drag == none)
        {
            const float x1 = std::round (xOf (ghostBeat)), x2 = std::round (xOf (ghostBeat + gridOrDefault()));
            const float y1 = std::round (yOf (ghostPitch)), y2 = std::round (yOf (ghostPitch) + rowH);
            auto r = Rectangle<float> (x1, y1 + 1.0f, jmax (2.0f, x2 - x1 - 1.0f), jmax (2.0f, y2 - y1 - 1.0f));
            g.setColour (pal.accent.withAlpha (0.28f));
            g.fillRect (r);
            g.setColour (pal.accent.withAlpha (0.8f));
            g.drawRect (r, 1.0f);
        }

        if (drag == marquee)
        {
            g.setColour (pal.accent.withAlpha (0.12f));
            g.fillRect (marqueeRect);
            g.setColour (pal.accent);
            g.drawRect (marqueeRect, 1.2f);
        }

        // Marcador y cabezal
        g.setColour (pal.accent.withAlpha (0.55f));
        g.drawVerticalLine (roundToInt (xOf (insertBeat)), grid.getY(), grid.getBottom());
        if (playing.load())
        {
            const double head = playFromBeat + (Time::getMillisecondCounterHiRes() - playStartMs.load()) / 1000.0 * bpm / 60.0;
            g.setColour (pal.accent);
            g.fillRect (xOf (head) - 1.0f, grid.getY(), 2.0f, grid.getHeight());
        }

        if (notes.empty())
        {
            g.setColour (ink.withAlpha (0.45f));
            g.setFont (ctx.fonts.ui (16.0f, 1));
            g.drawFittedText (U8 ("Doble clic para poner una nota (o lápiz con B) · arrastra para elegir varias\n")
                              + cmdName() + U8 ("+Z deshace · ") + cmdName() + U8 ("+rueda hace zoom · arrastra la regla de arriba para acercar"),
                              grid.reduced (40.0f).toNearestInt(), Justification::centred, 3);
        }
    }

    // Regla (arrastra arriba/abajo para acercar, como en Ableton)
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (rulerR.toNearestInt());
        g.setColour (pal.body1);
        g.fillRect (rulerR);
        g.setFont (rulerFont);
        double tick = 1.0;
        while (tick * pxPerBeat < 10.0) tick *= 2.0;
        for (double b = std::floor (b0 / bar) * bar; b < b1; b += tick)
        {
            const float x = xOf (b);
            const bool isBar = std::abs (b / bar - std::round (b / bar)) < 1.0e-6;
            g.setColour (pal.text.withAlpha (isBar ? 0.6f : 0.2f));
            g.drawVerticalLine (roundToInt (x), rulerR.getBottom() - (isBar ? 12.0f : 6.0f), rulerR.getBottom());
            if (isBar && (bar * pxPerBeat >= 26.0 || ((int) std::llround (b / bar)) % 4 == 0))
                g.drawText (String ((int) std::llround (b / bar) + 1), Rectangle<float> (x + 4.0f, rulerR.getY(), 40.0f, rulerR.getHeight() - 4.0f), Justification::centredLeft, false);
        }
        const float cx = xOf (insertBeat);
        Path tri;
        tri.addTriangle (cx - 6.0f, rulerR.getY() + 2.0f, cx + 6.0f, rulerR.getY() + 2.0f, cx, rulerR.getY() + 11.0f);
        g.setColour (pal.accent);
        g.fillPath (tri);
    }

    // Teclado de lado (claro, como en Ableton y FL)
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (keys.toNearestInt());
        const Colour whiteKey (0xffeeeeec), blackKey (0xff1c1c1f);
        g.setColour (whiteKey);
        g.fillRect (keys);
        g.setFont (smallFont);
        for (int n = rowBottom; n <= rowTop; ++n)
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
                    g.drawText ("C" + String (n / 12 - 1), Rectangle<float> (keys.getRight() - 34.0f, y, 30.0f, rowH), Justification::centredRight, false);
                }
            }
        }
        g.setColour (ink.withAlpha (0.25f));
        g.drawVerticalLine ((int) keys.getRight() - 1, keys.getY(), keys.getBottom());
    }

    // Carril de fuerza (velocity)
    {
        g.setColour (pal.night ? pal.plate.darker (0.25f) : pal.card);
        g.fillRoundedRectangle (Rectangle<float> (0.0f, lane.getY(), lane.getRight(), lane.getHeight()), 8.0f);
        g.setColour (pal.dim);
        g.setFont (smallFont);
        g.drawFittedText (U8 ("Fuerza"), Rectangle<int> (6, (int) lane.getY() + 6, (int) keysW - 10, 36), Justification::topLeft, 2);
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (lane.toNearestInt());
        const bool sel = anySelected();
        const auto in = lane.reduced (0.0f, 8.0f);
        for (auto& n : notes)
        {
            const float x = xOf (n.start) + 2.0f;
            if (x < lane.getX() - 6.0f || x > lane.getRight() + 6.0f) continue;
            const float top = in.getBottom() - in.getHeight() * (float) n.velocity / 127.0f;
            g.setColour (pal.accent.withAlpha (sel && ! n.selected ? 0.3f : 0.95f));
            g.fillRect (x - 1.0f, top, 2.0f, in.getBottom() - top);
            g.fillEllipse (x - 4.5f, top - 4.5f, 9.0f, 9.0f);
        }
    }
}

//==============================================================================
// Mouse (como en Ableton)
//==============================================================================
void PianoRoll::mouseDown (const MouseEvent& e)
{
    grabKeyboardFocus();
    const auto p = e.position;
    downPos = lastPos = p;
    changed = false;
    fineDecided = false;
    fineMode = false;
    preEdit = notes;
    drag = none;

    if (keysArea().contains (p)) { drag = keysPreview; previewPitch (noteOfY (p.y), 100); repaint(); return; }
    if (rulerArea().contains (p))
    {
        drag = ruler;
        insertBeat = jmax (0.0, snapFloor (beatOf (p.x)));
        playFromBeat = insertBeat;
        rulerAnchorBeat = beatOf (p.x);
        rulerStartPx = pxPerBeat;
        repaint();
        return;
    }
    if (laneArea().contains (p)) { drag = velocityLane; mouseDrag (e); return; }
    if (! gridArea().contains (p)) return;

    if (e.mods.isMiddleButtonDown()) { drag = panning; return; }

    int edge = 0;
    const int idx = noteAt (p, edge);

    if (e.mods.isPopupMenu())
    {
        if (idx >= 0 && ! notes[(size_t) idx].selected) { for (auto& n : notes) n.selected = false; notes[(size_t) idx].selected = true; }
        repaint();
        showContextMenu (idx, beatOf (p.x));
        return;
    }

    if (drawMode)
    {
        if (idx >= 0)
        {
            // Lápiz sobre una nota: la quita (como en Ableton)
            notes.erase (notes.begin() + idx);
            changed = true;
            repaint();
            return;
        }
        for (auto& n : notes) n.selected = false;
        drawPitch = noteOfY (p.y);
        strokeCells.clear();
        const double q = gridOrDefault();
        const double b = jmax (0.0, std::floor (beatOf (p.x) / q + 1.0e-9) * q);
        addNote (drawPitch, b);
        strokeCells[(long long) std::llround (b / q)] = notes.size() - 1;
        insertBeat = b;
        drag = drawing;
        changed = true;
        repaint();
        return;
    }

    if (idx >= 0)
    {
        auto& hit = notes[(size_t) idx];
        if (e.mods.isShiftDown())
        {
            hit.selected = ! hit.selected;
            if (! hit.selected) { repaint(); return; }
        }
        else if (! hit.selected)
        {
            for (auto& n : notes) n.selected = false;
            hit.selected = true;
        }

        dragNote = idx;
        if (copyMod (e.mods) && edge == 0)
        {
            // Alt (Mac) / Ctrl (Windows) + arrastrar: copia las notas
            std::vector<Note> copies;
            for (auto& n : notes) if (n.selected) { copies.push_back (n); n.selected = false; }
            const auto anchor = notes[(size_t) idx];
            for (auto& c : copies) notes.push_back (c);
            for (size_t i = notes.size() - copies.size(); i < notes.size(); ++i)
                if (notes[i].note == anchor.note && std::abs (notes[i].start - anchor.start) < 1.0e-9) { dragNote = (int) i; break; }
            changed = true;
        }
        fineMode = fineMod (e.mods);
        drag = edge == 1 ? resizeEnd : (edge == -1 ? resizeStart : moving);
        dragOrig = notes;
        lastVelocity = notes[(size_t) dragNote].velocity;
        insertBeat = snapFloor (notes[(size_t) dragNote].start);
        previewPitch (notes[(size_t) dragNote].note, notes[(size_t) dragNote].velocity);
        repaint();
        return;
    }

    // Espacio vacío: cuadro para elegir varias y marcador
    if (! e.mods.isShiftDown()) for (auto& n : notes) n.selected = false;
    dragOrig = notes;
    drag = marquee;
    marqueeRect = {};
    insertBeat = jmax (0.0, snapFloor (beatOf (p.x)));
    playFromBeat = insertBeat;
    repaint();
}

void PianoRoll::mouseDrag (const MouseEvent& e)
{
    const auto p = e.position;
    const auto g = gridArea();

    // Cmd/Alt sobre una nota: arriba/abajo cambia la fuerza; a los lados mueve sin rejilla
    if (drag == moving && fineMode && ! fineDecided && p.getDistanceFrom (downPos) > 3.0f)
    {
        fineDecided = true;
        if (std::abs (p.y - downPos.y) > std::abs (p.x - downPos.x)) drag = noteVelocity;
    }

    switch (drag)
    {
        case moving:
        {
            const auto& a = dragOrig[(size_t) dragNote];
            const double dx = beatOf (p.x) - beatOf (downPos.x);
            const int dy = noteOfY (p.y) - noteOfY (downPos.y);
            double dBeat = (fineMode ? a.start + dx : jmax (0.0, snapRound (a.start + dx))) - a.start;
            double minStart = 1.0e9;
            int lo = 127, hi = 0;
            for (auto& n : dragOrig) if (n.selected) { minStart = jmin (minStart, n.start); lo = jmin (lo, n.note); hi = jmax (hi, n.note); }
            dBeat = jmax (dBeat, -minStart);
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
            break;
        }
        case resizeEnd:
        case resizeStart:
        {
            const auto& a = dragOrig[(size_t) dragNote];
            const double dx = beatOf (p.x) - beatOf (downPos.x);
            const double minLen = grid() > 0.0 ? jmin (grid(), 0.25) : 1.0 / 64.0;
            if (drag == resizeEnd)
            {
                const double newEnd = fineMode ? a.start + a.length + dx : snapRound (a.start + a.length + dx);
                const double d = newEnd - (a.start + a.length);
                for (size_t i = 0; i < notes.size(); ++i)
                    if (dragOrig[i].selected) notes[i].length = jmax (minLen, dragOrig[i].length + d);
                changed = changed || std::abs (d) > 1.0e-9;
            }
            else
            {
                const double newStart = fineMode ? a.start + dx : snapRound (a.start + dx);
                const double d = newStart - a.start;
                for (size_t i = 0; i < notes.size(); ++i)
                    if (dragOrig[i].selected)
                    {
                        const double end = dragOrig[i].start + dragOrig[i].length;
                        notes[i].start = jlimit (0.0, end - minLen, dragOrig[i].start + d);
                        notes[i].length = end - notes[i].start;
                    }
                changed = changed || std::abs (d) > 1.0e-9;
            }
            break;
        }
        case noteVelocity:
        {
            const int dv = roundToInt ((downPos.y - p.y) * 0.6f);
            for (size_t i = 0; i < notes.size(); ++i)
                if (dragOrig[i].selected) notes[i].velocity = jlimit (1, 127, dragOrig[i].velocity + dv);
            lastVelocity = notes[(size_t) dragNote].velocity;
            status = "Fuerza " + String (lastVelocity);
            changed = changed || dv != 0;
            break;
        }
        case marquee:
        {
            marqueeRect = Rectangle<float> (downPos, p).getIntersection (g);
            for (size_t i = 0; i < notes.size(); ++i)
            {
                const auto& n = notes[i];
                auto r = Rectangle<float> (xOf (n.start), yOf (n.note), (float) (n.length * pxPerBeat), rowH);
                notes[i].selected = dragOrig[i].selected || r.intersects (marqueeRect);
            }
            break;
        }
        case drawing:
        {
            // Lápiz como en Ableton: sigue al mouse a los lados y también arriba y abajo
            // (cada casilla de la rejilla tiene una nota; si regresas, la nota cambia de altura).
            // Con Shift la altura se queda fija.
            const double q = gridOrDefault();
            const double bA = beatOf (lastPos.x), bB = beatOf (p.x);
            const long long cA = (long long) std::floor (bA / q + 1.0e-9), cB = (long long) std::floor (bB / q + 1.0e-9);
            const long long lo = jmin (cA, cB), hi = jmax (cA, cB);
            for (long long cell = lo; cell <= hi; ++cell)
            {
                const double b = (double) cell * q;
                if (b < 0.0) continue;
                // Altura en esa casilla: la del trazo entre el punto anterior y el actual
                const double mid = b + q * 0.5;
                const float t = std::abs (bB - bA) < 1.0e-9 ? 1.0f : (float) jlimit (0.0, 1.0, (mid - bA) / (bB - bA));
                const int pitch = e.mods.isShiftDown() ? drawPitch : noteOfY (lastPos.y + (p.y - lastPos.y) * (cA == cB ? 1.0f : t));
                auto it = strokeCells.find (cell);
                if (it != strokeCells.end())
                {
                    auto& n = notes[it->second];
                    if (n.note != pitch) { n.note = pitch; previewPitch (pitch, n.velocity); changed = true; }
                    continue;
                }
                bool exists = false;
                for (auto& n : notes) if (n.note == pitch && std::abs (n.start - b) < 1.0e-6) exists = true;
                if (! exists)
                {
                    addNote (pitch, b);
                    strokeCells[cell] = notes.size() - 1;
                    changed = true;
                }
            }
            break;
        }
        case velocityLane:
        {
            const auto in = laneArea().reduced (0.0f, 8.0f);
            const bool sel = anySelected();
            const float x1 = jmin (lastPos.x, p.x) - 4.0f, x2 = jmax (lastPos.x, p.x) + 4.0f;
            for (auto& n : notes)
            {
                if (sel && ! n.selected) continue;
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
        case ruler:
        {
            // Como la regla de Ableton: arriba/abajo acerca o aleja, a los lados se mueve
            pxPerBeat = rulerStartPx * std::exp ((p.y - downPos.y) / 110.0f);
            pxPerBeat = jlimit (6.0f, 900.0f, pxPerBeat);
            scrollBeat = rulerAnchorBeat - (double) (p.x - g.getX()) / pxPerBeat;
            clampView();
            updateScrollBars();
            break;
        }
        case panning:
        {
            scrollBeat -= (double) (p.x - lastPos.x) / pxPerBeat;
            topRow += (double) (p.y - lastPos.y) / rowH;
            clampView();
            updateScrollBars();
            break;
        }
        case keysPreview:
        {
            const int n = noteOfY (p.y);
            if (n != previewNote && keysArea().contains (p)) previewPitch (n, 100);
            break;
        }
        case none:
        default: break;
    }
    lastPos = p;
    repaint();
}

void PianoRoll::mouseUp (const MouseEvent&)
{
    if (changed) commit();
    else if (drag == noteVelocity || drag == moving) status = String ((int) notes.size()) + " notas";
    drag = none;
    dragNote = -1;
    changed = false;
    marqueeRect = {};
    repaint();
}

void PianoRoll::mouseDoubleClick (const MouseEvent& e)
{
    // Flecha: doble clic pone una nota en el espacio vacío o quita la nota (como en Ableton)
    if (drawMode || ! gridArea().contains (e.position)) return;
    int edge = 0;
    const int idx = noteAt (e.position, edge);
    preEdit = notes;
    if (idx >= 0) notes.erase (notes.begin() + idx);
    else
    {
        for (auto& n : notes) n.selected = false;
        addNote (noteOfY (e.position.y), snapFloor (beatOf (e.position.x)));
    }
    commit();
}

static MouseCursor pencilCursor()
{
    // Lápiz con la punta exacta abajo a la izquierda (como en Ableton)
    static MouseCursor cursor = []
    {
        const int s = 56;   // al doble, para que se vea nítido en pantallas Retina
        Image img (Image::ARGB, s, s, true);
        {
            Graphics g (img);
            g.addTransform (AffineTransform::scale (2.0f));
            Path body;
            body.startNewSubPath (3.0f, 25.0f);           // punta
            body.lineTo (6.0f, 17.0f);
            body.lineTo (20.0f, 3.0f);
            body.lineTo (25.0f, 8.0f);
            body.lineTo (11.0f, 22.0f);
            body.closeSubPath();
            g.setColour (Colours::white);
            g.strokePath (body, PathStrokeType (3.2f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (Colour (0xff1c1c1f));
            g.fillPath (body);
            g.setColour (Colours::white.withAlpha (0.9f));
            g.drawLine (7.0f, 17.5f, 10.5f, 21.0f, 1.2f);
            g.drawLine (17.5f, 5.5f, 22.5f, 10.5f, 1.2f);
        }
        return MouseCursor (ScaledImage (img, 2.0), { 3, 25 });
    }();
    return cursor;
}

void PianoRoll::mouseMove (const MouseEvent& e)
{
    const auto p = e.position;
    MouseCursor c = MouseCursor::NormalCursor;
    const int oldGhost = ghostPitch;
    const double oldBeat = ghostBeat;
    ghostPitch = -1;
    if (gridArea().contains (p))
    {
        int edge = 0;
        const int idx = noteAt (p, edge);
        if (drawMode)
        {
            c = pencilCursor();
            if (idx < 0)
            {
                const double q = gridOrDefault();
                ghostPitch = noteOfY (p.y);
                ghostBeat = jmax (0.0, std::floor (beatOf (p.x) / q + 1.0e-9) * q);
            }
        }
        else if (idx >= 0) c = edge != 0 ? MouseCursor::LeftRightResizeCursor : MouseCursor::NormalCursor;
    }
    if (ghostPitch != oldGhost || std::abs (ghostBeat - oldBeat) > 1.0e-9) repaint (gridArea().toNearestInt());
    else if (laneArea().contains (p)) c = MouseCursor::UpDownResizeCursor;
    else if (rulerArea().contains (p)) c = MouseCursor::UpDownLeftRightResizeCursor;
    else if (keysArea().contains (p)) c = MouseCursor::PointingHandCursor;
    setMouseCursor (c);
}

void PianoRoll::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    // Proporcional al movimiento: suave con trackpad y con rueda
    // Como el trackpad de la Mac (y como Ableton): lo que ves sigue a tus dedos
    const float dy = w.deltaY;
    const float dx = w.deltaX;
    const auto g = gridArea();
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        const double anchor = beatOf (e.position.x);
        pxPerBeat *= std::exp ((std::abs (dy) > std::abs (dx) ? dy : dx) * 1.3f);
        pxPerBeat = jlimit (6.0f, 900.0f, pxPerBeat);
        scrollBeat = anchor - (double) (e.position.x - g.getX()) / pxPerBeat;
    }
    else if (e.mods.isAltDown())
    {
        const double anchorRow = topRow - (double) (e.position.y - g.getY()) / rowH;
        rowH *= std::exp (dy * 1.1f);
        rowH = jlimit (6.0f, 40.0f, rowH);
        topRow = anchorRow + (double) (e.position.y - g.getY()) / rowH;
    }
    else
    {
        const double visBeats = g.getWidth() / pxPerBeat;
        if (e.mods.isShiftDown()) scrollBeat -= (double) dy * visBeats * 0.5;
        else
        {
            topRow += (double) dy * 28.0;
            scrollBeat -= (double) dx * visBeats * 0.5;
        }
    }
    clampView();
    updateScrollBars();
    repaint();
}

void PianoRoll::mouseMagnify (const MouseEvent& e, float scale)
{
    const double anchor = beatOf (e.position.x);
    pxPerBeat = jlimit (6.0f, 900.0f, pxPerBeat * scale);
    scrollBeat = anchor - (double) (e.position.x - gridArea().getX()) / pxPerBeat;
    clampView();
    updateScrollBars();
    repaint();
}

bool PianoRoll::keyPressed (const KeyPress& key)
{
    const auto mods = key.getModifiers();
    const bool cmd = mods.isCommandDown();
    const int code = key.getKeyCode();
    auto is = [code] (char c) { return code == c || code == (c - 'A' + 'a'); };

    // Tus atajos (metrónomo y grabar) primero
    switch (shortcutFor ? shortcutFor (key) : -1)
    {
        case 0:   // metrónomo: aquí prende o apaga el clic al reproducir
            metroOn = ! metroOn;
            metroBtn.setActive (metroOn);
            return true;
        case 1:   // parar el metrónomo (las notas siguen)
        {
            bool used = onStopMetronome && onStopMetronome();
            if (playing.load() && metroOn) { metroOn = false; metroBtn.setActive (false); used = true; }
            if (used) return true;
            break;
        }
        case 2:
            if (onRecordKey) { onRecordKey(); return true; }
            break;
        default: break;
    }

    if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey) { deleteSelected(); return true; }
    if (key == KeyPress::escapeKey) { selectAll (false); return true; }
    if (key == KeyPress::spaceKey) { togglePlay(); return true; }

    if (key.isKeyCode (KeyPress::upKey))    { transpose (mods.isShiftDown() ? 12 : 1); return true; }
    if (key.isKeyCode (KeyPress::downKey))  { transpose (mods.isShiftDown() ? -12 : -1); return true; }
    if (key.isKeyCode (KeyPress::leftKey))  { if (mods.isShiftDown()) stretch (-1); else nudge (-1); return true; }
    if (key.isKeyCode (KeyPress::rightKey)) { if (mods.isShiftDown()) stretch (1); else nudge (1); return true; }
    if (cmd)
    {
        if (is ('A')) { selectAll (true); return true; }
        if (is ('C')) { copySelected (false); return true; }
        if (is ('X')) { copySelected (true); return true; }
        if (is ('V')) { paste(); return true; }
        if (is ('D')) { duplicate(); return true; }
        if (is ('E')) { splitAt (insertBeat, anySelected()); return true; }
        if (is ('U')) { quantize(); return true; }
        if (is ('Z')) { if (mods.isShiftDown()) redo(); else undo(); return true; }
        if (is ('Y')) { redo(); return true; }
        if (code == '1') { gridIndex = jmax (0, gridIndex - 1); snapOn = true; gridBtn.setLabel (gridName()); repaint(); return true; }
        if (code == '2') { gridIndex = jmin (5, gridIndex + 1); snapOn = true; gridBtn.setLabel (gridName()); repaint(); return true; }
        if (code == '3') { triplets = ! triplets; gridBtn.setLabel (gridName()); repaint(); return true; }
        if (code == '4') { snapOn = ! snapOn; gridBtn.setLabel (gridName()); repaint(); return true; }
        return false;
    }
    if (is ('B')) { setDraw (! drawMode); return true; }
    if (code == '+' || code == '=') { pxPerBeat *= 1.25f; clampView(); updateScrollBars(); repaint(); return true; }
    if (code == '-') { pxPerBeat /= 1.25f; clampView(); updateScrollBars(); repaint(); return true; }
    return false;
}

//==============================================================================
// Reproducir
//==============================================================================
void PianoRoll::togglePlay()
{
    if (playing.load()) { stopPlay(); return; }
    {
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
        nextClickBeat = std::ceil (playFromBeat - 1.0e-6);
        playStartMs.store (Time::getMillisecondCounterHiRes() + 40.0);
        playing.store (true);
    }
    playBtn.setIcon ("stop");
    playBtn.setActive (true);
    HighResolutionTimer::startTimer (2);
}

void PianoRoll::stopPlay()
{
    HighResolutionTimer::stopTimer();
    {
        const ScopedLock sl (playLock);
        for (int n = 0; n < 128; ++n)
            while (held[(size_t) n] > 0) { kb.noteOff (1, n, 0.0f); --held[(size_t) n]; }
        playing.store (false);
    }
    playBtn.setIcon ("play");
    playBtn.setActive (false);
    repaint();
}

void PianoRoll::hiResTimerCallback()
{
    const ScopedLock sl (playLock);
    const double t = (Time::getMillisecondCounterHiRes() - playStartMs.load()) / 1000.0;
    // Metrónomo: un clic en cada tiempo (el fuerte al empezar cada compás)
    const double head = playFromBeat + t * bpm / 60.0;
    while (t >= 0.0 && head + 1.0e-6 >= nextClickBeat)
    {
        if (metroOn && onMetroClick)
            onMetroClick (((int) std::llround (nextClickBeat)) % jmax (1, beatsPerBar) == 0);
        nextClickBeat += 1.0;
    }
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
        const double head = playFromBeat + (Time::getMillisecondCounterHiRes() - playStartMs.load()) / 1000.0 * bpm / 60.0;
        const auto g = gridArea();
        if (xOf (head) > g.getRight() - 20.0f) { scrollBeat = head - 1.0; clampView(); updateScrollBars(); }
        repaint (gridArea().getUnion (rulerArea()).toNearestInt());
    }
    else if (playBtn.isActive())
    {
        playBtn.setIcon ("play");
        playBtn.setActive (false);
        repaint();
    }

    // Chip de audio: se repinta solo si cambia (progreso, listo o viejo)
    if (++audioTick % 6 == 0 || audioState >= 0)
    {
        audioState = -2;
    }
}

//==============================================================================
PianoRollWindow::PianoRollWindow (UiContext& c, MidiKeyboardState& kb, LookAndFeel& lf)
    : DocumentWindow ("Piano roll", c.vis.pal.body2, DocumentWindow::allButtons)
{
    setLookAndFeel (&lf);
    setUsingNativeTitleBar (true);
    rollComp = new PianoRoll (c, kb);

    // Que quepa en la pantalla
    int w = 1180, h = 700;
    if (auto* d = Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        w = jmin (w, (int) (d->userArea.getWidth() * 0.9f));
        h = jmin (h, (int) (d->userArea.getHeight() * 0.85f));
    }
    rollComp->setSize (w, h);
    setContentOwned (rollComp, true);
    setResizable (true, false);
    setResizeLimits (720, 420, 6000, 4000);
    setAlwaysOnTop (true);   // que no se esconda detrás de la ventana del plugin
    centreWithSize (getWidth(), getHeight());
    startTimer (400);
}

PianoRollWindow::~PianoRollWindow()
{
    stopTimer();
    clearContentComponent();
    setLookAndFeel (nullptr);
}

#if JUCE_MAC
void midicatSetWindowAbove (void* nsViewHandle, bool above);
bool midicatIsWindowFullScreen (void* nsViewHandle);
#endif

void PianoRollWindow::keepInFront()
{
    auto* peer = getPeer();
    if (peer == nullptr || ! isVisible()) return;
    // ¿Ocupa toda la pantalla? (pantalla completa del sistema o casi todo el monitor)
    bool big = isFullScreen() || peer->isFullScreen();
   #if JUCE_MAC
    big = big || midicatIsWindowFullScreen (peer->getNativeHandle());
   #endif
    if (auto* d = Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds()))
        big = big || (getWidth() >= d->userArea.getWidth() * 0.85f && getHeight() >= d->userArea.getHeight() * 0.8f);
    if (big == raised) return;
    raised = big;
   #if JUCE_MAC
    midicatSetWindowAbove (peer->getNativeHandle(), big);
   #else
    if (big) toFront (false);
   #endif
}

void PianoRollWindow::open (const File& f)
{
    rollComp->loadFile (f);
    setName (U8 ("Piano roll · ") + f.getFileNameWithoutExtension());
    setVisible (true);
    toFront (true);
    SafePointer<PianoRoll> safe (rollComp);
    MessageManager::callAsync ([safe] { if (auto* r = safe.getComponent()) r->grabKeyboardFocus(); });
}
