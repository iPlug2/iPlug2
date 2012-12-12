#ifndef _IVECSLIDERS_
#define _IVECSLIDERS_

#ifdef WIN32
#define round(x) floor(x + 0.5)
#endif


#define MAXSLIDERS 512

class MultiSliderControlV: public IControl
{
public:
  MultiSliderControlV(IPlugBase *pPlug,
                      IRECT pR,
                      int paramIdx,
                      int numSliders,
                      int handleWidth,
                      const IColor* bgcolor,
                      const IColor* fgcolor,
                      const IColor* hlcolor)
    : IControl(pPlug, pR, paramIdx)
  {
    mBgColor = *bgcolor;
    mFgColor = *fgcolor;
    mHlColor = *hlcolor;

    mNumSliders = numSliders;
    mHighlighted = -1;
    mGrain = 0.001;
    mSliderThatChanged = -1;

    float sliderWidth = floor((float) mRECT.W() / (float) numSliders);

    mSteps = new double[numSliders];

    for(int i=0; i<numSliders; i++)
    {
      int lpos = (i * sliderWidth);
      mSteps[i] = 0.;

      mSliderBounds[i] = new IRECT(mRECT.L + lpos , mRECT.T, mRECT.L + lpos + sliderWidth, mRECT.B);
    }

    mHandleWidth = handleWidth;
  }

  ~MultiSliderControlV()
  {
    delete mSteps;

    for(int i=0; i<mNumSliders; i++)
    {
      delete mSliderBounds[i];
    }
  }

  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&mBgColor, &mRECT);

    for(int i=0; i<mNumSliders; i++)
    {
      float yPos = mSteps[i] * mRECT.H();
      int top = mRECT.B - yPos;
      //int bottom = top + 10;
      int bottom = mRECT.B;

      IColor * color = &mFgColor;
      if(i == mHighlighted) color = &mHlColor;

      IRECT srect = IRECT(mSliderBounds[i]->L, top, mSliderBounds[i]->R-1, bottom);
      pGraphics->FillIRect(color, &srect );
    }

    return true;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    SnapToMouse(x, y);
  }

  void OnMouseUp(int x, int y, IMouseMod* pMod)
  {
    //TODO: check this isn't going to cause problems... this will happen from the gui thread
    mPlug->ModifyCurrentPreset();
    mPlug->DirtyPTCompareState();
  }

  void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
  {
    SnapToMouse(x, y);
  }

  void SnapToMouse(int x, int y)
  {
    x = BOUNDED(x, mRECT.L, mRECT.R-1);
    y = BOUNDED(y, mRECT.T, mRECT.B-1);

    float yValue =  (float) (y-mRECT.T) / (float) mRECT.H();

    yValue = round( yValue / mGrain ) * mGrain;

    int sliderTest = mNumSliders-1;
    bool foundIntersection = false;

    while (!foundIntersection)
    {
      foundIntersection = mSliderBounds[sliderTest]->Contains(x, y);

      if (!foundIntersection && sliderTest !=0 ) sliderTest--;
    }

    if (foundIntersection)
    {
      //mHighlighted = sliderTest;
      mSteps[sliderTest] = 1. - BOUNDED(yValue, 0., 1.);
      mSliderThatChanged = sliderTest;
      mPlug->OnParamChange(mParamIdx); // TODO: rethink this WRT threading
    }
    else
    {
      mSliderThatChanged = -1;
      //mHighlighted = -1;
    }

    SetDirty();
  }

  void GetLatestChange(double* data)
  {
    data[mSliderThatChanged] = mSteps[mSliderThatChanged];
  }

  void GetState(double* data)
  {
    memcpy( data, mSteps, mNumSliders * sizeof(double));
  }

  void SetState(double* data)
  {
    memcpy(mSteps, data, mNumSliders * sizeof(double));

    SetDirty();
  }

  void SetHighlight(int i)
  {
    mHighlighted = i;

    SetDirty();
  }

private:
  IColor mBgColor, mFgColor, mHlColor;
  int mNumSliders;
  int mHandleWidth;
  int mSliderThatChanged;
  double *mSteps;
  double mGrain;
  IRECT *mSliderBounds[MAXSLIDERS];
  int mHighlighted;
};


class IVSliderControl: public IControl
{
public:
  IVSliderControl(IPlugBase *pPlug,
                  IRECT pR,
                  int paramIdx,
                  int handleWidth,
                  const IColor* bgcolor,
                  const IColor* fgcolor)
    : IControl(pPlug, pR, paramIdx)
  {
    mBgColor = *bgcolor;
    mFgColor = *fgcolor;
    mHandleWidth = handleWidth;
  }

  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&mBgColor, &mRECT);

    float yPos = mValue * mRECT.H();
    int top = mRECT.B - yPos;

    IRECT innerRect = IRECT(mRECT.L+2, top, mRECT.R-2, mRECT.B);
    pGraphics->FillIRect(&mFgColor, &innerRect);

    return true;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    SnapToMouse(x, y);
  }

  void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
  {
    SnapToMouse(x, y);
  }

  void SnapToMouse(int x, int y)
  {
    x = BOUNDED(x, mRECT.L, mRECT.R-1);
    y = BOUNDED(y, mRECT.T, mRECT.B-1);

    float yValue = 1. - (float) (y-mRECT.T) / (float) mRECT.H();

    mValue = round( yValue / 0.001 ) * 0.001;

    mPlug->SetParameterFromGUI(mParamIdx, BOUNDED(mValue, 0., 1.));

    SetDirty();
  }

private:
  IColor mBgColor, mFgColor;
  int mHandleWidth;
};

#endif _IVECSLIDERS_
