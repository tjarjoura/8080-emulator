#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include "emulator.h"

#define WIDTH  224
#define HEIGHT 256

#define MIDDLE 0
#define BOTTOM 1

SDL_Window *window;
SDL_Surface *window_surface;
SDL_Surface *video;


void quit_sdl() {
  SDL_FreeSurface(video);
  SDL_DestroyWindow(window);
  SDL_Quit();
  exit(1);
}

void initialize_sdl() {
  signal(SIGINT, quit_sdl);

  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0)
    die_error("SDL_Init(): %s\n", SDL_GetError());

  if ((window = SDL_CreateWindow("8080 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
          WIDTH*4, HEIGHT*4, 0)) == NULL)
      die_error("SDL_CreateWindow(): %s\n", SDL_GetError());

  if ((window_surface = SDL_GetWindowSurface(window)) == NULL)
    die_error("SDL_GetWindowSurface(): %s\n", SDL_GetError());

  if((video = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0, 0, 0, 0)) == NULL)
    die_error("SDL_CreateRGBSurface(): %s\n", SDL_GetError());

}

void blit_video() {
  SDL_Rect src_rect;
  SDL_Rect dst_rect;

  /* Original space invaders was only 256x224 -- scale x4 */
  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.w = WIDTH;
  src_rect.h = HEIGHT;

  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.w = 4 * WIDTH;
  dst_rect.h = 4 * HEIGHT;

  if (SDL_BlitScaled(video, &src_rect, window_surface, &dst_rect) < 0)
    die_error("SDL_BlitScaled: %s\n", SDL_GetError());
}

int get_rotated_index(int i) {
  int col = i / HEIGHT;
  int row = HEIGHT - (i % HEIGHT) - 1;

  //usleep(50000);
  //printf("Got row %d, col %d, i:%d for row %d, col %d, i:%d\n", row, col, WIDTH * row + col, i / HEIGHT, i % HEIGHT, i);
  return WIDTH * row + col;
}

void copy_half(int half) {
  int i, j, k;
  uint16_t high, low;
  uint32_t *pixels = video->pixels;
  uint8_t byte;

  /* Video RAM is addresses 0x2400-0x3FFF, 256x224 resolution */
  /* We want to copy half of the screen every 1/60 seconds */
  /* Byte 0x3400 is what space invaders considers the "middle" */

  if (half == MIDDLE) {
    low = 0x2400;
    high = 0x3400;
    i = 0;
  } else {
    low = 0x3400;
    high = 0x4000;
    i = 0x1000 * 8; /* One bit per pixel */
  }

  for (j = low; j < high; j++) {
    byte = memory[j];

    for (k = 0; k < 8; k++) {
      if (byte & 0x01) /* Pixels are either on or off, I use white and black for the colors */
        *(pixels + get_rotated_index(i)) = SDL_MapRGB(video->format, 255, 255, 255);
      else
        *(pixels + get_rotated_index(i)) = 0;

      byte >>= 1;
      i++;
    }
  }
}

/* Simulates the display used by space invaders */
void display() {
  static clock_t last_interrupt = 0;
  static int loc = MIDDLE;

  if (state.interrupts_enabled && (((double) (clock() - last_interrupt) / (double) CLOCKS_PER_SEC) > 1.0 / 60.0)) {
    copy_half(loc);

    switch (loc) {
      case MIDDLE:
        interrupt(0xcf); /* RST 1 */
        break;
      case BOTTOM:
        interrupt(0xd7); /* RST 7 */
        break;
    }

    blit_video();
    SDL_UpdateWindowSurface(window);

    loc = !loc;
    last_interrupt = clock();
  }
}

void input() {
  SDL_Event event;
  uint8_t inp1 = 0x08, inp2;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT)
      quit_sdl();

    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_c:
          inp1 |= 0x01; /* Insert quarter */
          break;

        case SDLK_RETURN: /* Start */
          inp1 |= 0x04;
          break;
        case SDLK_SPACE: /* Fire */
          inp1 |= 0x10;
          break;
        case SDLK_LEFT: /* Left */
          inp1 |= 0x20;
          break;
        case SDLK_RIGHT: /* Right */
          inp1 |= 0x40;
          break;
      }
    }
  }

  state.input_pins[1] = inp1; /* Space Invaders will read the input from here */
}
