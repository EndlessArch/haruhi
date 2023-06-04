#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <memory>

#include <mtl.hpp>

#define impl_deleter(dummyName, targetTypeName)  \
struct __impl_deleter_##dummyName {\
  void operator()(targetTypeName*p) const noexcept {\
    p->release();\
  }\
}

int main(int argc, char * argv[]) {

  impl_deleter(arpool, NS::AutoreleasePool);
  std::unique_ptr<NS::AutoreleasePool, __impl_deleter_arpool> pool_alloc(NS::AutoreleasePool::alloc()->init());

  impl_deleter(mtldev, MTL::Device);
  std::unique_ptr<MTL::Device, __impl_deleter_mtldev> device(MTL::CreateSystemDefaultDevice());

  constexpr auto arr_size = 1<<5;
  constexpr auto buf_size = arr_size * sizeof(float);
  class Metal {
  private:
  protected:
    MTL::Device* device_;
    MTL::ComputePipelineState* pipelineState_;
    MTL::CommandQueue* commandQueue_;

    MTL::Buffer* bufA, *bufB, *bufRes;

  public:
    Metal() noexcept {}
    ~Metal() noexcept {
      // device_->release();
      pipelineState_->release();
      commandQueue_->release();
      bufA->release();
      bufB->release();
      bufRes->release();
    }
    void init(MTL::Device* p) {
      device_ = p;

      auto lib = device_->newDefaultLibrary();
      if(!lib) {
        printf("Failed to load device library\n");
        std::abort();
      }

      auto fn_name = NS::String::string("add_arrays", NS::ASCIIStringEncoding);
      auto add_fn = lib->newFunction(fn_name);
      if(!add_fn) {
        printf("Failed to find function\n");
        std::abort();
      }

      NS::Error* err;

      pipelineState_ = device_->newComputePipelineState(add_fn, &err);
      commandQueue_ = device_->newCommandQueue();
    }
    void prepare() {
      bufA = device_->newBuffer(arr_size, MTL::ResourceStorageModeShared);
      bufB = device_->newBuffer(arr_size, MTL::ResourceStorageModeShared);
      bufRes = device_->newBuffer(arr_size, MTL::ResourceStorageModeShared);

      for(auto it : {bufA, bufB}) {
        float* pVal = (float *)it->contents();
        for(unsigned long i = 0; i < arr_size; ++i)
          pVal[i] = (float)std::rand() / (float)RAND_MAX;
      }
    }
    void send_command() {
      MTL::CommandBuffer* cb = commandQueue_->commandBuffer();
      MTL::ComputeCommandEncoder* cce = cb->computeCommandEncoder();

      {
        cce->setComputePipelineState(pipelineState_);
        cce->setBuffer(bufA, 0, 0);
        cce->setBuffer(bufB, 0, 1);
        cce->setBuffer(bufRes, 0, 2);

        MTL::Size grid_sz = MTL::Size(arr_size, 1, 1);

        NS::UInteger _thread_group_sz = pipelineState_->maxTotalThreadsPerThreadgroup();
        if(_thread_group_sz > arr_size)
          _thread_group_sz = arr_size;

        MTL::Size thread_group_sz = MTL::Size(_thread_group_sz, 1, 1);

        cce->dispatchThreads(grid_sz, thread_group_sz);
      }
      cce->endEncoding();
      cb->commit();
      cb->waitUntilCompleted();
    }
  };

  auto metal = std::make_unique<Metal>();
  metal->init(device.get());
  metal->prepare();
  metal->send_command();

  return 0;
}

void add_arrays(const float* iA, const float* iB, float* res, int len) {
  for(int i = 0; i< len; ++i)
    res[i] = iA[i] + iB[i];
}