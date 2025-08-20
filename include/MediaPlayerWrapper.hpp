#ifndef MEDIAPLAYERWRAPPER_HPP
#define MEDIAPLAYERWRAPPER_HPP

#include <_macros/Namespaces.hpp>

#include <GStreamer/MediaPlayer.hpp>

#include <QObject>
#include <QTimer>

MP_BEGIN_NAMESPACE

class MediaPlayerWrapper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool   playing  READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(qint64 position READ position  NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration  NOTIFY durationChanged)

public:
    explicit MediaPlayerWrapper(QObject* parent = nullptr);


public:
    Q_INVOKABLE bool isPlaying() const;
    Q_INVOKABLE qint64 position() const;
    Q_INVOKABLE qint64 duration() const;

    Q_INVOKABLE void loadAudio(const QString &uriPath);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void seek(const qint64& pos);
    Q_INVOKABLE void fullRewind();


signals:
    void playingChanged();
    void positionChanged();
    void durationChanged();


protected:


private:
    void PollPosition_();


private:
    gstreamer::MediaPlayer   m_player_;
    QTimer                   m_position_timer_;
    qint64                   m_last_position_{ -1 };

    bool                     m_is_playing_{};
    bool                     m_media_loaded_{};


private slots:
    void onPlayerStateChanged_(bool playing);
    void onDurationChanged_();
};

MP_END_NAMESPACE

#endif // MEDIAPLAYERWRAPPER_HPP
