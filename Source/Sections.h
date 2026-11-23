#pragma once

#include "Views.h"
#include "Pieces.h"

//==============================================================================
// Una sección de MidiCat. Puede vivir dentro de la ventana principal o en su propia ventana.
class Section : public juce::Component
{
public:
    Section (UiContext& c, juce::String sectionId, juce::String sectionTitle, juce::Point<int> naturalSize);

    const juce::String id, title;
    const juce::Point<int> natural;     // tamaño normal (100 %)
    bool floating = false;              // vive en su propia ventana
    float k = 1.0f;                     // escala del contenido
    std::function<void()> onDetach;

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshControls();             // mostrar u ocultar controles según el estado
    bool transparentMode() const { return floating && ctx.vis.transparent; }

protected:
    virtual void paintContent (juce::Graphics&, juce::Rectangle<float> area) = 0;
    virtual void layoutContent (juce::Rectangle<int>) {}
    virtual bool wantsDetachButton() const { return true; }
    void caption (juce::Graphics&, juce::Rectangle<float> area, const juce::String& text, juce::Colour colour);

    UiContext& ctx;
    PushButton detachBtn;
};

//==============================================================================
class ScoreSection : public Section
{
public:
    explicit ScoreSection (UiContext& c);

protected:
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;
};

//==============================================================================
// La pantalla del centro: acorde grande, grado y notas
class ScreenSection : public Section
{
public:
    explicit ScreenSection (UiContext& c);

protected:
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;

private:
    pieces::GlowText glowBig, glowSmall;
};

//==============================================================================
// Los grados (acordes de la tonalidad) debajo de la pantalla
class DegreesSection : public Section, private Animated
{
public:
    explicit DegreesSection (UiContext& c);
    std::function<void (int, float)> onPreview;   // grado y fuerza (más abajo = más fuerte, como en FL)
    std::function<void()> onRelease;              // se soltó el mouse (el acorde dura lo que lo dejes apretado)
    std::function<void (int)> onOctave;           // subir o bajar la octava de los acordes
    std::function<void()> onSettings;             // clic derecho: teclas y pads para los grados
    void setOctave (int o) { if (o != octave) { octave = o; repaint(); } }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

protected:
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;
    bool wantsDetachButton() const override { return false; }

private:
    bool stepAnimation (float dt) override;
    juce::Rectangle<float> tile (int i) const;
    juce::Rectangle<float> listArea, octArea;
    int hover = -1, pressed = -1;
    int octave = 3, octPressed = 0;
    std::array<float, 7> press {};
    pieces::GlowText glowCache;
};

//==============================================================================
class CircleSection : public Section
{
public:
    explicit CircleSection (UiContext& c);
    std::function<void (int rootPc, bool minor)> onKeyClicked;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    bool isAnimating() const { return animating; }

protected:
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;

private:
    bool ringAt (juce::Point<float>, int& index, bool& minor) const;
    void buildLoop (const std::vector<int>& idx, float rn, float knobR, std::vector<juce::Point<float>>& out) const;
    void buildStraight (const std::vector<int>& idx, float rn, std::vector<juce::Point<float>>& out) const;
    juce::Point<float> centre;
    float radius = 100.0f;
    int hoverIndex = -1;
    bool hoverMinor = false;

    // Animación de la constelación
    uint16_t shownMask = 0;
    double changedAt = -10.0, releasedAt = -10.0;
    std::vector<juce::Point<float>> lastLoop;
    bool animating = false;
};

//==============================================================================
class KeyboardSection : public Section
{
public:
    explicit KeyboardSection (UiContext& c);

    Switch pianoSw, scaleSw;
    Knob volume;
    FieldButton soundBtn;
    PushButton pedalBtn, keysColourBtn, uiColourBtn, leftBtn, rightBtn, oct2, oct3, oct4;
    std::function<void (int note, float velocity, bool on)> onNote;
    std::function<void (int direction)> onShift;
    std::function<void (int octaves)> onOctaves;

    void setRange (int startC, int octaves);
    int firstNote() const { return startC; }
    int lastNote() const { return startC + 12 * octaves; }

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

protected:
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;
    void layoutContent (juce::Rectangle<int>) override;

private:
    struct KeyGeom { juce::Rectangle<float> r; bool black; };
    KeyGeom keyRect (int note) const;
    int noteAt (juce::Point<float>, float& velocity) const;
    bool showCheeks() const;
    void drawKey (juce::Graphics&, int note, const KeyGeom&) const;
    juce::Rectangle<float> keysArea, leftCheek, rightCheek, rangeDisplay;
    int startC = 48, octaves = 3;
    int mouseNote = -1;
};
