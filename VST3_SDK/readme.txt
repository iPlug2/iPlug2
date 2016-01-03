extract the VST3.6.5 SDK here (vstsdk365_12_11_2015_build_67.zip, preserving the folder structure. i.e

WDL-OL/VST3_SDK/base/source
WDL-OL/VST3_SDK/pluginterfaces
WDL-OL/VST3_SDK/public.sdk/source

Once you've put the SDK files in place, you should discard changes in these two files via git…

WDL-OL/VST3_SDK/base/mac/base.xcodeproj
WDL-OL/VST3_SDK/base/win/base_vc10.vcxproj