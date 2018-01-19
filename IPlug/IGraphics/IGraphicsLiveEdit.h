#pragma once
#ifndef NDEBUG

#include "IControl.h"

class IGraphicsLiveEdit : public IControl
{
public:
  IGraphicsLiveEdit(IPlugBaseGraphics& plug, const char* pathToSourceFile, int gridSize);
  ~IGraphicsLiveEdit() {}

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  
  void Draw(IGraphics& graphics) override;
  bool IsDirty() override { return true; }

  inline IRECT GetHandleRect(IRECT& r)
  {
    return IRECT(r.R - RESIZE_HANDLE_SIZE, r.B - RESIZE_HANDLE_SIZE, r.R, r.B);
  }

  inline int SnapToGrid(int input)
  {
    if (mGridSize > 1)
      return (input / mGridSize) * mGridSize;
    else
      return input;
  }

private:
  bool mEditModeActive = false;
  bool mLiveEditingEnabled = false;
  bool mMouseClickedOnResizeHandle = false;
  bool mMouseIsDragging = false;
  WDL_String mPathToSourceFile;
  WDL_String mErrorMessage;

  IColor mGridColor = COLOR_GRAY;
  IColor mRectColor = COLOR_WHITE;
  static const int RESIZE_HANDLE_SIZE = 10;

  IRECT mMouseDownRECT = IRECT(0, 0, 0, 0);
  IRECT mMouseDownTargetRECT = IRECT(0, 0, 0, 0);

  float mMouseDownX = 0;
  float mMouseDownY = 0;
  
  int mGridSize = 10;
  int mClickedOnControl = -1;
};

#endif // !NDEBUG
