extract the VST3.6.8 SDK here (https://download.steinberg.net/sdk_downloads/vstsdk368_08_11_2017_build_121.zip, preserving the folder structure. i.e

WDL-OL/VST3_SDK/base/source
WDL-OL/VST3_SDK/pluginterfaces
WDL-OL/VST3_SDK/public.sdk/source

Once you've put the SDK files in place, you should discard any changes in these two files via git

WDL-OL/VST3_SDK/base/mac/base.xcodeproj
WDL-OL/VST3_SDK/base/win/base.vcxproj