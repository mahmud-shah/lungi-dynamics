#include "PhotoLab.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

const int kPhotoTextureSize = 512;
const int kGrayTextureSize = 256;
const int kMinDrag = 8;

void* const kSmall = GLUT_BITMAP_HELVETICA_12;
void* const kMedium = GLUT_BITMAP_HELVETICA_18;

void text(float x, float y, const char* s, void* font = kSmall)
{
    glRasterPos2f(x, y);
    glutBitmapString(font, reinterpret_cast<const unsigned char*>(s));
}

void rect(float x, float y, float w, float h, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void outline(float x, float y, float w, float h)
{
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

unsigned int makeTexture()
{
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

unsigned char toByte(float v)
{
    return static_cast<unsigned char>(clamp01(v) * 255.0f);
}

} // namespace

void PhotoLab::create()
{
    photoTexture_ = makeTexture();
    grayTexture_ = makeTexture();
    tileTexture_ = makeTexture();
}

bool PhotoLab::loadPhoto(const std::string& path)
{
    std::string error;
    RgbImage loaded;
    if (!ImageProc::loadImage(path, loaded, error)) {
        status_ = error + " (use a JPG, PNG or BMP with an English file path)";
        return false;
    }
    photo_ = loaded;
    const size_t slash = path.find_last_of("/\\");
    fileName_ = slash == std::string::npos ? path : path.substr(slash + 1);

    selection_.width = photo_.width * 6 / 10;
    selection_.height = photo_.height * 6 / 10;
    selection_.x = (photo_.width - selection_.width) / 2;
    selection_.y = (photo_.height - selection_.height) / 2;
    resetToAuto();
    uploadPhotoTexture();
    return true;
}

void PhotoLab::uploadPhotoTexture()
{
    scratch_.resize(kPhotoTextureSize * kPhotoTextureSize * 3);
    for (int y = 0; y < kPhotoTextureSize; ++y) {
        const int sy = y * photo_.height / kPhotoTextureSize;
        for (int x = 0; x < kPhotoTextureSize; ++x) {
            const int sx = x * photo_.width / kPhotoTextureSize;
            const Vec3& c = photo_.at(sx, sy);
            const int i = (y * kPhotoTextureSize + x) * 3;
            scratch_[i] = toByte(c.x);
            scratch_[i + 1] = toByte(c.y);
            scratch_[i + 2] = toByte(c.z);
        }
    }
    glBindTexture(GL_TEXTURE_2D, photoTexture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, kPhotoTextureSize, kPhotoTextureSize, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 scratch_.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void PhotoLab::uploadPreviewTextures()
{
    const GrayImage& g = analysis_.normalized;
    if (g.width > 0) {
        scratch_.resize(kGrayTextureSize * kGrayTextureSize * 3);
        for (int y = 0; y < kGrayTextureSize; ++y) {
            const int sy = y * g.height / kGrayTextureSize;
            for (int x = 0; x < kGrayTextureSize; ++x) {
                const unsigned char v = toByte(g.at(x * g.width / kGrayTextureSize, sy));
                const int i = (y * kGrayTextureSize + x) * 3;
                scratch_[i] = v;
                scratch_[i + 1] = v;
                scratch_[i + 2] = v;
            }
        }
        glBindTexture(GL_TEXTURE_2D, grayTexture_);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, kGrayTextureSize, kGrayTextureSize, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     scratch_.data());
    }
    if (!analysis_.tile.empty()) {
        glBindTexture(GL_TEXTURE_2D, tileTexture_);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, analysis_.tileSize, analysis_.tileSize, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     analysis_.tile.data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void PhotoLab::analyze()
{
    analysis_ = ImageProc::analyze(photo_, selection_, colorChoice_, overrideX_, overrideY_);
    status_ = analysis_.message;
    uploadPreviewTextures();
}

void PhotoLab::cycleColorCount()
{
    colorChoice_ = colorChoice_ == 0 ? 2 : (colorChoice_ >= 4 ? 0 : colorChoice_ + 1);
    if (hasPhoto()) {
        analyze();
    }
}

// Manual override for when the automatic period is wrong: the arrow keys
// start from the detected (or fallback) cell size and change it 1 px at a time.
void PhotoLab::adjustPeriod(float dx, float dy)
{
    if (!hasPhoto()) {
        return;
    }
    if (dx != 0.0f) {
        overrideX_ = std::max(4.0f, (overrideX_ > 0.0f ? overrideX_ : analysis_.cellWidth) + dx);
    }
    if (dy != 0.0f) {
        overrideY_ = std::max(4.0f, (overrideY_ > 0.0f ? overrideY_ : analysis_.cellHeight) + dy);
    }
    analyze();
}

void PhotoLab::resetToAuto()
{
    overrideX_ = 0.0f;
    overrideY_ = 0.0f;
    if (hasPhoto()) {
        analyze();
    }
}

bool PhotoLab::screenToImage(int sx, int sy, int& ix, int& iy) const
{
    const float wx = static_cast<float>(sx);
    const float wy = static_cast<float>(windowHeight_ - sy);
    const float u = (wx - imageRect_.x) / imageRect_.w;
    const float v = 1.0f - (wy - imageRect_.y) / imageRect_.h;
    ix = static_cast<int>(clamp01(u) * (photo_.width - 1));
    iy = static_cast<int>(clamp01(v) * (photo_.height - 1));
    return u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f;
}

void PhotoLab::mouseDown(int x, int y)
{
    int ix = 0;
    int iy = 0;
    if (hasPhoto() && screenToImage(x, y, ix, iy)) {
        dragging_ = true;
        dragStartX_ = ix;
        dragStartY_ = iy;
    }
}

void PhotoLab::mouseDrag(int x, int y)
{
    if (!dragging_) {
        return;
    }
    int ix = 0;
    int iy = 0;
    screenToImage(x, y, ix, iy);
    selection_.x = std::min(dragStartX_, ix);
    selection_.y = std::min(dragStartY_, iy);
    selection_.width = std::abs(ix - dragStartX_);
    selection_.height = std::abs(iy - dragStartY_);
}

void PhotoLab::mouseUp(int x, int y)
{
    if (!dragging_) {
        return;
    }
    mouseDrag(x, y);
    dragging_ = false;
    if (selection_.width >= kMinDrag && selection_.height >= kMinDrag) {
        overrideX_ = 0.0f;
        overrideY_ = 0.0f;
        analyze();
    }
}

void PhotoLab::drawTexturedQuad(unsigned int texture, const Rect& r, float repeat) const
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, repeat);
    glVertex2f(r.x, r.y);
    glTexCoord2f(repeat, repeat);
    glVertex2f(r.x + r.w, r.y);
    glTexCoord2f(repeat, 0.0f);
    glVertex2f(r.x + r.w, r.y + r.h);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(r.x, r.y + r.h);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void PhotoLab::drawGraph(const std::vector<float>& values, const Rect& r, int markIndex, const char* title) const
{
    rect(r.x, r.y, r.w, r.h, 1.0f, 1.0f, 1.0f, 0.7f);
    glColor3f(0.25f, 0.2f, 0.18f);
    outline(r.x, r.y, r.w, r.h);
    text(r.x, r.y + r.h + 5, title);
    if (values.size() < 2) {
        return;
    }

    float lo = values[0];
    float hi = values[0];
    for (float v : values) {
        lo = std::min(lo, v);
        hi = std::max(hi, v);
    }
    const float range = hi - lo > 1e-6f ? hi - lo : 1.0f;

    glColor3f(0.15f, 0.35f, 0.60f);
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < values.size(); ++i) {
        const float x = r.x + r.w * i / (values.size() - 1);
        const float y = r.y + 3 + (r.h - 6) * (values[i] - lo) / range;
        glVertex2f(x, y);
    }
    glEnd();

    if (markIndex > 0 && markIndex < static_cast<int>(values.size())) {
        const float x = r.x + r.w * markIndex / (values.size() - 1);
        glColor3f(0.85f, 0.15f, 0.10f);
        glBegin(GL_LINES);
        glVertex2f(x, r.y);
        glVertex2f(x, r.y + r.h);
        glEnd();
    }
}

void PhotoLab::drawImagePanel()
{
    const float aspect = static_cast<float>(photo_.width) / photo_.height;
    Rect area = imageRect_;
    if (area.w / area.h > aspect) {
        const float w = area.h * aspect;
        area.x += (area.w - w) * 0.5f;
        area.w = w;
    } else {
        const float h = area.w / aspect;
        area.y += (area.h - h) * 0.5f;
        area.h = h;
    }
    imageRect_ = area;
    drawTexturedQuad(photoTexture_, area, 1.0f);

    const float sx = area.w / photo_.width;
    const float sy = area.h / photo_.height;
    const float x = area.x + selection_.x * sx;
    const float y = area.y + area.h - (selection_.y + selection_.height) * sy;
    glLineWidth(2.0f);
    glColor3f(1.0f, 0.85f, 0.10f);
    outline(x, y, selection_.width * sx, selection_.height * sy);

    if (analysis_.cellWidth > 0.0f && !dragging_) {
        glColor3f(0.95f, 0.25f, 0.15f);
        outline(x, y + selection_.height * sy - analysis_.cellHeight * sy,
                analysis_.cellWidth * sx, analysis_.cellHeight * sy);
    }
    glLineWidth(1.0f);
}

void PhotoLab::drawSteps(int width, int height)
{
    char line[160];
    const float left = width * 0.52f;
    const float column = (width - left - 30) / 2.0f;
    const float box = std::min(column - 20.0f, height * 0.26f);
    float top = height - 80.0f;

    glColor3f(0.25f, 0.2f, 0.18f);
    text(left, top + 8, "1. Grayscale + illumination normalised");
    drawTexturedQuad(grayTexture_, {left, top - box, box, box}, 1.0f);
    glColor3f(0.25f, 0.2f, 0.18f);
    text(left + column, top + 8, "5. Seamless tile, repeated 2 x 2");
    drawTexturedQuad(tileTexture_, {left + column, top - box, box, box}, 2.0f);
    top -= box + 40;

    const float graphH = height * 0.08f;
    drawGraph(analysis_.columnProfile, {left, top - graphH, column - 20, graphH}, 0, "2. Column profile (repeat across)");
    drawGraph(analysis_.rowProfile, {left + column, top - graphH, column - 20, graphH}, 0, "2. Row profile (repeat down)");
    top -= graphH + 34;

    const PeriodEstimate& hx = analysis_.horizontal;
    const PeriodEstimate& vy = analysis_.vertical;
    std::snprintf(line, sizeof(line), "3. DFT across: %s", hx.found ? "" : (hx.uniform ? "plain" : "no clear peak"));
    if (hx.found) {
        std::snprintf(line, sizeof(line), "3. DFT across: k=%d -> %.1f px (x%.1f)", hx.peakBin, hx.period, hx.confidence);
    }
    drawGraph(hx.spectrum, {left, top - graphH, column - 20, graphH}, hx.found ? hx.peakBin : 0, line);
    std::snprintf(line, sizeof(line), "3. DFT down: %s", vy.found ? "" : (vy.uniform ? "plain" : "no clear peak"));
    if (vy.found) {
        std::snprintf(line, sizeof(line), "3. DFT down: k=%d -> %.1f px (x%.1f)", vy.peakBin, vy.period, vy.confidence);
    }
    drawGraph(vy.spectrum, {left + column, top - graphH, column - 20, graphH}, vy.found ? vy.peakBin : 0, line);
    top -= graphH + 34;

    std::snprintf(line, sizeof(line), "4. K-means colours (%s, K to change)",
                  colorChoice_ == 0 ? "auto" : (colorChoice_ == 2 ? "2" : (colorChoice_ == 3 ? "3" : "4")));
    glColor3f(0.25f, 0.2f, 0.18f);
    text(left, top + 8, line);
    for (size_t i = 0; i < analysis_.palette.size(); ++i) {
        const Vec3& c = analysis_.palette[i];
        const float x = left + i * 120.0f;
        rect(x, top - 44, 46, 40, c.x, c.y, c.z, 1.0f);
        glColor3f(0.25f, 0.2f, 0.18f);
        outline(x, top - 44, 46, 40);
        std::snprintf(line, sizeof(line), "%.0f%%", analysis_.paletteShare[i] * 100.0f);
        text(x + 52, top - 28, line);
    }
    std::snprintf(line, sizeof(line), "Repeat cell used: %.0f x %.0f px%s", analysis_.cellWidth, analysis_.cellHeight,
                  overrideX_ > 0.0f || overrideY_ > 0.0f ? "  (manual)" : "");
    text(left, top - 66, line);
}

void PhotoLab::draw(int width, int height)
{
    windowHeight_ = height;

    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, width, 0.0, height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    rect(0, 0, width, height, 0.96f, 0.92f, 0.84f, 1.0f);

    glColor3f(0.55f, 0.16f, 0.12f);
    text(20, height - 34, "PHOTO LAB - make a lungi from your own fabric photo", kMedium);
    glColor3f(0.25f, 0.2f, 0.18f);
    if (hasPhoto()) {
        char line[200];
        std::snprintf(line, sizeof(line), "%s  (%d x %d px after resizing)", fileName_.c_str(), photo_.width, photo_.height);
        text(20, height - 54, line);
    }

    imageRect_ = {20.0f, 90.0f, width * 0.48f, height - 160.0f};
    if (hasPhoto()) {
        drawImagePanel();
        drawSteps(width, height);
    } else {
        rect(imageRect_.x, imageRect_.y, imageRect_.w, imageRect_.h, 0.9f, 0.86f, 0.78f, 1.0f);
        glColor3f(0.25f, 0.2f, 0.18f);
        text(imageRect_.x + 30, imageRect_.y + imageRect_.h * 0.5f, "Press O to open a fabric photo", kMedium);
        text(imageRect_.x + 30, imageRect_.y + imageRect_.h * 0.5f - 24,
             "Best: lungi laid flat, photographed straight from above, in daylight.");
    }

    rect(0, 0, width, 70, 0.20f, 0.16f, 0.14f, 0.9f);
    if (analysis_.ok) {
        glColor3f(0.55f, 0.95f, 0.55f);
    } else {
        glColor3f(1.0f, 0.65f, 0.45f);
    }
    text(20, 44, status_.c_str(), kMedium);
    glColor3f(0.95f, 0.92f, 0.85f);
    text(20, 16, "O: open photo   Drag: select fabric   K: colours   Arrows: adjust repeat   A: auto   ENTER: wear it   Esc: back");

    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}
