# WDL-OL IPlug style/coding guidelines

to try and tidy up the IPlug code base

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
