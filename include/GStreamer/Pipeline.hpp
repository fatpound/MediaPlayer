#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <_macros/Namespaces.hpp>
#include <_misc/FatCodex/Macros/ExternalWarnings_MSVC.hpp>

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : MSVC_EXWARN_GST)
#endif

#include <gst/gst.h>

#ifdef _MSC_VER
#pragma warning (pop)
#endif

#include <GStreamer/IEffectBin.hpp>
#include <GStreamer/PitchEffectBin.hpp>

#include <cstdint>

#include <string>
#include <functional>
#include <thread>
#include <semaphore>
#include <latch>

GST_BEGIN_NAMESPACE

class Pipeline
{
    struct alignas(64) Data
    {
        GstElement* uridecodebin{};
        GstElement* audioconvert{};
        GstElement* audioresample{};
        GstElement* audiosink{};

        GstElement* tee{};
        GstElement* input_selector{};
    };
    struct alignas(64) Task
    {
        enum class Type : std::uint8_t
        {
            None         ,
            BuildPipeline,
            LoadAudio    ,
            Play         ,
            Pause        ,
            Seek         ,
            Quit
        };

        Type           type{ Type::None };
        std::string    name{};
        std::size_t    seek_val{};
        Pipeline*      pipeline{};
    };


public:
    explicit Pipeline(std::vector<std::unique_ptr<IEffectBin>> effects);

    explicit Pipeline();
    explicit Pipeline(const Pipeline&)     = delete;
    explicit Pipeline(Pipeline&&) noexcept = delete;

    auto operator = (const Pipeline&)     -> Pipeline& = delete;
    auto operator = (Pipeline&&) noexcept -> Pipeline& = delete;
    ~Pipeline() noexcept;


public:
    auto QueryPosition () const noexcept -> std::size_t;
    auto QueryDuration () const noexcept -> std::size_t;
    auto IsPlaying     () const noexcept -> bool;

    void LoadAudio (const std::string& uriPath) noexcept;
    void AddEffect (std::unique_ptr<IEffectBin> pEffect);
    void Play      () noexcept;
    void Pause     () noexcept;
    void Seek      (const std::size_t& pos) noexcept;

    void SetStateChangedCallback (std::function<void(bool)> callback);
    void SetMediaChangedCallback (std::function<void()>     callback);


protected:


private:
    static auto S_BusCallback_(GstBus* bus, GstMessage* msg, gpointer data) noexcept -> gboolean;
    static auto S_TaskHandler_(gpointer data) noexcept -> gboolean;

    static void S_PadAddedHandlerOf_uridecodebin_(GstElement* src, GstPad* newPad, Data* data) noexcept;
    static void S_ParseAndPrintError_(GstMessage* msg) noexcept;


private:
    auto GetState_          () const noexcept -> GstState;
    void SetUri_            (const std::string& uriPath);
    void SetState_          (GstState new_state) noexcept;

    void Setup_             () noexcept;
    void SetupElements_     () noexcept;
    void SetupBusWatch_     () noexcept;
    void SetupBin_          () noexcept;
    void SetupLinks_        () noexcept;
    void SetupGMainLoop_    ();

    void LoadAudio_         (const std::string& uriPath) noexcept;
    void Play_              () noexcept;
    void Pause_             () noexcept;
    void Seek_              (const std::size_t& pos) noexcept;
    void Quit_              () noexcept;

    void DispatchTask_      (Task&& task) noexcept;
    void OnGstStateChanged_ (bool isPlaying);
    void Cleanup_           () noexcept;
    void CleanupGMainLoop_  () noexcept;
    void WorkerLoop_        () noexcept;


private:
    Data                                       m_data_{};

    GstElement*                                m_pPipeline_{};
    GMainContext*                              m_pContext_{};
    GMainLoop*                                 m_pLoop_{};
    GstState                                   m_state_{ GST_STATE_NULL };

    std::vector<std::unique_ptr<IEffectBin>>   m_effects_;
    std::string                                m_loaded_uri_;
    bool                                       m_new_media_loaded_{};

    std::function<void(bool)>                  m_state_change_callback_;
    std::function<void()>                      m_media_change_callback_;

    std::latch                                 m_setup_latch_{ 1 };
    std::binary_semaphore                      m_work_start_signal_{ 0 };
    std::thread                                m_worker_;
};

GST_END_NAMESPACE

#endif // PIPELINE_HPP
