#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>

// Texto con acentos: siempre pasar por UTF-8.
inline juce::String U8 (const char* s) { return juce::String::fromUTF8 (s); }

//==============================================================================
// Tipografías incrustadas (compartidas entre todas las instancias del plugin).
struct Fonts
{
    Fonts();

    juce::Font ui (float height, int weight = 0) const;   // Zen Kaku Gothic New: 0 regular, 1 medium, 2 bold
    juce::Font pixel (float height) const;                 // Jersey 10 (pantalla del estilo Sampler)
    juce::Font music (float emPixels) const;               // Bravura (SMuFL)

    juce::Typeface::Ptr regular, medium, bold, pixelFace, bravura;
};

//==============================================================================
// Estilos base de la interfaz (cada uno con modo día y noche)
struct LookInfo { juce::String id, name, description; int defaultKeys; };
const std::vector<LookInfo>& looks();
int lookIndex (const juce::String& id);

enum KeyStyle { keysPastilla = 0, keysBotones = 1, keysPlana = 2, keysClasica = 3 };
juce::StringArray keyStyleNames();

// Paleta ya resuelta: todo lo que se necesita para dibujar
struct Skin
{
    int look = 0;
    bool night = false;
    juce::Colour backdrop, body1, body2, edge, text, dim;
    juce::Colour btn, btnEdge, btnShadow, btnText;
    juce::Colour plate, plateLine, card, ink;
    juce::Colour screen, scrText, scrDim;
    juce::Colour accent, accentInk, lit;
    juce::Colour caps[4];
    juce::Colour keyW, keyWEdge, keyB, keyBed;
    juce::Colour region, ring;
    float radius = 30.0f;       // esquinas del aparato (a escala 1)
    bool pixelScreen = false;   // letras pixeladas en la pantalla (Sampler)
    int keyStyle = keysPastilla;
    bool rainbow = false;

    juce::Colour noteColour (int midiNote) const;
    juce::Colour noteTag (int index) const;   // colores de las etiquetas de notas en la pantalla
};

// Paletas rápidas: un tono para el cuerpo y un color de acento
struct QuickPalette { juce::String name; juce::Colour tint, accent; };
const std::vector<QuickPalette>& quickPalettes();

// Muestras de color para elegir rápido
const std::vector<juce::Colour>& swatches();

//==============================================================================
// Estado de una sección que se puede separar en su propia ventana.
struct FloatState
{
    bool floating = false;
    juce::Rectangle<int> bounds;   // vacío = posición y tamaño normales
    bool onTop = true;
    bool collapsed = false;        // minimizada (solo se ve su barra)
};

// Un tema guardado por ti (estilo, día/noche y colores)
struct ThemePreset
{
    juce::String name, look;
    int dayNight = 0, keyStyle = -1;
    juce::Colour uiTint, uiAccent, keyLit, keyWhite, keyBlack;
    bool rainbow = false;
};

// Un acomodo guardado de las ventanas (separadas, invisibles, minimizadas…)
struct LayoutPreset
{
    juce::String name;
    std::map<juce::String, FloatState> floats;
    bool transparent = false, hideParams = false;
    int editorW = 0, editorH = 0;
};

// Preferencias visuales globales (se guardan en la compu y se comparten entre instancias).
class Prefs : public juce::ChangeBroadcaster
{
public:
    Prefs();
    ~Prefs() override;

    juce::String look = "campo";
    int dayNight = 0;              // 0 día, 1 noche, 2 automático (según la hora)
    int keyStyle = -1;             // -1 según el estilo; si no, KeyStyle
    int octaves = 3;
    int startC = 48;               // la tecla Do más grave que se ve
    juce::Colour uiTint, uiAccent;            // transparentes = los del estilo
    juce::Colour keyLit, keyWhite, keyBlack;  // transparentes = los del estilo
    bool rainbow = false;
    bool latin = false;
    bool sevenths = false;
    bool midiAuto = true;
    bool hideParams = false;
    bool transparent = false;
    bool lowLatencyDone = false;

    std::map<juce::String, FloatState> floats;   // por sección: "partitura", "acorde", "tablillas", "circulo", "teclado"

    // Metrónomo y grabadora
    float metroBpm = 100.0f;
    int metroBeats = 4;
    int metroSound = 0;
    float metroVolume = -8.0f;
    bool metroFollowHost = true;
    bool countIn = true;
    bool recordMouse = true;

    // Tus presets
    std::vector<ThemePreset> themes;
    std::vector<LayoutPreset> layouts;
    ThemePreset captureTheme (const juce::String& name) const;
    void applyTheme (const ThemePreset&);

    void save (bool notify = true);
    Skin palette (const juce::String& lookOverride = {}) const;
    bool isNight() const;
    int effectiveKeyStyle() const;

private:
    std::unique_ptr<juce::PropertiesFile> file;
};
