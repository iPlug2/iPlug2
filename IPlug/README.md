# WDL-OL IPlug style/coding guidelines

to try and tidy up the IPlug code base

- “IGraphics” should be optional so you can use the nice and lean IPlugBase and IPlugVST/AU/etc with whatever UI
- “IGraphics” is not abandoned, in fact once again (as it was when I first took on maintenance) it becomes an interface, with IPlugLice inheriting from IGraphics (providing the current UI rendering)
- I have IGraphicsAGG,  coded by someone else,  but I think it's probably too slow (although it looks lovely)
- I have IGraphicsCairo, which needs implementing but should be very quick since quite a lot of work has already been done on IPlug Cairo (just not a very nice way)
- what I really want to try is to do the graphics with NanoVG, this might be possible with an IGraphicsNanoVG, but I expect it might be better to not base that on IGraphics
- many things have been adapted for C++11, const correctness and taking references rather than pointers where that makes sense
- possibly split IPlugBase into IPlugController and IPlugProcessor interfaces, for making real VST3s and better for WAMs
- I want to prioritise the size of the binary (at least for the DSP part), this is for building WAMs, where WASM binary size is important
- rework app wrapper
- separate IPlug from WDL, so that it could be rewritten with the STL, rather than Cockos’ wdlstring etc (although I do like the small footprint of WDL code)
- make it possible 2 have WDL as a git submodule of plug-in project, instead of plug-in projects referencing the same parent WDL folder (I think this is better for CI)
- CMake support out of the box
- seamless retina support
- try and keep it simple to use for beginners (I think iPlug beats JUCE in this respect)


// indents using 2 spaces - no tabs
// C++11 stuff: override keyword, final keyword, auto where sensible
// noexcept keyword?
// currently does not use the STL, unless API classes depend on it. WDL_String is used rather than std::string
// IPlug should be able to produce very small binaries, and not include large dependencies or unnecessary code

class MyClass
{
public:
  MyClass()
  {

  }

  ~MyClass()
  {
  }

  /** Doxygen description like this. cstring arguments don't need prefix pMyCStringVariable, but other pointer arguments should be prefixed with p and camel case */
  void MyFunction(const char* str, IControl* pControl, WDL_String& myWDLStringVariable) const
  {
    const double myVarible = 0.5; // should we always bother specifying const here if we know myVariable won't change?
  }

private:
  IControl* mControl; // member variables have prefix m camel case
}
