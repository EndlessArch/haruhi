#include "LoadResource.hxx"

// #include <mtl.hpp>
#include <MetalKit/MetalKit.hpp>

namespace HaruhiResourceLoader {

namespace ImageUtil {

std::tuple<size_t, std::unique_ptr<spng_ihdr>, void*>
load_image_at(const char * img_pth) noexcept {

  spng_ctx* p_ctx = spng_ctx_new(0);
  assert(p_ctx);

  auto block_texture = fopen("./resource/blocks.png", "r");

  spng_set_png_file(p_ctx, block_texture);

  size_t img_sz;
  spng_decoded_image_size(p_ctx, SPNG_FMT_RGB8, &img_sz);

  printf("img size: %lu\n", img_sz);

  void* img_buf = malloc(img_sz);
  spng_decode_image(p_ctx, img_buf, img_sz, SPNG_FMT_RGB8, 0);

  std::unique_ptr<spng_ihdr> ihdr = {};
  spng_get_ihdr(p_ctx, &*ihdr);

  spng_ctx_free(p_ctx);

  return std::make_tuple(img_sz, std::move(ihdr), img_buf);
}

} // ns ImageUtil

void
loadResources(MTL::Device* pDevice) noexcept {
  using namespace NS;

  MTK::TextureLoader* pTexLoader = MTK::TextureLoader::alloc()->init(pDevice);

  Error* err = nullptr;

  auto pTex = 
    pTexLoader->newTexture(
      URL::fileURLWithPath(
        String::alloc()->string("./resource/blocks.png", UTF8StringEncoding)),
      nullptr, &err);
  ;

  printf("%lu\n", pTex->buffer()->length());

  return;
}

} // ns HaruhiResourceLoader