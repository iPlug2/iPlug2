#pragma once

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
colored, then you should override the DrawKey() method.

pKeyCoords should contain the x-coordinates of each key relative to the
start of the octave. (Note that only the coordinates for the flat/sharp keys
are actually used, the coordinates for the "regular" keys are ignored.)

Here is code snippet defining a 4-octave keyboard starting at MIDI note 48
(C3):

IBitmap regular = graphics.LoadBitmap(REGULAR_KEYS_ID, REGULAR_KEYS_PNG, 6);
IBitmap sharp   = graphics.LoadBitmap(SHARP_KEY_ID,    SHARP_KEY_PNG);

//                    C#      D#          F#      G#        A#
int coords[12] = { 0, 13, 23, 39, 46, 69, 82, 92, 107, 115, 131, 138 };

// Store a pointer to the keyboard in member variable IControl* mKeyboard
mKeyboard = new IKeyboardControl(this, x, y, 48, 4, &regular, &sharp, coords);

graphics.AttachControl(mKeyboard);

The plug-in should provide the following methods, so the keyboard control
can pull status information from the plug-in, and send MIDI Note On/Off
message to the plug-in:

// Should return non-zero if one or more keys are playing.
int MyPlug::GetNumKeys()
{
  return mNumKeys;
}

// Should return true if the specified key is playing.
bool MyPlug::GetKeyStatus(int key)
{
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

/** A (musical) keyboard control
*   @ingroup Controls
*/
/*
class IKeyboardControl: public IControl
{
public:
  IKeyboardControl(IPlugBaseGraphics& plug, float x, float y, int minNote, int nOctaves, IBitmap& regularKeys, IBitmap& sharpKey, const int *pKeyCoords = 0):
    IControl(plug, IRECT(x, y, regularKeys), kNoParameter),
    mMinNote(minNote), mNumOctaves(nOctaves), mRegularKeys(regularKeys), mSharpKey(sharpKey),
    mOctaveWidth(regularKeys.W * 7), mMaxKey(nOctaves * 12), mKey(-1)
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

void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mod.R) return;

    // Skip if this key is already being played using the mouse.
    int key = GetMouseKey(x, y);
    if (key == mKey) return;

    if (((PLUG_CLASS_NAME*)mPlug).GetKeyStatus(key + mMinNote)) return;

    mKey = key;

    //Update the keyboard in the GUI.
    SetDirty();
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    // Skip if no key is playing.
    if (mKey < 0) return;
    mKey = -1;

    // Update the keyboard in the GUI.
    SetDirty();
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    //if(y < mTargetRECT.T || y >= mTargetRECT.B);
    OnMouseDown(x, y, mod);
  }

  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override {}

  // Draws only the keys that are currently playing.
  void Draw(IGraphics& graphics) override
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
  }

protected:
  virtual void DrawKey(IGraphics& graphics, IRECT* pR, int key, int note, bool sharp)
  {
    if (sharp)
      graphics.DrawBitmap(mSharpKey, pR, 0, &mBlend);
    else
      graphics.DrawBitmap(mRegularKeys, pR, key < mMaxKey ? mBitmapN[note] : 6, &mBlend);
  }

  // Override the above method if e.g. you want to draw the 1st octave
  // using different bitmaps:

  //virtual void DrawKey(IGraphics& graphics, IRECT* pR, int key, int note, bool sharp)
  //{
  //  if (sharp)
  //  {
  //    graphics.DrawBitmap(mSharpKey, pR, key < 12 ? 1 : 2, &mBlend);
  //  }
  //  else
  //  {
  //    int n;
  //    if (key < mMaxKey)
  //    {
  //      n = mBitmapN[note];
  //      if (key >= 12) n += 5;
  //    }
  //    else
  //    {
  //      n = 11;
  //    }
  //    graphics.DrawBitmap(mRegularKeys, pR, n, &mBlend);
  //  }
  //}



  // Returns the key number at the (x, y) coordinates.
  int GetMouseKey(float x, float y)
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
*/



/*

IVKeyboardControl by Eugene Yakshin, 2018

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


This keyboard is runtime customizable. Any key range is supported.
Key proportions, colors and some other design elements can be changed at any time too.
See the interface for details.
*/

class IVKeyboardControl : public IControl
{
public:

  IVKeyboardControl(IPlugBaseGraphics& pPlug, float x, float y, float w, float h, int minNote, int maxNote) :
    IControl(pPlug, IRECT(x, y, x + w, y + h))
  {
    mDblAsSingleClick = true;
    mShowNoteAndVel = false;
    mDrawShadows = true;
    mDrawBorder = true;
    mColB = IColor(255, 70, 70, 70);
    mColW = IColor(255, 240, 240, 240);
    mColPressed = IColor(60, 0, 0, 0);
    mBAlpha = 100;
    mText.mColor = mColB;
    mBKWidthR = 0.6f;
    mBKHeightR = 0.6f;
    mKey = -1;
    mMouseOverKey = -1;
    mVelocity = 0.0;
    mVelByWheel = false;
    bool keepWidth = !(w < 0.0);
    if (w < 0.0)
    {
      mRECT.R = mRECT.L + mRECT.H();
      mTargetRECT = mRECT;
    }
    SetMinMaxNote(minNote, maxNote, keepWidth);
  }

  IVKeyboardControl(IPlugBaseGraphics& pPlug, float x, float y, int minNote, int maxNote) :
    IVKeyboardControl(pPlug, x, y, -1.0, 70.0, minNote, maxNote) {}

  ~IVKeyboardControl()
  {
    mKeyRects.Empty(true);
    mNoteIsPlayed.Empty(true);
    mKeyIsBlack.Empty(true);
  }

  // returns pressed key number inside the keyboard
  int GetKey() const
  {
    return mKey;
  }
  // returns pressed MIDI note number
  int GetNote() const
  {
    if (mKey > -1) return mMinNote + mKey;
    else return -1;
  }

  double GetVelocity() const
  {
    return mVelocity * 127.f;
  }

  double GetVelocityNormalized() const
  {
    return mVelocity;
  }

  int GetVelocityInt() const
  {
    return (int)(mVelocity * 127. + 0.5);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    int oldK = mKey;
    mKey = GetKeyUnderMouse(x, y);
    if (oldK != mKey) mVelByWheel = false;
    mMouseOverKey = mKey;
    if (!mVelByWheel) UpdateVelocity(y);
    SetDirty();
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if (mKey > -1)
    {
      mKey = -1;
      mMouseOverKey = -1;
      mVelocity = 0.0;
      mVelByWheel = false;
      SetDirty();
    }
  }

  void OnMouseOut() override
  {
    if (mKey > -1 || mShowNoteAndVel)
    {
      mKey = -1;
      mMouseOverKey = -1;
      mVelocity = 0.0;
      mVelByWheel = false;
      SetDirty();
    }
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    OnMouseDown(x, y, mod);
  }

  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    if (mKey > -1)
    {
      if (mod.C || mod.S) mVelocity += 0.003f * d;
      else mVelocity += 0.03f * d;
      mVelByWheel = true;
      mVelocity = BOUNDED(mVelocity, 1.f / 127.f, 1.f);
#ifdef _DEBUG
      SetDirty();
#else
      if (mShowNoteAndVel) SetDirty();
#endif
    }
  }

  void OnMouseOver(float x, float y, const IMouseMod& pMod) override
  {
    if (mShowNoteAndVel)
    {
      mMouseOverKey = GetKeyUnderMouse(x, y);
      SetDirty();
    }
  }

  void Draw(IGraphics& pG) override
  {
    if (mRECT != mTargetRECT)
    {
      // cleanup old area after resizing
      auto tc = IColor(15, 127, 127, 127); // Lice used to ignore drawing with very low alpha
      auto cr = mRECT.GetPadded(0, 0, 1, 1);
      pG.FillRect(tc, cr);
      mRECT = mTargetRECT;
      Redraw();
    }

    auto shadowC = IColor(60, 0, 0, 0);
    pG.FillRect(mColW,mTargetRECT);

    // first draw whites
    for (int i = 0; i < mKeyRects.GetSize(); ++i)
    {
      if (!*(mKeyIsBlack.Get(i)))
      {
        pG.DrawRect(mText.mColor, *(mKeyRects.Get(i)));
        if (i == mKey || *(mNoteIsPlayed.Get(i)))
        {
          // draw played white key
          auto r = *mKeyRects.Get(i);
          pG.FillRect(mColPressed, r);
          if (mDrawShadows)
          {
            r.R = r.L + 0.35f * r.W();
            pG.FillRect(shadowC, r);
          }
        }
      }
    }

    // then blacks
    for (int i = 0; i < mKeyRects.GetSize(); ++i)
    {
      if (*(mKeyIsBlack.Get(i)))
      {
        // first draw underlying shadows
        if (mDrawShadows && i != mKey && !*(mNoteIsPlayed.Get(i))
            && i < mKeyRects.GetSize() - 1)
        {
          auto r = *mKeyRects.Get(i);
          float w = r.W();
          r.L += 0.6f * w;
          if (i + 1 == mKey || *(mNoteIsPlayed.Get(i + 1)))
          {
            // if white to the right is pressed, shadow is longer
            w *= 1.3f;
            r.B = r.T + 1.05f * r.H();
          }
          r.R = r.L + w;
          pG.FillRect(shadowC, r);
        }
        pG.FillRect(mColB, *(mKeyRects.Get(i)));
        if (i == mKey || *(mNoteIsPlayed.Get(i)))
        {
          // draw played black key
          auto cBP = mColPressed;
          cBP.A = (int) mBAlpha;
          pG.FillRect(cBP, *(mKeyRects.Get(i)));
        }
      }
    }

    if (mDrawBorder) pG.DrawRect(mText.mColor, mTargetRECT);

    if (mShowNoteAndVel)
    {
      if (mMouseOverKey > -1)
      {
        auto r = *mKeyRects.Get(mMouseOverKey);
        r.B = r.T + 1.2f * mText.mSize;
        r.R = r.L + 35.0f;
        WDL_String t = NoteName(mMinNote + mMouseOverKey);
        if (mKey > -1)
        {
          t.AppendFormatted(16, ", vel: %3.2f", GetVelocity());
          r.R += 60.0;
        }
        auto e = r.R - mRECT.R;
        if (e > 0.0)
        {
          r.L -= e;
          r.R -= e;
        }
        pG.FillRect(mColW, r);
        pG.DrawRect(mText.mColor, r);
        pG.DrawText(mText, t.Get(), r);
      }
    }

#ifdef _DEBUG
    //pG->DrawRect(&COLOR_GREEN, &mTargetRECT);
    //pG->DrawRect(&COLOR_BLUE, &mRECT);
    //for (int i = 0; i < mKeyRects.GetSize(); ++i) pG->DrawRect(&COLOR_ORANGE, mKeyRects.Get(i));
    WDL_String ti;
    ti.SetFormatted(32, "key: %d, vel: %3.2f", mKey, GetVelocity());
    //ti.SetFormatted(32, "key: %d, vel: %d", mKey, GetVelocityInt());
    //ti.SetFormatted(16, "mBAlpha: %d", mBAlpha);
    IText txt(20, COLOR_RED);
    auto& mr = mRECT;
    IRECT tr(mr.L + 20, mr.B - 20, mr.L + 160, mr.B);
    pG.DrawText(txt, ti.Get(), tr);
#endif
  }

  void SetMinMaxNote(int min, int max, bool keepWidth = true)
  {
    if (min < 0 || max < 0) return;
    if (min < max)
    {
      mMinNote = min;
      mMaxNote = max;
    }
    else
    {
      mMinNote = max;
      mMaxNote = min;
    }
    mNoteIsPlayed.Empty(true);
    for (int n = mMinNote; n <= mMaxNote; ++n) // todo here use a call to host
      mNoteIsPlayed.Add(new bool(false));    // to keep visible state actual
    RecreateRects(keepWidth);
  }

  void SetNoteIsPlayed(int noteNum, bool played)
  {
    if (noteNum < mMinNote || noteNum > mMaxNote) return;
    mNoteIsPlayed.Set(noteNum - mMinNote, &played);
    SetDirty();
  }

  void SetBlackToWhiteWidthAndHeightRatios(float widthR, float heightR = 0.6)
  {
    if (widthR <= 0.0 || heightR <= 0.0) return;
    if (widthR > 1.0) widthR = 1.0;
    if (heightR > 1.0) heightR = 1.0;
    mBKHeightR = heightR;
    float r = widthR / mBKWidthR;
    mBKWidthR = widthR;
    auto& tR = mTargetRECT;
    float bkBot = tR.T + tR.H() * heightR;
    for (int i = 0; i < mKeyRects.GetSize(); ++i)
    {
      if (*(mKeyIsBlack.Get(i)))
      {
        auto& kr = *(mKeyRects.Get(i));
        kr.B = bkBot;
        auto d = 0.5f * kr.W();
        float mid = 0.5f * (kr.L + kr.R);
        kr.L = mid - d * r;
        kr.R = mid + d * r;
        if (kr.L < tR.L)kr.L = tR.L;
        if (kr.R > tR.R)kr.R = tR.R;
      }
    }
    SetDirty();
  }

  void SetHeight(float h, bool keepProportions = false)
  {
    if (h < 0) return;
    auto& tR = mTargetRECT;
    auto r = h / tR.H();
    tR.B = tR.T + tR.H() * r;
    for (int i = 0; i < mKeyRects.GetSize(); ++i)
    {
      auto& kr = *(mKeyRects.Get(i));
      kr.B = kr.T + kr.H() * r;
    }
    if (keepProportions) SetWidth(tR.W() * r);
    SetDirty();
  }

  void SetWidth(float w, bool keepProportions = false)
  {
    if (w < 0) return;
    auto& tR = mTargetRECT;
    auto r = w / tR.W();
    tR.R = tR.L + tR.W() * r;
    for (int i = 0; i < mKeyRects.GetSize(); ++i)
    {
      auto& kr = *(mKeyRects.Get(i));
      auto kw = kr.W();
      auto d = kr.L - tR.L;
      kr.L = tR.L + d * r;
      kr.R = kr.L + kw * r;
    }
    if (keepProportions) SetHeight(tR.H() * r);
    SetDirty();
  }

  void SetShowNotesAndVelocity(bool show)
  {
    mShowNoteAndVel = show;
  }

  void SetColors(IColor blacks, IColor whites, IColor pressed = IColor(60, 0, 0, 0), IColor line = IColor(255, 80, 80, 80))
  {
    mColB = blacks;
    mColW = whites;
    mColPressed = pressed;
    mText.mColor = line;

    auto Luminocity = [] (IColor c)
    {
      auto min = c.R < c.G ? (c.R < c.B ? c.R : c.B) : (c.G < c.B ? c.G : c.B);
      auto max = c.R > c.G ? (c.R > c.B ? c.R : c.B) : (c.G > c.B ? c.G : c.B);
      return (min + max) / 2;
    };

    mBAlpha = (float)pressed.A;

    if (mBAlpha < 240)
    {
      float lumW = Luminocity(whites) * whites.A / 255.f;
      float transp = pressed.A / 255.f;
      float lumP = Luminocity(pressed) * transp;
      float lumRes = (1.f - transp) * lumW + lumP;
      float dWLum = lumRes - lumW;

      float lumB = Luminocity(blacks) * blacks.A / 255.f;

      if ((dWLum < 0 && lumB < lumW) || (dWLum > 0 && lumB > lumW))
      {
        float dbWB = lumW - lumB; // not used in the conditions ^^ for readability
        mBAlpha += (255.f - mBAlpha) * (1.f - dbWB * dbWB / 255.f / 255.f) + 0.5f;
      }
      else
        mBAlpha += dWLum + 0.5f;

      mBAlpha = BOUNDED(mBAlpha, 15.f, 255.f);
    }

    SetDirty();
  }

  void SetDrawShadows(bool draw)
  {
    mDrawShadows = draw;
  }

  void SetDrawBorder(bool draw)
  {
    mDrawBorder = draw;
    SetDirty();
  }

protected:
  int mMinNote, mMaxNote;
  int mKey, mMouseOverKey;
  float mVelocity;
  float mBKWidthR, mBKHeightR;
  bool mShowNoteAndVel, mDrawShadows, mDrawBorder;
  bool mVelByWheel;
  WDL_PtrList<bool> mKeyIsBlack;
  WDL_PtrList<bool> mNoteIsPlayed;
  WDL_PtrList<IRECT> mKeyRects;
  IColor mColB;
  IColor mColW;
  IColor mColPressed;
  float mBAlpha; // needed cause not any mColPressed will have nice contrast on black keys


  void RecreateRects(bool keepWidth)
  {
    // save key width if needed
    float whiteW = 0.0;
    if (!keepWidth)
    {
      if (mKeyRects.GetSize())
      {
        whiteW = mKeyRects.Get(0)->W();
        if (*(mKeyIsBlack.Get(0))) whiteW /= mBKWidthR;
      }
      else whiteW = 0.2f * mTargetRECT.H();
    }
    mKeyRects.Empty(true);

    // create size-independent data.
    // black key middle isn't aligned exactly between whites
    float wPadStart = 0.0; // 1st note may be black
    float wPadEnd = 0.0;   // last note may be black

    auto ShiftForKey = [this] (int note)
    {
      // usually black key width + distance to the closest black key = white key width,
      // and often b width is ~0.6 * w width
      if (note == 0) return 0.f;
      else if (note % 12 == 1)  return 7.f / 12.f;
      else if (note % 12 == 3)  return 5.f / 12.f;
      else if (note % 12 == 6)  return 2.f / 3.f;
      else if (note % 12 == 8)  return 0.5f;
      else if (note % 12 == 10) return 1.f / 3.f;
      else return 0.f;
    };

    wPadStart = ShiftForKey(mMinNote);
    if (mMinNote != mMaxNote) wPadEnd = 1.f - ShiftForKey(mMaxNote);

    mKeyIsBlack.Empty(true);
    float numWhites = 0.0;;
    for (int n = mMinNote; n <= mMaxNote; ++n)
    {
      if (n % 12 == 1 || n % 12 == 3 || n % 12 == 6 || n % 12 == 8|| n % 12 == 10)
      {
        mKeyIsBlack.Add(new bool(true));
      }
      else
      {
        mKeyIsBlack.Add(new bool(false));
        numWhites += 1.0;
      }
    }


    // build rects
    auto& top = mTargetRECT.T;
    auto& whiteB = mTargetRECT.B;
    float blackB = top + mTargetRECT.H() * mBKHeightR;

    if (whiteW == 0) whiteW = 0.2f * mTargetRECT.H(); // first call from the constructor
    if (keepWidth)
    {
      whiteW = mTargetRECT.W();
      if (numWhites) whiteW /= (numWhites + wPadStart + wPadEnd);
    }
    float blackW = whiteW;
    if (numWhites) blackW *= mBKWidthR;

    float prevWhiteRectR = mTargetRECT.L;

    for (int k = 0; k < mKeyIsBlack.GetSize(); ++k)
    {
      if (*(mKeyIsBlack.Get(k)))
      {
        float l = prevWhiteRectR;
        if (k != 0)
        {
          auto s = ShiftForKey(mMinNote + k);
          l -= s * blackW;
        }
        else prevWhiteRectR += wPadStart * blackW;
        mKeyRects.Add(new IRECT(l, top, l + blackW, blackB));
      }
      else
      {
        mKeyRects.Add(new IRECT(prevWhiteRectR, top, prevWhiteRectR + whiteW, whiteB));
        prevWhiteRectR += whiteW;
      }
    }

    mTargetRECT.R = (mKeyRects.Get(mKeyRects.GetSize() - 1))->R;

    SetDirty();
  }

  int GetKeyUnderMouse(float x, float y)
  {
    // black keys are on top
    int k = -1;
    for (int i = 0; i < mKeyRects.GetSize(); ++i)
    {
      if (*(mKeyIsBlack.Get(i)) && mKeyRects.Get(i)->Contains(x, y))
      {
        k = i;
        break;
      }
    }
    if (k < 0) for (int i = 0; i < mKeyRects.GetSize(); ++i)
      {
        if (!*(mKeyIsBlack.Get(i)) && mKeyRects.Get(i)->Contains(x, y))
        {
          k = i;
          break;
        }
      }

    return k;
  }

  void UpdateVelocity(float y)
  {
    if (mKey > -1)
    {
      auto kr = mKeyRects.Get(mKey);
      mVelocity = (float) (y - kr->T) / (0.95f * kr->H());
      // 0.95 is to get max velocity around the bottom
      mVelocity = BOUNDED(mVelocity, 1.f / 127.f, 1.f);
    }
    else mVelocity = 0.f;
  }

  WDL_String NoteName(int midiNoteNum, bool addOctave = true)
  {
    int oct = midiNoteNum / 12;
    midiNoteNum -= 12 * oct;
    const char* notes[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    const char* n = notes[midiNoteNum];
    WDL_String name;
    name.Set(n);
    if (addOctave) name.AppendFormatted(2, "%d", --oct);
    return name;
  }

};