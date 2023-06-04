#ifndef HARUHI_GAMEENGINE_HXX
#define HARUHI_GAMEENGINE_HXX

#include "ResourcePool.hxx"

// yes, game engine itself is actually haruhi
class Haruhi {
  static std::unique_ptr<HaruhiResourcePool> res_pool_;
public:

  Haruhi();

  HaruhiResourcePool* accessResourcePool() const noexcept;
};

#endif