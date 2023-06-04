#ifndef HARUHI_RESOURCEPOOL_HXX
#define HARUHI_RESOURCEPOOL_HXX

#include <string>
#include <map>

namespace MTL {
class Texture;
} // ns MTL

class HaruhiResourcePool {
  std::map<std::string, MTL::Texture*> texture_pool_;
public:
  HaruhiResourcePool();
  ~HaruhiResourcePool();

  MTL::Texture* getTexture(std::string) noexcept;
  void setTexture(std::string, MTL::Texture*) noexcept;
};

#endif