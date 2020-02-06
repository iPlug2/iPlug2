#pragma once

#include "Yoga.h"
#include "IGraphicsStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** IFlexBox is a basic C++ helper for Yoga https://yogalayout.com. 
 * For advanced use, probably best just to use Yoga directly */
class IFlexBox
{
public:
  IFlexBox();
  
  ~IFlexBox();
  
  /** Initialize the IFlexBox flex container
   * @param r The IRECT bounds for the flex container
   * @param direction https://yogalayout.com/docs/flex-direction
   * @param justify https://yogalayout.com/docs/justify-content
   * @param wrap https://yogalayout.com/docs/flex-wrap
   * @param padding https://yogalayout.com/docs/margins-paddings-borders
   * @param margin https://yogalayout.com/docs/margins-paddings-borders */
  void Init(const IRECT& r, YGFlexDirection direction = YGFlexDirectionRow, YGJustify justify = YGJustifyFlexStart, YGWrap wrap = YGWrapNoWrap, float padding = 0.f, float margin = 0.f);
  
  /** Add a flex item, with some parameters
   * @param width Postitive value sets width in pixels, negative value sets percentage or YGUndefined for "auto" https://yogalayout.com/docs/width-height
   * @param height Postitive value sets height In pixels, negative value sets percentage or YGUndefined for "auto" https://yogalayout.com/docs/width-height
   * @param alignSelf https://yogalayout.com/docs/align-items
   * @param grow https://yogalayout.com/docs/flex
   * @param shrink https://yogalayout.com/docs/flex
   * @param margin https://yogalayout.com/docs/margins-paddings-borders
   * @return YGNodeRef The newly added YGNodeRef for the item (owned by this class) */
  YGNodeRef AddItem(float width, float height, YGAlign alignSelf = YGAlignAuto, float grow = 0.f, float shrink = 1.f, float margin = 0.f);
  
  /** Add a flex item manually
   * @param item A new YGNodeRef to add (owndership transferred) */
  void AddItem(YGNodeRef item);
  
  /** Calculate the layout, call after add all items
   * @param direction https://yogalayout.com/docs/layout-direction */
  void CalcLayout(YGDirection direction = YGDirectionLTR);
  
  /** Get an IRECT of the root node bounds */
  IRECT GetRootBounds() const;

  /** Get the bounds for a particular flex item */
  IRECT GetItemBounds(int nodeIndex) const;
  
private:
  int mNodeCounter = 0;
  YGConfigRef mConfigRef;
  YGNodeRef mRootNodeRef;
};

END_IPLUG_NAMESPACE
END_IGRAPHICS_NAMESPACE
