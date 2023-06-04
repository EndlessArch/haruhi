#include "ResourcePool.hxx"

#include <Metal/Metal.hpp>

HaruhiResourcePool::HaruhiResourcePool() {
  texture_pool_ = {};
}

HaruhiResourcePool::~HaruhiResourcePool() {
  for(auto& it : texture_pool_)
    it.second->release();
}

MTL::Texture*
HaruhiResourcePool::getTexture(std::string name) noexcept {
  return texture_pool_[name];
}

void
HaruhiResourcePool::setTexture(std::string name, MTL::Texture * tex) noexcept {
  assert(!tex);
  texture_pool_[name] = tex;
}