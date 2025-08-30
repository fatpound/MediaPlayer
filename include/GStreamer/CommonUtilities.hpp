#ifndef COMMONUTILITIES_HPP
#define COMMONUTILITIES_HPP

#include <_macros/Namespaces.hpp>

#include <memory>

GST_BEGIN_NAMESPACE

template <typename T>
using UniqueGstPtr = std::unique_ptr<T, decltype([](T* const ptr) noexcept -> void { gst_object_unref(ptr); })>;

GST_END_NAMESPACE

#endif // COMMONUTILITIES_HPP
