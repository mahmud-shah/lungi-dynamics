#include "Evaluation.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cstdio>
#include <ctime>

#include "Platform.h"
#include "Wind.h"

namespace {

const float kWarmUpSeconds = 1.5f;
const float kMeasureSeconds = 6.0f;
const float kTimeSteps[] = {1.0f / 60.0f, 1.0f / 120.0f, 1.0f / 240.0f};

const char* const kRunLogPath = "logs/runs.csv";
const char* const kRunLogHeader =
    "date_time,difficulty,lungi,survival_s,strongest_gust_ms,outcome,score,tier,avg_fps\n";

void writeLittleEndian(FILE* f, unsigned int value, int bytes)
{
    for (int i = 0; i < bytes; ++i) {
        std::fputc(static_cast<int>((value >> (8 * i)) & 0xFF), f);
    }
}

} // namespace

std::string timestampString()
{
    const std::time_t now = std::time(nullptr);
    char text[32];
    std::strftime(text, sizeof(text), "%Y%m%d_%H%M%S", std::localtime(&now));
    return text;
}

void Evaluation::start()
{
    configs_.clear();
    rows_.clear();
    for (float dt : kTimeSteps) {
        for (int preset = 0; preset < Wind::presetCount(); ++preset) {
            configs_.push_back({dt, preset});
        }
    }
    index_ = 0;
    elapsed_ = 0.0f;
    stats_ = Stats();
    savedPath_.clear();
    running_ = true;
}

// Returns true when a new configuration starts, so the caller can reset the
// world and apply the new time step and wind.
bool Evaluation::update(const FrameSample& sample, int solverIterations)
{
    if (!running_) {
        return false;
    }
    elapsed_ += sample.frameSeconds;
    if (elapsed_ > kWarmUpSeconds) {
        ++stats_.frames;
        stats_.time += sample.frameSeconds;
        stats_.worstFrame = std::max(stats_.worstFrame, sample.frameSeconds);
        stats_.physicsMs += sample.physicsMs;
        stats_.maxStretch = std::max(stats_.maxStretch, sample.maxStretch);
        stats_.meanStretch += sample.meanStretch;
        stats_.knotForce += sample.knotForce;
        stats_.recoveries = sample.recoveries;
    }
    if (elapsed_ < kWarmUpSeconds + kMeasureSeconds) {
        return false;
    }

    finishConfig(solverIterations);
    ++index_;
    elapsed_ = 0.0f;
    stats_ = Stats();
    if (index_ >= count()) {
        index_ = count() - 1;
        running_ = false;
        writeFile();
        return false;
    }
    return true;
}

void Evaluation::finishConfig(int solverIterations)
{
    const EvalConfig& c = configs_[index_];
    const float frames = static_cast<float>(std::max(1, stats_.frames));
    char row[256];
    std::snprintf(row, sizeof(row), "%.5f,%.0f,%d,%d,%s,%.1f,%.2f,%.2f,%.3f,%.2f,%.3f,%.1f,%d\n",
                  c.timeStep, 1.0f / c.timeStep, solverIterations, c.windPreset,
                  Wind::presetName(c.windPreset),
                  stats_.frames / std::max(0.001f, stats_.time),
                  1000.0f * stats_.time / frames, 1000.0f * stats_.worstFrame,
                  stats_.physicsMs / frames, 100.0f * stats_.maxStretch,
                  100.0f * stats_.meanStretch / frames, stats_.knotForce / frames, stats_.recoveries);
    rows_.push_back(row);
}

void Evaluation::writeFile()
{
    Platform::makeDirectory("logs");
    savedPath_ = "logs/evaluation_" + timestampString() + ".csv";
    FILE* f = std::fopen(savedPath_.c_str(), "w");
    if (!f) {
        savedPath_ = "(could not write logs/ - check folder permissions)";
        return;
    }
    std::fputs("time_step_s,steps_per_s,solver_iterations,wind_preset,wind_name,avg_fps,avg_frame_ms,"
               "worst_frame_ms,physics_ms_per_frame,max_stretch_pct,mean_stretch_pct,avg_knot_force_N,"
               "recoveries\n", f);
    for (const std::string& row : rows_) {
        std::fputs(row.c_str(), f);
    }
    std::fclose(f);
}

bool appendRunLog(const std::string& line)
{
    Platform::makeDirectory("logs");
    FILE* existing = std::fopen(kRunLogPath, "r");
    const bool isNew = existing == nullptr;
    if (existing) {
        std::fclose(existing);
    }
    FILE* f = std::fopen(kRunLogPath, "a");
    if (!f) {
        return false;
    }
    if (isNew) {
        std::fputs(kRunLogHeader, f);
    }
    std::fputs(line.c_str(), f);
    std::fclose(f);
    return true;
}

// Saves the finished frame (call just before swapping buffers) as a 24-bit BMP. glReadPixels returns rows from
// the bottom up, which is also the order BMP stores them in; each row is
// padded to a multiple of 4 bytes and stored as blue-green-red.
std::string saveScreenshot(int width, int height)
{
    Platform::makeDirectory("screenshots");
    const std::string path = "screenshots/lungi_" + timestampString() + ".bmp";
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) {
        return "";
    }
    const int rowSize = (width * 3 + 3) & ~3;
    const unsigned int imageSize = static_cast<unsigned int>(rowSize * height);
    std::fputc('B', f);
    std::fputc('M', f);
    writeLittleEndian(f, 54 + imageSize, 4);
    writeLittleEndian(f, 0, 4);
    writeLittleEndian(f, 54, 4);
    writeLittleEndian(f, 40, 4);
    writeLittleEndian(f, static_cast<unsigned int>(width), 4);
    writeLittleEndian(f, static_cast<unsigned int>(height), 4);
    writeLittleEndian(f, 1, 2);
    writeLittleEndian(f, 24, 2);
    writeLittleEndian(f, 0, 4);
    writeLittleEndian(f, imageSize, 4);
    writeLittleEndian(f, 2835, 4);
    writeLittleEndian(f, 2835, 4);
    writeLittleEndian(f, 0, 4);
    writeLittleEndian(f, 0, 4);

    std::vector<unsigned char> row(rowSize, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const unsigned char* p = &pixels[(static_cast<size_t>(y) * width + x) * 3];
            row[x * 3 + 0] = p[2];
            row[x * 3 + 1] = p[1];
            row[x * 3 + 2] = p[0];
        }
        std::fwrite(row.data(), 1, rowSize, f);
    }
    std::fclose(f);
    return path;
}
