#include <stdio.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define WIDTH 64
#define HEIGHT 32

unsigned char chip8_fontset[80] = {
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

typedef struct {
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
  unsigned char gfx[64 * 32];

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
} Chip8;

void chip8_initialize(Chip8* chip8);
void chip8_loadGame(Chip8* chip8, const char* filename);

int main() {
  Chip8 chip8;

  chip8_initialize(&chip8);
  chip8_loadGame(&chip8, "games/br8kout.ch8");

  return 0;
}

void chip8_initialize(Chip8* chip8) {
  // Initialize the Chip-8 system
  chip8->pc = 0x200; // Program counter starts at 0x200
  chip8->opcode = 0; // Reset opcode
  chip8->I = 0;      // Reset index register
  chip8->sp = 0;     // Reset stack pointer

  // Clear display
  for (int i = 0; i < ARRAY_SIZE(chip8->gfx); i++) {
    chip8->gfx[i] = 0;
  }

  // Clear stack
  for (int i = 0; i < ARRAY_SIZE(chip8->stack); i++) {
    chip8->stack[i] = 0;
  }

  // Clear registers V0-VF
  for (int i = 0; i < ARRAY_SIZE(chip8->V); i++) {
    chip8->V[i] = 0;
  }

  // Clear memory
  for (int i = 0; i < ARRAY_SIZE(chip8->memory); i++) {
    chip8->memory[i] = 0;
  }

  // Load fontset
  for (int i = 0; i < 80; ++i) {
    chip8->memory[i] = chip8_fontset[i];
  }

  // Reset timers
  // TODO: to understand how timers work in Chip-8
  chip8->delay_timer = 0;
  chip8->sound_timer = 0;
}

void chip8_loadGame(Chip8* chip8, const char* filename) {
  // Load a Chip-8 game into memory
  FILE* file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Could not open game file: %s\n", filename);
    return;
  }

  // Read the game into memory starting at 0x200
  fread(chip8->memory + 0x200, sizeof(chip8->memory[0]),
    ARRAY_SIZE(chip8->memory) - 0x200, file);
  fclose(file);
}
