#include "TestSphere.h"

#include <GL/freeglut.h>
#include <cmath>

TestSphere::TestSphere(const Vec3& center, float radius, const Vec3& color, int slices, int stacks)
    : center_(center), color_(color)
{
    const float pi = 3.14159265f;

    for (int stack = 0; stack <= stacks; ++stack) {
        const float phi = pi * stack / stacks;
        for (int slice = 0; slice <= slices; ++slice) {
            const float theta = 2.0f * pi * slice / slices;
            const Vec3 n(std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta));
            normals_.push_back(n);
            positions_.push_back(n * radius);
        }
    }

    const unsigned int ring = static_cast<unsigned int>(slices + 1);
    for (unsigned int stack = 0; stack < static_cast<unsigned int>(stacks); ++stack) {
        for (unsigned int slice = 0; slice < static_cast<unsigned int>(slices); ++slice) {
            const unsigned int a = stack * ring + slice;
            const unsigned int b = a + ring;
            indices_.push_back(a);
            indices_.push_back(a + 1);
            indices_.push_back(b);
            indices_.push_back(a + 1);
            indices_.push_back(b + 1);
            indices_.push_back(b);
        }
    }

    rampCoords_.resize(positions_.size());
}

void TestSphere::draw(const ToonRamp& ramp, const Vec3& lightDir, float brightness)
{
    for (size_t i = 0; i < normals_.size(); ++i) {
        rampCoords_[i] = ToonRamp::coordinate(normals_[i], lightDir);
    }

    const Vec3 c = color_ * brightness;
    glColor3f(c.x, c.y, c.z);

    glPushMatrix();
    glTranslatef(center_.x, center_.y, center_.z);

    ramp.bind();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3), &positions_[0].x);
    glTexCoordPointer(1, GL_FLOAT, 0, rampCoords_.data());
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, indices_.data());
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    ramp.unbind();

    glPopMatrix();
}
