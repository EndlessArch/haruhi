#ifndef HARUHI_LOADRESOURCE_HXX
#define HARUHI_LOADRESOURCE_HXX

#include <memory>
#include <tuple>

#include <spng/spng.h>

namespace MTL {
class Device;
} // ns MTL

namespace HaruhiResourceLoader {

namespace ImageUtil {

std::tuple<size_t, std::unique_ptr<spng_ihdr>, void*>
load_image_at(const char *) noexcept;

} // ns ImageUtil

void
loadResources(MTL::Device*) noexcept;

} // ns HaruhiResourceLoader

#endif