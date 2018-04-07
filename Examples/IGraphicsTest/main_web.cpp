#include <emscripten.h>
#include "IGraphicsTest.h"

IGraphicsTest gIGraphicsTest;

void tick();

int main()
{
  gIGraphicsTest.GetUI()->Resize(980, 580, 1);
//   gIGraphicsTest.GetUI()->Draw(gIGraphicsTest.GetUI()->GetBounds());

#ifdef __EMSCRIPTEN__
  // void emscripten_set_main_loop(em_callback_func func, int fps, int simulate_infinite_loop);
  emscripten_set_main_loop(tick, UI_FPS, 1);
#else
  while (1) {
    tick();
  }
#endif

  return 0;
}

void tick()
{

}