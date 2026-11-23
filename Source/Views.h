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
    std::array<float, 128> level {};     // brillo animado de cada tecla (0..1)
    std::array<float, 12> pcGlow {};     // brillo animado de cada nota en el círculo
    std::vector<int> active;             // el acorde de ahora (lo que se nombra), ordenado
    std::vector<int> sounding;           // todo lo que suena (teclas abajo + pedal), ordenado
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
    int activeDegree = -1;
    std::array<float, 7> degreeGlow {};

    // Animaciones
    juce::String bigText, prevBigText;
    float bigFade = 1.0f;
    double time = 0.0;                   // segundos (para lo que se mueve solo)
    float knobAngle = 0.0f;              // la perilla del círculo apunta a la tonalidad
    float viewWhite = 28.0f;             // primera tecla blanca visible (animada al cambiar de octava)
    float viewWhites = 22.0f;            // cuántas teclas blancas caben (animado al cambiar de octavas)

    bool latin = false;
    bool sevenths = false;
    bool hideParams = false;
    bool transparent = false;
    Skin pal;
};

struct UiContext
{
    Vis& vis;
    Fonts& fonts;
};

//==============================================================================
namespace draw
{
    float textWidth (const juce::Font&, const juce::String&);
    juce::Font fitUi (const Fonts&, const juce::String&, float maxH, float maxW, int weight);

    // El aparato: cuerpo con esquinas redondas, bisel y sombra
    juce::Path devicePath (juce::Rectangle<float>, const Skin&, float k);
    void device (juce::Graphics&, juce::Rectangle<float>, const Skin&, float k, bool backdrop);
    // Botón físico (press 0..1 lo hunde)
    void button (juce::Graphics&, juce::Rectangle<float>, const Skin&, float radius, float press, bool active, float hover = 0.0f);
    // Zona hundida en el cuerpo (placa del círculo, cama del teclado)
    void recess (juce::Graphics&, juce::Rectangle<float>, const Skin&, float radius, juce::Colour fill);
    // Tarjeta levantada (pentagrama)
    void card (juce::Graphics&, juce::Rectangle<float>, const Skin&, float radius);
    // Pantalla negra con marco
    void screen (juce::Graphics&, juce::Rectangle<float>, const Skin&, float k, int shape = 0);
    // Formas: 0 redondeada, 1 recta, 2 pastilla, 3 óvalo, 4 biselada, 5 muy redonda
    juce::Path shapePath (juce::Rectangle<float>, int kind, float radius);
    void recessShape (juce::Graphics&, const juce::Path&, const Skin&, juce::Colour fill);
    void cardShape (juce::Graphics&, const juce::Path&, const Skin&);
    // Perilla con tapa de color
    void knobBody (juce::Graphics&, juce::Point<float> c, float r, const Skin&, juce::Colour cap);

    void icon (juce::Graphics&, const juce::String& name, juce::Rectangle<float>, juce::Colour, float stroke = 1.7f);
    // Logo: conector MIDI con cara de gato
    void catLogo (juce::Graphics&, juce::Rectangle<float>, juce::Colour line, juce::Colour eyes);
    void shadowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, juce::Colour, bool shadow);
}

//==============================================================================
// Ajustes de movimiento de toda la interfaz (los pone el editor según tus preferencias)
struct MotionSettings
{
    static inline float speed = 1.0f;     // 60 = sin animaciones (todo al instante)
    static inline bool panels = true;     // los paneles aparecen con transición
};

// Ayuda para animar: corre a 60 cuadros por segundo solo mientras algo se mueve.
class Animated : private juce::Timer
{
public:
    ~Animated() override { stopTimer(); }
protected:
    void animate() { if (! isTimerRunning()) startTimerHz (60); }
    virtual bool stepAnimation (float dt) = 0;   // true = sigue moviéndose
private:
    void timerCallback() override { if (! stepAnimation (juce::jmin (1.0f, MotionSettings::speed / 60.0f))) stopTimer(); }
};

static inline float approach (float v, float target, float rate, float dt)
{
    const float a = 1.0f - std::pow (1.0f - rate, dt * 60.0f);
    return std::abs (target - v) < 0.002f ? target : v + (target - v) * a;
}

//==============================================================================
class Switch : public juce::Component, private Animated
{
public:
    explicit Switch (UiContext& c) : ctx (c) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setOn (bool v);
    std::function<void (bool)> onChange;

private:
    bool stepAnimation (float dt) override;
    UiContext& ctx;
    bool on = false;
    float pos = 0.0f;
};

class Knob : public juce::Component, private Animated
{
public:
    explicit Knob (UiContext& c) : ctx (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void setValue (float normalised, juce::String display);
    std::function<void (float)> onValue;
    std::function<void()> onGestureStart, onGestureEnd, onReset;
    juce::String valueText;
    int capIndex = 1;

private:
    bool stepAnimation (float dt) override;
    UiContext& ctx;
    float value = 0.5f, shown = 0.5f, dragStart = 0.5f;
    bool dragging = false;
};

class Segmented : public juce::Component, private Animated
{
public:
    Segmented (UiContext& c, juce::StringArray opts) : ctx (c), options (std::move (opts)) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setSelected (int i);
    int getSelected() const { return selected; }
    std::function<void (int)> onChange;
    float fontScale = 1.0f;

private:
    bool stepAnimation (float dt) override;
    UiContext& ctx;
    juce::StringArray options;
    int selected = 0;
    float pos = 0.0f;
};

// Radio de las esquinas según la forma de botones elegida (normal = la proporción de siempre)
float shapedRadius (const Skin&, float normal);

// Botón físico con ícono y/o texto, que se hunde al presionarlo
class PushButton : public juce::Component, public juce::SettableTooltipClient, private Animated
{
public:
    PushButton (UiContext& c, juce::String iconName, juce::String tip, juce::String text = {});
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hoverT = 1.0f; animate(); }
    void mouseExit (const juce::MouseEvent&) override  { hoverT = 0.0f; animate(); }
    void setIcon (const juce::String& n) { if (n != iconId) { iconId = n; repaint(); } }
    void setActive (bool a) { if (a != active) { active = a; repaint(); } }
    void setLabel (const juce::String& t) { if (t != label) { label = t; repaint(); } }
    bool isActive() const { return active; }
    std::function<void()> onClick;
    bool framed = true;          // false: sin botón (solo ícono), para barras
    bool lightOnDark = false;    // sobre fondos transparentes
    juce::Colour swatch;         // un puntito de color junto al texto
    float radiusScale = 0.3f;
    std::function<void()> onMore;    // flechita de ajustes (abajo a la derecha, o clic derecho)
    juce::Colour activeColour;       // color cuando está prendido (si no, el acento)
    juce::Colour iconColour;         // color del ícono cuando está apagado (si no, el normal)
    void setFlash (float f) { if (std::abs (f - flash) > 0.01f) { flash = f; repaint(); } }

private:
    bool inMoreZone (juce::Point<int>) const;
    float flash = 0.0f;
    bool stepAnimation (float dt) override;
    UiContext& ctx;
    juce::String iconId, label;
    bool active = false, down = false;
    float press = 0.0f, hover = 0.0f, hoverT = 0.0f;
};

// Botón con etiqueta pequeña y valor (MIDI, Tonalidad, Escala, Latencia)
class FieldButton : public juce::Component, private Animated
{
public:
    FieldButton (UiContext& c, juce::String label) : ctx (c), text (std::move (label)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { down = true; animate(); }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override;
    void setValueText (const juce::String& v) { if (v != value) { value = v; repaint(); } }
    void setLed (int mode) { if (mode != led) { led = mode; repaint(); } } // -1 sin led, 0 apagado, 1 conectado, 2 actividad
    std::function<void()> onClick;
    float k = 1.0f;

private:
    bool stepAnimation (float dt) override;
    UiContext& ctx;
    juce::String text, value;
    int led = -1;
    bool down = false;
    float press = 0.0f;
};

//==============================================================================
class RailBar : public juce::Component
{
public:
    explicit RailBar (UiContext& c);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setShowReset (bool s) { if (s != showReset) { showReset = s; resized(); } }

    FieldButton midi, key, scale, latency;
    PushButton recBtn, metroBtn;
    PushButton dayBtn, colourBtn, transpBtn, splitBtn, windowsBtn, resetBtn, hideBtn, menuBtn;
    float k = 1.0f;

private:
    UiContext& ctx;
    bool showReset = false;
    juce::Rectangle<float> logoArea;
    juce::Array<juce::Rectangle<float>> trays;
};

//==============================================================================
// Panel que se abre encima de la interfaz. Se cierra con la X, con Esc o con un clic afuera.
class Popover : public juce::Component
{
public:
    Popover (UiContext& c, const juce::String& title, std::unique_ptr<juce::Component> content, juce::Rectangle<int> anchor);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void parentHierarchyChanged() override { resized(); }
    std::function<void()> onClose;
    juce::String anchorId;
    float dimAmount = 1.0f;   // qué tanto se oscurece lo de atrás (0 = nada, para ver los cambios en vivo)
    void setAvoid (juce::Rectangle<int> r) { avoid = r; userOffset = {}; resized(); repaint(); }

private:
    void requestClose();
    juce::Rectangle<int> avoid;
    juce::Point<int> userOffset, dragStartOffset;
    bool draggingPanel = false;
    UiContext& ctx;
    juce::String title;
    std::unique_ptr<juce::Component> content;
    juce::Rectangle<int> anchor;
    juce::Rectangle<int> panel;
    PushButton closeBtn;
};

//==============================================================================
// Selector de color: muestras rápidas + selector completo
class ColourPicker : public juce::Component, private juce::ChangeListener
{
public:
    explicit ColourPicker (UiContext& c);
    ~ColourPicker() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void setColour (juce::Colour c);
    std::function<void (juce::Colour)> onColour;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    juce::Rectangle<float> swatchRect (int i) const;
    UiContext& ctx;
    juce::ColourSelector selector { juce::ColourSelector::showColourspace | juce::ColourSelector::showSliders, 4, 0 };
    juce::Colour current;
    bool settingProgrammatically = false;
};

// Color de toda la interfaz: estilo, día/noche, tono y acento
class InterfacePanel : public juce::Component
{
public:
    InterfacePanel (UiContext& c, Prefs& p);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> lookTile (int i) const;
    juce::Rectangle<float> paletteChip (int i) const;
    juce::Rectangle<float> themeChip (int i) const;
    void refreshPicker();
    void setNaming (bool n);
    UiContext& ctx;
    Prefs& prefs;
    Segmented mode, target;
    ColourPicker picker;
    PushButton resetBtn, saveThemeBtn, confirmBtn, cancelBtn;
    juce::TextEditor nameEditor;
    bool naming = false;
    int confirmDelete = -1;
    juce::Rectangle<int> looksArea, chipsArea;
};

// Color y estilo de las teclas
class KeysPanel : public juce::Component
{
public:
    KeysPanel (UiContext& c, Prefs& p);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void refreshPicker();
    UiContext& ctx;
    Prefs& prefs;
    Segmented style, target;
    Switch rainbow;
    ColourPicker picker;
    PushButton resetBtn;
    juce::Rectangle<int> rainbowRow;
};

class LatencyPanel : public juce::Component
{
public:
    LatencyPanel (UiContext& c, juce::RangedAudioParameter& p, bool standalone,
                  std::function<juce::String()> deviceInfo, std::function<void (int)> setBuffer);
    ~LatencyPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    UiContext& ctx;
    juce::Slider slider;
    std::unique_ptr<juce::SliderParameterAttachment> attach;
    bool standalone;
    std::function<juce::String()> deviceInfo;
    std::function<void (int)> setBuffer;
    juce::OwnedArray<PushButton> bufferButtons;
};

class TextPanel : public juce::Component
{
public:
    TextPanel (UiContext& c, juce::String text, int width);
    void paint (juce::Graphics&) override;

private:
    UiContext& ctx;
    juce::String text;
};
