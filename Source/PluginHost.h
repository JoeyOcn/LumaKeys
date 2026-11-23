#pragma once

#include <JuceHeader.h>

//==============================================================================
// En la app sola (sin DAW): busca tus instrumentos VST3/AU y toca con ellos.
class PluginHost : private juce::Thread
{
public:
    PluginHost();
    ~PluginHost() override;

    // --- Hilo de la interfaz
    void startScan();
    bool isScanning() const noexcept { return scanning.load(); }
    float scanProgress() const noexcept { return progress.load(); }
    juce::String scanningName() const;
    juce::Array<juce::PluginDescription> instruments() const;   // ordenados por nombre
    std::function<void()> onListChanged;                         // al terminar de buscar

    void load (const juce::PluginDescription&, std::function<void (const juce::String& error)> done, const juce::MemoryBlock* stateToRestore = nullptr);
    void unload();
    bool hasInstrument() const noexcept { return loaded.load(); }
    juce::String instrumentName() const { return current.name; }
    const juce::PluginDescription& description() const { return current; }
    juce::AudioPluginInstance* instance() const noexcept { return inst.get(); }
    std::function<void()> onBeforeChange;   // para cerrar la ventana del instrumento antes de cambiarlo
    std::function<void()> onChanged;

    std::unique_ptr<juce::XmlElement> createState() const;
    void restoreState (const juce::XmlElement&);

    // --- Hilo de audio
    void prepare (double sampleRate, int blockSize);
    bool render (juce::AudioBuffer<float>& out, juce::MidiBuffer& midi, int numSamples) noexcept;
    void release();

private:
    void run() override;
    void saveList() const;
    void allocateScratch();

    juce::AudioPluginFormatManager formats;
    juce::KnownPluginList known;
    juce::File dataDir, listFile, pedalFile;

    std::unique_ptr<juce::AudioPluginInstance> inst;
    juce::PluginDescription current;
    juce::CriticalSection lock;
    juce::AudioBuffer<float> scratch;
    std::atomic<bool> loaded { false };
    double rate = 44100.0;
    int block = 512;

    std::atomic<bool> scanning { false };
    std::atomic<float> progress { 0.0f };
    juce::CriticalSection nameLock;
    juce::String scanName;
    std::shared_ptr<bool> alive { std::make_shared<bool> (true) };
    int loadId = 0;
};
