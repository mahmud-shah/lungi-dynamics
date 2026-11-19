#pragma once

#include "Vec3.h"

class ToonRamp {
public:
    void create();
    void bind() const;
    void unbind() const;

    static float coordinate(const Vec3& normal, const Vec3& lightDir);

private:
    unsigned int textureId_ = 0;
};
