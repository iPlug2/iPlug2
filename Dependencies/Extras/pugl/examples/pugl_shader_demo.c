// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  An example of drawing with OpenGL 3/4.

  This is an example of using OpenGL for pixel-perfect 2D drawing.  It uses
  pixel coordinates for positions and sizes so that things work roughly like a
  typical 2D graphics API.

  The program draws a bunch of rectangles with borders, using instancing.
  Each rectangle has origin, size, and fill color attributes, which are shared
  for all four vertices.  On each frame, a single buffer with all the
  rectangle data is sent to the GPU, and everything is drawn with a single
  draw call.

  This is not particularly realistic or optimal, but serves as a decent rough
  benchmark for how much simple geometry you can draw.  The number of
  rectangles can be given on the command line.  For reference, it begins to
  struggle to maintain 60 FPS on my machine (1950x + Vega64) with more than
  about 100000 rectangles.
*/

#include "demo_utils.h"
#include "file_utils.h"
#include "rects.h"
#include "shader_utils.h"
#include "test/test_utils.h"

#include "glad/glad.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#  define SHADER_DIR "../"
#else
#  define SHADER_DIR "shaders/"
#endif

static const PuglSpan  defaultSpan   = 512;
static const uintptr_t resizeTimerId = 1U;

typedef struct {
  const char*     programPath;
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  size_t          numRects;
  Rect*           rects;
  Program         drawRect;
  GLuint          vao;
  GLuint          vbo;
  GLuint          instanceVbo;
  GLuint          ibo;
  double          lastDrawDuration;
  double          lastFrameEndTime;
  double          mouseX;
  double          mouseY;
  unsigned        framesDrawn;
  int             quit;
} PuglTestApp;

static PuglStatus
setupGl(PuglTestApp* app);

static void
teardownGl(PuglTestApp* app);

static void
onConfigure(PuglView* view, double width, double height)
{
  (void)view;

  glEnable(GL_BLEND);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
  glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glViewport(0, 0, (int)width, (int)height);
}

static void
onExpose(PuglView* view)
{
  PuglTestApp*   app    = (PuglTestApp*)puglGetHandle(view);
  const PuglRect frame  = puglGetFrame(view);
  const float    width  = (float)frame.width;
  const float    height = (float)frame.height;
  const double   time   = puglGetTime(puglGetWorld(view));

  // Construct projection matrix for 2D window surface (in pixels)
  mat4 proj;
  mat4Ortho(
    proj, 0.0f, (float)frame.width, 0.0f, (float)frame.height, -1.0f, 1.0f);

  // Clear and bind everything that is the same for every rect
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(app->drawRect.program);
  glBindVertexArray(app->vao);

  // Update horizontal mouse cursor line (last rect)
  Rect* const mouseH   = &app->rects[app->numRects];
  mouseH->pos[0]       = (float)(app->mouseX - 8.0);
  mouseH->pos[1]       = (float)(frame.height - app->mouseY - 1.0);
  mouseH->size[0]      = 16.0f;
  mouseH->size[1]      = 2.0f;
  mouseH->fillColor[0] = 1.0f;
  mouseH->fillColor[1] = 1.0f;
  mouseH->fillColor[2] = 1.0f;
  mouseH->fillColor[3] = 0.5f;

  // Update vertical mouse cursor line (second last rect)
  Rect* const mouseV   = &app->rects[app->numRects + 1];
  mouseV->pos[0]       = (float)(app->mouseX - 2.0);
  mouseV->pos[1]       = (float)(frame.height - app->mouseY - 8.0);
  mouseV->size[0]      = 2.0f;
  mouseV->size[1]      = 16.0f;
  mouseV->fillColor[0] = 1.0f;
  mouseV->fillColor[1] = 1.0f;
  mouseV->fillColor[2] = 1.0f;
  mouseV->fillColor[3] = 0.5f;

  for (size_t i = 0; i < app->numRects; ++i) {
    moveRect(&app->rects[i], i, app->numRects, width, height, time);
  }

  glBufferData(GL_UNIFORM_BUFFER, sizeof(proj), &proj, GL_STREAM_DRAW);

  glBufferSubData(GL_ARRAY_BUFFER,
                  0,
                  (GLsizeiptr)((app->numRects + 2) * sizeof(Rect)),
                  app->rects);

  glDrawElementsInstanced(GL_TRIANGLE_STRIP,
                          4,
                          GL_UNSIGNED_INT,
                          NULL,
                          (GLsizei)((app->numRects + 2) * 4));

  ++app->framesDrawn;

  app->lastFrameEndTime = puglGetTime(puglGetWorld(view));
  app->lastDrawDuration = app->lastFrameEndTime - time;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  printEvent(event, "Event: ", app->opts.verbose);

  switch (event->type) {
  case PUGL_REALIZE:
    setupGl(app);
    break;
  case PUGL_UNREALIZE:
    teardownGl(app);
    break;
  case PUGL_CONFIGURE:
    onConfigure(view, event->configure.width, event->configure.height);
    break;
  case PUGL_UPDATE:
    puglPostRedisplay(view);
    break;
  case PUGL_EXPOSE:
    onExpose(view);
    break;
  case PUGL_CLOSE:
    app->quit = 1;
    break;
  case PUGL_LOOP_ENTER:
    puglStartTimer(view,
                   resizeTimerId,
                   1.0 / (double)puglGetViewHint(view, PUGL_REFRESH_RATE));
    break;
  case PUGL_LOOP_LEAVE:
    puglStopTimer(view, resizeTimerId);
    break;
  case PUGL_KEY_PRESS:
    if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE) {
      app->quit = 1;
    }
    break;
  case PUGL_MOTION:
    app->mouseX = event->motion.x;
    app->mouseY = event->motion.y;
    break;
  case PUGL_TIMER:
    if (event->timer.id == resizeTimerId) {
      puglPostRedisplay(view);
    }
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

static Rect*
makeRects(const size_t numRects)
{
  Rect* rects = (Rect*)calloc(numRects, sizeof(Rect));
  for (size_t i = 0; i < numRects; ++i) {
    rects[i] = makeRect(i, defaultSpan);
  }

  return rects;
}

static char*
loadShader(const char* const programPath, const char* const name)
{
  char* const path = resourcePath(programPath, name);
  fprintf(stderr, "Loading shader %s\n", path);

  FILE* const file = fopen(path, "rb");
  if (!file) {
    logError("Failed to open '%s'\n", path);
    return NULL;
  }

  free(path);
  fseek(file, 0, SEEK_END);
  const size_t fileSize = (size_t)ftell(file);

  fseek(file, 0, SEEK_SET);
  char* source = (char*)calloc(1, fileSize + 1U);

  if (fread(source, 1, fileSize, file) != fileSize) {
    free(source);
    source = NULL;
  }

  fclose(file);

  return source;
}

static int
parseOptions(PuglTestApp* app, int argc, char** argv)
{
  char* endptr = NULL;

  // Parse command line options
  app->numRects = 1024;
  app->opts     = puglParseTestOptions(&argc, &argv);
  if (app->opts.help) {
    return 1;
  }

  // Parse number of rectangles argument, if given
  if (argc >= 1) {
    app->numRects = (size_t)strtol(argv[0], &endptr, 10);
    if (endptr != argv[0] + strlen(argv[0])) {
      logError("Invalid number of rectangles: %s\n", argv[0]);
      return 1;
    }
  }

  return 0;
}

static void
setupPugl(PuglTestApp* app)
{
  // Create world, view, and rect data
  app->world = puglNewWorld(PUGL_PROGRAM, 0);
  app->view  = puglNewView(app->world);
  app->rects = makeRects(app->numRects + 2);

  // Set up world and view
  puglSetWorldString(app->world, PUGL_CLASS_NAME, "PuglShaderDemo");
  puglSetViewString(app->view, PUGL_WINDOW_TITLE, "Pugl OpenGL Shader Demo");
  puglSetSizeHint(app->view, PUGL_DEFAULT_SIZE, defaultSpan, defaultSpan);
  puglSetSizeHint(app->view, PUGL_MIN_SIZE, 128, 128);
  puglSetSizeHint(app->view, PUGL_MAX_SIZE, 2048, 2048);
  puglSetSizeHint(app->view, PUGL_MIN_ASPECT, 1, 1);
  puglSetSizeHint(app->view, PUGL_MAX_ASPECT, 16, 9);
  puglSetBackend(app->view, puglGlBackend());
  puglSetViewHint(app->view, PUGL_CONTEXT_API, app->opts.glApi);
  puglSetViewHint(
    app->view, PUGL_CONTEXT_VERSION_MAJOR, app->opts.glMajorVersion);
  puglSetViewHint(
    app->view, PUGL_CONTEXT_VERSION_MINOR, app->opts.glMinorVersion);
  puglSetViewHint(app->view, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_CORE_PROFILE);
  puglSetViewHint(app->view, PUGL_CONTEXT_DEBUG, app->opts.errorChecking);
  puglSetViewHint(app->view, PUGL_RESIZABLE, app->opts.resizable);
  puglSetViewHint(app->view, PUGL_SAMPLES, app->opts.samples);
  puglSetViewHint(app->view, PUGL_DOUBLE_BUFFER, app->opts.doubleBuffer);
  puglSetViewHint(app->view, PUGL_SWAP_INTERVAL, app->opts.sync);
  puglSetViewHint(app->view, PUGL_IGNORE_KEY_REPEAT, PUGL_TRUE);
  puglSetViewHint(app->view, PUGL_DARK_FRAME, PUGL_TRUE);
  puglSetHandle(app->view, app);
  puglSetEventFunc(app->view, onEvent);
}

static PuglStatus
setupGl(PuglTestApp* app)
{
  // Load GL and determine the shader header to load
  const char* headerFile = NULL;
  if (app->opts.glApi == PUGL_OPENGL_API) {
    if (!gladLoadGLLoader((GLADloadproc)&puglGetProcAddress)) {
      logError("Failed to load OpenGL\n");
      return PUGL_FAILURE;
    }

    headerFile =
      (app->opts.glMajorVersion == 3 && app->opts.glMinorVersion == 3)
        ? (SHADER_DIR "header_330.glsl")
      : (app->opts.glMajorVersion == 4 && app->opts.glMinorVersion == 2)
        ? (SHADER_DIR "header_420.glsl")
        : NULL;

  } else if (app->opts.glApi == PUGL_OPENGL_ES_API) {
    if (!gladLoadGLES2Loader((GLADloadproc)&puglGetProcAddress)) {
      logError("Failed to load OpenGL ES\n");
      return PUGL_FAILURE;
    }

    headerFile =
      (app->opts.glMajorVersion == 3 && app->opts.glMinorVersion == 2)
        ? (SHADER_DIR "header_320_es.glsl")
        : NULL;

  } else {
    logError("Unsupported API\n");
    return PUGL_FAILURE;
  }

  if (!headerFile) {
    logError("Unsupported OpenGL version\n");
    return PUGL_FAILURE;
  }

  // Load shader sources
  char* const headerSource = loadShader(app->programPath, headerFile);

  char* const vertexSource =
    loadShader(app->programPath, SHADER_DIR "rect.vert");

  char* const fragmentSource =
    loadShader(app->programPath, SHADER_DIR "rect.frag");

  if (!vertexSource || !fragmentSource) {
    logError("Failed to load shader sources\n");
    return PUGL_FAILURE;
  }

  // Compile rectangle shaders and program
  app->drawRect = compileProgram(headerSource, vertexSource, fragmentSource);
  free(fragmentSource);
  free(vertexSource);
  free(headerSource);
  if (!app->drawRect.program) {
    return PUGL_FAILURE;
  }

  // Get location of rectangle shader uniform block
  const GLuint globalsIndex =
    glGetUniformBlockIndex(app->drawRect.program, "UniformBufferObject");

  // Generate/bind a uniform buffer for setting rectangle properties
  GLuint uboHandle = 0;
  glGenBuffers(1, &uboHandle);
  glBindBuffer(GL_UNIFORM_BUFFER, uboHandle);
  glBindBufferBase(GL_UNIFORM_BUFFER, globalsIndex, uboHandle);

  // Generate/bind a VAO to track state
  glGenVertexArrays(1, &app->vao);
  glBindVertexArray(app->vao);

  // Generate/bind a VBO to store vertex position data
  glGenBuffers(1, &app->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, app->vbo);
  glBufferData(
    GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

  // Attribute 0 is position, 2 floats from the VBO
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);

  // Generate/bind a VBO to store instance attribute data
  glGenBuffers(1, &app->instanceVbo);
  glBindBuffer(GL_ARRAY_BUFFER, app->instanceVbo);
  glBufferData(GL_ARRAY_BUFFER,
               (GLsizeiptr)((app->numRects + 2) * sizeof(Rect)),
               app->rects,
               GL_STREAM_DRAW);

  // Attribute 1 is Rect::position
  glEnableVertexAttribArray(1);
  glVertexAttribDivisor(1, 4);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Rect), NULL);

  // Attribute 2 is Rect::size
  glEnableVertexAttribArray(2);
  glVertexAttribDivisor(2, 4);
  glVertexAttribPointer(
    2, 2, GL_FLOAT, GL_FALSE, sizeof(Rect), (const void*)offsetof(Rect, size));

  // Attribute 3 is Rect::fillColor
  glEnableVertexAttribArray(3);
  glVertexAttribDivisor(3, 4);
  glVertexAttribPointer(3,
                        4,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(Rect),
                        (const void*)offsetof(Rect, fillColor));

  // Set up the IBO to index into the VBO
  glGenBuffers(1, &app->ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->ibo);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices, GL_STATIC_DRAW);

  return PUGL_SUCCESS;
}

static void
teardownGl(PuglTestApp* app)
{
  glDeleteBuffers(1, &app->ibo);
  glDeleteBuffers(1, &app->vbo);
  glDeleteBuffers(1, &app->instanceVbo);
  glDeleteVertexArrays(1, &app->vao);
  deleteProgram(app->drawRect);
}

static double
updateTimeout(const PuglTestApp* const app)
{
  if (!puglGetVisible(app->view)) {
    return -1.0; // View is invisible (minimized), wait until something happens
  }

  if (!app->opts.sync) {
    return 0.0; // VSync explicitly disabled, run as fast as possible
  }

  /* To minimize input latency and get smooth performance during window
     resizing, we want to poll for events as long as possible before starting
     to draw the next frame.  This ensures that as many events are consumed as
     possible before starting to draw, or, equivalently, that the next rendered
     frame represents the latest events possible.  This is particularly
     important for mouse input and "live" window resizing, where many events
     tend to pile up within a frame.

     To do this, we keep track of the time when the last frame was finished
     drawing, and how long it took to expose (and assume this is relatively
     stable).  Then, we can calculate how much time there is from now until the
     time when we should start drawing to not miss the deadline, and use that
     as the timeout for puglUpdate().
  */

  const int    refreshRate      = puglGetViewHint(app->view, PUGL_REFRESH_RATE);
  const double now              = puglGetTime(app->world);
  const double nextFrameEndTime = app->lastFrameEndTime + (1.0 / refreshRate);
  const double neededTime       = 1.5 * app->lastDrawDuration;
  const double nextExposeTime   = nextFrameEndTime - neededTime;
  const double timeUntilNext    = nextExposeTime - now;

  return timeUntilNext;
}

int
main(int argc, char** argv)
{
  PuglTestApp app = {0};

  app.programPath         = argv[0];
  app.opts.glMajorVersion = 3;
  app.opts.glMinorVersion = 3;

  // Parse command line options
  if (parseOptions(&app, argc, argv)) {
    puglPrintTestUsage("pugl_shader_demo", "[NUM_RECTS] [GL_MAJOR]");
    return 1;
  }

  // Create and configure world and view
  setupPugl(&app);

  // Realize window (which will send a PUGL_REALIZE event)
  const PuglStatus st = puglRealize(app.view);
  if (st) {
    return logError("Failed to create window (%s)\n", puglStrerror(st));
  }

  // Show window
  printViewHints(app.view);
  puglShow(app.view, PUGL_SHOW_RAISE);

  // Grind away, drawing continuously
  const double   startTime  = puglGetTime(app.world);
  PuglFpsPrinter fpsPrinter = {startTime};
  while (!app.quit) {
    puglUpdate(app.world, fmax(0.0, updateTimeout(&app)));
    puglPrintFps(app.world, &fpsPrinter, &app.framesDrawn);
  }

  // Destroy window (which will send a PUGL_UNREALIZE event)
  puglFreeView(app.view);

  // Free everything else
  puglFreeWorld(app.world);
  free(app.rects);

  return 0;
}
