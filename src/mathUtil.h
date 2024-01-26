#pragma once
#include <cmath>
#include <iostream>

template <typename T>
class _Vec3
{
public:
    _Vec3()
    {}
    _Vec3(T x, T y, T z)
        :
        x(x),
        y(y),
        z(z)
    {}
    _Vec3(const _Vec3& vect)
        :
        _Vec3(vect.x, vect.y, vect.z)
    {}
    template <typename T2>
    explicit operator _Vec3<T2>() const
    {
        return { (T2)x, (T2)y, (T2)z };
    }
    T       LenSq() const
    {
        return x * x + y * y + z * z;
    }
    T       Len() const
    {
        return sqrt(LenSq());
    }
    _Vec3&  Normalize()
    {
        const T length = Len();
        x /= length;
        y /= length;
        z /= length;
        return *this;
    }
    _Vec3  GetNormalized() const
    {
        _Vec3 norm = *this;
        norm.Normalize();
        return norm;
    }
    T  operator*(const _Vec3& rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }
    _Vec3   operator-() const
    {
        return _Vec3(-x, -y, -z);
    }
    _Vec3&  operator=(const _Vec3& rhs)
    {
        x = rhs.x;
        y = rhs.y;
        z = rhs.z;
        return *this;
    }
    _Vec3&  operator+=(const _Vec3& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    _Vec3&  operator-=(const _Vec3& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }
    _Vec3   operator+(const _Vec3& rhs) const
    {
        return _Vec3(*this) += rhs;
    }
    _Vec3   operator-(const _Vec3& rhs) const
    {
        return _Vec3(*this) -= rhs;
    }
    _Vec3&  operator*=(const T& rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }
    _Vec3   operator*(const T& rhs) const
    {
        return _Vec3(*this) *= rhs;
    }
    _Vec3&  operator/=(const T& rhs)
    {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        return *this;
    }
    friend _Vec3 operator*(const float& lhs, const _Vec3& rhs)
    {
        return _Vec3(rhs) *= lhs;
    }
    _Vec3   operator/(const T& rhs) const
    {
        return _Vec3(*this) /= rhs;
    }
    bool operator==(const _Vec3& rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    bool operator!=(const _Vec3& rhs) const
    {
        return !(*this == rhs);
    }
public: 
   _Vec3 Cross(const _Vec3& rhs) const
    {
        return _Vec3(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
    }
    static _Vec3 Cross(const _Vec3& lhs, const _Vec3& rhs)
    {
        return lhs.Cross(rhs);
    }
    static T Dot(const _Vec3& lhs, const _Vec3& rhs)
    {
        return lhs * rhs;
    }
    static _Vec3 Reflect(const _Vec3& v, const _Vec3& n)
    {
        return v - n * (2 * Dot(v, n));
    }
    static _Vec3 Lerp(const _Vec3& a, const _Vec3& b, const T& t)
    {
        return a * (1 - t) + b * t;
    }
  
    static _Vec3 Normalize(const _Vec3& v)
    {
        return v.GetNormalized();
    }
    static T Distance(const _Vec3& a, const _Vec3& b)
    {
        return (a - b).Len();
    }
    //overload << operator
    friend std::ostream& operator<<(std::ostream& os, const _Vec3& v)
    {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
    //abs
    static _Vec3 Abs(const _Vec3& v)
    {
        return _Vec3(abs(v.x), abs(v.y), abs(v.z));
    }
    static bool Refract(const _Vec3& incident, const _Vec3& normal, float refractive_ratio, _Vec3& refracted) {
        auto incident_normalized = Normalize(incident);
        float cos_theta_i = Dot(-incident_normalized, normal);
        float sin2_theta_t = refractive_ratio * refractive_ratio * (1.0f - cos_theta_i * cos_theta_i);

        if (sin2_theta_t > 1.0f) {
            return false; // Total internal reflection occurs
        }

        float cos_theta_t = std::sqrt(1.0f - sin2_theta_t);
        refracted = (incident_normalized*refractive_ratio) + normal*(refractive_ratio * cos_theta_i - cos_theta_t) ;
        return true;
    }
    static _Vec3 Refract(const _Vec3 &I, const _Vec3 &N, float eta)
{
    float cos_theta_i = -std::max(0.0f, Dot(I, N));
    float eta_i = 1.0f;
    float eta_t = eta;
    _Vec3 N_t = N;
    if (cos_theta_i < 0)
    {
        // The ray is entering the material
        std::swap(eta_i, eta_t);
        N_t = -N;
        cos_theta_i = -cos_theta_i;
    }
    float eta_ratio = eta_i / eta_t;
    float k = 1.0f - eta_ratio * eta_ratio * (1.0f - cos_theta_i * cos_theta_i);
    if (k < 0.0f)
    {
        // Total internal reflection
        return Reflect(I, N);
    }
    else
    {
        // Refraction
        return eta_ratio * I + (eta_ratio * cos_theta_i - sqrtf(k)) * N_t;
    }
}

public:
    T x;
    T y;
    T z;
};

typedef _Vec3<float> Vec3;
typedef _Vec3<float> Point;
typedef _Vec3<double> Ved3;
typedef _Vec3<int> Vei3;