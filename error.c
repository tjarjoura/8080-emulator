#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "emulator.h"

void die_error(char *format, ...) {
  va_list aptr;

  fprintf(stderr, "[ERROR] ");
  va_start(aptr, format);
  vfprintf(stderr, format, aptr);
  va_end(aptr);

  fprintf(stderr, "Exiting.\n");
  quit_sdl();
}
