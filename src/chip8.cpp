#include "chip8.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <cstdarg>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

Chip8::Chip8(const char* filename) {
  pc = 0x200; // Program counter starts at 0x200
  opcode = 0; // Reset opcode
  I = 0;      // Reset index register
  sp = 0;     // Reset stack pointer

  // Clear display
  for (int i = 0; i < ARRAY_SIZE(gfx); i++) {
    gfx[i] = false;
  }

  // Clear stack
  for (int i = 0; i < ARRAY_SIZE(stack); i++) {
    stack[i] = 0;
  }

  // Clear registers V0-VF
  for (int i = 0; i < ARRAY_SIZE(V); i++) {
    V[i] = 0;
  }

  // Clear memory
  for (int i = 0; i < ARRAY_SIZE(memory); i++) {
    memory[i] = 0;
  }

  // Load fontset
  for (int i = 0; i < 80; ++i) {
    memory[i] = fontset[i];
  }

  // Reset timers
  // TODO: to understand how timers work in Chip-8
  delay_timer = 0;
  sound_timer = 0;

  drawFlag = false; // Initialize draw flag

  srand(time(NULL)); // Seed the random number generator

  loadGame(filename);
}

void Chip8::pressKeys(unsigned char key) {
  if (key < 16) {
    this->key[key] = true;
  }
}

void Chip8::releaseKeys(unsigned char key) {
  if (key < 16) {
    this->key[key] = false;
  }
}

void Chip8::loadGame(const char* filename) {
  std::ifstream file{ filename, std::ios::binary | std::ios::ate };
  if (!file) {
    std::cerr << "Could not open game file: " << filename << std::endl;
    // TODO: handle error properly
    return;
  }

  std::streamoff pos = file.tellg();
  if (pos < 0) {
    std::cerr << "Failed to determine file size\n";
    return;
  }
  std::size_t fileSize = static_cast<std::size_t>(pos);

  const std::size_t startAddress = 0x200;
  const std::size_t maxSize = ARRAY_SIZE(memory) - startAddress;
  if (fileSize > maxSize) {
    std::cerr << "Game file too large to fit in memory\n";
    return;
  }

  file.seekg(0, std::ios::beg);
  file.read(
    reinterpret_cast<char*>(memory + startAddress),
    static_cast<std::streamsize>(fileSize)
  );

  if (static_cast<std::size_t>(file.gcount()) != fileSize) {
    std::cerr << "Read error: only " << file.gcount() << " bytes were read\n";
    return;
  }
}

void Chip8::emulateCycle() {
  unsigned short opcode =
    memory[pc] << 8 | memory[pc + 1];

  switch (opcode & 0xF000) {
    case 0x0000:
      if (opcode == 0x00E0) { // 00E0 - Clears the screen
        for (int i = 0; i < ARRAY_SIZE(gfx); i++) {
          gfx[i] = false;
        }
        drawFlag = true;
        pc += 2;
        d_printf("%04X: Clear the screen\n", opcode);
      } else if (opcode == 0x00EE) { // 00EE - Returns from a subroutine
        sp--;
        pc = stack[sp];
        pc += 2;
        d_printf("%04X: Return from subroutine\n", opcode);
      } else {
        d_printf("Unknown opcode: 0x%X\n", opcode);
        exit(1);
      }
      break;

    case 0x1000: // 1NNN - Jump to address NNN
      pc = opcode & 0x0FFF;
      d_printf("%X: Jump to address 0x%03X\n", opcode, opcode & 0x0FFF);
      break;

    case 0x2000: // 2NNN - Calls subroutine at NNN
      stack[sp] = pc; // Push current PC onto stack
      sp++; // Increment stack pointer
      pc = opcode & 0x0FFF; // Set PC to NNN
      d_printf("%X: Call subroutine at 0x%03X\n", opcode, opcode & 0x0FFF);
      break;

    case 0x3000: // 3XNN - Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block).
      if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
        pc += 4; // Skip the next instruction
        d_printf("%X: Skip next instruction, V%X == %02X\n", opcode, (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      } else {
        pc += 2; // Move to the next instruction
        d_printf("%X: Do not skip next instruction, V%X != %02X\n", opcode, (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      }
      break;

    case 0x4000: // 4XNN - Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
      if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
        pc += 4; // Skip the next instruction
        d_printf("%X: Skip next instruction, V%X != %02X\n", opcode, (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      } else {
        pc += 2; // Move to the next instruction
        d_printf("%X: Do not skip next instruction, V%X == %02X\n", opcode, (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      }
      break;

    case 0x6000: // 6XNN - Sets VX to NN
      V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
      pc += 2;
      d_printf("%X: Set V%X to %d\n", opcode, (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;

    case 0x7000: // 7XNN - Adds NN to VX (carry flag is not changed).
      V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
      pc += 2;
      d_printf("%X: Add %02X to V%X\n", opcode, opcode & 0x00FF, (opcode & 0x0F00) >> 8);
      break;

    case 0x8000:
      switch (opcode & 0x000F) {
        case 0x0000: // 8XY0 - Sets VX to the value of VY.
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
          pc += 2;
          d_printf("%X: Set V%X to V%X\n", opcode, (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;

        case 0x0002: // 8XY2 - Sets VX to VX and VY. (bitwise AND operation).
          V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
          pc += 2;
          d_printf("%X: Set V%X to V%X AND V%X\n", opcode, (opcode & 0x0F00) >> 8, (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;

        case 0x0004: { // 8XY4 - Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not.
          unsigned char vx = V[(opcode & 0x0F00) >> 8];
          unsigned char vy = V[(opcode & 0x00F0) >> 4];
          V[(opcode & 0x0F00) >> 8] += vy;
          V[0xF] = (vx + vy > 255) ? 1 : 0; // Set carry flag
          pc += 2;
          d_printf("%X: Add V%X to V%X, VF set to %d\n", opcode, (opcode & 0x00F0) >> 4, (opcode & 0x0F00) >> 8, V[0xF]);
          break;
        }

        case 0x0005: { // 8XY5 - VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not).
          unsigned char vx = V[(opcode & 0x0F00) >> 8];
          unsigned char vy = V[(opcode & 0x00F0) >> 4];
          V[0xF] = (vx > vy) ? 1 : 0; // Set carry flag
          V[(opcode & 0x0F00) >> 8] -= vy;
          pc += 2;
          d_printf("%X: Subtract V%X from V%X, VF set to %d\n", opcode, (opcode & 0x00F0) >> 4, (opcode & 0x0F00) >> 8, V[0xF]);
          break;
        }

        default:
          d_printf("Unknown opcode: 0x%04X\n", opcode);
          exit(1);
      }
      break;

    case 0xA000: // ANNN: Sets I to the address NNN
      I = opcode & 0x0FFF;
      pc += 2;
      d_printf("%X: Set I to 0x%03X\n", opcode, opcode & 0x0FFF);
      break;

    case 0xC000: { // CXNN - Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
      unsigned char rand_num = rand() % 256; // Generate a random number
      V[(opcode & 0x0F00) >> 8] = rand_num & (opcode & 0x00FF);
      pc += 2;
      d_printf("%X: Set V%X to %d(random) AND %02X\n", opcode, (opcode & 0x0F00) >> 8, rand_num, opcode & 0x00FF);
      break;
    }

    case 0xD000: { // DXYN - Draws a sprite at coordinate (VX, VY) with N bytes of sprite data starting at the address stored in I.
      unsigned char x = V[(opcode & 0x0F00) >> 8];
      unsigned char y = V[(opcode & 0x00F0) >> 4];
      unsigned char height = opcode & 0x000F;

      for (int i = 0; i < height; i++) {
        unsigned char sprite_byte = memory[I + i];
        for (int j = 0; j < 8; j++) {
          unsigned char pixel = sprite_byte & (0x80 >> j);
          if (pixel) {
            if (gfx[(x + j) + ((y + i) * WIDTH)]) {
              V[0xF] = 1; // Set collision flag
            }
            gfx[(x + j) + ((y + i) * WIDTH)] ^= true;
          }
        }
      }

      drawFlag = true;
      pc += 2;

      d_printf("%X: Draw sprite at (%d, %d) with height %d\n", opcode, x, y, height);
      break;
    }

    case 0xE000:
      switch (opcode & 0x00FF) {
        // case 0x009E: // EX9E - Skips the next instruction if the key stored in VX is pressed.
        //   if (key[V[(opcode & 0x0F00) >> 8]]) {
        //     pc += 4; // Skip next instruction
        //     d_printf("%X: Skip next instruction, key V%X is pressed\n", opcode, (opcode & 0x0F00) >> 8);
        //   } else {
        //     pc += 2; // Move to next instruction
        //     d_printf("%X: Do not skip next instruction, key V%X is not pressed\n", opcode, (opcode & 0x0F00) >> 8);
        //   }
        //   break;

        case 0x00A1: // EXA1 - Skips the next instruction if the key stored in VX(only consider the lowest nibble) is not pressed (usually the next instruction is a jump to skip a code block).
          if (!key[V[(opcode & 0x0F00) >> 8]]) {
            pc += 4; // Skip next instruction
            d_printf("%X: Skip next instruction, key V%X is not pressed\n", opcode, (opcode & 0x0F00) >> 8);
          } else {
            pc += 2; // Move to next instruction
            d_printf("%X: Do not skip next instruction, key V%X is pressed\n", opcode, (opcode & 0x0F00) >> 8);
          }
          break;

        default:
          d_printf("Unknown opcode: 0x%04X\n", opcode);
          exit(1);
      }
      break;

    case 0xF000:
      switch (opcode & 0x00FF) {
        case 0x0007: // FX07 - Sets VX to the value of the delay timer.
          V[(opcode & 0x0F00) >> 8] = delay_timer;
          pc += 2;
          d_printf("%X: Set V%X to delay timer value %d\n", opcode, (opcode & 0x0F00) >> 8, delay_timer);
          break;

        case 0x0015: // FX15 - Sets the delay timer to VX.
          delay_timer = V[(opcode & 0x0F00) >> 8];
          pc += 2;
          d_printf("%X: Set delay timer to V%X\n", opcode, (opcode & 0x0F00) >> 8);
          break;

        case 0x0018: // FX18 - Sets the sound timer to VX.
          sound_timer = V[(opcode & 0x0F00) >> 8];
          pc += 2;
          d_printf("%X: Set sound timer to V%X\n", opcode, (opcode & 0x0F00) >> 8);
          break;

        case 0x0029: // FX29 - Sets I to the location of the sprite for the character in VX(only consider the lowest nibble). Characters 0-F (in hexadecimal) are represented by a 4x5 font.
          I = V[(opcode & 0x0F00) >> 8] * 0x5;
          pc += 2;
          d_printf("%X: Set I to sprite location for V%X\n", opcode, (opcode & 0x0F00) >> 8);
          break;

        case 0x0033: { // FX33 - Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
          unsigned short bcd = V[(opcode & 0x0F00) >> 8];
          memory[I] = bcd / 100;
          memory[I + 1] = (bcd / 10) % 10;
          memory[I + 2] = bcd % 10;
          pc += 2;
          d_printf("%X: Store binary-coded decimal representation of V%X at I: %d, %d, %d\n",
            opcode,
            (opcode & 0x0F00) >> 8,
            memory[I],
            memory[I + 1],
            memory[I + 2]
          );
          break;
        }
        case 0x0065: // FX65 - Fills from V0 to VX (including VX) with values from memory, starting at address I. The offset from I is increased by 1 for each value read, but I itself is left unmodified.
          for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
            V[i] = memory[I + i];
          }
          pc += 2;
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
  if (delay_timer > 0)
    --delay_timer;

  if (sound_timer > 0) {
    if (sound_timer == 1)
      printf("BEEP!\n");
    --sound_timer;
  }
}

std::array<std::array<bool, Chip8::WIDTH>, Chip8::HEIGHT> Chip8::getGraphics() const {
  std::array<std::array<bool, WIDTH>, HEIGHT> graphics = {};
  for (size_t y = 0; y < HEIGHT; ++y) {
    for (size_t x = 0; x < WIDTH; ++x) {
      graphics[y][x] = gfx[x + (y * WIDTH)];
    }
  }
  return graphics;
}

void Chip8::d_printf(const char* format, ...) {
  // TODO: port to c++ style
  if (DEBUG) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
  }
}

bool Chip8::getDrawFlag() const {
  return drawFlag;
}
