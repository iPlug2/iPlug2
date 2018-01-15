#pragma once

#include "IControl.h"

class IGraphicsLiveEdit : public IControl
{
public:
  IGraphicsLiveEdit(IPlugBaseGraphics& plug, const char* pathToSourceFile, int gridSize)
  : IControl(plug, IRECT(0, 0, 300, 300))
  , mPathToSourceFile(pathToSourceFile)
  , mGridSize(gridSize)
  {
    mTargetRECT = mRECT;
    mBlend.mWeight = 0.2f;
  }

  ~IGraphicsLiveEdit() {}

  void OnMouseDown(int x, int y, const IMouseMod& mod) override;
  void OnMouseUp(int x, int y, const IMouseMod& mod) override;
  void OnMouseDblClick(int x, int y, const IMouseMod& mod) override;
  void OnMouseOver(int x, int y, const IMouseMod& mod) override;
  void OnMouseDrag(int x, int y, int dX, int dY, const IMouseMod& mod) override;
  
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

  int mMouseDownX = 0;
  int mMouseDownY = 0;
  
  int mGridSize = 10;
  int mClickedOnControl = -1;
};
