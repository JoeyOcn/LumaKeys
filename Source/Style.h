#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>

// Texto con acentos: siempre pasar por UTF-8.
inline juce::String U8 (const char* s) { return juce::String::fromUTF8 (s); }

//==============================================================================
enum class FontKind { serif, sans, pixel };

// Tipografías incrustadas (compartidas entre todas las instancias del plugin).
struct Fonts
{
    Fonts();

    juce::Font ui (float height, int weight = 0) const;          // Zen Kaku Gothic New: 0 regular, 1 medium, 2 bold
    juce::Font serif (float height, bool extraBold = false) const; // Shippori Mincho
    juce::Font pixel (float height) const;                        // Press Start 2P (títulos)
    juce::Font pixelLabel (float height) const;                   // Jersey 10 (etiquetas)
    juce::Font display (FontKind k, float height) const;          // según la skin
    juce::Font label (FontKind k, float height, int weight = 1) const;
    juce::Font music (float emPixels) const;                      // Bravura (SMuFL)

    juce::Typeface::Ptr regular, medium, bold, serifBold, serifXBold, pixelFace, pixelSmallFace, bravura;
};

//==============================================================================
enum class Tex { none, nogal, hinoki, musgo, papel, washi, agua, cosmos, metal, aluminio, pergamino };

// Imágenes de textura, decodificadas una sola vez.
struct Textures
{
    juce::Image get (Tex t);
    std::map<int, juce::Image> cache;
};

//==============================================================================
// Una skin es el "modelo" completo: materiales, colores, tipografía y forma.
struct Skin
{
    juce::String id, name;
    Tex body = Tex::none;        juce::Colour bodyColour, bodyColour2;
    Tex paper = Tex::none;       juce::Colour paperColour;
    Tex cheekTex = Tex::none;
    juce::Colour text, dim, rail, railBorder, well, wellBorder, atril, lip;
    juce::Colour ink, noteInk, fall, chord, accent, accentInk, light, felt;
    juce::Colour keyBed, keyWhite, keyShade, keyBlack, keyLabel, markOn, keysBlock, cheek;
    juce::Colour knob, knobLine, knobCap, switchOn;
    juce::Colour ring, ringLine, region, regionMinor, center, constLine;
    juce::Colour tablet, tabletOn, tabletBorder;
    FontKind displayFont = FontKind::serif, uiFont = FontKind::sans;
    float radius = 10.0f;
    bool seal = false;      // sello rojo con la tónica (Hinoki)
    bool pixel = false;     // bordes de 8 bits (Videojuego)
    bool chamfer = false;   // esquinas cortadas (Futurista)
    bool ornate = false;    // doble filete (Vintage)
    bool vfd = false;       // pantalla tipo display ámbar (Retro)
    bool lacquer = false;   // reflejo de laca (Laca y oro)
    bool dark = true;       // la skin es oscura (para textos sobre el cuerpo)
};

const std::vector<Skin>& skins();
int skinIndex (const juce::String& id);

// Un tema es la luz del ambiente: se aplica encima de cualquier skin.
struct Theme { juce::String id, name, description; };
const std::vector<Theme>& themes();
int themeIndex (const juce::String& id);

struct Palette : Skin
{
    juce::String theme = "natural";
    int lightMode = 0;            // 0 color de la skin, 1 color propio, 2 arcoíris
    juce::Colour noteColour (int midiNote) const;
};

//==============================================================================
// Estado de una sección que se puede separar en su propia ventana.
struct FloatState
{
    bool floating = false;
    juce::Rectangle<int> bounds;   // vacío = posición y tamaño normales
    bool onTop = true;
};

// Preferencias visuales globales (se guardan en la compu y se comparten entre instancias).
class Prefs : public juce::ChangeBroadcaster
{
public:
    Prefs();
    ~Prefs() override;

    juce::String skin = "nogal";
    juce::String theme = "natural";
    int lightMode = 0;
    juce::Colour lightCustom { 0xff7fb8ff };
    bool latin = false;
    bool sevenths = false;
    bool midiAuto = true;
    bool hideParams = false;
    bool transparent = false;
    bool lowLatencyDone = false;

    std::map<juce::String, FloatState> floats;   // por sección: "circulo", "partitura", "tablillas", "acorde", "teclado"

    void save (bool notify = true);
    Palette palette() const;

private:
    std::unique_ptr<juce::PropertiesFile> file;
};
