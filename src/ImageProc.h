#pragma once

#include <string>
#include <vector>

#include "Vec3.h"

struct RgbImage {
    int width = 0;
    int height = 0;
    std::vector<Vec3> pixels;

    bool empty() const { return pixels.empty(); }
    const Vec3& at(int x, int y) const { return pixels[y * width + x]; }
};

struct GrayImage {
    int width = 0;
    int height = 0;
    std::vector<float> values;

    float at(int x, int y) const { return values[y * width + x]; }
};

struct Region {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct PeriodEstimate {
    std::vector<float> spectrum;
    int peakBin = 0;
    float period = 0.0f;
    float confidence = 0.0f;
    bool found = false;
    bool uniform = false;
};

struct FabricAnalysis {
    bool ok = false;
    std::string message;

    GrayImage normalized;
    std::vector<float> rowProfile;
    std::vector<float> columnProfile;
    PeriodEstimate horizontal;
    PeriodEstimate vertical;
    float cellWidth = 0.0f;
    float cellHeight = 0.0f;

    std::vector<Vec3> palette;
    std::vector<float> paletteShare;

    int tileSize = 128;
    std::vector<unsigned char> tile;
    Vec3 mainColor;
};

namespace ImageProc {

bool loadImage(const std::string& path, RgbImage& out, std::string& error);

RgbImage crop(const RgbImage& image, const Region& region);
GrayImage toGrayscale(const RgbImage& image);
GrayImage boxBlur(const GrayImage& image, int radius);
GrayImage normalizeIllumination(const GrayImage& gray, const GrayImage& illumination);
RgbImage normalizeColorIllumination(const RgbImage& image, const GrayImage& illumination);

std::vector<float> rowProfile(const GrayImage& image);
std::vector<float> columnProfile(const GrayImage& image);

std::vector<float> dftMagnitude(const std::vector<float>& signal);
PeriodEstimate estimatePeriod(const std::vector<float>& profile);

std::vector<Vec3> kMeansColors(const RgbImage& image, int k, std::vector<float>& share, float& error);
int chooseColorCount(const RgbImage& image);

std::vector<unsigned char> buildSeamlessTile(const RgbImage& image, float cellWidth, float cellHeight,
                                             const std::vector<Vec3>& palette, int size);

FabricAnalysis analyze(const RgbImage& image, const Region& region, int colorCount,
                       float periodXOverride, float periodYOverride);

} // namespace ImageProc
