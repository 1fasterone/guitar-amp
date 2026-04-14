cmake_minimum_required(VERSION 3.22)
project(GuitarAmp VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(GuitarAmp
    PRODUCT_NAME "Guitar Amp"
    VERSION      "0.1.0"
)

juce_generate_juce_header(GuitarAmp)

juce_add_binary_data(GuitarAmpAssets
    SOURCES Source/bluesteel.png
)

target_sources(GuitarAmp PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
)

# --- ASIO support -----------------------------------------------------------
# Requires the free Steinberg ASIO SDK (register at steinberg.net/developers).
# Set ASIO_SDK_DIR to the folder that contains asiosdk_2.3.x/common/asio.h
# e.g.  cmake -DASIO_SDK_DIR="C:/SDKs/ASIOSDK2.3" ..
# If the variable is not set, ASIO is silently disabled and WASAPI is used.
if(DEFINED ASIO_SDK_DIR AND EXISTS "${ASIO_SDK_DIR}/common/asio.h")
    message(STATUS "ASIO SDK found at ${ASIO_SDK_DIR} — enabling ASIO support")
    target_include_directories(GuitarAmp PRIVATE "${ASIO_SDK_DIR}/common")
    set(JUCE_ASIO_FLAG JUCE_ASIO=1)
else()
    message(STATUS "ASIO SDK not found — ASIO disabled (WASAPI Exclusive recommended)")
    set(JUCE_ASIO_FLAG JUCE_ASIO=0)
endif()
# ---------------------------------------------------------------------------

target_compile_definitions(GuitarAmp PRIVATE
    ${JUCE_ASIO_FLAG}
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:GuitarAmp,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:GuitarAmp,JUCE_VERSION>"
)

target_link_libraries(GuitarAmp PRIVATE GuitarAmpAssets)

target_link_libraries(GuitarAmp PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
