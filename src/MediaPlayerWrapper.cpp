#include "MediaPlayerWrapper.hpp"

#include <QDebug>
#include <QUrl>
#include <QString>

#include <cstddef>

MP_BEGIN_NAMESPACE

MediaPlayerWrapper::MediaPlayerWrapper(QObject *parent)
    :
    QObject{parent}
{
    m_player_.SetStateChangedCallback(
        [this](const bool isPlaying)
        {
            QMetaObject::invokeMethod(this, "onPlayerStateChanged_", Qt::QueuedConnection, Q_ARG(bool, isPlaying));
        }
    );

    m_player_.SetMediaChangedCallback(
        [this]()
        {
            QMetaObject::invokeMethod(this, "onDurationChanged_", Qt::QueuedConnection);
        }
    );

    m_position_timer_.setInterval(100);
    m_position_timer_.setTimerType(Qt::PreciseTimer);
    m_position_timer_.setSingleShot(false);

    connect(
        &m_position_timer_,
        &QTimer::timeout,
        this,
        &MediaPlayerWrapper::PollPosition_
    );
}


auto MediaPlayerWrapper::position() const -> qint64
{
    return static_cast<qint64>(m_player_.QueryPosition() / 1000000);
}

auto MediaPlayerWrapper::duration() const -> qint64
{
    return static_cast<qint64>(m_player_.QueryDuration() / 1000000);
}

auto MediaPlayerWrapper::isPlaying() const -> bool
{
    return m_is_playing_;
}

void MediaPlayerWrapper::loadAudio(const QString& uriPath)
{
    m_player_.LoadAudio(QString::fromUtf8(QUrl{ uriPath }.toEncoded()).toStdString());

    if (not m_media_loaded_)
    {
        m_position_timer_.start();
        m_media_loaded_ = true;
    }
}

void MediaPlayerWrapper::play()
{
    m_player_.Play();
}

void MediaPlayerWrapper::pause()
{
    m_player_.Pause();
}

void MediaPlayerWrapper::seek(const qint64& pos)
{
    m_player_.Seek(static_cast<std::size_t>(pos));
}

void MediaPlayerWrapper::fullRewind()
{
    seek(0U);
}

void MediaPlayerWrapper::setPitch(const qreal& rate)
{
    m_player_.SetPitchValue(static_cast<double>(rate));
}


void MediaPlayerWrapper::PollPosition_()
{
    const auto current = position();

    if (current not_eq m_last_position_)
    {
        m_last_position_ = current;
        emit positionChanged();
    }
}


void MediaPlayerWrapper::onPlayerStateChanged_(const bool playing)
{
    m_is_playing_ = playing;
    emit playingChanged();
}

void MediaPlayerWrapper::onDurationChanged_()
{
    emit durationChanged();
}

MP_END_NAMESPACE
