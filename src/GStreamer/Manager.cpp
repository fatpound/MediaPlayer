#include <GStreamer/Manager.hpp>

#include <_misc/FatCodex/Macros/ExternalWarnings_MSVC.hpp>

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : MSVC_EXWARN_GST)
#endif

#include <gst/gst.h>

#ifdef _MSC_VER
#pragma warning (pop)
#endif

GST_BEGIN_NAMESPACE

Manager::Manager(int& argc, char**& argv) noexcept
{
    gst_init(&argc, &argv);
}

Manager::~Manager() noexcept
{
    gst_deinit();
}

GST_END_NAMESPACE
