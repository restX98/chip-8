#ifndef CHIP8_HPP
#define CHIP8_HPP

#define DEBUG 1

#include <array>

class Chip8 {
public:
  Chip8(const char* filename);
  // ~Chip8();

  void loadGame(const char* filename);
  void emulateCycle();
  void pressKeys(unsigned char key);
  void releaseKeys(unsigned char key);

  static constexpr unsigned char WIDTH = 64;
  static constexpr unsigned char HEIGHT = 32;

  std::array<std::array<bool, WIDTH>, HEIGHT> getGraphics() const;
  // unsigned char getDelayTimer();
  // unsigned char getSoundTimer();

  bool getDrawFlag();

private:
  static constexpr unsigned char fontset[80] = {
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

  // Chip 8 has 35 opcodes
  // Each opcode is 2 bytes long
  unsigned short opcode;

  // Chip 8 has 4K memory in total
  unsigned char memory[4096];

  //  Chip 8 has 15 8-bit general purpose registers named V0,V1 up to VE.
  // The 16th register is used for the ‘carry flag’.
  unsigned char V[16];

  // Index Register and Program Counter
  // which can have a value from 0x000 to 0xFFF
  unsigned short I;
  unsigned short pc;

  // Systems memory map:
  // 0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
  // 0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
  // 0x200-0xFFF - Program ROM and work RAM

  // Chip 8 are black and white and the screen has a total of 2048 pixels (64 x
  // 32)
  bool gfx[WIDTH * HEIGHT];

  // Interupts and hardware registers.
  // The Chip 8 has none, but there are two timer registers that count at 60 Hz.
  // When set above zero they will count down to zero.
  unsigned char delay_timer;
  unsigned char sound_timer;

  // The system has 16 levels of stack
  unsigned short stack[16];
  unsigned short sp;

  // Chip 8 has a HEX based keypad (0x0-0xF)
  unsigned char key[16];

  bool drawFlag;

  void d_printf(const char* format, ...);
};

#endif
