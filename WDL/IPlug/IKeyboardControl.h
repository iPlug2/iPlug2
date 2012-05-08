#ifndef _IKEYBOARDCONTROL_
#define _IKEYBOARDCONTROL_

/*

IKeyboardControl
(c) Theo Niessink 2009, 2010
<http://www.taletn.com/>


This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software in a
   product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.


IKeyboardControl is a (musical) keyboard for IPlug instruments. The keyboard
starts and ends at C. Only depressed keys are drawn by this control, so the
entire keyboard (with all its keys released) should already be visible (e.g.
because it's in the background).

By default pRegularKeys should contain 6 bitmaps (C/F, D, E/B, G, A, high
C), while pSharpKey should only contain 1 bitmap (for all flat/sharp keys).
If you want to use more bitmaps, e.g. if the 1st octave is alternatively
coloured, then you should override the DrawKey() method.

pKeyCoords should contain the x-coordinates of each key relative to the
start of the octave. (Note that only the coordinates for the flat/sharp keys
are actually used, the coordinates for the "regular" keys are ignored.)

Here is code snippet defining a 4-octave keyboard starting at MIDI note 48
(C3):

IBitmap regular = pGraphics->LoadIBitmap(REGULAR_KEYS_ID, REGULAR_KEYS_PNG, 6);
IBitmap sharp   = pGraphics->LoadIBitmap(SHARP_KEY_ID,    SHARP_KEY_PNG);

//                    C#      D#          F#      G#        A#
int coords[12] = { 0, 13, 23, 39, 46, 69, 82, 92, 107, 115, 131, 138 };

// Store a pointer to the keyboard in member variable IControl* mKeyboard
mKeyboard = new IKeyboardControl(this, x, y, 48, 4, &regular, &sharp, coords);

pGraphics->AttachControl(mKeyboard);

The plug-in should provide the following methods, so the keyboard control
can pull status information from the plug-in, and send MIDI Note On/Off
message to the plug-in:

// Should return non-zero if one or more keys are playing.
int MyPlug::GetNumKeys()
{
  IMutexLock lock(this);
  return mNumKeys;
}

// Should return true if the specified key is playing.
bool MyPlug::GetKeyStatus(int key)
{
  IMutexLock lock(this);
  return mKeyStatus[key];
}

(Instead of int you can also use any other integer types for the
GetNumKeys() and GetKeyStatus() methods, i.e. "char GetNumKeys()" or
"bool GetKeyStatus(char note)" will also work.)

When the keyboard should be redrawn, e.g. when the plug-in has received a
MIDI Note On/Off message, the plug-in should call mKeyboard->SetDirty().

You should include this header file after your plug-in class has already
been declared, so it is propbably best to include it in your plug-in's main
.cpp file, e.g.:

#include "MyPlug.h"
#include "WDL/IPlug/IKeyboardControl.h" // Include after MyPlug.h

*/

class IKeyboardControl: public IControl
{
public:
  IKeyboardControl(IPlugBase* pPlug, int x, int y, int minNote, int nOctaves, IBitmap* pRegularKeys, IBitmap* pSharpKey, const int *pKeyCoords = 0):
    IControl(pPlug, IRECT(x, y, pRegularKeys), -1),
    mMinNote(minNote), mNumOctaves(nOctaves), mRegularKeys(*pRegularKeys), mSharpKey(*pSharpKey),
    mOctaveWidth(pRegularKeys->W * 7), mMaxKey(nOctaves * 12), mKey(-1)
  {
    memcpy(mKeyCoords, pKeyCoords, 12 * sizeof(int));
    mRECT.R += nOctaves * mOctaveWidth;
    mTargetRECT.R = mRECT.R;

    mDblAsSingleClick = true;
  }

  ~IKeyboardControl() {}

  // Returns the key currently playing, or -1 if no key is playing.
  inline int GetKey() const {return mKey; }
  // Returns the Note On velocity of the key currently playing.
  inline int GetVelocity() const { return 1 + int(mVelocity * 126. + 0.5); }
  // Returns the velocity as a floating point value.
  inline double GetReal() const { return mVelocity; }

  virtual void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (pMod->R) return;

    // Skip if this key is already being played using the mouse.
    int key = GetMouseKey(x, y);
    if (key == mKey) return;

    if (((PLUG_CLASS_NAME*)mPlug)->GetKeyStatus(key + mMinNote)) return;

    mKey = key;

    //Update the keyboard in the GUI.
    SetDirty();
  }

  virtual void OnMouseUp(int x, int y, IMouseMod* pMod)
  {
    // Skip if no key is playing.
    if (mKey < 0) return;
    mKey = -1;

    // Update the keyboard in the GUI.
    SetDirty();
  }

  virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
  {
    //if(y < mTargetRECT.T || y >= mTargetRECT.B);
    OnMouseDown(x, y, pMod);
  }

  virtual void OnMouseWheel(int x, int y, IMouseMod* pMod, int d) {}

  // Draws only the keys that are currently playing.
  virtual bool Draw(IGraphics* pGraphics)
  {
    // Skip if no keys are playing.
    if (((PLUG_CLASS_NAME*)mPlug)->GetNumKeys() == 0 && mKey == -1) return true;

    // "Regular" keys
    IRECT r(mRECT.L, mRECT.T, mRECT.L + mRegularKeys.W, mRECT.B);
    int key = 0;
    while (key < mMaxKey)
    {
      // Draw the key.
      int note = key % 12;
      if (((PLUG_CLASS_NAME*)mPlug)->GetKeyStatus(key + mMinNote) || key == mKey) DrawKey(pGraphics, &r, key, note, false);

      // Next, please!
      key += mNextKey[note];
      r.L += mRegularKeys.W;
      r.R += mRegularKeys.W;
    }
    // Draw the high C.
    if (((PLUG_CLASS_NAME*)mPlug)->GetKeyStatus(key + mMinNote) || key == mKey) DrawKey(pGraphics, &r, key, 0, false);

    // Flat/sharp keys
    int l = mRECT.L;
    r.B = mRECT.T + mSharpKey.H / mSharpKey.N;
    key = 1;
    while (true)
    {
      // Draw the key.
      int note = key % 12;

      r.L = l + mKeyCoords[note];
      r.R = r.L + mSharpKey.W;
      if (((PLUG_CLASS_NAME*)mPlug)->GetKeyStatus(key + mMinNote) || key == mKey) DrawKey(pGraphics, &r, key, note, true);

      // Next, please!
      key += mNextKey[note];
      if (key > mMaxKey) break;
      if (note == 10) l += mOctaveWidth;
    }

    return true;
  }

protected:
  virtual void DrawKey(IGraphics* pGraphics, IRECT* pR, int key, int note, bool sharp)
  {
    if (sharp)
      pGraphics->DrawBitmap(&mSharpKey, pR, 0, &mBlend);
    else
      pGraphics->DrawBitmap(&mRegularKeys, pR, key < mMaxKey ? mBitmapN[note] : 6, &mBlend);
  }

  /* Override the above method if e.g. you want to draw the 1st octave
  // using different bitmaps:

  virtual void DrawKey(IGraphics* pGraphics, IRECT* pR, int key, int note, bool sharp)
  {
    if (sharp)
    {
      pGraphics->DrawBitmap(&mSharpKey, pR, key < 12 ? 1 : 2, &mBlend);
    }
    else
    {
      int n;
      if (key < mMaxKey)
      {
        n = mBitmapN[note];
        if (key >= 12) n += 5;
      }
      else
      {
        n = 11;
      }
      pGraphics->DrawBitmap(&mRegularKeys, pR, n, &mBlend);
    }
  }

  */

  // Returns the key number at the (x, y) coordinates.
  int GetMouseKey(int x, int y)
  {
    // Skip if the coordinates are outside the keyboard's rectangle.
    if (x < mTargetRECT.L || x >= mTargetRECT.R || y < mTargetRECT.T || y >= mTargetRECT.B) return -1;
    x -= mTargetRECT.L;
    y -= mTargetRECT.T;

    // Calculate the octave.
    int octave = x / mOctaveWidth;
    x -= octave * mOctaveWidth;

    // Flat/sharp key
    int note;
    int h = mSharpKey.H / mSharpKey.N;

    if (y < h && octave < mNumOctaves)
    {
      // C#
      if (x < mKeyCoords[1]) goto RegularKey;
      if (x < mKeyCoords[1] + mSharpKey.W)
      {
        note = 1;
        goto CalcVelocity;
      }
      // D#
      if (x < mKeyCoords[3]) goto RegularKey;
      if (x < mKeyCoords[3] + mSharpKey.W)
      {
        note = 3;
        goto CalcVelocity;
      }
      // F#
      if (x < mKeyCoords[6]) goto RegularKey;
      if (x < mKeyCoords[6] + mSharpKey.W)
      {
        note = 6;
        goto CalcVelocity;
      }
      // G#
      if (x < mKeyCoords[8]) goto RegularKey;
      if (x < mKeyCoords[8] + mSharpKey.W)
      {
        note = 8;
        goto CalcVelocity;
      }
      // A#
      if (x < mKeyCoords[10]) goto RegularKey;
      if (x < mKeyCoords[10] + mSharpKey.W)
      {
        note = 10;
        goto CalcVelocity;
      }
    }

  RegularKey:
    {
      h = mRECT.H();
      int n = x / mRegularKeys.W;
      note = n * 2;
      if (n >= 3) note--;
    }

  CalcVelocity:
    // Calculate the velocity depeding on the vertical coordinate
    // relative to the key height.
    mVelocity = (double)y / (double)h;

    return note + octave * 12;
  }

  static const int mNextKey[12];
  static const int mBitmapN[12];

  IBitmap mRegularKeys, mSharpKey;
  int mKeyCoords[12];
  int mOctaveWidth, mNumOctaves;
  int mKey, mMinNote, mMaxKey;
  double mVelocity;
};


// Next "regular" or sharp/flat note relative to the current note.
const int IKeyboardControl::mNextKey[12] = { 2, 2, 2, 3, 1, 2, 2, 2, 2, 2, 3, 1 };

// The bitmap index number for each note.
const int IKeyboardControl::mBitmapN[12] = { 1, 0, 2, 0, 3, 1, 0, 4, 0, 5, 0, 3 };


#endif // _IKEYBOARDCONTROL_
