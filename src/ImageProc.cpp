#include "ImageProc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

#include "stb_image.h"

namespace {

const int kMaxImageSide = 1024;
const int kMinSelection = 24;
const float kMinConfidence = 3.0f;
const float kUniformStdDev = 0.05f;
const float kUniformCell = 32.0f;
const float kSubharmonicRatio = 0.3f;
const float kMaxShiftMismatch = 0.6f;
const float kSeamBlendFraction = 0.12f;
const int kMaxKMeansSamples = 20000;
const int kKMeansIterations = 20;
const float kTwoPi = 6.2831853f;

float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

float distanceSq(const Vec3& a, const Vec3& b)
{
    const Vec3 d = a - b;
    return dot(d, d);
}

Vec3 sampleBilinear(const RgbImage& image, float x, float y)
{
    x = clampf(x, 0.0f, image.width - 1.001f);
    y = clampf(y, 0.0f, image.height - 1.001f);
    const int x0 = static_cast<int>(x);
    const int y0 = static_cast<int>(y);
    const float fx = x - x0;
    const float fy = y - y0;
    const Vec3 top = lerp(image.at(x0, y0), image.at(x0 + 1, y0), fx);
    const Vec3 bottom = lerp(image.at(x0, y0 + 1), image.at(x0 + 1, y0 + 1), fx);
    return lerp(top, bottom, fy);
}

float meanOf(const std::vector<float>& v)
{
    float sum = 0.0f;
    for (float x : v) {
        sum += x;
    }
    return v.empty() ? 0.0f : sum / v.size();
}

float stdDevOf(const std::vector<float>& v)
{
    const float m = meanOf(v);
    float sum = 0.0f;
    for (float x : v) {
        sum += (x - m) * (x - m);
    }
    return v.empty() ? 0.0f : std::sqrt(sum / v.size());
}

// Average spread of brightness along each row. Close to zero means every
// row is a single colour, i.e. the cloth does not change across x at all
// (plain cloth, or stripes that run across). A rotated check is not flat
// along its rows, so it is not mistaken for plain cloth.
float meanSpreadAlongRows(const GrayImage& image)
{
    float total = 0.0f;
    std::vector<float> row(image.width);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            row[x] = image.at(x, y);
        }
        total += stdDevOf(row);
    }
    return total / image.height;
}

float meanSpreadAlongColumns(const GrayImage& image)
{
    float total = 0.0f;
    std::vector<float> column(image.height);
    for (int x = 0; x < image.width; ++x) {
        for (int y = 0; y < image.height; ++y) {
            column[y] = image.at(x, y);
        }
        total += stdDevOf(column);
    }
    return total / image.width;
}

// Checks that a detected period really is a repeat of the 2D image, not just
// of its averaged profile. If the cloth repeats every P pixels, shifting it
// by P lines it up with itself, while shifting by P/2 does not:
//   ratio = mean|I(x) - I(x + P)| / mean|I(x) - I(x + P/2)|
// A small ratio confirms the repeat. Rotated cloth fails this test, because
// after the shift the stripes no longer line up. The image is blurred by 2 px
// first so the tiny thread-level twill texture does not disturb the check.
float shiftMismatch(const GrayImage& image, float period, bool alongX)
{
    const int full = static_cast<int>(std::lround(period));
    const int half = static_cast<int>(std::lround(period * 0.5f));
    const int limit = alongX ? image.width : image.height;
    if (full < 2 || full >= limit) {
        return 1.0f;
    }
    double fullDiff = 0.0;
    double halfDiff = 0.0;
    int count = 0;
    for (int y = 0; y < image.height - (alongX ? 0 : full); ++y) {
        for (int x = 0; x < image.width - (alongX ? full : 0); ++x) {
            const float here = image.at(x, y);
            const float atFull = alongX ? image.at(x + full, y) : image.at(x, y + full);
            const float atHalf = alongX ? image.at(x + half, y) : image.at(x, y + half);
            fullDiff += std::fabs(here - atFull);
            halfDiff += std::fabs(here - atHalf);
            ++count;
        }
    }
    if (count == 0 || halfDiff <= 1e-9) {
        return 1.0f;
    }
    return static_cast<float>(fullDiff / halfDiff);
}

Region clampRegion(const Region& r, int width, int height)
{
    Region out;
    out.x = std::max(0, std::min(r.x, width - 1));
    out.y = std::max(0, std::min(r.y, height - 1));
    out.width = std::max(0, std::min(r.width, width - out.x));
    out.height = std::max(0, std::min(r.height, height - out.y));
    return out;
}

} // namespace

namespace ImageProc {

// Large phone photos are shrunk by averaging blocks of pixels, which keeps
// the analysis fast and removes sensor noise at the same time.
bool loadImage(const std::string& path, RgbImage& out, std::string& error)
{
    int w = 0;
    int h = 0;
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 3);
    if (!data) {
        error = std::string("Could not read image: ") + stbi_failure_reason();
        return false;
    }

    int factor = 1;
    while (w / factor > kMaxImageSide || h / factor > kMaxImageSide) {
        ++factor;
    }

    out.width = w / factor;
    out.height = h / factor;
    out.pixels.assign(out.width * out.height, Vec3());
    const float inv = 1.0f / (255.0f * factor * factor);
    for (int y = 0; y < out.height; ++y) {
        for (int x = 0; x < out.width; ++x) {
            Vec3 sum;
            for (int dy = 0; dy < factor; ++dy) {
                for (int dx = 0; dx < factor; ++dx) {
                    const unsigned char* p = data + ((y * factor + dy) * w + (x * factor + dx)) * 3;
                    sum += Vec3(p[0], p[1], p[2]);
                }
            }
            out.pixels[y * out.width + x] = sum * inv;
        }
    }
    stbi_image_free(data);
    return true;
}

RgbImage crop(const RgbImage& image, const Region& region)
{
    RgbImage out;
    out.width = region.width;
    out.height = region.height;
    out.pixels.resize(out.width * out.height);
    for (int y = 0; y < out.height; ++y) {
        for (int x = 0; x < out.width; ++x) {
            out.pixels[y * out.width + x] = image.at(region.x + x, region.y + y);
        }
    }
    return out;
}

// Luminance with the ITU-R BT.601 weights: the eye is most sensitive to
// green and least to blue, so Y = 0.299 R + 0.587 G + 0.114 B.
GrayImage toGrayscale(const RgbImage& image)
{
    GrayImage out;
    out.width = image.width;
    out.height = image.height;
    out.values.resize(image.pixels.size());
    for (size_t i = 0; i < image.pixels.size(); ++i) {
        const Vec3& c = image.pixels[i];
        out.values[i] = 0.299f * c.x + 0.587f * c.y + 0.114f * c.z;
    }
    return out;
}

// Box blur with a summed-area table: S(x, y) holds the sum of all pixels
// above and to the left, so any rectangle sum is S(D) - S(B) - S(C) + S(A),
// four lookups no matter how large the radius is.
GrayImage boxBlur(const GrayImage& image, int radius)
{
    const int w = image.width;
    const int h = image.height;
    std::vector<double> table((w + 1) * (h + 1), 0.0);
    for (int y = 0; y < h; ++y) {
        double rowSum = 0.0;
        for (int x = 0; x < w; ++x) {
            rowSum += image.at(x, y);
            table[(y + 1) * (w + 1) + (x + 1)] = table[y * (w + 1) + (x + 1)] + rowSum;
        }
    }

    GrayImage out;
    out.width = w;
    out.height = h;
    out.values.resize(w * h);
    for (int y = 0; y < h; ++y) {
        const int y0 = std::max(0, y - radius);
        const int y1 = std::min(h, y + radius + 1);
        for (int x = 0; x < w; ++x) {
            const int x0 = std::max(0, x - radius);
            const int x1 = std::min(w, x + radius + 1);
            const double sum = table[y1 * (w + 1) + x1] - table[y0 * (w + 1) + x1]
                             - table[y1 * (w + 1) + x0] + table[y0 * (w + 1) + x0];
            out.values[y * w + x] = static_cast<float>(sum / ((x1 - x0) * (y1 - y0)));
        }
    }
    return out;
}

// Flat-field correction. Uneven light (a shadow, a window on one side) is a
// slow change across the photo, so a heavy blur estimates it. Dividing by
// that estimate and multiplying by the overall mean keeps the weave pattern
// but removes the lighting:  g'(x, y) = g(x, y) * mean(L) / L(x, y)
GrayImage normalizeIllumination(const GrayImage& gray, const GrayImage& illumination)
{
    const float mean = meanOf(illumination.values);
    GrayImage out = gray;
    for (size_t i = 0; i < out.values.size(); ++i) {
        out.values[i] = clampf(gray.values[i] * mean / std::max(0.02f, illumination.values[i]), 0.0f, 1.0f);
    }
    return out;
}

RgbImage normalizeColorIllumination(const RgbImage& image, const GrayImage& illumination)
{
    const float mean = meanOf(illumination.values);
    RgbImage out = image;
    for (size_t i = 0; i < out.pixels.size(); ++i) {
        const Vec3 c = image.pixels[i] * (mean / std::max(0.02f, illumination.values[i]));
        out.pixels[i] = Vec3(clamp01(c.x), clamp01(c.y), clamp01(c.z));
    }
    return out;
}

// Row profile: the average brightness of each row. Horizontal stripes show
// up as a wave in this profile, so it measures the repeat in the vertical
// direction. The column profile does the same for vertical stripes.
std::vector<float> rowProfile(const GrayImage& image)
{
    std::vector<float> profile(image.height, 0.0f);
    for (int y = 0; y < image.height; ++y) {
        float sum = 0.0f;
        for (int x = 0; x < image.width; ++x) {
            sum += image.at(x, y);
        }
        profile[y] = sum / image.width;
    }
    return profile;
}

std::vector<float> columnProfile(const GrayImage& image)
{
    std::vector<float> profile(image.width, 0.0f);
    for (int x = 0; x < image.width; ++x) {
        float sum = 0.0f;
        for (int y = 0; y < image.height; ++y) {
            sum += image.at(x, y);
        }
        profile[x] = sum / image.height;
    }
    return profile;
}

// Discrete Fourier Transform, written out directly (O(N^2) is fine for a
// profile of at most ~1000 samples):
//   X[k] = sum_{n=0}^{N-1} x[n] * (cos(2 pi k n / N) - i sin(2 pi k n / N))
// The mean is removed first so bin 0 does not dominate, and a Hann window
// w[n] = 0.5 - 0.5 cos(2 pi n / (N - 1)) softens the ends of the profile so
// the cut-off at the selection border does not create false frequencies.
// Returns |X[k]| for k = 0 .. N/2.
std::vector<float> dftMagnitude(const std::vector<float>& signal)
{
    const int n = static_cast<int>(signal.size());
    const float mean = meanOf(signal);
    std::vector<float> windowed(n);
    for (int i = 0; i < n; ++i) {
        const float w = n > 1 ? 0.5f - 0.5f * std::cos(kTwoPi * i / (n - 1)) : 1.0f;
        windowed[i] = (signal[i] - mean) * w;
    }

    std::vector<float> magnitude(n / 2 + 1, 0.0f);
    for (int k = 0; k <= n / 2; ++k) {
        double re = 0.0;
        double im = 0.0;
        for (int i = 0; i < n; ++i) {
            const double angle = kTwoPi * static_cast<double>(k) * i / n;
            re += windowed[i] * std::cos(angle);
            im -= windowed[i] * std::sin(angle);
        }
        magnitude[k] = static_cast<float>(std::sqrt(re * re + im * im));
    }
    return magnitude;
}

// A pattern that repeats every P pixels across N pixels makes a peak at bin
// k = N / P. Bins below 2 (less than two repeats visible) and above N / 4
// (repeats shorter than 4 pixels, i.e. thread noise) are ignored.
// Checked cloth often has a stronger 2nd or 3rd harmonic than its
// fundamental, so after finding the tallest peak k we also accept k/2, k/3
// or k/4 if that bin still holds at least 30% of the peak: the longest
// such period is the true repeat. Parabolic interpolation around the
// chosen bin gives a sub-pixel period:  delta = 0.5 (a - c) / (a - 2b + c)
PeriodEstimate estimatePeriod(const std::vector<float>& profile)
{
    PeriodEstimate result;
    const int n = static_cast<int>(profile.size());
    result.spectrum = dftMagnitude(profile);

    const int kMin = 2;
    const int kMax = n / 4;
    if (kMax <= kMin) {
        return result;
    }

    int peak = kMin;
    float sum = 0.0f;
    for (int k = kMin; k <= kMax; ++k) {
        sum += result.spectrum[k];
        if (result.spectrum[k] > result.spectrum[peak]) {
            peak = k;
        }
    }
    const float average = sum / (kMax - kMin + 1);

    int chosen = peak;
    for (int divisor = 2; divisor <= 4; ++divisor) {
        const int candidate = static_cast<int>(std::lround(static_cast<float>(peak) / divisor));
        if (candidate < kMin) {
            break;
        }
        float best = 0.0f;
        int bestBin = candidate;
        for (int k = std::max(kMin, candidate - 1); k <= std::min(kMax, candidate + 1); ++k) {
            if (result.spectrum[k] > best) {
                best = result.spectrum[k];
                bestBin = k;
            }
        }
        if (best >= kSubharmonicRatio * result.spectrum[peak] && best >= kMinConfidence * average) {
            chosen = bestBin;
        }
    }

    float offset = 0.0f;
    if (chosen > kMin && chosen < kMax) {
        const float a = result.spectrum[chosen - 1];
        const float b = result.spectrum[chosen];
        const float c = result.spectrum[chosen + 1];
        const float denominator = a - 2.0f * b + c;
        if (std::fabs(denominator) > 1e-6f) {
            offset = clampf(0.5f * (a - c) / denominator, -0.5f, 0.5f);
        }
    }

    result.peakBin = chosen;
    result.period = n / (chosen + offset);
    result.confidence = average > 0.0f ? result.spectrum[peak] / average : 0.0f;
    result.found = result.confidence >= kMinConfidence;
    return result;
}

// K-means in RGB space. Start: k-means++ (each new centre is picked with
// probability proportional to its squared distance from the nearest
// existing centre, which spreads the centres out). Then repeat:
//   1. assign every pixel to its nearest centre,
//   2. move every centre to the mean of its pixels.
// error = sum of squared distances to the assigned centre (SSE).
std::vector<Vec3> kMeansColors(const RgbImage& image, int k, std::vector<float>& share, float& error)
{
    std::vector<Vec3> samples;
    const int stride = std::max(1, static_cast<int>(image.pixels.size()) / kMaxKMeansSamples);
    for (size_t i = 0; i < image.pixels.size(); i += stride) {
        samples.push_back(image.pixels[i]);
    }

    std::mt19937 rng(7u);
    std::vector<Vec3> centers;
    centers.push_back(samples[samples.size() / 2]);
    std::vector<float> nearest(samples.size());
    while (static_cast<int>(centers.size()) < k) {
        double total = 0.0;
        for (size_t i = 0; i < samples.size(); ++i) {
            float best = 1e9f;
            for (const Vec3& c : centers) {
                best = std::min(best, distanceSq(samples[i], c));
            }
            nearest[i] = best;
            total += best;
        }
        std::uniform_real_distribution<double> pick(0.0, total);
        double target = pick(rng);
        size_t chosen = 0;
        for (size_t i = 0; i < samples.size(); ++i) {
            target -= nearest[i];
            if (target <= 0.0) {
                chosen = i;
                break;
            }
        }
        centers.push_back(samples[chosen]);
    }

    std::vector<int> label(samples.size(), 0);
    std::vector<int> count(k, 0);
    for (int iteration = 0; iteration < kKMeansIterations; ++iteration) {
        for (size_t i = 0; i < samples.size(); ++i) {
            float best = 1e9f;
            for (int c = 0; c < k; ++c) {
                const float d = distanceSq(samples[i], centers[c]);
                if (d < best) {
                    best = d;
                    label[i] = c;
                }
            }
        }
        std::vector<Vec3> sums(k);
        std::fill(count.begin(), count.end(), 0);
        for (size_t i = 0; i < samples.size(); ++i) {
            sums[label[i]] += samples[i];
            ++count[label[i]];
        }
        for (int c = 0; c < k; ++c) {
            if (count[c] > 0) {
                centers[c] = sums[c] / static_cast<float>(count[c]);
            }
        }
    }

    error = 0.0f;
    for (size_t i = 0; i < samples.size(); ++i) {
        error += distanceSq(samples[i], centers[label[i]]);
    }

    std::vector<int> order(k);
    for (int c = 0; c < k; ++c) {
        order[c] = c;
    }
    std::sort(order.begin(), order.end(), [&count](int a, int b) { return count[a] > count[b]; });

    std::vector<Vec3> sorted;
    share.clear();
    for (int c : order) {
        sorted.push_back(centers[c]);
        share.push_back(static_cast<float>(count[c]) / samples.size());
    }
    return sorted;
}

// Elbow rule: keep adding colours while one more cluster still cuts the
// error by at least 30%. Two to four colours covers almost every lungi.
int chooseColorCount(const RgbImage& image)
{
    std::vector<float> share;
    float previous = 0.0f;
    kMeansColors(image, 2, share, previous);
    int k = 2;
    while (k < 4) {
        float next = 0.0f;
        kMeansColors(image, k + 1, share, next);
        if (next > 0.7f * previous) {
            break;
        }
        previous = next;
        ++k;
    }
    return k;
}

// One repeat cell (cellWidth x cellHeight pixels) is resampled into a square
// tile. To hide the seam, the first b texels of each edge are cross-faded
// with the cloth that comes one full period later, which is exactly what the
// opposite edge of the tile will meet when the tile repeats:
//   c(i) = lerp(S(u + cellWidth), S(u), i / b)   for i < b
// Each texel is then snapped to the nearest palette colour, which removes
// photo noise and gives clean woven colours.
std::vector<unsigned char> buildSeamlessTile(const RgbImage& image, float cellWidth, float cellHeight,
                                             const std::vector<Vec3>& palette, int size)
{
    const int blend = std::max(1, static_cast<int>(size * kSeamBlendFraction));
    std::vector<unsigned char> tile(size * size * 3);

    for (int j = 0; j < size; ++j) {
        const float v = (j + 0.5f) * cellHeight / size;
        const float ty = j < blend ? static_cast<float>(j) / blend : 1.0f;
        for (int i = 0; i < size; ++i) {
            const float u = (i + 0.5f) * cellWidth / size;
            const float tx = i < blend ? static_cast<float>(i) / blend : 1.0f;

            const Vec3 a = lerp(sampleBilinear(image, u + cellWidth, v), sampleBilinear(image, u, v), tx);
            const Vec3 b = lerp(sampleBilinear(image, u + cellWidth, v + cellHeight),
                                sampleBilinear(image, u, v + cellHeight), tx);
            const Vec3 sample = lerp(b, a, ty);

            Vec3 c = sample;
            float best = 1e9f;
            for (const Vec3& p : palette) {
                const float d = distanceSq(sample, p);
                if (d < best) {
                    best = d;
                    c = p;
                }
            }
            if ((i + j) % 2 == 1) {
                c *= 0.94f;
            }

            const int index = (j * size + i) * 3;
            tile[index + 0] = static_cast<unsigned char>(clamp01(c.x) * 255.0f);
            tile[index + 1] = static_cast<unsigned char>(clamp01(c.y) * 255.0f);
            tile[index + 2] = static_cast<unsigned char>(clamp01(c.z) * 255.0f);
        }
    }
    return tile;
}

FabricAnalysis analyze(const RgbImage& image, const Region& requested, int colorCount,
                       float periodXOverride, float periodYOverride)
{
    FabricAnalysis result;
    if (image.empty()) {
        result.message = "No photo loaded. Press O to open one.";
        return result;
    }
    const Region region = clampRegion(requested, image.width, image.height);
    if (region.width < kMinSelection || region.height < kMinSelection) {
        result.message = "Selection too small. Drag a larger box over the fabric.";
        return result;
    }

    const RgbImage selection = crop(image, region);
    const GrayImage gray = toGrayscale(selection);
    const GrayImage illumination = boxBlur(gray, std::max(region.width, region.height) / 4);
    result.normalized = normalizeIllumination(gray, illumination);
    const RgbImage flatColor = normalizeColorIllumination(selection, illumination);

    result.rowProfile = rowProfile(result.normalized);
    result.columnProfile = columnProfile(result.normalized);
    result.horizontal = estimatePeriod(result.columnProfile);
    result.vertical = estimatePeriod(result.rowProfile);
    result.horizontal.uniform = meanSpreadAlongRows(result.normalized) < kUniformStdDev;
    result.vertical.uniform = meanSpreadAlongColumns(result.normalized) < kUniformStdDev;
    const GrayImage patternScale = boxBlur(result.normalized, 2);
    if (result.horizontal.uniform ||
        (result.horizontal.found && shiftMismatch(patternScale, result.horizontal.period, true) > kMaxShiftMismatch)) {
        result.horizontal.found = false;
    }
    if (result.vertical.uniform ||
        (result.vertical.found && shiftMismatch(patternScale, result.vertical.period, false) > kMaxShiftMismatch)) {
        result.vertical.found = false;
    }

    auto pickCell = [](const PeriodEstimate& e, float overrideValue, float fallback) {
        if (overrideValue > 0.0f) {
            return overrideValue;
        }
        if (e.found) {
            return e.period;
        }
        return e.uniform ? kUniformCell : fallback;
    };
    result.cellWidth = std::min(pickCell(result.horizontal, periodXOverride, region.width * 0.5f), region.width * 0.5f);
    result.cellHeight = std::min(pickCell(result.vertical, periodYOverride, region.height * 0.5f), region.height * 0.5f);

    const int k = colorCount >= 2 ? colorCount : chooseColorCount(flatColor);
    float error = 0.0f;
    result.palette = kMeansColors(flatColor, k, result.paletteShare, error);

    result.tile = buildSeamlessTile(flatColor, result.cellWidth, result.cellHeight, result.palette, result.tileSize);
    Vec3 sum;
    for (size_t i = 0; i < result.palette.size(); ++i) {
        sum += result.palette[i] * result.paletteShare[i];
    }
    result.mainColor = sum;

    const bool xOk = result.horizontal.found || result.horizontal.uniform || periodXOverride > 0.0f;
    const bool yOk = result.vertical.found || result.vertical.uniform || periodYOverride > 0.0f;
    char text[160];
    if (xOk && yOk) {
        std::snprintf(text, sizeof(text), "Repeat %.0f x %.0f px, %d colours. Press ENTER to wear it!",
                      result.cellWidth, result.cellHeight, k);
        result.ok = true;
    } else {
        std::snprintf(text, sizeof(text), "No clear repeat %s. Set it with the arrow keys, or select a flatter area.",
                      !xOk && !yOk ? "found" : (!xOk ? "across" : "down"));
        result.ok = false;
    }
    result.message = text;
    return result;
}

} // namespace ImageProc
