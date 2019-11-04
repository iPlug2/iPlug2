# Adding Images

1. Place `png` (other formats may not be supported, e.g. `jpg`) in `MyPlugin/resources/img/MyPng.png`
2. Add filename as macro in `config.h`, e.g.
```
#define MYPNG_FN "MyPng.png" 
```
3. For windows edit the `MyPlugin/resources/main.rc` file in a text editor. add e.g.
```
3 TEXTINCLUDE
BEGIN
    "#include ""..\\config.h""\r\n"
    "ROBOTO_FN TTF ROBOTO_FN\0"
    "MYPNG_FN PNG MYPNG_FN\0"
END 
```
4. Load Bitmap in Layout Func (MyPlugin.cpp constructor)
```
const IBitmap bitmap1 = pGraphics->LoadBitmap(MYPNG_FN, 1/* num frames*/);
```
