#include "glm/glm.hpp"
#include "color.h"
#pragma once

struct Fragment {
    glm::ivec2 position; // X and Y coordinates of the pixel (in screen space)
    Color color;
    float z;
    glm::vec3 original;
};