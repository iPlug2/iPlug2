# IPlugWebUI
A basic volume control effect plug-in which uses a platform web view to host an HTML/CSS GUI

### NOTES

This is somewhat experimental, especially on Windows.

On Windows you will have to have downloaded and installed the latest edge chromium and you will have to right click the Visual Studio Solution/individual projects and install the nuget packages for ICoreWebView2 and the Windows Implementation Library. Then you will have to edit some hard-coded paths In IPlugWebUI.cpp, since your plugin needs to be able to load the WebView2Loader.dll and it needs to have a temporary folder that it can write to. This does not set that up for you, so if you wanted to use this in a way that would work on computers other than your own, you would need to add some logic to the installer to find a dedicated path to install WebView2Loader.dll and for the temp dir and the web root folder. On macOS and iOS the web root folder is copied into the application bundle, but on Windows you need to pass a hardcoded path.