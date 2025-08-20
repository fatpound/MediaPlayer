cmake_minimum_required(VERSION 3.19)

project(MediaPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES FALSE)

find_package(Qt6 REQUIRED COMPONENTS Quick Core)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_resources(MEDIA_RESOURCES resources.qrc)

qt_add_executable(appMediaPlayer

    include/GStreamer/CommonUtilities.hpp
    include/GStreamer/MediaPlayer.hpp
    include/GStreamer/Manager.hpp
    include/GStreamer/Pipeline.hpp
    include/MediaPlayerWrapper.hpp

    src/GStreamer/Manager.cpp
    src/GStreamer/MediaPlayer.cpp
    src/GStreamer/Pipeline.cpp
    src/MediaPlayerWrapper.cpp

    src/main.cpp

    _macros/Logging.hpp
    _macros/Namespaces.hpp

    _misc/FatCodex/Macros/ExternalWarnings_MSVC.hpp

    ${MEDIA_RESOURCES}
)

qt_add_qml_module(appMediaPlayer
    URI MediaPlayer
    VERSION 1.0
    RESOURCES
        resources.qrc
    QML_FILES
        Main.qml
        MPProgressBar.qml
        MPPlaybackControls.qml
        MPMenuBar.qml
        MPContentView.qml
)

include(_misc/FatCodex/CompileOptions.cmake)

set(GSTREAMER_ROOT $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})

target_include_directories(appMediaPlayer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/_macros
        ${PROJECT_SOURCE_DIR}/include
        ${GSTREAMER_ROOT}/include/gstreamer-1.0
        ${GSTREAMER_ROOT}/include/glib-2.0
        ${GSTREAMER_ROOT}/include/FLAC
        ${GSTREAMER_ROOT}/lib/glib-2.0/include
)

target_link_directories(appMediaPlayer
    PUBLIC
        ${GSTREAMER_ROOT}/lib
)

target_link_libraries(appMediaPlayer
    PRIVATE
        FatCodex::CompileOptions
        glib-2.0.lib
        gobject-2.0.lib
        gstreamer-1.0.lib
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appMediaPlayer PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appMediaPlayer
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appMediaPlayer
    PRIVATE
        Qt6::Core
        Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS appMediaPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
