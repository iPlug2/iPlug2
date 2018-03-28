#include "IGraphicsWeb.h"

IGraphicsWeb::IGraphicsWeb(IDelegate& dlg, int w, int h, int fps)
: IGraphicsPathBase(dlg, w, h, fps)
{
  DBGMSG("Hello IGraphics!\n");
}

IGraphicsWeb::~IGraphicsWeb()
{
}
