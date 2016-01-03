#include "emulator.h"

/* Emulates the hardware shift register used by Space Invaders */

static uint16_t shift_register;
static int shift_amount;

static uint8_t get_result() {
  return (uint8_t) (0xFF && shift_register >> (8 - shift_amount));
}

void shift_hardware(int dev, uint8_t byte) {
  switch (dev) {
    case 2:
      shift_amount = byte & 0x7;
      state.input_pins[3] = get_result(); /* Send the output to input port 3 */
      break;
    case 4:
      shift_register = ((uint16_t) byte << 8) | (shift_register >> 8);
      state.input_pins[3] = get_result();
      break;
  }
}
