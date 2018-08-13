#include "IGraphicsWeb.h"
#include <cstring>
#include <cstdio>


using namespace emscripten;

extern IGraphics* gGraphics;

IGraphicsWeb::IGraphicsWeb(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  val keys = val::global("Object").call<val>("keys", GetPreloadedImages());
  
  DBGMSG("Preloaded %i images\n", keys["length"].as<int>());
}

IGraphicsWeb::~IGraphicsWeb()
{
}

void IGraphicsWeb::HideMouseCursor(bool hide, bool returnToStartPos)
{
  if(hide)
    val::global("document")["body"]["style"].set("cursor", std::string("none"));
  else
    val::global("document")["body"]["style"].set("cursor", std::string("auto"));
}

bool IGraphicsWeb::OSFindResource(const char* name, const char* type, WDL_String& result)
{
  if (CStringHasContents(name))
  {
    WDL_String plusSlash;
    plusSlash.SetFormatted(strlen(name) + 1, "/%s", name);
    
    bool foundImage = GetPreloadedImages().call<bool>("hasOwnProperty", std::string(plusSlash.Get()));

    DBGMSG("found image %s %i\n", plusSlash.Get(), foundImage);
    
    if(foundImage)
    {
      result.Set(plusSlash.Get());
      return true;
    }
  }
  return false;
}

//static
void IGraphicsWeb::OnMainLoopTimer()
{
  IRECT r;
  
  if (gGraphics->IsDirty(r))
    gGraphics->Draw(r);
}

bool IGraphicsWeb::GetTextFromClipboard(WDL_String& str)
{
  val clipboardText = val::global("window")["clipboardData"].call<val>("getData", std::string("Text"));
  
  str.Set(clipboardText.as<std::string>().c_str());

  return true; // TODO: return?
}

#define MB_OK 0
#define MB_OKCANCEL 1
#define MB_YESNOCANCEL 3
#define MB_YESNO 4
#define MB_RETRYCANCEL 5

int IGraphicsWeb::ShowMessageBox(const char* str, const char* caption, int type)
{
  
  switch (type)
  {
    case MB_OK:
        val::global("window").call<val>("alert", std::string(str));
      break;
    case MB_YESNO:
        val::global("window").call<val>("confirm", std::string(str));
       break;
    // case MB_CANCEL:
    //   break;
    default:
      break;
  }

  return 0; // TODO: return value?
}

IPopupMenu* IGraphicsWeb::CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller)
{
  ReleaseMouseCapture();
  
  if(mPopupControl)
    return mPopupControl->CreatePopupMenu(menu, bounds, pCaller);
  else
  {
    //TODO: implement select box
    return nullptr;
  }
}

bool IGraphicsWeb::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  val::global("window").call<val>("open", std::string(url), std::string("_blank"));
  
  return true;
}

#if defined IGRAPHICS_CANVAS
#include "IGraphicsCanvas.cpp"
#elif defined IGRAPHICS_NANOVG
#include "IGraphicsNanoVG.cpp"
#endif
