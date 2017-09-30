#ifndef __IPLUGOPENGL__
#define __IPLUGOPENGL__

#include "IPlug_include_in_plug_hdr.h"

#ifdef OS_OSX
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
//#else
//#endif

class IOpenGLTestControl : public IControl
{
private:
  CGLContextObj mGLContext;
  WDL_TypedBuf<unsigned char> mData;
  float mRotateTri, mRotateQuad;

public:
  IOpenGLTestControl(IPlugBase* pPlug, IRECT pR)
    :	IControl(pPlug, pR, -1)
  {
    mRotateTri = mRotateQuad = 0.f;
    createContext();
    mData.Resize(mRECT.W() * mRECT.H() * 4);
  }

  ~IOpenGLTestControl()
  {
    destroyContext();
  }

  long setContext()
  {
    // Set the current context

    if(CGLSetCurrentContext(mGLContext))
    {
      DBGMSG("Could not make context current.");
      return 1;
    }

    return 0;
  }

  void restoreContext()
  {
    CGLSetCurrentContext(NULL);
  }

  long createContext()
  {
    const GLubyte *glstring;

    GLint npix;
    CGLPixelFormatObj PixelFormat;

    const CGLPixelFormatAttribute attributes[] =
    {
      //kCGLPFAOffScreen,
//      kCGLPFAColorSize, (CGLPixelFormatAttribute)8,
//      kCGLPFADepthSize, (CGLPixelFormatAttribute)16,
      kCGLPFAAccelerated, (CGLPixelFormatAttribute)0
    };

    // Create context if none exists

    CGLChoosePixelFormat(attributes, &PixelFormat, &npix);

    if (PixelFormat == NULL)
    {
      DBGMSG("Could not get pixel format.");
      return 1;
    }

    CGLCreateContext(PixelFormat, NULL, &mGLContext);

    if (mGLContext == NULL)
    {
      DBGMSG("Could not create rendering context.");
      return 1;
    }

    // Set the current context

    if(setContext())
      return 1;

    // Check OpenGL functionality:
    glstring = glGetString(GL_EXTENSIONS);

    if(!gluCheckExtension((const GLubyte *)"GL_EXT_framebuffer_object", glstring))
    {
      DBGMSG("The GL_EXT_framebuffer_object extension is not supported on this system.");
      return 1;
    }

    restoreContext();

    return 0;
  }


  void destroyContext()
  {
    if (mGLContext != NULL)
      CGLDestroyContext(mGLContext);
  }

  bool Draw(IGraphics* pGraphics)
  {
    GLuint framebuffer, renderbuffer;

    const int width = mRECT.W();
    const int height = mRECT.H();

    // Set context
    if (setContext())
      return false;

    // Set up a FBO with one renderbuffer attachment
    glGenFramebuffersEXT(1, &framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
    glGenRenderbuffersEXT(1, &renderbuffer);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, width, height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, renderbuffer);

    glViewport(0,0,width,height);						// Reset The Current Viewport

    glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
    glLoadIdentity();									// Reset The Projection Matrix

    // Calculate The Aspect Ratio Of The Window
    gluPerspective(45.0f,(GLfloat)width/(GLfloat)height, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
    glLoadIdentity();									// Reset The Modelview Matrix
    glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
    glClearColor(0.0f, 0.0f, 0.0f, 1.f);				// Black Background
    glClearDepth(1.0f);									// Depth Buffer Setup
    glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
    glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
    glLoadIdentity();									// Reset The Current Modelview Matrix
    glTranslatef(-1.5f,0.0f,-6.0f);						// Move Left 1.5 Units And Into The Screen 6.0
    glRotatef(mRotateTri,0.0f,1.0f,0.0f);						// Rotate The Triangle On The Y axis ( NEW )
    glBegin(GL_TRIANGLES);								// Start Drawing A Triangle
    glColor3f(1.0f,0.0f,0.0f);						// Red
    glVertex3f( 0.0f, 1.0f, 0.0f);					// Top Of Triangle (Front)
    glColor3f(0.0f,1.0f,0.0f);						// Green
    glVertex3f(-1.0f,-1.0f, 1.0f);					// Left Of Triangle (Front)
    glColor3f(0.0f,0.0f,1.0f);						// Blue
    glVertex3f( 1.0f,-1.0f, 1.0f);					// Right Of Triangle (Front)
    glColor3f(1.0f,0.0f,0.0f);						// Red
    glVertex3f( 0.0f, 1.0f, 0.0f);					// Top Of Triangle (Right)
    glColor3f(0.0f,0.0f,1.0f);						// Blue
    glVertex3f( 1.0f,-1.0f, 1.0f);					// Left Of Triangle (Right)
    glColor3f(0.0f,1.0f,0.0f);						// Green
    glVertex3f( 1.0f,-1.0f, -1.0f);					// Right Of Triangle (Right)
    glColor3f(1.0f,0.0f,0.0f);						// Red
    glVertex3f( 0.0f, 1.0f, 0.0f);					// Top Of Triangle (Back)
    glColor3f(0.0f,1.0f,0.0f);						// Green
    glVertex3f( 1.0f,-1.0f, -1.0f);					// Left Of Triangle (Back)
    glColor3f(0.0f,0.0f,1.0f);						// Blue
    glVertex3f(-1.0f,-1.0f, -1.0f);					// Right Of Triangle (Back)
    glColor3f(1.0f,0.0f,0.0f);						// Red
    glVertex3f( 0.0f, 1.0f, 0.0f);					// Top Of Triangle (Left)
    glColor3f(0.0f,0.0f,1.0f);						// Blue
    glVertex3f(-1.0f,-1.0f,-1.0f);					// Left Of Triangle (Left)
    glColor3f(0.0f,1.0f,0.0f);						// Green
    glVertex3f(-1.0f,-1.0f, 1.0f);					// Right Of Triangle (Left)
    glEnd();											// Done Drawing The Pyramid

    glLoadIdentity();									// Reset The Current Modelview Matrix
    glTranslatef(1.5f,0.0f,-7.0f);						// Move Right 1.5 Units And Into The Screen 7.0
    glRotatef(mRotateQuad,1.0f,1.0f,1.0f);					// Rotate The Quad On The X axis ( NEW )
    glBegin(GL_QUADS);									// Draw A Quad
    glColor3f(0.0f,1.0f,0.0f);						// Set The Color To Green
    glVertex3f( 1.0f, 1.0f,-1.0f);					// Top Right Of The Quad (Top)
    glVertex3f(-1.0f, 1.0f,-1.0f);					// Top Left Of The Quad (Top)
    glVertex3f(-1.0f, 1.0f, 1.0f);					// Bottom Left Of The Quad (Top)
    glVertex3f( 1.0f, 1.0f, 1.0f);					// Bottom Right Of The Quad (Top)
    glColor3f(1.0f,0.5f,0.0f);						// Set The Color To Orange
    glVertex3f( 1.0f,-1.0f, 1.0f);					// Top Right Of The Quad (Bottom)
    glVertex3f(-1.0f,-1.0f, 1.0f);					// Top Left Of The Quad (Bottom)
    glVertex3f(-1.0f,-1.0f,-1.0f);					// Bottom Left Of The Quad (Bottom)
    glVertex3f( 1.0f,-1.0f,-1.0f);					// Bottom Right Of The Quad (Bottom)
    glColor3f(1.0f,0.0f,0.0f);						// Set The Color To Red
    glVertex3f( 1.0f, 1.0f, 1.0f);					// Top Right Of The Quad (Front)
    glVertex3f(-1.0f, 1.0f, 1.0f);					// Top Left Of The Quad (Front)
    glVertex3f(-1.0f,-1.0f, 1.0f);					// Bottom Left Of The Quad (Front)
    glVertex3f( 1.0f,-1.0f, 1.0f);					// Bottom Right Of The Quad (Front)
    glColor3f(1.0f,1.0f,0.0f);						// Set The Color To Yellow
    glVertex3f( 1.0f,-1.0f,-1.0f);					// Top Right Of The Quad (Back)
    glVertex3f(-1.0f,-1.0f,-1.0f);					// Top Left Of The Quad (Back)
    glVertex3f(-1.0f, 1.0f,-1.0f);					// Bottom Left Of The Quad (Back)
    glVertex3f( 1.0f, 1.0f,-1.0f);					// Bottom Right Of The Quad (Back)
    glColor3f(0.0f,0.0f,1.0f);						// Set The Color To Blue
    glVertex3f(-1.0f, 1.0f, 1.0f);					// Top Right Of The Quad (Left)
    glVertex3f(-1.0f, 1.0f,-1.0f);					// Top Left Of The Quad (Left)
    glVertex3f(-1.0f,-1.0f,-1.0f);					// Bottom Left Of The Quad (Left)
    glVertex3f(-1.0f,-1.0f, 1.0f);					// Bottom Right Of The Quad (Left)
    glColor3f(1.0f,0.0f,1.0f);						// Set The Color To Violet
    glVertex3f( 1.0f, 1.0f,-1.0f);					// Top Right Of The Quad (Right)
    glVertex3f( 1.0f, 1.0f, 1.0f);					// Top Left Of The Quad (Right)
    glVertex3f( 1.0f,-1.0f, 1.0f);					// Bottom Left Of The Quad (Right)
    glVertex3f( 1.0f,-1.0f,-1.0f);					// Bottom Right Of The Quad (Right)
    glEnd();											// Done Drawing The Quad

    mRotateTri+=1.5f;											// Increase The Rotation Variable For The Triangle ( NEW )
    mRotateQuad-=1.65f;										// Decrease The Rotation Variable For The Quad ( NEW )

    // // Setup OpenGL for 2D drawing with the correct blending
//    glViewport(0, 0, width, height);
//    glMatrixMode (GL_PROJECTION);
//    glLoadIdentity ();
//    glOrtho (0, width, height, 0, 0, 1);
//    glMatrixMode (GL_MODELVIEW);
//
//    glDepthMask(GL_FALSE);
//    glEnable(GL_BLEND);
//    glEnable(GL_LINE_SMOOTH);
//    glEnable( GL_POINT_SMOOTH );
//    glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
//    glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
//    glDisable(GL_TEXTURE_2D);
//    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
//    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
//
//    glClearColor(0., 0., 0., 0.);
//    glClear(GL_COLOR_BUFFER_BIT);
//
//    glLineWidth(1.1);
//    glPointSize(2.6);
//
//    //DRAW
//    glBegin(GL_QUADS);
//      glColor4d(1., 0., 0., 1.);
//      glVertex2f(0, 0);
//      glVertex2f(width, 0);
//      glVertex2f(width, height);
//      glVertex2f(0, height);
//    glEnd();

    glFlush();
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, mData.Get());

    // Delete the renderbuffer attachment
    glDeleteRenderbuffersEXT(1, &renderbuffer);

    // Make the window the target
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glDeleteFramebuffersEXT(1, &framebuffer);

    restoreContext();

    unsigned char* val = mData.Get();

    for(int v=0; v<height; v++)
    {
      for(int h=0; h<width; h++)
      {
        LICE_pixel pix = LICE_RGBA(*val, *(val + 1), *(val + 2), *(val + 3));
        LICE_PutPixel(pGraphics->GetDrawBitmap(), mRECT.L + h, mRECT.B - v, pix, 1., LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA);
        val += 4;
      }
    }

    return true;
  }

  bool IsDirty()
  {
    return true;
  }

};
#endif
class IPlugOpenGL : public IPlug
{
public:

  IPlugOpenGL(IPlugInstanceInfo instanceInfo);
  ~IPlugOpenGL();

  void Reset();
  void OnParamChange(int paramIdx);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:

  double mGain;
};

#endif
