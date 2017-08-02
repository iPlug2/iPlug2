#ifndef __IPLUGDIGIVIEW__
#define __IPLUGDIGIVIEW__

#include "CNoResourceView.h"
#include "EditorInterface.h"

class IPlugDigiView : public CCustomView
{
public:
  IPlugDigiView() {}
  virtual ~IPlugDigiView() {}

  virtual void SetCustomUI(EditorInterface *customUI) { mCustomUI = customUI; }

  virtual void DrawContents(Rect* drawRect)
  {
    if(mCustomUI)
      mCustomUI->Draw(drawRect->left, drawRect->top, drawRect->right, drawRect->bottom);
  }

//  virtual void DrawBackground(Rect* drawRect) {}

protected:
  EditorInterface    *mCustomUI;        // pointer to UI interface

private:

};

#endif // __IPLUGDIGIVIEW__
