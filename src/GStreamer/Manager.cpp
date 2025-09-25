#include <GStreamer/Manager.hpp>
#include <GStreamer/FatGst.hpp>

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
