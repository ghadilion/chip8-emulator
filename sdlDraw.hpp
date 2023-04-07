#pragma once
#include "chip8.hpp"

class sdlDraw {

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *pixelSurface;
    SDL_Texture* texture;
    Chip8 *processor;

public:
    sdlDraw(bool, bool, bool, char*);
    void update(bool[32][64]);
    void display();
};

