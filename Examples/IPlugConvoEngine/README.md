# IPlugConvoEngine

iPlug2 WDL ConvoEngine example, based on IPlug convoengine example by Theo Niessink.

It can use
  * [r8brain](https://github.com/avaneev/r8brain-free-src)
  * WDL_Resampler
  * or linear interpolation

you change that behaviour by setting USE_WDL_RESAMPLER or USE_R8BRAIN as a preprocessor macro

r8brain source should be in the subdolder r8brain, and you need to add *r8bbase.cpp* to the targets you want to compile



```
IPlug convoengine example
(c) Theo Niessink 2010-2015
  
<http://www.taletn.com/>
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software in a
   product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
A simple IPlug plug-in effect that shows how to use WDL's fast convolution
engine.
```
