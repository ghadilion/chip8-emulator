#include "chip8.hpp"
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <ctime>

/******************************************************************************
/
/  instruction formats : IXYN or IXNN or INNN
/  where I : opcode         (first nibble)
/        X : second nibble  (denotes first register)
/        Y : third nibble   (denotes second register)
/        N : fourth nibble 
/       NN : second byte
/      NNN : last 3 nibbles 
/
******************************************************************************/

#define   I(instr)((instr & 0xF000) >> 12)
#define   X(instr)((instr & 0x0F00) >> 8)   
#define   Y(instr)((instr & 0x00F0) >> 4)   
#define   N(instr)(instr & 0x000F)        
#define  NN(instr)(instr & 0x00FF)        
#define NNN(instr)(instr & 0x0FFF)   

#define MAX_GAME_SIZE (0x1000 - 0x200)
#define MEM_SIZE 0x1000

#define V variableRegisters
#define PC programCounter

Chip8::Chip8(bool setAndShift, bool jumpOffsetVariable, bool loadAndStoreIdxInc, char *game) {

    this->setAndShift = setAndShift;
    this->jumpOffsetVariable = jumpOffsetVariable;
    this->loadAndStoreIdxInc = loadAndStoreIdxInc;

    // load game instrunctions into memory

    FILE *fgame;
    fgame = fopen(game, "rb");
    
    if (NULL == fgame) {
        fprintf(stderr, "Unable to open game: %s\n", game);
        exit(42);
    }
    
    fread(&memory[0x200], 1, MAX_GAME_SIZE, fgame);
    fseek(fgame, 0L, SEEK_END);

    // add one more instruction at end to loop back to beginning of program

    long gameSize =  ftell(fgame);
    memory[(0x200 + gameSize) % MEM_SIZE] = 0x12;
    memory[(0x200 + gameSize + 1) % MEM_SIZE] = 0x00;
    fclose(fgame);


    // load fonts into memory

    uint8_t fonts[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    for (int i = 0; i < 80; ++i)
        memory[0x50 + i] = fonts[i];

    PC = 0x200;
}

uint16_t Chip8::fetch() {
    uint16_t curInstruction = memory[PC];
    PC++;
    curInstruction <<= 8;
    curInstruction |= memory[PC];
    PC++;
    return curInstruction;
}

bool Chip8::decode(uint16_t instr) {
    bool refreshDisplay = false;
    switch(I(instr)) {
        case 0x0:
            switch(NNN(instr)) {
                case 0x0E0:                                             // 00E0
                    clearScreen();
                    refreshDisplay = true;
                    break;

                case 0x0EE:                                             // 00EE
                    _return();
                    break;

                default:
                    std::cerr << "Unexpected operand(s) in : " << instr;
                    exit(1);
            }
            break;
        
        case 0x1:
            jump(NNN(instr));                                           // 1NNN
            break;
        
        case 0x2:
            subroutine(NNN(instr));                                     // 2NNN
            break;

        case 0x3:
            skipIfEqImmed(X(instr), NN(instr));                         // 3XNN
            break;

        case 0x4:
            skipIfNeqImmed(X(instr), NN(instr));                        // 4XNN
            break;

        case 0x5:
            skipIfEq(X(instr), Y(instr));                               // 5XY0
            break;

        case 0x6:
            setImmed(X(instr), NN(instr));                              // 6XNN
            break;

        case 0x7:
            addImmed(X(instr), NN(instr));                              // 7XNN
            break;

        case 0x8:
            switch(N(instr)) {
                case 0x0:
                    set(X(instr), Y(instr));                            // 8XY0
                    break;
                
                case 0x1:
                    _or(X(instr), Y(instr));                            // 8XY1
                    break;
                
                case 0x2:
                    _and(X(instr), Y(instr));                           // 8XY2
                    break;
                
                case 0x3:
                    _xor(X(instr), Y(instr));                           // 8XY3
                    break;

                case 0x4:
                    add(X(instr), Y(instr));                            // 8XY4
                    break;

                case 0x5:
                    subXY(X(instr), Y(instr));                          // 8XY5
                    break;

                case 0x6:
                    shiftRight(X(instr), Y(instr));                     // 8XY6
                    break;

                case 0x7:
                    subYX(X(instr), Y(instr));                          // 8XY7
                    break;

                case 0xE:
                    shiftLeft(X(instr), Y(instr));                      // 8XYE
                    break;

                default:
                    std::cerr << "Unexpected operand(s) in " << instr;
                    exit(1);
            }
            break;

        case 0x9:
            skipIfNeq(X(instr), Y(instr));                              // 9XY0
            break;

        case 0xA:
            setIndex(NNN(instr));                                       // ANNN
            break;
        
        case 0xB:
            jumpWithOffset(NNN(instr));                                 // BNNN
            break;

        case 0xC:
            random(X(instr), NN(instr));                                // CXNN
            break;
        
        case 0xD:
            draw(X(instr), Y(instr), N(instr));                         // DXYN
            refreshDisplay = true;
            break;
        
        case 0xE:
            switch(NN(instr)) {
                case 0x9E:
                    skipIfKey(X(instr));                                // EX9E
                    break;

                case 0xA1:
                    skipIfNotKey(X(instr));                             // EXA1
                    break;
            }
            break;

        case 0xF:
            switch(NN(instr)) {
                case 0x07:
                    setXtoDelay(X(instr));                              // FX07
                    break;

                case 0x15:
                    setDelayToX(X(instr));                              // FX15
                    break;

                case 0x18:
                    setSoundToX(X(instr));                              // FX18
                    break;

                case 0x1E:
                    addToIndex(X(instr));                               // FX1E
                    break;

                case 0x0A:
                    getKey(X(instr));                                   // FX0A
                    break;

                case 0x29:
                    fontCharacter(X(instr));                            // FX29
                    break;

                case 0x33:
                    BCDconvert(X(instr));                               // FX33
                    break;

                case 0x55:
                    store(X(instr));                                    // FX55
                    break;

                case 0x65:
                    load(X(instr));                                     // FX65
                    break;

                default:
                    std::cerr << "Unexpected operand(s) in " << instr;
                    exit(1);
            }
            break;

        default:
            std::cerr << " Unexpected operator in " << instr;
            exit(1);
    }
    return refreshDisplay;
}

void Chip8::clearScreen() {
    for(int i = 0; i < 32; ++i)
        for(int j = 0; j < 64; ++j)
            display[i][j] = false;
}

void Chip8::_return() {
    PC = Stack.top();
    Stack.pop();
}

void Chip8::jump(uint16_t memLoc) {
    PC = memLoc;
}

void Chip8::subroutine(uint16_t memLoc) {
    Stack.push(PC);
    PC = memLoc; 
}

void Chip8::skipIfEqImmed(uint8_t regLoc, uint8_t value) {
    if(V[regLoc] == value)
        PC += 0x2;
}

void Chip8::skipIfNeqImmed(uint8_t regLoc, uint8_t value) {
    if(V[regLoc] != value)
        PC += 0x2;
}

void Chip8::skipIfEq(uint8_t regLoc1, uint8_t regLoc2) {
    if(V[regLoc1] == V[regLoc2])
        PC += 2;
}

void Chip8::setImmed(uint8_t regLoc, uint8_t value) {
    V[regLoc] = value;
}

void Chip8::addImmed(uint8_t regLoc, uint8_t value) {
    V[regLoc] += value;
}

void Chip8::set(uint8_t regLoc1, uint8_t regLoc2) {
    V[regLoc1] = V[regLoc2];    
}

void Chip8::_or(uint8_t regLoc1, uint8_t regLoc2) {
    V[regLoc1] |= V[regLoc2]; 
}

void Chip8::_and(uint8_t regLoc1, uint8_t regLoc2) {
    V[regLoc1] &= V[regLoc2]; 
}

void Chip8::_xor(uint8_t regLoc1, uint8_t regLoc2) {
    V[regLoc1] ^= V[regLoc2]; 
}

void Chip8::add(uint8_t regLoc1, uint8_t regLoc2) {
    V[regLoc1] += V[regLoc2];
    
    // handle overflow
    if(V[regLoc1] < V[regLoc2])
        V[0xF] = 1;
    else
        V[0xF] = 0;
}

void Chip8::subXY(uint8_t regLoc1, uint8_t regLoc2) {
    // handle underflow
    if(V[regLoc1] > V[regLoc2])
        V[0xF] = 1;
    else
        V[0xF] = 0;

    V[regLoc1] -= V[regLoc2];
}

void Chip8::shiftRight(uint8_t regLoc1, uint8_t regLoc2) {
    if(setAndShift)
        V[regLoc1] = V[regLoc2];
    V[regLoc1] >>= 1;
}

void Chip8::subYX(uint8_t regLoc1, uint8_t regLoc2) {
    // handle underflow
    if(V[regLoc2] > V[regLoc1])
        V[0xF] = 1;
    else
        V[0xF] = 0;

    V[regLoc1] = V[regLoc2] - V[regLoc1];
}

void Chip8::shiftLeft(uint8_t regLoc1, uint8_t regLoc2) {
    if(setAndShift)
        V[regLoc1] = V[regLoc2];
    V[regLoc1] <<= 1;
}

void Chip8::skipIfNeq(uint8_t regLoc1, uint8_t regLoc2) {
    if(V[regLoc1] != V[regLoc2])
        PC += 2;
}

void Chip8::setIndex(uint16_t memLoc) {
    indexRegister = memLoc;
}

void Chip8::jumpWithOffset(uint16_t memLoc) {
    PC = memLoc;
    if(jumpOffsetVariable)
        PC += V[X(memLoc)];
    else
        PC += V[0x0];
}

void Chip8::random(uint8_t regLoc, uint8_t value) {
    srand(time(0));
    V[regLoc] = value & (rand() % 256);
}

void Chip8::draw(uint8_t regLoc1, uint8_t regLoc2, uint8_t spriteHeight) {
    uint8_t xCoord = V[regLoc1] % 64, yCoord = V[regLoc2] % 32;
    V[0xF] = 0;
    for(int spriteRow = 0; spriteRow < spriteHeight; ++spriteRow) {
        int Y = yCoord + spriteRow;
        if(Y >= 32)
            break;
        uint8_t spriteRowData = memory[indexRegister + spriteRow];
        for(int spriteBit = 0; spriteBit < 8; ++spriteBit) {
            int X = xCoord + spriteBit;
            if(X >= 64)
                break;
            bool spritePixel = (spriteRowData & (1 << (7 - spriteBit))) != 0;
            if(spritePixel && display[Y][X])
                display[Y][X] = false, V[0xF] = 1;
            else if(spritePixel && !display[Y][X])
                display[Y][X] = true;
        }
    }
} 

void Chip8::skipIfKey(uint8_t regLoc) {
    if(keys[V[regLoc]])
        PC += 2;
}

void Chip8::skipIfNotKey(uint8_t regLoc) {
    if(!keys[V[regLoc]])
        PC += 2;
}

void Chip8::setXtoDelay(uint8_t regLoc) {
    V[regLoc] = delayTimer;
}

void Chip8::setDelayToX(uint8_t regLoc) {
    delayTimer = V[regLoc];
}

void Chip8::setSoundToX(uint8_t regLoc) {
    soundTimer = V[regLoc];
}

void Chip8::addToIndex(uint8_t regLoc) {
    indexRegister += V[regLoc];
    
    // handle overflow
    if(indexRegister < V[regLoc])
        V[0xF] = 0x1;
}

void Chip8::getKey(uint8_t regLoc) {
    bool noKeysPressed = true;
    uint8_t key;
    for(key = 0x0; key <= 0xF; ++key) {
        if(keys[key]) {
            noKeysPressed = false;
            break;
        }
    }
    if(noKeysPressed)
        PC -= 2;
    else
        V[regLoc] = key;
}

void Chip8::fontCharacter(uint8_t regLoc) {
    indexRegister = 0x50 + 5*N(V[regLoc]);
}

void Chip8::BCDconvert(uint8_t regLoc) {
    uint8_t temp = V[regLoc];
    for(int i = 2; i >= 0; i--) {
        memory[indexRegister + i] = temp % 10;
        temp /= 10;
    }
}

void Chip8::store(uint8_t memLoc) {
    for(uint8_t i = 0x0; i <= memLoc; ++i)
        memory[indexRegister + i] = V[i];

    if(loadAndStoreIdxInc)
        indexRegister += memLoc + 1;
}

void Chip8::load(uint8_t memLoc) {
    for(uint8_t i = 0x0; i <= memLoc; ++i)
        V[i] = memory[indexRegister + i];

    if(loadAndStoreIdxInc)
        indexRegister += memLoc + 1;
}

