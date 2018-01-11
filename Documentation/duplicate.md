## Duplicating Projects

The IPlugExamples folder contains a python script to duplicate an IPlug project. This allows you to very quickly create a new project based on one of the examples. It does a multiple file find and replace to substitute the new name of the project for the old name, and also to change the manufacturer name. Once you have done this you only need to change two more things by hand in *resource.h* to make your plugin unique.

You can duplicate a project as follows with the following commands in the OSX terminal or on the windows command prompt. In this example i will copy the IPlugEffect project to a new project called MyNewPlugin

- open terminal or cmd.exe and navigate to the IPlugExamples folder
- type `duplicate.py [inputprojectname] [outputprojectname] [manufacturername]`, e.g `duplicate.py IPlugEffect MyNewPlugin OliLarkin`

you might need to do `./duplicate.py` on OSX

- open `MyNewPlugin/config.h` and change *PLUG_UNIQUE_ID* and *PLUG_MFR_ID*