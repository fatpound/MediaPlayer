#ifndef IAUDIOEFFECTBIN_HPP
#define IAUDIOEFFECTBIN_HPP

#include <_macros/Namespaces.hpp>

#include <GStreamer/FatGst.hpp>

GST_BEGIN_NAMESPACE

class IAudioEffectBin
{
public:
    IAudioEffectBin() noexcept                  = default;
    IAudioEffectBin(const IAudioEffectBin&)     = default;
    IAudioEffectBin(IAudioEffectBin&&) noexcept = default;

    auto operator = (const IAudioEffectBin&)     -> IAudioEffectBin& = default;
    auto operator = (IAudioEffectBin&&) noexcept -> IAudioEffectBin& = default;
    virtual ~IAudioEffectBin() noexcept                              = default;


public:
    [[nodiscard]] virtual auto GetBin     () const -> GstElement* = 0;
    [[nodiscard]] virtual auto GetSinkPad () const -> GstPad*     = 0;
    [[nodiscard]] virtual auto GetSrcPad  () const -> GstPad*     = 0;


protected:


private:
};

GST_END_NAMESPACE

#endif // IAUDIOEFFECTBIN_HPP
