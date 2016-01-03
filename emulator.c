#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "emulator.h"

struct state state;
uint8_t memory[MEMSIZE];

enum {
  TYPE_UNKNOWN,
  TYPE_MOV,
  TYPE_ALR,
  TYPE_ALI,
  TYPE_JUMP,
  TYPE_PUSH,
  TYPE_POP,
  TYPE_RST,
  TYPE_ROT,
  TYPE_LXI,
  TYPE_MVI,
  TYPE_LDAX,
  TYPE_STAX,
  TYPE_INX,
  TYPE_DCX,
  TYPE_INR,
  TYPE_DCR,
  TYPE_DAD,
  TYPE_DIR_ADDR
};

uint8_t get_memory_byte() { // Returns byte pointed to by the H and L registers
  uint16_t addr = ((uint16_t) state.reg_h << 8) | ((uint16_t) state.reg_l);

  if (addr < 0 || addr >= MEMSIZE) {
    fprintf(stderr, "ERROR: Invalid memory address: %u\n", addr);
    exit(-1);
  }

  return memory[addr];
}

void set_memory_byte(uint8_t byte) { // Sets byte pointed to by the H and L registers.
  uint16_t addr = ((uint16_t) state.reg_h << 8) | ((uint16_t) state.reg_l);

  if (addr < 0 || addr >= MEMSIZE) {
    fprintf(stderr, "ERROR: Invalid memory address: %u\n", addr);
    exit(-1);
  }

  memory[addr] = byte;
}

void push_stack(uint16_t data) {
  memory[state.sp - 1] = (data & 0xFF00) >> 8; /* High byte */
  memory[state.sp - 2] = (data & 0xFF); /* Low byte */
  state.sp -= 2;
}

uint16_t pop_stack() {
  uint16_t data = (uint16_t) memory[state.sp + 1] << 8 | (uint16_t) memory[state.sp];
  state.sp += 2;
  return data;
}

/* jmp  - 11xxx01x
 * call - 11xxx10x
 * push - 11xx0101
*/

int get_instr_type(uint8_t opcode) {
  if ((opcode >= 0x40) && (opcode < 0x80))
    return TYPE_MOV;

  if ((opcode >= 0x80) && (opcode < 0xC0))
    return TYPE_ALR;

  if ((opcode & 0xC7) == 0xC6)
    return TYPE_ALI;

  if ((opcode & 0xCF) == 0xC1)
    return TYPE_POP;

  if ((opcode & 0xCF) == 0xC5)
    return TYPE_PUSH;

  if ((opcode & 0xC7) == 0xC7) /* This is the logic for figuring out the instruction format
                                  1. 11xxx111 == rst instruction format
                                  2. the xxx portion  is variable and refers to a register, so we mask it out with 0xC7(11000111)
                                  3. compare against 0xC7 (11000111) */
    return TYPE_RST;

  if ((opcode & 0xE7) == 0x7)
    return TYPE_ROT;

  if ((opcode & 0xCF) == 0x1)
    return TYPE_LXI;

  if ((opcode & 0xC7) == 0x6)
    return TYPE_MVI;

  if ((opcode & 0xEF) == 0x2)
    return TYPE_STAX;

  if ((opcode & 0xEF) == 0xa)
    return TYPE_LDAX;

  if ((opcode & 0xCF) == 0x3)
    return TYPE_INX;

  if ((opcode & 0xCF) == 0xb)
    return TYPE_DCX;

  if ((opcode & 0xC7) == 0x4)
    return TYPE_INR;

  if ((opcode & 0xC7) == 0x5)
    return TYPE_DCR;

  if ((opcode & 0xCF) == 0x9)
    return TYPE_DAD;

  if ((opcode & 0xE7) == 0x22)
    return TYPE_DIR_ADDR;

  if (((opcode >> 6) == 0x3) && (((opcode >> 1) & 0x3) < 3))
    return TYPE_JUMP;

  return TYPE_UNKNOWN;
}

void print_machine_state() {
  printf("FLAGS: Z=%d\tS=%d\tP=%d\tAC=%d\tCY=%d\n",
      state.flag_z, state.flag_s, state.flag_p, state.flag_ac, state.flag_cy);

  printf("REGIS: B=0x%02x\tC=0x%02x\tD=0x%02x\tE=0x%02x\tH=0x%02x\tL=0x%02x\tA=0x%02x\n",
      state.reg_b, state.reg_c, state.reg_d, state.reg_e, state.reg_h, state.reg_l, state.reg_a);

  printf("       SP=0x%04x\tPC=0x%04x\n", state.sp, state.pc);

  printf("STACK(0x%04x): [ %02x | %02x | %02x | %02x | %02x | %02x ... ]\n",
      state.sp, memory[state.sp], memory[state.sp + 1], memory[state.sp + 2],
      memory[state.sp + 3], memory[state.sp + 4], memory[state.sp + 5]);

  putchar('\n');
}

void execute(uint8_t opcode) {
  int instr_type;

  switch (opcode) {
    case 0x00: // NOP
      break;

    case 0x76: // HLT
      printf("HLT: Exiting program\n");
      exit(0);

    case 0xFB: // EI
      state.interrupts_enabled = 1;
      break;

    case 0xF3: // DI
      state.interrupts_enabled = 0;
      break;

    case 0xEB: // XCHNG
      xchg();
      break;

    case 0xE3: // XTHL
      xthl();
      break;

    case 0xF9: // SPHL
      set_register_pair(SP, get_register_pair(HL));
      break;

    case 0xE9: // PCHL
      state.pc = ((uint16_t) state.reg_h << 8) | ((uint16_t) state.reg_l);
      break;

    case 0x37: // STC
      state.flag_cy = 1;
      break;

    case 0x3F: // CMC
      state.flag_cy = !state.flag_cy;
      break;

    case 0x2F: // CMA
      state.reg_a = ~state.reg_a;
      break;

    case 0xDB: // IN
      state.reg_a = state.input_pins[memory[state.pc]];
      state.pc+= 1;
      break;

    case 0xD3: // OUT
      device_out(memory[state.pc], state.reg_a);
      state.pc+= 1;
      break;

    default:
      instr_type = get_instr_type(opcode);

      switch (instr_type) {
        case TYPE_LDAX:
          state.reg_a = memory[get_register_pair((opcode >> 4) & 0x1)];
          break;

        case TYPE_STAX:
          memory[get_register_pair((opcode >> 4) & 0x1)] = state.reg_a;
          break;

        case TYPE_INX:
          set_register_pair((opcode >> 4) & 0x3, get_register_pair((opcode >> 4) & 0x3) + 1);
          break;

        case TYPE_DCX:
          set_register_pair((opcode >> 4) & 0x3, get_register_pair((opcode >> 4) & 0x3) - 1);
          break;

        case TYPE_INR:
          increment((opcode >> 3) & 0x7);
          break;

        case TYPE_DCR:
          decrement((opcode >> 3) & 0x7);
          break;

        case TYPE_ALR:
          arithmetic_logic((opcode >> 3) & 0x7, get_register_content(opcode & 0x7));
          break;

        case TYPE_ALI:
          arithmetic_logic((opcode >> 3) & 0x7, memory[state.pc]);
          state.pc++;
          break;

        case TYPE_DAD:
          dad((opcode >> 4) & 0x3);
          break;

        case TYPE_MOV:
          set_register_content((opcode >> 3) & 0x7, /* dst */
                               get_register_content(opcode & 0x7) /* src */
          );
          break;

        case TYPE_MVI:
          set_register_content((opcode >> 3) & 0x7, /* Register identifier */
                               memory[state.pc]); /* immediate data */
          state.pc++;
          break;

        case TYPE_LXI:
          set_register_pair((opcode >> 4) & 0x3, /* Register pair identifier */
                            ((uint16_t) memory[state.pc + 1] << 8) | ((uint16_t) memory[state.pc]) /* immediate data */
          );
          state.pc += 2;
          break;

        case TYPE_JUMP:
          jump((opcode >> 3) & 0x7, /* condition to check against */
               (opcode >> 1) & 0x3, /* jmp, call, or ret */
               ((uint16_t) memory[state.pc + 1] << 8) | ((uint16_t) memory[state.pc]), /* address to jump to */
               opcode & 0x1 /* Special flag for deciding jmp vs. jnz, call vs. cz, ret vs. rz */
          );
          break;

        case TYPE_DIR_ADDR:
          direct_address((opcode >> 3) & 0x3, /* which op? */
                       ((uint16_t) memory[state.pc + 1] << 8) | ((uint16_t) memory[state.pc]) /* direct address*/
          );
          state.pc += 2;
          break;

        case TYPE_RST:
          reset((opcode >> 3) & 0x7);
          break;

        case TYPE_POP:
          pop((opcode >> 4) & 0x3);
          break;

        case TYPE_PUSH:
          push((opcode >> 4) & 0x3);
          break;

        case TYPE_ROT:
          rotate((opcode >> 3) & 0x3);
          break;

        case TYPE_UNKNOWN:
          die_error("Unsupported opcode: %x\n", opcode);
          exit(-1);
      }
   }
}

void interrupt(uint8_t opcode) {
  if (state.interrupts_enabled) {
    state.interrupts_enabled = 0;
    execute(opcode);
  }
}

void device_out(int dev, uint8_t byte) {
  switch(dev) {
    case 2:
    case 4:
      shift_hardware(dev, byte);
      break;
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s PATH\n", argv[0]);
    return 0;
  }

  initialize_sdl();
  state.interrupts_enabled = 1;

  FILE *fp = fopen(argv[1], "r");
  fread(memory, 8192, 1, fp); /* Load ROM */
  fclose(fp);

  uint8_t opcode;
  int count = 0;

  while (1) {
    //disassemble8080(memory, state.pc);

    opcode = memory[state.pc];
    state.pc += 1;
    count += 1;

    execute(opcode);
    display();
    input();

    //print_machine_state();
    // usleep(10000);
  }

  return 0;
}
