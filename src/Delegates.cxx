#include "Delegates.hxx"

#include "Renderer.hxx"

using namespace NS;

Menu*
HaruhiDelegate::createMenu() {
  using StringEncoding::UTF8StringEncoding;

  Menu* mainMenu = Menu::alloc()->init();

  MenuItem* appMenuItem = MenuItem::alloc()->init();
  Menu* appMenu = 
    Menu::alloc()->init(String::string("Appname", UTF8StringEncoding));
  
  String* appName = RunningApplication::currentApplication()->localizedName();
  String* quitItemName =
    NS::String::string("Quit", UTF8StringEncoding)->stringByAppendingString(appName);

  SEL quitCbFn =
    MenuItem::registerActionCallback("appQuit",
      [](void *, SEL, const Object* sender) {
        auto app = Application::sharedApplication();
        app->terminate(sender);
      });
  ;

  MenuItem* appQuitItem =
    appMenu->addItem(quitItemName, quitCbFn, String::string("q", UTF8StringEncoding));

  appQuitItem->setKeyEquivalentModifierMask(EventModifierFlagCommand);
  appMenuItem->setSubmenu(appMenu);

  MenuItem* winMenuItem = MenuItem::alloc()->init();
  Menu* winMenu = Menu::alloc()->init(String::string("Window", UTF8StringEncoding));

  SEL closeWinCbFn =
    MenuItem::registerActionCallback("windowClose",
      [](void *, SEL, const Object* sender) {
        auto app = Application::sharedApplication();
        app->windows()->object<Window>(0)->close();
      });
  ;
  MenuItem* closeWinItem =
    winMenu->addItem(String::string("Close Window", UTF8StringEncoding),
    closeWinCbFn,
    String::string("w", UTF8StringEncoding));
  closeWinItem->setKeyEquivalentModifierMask(EventModifierFlagCommand);

  winMenuItem->setSubmenu(winMenu);

  mainMenu->addItem(appMenuItem);
  mainMenu->addItem(winMenuItem);

  std::initializer_list<Object*> to_release
    { appMenuItem, winMenuItem, appMenu, winMenu };
  std::for_each(to_release.begin(), to_release.end(), [](Object* p){ p->release(); });

  return mainMenu->autorelease();
}

void
HaruhiDelegate::applicationWillFinishLaunching(NS::Notification * pNot) {

  Menu* menu = createMenu();
  Application* app = reinterpret_cast<Application*>(pNot->object());

  app->setMainMenu(menu);
  app->setActivationPolicy(NS::ActivationPolicyRegular);
}

void
HaruhiDelegate::applicationDidFinishLaunching(NS::Notification * pNot) {
  CGRect frame = (CGRect){{100., 100.}, {1024., 1024.}};

  p_win_ = Window::alloc()->init(
    frame,
    WindowStyleMaskBorderless,
    BackingStoreBuffered,
    false );
  ;

  p_device_ = MTL::CreateSystemDefaultDevice();

  p_mtkView_ = MTK::View::alloc()->init(frame, p_device_);
  p_mtkView_->setColorPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
  p_mtkView_->setClearColor(MTL::ClearColor::Make(.1, .1, .1, 1.));
  p_mtkView_->setDepthStencilPixelFormat(MTL::PixelFormatDepth16Unorm);
  p_mtkView_->setClearDepth(1.);

  viewDelegate_ = new HaruhiViewDelegate(p_device_);
  p_mtkView_->setDelegate(viewDelegate_);

  p_win_->setContentView(p_mtkView_);
  p_win_->setTitle(String::string("Welcome Haruhi", NS::UTF8StringEncoding));

  p_win_->makeKeyAndOrderFront(nullptr);

  Application* app = reinterpret_cast<Application*>(pNot->object());
  app->activateIgnoringOtherApps(true);
}

HaruhiViewDelegate::HaruhiViewDelegate(MTL::Device* pDevice)
: p_renderer_(new HaruhiRenderer(pDevice)) {
  ;
}

HaruhiViewDelegate::~HaruhiViewDelegate() {
  delete p_renderer_;
}

void
HaruhiViewDelegate::drawInMTKView(MTK::View *pView) {
  p_renderer_->draw(pView);
}