#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <array>

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

    // Letras para la pantalla (0 = las del estilo; ver screenFontNames())
    juce::Font display (int which, bool pixelStyle, float height) const;
    static juce::String displayText (int which, const juce::String&);

    juce::Typeface::Ptr regular, medium, bold, pixelFace, bravura;
    juce::Typeface::Ptr doto, vt323, orbitron, monoton, arcade, sixtyfour;
};

//==============================================================================
// Estilos base de la interfaz (cada uno con modo día y noche)
struct LookInfo { juce::String id, name, description; int defaultKeys; };
const std::vector<LookInfo>& looks();
int lookIndex (const juce::String& id);

enum KeyStyle { keysPastilla = 0, keysBotones = 1, keysPlana = 2, keysClasica = 3 };
juce::StringArray keyStyleNames();

// Diseños de cada pieza (pantalla, grados, círculo, pentagrama) y el movimiento.
// Los valores por defecto dejan todo como siempre.
struct PieceStyle
{
    // Pantalla del centro
    int scrSkin = 0;        // 0 del estilo, 1 LCD verde, 2 ámbar, 3 hielo, 4 papel, 5 vidrio, 6 neón
    int scrFont = 0;        // ver screenFontNames()
    int scrTexture = 0;     // 0 del diseño, 1 lisa, 2 líneas, 3 puntos, 4 grano, 5 reflejo
    juce::Colour scrText, scrBack;   // transparentes = los del diseño
    float scrGlow = -1.0f;  // luces (0..1); -1 = las del diseño
    int scrMotion = 0;      // 0 deslizar, 1 fundido, 2 zoom, 3 escribir, 4 sin movimiento
    // Grados
    int degSkin = 0;        // 0 del estilo, 1 pastilla, 2 plano, 3 contorno, 4 pantallitas, 5 colores en fila
    juce::Colour degColour; // luz del grado (transparente = acento)
    // Círculo de quintas
    int cirSkin = 0;        // 0 clásico, 1 líneas, 2 color, 3 vinilo, 4 neón
    int cirTexture = 0;     // 0 del diseño, 1 liso, 2 surcos, 3 metal, 4 grano, 5 puntos
    juce::Colour cirColour; // zona y tónica (transparente = acento)
    int cirRoute = 0;       // líneas entre notas: 0 rodean el centro, 1 por debajo, 2 se difuminan en medio, 3 por encima
    float cirLineW = 1.0f;  // grosor de las líneas (0.5..2.5)
    float cirKnob = 1.0f;   // tamaño del botón del centro (0.5..1.15)
    int cirNodes = 0;       // puntos de las notas: 0 con brillo, 1 anillos, 2 sin puntos
    // Pentagrama
    int stfSkin = 0;        // 0 tarjeta, 1 papel, 2 pizarra, 3 transparente, 4 neón, 5 pantalla
    int stfTexture = 0;     // 0 del diseño, 1 liso, 2 papel, 3 tiza, 4 pautado, 5 puntos
    juce::Colour stfInk, stfPaper;
    // Formas
    int scrShape = 0;       // pantalla: 0 redondeada, 1 recta, 2 pastilla, 3 ovalada
    int degShape = 0;       // grados: 0 redondeados, 1 cuadrados, 2 pastilla, 3 círculos
    int stfShape = 0;       // marco del pentagrama: 0 redondeado, 1 recto, 2 muy redondo, 3 biselado
    int cirShape = 0;       // placa del círculo: 0 redondeada, 1 recta, 2 muy redonda, 3 sin placa
    int btnShape = 0;       // botones de arriba y paneles: 0 redondeados, 1 círculos, 2 cuadrados
    // Movimiento
    bool anim = true;       // animaciones prendidas
    float animSpeed = 1.0f; // 0.4 lento .. 2.5 rápido
    float keyGlow = 0.5f;   // destello al tocar (0..1)
    int keyMotion = 0;      // 0 suave, 1 al instante, 2 con estela
    bool cirFlow = true;    // la luz que recorre las líneas del círculo
    bool panelMotion = true;// los paneles aparecen con transición
    // Qué se ve
    int scrIdle = 0;        // sin tocar nada: 0 la pantalla se queda vacía, 1 muestra la tonalidad
    int degLabels = 0;      // grados: 0 solo el número (I a VII), 1 número y acorde

    void read (const juce::XmlElement&);
    void write (juce::XmlElement&) const;
};

juce::StringArray screenSkinNames();
juce::StringArray screenFontNames();
juce::StringArray screenTextureNames();
juce::StringArray screenMotionNames();
juce::StringArray degreeSkinNames();
juce::StringArray circleSkinNames();
juce::StringArray circleTextureNames();
juce::StringArray circleRouteNames();
juce::StringArray circleNodeNames();
juce::StringArray staffSkinNames();
juce::StringArray staffTextureNames();
juce::StringArray keyMotionNames();
juce::StringArray screenShapeNames();
juce::StringArray degreeShapeNames();
juce::StringArray frameShapeNames();
juce::StringArray plateShapeNames();
juce::StringArray buttonShapeNames();
juce::StringArray screenIdleNames();
juce::StringArray degreeLabelNames();

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
    int texture = 0;            // textura del cuerpo: 0 nada, 1 metal cepillado, 2 papel, 3 grano
    bool flat = false;          // botones planos con contorno (estilo Papel)
    bool neon = false;          // bordes que brillan (estilo Neón)
    bool stepColours = false;   // grados de colores en fila (estilo Ritmo)
    float soft = 0.0f;          // colores suaves: más grises y menos brillantes (0..1)
    PieceStyle pieces;

    juce::Colour noteColour (int midiNote) const;
    juce::Colour noteTag (int index) const;   // colores de las etiquetas de notas en la pantalla
    juce::Colour soften (juce::Colour) const; // aplica los colores suaves
};

// Las ventanas (paneles, menús, piano roll): en modo día todo lo oscuro se vuelve claro
Skin windowSkin (const Skin&);

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

// Un acomodo guardado de las ventanas (separadas, invisibles, minimizadas…)
struct LayoutPreset
{
    juce::String name;
    std::map<juce::String, FloatState> floats;
    bool transparent = false, hideParams = false;
    int editorW = 0, editorH = 0;
};

// Un tema: todo lo de la interfaz (estilo, día/noche, colores, cada pieza, movimiento,
// teclas, nombres de las notas y, en los tuyos, también el acomodo de las ventanas)
struct ThemePreset
{
    juce::String name, look, group;
    int dayNight = 0;              // -1 = no lo cambia (los temas listos)
    int keyStyle = -1;
    juce::Colour uiTint, uiAccent, keyLit, keyWhite, keyBlack;
    bool uiTintExact = false;
    bool rainbow = false;
    float soft = 0.0f;
    PieceStyle pieces;
    bool hasExtras = false;        // nombres de notas, séptimas y rango del teclado
    bool latin = false, sevenths = false;
    int octaves = 3, startC = 48;
    bool hasLayout = false;        // acomodo de las ventanas
    LayoutPreset layout;

    void read (const juce::XmlElement&);
    void write (juce::XmlElement&) const;
};

// Temas listos, hechos con las opciones que ya existen (por grupos)
const std::vector<ThemePreset>& builtinThemes();

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
    bool uiTintExact = false;                 // el cuerpo toma exactamente el color elegido
    float soft = 0.0f;                        // colores suaves (0..1)
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
    bool recordAudio = false;      // ya no se ofrece
    int degreeOctave = 0;          // octava de los acordes de los grados (-2..2)

    // Diseño de cada pieza y movimiento
    PieceStyle pieces;

    // Tocar los grados con teclas de la compu o con pads MIDI (no se ven en la interfaz)
    std::array<int, 7> degreeKeys { { '1', '2', '3', '4', '5', '6', '7' } };   // 0 = sin tecla
    std::array<int, 7> degreePads { { -1, -1, -1, -1, -1, -1, -1 } };          // nota + 256 * canal (canal 0 = cualquiera); -1 = sin pad
    bool degreeKeysOn = true, degreePadsOn = true;
    static juce::String keyName (int code);
    static juce::String padName (int code);

    // Atajos de teclado (Mac y Windows): tecla + modificadores (Cmd/Ctrl, Alt/Opción, Shift)
    struct Shortcut { int code = 0, mods = 0; bool operator== (const Shortcut& o) const { return code == o.code && mods == o.mods; } };
    enum ShortcutId { scMetro = 0, scMetroStop, scRecord, numShortcuts };
    std::array<Shortcut, numShortcuts> shortcuts { { { 'm', 0 }, { juce::KeyPress::returnKey, 0 }, { 'r', 0 } } };
    static std::array<Shortcut, numShortcuts> defaultShortcuts();
    static juce::StringArray shortcutNames();
    static Shortcut shortcutOf (const juce::KeyPress&);
    static juce::String shortcutName (Shortcut);
    int shortcutFor (const juce::KeyPress&) const;   // -1 si esa tecla no es un atajo
    void assignShortcut (int id, Shortcut);             // quita la tecla de donde estuviera (otro atajo o un grado)

    // Tus presets
    std::vector<ThemePreset> themes;
    std::vector<LayoutPreset> layouts;
    ThemePreset captureTheme (const juce::String& name) const;   // sin el acomodo (eso lo pone el editor)
    void applyTheme (const ThemePreset&);                        // sin el acomodo (eso lo aplica el editor)

    // Tamaño de cada panel (lo ajustas arrastrando su esquina)
    std::map<juce::String, float> panelZoom;

    void save (bool notify = true);
    Skin palette (const juce::String& lookOverride = {}) const;
    Skin paletteFor (const ThemePreset&) const;                  // cómo se vería un tema (para las muestras)
    bool isNight() const;
    int effectiveKeyStyle() const;

private:
    std::unique_ptr<juce::PropertiesFile> file;
};
