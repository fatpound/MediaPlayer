#include <_exp/GStreamer/Manager.hpp>

#include "MediaPlayerWrapper.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

auto main(int argc, char *argv[]) -> int
{
    QGuiApplication app(argc, argv);

    const auto gst_mgr = fatx::gstreamer::Manager{ argc, argv };

    qmlRegisterType<media_player::MediaPlayerWrapper>("FatPound.MediaPlayer", 0, 1, "MediaPlayerWrapper");

    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("MediaPlayer", "Main");

    return QGuiApplication::exec();
}
