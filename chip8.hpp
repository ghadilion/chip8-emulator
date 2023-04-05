#pragma once

#include <cstdint>
#include <stack>

class Chip8 {

    uint8_t memory[4096];
    uint16_t programCounter;
    uint16_t indexRegister;
    std::stack<uint16_t> Stack;
    uint8_t variableRegisters[16];

    // instructions

    void clearScreen();                     // 00E0
    void _return();                         // 00EE
    void jump(uint16_t);                    // 1NNN
    void subroutine(uint16_t);              // 2NNN
    void skipIfEqImmed(uint8_t, uint8_t);   // 3XNN
    void skipIfNeqImmed(uint8_t, uint8_t);  // 4XNN
    void skipIfEq(uint8_t, uint8_t);        // 5XY0
    void setImmed(uint8_t, uint8_t);        // 6XNN
    void addImmed(uint8_t, uint8_t);        // 7XNN
    void set(uint8_t, uint8_t);             // 8XY0
    void _or(uint8_t, uint8_t);             // 8XY1
    void _and(uint8_t, uint8_t);            // 8XY2
    void _xor(uint8_t, uint8_t);            // 8XY3
    void add(uint8_t, uint8_t);             // 8XY4
    void subXY(uint8_t, uint8_t);           // 8XY5
    void shiftRight(uint8_t, uint8_t);      // 8XY6
    void subYX(uint8_t, uint8_t);           // 8XY7
    void shiftLeft(uint8_t, uint8_t);       // 8XYE
    void skipIfNeq(uint8_t, uint8_t);       // 9XY0
    void setIndex(uint16_t);                // ANNN
    void jumpWithOffset(uint16_t);          // BNNN
    void random(uint8_t, uint8_t);          // CXNN
    void draw(uint8_t, uint8_t, uint8_t);   // DXYN
    void skipIfKey(uint8_t);                // EX9E
    void skipIfNotKey(uint8_t);             // EXA1
    void setXtoDelay(uint8_t);              // FX07
    void setDelayToX(uint8_t);              // FX15
    void setSoundToX(uint8_t);              // FX18
    void addToIndex(uint8_t);               // FX1E
    void getKey(uint8_t);                   // FXOA
    void fontCharacter(uint8_t);            // FX29
    void BCDconvert(uint8_t);               // FX33
    void store(uint8_t);                    // FX55
    void load(uint8_t);                     // FX65

    // ambiguous instruction config

    bool setAndShift;                       // if true, set value of VX to VY 
                                            // before shift operations 8XY6 and 
                                            // 8XYE; if false, VX remains 
                                            // unchanged before shift

    bool jumpOffsetVariable;                // if true, jump with offset 
                                            // instruction BXNN jumps to the 
                                            // address XNN, plus the value in 
                                            // the register VX; if false, BNNN
                                            // jumps to address NNN plus the 
                                            // value in register V0

    bool loadAndStoreIdxInc;                // if true, set index register (I) 
                                            // value to I + X + 1 after 
                                            // executing load (FX65) or store 
                                            // (FX55) instructions; if false, I
                                            // remains unchanged

public:
    
    Chip8(bool, bool, bool, char *game);
    bool display[32][64];
    bool keys[16];
    uint8_t delayTimer;
    uint8_t soundTimer;
    uint16_t fetch();
    bool decode(uint16_t);

};

