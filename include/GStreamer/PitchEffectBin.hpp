#ifndef PITCHEFFECTBIN_HPP
#define PITCHEFFECTBIN_HPP

#include <_macros/Namespaces.hpp>

#include <GStreamer/IEffectBin.hpp>

GST_BEGIN_NAMESPACE

class PitchEffectBin : public IEffectBin
{
public:
    PitchEffectBin() noexcept;
    PitchEffectBin(const PitchEffectBin&)     = default;
    PitchEffectBin(PitchEffectBin&&) noexcept = default;

    auto operator = (const PitchEffectBin&)     -> PitchEffectBin& = default;
    auto operator = (PitchEffectBin&&) noexcept -> PitchEffectBin& = default;
    virtual ~PitchEffectBin() noexcept = default;


public:
    [[nodiscard]] virtual auto GetBin     () const noexcept -> GstElement* override;
    [[nodiscard]] virtual auto GetSinkPad () const noexcept -> GstPad*     override;
    [[nodiscard]] virtual auto GetSrcPad  () const noexcept -> GstPad*     override;


protected:


private:
    GstElement*  m_pBin_{};
};

GST_END_NAMESPACE

#endif // PITCHEFFECTBIN_HPP
