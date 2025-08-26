#include <GStreamer/Pipeline.hpp>
#include <GStreamer/CommonUtilities.hpp>

#include <_macros/Logging.hpp>

#include <QDebug>

#include <stdexcept>
#include <new>

GST_BEGIN_NAMESPACE

Pipeline::Pipeline(std::vector<std::unique_ptr<IEffectBin>> effects)
    :
    Pipeline()
{
    for (auto& effect : effects)
    {
        AddEffect(std::move<>(effect));
    }
}

Pipeline::Pipeline()
    :
    m_worker_(&Pipeline::WorkerLoop_, this)
{
    MP_PRINT("Initializing MediaPlayer Pipeline...\n");

    SetupGMainLoop_();
    DispatchTask_({ .type = Task::Type::BuildPipeline, .pipeline = this });

    MP_PRINT("Worker thread will start soon...\n");
    m_work_start_signal_.release();
}

Pipeline::~Pipeline() noexcept
{
    try
    {
        MP_PRINT("Stopping MediaPlayer Pipeline...\n");
        DispatchTask_({ .type = Task::Type::Quit, .pipeline = this });

        m_worker_.join();
        MP_PRINT("[DONE]\n");

        CleanupGMainLoop_();
    }
    catch ([[maybe_unused]] const std::exception& ex)
    {
        MP_PRINTERR("Exception caught in ~MediaPlayer: %s\n", ex.what());
    }
    catch (...)
    {
        MP_PRINTERR("Non-STD Exception caught in ~MediaPlayer!\n");
    }
}


auto Pipeline::QueryPosition() const noexcept -> std::size_t
{
    if (m_pPipeline_ == nullptr)
    {
        return 0;
    }

    gint64 pos{};

    if (gst_element_query_position(m_pPipeline_, GST_FORMAT_TIME, &pos))
    {
        return static_cast<std::size_t>(pos);
    }

    return 0;
}

auto Pipeline::QueryDuration() const noexcept -> std::size_t
{
    if (m_pPipeline_ == nullptr)
    {
        return 0;
    }

    gint64 dur{};

    if (gst_element_query_duration(m_pPipeline_, GST_FORMAT_TIME, &dur))
    {
        return static_cast<std::size_t>(dur);
    }

    return 0;
}

auto Pipeline::IsPlaying() const noexcept -> bool
{
    return GetState_() == GST_STATE_PLAYING;
}

void Pipeline::LoadAudio(const std::string& uriPath) noexcept
{
    if (m_loaded_uri_ not_eq uriPath)
    {
        m_new_media_loaded_ = true;

        DispatchTask_({ .type = Task::Type::LoadAudio, .name = uriPath, .pipeline = this });
    }
    else
    {
        MP_PRINT("The audio had already been loaded.\n");
    }
}

void Pipeline::AddEffect(std::unique_ptr<IEffectBin> pEffect)
{
    m_setup_latch_.wait();

    if (gst_bin_add(GST_BIN(m_pPipeline_), pEffect->GetBin()) == FALSE)
    {
        MP_PRINTERR("Could NOT add EffectBin to pipeline!\n");
        return;
    }

    {
        auto* const teePad  = gst_element_request_pad_simple(m_data_.tee, "src_%u");
        auto* const sinkPad = pEffect->GetSinkPad();

        MP_PRINT("Linking: %s --> %s ... ", GST_ELEMENT_NAME(teePad), GST_ELEMENT_NAME(sinkPad));
        if (GST_PAD_LINK_FAILED(gst_pad_link(teePad, sinkPad)))
        {
            MP_PRINTERR("[FAIL]\n");
            return;
        }
        MP_PRINT("[DONE]\n");
    }

    {
        auto* const srcPad = pEffect->GetSrcPad();
        auto* const selPad = gst_element_request_pad_simple(m_data_.input_selector, "sink_%u");

        MP_PRINT("Linking: %s --> %s ... ", GST_ELEMENT_NAME(srcPad), GST_ELEMENT_NAME(selPad));
        if (GST_PAD_LINK_FAILED(gst_pad_link(srcPad, selPad)))
        {
            MP_PRINTERR("[FAIL]\n");
            return;
        }
        MP_PRINT("[DONE]\n");

        if (m_effects_.empty())
        {
            g_object_set(m_data_.input_selector, "active-pad", selPad, nullptr);
        }
    }

    m_effects_.push_back(std::move<>(pEffect));
}

void Pipeline::Play() noexcept
{
    DispatchTask_({ .type = Task::Type::Play, .pipeline = this });
}

void Pipeline::Pause() noexcept
{
    DispatchTask_({ .type = Task::Type::Pause, .pipeline = this });
}

void Pipeline::Seek(const std::size_t& pos) noexcept
{
    DispatchTask_({ .type = Task::Type::Seek, .seek_val = pos, .pipeline = this });
}

void Pipeline::SetStateChangedCallback(std::function<void(bool)> callback)
{
    m_state_change_callback_ = callback;
}

void Pipeline::SetMediaChangedCallback(std::function<void()> callback)
{
    m_media_change_callback_ = callback;
}


auto Pipeline::S_BusCallback_(GstBus*, GstMessage* const msg, const gpointer data) noexcept -> gboolean
{
    auto& pipeline = *static_cast<Pipeline*>(data);

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ASYNC_DONE:
        if (pipeline.m_new_media_loaded_ and pipeline.m_media_change_callback_ not_eq nullptr)
        {
            pipeline.m_new_media_loaded_ = false;
            pipeline.m_media_change_callback_();
        }
        break;

    case GST_MESSAGE_EOS:
        MP_PRINT("End-Of-Stream reached.\n");
        pipeline.Seek(0U);
        pipeline.Pause();
        break;

    case GST_MESSAGE_ERROR:
        S_ParseAndPrintError_(msg);
        pipeline.Pause();
        break;

    case GST_MESSAGE_WARNING:
        MP_PRINT("GST_MESSAGE_WARNING\n");
        break;

    case GST_MESSAGE_INFO:
        MP_PRINT("GST_MESSAGE_INFO\n");
        break;

    default:
        break;
    }

    return TRUE;
}

auto Pipeline::S_TaskHandler_(const gpointer data) noexcept -> gboolean
{
    Task& task     = *static_cast<Task*>(data);
    auto& pipeline = *task.pipeline;

    if (pipeline.m_pPipeline_ == nullptr and task.type > Task::Type::Pause)
    {
        MP_PRINT("Task ignored: Pipeline is not ready for effect changes.\n");
        return G_SOURCE_REMOVE;
    }

    switch (task.type)
    {
    case Task::Type::BuildPipeline: pipeline.Setup_();              break;
    case Task::Type::LoadAudio:     pipeline.LoadAudio_(task.name); break;
    case Task::Type::Play:          pipeline.Play_();               break;
    case Task::Type::Pause:         pipeline.Pause_();              break;
    case Task::Type::Seek:          pipeline.Seek_(task.seek_val);  break;
    case Task::Type::Quit:          pipeline.Quit_();               break;

    default:
        MP_LOGWARN("Unhandled task type: %d\n", static_cast<int>(task.type));
        break;
    }

    return G_SOURCE_REMOVE;
}

void Pipeline::S_PadAddedHandlerOf_uridecodebin_([[maybe_unused]] GstElement* src, GstPad* newPad, Data* data) noexcept
{
    MP_PRINT("S_PadAddedHandlerOf_uridecodebin_ has been called.\n");
    MP_PRINT("Linking: '%s' '%s' pad to '%s' sink pad... ", GST_ELEMENT_NAME(src), GST_PAD_NAME(newPad), GST_ELEMENT_NAME(data->audioconvert));

    GstPad*  const audioconvert_1_sink_pad = gst_element_get_static_pad(data->audioconvert, "sink");
    GstCaps* const new_pad_caps            = gst_pad_get_current_caps(newPad);

    if (gst_pad_is_linked(audioconvert_1_sink_pad))
    {
        MP_PRINTERR("%s sink pad has already been linked. Ignoring.\n", GST_ELEMENT_NAME(data->audioconvert));
        goto exit;
    }

    if (const auto& new_pad_caps_type = gst_structure_get_name(gst_caps_get_structure(new_pad_caps, 0));
        g_str_has_prefix(new_pad_caps_type, "audio/x-raw") == FALSE)
    {
        MP_PRINTERR("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_caps_type);
        goto exit;
    }

    {
        [[maybe_unused]] const auto result = GST_PAD_LINK_FAILED(gst_pad_link(newPad, audioconvert_1_sink_pad));
        MP_PRINT("%s\n", result ? "[FAIL]" : "[DONE]");
    }


exit:
    if (new_pad_caps not_eq nullptr)
    {
        gst_caps_unref(new_pad_caps);
    }

    gst_object_unref(audioconvert_1_sink_pad);
}

void Pipeline::S_ParseAndPrintError_(GstMessage* msg) noexcept
{
    GError* err{};
    gchar* debug_info{};

    gst_message_parse_error(msg, &err, &debug_info);
    MP_PRINTERR("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    MP_PRINTERR("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);
}


auto Pipeline::GetState_() const noexcept -> GstState
{
    return m_state_;
}

void Pipeline::SetUri_(const std::string& uriPath)
{
    MP_PRINT("Setting URI to: '%s' ... ", uriPath.c_str());
    g_object_set(m_data_.uridecodebin, "uri", uriPath.c_str(), nullptr);
    MP_PRINT("[DONE]\n");

    m_loaded_uri_ = uriPath;
}

void Pipeline::SetState_(const GstState new_state) noexcept
{
    MP_PRINT("Setting pipeline state to: %s ... ", gst_element_state_get_name(new_state));

    if (gst_element_set_state(m_pPipeline_, new_state) == GST_STATE_CHANGE_FAILURE)
    {
        MP_PRINTERR("[FAILED]\n");
        return;
    }

    m_state_ = new_state;
    MP_PRINT("[DONE]\n");

    if (GetState_() == GST_STATE_PLAYING or GetState_() == GST_STATE_PAUSED)
    {
        OnGstStateChanged_(IsPlaying());
    }
}

void Pipeline::Setup_() noexcept
{
    if (m_pPipeline_ = gst_pipeline_new("media-player-audio-pipeline"); m_pPipeline_ == nullptr)
    {
        MP_PRINTERR("'media-player-audio-pipeline' could be created!\n");
        return;
    }

    SetupElements_();
    SetupBusWatch_();
    SetupBin_();
    SetupLinks_();

    g_signal_connect(m_data_.uridecodebin, "pad-added", G_CALLBACK(&Pipeline::S_PadAddedHandlerOf_uridecodebin_), &m_data_);

    SetState_(GST_STATE_NULL);
    SetState_(GST_STATE_READY);

    MP_PRINT("Pipeline core has been built successfully.\n");

    m_setup_latch_.count_down();
}

void Pipeline::SetupElements_() noexcept
{
#define MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(MEMBERNAME, FACTORYNAME, NAME)                                          \
    m_data_.MEMBERNAME = gst_element_factory_make(#FACTORYNAME, #FACTORYNAME #NAME);                                \
    if (m_data_.MEMBERNAME == nullptr)                                                                              \
    {                                                                                                               \
        MP_PRINTERR("Failed to create GStreamer element: '%s' of type '%s'\n", #FACTORYNAME #NAME, #FACTORYNAME);   \
    }


#ifdef FATLIB_BUILDING_ON_WINDOWS
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(audiosink,      wasapisink,           1);
#else
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(audiosink,      autoaudiosink,        1);
#endif

    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(uridecodebin,   uridecodebin,         1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(audioconvert,   audioconvert,         1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(audioresample,  audioresample,        1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(tee,            tee,                  1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(input_selector, input-selector,       1);


#undef MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA

#ifdef FATLIB_BUILDING_ON_WINDOWS
    g_object_set(m_data_.audiosink, "low-latency", true, nullptr);
#endif
}

void Pipeline::SetupBusWatch_() noexcept
{
    // https://gstreamer.freedesktop.org/documentation/gstreamer/gstpipeline.html?gi-language=c#gstpipeline-page
    //
    // Before changing the state of the GstPipeline (see GstElement) a GstBus should be retrieved with gst_pipeline_get_bus.
    // This GstBus should then be used to receive GstMessage from the elements in the pipeline.
    //

    const auto pPipelineBus = UniqueGstPtr<GstBus>{ gst_pipeline_get_bus(reinterpret_cast<GstPipeline*>(m_pPipeline_)) };

    // must be called before setting a pipeline state, see: func. definition
    //
    gst_bus_add_watch(pPipelineBus.get(), &Pipeline::S_BusCallback_, this);
}

void Pipeline::SetupBin_() noexcept
{
    gst_bin_add_many(
        GST_BIN(m_pPipeline_),
        m_data_.uridecodebin,
        m_data_.audioconvert,
        m_data_.audioresample,
        m_data_.tee,
        m_data_.input_selector,
        m_data_.audiosink,
        nullptr
    );
}

void Pipeline::SetupLinks_() noexcept
{
#define MP_GST_ELEMENT_LINK_MANY_WITH_LOGS(...)                             \
    MP_PRINT("Linking pipeline elements: " #__VA_ARGS__ " ... ");           \
    if (gst_element_link_many(__VA_ARGS__ __VA_OPT__(,) nullptr) == FALSE)  \
    {                                                                       \
        MP_PRINTERR("[FAILED]\n");                                          \
    }                                                                       \
    MP_PRINT("[DONE]\n");


    MP_GST_ELEMENT_LINK_MANY_WITH_LOGS(m_data_.audioconvert, m_data_.audioresample, m_data_.tee);
    MP_GST_ELEMENT_LINK_MANY_WITH_LOGS(m_data_.input_selector, m_data_.audiosink);


#undef MP_GST_ELEMENT_LINK_MANY_WITH_LOGS
}

void Pipeline::SetupGMainLoop_()
{
    MP_PRINT("Setting up GMainLoop ... ");

    if (m_pContext_ = g_main_context_new(); m_pContext_ == nullptr)
    {
        throw std::runtime_error{"Could not crete main context for GMainLoop!"};
    }

    if (m_pLoop_= g_main_loop_new(m_pContext_, FALSE); m_pLoop_ == nullptr)
    {
        throw std::runtime_error{"Could not crete loop for GMainLoop!"};
    }

    MP_PRINT("[DONE]\n");
}

void Pipeline::LoadAudio_(const std::string& uriPath) noexcept
{
    MP_PRINT("Executing task: 'LoadAudio'\n");

    if (m_pPipeline_ == nullptr)
    {
        MP_PRINTERR("Pipeline does not exist!\n");
        return;
    }

    SetState_(GST_STATE_READY);
    SetUri_(uriPath);
    Pause_();
}

void Pipeline::Play_() noexcept
{
    MP_PRINT("Executing task: 'Play'\n");

    if (m_pPipeline_ == nullptr)
    {
        MP_PRINTERR("Pipeline does not exist!\n");
        return;
    }

    if (GetState_() == GST_STATE_PLAYING)
    {
        MP_PRINT("Pipeline is already in the playing state.\n");
        return;
    }

    if (m_loaded_uri_ not_eq "")
    {
        SetState_(GST_STATE_PLAYING);
    }
    else
    {
        MP_LOGWARN("No audio is loaded.\n");
    }
}

void Pipeline::Pause_() noexcept
{
    MP_PRINT("Executing task: 'Pause'\n");

    if (m_pPipeline_ == nullptr)
    {
        MP_PRINTERR("Pipeline does not exist!\n");
        return;
    }

    if (GetState_() == GST_STATE_PAUSED)
    {
        MP_PRINT("Pipeline is already in the paused state.\n");
        return;
    }

    SetState_(GST_STATE_PAUSED);
}

void Pipeline::Seek_(const std::size_t& pos) noexcept
{
    MP_PRINT("Executing task: 'Seek'\n");

    if (m_pPipeline_ == nullptr)
    {
        MP_PRINTERR("Pipeline does not exist!\n");
        return;
    }

    if (m_loaded_uri_ == "")
    {
        MP_LOGWARN("No audio is loaded.\n");
        return;
    }

    MP_PRINT("Seeking to %lld ... ", pos);
    if (gst_element_seek_simple(
            m_pPipeline_,
            GST_FORMAT_TIME,
            static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH bitor GST_SEEK_FLAG_KEY_UNIT),
            pos * GST_MSECOND)
            == FALSE)
    {
        MP_PRINTERR("[FAILED]\n");
    }
    MP_PRINT("[DONE]\n");
}

void Pipeline::Quit_() noexcept
{
    MP_PRINT("Executing task: 'Quit'\n");

    if (m_pLoop_ == nullptr)
    {
        MP_PRINTERR("GMainLoop does not exist!\n");
        return;
    }

    if (m_pPipeline_ not_eq nullptr)
    {
        if (GetState_() == GST_STATE_PLAYING)
        {
            Pause_();
        }

        Cleanup_();
    }
    else
    {
        MP_PRINTERR("Pipeline does NOT exist!\n");
    }

    MP_PRINT("Sending GMainLoop quit signal to worker thread ... ");
    g_main_loop_quit(m_pLoop_);
    MP_PRINT("[DONE]\n");
}

void Pipeline::DispatchTask_(Task&& task) noexcept
{
    auto* const src = g_idle_source_new();

    g_source_set_callback(
        src,
        &Pipeline::S_TaskHandler_,
        new (std::nothrow) Task(std::move<>(task)),
        [](const gpointer data) noexcept
        {
            delete static_cast<Task*>(data);
        }
    );

    g_source_attach(src, m_pContext_);
    g_source_unref(src);
}

void Pipeline::OnGstStateChanged_(const bool isPlaying)
{
    if (m_state_change_callback_)
    {
        m_state_change_callback_(isPlaying);
    }
}

void Pipeline::Cleanup_() noexcept
{
    MP_PRINT("CleanupPipeline_ has been called.\n");

    if (m_pPipeline_ == nullptr)
    {
        MP_PRINTERR("Pipeline does not exist!\n");
        return;
    }

    MP_PRINT("Cleaning up pipeline...\n");
    SetState_(GST_STATE_NULL);

    MP_PRINT("Removing pipeline bus watch ... ");
    if (const auto pPipelineBus = UniqueGstPtr<GstBus>{ gst_pipeline_get_bus(reinterpret_cast<GstPipeline*>(m_pPipeline_)) };
        gst_bus_remove_watch(pPipelineBus.get()) == FALSE)
    {
        MP_PRINTERR("[FAILED]\n");
        return;
    }
    MP_PRINT("[DONE]\n");

    SetState_(GST_STATE_VOID_PENDING);
    gst_object_unref(m_pPipeline_);
    m_pPipeline_ = nullptr;

    m_data_ = {};
    MP_PRINT("Pipeline has been cleared.\n");
}

void Pipeline::CleanupGMainLoop_() noexcept
{
    MP_PRINT("Cleaning-up GMainLoop...\n");

    g_main_loop_unref(m_pLoop_);
    m_pLoop_ = nullptr;

    g_main_context_unref(m_pContext_);
    m_pContext_ = nullptr;

    MP_PRINT("GMainLoop has been freed...\n");
}

void Pipeline::WorkerLoop_() noexcept
{
    m_work_start_signal_.acquire();
    MP_PRINT("Worker thread has just started.\n");

    MP_PRINT("Worker thread is setting its default context ... ");
    g_main_context_push_thread_default(m_pContext_);
    MP_PRINT("[DONE]\n");

    MP_PRINT("GMainLoop is starting to run...\n");
    g_main_loop_run(m_pLoop_);
    MP_PRINT("GMainLoop has Stopped.\n");

    MP_PRINT("Worker thread is UN-setting its default context ... ");
    g_main_context_pop_thread_default(m_pContext_);
    MP_PRINT("[DONE]\n");

    MP_PRINT("Worker thread is Stopping ... ");
}

GST_END_NAMESPACE
