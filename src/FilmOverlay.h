#pragma once

// A soft vignette and a light film grain drawn over the finished frame, so
// the picture feels like a painted, filmed animation frame.
class FilmOverlay {
public:
    void create();
    void draw(int width, int height, float storm, float flash);

private:
    unsigned int grainTexture_ = 0;
    unsigned int frame_ = 0;
};
