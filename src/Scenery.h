#pragma once

#include <random>
#include <vector>

#include "ToonRamp.h"
#include "Vec3.h"

class Wind;

// Static, toon-lit geometry with one colour per vertex (hut, haystacks,
// distant trees, banana trunks).
class PropMesh {
public:
    void addTriangle(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& color);
    void addQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec3& color);
    void addQuadWithNormal(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
                           const Vec3& normal, const Vec3& color);
    void addBox(const Vec3& lo, const Vec3& hi, const Vec3& color);
    void addSphere(const Vec3& center, float radius, const Vec3& color, int slices, int stacks);
    void addCone(const Vec3& base, float radius, float height, const Vec3& color, int slices);

    void finish(const Vec3& lightDir);
    void draw(const ToonRamp& ramp, float brightness);
    void clear();

private:
    void addVertex(const Vec3& p, const Vec3& n, const Vec3& c);

    std::vector<Vec3> positions_;
    std::vector<Vec3> normals_;
    std::vector<Vec3> colors_;
    std::vector<Vec3> drawColors_;
    std::vector<float> rampCoords_;
};

class Scenery {
public:
    void build(const Vec3& lightDir);
    void update(float dt, const Wind& wind, float storm);

    void drawClouds(const Vec3& eye, float storm, float flash) const;
    void drawGround(float brightness);
    void drawProps(const ToonRamp& ramp, const Vec3& lightDir, float brightness);
    void drawGrass(float brightness);
    void drawDebris(float brightness) const;
    void drawRain(float brightness) const;
    void drawLightningBolt() const;

    float flash() const { return flash_; }
    bool consumeThunder();

private:
    struct Cloud {
        Vec3 center;
        std::vector<Vec3> puffOffsets;
        std::vector<float> puffRadii;
        float height;
    };

    struct BananaTree {
        Vec3 base;
        float height;
        float phase;
        std::vector<float> leafAngles;
        std::vector<float> leafLengths;
        std::vector<bool> leafDry;
    };

    struct Blade {
        Vec3 base;
        float height;
        float lean;
        float phase;
    };

    struct Debris {
        Vec3 position;
        float spin;
        float angle;
        bool paper;
    };

    void buildGround();
    void buildClouds();
    void buildHut();
    void buildBananaTrees();
    void buildHaystacks();
    void buildTreeLine();
    void buildGrass();
    void buildDebris();
    void buildBananaLeaves(float time, const Vec3& windDir, float windSpeed);
    void updateRain(float dt, const Wind& wind, float storm);
    void updateLightning(float dt, float storm);
    bool blocked(float x, float z) const;
    float random(float lo, float hi);

    std::mt19937 rng_;
    float time_ = 0.0f;
    Vec3 windDir_{1.0f, 0.0f, 0.0f};
    float windSpeed_ = 0.0f;

    int groundSize_ = 0;
    std::vector<Vec3> groundPositions_;
    std::vector<Vec3> groundColors_;
    std::vector<Vec3> groundDrawColors_;
    std::vector<unsigned int> groundIndices_;

    std::vector<Cloud> clouds_;
    PropMesh staticProps_;
    PropMesh treeLine_;
    std::vector<BananaTree> bananas_;
    PropMesh bananaLeaves_;
    Vec3 lightDir_;

    std::vector<Blade> blades_;
    std::vector<Vec3> grassPositions_;
    std::vector<Vec3> grassColors_;
    std::vector<Vec3> grassDrawColors_;

    std::vector<Debris> debris_;
    std::vector<Vec3> rain_;
    int activeRain_ = 0;

    float flash_ = 0.0f;
    float timeToLightning_ = 5.0f;
    float thunderDelay_ = -1.0f;
    bool thunderPending_ = false;
    std::vector<Vec3> bolt_;
};
