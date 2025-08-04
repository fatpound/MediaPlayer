#ifndef MEDIAPLAYER_HPP
#define MEDIAPLAYER_HPP

#include <_macros/Namespaces.hpp>

#include <GStreamer/Pipeline.hpp>

#include <QObject>
#include <QObject>
#include <QTimer>

#include <string>
#include <functional>

#define GST_MP_FOR_QML

GST_BEGIN_NAMESPACE

class MediaPlayer
{
public:
    explicit MediaPlayer();
    explicit MediaPlayer(const MediaPlayer&)     = delete;
    explicit MediaPlayer(MediaPlayer&&) noexcept = delete;

    auto operator = (const MediaPlayer&)     -> MediaPlayer& = delete;
    auto operator = (MediaPlayer&&) noexcept -> MediaPlayer& = delete;
    ~MediaPlayer();


public:
    auto QueryPosition () const noexcept -> std::size_t;
    auto QueryDuration () const noexcept -> std::size_t;
    auto IsPlaying     () const noexcept -> bool;

    void LoadAudio (const std::string& uriPath) noexcept;
    void Play      ()  noexcept;
    void Pause     () noexcept;
    void Seek      (const std::size_t& pos) noexcept;

    void SetStateChangedCallback (std::function<void(bool)> callback);
    void SetMediaChangedCallback (std::function<void()>     callback);


protected:


private:
    Pipeline  m_pipeline_{};
};

GST_END_NAMESPACE

#endif // MEDIAPLAYER_HPP
