On Windows the location of VST2 plugins is not specified by the VST2 SDK. A registry key tells VST2 hosts where to find them. 

NOTE: these locations assume 64 bit windows.

* VSTPluginsPath at HKEY_LOCAL_MACHINE\SOFTWARE\VST\ speficies the path for 64 bit vst2 dlls
* VSTPluginsPath at HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\VST speficies the path for 32 bit vst2 dlls

If you don't have the registry keys allready you can use these .reg files to set them. Double click them to add the paths to the Windows registry. It will set the paths to ```C:\\Program Files\VstPlugins\``` and ```C:\Program Files (x86)\VstPlugins\``` respectively.

