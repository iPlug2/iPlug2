// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "cube_view.h"
#include "demo_utils.h"
#include "test/test_utils.h"

#include "pugl/gl.hpp"
#include "pugl/pugl.h"
#include "pugl/pugl.hpp"

#include <cmath>

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

  static pugl::Status onEvent(const pugl::ConfigureEvent& event) noexcept;
  pugl::Status        onEvent(const pugl::UpdateEvent& event) noexcept;
  pugl::Status        onEvent(const pugl::ExposeEvent& event) noexcept;
  pugl::Status        onEvent(const pugl::KeyPressEvent& event) noexcept;
  pugl::Status        onEvent(const pugl::CloseEvent& event) noexcept;

  bool quit() const { return _quit; }

private:
  double _xAngle{0.0};
  double _yAngle{0.0};
  double _lastDrawTime{0.0};
  bool   _quit{false};
};

pugl::Status
CubeView::onEvent(const pugl::ConfigureEvent& event) noexcept
{
  reshapeCube(static_cast<float>(event.width),
              static_cast<float>(event.height));

  return pugl::Status::success;
}

pugl::Status
CubeView::onEvent(const pugl::UpdateEvent&) noexcept
{
  // Normally, we would post a redisplay:
  // return postRedisplay();

  // But for testing, use sendEvent() instead:
  return sendEvent(pugl::ExposeEvent{
    0U, PuglCoord{0}, PuglCoord{0}, frame().width, frame().height});
}

pugl::Status
CubeView::onEvent(const pugl::ExposeEvent&) noexcept
{
  const double thisTime = world().time();
  const double dTime    = thisTime - _lastDrawTime;
  const double dAngle   = dTime * 100.0;

  _xAngle = fmod(_xAngle + dAngle, 360.0);
  _yAngle = fmod(_yAngle + dAngle, 360.0);
  displayCube(cobj(),
              8.0f,
              static_cast<float>(_xAngle),
              static_cast<float>(_yAngle),
              false);

  _lastDrawTime = thisTime;

  return pugl::Status::success;
}

pugl::Status
CubeView::onEvent(const pugl::KeyPressEvent& event) noexcept
{
  if (event.key == PUGL_KEY_ESCAPE || event.key == 'q') {
    _quit = true;
  }

  return pugl::Status::success;
}

pugl::Status
CubeView::onEvent(const pugl::CloseEvent&) noexcept
{
  _quit = true;

  return pugl::Status::success;
}

int
main(int argc, char** argv)
{
  const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);
  if (opts.help) {
    puglPrintTestUsage("pugl_cpp_demo", "");
    return 1;
  }

  pugl::World    world{pugl::WorldType::program};
  CubeView       view{world};
  PuglFpsPrinter fpsPrinter{};

  world.setString(pugl::StringHint::className, "PuglCppDemo");

  view.setString(pugl::StringHint::windowTitle, "Pugl C++ Demo");
  view.setSizeHint(pugl::SizeHint::defaultSize, 512, 512);
  view.setSizeHint(pugl::SizeHint::minSize, 64, 64);
  view.setSizeHint(pugl::SizeHint::maxSize, 1024, 1024);
  view.setSizeHint(pugl::SizeHint::minAspect, 1, 1);
  view.setSizeHint(pugl::SizeHint::maxAspect, 16, 9);
  view.setBackend(pugl::glBackend());
  view.setHint(pugl::ViewHint::resizable, opts.resizable);
  view.setHint(pugl::ViewHint::samples, opts.samples);
  view.setHint(pugl::ViewHint::doubleBuffer, opts.doubleBuffer);
  view.setHint(pugl::ViewHint::swapInterval, opts.sync);
  view.setHint(pugl::ViewHint::ignoreKeyRepeat, opts.ignoreKeyRepeat);
  view.realize();
  view.show(pugl::ShowCommand::passive);

  unsigned framesDrawn = 0;
  while (!view.quit()) {
    world.update(0.0);

    ++framesDrawn;
    puglPrintFps(world.cobj(), &fpsPrinter, &framesDrawn);
  }

  return 0;
}
