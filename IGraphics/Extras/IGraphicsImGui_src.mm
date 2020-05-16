/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

/**
* @file Include this in your macOS/iOS target, making sure to enable ARC for this specific file
**/

#if defined IGRAPHICS_IMGUI
#include "imgui_impl_metal.mm"
#include "IGraphicsImGui.cpp"
#endif
