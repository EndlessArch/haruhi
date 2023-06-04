#include "GameEngine.hxx"

std::unique_ptr<HaruhiResourcePool>
Haruhi::res_pool_ = {};

Haruhi::Haruhi() {
  res_pool_ = std::make_unique<HaruhiResourcePool>(HaruhiResourcePool());
}

HaruhiResourcePool*
Haruhi::accessResourcePool() const noexcept {
  return res_pool_.get();
}