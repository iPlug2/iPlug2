# Quickstart

@todo lots more to do here!



## Adding/modifying image resources

One of the most common things people post about on the WDL Forum is how to add an image resource. Perhaps they tried and got an assertion when they try and run the plug-in or app. Something like:

```C++
Assertion failed: (imgResourceFound), function LoadIBitmap...
```

This basically means that the resource has not been found. Perhaps it didn't get included in the bundle (OSX) or baked into the binary (WIN) or perhaps it is there but was not referenced correctly in the source code.

***

### Here are some instructions about how to add an image:

Let's assume our WDL-OL IPlug Project is called _MyNewPlugin_ and the image we want to add is called _cat.png_.

The IPlug Project should exist at the same level as _WDL-OL/IPlugExamples/IPlugEffect_. Personally I put all my IPlug projects in a folder called _WDL-OL/Projects_ (which is ignored in the .gitignore file), so if I were to make a new project I would first use [the duplicate script](@todo) to clone the IPlugEffect example and then I would move that folder to _WDL-OL/Projects/MyNewPlugin_. If you don't yet know about how to do this, it is explained in the @todo file.


* Firstly, make sure your image is in PNG format. 
  By default WDL-OL IPlug only works with PNG files.

* Copy the PNG file to the IPlug Project's image resource folder e.g.

  _WDL-OL/Projects/MyNewPlugin/Resources/img/cat.png_

* Now add a reference to the file location to _WDL-OL/Projects/MyNewPlugin/config.h_

```C++ 
// Image resource locations for this plug.
#define KNOB_FN "resources/img/knob.png"
#define CAT_FN "resources/img/cat.png" 
```

the following steps differ depending on which platform/IDE you are using

* **Xcode:** Open _WDL-OL/Projects/MyNewPlugin/MyNewPlugin.xcodeproj_ and drag cat.png into the folder _Resources/img/_   inside the Xcode project. Xcode will give you a dialog asking which targets you want to add the resource to.    Usually you would tick all of them except the one called "All" which is an Aggregate target for building all formats at once. You can now verify that your resource will be added to the plug-in/app bundle by checking the target's "Copy Bundle Resources" build-phase.

* **Visual Studio:** Edit _WDL-OL/Projects/MyNewPlugin/MyNewPlugin.rc_ @todo

* Now you have included the resource in your build, you can actually use it in the source code. In the plug-in's constructor, after creating the graphics context you can call:

```C++
IBitmap cat = pGraphics->LoadIBitmap(CAT_FN, 1 /*num frames*/);
```

In order to be able to use cat.png in an IControl that uses a bitmap, e.g. an IBitmapControl.

***

### Things that could go wrong:

* On macOS every bundle has a Info.plist file and when image resources are loaded the IGraphics/Lice image loading routines need to have the correct bundle identifier in order to find the file. The elements that make up the bundle identifier are in _config.h_ and are concatenated together in IPlug_include_in_plug_hdr.h. Therefore if you decide to modify BUNDLE_MFR or BUNDLE_NAME in _config.h_ it is of utmost importance that you also edit the .plist file for the targets to mirror those changes, otherwise the bundle ID will not match.