#ifndef COMMONUTILITIES_HPP
#define COMMONUTILITIES_HPP

#include <_macros/Logging.hpp>
#include <_macros/Namespaces.hpp>

#include <gst/gst.h>

#include <string>
#include <memory>

GST_BEGIN_NAMESPACE

template <typename T>
using UniqueGstPtr = std::unique_ptr<T, decltype([](T* const ptr) noexcept -> void { gst_object_unref(ptr); })>;

inline auto CreateBin(const std::string& name) noexcept -> GstElement*
{
    auto* const ptr = gst_bin_new(name.c_str());

    if (ptr == nullptr)
    {
        MP_PRINTERR("Could NOT create GStreamer bin!");
    }

    return ptr;
}

inline auto CreatePlugin(const std::string& factoryName, const std::string& name) noexcept -> GstElement*
{
    auto* const ptr = gst_element_factory_make(factoryName.c_str(), name.c_str());

    if (ptr == nullptr)
    {
        MP_PRINTERR("Could NOT create GStreamer plugin!");
    }

    return ptr;
}

GST_END_NAMESPACE

#endif // COMMONUTILITIES_HPP
