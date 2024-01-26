#pragma once
#include <iostream>
#include <sstream>
class Color
{
public:
    float R, G, B;
    Color(float r, float g, float b) : R(r), G(g), B(b) {}
    Color() : R(0), G(0), B(0) {}
    Color operator+(const Color& c) const {
        return Color(R + c.R, G + c.G, B + c.B);
    }
    Color(const Color& c) {
        R = c.R;
        G = c.G;
        B = c.B;
    }
    Color& operator=(const Color& c) {
        R = c.R;
        G = c.G;
        B = c.B;
        return *this;
    }
    Color operator*(const Color& c) const {
        return Color(R * c.R, G * c.G, B * c.B);
    }
    Color operator*(float f) const {
        return Color(R * f, G * f, B * f);
    }
    Color operator *=(float f) {
        R *= f;
        G *= f;
        B *= f;
        return *this;
    }
    friend Color operator*(float f, const Color& c) {
        return Color(c.R * f, c.G * f, c.B * f);
    }
    bool operator==(const Color& c) const {
        return R == c.R && G == c.G && B == c.B;
    }
    bool operator!=(const Color& c) const {
        return !(*this == c);
    }
    Color& operator+=(const Color& c) {
        R += c.R;
        G += c.G;
        B += c.B;
        return *this;
    }
   //clamp the color values to [0, 1]
    static Color Clamp(const Color& c)
    {
     return Color(std::min(std::max(c.R, 0.0f), 1.0f), std::min(std::max(c.G, 0.0f), 1.0f), std::min(std::max(c.B, 0.0f), 1.0f));
    }
   static Color scale_color(const Color& c)
   {
    return Color(int(c.R * 255), int(c.G * 255), int(c.B * 255));
   }
    friend std::ostream& operator << (std::ostream& os, const Color& c)
    {
        os <<c.R << " " << c.G << " " << c.B<<" ";
        return os;
    }
    static std::string Print(const Color c)
    {
        std::stringstream ss;
        ss<<"("<<c.R << ", " << c.G << ", " << c.B<<")";
        return ss.str();
    }
    static Color Lerp(const Color& color1, const Color& color2, float t) {
    return color1 * (1.0f - t) + color2 * t;
    }
    
    
};