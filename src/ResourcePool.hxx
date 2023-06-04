#ifndef HARUHI_RESOURCEPOOL_HXX
#define HARUHI_RESOURCEPOOL_HXX

#include <map>

#include <mtl.hpp>

class HaruhiResourcePool {
  std::map<std::string, MTL::Texture*> texture_pool_;
public:
  HaruhiResourcePool();
  ~HaruhiResourcePool();

  MTL::Texture* getTexture(std::string) noexcept;
  void setTexture(std::string, MTL::Texture*) noexcept;
};

#endif