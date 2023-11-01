#include "glm/glm.hpp"
#include "color.h"
#pragma once

struct Fragment {
    glm::ivec2 position;
    Color color;
    float z;
    glm::vec3 original;
};