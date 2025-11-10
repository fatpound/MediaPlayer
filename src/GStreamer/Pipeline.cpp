#include <GStreamer/Pipeline.hpp>
#include <GStreamer/CommonUtilities.hpp>

#include <stdexcept>
#include <new>

GST_BEGIN_NAMESPACE

Pipeline::Pipeline(std::shared_ptr<IAudioEffectBin> pEffect)
    :
    Pipeline()
{
    if (pEffect not_eq nullptr)
    {
        DispatchTask_({ .type = Task::Type::AttachEffect, .effect = pEffect, .pipeline = this });
    }
}

Pipeline::Pipeline()
    :
    m_worker_(&Pipeline::WorkerLoop_, this)
{
    g_print("Initializing MediaPlayer Pipeline...\n");

    SetupGMainLoop_();
    DispatchTask_({ .type = Task::Type::BuildPipeline, .pipeline = this });

    g_print("Worker thread will start soon...\n");
    m_work_start_signal_.release();
}

Pipeline::~Pipeline() noexcept
{
    try
    {
        g_print("Stopping MediaPlayer Pipeline...\n");
        DispatchTask_({ .type = Task::Type::Quit, .pipeline = this });

        m_worker_.join();
        g_print("[DONE]\n");

        CleanupGMainLoop_();
    }
    catch ([[maybe_unused]] const std::exception& ex)
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

void Pipeline::RunFunc(std::function<void()> fn) noexcept
{
    DispatchTask_({ .type = Task::Type::RunFunc, .func = fn, .pipeline = this });
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
        g_print("End-Of-Stream reached.\n");
        pipeline.Seek(0U);
        pipeline.Pause();
        break;

    case GST_MESSAGE_ERROR:
        PrintErrorMsg(msg);
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
        g_printerr("Pipeline does NOT exist yet and tasks will not be handled!\n");

        return G_SOURCE_REMOVE;
    }

    switch (task.type)
    {
    case Task::Type::BuildPipeline: pipeline.Setup_();                        break;
    case Task::Type::AttachEffect:  pipeline.AttachAudioEffect_(task.effect); break;
    case Task::Type::DetachEffect:  pipeline.DetachAudioEffect_();            break;
    case Task::Type::LoadAudio:     pipeline.LoadAudio_(task.name);           break;
    case Task::Type::Play:          pipeline.Play_();                         break;
    case Task::Type::Pause:         pipeline.Pause_();                        break;
    case Task::Type::Seek:          pipeline.Seek_(task.seek_val);            break;
    case Task::Type::RunFunc:       pipeline.RunFunc_(task.func);             break;
    case Task::Type::Quit:          pipeline.Quit_();                         break;

    default:
        g_warning("Unhandled task type: %d\n", static_cast<int>(task.type));
        break;
    }

    return G_SOURCE_REMOVE;
}

void Pipeline::S_PadAddedHandlerOf_uridecodebin_([[maybe_unused]] GstElement* src, GstPad* newPad, Data* data) noexcept
{
    g_print("S_PadAddedHandlerOf_uridecodebin_ has been called.\n");
    g_print("Linking: '%s' '%s' pad to '%s' sink pad ... ", GST_ELEMENT_NAME(src), GST_PAD_NAME(newPad), GST_ELEMENT_NAME(data->audioconvert));

    GstPad*  const audioconvert_1_sink_pad = gst_element_get_static_pad(data->audioconvert, "sink");
    GstCaps* const new_pad_caps            = gst_pad_get_current_caps(newPad);

    if (gst_pad_is_linked(audioconvert_1_sink_pad))
    {
        g_printerr("\n%s sink pad has already been linked. Ignoring.\n", GST_ELEMENT_NAME(data->audioconvert));
        goto exit;
    }

    if (const auto& new_pad_caps_type = gst_structure_get_name(gst_caps_get_structure(new_pad_caps, 0));
        g_str_has_prefix(new_pad_caps_type, "audio/x-raw") == FALSE)
    {
        g_printerr("\nIt has type '%s' which is not raw audio. Ignoring.\n", new_pad_caps_type);
        goto exit;
    }

    {
        [[maybe_unused]] const auto result = GST_PAD_LINK_FAILED(gst_pad_link(newPad, audioconvert_1_sink_pad));
        g_print("%s\n", result ? "[FAILED]" : "[DONE]");
    }


exit:
    if (new_pad_caps not_eq nullptr)
    {
        gst_caps_unref(new_pad_caps);
    }

    gst_object_unref(audioconvert_1_sink_pad);
}


auto Pipeline::GetState_() const noexcept -> GstState
{
    return m_state_;
}

void Pipeline::SetUri_(const std::string& uriPath)
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
    SetupBusWatch_();
    SetupBin_();
    SetupLinks_();
    SetupIdentityBranch_();

    g_signal_connect(m_data_.uridecodebin, "pad-added", G_CALLBACK(&Pipeline::S_PadAddedHandlerOf_uridecodebin_), &m_data_);

    SetState_(GST_STATE_NULL);
    SetState_(GST_STATE_READY);

#ifdef FATLIB_BUILDING_ON_WINDOWS
    g_object_set(m_data_.audiosink, "low-latency", true, nullptr);
#endif

    g_print("Pipeline core has been built successfully.\n");
}

void Pipeline::SetupElements_() noexcept
{
    m_data_.uridecodebin   = CreatePlugin("uridecodebin"  , "uridecodebin-1");
    m_data_.audioconvert   = CreatePlugin("audioconvert"  , "audioconvert-1");
    m_data_.audioresample  = CreatePlugin("audioresample" , "audioresample-1");
    m_data_.tee            = CreatePlugin("tee"           , "tee-1");
    m_data_.queue_identity = CreatePlugin("queue"         , "queue-identity-1");
    m_data_.identity       = CreatePlugin("identity"      , "identity-1");
    m_data_.input_selector = CreatePlugin("input-selector", "input-selector-1");

#ifdef FATLIB_BUILDING_ON_WINDOWS
    m_data_.audiosink      = CreatePlugin("wasapisink"    , "wasapisink-1");
#else
    m_data_.audiosink      = CreatePlugin("autoaudiosink" , "autoaudiosink-1");
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
        m_data_.queue_identity,
        m_data_.identity,
        m_data_.input_selector,
        m_data_.audiosink,
        nullptr
    );
}

void Pipeline::SetupLinks_() noexcept
{
    LinkElements(m_data_.audioconvert  , m_data_.audioresample);
    LinkElements(m_data_.audioresample , m_data_.tee);
    LinkElements(m_data_.queue_identity, m_data_.identity);
    LinkElements(m_data_.input_selector, m_data_.audiosink);
}

void Pipeline::SetupIdentityBranch_() noexcept
{
    {
        const auto teeCleanSrcPad = UniqueGstPtr<GstPad>{ gst_element_request_pad_simple(m_data_.tee, "src_%u") };
        const auto drySinkPad     = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(m_data_.queue_identity, "sink") };

        if (GST_PAD_LINK_FAILED(gst_pad_link(teeCleanSrcPad.get(), drySinkPad.get())))
        {
            g_printerr("Failed to link tee to identity.\n");
        }
    }

    m_data_.dry_sel_pad = gst_element_request_pad_simple(m_data_.input_selector, "sink_%u");
    g_print("Obtained 'identity_pad': %s\n", gst_pad_get_name(m_data_.dry_sel_pad));

    if (const auto drySrcPad = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(m_data_.identity, "src") };
        GST_PAD_LINK_FAILED(gst_pad_link(drySrcPad.get(), m_data_.dry_sel_pad)))
    {
        g_printerr("Failed to link identity to input-selector.\n");
    }
}

void Pipeline::SetupGMainLoop_()
{
    g_print("Setting up GMainLoop ... ");

    if (m_pContext_ = g_main_context_new(); m_pContext_ == nullptr)
    {
        throw std::runtime_error{"Could not create main context for GMainLoop!"};
    }

    if (m_pLoop_= g_main_loop_new(m_pContext_, FALSE); m_pLoop_ == nullptr)
    {
        throw std::runtime_error{"Could not create loop for GMainLoop!"};
    }

    g_print("[DONE]\n");
}

void Pipeline::AttachAudioEffect_(std::shared_ptr<IAudioEffectBin> pEffect) noexcept
{
    if (pEffect == nullptr)
    {
        g_printerr(__func__);
        g_printerr(": null chain\n");
        return;
    }

    if (m_pAttachedEffect_ not_eq nullptr)
    {
        g_printerr(__func__);
        g_printerr(": a chain is already attached. Remove it first.\n");
        return;
    }

    auto* const pEffectBin = pEffect->GetBin();

    if (gst_bin_add(GST_BIN(m_pPipeline_), pEffectBin) == FALSE)
    {
        g_printerr(__func__);
        g_printerr(": Could not add chain bin to pipeline\n");
        return;
    }

    m_data_.queue_wet = CreatePlugin("queue", "queue-wet-1");

    if (m_data_.queue_wet == nullptr)
    {
        g_printerr(__func__);
        g_printerr(": Failed to create queue-wet\n");

        gst_bin_remove(GST_BIN(m_pPipeline_), pEffectBin);

        return;
    }

    if (gst_bin_add(GST_BIN(m_pPipeline_), m_data_.queue_wet) == FALSE)
    {
        g_printerr(__func__);
        g_printerr(": Could not add queue-wet to pipeline\n");

        gst_bin_remove(GST_BIN(m_pPipeline_), pEffectBin);
        gst_object_unref(m_data_.queue_wet);
        m_data_.queue_wet = nullptr;

        return;
    }

    gst_element_sync_state_with_parent(m_data_.queue_wet);
    gst_element_sync_state_with_parent(pEffectBin);

    m_data_.effect_tee_pad = gst_element_request_pad_simple(m_data_.tee, "src_%u");

    if (m_data_.effect_tee_pad == nullptr)
    {
        g_printerr(__func__);
        g_printerr(": Failed to request tee pad\n");

        gst_bin_remove(GST_BIN(m_pPipeline_), m_data_.queue_wet);
        gst_bin_remove(GST_BIN(m_pPipeline_), pEffectBin);
        m_data_.queue_wet = nullptr;

        return;
    }

    // TODO: simplify this

    {
        const auto queueSink = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(m_data_.queue_wet, "sink") };

        if (GST_PAD_LINK_FAILED(gst_pad_link(m_data_.effect_tee_pad, queueSink.get())))
        {
            g_printerr(__func__);
            g_printerr(": Failed to link tee -> queue_wet\n");

            gst_element_release_request_pad(m_data_.tee, m_data_.effect_tee_pad);
            gst_object_unref(m_data_.effect_tee_pad);
            gst_bin_remove(GST_BIN(m_pPipeline_), m_data_.queue_wet);
            gst_bin_remove(GST_BIN(m_pPipeline_), pEffectBin);
            m_data_.effect_tee_pad = nullptr;
            m_data_.queue_wet      = nullptr;

            return;
        }
    }

    {
        const auto queueSrc  = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(m_data_.queue_wet, "src") };
        const auto chainSink = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(pEffectBin, "sink") };

        if (GST_PAD_LINK_FAILED(gst_pad_link(queueSrc.get(), chainSink.get())))
        {
            g_printerr(__func__);
            g_printerr(": Failed to link queue_wet -> chain\n");

            gst_pad_unlink(m_data_.effect_tee_pad, queueSrc.get());
            gst_element_release_request_pad(m_data_.tee, m_data_.effect_tee_pad);
            gst_object_unref(m_data_.effect_tee_pad);
            gst_bin_remove(GST_BIN(m_pPipeline_), m_data_.queue_wet);
            gst_bin_remove(GST_BIN(m_pPipeline_), pEffectBin);
            m_data_.effect_tee_pad = nullptr;
            m_data_.queue_wet      = nullptr;

            return;
        }
    }

    m_data_.effect_sel_pad = gst_element_request_pad_simple(m_data_.input_selector, "sink_%u");

    if (m_data_.effect_sel_pad == nullptr)
    {
        g_printerr(__func__);
        g_printerr(": Failed to request input-selector sink pad\n");

        gst_element_release_request_pad(m_data_.tee, m_data_.effect_tee_pad);
        gst_object_unref(m_data_.effect_tee_pad);
        gst_bin_remove(GST_BIN(m_pPipeline_), m_data_.queue_wet);
        gst_bin_remove(GST_BIN(m_pPipeline_), pEffectBin);
        m_data_.effect_tee_pad = nullptr;
        m_data_.queue_wet      = nullptr;

        return;
    }

    {
        const auto chainSrc = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(pEffectBin, "src") };

        if (GST_PAD_LINK_FAILED(gst_pad_link(chainSrc.get(), m_data_.effect_sel_pad)))
        {
            g_printerr(__func__);
            g_printerr(": Failed to link chain -> input-selector\n");

            gst_element_release_request_pad(m_data_.input_selector, m_data_.effect_sel_pad);
            gst_object_unref(m_data_.effect_sel_pad);
            gst_element_release_request_pad(m_data_.tee, m_data_.effect_tee_pad);
            gst_object_unref(m_data_.effect_tee_pad);
            gst_bin_remove(GST_BIN(m_pPipeline_), m_data_.queue_wet);
            gst_bin_remove(GST_BIN(m_pPipeline_), pEffectBin);
            m_data_.effect_sel_pad = nullptr;
            m_data_.effect_tee_pad = nullptr;
            m_data_.queue_wet = nullptr;

            return;
        }
    }

    g_object_set(m_data_.input_selector, "active-pad", m_data_.effect_sel_pad, nullptr);

    m_pAttachedEffect_ = pEffect;

    g_print(__func__);
    g_print(": Effect chain attached and active.\n");
}

void Pipeline::DetachAudioEffect_() noexcept
{
    if (m_pAttachedEffect_ == nullptr)
    {
        g_printerr(__func__);
        g_printerr(": No audio effect chain attached.\n");
        return;
    }

    {
        auto* const pEffectBin = m_pAttachedEffect_->GetBin();

        if (m_data_.dry_sel_pad not_eq nullptr)
        {
            g_object_set(m_data_.input_selector, "active-pad", m_data_.dry_sel_pad, nullptr);

            g_printerr(__func__);
            g_print(": Switched input-selector to dry path.\n");
        }

        if (m_data_.effect_sel_pad not_eq nullptr)
        {
            gst_element_release_request_pad(m_data_.input_selector, m_data_.effect_sel_pad);
            gst_object_unref(m_data_.effect_sel_pad);
            m_data_.effect_sel_pad = nullptr;
        }

        if (m_data_.effect_tee_pad not_eq nullptr)
        {
            gst_element_release_request_pad(m_data_.tee, m_data_.effect_tee_pad);
            gst_object_unref(m_data_.effect_tee_pad);
            m_data_.effect_tee_pad = nullptr;
        }

        if (m_data_.queue_wet not_eq nullptr)
        {
            gst_element_set_state(m_data_.queue_wet, GST_STATE_NULL);
            gst_bin_remove(GST_BIN(m_pPipeline_), m_data_.queue_wet);
            m_data_.queue_wet = nullptr;
        }

        if (pEffectBin not_eq nullptr)
        {
            gst_element_set_state(pEffectBin, GST_STATE_NULL);
            gst_bin_remove(GST_BIN(m_pPipeline_), pEffectBin);
        }
    }

    m_pAttachedEffect_.reset();

    g_print(__func__);
    g_print(": Audio effect chain detached and cleaned up.\n");
}

void Pipeline::LoadAudio_(const std::string& uriPath) noexcept
{
    g_print("Executing task: 'LoadAudio'\n");

    if (m_pPipeline_ == nullptr)
    {
        g_printerr("Pipeline does not exist!\n");
        return;
    }

    SetState_(GST_STATE_READY);
    SetUri_(uriPath);
    Pause_();
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

    if (not m_loaded_uri_.empty())
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

void Pipeline::Seek_(const std::size_t& pos) noexcept
{
    g_print("Executing task: 'Seek'\n");

    if (m_pPipeline_ == nullptr)
    {
        g_printerr("Pipeline does not exist!\n");
        return;
    }

    if (m_loaded_uri_.empty())
    {
        g_warning("No audio is loaded.\n");
        return;
    }

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
}

void Pipeline::RunFunc_(std::function<void()> func) noexcept
{
    try
    {
        func();
    }
    catch (...)
    {

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
    m_data_      = {};

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
    m_work_start_signal_.acquire();
    g_print("Worker thread has just started.\n");

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
