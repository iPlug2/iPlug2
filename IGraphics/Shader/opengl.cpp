#include "opengl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

#if !defined(GL_STACK_OVERFLOW)
#  define GL_STACK_OVERFLOW 0x0503
#endif

#if !defined(GL_STACK_UNDERFLOW)
#  define GL_STACK_UNDERFLOW 0x0504
#endif

bool nanogui_check_glerror(const char *cmd) {
    GLenum err = glGetError();
    const char *msg = nullptr;

    switch (err) {
        case GL_NO_ERROR:
            // printf("OK: %s\n", cmd);
            return false;

        case GL_INVALID_ENUM:
            msg = "invalid enumeration";
            break;

        case GL_INVALID_VALUE:
            msg = "invalid value";
            break;

        case GL_INVALID_OPERATION:
            msg = "invalid operation";
            break;

        case GL_INVALID_FRAMEBUFFER_OPERATION:
            msg = "invalid framebuffer operation";
            break;

        case GL_OUT_OF_MEMORY:
            msg = "out of memory";
            break;

        case GL_STACK_UNDERFLOW:
            msg = "stack underflow";
            break;

        case GL_STACK_OVERFLOW:
            msg = "stack overflow";
            break;

        default:
            msg = "unknown error";
            break;
    }

    fprintf(stderr, "OpenGL error (%s) during operation \"%s\"!\n", msg, cmd);
    return true;
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
