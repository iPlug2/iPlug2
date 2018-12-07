As of October 2018, Steinberg no longer allows the distribution of VST2 plug-ins unless you have a signed agreement. For those developers who did obtain a signed agreement before October 2018, and who still have a copy of the VST2 SDK (or VST3 SDK < 3.6.11,  which contained the relevant header files), read on,  otherwise you best think about other plug-in formats...

put *aeffect.h* and *aeffectx.h* from the VST2.4 SDK here

these are the files you need to put in this folder

`/VST2_SDK/pluginterfaces/vst2.x/aeffect.h`  
`/VST2_SDK/pluginterfaces/vst2.x/aeffectx.h`  

so it looks like:
  
`Dependencies/IPlug/VST2_SDK/`  
`Dependencies/IPlug/VST2_SDK/aeffect.h`  
`Dependencies/IPlug/VST2_SDK/aeffectx.h`  