#pragma once

#include <GL/freeglut.h>
#include <cstdio>

inline void glCheck(const char* where)
{
#ifndef NDEBUG
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        std::fprintf(stderr, "[GL] error 0x%04X after %s\n", static_cast<unsigned>(err), where);
        err = glGetError();
    }
#else
    (void)where;
#endif
}
