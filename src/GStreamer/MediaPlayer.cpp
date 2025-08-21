#include <GStreamer/MediaPlayer.hpp>

#include <_macros/Logging.hpp>

#include <QDebug>
#include <QUrl>
#include <QString>

GST_BEGIN_NAMESPACE

MediaPlayer::MediaPlayer()
{
    MP_PRINT("Starting MediaPlayer...\n");
}

MediaPlayer::~MediaPlayer()
{
    MP_PRINT("Shutting down MediaPlayer...\n");
}


auto MediaPlayer::QueryPosition() const noexcept -> std::size_t
{
    return m_pipeline_.QueryPosition();
}
auto MediaPlayer::QueryDuration() const noexcept -> std::size_t
{
    return m_pipeline_.QueryDuration();
}

auto MediaPlayer::IsPlaying() const noexcept -> bool
{
    return m_pipeline_.IsPlaying();
}

void MediaPlayer::LoadAudio(const std::string& uriPath) noexcept
{
    m_pipeline_.LoadAudio(uriPath);
}

void MediaPlayer::Play()  noexcept
{
    m_pipeline_.Play();
}

void MediaPlayer::Pause() noexcept
{
    m_pipeline_.Pause();
}

void MediaPlayer::Seek(const std::size_t& pos) noexcept
{
    m_pipeline_.Seek(pos);
}

void MediaPlayer::SetStateChangedCallback(std::function<void(bool)> callback)
{
    m_pipeline_.SetStateChangedCallback(callback);
}

void MediaPlayer::SetMediaChangedCallback(std::function<void()>     callback)
{
    m_pipeline_.SetMediaChangedCallback(callback);
}

GST_END_NAMESPACE
