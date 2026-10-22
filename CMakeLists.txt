cmake_minimum_required(VERSION 3.22)

# The JUCE build of Samplebrain: the original brain engine (brain/src) behind a
# new interface in the Sungam graphite style. The Qt build (samplebrain.pro) is
# left as it was.
#
# Nothing to install first: JUCE replaces PortAudio (audio devices) and
# libsndfile (file decoding), an in-process bus replaces liblo, and the FFT is
# the vendored, header-only pocketfft.

# Must precede project(): these drive the compiler checks. Universal binary;
# arm64 macOS starts at 11.0, so the deployment target has to clear that.
if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING
            "macOS build architectures" FORCE)
    endif()
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING
            "Minimum macOS version" FORCE)
    endif()
endif()

project(Samplebrain VERSION 0.19.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: either an absolute checkout (-DJUCE_DIR=...) or the macOS default
# install location.
set(JUCE_DIR "/Applications/JUCE" CACHE PATH "Path to a JUCE source checkout")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR} — pass -DJUCE_DIR=/path/to/JUCE")
endif()
add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)

# -- The brain engine, unchanged apart from its I/O seams ----------------------
add_library(brain STATIC
    brain/src/block.cpp
    brain/src/brain.cpp
    brain/src/fft.cpp
    brain/src/mfcc.cpp
    brain/src/renderer.cpp
    brain/src/search_params.cpp
    brain/src/status.cpp
    brain/src/window.cpp
    brain/src/block_stream.cpp
    brain/src/aquila/filter/MelFilterBank.cpp
    brain/src/aquila/filter/MelFilter.cpp
    brain/src/aquila/transform/Dct.cpp
    brain/src/spiralcore/sample.cpp
    brain/src/spiralcore/ring_buffer.cpp
    brain/src/spiralcore/command_ring_buffer.cpp
    brain/src/spiralcore/local_bus.cpp
    brain/src/spiralcore/allocator.cpp
    brain/src/spiralcore/stream.cpp
    app/process_thread.cpp)

target_include_directories(brain PUBLIC
    brain/src
    app
    third_party/pocketfft)
target_compile_definitions(brain PUBLIC
    SAMPLEBRAIN_LOCAL_BUS=1
    SAMPLEBRAIN_POCKETFFT=1
    SAMPLEBRAIN_HOST_SOUNDFILE=1)
# Old code, kept as it is: silence its warnings rather than edit it.
target_compile_options(brain PRIVATE -O3 -w)
find_package(Threads REQUIRED)
target_link_libraries(brain PUBLIC Threads::Threads)

# -- The app --------------------------------------------------------------------
juce_add_gui_app(Samplebrain
    PRODUCT_NAME            "Samplebrain"
    COMPANY_NAME            "Sungam"
    BUNDLE_ID               "is.sungam.samplebrain"
    ICON_BIG                "${CMAKE_CURRENT_SOURCE_DIR}/desktop/samplebrain.png"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Samplebrain can listen to an input as its target.")

target_sources(Samplebrain PRIVATE
    Source/Main.cpp
    Source/Engine.cpp
    Source/MainComponent.cpp
    Source/ui/LookAndFeel.cpp
    Source/ui/Controls.cpp
    Source/ui/SoundList.cpp
    Source/ui/TargetView.cpp)

target_compile_definitions(Samplebrain PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Samplebrain,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Samplebrain,JUCE_VERSION>")

target_include_directories(Samplebrain PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_link_libraries(Samplebrain
    PRIVATE
        brain
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
