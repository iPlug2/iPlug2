#include "IGraphicsFlexBox.h"

using namespace iplug;
using namespace igraphics;

IFlexBox::IFlexBox()
{
  mConfigRef = YGConfigNew();
  mRootNodeRef = YGNodeNewWithConfig(mConfigRef);
}

IFlexBox::~IFlexBox()
{
  YGNodeFreeRecursive(mRootNodeRef);
  YGConfigFree(mConfigRef);
}

void IFlexBox::Init(const IRECT& r, YGFlexDirection direction, YGJustify justify, YGWrap wrap, float padding, float margin)
{
  YGNodeStyleSetWidth(mRootNodeRef, r.W());
  YGNodeStyleSetHeight(mRootNodeRef, r.H());
  YGNodeStyleSetFlexDirection(mRootNodeRef, direction);
  YGNodeStyleSetJustifyContent(mRootNodeRef, justify);
  YGNodeStyleSetFlexWrap(mRootNodeRef, wrap);
  YGNodeStyleSetPadding(mRootNodeRef, YGEdgeAll, padding);
  YGNodeStyleSetMargin(mRootNodeRef, YGEdgeAll, margin);
}

void IFlexBox::CalcLayout(YGDirection direction)
{
  YGNodeCalculateLayout(mRootNodeRef, YGUndefined, YGUndefined, direction);
}

YGNodeRef IFlexBox::AddItem(float width, float height, YGAlign alignSelf, float grow, float shrink, float margin)
{
  int index = mNodeCounter;
  YGNodeRef child = YGNodeNew();
  
  if(width == YGUndefined)
    YGNodeStyleSetWidthAuto(child);
  else if(width < 0.f)
    YGNodeStyleSetWidthPercent(child, width * -1.f);
  else
    YGNodeStyleSetWidth(child, width);
  
  if(height == YGUndefined)
    YGNodeStyleSetHeightAuto(child);
  else if(height < 0.f)
    YGNodeStyleSetHeightPercent(child, height * -1.f);
  else
    YGNodeStyleSetHeight(child, height);
  
  YGNodeStyleSetAlignSelf(child, alignSelf);
  YGNodeStyleSetMargin(child, YGEdgeAll, margin);
  YGNodeStyleSetFlexGrow(child, grow);
  YGNodeStyleSetFlexShrink(child, shrink);
  YGNodeInsertChild(mRootNodeRef, child, index);
  
  mNodeCounter++;
  
  return child;
}

void IFlexBox::AddItem(YGNodeRef child)
{
  YGNodeInsertChild(mRootNodeRef, child, mNodeCounter++);
}

IRECT IFlexBox::GetRootBounds() const
{
  return IRECT(YGNodeLayoutGetLeft(mRootNodeRef),
               YGNodeLayoutGetTop(mRootNodeRef),
               YGNodeLayoutGetLeft(mRootNodeRef) + YGNodeLayoutGetWidth(mRootNodeRef),
               YGNodeLayoutGetTop(mRootNodeRef)  + YGNodeLayoutGetHeight(mRootNodeRef));
}

IRECT IFlexBox::GetItemBounds(int nodeIndex) const
{
  YGNodeRef child = YGNodeGetChild(mRootNodeRef, nodeIndex);
  return IRECT(YGNodeLayoutGetLeft(mRootNodeRef) + YGNodeLayoutGetLeft(child),
               YGNodeLayoutGetTop(mRootNodeRef)  + YGNodeLayoutGetTop(child),
               YGNodeLayoutGetLeft(mRootNodeRef) + YGNodeLayoutGetLeft(child) + YGNodeLayoutGetWidth(child),
               YGNodeLayoutGetTop(mRootNodeRef)  + YGNodeLayoutGetTop(child)  + YGNodeLayoutGetHeight(child));
};

// TODO: eventually build Yoga as a static library,
// for now include Yoga .cpp files here
#include "YGLayout.cpp"
#include "YGEnums.cpp"
#include "YGNodePrint.cpp"
#include "YGValue.cpp"
#include "YGConfig.cpp"
#include "YGNode.cpp"
#include "YGStyle.cpp"
#include "Yoga.cpp"
#include "Utils.cpp"
#include "log.cpp"
#include "event/event.cpp"
