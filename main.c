#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define DEBUG 1

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

  // Draw flag to indicate if the screen needs to be updated
  unsigned char drawFlag;
} Chip8;

void d_printf(const char* format, ...) {
  if (DEBUG) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
  }
}

void chip8_initialize(Chip8* chip8);
void chip8_loadGame(Chip8* chip8, const char* filename);
void chip8_emulateCycle(Chip8* chip8);
void chip8_drawGraphics(Chip8* chip8);

int main() {
  Chip8 chip8;

  chip8_initialize(&chip8);
  chip8_loadGame(&chip8, "games/pong2.c8");

  while (1) {
    chip8_emulateCycle(&chip8);

    // If the draw flag is set, update the screen
    if (chip8.drawFlag)
      chip8_drawGraphics(&chip8); // Implement graphics drawing

  }
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

  chip8->drawFlag = 0; // Initialize draw flag

  srand(time(NULL)); // Seed the random number generator
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

void chip8_emulateCycle(Chip8* chip8) {
  unsigned short opcode =
    chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc + 1];

  switch (opcode & 0xF000) {
    case 0x0000:
      if (opcode == 0x00E0) { // 00E0 - Clears the screen
        for (int i = 0; i < ARRAY_SIZE(chip8->gfx); i++) {
          chip8->gfx[i] = 0;
        }
        chip8->drawFlag = 1;
        chip8->pc += 2;
        d_printf("%04X: Clear the screen\n", opcode);
      } else if (opcode == 0x00EE) { // 00EE - Returns from a subroutine
        chip8->sp--;
        chip8->pc = chip8->stack[chip8->sp];
        chip8->pc += 2;
        d_printf("%04X: Return from subroutine\n", opcode);
      } else {
        d_printf("Unknown opcode: 0x%X\n", opcode);
        exit(1);
      }
      break;

    case 0x1000: // 1NNN - Jump to address NNN
      chip8->pc = opcode & 0x0FFF;
      d_printf("%X: Jump to address 0x%03X\n", opcode, opcode & 0x0FFF);
      break;

    case 0x2000: // 2NNN - Calls subroutine at NNN
      chip8->stack[chip8->sp] = chip8->pc; // Push current PC onto stack
      chip8->sp++; // Increment stack pointer
      chip8->pc = opcode & 0x0FFF; // Set PC to NNN
      d_printf("%X: Call subroutine at 0x%03X\n", opcode, opcode & 0x0FFF);
      break;

    case 0x3000: // 3XNN - Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block).
      if (chip8->V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
        chip8->pc += 4; // Skip the next instruction
        d_printf("%X: Skip next instruction, V%X == %02X\n", opcode, (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      } else {
        chip8->pc += 2; // Move to the next instruction
        d_printf("%X: Do not skip next instruction, V%X != %02X\n", opcode, (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      }
      break;

    case 0x4000: // 4XNN - Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
      if (chip8->V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
        chip8->pc += 4; // Skip the next instruction
        d_printf("%X: Skip next instruction, V%X != %02X\n", opcode, (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      } else {
        chip8->pc += 2; // Move to the next instruction
        d_printf("%X: Do not skip next instruction, V%X == %02X\n", opcode, (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      }
      break;

    case 0x6000: // 6XNN - Sets VX to NN
      chip8->V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
      chip8->pc += 2;
      d_printf("%X: Set V%X to %d\n", opcode, (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;

    case 0x7000: // 7XNN - Adds NN to VX (carry flag is not changed).
      chip8->V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
      chip8->pc += 2;
      d_printf("%X: Add %02X to V%X\n", opcode, opcode & 0x00FF, (opcode & 0x0F00) >> 8);
      break;

    case 0x8000:
      switch (opcode & 0x000F) {
        case 0x0000: // 8XY0 - Sets VX to the value of VY.
          chip8->V[(opcode & 0x0F00) >> 8] = chip8->V[(opcode & 0x00F0) >> 4];
          chip8->pc += 2;
          d_printf("%X: Set V%X to V%X\n", opcode, (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;

        case 0x0002: // 8XY2 - Sets VX to VX and VY. (bitwise AND operation).
          chip8->V[(opcode & 0x0F00) >> 8] &= chip8->V[(opcode & 0x00F0) >> 4];
          chip8->pc += 2;
          d_printf("%X: Set V%X to V%X AND V%X\n", opcode, (opcode & 0x0F00) >> 8, (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;

        case 0x0004: { // 8XY4 - Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not.
          unsigned char vx = chip8->V[(opcode & 0x0F00) >> 8];
          unsigned char vy = chip8->V[(opcode & 0x00F0) >> 4];
          chip8->V[(opcode & 0x0F00) >> 8] += vy;
          chip8->V[0xF] = (vx + vy > 255) ? 1 : 0; // Set carry flag
          chip8->pc += 2;
          d_printf("%X: Add V%X to V%X, VF set to %d\n", opcode, (opcode & 0x00F0) >> 4, (opcode & 0x0F00) >> 8, chip8->V[0xF]);
          break;
        }

        default:
          d_printf("Unknown opcode: 0x%04X\n", opcode);
          exit(1);
      }
      break;

    case 0xA000: // ANNN: Sets I to the address NNN
      chip8->I = opcode & 0x0FFF;
      chip8->pc += 2;
      d_printf("%X: Set I to 0x%03X\n", opcode, opcode & 0x0FFF);
      break;

    case 0xC000: { // CXNN - Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
      unsigned char rand_num = rand() % 256; // Generate a random number
      chip8->V[(opcode & 0x0F00) >> 8] = rand_num & (opcode & 0x00FF);
      chip8->pc += 2;
      d_printf("%X: Set V%X to %d(random) AND %02X\n", opcode, (opcode & 0x0F00) >> 8, rand_num, opcode & 0x00FF);
      break;
    }

    case 0xD000: { // DXYN - Draws a sprite at coordinate (VX, VY) with N bytes of sprite data starting at the address stored in I.
      unsigned char x = chip8->V[(opcode & 0x0F00) >> 8];
      unsigned char y = chip8->V[(opcode & 0x00F0) >> 4];
      unsigned char height = opcode & 0x000F;

      for (int i = 0; i < height; i++) {
        unsigned char sprite_byte = chip8->memory[chip8->I + i];
        for (int j = 0; j < 8; j++) {
          unsigned char pixel = sprite_byte & (0x80 >> j);
          if (pixel) {
            if (chip8->gfx[(x + j) + ((y + i) * WIDTH)] == 1) {
              chip8->V[0xF] = 1; // Set collision flag
            }
            chip8->gfx[(x + j) + ((y + i) * WIDTH)] ^= 1;
          }
        }
      }

      chip8->gfx[x + (y * WIDTH)] = 1;
      chip8->drawFlag = 1;
      chip8->pc += 2;

      d_printf("%X: Draw sprite at (%d, %d) with height %d\n", opcode, x, y, height);
      break;
    }

    case 0xF000:
      switch (opcode & 0x00FF) {
        case 0x0007: // FX07 - Sets VX to the value of the delay timer.
          chip8->V[(opcode & 0x0F00) >> 8] = chip8->delay_timer;
          chip8->pc += 2;
          d_printf("%X: Set V%X to delay timer value %d\n", opcode, (opcode & 0x0F00) >> 8, chip8->delay_timer);
          break;

        case 0x0015: // FX15 - Sets the delay timer to VX.
          chip8->delay_timer = chip8->V[(opcode & 0x0F00) >> 8];
          chip8->pc += 2;
          d_printf("%X: Set delay timer to V%X\n", opcode, (opcode & 0x0F00) >> 8);
          break;

        case 0x0018: // FX18 - Sets the sound timer to VX.
          chip8->sound_timer = chip8->V[(opcode & 0x0F00) >> 8];
          chip8->pc += 2;
          d_printf("%X: Set sound timer to V%X\n", opcode, (opcode & 0x0F00) >> 8);
          break;

        case 0x0029: // FX29 - Sets I to the location of the sprite for the character in VX(only consider the lowest nibble). Characters 0-F (in hexadecimal) are represented by a 4x5 font.
          chip8->I = chip8->V[(opcode & 0x0F00) >> 8] * 0x5;
          chip8->pc += 2;
          d_printf("%X: Set I to sprite location for V%X\n", opcode, (opcode & 0x0F00) >> 8);
          break;

        case 0x0033: // FX33 - Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
          unsigned short bcd = chip8->V[(opcode & 0x0F00) >> 8];
          chip8->memory[chip8->I] = bcd / 100;
          chip8->memory[chip8->I + 1] = (bcd / 10) % 10;
          chip8->memory[chip8->I + 2] = bcd % 10;
          chip8->pc += 2;
          d_printf("%X: Store binary-coded decimal representation of V%X at I: %d, %d, %d\n",
            opcode,
            (opcode & 0x0F00) >> 8,
            chip8->memory[chip8->I],
            chip8->memory[chip8->I + 1],
            chip8->memory[chip8->I + 2]
          );
          break;

        case 0x0065: // FX65 - Fills from V0 to VX (including VX) with values from memory, starting at address I. The offset from I is increased by 1 for each value read, but I itself is left unmodified.
          for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
            chip8->V[i] = chip8->memory[chip8->I + i];
          }
          chip8->pc += 2;
          d_printf("%X: Fill V0 to V%X with values from memory starting at I\n", opcode, (opcode & 0x0F00) >> 8);
          break;

        default:
          d_printf("Unknown opcode: 0x%04X\n", opcode);
          exit(1);
      }
      break;

    default:
      d_printf("Unknown opcode: 0x%04X\n", opcode);
      exit(1);
  }

  // Update timers
  if (chip8->delay_timer > 0)
    --chip8->delay_timer;

  if (chip8->sound_timer > 0) {
    if (chip8->sound_timer == 1)
      printf("BEEP!\n");
    --chip8->sound_timer;
  }
}

void chip8_drawGraphics(Chip8* chip8) {
  // system("clear"); // Clear the console (for Linux/Unix)
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      unsigned char pixel = chip8->gfx[x + (y * WIDTH)];
      // Render the pixel (this is just a placeholder)
      if (pixel) {
        printf("█");
      } else {
        printf(" ");
      }
    }
    printf("\n");
  }
  chip8->drawFlag = 0;
}
