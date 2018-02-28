## Duplicating Projects

The IPlugExamples folder contains a python script to duplicate an IPlug project. This allows you to very quickly create a new project based on one of the examples. It does a multiple file find and replace to substitute the new name of the project for the old name, and also to change the manufacturer name. Once you have done this you only need to change two more things by hand in **resource.h** to make your plugin unique.

You can duplicate a project as follows with the following commands in the macOS terminal or on the windows command prompt. In this example i will copy the IPlugEffect project to a new project called MyNewPlugin

###macOS
- open terminal and navigate to the Examples folder
- type `python duplicate.py [inputprojectname] [outputprojectname] [manufacturername]`, e.g `duplicate.py IPlugEffect MyNewPlugin OliLarkin`

###Windows
- make sure you have python 2.7+ installed (should work with python 3)
- open cmd.exe and navigate to the Examples folder (perhaps the easiest thing to do is to hold SHIFT and right click in the explorer window to select "Open command window here...")
- type `python duplicate.py [inputprojectname] [outputprojectname] [manufacturername]`, e.g `duplicate.py IPlugEffect MyNewPlugin OliLarkin`

--

You MUST open `MyNewPlugin/config.h` and change ```PLUG_UNIQUE_ID``` and ```PLUG_MFR_ID```  otherwise you may have conflicting plug-ins