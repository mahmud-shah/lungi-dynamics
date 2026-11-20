#pragma once

#include <string>
#include <vector>

#include "ImageProc.h"

class PhotoLab {
public:
    void create();
    bool loadPhoto(const std::string& path);
    bool hasPhoto() const { return !photo_.empty(); }

    void draw(int width, int height);

    void mouseDown(int x, int y);
    void mouseDrag(int x, int y);
    void mouseUp(int x, int y);

    void cycleColorCount();
    void adjustPeriod(float dx, float dy);
    void resetToAuto();

    bool canApply() const { return analysis_.ok; }
    const FabricAnalysis& analysis() const { return analysis_; }
    const std::string& status() const { return status_; }

private:
    struct Rect {
        float x, y, w, h;
    };

    void analyze();
    void uploadPhotoTexture();
    void uploadPreviewTextures();
    bool screenToImage(int sx, int sy, int& ix, int& iy) const;

    void drawImagePanel();
    void drawSteps(int width, int height);
    void drawTexturedQuad(unsigned int texture, const Rect& r, float repeat) const;
    void drawGraph(const std::vector<float>& values, const Rect& r, int markIndex, const char* title) const;

    RgbImage photo_;
    std::string fileName_;
    std::string status_ = "Press O to open a photo of flat lungi fabric.";
    FabricAnalysis analysis_;
    Region selection_;

    int colorChoice_ = 0;
    float overrideX_ = 0.0f;
    float overrideY_ = 0.0f;

    bool dragging_ = false;
    int dragStartX_ = 0;
    int dragStartY_ = 0;

    int windowHeight_ = 720;
    Rect imageRect_ = {0, 0, 0, 0};

    unsigned int photoTexture_ = 0;
    unsigned int grayTexture_ = 0;
    unsigned int tileTexture_ = 0;
    std::vector<unsigned char> scratch_;
};
