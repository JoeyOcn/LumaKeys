#pragma once

#include <JuceHeader.h>
#include "Metronome.h"

//==============================================================================
// Grabadora MIDI: lo que tocas se guarda como archivo .mid en la carpeta de grabaciones,
// listo para arrastrarlo a Ableton, FL Studio o cualquier DAW.
class MidiRecorder : private juce::Timer
{
public:
    MidiRecorder();
    ~MidiRecorder() override;

    // --- Hilo de la interfaz
    void start();
    void stop();
    void toggle() { if (state() == 0) start(); else stop(); }
    int state() const noexcept { return st.load(); }     // 0 nada, 1 cuenta antes de grabar, 2 grabando
    double elapsedSeconds() const noexcept;
    int beatsLeftInCountIn() const noexcept;

    std::atomic<bool> countIn { true };
    std::atomic<bool> recordMouse { true };   // también lo que tocas con el mouse (teclas y grados)

    static juce::File folder();
    static juce::Array<juce::File> recordings();   // la más nueva primero
    std::function<void (const juce::File&)> onSaved;
    std::function<void()> onEmpty;                  // se paró sin notas

    // --- Hilo de audio
    void process (const juce::MidiBuffer& played, const juce::MidiBuffer& mouse, int numSamples,
                  const Metronome& metro, bool metronomeOn, double sampleRate) noexcept;

    // --- Audio: se graba el sonido elegido (piano o tu instrumento) en un .wav junto al .mid
    std::atomic<bool> recordAudio { true };
    void writeAudio (const juce::AudioBuffer<float>& buffer, int numSamples) noexcept;   // hilo de audio
    static juce::File audioFileFor (const juce::File& midi) { return midi.withFileExtension (".wav"); }
    // El WAV solo cuenta si es más nuevo que el MIDI (si editaste el MIDI después, hay que pasarlo otra vez)
    static juce::File freshAudioFor (const juce::File& midi)
    {
        auto w = audioFileFor (midi);
        return w.existsAsFile() && w.getLastModificationTime() >= midi.getLastModificationTime() ? w : juce::File();
    }
    std::function<void (const juce::File& wav)> onAudioSaved;

    // --- Pasar un .mid a audio: se toca con el sonido elegido y se graba (en tiempo real)
    bool startBounce (const juce::File& midiFile);
    void cancelBounce();
    bool isBouncing() const noexcept { return bounceRunning.load() || bounceArm.load(); }
    float bounceProgress() const noexcept;
    juce::File bouncingFile() const { return bounceTarget; }
    void injectBounce (juce::MidiBuffer& midi, int numSamples) noexcept;   // hilo de audio

private:
    bool openWriter (const juce::File& target, double sampleRate);
    void closeWriter (bool keep);
    struct Ev { uint8_t d[3] {}; uint8_t size = 0; int64_t time = 0; };
    void push (const juce::MidiMessage& m, int64_t time) noexcept;
    void timerCallback() override;
    void finish();

    juce::AbstractFifo fifo { 16384 };
    std::vector<Ev> ring;
    std::vector<Ev> take;

    std::atomic<int> st { 0 };
    std::atomic<bool> startReq { false }, stopReq { false };
    std::atomic<int64_t> clock { 0 }, origin { 0 }, stopAt { -1 };
    std::atomic<double> rate { 44100.0 }, bpmAtStart { 120.0 }, spbAtStart { 22050.0 };
    std::atomic<int> beatsAtStart { 4 };
    std::atomic<bool> grid { false };

    // Audio
    juce::TimeSliceThread writerThread { "Midi Cat: audio" };
    juce::SpinLock writerLock;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
    juce::File writerTemp, writerTarget;
    std::atomic<int> audioMode { 0 };            // 0 nada, 1 grabando, 2 pasando a audio
    std::atomic<int64_t> audioFrom { -1 }, audioUntil { -1 };
    std::atomic<bool> audioDone { false };
    juce::String takeBase;                       // nombre de la toma (para el .wav)
    bool takeEmpty = false;

    // Pasar a audio
    struct BEv { int64_t sample = 0; uint8_t d[3] {}; uint8_t size = 0; };
    std::vector<BEv> bounceEvents;
    size_t bounceNext = 0;
    std::atomic<int64_t> bouncePos { 0 }, bounceLen { 1 };
    std::atomic<bool> bounceRunning { false }, bounceArm { false }, bounceStop { false };
    juce::File bounceTarget;
};

//==============================================================================
// Lo que hay dentro de un .mid (para verlo y cambiar la fuerza de las notas)
struct MidiClip
{
    struct Note { int note = 60; int velocity = 100; double start = 0.0, end = 0.0; int onIndex = -1; };
    juce::File file;
    juce::MidiFile midi;
    std::vector<Note> notes;      // en tiempos (negras)
    double lengthBeats = 0.0;
    double seconds = 0.0;
    double bpm = 120.0;

    bool load (const juce::File&);
    bool saveVelocities();
};
