#ifndef HARUHI_RENDERER_HXX
#define HARUHI_RENDERER_HXX

#include <mtl.hpp>

constexpr auto MAX_FRAMES_IN_FLIGHT =
#ifndef HARUHI_FRAMES_IN_FLIGHT
3;
#else
HARUHI_MAX_FRAMES_IN_FLIGHT;
#endif

class Haruhi;

class HaruhiRenderer {
  Haruhi* p_haruhi_;

  MTL::Device* p_device_;
  MTL::CommandQueue* p_cmd_queue_;
  MTL::Library* p_shader_lib_;
  MTL::RenderPipelineState* p_rps_;
  MTL::ComputePipelineState* p_cps_;
  MTL::DepthStencilState* p_dss_;

  MTL::Texture* p_texture_;
  MTL::Buffer
    * pVertexBuf,
    * pInstanceBuf[MAX_FRAMES_IN_FLIGHT],
    * pCameraBuf[MAX_FRAMES_IN_FLIGHT],
    * pIndexBuf, * pTextureAnimationBuf;
  ;

  float angle_;
  unsigned frame_;
  dispatch_semaphore_t sema_;
  unsigned animation_ind_;

public:

  HaruhiRenderer(Haruhi*, MTL::Device*);
  ~HaruhiRenderer();

  void buildShaders();
  void buildComputePipeline();
  void buildDepthStencilStates();
  void buildTextures();
  void buildBufs();
  void computeTexture(MTL::CommandBuffer*);
  void draw(MTK::View*);
};

#endif