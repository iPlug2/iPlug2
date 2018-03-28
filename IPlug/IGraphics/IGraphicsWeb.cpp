#include "IGraphicsWeb.h"

using namespace emscripten;

IGraphicsWeb::IGraphicsWeb(IDelegate& dlg, int w, int h, int fps)
: IGraphicsPathBase(dlg, w, h, fps)
{
  DBGMSG("Hello IGraphics!\n");
  
  val doc = val::global("Document");
  val canvas = doc.call<val>("getElementById('canvas')");
  val ctx = canvas.call<val>("getContext('2d')");
  ctx.set("fillStyle", val("red"));
  ctx.call<val>("fillRect(10, 10, 100, 100");
}

IGraphicsWeb::~IGraphicsWeb()
{
}
