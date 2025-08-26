#ifndef IEFFECTBIN_H
#define IEFFECTBIN_H

#include <_macros/Namespaces.hpp>

#include <gst/gst.h>

GST_BEGIN_NAMESPACE

class IEffectBin
{
public:
    IEffectBin() noexcept             = default;
    IEffectBin(const IEffectBin&)     = default;
    IEffectBin(IEffectBin&&) noexcept = default;

    auto operator = (const IEffectBin&)     -> IEffectBin& = default;
    auto operator = (IEffectBin&&) noexcept -> IEffectBin& = default;
    virtual ~IEffectBin() noexcept = default;


public:
    [[nodiscard]] virtual auto GetBin     () const -> GstElement* = 0;
    [[nodiscard]] virtual auto GetSinkPad () const -> GstPad*     = 0;
    [[nodiscard]] virtual auto GetSrcPad  () const -> GstPad*     = 0;


protected:


private:
};

GST_END_NAMESPACE

#endif // IEFFECTBIN_H
