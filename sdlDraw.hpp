#pragma once
#include "chip8.hpp"

class sdlDraw {

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *pixelSurface;
    SDL_Texture* texture;

    Chip8 *processor;
    int clockSpeed;
    int pixelSize;
    int screenWidth;
    int screenHeight;

public:
    sdlDraw(char*, bool, bool, bool, int, int);
    void update(bool[32][64]);
    void display();
};

