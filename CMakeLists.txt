cmake_minimum_required(VERSION 3.22)

# the only place the version lives. juce_add_plugin picks it up for the plugin metadata, AboutOverlay prints it as JucePlugin_VersionString
project(LDatLpMode VERSION 0.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# JUCE comes in as a git submodule. run `git submodule update --init` first.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE/CMakeLists.txt")
    message(FATAL_ERROR "third_party/JUCE is empty. run: git submodule update --init")
endif()
add_subdirectory(third_party/JUCE)

juce_add_plugin(LDatLpMode
    # PRODUCT_NAME is the file name, keep it free of spaces
    PRODUCT_NAME "L-DAT_LP_Mode"
    # name shown in the DAW
    PLUGIN_NAME "L-DAT LP Mode"
    COMPANY_NAME "MIRRORBALLPUBLISHING"
    # reverse dns of the domain we actually own. not a technical requirement, but squatting on a domain someone else may register is asking for it
    BUNDLE_ID "cc.mirrorball.ldatlpmode"
    PLUGIN_MANUFACTURER_CODE MIPP
    # AU wants exactly one upper case letter. this also changes the VST3 class id, see doc/notes/2026-09-20-macos-au-pitfalls.md
    PLUGIN_CODE Ldat
    # AU is dropped off apple by _juce_get_platform_plugin_kinds, so this list needs no platform guard
    FORMATS AU VST3 Standalone
    VST3_CATEGORIES Fx Distortion
    # 'aufx'. JUCE picks this anyway for a non synth, spelled out so the auval command line reads straight off this file
    AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    # the standalone opens an input device. macOS 10.14+ kills an app that touches audio input without this plist key
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "L-DAT LP Mode needs audio input to process it."
    # the standalone audio settings dialog has a Bluetooth MIDI button hardwired by JUCE. without this plist key TCC kills the app on click
    BLUETOOTH_PERMISSION_ENABLED TRUE
    BLUETOOTH_PERMISSION_TEXT "L-DAT LP Mode does not use MIDI. Bluetooth is only touched by the JUCE audio settings dialog."
    # ad-hoc or self-signed, no Developer ID and no notarisation. xcode generator only, XCODE_ATTRIBUTE_* means nothing to ninja
    HARDENED_RUNTIME_ENABLED TRUE
    # off everywhere. the mac pkg installs into /Library, a copy into ~/Library
    # on top of that shows up twice in the DAW. windows default dir is not writable
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(LDatLpMode PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/dsp/DatNonlinear.cpp
    src/dsp/Emphasis.cpp
    src/dsp/Lowpass32k.cpp
    src/ui/AboutOverlay.cpp
    src/ui/Fonts.cpp
    src/ui/PotLookAndFeel.cpp
    src/ui/SegmentMeter.cpp
)

# fonts go in unmodified, that keeps them Original Versions under the OFL.
# resources/fonts/Host_Grotesk/OFL.txt ships with the release, see LICENSE
juce_add_binary_data(LDatResources SOURCES
    resources/ldat.svg
    resources/about.txt
    resources/fonts/Host_Grotesk/static/HostGrotesk-SemiBold.ttf
    resources/fonts/Host_Grotesk/static/HostGrotesk-ExtraBold.ttf
)

# OFL 2 wants the font license to travel with every copy, so each mac bundle carries LICENSE and OFL.txt in Contents/Resources. resources get copied before codesign runs, so they are inside the signature
if(APPLE)
    set(kLicenseFiles
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/Host_Grotesk/OFL.txt")
    set_source_files_properties(${kLicenseFiles} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    foreach(format_target LDatLpMode_AU LDatLpMode_VST3 LDatLpMode_Standalone)
        target_sources(${format_target} PRIVATE ${kLicenseFiles})
        # JUCE sets --timestamp. apple's timestamp server may refuse a self-signed cert, and a timestamp only matters for notarisation
        set_target_properties(${format_target} PROPERTIES
            XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp=none")
    endforeach()
endif()

target_include_directories(LDatLpMode PRIVATE src)

# JUCE sources are utf-8, without this MSVC on a Japanese code page spams C4819
if(MSVC)
    target_compile_options(LDatLpMode PRIVATE /utf-8)
endif()

target_compile_definitions(LDatLpMode PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(LDatLpMode
    PRIVATE
        LDatResources
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

enable_testing()
add_subdirectory(test)