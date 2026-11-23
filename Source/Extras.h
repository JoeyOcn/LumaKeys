#pragma once

#include "Views.h"
#include "Metronome.h"
#include "Recorder.h"
#include "PluginHost.h"

//==============================================================================
// Escribir un nombre (para guardar un tema o un acomodo)
class NamePanel : public juce::Component
{
public:
    NamePanel (UiContext& c, const juce::String& prompt, const juce::String& suggested);
    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    std::function<void (const juce::String&)> onSave;

private:
    void commit();
    UiContext& ctx;
    juce::String prompt;
    juce::TextEditor editor;
    PushButton saveBtn;
};

//==============================================================================
// Número grande del tempo: se arrastra hacia arriba/abajo o con la rueda
class BpmDisplay : public juce::Component
{
public:
    explicit BpmDisplay (UiContext& c) : ctx (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void setBpm (float b, bool fromHost) { if (std::abs (b - bpm) > 0.01f || fromHost != host) { bpm = b; host = fromHost; repaint(); } }
    std::function<void (float)> onChange;

private:
    UiContext& ctx;
    float bpm = 100.0f, dragStart = 100.0f;
    bool host = false;
};

class MetronomePanel : public juce::Component, private juce::Timer
{
public:
    MetronomePanel (UiContext& c, Prefs& p, MetronomeSettings& s, Metronome& m, bool inDaw, std::function<void()> settingsChanged);
    ~MetronomePanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setBpm (float b);
    UiContext& ctx;
    Prefs& prefs;
    MetronomeSettings& settings;
    Metronome& metro;
    bool inDaw;
    std::function<void()> changed;

    Switch onSw, followSw;
    BpmDisplay bpm;
    PushButton minusBtn, plusBtn, tapBtn;
    Segmented beats;
    juce::OwnedArray<PushButton> soundBtns;
    juce::Slider volume;
    juce::Array<double> taps;
    uint32_t lastCount = 0;
    float beatFlash = 0.0f;
    int beatShown = -1;
};

//==============================================================================
// Ver una grabación: notas arriba y la fuerza (velocity) abajo, que se dibuja como en FL Studio
class ClipView : public juce::Component
{
public:
    explicit ClipView (UiContext& c) : ctx (c) {}
    void setFile (const juce::File&);
    const juce::File& getFile() const { return clip.file; }
    bool hasClip() const { return loaded; }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    std::function<void()> onEdited;
    std::function<void()> onOpen;    // clic en las notas: abrir el piano roll

private:
    juce::Rectangle<float> rollArea() const;
    juce::Rectangle<float> laneArea() const;
    float xOf (double beat) const;
    void paintVelocity (juce::Point<float> from, juce::Point<float> to);
    UiContext& ctx;
    MidiClip clip;
    bool loaded = false, edited = false;
    int lowNote = 48, highNote = 72;
    juce::Point<float> lastDrag;
    int hoverNote = -1;
};

// Botón que se arrastra: lleva un archivo (MIDI o audio) a Ableton, FL Studio o el escritorio
class FileDragChip : public juce::Component, public juce::SettableTooltipClient
{
public:
    FileDragChip (UiContext& c, juce::String text, juce::String icon);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<juce::File()> getFile;
    std::function<void()> onClickWithoutFile;   // por ejemplo, pasar a audio
    juce::String text, icon, textWithoutFile;
    float progress = -1.0f;                      // 0..1 mientras se hace el archivo

private:
    UiContext& ctx;
    bool down = false, dragging = false;
};

class RecordingsPanel : public juce::Component, private juce::Timer
{
public:
    RecordingsPanel (UiContext& c, Prefs& p, MidiRecorder& r, MetronomeSettings& ms, std::function<void()> settingsChanged);
    ~RecordingsPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshList (bool selectNewest = false);

    std::function<void (const juce::File&)> onPlay;
    std::function<void()> onStopPlay;
    std::function<bool()> isPlaying;
    std::function<void (const juce::File&)> onEdit;   // abrir en el piano roll
    std::function<void()> onNew;                      // MIDI nuevo vacío

private:
    struct Row { juce::File file; juce::String title, info; bool hasAudio = false; };
    class List;
    void timerCallback() override;
    void select (int i);
    void toAudio (const juce::File& midi);
    juce::File selectedFile() const;
    UiContext& ctx;
    Prefs& prefs;
    MidiRecorder& rec;
    MetronomeSettings& metro;
    std::function<void()> changed;

    PushButton recBtn, folderBtn, newBtn, editBtn;
    Switch countSw, mouseSw, audioSw;
    FileDragChip midiChip, audioChip;
    std::vector<Row> rows;
    int selected = -1;
    std::unique_ptr<List> list;
    juce::Viewport viewport;
    ClipView clipView;
    juce::String statusText;
    juce::Time lastFolderTime;
    int lastCount = -1;
    bool wasBouncing = false;
};

//==============================================================================
// Tus instrumentos (solo en la app sola)
class PluginsPanel : public juce::Component, private juce::Timer
{
public:
    PluginsPanel (UiContext& c, PluginHost& h, std::function<void()> openWindow, std::function<void()> backToPiano);
    ~PluginsPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    class List;
    void timerCallback() override;
    UiContext& ctx;
    PluginHost& host;
    std::function<void()> openWindow, backToPiano;
    PushButton scanBtn, openBtn, pianoBtn;
    juce::TextEditor filter;
    std::unique_ptr<List> list;
    juce::Viewport viewport;
    juce::String message;
    bool wasScanning = false, scanRequested = false;
    int tick = 0;
};
