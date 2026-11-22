#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <thread>

//==============================================================================
// Muestras del piano (Salamander Grand Piano, CC BY 3.0). Se cargan una sola vez por proceso,
// en un hilo aparte, y todas las instancias del plugin las comparten.
class PianoSampleBank
{
public:
    PianoSampleBank();
    ~PianoSampleBank();

    struct Sample
    {
        int rootNote = 60;
        double sampleRate = 44100.0;
        juce::AudioBuffer<float> data;
    };

    bool isReady() const noexcept { return ready.load (std::memory_order_acquire); }
    const Sample* sampleFor (int midiNote) const noexcept; // solo válido si isReady()

private:
    void load();

    std::vector<Sample> samples;
    std::array<int, 128> lookup {};
    std::atomic<bool> ready { false };
    std::atomic<bool> cancel { false };
    std::thread loader;
};

//==============================================================================
class PianoSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class PianoVoice : public juce::SynthesiserVoice
{
public:
    explicit PianoVoice (const PianoSampleBank& b) : bank (b) {}

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<PianoSound*> (s) != nullptr; }
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int startSample, int numSamples) override;

private:
    const PianoSampleBank& bank;
    const PianoSampleBank::Sample* sample = nullptr;

    double pos = 0.0, step = 1.0;
    float gain = 0.0f;
    float env = 0.0f, attackInc = 0.0f, releaseMul = 0.999f;
    bool releasing = false;
    float lpCoef = 1.0f, lpL = 0.0f, lpR = 0.0f;
};

//==============================================================================
class PianoEngine
{
public:
    PianoEngine();

    void prepare (double sampleRate);
    void render (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi);
    void allNotesOff();
    bool isReady() const { return bank->isReady(); }

private:
    juce::SharedResourcePointer<PianoSampleBank> bank;
    juce::Synthesiser synth;
};
