#pragma once

#include "Views.h"

//==============================================================================
// Una sección de MidiSky. Puede vivir dentro de la ventana principal o en su propia ventana.
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
    enum class PanelKind { atril, fall, keys };
    virtual PanelKind panelKind() const = 0;
    virtual void paintContent (juce::Graphics&, juce::Rectangle<float> area) = 0;
    virtual void layoutContent (juce::Rectangle<int>) {}
    juce::Rectangle<float> contentArea() const;
    void caption (juce::Graphics&, juce::Rectangle<float>& area, const juce::String& text);

    UiContext& ctx;
    IconButton detachBtn;
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

protected:
    PanelKind panelKind() const override { return PanelKind::atril; }
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;

private:
    bool ringAt (juce::Point<float>, int& index, bool& minor) const;
    juce::Point<float> centre;
    float radius = 100.0f;
    int hoverIndex = -1;
    bool hoverMinor = false;
};

//==============================================================================
class ScoreSection : public Section
{
public:
    explicit ScoreSection (UiContext& c);

protected:
    PanelKind panelKind() const override { return PanelKind::atril; }
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;

private:
    juce::Image paperCache;      // papel y su sombra, ya dibujados
    juce::String paperKey;
};

//==============================================================================
class TabletsSection : public Section
{
public:
    explicit TabletsSection (UiContext& c);
    std::function<void (int)> onPreview;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

protected:
    PanelKind panelKind() const override { return PanelKind::atril; }
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;

private:
    juce::Rectangle<float> tablet (int i) const;
    juce::Rectangle<float> listArea;
    int hover = -1;
};

//==============================================================================
class ChordSection : public Section
{
public:
    explicit ChordSection (UiContext& c);

protected:
    PanelKind panelKind() const override { return PanelKind::fall; }
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;
};

//==============================================================================
class KeyboardSection : public Section
{
public:
    explicit KeyboardSection (UiContext& c);

    Switch pianoSw, scaleSw;
    Knob volume;
    Segmented names;
    std::function<void (int note, float velocity, bool on)> onNote;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    static constexpr int lowNote = 21, highNote = 108;

protected:
    PanelKind panelKind() const override { return PanelKind::keys; }
    void paintContent (juce::Graphics&, juce::Rectangle<float>) override;
    void layoutContent (juce::Rectangle<int>) override;

private:
    bool showCheeks() const;
    struct KeyGeom { juce::Rectangle<float> r; bool black; };
    KeyGeom keyRect (int note) const;
    int noteAt (juce::Point<float>, float& velocity) const;
    juce::Rectangle<float> keysArea, leftCheek, rightCheek;
    int mouseNote = -1;
};
