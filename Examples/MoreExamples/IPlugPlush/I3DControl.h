#include "../../WDL/plush2/plush.h"

class I3DControl : public IControl
{
private:
  pl_Mat* mMaterial;
  pl_Obj* mPlane;
  pl_Obj* mObj;
  pl_Cam* mCam;
  pl_Light* mLight;
  LICE_SubBitmap* mBitmap;
  LICE_MemBitmap* mTexture;
public:
  I3DControl(IPlugBase* pPlug, IRECT pR)
    : IControl(pPlug, pR, -1),
      mBitmap(0)
  {

    mTexture = new LICE_MemBitmap(1, 1);
    //LICE_TexGen_Noise(mTexture, NULL, 1.0, 1.0, 1.0, 1.0f, NOISE_MODE_WOOD, 8);
    LICE_FillRect(mTexture, 0, 0, 1, 1, LICE_RGBA(255, 0 , 0, 255), 1., LICE_BLIT_MODE_COPY);

    mMaterial = new pl_Mat;
    //mMaterial->Lightable = false; // defaults to true
    mMaterial->Smoothing = false; // defaults to true

    mMaterial->Ambient[0] = mMaterial->Ambient[1] = mMaterial->Ambient[2] = 0.05;
    mMaterial->Diffuse[0] = mMaterial->Diffuse[1] = mMaterial->Diffuse[2] = 0.5;
    mMaterial->Texture = mTexture;

    //mMaterial->PerspectiveCorrect=16;
    // mMaterial->SolidCombineMode = LICE_BLIT_MODE_ADD;
    mMaterial->SolidCombineMode = LICE_BLIT_MODE_COPY;
    mMaterial->SolidOpacity = 1.;
    //mMaterial->FadeDist = 300.0;

    mObj = plMakeCone(10, 30, 20, true, mMaterial);
    //mObj = plMakeCone(20, 100, 20, true, mMaterial);

    mCam = new pl_Cam;
    mCam->AspectRatio = 1.0;
    mCam->X = 0.0;
    mCam->Y = 0.0;
    mCam->Z = -200.0;
    mCam->WantZBuffer = true;
    mCam->SetTarget(0, 0, 0);
    mCam->ClipBack = 300.0;

    mLight = new pl_Light;
    mLight->Set(PL_LIGHT_POINT, //  mode: the mode of the light (PL_LIGHT_*)
                // either the position of the light (PL_LIGHT_POINT*) or the angle in degrees of the light (PL_LIGHT_VECTOR)
                100.0, // X
                0, // Y
                -900.0, // Z
                // intensity: the intensity of the light (0.0-1.0)
                0.9f, // R
                0.9f, // G
                0.9f, // B
                2000); // halfDist: the distance at which PL_LIGHT_POINT_DISTANCE is 1/2 intensity
  }

  ~I3DControl()
  {
    if (mBitmap)
      delete mBitmap;

    delete mMaterial;
    delete mTexture;
    delete mObj;
    delete mLight;
    delete mCam;
  }

  bool Draw(IGraphics* pGraphics)
  {
    static double a;
    a+=0.003;

    LICE_GradRect(pGraphics->GetDrawBitmap(),mRECT.L,mRECT.T, mRECT.W(), mRECT.H(),
                  0.5*sin(a*14.0),0.5*cos(a*2.0+1.3),0.5*sin(a*4.0),1.0,
                  (cos(a*37.0))/mRECT.W()*0.5,(sin(a*17.0))/mRECT.W()*0.5,(cos(a*7.0))/mRECT.W()*0.5,0,
                  (sin(a*12.0))/mRECT.H()*0.5,(cos(a*4.0))/mRECT.H()*0.5,(cos(a*3.0))/mRECT.H()*0.5,0,
                  LICE_BLIT_MODE_COPY);

    if (!mBitmap)
      mBitmap = new LICE_SubBitmap(pGraphics->GetDrawBitmap(), mRECT.L, mRECT.T, mRECT.W(), mRECT.H());

    mObj->Xa += 6.8;
    //mObj->Ya += -0.3;
    //mObj->Zp+=-0.3;

    //LICE_Clear(mBitmap, LICE_RGBA(200, 200, 200, 255));
    mCam->Begin(mBitmap);
    mCam->RenderLight(mLight);
    mCam->RenderObject(mObj);
    mCam->SortToCurrent();
    mCam->End();
    return true;
  }

  bool IsDirty() { return true; }
};
