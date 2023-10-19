#include <SDL.h>
#pragma once

struct Color {
    uint8_t r, g, b, a;

    Color(Uint8 red = 0, Uint8 green = 0, Uint8 blue = 0, Uint8 alpha = 255)
            : r(red), g(green), b(blue), a(alpha) {}

    // Sobrecarga del operador * para la multiplicación por un escalar
    Color operator*(float scalar) const {
        Color result;
        result.r = static_cast<uint8_t>(r * scalar);
        result.g = static_cast<uint8_t>(g * scalar);
        result.b = static_cast<uint8_t>(b * scalar);
        result.a = static_cast<uint8_t>(a * scalar);
        return result;
    }

    // Sobrecarga del operador + para la suma de colores
    Color operator+(const Color& other) const {
        Color result;
        result.r = static_cast<uint8_t>(r + other.r);
        result.g = static_cast<uint8_t>(g + other.g);
        result.b = static_cast<uint8_t>(b + other.b);
        result.a = static_cast<uint8_t>(a + other.a);
        return result;
    }
};
