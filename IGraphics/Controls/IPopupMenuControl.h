#pragma once

#include "IControl.h"

/** A base control for a pop-up menu/drop-down list that stays within the bounds of the IGraphics context
 * On the web and on iOS, we don't have traditional contextual menus. This class provides a way of making a menu
 * that works across all platforms */
class IPopupMenuControl : public IControl
{
public:
  enum EPopupState
  {
    kCollapsed = 0,
    kExpanding = 1,
    kExpanded = 2,
    kCollapsing = 3,
  };
  
  /** @param dlg The editor delegate that this control is attached to
   * @param collapsedBounds If this control, when collapsed should occupy an area of the graphics context, specify this, otherwise the collapsed area is empty
   * @param expandedBounds If you want to explicitly specify the size of the expanded pop-up, you can specify an area here */
  IPopupMenuControl(IGEditorDelegate& dlg, int paramIdx = kNoParameter, IText text = DEFAULT_TEXT, IRECT collapsedBounds = IRECT(), IRECT expandedBounds = IRECT(), EDirection direction = kVertical);
  virtual ~IPopupMenuControl() {}
  
  //IControl
  virtual bool IsDirty() override
  {
    return (GetState() > kCollapsed) | IControl::IsDirty();
  }
  
  void Draw(IGraphics& g) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override;
  
  void OnEndAnimation() override;
  
  //IPopupMenuControl
  virtual void DrawBackground(IGraphics& g, const IRECT& bounds);
  virtual void DrawShadow(IGraphics& g, const IRECT& bounds);
  virtual void DrawCell(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem);
  virtual void DrawHighlightCell(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem);
  virtual void DrawCellText(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem);
  virtual void DrawHighlightCellText(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem);
  virtual void DrawSeparator(IGraphics& g, const IRECT& bounds);
  
  /** Call this to create a pop-up menu
   @param menu Reference to a menu from which to populate this user interface control. NOTE: this object should not be a temporary, otherwise when the menu returns asynchronously, it may not exist.
   @param pCaller The IControl that called this method, and will receive the call back after menu selection
   @return the menu */
  IPopupMenu* CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller);
  
  EPopupState GetState() const { return mState; }
  bool GetExpanded() const { return mState == kExpanded; }
private:
  
  /** This method is called to expand the modal pop-up menu. It calculates the dimensions and wrapping, to keep the cells within the graphics context. It handles the dirtying of the graphics context, and modification of graphics behaviours such as tooltips and mouse cursor */
  void Expand();
  
  /** This method is called to collapse the modal pop-up menu and make it invisible. It handles the dirtying of the graphics context, and modification of graphics behaviours such as tooltips and mouse cursor */
  virtual void Collapse();
  
  float CellWidth() const { return mSingleCellBounds.W(); }
  float CellHeight() const { return mSingleCellBounds.H(); }
  
  /** Checks if any of the expanded cells contain a x, y coordinate, and if so returns an IRECT pointer to the cell bounds
   * @param x X position to test
   * @param y Y position to test
   * @return Pointer to the cell bounds IRECT, or nullptr if nothing got hit */
  IRECT* HitTestCells(float x, float y) const
  {
    for(auto i = 0; i < mExpandedCellBounds.GetSize(); i++)
    {
      IRECT* r = mExpandedCellBounds.Get(i);
      if(r->Contains(x, y))
      {
        return r;
      }
    }
    return nullptr;
  }
  
protected:
  IRECT mSpecifiedCollapsedBounds;
  IRECT mSpecifiedExpandedBounds;
private:
  EPopupState mState = kCollapsed;
  EDirection mDirection;
  IRECT mSingleCellBounds; // The dimensions are the dimensions of 1 cell.
  WDL_PtrList<IRECT> mExpandedCellBounds; // The size of this array will always correspond to the number of items in the top level of the menu
  IRECT* mMouseCellBounds = nullptr;
  IControl* mCaller = nullptr;
  IPopupMenu* mMenu = nullptr; // This control does not own the menu
  float mCellGap = 2.f; // the gap between cells
  int mMaxColumnItems = 0; // how long the list can get before adding a new column - 0 equals no limit
  int mMaxRowItems = 0; // how long the list can get before adding a new row - 0 equals no limit
  float mSeparatorSize = 2.; // The size in pixels of a separator. This could be width or height
  bool mScrollIfTooBig = false;
  const float TEXT_PAD = 5.; // 5px on either side of text
  float mPadding = 5.; // How much white space between the background and the cells
  IBlend mBlend = { kBlendNone, 0.f };
  float mRoundness = 5.f;
  float mDropShadowSize = 10.f;
  float mOpacity = 0.95f;
};
