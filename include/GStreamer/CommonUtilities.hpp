#ifndef COMMONUTILITIES_HPP
#define COMMONUTILITIES_HPP

#include <_macros/Namespaces.hpp>

#include <GStreamer/FatGst.hpp>
#include <GStreamer/Logging.hpp>

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
        MP_PRINTERR("Could NOT create GStreamer bin %s!", name.c_str());
    }

    return ptr;
}

inline auto CreatePlugin(const std::string& factoryName, const std::string& name) noexcept -> GstElement*
{
    auto* const ptr = gst_element_factory_make(factoryName.c_str(), name.c_str());

    if (ptr == nullptr)
    {
        MP_PRINTERR("Could NOT create GStreamer plugin '%s' of type '%s'\n", factoryName.c_str(), name.c_str());
    }

    return ptr;
}

inline void LinkElements(GstElement* const src, GstElement* const dest) noexcept
{
    MP_PRINT("Linking pipeline elements %s => %s ... ", gst_element_get_name(src), gst_element_get_name(dest));
    if (gst_element_link(src, dest) == FALSE)
    {
        MP_PRINTERR("[FAILED]\n");
    }
    MP_PRINT("[DONE]\n");
}

inline void PrintErrorMsg(GstMessage* const msg) noexcept
{
    GError* err{};
    gchar* debug_info{};

    gst_message_parse_error(msg, &err, &debug_info);
    MP_PRINTERR("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    MP_PRINTERR("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);
}

GST_END_NAMESPACE

#endif // COMMONUTILITIES_HPP
