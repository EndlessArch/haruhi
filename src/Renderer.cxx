#include "Renderer.hxx"

using namespace NS;

HaruhiRenderer::HaruhiRenderer(MTL::Device* pDev)
: p_device_(pDev), angle_(0.), frame_(0), animation_ind_(0)
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
        v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                               device const InstanceData* instanceData [[buffer(1)]],
                               device const CameraData& cameraData [[buffer(2)]],
                               uint vertexId [[vertex_id]],
                               uint instanceId [[instance_id]] )
        {
            v2f o;
            const device VertexData& vd = vertexData[ vertexId ];
            float4 pos = float4( vd.position, 1.0 );
            pos = instanceData[ instanceId ].instanceTransform * pos;
            pos = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
            o.position = pos;
            float3 normal = instanceData[ instanceId ].instanceNormalTransform * vd.normal;
            normal = cameraData.worldNormalTransform * normal;
            o.normal = normal;
            o.texcoord = vd.texcoord.xy;
            o.color = half3( instanceData[ instanceId ].instanceColor.rgb );
            return o;
        }
        half4 fragment fragMain( v2f in [[stage_in]], texture2d< half, access::sample > tex [[texture(0)]] )
        {
            constexpr sampler s( address::repeat, filter::linear );
            half3 texel = tex.sample( s, in.texcoord ).rgb;
            // assume light coming from (front-top-right)
            float3 l = normalize(float3( 1.0, 1.0, 0.8 ));
            float3 n = normalize( in.normal );
            half ndotl = half( saturate( dot( n, l ) ) );
            half3 illum = (in.color * texel * 0.1) + (in.color * texel * ndotl);
            return half4( illum, 1.0 );
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
    pLib->newFunction(String::string("vertexMain", UTF8StringEncoding));
  MTL::Function* fragFn =
    pLib->newFunction(String::string("fragMain", UTF8StringEncoding));
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

  std::initializer_list<Object*> to_release {vertexFn, fragFn, pDesc};
  std::for_each(to_release.begin(), to_release.end(), [](Object*p){ p->release(); });
  p_shader_lib_ = pLib;
}

void
HaruhiRenderer::buildComputePipeline() {
  const char * kernel_source = R"(
    #include <metal_stdlib>
    using namespace metal;
    kernel void mandelbrot_set( texture2d< half, access::write > tex [[texture(0)]],
                                uint2 index [[thread_position_in_grid]],
                                uint2 gridSize [[threads_per_grid]],
                                device const uint* frame [[buffer(0)]])
    {
      constexpr float kAnimationFrequency = 0.01;
      constexpr float kAnimationSpeed = 4;
      constexpr float kAnimationScaleLow = 0.62;
      constexpr float kAnimationScale = 0.38;
      constexpr float2 kMandelbrotPixelOffset = {-0.2, -0.35};
      constexpr float2 kMandelbrotOrigin = {-1.2, -0.32};
      constexpr float2 kMandelbrotScale = {2.2, 2.0};
      // Map time to zoom value in [kAnimationScaleLow, 1]
      float zoom = kAnimationScaleLow + kAnimationScale * cos(kAnimationFrequency * *frame);
      // Speed up zooming
      zoom = pow(zoom, kAnimationSpeed);
      //Scale
      float x0 = zoom * kMandelbrotScale.x *
        ((float)index.x / gridSize.x + kMandelbrotPixelOffset.x) + kMandelbrotOrigin.x;
      float y0 = zoom * kMandelbrotScale.y * 
        ((float)index.y / gridSize.y + kMandelbrotPixelOffset.y) + kMandelbrotOrigin.y;
      // Implement Mandelbrot set
      float x = 0.0;
      float y = 0.0;
      uint iteration = 0;
      uint max_iteration = 1000;
      float xtmp = 0.0;
      while(x * x + y * y <= 4 && iteration < max_iteration) {
        xtmp = x * x - y * y + x0;
        y = 2 * x * y + y0;
        x = xtmp;
        iteration += 1;
      }
      // Convert iteration result to colors
      half color = (0.5 + 0.5 * cos(3.0 + iteration * 0.15));
      tex.write(half4(color, color, color, 1.0), index, 0);
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

  MTL::Function* pMandelbrotFn =
    pComputeLib->newFunction(String::string("mandelbrot_set", UTF8StringEncoding));
  p_cps_ = p_device_->newComputePipelineState(pMandelbrotFn, &pErr);
  if(!p_cps_) {
    printf("%s", pErr->localizedDescription()->utf8String());
    abort();
  }

  pMandelbrotFn->release();
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

constexpr auto TEXTURE_WIDTH = 128;
constexpr auto TEXTURE_HEIGHT = 128;

void
HaruhiRenderer::buildTextures() {
  MTL::TextureDescriptor* pTextureDesc = MTL::TextureDescriptor::alloc()->init();
  pTextureDesc->setWidth(TEXTURE_WIDTH);
  pTextureDesc->setHeight(TEXTURE_HEIGHT);
  pTextureDesc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
  pTextureDesc->setTextureType(MTL::TextureType2D);
  pTextureDesc->setStorageMode(MTL::StorageModeManaged);
  pTextureDesc->setUsage(
    MTL::ResourceUsageSample | MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
  ;

  MTL::Texture* pTexture = p_device_->newTexture(pTextureDesc);
  p_texture_ = pTexture;

  pTextureDesc->release();
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
      0,  1,  2,  2,  3,  0, /* front */
      4,  5,  6,  6,  7,  4, /* right */
      8,  9, 10, 10, 11,  8, /* back */
    12, 13, 14, 14, 15, 12, /* left */
    16, 17, 18, 18, 19, 16, /* top */
    20, 21, 22, 22, 23, 20, /* bot */
  };

  const size_t vertexData_sz = sizeof(verts);
  const size_t indexData_sz = sizeof(indices);

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

  const size_t instanceData_sz = MAX_FRAMES_IN_FLIGHT*1000*sizeof(shader_t::InstanceData);
  for(size_t i = 0; i< MAX_FRAMES_IN_FLIGHT; ++i)
    pInstanceBuf[i] = p_device_->newBuffer(instanceData_sz, MTL::ResourceStorageModeManaged);

  const size_t cameraData_sz = MAX_FRAMES_IN_FLIGHT*sizeof(shader_t::CameraData);
  for(size_t i = 0; i< MAX_FRAMES_IN_FLIGHT; ++i)
    pCameraBuf[i] = p_device_->newBuffer(cameraData_sz, MTL::ResourceStorageModeManaged);

  pTextureAnimationBuf = p_device_->newBuffer(sizeof(unsigned), MTL::ResourceStorageModeManaged);
}

void
HaruhiRenderer::generateMandelbrotTexture(MTL::CommandBuffer* pCmdBuf) {
  if(!pCmdBuf) {
    printf("Command buffer got null-ed\n");
    abort();
  }

  unsigned* ptr = reinterpret_cast<unsigned *>(pTextureAnimationBuf->contents());
  *ptr = animation_ind_ % 5000;
  pTextureAnimationBuf->didModifyRange(Range::Make(0, sizeof(unsigned)));

  MTL::ComputeCommandEncoder* pCCE = pCmdBuf->computeCommandEncoder();

  pCCE->setComputePipelineState(p_cps_);
  pCCE->setTexture(p_texture_, 0);
  pCCE->setBuffer(pTextureAnimationBuf, 0, 0);

  MTL::Size grid_sz = MTL::Size(TEXTURE_WIDTH, TEXTURE_HEIGHT, 1);

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
  HaruhiRenderer* p_renderer = this;
  p_cmd_buf->addCompletedHandler([p_renderer](MTL::CommandBuffer* p_cmd) {
    dispatch_semaphore_signal(p_renderer->sema_);
  });

  angle_ += .002;

  const float scl = .2f;
  shader_t::InstanceData* p_instanceData =
    reinterpret_cast<shader_t::InstanceData*>(p_instanceData_buf->contents());
  ;
  float3 objPos = { 0., 0., -10. };

  float4x4 rt = math::makeTranslate(objPos);
  float4x4 rr1 = math::makeYRotate(-angle_);
  float4x4 rr0 = math::makeXRotate(angle_ * .5);
  float4x4 rtInv = math::makeTranslate({ -objPos.x, -objPos.y, -objPos.z });
  float4x4 fullObjRot = rt * rr1 * rr0 * rtInv;

  size_t ix = 0, iy = 0, iz = 0;
  for(size_t i = 0; i< 1000; ++i) {
    if(ix == 10) {
      ix = 0;
      iy += 1;
    }
    if(iy == 10) {
      iy = 0;
      iz += 1;
    }

    float4x4 scale = math::makeScale((float3){ scl, scl, scl });
    float4x4 zrot = math::makeZRotate(angle_ * sinf((float)ix));
    float4x4 yrot = math::makeYRotate(angle_ * cosf((float)iy));
    
    float x = ((float)ix - 5.) * (2. * scl) + scl;
    float y = ((float)iy - 5.) * (2. * scl) + scl;
    float z = ((float)iz - 5.) * (2. * scl);

    float4x4 translate = math::makeTranslate(math::add(objPos, {x, y, z}));

    p_instanceData[i].instanceTransform = fullObjRot * translate * yrot * zrot * scale;
    p_instanceData[i].instanceNormalTransform = math::discardTranslation(p_instanceData[i].instanceTransform);

    float iDivNumInstances = i / 1000.;
    float r = iDivNumInstances;
    float g = 1. - r;
    float b = sinf(3.141592 * 2. * iDivNumInstances);
    p_instanceData[i].instanceColor = (float4){ r, g, b, 1. };

    ix += 1;
  }
  p_instanceData_buf->didModifyRange(Range::Make(0, p_instanceData_buf->length()));

  MTL::Buffer* p_cameraData_buf = pCameraBuf[frame_];
  shader_t::CameraData* p_cameraData = reinterpret_cast<shader_t::CameraData *>(p_cameraData_buf->contents());
  p_cameraData->perspTransform = math::makePerspective(45. * 3.141592 / 180., 1., .03, 500.);
  p_cameraData->worldTransform = math::makeIdentity();
  p_cameraData->worldNormalTransform = math::discardTranslation(p_cameraData->worldTransform);
  p_cameraData_buf->didModifyRange(Range::Make(0, sizeof(shader_t::CameraData)));

  generateMandelbrotTexture(p_cmd_buf);

  MTL::RenderPassDescriptor* p_rpd = pView->currentRenderPassDescriptor();
  MTL::RenderCommandEncoder* p_rce = p_cmd_buf->renderCommandEncoder(p_rpd);

  p_rce->setRenderPipelineState(p_rps_);
  p_rce->setDepthStencilState(p_dss_);

  p_rce->setVertexBuffer(pVertexBuf, 0, 0);
  p_rce->setVertexBuffer(p_instanceData_buf, 0, 1);
  p_rce->setVertexBuffer(p_cameraData_buf, 0, 2);

  p_rce->setFragmentTexture(p_texture_, 0);

  p_rce->setCullMode(MTL::CullModeBack);
  p_rce->setFrontFacingWinding(MTL::WindingCounterClockwise);

  p_rce->drawIndexedPrimitives(
    MTL::PrimitiveTypeTriangle,
    6*6,
    MTL::IndexType::IndexTypeUInt16,
    pIndexBuf,
    0,
    1000 );
  ;

  p_rce->endEncoding();
  p_cmd_buf->presentDrawable(pView->currentDrawable());
  p_cmd_buf->commit();

  pARPool->release();
}