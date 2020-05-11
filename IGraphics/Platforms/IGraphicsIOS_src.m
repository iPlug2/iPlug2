/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

#include "MSColorComponentView.m"
#include "MSColorSelectionViewController.m"
#include "MSColorWheelView.m"
#include "MSRGBView.m"
#include "MSThumbView.m"
#include "MSColorSelectionView.m"
#include "MSColorUtils.m"
#include "MSHSBView.m"
#include "MSSliderView.m"
#include "UIControl+HitTestEdgeInsets.m"
