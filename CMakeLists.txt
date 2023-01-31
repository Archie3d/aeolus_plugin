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

set(plugin_formats
    VST3
    Standalone
)

if(APPLE)
    list(APPEND plugin_formats AU)
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
    VERSION "0.2.0"
    PLUGIN_MANUFACTURER_CODE "Arbe"
    PLUGIN_CODE ${PLUGIN_CODE}
    PLUGIN_DESCRIPTION ${PLUGIN_DESCRIPTION}
    PRODUCT_NAME ${PRODUCT_NAME}
    COMPANY_NAME "Arthur Benilov"
    IS_SYNTH TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT TRUE
    FORMATS ${plugin_formats}
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icons/icon256.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icons/icon64.png"
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

file(GLOB_RECURSE res "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.*")

juce_add_binary_data(${TARGET}_res SOURCES ${res})
target_link_libraries(${TARGET} PUBLIC ${TARGET}_res)

target_sources(${TARGET} PRIVATE ${src})

target_compile_definitions(${TARGET}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
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
    target_compile_options(${TARGET} PRIVATE "-mfma")
endif()
