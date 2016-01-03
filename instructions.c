#include <stdio.h>
#include "emulator.h"

#define ADD 0
#define ADC 1
#define SUB 2
#define SBB 3
#define ANA 4
#define XRA 5
#define ORA 6
#define CMP 7

#define RET 0
#define JMP 1
#define CALL 2

#define RLC 0
#define RRC 1
#define RAL 2
#define RAR 3

#define SHLD 0
#define LHLD 1
#define STA  2
#define LDA  3

void mov(uint8_t opcode) {
  int src = opcode & 0x7; /* The source register is defined by the last three bits */
  int dest = (opcode >> 3) & 0x7; // The destination register is defined by the 4th, 5th, and 6th bits
  set_register_content(dest, get_register_content(src));
}

void arithmetic_logic(int op, uint8_t data) {
  uint16_t result;

  state.flag_cy = state.flag_ac = 0; /* Reset the carry flags */

  /* Calculate the appropriate result based on the opcode */
  switch (op) {
    case ADD:
      result = (uint16_t) get_register_content(REG_A) + (uint16_t) data;
      break;
    case ADC:
      result = (uint16_t) get_register_content(REG_A) + (uint16_t) data + (uint16_t) state.flag_cy;
      break;
    case SUB:
    case CMP:
      result = (uint16_t) get_register_content(REG_A) - (uint16_t) data;
      break;
    case SBB:
      result = (uint16_t) get_register_content(REG_A) - (uint16_t) data - (uint16_t) state.flag_cy;
      break;
    case ANA:
      result = (uint16_t) get_register_content(REG_A) & (uint16_t) data;
      break;
    case XRA:
      result = (uint16_t) get_register_content(REG_A) ^ (uint16_t) data;
      break;
    case ORA:
      result = (uint16_t) get_register_content(REG_A) | (uint16_t) data;
      break;
  }

  /* Set the flags */
  if ((op == SUB) || (op == SBB) || (op == CMP)) {
    state.flag_cy = (result < 0xFF);
  } else {
    state.flag_cy = (result > 0xFF);
  }

  state.flag_z = !result;
  state.flag_s = ((result & 0x80) == 0x80); /* Mask the get the sign bit -- 0x80 == 10000000 */
  state.flag_p = check_parity((uint8_t) result);

  if (op != CMP) {
    set_register_content(REG_A, result & 0xFF);
  }
}

void increment(int regnum) {
  uint16_t result = (uint16_t) get_register_content(regnum) + 1;

  state.flag_cy = (result > 0xFF);
  state.flag_z = !result;
  state.flag_s = ((result & 0x80) == 0x80); /* Mask the get the sign bit -- 0x80 == 10000000 */
  state.flag_p = check_parity((uint8_t) result);

  set_register_content(regnum, result & 0xFF);
}

void dad(int regpair) {
  uint32_t result = (uint32_t) get_register_pair(regpair) + (uint32_t) get_register_pair(HL);
  state.flag_cy = (result > 0xFFFF);
  set_register_pair(HL, result & 0xFFFF);
}

void decrement(int regnum) {
  uint16_t result = (uint16_t) get_register_content(regnum) - 1;

  state.flag_cy = (result < 0xFF);
  state.flag_z = !result;
  state.flag_s = ((result & 0x80) == 0x80);
  state.flag_p = check_parity((uint8_t) result);

  set_register_content(regnum, result & 0xFF);
}

void jump(int cond, int op, uint16_t addr, int condflg) {
  if (op != RET)
    state.pc += 2;

  if (!get_cond(cond, op, condflg))
    return;

  switch(op) {
    case RET:
      state.pc = pop_stack();
      return;
    case JMP:
      state.pc = addr;
      break;
    case CALL:
      push_stack(state.pc);
      state.pc = addr;
      break;
  }
}

void direct_address(int op, uint16_t addr) {
  switch (op) {
    case SHLD:
      memory[addr] = state.reg_l;
      memory[addr + 1] = state.reg_h;
      break;
    case LHLD:
      state.reg_l = memory[addr];
      state.reg_h = memory[addr+1];
      break;
    case STA:
      memory[addr] = state.reg_a;
      break;
    case LDA:
      state.reg_a = memory[addr];
      break;
  }
}

void reset(int loc) {
  push_stack(state.pc);

  switch (loc) {
    case 0: state.pc = 0x0000; return;
    case 1: state.pc = 0x0008; return;
    case 2: state.pc = 0x0010; return;
    case 3: state.pc = 0x0018; return;
    case 4: state.pc = 0x0020; return;
    case 5: state.pc = 0x0028; return;
    case 6: state.pc = 0x0030; return;
    case 7: state.pc = 0x0038; return;
  }
}

void push(int regpair) {
  uint16_t data;
  if (regpair == 3) { /* Small workaround -- this normally signifies the SP register except for push and pop */
    data = get_register_pair(FA);
  } else {
    data = get_register_pair(regpair);
  }

  push_stack(data);
}

void pop(int regpair) {
  uint16_t data = pop_stack();

  if (regpair == 3) { /* Small workaround -- this normally signifies the SP register except for push and pop */
    set_register_pair(FA, data);
  } else {
    set_register_pair(regpair, data);
  }
}

void rotate(int op) {
  int highbit = (state.reg_a & (1 << 7)) >> 7;
  int lowbit = state.reg_a & 1;

  switch (op) {
    case RLC:
      state.reg_a = (state.reg_a << 1) | highbit;
      state.flag_cy = highbit;
      break;
    case RRC:
      state.reg_a = (state.reg_a >> 1) | (lowbit << 7);
      state.flag_cy = lowbit;
      break;
    case RAL:
      state.reg_a = (state.reg_a << 1) | state.flag_cy;
      state.flag_cy = highbit;
      break;
    case RAR:
      state.reg_a = (state.reg_a >> 1) | (state.flag_cy << 7);
      state.flag_cy = lowbit;
      break;
  }
}

void xchg() {
  uint16_t tmp = get_register_pair(HL);
  set_register_pair(HL, get_register_pair(DE));
  set_register_pair(DE, tmp);
}

void xthl() {
  uint16_t tmp = pop_stack();
  push_stack(get_register_pair(HL));
  set_register_pair(HL, tmp);
}

