#ifndef HARUHI_DELEGATES_HPP
#define HARUHI_DELEGATES_HPP

#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#include "MemoryUtility.hxx"

class HaruhiRenderer;
class HaruhiViewDelegate;

class HaruhiDelegate : public NS::ApplicationDelegate {
  NS::Window* p_win_;
  MTK::View* p_mtkView_;
  MTL::Device* p_device_;
  HaruhiViewDelegate* viewDelegate_;
  
public:
  virtual ~HaruhiDelegate() = default;

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
  HaruhiViewDelegate(MTL::Device*);
  virtual ~HaruhiViewDelegate();
  void drawInMTKView(MTK::View*) override;
};

#endif