/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#ifndef IGRAPHICS_SKIA
#error This IControl only works with the Skia graphics backend
#endif

/**
 * @file
 * @copydoc ISkShaderControl
 */

#include "IControl.h"
#include "SkRuntimeEffect.h"
#include "SkGradientShader.h"
#include "SkPerlinNoiseShader.h"
#include "IGraphicsSkia.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control that hosts an SkSL fragment shader
 * https://github.com/google/skia/blob/master/src/sksl/README
 * Shaders ported from Calvary SkSL ports of shadertoy examples
 * https://medium.com/cavalry-animation/sksl-in-cavalry-932d503c889e
 * @ingroup IControls */
class ISkShaderControl : public IControl
{
public:
  ISkShaderControl(const IRECT& bounds)
  : IControl(bounds)
  {
    // load a file
//    auto data = SkData::MakeFromFileName("C:\\Users\\oli\\Desktop\\SkShader.frag");
//    mShaderStr = SkString((const char*)data->bytes(), data->size());
//
//    DBGMSG(mShaderStr.c_str());

    // basic with input shader, not yet working
//    mShaderStr = R"(
//      in shader child;
//      uniform float iTime;
//      uniform float2 iResolution;
//      void main(float2 p, inout half4 color) {
//          color = sample(child, p);
//      }
//      )";

    // basic
//    mShaderStr = R"(
//    uniform float iTime;
//    uniform float2 iResolution;
//    void main(float2 fragCoord, inout half4 color) {
//      float2 st = fragCoord/iResolution.xy;
//      color = half4(st, 0., 1.);
//    }
//    )";
    
    // spiral
//    mShaderStr = R"(
//    uniform float iTime;
//    uniform float2 iResolution;
//
//    void main(float2 p, inout half4 color) {
//      float2 in_center = float2(300., 300.);
//      float4 in_colors0 = float4(1.,1.,1.,1.);
//      float4 in_colors1 = float4(0.,0.,0.,1.);
//      float2 pp = p - in_center;
//      float radius = sqrt(dot(pp, pp));
//      radius = sqrt(radius);
//      float angle = atan(pp.y / pp.x);
//      float t = (angle + 3.1415926/2) / (3.1415926);
//      t += radius * iTime;
//      t = fract(t);
//      color = half4(mix(in_colors0, in_colors1, t));
//    }
//    )";
    
    // noise
//    mShaderStr = R"(
//    uniform float iTime;
//    uniform float2 iResolution;
//    float random (float2 st) {
//      return fract(sin(dot(st.xy , float2(12.9898,78.233))) * 43758.5453);
//    }
//    void main(float2 fragCoord, inout half4 fragColor) {
//      float2 st = fragCoord/iResolution.xy;
//      st *=max(1.0, 20); // Scale the coordinate system
//      float2 ipos = floor(st); // get the integer coords
//      float2 fpos = fract(st); // get the fractional coords
//      float3 color = float3(random(ipos));
//      fragColor = half4(color, 1.0);
//    }
//    )";
    
    // Solar Wind
//    mShaderStr = R"(
//    uniform float iTime;
//    uniform float2 iResolution;
//    uniform float glow;
//
//    float wind(float3 p) {
//      const float sphsize=.7; // planet size
//      const float dist=.27; // distance for glow and distortion
//      const float perturb=.3; // distortion amount of the flow around the planet
//      const float fade=.005; //fade by distance
//      const float windspeed=.4; // speed of wind flow
//
//      // fractal params
//      const int iterations=13;
//      const float fractparam=.7;
//      const float3 offset=float3(1.0,2.,-1.5);
//
//
//      float d=max(0.,dist-max(0.,length(p)-sphsize)/sphsize)/dist; // for distortion and glow area
//      float x=max(0.2,p.x*2.); // to increase glow on left side
//      p.y*=1.+max(0.,-p.x-sphsize*.25)*1.5; // left side distortion (cheesy)
//      p-=d*normalize(p)*perturb; // spheric distortion of flow
//      p+=float3(iTime*windspeed,0.,0.); // flow movement
//      p=abs(fract((p+offset)*.1)-.5); // tile folding
//      for (int i=0; i<iterations; i++) {
//        p=abs(p)/dot(p,p)-fractparam; // the magic formula for the hot flow
//      }
//      return length(p)*(1.+d*glow*x)+d*glow*x; // return the result with glow applied
//    }
//
//    void main(float2 fragCoord, inout half4 color)
//    {
//      const float steps=110.; // number of steps for the volumetric rendering
//      const float stepsize=.025;
//      const float brightness=.43;
//      const float displacement=.015; // hot air effect
//      const float windspeed=.4; // speed of wind flow
//      const float sphsize=.7; // planet size
//      const float fade=.005; //fade by distance
//      const float3 planetcolor=float3(0.8,0.3,0.2);
//
//      // get ray dir
//      float2 uv = fragCoord.xy / iResolution.xy;
//      float3 dir=float3(uv,1.);
//      dir.x*=iResolution.x/iResolution.y;
//      //  float3 from=float3(0.,0.,-2.+texture(iChannel0,uv*.5+iTime).x*stepsize); //from+dither
//
//      float3 from=float3(0.,0.,-2.+1*stepsize); //from+dither
//
//      // volumetric rendering
//      float v=0., l=-0.0001, t=iTime*windspeed*.2;
//      for (float r=10.;r<steps;r++) {
//        float3 p=from+r*dir*stepsize;
//        float tx=1.0*displacement; // hot air effect
//        if (length(p)-sphsize-tx>0.)
//        // outside planet, accumulate values as ray goes, applying distance fading
//          v+=min(50.,wind(p))*max(0.,1.-r*fade);
//        else if (l<0.)
//        //inside planet, get planet shading if not already
//        //loop continues because of previous problems with breaks and not always optimizes much
//          l=pow(max(.53,dot(normalize(p),normalize(float3(-1.,.5,-0.3)))),4.)
//          *(.5+1*2.);
//        }
//      v/=steps; v*=brightness; // average values and apply bright factor
//      float3 col=float3(v*1.25,v*v,v*v*v)+l*planetcolor; // set color
//      col*=1.-length(pow(abs(uv),float2(5.)))*14.; // vignette (kind of)
//      color = half4(col,1.0);
//    }
//    )";

    // Stars
//    mShaderStr = R"(
//    uniform float iTime;
//    uniform float2 iResolution;
//
//    void main(float2 fragCoord, inout half4 color)
//    {
//      int iterations =  17;
//      float formuparam = 0.53;
//
//      int volsteps = 20;
//      float stepsize = 0.1;
//
//      float zoom = 0.800;
//      float tile = 0.850;
//      float speed = 0.010;
//
//      float brightness = 0.0015;
//      float darkmatter = 0.300;
//      float distfading = 0.730;
//      float saturation = 0.850;
//
//      //get coords and direction
//      float2 uv=fragCoord.xy/iResolution.xy-.5;
//      uv.y*=iResolution.y/iResolution.x;
//      float3 dir=float3(uv*zoom,1.);
//      float time=iTime*speed+.25;
//
//      // //mouse rotation
//      float a1=.5+0/iResolution.x*2.;
//      float a2=.8+0/iResolution.y*2.;
//      float2x2 rot1=float2x2(cos(a1),sin(a1),-sin(a1),cos(a1));
//      float2x2 rot2=float2x2(cos(a2),sin(a2),-sin(a2),cos(a2));
//      dir.xz*=rot1;
//      dir.xy*=rot2;
//      float3 from=float3(1.,.5,0.5);
//      from+=float3(time*2.,time,-2.);
//      from.xz*=rot1;
//      from.xy*=rot2;
//
//      // //volumetric rendering
//      float s=0.1,fade=1.;
//      float3 v=float3(0.);
//      for (int r=0; r<volsteps; r++) {
//        float3 p=from+s*dir*.5;
//        p = abs(float3(tile)-mod(p,float3(tile*2.))); // tiling fold
//        float pa,a=pa=0.;
//        for (int i=0; i<iterations; i++) {
//          p=abs(p)/dot(p,p)-formuparam; // the magic formula
//          a+=abs(length(p)-pa); // absolute sum of average change
//          pa=length(p);
//        }
//        float dm=max(0.,darkmatter-a*a*.001); //dark matter
//        a*=a*a; // add contrast
//        if (r>6) fade*=1.-dm; // dark matter, don't render near
//        // v+=float3(dm,dm*.5,0.);
//        v+=fade;
//        v+=float3(s,s*s,s*s*s*s)*a*brightness*fade; // coloring based on distance
//        fade*=distfading; // distance fading
//        s+=stepsize;
//      }
//      v=mix(float3(length(v)),v,saturation); //color adjust
//      color = half4(v*.01,1.);
//    }
//    )";
        
    sk_sp<SkShader> shader;

//    shader = SkGradientShader::MakeLinear(points, colors, nullptr, 2, SkTileMode::kClamp);
//    mShaders.push_back(std::make_pair("Linear Gradient", shader));
//
//    shader = SkGradientShader::MakeRadial({ 128, 128 }, 128, colors, nullptr, 2,
//                                          SkTileMode::kClamp);
//    mShaders.push_back(std::make_pair("Radial Gradient", shader));
//
//    shader = SkGradientShader::MakeSweep(128, 128, colors, nullptr, 2);
//    mShaders.push_back(std::make_pair("Sweep Gradient", shader));
//
//    shader = GetResourceAsImage("images/mandrill_256.png")->makeShader();
//    mShaders.push_back(std::make_pair("Mandrill", shader));

    shader = SkPerlinNoiseShader::MakeImprovedNoise(0.025f, 0.025f, 3, 0.0f);
    mShaders.push_back(std::make_pair("Perlin Noise", shader));
    
    SetActionFunction([&](IControl* pCaller) {
      SetAnimation([&](IControl* pCaller) {
        float* pInputLoc = (float*) mShaderInputs.get();
        pInputLoc[0] = pCaller->GetAnimationProgress();
        pInputLoc[1] = pCaller->GetRECT().W();
        pInputLoc[2] = pCaller->GetRECT().H();

        if (pCaller->GetAnimationProgress() > 1.) {
          pCaller->OnEndAnimation();
        }

      }, 10000);
    });
    
    Rebuild();
    SetDirty(true);
  }

  void Draw(IGraphics& g) override
  {
    if(mRTEffect)
      DoDraw((SkCanvas*) g.GetDrawContext());
  }
  
  void DoDraw(SkCanvas* canvas)
  {
    canvas->save();
    canvas->translate(mRECT.L, mRECT.T);
    canvas->drawRect({ 0, 0, mRECT.W(), mRECT.H() }, mPaint);
    canvas->restore();
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    SetDirty(true);
  }
  
  bool Rebuild()
  {
    auto [effect, errorText] = SkRuntimeEffect::Make(mShaderStr);
    if (!effect) {
      DBGMSG("%s\n", errorText.c_str());
      return false;
    }

    size_t oldSize = mRTEffect ? mRTEffect->inputSize() : 0;
    mShaderInputs.realloc(effect->inputSize());
    if (effect->inputSize() > oldSize) {
      memset(mShaderInputs.get() + oldSize, 0, effect->inputSize() - oldSize);
    }
    mChildren.resize_back((int) effect->children().count());
    for (auto& c : mChildren) {
      if (!c) {
        c = mShaders[0].second;
      }
    }
    
    mRTEffect = effect;
    
    auto inputs = SkData::MakeWithoutCopy(mShaderInputs.get(), mRTEffect->inputSize());
    auto shader = mRTEffect->makeShader(std::move(inputs), mChildren.data(), mChildren.count(),
                                      nullptr, false);
    
    mPaint.setShader(std::move(shader));
    

    return true;
  }
  
private:
  SkString mShaderStr;
  sk_sp<SkRuntimeEffect> mRTEffect;
  SkAutoTMalloc<char> mShaderInputs;
  SkTArray<sk_sp<SkShader>> mChildren;
  SkPaint mPaint;
  
  // Named shaders that can be selected as inputs
  SkTArray<std::pair<const char*, sk_sp<SkShader>>> mShaders;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
