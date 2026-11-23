#pragma once

#include "Views.h"

//==============================================================================
// Piano roll estilo FL Studio para editar los .mid de Midi Cat
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

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

    struct Note { int note = 60; double start = 0.0, length = 0.25; int velocity = 100; bool selected = false; };

private:
    enum Tool { draw, select, slice, erase };
    enum Drag { none, moving, resizing, marquee, erasing, slicing, velocity, panning, keysPreview };

    // Archivo
    void save();
    void pushUndo();
    void undo();
    void redo();

    // Edición
    void deleteSelected();
    void selectAll (bool sel);
    void copySelected (bool cut);
    void paste();
    void duplicate();
    void quantize();
    void transpose (int semis);
    void nudge (int steps);
    void setTool (Tool t);
    void showSnapMenu();
    int noteAt (juce::Point<float> p, bool& nearEnd) const;
    double snapValue() const;
    double snapRound (double b) const;
    double snapFloor (double b) const;
    double totalBeats() const;
    juce::String snapName() const;
    void previewPitch (int note, int vel);
    void updateScrollBars();
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

    Tool tool = draw;
    int snapIndex = 2;
    double lastLength = 1.0;
    int lastVelocity = 100;
    double cursorBeat = 0.0;

    // Vista
    double scrollBeat = 0.0;
    float pxPerBeat = 80.0f;
    float rowH = 16.0f;
    int topNote = 84;

    // Arrastre
    Drag drag = none;
    int dragNote = -1;
    juce::Point<float> downPos, lastPos;
    std::vector<Note> dragOrig, preEdit;
    bool changed = false;
    juce::Rectangle<float> marqueeRect;
    int previewNote = -1;
    double previewOffAt = 0.0;

    // Reproducción
    struct Ev { double t; int note; float vel; bool on; };
    std::vector<Ev> playEvents;
    size_t playNext = 0;
    std::atomic<double> playStartMs { 0.0 };
    double playFromBeat = 0.0;
    std::atomic<bool> playing { false };
    std::array<int, 128> held {};
    juce::CriticalSection playLock;

    juce::String status;
    PushButton drawBtn, selectBtn, sliceBtn, eraseBtn, snapBtn, quantBtn, undoBtn, redoBtn, playBtn;
    class DragOut;
    std::unique_ptr<DragOut> dragOut;
    juce::ScrollBar hbar { false }, vbar { true };
};

//==============================================================================
class PianoRollWindow : public juce::DocumentWindow
{
public:
    PianoRollWindow (UiContext& c, juce::MidiKeyboardState& kb, juce::LookAndFeel& lf);
    ~PianoRollWindow() override;
    void open (const juce::File&);
    void closeButtonPressed() override { if (onClose) onClose(); }
    std::function<void()> onClose;
    PianoRoll& roll() { return *rollComp; }

private:
    PianoRoll* rollComp = nullptr;
};
