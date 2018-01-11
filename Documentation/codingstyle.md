# Coding style guidelines

These guidelines exist in order to try and tidy up the IPlug code base. Every developer has their own preferences, here we are stuck with some legacy choices that may not be the best/most modern approaches, but are used widely. We want to aim for consistency across the code base. 

* indentation uses 2 spaces - no tabs  
* files should be unix format line endings, unless they are windows specific
* C++11 stuff: override keyword, final keyword, auto where sensible  
* do not use the STL, unless API classes depend on it. WDL_String is used rather than std::string, setters that require string arguments take const char* UTF8 cstrings.  
* IPlug should be able to produce very small binaries, and not include large dependencies or unnecessary code  
* @todo

###An example class:

```C++
class MyClass
{
public:
  MyClass()
  {
  }

  ~MyClass()
  {
  }

  /** Doxygen description like this. Using JAVADOC brief style. 
  cstring args don't need prefix pCString, 
  but other pointer arguments should be prefixed with pCamelCase */
  void MyFunction(const char* str, IControl* pControl, WDL_String& fileName) const
  {
    const double myVarible = 0.5; // const where relevant
  }

private:
  IControl* mControl; // member variables have prefix mCamelCase
}
```
