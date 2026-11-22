#pragma once

#include <JuceHeader.h>
#include "Theory.h"
#include "Style.h"

//==============================================================================
// Todo lo que la interfaz necesita saber en cada cuadro.
struct Vis
{
    std::array<uint8_t, 128> down {}, sus {};
    std::array<float, 128> flash {};
    std::vector<int> active;          // notas sonando (teclas abajo + pedal), ordenadas
    uint16_t activeMask = 0;
    int lowest = -1;
    bool pedal = false;

    int keyRoot = 0;
    int scale = 0;
    bool autoKey = true;
    bool autoHasData = false;
    theory::KeySig sig;
    uint16_t scaleMask = 0;
    bool highlight = true;

    theory::Chord chord;
    theory::Degree degree;
    std::vector<theory::Diatonic> diatonic;

    bool latin = false;
    bool sevenths = false;
    Palette pal;
};

struct UiContext
{
    Vis& vis;
    Fonts& fonts;
};

//==============================================================================
namespace draw
{
    void plate  (juce::Graphics&, juce::Rectangle<float>, const Palette&, float radius = 8.0f);
    void well   (juce::Graphics&, juce::Rectangle<float>, const Palette&, float radius = 6.0f);
    void screen (juce::Graphics&, juce::Rectangle<float>, const Palette&, float radius = 8.0f);
    void caption (juce::Graphics&, juce::Rectangle<float> area, const juce::String& text, juce::Colour, const Fonts&);
    void stars (juce::Graphics&, juce::Rectangle<float>, int count, int seed, juce::Colour tint);
    void led (juce::Graphics&, juce::Point<float> c, float r, juce::Colour, bool on);
}

//==============================================================================
class StaffView : public juce::Component
{
public:
    explicit StaffView (UiContext& c) : ctx (c) {}
    void paint (juce::Graphics&) override;

private:
    UiContext& ctx;
};

//==============================================================================
class ChordScreen : public juce::Component
{
public:
    explicit ChordScreen (UiContext& c) : ctx (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    std::function<void (int index)> onChipClicked;

private:
    juce::Rectangle<float> chipArea() const;
    juce::Rectangle<float> screenArea() const;
    juce::Rectangle<float> chipBounds (int i, int count) const;
    UiContext& ctx;
    int hoverChip = -1;
};

//==============================================================================
class CircleView : public juce::Component
{
public:
    explicit CircleView (UiContext& c) : ctx (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    std::function<void (int rootPc, bool minor)> onKeyClicked;

private:
    bool hitTestRing (juce::Point<float> p, int& index, bool& minor) const;
    void geometry (juce::Point<float>& c, float& r) const;
    UiContext& ctx;
    int hoverIndex = -1;
    bool hoverMinor = false;
};

//==============================================================================
class KeyboardView : public juce::Component
{
public:
    explicit KeyboardView (UiContext& c) : ctx (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    std::function<void (int note, float velocity, bool on)> onNote;

    static constexpr int lowNote = 21, highNote = 108;

private:
    struct KeyGeom { juce::Rectangle<float> r; bool black; };
    KeyGeom keyRect (int note) const;
    int noteAt (juce::Point<float> p, float& velocity) const;
    juce::Rectangle<float> keysArea() const;
    UiContext& ctx;
    int mouseNote = -1;
};

//==============================================================================
// Controles propios (más fáciles de pintar a la medida que los de JUCE).
class Switch : public juce::Component
{
public:
    Switch (UiContext& c, juce::String label) : ctx (c), text (std::move (label)) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setOn (bool v) { if (on != v) { on = v; repaint(); } }
    bool isOn() const { return on; }
    std::function<void (bool)> onChange;

private:
    UiContext& ctx;
    juce::String text;
    bool on = false;
};

class Knob : public juce::Component
{
public:
    Knob (UiContext& c, juce::String label) : ctx (c), text (std::move (label)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    void setValue (float normalised, juce::String display);
    std::function<void (float)> onValue;        // 0..1
    std::function<void()> onGestureStart, onGestureEnd, onReset;

private:
    UiContext& ctx;
    juce::String text, valueText;
    float value = 0.5f, dragStart = 0.5f;
};

class DropButton : public juce::Component
{
public:
    DropButton (UiContext& c, juce::String label) : ctx (c), text (std::move (label)) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }
    void setValueText (const juce::String& v) { if (v != value) { value = v; repaint(); } }
    std::function<void()> onClick;

private:
    UiContext& ctx;
    juce::String text, value;
    bool hover = false;
};

class IconButton : public juce::Component
{
public:
    enum class Icon { sun, moon, menu, palette };
    IconButton (UiContext& c, Icon i) : ctx (c), icon (i) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }
    void setIcon (Icon i) { icon = i; repaint(); }
    std::function<void()> onClick;

private:
    UiContext& ctx;
    Icon icon;
    bool hover = false;
};

class Segmented : public juce::Component
{
public:
    Segmented (UiContext& c, juce::String label, juce::StringArray opts) : ctx (c), text (std::move (label)), options (std::move (opts)) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setSelected (int i) { if (i != selected) { selected = i; repaint(); } }
    std::function<void (int)> onChange;

private:
    juce::Rectangle<float> segArea() const;
    UiContext& ctx;
    juce::String text;
    juce::StringArray options;
    int selected = 0;
};

//==============================================================================
class TopBar : public juce::Component
{
public:
    explicit TopBar (UiContext& c);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

    void setMidiStatus (bool connected, bool activity, const juce::String& text, bool clickable);

    DropButton keyButton, scaleButton;
    IconButton modeButton, menuButton;
    std::function<void()> onMidiClicked;

private:
    juce::Rectangle<float> midiArea() const;
    UiContext& ctx;
    bool midiConnected = false, midiActivity = false, midiClickable = false;
    juce::String midiText;
};

class ControlStrip : public juce::Component
{
public:
    explicit ControlStrip (UiContext& c);
    void paint (juce::Graphics&) override;
    void resized() override;

    Switch highlight, pianoSound;
    IconButton skinButton;
    Knob volume;
    Segmented names;

private:
    UiContext& ctx;
    std::array<float, 3> dividers {};
};

//==============================================================================
class SkinPicker : public juce::Component
{
public:
    SkinPicker (UiContext& c, int current);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    std::function<void (int)> onPick;

private:
    juce::Rectangle<float> cell (int i) const;
    UiContext& ctx;
    int current = 0, hover = -1;
};
