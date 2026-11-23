#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Sections.h"
#include "Floating.h"
#include "Extras.h"
#include "PianoRoll.h"

//==============================================================================
class MidiSkyLook : public juce::LookAndFeel_V4
{
public:
    explicit MidiSkyLook (Fonts& f) : fonts (f) {}
    void setPalette (const Skin& p);
    juce::Font getPopupMenuFont() override { return fonts.ui (16.0f, 1); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return fonts.ui (juce::jmin (15.0f, (float) h * 0.55f), 1); }
    juce::Font getLabelFont (juce::Label&) override { return fonts.ui (14.5f, 0); }
    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

private:
    Fonts& fonts;
    Skin pal;
};

//==============================================================================
// Toca una grabación (para escucharla antes de arrastrarla)
class ClipPlayer : private juce::HighResolutionTimer
{
public:
    explicit ClipPlayer (juce::MidiKeyboardState& s) : state (s) {}
    ~ClipPlayer() override { stop(); }
    void play (const juce::File&);
    void stop();
    bool isPlaying() const { return isTimerRunning(); }

private:
    void hiResTimerCallback() override;
    struct Ev { double t; int note; float vel; bool on; };
    juce::MidiKeyboardState& state;
    std::vector<Ev> events;
    size_t next = 0;
    double startMs = 0.0;
    std::array<int, 128> held {};
};

// Ventana con la interfaz de tu instrumento VST3/AU
class HostedWindow : public juce::DocumentWindow
{
public:
    HostedWindow (const juce::String& name, juce::Component* editor, juce::Colour bg);
    void closeButtonPressed() override { if (onClose) onClose(); }
    std::function<void()> onClose;
};

//==============================================================================
class MidiSkyEditor : public juce::AudioProcessorEditor,
                      private juce::Timer,
                      private juce::ChangeListener
{
public:
    explicit MidiSkyEditor (MidiSkyProcessor&);
    ~MidiSkyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void frame();

    void refreshPalette();
    void rebuildBackground();
    void updateKeyState (bool& keyChanged);
    void updateMidiStatus();
    void applyLowLatencyOnce();
    void applyRange (int startC, int octaves);

    // Secciones y ventanas
    std::vector<Section*> sections();
    Section* sectionById (const juce::String& id);
    void detach (Section& s, bool save = true);
    void dock (Section& s, bool save = true);
    void restoreFloating();
    void recreateFloating();
    juce::Rectangle<int> defaultFloatBounds (Section& s);
    void setTransparent (bool on);
    void setHideParams (bool on);
    void resetLayout();
    void fitHeightToContent (float keepK);
    float naturalHeight() const;
    bool anyFloating();
    void updateResetButton();
    void setDayNight (int mode);

    // Paneles y menús
    void showPopover (const juce::String& id, const juce::String& title, std::unique_ptr<juce::Component> content, juce::Component& anchor);
    void closePopover();
    void showKeyMenu();
    void showScaleMenu();
    void showMainMenu();
    void showMidiMenu();
    void showWindowsMenu();
    void showSoundMenu();
    void showInterfacePanel (juce::Component& anchor);
    void showKeysPanel (juce::Component& anchor);
    void showLatencyPanel();
    void showAbout();
    void showMetronomePanel (juce::Component& anchor);
    void showRecordingsPanel (juce::Component& anchor);
    void showPluginsPanel (juce::Component& anchor);
    void openHostedWindow();
    void openPianoRoll (juce::File f);   // copia: el panel que la pide se cierra
    void newMidiFile();
    void saveLayoutPreset (const juce::String& name);
    void applyLayoutPreset (const LayoutPreset&);
    void updateTransport (float dt);
    juce::PopupMenu::Options menuOptions (juce::Component& target, int minWidth = 240);

    void previewChord (int index, float velocity = 0.68f);
    void stopPreview();
    void setChoice (const juce::String& id, int index);
    void setBool (const juce::String& id, bool value);
    bool isStandalone() const;

    MidiSkyProcessor& proc;
    juce::SharedResourcePointer<Fonts> fonts;
    juce::SharedResourcePointer<Prefs> prefs;
    MidiSkyLook look;

    Vis vis;
    UiContext ctx { vis, *fonts };

    RailBar rail { ctx };
    ScoreSection score { ctx };
    ScreenSection screen { ctx };
    DegreesSection degrees { ctx };
    CircleSection circle { ctx };
    KeyboardSection keyboard { ctx };

    std::map<juce::String, std::unique_ptr<FloatingWindow>> windows;
    std::unique_ptr<Popover> popover;
    std::unique_ptr<HostedWindow> hostedWindow;
    std::unique_ptr<PianoRollWindow> rollWindow;
    ClipPlayer clipPlayer { proc.uiKeyboard };
    uint32_t lastBeatCount = 0;
    float metroFlash = 0.0f;
    int lastRecState = 0;
    bool lastLatched = false, lastPedalShown = false;

    juce::Image background;
    float k = 1.0f;
    bool lastNight = false;
    int nightCheckCounter = 0;
    juce::String demoPopover;

    std::unique_ptr<juce::VBlankAttachment> vblank;
    std::unique_ptr<juce::ParameterAttachment> volumeAttachment;

    // Detección automática de tonalidad
    std::array<float, 12> hist {};
    int autoRoot = 0;
    bool autoMinor = false;
    bool autoHas = false;

    double lastFrame = 0.0;
    double startTime = 0.0;
    std::vector<int> previewNotes;
    double previewOffAt = 0.0, previewStartedAt = 0.0;
    juce::Array<juce::MidiDeviceInfo> knownMidiInputs;
    bool restoredFloats = false;

    int lastRoot = -1, lastScale = -1;
    bool lastLatin = false, lastSevenths = false, lastHighlight = true, lastAuto = true, lastAutoHas = false;
    std::vector<int> lastActive;

    // Qué notas forman "el acorde de ahora" (con pedal, las viejas no se mezclan)
    std::array<double, 128> onsetMs {}, releaseMs {};
    double phraseStartMs = 0.0, lastOnsetMs = 0.0;
    bool lastPedalSeen = false;
    std::vector<int> candidate;
    double candidateSince = 0.0;
    std::vector<int> chordSetNow (double now);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSkyEditor)
};
