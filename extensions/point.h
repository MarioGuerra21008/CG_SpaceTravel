#include <SDL.h>
#pragma once

void point(int x, int y, SDL_Renderer* renderer) {
    SDL_RenderDrawPoint(renderer, x, y);
}