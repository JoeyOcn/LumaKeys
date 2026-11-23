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
    std::vector<int> active;             // notas sonando (teclas abajo + pedal), ordenadas
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

    // Animación del nombre del acorde
    juce::String bigText, prevBigText;
    float bigFade = 1.0f;

    bool latin = false;
    bool sevenths = false;
    bool hideParams = false;
    bool transparent = false;
    Palette pal;
};

struct UiContext
{
    Vis& vis;
    Fonts& fonts;
    Textures& tex;
};

//==============================================================================
namespace draw
{
    // Panel con la forma de la skin (redondo, pixel, esquinas cortadas, doble filete)
    void panel (juce::Graphics&, juce::Rectangle<float>, juce::Colour fill, juce::Colour border, const Palette&, float k, float radiusScale = 1.0f);
    juce::Path panelPath (juce::Rectangle<float>, const Palette&, float k, float radiusScale = 1.0f);
    void body (juce::Graphics&, juce::Rectangle<float>, const Palette&, Textures&);
    void texture (juce::Graphics&, const juce::Image&, juce::Rectangle<float>, float alpha = 1.0f);
    void themeLight (juce::Graphics&, juce::Rectangle<float>, const Palette&);   // luz y partículas (encima)
    void themeScene (juce::Graphics&, juce::Rectangle<float>, const Palette&);   // paisaje, sol o luna (al fondo)
    // Marco de la ventana con esquinas redondas, bisel y sombra interior
    juce::Path chassisPath (juce::Rectangle<float>, const Palette&, float k);
    void chassis (juce::Graphics&, juce::Rectangle<float>, const Palette&, float k, Textures&, bool backdrop);
    void shadowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, juce::Colour, bool shadow);
    void icon (juce::Graphics&, const juce::String& name, juce::Rectangle<float>, juce::Colour, float stroke = 1.6f);
    float textWidth (const juce::Font&, const juce::String&);
    juce::Font fit (const Fonts&, FontKind, const juce::String&, float maxH, float maxW, bool display);
}

// La luz del tema ya dibujada, para no recalcularla en cada cuadro.
struct ThemeCache
{
    void draw (juce::Graphics&, juce::Rectangle<float>, const Palette&, float scale);
    juce::Image image;
    juce::String key;
};

//==============================================================================
// Controles propios
class Switch : public juce::Component
{
public:
    explicit Switch (UiContext& c) : ctx (c) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setOn (bool v) { if (on != v) { on = v; repaint(); } }
    std::function<void (bool)> onChange;

private:
    UiContext& ctx;
    bool on = false;
};

class Knob : public juce::Component
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

private:
    UiContext& ctx;
    float value = 0.5f, dragStart = 0.5f;
};

class Segmented : public juce::Component
{
public:
    Segmented (UiContext& c, juce::StringArray opts) : ctx (c), options (std::move (opts)) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setSelected (int i) { if (i != selected) { selected = i; repaint(); } }
    std::function<void (int)> onChange;

private:
    UiContext& ctx;
    juce::StringArray options;
    int selected = 0;
};

class IconButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    IconButton (UiContext& c, juce::String iconName, juce::String tip) : ctx (c), iconId (std::move (iconName)) { setTooltip (tip); setTitle (tip); }
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }
    void setIcon (const juce::String& n) { if (n != iconId) { iconId = n; repaint(); } }
    void setActive (bool a) { if (a != active) { active = a; repaint(); } }
    std::function<void()> onClick;
    juce::String label;          // texto junto al ícono (si cabe)
    bool framed = true;
    bool lightOnDark = false;   // para barras sobre fondos transparentes

private:
    UiContext& ctx;
    juce::String iconId;
    bool hover = false, active = false;
};

class FieldButton : public juce::Component
{
public:
    FieldButton (UiContext& c, juce::String label) : ctx (c), text (std::move (label)) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }
    void setValueText (const juce::String& v) { if (v != value) { value = v; repaint(); } }
    void setLed (int mode) { if (mode != led) { led = mode; repaint(); } } // -1 sin led, 0 apagado, 1 conectado, 2 actividad
    std::function<void()> onClick;
    float k = 1.0f;

private:
    UiContext& ctx;
    juce::String text, value;
    bool hover = false;
    int led = -1;
};

//==============================================================================
class RailBar : public juce::Component
{
public:
    explicit RailBar (UiContext& c);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setStatus (const juce::String& s, double seconds = 4.0);
    void tick (double nowMs);

    FieldButton midi, key, scale, latency;
    IconButton dayBtn, themeBtn, skinBtn, transpBtn, splitBtn, windowsBtn, resetBtn, hideBtn, menuBtn;
    float k = 1.0f;
    void setShowReset (bool s) { if (s != showReset) { showReset = s; resized(); } }

private:
    UiContext& ctx;
    juce::String status;
    double statusUntil = 0.0;
    juce::Rectangle<float> statusArea;
    bool showReset = false;
};

//==============================================================================
// Aviso corto que aparece debajo de la barra
class Toast : public juce::Component
{
public:
    explicit Toast (UiContext& c) : ctx (c) { setInterceptsMouseClicks (false, false); setVisible (false); }
    void show (const juce::String& text, double seconds = 3.5);
    void tick (double nowMs);
    void paint (juce::Graphics&) override;
    juce::String text;

private:
    UiContext& ctx;
    double until = 0.0;
};

//==============================================================================
class ThemePicker : public juce::Component
{
public:
    ThemePicker (UiContext& c, int currentTheme, int dayNightMode);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    std::function<void (int)> onPick, onMode;

private:
    juce::Rectangle<float> cell (int i) const;
    bool showsNight() const;
    UiContext& ctx;
    Segmented mode;
    int current = 0, hover = -1, modeSel = 0;
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
    juce::OwnedArray<juce::TextButton> bufferButtons;
};
