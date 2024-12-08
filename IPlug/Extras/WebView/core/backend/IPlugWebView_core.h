 /*
 ==============================================================================
 
  MIT License

  iPlug2 WebView Core
  Copyright (c) 2024 Aid Vllasaliu
  GitHub(s): https://github.com/aidv, https://github.com/superkraft-io/
  As a contribution by splitter.ai

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 
 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"
#include "IPlugLogger.h"
#include "wdlstring.h"
#include <functional>
#include <memory>

#include "IPlugWebView_ipc.h"

BEGIN_IPLUG_NAMESPACE

class IPlugWebView_Core
{
public:
  IPlugWebView_IPC ipc;
private:
};

END_IPLUG_NAMESPACE
