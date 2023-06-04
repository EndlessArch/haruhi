#include <mtl.hpp>

#include "Delegates.hxx"
#include "MemoryUtility.hxx"

int main(int argc, char * argv[]) {

  nsp_unique<NS::AutoreleasePool> pARPool(NS::AutoreleasePool::alloc()->init());

  HaruhiDelegate hd;

  nsp_unique<NS::Application> pApp(NS::Application::sharedApplication());
  pApp->setDelegate(&hd);
  pApp->run();

  return 0;
}