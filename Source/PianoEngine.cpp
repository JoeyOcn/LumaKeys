#include "PianoEngine.h"
#include "MidiSkyData.h"

//==============================================================================
PianoSampleBank::PianoSampleBank()
{
    lookup.fill (0);
    loader = std::thread ([this] { load(); });
}

PianoSampleBank::~PianoSampleBank()
{
    cancel.store (true);
    if (loader.joinable())
        loader.join();
}

static int noteFromSampleName (const juce::String& fileName)
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
    std::vector<Sample> loaded;

    for (int i = 0; i < MidiSkyData::namedResourceListSize; ++i)
    {
        if (cancel.load()) return;

        const char* resName = MidiSkyData::namedResourceList[i];
        const juce::String original (MidiSkyData::getNamedResourceOriginalFilename (resName));
        if (! original.endsWithIgnoreCase (".ogg"))
            continue;

        const int note = noteFromSampleName (original);
        if (note < 0) continue;

        int size = 0;
        const char* data = MidiSkyData::getNamedResource (resName, size);
        if (data == nullptr || size <= 0) continue;

        auto stream = std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false);
        std::unique_ptr<juce::AudioFormatReader> reader (ogg.createReaderFor (stream.release(), true));
        if (reader == nullptr) continue;

        Sample s;
        s.rootNote = note;
        s.sampleRate = reader->sampleRate;
        const int len = (int) reader->lengthInSamples;
        const int ch = juce::jlimit (1, 2, (int) reader->numChannels);
        s.data.setSize (ch, len + 4);
        s.data.clear();
        reader->read (&s.data, 0, len, 0, true, ch > 1);
        loaded.push_back (std::move (s));
    }

    std::sort (loaded.begin(), loaded.end(), [] (const Sample& x, const Sample& y) { return x.rootNote < y.rootNote; });

    std::array<int, 128> lk {};
    for (int n = 0; n < 128; ++n)
    {
        int best = 0, bestDist = 1000;
        for (int i = 0; i < (int) loaded.size(); ++i)
        {
            const int d = std::abs (loaded[(size_t) i].rootNote - n);
            if (d < bestDist) { bestDist = d; best = i; }
        }
        lk[(size_t) n] = best;
    }

    if (loaded.empty() || cancel.load())
        return;

    samples = std::move (loaded);
    lookup = lk;
    ready.store (true, std::memory_order_release);
}

const PianoSampleBank::Sample* PianoSampleBank::sampleFor (int midiNote) const noexcept
{
    if (! isReady() || samples.empty()) return nullptr;
    return &samples[(size_t) lookup[(size_t) juce::jlimit (0, 127, midiNote)]];
}

//==============================================================================
void PianoVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int)
{
    sample = bank.sampleFor (midiNote);
    if (sample == nullptr)
    {
        clearCurrentNote();
        return;
    }

    const double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    step = std::pow (2.0, (midiNote - sample->rootNote) / 12.0) * sample->sampleRate / sr;
    pos = 0.0;

    const float v = juce::jlimit (0.0f, 1.0f, velocity);
    gain = 0.08f + 0.92f * std::pow (v, 1.6f);

    // Más fuerte = más brillante, como un piano real.
    double fc = 650.0 * std::pow (2.0, 4.9 * v) * std::pow (2.0, (midiNote - 60) / 30.0);
    fc = juce::jlimit (400.0, 18000.0, fc);
    lpCoef = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * fc / sr));
    lpL = lpR = 0.0f;

    env = 0.0f;
    attackInc = (float) (1.0 / (0.0015 * sr));
    releasing = false;

    const double releaseSec = juce::jmap ((double) juce::jlimit (21, 108, midiNote), 21.0, 108.0, 0.60, 0.20);
    releaseMul = (float) std::exp (std::log (0.001) / (releaseSec * sr));
}

void PianoVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff && sample != nullptr)
    {
        releasing = true;
        return;
    }
    sample = nullptr;
    clearCurrentNote();
}

void PianoVoice::renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples)
{
    if (sample == nullptr)
        return;

    const auto& data = sample->data;
    const float* inL = data.getReadPointer (0);
    const float* inR = data.getReadPointer (data.getNumChannels() > 1 ? 1 : 0);
    const int len = data.getNumSamples() - 2;

    float* outL = out.getWritePointer (0);
    float* outR = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;

    for (int s = startSample; s < startSample + numSamples; ++s)
    {
        const int i = (int) pos;
        if (i >= len)
        {
            sample = nullptr;
            clearCurrentNote();
            return;
        }

        const float frac = (float) (pos - i);
        const float l = inL[i] + (inL[i + 1] - inL[i]) * frac;
        const float r = inR[i] + (inR[i + 1] - inR[i]) * frac;

        if (! releasing)
        {
            env += attackInc;
            if (env > 1.0f) env = 1.0f;
        }
        else
        {
            env *= releaseMul;
            if (env < 0.0005f)
            {
                sample = nullptr;
                clearCurrentNote();
                return;
            }
        }

        lpL += lpCoef * (l - lpL);
        lpR += lpCoef * (r - lpR);

        const float g = gain * env;
        if (outR != nullptr)
        {
            outL[s] += lpL * g;
            outR[s] += lpR * g;
        }
        else
        {
            outL[s] += 0.5f * (lpL + lpR) * g;
        }

        pos += step;
    }
}

//==============================================================================
PianoEngine::PianoEngine()
{
    for (int i = 0; i < 40; ++i)
        synth.addVoice (new PianoVoice (*bank));
    synth.addSound (new PianoSound());
    synth.setNoteStealingEnabled (true);
}

void PianoEngine::prepare (double sampleRate)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
}

void PianoEngine::render (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
}

void PianoEngine::allNotesOff()
{
    synth.allNotesOff (0, false);
}
