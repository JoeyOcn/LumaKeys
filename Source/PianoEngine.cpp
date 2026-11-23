#include "PianoEngine.h"
#include "MidiSkyData.h"

juce::StringArray instrumentNames()
{
    return { "Piano de cola", "Piano de concierto", "Piano suave", "Piano de bar",
             juce::String::fromUTF8 ("Piano eléctrico"), juce::String::fromUTF8 ("Eléctrico con trémolo"),
             "Celesta", "Piano de juguete", juce::String::fromUTF8 ("Clavecín") };
}

//==============================================================================
PianoSampleBank::PianoSampleBank()
{
    loader = std::thread ([this] { load(); });
}

PianoSampleBank::~PianoSampleBank()
{
    cancel.store (true);
    if (loader.joinable())
        loader.join();
}

static int noteFromSalamanderName (const juce::String& fileName)
{
    // "C4.ogg", "Ds4.ogg", "Fs1.ogg", "A0.ogg"
    auto base = fileName.upToFirstOccurrenceOf (".", false, false);
    if (base.isEmpty()) return -1;
    int pc = -1;
    switch (base[0])
    {
        case 'C': pc = 0; break;  case 'D': pc = 2; break;  case 'E': pc = 4; break;
        case 'F': pc = 5; break;  case 'G': pc = 7; break;  case 'A': pc = 9; break;
        case 'B': pc = 11; break; default: return -1;
    }
    int idx = 1;
    if (base.length() > 1 && (base[1] == 's' || base[1] == '#')) { ++pc; ++idx; }
    const int octave = base.substring (idx).getIntValue();
    return 12 * (octave + 1) + pc;
}

void PianoSampleBank::load()
{
    juce::OggVorbisAudioFormat ogg;
    struct Loaded { int set; Sample s; };
    std::vector<Loaded> loaded;

    for (int i = 0; i < MidiSkyData::namedResourceListSize; ++i)
    {
        if (cancel.load()) return;

        const char* resName = MidiSkyData::namedResourceList[i];
        const juce::String original (MidiSkyData::getNamedResourceOriginalFilename (resName));
        if (! original.endsWithIgnoreCase (".ogg"))
            continue;

        int set = salamander, layer = 1, note = -1;
        if (original.startsWith ("spl_"))
        {
            // spl_mp_062.ogg / spl_ff_062.ogg
            set = steinway;
            layer = original.substring (4, 6) == "mp" ? 0 : 1;
            note = original.substring (7, 10).getIntValue();
        }
        else if (original.startsWith ("rho_"))
        {
            // rho_v2_050.ogg / rho_v5_050.ogg
            set = rhodes;
            layer = original.substring (5, 6).getIntValue() <= 3 ? 0 : 1;
            note = original.substring (7, 10).getIntValue();
        }
        else
        {
            note = noteFromSalamanderName (original);
        }
        if (note <= 0 || note > 127) continue;

        int size = 0;
        const char* data = MidiSkyData::getNamedResource (resName, size);
        if (data == nullptr || size <= 0) continue;

        auto stream = std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false);
        std::unique_ptr<juce::AudioFormatReader> reader (ogg.createReaderFor (stream.release(), true));
        if (reader == nullptr) continue;

        Loaded L;
        L.set = set;
        L.s.rootNote = note;
        L.s.layer = layer;
        L.s.sampleRate = reader->sampleRate;
        const int len = (int) reader->lengthInSamples;
        const int ch = juce::jlimit (1, 2, (int) reader->numChannels);
        L.s.data.setSize (ch, len + 4);
        L.s.data.clear();
        reader->read (&L.s.data, 0, len, 0, true, ch > 1);
        loaded.push_back (std::move (L));
    }

    if (loaded.empty() || cancel.load())
        return;

    std::vector<Sample> all;
    std::array<std::array<std::array<int, 128>, 2>, numSets> lk {};
    std::array<bool, numSets> soft {};
    for (auto& L : loaded) if (L.s.layer == 0) soft[(size_t) L.set] = true;

    for (int set = 0; set < numSets; ++set)
        for (int layer = 0; layer < 2; ++layer)
        {
            const int useLayer = soft[(size_t) set] ? layer : 1;
            for (int n = 0; n < 128; ++n)
            {
                int best = -1, bestDist = 1000;
                for (int i = 0; i < (int) loaded.size(); ++i)
                {
                    const auto& L = loaded[(size_t) i];
                    if (L.set != set || L.s.layer != useLayer) continue;
                    const int d = std::abs (L.s.rootNote - n);
                    if (d < bestDist) { bestDist = d; best = i; }
                }
                lk[(size_t) set][(size_t) layer][(size_t) n] = best;
            }
        }

    all.reserve (loaded.size());
    for (auto& L : loaded) all.push_back (std::move (L.s));
    // Los índices de lk apuntan a "loaded"; "all" tiene el mismo orden.
    samples = std::move (all);
    lookup = lk;
    hasSoftLayer = soft;
    ready.store (true, std::memory_order_release);
}

const PianoSampleBank::Sample* PianoSampleBank::sampleFor (int set, int midiNote, float velocity) const noexcept
{
    if (! isReady() || samples.empty()) return nullptr;
    set = juce::jlimit (0, numSets - 1, set);
    const float split = set == rhodes ? 0.62f : 0.5f;
    const int layer = (hasSoftLayer[(size_t) set] && velocity < split) ? 0 : 1;
    const int idx = lookup[(size_t) set][(size_t) layer][(size_t) juce::jlimit (0, 127, midiNote)];
    if (idx < 0) return nullptr;
    return &samples[(size_t) idx];
}

//==============================================================================
PianoVoice::PianoVoice (const PianoSampleBank& b, const std::atomic<int>& instrument)
    : bank (b), instrumentRef (instrument)
{
    string1.assign (8192, 0.0f);
    string2.assign (8192, 0.0f);
}

static double mtof (double note) { return 440.0 * std::pow (2.0, (note - 69.0) / 12.0); }

void PianoVoice::finish()
{
    sample = nullptr;
    active = false;
    clearCurrentNote();
}

void PianoVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int)
{
    inst = (Instrument) juce::jlimit (0, (int) Instrument::count - 1, instrumentRef.load());
    sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    const float v = juce::jlimit (0.0f, 1.0f, velocity);
    releasing = false;
    env = 0.0f;
    ageSamples = 0;
    doubled = false;
    tremInc = 0.0;
    active = true;

    const bool sampled = inst == Instrument::grand || inst == Instrument::concert || inst == Instrument::felt
                      || inst == Instrument::honky || inst == Instrument::rhodes || inst == Instrument::rhodesTrem;
    if (sampled)
    {
        int set = PianoSampleBank::salamander;
        if (inst == Instrument::concert || inst == Instrument::honky) set = PianoSampleBank::steinway;
        if (inst == Instrument::rhodes || inst == Instrument::rhodesTrem) set = PianoSampleBank::rhodes;
        sample = bank.sampleFor (set, midiNote, v);
        if (sample == nullptr) { finish(); return; }

        step = std::pow (2.0, (midiNote - sample->rootNote) / 12.0) * sample->sampleRate / sr;
        pos = 0.0;
        gain = 0.08f + 0.92f * std::pow (v, 1.6f);

        // Más fuerte = más brillante, como un piano real
        double fc = 650.0 * std::pow (2.0, 4.9 * v) * std::pow (2.0, (midiNote - 60) / 30.0);
        double attackSec = 0.0015, releaseSec = juce::jmap ((double) juce::jlimit (21, 108, midiNote), 21.0, 108.0, 0.60, 0.20);
        if (inst == Instrument::felt)
        {
            fc *= 0.28;                 // fieltro entre el martillo y la cuerda: suave y oscuro
            attackSec = 0.007;
            gain *= 1.15f;
            releaseSec *= 1.3;
        }
        if (inst == Instrument::honky)
        {
            doubled = true;              // dos cuerdas un poco desafinadas
            step2 = step * std::pow (2.0, 11.0 / 1200.0);
            step *= std::pow (2.0, -6.0 / 1200.0);
            pos2 = 0.0;
            gain *= 0.62f;
            fc *= 1.3;
        }
        if (set == PianoSampleBank::rhodes)
        {
            fc = 1400.0 * std::pow (2.0, 3.6 * v) * std::pow (2.0, (midiNote - 60) / 36.0);
            releaseSec = 0.22;
            gain *= 1.25f;
        }
        if (inst == Instrument::rhodesTrem)
        {
            tremInc = 4.6 / sr;           // trémolo estéreo como un Rhodes de maleta
            tremPhase = 0.25;
        }
        fc = juce::jlimit (300.0, 18000.0, fc);
        lpCoef = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * fc / sr));
        lpL = lpR = 0.0f;
        attackInc = (float) (1.0 / (attackSec * sr));
        releaseMul = (float) std::exp (std::log (0.001) / (releaseSec * sr));
        return;
    }

    sample = nullptr;
    for (int i = 0; i < 4; ++i) { phase[i] = 0.0; phaseInc[i] = 0.0; partAmp[i] = 0.0f; partDecay[i] = 1.0f; }
    attackInc = (float) (1.0 / (0.001 * sr));
    const float noteNorm = juce::jlimit (0.0f, 1.0f, (float) (midiNote - 36) / 60.0f);

    if (inst == Instrument::celesta)
    {
        // Barras de metal golpeadas: FM con un brillo que se apaga rápido
        const double f = mtof (midiNote + 12);
        phaseInc[0] = f / sr;
        phaseInc[1] = f * 4.0 / sr;
        phaseInc[2] = f * 2.0 / sr;
        fmIndex = 0.6f + 2.2f * v;
        fmDecay = (float) std::exp (-1.0 / (0.07 * sr));
        partAmp[0] = 0.42f * (0.3f + 0.7f * v);
        partDecay[0] = (float) std::exp (-1.0 / ((2.2 - 1.6 * noteNorm) * sr));
        partAmp[2] = 0.08f * v;
        partDecay[2] = (float) std::exp (-1.0 / (0.35 * sr));
        releaseMul = (float) std::exp (std::log (0.001) / (0.45 * sr));
        maxAge = (int) (4.0 * sr);
    }
    else if (inst == Instrument::toy)
    {
        // Varillas de metal de un piano de juguete: parciales desafinados y un golpecito
        const double f = mtof (midiNote + 12);
        const double ratios[4] = { 1.0, 2.0, 6.27, 11.8 };
        const float amps[4] = { 0.38f, 0.07f, 0.16f, 0.07f };
        const double taus[4] = { 1.3 - 0.8 * noteNorm, 0.45, 0.09, 0.03 };
        for (int i = 0; i < 4; ++i)
        {
            phaseInc[i] = f * ratios[i] / sr;
            partAmp[i] = amps[i] * (0.35f + 0.65f * v) * (i == 0 ? 1.0f : (0.5f + 0.5f * v));
            partDecay[i] = (float) std::exp (-1.0 / (taus[i] * sr));
            if (phaseInc[i] > 0.45) partAmp[i] = 0.0f;
        }
        fmIndex = 0.0f;
        clickEnv = 0.25f * v;
        releaseMul = (float) std::exp (std::log (0.001) / (0.2 * sr));
        maxAge = (int) (3.0 * sr);
    }
    else // clavecín
    {
        const double f = mtof (midiNote);
        const double d1 = juce::jlimit (4.0, 8000.0, sr / f - 0.5);
        const double d2 = juce::jlimit (4.0, 8000.0, sr / (2.0 * f) - 0.5);
        len1 = (int) std::round (d1 * 16.0);   // guardamos el retardo en 1/16 de muestra
        len2 = (int) std::round (d2 * 16.0);
        std::fill (string1.begin(), string1.end(), 0.0f);
        std::fill (string2.begin(), string2.end(), 0.0f);
        idx1 = idx2 = 0;
        last1 = last2 = 0.0f;
        // La púa: un ruido brillante del largo de la cuerda
        float prev = 0.0f;
        const int n1 = (int) std::ceil (d1) + 1, n2 = (int) std::ceil (d2) + 1;
        for (int i = 0; i < n1; ++i)
        {
            const float r = noise.nextFloat() * 2.0f - 1.0f;
            string1[(size_t) ((8192 - 1 - i) & 8191)] = (r - 0.6f * prev) * (0.35f + 0.65f * v) * 0.5f;
            prev = r;
        }
        for (int i = 0; i < n2; ++i)
            string2[(size_t) ((8192 - 1 - i) & 8191)] = (noise.nextFloat() * 2.0f - 1.0f) * (0.3f + 0.5f * v) * 0.22f;
        const double t60 = 7.0 - 5.0 * noteNorm;
        loss = (float) std::pow (0.001, 1.0 / (t60 * f));
        releaseMul = (float) std::exp (std::log (0.001) / (0.12 * sr));
        maxAge = (int) ((t60 + 0.5) * sr);
        env = 1.0f;
    }
    gain = 1.0f;
}

void PianoVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff && active)
    {
        releasing = true;
        return;
    }
    finish();
}

float PianoVoice::nextSynthSample()
{
    if (inst == Instrument::harpsichord)
    {
        auto readFrac = [] (const std::vector<float>& buf, int w, int len16)
        {
            const float d = (float) len16 / 16.0f;
            const float rp = (float) w - d;
            const int i0 = (int) std::floor (rp);
            const float fr = rp - (float) i0;
            const float a = buf[(size_t) (i0 & 8191)], b = buf[(size_t) ((i0 + 1) & 8191)];
            return a + (b - a) * fr;
        };
        const float y1 = readFrac (string1, idx1, len1);
        const float y2 = readFrac (string2, idx2, len2);
        const float f1 = loss * (0.62f * y1 + 0.38f * last1);
        const float f2 = loss * (0.62f * y2 + 0.38f * last2);
        last1 = y1; last2 = y2;
        string1[(size_t) (idx1 & 8191)] = f1;
        string2[(size_t) (idx2 & 8191)] = f2;
        idx1 = (idx1 + 1) & 8191;
        idx2 = (idx2 + 1) & 8191;
        return (y1 + y2) * 0.9f;
    }

    float out = 0.0f;
    if (inst == Instrument::celesta)
    {
        const double mod = std::sin (juce::MathConstants<double>::twoPi * phase[1]) * fmIndex;
        out = (float) std::sin (juce::MathConstants<double>::twoPi * phase[0] + mod) * partAmp[0]
            + (float) std::sin (juce::MathConstants<double>::twoPi * phase[2]) * partAmp[2];
        fmIndex *= fmDecay;
        partAmp[0] *= partDecay[0];
        partAmp[2] *= partDecay[2];
        for (int i = 0; i < 3; ++i) { phase[i] += phaseInc[i]; if (phase[i] >= 1.0) phase[i] -= 1.0; }
        return out;
    }

    // juguete
    for (int i = 0; i < 4; ++i)
    {
        out += (float) std::sin (juce::MathConstants<double>::twoPi * phase[i]) * partAmp[i];
        partAmp[i] *= partDecay[i];
        phase[i] += phaseInc[i];
        if (phase[i] >= 1.0) phase[i] -= 1.0;
    }
    if (clickEnv > 0.0005f)
    {
        out += (noise.nextFloat() * 2.0f - 1.0f) * clickEnv;
        clickEnv *= 0.985f;
    }
    return out;
}

void PianoVoice::renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples)
{
    if (! active)
        return;

    float* outL = out.getWritePointer (0);
    float* outR = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;

    if (sample == nullptr)
    {
        // Instrumentos sintetizados
        for (int s = startSample; s < startSample + numSamples; ++s)
        {
            if (! releasing) { env += attackInc; if (env > 1.0f) env = 1.0f; }
            else
            {
                env *= releaseMul;
                if (env < 0.0005f) { finish(); return; }
            }
            if (++ageSamples > maxAge) { finish(); return; }
            const float x = nextSynthSample() * env * 0.55f;
            outL[s] += x;
            if (outR != nullptr) outR[s] += x;
        }
        return;
    }

    const auto& data = sample->data;
    const float* inL = data.getReadPointer (0);
    const float* inR = data.getReadPointer (data.getNumChannels() > 1 ? 1 : 0);
    const int len = data.getNumSamples() - 2;

    for (int s = startSample; s < startSample + numSamples; ++s)
    {
        const int i = (int) pos;
        if (i >= len) { finish(); return; }

        const float frac = (float) (pos - i);
        float l = inL[i] + (inL[i + 1] - inL[i]) * frac;
        float r = inR[i] + (inR[i + 1] - inR[i]) * frac;

        if (doubled)
        {
            const int j = (int) pos2;
            if (j < len)
            {
                const float f2 = (float) (pos2 - j);
                l += inL[j] + (inL[j + 1] - inL[j]) * f2;
                r += inR[j] + (inR[j + 1] - inR[j]) * f2;
            }
            pos2 += step2;
        }

        if (! releasing)
        {
            env += attackInc;
            if (env > 1.0f) env = 1.0f;
        }
        else
        {
            env *= releaseMul;
            if (env < 0.0005f) { finish(); return; }
        }

        lpL += lpCoef * (l - lpL);
        lpR += lpCoef * (r - lpR);

        float gl = gain * env, gr = gl;
        if (tremInc > 0.0)
        {
            const float t = (float) std::sin (juce::MathConstants<double>::twoPi * tremPhase);
            gl *= 0.72f + 0.28f * t;
            gr *= 0.72f - 0.28f * t;
            tremPhase += tremInc;
            if (tremPhase >= 1.0) tremPhase -= 1.0;
        }
        if (outR != nullptr)
        {
            outL[s] += lpL * gl;
            outR[s] += lpR * gr;
        }
        else
        {
            outL[s] += 0.5f * (lpL * gl + lpR * gr);
        }

        pos += step;
    }
}

//==============================================================================
PianoEngine::PianoEngine()
{
    for (int i = 0; i < 40; ++i)
        synth.addVoice (new PianoVoice (*bank, instrument));
    synth.addSound (new PianoSound());
    synth.setNoteStealingEnabled (true);
}

// Volumen de cada sonido, para que todos suenen parejo al cambiar de uno a otro
static float instrumentGain (int i)
{
    static const float g[] = { 1.45f, 0.50f, 1.30f, 0.50f, 0.27f, 0.33f, 0.70f, 0.80f, 1.50f };
    return g[juce::jlimit (0, (int) std::size (g) - 1, i)];
}

void PianoEngine::prepare (double sampleRate)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    level.reset (sampleRate, 0.05);
    level.setCurrentAndTargetValue (instrumentGain (instrument.load()));
}

void PianoEngine::render (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
    level.setTargetValue (instrumentGain (instrument.load()));
    if (! level.isSmoothing())
    {
        buffer.applyGain (level.getTargetValue());
        return;
    }
    const int chans = buffer.getNumChannels();
    for (int s = 0; s < buffer.getNumSamples(); ++s)
    {
        const float gv = level.getNextValue();
        for (int c = 0; c < chans; ++c)
            buffer.getWritePointer (c)[s] *= gv;
    }
}

void PianoEngine::allNotesOff()
{
    synth.allNotesOff (0, false);
}
