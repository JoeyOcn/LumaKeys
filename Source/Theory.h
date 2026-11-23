#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

// Teoría musical: escalas, armaduras, nombres de notas, detección de acordes y tonalidad.
namespace theory
{
    //==============================================================================
    struct ScaleDef
    {
        const char* name;          // nombre en español
        const char* shortName;
        std::vector<int> steps;    // semitonos desde la tónica
        int parentOffset;          // distancia a la tónica de la escala mayor "madre" (para la armadura)
        bool minorFlavour;         // la tónica suena menor
    };

    const std::vector<ScaleDef>& scales();
    int numScales();

    //==============================================================================
    struct KeySig
    {
        int count = 0;                      // número de alteraciones
        bool flats = false;                 // true = bemoles
        std::array<int, 7> acc { 0, 0, 0, 0, 0, 0, 0 }; // alteración por letra C D E F G A B
    };

    KeySig keySignature (int rootPc, int scaleIndex);

    struct Spelled
    {
        int letter = 0;   // 0..6 = C D E F G A B
        int acc = 0;      // -1 bemol, 0 natural, +1 sostenido
        int octave = 4;   // octava científica (C4 = do central)
        int diatonic() const { return letter + 7 * octave; }
    };

    Spelled spell (int midiNote, const KeySig& sig);
    int spellPc (int pc, const KeySig& sig, int& accOut); // devuelve la letra

    juce::String letterName (int letter, bool latin);
    juce::String accText (int acc);
    juce::String pcName (int pc, const KeySig& sig, bool latin);
    juce::String noteName (int midiNote, const KeySig& sig, bool latin, bool withOctave = true);

    // Nombre en español de una tonalidad: "Do mayor", "La menor"
    juce::String keyTitle (int rootPc, int scaleIndex, bool latin);

    //==============================================================================
    enum class Quality { major, minor, diminished, augmented, suspended, power, halfDim };

    struct Chord
    {
        bool valid = false;
        int root = 0;          // pitch class
        int bass = 0;          // pitch class
        int templ = -1;
        bool noFifth = false;
        juce::String symbol;   // "Cmaj7", "Am/C"
        juce::String quality;  // "mayor con 7ª mayor"
        juce::String inversion;
        Quality q = Quality::major;
        bool hasSeventh = false;
        juce::String suffix;   // "maj7"
    };

    Chord detectChord (uint16_t pcMask, int bassPc, const KeySig& sig, bool latin);

    juce::String intervalName (int semitones);
    // Intervalo entre la nota más grave y la siguiente nota distinta (ignora octavas repetidas)
    inline int firstInterval (const std::vector<int>& sorted)
    {
        if (sorted.empty()) return 0;
        for (auto n : sorted) if ((n - sorted.front()) % 12 != 0) return n - sorted.front();
        return 12;
    }

    // Grado romano y función dentro de la tonalidad. Vacío si no aplica.
    struct Degree { juce::String roman; juce::String function; bool inScale = false; };
    Degree degreeOf (const Chord& c, int keyRoot, int scaleIndex);
    juce::String noteDegree (int pc, int keyRoot, int scaleIndex); // "Grado 3 · Mediante"

    //==============================================================================
    struct Diatonic { juce::String roman; juce::String symbol; int rootPc = 0; std::vector<int> pcs; uint16_t mask = 0; };
    std::vector<Diatonic> diatonicChords (int keyRoot, int scaleIndex, bool sevenths, bool latin);

    uint16_t scaleMask (int keyRoot, int scaleIndex);

    //==============================================================================
    struct KeyGuess { int root = 0; bool minor = false; float score = 0.0f; };
    KeyGuess guessKey (const std::array<float, 12>& hist);
    float keyScore (const std::array<float, 12>& hist, int root, bool minor);

    inline int pcOf (int midi) { return ((midi % 12) + 12) % 12; }
    inline int countBits (uint16_t m) { int c = 0; while (m) { c += (m & 1); m >>= 1; } return c; }
}
