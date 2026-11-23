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

private:
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
