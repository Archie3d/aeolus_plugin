##  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 3 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.20)

project(Aeolus)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "Runtime Library")
endif()

option(WITH_ASIO "Enable ASIO audio interface" ON)

option(WITH_MULTIBUS_OUTPUT "Enable multibus output" OFF)

add_subdirectory(JUCE)
add_subdirectory(clap-juce-extensions EXCLUDE_FROM_ALL)

set(plugin_formats
    VST3
    Standalone
)

if(APPLE)
    list(APPEND plugin_formats AU)
elseif(UNIX AND NOT APPLE)
    list(APPEND plugin_formats LV2)
endif()

if(WITH_MULTIBUS_OUTPUT)
    set(TARGET "${PROJECT_NAME}-multibus")
    set(PRODUCT_NAME "${PROJECT_NAME}-multibus")
    set(PLUGIN_DESCRIPTION "${PRODUCT_NAME} multibus")
    set(PLUGIN_CODE "E6hf")
else()
    set(TARGET "${PROJECT_NAME}")
    set(PRODUCT_NAME "${PROJECT_NAME}")
    set(PLUGIN_DESCRIPTION "${PRODUCT_NAME} stereo")
    set(PLUGIN_CODE "E6he")
endif()


juce_add_plugin(${TARGET}
    VERSION "0.2.1"
    PLUGIN_MANUFACTURER_CODE "Arbe"
    PLUGIN_CODE ${PLUGIN_CODE}
    PLUGIN_DESCRIPTION ${PLUGIN_DESCRIPTION}
    PRODUCT_NAME ${PRODUCT_NAME}
    COMPANY_NAME "Arthur Benilov"
    BUNDLE_ID "com.ArthurBenilov.Aeolus"
    IS_SYNTH TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT TRUE
    FORMATS ${plugin_formats}
    LV2URI "https://github.com/Archie3d/aeolus_plugin"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icons/icon256.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icons/icon64.png"
)

clap_juce_extensions_plugin(TARGET ${TARGET}
    CLAP_ID "com.ArthurBenilov.Aeolus"
    CLAP_FEATURES instrument "virtual analog"
)


target_link_libraries(${TARGET}
    PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_audio_basics
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
)

juce_generate_juce_header(${TARGET})

target_include_directories(${TARGET}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

file(GLOB_RECURSE src
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

source_group(Source
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Parameters.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Parameters.cpp
)

source_group(Source\\mts
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/mts/libMTSClient.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/mts/libMTSClient.cpp
)

source_group(Source\\ui
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/CustomLookAndFeel.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/CustomLookAndFeel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/CustomMidiKeyboard.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/CustomMidiKeyboard.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/DivisionControlPanel.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/DivisionControlPanel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/DivisionView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/DivisionView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/GlobalTuningComponent.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/GlobalTuningComponent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/MidiChannelsComponent.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/MidiChannelsComponent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/MidiChannelsSelectComponent.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/MidiChannelsSelectComponent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/LevelIndicator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/LevelIndicator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/OverlayComponent.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/OverlayComponent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/ParameterSlider.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/ParameterSlider.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/SequencerView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/SequencerView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/StopButton.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/StopButton.cpp
)

source_group(Source\\aeolus
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/addsynth.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/addsynth.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/audioparam.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/audioparam.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/division.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/division.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/engine.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/engine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/globals.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/globals.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/levelmeter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/levelmeter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/list.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/memory.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/rankwave.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/rankwave.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/ringbuffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/scale.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/scale.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/sema.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/sema.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/sequencer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/sequencer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/simd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/simd.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/stop.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/stop.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/voice.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/voice.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/worker.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/worker.cpp
)

source_group(Source\\aeolus\\dsp
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/adsrenv.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/adsrenv.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/chiff.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/chiff.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/convolve.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/convolver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/convolver.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/delay.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/delay.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/fft.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/fft.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/filter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/filter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/interpolator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/interpolator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/spatial.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/aeolus/dsp/spatial.cpp
)

file(GLOB_RECURSE res "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.*")

juce_add_binary_data(${TARGET}_res SOURCES ${res})
target_link_libraries(${TARGET} PUBLIC ${TARGET}_res)

target_sources(${TARGET} PRIVATE ${src})

target_compile_definitions(${TARGET}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if(WITH_ASIO)
    target_include_directories(${TARGET}
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/asiosdk_2.3.3_2019-06-14/common"
    )

    target_compile_definitions(${TARGET} PUBLIC JUCE_ASIO=1)
else()
    target_compile_definitions(${TARGET} PUBLIC JUCE_ASIO=0)
endif()

if(WITH_MULTIBUS_OUTPUT)
    target_compile_definitions(${TARGET} PUBLIC AEOLUS_MULTIBUS_OUTPUT=1)
else()
    target_compile_definitions(${TARGET} PUBLIC AEOLUS_MULTIBUS_OUTPUT=0)
endif()

if(APPLE)
    target_compile_definitions(${TARGET} PUBLIC JUCE_AU=1)

    # This option is relevant to x86_64 target architecture,
    # but I do not see how to detect the target architecture
    # during CMake generation time. So this will cause warnings when
    # compiling for arm64.
    target_compile_options(${TARGET} PRIVATE "-mfma")
endif()
