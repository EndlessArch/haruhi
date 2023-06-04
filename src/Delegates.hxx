#ifndef HARUHI_DELEGATES_HPP
#define HARUHI_DELEGATES_HPP

// #include <mtl.hpp>
#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "GameEngine.hxx"
#include "MemoryUtility.hxx"

class HaruhiRenderer;
class HaruhiViewDelegate;

class HaruhiDelegate : public NS::ApplicationDelegate {
  NS::Window* p_win_;
  MTK::View* p_mtkView_;
  MTL::Device* p_device_;
  HaruhiViewDelegate* viewDelegate_;

  Haruhi* engine_;
  
public:
  HaruhiDelegate();
  virtual ~HaruhiDelegate();

  NS::Menu* createMenu();
  void applicationWillFinishLaunching(NS::Notification*) override;
  void applicationDidFinishLaunching(NS::Notification*) override;
  bool applicationShouldTerminateAfterLastWindowClosed(NS::Application*) override {
    return true;
  }
};

class HaruhiViewDelegate : public MTK::ViewDelegate {
  HaruhiRenderer* p_renderer_;
public:
  HaruhiViewDelegate(Haruhi*, MTL::Device*);
  virtual ~HaruhiViewDelegate();
  void drawInMTKView(MTK::View*) override;
};

#endif