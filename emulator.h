#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdint.h>

#define MEMSIZE 65536

#define REG_B 0
#define REG_C 1
#define REG_D 2
#define REG_E 3
#define REG_H 4
#define REG_L 5
#define MEM_REF 6
#define REG_A 7

#define BC 0
#define DE 1
#define HL 2
#define SP 3
#define FA 4

/* Our global machine state */
struct state {
  uint8_t reg_b;
  uint8_t reg_c;
  uint8_t reg_d;
  uint8_t reg_e;
  uint8_t reg_h;
  uint8_t reg_l;
  uint8_t reg_a;

  uint16_t sp; /* stack pointer */
  uint16_t pc; /* program counter */

  uint8_t flag_z; /* zero flag */
  uint8_t flag_s; /* sign flag */
  uint8_t flag_p; /* parity flag */
  uint8_t flag_cy; /* carry flag */
  uint8_t flag_ac; /* auxillary carry flag */

  uint8_t interrupts_enabled;

  uint8_t input_pins[256];
};

extern struct state state;
extern uint8_t memory[MEMSIZE];

void interrupt(uint8_t opcode);

/* register */
uint8_t get_register_content(int reg_num);
void set_register_content(int reg_num, uint8_t byte);
uint16_t get_register_pair(int regpair);
void set_register_pair(int regpair, uint16_t data);
int get_cond(int cond, int op, int condflg);

/* memory */
uint8_t get_memory_byte();
void set_memory_byte(uint8_t byte);
void push_stack(uint16_t data);
uint16_t pop_stack();

/* utility */
uint8_t get_flagbyte();
void restore_flags(uint8_t flagbyte);
int check_parity(uint8_t byte);

/* disassemble */
int disassemble8080(uint8_t *codebuffer, int pc);

/* hardware */
void device_out(int dev, uint8_t byte);
void input();
void shift_hardware(int dev, uint8_t byte);
void initialize_sdl();
void quit_sdl();
void display();

/* error */
void die_error(char *format, ...);

/* instructions */
void arithmetic_logic(int op, uint8_t data);
void increment(int regnum);
void decrement(int regnum);
void jump(int cond, int op, uint16_t addr, int condflg);
void direct_address(int op, uint16_t addr);
void rotate(int op);
void dad(int regpair);
void reset(int loc);
void xchg();
void xthl();
void sta(uint16_t addr);
void lda(uint16_t addr);
void shld(uint16_t addr);
void lhld(uint16_t addr);
void sphl();
void pop(int regpair);
void push(int regpair);

#endif
