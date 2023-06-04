#include <mtl.hpp>

#include <iostream>
#include <memory>

int main(int argc, char * argv[]) {

  struct __mtl_delete {
    void operator()(MTL::Device* a) const noexcept {
      a->release();
  }};

  std::unique_ptr<MTL::Device, __mtl_delete> device(MTL::CreateSystemDefaultDevice());

  std::cout << "Device: " << device->name()->utf8String() << '\n';

  return 0;
}