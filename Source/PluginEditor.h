#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Sections.h"
#include "Floating.h"

//==============================================================================
class MidiSkyLook : public juce::LookAndFeel_V4
{
public:
    explicit MidiSkyLook (Fonts& f) : fonts (f) {}
    void setPalette (const Palette& p);
    juce::Font getPopupMenuFont() override { return fonts.ui (15.0f, 0); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return fonts.ui (juce::jmin (15.0f, (float) h * 0.55f), 1); }
    juce::Font getLabelFont (juce::Label&) override { return fonts.ui (14.0f, 0); }
    void drawCallOutBoxBackground (juce::CallOutBox&, juce::Graphics&, const juce::Path&, juce::Image&) override;

private:
    Fonts& fonts;
    Palette pal;
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
    void paintOverChildren (juce::Graphics&) override;
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

    // Menús
    void showKeyMenu();
    void showScaleMenu();
    void showMainMenu();
    void showMidiMenu();
    void showThemeMenu();
    void showWindowsMenu();
    void showSkinPicker();
    void showLatencyPanel();
    void showLightColourPicker();
    void showAbout();

    void previewChord (int index);
    void stopPreview();
    void setChoice (const juce::String& id, int index);
    void setBool (const juce::String& id, bool value);
    bool isStandalone() const;

    MidiSkyProcessor& proc;
    juce::SharedResourcePointer<Fonts> fonts;
    juce::SharedResourcePointer<Prefs> prefs;
    juce::SharedResourcePointer<Textures> textures;
    MidiSkyLook look;

    Vis vis;
    UiContext ctx { vis, *fonts, *textures };

    RailBar rail { ctx };
    CircleSection circle { ctx };
    ScoreSection score { ctx };
    TabletsSection tablets { ctx };
    ChordSection chord { ctx };
    KeyboardSection keyboard { ctx };

    std::map<juce::String, std::unique_ptr<FloatingWindow>> windows;
    juce::Component::SafePointer<juce::ColourSelector> colourSelector;

    juce::Image background;
    juce::Rectangle<int> atrilBounds;
    float k = 1.0f;

    std::unique_ptr<juce::VBlankAttachment> vblank;
    std::unique_ptr<juce::ParameterAttachment> volumeAttachment;

    // Detección automática de tonalidad
    std::array<float, 12> hist {};
    int autoRoot = 0;
    bool autoMinor = false;
    bool autoHas = false;

    double lastFrame = 0.0;
    std::vector<int> previewNotes;
    double previewOffAt = 0.0;
    juce::Array<juce::MidiDeviceInfo> knownMidiInputs;
    bool restoredFloats = false;

    int lastRoot = -1, lastScale = -1;
    bool lastLatin = false, lastSevenths = false, lastHighlight = true, lastPedal = false, lastAuto = true, lastAutoHas = false;
    std::vector<int> lastActive;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSkyEditor)
};
