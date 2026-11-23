#pragma once

#include <JuceHeader.h>
#include "PianoEngine.h"
#include "Metronome.h"
#include "Recorder.h"
#include "PluginHost.h"
#include "Style.h"
#include "Theory.h"

//==============================================================================
// Estado de las teclas, compartido sin bloqueos entre el hilo de audio y la interfaz.
struct NoteTracker
{
    std::array<std::atomic<uint8_t>, 128> down {};       // velocidad si la tecla está abajo
    std::array<std::atomic<uint8_t>, 128> sustained {};  // notas sostenidas por el pedal
    std::atomic<bool> pedal { false };

    struct Event { uint8_t note = 0, velocity = 0; };
    juce::AbstractFifo fifo { 1024 };
    std::array<Event, 1024> events {};

    void handle (const juce::MidiMessage& m) noexcept;
    void clear() noexcept;
    void push (uint8_t note, uint8_t vel) noexcept;
};

//==============================================================================
class MidiSkyProcessor : public juce::AudioProcessor
{
public:
    MidiSkyProcessor();
    ~MidiSkyProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override { if (host != nullptr) host->release(); }
    void reset() override { wasLatched = false; }
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState uiKeyboard;   // notas tocadas con el mouse
    juce::MidiKeyboardState degreeKeyboard;   // grados tocados con teclas de la compu (se graban siempre)
    NoteTracker notes;

    // Grados con pads MIDI: el acorde de cada grado (lo pone la interfaz) y qué pad toca cuál
    void setDegreeChord (int degree, const std::vector<int>& notesOfChord) noexcept;
    void updateDegreeChords (int keyRoot, int scale, bool sevenths, int octave);
    std::vector<int> degreeChordNotes (int degree) const;
    static std::vector<int> voiceDegree (const theory::Diatonic&, int octave);
    std::atomic<int> lastNoteEvent { 0 };   // última nota que llegó: contador << 16 | canal << 8 | nota (para aprender pads)

    bool isPianoReady() const { return piano.isReady(); }
    std::atomic<int> lastMidiActivityMs { 0 };
    std::atomic<bool> listen { true };   // "desconectar" el MIDI del DAW sin tocar Ableton
    std::atomic<bool> allOffRequest { false };  // apagar todas las notas (pedido desde la interfaz)
    std::atomic<double> currentRate { 44100.0 };

    // Tamaño de la ventana (se guarda con el proyecto)
    std::atomic<int> editorW { 1290 }, editorH { 760 };

    // Metrónomo, grabadora e instrumentos propios (estos últimos solo en la app sola)
    MetronomeSettings metroSettings;
    Metronome metronome;
    MidiRecorder recorder;
    std::unique_ptr<PluginHost> host;
    void applyPrefs();   // copia los ajustes guardados (metrónomo, grabadora) al audio

private:
    PianoEngine piano;
    std::atomic<float>* pianoOn = nullptr;
    std::atomic<float>* volume = nullptr;
    std::atomic<float>* latencyMs = nullptr;
    std::atomic<float>* instrumentParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    juce::MidiBuffer pedalScratch, mouseScratch, padScratch, degreeScratch;
    std::array<std::array<std::atomic<int>, 6>, 7> degreeChords {};   // -1 = sin nota
    std::array<std::atomic<int>, 7> padMap {};                         // nota + 256 * canal, -1 = sin pad
    std::atomic<bool> padsOn { true };
    std::array<std::array<int, 6>, 7> padHeld {};                      // notas que dejó sonando cada pad
    std::array<uint8_t, 128> padRefs {};
    int noteEventCounter = 0;
    juce::SharedResourcePointer<Prefs> prefs;
    bool wasLatched = false;
    bool wasHosted = false;

    // Compensación de latencia: las notas de afuera se retrasan N ms (sonido y luces juntos)
    juce::MidiBuffer delayQueue, delayScratch;
    int lastDelaySamples = 0;
    bool wasOn = true;
    bool wasListening = true;
    juce::SmoothedValue<float> gainSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSkyProcessor)
};
