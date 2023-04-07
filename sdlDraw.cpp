#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <cstring>
#include "sdlDraw.hpp"
#include "chip8.hpp"

sdlDraw::sdlDraw(char *game, 
        bool setAndShift = false, 
        bool jumpOffsetVariable = false, 
        bool loadStoreIdxInc = false, 
        int clockSpeed = 700, 
        int pixelSize = 8
    ) {

    this->pixelSize = pixelSize;
    this->clockSpeed = clockSpeed;
    screenWidth = pixelSize * 64;
    screenHeight = pixelSize * 32;
    
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Chip 8 Emulator", 
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                screenWidth, 
                screenHeight,
                SDL_WINDOW_SHOWN);
    
    renderer = SDL_CreateRenderer(window, -1, 0);
    
    texture = SDL_CreateTexture(renderer,
                    SDL_PIXELFORMAT_RGBA32, 
                    SDL_TEXTUREACCESS_STREAMING, 
                    screenWidth, 
                    screenHeight);
    
    processor = new Chip8(setAndShift, jumpOffsetVariable, loadStoreIdxInc, game);

}

void sdlDraw::update(bool pixels[32][64]) {

    Uint32* screenPixels = NULL;
    int pitch = 0;
    SDL_LockTexture(texture, NULL, (void**)&screenPixels, &pitch);
    
    for (int y = 0; y < screenHeight; y++) {
        for (int x = 0; x < screenWidth; x++) {
            int index = y * (pitch / sizeof(Uint32)) + x;
            if(pixels[y/pixelSize][x/pixelSize] == 1)
                screenPixels[index] = 0xFFFFFFFF; // set pixel to white
            else
                screenPixels[index] = 0xFF000000; // set pixel to black
        }
    }
    SDL_UnlockTexture(texture);
}

void sdlDraw::display() {

    SDL_Event e;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime, timer;
    startTime = std::chrono::high_resolution_clock::now();
    timer = startTime;

    // determine state of input keys (whether pressed or not)
    const uint8_t* state = SDL_GetKeyboardState(nullptr);

    while(true) {

        if( SDL_PollEvent( &e ) != 0 && e.type == SDL_QUIT) 
            break;
         
        endTime = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<int, std::micro> 
            duration = std::chrono::duration_cast<std::chrono::microseconds> (endTime-startTime),
            timerDuration = std::chrono::duration_cast<std::chrono::microseconds> (endTime-timer);

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
        if(duration.count() < (int)(1e6/clockSpeed))
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

        SDL_PumpEvents(); // update keyboard state

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
    std::cout << "\t-h\t--help\t\t\tDisplays this help text\n";
    std::cout << "\t-p=n\t--pixel-size=n\t\tSets pixel size to n\n";
    std::cout << "\t-c=n\t--clock-speed=n\t\tSets clock speed to n\n\n";
    std::cout << "Following options enable configuration of ambiguous instructions\n\n";
    std::cout << "\t-s\t--set-and-shift\t\t\tSet value of VX to VY before shift operations 8XY6 and 8XYE\n";
    std::cout << "\t-j\t--jump-offset-variable\t\tJump with offset instruction";
    std::cout << "BNNN jumps to the address XNN, plus the value in the register VX\n";
    std::cout << "\t-l\t--load-store-idx-inc\t\tSet index register (I) value to";
    std::cout << "I + X + 1 after executing load (FX65) or store (FX55) instructions\n\n";
}


int main(int argc, char *argv[]) {
    bool setAndShift = 0, jumpOffsetVariable = 0, loadStoreIdxInc = 0;
    int pixelSize = 8, clockSpeed = 700;
    if(argc < 2 || strcmp(argv[argc-1], "--help") == 0 
            || strcmp(argv[argc-1], "-h") == 0) {
        help();
        return 1;
    }

    // process commandline args
    for(int i = 1; i < argc-1; ++i) {
        if(strncmp(argv[i], "--pixel-size=", 13) == 0) 
            pixelSize = atoi(argv[i]+13);

        else if(strncmp(argv[i], "-p=", 3) == 0) 
            pixelSize = atoi(argv[i]+3);
        
        else if(strncmp(argv[i], "--clock-speed=", 14) == 0)
            clockSpeed = atoi(argv[i]+14);
        
        else if(strncmp(argv[i], "-c=", 3) == 0)
            clockSpeed = atoi(argv[i]+3);

        else if(strcmp(argv[i], "--set-and-shift") == 0)
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

    sdlDraw _sdlDraw(argv[argc-1], setAndShift, jumpOffsetVariable, 
                        loadStoreIdxInc, clockSpeed, pixelSize);
    
    _sdlDraw.display();
    return 0;
}
