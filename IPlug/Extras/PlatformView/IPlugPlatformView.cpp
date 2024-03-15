/*
==============================================================================

This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

See LICENSE.txt for  more info.

==============================================================================
*/

#include "IPlugPlatformView.h"
#include "IPlugPaths.h"
#include <string>
#include <windows.h>
#include <shlobj.h>
#include <cassert>

using namespace iplug;

IPlatformView::IPlatformView(bool opaque)
{
}

IPlatformView::~IPlatformView()
{
  RemovePlatformView();
}


void* IPlatformView::CreatePlatformView(void* pParent, float x, float y, float w, float h, float scale)
{
  HWND hWnd = (HWND) pParent;

  x *= scale;
  y *= scale;
  w *= scale;
  h *= scale;

  // TODO

  return nullptr;
}

void IPlatformView::RemovePlatformView() { mPlatformView = nullptr; }

void IPlatformView::SetChildViewBounds(float x, float y, float w, float h, float scale)
{
  //if (mPlatformViewCtrlr)
  //{
  //  x *= scale;
  //  y *= scale;
  //  w *= scale;
  //  h *= scale;

  //}
}
