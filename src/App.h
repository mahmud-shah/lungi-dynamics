#pragma once

#include "FpsCounter.h"

// The only global mutable state in the program. GLUT callbacks are plain C
// functions with no user pointer, so they need one place to reach shared data.
// Everything else lives inside the objects this struct owns.
struct AppState {
    int windowWidth = 1280;
    int windowHeight = 720;
    FpsCounter fps;
};
