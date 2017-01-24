#ifndef COLLISION_H
#define COLLISION_H

#include <ngl/Vec3.h>

class Collision
{


public:

    struct Sphere
    {
        ngl::Vec3 m_center;
        float m_radius;
    };

    bool SphereToPlane(const Sphere& sphere, const ngl::Vec3& planePoint, const ngl::Vec3& planePointNormal);

    Collision();
    ~Collision();

};

#endif // COLLISION_H
