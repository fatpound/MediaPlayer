#include <GStreamer/PitchEffectBin.hpp>
#include <GStreamer/CommonUtilities.hpp>
#include <GStreamer/Pipeline.hpp>

#include <_macros/Logging.hpp>

GST_BEGIN_NAMESPACE

PitchEffectBin::PitchEffectBin() noexcept
    :
    m_pBin_(CreateBin("pitch_bin")),
    m_pValve_(CreatePlugin("valve", "valve_pitch")),
    m_pPitch_(CreatePlugin("pitch", "pitch"))
{
    gst_bin_add_many(GST_BIN(m_pBin_), m_pValve_, m_pPitch_, nullptr);

    LinkElements(m_pValve_, m_pPitch_);

    const auto sinkpad = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(m_pValve_, "sink") };
    const auto srcpad  = UniqueGstPtr<GstPad>{ gst_element_get_static_pad(m_pPitch_, "src") };

    gst_element_add_pad(m_pBin_, gst_ghost_pad_new("sink", sinkpad.get()));
    gst_element_add_pad(m_pBin_, gst_ghost_pad_new("src",  srcpad.get()));
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


void PitchEffectBin::SetPitchAsync(Pipeline& pipeline, const double newPitch)
{
    pipeline.RunFunc(
        [pPitch = this->m_pPitch_, newPitch] () noexcept -> void
        {
            if (pPitch not_eq nullptr)
            {
                g_object_set(G_OBJECT(pPitch), "pitch", newPitch, nullptr);
            }
        }
    );
}

GST_END_NAMESPACE
