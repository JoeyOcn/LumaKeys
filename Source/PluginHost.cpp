// Los encabezados de Apple van antes que JUCE para que no choquen nombres (Point, AudioBuffer…)
#if defined (__APPLE__)
 #include <AudioToolbox/AudioToolbox.h>
#endif

#include "PluginHost.h"

using namespace juce;

#if JUCE_MAC
// Lista los instrumentos Audio Unit sin abrir ninguno (rápido y sin trabar la ventana).
// Para eso se leen sus datos directo del sistema, con el mismo formato que usa JUCE.
static String osTypeString (OSType t)
{
    const juce_wchar s[4] = { (juce_wchar) ((t >> 24) & 0xff), (juce_wchar) ((t >> 16) & 0xff),
                              (juce_wchar) ((t >> 8) & 0xff), (juce_wchar) (t & 0xff) };
    return String (s, 4);
}

static void listAudioUnitInstruments (KnownPluginList& known, std::function<void (const String&)> progress)
{
    AudioComponentDescription want {};
    want.componentType = kAudioUnitType_MusicDevice;
    AudioComponent comp = nullptr;
    while ((comp = AudioComponentFindNext (comp, &want)) != nullptr)
    {
        AudioComponentDescription cd {};
        if (AudioComponentGetDescription (comp, &cd) != noErr) continue;

        String full;
        CFStringRef cfName = nullptr;
        if (AudioComponentCopyName (comp, &cfName) == noErr && cfName != nullptr)
        {
            full = String::fromCFString (cfName);
            CFRelease (cfName);
        }
        String manufacturer = full.upToFirstOccurrenceOf (":", false, false).trim();
        String name = full.fromFirstOccurrenceOf (":", false, false).trim();
        if (name.isEmpty()) { name = full; manufacturer = {}; }
        if (progress) progress (name);

        UInt32 version = 0;
        AudioComponentGetVersion (comp, &version);

        PluginDescription d;
        d.name = d.descriptiveName = name;
        d.manufacturerName = manufacturer;
        d.pluginFormatName = "AudioUnit";
        d.category = "Synths";
        d.fileOrIdentifier = "AudioUnit:Synths/" + osTypeString (cd.componentType) + "," + osTypeString (cd.componentSubType) + "," + osTypeString (cd.componentManufacturer);
        d.uniqueId = d.deprecatedUid = ((int) cd.componentType) ^ ((int) cd.componentSubType) ^ ((int) cd.componentManufacturer);
        d.version = String ((version >> 16) & 0xffff) + "." + String ((version >> 8) & 0xff) + "." + String (version & 0xff);
        d.isInstrument = true;
        d.numInputChannels = 0;
        d.numOutputChannels = 2;
        d.lastInfoUpdateTime = Time::getCurrentTime();
        known.addType (d);
    }
}
#endif

static File hostDataDir()
{
   #if JUCE_MAC
    return File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Application Support").getChildFile ("Midi Cat");
   #else
    return File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Midi Cat");
   #endif
}

PluginHost::PluginHost() : Thread ("Midi Cat: buscar plugins")
{
    formats.addDefaultFormats();
    dataDir = hostDataDir();
    dataDir.createDirectory();
    listFile = dataDir.getChildFile ("instrumentos.xml");
    pedalFile = dataDir.getChildFile ("plugin-que-fallo.txt");
    if (auto xml = parseXML (listFile))
        known.recreateFromXml (*xml);
}

PluginHost::~PluginHost()
{
    *alive = false;
    stopThread (8000);
    const ScopedLock sl (lock);
    if (inst != nullptr) inst->releaseResources();
    inst.reset();
}

//==============================================================================
void PluginHost::startScan()
{
    if (scanning.exchange (true)) return;
    progress.store (0.0f);
    startThread();
}

String PluginHost::scanningName() const
{
    const ScopedLock sl (nameLock);
    return scanName;
}

void PluginHost::run()
{
   #if JUCE_MAC
    // Audio Units: se listan sin abrirlos (si se abrieran, cada uno trabaría la ventana)
    listAudioUnitInstruments (known, [this] (const String& n)
    {
        const ScopedLock sl (nameLock);
        scanName = n;
    });
    progress.store (0.15f);
   #endif

    // VST3: se revisan en este hilo aparte (la ventana sigue libre)
    std::vector<AudioPluginFormat*> list;
    for (int i = 0; i < formats.getNumFormats(); ++i)
        if (formats.getFormat (i)->getName() == "VST3")
            list.push_back (formats.getFormat (i));

    for (size_t fi = 0; fi < list.size() && ! threadShouldExit(); ++fi)
    {
        auto* f = list[fi];
        PluginDirectoryScanner scanner (known, *f, f->getDefaultLocationsToSearch(), true, pedalFile, true);
        String name;
        for (;;)
        {
            if (threadShouldExit()) break;
            {
                const ScopedLock sl (nameLock);
                scanName = File::createFileWithoutCheckingPath (scanner.getNextPluginFileThatWillBeScanned()).getFileNameWithoutExtension();
            }
            const bool more = scanner.scanNextFile (true, name);
            progress.store (0.15f + 0.85f * ((float) fi + scanner.getProgress()) / (float) jmax ((size_t) 1, list.size()));
            if (! more) break;
        }
    }
    saveList();
    {
        const ScopedLock sl (nameLock);
        scanName = {};
    }
    progress.store (1.0f);
    scanning.store (false);

    std::weak_ptr<bool> w = alive;
    MessageManager::callAsync ([this, w]
    {
        if (auto a = w.lock()) if (*a && onListChanged) onListChanged();
    });
}

void PluginHost::saveList() const
{
    if (auto xml = known.createXml())
        xml->writeTo (listFile);
}

Array<PluginDescription> PluginHost::instruments() const
{
    Array<PluginDescription> out;
    for (auto& d : known.getTypes())
    {
        if (! d.isInstrument) continue;
        bool self = d.name.containsIgnoreCase ("Midi Cat") || d.name.containsIgnoreCase ("MidiSky");
       #if MIDISKY_DEMO
        if (SystemStats::getEnvironmentVariable ("MIDISKY_ALLOW_SELF", "").isNotEmpty()) self = false;
       #endif
        if (self) continue;   // no a sí mismo
        out.add (d);
    }
    std::sort (out.begin(), out.end(), [] (const PluginDescription& a, const PluginDescription& b)
    {
        const int c = a.name.compareIgnoreCase (b.name);
        return c != 0 ? c < 0 : a.pluginFormatName < b.pluginFormatName;
    });
    return out;
}

//==============================================================================
void PluginHost::allocateScratch()
{
    int ch = 2;
    if (inst != nullptr)
        ch = jmax (2, inst->getTotalNumInputChannels(), inst->getTotalNumOutputChannels());
    scratch.setSize (ch, jmax (block, 4096), false, true, false);
}

void PluginHost::load (const PluginDescription& desc, std::function<void (const String&)> done, const MemoryBlock* stateToRestore)
{
    const int id = ++loadId;
    std::weak_ptr<bool> w = alive;
    std::shared_ptr<MemoryBlock> state;
    if (stateToRestore != nullptr) state = std::make_shared<MemoryBlock> (*stateToRestore);

    formats.createPluginInstanceAsync (desc, rate, block,
        [this, w, id, desc, done, state] (std::unique_ptr<AudioPluginInstance> p, const String& error)
        {
            auto a = w.lock();
            if (a == nullptr || ! *a) return;
            if (id != loadId) return;   // ya se pidió otro
            if (p == nullptr)
            {
                if (done) done (error.isNotEmpty() ? error : String ("No se pudo abrir."));
                return;
            }
            p->enableAllBuses();
            if (p->getBusCount (false) > 0)
            {
                auto layout = p->getBusesLayout();
                if (! layout.outputBuses.isEmpty())
                {
                    layout.outputBuses.getReference (0) = AudioChannelSet::stereo();
                    if (p->checkBusesLayoutSupported (layout)) p->setBusesLayout (layout);
                }
            }
            if (state != nullptr && state->getSize() > 0)
                p->setStateInformation (state->getData(), (int) state->getSize());
            p->setRateAndBufferSizeDetails (rate, block);
            p->prepareToPlay (rate, block);

            if (onBeforeChange) onBeforeChange();
            std::unique_ptr<AudioPluginInstance> old;
            {
                const ScopedLock sl (lock);
                old = std::move (inst);
                inst = std::move (p);
                current = desc;
                allocateScratch();
                loaded.store (true);
            }
            if (old != nullptr) old->releaseResources();
            old.reset();
            if (onChanged) onChanged();
            if (done) done ({});
        });
}

void PluginHost::unload()
{
    ++loadId;
    if (inst == nullptr) return;
    if (onBeforeChange) onBeforeChange();
    std::unique_ptr<AudioPluginInstance> old;
    {
        const ScopedLock sl (lock);
        old = std::move (inst);
        current = {};
        loaded.store (false);
    }
    if (old != nullptr) old->releaseResources();
    old.reset();
    if (onChanged) onChanged();
}

std::unique_ptr<XmlElement> PluginHost::createState() const
{
    if (inst == nullptr) return {};
    auto xml = std::make_unique<XmlElement> ("HOSTED");
    if (auto d = current.createXml()) xml->addChildElement (d.release());
    MemoryBlock mb;
    inst->getStateInformation (mb);
    xml->setAttribute ("state", mb.toBase64Encoding());
    return xml;
}

void PluginHost::restoreState (const XmlElement& xml)
{
    PluginDescription d;
    if (auto* dx = xml.getChildByName ("PLUGIN"))
        if (d.loadFromXml (*dx))
        {
            MemoryBlock mb;
            mb.fromBase64Encoding (xml.getStringAttribute ("state"));
            load (d, {}, &mb);
        }
}

//==============================================================================
void PluginHost::prepare (double sampleRate, int blockSize)
{
    const ScopedLock sl (lock);
    rate = sampleRate;
    block = jmax (32, blockSize);
    if (inst != nullptr)
    {
        inst->releaseResources();
        inst->setRateAndBufferSizeDetails (rate, block);
        inst->prepareToPlay (rate, block);
    }
    allocateScratch();
}

void PluginHost::release()
{
    const ScopedLock sl (lock);
    if (inst != nullptr) inst->releaseResources();
}

bool PluginHost::render (juce::AudioBuffer<float>& out, MidiBuffer& midi, int n) noexcept
{
    const ScopedTryLock sl (lock);
    if (! sl.isLocked() || inst == nullptr) return false;
    const int ch = jmax (2, inst->getTotalNumInputChannels(), inst->getTotalNumOutputChannels());
    if (scratch.getNumChannels() < ch || scratch.getNumSamples() < n) return false;

    juce::AudioBuffer<float> view (scratch.getArrayOfWritePointers(), ch, n);
    view.clear();
    inst->processBlock (view, midi);

    const int outs = jmax (1, inst->getTotalNumOutputChannels());
    for (int c = 0; c < out.getNumChannels(); ++c)
        out.addFrom (c, 0, view, jmin (c, outs - 1), 0, n);
    return true;
}
