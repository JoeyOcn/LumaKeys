#pragma once

#include "Views.h"

//==============================================================================
// Diseños de cada pieza (pantalla, grados, círculo, pentagrama), texturas y luces.
namespace pieces
{
    // Texturas sutiles (se dibujan encima de un color ya pintado)
    enum Texture { texNone = 0, texBrushed, texPaper, texGrain, texDots, texLines, texChalk, texRuled, texGrooves, texRadialMetal, texGlass, texPixelGrid };

    // Rellena la figura con la textura. 'centre' sirve para las texturas redondas (surcos, metal).
    void texture (juce::Graphics&, const juce::Path& shape, int kind, bool onDark, float strength = 1.0f,
                  juce::Point<float> centre = {}, float radius = 0.0f);

    // Pantalla del centro ya resuelta (diseño + colores propios)
    struct ScreenLook
    {
        juce::Colour bg, text, dim;
        int texture = texNone;
        float glow = 0.0f;
        bool light = false, glass = false;
        int font = 0;
    };
    ScreenLook screenLook (const Skin&);

    // Pentagrama ya resuelto
    struct StaffLook
    {
        int skin = 0;
        juce::Colour paper, ink, dim;
        int texture = texNone;
        float glow = 0.0f;
        bool card = true;       // tarjeta levantada (con sombra)
        bool none = false;      // sin fondo (transparente)
    };
    StaffLook staffLook (const Skin&);

    // Círculo: textura elegida (o la del diseño)
    int circleTexture (const Skin&);
    juce::Colour circleColour (const Skin&);

    // Texto con brillo de verdad (borroso). Guarda las imágenes para no recalcular en cada cuadro.
    class GlowText
    {
    public:
        void draw (juce::Graphics&, const juce::String& text, const juce::Font&, juce::Rectangle<float> box,
                   juce::Justification, juce::Colour, float amount, float scale = 1.0f, juce::Point<float> offset = {});
    private:
        struct Entry { juce::String key; juce::Image img; juce::Rectangle<float> area; uint32_t used = 0; };
        std::vector<Entry> cache;
        uint32_t tick = 0;
    };

    // Un botón de grado con el diseño elegido
    void degreeTile (juce::Graphics&, UiContext&, juce::Rectangle<float> r, int index, const juce::String& roman,
                     const juce::String& symbol, float glow, float press, float hover, float k, GlowText* glowCache);
}
