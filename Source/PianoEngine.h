#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <thread>

//==============================================================================
// Los sonidos de Midi Cat (inspirados en las colecciones de teclados clásicos)
enum class Instrument
{
    grand = 0,      // Piano de cola (Yamaha, Salamander)
    concert,        // Piano de concierto (Steinway, Splendid)
    felt,           // Piano suave (con fieltro)
    honky,          // Piano de bar (dos cuerdas desafinadas)
    rhodes,         // Piano eléctrico (Rhodes Mark I)
    rhodesTrem,     // Eléctrico con trémolo
    celesta,        // Celesta (síntesis)
    toy,            // Piano de juguete (síntesis)
    harpsichord,    // Clavecín (síntesis de cuerda pulsada)
    count
};

juce::StringArray instrumentNames();

//==============================================================================
// Muestras grabadas. Se cargan una sola vez por proceso, en un hilo aparte,
// y todas las instancias del plugin las comparten.
class PianoSampleBank
{
public:
    PianoSampleBank();
    ~PianoSampleBank();

    enum Set { salamander = 0, steinway = 1, rhodes = 2, numSets = 3 };

    struct Sample
    {
        int rootNote = 60;
        int layer = 0;          // 0 suave, 1 fuerte
        double sampleRate = 44100.0;
        juce::AudioBuffer<float> data;
    };

    bool isReady() const noexcept { return ready.load (std::memory_order_acquire); }
    // solo válido si isReady()
    const Sample* sampleFor (int set, int midiNote, float velocity) const noexcept;

private:
    void load();

    std::vector<Sample> samples;
    std::array<std::array<std::array<int, 128>, 2>, numSets> lookup {};
    std::array<bool, numSets> hasSoftLayer {};
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
    PianoVoice (const PianoSampleBank& b, const std::atomic<int>& instrument);

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<PianoSound*> (s) != nullptr; }
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int startSample, int numSamples) override;

private:
    void finish();
    float nextSynthSample();

    const PianoSampleBank& bank;
    const std::atomic<int>& instrumentRef;
    Instrument inst = Instrument::grand;
    const PianoSampleBank::Sample* sample = nullptr;
    bool active = false;

    // Muestras
    double pos = 0.0, step = 1.0, pos2 = 0.0, step2 = 1.0;
    bool doubled = false;
    float gain = 0.0f;
    float env = 0.0f, attackInc = 0.0f, releaseMul = 0.999f;
    bool releasing = false;
    float lpCoef = 1.0f, lpL = 0.0f, lpR = 0.0f;
    double tremPhase = 0.0, tremInc = 0.0;

    // Síntesis
    double phase[4] {}, phaseInc[4] {};
    float partAmp[4] {}, partDecay[4] {};
    float fmIndex = 0.0f, fmDecay = 1.0f;
    float clickEnv = 0.0f;
    juce::Random noise;
    std::vector<float> string1, string2;   // cuerdas del clavecín (Karplus-Strong)
    int len1 = 0, len2 = 0, idx1 = 0, idx2 = 0;
    float last1 = 0.0f, last2 = 0.0f, loss = 0.996f;
    double sr = 44100.0;
    int ageSamples = 0, maxAge = 0;
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
    void setInstrument (int i) { instrument.store (juce::jlimit (0, (int) Instrument::count - 1, i)); }

private:
    juce::SharedResourcePointer<PianoSampleBank> bank;
    std::atomic<int> instrument { 0 };
    juce::Synthesiser synth;
    juce::SmoothedValue<float> level { 1.45f };
};
