cmake_minimum_required(VERSION 3.19)

project(MediaPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES FALSE)

find_package(Qt6 REQUIRED COMPONENTS Quick Core)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_resources(MEDIA_RESOURCES resources.qrc)

qt_add_executable(MediaPlayer

    external/FatLibrary/_exp/GStreamer/AudioEffectChain.hpp
    external/FatLibrary/_exp/GStreamer/AudioPitchEffect.hpp
    external/FatLibrary/_exp/GStreamer/Common.hpp
    external/FatLibrary/_exp/GStreamer/FatGst.hpp
    external/FatLibrary/_exp/GStreamer/GStreamer.hpp
    external/FatLibrary/_exp/GStreamer/IAudioEffect.hpp
    external/FatLibrary/_exp/GStreamer/IEffectChain.hpp
    external/FatLibrary/_exp/GStreamer/Manager.hpp
    external/FatLibrary/_exp/GStreamer/MediaPlayer.hpp
    external/FatLibrary/_exp/GStreamer/Pipeline.hpp

    include/_macros/Namespaces.hpp
    include/MediaPlayerWrapper.hpp

    src/MediaPlayerWrapper.cpp
    src/main.cpp

    ${MEDIA_RESOURCES}
)

set_source_files_properties(
    MPTheme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(MediaPlayer
    URI MediaPlayer
    VERSION 1.0
    RESOURCES
        resources.qrc
    QML_FILES
        Main.qml
        MPMenuBar.qml
        MPPitchControlPopup.qml
        MPPlaybackControls.qml
        MPSettingsButton.qml
        MPProgressBar.qml
        MPTheme.qml
)

include(external/FatLibrary/_misc/FatCodex/CompileOptions.cmake)

set(GSTREAMER_ROOT $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})

target_include_directories(MediaPlayer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/external/FatLibrary
        ${GSTREAMER_ROOT}/include/gstreamer-1.0
        ${GSTREAMER_ROOT}/include/glib-2.0
        ${GSTREAMER_ROOT}/lib/glib-2.0/include
)

target_link_directories(MediaPlayer
    PUBLIC
        ${GSTREAMER_ROOT}/lib
)

target_link_libraries(MediaPlayer
    PRIVATE
        FatCodex::CompileOptions
        glib-2.0.lib
        gobject-2.0.lib
        gstreamer-1.0.lib
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(MediaPlayer PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.MediaPlayer
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(MediaPlayer
    PRIVATE
        Qt6::Core
        Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS MediaPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
