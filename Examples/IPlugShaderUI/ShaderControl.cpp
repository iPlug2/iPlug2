#include "ShaderControl.h"


#ifdef IGRAPHICS_GL

#if defined OS_MAC || defined OS_IOS
  #if defined IGRAPHICS_GL2
    #include <OpenGL/gl.h>
  #elif defined IGRAPHICS_GL3
    #include <OpenGL/gl3.h>
  #endif
#else
  #include <glad/glad.h>
#endif

#include "IGraphicsNanoVG.h"
#include "config.h"
#include <fstream>

GLuint ShaderControl::sProgram = 0;
int ShaderControl::sInstanceCount = 0;

ShaderControl::ShaderControl(const IRECT& bounds, int paramIdx, const IColor& backgroundColor)
  : IKnobControlBase(bounds, paramIdx)
  , mBackgroundColor(backgroundColor)
{
  SetTooltip("ShaderControl");
  sInstanceCount++;
}

ShaderControl::~ShaderControl()
{
  CleanUp();

  sInstanceCount--;
  if (sInstanceCount == 0 && sProgram)
  {
    glDeleteProgram(sProgram);
    sProgram = 0;
  }
}

void ShaderControl::CleanUp()
{
  if (mFBO)
    nvgDeleteFramebuffer(reinterpret_cast<NVGframebuffer*>(mFBO));

  if (mVAO)
    glDeleteVertexArrays(1, &mVAO);

  if (mVBO)
    glDeleteBuffers(1, &mVBO);

  mFBO = nullptr;
  mVAO = 0;
  mVBO = 0;
}

auto loadShader = [](const char* shaderSourceCStr, const char* filename, GLenum shaderType) {
  DBGMSG("Shader source for %s:\n%s\n", filename, shaderSourceCStr);

  GLuint shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &shaderSourceCStr, NULL);
  glCompileShader(shader);

  GLint isCompiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
  if (!isCompiled)
  {
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    char* buf = (char*)malloc(maxLength + 1);
    glGetShaderInfoLog(shader, maxLength, &maxLength, buf);
    DBGMSG("Shader compilation failed for %s:\n%s\n", filename, buf);
    free(buf);
    glDeleteShader(shader);
    return GLuint(0);
  }
  DBGMSG("Successfully compiled shader: %s\n", filename);
  return shader;
};

auto createProgram = [](GLuint vertexShader, GLuint fragmentShader) {
  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);

  // Before linking, let's verify our attributes
  GLint numAttributes;
  glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numAttributes);
  // DBGMSG("Number of active attributes before linking: %d\n", numAttributes);

  glLinkProgram(program);

  GLint isLinked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
  if (!isLinked)
  {
    GLint maxLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
    char* buf = (char*)malloc(maxLength + 1);
    glGetProgramInfoLog(program, maxLength, &maxLength, buf);
    DBGMSG("Program linking failed:\n%s\n", buf);
    free(buf);
    return GLuint(0);
  }

  // After linking, verify our attributes and uniforms
  glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numAttributes);
  DBGMSG("Number of active attributes after linking: %d\n", numAttributes);

  GLint numUniforms;
  glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
  DBGMSG("Number of active uniforms: %d\n", numUniforms);

  // Print info about each attribute
  char name[256];
  GLint size;
  GLenum type;
  for (int i = 0; i < numAttributes; i++)
  {
    glGetActiveAttrib(program, i, sizeof(name), NULL, &size, &type, name);
    DBGMSG("Attribute %d: %s (type: 0x%x, size: %d)\n", i, name, type, size);
  }

  // Print info about each uniform
  for (int i = 0; i < numUniforms; i++)
  {
    glGetActiveUniform(program, i, sizeof(name), NULL, &size, &type, name);
    DBGMSG("Uniform %d: %s (type: 0x%x, size: %d)\n", i, name, type, size);
  }

  DBGMSG("Successfully linked program\n");
  return program;
};

void ShaderControl::Draw(IGraphics& g)
{
  NVGcontext* vg = static_cast<NVGcontext*>(g.GetDrawContext());
  int w = static_cast<int>(mRECT.W() * g.GetDrawScale());
  int h = static_cast<int>(mRECT.H() * g.GetDrawScale());

  if (invalidateFBO)
  {
    if (mFBO)
      nvgluDeleteFramebuffer(reinterpret_cast<NVGframebuffer*>(mFBO));

    // Use scaled dimensions for the FBO
    float scale = g.GetTotalScale();
    int scaledW = static_cast<int>(mRECT.W() * scale);
    int scaledH = static_cast<int>(mRECT.H() * scale);

//    DBGMSG("Creating FBO with scaled dimensions: %d x %d (scale: %.1f)\n", scaledW, scaledH, scale);

    NVGframebuffer* fbo = nvgCreateFramebuffer(vg, scaledW, scaledH, 0);
    if (!fbo)
    {
      DBGMSG("Failed to create NanoVG framebuffer\n");
      return;
    }
    mFBO = (void*)fbo;

    invalidateFBO = false;
  }

  g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

  nvgEndFrame(vg);
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mInitialFBO);

  nvgBindFramebuffer(reinterpret_cast<NVGframebuffer*>(mFBO));
  nvgBeginFrame(vg, static_cast<float>(w), static_cast<float>(h), static_cast<float>(g.GetTotalScale()));

  GLint last_program, last_vao, last_vbo, last_texture, last_active_texture;
  GLboolean depth_test, blend_enabled;
  GLint blend_src, blend_dst;
  GLint viewport[4];

  glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vao);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_vbo);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
  depth_test = glIsEnabled(GL_DEPTH_TEST);
  blend_enabled = glIsEnabled(GL_BLEND);
  glGetIntegerv(GL_BLEND_SRC, &blend_src);
  glGetIntegerv(GL_BLEND_DST, &blend_dst);
  glGetIntegerv(GL_VIEWPORT, viewport);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (mVAO == 0)
  {
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);

    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    static const float positions[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};

    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
  }

  // Create shader program only once for all instances
  if (sProgram == 0)
  {
    WDL_String vertexShader, fragmentShader;
    auto resLocation = iplug::LocateResource(CIRCLE_VERT_SHADER_FN, "vert", vertexShader, GetUI()->GetBundleID(), nullptr, "");

#ifdef OS_WIN
    if (resLocation == EResourceLocation::kWinBinary)
    {
      int size;
      auto res = iplug::LoadWinResource(vertexShader.Get(), "vert", size, GetUI()->GetWinModuleHandle());
      vertexShader.Set((const char*)res, size + 1);
    }
    else
#endif
    if (resLocation == EResourceLocation::kAbsolutePath)
    {
      std::ifstream shaderFile(vertexShader.Get());
      if (!shaderFile.is_open())
      {
        DBGMSG("Failed to open shader file: %s\n", CIRCLE_VERT_SHADER_FN);
      }

      std::string shaderSource((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
      vertexShader.Set(shaderSource.c_str());
    }
    
    resLocation = iplug::LocateResource(CIRCLE_FRAG_SHADER_FN, "frag", fragmentShader, GetUI()->GetBundleID(), nullptr, "");

#ifdef OS_WIN
    if (resLocation == EResourceLocation::kWinBinary)
    {
      int size;
      auto res = iplug::LoadWinResource(fragmentShader.Get(), "frag", size, GetUI()->GetWinModuleHandle());
      fragmentShader.Set((const char*)res, size + 1);
    }
    else
#endif
    if (resLocation == EResourceLocation::kAbsolutePath)
    {
      std::ifstream shaderFile(fragmentShader.Get());
      if (!shaderFile.is_open())
      {
        DBGMSG("Failed to open shader file: %s\n", CIRCLE_FRAG_SHADER_FN);
      }

      std::string shaderSource((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
      fragmentShader.Set(shaderSource.c_str());
    }

    GLuint vs = loadShader(vertexShader.Get(), CIRCLE_VERT_SHADER_FN, GL_VERTEX_SHADER);
    GLuint fs = loadShader(fragmentShader.Get(), CIRCLE_FRAG_SHADER_FN, GL_FRAGMENT_SHADER);

    if (vs && fs)
    {
      sProgram = createProgram(vs, fs);
      glDeleteShader(vs);
      glDeleteShader(fs);
    }
  }

  // Render with our shader
  if (sProgram && mVAO)
  {
//    DBGMSG("\n=== Starting Render Pass ===\n");

    glClearColor(mBackgroundColor.R / 255.0, mBackgroundColor.G / 255.0, mBackgroundColor.B / 255.0, mBackgroundColor.A / 255.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(sProgram);
    glBindVertexArray(mVAO);

    GLint radiusLoc = glGetUniformLocation(sProgram, "radius");
    if (radiusLoc != -1)
    {
      auto radius = static_cast<float>(GetValue());
//      DBGMSG("Setting radius to: %f\n", radius);
      glUniform1f(radiusLoc, radius);
    }

    // Set viewport to match our scaled control bounds
    float scale = g.GetTotalScale();
    int scaledW = static_cast<int>(mRECT.W() * scale);
    int scaledH = static_cast<int>(mRECT.H() * scale);
//    DBGMSG("Setting viewport to scaled dimensions: %d x %d (scale: %.1f)\n", scaledW, scaledH, scale);
    glViewport(0, 0, scaledW, scaledH);

    GLenum err;
//    while ((err = glGetError()) != GL_NO_ERROR)
//    {
//      DBGMSG("GL error before draw: 0x%x\n", err);
//    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//    DBGMSG("Drew %d vertices\n", 4);

//    while ((err = glGetError()) != GL_NO_ERROR)
//    {
//      DBGMSG("GL error after draw: 0x%x\n", err);
//    }
  }
//  else
//  {
//    DBGMSG("Not rendering - Program: %d, VAO: %d\n", sProgram, mVAO);
//  }

  glUseProgram(last_program);
  glBindVertexArray(last_vao);
  glBindBuffer(GL_ARRAY_BUFFER, last_vbo);
  glBindTexture(GL_TEXTURE_2D, last_texture);
  glActiveTexture(last_active_texture);
  if (depth_test)
    glEnable(GL_DEPTH_TEST);
  else
    glDisable(GL_DEPTH_TEST);
  if (blend_enabled)
    glEnable(GL_BLEND);
  else
    glDisable(GL_BLEND);
  glBlendFunc(blend_src, blend_dst);
  glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

  nvgBeginFrame(vg, static_cast<float>(g.WindowWidth()), static_cast<float>(g.WindowHeight()), static_cast<float>(g.GetTotalScale()));
  APIBitmap apibmp{reinterpret_cast<NVGframebuffer*>(mFBO)->image, w, h, 1, 1.};
  IBitmap bmp{&apibmp, 1, false};
  nvgEndFrame(vg);
  glBindFramebuffer(GL_FRAMEBUFFER, mInitialFBO);

  g.DrawFittedBitmap(bmp, mRECT);

//  GLint major, minor;
//  glGetIntegerv(GL_MAJOR_VERSION, &major);
//  glGetIntegerv(GL_MINOR_VERSION, &minor);
//  DBGMSG("OpenGL Version: %d.%d\n", major, minor);
//
//  GLint profile;
//  glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
//  DBGMSG("OpenGL Profile: %s\n", (profile & GL_CONTEXT_CORE_PROFILE_BIT) ? "Core" : (profile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) ? "Compatibility" : "Unknown");
}
#endif
