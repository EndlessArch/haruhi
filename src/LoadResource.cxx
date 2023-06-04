#include "LoadResource.hxx"

// #include <mtl.hpp>
#include <MetalKit/MetalKit.hpp>

#include "ResourcePool.hxx"

namespace HaruhiResourceLoader {

namespace ImageUtil {

std::tuple<size_t, std::unique_ptr<spng_ihdr>, void*>
load_image_at(const char * img_pth) noexcept {

  spng_ctx* p_ctx = spng_ctx_new(0);
  assert(p_ctx);

  auto block_texture = fopen("./resource/blocks.png", "r");

  spng_set_png_file(p_ctx, block_texture);

  size_t img_sz;
  spng_decoded_image_size(p_ctx, SPNG_FMT_RGBA8, &img_sz);

  void* img_buf = malloc(img_sz);
  spng_decode_image(p_ctx, img_buf, img_sz, SPNG_FMT_RGBA8, 0);

  auto ihdr = std::make_unique<spng_ihdr>((struct spng_ihdr){});
  spng_get_ihdr(p_ctx, ihdr.get());

  spng_ctx_free(p_ctx);

  return std::make_tuple(img_sz, std::move(ihdr), img_buf);
}

} // ns ImageUtil

void
loadResources(MTL::Device* pDevice, HaruhiResourcePool* pResPool) noexcept {
  using namespace NS;

  auto [sz, ihdr, buf] = ImageUtil::load_image_at("./resource/grass.jpeg");

  MTL::TextureDescriptor* pTexDesc =
    MTL::TextureDescriptor::texture2DDescriptor(
      MTL::PixelFormatRGBA8Unorm_sRGB, ihdr->width, ihdr->height, false);
  pTexDesc->setTextureType(MTL::TextureType2D);
  pTexDesc->setUsage(MTL::TextureUsageRenderTarget|MTL::TextureUsageShaderRead);
  pTexDesc->setResourceOptions(MTL::ResourceStorageModeManaged);

  auto tex_buf = pDevice->newBuffer(sz, MTL::ResourceStorageModeManaged);
  memcpy(tex_buf->contents(), buf, sz);
  tex_buf->didModifyRange(Range::Make(0, sz));
  // free(buf);
  // printf("%u %lu\n", ihdr->width, sz);
  auto blocks = tex_buf->newTexture(pTexDesc, (unsigned long long)buf, 4*ihdr->width);

  pResPool->setTexture("blocks", blocks);

  return;
}

} // ns HaruhiResourceLoader