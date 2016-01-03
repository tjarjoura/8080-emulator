#include <stdio.h>
#include <stdlib.h>
#include "emulator.h"

#define NZ 0
#define Z 1
#define NC 2
#define C 3
#define PO 4
#define PE 5
#define P 6
#define M 7

uint8_t get_register_content(int regnum) {
  switch (regnum) {
    case REG_B: return state.reg_b;
    case REG_C: return state.reg_c;
    case REG_D: return state.reg_d;
    case REG_E: return state.reg_e;
    case REG_H: return state.reg_h;
    case REG_L: return state.reg_l;
    case MEM_REF: return get_memory_byte();
    case REG_A: return state.reg_a;
    default:
      fprintf(stderr, "get_register_content(): Invalid regnum: %d. Exiting.\n", regnum);
      exit(-1);
  }
}

void set_register_content(int reg_num, uint8_t byte) {
  switch (reg_num) {
    case REG_B: state.reg_b = byte; break;
    case REG_C: state.reg_c = byte; break;
    case REG_D: state.reg_d = byte; break;
    case REG_E: state.reg_e = byte; break;
    case REG_H: state.reg_h = byte; break;
    case REG_L: state.reg_l = byte; break;
    case MEM_REF: set_memory_byte(byte); break;
    case REG_A: state.reg_a = byte; break;
  }
}

uint16_t get_register_pair(int regpair) {
  switch (regpair) {
    case BC:
      return ((uint16_t) state.reg_b << 8) | (uint16_t) state.reg_c;
    case DE:
      return ((uint16_t) state.reg_d << 8) | (uint16_t) state.reg_e;
    case HL:
      return ((uint16_t) state.reg_h << 8) | (uint16_t) state.reg_l;
    case SP:
      return state.sp;
    case FA:
      return ((uint16_t) get_flagbyte() << 8) | (uint16_t) state.reg_a;
    default:
      fprintf(stderr, "get_register_pair(): Invalid regpair: %d. Exiting.\n", regpair);
      exit(-1);
  }
}

void set_register_pair(int regpair, uint16_t data) {
  switch (regpair) {
    case BC:
      state.reg_b = (data & 0xFF00) >> 8;
      state.reg_c = data & 0xFF;
      return;
    case DE:
      state.reg_d = (data & 0xFF00) >> 8;
      state.reg_e = data & 0xFF;
      return;
    case HL:
      state.reg_h = (data & 0xFF00) >> 8;
      state.reg_l = data & 0xFF;
      return;
    case SP:
      state.sp = data;
      return;
    case FA:
      restore_flags((data & 0xFF00) >> 8);
      state.reg_a = data & 0xFF;
      return;
  }
}


int get_cond(int cond, int op, int condflg) {
  switch(cond) {
    case NZ: return (condflg && (op == 1)) ? 1 : !state.flag_z;
    case Z:  return (condflg && (op != 1)) ? 1 : state.flag_z;
    case NC: return !state.flag_cy;
    case C:  return state.flag_cy;
    case PO: return !state.flag_p;
    case PE: return state.flag_p;
    case P:  return !state.flag_s;
    case M:  return state.flag_s;
    default:
      fprintf(stderr, "get_cond(): Invalid cond: %d. Exiting.\n", cond);
      exit(-1);
  }
}

