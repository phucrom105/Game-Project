#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>

namespace MathHelpers {
    constexpr float DtoR = 0.0174533f;
    constexpr float RtoD = 57.2958f;
    constexpr float SQRT2 = 1.41421356237f;
    constexpr double PI = 3.14159265358979323846;
    constexpr double HALF_PI = PI / 2.0;
    constexpr double QUARTER_PI = PI / 4.0;
    constexpr double EIGHTH_PI = PI / 8.0;
    constexpr double TWO_PI = 2.0 * PI;

    static float flength(const sf::Vector2f& rVector) {
        float flength = sqrt(rVector.x * rVector.x + rVector.y * rVector.y);
        return flength;
    }

    static sf::Vector2f normalize(const sf::Vector2f& rVector) {
        if (rVector.x == 0 && rVector.y == 0) {
            return rVector; // Avoid division by zero
        }

        float length = flength(rVector);
        sf::Vector2f vNormalizeVector(rVector.x / length, rVector.y / length);
        return vNormalizeVector;
    }

    static constexpr float Angle(const sf::Vector2f& a) {
        if (a.x == 0) {
            if (a.y > 0) {
                return 0.0f;
            }
            return 180.0;
        }
        float angle = atan2f(a.y, a.x) - HALF_PI;

        if (angle < 0.0f) {
            angle += TWO_PI;
        }
        return angle * RtoD;
    }
}