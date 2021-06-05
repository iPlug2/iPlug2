<!--
# Quick Start

\todo Lots more to do here!

## Adding/modifying resources

One of the most common things people post about on the WDL Forum is how to add a resource, such as an image. Perhaps they got an assertion when they tried to run the plug-in or app. Something like:

```cpp
Assertion failed: (resourceFound), function LoadIBitmap...
```

This basically means that the resource has not been found. Perhaps it didn't get included in the bundle (OSX) or baked into the binary (WIN) or perhaps it is there but was not referenced correctly in the source code.

***

### Here are some instructions about how to add an image:

Let's assume our IPlug 2 Project is called _MyNewPlugin_ and the image we want to add is called _cat.png_.

The IPlug Project should exist at the same level as _iPlug2/Examples/IPlugEffect_. Personally I put all my IPlug projects in a folder called _iPlug2/Projects_ (which is ignored in the .gitignore file), so if I were to make a new project I would first use the [duplicate script](md_duplicate.html) to clone the IPlugEffect example and then I would move that folder to _iPlug2/Projects/MyNewPlugin_.


* Firstly, make sure your image is in PNG format. 
  By default iPlug2 only works with PNG files.

* Copy the PNG file to the IPlug Project's image resource folder, e.g.

  _iPlug2/Projects/MyNewPlugin/Resources/img/cat.png_

* Now add a reference to the file location to _iPlug2/Projects/MyNewPlugin/config.h_

```cpp
// Image resource locations for this plug.  
#define KNOB_FN "knob.png"  
#define CAT_FN "cat.png" 
```

The following steps differ depending on which platform/IDE you are using:

**Xcode**

Open _iPlug2/Projects/MyNewPlugin/MyNewPlugin.xcodeproj_ and drag cat.png into the folder _Resources/img/_   inside the Xcode project. Xcode will give you a dialog asking which targets you want to add the resource to.    Usually you would tick all of them except the one called "All" which is an Aggregate target for building all formats at once. You can now verify that your resource will be added to the plug-in/app bundle by checking the target's "Copy Bundle Resources" build-phase.

**Visual Studio**

Edit _iPlug2/Projects/MyNewPlugin/MyNewPlugin.rc_
\todo Complete the guide for Visual Studio

Now you have included the resource in your build, you can actually use it in the source code. In the plug-in's constructor, after creating the graphics context you can call:

```cpp
IBitmap cat = pGraphics->LoadIBitmap(CAT_FN, 1 /*num frames*/);
```

In order to be able to use cat.png in an IControl that uses a bitmap, e.g. an IBitmapControl.

***

### Things that could go wrong:

* On macOS every bundle has an Info.plist file, and when image resources are loaded, the IGraphics/Lice image loading routines need to have the correct bundle identifier in order to find the file. The elements that make up the bundle identifier are in _config.h_ and are concatenated together in IPlug_include_in_plug_hdr.h. Therefore if you decide to modify `BUNDLE_MFR` or `BUNDLE_NAME` in _config.h_ it is of utmost importance that you also edit the .plist file for the targets to mirror those changes, otherwise the bundle ID will not match.

-->
