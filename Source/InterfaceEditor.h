#pragma once

#include "Pieces.h"

//==============================================================================
// Personalizar: todo en un solo lugar (estilo, teclas, pantalla, grados, círculo,
// pentagrama, formas y movimiento). Cada cambio se ve al instante.
//==============================================================================

// Lo que cambia de alto según el ancho (para acomodarse en la lista)
struct HeightForWidth
{
    virtual ~HeightForWidth() = default;
    virtual int heightFor (int width) = 0;
};

// Opciones en pastillas (una elegida). Se acomodan solas en renglones.
class ChipGroup : public juce::Component
{
public:
    ChipGroup (UiContext& c, juce::StringArray n) : ctx (c), names (std::move (n)) {}
    std::function<void (int)> onChange;
    std::function<juce::Font (int index, float height)> fontFor;                     // letra de muestra
    std::function<void (juce::Graphics&, juce::Rectangle<float>, int)> decorate;    // puntitos de color
    float decorateW = 0.0f;
    void setSelected (int i) { if (i != selected) { selected = i; repaint(); } }
    int heightFor (int width);
    void resized() override { layout (getWidth()); }
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseUp (const juce::MouseEvent&) override;

private:
    void layout (int width);
    UiContext& ctx;
    juce::StringArray names;
    std::vector<juce::Rectangle<float>> rects;
    int selected = -1, hover = -1;
};

// Deslizador fino (Shift = más fino, doble clic = valor normal)
class MiniSlider : public juce::Component
{
public:
    MiniSlider (UiContext& c, float lo, float hi, float def, std::function<juce::String (float)> fmt)
        : ctx (c), minV (lo), maxV (hi), defV (def), format (std::move (fmt)) {}
    void setValue (float v) { value = juce::jlimit (minV, maxV, v); repaint(); }
    std::function<void (float)> onChange;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Rectangle<float> track() const;
    void set (float v);
    UiContext& ctx;
    float minV, maxV, defV, value = 0.0f, dragStartV = 0.0f;
    std::function<juce::String (float)> format;
};

// Número que se arrastra (arriba/abajo), con la rueda o escribiéndolo (doble clic)
class NumberBox : public juce::Component
{
public:
    NumberBox (UiContext& c, juce::String l, int lo, int hi) : ctx (c), label (std::move (l)), minV (lo), maxV (hi) {}
    void setValue (int v) { value = juce::jlimit (minV, maxV, v); repaint(); }
    std::function<void (int)> onChange;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void set (int v);
    UiContext& ctx;
    juce::String label;
    int minV, maxV, value = 0, dragStartV = 0;
    std::unique_ptr<juce::TextEditor> editor;
};

// Selector de color preciso: cuadro de saturación y brillo, tira de tono, hex y RGB
class PrecisePicker : public juce::Component
{
public:
    explicit PrecisePicker (UiContext& c);
    void setColour (juce::Colour c);
    std::function<void (juce::Colour)> onColour;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void fromHSV (bool notify);
    void syncFields();
    juce::Rectangle<float> swatchRect (int i) const;
    UiContext& ctx;
    juce::Colour current, original;
    float hue = 0.0f, sat = 0.0f, val = 1.0f;
    juce::Rectangle<float> svArea, hueArea, prevArea;
    juce::Image svImage;
    float svImageHue = -1.0f;
    int dragging = 0;   // 1 cuadro, 2 tono
    juce::Point<float> dragStart;
    float startH = 0.0f, startS = 0.0f, startV = 0.0f;
    juce::TextEditor hex;
    NumberBox rBox, gBox, bBox;
};

// Varios colores que se editan con el mismo selector (por ejemplo: letras y fondo)
struct ColourTarget
{
    juce::String name;
    std::function<juce::Colour()> get, base;   // transparente = el del diseño (base)
    std::function<void (juce::Colour)> set;
};

class ColourEditor : public juce::Component
{
public:
    ColourEditor (UiContext& c, std::vector<ColourTarget> t, juce::String resetText, std::function<void()> onReset);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    UiContext& ctx;
    std::vector<ColourTarget> targets;
    Segmented tabs;
    PrecisePicker picker;
    PushButton resetBtn;
    std::function<void()> resetFn;
};

// Lista de opciones con su título (se desplaza si no cabe)
class OptionsList : public juce::Component
{
public:
    explicit OptionsList (UiContext& c) : ctx (c) {}
    ChipGroup& chips (const juce::String& label, juce::StringArray names, int selected, std::function<void (int)>);
    MiniSlider& slider (const juce::String& label, float lo, float hi, float def, float value,
                        std::function<juce::String (float)> fmt, std::function<void (float)>);
    Switch& toggle (const juce::String& label, bool on, std::function<void (bool)>);
    void custom (const juce::String& label, std::unique_ptr<juce::Component>, int height);
    int layout (int width);
    void paint (juce::Graphics&) override;

private:
    struct Row { juce::String label; std::unique_ptr<juce::Component> comp; int height = 0; int kind = 0; juce::Rectangle<int> labelArea; };
    UiContext& ctx;
    std::vector<Row> rows;
};

//==============================================================================
// Pestañas en columna (a la izquierda del panel)
class SideTabs : public juce::Component
{
public:
    SideTabs (UiContext& c, juce::StringArray n) : ctx (c), names (std::move (n)) {}
    std::function<void (int)> onChange;
    void setSelected (int i) { selected = i; repaint(); }
    int getSelected() const { return selected; }
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> item (int i) const { return { 0.0f, (float) i * 44.0f, (float) getWidth(), 40.0f }; }
    UiContext& ctx;
    juce::StringArray names;
    int selected = 0, hover = -1;
};

class CustomizePanel : public juce::Component
{
public:
    CustomizePanel (UiContext& c, Prefs& p, std::function<void()> openDegreeInput);
    void resized() override;
    void paint (juce::Graphics&) override;
    std::function<void (int tab)> onTab;   // para acomodar el panel sin tapar la pieza que editas
    int currentTab() const { return tabs.getSelected(); }
    static juce::StringArray tabNames();
    static void setStartTab (int t);

private:
    void showTab (int i);
    void build (int i);
    UiContext& ctx;
    Prefs& prefs;
    std::function<void()> openDegrees;
    SideTabs tabs;
    std::unique_ptr<juce::Component> page;
};

//==============================================================================
// Teclas de la compu y pads MIDI para tocar los grados (no se ven en la interfaz)
class DegreeInputPanel : public juce::Component, private juce::Timer
{
public:
    DegreeInputPanel (UiContext& c, Prefs& p, std::function<int()> lastPadEvent);
    ~DegreeInputPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    static bool capturing;   // mientras se elige una tecla, las teclas no tocan grados

private:
    void timerCallback() override;
    juce::Rectangle<float> slot (int row, int kind) const;
    void listen (int row, int kind);
    UiContext& ctx;
    Prefs& prefs;
    std::function<int()> lastPad;
    Switch keysOn, padsOn;
    PushButton learnAllBtn, clearBtn, defaultsBtn;
    int listenRow = -1, listenKind = 0;   // 0 tecla, 1 pad
    bool learnAll = false;
    int lastSeenPad = 0;
};
