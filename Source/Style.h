#pragma once

#include <JuceHeader.h>
#include <vector>

// Texto con acentos: siempre pasar por UTF-8.
inline juce::String U8 (const char* s) { return juce::String::fromUTF8 (s); }

//==============================================================================
// Tipografías incrustadas (compartidas entre todas las instancias del plugin).
struct Fonts
{
    Fonts();

    juce::Font ui (float height, int weight = 0) const;   // 0 regular, 1 semibold, 2 bold
    juce::Font music (float emPixels) const;              // Bravura (SMuFL)

    juce::Typeface::Ptr regular, semibold, bold, bravura;
};

//==============================================================================
// Una skin es el color de la luz: teclas encendidas, pantalla y acentos.
struct Skin
{
    const char* name;
    juce::Colour light;
    bool rainbow = false;
    bool custom = false;
};

const std::vector<Skin>& skins();

struct Palette
{
    bool dark = true;
    bool rainbow = false;
    juce::Colour accent;      // color de la luz
    juce::Colour onAccent;    // texto encima del color de la luz

    // Cuerpo del aparato
    juce::Colour bodyTop, bodyBottom;
    juce::Colour plate, plateEdge, plateHighlight;
    juce::Colour well, wellEdge;

    // Texto sobre el cuerpo
    juce::Colour text, dim, faint;

    // Pantalla
    juce::Colour screen, screenEdge, screenText, screenDim, screenFaint;

    // Teclado y pentagrama
    juce::Colour keyWhite, keyWhiteShade, keyEdge, keyBlack, keyBlackTop, felt;
    juce::Colour ink;

    juce::Colour noteColour (int midiNote) const;
};

//==============================================================================
// Preferencias visuales globales (se guardan en la compu y se comparten entre instancias).
class Prefs : public juce::ChangeBroadcaster
{
public:
    Prefs();
    ~Prefs() override;

    int skin = 0;
    bool dark = true;
    juce::Colour custom { 0xff4de1ff };
    bool latin = false;
    bool sevenths = false;
    bool midiAuto = true;

    void save();
    Palette palette() const;

private:
    std::unique_ptr<juce::PropertiesFile> file;
};
