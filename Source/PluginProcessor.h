#pragma once

#include <JuceHeader.h>
#include "PianoEngine.h"

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
    void releaseResources() override {}
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
    NoteTracker notes;

    bool isPianoReady() const { return piano.isReady(); }
    std::atomic<int> lastMidiActivityMs { 0 };
    std::atomic<bool> listen { true };   // "desconectar" el MIDI del DAW sin tocar Ableton
    std::atomic<bool> allOffRequest { false };  // apagar todas las notas (pedido desde la interfaz)
    std::atomic<double> currentRate { 44100.0 };

    // Tamaño de la ventana (se guarda con el proyecto)
    std::atomic<int> editorW { 1200 }, editorH { 760 };

private:
    PianoEngine piano;
    std::atomic<float>* pianoOn = nullptr;
    std::atomic<float>* volume = nullptr;
    std::atomic<float>* latencyMs = nullptr;
    std::atomic<float>* instrumentParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    juce::MidiBuffer pedalScratch;
    bool wasLatched = false;

    // Compensación de latencia: las notas de afuera se retrasan N ms (sonido y luces juntos)
    juce::MidiBuffer delayQueue, delayScratch;
    int lastDelaySamples = 0;
    bool wasOn = true;
    bool wasListening = true;
    juce::SmoothedValue<float> gainSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSkyProcessor)
};
