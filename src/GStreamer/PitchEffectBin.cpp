#include <GStreamer/PitchEffectBin.hpp>

#include <_macros/Logging.hpp>

GST_BEGIN_NAMESPACE

PitchEffectBin::PitchEffectBin() noexcept
    :
    m_pBin_(gst_bin_new("pitch_bin"))
{
    if (m_pBin_ == nullptr)
    {
        MP_PRINTERR("Could NOT create GstBin!\n");

        return;
    }

    auto* const queue  = gst_element_factory_make("queue",         "queue_wet");
    auto* const valve  = gst_element_factory_make("valve",         "valve_pitch");
    auto* const pitch  = gst_element_factory_make("pitch",         "pitch");
    auto* const conv   = gst_element_factory_make("audioconvert",  "post_conv");
    auto* const resamp = gst_element_factory_make("audioresample", "post_resamp");

    gst_bin_add_many(GST_BIN(m_pBin_), queue, valve, pitch, conv, resamp, nullptr);
    gst_element_link_many(queue, valve, pitch, conv, resamp, nullptr);


    // GHOST PADs

    auto* const sinkpad = gst_element_get_static_pad(queue, "sink");
    auto* const srcpad  = gst_element_get_static_pad(resamp, "src");
    gst_element_add_pad(m_pBin_, gst_ghost_pad_new("sink", sinkpad));
    gst_element_add_pad(m_pBin_, gst_ghost_pad_new("src",  srcpad));
    gst_object_unref(sinkpad);
    gst_object_unref(srcpad);


    // demo

    g_object_set(pitch, "pitch", 1.2, nullptr);
}


auto PitchEffectBin::GetBin() const noexcept -> GstElement*
{
    return m_pBin_;
}

auto PitchEffectBin::GetSinkPad() const noexcept -> GstPad*
{
    return gst_element_get_static_pad(m_pBin_, "sink");
}

auto PitchEffectBin::GetSrcPad() const noexcept -> GstPad*
{
    return gst_element_get_static_pad(m_pBin_, "src");
}

GST_END_NAMESPACE
