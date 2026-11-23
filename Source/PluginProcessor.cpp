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
}

bool MidiSkyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void MidiSkyProcessor::prepareToPlay (double sampleRate, int)
{
    currentRate.store (sampleRate);
    delayQueue.ensureSize (16384);
    delayScratch.ensureSize (16384);
    pedalScratch.ensureSize (8192);
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

    uiKeyboard.processNextMidiBuffer (midi, 0, n, true);

    // Pedal en pantalla: sostiene como un pedal de verdad (y mientras está puesto ignora el pedal físico)
    const bool latch = sustainParam->load() > 0.5f;
    if (latch || latch != wasLatched)
    {
        pedalScratch.clear();
        if (latch != wasLatched)
            pedalScratch.addEvent (juce::MidiMessage::controllerEvent (1, 64, latch ? 127 : 0), 0);
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (latch && m.isController() && m.getControllerNumber() == 64) continue;
            pedalScratch.addEvent (m, meta.samplePosition);
        }
        midi.swapWith (pedalScratch);
        wasLatched = latch;
    }
    piano.setInstrument ((int) instrumentParam->load());

    for (const auto meta : midi)
        notes.handle (meta.getMessage());

    const bool on = pianoOn->load() > 0.5f;
    if (on != wasOn)
    {
        if (! on) piano.allNotesOff();
        wasOn = on;
    }

    if (! on)
        return;

    piano.render (buffer, midi);

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
        copyXmlToBinary (*xml, dest);
}

void MidiSkyProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.hasType (apvts.state.getType()))
        {
            editorW.store ((int) tree.getProperty ("editorW", 1200));
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
