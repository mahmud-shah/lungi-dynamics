#pragma once

#include <vector>

#include "Vec3.h"

struct Stripe {
    Vec3 color;
    float width;
};

// A woven check is defined like a real tartan "sett": the sequence of
// coloured thread bands across the warp (vertical threads) and the weft
// (horizontal threads). Stripes and plain cloth are special cases.
struct FabricPattern {
    const char* name;
    std::vector<Stripe> warp;
    std::vector<Stripe> weft;
};

namespace Patterns {

std::vector<FabricPattern> builtIn();

} // namespace Patterns

class FabricTexture {
public:
    void create();
    void upload(const FabricPattern& pattern);
    void uploadTexels(const std::vector<unsigned char>& rgb, int size, const Vec3& mainColor);
    void bind() const;
    void unbind() const;

    Vec3 mainColor() const { return mainColor_; }
    int size() const { return kSize; }

private:
    static const int kSize = 128;

    unsigned int textureId_ = 0;
    std::vector<unsigned char> texels_;
    Vec3 mainColor_;
};
