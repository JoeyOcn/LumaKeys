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
struct Skin
{
    const char* name;
    juce::Colour a, b;
    bool rainbow = false;
    bool custom = false;
};

const std::vector<Skin>& skins();

struct Palette
{
    bool dark = true;
    bool rainbow = false;
    juce::Colour a, b;

    juce::Colour skyTop, skyBottom;
    juce::Colour panel, panelEdge, panelShine;
    juce::Colour screenTop, screenBottom, screenEdge;
    juce::Colour text, dim, faint;
    juce::Colour screenText, screenDim;
    juce::Colour keyWhite, keyWhiteShade, keyEdge, keyBlack, keyBlackTop;
    juce::Colour ink; // tinta del pentagrama

    juce::Colour noteColour (int midiNote) const;
    juce::Colour accent() const { return a; }
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
