#include <emscripten.h>
#include "IGraphicsTest.h"

IGraphicsTest* gIGraphicsTest = nullptr;

void tick();

int main()
{
  gIGraphicsTest = new IGraphicsTest();
  gIGraphicsTest->GetUI()->Resize(400, 400, 1);
//   gIGraphicsTest->GetUI()->Draw(gIGraphicsTest->GetUI()->GetBounds());

#ifdef __EMSCRIPTEN__
  // void emscripten_set_main_loop(em_callback_func func, int fps, int simulate_infinite_loop);
  emscripten_set_main_loop(tick, UI_FPS, 1);
#else
  while (1) {
    tick();
  }
#endif

  delete gIGraphicsTest;

  return 0;
}

void tick()
{
  IRECT r;

  if ( gIGraphicsTest->GetUI()->IsDirty(r))
     gIGraphicsTest->GetUI()->Draw(r);
}