#include <GStreamer/Manager.hpp>

#include "MediaPlayerWrapper.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

// NOLINTBEGIN(readability-static-accessed-through-instance)

auto main(int argc, char *argv[]) -> int
{
    QGuiApplication app(argc, argv);

    const gstreamer::Manager gst_mgr{ argc, argv };

    qmlRegisterType<media_player::MediaPlayerWrapper>("Custom.MediaPlayer", 1, 0, "MediaPlayerWrapper");

    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("MediaPlayer", "Main");

    return app.exec();
}

// NOLINTEND(readability-static-accessed-through-instance)
