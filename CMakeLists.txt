cmake_minimum_required(VERSION 3.22)

set(FILO_VERSION "0.1.0")
find_program(GIT_EXECUTABLE git)
if(GIT_EXECUTABLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_LATEST_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(GIT_LATEST_TAG)
        string(REGEX MATCH "[0-9]+(\\.[0-9]+)?(\\.[0-9]+)?" GIT_NUMERIC "${GIT_LATEST_TAG}")
        if(GIT_NUMERIC)
            set(FILO_VERSION "${GIT_NUMERIC}")
        endif()
    endif()
endif()
message(STATUS "Filo project version: ${FILO_VERSION}")

project(Filo VERSION ${FILO_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    OPTIONS
        "JUCE_BUILD_EXAMPLES OFF"
        "JUCE_BUILD_EXTRAS OFF"
)

juce_add_gui_app(Filo
    BUNDLE_ID                       "audio.tondo.filo"
    PRODUCT_NAME                    "Tondo Host"
    COMPANY_NAME                    "Tondo Audio"
    MICROPHONE_PERMISSION_ENABLED   TRUE
    MICROPHONE_PERMISSION_TEXT      "Tondo Host needs audio input access to process your guitar signal."
)

target_sources(Filo PRIVATE
    src/Main.cpp
    src/MainWindow.cpp
    src/MainComponent.cpp
    src/audio/AudioEngine.cpp
    src/plugins/PluginManager.cpp
    src/plugins/PluginScannerSubprocess.cpp
    src/ui/DeviceSelectorBar.cpp
    src/ui/LevelMeter.cpp
    src/ui/PluginChainView.cpp
)

target_include_directories(Filo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(Filo PRIVATE
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Filo,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Filo,JUCE_VERSION>"
)

set(FILO_VERSION_HEADER ${CMAKE_BINARY_DIR}/filo_version.h)
add_custom_target(filo_version ALL
    BYPRODUCTS ${FILO_VERSION_HEADER}
    COMMAND ${CMAKE_COMMAND}
        -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DOUTPUT_FILE=${FILO_VERSION_HEADER}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersion.cmake
    COMMENT "Refreshing Filo version header from git")
add_dependencies(Filo filo_version)
target_include_directories(Filo PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(Filo PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_devices
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
