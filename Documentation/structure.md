## Structure of an iPlug2 project

TODO: explain

| File                                      | Description                                                                                            |
| ----------------------------------------- | ------------------------------------------------------------------------------------------------------ |
| IPlugEffect-app.props                     | Visual Studio 2022 property sheet to provide settings that are shared between standalone x86/x64       |
| IPlugEffect-app.vcxproj                   | VS2022 project for standalone                                                                          |
| IPlugEffect-app.vcxproj.filters           | VS2022 settings file (don't edit manually)                                                             |
| IPlugEffect-app.vcxproj.user              | VS2022 settings file (don't edit manually)                                                             |
| IPlugEffect-vst2.props                    | VS2022 property sheet to provide settings that are shared between VST2 x86/x64                         |
| IPlugEffect-vst2.vcxproj                  | VS2022 project for VST2                                                                                |
| IPlugEffect-vst2.vcxproj.user             | VS2022 settings file (don't edit manually)                                                             |
| IPlugEffect-vst3.props                    | VS2022 property sheet to provide settings that are shared between VST3 x86/x64                         |
| IPlugEffect-vst3.vcxproj                  | VS2022 project for VST3                                                                                |
| IPlugEffect-vst3.vcxproj.filters          | VS2022 settings file (don't edit manually)                                                             |
| IPlugEffect-vst3.vcxproj.user             | VS2022 settings file (don't edit manually)                                                             |
| IPlugEffect.cpp                           | Source code - plugin's implementation                                                                  |
| IPlugEffect.exp                           | macOS auv2 symbol exports file                                                                         |
| IPlugEffect.h                             | Source code - plugin's interface                                                                       |
| IPlugEffect.rc                            | Windows .rc resource file                                                                              |
| IPlugEffect.sln                           | VS2022 solution for Windows VST2, VST3 and standalone builds                                           |
| IPlugEffect.xcconfig                      | Xcode xcconfig file to provide settings that span various targets (includes ../../common-mac.xcconfig) |
| IPlugEffect.xcodeproj                     | Main xcode project (3.2) for all macOS builds                                                          |
| resource.h                                | Source code - plugin's characteristices                                                                |
| resources/English.lproj/                  | Interface Builder folder for macOS standalone menu                                                     |
| resources/English.lproj/InfoPlist.strings | Interface Builder file for macOS standalone menu                                                       |
| resources/English.lproj/MainMenu.xib      | Interface Builder file for macOS standalone menu                                                       |
| resources/img/knob.png                    | Plugin GUI image resource                                                                              |
| resources/IPlugEffect-AU-Info.plist       | info.plist for AU                                                                                      |
| resources/IPlugEffect-macOS-Info.plist    | info.plist for macOS app                                                                               |
| resources/IPlugEffect-VST2-Info.plist     | info.plist for VST2                                                                                    |
| resources/IPlugEffect-VST3-Info.plist     | info.plist for VST3                                                                                    |
| resources/IPlugEffect.icns                | macOS icon for all bundles .vst/vst3/.component/.app/.dpm                                              |
| resources/IPlugEffect.ico                 | Windows icon for standalone                                                                            |
