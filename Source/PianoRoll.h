#pragma once
#include <map>

#include "Views.h"

//==============================================================================
// Piano roll para editar los .mid de MidiCat, con los mismos movimientos que Ableton:
// flecha por defecto, lápiz con B, doble clic pone o quita notas, las orillas alargan,
// Alt/Ctrl + arrastrar copia, Cmd/Alt + arrastrar cambia la fuerza, Cmd+D duplica,
// Cmd+E divide, Cmd+U cuantiza, Cmd+1/2/3/4 cambian la rejilla.
class PianoRoll : public juce::Component,
                  private juce::ScrollBar::Listener,
                  private juce::Timer,
                  private juce::HighResolutionTimer
{
public:
    PianoRoll (UiContext& c, juce::MidiKeyboardState& keyboard);
    ~PianoRoll() override;

    bool loadFile (const juce::File&);
    const juce::File& getFile() const { return file; }
    std::function<void()> onSaved;
    std::function<void (const juce::File&)> onConvertToAudio;
    std::function<float()> audioProgress;   // 0..1 mientras se pasa a audio este archivo; -1 si no
    std::function<void (bool accent)> onMetroClick;   // un clic del metrónomo (lo toca el procesador)
    void setStatus (const juce::String& s) { status = s; repaint (0, 0, getWidth(), 64); }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scaleFactor) override;
    bool keyPressed (const juce::KeyPress&) override;

    struct Note { int note = 60; double start = 0.0, length = 0.25; int velocity = 100; bool selected = false; };

private:
    enum Drag { none, moving, resizeEnd, resizeStart, marquee, drawing, velocityLane, noteVelocity, panning, keysPreview, ruler };

    // Archivo
    void save();
    void commit();           // guarda y deja el paso para deshacer
    void undo();
    void redo();

    // Edición
    bool anySelected() const;
    void deleteSelected();
    void selectAll (bool sel);
    void copySelected (bool cut);
    void paste();
    void duplicate();
    void quantize();
    void splitAt (double beat, bool onlySelected);
    void transpose (int semis);
    void nudge (int steps);
    void stretch (int steps);
    void setDraw (bool on);
    void showSnapMenu();
    void showContextMenu (int noteIndex, double beat);
    void addNote (int pitch, double start);
    int noteAt (juce::Point<float> p, int& edge) const;   // edge: -1 inicio, 0 cuerpo, 1 final
    double grid() const;          // tamaño de la rejilla en tiempos (0 = sin ajuste)
    double gridOrDefault() const;
    double snapRound (double b) const;
    double snapFloor (double b) const;
    double totalBeats() const;
    juce::String gridName() const;
    void previewPitch (int note, int vel);
    void updateScrollBars();
    void clampView();
    void togglePlay();
    void stopPlay();

    // Geometría
    juce::Rectangle<float> gridArea() const;
    juce::Rectangle<float> keysArea() const;
    juce::Rectangle<float> rulerArea() const;
    juce::Rectangle<float> laneArea() const;
    float xOf (double beat) const;
    double beatOf (float x) const;
    float yOf (int note) const;
    int noteOfY (float y) const;

    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;
    void timerCallback() override;
    void hiResTimerCallback() override;

    UiContext& ctx;
    juce::MidiKeyboardState& kb;
    juce::File file;
    std::vector<Note> notes;
    struct Other { double beat; juce::MidiMessage msg; };
    std::vector<Other> others;   // pedal y demás (se conservan)
    double bpm = 120.0;
    int beatsPerBar = 4;

    std::vector<std::vector<Note>> undoStack, redoStack;
    static std::vector<Note> clipboard;

    bool drawMode = false;
    int gridIndex = 1;        // 1/16
    bool triplets = false, snapOn = true;
    int lastVelocity = 100;
    double insertBeat = 0.0;

    // Vista (con decimales para que se mueva suave)
    double scrollBeat = 0.0;
    double topRow = 84.0;
    float pxPerBeat = 80.0f;
    float rowH = 16.0f;

    // Arrastre
    Drag drag = none;
    int dragNote = -1;
    juce::Point<float> downPos, lastPos;
    std::vector<Note> dragOrig, preEdit;
    bool changed = false, fineDecided = false, fineMode = false;
    juce::Rectangle<float> marqueeRect;
    int drawPitch = -1;
    int keyRoot = -1, keyScale = 0;   // tonalidad guardada en el archivo
    std::map<long long, size_t> strokeCells;   // casillas que ya pintó este trazo del lápiz
    int ghostPitch = -1;                         // dónde caería la nota del lápiz
    double ghostBeat = 0.0;
    void mouseExit (const juce::MouseEvent&) override { if (ghostPitch >= 0) { ghostPitch = -1; repaint(); } }
    double rulerAnchorBeat = 0.0;
    float rulerStartPx = 80.0f;
    int previewNote = -1;
    double previewOffAt = 0.0;

    // Reproducir
    struct Ev { double t; int note; float vel; bool on; };
    std::vector<Ev> playEvents;
    size_t playNext = 0;
    std::atomic<double> playStartMs { 0.0 };
    double playFromBeat = 0.0;
    std::atomic<bool> playing { false };
    std::array<int, 128> held {};
    juce::CriticalSection playLock;

    juce::String status;
    juce::Font noteFont, smallFont, rulerFont;
    PushButton drawBtn, gridBtn, quantBtn, splitBtn, dupBtn, undoBtn, redoBtn, playBtn, metroBtn;
    static inline bool metroOn = true;     // metrónomo al reproducir (se queda para la próxima vez)
    double nextClickBeat = 0.0;
    class DragOut;
    std::unique_ptr<DragOut> dragOut, audioOut;
    int audioState = -2, audioTick = 0;   // para repintar el chip de audio solo cuando cambia
    // Las barras se mueven igual que el resto del piano roll (como el trackpad)
    struct WheelBar : juce::ScrollBar
    {
        using juce::ScrollBar::ScrollBar;
        void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
        {
            if (auto* p = getParentComponent()) p->mouseWheelMove (e.getEventRelativeTo (p), w);
        }
    };
    WheelBar hbar { false }, vbar { true };
};

//==============================================================================
class PianoRollWindow : public juce::DocumentWindow, private juce::Timer
{
public:
    PianoRollWindow (UiContext& c, juce::MidiKeyboardState& kb, juce::LookAndFeel& lf);
    ~PianoRollWindow() override;
    void open (const juce::File&);
    void closeButtonPressed() override { if (onClose) onClose(); }
    void activeWindowStatusChanged() override { keepInFront(); }
    std::function<void()> onClose;
    PianoRoll& roll() { return *rollComp; }

private:
    void timerCallback() override { keepInFront(); }
    void keepInFront();   // en pantalla completa (o casi), por encima de la ventana del plugin
    PianoRoll* rollComp = nullptr;
    bool raised = false;
};
