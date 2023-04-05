#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <cstring>
#include "sdlDraw.hpp"
#include "chip8.hpp"

const int SCREEN_WIDTH = 8 * 64;
const int SCREEN_HEIGHT = 8 * 32;
const int PIXEL_SIZE = 8;

sdlDraw::sdlDraw(bool setAndShift, bool jumpOffsetVariable, bool loadStoreIdxInc, char *game) {
     // Initialize SDL
    SDL_Init(SDL_INIT_VIDEO);

    // Create a window
    window = SDL_CreateWindow("Chip 8 Emulator", 
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, 
            SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    // Create a renderer for the window
    renderer = SDL_CreateRenderer(window, -1, 0);

    // Create a black pixel surface with the same dimensions as the pixel array
    pixelSurface = SDL_CreateRGBSurface(0, SCREEN_WIDTH,
            SCREEN_HEIGHT , 32, 0, 0, 0, 0);

    // initialize chip8 cpu
    
    processor = new Chip8(setAndShift, jumpOffsetVariable, loadStoreIdxInc, game);
}

void sdlDraw::update(bool pixels[32][64]) {
    // Draw white pixels for every 1 and black pixels for every 0
    for (int i = 0; i < SCREEN_HEIGHT/ PIXEL_SIZE; i++) {
        for (int j = 0; j < SCREEN_WIDTH / PIXEL_SIZE; j++) {
            SDL_Rect pixelRect = { j * PIXEL_SIZE, i * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE };
            if (pixels[i][j] == 1) 
                SDL_FillRect(pixelSurface, &pixelRect, SDL_MapRGB(pixelSurface->format, 255, 255, 255));
            else 
                SDL_FillRect(pixelSurface, &pixelRect, SDL_MapRGB(pixelSurface->format, 0, 0, 0));
            
        }
    }
}

void sdlDraw::display() {

    SDL_Event e;
    SDL_Texture* texture;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime, timer;
    startTime = std::chrono::high_resolution_clock::now();
    timer = startTime;
    while(true) {

        if( SDL_PollEvent( &e ) != 0 && e.type == SDL_QUIT) 
            break;
         
        endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<int, std::micro> duration = 
        std::chrono::duration_cast<std::chrono::microseconds>(endTime-startTime),
        timerDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime-timer);
        
        if(timerDuration.count() >= (int)(1e6/60)) {
            if(processor->delayTimer > 0)
                processor->delayTimer--;
            if(processor->soundTimer > 0) {
                std::cout << "\a";
                processor->soundTimer--;
            }
            timer = std::chrono::high_resolution_clock::now();
        }


        // skip iter if clock cycle not encountered
        if(duration.count() < (int)(1e6/700))
            continue;

 /******************************************************************************
 *
 *      Keyboard            Original
 *      Input               Hex Input
 *      
 *      1 2 3 4             1 2 3 C
 *      Q W E R     -->     4 5 6 D
 *      A S D F     -->     7 8 9 E
 *      Z X C V             A 0 B F
 *
******************************************************************************/

        // determine state of input keys (whether pressed or not)
        const uint8_t* state = SDL_GetKeyboardState(nullptr);
        
        bool tempArray[16] = {
            state[SDL_SCANCODE_X], state[SDL_SCANCODE_1], state[SDL_SCANCODE_2],
            state[SDL_SCANCODE_3], state[SDL_SCANCODE_Q], state[SDL_SCANCODE_W], 
            state[SDL_SCANCODE_E], state[SDL_SCANCODE_A], state[SDL_SCANCODE_S], 
            state[SDL_SCANCODE_D], state[SDL_SCANCODE_Z], state[SDL_SCANCODE_C],
            state[SDL_SCANCODE_4], state[SDL_SCANCODE_R], state[SDL_SCANCODE_F],
            state[SDL_SCANCODE_V]
        };
        
        for(int i = 0x0; i <= 0xF; ++i)
            processor->keys[i] = tempArray[i];
        
        bool refreshDisplay = processor->decode(processor->fetch());
        
        if(refreshDisplay) {
            update(processor->display);

            texture = SDL_CreateTextureFromSurface(renderer, pixelSurface);

            // Blit the pixel surface onto the window
            SDL_RenderCopy(renderer, texture, NULL, NULL);

            // Update the screen and wait for the user to close the window
            SDL_RenderPresent(renderer);
           
        }

        // reset clock cycle 
        startTime = std::chrono::high_resolution_clock::now();

    }
   
    // Clean up
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(pixelSurface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
 
}

void help() {
    std::cout << "Usage: ./chip8emu [OPTIONS] <game-file>\n";
    std::cout << "Chip 8 Emulator\n";
    std::cout << "Example: ./chip8emu -sjl game_rom.ch8\n\n";
    std::cout << "Options:\n\n";
    std::cout << "\t-h\t--help\t\tDisplays this help text\n\n";
    std::cout << "Following options enable configuration of ambiguous instructions\n\n";
    std::cout << "\t-s\t--set-and-shift\t\tSet value of VX to VY before shift operations 8XY6 and 8XYE\n\n";
    std::cout << "\t-j\t--jump-offset-variable\t\tJump with offset instruction BNNN jumps to the address XNN, plus the value in the register VX\n";
    std::cout << "\t-l\t--load-store-idx-inc\t\tSet index register (I) value to I + X + 1 after executing load (FX65) or store (FX55) instructions\n";
}


int main(int argc, char *argv[]) {
    bool setAndShift = 0, jumpOffsetVariable = 0, loadStoreIdxInc = 0;
    if(argc < 2) {
        help();
        return 1;
    }

    // process commandline args
    for(int i = 1; i < argc-1; ++i) {
        if(strcmp(argv[i], "--set-and-shift") == 0)
            setAndShift = 1;

        else if(strcmp(argv[i], "--jump-offset-variable") == 0)
            jumpOffsetVariable = 1;
        
        else if(strcmp(argv[i], "--load-store-idx-inc") == 0)
            loadStoreIdxInc = 1;
        
        else if(strncmp(argv[i], "-", 1) == 0) {
            bool noOptions = 1;
            for(int j = 1; argv[i][j] != '\0'; ++j) {
                switch(argv[i][j]) {
                    case 's':
                        setAndShift = 1;
                        noOptions = 0;
                        break;
                    
                    case 'j':
                        jumpOffsetVariable = 1;
                        noOptions = 0;
                        break;
                    
                    case 'l':
                        loadStoreIdxInc = 1;
                        noOptions = 0;
                        break;

                    default:
                        help();
                        return 1;
                }
            }
            if(noOptions) {
                help();
                return 1;
            }
        }

        else {
            help();
            return 1;
        }
    }

    sdlDraw _sdlDraw(setAndShift, jumpOffsetVariable, loadStoreIdxInc, argv[argc-1]);
    _sdlDraw.display();
    return 0;
}