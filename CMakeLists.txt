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
    PRODUCT_NAME                    "Filo"
    COMPANY_NAME                    "Tondo Audio"
    ICON_BIG                        "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo/app_icon.png"
    MICROPHONE_PERMISSION_ENABLED   TRUE
    MICROPHONE_PERMISSION_TEXT      "Filo needs audio input access to process your guitar signal."
)

set(FILO_ASSET_SOURCES
    resources/fonts/InterVariable.ttf
)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo/logo.svg")
    list(APPEND FILO_ASSET_SOURCES resources/logo/logo.svg)
    target_compile_definitions(Filo PRIVATE FILO_HAS_LOGO=1)
endif()

juce_add_binary_data(FiloAssets
    HEADER_NAME FiloAssets.h
    NAMESPACE filo::assets
    SOURCES ${FILO_ASSET_SOURCES}
)

target_sources(Filo PRIVATE
    src/Main.cpp
    src/MainWindow.cpp
    src/MainComponent.cpp
    src/audio/AudioEngine.cpp
    src/plugins/PluginManager.cpp
    src/plugins/PluginScannerSubprocess.cpp
    src/ui/DeviceSelectorBar.cpp
    src/ui/FiloLookAndFeel.cpp
    src/ui/IconButton.cpp
    src/ui/IconPaths.cpp
    src/ui/LevelMeter.cpp
    src/ui/LogoView.cpp
    src/ui/PluginChainView.cpp
    src/ui/PluginPicker.cpp
    src/ui/ScanOverlay.cpp
)

target_include_directories(Filo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(Filo PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Filo,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Filo,JUCE_VERSION>"
)

if(APPLE)
    target_compile_definitions(Filo PRIVATE JUCE_PLUGINHOST_AU=1)
endif()

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
    FiloAssets
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_devices
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

if(APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo/app_icon.png")
    add_custom_command(TARGET Filo POST_BUILD
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cmake/make_icns.sh"
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo/app_icon.png"
                "$<TARGET_FILE_DIR:Filo>/../Resources/Icon.icns"
        VERBATIM
        COMMENT "Generating multi-resolution Icon.icns")
endif()
