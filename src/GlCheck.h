#pragma once

#include <GL/freeglut.h>
#include <cstdio>

// OpenGL reports errors through a flag queue instead of return values, so a
// bad call fails silently unless someone asks. Calling this after each block
// of GL work tells us which block caused the error. Compiled out in Release
// (NDEBUG) so it costs nothing in FPS measurements.
inline void glCheck(const char* where)
{
#ifndef NDEBUG
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        std::fprintf(stderr, "[GL] error 0x%04X after %s\n",
                     static_cast<unsigned>(err), where);
        err = glGetError();
    }
#else
    (void)where;
#endif
}
