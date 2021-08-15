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

cmake_minimum_required(VERSION 3.16)

project(Aeolus)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "Runtime Library")
endif()

add_subdirectory(JUCE)

set(plugin_formats
    VST3
    Standalone
)

juce_add_plugin(${PROJECT_NAME}
    VERSION "0.1.8"
    PLUGIN_MANUFACTURER_CODE "Arbe"
    PLUGIN_CODE "E6he"
    PLUGIN_DESCRIPTION "Aeolus"
    PRODUCT_NAME "Aeolus"
    COMPANY_NAME "Arthur Benilov"
    IS_SYNTH TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT TRUE
    FORMATS ${plugin_formats}
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icons/icon256.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icons/icon64.png"
)

target_link_libraries(${PROJECT_NAME}
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

juce_generate_juce_header(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "${CMAKE_CURRENT_SOURCE_DIR}/asiosdk_2.3.3_2019-06-14/common"
)

file(GLOB_RECURSE src
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

file(GLOB_RECURSE res "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.*")

juce_add_binary_data(${PROJECT_NAME}_res SOURCES ${res})
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}_res)

target_sources(${PROJECT_NAME} PRIVATE ${src})

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_ASIO=1
        JUCE_WEB_BROWSER=0
        JUSE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_AU=1)
    target_compile_options(${PROJECT_NAME} PRIVATE "-mfma")
endif()
