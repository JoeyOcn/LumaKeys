cmake_minimum_required(VERSION 3.22)

# Windows: runtime estático para que el plugin no dependa de instalar nada extra.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS mínimo")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Binario universal" FORCE)
    endif()
endif()

project(MidiSky VERSION 1.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: se descarga solo. Para usar una copia local: -DFETCHCONTENT_SOURCE_DIR_JUCE=/ruta/a/JUCE
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.9
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(MIDISKY_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND MIDISKY_FORMATS AU)
endif()

juce_add_plugin(MidiSky
    PRODUCT_NAME "MidiSky"
    COMPANY_NAME "Joey Ocn"
    COMPANY_WEBSITE "https://github.com/JoeyOcn"
    BUNDLE_ID "com.joeyocn.midisky"
    PLUGIN_MANUFACTURER_CODE Jocn
    PLUGIN_CODE Msky
    FORMATS ${MIDISKY_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Tools
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    DESCRIPTION "Visualizador de escalas, acordes y notas con piano"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/icon_1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/icon_256.png"
    MICROPHONE_PERMISSION_ENABLED FALSE)

juce_generate_juce_header(MidiSky)

target_sources(MidiSky PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PianoEngine.cpp
    Source/Theory.cpp
    Source/Views.cpp
    Source/Sections.cpp
    Source/Floating.cpp
    Source/ThemeArt.cpp
    Source/Style.cpp)

file(GLOB MIDISKY_PIANO_SAMPLES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/piano/*.ogg)
file(GLOB MIDISKY_TEXTURES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/tex/*.jpg)
juce_add_binary_data(MidiSkyData
    HEADER_NAME MidiSkyData.h
    NAMESPACE MidiSkyData
    SOURCES
        ${MIDISKY_PIANO_SAMPLES}
        ${MIDISKY_TEXTURES}
        assets/fonts/Bravura.otf
        assets/fonts/ShipporiMincho-Bold.ttf
        assets/fonts/ShipporiMincho-ExtraBold.ttf
        assets/fonts/ZenKakuGothicNew-Regular.ttf
        assets/fonts/ZenKakuGothicNew-Medium.ttf
        assets/fonts/ZenKakuGothicNew-Bold.ttf
        assets/fonts/PressStart2P-Regular.ttf
        assets/fonts/Jersey10-Regular.ttf)

if(MSVC)
    target_compile_options(MidiSky PRIVATE /utf-8)
else()
    set_target_properties(MidiSkyData PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

option(MIDISKY_DEMO "Modo de prueba (notas por variable de entorno)" OFF)
if(MIDISKY_DEMO)
    target_compile_definitions(MidiSky PUBLIC MIDISKY_DEMO=1)
endif()

target_compile_definitions(MidiSky PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_OGGVORBIS=1
    JUCE_MODAL_LOOPS_PERMITTED=0)

target_link_libraries(MidiSky
    PRIVATE
        MidiSkyData
        juce::juce_audio_utils
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
