#include "collision.h"

Collision::Collision()
{
}
Collision::~Collision()
{
}

bool Collision::SphereToPlane(const Sphere& sphere, const ngl::Vec3& planePoint, const ngl::Vec3& planePointNormal)
{
    //Calculate a vector from the point on the plane to the center of the sphere
    ngl::Vec3 vecTemp(sphere.m_center - planePoint);

    //Calculate the distance: dot product of the new vector with the plane's normal
    float fDist= vecTemp.dot(planePointNormal);

    if(fDist > sphere.m_radius)
    {
        //The sphere is not touching the plane
        return false;
    }

    //Else, the sphere is colliding with the plane
    return true;
}
