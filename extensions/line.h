#include <SDL.h>
#include "glm/glm.hpp"
#pragma once

void line(const glm::vec3& start, const glm::vec3& end, SDL_Renderer* renderer) {
    SDL_RenderDrawLine(renderer,
                       static_cast<int>(start.x), static_cast<int>(start.y),
                       static_cast<int>(end.x), static_cast<int>(end.y));
}