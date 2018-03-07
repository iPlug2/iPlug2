# Code Style

These guidelines exist in order to try and tidy up the IPlug code base. Every developer has their own preferences, here we are stuck with some legacy choices that may not be the best/most modern approaches, but are used widely. We want to aim for consistency across the code base. 

* Indentation uses 2 spaces - no tabs  

* Files should be unix format line endings, unless they are windows specific

* C++11 stuff: `override` keyword, `final` keyword, `auto` where sensible

* Do not use the STL, unless API classes depend on it. `WDL_String` is used rather than `std::string`, setters that require string arguments take `const char*` UTF8 cstrings

* IPlug should be able to produce very small binaries, and not include large dependencies or unnecessary code
* Doxygen description uses JAVADOC brief style. 
* Variables are declared with camel case with the first letter non-capitalised
* Pointer arguments and local variables should be prefixed with the letter p e.g. ``pCamelCase``
* Member variables prefixed with the letter m e.g. ``mControl``
* Member variables that are pointers do not need a "p"
* Member methods that should not be called by the "IPlug User", but are still accessible, for whatever reason prefixed with an underscore

\todo Expand this

###An example class:

```cpp
class MyClass
{
public:
  MyClass()
  {
  }

  ~MyClass()
  {
  }

  void MyFunction(const char* str, IControl* pControl, WDL_String& fileName) const
  {
    const double myVarible = 0.5; // const where relevant
    
    //single line if statements should look like this
    if (0.5 > 0.4)
      printf("hello world\n");
      
  }

private:
  IControl* mControl; // member variables have prefix mCamelCase
}
```
