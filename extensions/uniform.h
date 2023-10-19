#include "glm/glm.hpp"
#pragma once

struct Uniform {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewport;
};