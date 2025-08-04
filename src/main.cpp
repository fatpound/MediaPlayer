#include <GStreamer/Manager.hpp>

#include "MediaPlayerWrapper.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    gstreamer::Manager gst_mgr{ argc, argv };

    qmlRegisterType<media_player::MediaPlayerWrapper>("Custom.MediaPlayer", 1, 0, "MediaPlayerWrapper");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
    Qt::QueuedConnection);
    engine.loadFromModule("MediaPlayer", "Main");

    return app.exec();
}
