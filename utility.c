#include "emulator.h"

uint8_t get_flagbyte() {
  uint8_t flagbyte = 0;
  flagbyte = 0x02 | ((1 << 7) & state.flag_s) | ((1 << 6) & state.flag_z) |
    ((1 << 4) & state.flag_ac) | ((1 << 2) & state.flag_p) | state.flag_cy;

  return flagbyte;
}

void restore_flags(uint8_t flagbyte) {
  state.flag_s = flagbyte & (1 << 7);
  state.flag_z = flagbyte & (1 << 6);
  state.flag_ac = flagbyte & (1 << 4);
  state.flag_p = flagbyte & (1 << 2);
  state.flag_cy = flagbyte & 1;
}

int check_parity(uint8_t byte) {
  unsigned int count = 0, i, b = 1;

  for (i = 0; i < 8; i++) {
    if (byte & (b << i) )
      count++;
  }

  return !(count % 2);
}
