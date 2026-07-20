#include "IPlugOpenGL.h"
#include "IPlug_include_in_plug_src.h"

#include "pugl/attributes.h"

#include "pugl/gl.hpp"
#include "pugl/pugl.hpp"
#include "cube_view.h"
#include "demo_utils.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "Dwmapi.lib")

class CubeView : public pugl::View
{
public:
  explicit CubeView(pugl::World& world)
    : pugl::View{world}
  {
    setEventHandler(*this);
  }

  template<PuglEventType t, class Base>
  pugl::Status onEvent(const pugl::Event<t, Base>&) noexcept
  {
    return pugl::Status::success;
  }

  static pugl::Status onEvent(const pugl::ConfigureEvent& event) noexcept
  {
    reshapeCube(static_cast<float>(event.width), static_cast<float>(event.height));

    return pugl::Status::success;
  }

  pugl::Status onEvent(const pugl::UpdateEvent& event) noexcept
  {
    return postRedisplay();
  }

  pugl::Status onEvent(const pugl::ExposeEvent& event) noexcept
  {
    const double thisTime = world().time();
    const double dTime = thisTime - _lastDrawTime;
    const double dAngle = dTime * 100.0;

    _xAngle = fmod(_xAngle + dAngle, 360.0);
    _yAngle = fmod(_yAngle + dAngle, 360.0);
    displayCube(cobj(), 8.0f, static_cast<float>(_xAngle), static_cast<float>(_yAngle), false);

    _lastDrawTime = thisTime;

    return pugl::Status::success;
  }

  pugl::Status onEvent(const pugl::KeyPressEvent& event) noexcept
  {
    if (event.key == PUGL_KEY_ESCAPE || event.key == 'q')
    {
      _quit = true;
    }

    return pugl::Status::success;
  }

  pugl::Status onEvent(const pugl::CloseEvent& event) noexcept
  {
    _quit = true;

    return pugl::Status::success;
  }

  bool quit() const { return _quit; }

private:
  double _xAngle{0.0};
  double _yAngle{0.0};
  double _lastDrawTime{0.0};
  bool   _quit{false};
};


IPlugOpenGL::IPlugOpenGL(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
}

void IPlugOpenGL::OnIdle()
{
  if (mWorld)
    mWorld->update(0.0);
}

void* IPlugOpenGL::OpenWindow(void* pParent)
{
  mWorld = std::make_unique<pugl::World>(pugl::WorldType::module);
  mView = std::make_unique<CubeView>(*mWorld);

  mWorld->setString(pugl::StringHint::className, "PuglCppDemo");

  mView->setString(pugl::StringHint::windowTitle, "Pugl C++ Demo");
  mView->setSizeHint(pugl::SizeHint::defaultSize, 512, 512);
  mView->setSizeHint(pugl::SizeHint::minSize, 64, 64);
  //mView->setSizeHint(pugl::SizeHint::maxSize, 1024, 1024);
  mView->setSizeHint(pugl::SizeHint::minAspect, 1, 1);
  mView->setSizeHint(pugl::SizeHint::maxAspect, 16, 9);
  mView->setBackend(pugl::glBackend());
//  mView->setHint(pugl::ViewHint::resizable, true);
//  mView->setHint(pugl::ViewHint::refreshRate, 60);
//  mView->setHint(pugl::ViewHint::doubleBuffer, true);
//  mView->setHint(pugl::ViewHint::swapInterval, true);
  //mView->setHint(pugl::ViewHint::ignoreKeyRepeat, opts.ignoreKeyRepeat);

  mView->setParentWindow(reinterpret_cast<pugl::NativeView>(pParent));
  mView->realize();
  mView->show(pugl::ShowCommand::passive);

  OnUIOpen();

  return reinterpret_cast<void*>(mView->nativeView());
}

void IPlugOpenGL::OnParentWindowResize(int width, int height)
{
  if (mView)
  {
    mView->setFrame({0, 0, static_cast<PuglSpan>(width * 2), static_cast<PuglSpan>(height * 2)});
  }
}

void IPlugOpenGL::CloseWindow()
{
  mView = nullptr;
  mWorld = nullptr;
  
  IEditorDelegate::CloseWindow();
}

void IPlugOpenGL::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
