#include <stdlib.h>
#include "SDL.h"

int main(int argc, char** argv)
{

  SDL_Surface *screen;

  if(SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }

  atexit(SDL_Quit);

  fprintf(stderr, "SDL Inited!\n");

  screen = SDL_SetVideoMode(640, 480, 8, SDL_HWSURFACE);

  if(screen == NULL) {
    fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
    exit(1);
  }

  fprintf(stderr, "Video mode set to 640x480x8bpp\n");

  return 0;
}




