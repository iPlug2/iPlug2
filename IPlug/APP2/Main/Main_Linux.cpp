/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#ifdef OS_LINUX

#include "IPlugAPP2_Host.h"
#include "IPlugSWELL.h"
#include "config.h"

using namespace iplug;

HWND gHWND = nullptr;
HINSTANCE gHINSTANCE = nullptr;

/**
 * Linux application entry point
 *
 * Uses SWELL for cross-platform window/dialog compatibility.
 */
int main(int argc, char* argv[])
{
  // Initialize SWELL
  SWELL_initargs(&argc, &argv);

  // Create and initialize host
  IPlugAPP2Host* pHost = IPlugAPP2Host::Create();
  if (!pHost)
  {
    fprintf(stderr, "Failed to create application host\n");
    return 1;
  }

  if (!pHost->Init())
  {
    fprintf(stderr, "Failed to initialize application\n");
    return 1;
  }

  // TODO: Create main window using SWELL

  // Message loop
  // SWELL provides a compatible message loop for Linux

  // Cleanup
  pHost->StopAudio();
  pHost->CloseWindow();

  return 0;
}

#endif // OS_LINUX
