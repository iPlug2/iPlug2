download the latest VST3 SDK:

[http://www.steinberg.net/en/company/developer.html
](http://www.steinberg.net/en/company/developer.html
)

extract the zip file here preserving the folder structure so it looks like:

`WDL-OL/`  
`WDL-OL/VST3_SDK/`  
`WDL-OL/VST3_SDK/base/source`  
`WDL-OL/VST3_SDK/base/thread`  
`WDL-OL/VST3_SDK/pluginterfaces`  
`WDL-OL/VST3_SDK/public.sdk/source`  

Once you've put the SDK files in place, you should discard any changes in these two files via git:

`WDL-OL/VST3_SDK/base/mac/base.xcodeproj`  
`WDL-OL/VST3_SDK/base/win/base.vcxproj`  