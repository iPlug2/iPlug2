/*

 IVKeyboardControl by Eugene Yakshin, 2018

 based on

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


 This keyboard is runtime customizable. Any key range is supported.
 Key proportions, colors and some other design elements can be changed at any time too.
 See the interface for details.
 */

class IVKeyboardControl : public IControl
                        , public IVectorBase
{
public:
  static const IColor DEFAULT_BK_COLOR;
  static const IColor DEFAULT_WK_COLOR;
  static const IColor DEFAULT_PK_COLOR;
  static const IColor DEFAULT_FR_COLOR;

  // map to IVectorBase colors
  enum EVKColor
  {
    kBK = kFG,
    kWK = kBG,
    kPK = kHL,
    //kFR = kFR
  };
  
  IVKeyboardControl(IDelegate& dlg, IRECT rect,
                    int minNote = 36, int maxNote = 60);

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnResize() override;

  void Draw(IGraphics& graphics) override;

  void SetMinMaxNote(int min, int max, bool keepWidth = true);
  void SetNoteIsPlayed(int noteNum, bool played);
  void SetBlackToWhiteWidthAndHeightRatios(float widthR, float heightR = 0.6);
  void SetHeight(float h, bool keepProportions = false);
  void SetWidth(float w, bool keepProportions = false);
  void SetShowNotesAndVelocity(bool show);
  void SetColors(const IColor bkColor, const IColor& wkColor, const IColor& pkColor = DEFAULT_PK_COLOR, const IColor& frColor = DEFAULT_FR_COLOR);

  void SetDrawShadows(bool draw) // todo make shadow offset a settable member
  {
    mDrawShadows = draw;
    SetDirty();
  }

  void SetDrawBorders(bool draw)
  {
    mDrawBorders = draw;
    SetDirty();
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

  double GetVelocity() const { return mVelocity * 127.f; }
  double GetVelocityNormalized() const { return mVelocity; }
  int GetVelocityInt() const { return (int)(mVelocity * 127. + 0.5); }

private:
  void RecreateKeyBounds(bool keepWidth);
  int GetKeyUnderMouse(float x, float y);
  void UpdateVelocity(float y);
  void GetNoteNameStr(int midiNoteNum, bool addOctave, WDL_String& str);
  bool IsBlackKey(int i) const { return *(mIsBlackKeyList.Get() + i); }
  float KeyLCoord(int i) { return *(mKeyLCoords.Get() + i); }
  float* KeyLCoordPtr(int i) { return mKeyLCoords.Get() + i; }
  bool NoteIsPlayed(int i) const { return *(mNoteIsPlayed.Get() + i); }
  int NumKeys() const { return mMaxNote - mMinNote + 1; }

  float CalcBKWidth() const
  {
    auto w = mWKWidth;
    if (NumKeys() > 1)
      w *= mBKWidthR;
    return w;
  }

protected:
  bool mShowNoteAndVel = false;
  bool mDrawShadows = true;
  bool mDrawBorders = true;

  float mWKWidth = 0.f;
  float mBKWidthR = 0.6f;
  float mBKHeightRatio = 0.6f;
  float mBKAlpha = 100.f;
  int mKey = -1;
  int mMouseOverKey = -1;
  float mVelocity = 0.f;
  bool mVelByWheel = false;
  int mMinNote, mMaxNote;
  WDL_TypedBuf<bool> mIsBlackKeyList;
  WDL_TypedBuf<bool> mNoteIsPlayed;
  WDL_TypedBuf<float> mKeyLCoords;
};