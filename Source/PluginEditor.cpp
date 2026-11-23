#include "PluginEditor.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

using namespace juce;

//==============================================================================
void MidiSkyLook::setPalette (const Palette& p)
{
    pal = p;
    const auto menuBg = p.dark ? p.bodyColour2.interpolatedWith (Colours::black, 0.3f) : Colour (0xfff6f4f0);
    const auto menuText = p.dark ? Colour (0xffefe9df) : Colour (0xff1e1a16);
    setColour (PopupMenu::backgroundColourId, menuBg);
    setColour (PopupMenu::textColourId, menuText);
    setColour (PopupMenu::headerTextColourId, menuText.withAlpha (0.6f));
    setColour (PopupMenu::highlightedBackgroundColourId, p.accent.withAlpha (0.35f));
    setColour (PopupMenu::highlightedTextColourId, menuText);
    setColour (ResizableWindow::backgroundColourId, menuBg);
    setColour (ColourSelector::backgroundColourId, Colours::transparentBlack);
    setColour (ColourSelector::labelTextColourId, menuText);
    setColour (Slider::textBoxTextColourId, menuText);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Label::textColourId, menuText);
    setColour (AlertWindow::backgroundColourId, menuBg);
    setColour (AlertWindow::textColourId, menuText);
    setColour (TextButton::buttonColourId, p.accent.withAlpha (0.25f));
    setColour (TextButton::textColourOffId, menuText);
    setColour (TooltipWindow::backgroundColourId, menuBg);
    setColour (TooltipWindow::textColourId, menuText);
}

void MidiSkyLook::drawCallOutBoxBackground (CallOutBox&, Graphics& g, const Path& path, Image&)
{
    g.setColour (Colours::black.withAlpha (0.3f));
    g.fillPath (path, AffineTransform::translation (0.0f, 3.0f));
    g.setColour (findColour (PopupMenu::backgroundColourId));
    g.fillPath (path);
    g.setColour (pal.accent.withAlpha (0.5f));
    g.strokePath (path, PathStrokeType (1.0f));
}

void MidiSkyLook::drawCornerResizer (Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    // Tres puntitos en diagonal, metidos para respetar la curva del marco
    const auto col = (pal.dark ? Colours::white : Colours::black).withAlpha (isMouseOver || isMouseDragging ? 0.55f : 0.28f);
    g.setColour (col);
    const float s = (float) jmin (w, h);
    const float d = jmax (1.6f, s * 0.11f);
    const Point<float> base ((float) w - s * 0.42f, (float) h - s * 0.42f);
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

MidiSkyEditor::MidiSkyEditor (MidiSkyProcessor& p)
    : AudioProcessorEditor (p), proc (p), look (*fonts)
{
    setOpaque (true);
    setLookAndFeel (&look);

    addAndMakeVisible (rail);
    for (auto* s : sections()) addAndMakeVisible (s);
    addChildComponent (toast);

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
    rail.themeBtn.onClick   = [this] { showThemePicker(); };
    rail.dayBtn.onClick     = [this] { setDayNight (prefs->isNight() ? 0 : 1); };
    {
        Component::SafePointer<MidiSkyEditor> safeEditor (this);
        rail.resetBtn.onClick = [safeEditor]
        {
            MessageManager::callAsync ([safeEditor] { if (auto* ed = safeEditor.getComponent()) ed->resetLayout(); });
        };
    }
    rail.skinBtn.onClick    = [this] { showSkinPicker(); };
    rail.transpBtn.onClick  = [this] { setTransparent (! prefs->transparent); };
    rail.splitBtn.onClick   = [this] { if (keyboard.floating) dock (keyboard); else detach (keyboard); };
    rail.windowsBtn.onClick = [this] { showWindowsMenu(); };
    rail.hideBtn.onClick    = [this] { setHideParams (! prefs->hideParams); };
    rail.menuBtn.onClick    = [this] { showMainMenu(); };

    for (auto* s : sections())
        s->onDetach = [this, s] { detach (*s); };

    // --- Teclado y sus controles
    keyboard.pianoSw.onChange = [this] (bool v) { setBool ("piano", v); };
    keyboard.scaleSw.onChange = [this] (bool v) { setBool ("highlight", v); };
    keyboard.names.onChange   = [this] (int i) { prefs->latin = (i == 1); prefs->save(); };
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

    // --- Círculo y tablillas
    circle.onKeyClicked = [this] (int pc, bool minor)
    {
        setChoice ("root", pc + 1);
        setChoice ("scale", minor ? 1 : 0);
    };
    tablets.onPreview = [this] (int i) { previewChord (i); };

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
    }
   #endif

    refreshPalette();

    const int startW = jlimit (480, 3200, proc.editorW.load());
    const int startH = jlimit (150, 2200, proc.editorH.load());
    setResizable (true, true);
    setResizeLimits (480, 150, 3200, 2200);
    setSize (startW, startH);

    applyLowLatencyOnce();

    lastFrame = Time::getMillisecondCounterHiRes();
    frame();
    vblank = std::make_unique<VBlankAttachment> (this, [this] { frame(); });
    startTimerHz (12);
}

MidiSkyEditor::~MidiSkyEditor()
{
    stopTimer();
    vblank.reset();
    stopPreview();
    prefs->removeChangeListener (this);
    if (colourSelector != nullptr) colourSelector->removeChangeListener (this);
    for (auto& [id, w] : windows)
        if (w != nullptr) w->releaseSection();
    windows.clear();
    setLookAndFeel (nullptr);
}

bool MidiSkyEditor::isStandalone() const
{
    return proc.wrapperType == AudioProcessor::wrapperType_Standalone;
}

std::vector<Section*> MidiSkyEditor::sections()
{
    return { &circle, &score, &tablets, &chord, &keyboard };
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

//==============================================================================
void MidiSkyEditor::refreshPalette()
{
    vis.pal = prefs->palette();
    vis.latin = prefs->latin;
    vis.sevenths = prefs->sevenths;
    look.setPalette (vis.pal);
    keyboard.names.setSelected (vis.latin ? 1 : 0);
    rail.transpBtn.setActive (prefs->transparent);
    rail.splitBtn.setActive (keyboard.floating);
    rail.hideBtn.setIcon (prefs->hideParams ? "eye" : "eyeoff");
    rail.dayBtn.setIcon (vis.pal.night ? "moon" : "sun");
    rail.dayBtn.setActive (prefs->dayNight == 2);
    lastNight = vis.pal.night;
    rebuildBackground();
    lastRoot = -1;
    repaint();
    for (auto& [id, w] : windows)
        if (w != nullptr) w->repaint();
}

void MidiSkyEditor::changeListenerCallback (ChangeBroadcaster* src)
{
    if (colourSelector != nullptr && src == colourSelector.getComponent())
    {
        prefs->lightCustom = colourSelector->getCurrentColour().withAlpha (1.0f);
        prefs->lightMode = 1;
        prefs->save();
        return;
    }

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

    const auto& pal = vis.pal;
    draw::chassis (g, getLocalBounds().toFloat(), pal, k, *textures, true);

    if (! atrilBounds.isEmpty())
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (draw::chassisPath (getLocalBounds().toFloat().reduced (0.5f), pal, k));
        auto a = atrilBounds.toFloat();
        draw::panel (g, a, pal.atril, pal.railBorder, pal, k);
        const bool lip = ! pal.modern && pal.id != "minimalista";
        if (lip)
        {
            auto l = a.removeFromBottom (14.0f * k);
            g.setColour (pal.lip);
            g.fillPath (draw::panelPath (l, pal, k, 0.5f));
            g.setColour (Colours::white.withAlpha (0.08f));
            g.fillRect (l.withHeight (1.0f));
        }
    }
}

void MidiSkyEditor::paint (Graphics& g)
{
    if (background.isValid()) g.drawImage (background, getLocalBounds().toFloat());
    else                      g.fillAll (Colour (0xff0b0b0d));
}

void MidiSkyEditor::paintOverChildren (Graphics& g)
{
    if (vis.pal.look().isEmpty()) return;
    Graphics::ScopedSaveState ss (g);
    // La luz del tema no va encima de los menús ni de los avisos
    for (auto* c : getChildren())
        if (c->isVisible() && (dynamic_cast<CallOutBox*> (c) != nullptr || c == &toast))
            g.excludeClipRegion (c->getBounds());
    g.reduceClipRegion (draw::chassisPath (getLocalBounds().toFloat().reduced (0.5f), vis.pal, k));
    themeCache.draw (g, getLocalBounds().toFloat(), vis.pal, jlimit (1.0f, 2.0f, Component::getApproximateScaleFactorForComponent (this)));
}

// Medidas a escala 1 (ventana normal de 1200 x 760)
static constexpr float kMargin = 18.0f, kGap = 10.0f, kRailH = 58.0f, kRailHidden = 50.0f,
                       kAtrilH = 328.0f, kChordH = 118.0f, kKeysH = 190.0f;

float MidiSkyEditor::naturalHeight() const
{
    float h = kMargin * 2.0f + (vis.hideParams ? kRailHidden : kRailH);
    if (! circle.floating || ! score.floating || ! tablets.floating) h += kGap + kAtrilH;
    if (! chord.floating) h += kGap + kChordH;
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

    k = jlimit (0.45f, 2.6f, jmin ((float) getWidth() / 1200.0f, (float) getHeight() / naturalHeight()));
    const int m = roundToInt (kMargin * k);
    const int gap = roundToInt (kGap * k);

    auto b = getLocalBounds().reduced (m);
    rail.k = k;
    rail.setBounds (b.removeFromTop (roundToInt ((vis.hideParams ? kRailHidden : kRailH) * k)));
    rail.resized();
    toast.setBounds (getLocalBounds().withY (rail.getBottom() + gap / 2).withHeight (roundToInt (34.0f * jmax (0.8f, k))).reduced (m * 2, 0));
    b.removeFromTop (gap);

    std::vector<Section*> atrilItems;
    for (auto* s : std::initializer_list<Section*> { &circle, &score, &tablets })
        if (! s->floating) atrilItems.push_back (s);
    const bool hasAtril = ! atrilItems.empty();

    for (auto* s : sections())
        if (! s->floating) s->k = k;

    if (! keyboard.floating)
    {
        const int kh = (! hasAtril && chord.floating) ? b.getHeight() : roundToInt (kKeysH * k);
        keyboard.setBounds (b.removeFromBottom (jmin (kh, b.getHeight())));
        b.removeFromBottom (gap);
    }

    if (! chord.floating)
    {
        const int ch = hasAtril ? roundToInt (kChordH * k) : b.getHeight();
        chord.setBounds (b.removeFromBottom (jmax (0, jmin (ch, b.getHeight()))));
        b.removeFromBottom (gap);
    }

    if (hasAtril && b.getHeight() > 40)
    {
        atrilBounds = b;
        auto inner = b.reduced (roundToInt (14.0f * k), roundToInt (10.0f * k)).withTrimmedBottom (roundToInt (14.0f * k));
        const int side = inner.getHeight();
        if (! circle.floating)
        {
            circle.setBounds (inner.removeFromLeft (jmin (side, inner.getWidth() / ((int) atrilItems.size()))));
            inner.removeFromLeft (gap * 2);
        }
        if (! tablets.floating)
        {
            tablets.setBounds (inner.removeFromRight (jmin (roundToInt (290.0f * k), inner.getWidth() / (score.floating ? 1 : 2))));
            inner.removeFromRight (gap * 2);
        }
        if (! score.floating)
            score.setBounds (inner);
    }
    else
    {
        atrilBounds = {};
        for (auto* s : atrilItems) s->setBounds ({});
    }

    for (auto* s : sections())
        if (! s->floating) s->resized();

    updateResetButton();
    rebuildBackground();
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
    else if (s.id == "acorde")  r = { origin.x + 40, origin.y + getHeight() - 60, w, h };
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
    const float keepK = k;

    auto& st = prefs->floats[s.id];
    auto bounds = st.bounds;
    if (bounds.isEmpty() || Desktop::getInstance().getDisplays().getDisplayForRect (bounds) == nullptr)
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
    w->shell().onReset = [this, id]
    {
        if (auto* sec = sectionById (id))
            if (auto* win = windows[id].get())
            {
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
    windows[id] = std::move (w);

    st.floating = true;
    s.refreshControls();
    rail.splitBtn.setActive (keyboard.floating);
    if (save)
    {
        prefs->save (false);
        toast.show (s.title + U8 (" está en su propia ventana. Para regresar todo, usa Restablecer."));
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
        if (it->second != nullptr) it->second->releaseSection();
        windows.erase (it);
    }
    s.floating = false;
    addAndMakeVisible (s);
    toast.toFront (false);
    prefs->floats[s.id].floating = false;
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
    const float keepK = k;
    for (auto* s : sections())
        if (s->floating) dock (*s, false);
    for (auto& [id, f] : prefs->floats) { f.floating = false; f.bounds = {}; f.onTop = true; }
    prefs->transparent = false;
    prefs->hideParams = false;
    vis.transparent = false;
    vis.hideParams = false;
    prefs->save (false);

    rail.transpBtn.setActive (false);
    rail.hideBtn.setIcon ("eyeoff");
    for (auto* s : sections()) s->refreshControls();
    fitHeightToContent (keepK);
    toast.show (U8 ("Listo: todo regresó a su lugar"));
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
    if (any) fitHeightToContent (jlimit (0.45f, 2.6f, (float) getWidth() / 1200.0f));
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

    if (on)
    {
        if (! anyFloating()) { detach (keyboard, true); detach (chord, true); }
        toast.show (U8 ("Modo transparente: las ventanas separadas no tienen fondo. Ponlas sobre tu video."), 6.0);
    }
    else
    {
        toast.show (U8 ("Modo transparente apagado"));
    }
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
    toast.show (mode == 0 ? U8 ("Modo día") : (mode == 1 ? U8 ("Modo noche") : U8 ("Modo automático: de día de 7 a 19 h, de noche el resto")));
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
        rail.key.setValueText (theory::pcName (vis.keyRoot, vis.sig, vis.latin) + (vis.autoKey ? U8 (" (auto)") : String()));
        rail.scale.setValueText (U8 (theory::scales()[(size_t) vis.scale].name));
        lastRoot = vis.keyRoot; lastScale = vis.scale; lastLatin = vis.latin;
        lastSevenths = vis.sevenths; lastAuto = vis.autoKey; lastAutoHas = vis.autoHasData;
    }
}

void MidiSkyEditor::frame()
{
    const double now = Time::getMillisecondCounterHiRes();
    const float dt = jlimit (0.0f, 0.1f, (float) ((now - lastFrame) / 1000.0));
    lastFrame = now;
    auto smooth = [dt] (float rate) { return 1.0f - std::pow (1.0f - rate, dt * 60.0f); };

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
                    hist[(size_t) (ev.note % 12)] += 0.5f + ev.velocity / 127.0f;
                }
            };
            take (s1, n1);
            take (s2, n2);
            fifo.finishedRead (n1 + n2);
        }
    }

    bool keysAnimating = false;
    const float up = smooth (0.6f), downRate = smooth (0.16f);
    const float flashDecay = std::pow (0.02f, dt);
    for (size_t n = 0; n < 128; ++n)
    {
        const auto d = proc.notes.down[n].load();
        const auto s = proc.notes.sustained[n].load();
        if (d != vis.down[n] || s != vis.sus[n]) keysAnimating = true;
        vis.down[n] = d;
        vis.sus[n] = s;
        if (d > 0) hist[n % 12] += dt * 1.2f;

        const float target = d > 0 ? 0.62f + 0.38f * (float) d / 127.0f : (s > 0 ? 0.32f + 0.2f * (float) s / 127.0f : 0.0f);
        const float boosted = jmin (1.0f, target + vis.flash[n] * 0.25f);
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
    const float histDecay = std::exp (-dt / 30.0f);
    for (auto& h : hist) h *= histDecay;

    const bool pedal = proc.notes.pedal.load();
    vis.active.clear();
    vis.activeMask = 0;
    for (int n = 0; n < 128; ++n)
        if (vis.down[(size_t) n] > 0 || vis.sus[(size_t) n] > 0)
        {
            vis.active.push_back (n);
            vis.activeMask = (uint16_t) (vis.activeMask | (1u << (unsigned) (n % 12)));
        }
    vis.lowest = vis.active.empty() ? -1 : vis.active.front();

    bool circleAnimating = false;
    for (int pc = 0; pc < 12; ++pc)
    {
        const float target = (vis.activeMask & (1u << (unsigned) pc)) ? 1.0f : 0.0f;
        float& gl = vis.pcGlow[(size_t) pc];
        if (std::abs (target - gl) > 0.004f) { gl += (target - gl) * (target > gl ? smooth (0.45f) : smooth (0.12f)); circleAnimating = true; }
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
        String big;
        if (count == 0) big = theory::keyTitle (vis.keyRoot, vis.scale, vis.latin);
        else if (count == 1) big = theory::noteName (vis.active.front(), vis.sig, vis.latin, true);
        else if (vis.chord.valid && (count >= 3 || vis.chord.suffix == "5")) big = vis.chord.symbol;
        else if (count == 2) big = theory::intervalName (vis.active[1] - vis.active[0]);
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
        lastActive = vis.active;
        score.repaint();
        tablets.repaint();
        chord.repaint();
        circle.repaint();
    }

    if (vis.bigFade < 1.0f)
    {
        vis.bigFade = jmin (1.0f, vis.bigFade + dt / 0.18f);
        chord.repaint();
    }

    if (keysAnimating || keyChanged || vis.highlight != lastHighlight || pedal != vis.pedal)
    {
        keyboard.repaint();
        if (keysAnimating) score.repaint();
    }
    if (circleAnimating || keyChanged) circle.repaint();
    if (keyChanged) { score.repaint(); tablets.repaint(); chord.repaint(); }
    lastHighlight = vis.highlight;
    vis.pedal = pedal;

    if (previewOffAt > 0.0 && now > previewOffAt)
        stopPreview();

    rail.tick (now);
    toast.tick (now);
}

void MidiSkyEditor::timerCallback()
{
    updateMidiStatus();
    keyboard.pianoSw.setOn (proc.apvts.getRawParameterValue ("piano")->load() > 0.5f);
    keyboard.scaleSw.setOn (proc.apvts.getRawParameterValue ("highlight")->load() > 0.5f);
    if (auto* lat = proc.apvts.getParameter ("latency"))
        rail.latency.setValueText (lat->getCurrentValueAsText());

    if (! restoredFloats && isShowing())
    {
        restoredFloats = true;
        restoreFloating();
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

        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&rail.midi).withMinimumWidth (260), [this, devices] (int r)
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
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&rail.midi).withMinimumWidth (260), [this] (int r)
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
    const int current = (int) proc.apvts.getRawParameterValue ("root")->load();
    PopupMenu m;
    m.addItem (1, U8 ("Auto (la detecto mientras tocas)"), true, current == 0);
    m.addSeparator();
    for (int i = 0; i < 12; ++i)
        m.addItem (2 + i, vis.latin ? U8 (rootNamesEs[i]) : U8 (rootNamesEn[i]), true, current == i + 1);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&rail.key).withMinimumWidth (rail.key.getWidth()),
                     [this] (int r) { if (r == 1) setChoice ("root", 0); else if (r >= 2) setChoice ("root", r - 1); });
}

void MidiSkyEditor::showScaleMenu()
{
    PopupMenu m;
    const auto& sc = theory::scales();
    m.addSectionHeader ("Escalas");
    for (int i = 0; i < (int) sc.size(); ++i)
    {
        if (i == 4) m.addSectionHeader ("Modos");
        if (i == 9) m.addSectionHeader (U8 ("Pentatónicas y blues"));
        m.addItem (1 + i, U8 (sc[(size_t) i].name), true, vis.scale == i);
    }
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&rail.scale).withMinimumWidth (rail.scale.getWidth()),
                     [this] (int r)
    {
        if (r <= 0) return;
        if (vis.autoKey) setChoice ("root", vis.keyRoot + 1);
        setChoice ("scale", r - 1);
    });
}

void MidiSkyEditor::showThemePicker()
{
    auto picker = std::make_unique<ThemePicker> (ctx, themeIndex (prefs->theme), prefs->dayNight);
    picker->onPick = [this] (int i)
    {
        prefs->theme = themes()[(size_t) i].id;
        prefs->save();
    };
    picker->onMode = [this] (int m) { setDayNight (m); };
    CallOutBox::launchAsynchronously (std::move (picker), getLocalArea (&rail, rail.themeBtn.getBounds()), this);
}

void MidiSkyEditor::showWindowsMenu()
{
    PopupMenu m;
    m.addSectionHeader ("Separar en su propia ventana");
    int i = 1;
    for (auto* s : sections())
        m.addItem (i++, s->title, true, s->floating);
    m.addSeparator();
    m.addItem (50, "Separar todo");
    m.addItem (51, U8 ("Restablecer: todo a su lugar"), anyFloating() || prefs->transparent);
    m.addItem (52, U8 ("Volver a acomodar las ventanas separadas"), anyFloating());
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&rail.windowsBtn).withMinimumWidth (260), [this] (int r)
    {
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
                    if (auto* w = windows[s->id].get()) w->setBounds (defaultFloatBounds (*s));
            }
            prefs->save (false);
        }
    });
}

void MidiSkyEditor::showMainMenu()
{
    PopupMenu names;
    names.addItem (10, U8 ("C D E (inglés)"), true, ! prefs->latin);
    names.addItem (11, U8 ("Do Re Mi (latino)"), true, prefs->latin);

    PopupMenu chordsM;
    chordsM.addItem (12, U8 ("Tríadas"), true, ! prefs->sevenths);
    chordsM.addItem (13, U8 ("Con séptima"), true, prefs->sevenths);

    PopupMenu light;
    light.addItem (20, "Color de la skin", true, prefs->lightMode == 0);
    light.addItem (21, U8 ("Arcoíris (cada nota un color)"), true, prefs->lightMode == 2);
    light.addItem (22, U8 ("Elegir un color…"), true, prefs->lightMode == 1);

    PopupMenu size;
    for (int pct : { 75, 100, 125, 150, 200 })
        size.addItem (300 + pct, String (pct) + " %");

    PopupMenu dn;
    dn.addItem (30, U8 ("Día"), true, prefs->dayNight == 0);
    dn.addItem (31, "Noche", true, prefs->dayNight == 1);
    dn.addItem (32, U8 ("Automático (según la hora)"), true, prefs->dayNight == 2);

    PopupMenu m;
    m.addSubMenu (U8 ("Modo día / noche"), dn);
    m.addSubMenu ("Nombres de las notas", names);
    m.addSubMenu ("Acordes de la tonalidad", chordsM);
    m.addSubMenu ("Color de las teclas", light);
    m.addSubMenu (U8 ("Tamaño de la ventana"), size);
    m.addItem (7, U8 ("Reiniciar detección automática"));
    m.addItem (15, U8 ("Restablecer diseño (todo a su lugar)"));
    m.addSeparator();
    if (isStandalone()) m.addItem (8, U8 ("Audio y MIDI…"));
    m.addItem (14, "Acerca de MidiSky");

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&rail.menuBtn).withMinimumWidth (240), [this] (int r)
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
        else if (r >= 30 && r <= 32) setDayNight (r - 30);
        else if (r == 20) { prefs->lightMode = 0; prefs->save(); }
        else if (r == 21) { prefs->lightMode = 2; prefs->save(); }
        else if (r == 22) showLightColourPicker();
        else if (r > 300)
        {
            const float s = (float) (r - 300) / 100.0f;
            setSize (roundToInt (1200.0f * s), roundToInt (naturalHeight() * s));
        }
    });
}

void MidiSkyEditor::showSkinPicker()
{
    auto picker = std::make_unique<SkinPicker> (ctx, skinIndex (prefs->skin));
    picker->onPick = [this] (int i)
    {
        prefs->skin = skins()[(size_t) i].id;
        prefs->save();
    };
    CallOutBox::launchAsynchronously (std::move (picker), getLocalArea (&rail, rail.skinBtn.getBounds()), this);
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
    auto panel = std::make_unique<LatencyPanel> (ctx, *lat, isStandalone(), info, setBuf);
    CallOutBox::launchAsynchronously (std::move (panel), getLocalArea (&rail, rail.latency.getBounds()), this);
}

void MidiSkyEditor::showLightColourPicker()
{
    auto cs = std::make_unique<ColourSelector> (ColourSelector::showColourAtTop | ColourSelector::showSliders | ColourSelector::showColourspace, 4, 6);
    cs->setName ("Color de las teclas");
    cs->setCurrentColour (prefs->lightCustom);
    cs->setSize (300, 320);
    cs->addChangeListener (this);
    colourSelector = cs.get();
    CallOutBox::launchAsynchronously (std::move (cs), getLocalArea (&rail, rail.menuBtn.getBounds()), this);
}

void MidiSkyEditor::showAbout()
{
    auto opts = MessageBoxOptions()
        .withIconType (MessageBoxIconType::NoIcon)
        .withTitle ("MidiSky 1.2")
        .withMessage (U8 ("Visualizador de escalas, acordes, notas y pentagrama.\n\n"
                          "Piano: Salamander Grand Piano de Alexander Holm (CC BY 3.0).\n"
                          "Notación: basada en la fuente Bravura de Steinberg (SIL OFL).\n"
                          "Tipografías: Shippori Mincho, Zen Kaku Gothic New, Press Start 2P y Jersey 10 (SIL OFL).\n"
                          "Hecho con JUCE (AGPLv3)."))
        .withButton ("Cerrar")
        .withAssociatedComponent (this);
    AlertWindow::showAsync (opts, nullptr);
}

//==============================================================================
void MidiSkyEditor::previewChord (int index)
{
    if (index < 0 || index >= (int) vis.diatonic.size()) return;
    stopPreview();
    const auto& d = vis.diatonic[(size_t) index];
    int prev = 48 + d.rootPc;
    if (prev < 52) prev += 12;
    previewNotes.push_back (prev - 12);
    previewNotes.push_back (prev);
    for (size_t i = 1; i < d.pcs.size(); ++i)
    {
        int n = prev + ((d.pcs[i] - prev % 12 + 12) % 12);
        if (n == prev) n += 12;
        previewNotes.push_back (n);
        prev = n;
    }
    for (auto n : previewNotes) proc.uiKeyboard.noteOn (1, n, 0.68f);
    previewOffAt = Time::getMillisecondCounterHiRes() + 1100.0;
}

void MidiSkyEditor::stopPreview()
{
    for (auto n : previewNotes) proc.uiKeyboard.noteOff (1, n, 0.0f);
    previewNotes.clear();
    previewOffAt = 0.0;
}
