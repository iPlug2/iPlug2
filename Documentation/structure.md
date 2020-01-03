## Structure of an iPlug2 project

TODO: explain

| FILE                                      |  WHAT IS IT?                                                                                                       |
|-------------------------------------------|--------------------------------------------------------------------------------------------------------------------|                                                                          |
| installer/changelog.txt                   | "List of changes in this version of the plugin, packaged in installer"                                             |
| installer/intro.rtf                       | intro text for macOS installer                                                                                     |
| installer/IPlugEffect-installer-bg.png    | background png for macOS installer                                                                                 |
| installer/IPlugEffect.iss                 | Innosetup file for WIN installer                                                                                   |
| installer/IPlugEffect.packproj            | Iceberg file for macOS installer                                                                                   |
| installer/license.rtf                     | license text for macOS and WIN installer                                                                           |
| installer/readmemacOS.rtf                 | macOS installer readme page                                                                                        |
| installer/readmewin.rtf                   | WIN installer readme page                                                                                          |
| IPlugEffect-app.props                     | Visual Studio 2019 property sheet to provide settings that are shared between standalone x86/x64                   |
| IPlugEffect-app.vcxproj                   | Visual Studio 2019 project for standalone                                                                          |
| IPlugEffect-app.vcxproj.filters           | VS2019 settings file (don't edit manually)                                                                         |
| IPlugEffect-app.vcxproj.user              | VS2019 settings file (don't edit manually)                                                                         |
| IPlugEffect-vst2.props                    | Visual Studio 2019 property sheet to provide settings that are shared between VST2 x86/x64                         |
| IPlugEffect-vst2.vcxproj                  | Visual Studio 2019 project for VST2                                                                                |
| IPlugEffect-vst2.vcxproj.user             | VS2019 settings file (don't edit manually)                                                                         |
| IPlugEffect-vst3.props                    | Visual Studio 2019 property sheet to provide settings that are shared between VST3 x86/x64                         |
| IPlugEffect-vst3.vcxproj                  | Visual Studio 2019 project for VST3                                                                                |
| IPlugEffect-vst3.vcxproj.filters          | VS2019 settings file (don't edit manually)                                                                         |
| IPlugEffect-vst3.vcxproj.user             | VS2019 settings file (don't edit manually)                                                                         |
| IPlugEffect.cpp                           | Source code - plugin's implementation                                                                              |
| IPlugEffect.exp                           | macOS auv2 symbol exports file                                                                                     |
| IPlugEffect.h                             | Source code - plugin's interface                                                                                   |
| IPlugEffect.rc                            | WIN .rc resource file                                                                                              |
| IPlugEffect.sln                           | "Visual Studio 2019 solution for WIN VST2, VST3 and standalone builds"                                             |
| IPlugEffect.xcconfig                      | Xcode xcconfig file to provide settings that span various targets (includes ../../common-mac.xcconfig)             | 
| IPlugEffect.xcodeproj                     | main xcode project (3.2) for all macOS builds                                                                      |
| makedist-mac.command                      | macOS script to build all binaries and package them in an installer with accompanying files                        |
| makedist-win.bat                          | WIN script to build all binaries and package them in an installer with accompanying files                          |
| manual/IPlugEffect_manual.pdf             | dummy pdf manual that will be included by the installer scripts                                                    |
| resource.h                                | Source code - plugin's characteristices                                                                            |
| resources/English.lproj/                  | interface builder folder for macOS standalone menu                                                                 |
| resources/English.lproj/InfoPlist.strings | interface builder file for macOS standalone menu                                                                   |
| resources/English.lproj/MainMenu.xib      | interface builder file for macOS standalone menu                                                                   |
| resources/img/knob.png                    | plugin gui image resource                                                                                          |
| resources/IPlugEffect-AU-Info.plist       | info.plist for au                                                                                                  |
| resources/IPlugEffect-macOS-Info.plist    | info.plist for macOS app                                                                                           |
| resources/IPlugEffect-VST2-Info.plist     | info.plist for vst2                                                                                                |
| resources/IPlugEffect-VST3-Info.plist     | info.plist for vst3                                                                                                |
| resources/IPlugEffect.icns                | macOS icon for all bundles .vst/vst3/.component/.app/.dpm                                                          |
| resources/IPlugEffect.ico                 | WIN icon for standalone                                                                                            |
| update_version.py                         | python script to update the version numbers in info.plist andwin/macOS installer files to match that in resource.h |
| validate_audiounit.command                | bash script to run macOS auval utility with plugin TYPE/MFR_ID/PLUGIN_ID as declared in resource.h                 |

-->