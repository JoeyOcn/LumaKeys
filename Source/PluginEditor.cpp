#include "PluginEditor.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

using namespace juce;

//==============================================================================
void MidiSkyLook::setPalette (const Skin& p)
{
    pal = p;
    const auto menuBg = p.night ? p.body1.brighter (0.08f) : p.body1.brighter (0.02f);
    const auto menuText = p.text;
    setColour (PopupMenu::backgroundColourId, menuBg.withAlpha (0.0f));    // transparente: el fondo redondo lo dibuja drawPopupMenuBackground
    setColour (PopupMenu::textColourId, menuText);
    setColour (PopupMenu::headerTextColourId, p.dim);
    setColour (PopupMenu::highlightedBackgroundColourId, p.accent);
    setColour (PopupMenu::highlightedTextColourId, p.accentInk);
    setColour (ResizableWindow::backgroundColourId, menuBg);
    setColour (ColourSelector::backgroundColourId, Colours::transparentBlack);
    setColour (ColourSelector::labelTextColourId, menuText);
    setColour (Slider::textBoxTextColourId, menuText);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::trackColourId, p.accent);
    setColour (Slider::thumbColourId, p.accent);
    setColour (Slider::backgroundColourId, p.night ? p.plate.brighter (0.15f) : p.plate.darker (0.1f));
    setColour (Label::textColourId, menuText);
    setColour (TextEditor::textColourId, menuText);
    setColour (TextEditor::backgroundColourId, p.btn);
    setColour (TextEditor::outlineColourId, p.btnEdge);
    setColour (TooltipWindow::backgroundColourId, menuBg);
    setColour (TooltipWindow::textColourId, menuText);
    setColour (TooltipWindow::outlineColourId, p.edge);
    setColour (ScrollBar::thumbColourId, p.text.withAlpha (0.28f));
    setColour (ScrollBar::trackColourId, Colours::transparentBlack);
    setColour (ScrollBar::backgroundColourId, Colours::transparentBlack);
}

void MidiSkyLook::drawPopupMenuBackground (Graphics& g, int width, int height)
{
    // Menús con la misma vibra del aparato: cuerpo del estilo, esquinas redondas y un brillo arriba
    const auto r = Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    const float rad = pal.pieces.btnShape == 2 ? 5.0f : 13.0f;
    const auto bg = pal.night ? pal.body1.brighter (0.06f) : pal.body1.brighter (0.03f);
    ColourGradient gr (bg.brighter (pal.night ? 0.05f : 0.02f), 0.0f, 0.0f, bg.darker (pal.night ? 0.08f : 0.03f), 0.0f, (float) height, false);
    g.setGradientFill (gr);
    g.fillRoundedRectangle (r, rad);
    if (pal.texture > 0 || pal.flat)
    {
        g.setColour (pal.edge.withAlpha (pal.flat ? 0.9f : 0.5f));
        g.drawRoundedRectangle (r.reduced (0.75f), rad, pal.flat ? 1.5f : 1.0f);
    }
    else
    {
        g.setColour (pal.night ? Colours::black.withAlpha (0.6f) : pal.edge);
        g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
        g.setColour (Colours::white.withAlpha (pal.night ? 0.07f : 0.7f));
        g.drawLine (rad, 1.5f, (float) width - rad, 1.5f, 1.0f);
    }
    if (pal.neon)
    {
        g.setColour (pal.accent.withAlpha (0.35f));
        g.drawRoundedRectangle (r.reduced (1.5f), rad - 1.0f, 1.2f);
    }
    // Una rayita del color de acento arriba, como la luz del aparato
    g.setColour (pal.accent.withAlpha (0.85f));
    g.fillRoundedRectangle (r.getCentreX() - 14.0f, 4.0f, 28.0f, 3.0f, 1.5f);
}

void MidiSkyLook::drawPopupMenuItem (Graphics& g, const Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted,
                                     bool isTicked, bool hasSubMenu, const String& text, const String& shortcutKeyText,
                                     const Drawable*, const Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour (pal.text.withAlpha (0.09f));
        g.fillRect (Rectangle<float> ((float) area.getX() + 14.0f, (float) area.getCentreY(), (float) area.getWidth() - 28.0f, 1.0f));
        return;
    }
    auto r = area.toFloat().reduced (5.0f, 1.5f);
    const float rad = pal.pieces.btnShape == 1 ? r.getHeight() * 0.5f : (pal.pieces.btnShape == 2 ? 3.0f : 8.0f);
    const bool hi = isHighlighted && isActive;
    if (hi)
    {
        g.setColour (pal.accent);
        g.fillRoundedRectangle (r, rad);
    }
    Colour col = textColour != nullptr ? *textColour : pal.text;
    if (hi) col = pal.accentInk;
    if (! isActive) col = col.withAlpha (0.35f);

    auto tickArea = r.removeFromLeft (30.0f);
    if (isTicked)
    {
        const auto c = tickArea.getCentre().translated (2.0f, 0.0f);
        g.setColour (hi ? pal.accentInk : pal.accent);
        g.fillEllipse (c.x - 4.5f, c.y - 4.5f, 9.0f, 9.0f);
    }
    if (hasSubMenu)
    {
        auto ar = r.removeFromRight (22.0f);
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::rotation (-MathConstants<float>::halfPi, ar.getCentreX(), ar.getCentreY()));
        draw::icon (g, "chev", ar.withSizeKeepingCentre (14.0f, 14.0f), col.withAlpha (hi ? 1.0f : 0.55f), 1.6f);
    }
    g.setColour (col);
    g.setFont (fonts.ui (15.0f, isTicked ? 2 : 1));
    g.drawFittedText (text, r.reduced (2.0f, 0.0f).toNearestInt(), Justification::centredLeft, 1, 0.85f);
    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour (col.withAlpha (0.55f));
        g.setFont (fonts.ui (13.0f, 1));
        g.drawText (shortcutKeyText, r.reduced (8.0f, 0.0f), Justification::centredRight, false);
    }
}

void MidiSkyLook::drawPopupMenuSectionHeader (Graphics& g, const Rectangle<int>& area, const String& name)
{
    g.setColour (pal.dim);
    g.setFont (fonts.ui (12.5f, 2));
    g.drawText (name, area.reduced (14, 0), Justification::bottomLeft, true);
}

void MidiSkyLook::getIdealPopupMenuItemSize (const String& text, bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight)
{
    if (isSeparator)
    {
        idealWidth = 50;
        idealHeight = 11;
        return;
    }
    idealHeight = jmax (32, standardMenuItemHeight);
    idealWidth = roundToInt (draw::textWidth (fonts.ui (15.0f, 2), text)) + 30 + 40;
}

Rectangle<int> MidiSkyLook::getTooltipBounds (const String& tipText, Point<int> screenPos, Rectangle<int> parentArea)
{
    const auto f = fonts.ui (14.0f, 1);
    const int w = jmin (360, roundToInt (draw::textWidth (f, tipText)) + 28);
    const int lines = jmax (1, (int) std::ceil (draw::textWidth (f, tipText) / 332.0f));
    const int h = 14 + lines * 20;
    return Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                           screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 12, w, h)
             .constrainedWithin (parentArea);
}

void MidiSkyLook::drawTooltip (Graphics& g, const String& text, int width, int height)
{
    const auto r = Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    const auto bg = pal.night ? pal.body1.brighter (0.1f) : pal.body1;
    g.setColour (bg);
    g.fillRoundedRectangle (r, 9.0f);
    g.setColour (pal.night ? Colours::black.withAlpha (0.6f) : pal.edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, 1.0f);
    g.setColour (pal.text);
    g.setFont (fonts.ui (14.0f, 1));
    g.drawFittedText (text, r.reduced (14.0f, 7.0f).toNearestInt(), Justification::centredLeft, 4, 0.9f);
}

void MidiSkyLook::drawCornerResizer (Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    // Tres puntitos en diagonal, metidos para respetar la curva del marco
    const auto col = (pal.night ? Colours::white : Colours::black).withAlpha (isMouseOver || isMouseDragging ? 0.55f : 0.25f);
    g.setColour (col);
    const float s = (float) jmin (w, h);
    const float d = jmax (1.6f, s * 0.11f);
    const Point<float> base ((float) w - s * 0.5f, (float) h - s * 0.5f);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j <= i; ++j)
        {
            const Point<float> p (base.x - (float) (i - j) * d * 2.1f, base.y - (float) j * d * 2.1f);
            g.fillEllipse (p.x - d * 0.5f, p.y - d * 0.5f, d, d);
        }
}

//==============================================================================
static const char* rootNamesEn[12] = { "C", "C#/Db", "D", "Eb", "E", "F", "F#/Gb", "G", "Ab", "A", "Bb", "B" };
static const char* rootNamesEs[12] = { "Do", "Do#/Reb", "Re", "Mib", "Mi", "Fa", "Fa#/Solb", "Sol", "Lab", "La", "Sib", "Si" };

static int whiteIndexOfNote (int note)
{
    static const int inOct[12] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };
    return (note / 12) * 7 + inOct[note % 12];
}

// Medidas a escala 1 (ventana normal de 1290 x 760)
static constexpr float kRefW = 1290.0f;
static constexpr float kMargin = 22.0f, kGap = 16.0f, kRailH = 62.0f, kRailHidden = 56.0f,
                       kMiddleH = 388.0f, kDegreesH = 104.0f, kKeysH = 234.0f, kSideW = 350.0f, kInnerGap = 12.0f;

MidiSkyEditor::MidiSkyEditor (MidiSkyProcessor& p)
    : AudioProcessorEditor (p), proc (p), look (*fonts)
{
    setOpaque (true);
    setLookAndFeel (&look);
    setWantsKeyboardFocus (true);   // para tocar los grados con las teclas de la compu

    addAndMakeVisible (rail);
    for (auto* s : sections()) addAndMakeVisible (s);

    prefs->addChangeListener (this);
    vis.hideParams = prefs->hideParams;
    vis.transparent = prefs->transparent;

    // Vaciar eventos viejos (para que no haya un destello al abrir)
    {
        auto& fifo = proc.notes.fifo;
        int s1, n1, s2, n2;
        fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
        fifo.finishedRead (n1 + n2);
    }

    // --- Barra de arriba
    rail.midi.onClick    = [this] { showMidiMenu(); };
    rail.key.onClick     = [this] { showKeyMenu(); };
    rail.scale.onClick   = [this] { showScaleMenu(); };
    rail.latency.onClick = [this] { showLatencyPanel(); };
    rail.dayBtn.onClick     = [this] { setDayNight (prefs->isNight() ? 0 : 1); };
    rail.colourBtn.onClick  = [this] { showInterfacePanel (rail.colourBtn); };
    rail.transpBtn.onClick  = [this] { setTransparent (! prefs->transparent); };
    rail.splitBtn.onClick   = [this] { if (keyboard.floating) dock (keyboard); else detach (keyboard); };
    rail.windowsBtn.onClick = [this] { showWindowsMenu(); };
    rail.hideBtn.onClick    = [this] { setHideParams (! prefs->hideParams); };
    rail.menuBtn.onClick    = [this] { showMainMenu(); };
    rail.recBtn.onClick     = [this] { proc.recorder.toggle(); };
    rail.recBtn.onMore      = [this] { showRecordingsPanel (rail.recBtn); };
    rail.metroBtn.onClick   = [this] { proc.metroSettings.on.store (! proc.metroSettings.on.load()); };
    rail.metroBtn.onMore    = [this] { showMetronomePanel (rail.metroBtn); };

    proc.recorder.onAudioSaved = [this] (const File& wav)
    {
        if (rollWindow != nullptr && wav == MidiRecorder::audioFileFor (rollWindow->roll().getFile()))
            rollWindow->roll().setStatus (U8 ("Audio listo: arrástralo desde «Arrastra audio» a Ableton o FL Studio"));
    };

    if (proc.host != nullptr)
    {
        proc.host->onBeforeChange = [this] { hostedWindow.reset(); };
        proc.host->onChanged = [this] { keyboard.repaint(); timerCallback(); };
    }
    {
        Component::SafePointer<MidiSkyEditor> safeEditor (this);
        rail.resetBtn.onClick = [safeEditor]
        {
            MessageManager::callAsync ([safeEditor] { if (auto* ed = safeEditor.getComponent()) ed->resetLayout(); });
        };
    }

    for (auto* s : sections())
        s->onDetach = [this, s] { detach (*s); };
    // Botoncito de ajustes de cada pieza
    screen.onTune   = [this] { showPiecePanel (PiecePanel::screenPiece); };
    degrees.onTune  = [this] { showPiecePanel (PiecePanel::degreesPiece); };
    circle.onTune   = [this] { showPiecePanel (PiecePanel::circlePiece); };
    score.onTune    = [this] { showPiecePanel (PiecePanel::staffPiece); };
    keyboard.onTune = [this] { showPiecePanel (PiecePanel::keysPiece); };
    circle.onLatin  = [this] (bool latin) { prefs->latin = latin; prefs->save(); };

    // --- Teclado y sus controles
    keyboard.pianoSw.onChange = [this] (bool v) { setBool ("piano", v); };
    keyboard.scaleSw.onChange = [this] (bool v) { setBool ("highlight", v); };
    keyboard.soundBtn.onClick = [this] { showSoundMenu(); };
    keyboard.pedalBtn.onClick = [this]
    {
        const bool on = proc.apvts.getRawParameterValue ("sustain")->load() > 0.5f;
        setBool ("sustain", ! on);
        keyboard.pedalBtn.setActive (! on || vis.pedal);
        keyboard.pedalBtn.setLabel (! on ? "Pedal puesto" : "Pedal");
    };
    keyboard.keysColourBtn.onClick = [this] { showKeysPanel (keyboard.keysColourBtn); };
    keyboard.uiColourBtn.onClick   = [this] { showInterfacePanel (keyboard.uiColourBtn); };
    keyboard.onShift   = [this] (int dir) { applyRange (prefs->startC + 12 * dir, prefs->octaves); };
    keyboard.onOctaves = [this] (int n) { applyRange (prefs->startC, n); };
    keyboard.onNote = [this] (int note, float vel, bool on)
    {
        if (on) proc.uiKeyboard.noteOn (1, note, vel);
        else    proc.uiKeyboard.noteOff (1, note, 0.0f);
    };

    if (auto* vol = proc.apvts.getParameter ("volume"))
    {
        volumeAttachment = std::make_unique<ParameterAttachment> (*vol, [this, vol] (float value)
        {
            const float n = vol->convertTo0to1 (value);
            keyboard.volume.setValue (n, vol->getText (n, 16));
            keyboard.repaint();
        });
        volumeAttachment->sendInitialUpdate();
        keyboard.volume.onGestureStart = [this] { volumeAttachment->beginGesture(); };
        keyboard.volume.onGestureEnd   = [this] { volumeAttachment->endGesture(); };
        keyboard.volume.onValue = [this, vol] (float n) { volumeAttachment->setValueAsPartOfGesture (vol->convertFrom0to1 (n)); };
        keyboard.volume.onReset = [this, vol] { volumeAttachment->setValueAsCompleteGesture (vol->convertFrom0to1 (vol->getDefaultValue())); };
    }

    // --- Círculo y grados
    circle.onKeyClicked = [this] (int pc, bool minor)
    {
        setChoice ("root", pc + 1);
        setChoice ("scale", minor ? 1 : 0);
    };
    degrees.onPreview = [this] (int i, float vel) { previewChord (i, vel); };
    degrees.onOctave = [this] (int d)
    {
        prefs->degreeOctave = jlimit (-2, 2, prefs->degreeOctave + d);
        prefs->save (false);
        degrees.setOctave (3 + prefs->degreeOctave);
        proc.updateDegreeChords (vis.keyRoot, vis.scale, vis.sevenths, prefs->degreeOctave);
        // La tonalidad va dentro de cada grabación (si es automática y aún no sabe, se calcula al arrastrar)
        proc.recorder.keyRoot.store (vis.autoKey && ! vis.autoHasData ? -1 : vis.keyRoot);
        proc.recorder.keyScale.store (vis.scale);
    };
    degrees.setOctave (3 + prefs->degreeOctave);
    degrees.onSettings = [this] { showDegreeInputPanel(); };
    degrees.onRelease = [this]
    {
        // Suena lo que lo dejes apretado (mínimo un instante, para que se oiga)
        previewOffAt = jmax (Time::getMillisecondCounterHiRes(), previewStartedAt + 220.0);
    };

   #if MIDISKY_DEMO
    {
        auto list = StringArray::fromTokens (SystemStats::getEnvironmentVariable ("MIDISKY_NOTES", ""), ",", "");
        for (auto& t : list)
            if (t.trim().isNotEmpty())
            {
                const int n = jlimit (0, 127, t.trim().getIntValue());
                proc.notes.down[(size_t) n].store (100);
                proc.notes.push ((uint8_t) n, 100);
            }
        if (SystemStats::getEnvironmentVariable ("MIDISKY_PEDAL", "") == "1") proc.notes.pedal.store (true);
        for (auto& t : StringArray::fromTokens (SystemStats::getEnvironmentVariable ("MIDISKY_SUS", ""), ",", ""))
            if (t.trim().isNotEmpty()) proc.notes.sustained[(size_t) jlimit (0, 127, t.trim().getIntValue())].store (90);
        if (SystemStats::getEnvironmentVariable ("MIDISKY_LATCH", "") == "1") setBool ("sustain", true);
        demoPopover = SystemStats::getEnvironmentVariable ("MIDISKY_POPOVER", "");
    }
   #endif

    applyRange (prefs->startC, prefs->octaves);
    vis.viewWhite = (float) whiteIndexOfNote (prefs->startC);
    vis.viewWhites = (float) (prefs->octaves * 7 + 1);
    refreshPalette();

    const int startW = jlimit (480, 3200, proc.editorW.load());
    const int startH = jlimit (150, 2200, proc.editorH.load());
    setResizable (true, true);
    setResizeLimits (480, 150, 3200, 2200);
    setSize (startW, startH);

    applyLowLatencyOnce();

    startTime = lastFrame = Time::getMillisecondCounterHiRes();
    frame();
    vblank = std::make_unique<VBlankAttachment> (this, [this] { frame(); });
    startTimerHz (12);
}

MidiSkyEditor::~MidiSkyEditor()
{
    stopTimer();
    vblank.reset();
    stopPreview();
    clipPlayer.stop();
    popover.reset();
    rollWindow.reset();
    hostedWindow.reset();
    proc.recorder.onAudioSaved = nullptr;
    if (proc.host != nullptr) { proc.host->onBeforeChange = nullptr; proc.host->onChanged = nullptr; proc.host->onListChanged = nullptr; }
    prefs->removeChangeListener (this);
    for (auto& [id, w] : windows)
        if (w != nullptr) { w->releaseSection(); w->setLookAndFeel (nullptr); }
    windows.clear();
    setLookAndFeel (nullptr);
}

bool MidiSkyEditor::isStandalone() const
{
    return proc.wrapperType == AudioProcessor::wrapperType_Standalone;
}

std::vector<Section*> MidiSkyEditor::sections()
{
    return { &score, &screen, &degrees, &circle, &keyboard };
}

Section* MidiSkyEditor::sectionById (const String& id)
{
    for (auto* s : sections()) if (s->id == id) return s;
    return nullptr;
}

//==============================================================================
void MidiSkyEditor::setChoice (const String& id, int index)
{
    if (auto* p = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (id)))
    {
        p->beginChangeGesture();
        *p = index;
        p->endChangeGesture();
    }
}

void MidiSkyEditor::setBool (const String& id, bool value)
{
    if (auto* p = dynamic_cast<AudioParameterBool*> (proc.apvts.getParameter (id)))
    {
        p->beginChangeGesture();
        *p = value;
        p->endChangeGesture();
    }
}

void MidiSkyEditor::applyRange (int startC, int octaves)
{
    octaves = jlimit (2, 4, octaves);
    startC = jlimit (12, 108 - 12 * octaves, startC - ((startC % 12) + 12) % 12);
    const bool changed = startC != prefs->startC || octaves != prefs->octaves;
    prefs->startC = startC;
    prefs->octaves = octaves;
    keyboard.setRange (startC, octaves);
    if (changed) prefs->save (false);
}

//==============================================================================
void MidiSkyEditor::refreshPalette()
{
    vis.pal = prefs->palette();
    winVis.pal = windowSkin (vis.pal);
    winVis.latin = prefs->latin;
    winVis.hideParams = vis.hideParams;
    MotionSettings::speed = prefs->pieces.anim ? prefs->pieces.animSpeed : 60.0f;
    MotionSettings::panels = prefs->pieces.anim && prefs->pieces.panelMotion;
    vis.latin = prefs->latin;
    vis.sevenths = prefs->sevenths;
    look.setPalette (winVis.pal);   // menús, avisos y ventanas: de día, claros
    rail.transpBtn.setActive (prefs->transparent);
    rail.splitBtn.setActive (keyboard.floating);
    rail.hideBtn.setIcon (prefs->hideParams ? "eye" : "eyeoff");
    rail.dayBtn.setIcon (prefs->isNight() ? "moon" : "sun");
    {
        const auto& sc = prefs->shortcuts;
        auto keyHint = [] (Prefs::Shortcut k) { return k.code > 0 ? U8 (" · tecla: ") + Prefs::shortcutName (k) : String(); };
        rail.metroBtn.setTooltip (U8 ("Metrónomo (la flechita abre sus ajustes)") + keyHint (sc[Prefs::scMetro]));
        rail.recBtn.setTooltip (U8 ("Grabar MIDI (la flechita abre tus grabaciones)") + keyHint (sc[Prefs::scRecord]));
    }
    rail.dayBtn.setActive (prefs->dayNight == 2);
    lastNight = prefs->isNight();
    applyRange (prefs->startC, prefs->octaves);
    rebuildBackground();
    lastRoot = -1;
    repaint();
    for (auto& [id, w] : windows)
        if (w != nullptr) w->repaint();
    if (popover != nullptr)
    {
        popover->sendLookAndFeelChange();
        popover->repaint();
        // El vidrio del panel se actualiza cuando ya se pintó lo de atrás
        Component::SafePointer<Popover> sp (popover.get());
        Timer::callAfterDelay (60, [sp] { if (sp != nullptr) sp->refreshGlass(); });
    }
}

void MidiSkyEditor::changeListenerCallback (ChangeBroadcaster*)
{
    proc.applyPrefs();
    // Otra instancia (o este editor) cambió las preferencias
    const bool hideChanged = vis.hideParams != prefs->hideParams;
    const bool transpChanged = vis.transparent != prefs->transparent;
    vis.hideParams = prefs->hideParams;
    vis.transparent = prefs->transparent;
    refreshPalette();
    if (transpChanged) recreateFloating();
    if (hideChanged || transpChanged)
    {
        rail.resized();
        for (auto* s : sections()) s->refreshControls();
        resized();
    }
}

void MidiSkyEditor::rebuildBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;
    const float scale = jlimit (1.0f, 2.0f, Component::getApproximateScaleFactorForComponent (this));
    background = Image (Image::RGB, roundToInt ((float) w * scale), roundToInt ((float) h * scale), false);
    Graphics g (background);
    g.addTransform (AffineTransform::scale (scale));
    draw::device (g, getLocalBounds().toFloat(), vis.pal, k, true);
}

void MidiSkyEditor::paint (Graphics& g)
{
    if (background.isValid()) g.drawImage (background, getLocalBounds().toFloat());
    else                      g.fillAll (vis.pal.body2);
}

float MidiSkyEditor::naturalHeight() const
{
    float h = kMargin * 2.0f + (vis.hideParams ? kRailHidden : kRailH);
    if (! score.floating || ! circle.floating || ! screen.floating) h += kGap + kMiddleH;
    else if (! degrees.floating) h += kGap + kDegreesH;
    if (! keyboard.floating) h += kGap + kKeysH;
    return h;
}

void MidiSkyEditor::fitHeightToContent (float keepK)
{
    const int h = jlimit (150, 2200, roundToInt (naturalHeight() * keepK));
    if (h != getHeight()) setSize (getWidth(), h);
    else resized();
}

void MidiSkyEditor::resized()
{
    proc.editorW.store (getWidth());
    proc.editorH.store (getHeight());

    k = jlimit (0.45f, 2.6f, jmin ((float) getWidth() / kRefW, (float) getHeight() / naturalHeight()));
    vis.uiScale = winVis.uiScale = k;
    const int m = roundToInt (kMargin * k);
    const int gap = roundToInt (kGap * k);

    auto b = getLocalBounds().reduced (m);
    rail.k = k;
    rail.setBounds (b.removeFromTop (roundToInt ((vis.hideParams ? kRailHidden : kRailH) * k)));
    rail.resized();
    b.removeFromTop (gap);

    for (auto* s : sections())
        if (! s->floating) s->k = k;

    const bool middle = ! score.floating || ! circle.floating || ! screen.floating || ! degrees.floating;
    if (! keyboard.floating)
    {
        const int kh = middle ? roundToInt (kKeysH * k) : b.getHeight();
        keyboard.setBounds (b.removeFromBottom (jmin (kh, b.getHeight())));
        b.removeFromBottom (gap);
    }

    if (middle && b.getHeight() > 20)
    {
        const bool centre = ! screen.floating || ! degrees.floating;
        const int side = roundToInt (kSideW * k);
        if (! score.floating)
        {
            const int w = centre ? side : (circle.floating ? b.getWidth() : b.getWidth() - side - gap);
            score.setBounds (b.removeFromLeft (jmin (w, b.getWidth())));
            b.removeFromLeft (gap);
        }
        if (! circle.floating)
        {
            circle.setBounds (b.removeFromRight (jmin (centre ? side : b.getWidth(), b.getWidth())));
            b.removeFromRight (gap);
        }
        if (centre)
        {
            if (! degrees.floating)
            {
                const int dh = roundToInt (kDegreesH * k);
                if (screen.floating) degrees.setBounds (b.withSizeKeepingCentre (b.getWidth(), jmin (dh, b.getHeight())));
                else
                {
                    degrees.setBounds (b.removeFromBottom (jmin (dh, b.getHeight())));
                    b.removeFromBottom (roundToInt (kInnerGap * k));
                }
            }
            if (! screen.floating) screen.setBounds (b);
        }
    }

    for (auto* s : sections())
        if (! s->floating) s->resized();
    if (popover != nullptr) { popover->setBounds (getLocalBounds()); popover->resized(); }

    updateResetButton();
    rebuildBackground();
}

//==============================================================================
// Paneles (se cierran con la X, con Esc o con un clic afuera)
//==============================================================================
void MidiSkyEditor::showPopover (const String& id, const String& title, std::unique_ptr<Component> content, Component& anchor)
{
    if (popover != nullptr)
    {
        const bool same = popover->anchorId == id;
        closePopover();
        if (same) return;   // tocar el mismo botón lo cierra
    }
    popover = std::make_unique<Popover> (winCtx, title, std::move (content), getLocalArea (&anchor, anchor.getLocalBounds()));
    popover->anchorId = id;
    popover->onClose = [this] { closePopover(); };
    // Cada panel recuerda su tamaño (se ajusta arrastrando su esquina)
    {
        auto it = prefs->panelZoom.find (id.upToFirstOccurrenceOf (":", false, false));
        const bool piece = id.startsWith ("pieza");
        popover->zoom = it != prefs->panelZoom.end() ? it->second : (piece ? 1.0f : 0.92f);
        const auto key = id.upToFirstOccurrenceOf (":", false, false);
        popover->onZoom = [this, key] (float z) { prefs->panelZoom[key] = z; prefs->save (false); };
    }
    Component::SafePointer<MidiSkyEditor> safe (this);
    popover->backdrop = [safe] (Rectangle<int> area, float scale) -> Image
    {
        if (auto* ed = safe.getComponent()) return ed->backdropFor (area, scale);
        return {};
    };
    addAndMakeVisible (*popover);
    popover->sendLookAndFeelChange();   // que los controles internos tomen los colores actuales
    popover->setBounds (getLocalBounds());
    popover->resized();
    popover->toFront (true);
    popover->grabKeyboardFocus();
    if (MotionSettings::panels)
    {
        popover->setAlpha (0.0f);
        Desktop::getInstance().getAnimator().animateComponent (popover.get(), popover->getBounds(), 1.0f, 160, false, 0.0, 1.0);
    }
}

Image MidiSkyEditor::backdropFor (Rectangle<int> area, float scale)
{
    // Lo que hay detrás del panel (sin el panel), chiquito para difuminarlo rápido
    area = area.getIntersection (getLocalBounds());
    const int w = jmax (1, roundToInt ((float) area.getWidth() * scale)), h = jmax (1, roundToInt ((float) area.getHeight() * scale));
    if (area.isEmpty()) return {};
    Image img (Image::RGB, w, h, true);
    Graphics g (img);
    g.addTransform (AffineTransform::translation ((float) -area.getX(), (float) -area.getY()).scaled (scale));
    paint (g);
    for (auto* c : getChildren())
    {
        if (c == popover.get() || ! c->isVisible() || ! c->getBounds().intersects (area)) continue;
        Graphics::ScopedSaveState ss (g);
        g.setOrigin (c->getPosition());
        c->paintEntireComponent (g, false);
    }
    return img;
}

void MidiSkyEditor::closePopover()
{
    if (popover != nullptr)
    {
        removeChildComponent (popover.get());
        popover.reset();
    }
}

PopupMenu::Options MidiSkyEditor::menuOptions (Component& target, int minWidth)
{
    return PopupMenu::Options().withTargetComponent (&target).withParentComponent (this)
                               .withMinimumWidth (minWidth).withStandardItemHeight (30);
}

LayoutPreset MidiSkyEditor::currentLayout (const String& name)
{
    LayoutPreset l;
    l.name = name;
    for (auto* s : sections())
    {
        auto f = prefs->floats[s->id];
        f.floating = s->floating;
        if (s->floating)
            if (auto* w = windows[s->id].get())
            {
                f.collapsed = w->shell().isCollapsed();
                if (! f.collapsed) f.bounds = w->getBounds();
            }
        l.floats[s->id] = f;
    }
    l.transparent = prefs->transparent;
    l.hideParams = prefs->hideParams;
    l.editorW = getWidth();
    l.editorH = getHeight();
    return l;
}

PanelHooks MidiSkyEditor::panelHooks()
{
    Component::SafePointer<MidiSkyEditor> safe (this);
    PanelHooks h;
    h.openDegreeInput = [safe]
    {
        MessageManager::callAsync ([safe] { if (auto* ed = safe.getComponent()) ed->showDegreeInputPanel(); });
    };
    h.captureTheme = [safe] (const String& name)
    {
        auto* ed = safe.getComponent();
        if (ed == nullptr) return ThemePreset();
        auto t = ed->prefs->captureTheme (name);
        t.hasLayout = true;
        t.layout = ed->currentLayout (name);
        return t;
    };
    h.applyTheme = [safe] (const ThemePreset& t)
    {
        auto* ed = safe.getComponent();
        if (ed == nullptr) return;
        ed->prefs->applyTheme (t);
        ed->applyRange (ed->prefs->startC, ed->prefs->octaves);
        if (t.hasLayout)
        {
            // El acomodo de las ventanas se aplica después (cierra este panel)
            const auto layout = t.layout;
            MessageManager::callAsync ([safe, layout] { if (auto* e2 = safe.getComponent()) e2->applyLayoutPreset (layout); });
        }
    };
    return h;
}

void MidiSkyEditor::showInterfacePanel (Component& anchor)
{
    auto panel = std::make_unique<CustomizePanel> (winCtx, *prefs, panelHooks());
    showPopover ("interfaz", "Personalizar", std::move (panel), anchor);
    if (popover != nullptr && popover->anchorId == "interfaz")
        popover->dimAmount = 0.0f;   // sin oscurecer: los cambios se ven en vivo
}

void MidiSkyEditor::showKeysPanel (Component&)
{
    showPiecePanel (PiecePanel::keysPiece);
}

void MidiSkyEditor::showPiecePanel (int piece)
{
    Section* sec = piece == PiecePanel::screenPiece ? (Section*) &screen
                 : piece == PiecePanel::degreesPiece ? (Section*) &degrees
                 : piece == PiecePanel::circlePiece ? (Section*) &circle
                 : piece == PiecePanel::staffPiece ? (Section*) &score : (Section*) &keyboard;
    auto panel = std::make_unique<PiecePanel> (winCtx, *prefs, piece, panelHooks());
    showPopover ("pieza:" + String (piece), PiecePanel::titleFor (piece), std::move (panel), *sec);
    if (popover != nullptr && popover->anchorId.startsWith ("pieza"))
    {
        popover->dimAmount = 0.0f;
        // Que no tape la pieza que estás ajustando
        if (! sec->floating) popover->setAvoid (sec->getBounds());
    }
}

//==============================================================================
// Ventanas separadas
//==============================================================================
bool MidiSkyEditor::anyFloating()
{
    for (auto* s : sections()) if (s->floating) return true;
    return false;
}

void MidiSkyEditor::updateResetButton()
{
    rail.setShowReset (anyFloating() || prefs->transparent);
}

Rectangle<int> MidiSkyEditor::defaultFloatBounds (Section& s)
{
    auto origin = isShowing() ? getScreenPosition() : Point<int> (120, 120);
    const auto size = FloatingShell::windowSizeFor (s);
    const int w = size.x, h = size.y;
    Rectangle<int> r;
    if (s.id == "teclado")      r = { origin.x + 20, origin.y + getHeight() + 30, w, h };
    else if (s.id == "acorde" || s.id == "tablillas") r = { origin.x + getWidth() / 2 - w / 2, origin.y + getHeight() + 20, w, h };
    else if (s.id == "partitura") r = { origin.x - w - 20, origin.y + 40, w, h };
    else                        r = { origin.x + getWidth() + 20, origin.y + 40, w, h };

    auto& displays = Desktop::getInstance().getDisplays();
    if (auto* d = displays.getDisplayForRect (r))
        r = r.constrainedWithin (d->userArea);
    else if (auto* pd = displays.getPrimaryDisplay())
        r = r.withPosition (pd->userArea.getCentre() - Point<int> (w / 2, h / 2)).constrainedWithin (pd->userArea);
    return r;
}

void MidiSkyEditor::detach (Section& s, bool save)
{
    if (s.floating && windows.count (s.id) > 0) return;
    closePopover();
    const float keepK = k;

    auto& st = prefs->floats[s.id];
    auto bounds = st.bounds;
    if (bounds.isEmpty() || bounds.getHeight() < FloatingShell::headerHeight + 20
        || Desktop::getInstance().getDisplays().getDisplayForRect (bounds) == nullptr)
        bounds = defaultFloatBounds (s);

    removeChildComponent (&s);
    auto w = std::make_unique<FloatingWindow> (ctx, s, prefs->transparent, st.onTop, bounds);
    w->setLookAndFeel (&look);
    const String id = s.id;
    w->onBoundsChanged = [this, id] (Rectangle<int> r)
    {
        prefs->floats[id].bounds = r;
        prefs->save (false);
    };
    Component::SafePointer<MidiSkyEditor> safe (this);
    w->shell().onDock = [safe, id]
    {
        MessageManager::callAsync ([safe, id]
        {
            if (auto* ed = safe.getComponent())
                if (auto* sec = ed->sectionById (id))
                    ed->dock (*sec);
        });
    };
    w->shell().onRestoreAll = [safe]
    {
        MessageManager::callAsync ([safe] { if (auto* ed = safe.getComponent()) ed->resetLayout(); });
    };
    w->shell().onMinimize = [this, id]
    {
        if (auto* win = windows[id].get())
        {
            const bool c = ! win->shell().isCollapsed();
            win->setCollapsed (c);
            prefs->floats[id].collapsed = c;
            prefs->save (false);
        }
    };
    w->shell().onReset = [this, id]
    {
        if (auto* sec = sectionById (id))
            if (auto* win = windows[id].get())
            {
                win->setCollapsed (false);
                prefs->floats[id].collapsed = false;
                const auto size = FloatingShell::windowSizeFor (*sec);
                win->setSize (size.x, size.y);
            }
    };
    w->shell().onTogglePin = [this, id]
    {
        auto& f = prefs->floats[id];
        f.onTop = ! f.onTop;
        if (auto* win = windows[id].get()) { win->setAlwaysOnTop (f.onTop); win->shell().setPinned (f.onTop); }
        prefs->save (false);
    };
    if (st.collapsed) w->setCollapsed (true);
    windows[id] = std::move (w);

    st.floating = true;
    s.refreshControls();
    rail.splitBtn.setActive (keyboard.floating);
    if (save)
    {
        prefs->save (false);
        fitHeightToContent (keepK);
    }
    else resized();
    repaint();
}

void MidiSkyEditor::dock (Section& s, bool save)
{
    const float keepK = k;
    auto it = windows.find (s.id);
    if (it != windows.end())
    {
        if (it->second != nullptr) { it->second->releaseSection(); it->second->setLookAndFeel (nullptr); }
        windows.erase (it);
    }
    s.floating = false;
    s.setVisible (true);
    addAndMakeVisible (s);
    if (popover != nullptr) popover->toFront (false);
    prefs->floats[s.id].floating = false;
    prefs->floats[s.id].collapsed = false;
    s.refreshControls();
    rail.splitBtn.setActive (keyboard.floating);
    if (save)
    {
        prefs->save (false);
        fitHeightToContent (keepK);
    }
    else resized();
    repaint();
}

void MidiSkyEditor::resetLayout()
{
    closePopover();
    const float keepK = k;
    for (auto* s : sections())
        if (s->floating) dock (*s, false);
    for (auto& [id, f] : prefs->floats) { f.floating = false; f.collapsed = false; f.bounds = {}; f.onTop = true; }
    prefs->transparent = false;
    prefs->hideParams = false;
    vis.transparent = false;
    vis.hideParams = false;
    prefs->save (false);

    rail.transpBtn.setActive (false);
    rail.hideBtn.setIcon ("eyeoff");
    for (auto* s : sections()) s->refreshControls();
    fitHeightToContent (keepK);
    repaint();
}

void MidiSkyEditor::restoreFloating()
{
    bool any = false;
    for (auto* s : sections())
    {
        auto it = prefs->floats.find (s->id);
        if (it != prefs->floats.end() && it->second.floating)
        {
            detach (*s, false);
            any = true;
        }
    }
    // Que la ventana principal quede del tamaño justo para lo que se quedó en ella
    if (any) fitHeightToContent (jlimit (0.45f, 2.6f, (float) getWidth() / kRefW));
}

void MidiSkyEditor::recreateFloating()
{
    std::vector<Section*> floating;
    for (auto* s : sections()) if (s->floating) floating.push_back (s);
    for (auto* s : floating) dock (*s, false);
    for (auto* s : floating) detach (*s, false);
}

void MidiSkyEditor::setTransparent (bool on)
{
    prefs->transparent = on;
    vis.transparent = on;
    prefs->save (false);
    recreateFloating();
    if (on && ! anyFloating()) { detach (keyboard, true); detach (screen, true); }
    rail.transpBtn.setActive (on);
    for (auto* s : sections()) s->refreshControls();
    resized();
    repaint();
}

void MidiSkyEditor::setHideParams (bool on)
{
    const float keepK = k;
    prefs->hideParams = on;
    vis.hideParams = on;
    prefs->save (false);
    rail.hideBtn.setIcon (on ? "eye" : "eyeoff");
    for (auto* s : sections()) s->refreshControls();
    fitHeightToContent (keepK);
    repaint();
}

void MidiSkyEditor::setDayNight (int mode)
{
    prefs->dayNight = jlimit (0, 2, mode);
    prefs->save();
}

//==============================================================================
// Estado musical
//==============================================================================
void MidiSkyEditor::updateKeyState (bool& keyChanged)
{
    const int rootParam  = (int) proc.apvts.getRawParameterValue ("root")->load();
    const int scaleParam = (int) proc.apvts.getRawParameterValue ("scale")->load();
    vis.autoKey = rootParam == 0;

    if (vis.autoKey)
    {
        float total = 0.0f;
        int distinct = 0;
        for (auto h : hist) { total += h; if (h > 0.3f) ++distinct; }
        if (total > 4.0f && distinct >= 3)
        {
            const auto gk = theory::guessKey (hist);
            if (! autoHas) { autoRoot = gk.root; autoMinor = gk.minor; autoHas = true; }
            else if (gk.root != autoRoot || gk.minor != autoMinor)
            {
                const float cur = theory::keyScore (hist, autoRoot, autoMinor);
                if (gk.score > cur + 0.06f) { autoRoot = gk.root; autoMinor = gk.minor; }
            }
        }
        vis.keyRoot = autoRoot;
        vis.scale = autoMinor ? 1 : 0;
        vis.autoHasData = autoHas;
    }
    else
    {
        vis.keyRoot = jlimit (0, 11, rootParam - 1);
        vis.scale = jlimit (0, theory::numScales() - 1, scaleParam);
    }
    vis.highlight = proc.apvts.getRawParameterValue ("highlight")->load() > 0.5f;

    keyChanged = vis.keyRoot != lastRoot || vis.scale != lastScale || vis.latin != lastLatin
                 || vis.sevenths != lastSevenths || vis.autoKey != lastAuto || vis.autoHasData != lastAutoHas;
    if (keyChanged)
    {
        vis.sig = theory::keySignature (vis.keyRoot, vis.scale);
        vis.scaleMask = theory::scaleMask (vis.keyRoot, vis.scale);
        vis.diatonic = theory::diatonicChords (vis.keyRoot, vis.scale, vis.sevenths, vis.latin);
        proc.updateDegreeChords (vis.keyRoot, vis.scale, vis.sevenths, prefs->degreeOctave);
        // La tonalidad va dentro de cada grabación (si es automática y aún no sabe, se calcula al arrastrar)
        proc.recorder.keyRoot.store (vis.autoKey && ! vis.autoHasData ? -1 : vis.keyRoot);
        proc.recorder.keyScale.store (vis.scale);
        rail.key.setValueText (theory::pcName (vis.keyRoot, vis.sig, vis.latin) + (vis.autoKey ? U8 (" (auto)") : String()));
        rail.scale.setValueText (U8 (theory::scales()[(size_t) vis.scale].name));
        lastRoot = vis.keyRoot; lastScale = vis.scale; lastLatin = vis.latin;
        lastSevenths = vis.sevenths; lastAuto = vis.autoKey; lastAutoHas = vis.autoHasData;
    }
}

std::vector<int> MidiSkyEditor::chordSetNow (double now)
{
    std::vector<int> held, sus;
    for (int n = 0; n < 128; ++n)
    {
        if (vis.down[(size_t) n] > 0) held.push_back (n);
        else if (vis.sus[(size_t) n] > 0) sus.push_back (n);
    }
    auto pcsOf = [] (const std::vector<int>& v) { uint16_t m = 0; for (auto n : v) m = (uint16_t) (m | (1u << (unsigned) (n % 12))); return theory::countBits (m); };

    double from = jmax (phraseStartMs, lastOnsetMs - 1600.0);
    std::vector<int> set = held;
    if (! held.empty())
    {
        // Si se acaba de tocar un acorde nuevo (varias notas juntas), solo cuenta ese
        double newest = 0.0;
        for (auto n : held) newest = jmax (newest, onsetMs[(size_t) n]);
        std::vector<int> group;
        for (auto n : held) if (onsetMs[(size_t) n] >= newest - 90.0) group.push_back (n);
        if (pcsOf (group) >= 2 && group.size() < held.size())
            set = group;
        if (pcsOf (set) >= 2)
        {
            double first = newest;
            for (auto n : set) first = jmin (first, onsetMs[(size_t) n]);
            phraseStartMs = jmax (phraseStartMs, first - 60.0);
            from = phraseStartMs;
        }
    }
    for (auto n : sus)
        if (onsetMs[(size_t) n] >= from) set.push_back (n);
    std::sort (set.begin(), set.end());
    set.erase (std::unique (set.begin(), set.end()), set.end());
    juce::ignoreUnused (now);
    return set;
}

void MidiSkyEditor::frame()
{
    const double now = Time::getMillisecondCounterHiRes();
    const float realDt = jlimit (0.0f, 0.1f, (float) ((now - lastFrame) / 1000.0));
    lastFrame = now;
    vis.time = (now - startTime) / 1000.0;
    // Movimiento: velocidad de las animaciones (o sin animaciones: todo llega al instante)
    const auto& motion = prefs->pieces;
    const float dt = motion.anim ? jmin (0.25f, realDt * motion.animSpeed) : 1.0f;
    auto smooth = [dt] (float rate) { return 1.0f - std::pow (1.0f - rate, jmin (60.0f, dt * 60.0f)); };

    // Notas nuevas (destellos e historial de tonalidad)
    {
        auto& fifo = proc.notes.fifo;
        const int ready = fifo.getNumReady();
        if (ready > 0)
        {
            int s1, n1, s2, n2;
            fifo.prepareToRead (ready, s1, n1, s2, n2);
            auto take = [&] (int start, int num)
            {
                for (int i = 0; i < num; ++i)
                {
                    const auto ev = proc.notes.events[(size_t) (start + i)];
                    vis.flash[ev.note] = 1.0f;
                    onsetMs[ev.note] = now;
                    lastOnsetMs = now;
                    hist[(size_t) (ev.note % 12)] += 0.5f + ev.velocity / 127.0f;
                }
            };
            take (s1, n1);
            take (s2, n2);
            fifo.finishedRead (n1 + n2);
        }
    }

    bool keysAnimating = false;
    const int km = motion.anim ? motion.keyMotion : 1;
    const float up = smooth (km == 1 ? 1.0f : 0.6f), downRate = smooth (km == 1 ? 0.55f : (km == 2 ? 0.045f : 0.16f));
    const float flashDecay = std::pow (km == 2 ? 0.12f : 0.02f, dt);
    const float flashBoost = motion.keyGlow * 0.5f;
    for (size_t n = 0; n < 128; ++n)
    {
        const auto d = proc.notes.down[n].load();
        const auto s = proc.notes.sustained[n].load();
        if (d != vis.down[n] || s != vis.sus[n]) keysAnimating = true;
        if (d == 0 && vis.down[n] > 0) releaseMs[n] = now;
        vis.down[n] = d;
        vis.sus[n] = s;
        if (d > 0) hist[n % 12] += realDt * 1.2f;

        // Las notas que deja sonando el pedal se van apagando poco a poco (como el sonido)
        float susLevel = 0.0f;
        if (d == 0 && s > 0)
        {
            const float age = (float) jmax (0.0, now - releaseMs[n]) / 1000.0f;
            const bool old = onsetMs[n] < phraseStartMs;
            susLevel = (0.30f + 0.18f * (float) s / 127.0f) * jmax (0.4f, std::exp (-age / 4.0f)) * (old ? 0.6f : 1.0f);
            if (age < 12.0f) keysAnimating = true;
        }
        const float target = d > 0 ? 0.62f + 0.38f * (float) d / 127.0f : susLevel;
        const float boosted = jmin (1.0f, target + vis.flash[n] * flashBoost);
        float& lv = vis.level[n];
        if (std::abs (boosted - lv) > 0.002f)
        {
            lv += (boosted - lv) * (boosted > lv ? up : downRate);
            keysAnimating = true;
        }
        else lv = boosted;
        if (vis.flash[n] > 0.01f) { vis.flash[n] *= flashDecay; keysAnimating = true; }
        else vis.flash[n] = 0.0f;
    }
    const float histDecay = std::exp (-realDt / 30.0f);
    for (auto& h : hist) h *= histDecay;

    const bool pedal = proc.notes.pedal.load();
    if (pedal != lastPedalSeen) { phraseStartMs = now; lastPedalSeen = pedal; }

    vis.sounding.clear();
    for (int n = 0; n < 128; ++n)
        if (vis.down[(size_t) n] > 0 || vis.sus[(size_t) n] > 0)
            vis.sounding.push_back (n);

    // El acorde que se nombra: se espera un instante a que las notas terminen de llegar
    // (o de soltarse) para que la pantalla y los grados no parpadeen
    {
        auto next = chordSetNow (now);
        if (next != candidate) { candidate = next; candidateSince = now; }
        if (candidate != vis.active)
        {
            const bool grows = std::includes (candidate.begin(), candidate.end(), vis.active.begin(), vis.active.end());
            const bool shrinks = std::includes (vis.active.begin(), vis.active.end(), candidate.begin(), candidate.end());
            const double wait = candidate.empty() ? 260.0 : (grows ? 40.0 : (shrinks ? 130.0 : 60.0));
            if (now - candidateSince >= wait || vis.active.empty())
            {
                vis.active = candidate;
                vis.activeMask = 0;
                for (auto n : vis.active) vis.activeMask = (uint16_t) (vis.activeMask | (1u << (unsigned) (n % 12)));
                vis.lowest = vis.active.empty() ? -1 : vis.active.front();
            }
        }
    }

    for (int pc = 0; pc < 12; ++pc)
    {
        const float target = (vis.activeMask & (1u << (unsigned) pc)) ? 1.0f : 0.0f;
        float& gl = vis.pcGlow[(size_t) pc];
        if (std::abs (target - gl) > 0.004f) gl += (target - gl) * (target > gl ? smooth (0.45f) : smooth (0.12f));
        else gl = target;
    }

    bool keyChanged = false;
    updateKeyState (keyChanged);

    const bool activeChanged = vis.active != lastActive;
    if (activeChanged || keyChanged)
    {
        vis.chord = theory::detectChord (vis.activeMask, vis.lowest >= 0 ? vis.lowest % 12 : 0, vis.sig, vis.latin);
        vis.degree = theory::degreeOf (vis.chord, vis.keyRoot, vis.scale);

        const int count = (int) vis.active.size();
        const int distinct = theory::countBits (vis.activeMask);
        String big;
        if (count == 0) big = vis.pal.pieces.scrIdle == 1 ? theory::keyTitle (vis.keyRoot, vis.scale, vis.latin) : String();
        else if (distinct == 1) big = theory::noteName (vis.active.front(), vis.sig, vis.latin, count == 1);
        else if (vis.chord.valid && (distinct >= 3 || vis.chord.suffix == "5")) big = vis.chord.symbol;
        else if (distinct == 2) big = theory::intervalName (theory::firstInterval (vis.active));
        else
        {
            StringArray sn;
            for (int pc = 0; pc < 12; ++pc)
                if (vis.activeMask & (1u << (unsigned) pc)) sn.add (theory::pcName (pc, vis.sig, vis.latin));
            big = sn.joinIntoString (" ");
        }
        if (big != vis.bigText)
        {
            vis.prevBigText = vis.bigText;
            vis.bigText = big;
            vis.bigFade = 0.0f;
        }

        // Grado que se enciende: el acorde tiene la raíz de ese grado y todas sus notas
        // son de ese acorde de la escala (sirve con tríadas, séptimas, inversiones y quintas)
        vis.activeDegree = -1;
        if (vis.chord.valid && theory::countBits (vis.activeMask) >= 2)
        {
            const auto full = theory::diatonicChords (vis.keyRoot, vis.scale, true, vis.latin);
            for (int i = 0; i < (int) full.size() && i < 7; ++i)
            {
                const auto& d = full[(size_t) i];
                if (vis.chord.root == d.rootPc && (vis.activeMask & ~d.mask) == 0) { vis.activeDegree = i; break; }
            }
        }
        lastActive = vis.active;
        score.repaint();
        screen.repaint();
    }

    // Grados: se encienden suavemente
    bool degreesMoving = false;
    for (int i = 0; i < 7; ++i)
    {
        float& gl = vis.degreeGlow[(size_t) i];
        const float target = i == vis.activeDegree ? 1.0f : 0.0f;
        if (std::abs (target - gl) > 0.004f) { gl += (target - gl) * smooth (target > gl ? 0.35f : 0.18f); degreesMoving = true; }
        else gl = target;
    }
    if (degreesMoving || keyChanged) degrees.repaint();

    if (vis.bigFade < 1.0f)
    {
        const float dur = motion.scrMotion == 3 ? jlimit (0.22f, 0.6f, 0.07f * (float) vis.bigText.length()) : 0.22f;
        vis.bigFade = jmin (1.0f, vis.bigFade + dt / dur);
        screen.repaint();
    }

    // La perilla del círculo gira hacia la tonalidad
    {
        const auto& sc = theory::scales()[(size_t) vis.scale];
        const int tonicIndex = sc.minorFlavour ? (theory::pcOf (vis.keyRoot + 3) * 7) % 12 : (theory::pcOf (vis.keyRoot) * 7) % 12;
        float target = (float) tonicIndex * MathConstants<float>::twoPi / 12.0f;
        float diff = std::remainder (target - vis.knobAngle, MathConstants<float>::twoPi);
        if (std::abs (diff) > 0.002f) { vis.knobAngle += diff * smooth (0.12f); circle.repaint(); }
        else vis.knobAngle = target;
    }

    // Teclado: seguir las notas que salen del rango y deslizar suavemente
    {
        // Solo se mueve si TODO lo que tocas cabe en las octavas que elegiste; si no cabe
        // (por ejemplo con 2 octavas y las manos separadas), se queda quieto en vez de
        // brincar de un lado a otro.
        if (! vis.active.empty() && ! keyboard.isMouseButtonDown (true))
        {
            const int lo = vis.active.front(), hi = vis.active.back();
            const int span = 12 * prefs->octaves;
            const int start = prefs->startC;
            const bool fits = lo >= start && hi <= start + span;
            if (! fits)
            {
                int best = -1;
                for (int c = 12; c <= 108 - span; c += 12)
                    if (c <= lo && c + span >= hi && (best < 0 || std::abs (c - start) < std::abs (best - start)))
                        best = c;
                if (best >= 0 && best != start)
                    applyRange (best, prefs->octaves);
            }
        }

        const float tw = (float) whiteIndexOfNote (prefs->startC);
        const float tn = (float) (prefs->octaves * 7 + 1);
        bool viewMoving = false;
        if (std::abs (tw - vis.viewWhite) > 0.002f) { vis.viewWhite += (tw - vis.viewWhite) * smooth (0.2f); viewMoving = true; }
        else vis.viewWhite = tw;
        if (std::abs (tn - vis.viewWhites) > 0.002f) { vis.viewWhites += (tn - vis.viewWhites) * smooth (0.2f); viewMoving = true; }
        else vis.viewWhites = tn;
        if (keysAnimating || viewMoving || keyChanged || vis.highlight != lastHighlight || pedal != vis.pedal)
            keyboard.repaint();
    }
    if (keysAnimating) score.repaint();
    if (circle.isAnimating() || keyChanged || activeChanged) circle.repaint();
    if (keyChanged) { score.repaint(); screen.repaint(); }
    lastHighlight = vis.highlight;
    vis.pedal = pedal;

    if (previewOffAt > 0.0 && now > previewOffAt)
        stopPreview();

    // Pedal: el botón responde al instante (pedal en pantalla o pedal físico)
    const bool latched = proc.apvts.getRawParameterValue ("sustain")->load() > 0.5f;
    if (latched != lastLatched || vis.pedal != lastPedalShown)
    {
        keyboard.pedalBtn.setActive (latched || vis.pedal);
        keyboard.pedalBtn.setLabel (latched ? "Pedal puesto" : "Pedal");
        lastLatched = latched;
        lastPedalShown = vis.pedal;
    }
    updateTransport (realDt);
}

void MidiSkyEditor::updateTransport (float dt)
{
    // Metrónomo: el botón se prende y parpadea con cada tiempo
    const bool metroOn = proc.metroSettings.on.load();
    rail.metroBtn.setActive (metroOn);
    const auto count = proc.metronome.uiBeatCount.load();
    if (count != lastBeatCount)
    {
        lastBeatCount = count;
        metroFlash = proc.metronome.uiBeat.load() == 0 ? 1.0f : 0.6f;
    }
    metroFlash = jmax (0.0f, metroFlash - dt * 5.0f);
    rail.metroBtn.setFlash (metroOn ? metroFlash : 0.0f);

    // Grabar: rojo mientras graba; durante la cuenta parpadea con los tiempos
    const int rs = proc.recorder.state();
    rail.recBtn.setActive (rs == 2);
    rail.recBtn.setIcon (rs == 0 ? "rec" : "stop");
    if (rs == 1) rail.recBtn.setFlash (metroFlash);
    else if (rs == 2) rail.recBtn.setFlash (0.35f + 0.35f * std::sin ((float) vis.time * 4.0f));
    else rail.recBtn.setFlash (0.0f);
    lastRecState = rs;
}

void MidiSkyEditor::timerCallback()
{
    updateMidiStatus();
    keyboard.pianoSw.setOn (proc.apvts.getRawParameterValue ("piano")->load() > 0.5f);
    keyboard.scaleSw.setOn (proc.apvts.getRawParameterValue ("highlight")->load() > 0.5f);
    const int inst = (int) proc.apvts.getRawParameterValue ("instrument")->load();
    if (proc.host != nullptr && proc.host->hasInstrument())
        keyboard.soundBtn.setValueText (proc.host->instrumentName());
    else
        keyboard.soundBtn.setValueText (instrumentNames()[inst]);
    keyboard.soundBtn.setLed (proc.isPianoReady() ? -1 : 0);
    if (auto* lat = proc.apvts.getParameter ("latency"))
        rail.latency.setValueText (lat->getCurrentValueAsText());

    if (! restoredFloats && isShowing())
    {
        restoredFloats = true;
        restoreFloating();
        if (isStandalone()) grabKeyboardFocus();
       #if MIDISKY_DEMO
        if (demoPopover == "interfaz") CustomizePanel::setStartTab (SystemStats::getEnvironmentVariable ("MIDISKY_TAB", "0").getIntValue());
        if (demoPopover == "grados") showDegreeInputPanel();
        if (demoPopover.startsWith ("pieza")) showPiecePanel (demoPopover.getTrailingIntValue());
        if (demoPopover == "metro") showMetronomePanel (rail.metroBtn);
        if (demoPopover == "atajos") showShortcutsPanel();
        if (demoPopover == "grabaciones") showRecordingsPanel (rail.recBtn);
        {
            const auto dt = SystemStats::getEnvironmentVariable ("MIDISKY_DRAGTEST", "");
            if (dt.isNotEmpty()) File ("/tmp/dragtest.txt").replaceWithText (MidiRecorder::dragCopy (File (dt)).getFullPathName());
        }
        if (demoPopover == "roll")
        {
            const auto recs = MidiRecorder::recordings();
            if (! recs.isEmpty()) openPianoRoll (recs[0]);
        }
       #endif
        if (demoPopover == "interfaz") showInterfacePanel (keyboard.uiColourBtn);
        if (demoPopover == "teclas") showKeysPanel (keyboard.keysColourBtn);
    }

    // Modo automático: revisar la hora de vez en cuando
    if (++nightCheckCounter >= 12 * 30)
    {
        nightCheckCounter = 0;
        if (prefs->dayNight == 2 && prefs->isNight() != lastNight)
            refreshPalette();
    }
}

//==============================================================================
// MIDI y audio
//==============================================================================
void MidiSkyEditor::applyLowLatencyOnce()
{
   #if JucePlugin_Build_Standalone
    if (! isStandalone() || prefs->lowLatencyDone) return;
    if (auto* holder = StandalonePluginHolder::getInstance())
    {
        auto setup = holder->deviceManager.getAudioDeviceSetup();
        if (setup.bufferSize == 0 || setup.bufferSize > 256)
        {
            setup.bufferSize = 256;
            holder->deviceManager.setAudioDeviceSetup (setup, true);
        }
        prefs->lowLatencyDone = true;
        prefs->save (false);
    }
   #endif
}

void MidiSkyEditor::updateMidiStatus()
{
    const bool activity = (int) Time::getMillisecondCounter() - proc.lastMidiActivityMs.load() < 140;

   #if JucePlugin_Build_Standalone
    if (isStandalone())
    {
        if (auto* holder = StandalonePluginHolder::getInstance())
        {
            auto& dm = holder->deviceManager;
            const auto devices = MidiInput::getAvailableDevices();
            if (prefs->midiAuto)
            {
                for (auto& d : devices)
                {
                    bool known = false;
                    for (auto& kd : knownMidiInputs) if (kd.identifier == d.identifier) known = true;
                    if (! known && ! dm.isMidiInputDeviceEnabled (d.identifier))
                        dm.setMidiInputDeviceEnabled (d.identifier, true);
                }
            }
            knownMidiInputs = devices;

            StringArray enabled;
            for (auto& d : devices)
                if (dm.isMidiInputDeviceEnabled (d.identifier)) enabled.add (d.name);

            String text;
            if (enabled.isEmpty()) text = devices.isEmpty() ? U8 ("Sin teclado MIDI") : U8 ("Desconectado");
            else if (enabled.size() == 1) text = enabled[0];
            else text = String (enabled.size()) + " dispositivos";
            rail.midi.setValueText (text);
            rail.midi.setLed (activity ? 2 : (enabled.isEmpty() ? 0 : 1));
            return;
        }
    }
   #endif

    const bool listening = proc.listen.load();
    String host = PluginHostType().getHostDescription();
    if (host.isEmpty() || host == "Unknown") host = "el DAW";
    rail.midi.setValueText (listening ? U8 ("Desde ") + host : U8 ("En pausa"));
    rail.midi.setLed (! listening ? 0 : (activity ? 2 : 1));
}

void MidiSkyEditor::showMidiMenu()
{
    closePopover();
    PopupMenu m;
   #if JucePlugin_Build_Standalone
    if (isStandalone())
    {
        auto* holder = StandalonePluginHolder::getInstance();
        if (holder == nullptr) return;
        auto& dm = holder->deviceManager;
        const auto devices = MidiInput::getAvailableDevices();
        m.addSectionHeader ("Entradas MIDI");
        if (devices.isEmpty()) m.addItem (-1, U8 ("No se encontró ningún teclado MIDI"), false);
        for (int i = 0; i < devices.size(); ++i)
            m.addItem (100 + i, devices[i].name, true, dm.isMidiInputDeviceEnabled (devices[i].identifier));
        m.addSeparator();
        m.addItem (1, U8 ("Conectar todo"), ! devices.isEmpty());
        m.addItem (2, U8 ("Desconectar todo"));
        m.addSeparator();
        m.addItem (3, U8 ("Audio y MIDI (avanzado)…"));

        m.showMenuAsync (menuOptions (rail.midi, 280), [this, devices] (int r)
        {
            auto* h = StandalonePluginHolder::getInstance();
            if (h == nullptr || r == 0) return;
            auto& d = h->deviceManager;
            if (r == 1) { for (auto& dev : devices) d.setMidiInputDeviceEnabled (dev.identifier, true); prefs->midiAuto = true; prefs->save (false); }
            else if (r == 2)
            {
                for (auto& dev : devices) d.setMidiInputDeviceEnabled (dev.identifier, false);
                prefs->midiAuto = false;
                prefs->save (false);
                proc.allOffRequest.store (true);
            }
            else if (r == 3) h->showAudioSettingsDialog();
            else if (r >= 100 && r - 100 < devices.size())
            {
                const auto& dev = devices[r - 100];
                const bool en = ! d.isMidiInputDeviceEnabled (dev.identifier);
                d.setMidiInputDeviceEnabled (dev.identifier, en);
                if (! en) proc.allOffRequest.store (true);
            }
            updateMidiStatus();
        });
        return;
    }
   #endif

    String host = PluginHostType().getHostDescription();
    if (host.isEmpty() || host == "Unknown") host = "el DAW";
    const bool listening = proc.listen.load();
    m.addSectionHeader (U8 ("MIDI que llega desde ") + host);
    m.addItem (1, "Escuchar (conectado)", true, listening);
    m.addItem (2, "Pausar (desconectado)", true, ! listening);
    m.addSeparator();
    m.addItem (-1, U8 ("Esto no desconecta nada en ") + host + ".", false);
    m.showMenuAsync (menuOptions (rail.midi, 280), [this] (int r)
    {
        if (r == 1) proc.listen.store (true);
        if (r == 2) proc.listen.store (false);
        updateMidiStatus();
    });
}

//==============================================================================
// Menús
//==============================================================================
void MidiSkyEditor::showKeyMenu()
{
    closePopover();
    const int current = (int) proc.apvts.getRawParameterValue ("root")->load();
    PopupMenu m;
    m.addItem (1, U8 ("Auto (la detecto mientras tocas)"), true, current == 0);
    m.addSeparator();
    for (int i = 0; i < 12; ++i)
        m.addItem (2 + i, vis.latin ? U8 (rootNamesEs[i]) : U8 (rootNamesEn[i]), true, current == i + 1);
    m.showMenuAsync (menuOptions (rail.key, rail.key.getWidth()),
                     [this] (int r) { if (r == 1) setChoice ("root", 0); else if (r >= 2) setChoice ("root", r - 1); });
}

void MidiSkyEditor::showScaleMenu()
{
    closePopover();
    PopupMenu m;
    const auto& sc = theory::scales();
    m.addSectionHeader ("Escalas");
    for (int i = 0; i < (int) sc.size(); ++i)
    {
        if (i == 4) m.addSectionHeader ("Modos");
        if (i == 9) m.addSectionHeader (U8 ("Pentatónicas y blues"));
        m.addItem (1 + i, U8 (sc[(size_t) i].name), true, vis.scale == i);
    }
    m.showMenuAsync (menuOptions (rail.scale, rail.scale.getWidth()), [this] (int r)
    {
        if (r <= 0) return;
        if (vis.autoKey) setChoice ("root", vis.keyRoot + 1);
        setChoice ("scale", r - 1);
    });
}

void MidiSkyEditor::showSoundMenu()
{
    closePopover();
    const int current = (int) proc.apvts.getRawParameterValue ("instrument")->load();
    const auto names = instrumentNames();
    PopupMenu m;
    m.addSectionHeader (U8 ("Pianos acústicos"));
    for (int i : { 0, 1, 2, 3 }) m.addItem (1 + i, names[i], true, current == i);
    m.addSectionHeader (U8 ("Pianos eléctricos"));
    for (int i : { 4, 5 }) m.addItem (1 + i, names[i], true, current == i);
    m.addSectionHeader ("Otros teclados");
    for (int i : { 6, 7, 8 }) m.addItem (1 + i, names[i], true, current == i);

    const bool hosted = proc.host != nullptr && proc.host->hasInstrument();
    Array<PluginDescription> mine;
    if (proc.host != nullptr)
    {
        // App sola: tus instrumentos VST3 / AU
        mine = proc.host->instruments();
        m.addSectionHeader ("Tus instrumentos (VST3 / AU)");
        for (int i = 0; i < mine.size() && i < 60; ++i)
        {
            const auto& d = mine.getReference (i);
            const bool cur = hosted && proc.host->description().fileOrIdentifier == d.fileOrIdentifier && proc.host->description().uniqueId == d.uniqueId;
            m.addItem (100 + i, d.name + (d.pluginFormatName == "AudioUnit" ? "  (AU)" : "  (VST3)"), true, cur);
        }
        if (hosted) m.addItem (90, U8 ("Abrir la ventana de ") + proc.host->instrumentName());
        m.addItem (91, mine.isEmpty() ? U8 ("Buscar mis instrumentos…") : U8 ("Buscar o administrar instrumentos…"));
    }
    m.showMenuAsync (menuOptions (keyboard.soundBtn, 260), [this, mine] (int r)
    {
        if (r <= 0) return;
        if (r < 90)
        {
            if (proc.host != nullptr) proc.host->unload();
            setChoice ("instrument", r - 1);
        }
        else if (r == 90) openHostedWindow();
        else if (r == 91) showPluginsPanel (keyboard.soundBtn);
        else if (r >= 100 && r - 100 < mine.size() && proc.host != nullptr)
        {
            const auto d = mine[r - 100];
            Component::SafePointer<MidiSkyEditor> safe (this);
            proc.host->load (d, [safe] (const String& err)
            {
                if (auto* ed = safe.getComponent())
                {
                    if (err.isNotEmpty())
                        ed->showPopover ("error", "No se pudo abrir", std::make_unique<TextPanel> (ed->winCtx, err, 420), ed->keyboard.soundBtn);
                    else
                        ed->openHostedWindow();
                }
            });
        }
    });
}

void MidiSkyEditor::showWindowsMenu()
{
    closePopover();
    PopupMenu m;
    m.addSectionHeader ("Separar en su propia ventana");
    int i = 1;
    for (auto* s : sections())
        m.addItem (i++, s->title, true, s->floating);
    m.addSeparator();
    m.addItem (50, "Separar todo");
    m.addItem (51, U8 ("Restablecer: todo a su lugar"), anyFloating() || prefs->transparent);
    m.addItem (52, U8 ("Volver a acomodar las ventanas separadas"), anyFloating());

    m.addSectionHeader ("Mis acomodos");
    for (int i = 0; i < (int) prefs->layouts.size(); ++i)
        m.addItem (200 + i, prefs->layouts[(size_t) i].name);
    if (prefs->layouts.empty()) m.addItem (-1, U8 ("(todavía no guardas ninguno)"), false);
    m.addItem (60, U8 ("Guardar este acomodo…"));
    if (! prefs->layouts.empty())
    {
        PopupMenu del;
        for (int i = 0; i < (int) prefs->layouts.size(); ++i)
            del.addItem (300 + i, prefs->layouts[(size_t) i].name);
        m.addSubMenu ("Borrar un acomodo", del);
    }
    m.showMenuAsync (menuOptions (rail.windowsBtn, 280), [this] (int r)
    {
        if (r == 60)
        {
            auto panel = std::make_unique<NamePanel> (winCtx, U8 ("Nombre del acomodo (guarda las ventanas separadas, invisibles y su lugar)"),
                                                      "Acomodo " + String ((int) prefs->layouts.size() + 1));
            panel->onSave = [this] (const String& name) { saveLayoutPreset (name); MessageManager::callAsync ([this] { closePopover(); }); };
            showPopover ("acomodo", "Guardar acomodo", std::move (panel), rail.windowsBtn);
            return;
        }
        if (r >= 200 && r < 200 + (int) prefs->layouts.size()) { const auto l = prefs->layouts[(size_t) (r - 200)]; applyLayoutPreset (l); return; }
        if (r >= 300 && r < 300 + (int) prefs->layouts.size()) { prefs->layouts.erase (prefs->layouts.begin() + (r - 300)); prefs->save (false); return; }
        auto secs = sections();
        if (r >= 1 && r <= (int) secs.size())
        {
            auto* s = secs[(size_t) (r - 1)];
            if (s->floating) dock (*s); else detach (*s);
        }
        else if (r == 50) { for (auto* s : secs) if (! s->floating) detach (*s); }
        else if (r == 51) resetLayout();
        else if (r == 52)
        {
            for (auto* s : secs)
            {
                prefs->floats[s->id].bounds = {};
                if (s->floating)
                    if (auto* w = windows[s->id].get()) { w->setCollapsed (false); w->setBounds (defaultFloatBounds (*s)); }
            }
            prefs->save (false);
        }
    });
}

void MidiSkyEditor::showMainMenu()
{
    closePopover();
    PopupMenu dn;
    dn.addItem (30, U8 ("Día"), true, prefs->dayNight == 0);
    dn.addItem (31, "Noche", true, prefs->dayNight == 1);
    dn.addItem (32, U8 ("Automático (según la hora)"), true, prefs->dayNight == 2);

    PopupMenu chordsM;
    chordsM.addItem (12, U8 ("Tríadas"), true, ! prefs->sevenths);
    chordsM.addItem (13, U8 ("Con séptima"), true, prefs->sevenths);

    PopupMenu size;
    for (int pct : { 75, 100, 125, 150, 200 })
        size.addItem (300 + pct, String (pct) + " %");

    PopupMenu m;
    m.addItem (20, U8 ("Personalizar (estilo, temas y movimiento)…"));
    m.addSubMenu (U8 ("Modo día / noche"), dn);
    m.addSeparator();
    m.addItem (22, U8 ("Metrónomo…"));
    m.addItem (23, U8 ("Grabaciones MIDI…"));
    m.addItem (25, U8 ("Nuevo MIDI en el piano roll…"));
    m.addItem (27, U8 ("Grados: teclas y pads…"));
    m.addItem (28, U8 ("Atajos de teclado (metrónomo y grabar)…"));
    if (proc.host != nullptr) m.addItem (24, U8 ("Tus instrumentos VST3 / AU…"));
    m.addSeparator();
    m.addSubMenu ("Acordes de la tonalidad", chordsM);
    m.addSubMenu (U8 ("Tamaño de la ventana"), size);
    m.addItem (26, U8 ("Latencia…"));
    m.addItem (7, U8 ("Reiniciar detección automática"));
    m.addItem (15, U8 ("Restablecer diseño (todo a su lugar)"));
    m.addSeparator();
    if (isStandalone()) m.addItem (8, U8 ("Audio y MIDI…"));
    m.addItem (14, "Acerca de MidiCat");

    m.showMenuAsync (menuOptions (rail.menuBtn, 260), [this] (int r)
    {
        if (r == 7) { hist.fill (0.0f); autoHas = false; autoRoot = 0; autoMinor = false; lastRoot = -1; }
        else if (r == 8)
        {
           #if JucePlugin_Build_Standalone
            if (auto* h = StandalonePluginHolder::getInstance()) h->showAudioSettingsDialog();
           #endif
        }
        else if (r == 10 || r == 11) { prefs->latin = (r == 11); prefs->save(); }
        else if (r == 12 || r == 13) { prefs->sevenths = (r == 13); prefs->save(); }
        else if (r == 14) showAbout();
        else if (r == 15) resetLayout();
        else if (r == 20) showInterfacePanel (rail.colourBtn);
        else if (r == 21) showPiecePanel (PiecePanel::keysPiece);
        else if (r == 22) showMetronomePanel (rail.metroBtn);
        else if (r == 23) showRecordingsPanel (rail.recBtn);
        else if (r == 24) showPluginsPanel (keyboard.soundBtn);
        else if (r == 25) newMidiFile();
        else if (r == 26) showLatencyPanel();
        else if (r == 27) showDegreeInputPanel();
        else if (r == 28) showShortcutsPanel();
        else if (r >= 30 && r <= 32) setDayNight (r - 30);
        else if (r >= 40 && r < 40 + (int) looks().size()) { prefs->look = looks()[(size_t) (r - 40)].id; prefs->save(); }
        else if (r >= 60 && r < 64) { prefs->keyStyle = r - 60; prefs->save(); }
        else if (r > 300)
        {
            const float s = (float) (r - 300) / 100.0f;
            setSize (roundToInt (kRefW * s), roundToInt (naturalHeight() * s));
        }
    });
}

void MidiSkyEditor::showLatencyPanel()
{
    auto* lat = proc.apvts.getParameter ("latency");
    if (lat == nullptr) return;

    std::function<String()> info;
    std::function<void (int)> setBuf;
   #if JucePlugin_Build_Standalone
    if (isStandalone())
    {
        info = []
        {
            if (auto* h = StandalonePluginHolder::getInstance())
                if (auto* dev = h->deviceManager.getCurrentAudioDevice())
                {
                    const double sr = dev->getCurrentSampleRate();
                    const int total = dev->getOutputLatencyInSamples() + dev->getCurrentBufferSizeSamples();
                    return "Ahora: " + String (dev->getCurrentBufferSizeSamples()) + U8 (" muestras, salida aprox. ")
                           + String (sr > 0 ? 1000.0 * total / sr : 0.0, 1) + " ms";
                }
            return String();
        };
        setBuf = [] (int bs)
        {
            if (auto* h = StandalonePluginHolder::getInstance())
            {
                auto setup = h->deviceManager.getAudioDeviceSetup();
                setup.bufferSize = bs;
                h->deviceManager.setAudioDeviceSetup (setup, true);
            }
        };
    }
   #endif
    showPopover ("latencia", "Compensar latencia", std::make_unique<LatencyPanel> (winCtx, *lat, isStandalone(), info, setBuf), rail.menuBtn);
}

void MidiSkyEditor::showAbout()
{
    const auto text = U8 ("MidiCat 1.5: visualizador de escalas, acordes, notas y pentagrama, con pianos, metrónomo, grabadora MIDI y piano roll.\n\n"
                          "Piano de cola: Salamander Grand Piano de Alexander Holm (CC BY 3.0).\n"
                          "Piano de concierto: Splendid Grand Piano, muestras de dominio público de Akai.\n"
                          "Piano eléctrico: jRhodes3d de Jeff Learman (CC BY 4.0), github.com/sfzinstruments/jlearman.jRhodes3d\n"
                          "Notación: basada en la fuente Bravura de Steinberg (SIL OFL).\n"
                          "Tipografías: Zen Kaku Gothic New, Jersey 10, Doto, VT323, Orbitron, Monoton, Press Start 2P y Sixtyfour (SIL OFL).\n"
                          "Hecho con JUCE (AGPLv3).");
    showPopover ("acerca", "Acerca de MidiCat", std::make_unique<TextPanel> (winCtx, text, 520), rail.menuBtn);
}

//==============================================================================
// Metrónomo, grabaciones, instrumentos y acomodos
//==============================================================================
void MidiSkyEditor::showMetronomePanel (Component& anchor)
{
    showPopover ("metro", U8 ("Metrónomo"),
                 std::make_unique<MetronomePanel> (winCtx, *prefs, proc.metroSettings, proc.metronome, ! isStandalone(), [this] { proc.applyPrefs(); }),
                 anchor);
}

void MidiSkyEditor::showRecordingsPanel (Component& anchor)
{
    auto panel = std::make_unique<RecordingsPanel> (winCtx, *prefs, proc.recorder, proc.metroSettings, [this] { proc.applyPrefs(); });
    panel->onPlay = [this] (const File& f) { clipPlayer.play (f); };
    panel->onStopPlay = [this] { clipPlayer.stop(); };
    panel->isPlaying = [this] { return clipPlayer.isPlaying(); };
    panel->onEdit = [this] (const File& f) { openPianoRoll (f); };
    panel->onNew = [this] { newMidiFile(); };
    showPopover ("grabaciones", "Grabaciones MIDI", std::move (panel), anchor);
}

void MidiSkyEditor::showPluginsPanel (Component& anchor)
{
    if (proc.host == nullptr) return;
    auto panel = std::make_unique<PluginsPanel> (winCtx, *proc.host,
                                                 [this] { openHostedWindow(); },
                                                 [this] { if (proc.host != nullptr) proc.host->unload(); });
    auto* raw = panel.get();
    Component::SafePointer<Component> safePanel (raw);
    proc.host->onListChanged = [safePanel]
    {
        if (auto* p = dynamic_cast<PluginsPanel*> (safePanel.getComponent())) p->refresh();
    };
    showPopover ("instrumentos", "Tus instrumentos VST3 / AU", std::move (panel), anchor);
}

void MidiSkyEditor::openPianoRoll (File f)
{
    clipPlayer.stop();
    if (rollWindow == nullptr)
    {
        rollWindow = std::make_unique<PianoRollWindow> (winCtx, proc.uiKeyboard, look);
        auto* metro = &proc.metronome;
        rollWindow->roll().onMetroClick = [metro] (bool accent) { metro->clickNow (accent); };
        auto* ms = &proc.metroSettings;
        rollWindow->roll().onStopMetronome = [ms]
        {
            if (! ms->on.load()) return false;
            ms->on.store (false);
            return true;
        };
        Component::SafePointer<MidiSkyEditor> safeEd (this);
        rollWindow->roll().shortcutFor = [safeEd] (const KeyPress& k) { return safeEd != nullptr ? safeEd->prefs->shortcutFor (k) : -1; };
        rollWindow->roll().onRecordKey = [safeEd] { if (auto* ed = safeEd.getComponent()) ed->proc.recorder.toggle(); };
        Component::SafePointer<MidiSkyEditor> safe (this);
        rollWindow->onClose = [safe]
        {
            MessageManager::callAsync ([safe] { if (auto* e = safe.getComponent()) e->rollWindow.reset(); });
        };
    }
    closePopover();
    rollWindow->open (f);
}

void MidiSkyEditor::newMidiFile()
{
    // Un MIDI vacío de 4 compases, con el tempo del metrónomo
    auto dir = MidiRecorder::folder();
    dir.createDirectory();
    auto file = dir.getNonexistentChildFile ("MidiCat " + Time::getCurrentTime().formatted ("%Y-%m-%d %H.%M.%S"), ".mid", false);
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::tempoMetaEvent ((int) std::llround (60000000.0 / jmax (20.0f, prefs->metroBpm))), 0.0);
    seq.addEvent (MidiMessage::timeSignatureMetaEvent (prefs->metroBeats, 4), 0.0);
    MidiRecorder::addKeyInfo (seq, vis.keyRoot, vis.scale);
    MidiFile mf;
    mf.setTicksPerQuarterNote (960);
    mf.addTrack (seq);
    {
        FileOutputStream os (file);
        if (! os.openedOk()) return;
        mf.writeTo (os, 1);
    }
    openPianoRoll (file);
}

void MidiSkyEditor::openHostedWindow()
{
    if (proc.host == nullptr || ! proc.host->hasInstrument()) return;
    if (hostedWindow != nullptr) { hostedWindow->setVisible (true); hostedWindow->toFront (true); return; }
    auto* inst = proc.host->instance();
    if (inst == nullptr) return;
    Component* ed = inst->hasEditor() ? inst->createEditorIfNeeded() : nullptr;
    if (ed == nullptr) ed = new GenericAudioProcessorEditor (*inst);
    closePopover();
    hostedWindow = std::make_unique<HostedWindow> (proc.host->instrumentName(), ed, vis.pal.body2);
    Component::SafePointer<MidiSkyEditor> safe (this);
    hostedWindow->onClose = [safe]
    {
        MessageManager::callAsync ([safe] { if (auto* e = safe.getComponent()) e->hostedWindow.reset(); });
    };
}

void MidiSkyEditor::saveLayoutPreset (const String& name)
{
    const auto l = currentLayout (name);
    bool replaced = false;
    for (auto& e : prefs->layouts) if (e.name == name) { e = l; replaced = true; }
    if (! replaced) prefs->layouts.push_back (l);
    prefs->save (false);
}

void MidiSkyEditor::applyLayoutPreset (const LayoutPreset& l)
{
    closePopover();
    for (auto* s : sections())
        if (s->floating) dock (*s, false);
    prefs->floats = l.floats;
    prefs->transparent = l.transparent;
    prefs->hideParams = l.hideParams;
    vis.transparent = l.transparent;
    vis.hideParams = l.hideParams;
    prefs->save (false);
    rail.transpBtn.setActive (l.transparent);
    rail.hideBtn.setIcon (l.hideParams ? "eye" : "eyeoff");
    for (auto* s : sections())
    {
        auto it = prefs->floats.find (s->id);
        if (it != prefs->floats.end() && it->second.floating) detach (*s, false);
    }
    for (auto* s : sections()) s->refreshControls();
    rail.resized();
    if (l.editorW > 100 && l.editorH > 100) setSize (l.editorW, l.editorH);
    resized();
    repaint();
}

//==============================================================================
void ClipPlayer::play (const File& f)
{
    stop();
    MidiClip clip;
    if (! clip.load (f)) return;
    events.clear();
    const double secPerBeat = 60.0 / jmax (1.0, clip.bpm);
    for (auto& n : clip.notes)
    {
        events.push_back ({ n.start * secPerBeat, n.note, (float) n.velocity / 127.0f, true });
        events.push_back ({ jmax (n.start + 0.01, n.end) * secPerBeat - 0.001, n.note, 0.0f, false });
    }
    std::sort (events.begin(), events.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && ! a.on && b.on); });
    next = 0;
    held.fill (0);
    startMs = Time::getMillisecondCounterHiRes() + 60.0;
    startTimer (2);
}

void ClipPlayer::stop()
{
    stopTimer();
    for (int n = 0; n < 128; ++n)
        while (held[(size_t) n] > 0) { state.noteOff (1, n, 0.0f); --held[(size_t) n]; }
}

void ClipPlayer::hiResTimerCallback()
{
    const double t = (Time::getMillisecondCounterHiRes() - startMs) / 1000.0;
    while (next < events.size() && events[next].t <= t)
    {
        const auto& e = events[next++];
        if (e.on) { state.noteOn (1, e.note, jmax (0.05f, e.vel)); ++held[(size_t) e.note]; }
        else if (held[(size_t) e.note] > 0) { state.noteOff (1, e.note, 0.0f); --held[(size_t) e.note]; }
    }
    if (next >= events.size()) stopTimer();
}

HostedWindow::HostedWindow (const String& name, Component* editor, Colour bg)
    : DocumentWindow (name, bg, DocumentWindow::closeButton | DocumentWindow::minimiseButton)
{
    setUsingNativeTitleBar (true);
    setContentOwned (editor, true);
    if (auto* ape = dynamic_cast<AudioProcessorEditor*> (editor))
        setResizable (ape->isResizable(), false);
    setAlwaysOnTop (false);
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
    toFront (true);
}

//==============================================================================
void MidiSkyEditor::previewChord (int index, float velocity)
{
    if (index < 0 || index >= (int) vis.diatonic.size()) return;
    stopPreview();
    previewNotes = proc.degreeChordNotes (index);
    for (auto n : previewNotes) proc.uiKeyboard.noteOn (1, n, velocity);
    previewStartedAt = Time::getMillisecondCounterHiRes();
    previewOffAt = previewStartedAt + 30000.0;   // se apaga al soltar el mouse
}

//==============================================================================
// Grados con las teclas de la compu (lo que tocas se oye y se graba; en la interfaz no se ven las teclas)
//==============================================================================
static int normalisedKey (const KeyPress& k)
{
    int code = k.getKeyCode();
    if (code >= 'A' && code <= 'Z') code += 'a' - 'A';
    return code;
}

bool MidiSkyEditor::keyPressed (const KeyPress& key)
{
    if (DegreeInputPanel::capturing) return false;
    if (runShortcut (prefs->shortcutFor (key))) return true;
    if (! prefs->degreeKeysOn) return false;
    const auto mods = key.getModifiers();
    if (mods.isCommandDown() || mods.isCtrlDown() || mods.isAltDown()) return false;
    const int code = normalisedKey (key);
    for (int i = 0; i < 7; ++i)
        if (code != 0 && prefs->degreeKeys[(size_t) i] == code)
        {
            if (! degreeDown[(size_t) i]) startDegree (i);
            return true;
        }
    return false;
}

bool MidiSkyEditor::runShortcut (int id)
{
    // Atajos de teclado (los eliges en ☰ > Atajos de teclado)
    if (id == Prefs::scMetro)
    {
        const bool on = ! proc.metroSettings.on.load();
        proc.metroSettings.on.store (on);
        rail.metroBtn.setActive (on);
        return true;
    }
    if (id == Prefs::scMetroStop)
    {
        if (! proc.metroSettings.on.load()) return false;   // si no suena, la tecla sigue libre
        proc.metroSettings.on.store (false);
        rail.metroBtn.setActive (false);
        return true;
    }
    if (id == Prefs::scRecord)
    {
        proc.recorder.toggle();
        return true;
    }
    return false;
}

void MidiSkyEditor::showShortcutsPanel()
{
    Component::SafePointer<MidiSkyEditor> safe (this);
    showPopover ("atajos", "Atajos de teclado", std::make_unique<ShortcutsPanel> (winCtx, *prefs, [safe]
    {
        MessageManager::callAsync ([safe] { if (auto* ed = safe.getComponent()) ed->showDegreeInputPanel(); });
    }), rail.menuBtn);
}

bool MidiSkyEditor::keyStateChanged (bool)
{
    bool used = false;
    for (int i = 0; i < 7; ++i)
        if (degreeDown[(size_t) i] && ! KeyPress::isKeyCurrentlyDown (prefs->degreeKeys[(size_t) i]))
        {
            stopDegree (i);
            used = true;
        }
    return used;
}

void MidiSkyEditor::startDegree (int i)
{
    degreeDown[(size_t) i] = true;
    heldDegree[(size_t) i] = proc.degreeChordNotes (i);
    for (auto n : heldDegree[(size_t) i])
        if (degreeRefs[(size_t) n]++ == 0)
            proc.degreeKeyboard.noteOn (1, n, 0.78f);
}

void MidiSkyEditor::stopDegree (int i)
{
    degreeDown[(size_t) i] = false;
    for (auto n : heldDegree[(size_t) i])
        if (degreeRefs[(size_t) n] > 0 && --degreeRefs[(size_t) n] == 0)
            proc.degreeKeyboard.noteOff (1, n, 0.0f);
    heldDegree[(size_t) i].clear();
}

void MidiSkyEditor::showDegreeInputPanel()
{
    Component::SafePointer<MidiSkyEditor> safe (this);
    auto lastPad = [safe]() -> int { return safe != nullptr ? safe->proc.lastNoteEvent.load() : 0; };
    showPopover ("gradosTeclas", "Teclas y pads para los grados", std::make_unique<DegreeInputPanel> (winCtx, *prefs, lastPad), degrees);
}

void MidiSkyEditor::stopPreview()
{
    for (auto n : previewNotes) proc.uiKeyboard.noteOff (1, n, 0.0f);
    previewNotes.clear();
    previewOffAt = 0.0;
}
