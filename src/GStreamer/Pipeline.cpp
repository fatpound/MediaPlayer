#include <GStreamer/Pipeline.hpp>
#include <GStreamer/CommonUtilities.hpp>

#include <QDebug>

#include <stdexcept>
#include <new>

//                                                              +----------------+
//                                                         +--->|    identity    |-------------------------------------------------+
//                                                         |    +----------------+                                                 |
//                                                         |                                                                       |
// +--------------+    +-----------------------+           |                                                                       v
// | uridecodebin |--->| audioconvert/resample |-->|  tee  |                                                                       +----------------+    +---------------+
// +--------------+    +-----------------------+           |                                                                       | input-selector |--->| autoaudiosink |
//                                                         |                                                                       +----------------+    +---------------+
//                                                         |                                                                       ^
//                                                         |                                                                       |
//                                                         |    +-------------+    +-------+    +-----------------+    +-----------+
//                                                         +--->| valve_pitch |--->| pitch |--->| valve_audioecho |--->| audioecho |
//                                                              +-------------+    +-------+    +-----------------+    +-----------+
//
//

GST_BEGIN_NAMESPACE

Pipeline::Pipeline()
    :
    m_worker_(&Pipeline::WorkerLoop_, this)
{
    g_print("Initializing MediaPlayer Pipeline...\n");

    SetupGMainLoop_();
    DispatchTask_({ .type = Task::Type::BuildPipeline, .pipeline = this });

    g_print("Worker thread is starting ... ");
    m_start_signal_.release();
}

Pipeline::~Pipeline() noexcept
{
    try
    {
        g_print("Stopping MediaPlayer Pipeline...\n");
        DispatchTask_({ .type = Task::Type::Quit, .name = {}, .pipeline = this });

        m_worker_.join();
        g_print("[DONE]\n");

        CleanupGMainLoop_();
    }
    catch (const std::exception& ex)
    {
        g_printerr("Exception caught in ~MediaPlayer: %s\n", ex.what());
    }
    catch (...)
    {
        g_printerr("Non-STD Exception caught in ~MediaPlayer!\n");
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
        g_print("The audio had already been loaded.\n");
    }
}

void Pipeline::Play() noexcept
{
    DispatchTask_({ .type = Task::Type::Play, .name = {}, .pipeline = this });
}

void Pipeline::Pause() noexcept
{
    DispatchTask_({ .type = Task::Type::Pause, .name = {}, .pipeline = this });
}

void Pipeline::Seek(const std::size_t& pos, const bool& eosRewind) noexcept
{
    DispatchTask_({ .type = Task::Type::Seek, .name = {}, .seek_val = pos, .eos_rewind = eosRewind, .pipeline = this });
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
    // TODO: ?
    // case GST_MESSAGE_STATE_CHANGED:
    //     break;

    case GST_MESSAGE_ASYNC_DONE:
        if (pipeline.m_new_media_loaded_ and pipeline.m_media_change_callback_)
        {
            pipeline.m_new_media_loaded_ = false;
            pipeline.m_media_change_callback_();
        }
        break;

    case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        pipeline.Seek(0U, true);
        break;

    case GST_MESSAGE_ERROR:
        S_ParseAndPrintError_(msg);
        pipeline.Pause();
        break;

    case GST_MESSAGE_WARNING:
        g_print("GST_MESSAGE_WARNING\n");
        break;

    case GST_MESSAGE_INFO:
        g_print("GST_MESSAGE_INFO\n");
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
        g_print("Task ignored: Pipeline is not ready for effect changes.\n");
        return G_SOURCE_REMOVE;
    }

    switch (task.type)
    {
    case Task::Type::BuildPipeline: pipeline.Setup_();                      break;
    case Task::Type::LoadAudio:     pipeline.LoadAudio_(task.name);                 break;
    case Task::Type::Play:          pipeline.Play_();                               break;
    case Task::Type::Pause:         pipeline.Pause_();                              break;
    case Task::Type::Seek:          pipeline.Seek_(task.seek_val, task.eos_rewind); break;
    case Task::Type::Quit:          pipeline.Quit_();                               break;

    default:
        g_warning("Unhandled task type: %d\n", static_cast<int>(task.type));
        break;
    }

    return G_SOURCE_REMOVE;
}

void Pipeline::S_PadAddedHandlerOf_uridecodebin_(GstElement* src, GstPad* newPad, Data* data) noexcept
{
    g_print("S_PadAddedHandlerOf_uridecodebin_ has been called.\n");
    g_print("Trying to link '%s' '%s' pad to '%s' sink pad ... ", GST_ELEMENT_NAME(src), GST_PAD_NAME(newPad), GST_ELEMENT_NAME(data->audioconvert));

    GstPad*  const audioconvert_1_sink_pad = gst_element_get_static_pad(data->audioconvert, "sink");
    GstCaps* const new_pad_caps            = gst_pad_get_current_caps(newPad);

    if (gst_pad_is_linked(audioconvert_1_sink_pad))
    {
        g_printerr("%s sink pad has already been linked. Ignoring.\n", GST_ELEMENT_NAME(data->audioconvert));
        goto exit;
    }

    if (const auto& new_pad_caps_type = gst_structure_get_name(gst_caps_get_structure(new_pad_caps, 0));
        g_str_has_prefix(new_pad_caps_type, "audio/x-raw") == FALSE)
    {
        g_printerr("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_caps_type);
        goto exit;
    }

    g_print("%s\n", GST_PAD_LINK_FAILED(gst_pad_link(newPad, audioconvert_1_sink_pad)) ? "[FAIL]" : "[DONE]");


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
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);
}


auto Pipeline::GetTeePadTemplate_() const noexcept -> GstPadTemplate*
{
    return gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(m_data_.tee), "src_%u");
}

auto Pipeline::GetState_() const noexcept -> GstState
{
    return m_state_;
}

void Pipeline::SetUri_(const std::string& uriPath) noexcept
{
    g_print("Setting URI to: '%s' ... ", uriPath.c_str());
    g_object_set(m_data_.uridecodebin, "uri", uriPath.c_str(), nullptr);
    g_print("[DONE]\n");

    m_loaded_uri_ = uriPath;
}

void Pipeline::SetState_(const GstState new_state) noexcept
{
    g_print("Setting pipeline state to: %s ... ", gst_element_state_get_name(new_state));

    if (gst_element_set_state(m_pPipeline_, new_state) == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("[FAILED]\n");
        return;
    }

    m_state_ = new_state;
    g_print("[DONE]\n");

    if (GetState_() == GST_STATE_PLAYING or GetState_() == GST_STATE_PAUSED)
    {
        OnGstStateChanged_(IsPlaying());
    }
}

void Pipeline::Setup_() noexcept
{
    if (m_pPipeline_ = gst_pipeline_new("media-player-audio-pipeline"); m_pPipeline_ == nullptr)
    {
        g_printerr("'media-player-audio-pipeline' could be created!\n");
        return;
    }

    SetupElements_();
    SetupBusWatch_(); // must be called before setting a pipeline state, see: func. definition
    SetupBin_();
    SetupLinks_();

    g_signal_connect(m_data_.uridecodebin, "pad-added", G_CALLBACK(&Pipeline::S_PadAddedHandlerOf_uridecodebin_), &m_data_);

    SetState_(GST_STATE_NULL);
    SetState_(GST_STATE_READY);

    g_object_set(m_data_.input_selector, "active-pad", m_data_.wet_path_pad, nullptr);
    g_object_set(m_data_.pitch, "pitch", 1.2, nullptr);

    g_print("Pipeline built successfully.\n");
}

void Pipeline::SetupElements_() noexcept
{
#define MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(MEMBERNAME, FACTORYNAME, NAME)                                          \
    m_data_.MEMBERNAME = gst_element_factory_make(#FACTORYNAME, #FACTORYNAME #NAME);                                \
    if (m_data_.MEMBERNAME == nullptr)                                                                              \
    {                                                                                                               \
        g_printerr("Failed to create GStreamer element: '%s' of type '%s'\n", #FACTORYNAME #NAME, #FACTORYNAME);    \
    }


    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(uridecodebin,   uridecodebin,         1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(audioconvert,   audioconvert,         1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(audioresample,  audioresample,        1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(autoaudiosink,  autoaudiosink,        1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(tee,            tee,                  1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(input_selector, input-selector,       1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(identity,       identity,             1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(pitch,          pitch,                1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(post_convert,   audioconvert,         2);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(post_resample,  audioresample,        2);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(valve_pitch,    valve,          pitch-1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(queue_dry,      queue,            dry-1);
    MP_MAKE_GST_ELEMENT_FOR_CUSTOM_DATA(queue_wet,      queue,            wet-1);


#undef MP_MAKE_NAMED_ELEMENT
#undef MP_MAKE_ELEMENT
}

void Pipeline::SetupBusWatch_() noexcept
{
    // https://gstreamer.freedesktop.org/documentation/gstreamer/gstpipeline.html?gi-language=c#gstpipeline-page
    //
    // Before changing the state of the GstPipeline (see GstElement) a GstBus should be retrieved with gst_pipeline_get_bus.
    // This GstBus should then be used to receive GstMessage from the elements in the pipeline.
    //

    const auto pPipelineBus = UniqueGstPtr<GstBus>{ gst_pipeline_get_bus(reinterpret_cast<GstPipeline*>(m_pPipeline_)) };

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
        m_data_.queue_dry,
        m_data_.identity,
        m_data_.queue_wet,
        m_data_.valve_pitch,
        m_data_.pitch,
        m_data_.post_convert,
        m_data_.post_resample,
        m_data_.input_selector,
        m_data_.autoaudiosink,
        nullptr
    );
}

void Pipeline::SetupLinks_() noexcept
{
#define MP_GST_ELEMENT_LINK_MANY_WITH_LOGS(...)                             \
    g_print("Linking pipeline elements: " #__VA_ARGS__ " ... ");            \
    if (gst_element_link_many(__VA_ARGS__ __VA_OPT__(,) nullptr) == FALSE)  \
    {                                                                       \
        g_printerr("[FAILED]\n");                                           \
    }                                                                       \
    g_print("[DONE]\n");


    MP_GST_ELEMENT_LINK_MANY_WITH_LOGS(m_data_.audioconvert, m_data_.audioresample, m_data_.tee);
    MP_GST_ELEMENT_LINK_MANY_WITH_LOGS(m_data_.queue_dry, m_data_.identity);
    MP_GST_ELEMENT_LINK_MANY_WITH_LOGS(m_data_.queue_wet, m_data_.valve_pitch, m_data_.pitch, m_data_.post_convert, m_data_.post_resample);
    MP_GST_ELEMENT_LINK_MANY_WITH_LOGS(m_data_.input_selector, m_data_.autoaudiosink);


#undef MP_GST_ELEMENT_LINK_MANY_WITH_LOGS

    SetupDryPath_();
    SetupWetPath_();
}

void Pipeline::SetupDryPath_() noexcept
{
    {
        const auto teeCleanSrcPad = UniqueGstPtr<GstPad>{ gst_element_request_pad(m_data_.tee, GetTeePadTemplate_(), nullptr, nullptr) };
        const auto drySinkPad     = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(m_data_.queue_dry, "sink") };

        if (GST_PAD_LINK_FAILED(gst_pad_link(teeCleanSrcPad.get(), drySinkPad.get())))
        {
            g_printerr("Failed to link tee to identity.\n");
        }
    }

    m_data_.dry_path_pad = gst_element_request_pad_simple(m_data_.input_selector, "sink_%u");
    g_print("Obtained 'clean_path_pad': %s\n", gst_pad_get_name(m_data_.dry_path_pad));

    if (const auto drySrcPad = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(m_data_.identity, "src") };
        GST_PAD_LINK_FAILED(gst_pad_link(drySrcPad.get(), m_data_.dry_path_pad)))
    {
        g_printerr("Failed to link identity to input-selector.\n");
    }
}

void Pipeline::SetupWetPath_() noexcept
{
    {
        const auto teeFxSrcPad    = UniqueGstPtr<GstPad>{ gst_element_request_pad(m_data_.tee, GetTeePadTemplate_(), nullptr, nullptr) };
        const auto firstFxSinkPad = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(m_data_.queue_wet, "sink") };

        if (GST_PAD_LINK_FAILED(gst_pad_link(teeFxSrcPad.get(), firstFxSinkPad.get())))
        {
            g_printerr("Failed to link tee to effects chain.\n");
        }
    }

    m_data_.wet_path_pad = gst_element_request_pad_simple(m_data_.input_selector, "sink_%u");
    g_print("Obtained 'effects_path_pad': %s\n", gst_pad_get_name(m_data_.wet_path_pad));

    if (const auto lastFxSrcPad = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(m_data_.post_resample, "src") };
        GST_PAD_LINK_FAILED(gst_pad_link(lastFxSrcPad.get(), m_data_.wet_path_pad)))
    {
        g_printerr("Failed to link effects chain to input-selector.\n");
    }
}

void Pipeline::SetupGMainLoop_()
{
    g_print("Setting up GMainLoop ... ");

    if (m_pContext_ = g_main_context_new(); m_pContext_ == nullptr)
    {
        throw std::runtime_error{"Could not crete main context for GMainLoop!"};
    }

    if (m_pLoop_= g_main_loop_new(m_pContext_, FALSE); m_pLoop_ == nullptr)
    {
        throw std::runtime_error{"Could not crete loop for GMainLoop!"};
    }

    g_print("[DONE]\n");
}

void Pipeline::LoadAudio_(const std::string& uriPath) noexcept
{
    g_print("Executing task: 'LoadAudio'\n");

    if (m_pPipeline_ == nullptr)
    {
        g_printerr("Pipeline does not exist!\n");
        return;
    }

    SetUri_(uriPath);
    SetState_(GST_STATE_READY);
    Pause_();

    m_media_change_callback_();
}

void Pipeline::Play_() noexcept
{
    g_print("Executing task: 'Play'\n");

    if (m_pPipeline_ == nullptr)
    {
        g_printerr("Pipeline does not exist!\n");
        return;
    }

    if (GetState_() == GST_STATE_PLAYING)
    {
        g_print("Pipeline is already in the playing state.\n");
        return;
    }

    if (m_loaded_uri_ not_eq "")
    {
        SetState_(GST_STATE_PLAYING);
    }
    else
    {
        g_warning("No audio is loaded.\n");
    }
}

void Pipeline::Pause_() noexcept
{
    g_print("Executing task: 'Pause'\n");

    if (m_pPipeline_ == nullptr)
    {
        g_printerr("Pipeline does not exist!\n");
        return;
    }

    if (GetState_() == GST_STATE_PAUSED)
    {
        g_print("Pipeline is already in the paused state.\n");
        return;
    }

    SetState_(GST_STATE_PAUSED);
}

void Pipeline::Seek_(const std::size_t& pos, const bool& eosRewind) noexcept
{
    g_print("Executing task: 'Seek'\n");

    if (m_pPipeline_ == nullptr)
    {
        g_printerr("Pipeline does not exist!\n");
        return;
    }

    if (m_loaded_uri_ == "")
    {
        g_warning("No audio is loaded.\n");
        return;
    }

    const auto& priorState = GetState_();

    Pause_();

    g_print("Seeking to %lld ... ", pos);
    if (gst_element_seek_simple(
            m_pPipeline_,
            GST_FORMAT_TIME,
            static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH bitor GST_SEEK_FLAG_KEY_UNIT),
            pos * GST_MSECOND)
            == FALSE)
    {
        g_printerr("[FAILED]\n");
    }
    g_print("[DONE]\n");

    if (priorState == GST_STATE_PLAYING and not eosRewind)
    {
        Play_();
    }
}

void Pipeline::Quit_() noexcept
{
    g_print("Executing task: 'Quit'\n");

    if (m_pLoop_ == nullptr)
    {
        g_printerr("GMainLoop does not exist!\n");
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
        g_printerr("Pipeline does NOT exist!\n");
    }

    g_print("Sending GMainLoop quit signal to worker thread ... ");
    g_main_loop_quit(m_pLoop_);
    g_print("[DONE]\n");
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
    g_print("CleanupPipeline_ has been called.\n");

    if (m_pPipeline_ == nullptr)
    {
        g_printerr("Pipeline does not exist!\n");
        return;
    }

    g_print("Cleaning up pipeline...\n");
    SetState_(GST_STATE_NULL);

    g_print("Removing pipeline bus watch ... ");
    if (const auto pPipelineBus = UniqueGstPtr<GstBus>{ gst_pipeline_get_bus(reinterpret_cast<GstPipeline*>(m_pPipeline_)) };
        gst_bus_remove_watch(pPipelineBus.get()) == FALSE)
    {
        g_printerr("[FAILED]\n");
        return;
    }
    g_print("[DONE]\n");

    SetState_(GST_STATE_VOID_PENDING);
    gst_object_unref(m_pPipeline_);
    m_pPipeline_ = nullptr;

    m_data_ = {};
    g_print("Pipeline has been cleared.\n");
}

void Pipeline::CleanupGMainLoop_() noexcept
{
    g_print("Cleaning-up GMainLoop...\n");

    g_main_loop_unref(m_pLoop_);
    m_pLoop_ = nullptr;

    g_main_context_unref(m_pContext_);
    m_pContext_ = nullptr;

    g_print("GMainLoop has been freed...\n");
}

void Pipeline::WorkerLoop_() noexcept
{
    m_start_signal_.acquire();
    g_print("[DONE]\n");

    g_print("Worker thread is setting its default context ... ");
    g_main_context_push_thread_default(m_pContext_);
    g_print("[DONE]\n");

    g_print("GMainLoop is starting to run...\n");
    g_main_loop_run(m_pLoop_);
    g_print("GMainLoop has Stopped.\n");

    g_print("Worker thread is UN-setting its default context ... ");
    g_main_context_pop_thread_default(m_pContext_);
    g_print("[DONE]\n");

    g_print("Worker thread is Stopping ... ");
}

GST_END_NAMESPACE
