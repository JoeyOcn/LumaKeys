#pragma once

#include <JuceHeader.h>

//==============================================================================
// Ajustes del metrónomo (los lee el hilo de audio sin bloqueos)
struct MetronomeSettings
{
    std::atomic<bool> on { false };
    std::atomic<float> bpm { 100.0f };      // el de Midi Cat (en la app o si el DAW no da tempo)
    std::atomic<int> beats { 4 };           // tiempos por compás
    std::atomic<int> sound { 0 };
    std::atomic<float> volumeDb { -8.0f };
    std::atomic<bool> followHost { true };  // dentro del DAW: usar el tempo y el compás del DAW
};

juce::StringArray metronomeSoundNames();

//==============================================================================
// Metrónomo con clics sutiles (sintetizados, sin archivos)
class Metronome
{
public:
    void prepare (double sampleRate);

    // Suma los clics al buffer. 'playHead' puede ser nulo (app sola).
    void process (juce::AudioBuffer<float>& out, int numSamples, const MetronomeSettings&, juce::AudioPlayHead* playHead);

    // Empieza el compás desde el tiempo 1 (para la cuenta antes de grabar)
    void restartBar() const noexcept { restartRequest.store (true); }

    // Para la grabadora (hilo de audio): cuántas muestras faltan para el próximo tiempo 1,
    // cuántas pasaron desde el último y cuánto dura un tiempo.
    double samplesPerBeat() const noexcept { return spb; }
    double beatPhase() const noexcept { return phase; }         // en tiempos, sigue creciendo
    int beatsPerBar() const noexcept { return barBeats; }
    double tempo() const noexcept { return bpmNow; }

    // Para la interfaz
    std::atomic<int> uiBeat { -1 };           // tiempo dentro del compás (0 = el fuerte)
    std::atomic<uint32_t> uiBeatCount { 0 };  // sube con cada tiempo
    std::atomic<float> uiTempo { 100.0f };
    std::atomic<bool> uiHostTempo { false };  // el tempo viene del DAW

    // Para escuchar un sonido al elegirlo
    void previewSound (int sound) noexcept { previewRequest.store (sound); }

private:
    void buildSounds();
    void startClick (int sound, bool accent, int offset) noexcept;

    double sr = 44100.0;
    double phase = 0.0;      // tiempos transcurridos
    double spb = 26460.0;    // muestras por tiempo
    double bpmNow = 100.0;
    int barBeats = 4;
    bool wasOn = false;
    mutable std::atomic<bool> restartRequest { false };
    std::atomic<int> previewRequest { -1 };

    // [sonido][0 = normal, 1 = fuerte]
    std::vector<std::array<std::vector<float>, 2>> sounds;
    const std::vector<float>* playing = nullptr;
    int playPos = 0, playStart = 0;
    juce::SmoothedValue<float> gain;
};
