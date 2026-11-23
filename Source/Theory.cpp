#include "Theory.h"

namespace theory
{
    static juce::String U8 (const char* s) { return juce::String::fromUTF8 (s); }
    static const int naturalPc[7] = { 0, 2, 4, 5, 7, 9, 11 };
    static const int sharpOrder[7] = { 3, 0, 4, 1, 5, 2, 6 }; // F C G D A E B
    static const int flatOrder[7]  = { 6, 2, 5, 1, 4, 0, 3 }; // B E A D G C F

    const std::vector<ScaleDef>& scales()
    {
        static const std::vector<ScaleDef> s {
            { "Mayor (jónica)",      "mayor",             { 0, 2, 4, 5, 7, 9, 11 }, 0,  false },
            { "Menor natural",       "menor",             { 0, 2, 3, 5, 7, 8, 10 }, 9,  true  },
            { "Menor armónica",      "menor armónica",    { 0, 2, 3, 5, 7, 8, 11 }, 9,  true  },
            { "Menor melódica",      "menor melódica",    { 0, 2, 3, 5, 7, 9, 11 }, 9,  true  },
            { "Dórica",              "dórica",            { 0, 2, 3, 5, 7, 9, 10 }, 2,  true  },
            { "Frigia",              "frigia",            { 0, 1, 3, 5, 7, 8, 10 }, 4,  true  },
            { "Lidia",               "lidia",             { 0, 2, 4, 6, 7, 9, 11 }, 5,  false },
            { "Mixolidia",           "mixolidia",         { 0, 2, 4, 5, 7, 9, 10 }, 7,  false },
            { "Locria",              "locria",            { 0, 1, 3, 5, 6, 8, 10 }, 11, true  },
            { "Pentatónica mayor",   "pentatónica mayor", { 0, 2, 4, 7, 9 },        0,  false },
            { "Pentatónica menor",   "pentatónica menor", { 0, 3, 5, 7, 10 },       9,  true  },
            { "Blues",               "blues",             { 0, 3, 5, 6, 7, 10 },    9,  true  },
        };
        return s;
    }

    int numScales() { return (int) scales().size(); }

    static int clampScale (int i) { return juce::jlimit (0, numScales() - 1, i); }

    //==============================================================================
    KeySig keySignature (int rootPc, int scaleIndex)
    {
        const auto& sc = scales()[(size_t) clampScale (scaleIndex)];
        rootPc = pcOf (rootPc);
        const int parent = pcOf (rootPc - sc.parentOffset);
        const int pos = (parent * 7) % 12; // posición en el círculo de quintas

        KeySig k;
        if (pos <= 5)        { k.count = pos;      k.flats = false; }
        else if (pos >= 7)   { k.count = 12 - pos; k.flats = true; }
        else
        {
            // 6 sostenidos o 6 bemoles: según cómo se escribe más natural la tónica
            const bool flatRoot = (rootPc == 1 || rootPc == 3 || rootPc == 8 || rootPc == 10);
            k.count = 6;
            k.flats = flatRoot;
        }

        for (int i = 0; i < k.count; ++i)
        {
            if (k.flats) k.acc[(size_t) flatOrder[i]]  = -1;
            else         k.acc[(size_t) sharpOrder[i]] = +1;
        }
        return k;
    }

    int spellPc (int pc, const KeySig& sig, int& accOut)
    {
        pc = pcOf (pc);

        for (int L = 0; L < 7; ++L)
            if (pcOf (naturalPc[L] + sig.acc[(size_t) L]) == pc) { accOut = sig.acc[(size_t) L]; return L; }

        for (int L = 0; L < 7; ++L)
            if (naturalPc[L] == pc) { accOut = 0; return L; }

        if (sig.count == 0)
        {
            switch (pc)
            {
                case 1:  accOut = +1; return 0; // C#
                case 3:  accOut = -1; return 2; // Eb
                case 6:  accOut = +1; return 3; // F#
                case 8:  accOut = -1; return 5; // Ab
                case 10: accOut = -1; return 6; // Bb
                default: break;
            }
        }

        if (sig.flats)
        {
            for (int L = 0; L < 7; ++L)
                if (naturalPc[L] == pcOf (pc + 1)) { accOut = -1; return L; }
        }

        for (int L = 0; L < 7; ++L)
            if (naturalPc[L] == pcOf (pc - 1)) { accOut = +1; return L; }

        accOut = 0;
        return 0;
    }

    Spelled spell (int midiNote, const KeySig& sig)
    {
        Spelled s;
        int acc = 0;
        s.letter = spellPc (pcOf (midiNote), sig, acc);
        s.acc = acc;
        const int natural = midiNote - acc;
        s.octave = (int) std::floor (natural / 12.0) - 1;
        return s;
    }

    juce::String letterName (int letter, bool latin)
    {
        static const char* en[7] = { "C", "D", "E", "F", "G", "A", "B" };
        static const char* es[7] = { "Do", "Re", "Mi", "Fa", "Sol", "La", "Si" };
        letter = juce::jlimit (0, 6, letter);
        return latin ? es[letter] : en[letter];
    }

    juce::String accText (int acc)
    {
        if (acc > 0) return "#";
        if (acc < 0) return "b";
        return {};
    }

    juce::String pcName (int pc, const KeySig& sig, bool latin)
    {
        int acc = 0;
        const int L = spellPc (pc, sig, acc);
        return letterName (L, latin) + accText (acc);
    }

    juce::String noteName (int midiNote, const KeySig& sig, bool latin, bool withOctave)
    {
        const auto s = spell (midiNote, sig);
        auto n = letterName (s.letter, latin) + accText (s.acc);
        if (withOctave) n << s.octave;
        return n;
    }

    juce::String keyTitle (int rootPc, int scaleIndex, bool latin)
    {
        const auto sig = keySignature (rootPc, scaleIndex);
        return pcName (rootPc, sig, latin) + " " + juce::String (juce::CharPointer_UTF8 (scales()[(size_t) clampScale (scaleIndex)].shortName));
    }

    //==============================================================================
    struct Templ
    {
        const char* suffix;
        const char* quality;
        std::vector<int> iv;
        int complexity;
        Quality q;
        bool seventh;
        const char* romanSuffix;
    };

    static const std::vector<Templ>& templates()
    {
        static const std::vector<Templ> t {
            { "",        "mayor",                         { 0, 4, 7 },         0, Quality::major,      false, "" },
            { "m",       "menor",                         { 0, 3, 7 },         0, Quality::minor,      false, "" },
            { "dim",     "disminuido",                    { 0, 3, 6 },         1, Quality::diminished, false, "°" },
            { "aug",     "aumentado",                     { 0, 4, 8 },         1, Quality::augmented,  false, "+" },
            { "sus2",    "suspendido de 2ª",              { 0, 2, 7 },         2, Quality::suspended,  false, "sus2" },
            { "sus4",    "suspendido de 4ª",              { 0, 5, 7 },         2, Quality::suspended,  false, "sus4" },
            { "5",       "quinta (power chord)",          { 0, 7 },            1, Quality::power,      false, "5" },
            { "7",       "séptima de dominante",          { 0, 4, 7, 10 },     1, Quality::major,      true,  "7" },
            { "maj7",    "mayor con 7ª mayor",            { 0, 4, 7, 11 },     1, Quality::major,      true,  "maj7" },
            { "m7",      "menor con 7ª",                  { 0, 3, 7, 10 },     1, Quality::minor,      true,  "7" },
            { "m7b5",    "semidisminuido",                { 0, 3, 6, 10 },     2, Quality::halfDim,    true,  "ø7" },
            { "dim7",    "disminuido con 7ª",             { 0, 3, 6, 9 },      2, Quality::diminished, true,  "°7" },
            { "m(maj7)", "menor con 7ª mayor",            { 0, 3, 7, 11 },     3, Quality::minor,      true,  "(maj7)" },
            { "6",       "mayor con 6ª",                  { 0, 4, 7, 9 },      2, Quality::major,      false, "6" },
            { "m6",      "menor con 6ª",                  { 0, 3, 7, 9 },      2, Quality::minor,      false, "6" },
            { "7sus4",   "séptima suspendida",            { 0, 5, 7, 10 },     2, Quality::suspended,  true,  "7sus4" },
            { "add9",    "mayor con 9ª añadida",          { 0, 2, 4, 7 },      2, Quality::major,      false, "add9" },
            { "m(add9)", "menor con 9ª añadida",          { 0, 2, 3, 7 },      2, Quality::minor,      false, "(add9)" },
            { "9",       "novena de dominante",           { 0, 2, 4, 7, 10 },  2, Quality::major,      true,  "9" },
            { "maj9",    "mayor con 9ª",                  { 0, 2, 4, 7, 11 },  2, Quality::major,      true,  "maj9" },
            { "m9",      "menor con 9ª",                  { 0, 2, 3, 7, 10 },  2, Quality::minor,      true,  "9" },
            { "6/9",     "seis nueve",                    { 0, 2, 4, 7, 9 },   3, Quality::major,      false, "6/9" },
            { "7#5",     "séptima aumentada",             { 0, 4, 8, 10 },     3, Quality::augmented,  true,  "+7" },
            { "7b9",     "séptima con 9ª menor",          { 0, 1, 4, 7, 10 },  3, Quality::major,      true,  "7b9" },
            { "m11",     "menor con 11ª",                 { 0, 2, 3, 5, 7, 10 }, 3, Quality::minor,    true,  "11" },
            { "13",      "trecena de dominante",          { 0, 2, 4, 7, 9, 10 }, 3, Quality::major,    true,  "13" },
        };
        return t;
    }

    static uint16_t maskOf (const std::vector<int>& iv)
    {
        uint16_t m = 0;
        for (auto i : iv) m = (uint16_t) (m | (1u << (unsigned) pcOf (i)));
        return m;
    }

    static uint16_t rotateDown (uint16_t mask, int r)
    {
        r = pcOf (r);
        const unsigned m = mask;
        return (uint16_t) (((m >> (unsigned) r) | (m << (unsigned) (12 - r))) & 0xFFFu);
    }

    Chord detectChord (uint16_t pcMask, int bassPc, const KeySig& sig, bool latin)
    {
        Chord best;
        pcMask &= 0xFFF;
        if (countBits (pcMask) < 2)
            return best;

        int bestScore = -1000;
        const auto& T = templates();

        for (int r = 0; r < 12; ++r)
        {
            if ((pcMask & (1u << (unsigned) r)) == 0) continue;
            const auto rel = rotateDown (pcMask, r);

            for (int ti = 0; ti < (int) T.size(); ++ti)
            {
                const auto& t = T[(size_t) ti];
                const auto tm = maskOf (t.iv);
                int score = -1000;
                bool noFifth = false;

                if (rel == tm)
                    score = 100 - t.complexity * 3;
                else if (t.iv.size() >= 4 && (tm & (1u << 7)) != 0
                         && rel == (uint16_t) (tm & ~(1u << 7)) && countBits (rel) >= 3)
                {
                    score = 78 - t.complexity * 3;
                    noFifth = true;
                }

                if (score < 0) continue;
                if (bassPc >= 0 && r == pcOf (bassPc)) score += 12;

                if (score > bestScore)
                {
                    bestScore = score;
                    best.valid = true;
                    best.root = r;
                    best.templ = ti;
                    best.noFifth = noFifth;
                }
            }
        }

        if (! best.valid)
        {
            // Acorde con un bajo ajeno (ej. G7 con Do en el bajo = G7/C)
            const int b = pcOf (bassPc);
            const auto upper = (uint16_t) (pcMask & ~(1u << (unsigned) b));
            if ((pcMask & (1u << (unsigned) b)) != 0 && countBits (upper) >= 3)
            {
                auto c = detectChord (upper, -1, sig, latin);
                if (c.valid)
                {
                    c.bass = b;
                    c.symbol << "/" << pcName (b, sig, latin);
                    c.inversion = "con bajo en " + pcName (b, sig, true);
                    c.quality << U8 (" (bajo fuera del acorde)");
                }
                return c;
            }
            return best;
        }

        const auto& t = T[(size_t) best.templ];
        best.bass = bassPc >= 0 ? pcOf (bassPc) : best.root;
        best.q = t.q;
        best.hasSeventh = t.seventh;
        best.suffix = juce::String (juce::CharPointer_UTF8 (t.suffix));

        best.symbol = pcName (best.root, sig, latin) + best.suffix;
        if (best.bass != best.root)
            best.symbol << "/" << pcName (best.bass, sig, latin);

        best.quality = juce::String (juce::CharPointer_UTF8 (t.quality));
        if (best.noFifth)
            best.quality << juce::String (juce::CharPointer_UTF8 (" (sin 5ª)"));

        const int bi = pcOf (best.bass - best.root);
        if (bi == 0)                                   best.inversion = "estado fundamental";
        else if (bi == 3 || bi == 4)                   best.inversion = juce::String (juce::CharPointer_UTF8 ("1ª inversión"));
        else if (bi == 6 || bi == 7 || bi == 8)        best.inversion = juce::String (juce::CharPointer_UTF8 ("2ª inversión"));
        else if (bi == 9 || bi == 10 || bi == 11)      best.inversion = juce::String (juce::CharPointer_UTF8 ("3ª inversión"));
        else                                           best.inversion = "bajo en " + pcName (best.bass, sig, true);

        return best;
    }

    juce::String intervalName (int semitones)
    {
        semitones = std::abs (semitones);
        static const char* names[12] = {
            "Unísono", "2ª menor", "2ª mayor", "3ª menor", "3ª mayor", "4ª justa",
            "Tritono", "5ª justa", "6ª menor", "6ª mayor", "7ª menor", "7ª mayor"
        };
        if (semitones == 12) return "Octava";
        if (semitones == 0)  return juce::String (juce::CharPointer_UTF8 (names[0]));

        auto n = juce::String (juce::CharPointer_UTF8 (names[semitones % 12]));
        if (semitones > 12)
        {
            if (semitones % 12 == 0) return juce::String (semitones / 12) + " octavas";
            n << " + " << (semitones / 12 == 1 ? juce::String ("octava") : juce::String (semitones / 12) + " octavas");
        }
        return n;
    }

    //==============================================================================
    static const std::vector<int>& sevenNoteSteps (int scaleIndex)
    {
        const auto& sc = scales()[(size_t) clampScale (scaleIndex)];
        if (sc.steps.size() == 7) return sc.steps;
        return sc.minorFlavour ? scales()[1].steps : scales()[0].steps;
    }

    static juce::String degreeFunction (int index, int rel)
    {
        switch (index)
        {
            case 0: return juce::String (juce::CharPointer_UTF8 ("Tónica · reposo"));
            case 1: return juce::String (juce::CharPointer_UTF8 ("Supertónica · movimiento"));
            case 2: return juce::String (juce::CharPointer_UTF8 ("Mediante · reposo suave"));
            case 3: return juce::String (juce::CharPointer_UTF8 ("Subdominante · movimiento"));
            case 4: return juce::String (juce::CharPointer_UTF8 ("Dominante · tensión"));
            case 5: return juce::String (juce::CharPointer_UTF8 ("Superdominante · reposo suave"));
            case 6: return rel == 10 ? juce::String (juce::CharPointer_UTF8 ("Subtónica · movimiento"))
                                     : juce::String (juce::CharPointer_UTF8 ("Sensible · tensión"));
            default: return {};
        }
    }

    Degree degreeOf (const Chord& c, int keyRoot, int scaleIndex)
    {
        Degree d;
        if (! c.valid) return d;

        static const char* romans[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
        static const char* chroma[12] = { "I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII" };

        const auto& steps = sevenNoteSteps (scaleIndex);
        const int rel = pcOf (c.root - keyRoot);

        int index = -1;
        for (int i = 0; i < (int) steps.size(); ++i)
            if (steps[(size_t) i] == rel) index = i;

        // Solo el número del grado en romano (I II III IV V VI VII), sin la calidad del acorde
        d.roman = index >= 0 ? juce::String (romans[index]) : juce::String (chroma[rel]);
        d.inScale = index >= 0;
        d.function = index >= 0 ? degreeFunction (index, rel) : juce::String (juce::CharPointer_UTF8 ("Acorde prestado / cromático"));
        return d;
    }

    juce::String noteDegree (int pc, int keyRoot, int scaleIndex)
    {
        const auto& sc = scales()[(size_t) clampScale (scaleIndex)];
        const int rel = pcOf (pc - keyRoot);
        for (int i = 0; i < (int) sc.steps.size(); ++i)
        {
            if (sc.steps[(size_t) i] == rel)
            {
                if (sc.steps.size() == 7)
                {
                    auto f = degreeFunction (i, rel);
                    return "Grado " + juce::String (i + 1) + juce::String::fromUTF8 (" · ") + f.upToFirstOccurrenceOf (juce::String::fromUTF8 (" ·"), false, false);
                }
                return "Nota " + juce::String (i + 1) + " de la escala";
            }
        }
        return "Fuera de la escala";
    }

    std::vector<Diatonic> diatonicChords (int keyRoot, int scaleIndex, bool sevenths, bool latin)
    {
        std::vector<Diatonic> out;
        const auto& steps = sevenNoteSteps (scaleIndex);
        const auto sig = keySignature (keyRoot, scaleIndex);

        for (int i = 0; i < 7; ++i)
        {
            Diatonic dc;
            const int n = sevenths ? 4 : 3;
            for (int k = 0; k < n; ++k)
            {
                const int pc = pcOf (keyRoot + steps[(size_t) ((i + 2 * k) % 7)]);
                dc.pcs.push_back (pc);
                dc.mask = (uint16_t) (dc.mask | (1u << (unsigned) pc));
            }
            dc.rootPc = dc.pcs.front();
            const auto chord = detectChord (dc.mask, dc.rootPc, sig, latin);
            dc.symbol = chord.valid ? chord.symbol : pcName (dc.rootPc, sig, latin);
            dc.roman = degreeOf (chord, keyRoot, scaleIndex).roman;
            out.push_back (dc);
        }
        return out;
    }

    uint16_t scaleMask (int keyRoot, int scaleIndex)
    {
        uint16_t m = 0;
        for (auto s : scales()[(size_t) clampScale (scaleIndex)].steps)
            m = (uint16_t) (m | (1u << (unsigned) pcOf (keyRoot + s)));
        return m;
    }

    //==============================================================================
    static const float kkMajor[12] = { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
    static const float kkMinor[12] = { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };

    static float correlation (const float* a, const float* b)
    {
        float ma = 0, mb = 0;
        for (int i = 0; i < 12; ++i) { ma += a[i]; mb += b[i]; }
        ma /= 12; mb /= 12;
        float num = 0, da = 0, db = 0;
        for (int i = 0; i < 12; ++i)
        {
            num += (a[i] - ma) * (b[i] - mb);
            da  += (a[i] - ma) * (a[i] - ma);
            db  += (b[i] - mb) * (b[i] - mb);
        }
        return (da <= 0 || db <= 0) ? 0.0f : num / std::sqrt (da * db);
    }

    float keyScore (const std::array<float, 12>& hist, int root, bool minor)
    {
        float rot[12];
        for (int i = 0; i < 12; ++i) rot[i] = hist[(size_t) pcOf (i + root)];
        return correlation (rot, minor ? kkMinor : kkMajor);
    }

    KeyGuess guessKey (const std::array<float, 12>& hist)
    {
        const float* maj = kkMajor;
        const float* mnr = kkMinor;

        auto corr = [] (const float* a, const float* b)
        {
            float ma = 0, mb = 0;
            for (int i = 0; i < 12; ++i) { ma += a[i]; mb += b[i]; }
            ma /= 12; mb /= 12;
            float num = 0, da = 0, db = 0;
            for (int i = 0; i < 12; ++i)
            {
                num += (a[i] - ma) * (b[i] - mb);
                da  += (a[i] - ma) * (a[i] - ma);
                db  += (b[i] - mb) * (b[i] - mb);
            }
            return (da <= 0 || db <= 0) ? 0.0f : num / std::sqrt (da * db);
        };

        KeyGuess best { 0, false, -2.0f };
        for (int r = 0; r < 12; ++r)
        {
            float rot[12];
            for (int i = 0; i < 12; ++i) rot[i] = hist[(size_t) pcOf (i + r)];
            const float cm = corr (rot, maj);
            const float cn = corr (rot, mnr);
            if (cm > best.score) best = { r, false, cm };
            if (cn > best.score) best = { r, true, cn };
        }
        return best;
    }
}
