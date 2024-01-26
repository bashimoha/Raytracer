#pragma once
#include "mathUtil.h"
#include "image.h"
#include "input.h"
#include "color.h"

#include <cmath>
struct RayResult
{
    float t;
    Vec3 interpolated_normal; 
    Vec3 interpolated_uv;
    Object* object=nullptr;
    bool inside = false;
};

class Ray
{
public:
    Ray(Point origin, Vec3 direction) : origin(origin), direction(direction) {}
    Point origin;
    Vec3 direction;
    // compute some distance along the ray at "time" t
    Point at(float t)  {
        return origin +  direction*t;
    }

    float IntersectSphere(Sphere sphere) {
        // Compute the coefficients of the quadratic equation
        float a = Vec3::Dot(direction, direction);
        float b = 2 * Vec3::Dot(direction, origin - sphere.pos);
        float c = Vec3::Dot(origin - sphere.pos, origin - sphere.pos) - sphere.radius * sphere.radius;
        // Compute the discriminant
        float discriminant = b * b - 4 * a * c;
        // If the discriminant is negative, there are no real roots
        if (discriminant < 0) {
            return -1;
        }
        // Compute the two roots
        float t1 = (-b + sqrt(discriminant)) / (2 * a);
        float t2 = (-b - sqrt(discriminant)) / (2 * a);
        // If both roots are negative, the ray starts inside the sphere
        if (t1 < 0 && t2 < 0) {
            return -1;
        }
        if (t1 < 0) {
            return t2;
        }
        if (t2 < 0) {
            return t1;
        }
        return fmin(t1, t2);
    }
    float IntersectTriangle(Face face, Vec3& interpolated_normal, Vec3& interpolated_uv)
    {
        // 1. Compute the normal of the triangle
        Vec3 v0v1 = face.v1.pos - face.v0.pos;
        Vec3 v0v2 = face.v2.pos - face.v0.pos;
        Vec3 N = v0v1.Cross(v0v2);

        // 2. Apply plane equation to find the intersection point
        float d = Vec3::Dot(N, face.v0.pos);
        if (Vec3::Dot(N, direction) == 0) {
            return -1;
        }
        float t = (d - Vec3::Dot(N, origin)) / Vec3::Dot(N, direction);
        if (t < 0) {
            return -1;
        }
        auto intersection = at(t);

        // 3. Check if the intersection point is inside the triangle using barycentric coordinates
        Vec3 C;
        Vec3 edge0 = face.v1.pos - face.v0.pos;
        Vec3 vp0 = intersection - face.v0.pos;
        C = edge0.Cross(vp0);
        if (Vec3::Dot(N, C) < 0) {
            return -1;
        }
        Vec3 edge1 = face.v2.pos - face.v1.pos;
        Vec3 vp1 = intersection - face.v1.pos;
        C = edge1.Cross(vp1);
        if (Vec3::Dot(N, C) < 0) {
            return -1;
        }
        Vec3 edge2 = face.v0.pos - face.v2.pos;
        Vec3 vp2 = intersection - face.v2.pos;
        C = edge2.Cross(vp2);
        if (Vec3::Dot(N, C) < 0) {
            return -1;
        }

        // 4. Calculate the barycentric coordinates of the intersection point and interpolate the normal
        float areaABC = N.Len();
        float areaPBC = edge1.Cross(intersection - face.v1.pos).Len();
        float areaPCA = edge2.Cross(intersection - face.v2.pos).Len();
        float alpha = areaPBC / areaABC;
        float beta = areaPCA / areaABC;
        float gamma = 1.0f - alpha - beta;
        // interpolated_normal = (face.v0.normal * gamma) + (face.v1.normal * alpha) + (face.v2.normal * beta);
        interpolated_normal = Vec3::Normalize((face.v0.normal * alpha) + (face.v1.normal * beta) + (face.v2.normal * gamma));
        float u = (face.v0.texture_coord.first * alpha) + (face.v1.texture_coord.first * beta) + (face.v2.texture_coord.first * gamma);
        float v = (face.v0.texture_coord.second * alpha) + (face.v1.texture_coord.second * beta) + (face.v2.texture_coord.second * gamma);
        interpolated_uv = Vec3(u, v, 0);
        interpolated_normal.Normalize();
        return t;
    }
    // Point IntersectionPoint(Sphere sphere, int* t)
    //     {
    //         *t = IntersectSphere(sphere);
    //         return at(*t);
    //     }
    RayResult Intersect(Object* object)
    {
        RayResult result;
        result.t = -1;
        if(object->type == ObjectType::SPHERE)
        {
            auto sphere = dynamic_cast<Sphere*>(object);
            result.t = IntersectSphere(*sphere);
            //check if inside
            if(result.t < 0)
            {
                result.t = IntersectSphere(*sphere);
                if(result.t > 0)
                {
                    result.inside = true;
                }
            }
            auto intersect_point = at(result.t);
           result.interpolated_normal = Vec3::Normalize(intersect_point - sphere->pos)/sphere->radius;
           
        }
        else if(object->type == ObjectType::FACE)
        {
            auto triangle = dynamic_cast<Face*>(object);
            result.t = IntersectTriangle(*triangle, result.interpolated_normal, result.interpolated_uv);
            //check if inside
            if(result.t < 0)
            {
                result.t = IntersectTriangle(*triangle, result.interpolated_normal, result.interpolated_uv);
                if(result.t > 0)
                {
                    result.inside = true;
                }
            }
        }


        return result;
    }
};
