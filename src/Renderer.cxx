#include "Renderer.hxx"

#include "GameEngine.hxx"

using namespace NS;

HaruhiRenderer::HaruhiRenderer(Haruhi* pHaru, MTL::Device* pDev)
: p_haruhi_(pHaru), p_device_(pDev), angle_(0.), frame_(0), animation_ind_(0)
{
  p_cmd_queue_ = p_device_->newCommandQueue();
  buildShaders();
  buildComputePipeline();
  buildDepthStencilStates();
  buildTextures();
  buildBufs();

  sema_ = dispatch_semaphore_create(MAX_FRAMES_IN_FLIGHT);
}

HaruhiRenderer::~HaruhiRenderer() {
  pTextureAnimationBuf->release();
  p_texture_->release();
  p_shader_lib_->release();
  p_dss_->release();
  pVertexBuf->release();
  for(int i = 0; i< MAX_FRAMES_IN_FLIGHT; ++i)
    pInstanceBuf[i]->release();
  for(int i = 0; i< MAX_FRAMES_IN_FLIGHT; ++i)
    pCameraBuf[i]->release();
  pIndexBuf->release();
  p_cps_->release();
  p_rps_->release();
  p_cmd_queue_->release();
  p_device_->release();
}

void
HaruhiRenderer::buildShaders() {
  const char * shader_source = R"(
    #include <metal_stdlib>
    using namespace metal;
    struct v2f
    {
        float4 position [[position]];
        float3 normal;
        half3 color;
        float2 texcoord;
    };
    struct VertexData
    {
        float3 position;
        float3 normal;
        float2 texcoord;
    };
    struct InstanceData
    {
        float4x4 instanceTransform;
        float3x3 instanceNormalTransform;
        float4 instanceColor;
    };
    struct CameraData
    {
        float4x4 perspectiveTransform;
        float4x4 worldTransform;
        float3x3 worldNormalTransform;
    };
    v2f vertex fn_vertex(device const VertexData* vertexData [[buffer(0)]],
                          device const InstanceData* instanceData [[buffer(1)]],
                          device const CameraData& cameraData [[buffer(2)]],
                          uint vertexId [[vertex_id]],
                          uint instanceId [[instance_id]] )
    {
      device const auto& cvert = vertexData[vertexId];
      device const auto& cinst = instanceData[instanceId];
      device const auto& ccam = cameraData;

      float4 pos =
        ccam.perspectiveTransform * ccam.worldTransform
        * cinst.instanceTransform * float4(cvert.position, 1.0);

      float3 norm =
        ccam.worldNormalTransform *
        (cinst.instanceNormalTransform * cvert.normal);
      ;
      return (struct v2f){pos, norm, half3(cinst.instanceColor.rgb), cvert.texcoord.xy};
    }
    half4 fragment fn_frag(
        v2f in [[stage_in]],
        texture2d<half, access::sample> tex [[texture(0)]],
        sampler texSampler [[sampler(0)]]
      ) {
        // half3 texel = {.5, .5, .5};
      half3 texel = tex.sample( texSampler, in.texcoord ).rgb;

      float3 l = normalize(float3( 1.0, 1.0, 0.8 ));
      float3 n = normalize( in.normal );

      half ndotl = half( saturate( dot( n, l ) ) );

      half3 illum = (in.color * texel * 0.1) + (in.color * texel * ndotl);
      return half4( illum, 1.0 );

      // return half4(1.);
      return half4(tex.sample(texSampler, in.texcoord).rgb, 1.);
    }
    )";
  ;

  Error* pErr = nullptr;

  MTL::Library* pLib =
    p_device_->newLibrary(
      String::string(shader_source, UTF8StringEncoding),
      nullptr, &pErr);
  ;

  if(!pLib) {
    printf("%s", pErr->localizedDescription()->utf8String());
    abort();
  }

  MTL::Function* vertexFn =
    pLib->newFunction(String::string("fn_vertex", UTF8StringEncoding));
  MTL::Function* fragFn =
    pLib->newFunction(String::string("fn_frag", UTF8StringEncoding));
  ;

  MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
  pDesc->setVertexFunction(vertexFn);
  pDesc->setFragmentFunction(fragFn);
  pDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
  pDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth16Unorm);

  p_rps_ = p_device_->newRenderPipelineState(pDesc, &pErr);
  if(!p_rps_) {
    printf("%s", pErr->localizedDescription()->utf8String());
    abort();
  }

  for(auto it : (Object*[]){vertexFn, fragFn, pDesc})
    it->release();
  p_shader_lib_ = pLib;
}

void
HaruhiRenderer::buildComputePipeline() {
  const char * kernel_source = R"(
    #include <metal_stdlib>
    using namespace metal;
    kernel void compute_texture(texture2d<float, access::write> out [[texture(0)]],
                                texture2d<float, access::read>  in  [[texture(1)]],
                                device const uint* image [[buffer(0)]],
                                uint2 id [[thread_position_in_grid]])
    {
      float3 val = in.read(id).rgb;
      constexpr float emphasis = 1.; // .9
      out.write(emphasis * float4(val.r, val.g, val.b, 1.0), id);
    }
  )";

  Error * pErr = nullptr;

  MTL::Library* pComputeLib =
    p_device_->newLibrary(
      String::string(kernel_source, UTF8StringEncoding), nullptr, &pErr);
  ;
  if(!pComputeLib) {
    printf("%s", pErr->localizedDescription()->utf8String());
    abort();
  }

  MTL::Function* pComputeFn =
    pComputeLib->newFunction(String::string("compute_texture", UTF8StringEncoding));
  p_cps_ = p_device_->newComputePipelineState(pComputeFn, &pErr);
  if(!p_cps_) {
    printf("%s", pErr->localizedDescription()->utf8String());
    abort();
  }

  pComputeFn->release();
  pComputeLib->release();
}

void
HaruhiRenderer::buildDepthStencilStates() {
  MTL::DepthStencilDescriptor* pDSdesc = MTL::DepthStencilDescriptor::alloc()->init();
  pDSdesc->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
  pDSdesc->setDepthWriteEnabled(true);

  p_dss_ = p_device_->newDepthStencilState(pDSdesc);

  pDSdesc->release();
}

void
HaruhiRenderer::buildTextures() {

  Error* pErr = nullptr;

  // 22, 8 9 10
  p_texture_ = p_haruhi_->accessResourcePool()->getTexture("blocks");

  // MTL::TextureDescriptor::textureBufferDescriptor texDesc(
  //   MTL::PixelFormatA8Unorm, 16,
  //   MTL::ResourceStorageModeManaged, MTL::ResourceUsageSample);

  // MTL::TextureDescriptor* pTextureDesc = MTL::TextureDescriptor::alloc()->init();
  // pTextureDesc->setWidth(TEXTURE_WIDTH);
  // pTextureDesc->setHeight(TEXTURE_HEIGHT);
  // pTextureDesc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
  // pTextureDesc->setTextureType(MTL::TextureType2D);
  // pTextureDesc->setStorageMode(MTL::StorageModeManaged);
  // pTextureDesc->setUsage(
  //   MTL::ResourceUsageSample | MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
  // ;

  // MTL::Texture* pTexture = p_device_->newTexture(pTextureDesc);//->buffer();
  // p_texture_ = pTexture;

  // pTextureDesc->release();
}

#include <simd/simd.h>

namespace shader_t {

struct VertexData {
  simd::float3 pos;
  simd::float3 norm;
  simd::float2 texcoord;
};

struct InstanceData {
  simd::float4x4 instanceTransform;
  simd::float3x3 instanceNormalTransform;
  simd::float4 instanceColor;
};

struct CameraData {
  simd::float4x4 perspTransform;
  simd::float4x4 worldTransform;
  simd::float3x3 worldNormalTransform;
};

} // ns shader_t

void
HaruhiRenderer::buildBufs() {
  using simd::float2;
  using simd::float3;

  constexpr float s = .5f;

  shader_t::VertexData verts[] = {
    { { -s, -s, +s }, {  0.f,  0.f,  1.f }, { 0.f, 1.f } },
    { { +s, -s, +s }, {  0.f,  0.f,  1.f }, { 1.f, 1.f } },
    { { +s, +s, +s }, {  0.f,  0.f,  1.f }, { 1.f, 0.f } },
    { { -s, +s, +s }, {  0.f,  0.f,  1.f }, { 0.f, 0.f } },

    { { +s, -s, +s }, {  1.f,  0.f,  0.f }, { 0.f, 1.f } },
    { { +s, -s, -s }, {  1.f,  0.f,  0.f }, { 1.f, 1.f } },
    { { +s, +s, -s }, {  1.f,  0.f,  0.f }, { 1.f, 0.f } },
    { { +s, +s, +s }, {  1.f,  0.f,  0.f }, { 0.f, 0.f } },

    { { +s, -s, -s }, {  0.f,  0.f, -1.f }, { 0.f, 1.f } },
    { { -s, -s, -s }, {  0.f,  0.f, -1.f }, { 1.f, 1.f } },
    { { -s, +s, -s }, {  0.f,  0.f, -1.f }, { 1.f, 0.f } },
    { { +s, +s, -s }, {  0.f,  0.f, -1.f }, { 0.f, 0.f } },

    { { -s, -s, -s }, { -1.f,  0.f,  0.f }, { 0.f, 1.f } },
    { { -s, -s, +s }, { -1.f,  0.f,  0.f }, { 1.f, 1.f } },
    { { -s, +s, +s }, { -1.f,  0.f,  0.f }, { 1.f, 0.f } },
    { { -s, +s, -s }, { -1.f,  0.f,  0.f }, { 0.f, 0.f } },

    { { -s, +s, +s }, {  0.f,  1.f,  0.f }, { 0.f, 1.f } },
    { { +s, +s, +s }, {  0.f,  1.f,  0.f }, { 1.f, 1.f } },
    { { +s, +s, -s }, {  0.f,  1.f,  0.f }, { 1.f, 0.f } },
    { { -s, +s, -s }, {  0.f,  1.f,  0.f }, { 0.f, 0.f } },

    { { -s, -s, -s }, {  0.f, -1.f,  0.f }, { 0.f, 1.f } },
    { { +s, -s, -s }, {  0.f, -1.f,  0.f }, { 1.f, 1.f } },
    { { +s, -s, +s }, {  0.f, -1.f,  0.f }, { 1.f, 0.f } },
    { { -s, -s, +s }, {  0.f, -1.f,  0.f }, { 0.f, 0.f } }
  };
  
  uint16_t indices[] = {
    // front
    0,  1,  2,
    2,  3,  0,
    // right
    4,  5,  6,
    6,  7,  4,
    // back
    8,  9, 10,
    10, 11, 8,
    // left
    12, 13, 14,
    14, 15, 12,
    //top
    16, 17, 18,
    18, 19, 16,
    //bot
    20, 21, 22,
    22, 23, 20,
  };

  constexpr size_t vertexData_sz = sizeof(verts);
  constexpr size_t indexData_sz = sizeof(indices);

  MTL::Buffer* pVertBuf =
    p_device_->newBuffer(vertexData_sz, MTL::ResourceStorageModeManaged);
  MTL::Buffer* pIndBuf =
    p_device_->newBuffer(indexData_sz, MTL::ResourceStorageModeManaged);
  ;

  pVertexBuf = pVertBuf;
  pIndexBuf = pIndBuf;

  memcpy(pVertexBuf->contents(), verts, vertexData_sz);
  memcpy(pIndexBuf->contents(), indices, indexData_sz);

  pVertexBuf->didModifyRange(Range::Make(0, pVertexBuf->length()));
  pIndexBuf->didModifyRange(Range::Make(0, pIndexBuf->length()));

  const size_t instanceData_sz = MAX_FRAMES_IN_FLIGHT*1*sizeof(shader_t::InstanceData);
  for(size_t i = 0; i< MAX_FRAMES_IN_FLIGHT; ++i)
    pInstanceBuf[i] = p_device_->newBuffer(instanceData_sz, MTL::ResourceStorageModeManaged);

  const size_t cameraData_sz = MAX_FRAMES_IN_FLIGHT*sizeof(shader_t::CameraData);
  for(size_t i = 0; i< MAX_FRAMES_IN_FLIGHT; ++i)
    pCameraBuf[i] = p_device_->newBuffer(cameraData_sz, MTL::ResourceStorageModeManaged);

  pTextureAnimationBuf = p_device_->newBuffer(sizeof(unsigned), MTL::ResourceStorageModeManaged);
}

void
HaruhiRenderer::computeTexture(MTL::CommandBuffer* pCmdBuf) {
  if(!pCmdBuf) {
    printf("Command buffer got null-ed\n");
    abort();
  }

  // unsigned* ptr = reinterpret_cast<unsigned *>(pTextureAnimationBuf->contents());
  //# *ptr = (animation_ind_++) % 5000;
  // pTextureAnimationBuf->didModifyRange(Range::Make(0, sizeof(unsigned)));

  MTL::ComputeCommandEncoder* pCCE = pCmdBuf->computeCommandEncoder();

  pCCE->setComputePipelineState(p_cps_);
  pCCE->setTexture(p_texture_, 0);
  pCCE->setTexture(p_texture_, 1);
  pCCE->setBuffer(pTextureAnimationBuf, 0, 0);

  MTL::Size grid_sz(16, 16, 1); // chunk?

  UInteger thread_group_sz_ = p_cps_->maxTotalThreadsPerThreadgroup();
  MTL::Size thread_gruop_sz(thread_group_sz_, 1, 1);

  pCCE->dispatchThreads(grid_sz, thread_gruop_sz);

  pCCE->endEncoding();
}

#include "MathUtil.hxx"

void
HaruhiRenderer::draw(MTK::View * pView) {
  using simd::float3;
  using simd::float4;
  using simd::float4x4;

  AutoreleasePool* pARPool = AutoreleasePool::alloc()->init();

  frame_ = (frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
  MTL::Buffer* p_instanceData_buf = pInstanceBuf[frame_];

  MTL::CommandBuffer* p_cmd_buf = p_cmd_queue_->commandBuffer();

  dispatch_semaphore_wait(sema_, DISPATCH_TIME_FOREVER);
  p_cmd_buf->addCompletedHandler([this](MTL::CommandBuffer* p_cmd) {
    dispatch_semaphore_signal(this->sema_);
  });

  static uint __cnt = 0; ++__cnt;

  shader_t::InstanceData* p_instanceData =
    reinterpret_cast<shader_t::InstanceData*>(p_instanceData_buf->contents());
  p_instanceData[0].instanceTransform =
    math::makeTranslate({ 0., 0., -3. })
    * math::makeYRotate(__cnt*0.02*3.14) * math::makeXRotate(0.2);
  p_instanceData[0].instanceNormalTransform =
    math::discardTranslation(p_instanceData[0].instanceTransform);
  p_instanceData[0].instanceColor = {.5,.5,.5,1.};//{ 0., 5., 5., 1. };
  p_instanceData_buf->didModifyRange(Range::Make(0, p_instanceData_buf->length()));

  MTL::Buffer* p_cameraData_buf = pCameraBuf[frame_];
  shader_t::CameraData* p_cameraData =
    reinterpret_cast<shader_t::CameraData *>(p_cameraData_buf->contents());
  constexpr float FOV = 90.;
  p_cameraData->perspTransform =
    math::makePerspective(FOV * 3.141592 / 180., 1., .03, 500.);
  // const auto delta = animation_ind_/10.;  // printf("%d\n", animation_ind_);
  p_cameraData->worldTransform = math::makeIdentity();
    // // * math::makeTranslate({cosf(3.14*frame_*.02), sinf(3.14*frame_*.02), 1.});
  p_cameraData->worldNormalTransform = math::discardTranslation(p_cameraData->worldTransform);
  p_cameraData_buf->didModifyRange(Range::Make(0, 1*sizeof(shader_t::CameraData)));

  // WARNING: Maybe you should restart your computer
  // computeTexture(p_cmd_buf);

  MTL::SamplerDescriptor* p_sd = MTL::SamplerDescriptor::alloc()->init();
  // p_sd->setNormalizedCoordinates(false);
  p_sd->setMagFilter(MTL::SamplerMinMagFilterLinear);
  p_sd->setMinFilter(MTL::SamplerMinMagFilterLinear);
  p_sd->setSAddressMode(MTL::SamplerAddressModeRepeat);
  p_sd->setTAddressMode(MTL::SamplerAddressModeRepeat);
  auto p_ss = p_device_->newSamplerState(p_sd);

  MTL::RenderPassDescriptor* p_rpd = pView->currentRenderPassDescriptor();

  p_rpd->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0., .8, 1., 1.));

  MTL::RenderCommandEncoder* p_rce = p_cmd_buf->renderCommandEncoder(p_rpd);

  p_rce->setRenderPipelineState(p_rps_);
  p_rce->setDepthStencilState(p_dss_);

  p_rce->setVertexBuffer(pVertexBuf, 0, 0);
  p_rce->setVertexBuffer(p_instanceData_buf, 0, 1);
  p_rce->setVertexBuffer(p_cameraData_buf, 0, 2);

  p_rce->setFragmentTexture(p_texture_, 0);
  p_rce->setFragmentSamplerState(p_ss, 0);

  p_rce->setCullMode(MTL::CullModeBack);
  p_rce->setFrontFacingWinding(MTL::WindingCounterClockwise);

  p_rce->drawIndexedPrimitives(
    MTL::PrimitiveTypeTriangle,
    6*6,
    MTL::IndexType::IndexTypeUInt16,
    pIndexBuf,
    0,
    1/*instance cnt*/);
  ;

  p_rce->endEncoding();
  p_cmd_buf->presentDrawable(pView->currentDrawable());
  p_cmd_buf->commit();

  pARPool->release();
}