#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Theory.h"
#include "Style.h"

//==============================================================================
void NoteTracker::push (uint8_t note, uint8_t vel) noexcept
{
    int s1, n1, s2, n2;
    fifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0)      events[(size_t) s1] = { note, vel };
    else if (n2 > 0) events[(size_t) s2] = { note, vel };
    fifo.finishedWrite (n1 + n2);
}

void NoteTracker::handle (const juce::MidiMessage& m) noexcept
{
    if (m.isNoteOn())
    {
        const auto n = (size_t) m.getNoteNumber();
        const auto v = (uint8_t) juce::jlimit (1, 127, (int) m.getVelocity());
        down[n].store (v);
        sustained[n].store (0);
        push ((uint8_t) n, v);
    }
    else if (m.isNoteOff())
    {
        const auto n = (size_t) m.getNoteNumber();
        const auto v = down[n].load();
        if (pedal.load() && v > 0)
            sustained[n].store (v);
        down[n].store (0);
    }
    else if (m.isSustainPedalOn())
    {
        pedal.store (true);
    }
    else if (m.isSustainPedalOff())
    {
        pedal.store (false);
        for (auto& s : sustained) s.store (0);
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        clear();
    }
}

void NoteTracker::clear() noexcept
{
    for (auto& d : down) d.store (0);
    for (auto& s : sustained) s.store (0);
    pedal.store (false);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MidiSkyProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "piano", 1 }, "Sonido de piano", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "instrument", 1 }, "Instrumento", instrumentNames(), 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "sustain", 1 }, U8 ("Pedal (sostener)"), false));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "volume", 1 }, "Volumen",
        NormalisableRange<float> (-48.0f, 6.0f, 0.1f, 2.2f), -3.0f,
        AudioParameterFloatAttributes().withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) { return v <= -47.9f ? String ("-inf dB") : String (v, 1) + " dB"; })));

    StringArray roots { "Auto" };
    for (auto r : { "C", "C#/Db", "D", "Eb", "E", "F", "F#/Gb", "G", "Ab", "A", "Bb", "B" })
        roots.add (r);
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "root", 1 }, "Tonalidad", roots, 0));

    StringArray sc;
    for (auto& s : theory::scales())
        sc.add (U8 (s.name));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Escala", sc, 0));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "highlight", 1 }, "Resaltar escala", true));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "latency", 1 }, "Compensar latencia",
        NormalisableRange<float> (0.0f, 300.0f, 1.0f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("ms")
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; })));

    return layout;
}

MidiSkyProcessor::MidiSkyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MidiSky", createLayout())
{
    pianoOn = apvts.getRawParameterValue ("piano");
    volume  = apvts.getRawParameterValue ("volume");
    latencyMs = apvts.getRawParameterValue ("latency");
    instrumentParam = apvts.getRawParameterValue ("instrument");
    sustainParam = apvts.getRawParameterValue ("sustain");

    if (wrapperType == wrapperType_Standalone)
        host = std::make_unique<PluginHost>();
    for (auto& d : degreeChords) for (auto& n : d) n.store (-1);
    for (auto& pm : padMap) pm.store (-1);
    updateDegreeChords (0, 0, prefs->sevenths, prefs->degreeOctave);
    applyPrefs();
   #if MIDISKY_DEMO
    if (juce::SystemStats::getEnvironmentVariable ("MIDISKY_METRO", "").isNotEmpty())
    {
        metroSettings.on.store (true);
        metroSettings.sound.store (juce::SystemStats::getEnvironmentVariable ("MIDISKY_METRO", "0").getIntValue());
        metroSettings.followHost.store (false);
    }
   #endif
}

void MidiSkyProcessor::applyPrefs()
{
    metroSettings.bpm.store (prefs->metroBpm);
    metroSettings.beats.store (prefs->metroBeats);
    metroSettings.sound.store (prefs->metroSound);
    metroSettings.volumeDb.store (prefs->metroVolume);
    metroSettings.followHost.store (prefs->metroFollowHost);
    recorder.countIn.store (prefs->countIn);
    recorder.recordMouse.store (prefs->recordMouse);
    recorder.recordAudio.store (false);
    padsOn.store (prefs->degreePadsOn);
    for (int i = 0; i < 7; ++i) padMap[(size_t) i].store (prefs->degreePads[(size_t) i]);
}

// El acorde de un grado como suena en Midi Cat: el bajo una octava abajo y las notas juntas arriba
std::vector<int> MidiSkyProcessor::voiceDegree (const theory::Diatonic& d, int octave)
{
    std::vector<int> out;
    int prev = 48 + 12 * octave + d.rootPc;
    if (prev < 52 + 12 * octave) prev += 12;
    out.push_back (prev - 12);
    out.push_back (prev);
    for (size_t i = 1; i < d.pcs.size(); ++i)
    {
        int n = prev + ((d.pcs[i] - prev % 12 + 12) % 12);
        if (n == prev) n += 12;
        out.push_back (n);
        prev = n;
    }
    for (auto& n : out) n = juce::jlimit (0, 127, n);
    return out;
}

void MidiSkyProcessor::updateDegreeChords (int keyRoot, int scale, bool sevenths, int octave)
{
    const auto dia = theory::diatonicChords (keyRoot, scale, sevenths, false);
    for (int i = 0; i < 7; ++i)
        setDegreeChord (i, i < (int) dia.size() ? voiceDegree (dia[(size_t) i], octave) : std::vector<int>());
}

std::vector<int> MidiSkyProcessor::degreeChordNotes (int degree) const
{
    std::vector<int> out;
    if (degree < 0 || degree >= 7) return out;
    for (auto& a : degreeChords[(size_t) degree])
        if (const int n = a.load(); n >= 0) out.push_back (n);
    return out;
}

void MidiSkyProcessor::setDegreeChord (int degree, const std::vector<int>& chord) noexcept
{
    if (degree < 0 || degree >= 7) return;
    for (int j = 0; j < 6; ++j)
        degreeChords[(size_t) degree][(size_t) j].store (j < (int) chord.size() ? juce::jlimit (0, 127, chord[(size_t) j]) : -1);
}

bool MidiSkyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void MidiSkyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    metronome.prepare (sampleRate);
    mouseScratch.ensureSize (4096);
    if (host != nullptr) host->prepare (sampleRate, samplesPerBlock);
    currentRate.store (sampleRate);
    delayQueue.ensureSize (16384);
    delayScratch.ensureSize (16384);
    pedalScratch.ensureSize (8192);
    padScratch.ensureSize (8192);
    degreeScratch.ensureSize (4096);
    delayQueue.clear();
    lastDelaySamples = 0;
    piano.prepare (sampleRate);
    wasLatched = false;   // si el pedal en pantalla está puesto, se vuelve a mandar
    gainSmooth.reset (sampleRate, 0.03);
    gainSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (volume->load(), -47.9f));
}

void MidiSkyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    const bool listening = listen.load();
    if (listening != wasListening)
    {
        if (! listening) { notes.clear(); piano.allNotesOff(); }
        wasListening = listening;
    }
    if (! listening)
        midi.clear();

    bool external = false;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOnOrOff() || m.isController()) external = true;
    }
    if (external)
        lastMidiActivityMs.store ((int) juce::Time::getMillisecondCounter());

    // Compensar latencia: retrasar las notas que vienen de afuera
    const int delaySamples = juce::jlimit (0, (int) (getSampleRate() * 0.5),
                                           (int) std::lround (latencyMs->load() * getSampleRate() / 1000.0));
    if (delaySamples != lastDelaySamples)
    {
        // Si cambia el retraso, lo pendiente sale ya (así no quedan notas pegadas)
        for (const auto meta : delayQueue)
            midi.addEvent (meta.getMessage(), 0);
        delayQueue.clear();
        lastDelaySamples = delaySamples;
    }
    if (delaySamples > 0 || ! delayQueue.isEmpty())
    {
        for (const auto meta : midi)
            delayQueue.addEvent (meta.getMessage(), meta.samplePosition + delaySamples);
        midi.clear();
        delayScratch.clear();
        for (const auto meta : delayQueue)
        {
            if (meta.samplePosition < n) midi.addEvent (meta.getMessage(), meta.samplePosition);
            else                         delayScratch.addEvent (meta.getMessage(), meta.samplePosition - n);
        }
        delayQueue.swapWith (delayScratch);
    }

    if (allOffRequest.exchange (false))
    {
        notes.clear();
        piano.allNotesOff();
    }

    // Pads MIDI que tocan grados: el pad se cambia por el acorde de su grado (se oye, se ve y se graba)
    {
        bool anyPad = false;
        const bool usePads = padsOn.load();
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn())
            {
                ++noteEventCounter;
                lastNoteEvent.store (((noteEventCounter & 0x7fff) << 16) | (m.getChannel() << 8) | m.getNoteNumber());
            }
            if (usePads && m.isNoteOnOrOff()) anyPad = true;
        }
        if (anyPad)
        {
            padScratch.clear();
            for (const auto meta : midi)
            {
                const auto m = meta.getMessage();
                int deg = -1;
                if (m.isNoteOnOrOff())
                    for (int i = 0; i < 7 && deg < 0; ++i)
                    {
                        const int code = padMap[(size_t) i].load();
                        if (code < 0) continue;
                        const int note = code & 0x7f, ch = code >> 8;
                        if (note == m.getNoteNumber() && (ch == 0 || ch == m.getChannel())) deg = i;
                    }
                if (deg < 0) { padScratch.addEvent (m, meta.samplePosition); continue; }
                auto& held = padHeld[(size_t) deg];
                // Primero se suelta lo que el pad dejó sonando
                for (auto& n : held)
                    if (n > 0 && --padRefs[(size_t) (n - 1)] == 0)
                        padScratch.addEvent (juce::MidiMessage::noteOff (1, n - 1), meta.samplePosition);
                held.fill (0);
                if (m.isNoteOn())
                {
                    const float vel = m.getFloatVelocity();
                    for (int j = 0; j < 6; ++j)
                    {
                        const int n = degreeChords[(size_t) deg][(size_t) j].load();
                        if (n < 0) continue;
                        held[(size_t) j] = n + 1;
                        if (padRefs[(size_t) n]++ == 0)
                            padScratch.addEvent (juce::MidiMessage::noteOn (1, n, vel), meta.samplePosition);
                    }
                }
            }
            midi.swapWith (padScratch);
        }
    }

    // Grados con las teclas de la compu: van con lo de afuera (siempre se graban)
    degreeScratch.clear();
    degreeKeyboard.processNextMidiBuffer (degreeScratch, 0, n, true);
    for (const auto meta : degreeScratch)
        midi.addEvent (meta.getMessage(), meta.samplePosition);

    // Lo tocado con el mouse va aparte (para poder grabarlo o no)
    mouseScratch.clear();
    uiKeyboard.processNextMidiBuffer (mouseScratch, 0, n, true);

    // Pedal en pantalla: sostiene como un pedal de verdad (y mientras está puesto ignora el pedal físico)
    const bool latch = sustainParam->load() > 0.5f;
    pedalScratch.clear();
    if (latch != wasLatched)
        pedalScratch.addEvent (juce::MidiMessage::controllerEvent (1, 64, latch ? 127 : 0), 0);
    wasLatched = latch;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (latch && m.isController() && m.getControllerNumber() == 64) continue;
        pedalScratch.addEvent (m, meta.samplePosition);
    }

    // Grabadora: lo que llega del teclado (y el pedal), y si se quiere, lo del mouse
    const bool metroOn = metroSettings.on.load();
    recorder.process (pedalScratch, mouseScratch, n, metronome, metroOn, getSampleRate());

    for (const auto meta : mouseScratch)
        pedalScratch.addEvent (meta.getMessage(), meta.samplePosition);
    midi.swapWith (pedalScratch);
    recorder.injectBounce (midi, n);   // pasar un MIDI a audio: sus notas entran aquí
    piano.setInstrument ((int) instrumentParam->load());

    for (const auto meta : midi)
        notes.handle (meta.getMessage());

    const bool on = pianoOn->load() > 0.5f;
    if (on != wasOn)
    {
        if (! on) piano.allNotesOff();
        wasOn = on;
    }

    if (on)
    {
        // Con un instrumento tuyo cargado (app sola), suena ese; si no, el piano de MidiCat
        const bool hosted = host != nullptr && host->hasInstrument() && host->render (buffer, midi, n);
        if (hosted != wasHosted) { piano.allNotesOff(); wasHosted = hosted; }
        if (! hosted)
            piano.render (buffer, midi);
    }

    gainSmooth.setTargetValue (juce::Decibels::decibelsToGain (volume->load(), -47.9f));
    const int chans = buffer.getNumChannels();
    for (int s = 0; s < n; ++s)
    {
        const float g = gainSmooth.getNextValue();
        for (int c = 0; c < chans; ++c)
        {
            auto* d = buffer.getWritePointer (c);
            // Limitador suave (continuo) para que los acordes grandes no saturen.
            const float x = d[s] * g;
            const float ax = std::abs (x);
            d[s] = ax < 0.8f ? x : std::copysign (0.8f + 0.2f * std::tanh ((ax - 0.8f) / 0.2f), x);
        }
    }

    // Audio de la grabación (sin el metrónomo)
    recorder.writeAudio (buffer, n);

    // El metrónomo va aparte (su propio volumen)
    metronome.process (buffer, n, metroSettings, getPlayHead());
}

//==============================================================================
juce::AudioProcessorEditor* MidiSkyProcessor::createEditor()
{
    return new MidiSkyEditor (*this);
}

void MidiSkyProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("editorW", editorW.load(), nullptr);
    state.setProperty ("editorH", editorH.load(), nullptr);
    if (auto xml = state.createXml())
    {
        if (host != nullptr)
            if (auto h = host->createState())
                xml->addChildElement (h.release());
        copyXmlToBinary (*xml, dest);
    }
}

void MidiSkyProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        if (auto* h = xml->getChildByName ("HOSTED"))
        {
            if (host != nullptr) host->restoreState (*h);
            xml->removeChildElement (h, true);
        }
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.hasType (apvts.state.getType()))
        {
            {
                int w = (int) tree.getProperty ("editorW", 1290);
                if (w == 1200 && (int) tree.getProperty ("editorH", 760) == 760) w = 1290;   // tamaño normal de antes → el de ahora
                editorW.store (w);
            }
            editorH.store ((int) tree.getProperty ("editorH", 760));
            apvts.replaceState (tree);
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiSkyProcessor();
}
