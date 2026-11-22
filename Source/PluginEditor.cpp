#include "PluginEditor.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

using namespace juce;

//==============================================================================
void MidiSkyLook::setPalette (const Palette& p)
{
    pal = p;
    const auto menuBg = p.dark ? Colour (0xff1d1f23) : Colour (0xfff1f2f4);
    setColour (PopupMenu::backgroundColourId, menuBg);
    setColour (PopupMenu::textColourId, p.text);
    setColour (PopupMenu::headerTextColourId, p.dim);
    setColour (PopupMenu::highlightedBackgroundColourId, p.accent.withAlpha (0.30f));
    setColour (PopupMenu::highlightedTextColourId, p.dark ? Colours::white : p.text);
    setColour (ResizableWindow::backgroundColourId, menuBg);
    setColour (ColourSelector::backgroundColourId, Colours::transparentBlack);
    setColour (ColourSelector::labelTextColourId, p.text);
    setColour (Slider::textBoxTextColourId, p.text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Label::textColourId, p.text);
    setColour (AlertWindow::backgroundColourId, menuBg);
    setColour (AlertWindow::textColourId, p.text);
    setColour (TextButton::buttonColourId, p.plate);
    setColour (TextButton::textColourOffId, p.text);
}

void MidiSkyLook::drawCallOutBoxBackground (CallOutBox&, Graphics& g, const Path& path, Image&)
{
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillPath (path, AffineTransform::translation (0.0f, 3.0f));
    g.setColour (pal.dark ? Colour (0xff1d1f23) : Colour (0xfff1f2f4));
    g.fillPath (path);
    g.setColour (pal.plateEdge);
    g.strokePath (path, PathStrokeType (1.0f));
}

//==============================================================================
static const char* rootNamesEn[12] = { "C", "C#/Db", "D", "Eb", "E", "F", "F#/Gb", "G", "Ab", "A", "Bb", "B" };
static const char* rootNamesEs[12] = { "Do", "Do#/Reb", "Re", "Mib", "Mi", "Fa", "Fa#/Solb", "Sol", "Lab", "La", "Sib", "Si" };

MidiSkyEditor::MidiSkyEditor (MidiSkyProcessor& p)
    : AudioProcessorEditor (p), proc (p), look (*fonts)
{
    setOpaque (true);
    setLookAndFeel (&look);

    addAndMakeVisible (top);
    addAndMakeVisible (staff);
    addAndMakeVisible (screen);
    addAndMakeVisible (circle);
    addAndMakeVisible (strip);
    addAndMakeVisible (keys);

    prefs->addChangeListener (this);

    // --- Barra superior
    top.keyButton.onClick   = [this] { showKeyMenu(); };
    top.scaleButton.onClick = [this] { showScaleMenu(); };
    top.menuButton.onClick  = [this] { showMainMenu(); };
    top.modeButton.onClick  = [this]
    {
        prefs->dark = ! prefs->dark;
        prefs->save();
    };
    top.onMidiClicked = [this]
    {
        if (isStandalone()) showMidiMenu();
        else proc.listen.store (! proc.listen.load());
        updateMidiStatus();
    };

    // --- Tira de controles
    strip.highlight.onChange  = [this] (bool v) { setBool ("highlight", v); };
    strip.pianoSound.onChange = [this] (bool v) { setBool ("piano", v); };
    strip.skinButton.onClick  = [this] { showSkinPicker(); };
    strip.names.onChange      = [this] (int i) { prefs->latin = (i == 1); prefs->save(); };

    if (auto* vol = proc.apvts.getParameter ("volume"))
    {
        volumeAttachment = std::make_unique<ParameterAttachment> (*vol, [this, vol] (float value)
        {
            const float n = vol->convertTo0to1 (value);
            strip.volume.setValue (n, vol->getText (n, 16));
        });
        volumeAttachment->sendInitialUpdate();

        strip.volume.onGestureStart = [this] { volumeAttachment->beginGesture(); };
        strip.volume.onGestureEnd   = [this] { volumeAttachment->endGesture(); };
        strip.volume.onValue = [this, vol] (float n) { volumeAttachment->setValueAsPartOfGesture (vol->convertFrom0to1 (n)); };
        strip.volume.onReset = [this, vol] { volumeAttachment->setValueAsCompleteGesture (vol->convertFrom0to1 (vol->getDefaultValue())); };
    }

    // --- Pantalla, círculo y teclado
    screen.onChipClicked = [this] (int i) { previewChord (i); };
    circle.onKeyClicked = [this] (int pc, bool minor)
    {
        setChoice ("root", pc + 1);
        setChoice ("scale", minor ? 1 : 0);
    };
    keys.onNote = [this] (int note, float vel, bool on)
    {
        if (on) proc.uiKeyboard.noteOn (1, note, vel);
        else    proc.uiKeyboard.noteOff (1, note, 0.0f);
    };

    refreshPalette();

   #if MIDISKY_DEMO
    // Solo para pruebas locales: MIDISKY_NOTES="60,64,67" enciende esas teclas.
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

    const int startW = jlimit (1040, 2800, proc.editorW.load());
    const int startH = jlimit (680, 1800, proc.editorH.load());
    setResizable (true, true);
    setResizeLimits (1040, 680, 2800, 1800);
    setSize (startW, startH);

    lastTick = Time::getMillisecondCounterHiRes();
    timerCallback();
    startTimerHz (60);
}

MidiSkyEditor::~MidiSkyEditor()
{
    stopTimer();
    stopPreview();
    prefs->removeChangeListener (this);
    if (colourSelector != nullptr) colourSelector->removeChangeListener (this);
    setLookAndFeel (nullptr);
}

bool MidiSkyEditor::isStandalone() const
{
    return proc.wrapperType == AudioProcessor::wrapperType_Standalone;
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
    top.modeButton.setIcon (vis.pal.dark ? IconButton::Icon::sun : IconButton::Icon::moon);
    strip.names.setSelected (vis.latin ? 1 : 0);
    rebuildBackground();
}

void MidiSkyEditor::changeListenerCallback (ChangeBroadcaster* src)
{
    if (colourSelector != nullptr && src == colourSelector.getComponent())
    {
        prefs->custom = colourSelector->getCurrentColour().withAlpha (1.0f);
        prefs->save();
        return;
    }

    refreshPalette();
    lastRoot = -1; // fuerza recalcular
    repaint();
}

void MidiSkyEditor::rebuildBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;

    bgScale = jlimit (1.0f, 2.0f, Component::getApproximateScaleFactorForComponent (this));
    background = Image (Image::RGB, roundToInt ((float) w * bgScale), roundToInt ((float) h * bgScale), false);
    Graphics g (background);
    g.addTransform (AffineTransform::scale (bgScale));

    const auto& pal = vis.pal;
    const auto b = Rectangle<float> (0.0f, 0.0f, (float) w, (float) h);
    ColourGradient body (pal.bodyTop, 0.0f, 0.0f, pal.bodyBottom, 0.0f, (float) h, false);
    g.setGradientFill (body);
    g.fillRect (b);

    // Grano fino, como metal anodizado
    Random rng (99);
    const int grains = (int) (b.getWidth() * b.getHeight() / 14.0f);
    for (int i = 0; i < grains; ++i)
    {
        const bool light = rng.nextBool();
        g.setColour ((light ? Colours::white : Colours::black).withAlpha (pal.dark ? 0.035f : 0.045f));
        g.fillRect (rng.nextFloat() * b.getWidth(), rng.nextFloat() * b.getHeight(), 1.0f, 1.0f);
    }
}

void MidiSkyEditor::paint (Graphics& g)
{
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());
    else
        g.fillAll (vis.pal.bodyTop);
}

void MidiSkyEditor::resized()
{
    proc.editorW.store (getWidth());
    proc.editorH.store (getHeight());

    auto b = getLocalBounds().reduced (14);
    top.setBounds (b.removeFromTop (70));
    b.removeFromTop (12);

    const int kbH = jlimit (150, 320, (int) ((float) getHeight() * 0.25f));
    keys.setBounds (b.removeFromBottom (kbH));
    b.removeFromBottom (12);
    strip.setBounds (b.removeFromBottom (62));
    b.removeFromBottom (12);

    const int side = (int) ((float) b.getWidth() * 0.285f);
    staff.setBounds (b.removeFromLeft (side));
    b.removeFromLeft (12);
    circle.setBounds (b.removeFromRight (side));
    b.removeFromRight (12);
    screen.setBounds (b);

    rebuildBackground();
}

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
            if (! autoHas)
            {
                autoRoot = gk.root; autoMinor = gk.minor; autoHas = true;
            }
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

        const bool latin = vis.latin;
        top.keyButton.setValueText (theory::pcName (vis.keyRoot, vis.sig, latin) + (vis.autoKey ? U8 ("  (auto)") : String()));
        top.scaleButton.setValueText (U8 (theory::scales()[(size_t) vis.scale].name));

        lastRoot = vis.keyRoot; lastScale = vis.scale; lastLatin = vis.latin;
        lastSevenths = vis.sevenths; lastAuto = vis.autoKey; lastAutoHas = vis.autoHasData;
    }
}

void MidiSkyEditor::timerCallback()
{
    const double now = Time::getMillisecondCounterHiRes();
    const float dt = jlimit (0.0f, 0.1f, (float) ((now - lastTick) / 1000.0));
    lastTick = now;
    ++tickCount;

    // Notas que llegan (para destellos e historial de tonalidad)
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

    bool keysChanged = false, flashing = false;
    const float flashDecay = std::pow (0.015f, dt);
    for (size_t n = 0; n < 128; ++n)
    {
        const auto d = proc.notes.down[n].load();
        const auto s = proc.notes.sustained[n].load();
        if (d != vis.down[n] || s != vis.sus[n]) keysChanged = true;
        vis.down[n] = d;
        vis.sus[n] = s;
        if (d > 0) hist[n % 12] += dt * 1.2f;

        if (vis.flash[n] > 0.01f) { vis.flash[n] *= flashDecay; flashing = true; }
        else vis.flash[n] = 0.0f;
    }
    const float histDecay = std::exp (-dt / 30.0f);
    for (auto& h : hist) h *= histDecay;

    vis.pedal = proc.notes.pedal.load();

    vis.active.clear();
    vis.activeMask = 0;
    for (int n = 0; n < 128; ++n)
        if (vis.down[(size_t) n] > 0 || vis.sus[(size_t) n] > 0)
        {
            vis.active.push_back (n);
            vis.activeMask = (uint16_t) (vis.activeMask | (1u << (unsigned) (n % 12)));
        }
    vis.lowest = vis.active.empty() ? -1 : vis.active.front();

    bool keyChanged = false;
    updateKeyState (keyChanged);

    const bool activeChanged = vis.active != lastActive;
    if (activeChanged || keyChanged)
    {
        vis.chord = theory::detectChord (vis.activeMask, vis.lowest >= 0 ? vis.lowest % 12 : 0, vis.sig, vis.latin);
        vis.degree = theory::degreeOf (vis.chord, vis.keyRoot, vis.scale);
        lastActive = vis.active;
        staff.repaint();
        screen.repaint();
        circle.repaint();
    }

    const bool hl = vis.highlight;
    if (keysChanged || flashing || keyChanged || hl != lastHighlight || vis.pedal != lastPedal)
        keys.repaint();
    lastHighlight = hl;
    lastPedal = vis.pedal;

    if (keyChanged)
        top.repaint();

    strip.highlight.setOn (hl);
    strip.pianoSound.setOn (proc.apvts.getRawParameterValue ("piano")->load() > 0.5f);

    // Acorde de prueba
    if (previewOffAt > 0.0 && now > previewOffAt)
        stopPreview();

    if (tickCount % 6 == 0)
        updateMidiStatus();
}

//==============================================================================
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

            // Conexión automática de teclados nuevos
            if (prefs->midiAuto)
            {
                for (auto& d : devices)
                {
                    bool known = false;
                    for (auto& k : knownMidiInputs) if (k.identifier == d.identifier) known = true;
                    if (! known && ! dm.isMidiInputDeviceEnabled (d.identifier))
                        dm.setMidiInputDeviceEnabled (d.identifier, true);
                }
            }
            knownMidiInputs = devices;

            StringArray enabled;
            for (auto& d : devices)
                if (dm.isMidiInputDeviceEnabled (d.identifier)) enabled.add (d.name);

            String text;
            if (enabled.isEmpty())
                text = devices.isEmpty() ? U8 ("Sin teclado MIDI") : U8 ("Desconectado · clic para conectar");
            else if (enabled.size() == 1)
                text = enabled[0];
            else
                text = String (enabled.size()) + " dispositivos";

            top.setMidiStatus (! enabled.isEmpty(), activity, text, true);
            return;
        }
    }
   #endif

    const bool listening = proc.listen.load();
    String host = PluginHostType().getHostDescription();
    if (host.isEmpty() || host == "Unknown") host = "el DAW";
    top.setMidiStatus (listening, activity && listening,
                       listening ? U8 ("Desde ") + host : U8 ("En pausa · clic para escuchar"), true);
}

void MidiSkyEditor::showMidiMenu()
{
   #if JucePlugin_Build_Standalone
    auto* holder = StandalonePluginHolder::getInstance();
    if (holder == nullptr) return;

    auto& dm = holder->deviceManager;
    const auto devices = MidiInput::getAvailableDevices();

    PopupMenu m;
    m.addSectionHeader ("Entradas MIDI");
    if (devices.isEmpty())
        m.addItem (-1, U8 ("No se encontró ningún teclado MIDI"), false);
    for (int i = 0; i < devices.size(); ++i)
        m.addItem (100 + i, devices[i].name, true, dm.isMidiInputDeviceEnabled (devices[i].identifier));
    m.addSeparator();
    m.addItem (1, U8 ("Conectar todo"), ! devices.isEmpty());
    m.addItem (2, U8 ("Desconectar todo"));
    m.addSeparator();
    m.addItem (3, U8 ("Audio y MIDI (avanzado)…"));

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&top).withMinimumWidth (260),
                     [this, devices] (int r)
    {
        auto* h = StandalonePluginHolder::getInstance();
        if (h == nullptr || r == 0) return;
        auto& d = h->deviceManager;

        if (r == 1)
        {
            for (auto& dev : devices) d.setMidiInputDeviceEnabled (dev.identifier, true);
            prefs->midiAuto = true; prefs->save();
        }
        else if (r == 2)
        {
            for (auto& dev : devices) d.setMidiInputDeviceEnabled (dev.identifier, false);
            prefs->midiAuto = false; prefs->save();
            proc.notes.clear();
        }
        else if (r == 3)
        {
            h->showAudioSettingsDialog();
        }
        else if (r >= 100 && r - 100 < devices.size())
        {
            const auto& dev = devices[r - 100];
            const bool en = ! d.isMidiInputDeviceEnabled (dev.identifier);
            d.setMidiInputDeviceEnabled (dev.identifier, en);
            if (! en) proc.notes.clear();
        }
        updateMidiStatus();
    });
   #endif
}

//==============================================================================
void MidiSkyEditor::showKeyMenu()
{
    const int current = (int) proc.apvts.getRawParameterValue ("root")->load();
    PopupMenu m;
    m.addItem (1, U8 ("Auto (la detecto mientras tocas)"), true, current == 0);
    m.addSeparator();
    for (int i = 0; i < 12; ++i)
        m.addItem (2 + i, vis.latin ? U8 (rootNamesEs[i]) : U8 (rootNamesEn[i]), true, current == i + 1);

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&top.keyButton).withMinimumWidth (top.keyButton.getWidth()),
                     [this] (int r)
    {
        if (r == 1)
        {
            setChoice ("root", 0);
        }
        else if (r >= 2)
        {
            setChoice ("root", r - 1);
        }
    });
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

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&top.scaleButton).withMinimumWidth (top.scaleButton.getWidth()),
                     [this] (int r)
    {
        if (r <= 0) return;
        if (vis.autoKey) setChoice ("root", vis.keyRoot + 1); // fija la tonalidad detectada
        setChoice ("scale", r - 1);
    });
}

void MidiSkyEditor::showMainMenu()
{
    PopupMenu skinsMenu;
    for (int i = 0; i < (int) skins().size(); ++i)
        skinsMenu.addItem (100 + i, U8 (skins()[(size_t) i].name), true, prefs->skin == i);
    skinsMenu.addSeparator();
    skinsMenu.addItem (5, U8 ("Elegir color personalizado…"));

    PopupMenu namesMenu;
    namesMenu.addItem (10, U8 ("C D E (inglés)"), true, ! prefs->latin);
    namesMenu.addItem (11, U8 ("Do Re Mi (latino)"), true, prefs->latin);

    PopupMenu chordsMenu;
    chordsMenu.addItem (12, U8 ("Tríadas"), true, ! prefs->sevenths);
    chordsMenu.addItem (13, U8 ("Con séptima"), true, prefs->sevenths);

    PopupMenu m;
    m.addSubMenu ("Skins", skinsMenu);
    m.addItem (6, prefs->dark ? U8 ("Cambiar a modo día") : U8 ("Cambiar a modo noche"));
    m.addSeparator();
    m.addSubMenu ("Nombres de las notas", namesMenu);
    m.addSubMenu ("Acordes de la tonalidad", chordsMenu);
    m.addItem (7, U8 ("Reiniciar detección automática"));
    m.addSeparator();
    if (isStandalone())
        m.addItem (8, U8 ("Audio y MIDI…"));
    m.addItem (9, U8 ("Tamaño original de la ventana"));
    m.addItem (14, U8 ("Acerca de MidiSky"));

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&top.menuButton).withMinimumWidth (240),
                     [this] (int r)
    {
        if (r >= 100) { prefs->skin = r - 100; prefs->save(); if (skins()[(size_t) prefs->skin].custom) showColourPicker(); }
        else if (r == 5) { prefs->skin = (int) skins().size() - 1; prefs->save(); showColourPicker(); }
        else if (r == 6) { prefs->dark = ! prefs->dark; prefs->save(); }
        else if (r == 7) { hist.fill (0.0f); autoHas = false; autoRoot = 0; autoMinor = false; lastRoot = -1; }
        else if (r == 8)
        {
           #if JucePlugin_Build_Standalone
            if (auto* h = StandalonePluginHolder::getInstance()) h->showAudioSettingsDialog();
           #endif
        }
        else if (r == 9) setSize (1180, 760);
        else if (r == 10 || r == 11) { prefs->latin = (r == 11); prefs->save(); }
        else if (r == 12 || r == 13) { prefs->sevenths = (r == 13); prefs->save(); }
        else if (r == 14) showAbout();
    });
}

void MidiSkyEditor::showSkinPicker()
{
    auto picker = std::make_unique<SkinPicker> (ctx, prefs->skin);
    Component::SafePointer<MidiSkyEditor> safe (this);
    picker->onPick = [this, safe] (int i)
    {
        prefs->skin = i;
        prefs->save();
        if (skins()[(size_t) i].custom)
            MessageManager::callAsync ([safe]
            {
                if (safe != nullptr) safe->showColourPicker();
            });
    };
    auto area = getLocalArea (&strip, strip.skinButton.getBounds());
    CallOutBox::launchAsynchronously (std::move (picker), area, this);
}

void MidiSkyEditor::showColourPicker()
{
    auto cs = std::make_unique<ColourSelector> (ColourSelector::showColourAtTop | ColourSelector::showSliders | ColourSelector::showColourspace, 4, 6);
    cs->setName ("Color");
    cs->setCurrentColour (prefs->custom);
    cs->setSize (300, 320);
    cs->addChangeListener (this);
    colourSelector = cs.get();
    auto area = getLocalArea (&strip, strip.skinButton.getBounds());
    CallOutBox::launchAsynchronously (std::move (cs), area, this);
}

void MidiSkyEditor::showAbout()
{
    auto opts = MessageBoxOptions()
        .withIconType (MessageBoxIconType::NoIcon)
        .withTitle ("MidiSky 1.0")
        .withMessage (U8 ("Visualizador de escalas, acordes, notas y pentagrama.\n\n"
                          "Piano: Salamander Grand Piano de Alexander Holm (CC BY 3.0).\n"
                          "Notación: fuente Bravura de Steinberg (SIL OFL).\n"
                          "Tipografía: Outfit (SIL OFL).\n"
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
    for (size_t k = 1; k < d.pcs.size(); ++k)
    {
        int n = prev + ((d.pcs[k] - prev % 12 + 12) % 12);
        if (n == prev) n += 12;
        previewNotes.push_back (n);
        prev = n;
    }

    for (auto n : previewNotes)
        proc.uiKeyboard.noteOn (1, n, 0.68f);
    previewOffAt = Time::getMillisecondCounterHiRes() + 1100.0;
}

void MidiSkyEditor::stopPreview()
{
    for (auto n : previewNotes)
        proc.uiKeyboard.noteOff (1, n, 0.0f);
    previewNotes.clear();
    previewOffAt = 0.0;
}
