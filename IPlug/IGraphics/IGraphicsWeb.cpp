#include "IGraphicsWeb.h"
#include <string>

using namespace emscripten;

IGraphicsWeb::IGraphicsWeb(IDelegate& dlg, int w, int h, int fps)
: IGraphicsPathBase(dlg, w, h, fps)
{
  DBGMSG("HELLO IGraphics!\n");

//  val document = val::global("document");
//  document["body"]["style"].set("backgroundColor", "SteelBlue");
//
//  val canvas = val::global("document").call<val>("getElementById", std::string("canvas"));
//  val ctx = canvas.call<val>("getContext", std::string("2d"));
//  ctx.set("fillStyle", std::string("red"));
//  ctx.call<val>("fillRect", ;
}

IGraphicsWeb::~IGraphicsWeb()
{
}
