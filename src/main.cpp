#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <unordered_map>

#include "chip8.hpp"

#define APP_NAME "Chip-8 Emulator"
#define APP_VERSION "1.0"
#define APP_IDENTIFIER "com.emulators.chip8"

#define WINDOW_RESIZABLE false

#define SCALE 30
#define WIDTH 64
#define HEIGHT 32

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;

static Chip8* chip8 = nullptr;

const std::unordered_map<SDL_Scancode, unsigned char> scancode_to_chip8 = {
  { SDL_SCANCODE_1, 0x1 },
  { SDL_SCANCODE_2, 0x2 },
  { SDL_SCANCODE_3, 0x3 },
  { SDL_SCANCODE_4, 0xC },
  { SDL_SCANCODE_Q, 0x4 },
  { SDL_SCANCODE_W, 0x5 },
  { SDL_SCANCODE_E, 0x6 },
  { SDL_SCANCODE_R, 0xD },
  { SDL_SCANCODE_A, 0x7 },
  { SDL_SCANCODE_S, 0x8 },
  { SDL_SCANCODE_D, 0x9 },
  { SDL_SCANCODE_F, 0xE },
  { SDL_SCANCODE_Z, 0xA },
  { SDL_SCANCODE_X, 0x0 },
  { SDL_SCANCODE_C, 0xB },
  { SDL_SCANCODE_V, 0xF }
};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  SDL_SetAppMetadata(APP_NAME, APP_VERSION, APP_IDENTIFIER);

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("examples/renderer/primitives", WIDTH * SCALE, HEIGHT * SCALE, 0, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetWindowResizable(window, WINDOW_RESIZABLE);

  chip8 = new Chip8("../games/tetris.c8");

  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
  }
  if (event->type == SDL_EVENT_KEY_DOWN) {
    auto it = scancode_to_chip8.find(event->key.scancode);
    if (it != scancode_to_chip8.end()) {
      chip8->pressKeys(it->second);
    }
    SDL_Log("Key pressed: %s", SDL_GetScancodeName(event->key.scancode));
  } else if (event->type == SDL_EVENT_KEY_UP) {
    auto it = scancode_to_chip8.find(event->key.scancode);
    if (it != scancode_to_chip8.end()) {
      chip8->releaseKeys(it->second);
    }
    SDL_Log("Key released: %s", SDL_GetScancodeName(event->key.scancode));
  }

  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate) {

  chip8->emulateCycle();

  if (chip8->getDrawFlag()) {
    /* as you can see from this, rendering draws over whatever was drawn before it. */
    SDL_SetRenderDrawColor(renderer, 33, 33, 33, SDL_ALPHA_OPAQUE);  /* dark gray, full alpha */
    SDL_RenderClear(renderer);  /* start with a blank canvas. */


    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);  /* blue, full alpha */
    const auto graphics = chip8->getGraphics();
    for (size_t row = 0; row < graphics.size(); ++row) {
      for (size_t column = 0; column < graphics[row].size(); ++column) {
        if (graphics[row][column] != 0) {
          SDL_FRect rect;
          rect.x = column * SCALE;
          rect.y = row * SCALE;
          rect.w = SCALE;
          rect.h = SCALE;
          SDL_RenderFillRect(renderer, &rect);
        }
      }
    }

    SDL_RenderPresent(renderer);  /* put it all on the screen! */

  }

  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  /* SDL will clean up the window/renderer for us. */
}
