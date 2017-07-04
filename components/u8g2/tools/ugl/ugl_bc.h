/*

  ugl_bc.h
  
  bytecode interpreter
  
*/


#ifndef _UGL_BC_H
#define _UGL_BC_H

#include <stdint.h>

#define BC_STACK_SIZE 64

struct bc_struct
{
  uint8_t *code;
  uint8_t stack_pointer;		/* starts at 0 and grows */
  uint16_t stack[BC_STACK_SIZE];
};
typedef struct bc_struct bc_t;

typedef void (*bc_buildin_fn)(bc_t *bc);
extern bc_buildin_fn bc_buildin_list[];

/* lower 4 bit: 0, upper 4 bit + next byte --> 12 bit value */
#define BC_CMD_LOAD_12BIT (0x00)

/* lower 4 bit: 1, upper 4 bit + next byte --> 12 bit value */
#define BC_CMD_CALL_BUILDIN (0x01)

/* lower 4 bit: 2, upper 4 bit + next byte --> 12 bit value */
#define BC_CMD_CALL_BUILDIN_POP_STACK (0x02)

/* lower 4 bit: 2, upper 4 bit + next byte --> 12 bit relative adr */
#define BC_CMD_BRANCH (0x03)

/* lower 4 bit: 15, upper 4 bit: 0  --> put 0 on stack */
#define BC_CMD_LOAD_0 (0x0f)
/* lower 4 bit: 15, upper 4 bit: 1  --> put 1 on stack */
#define BC_CMD_LOAD_1 (0x1f)
/* lower 4 bit: 15, upper 4 bit: 2  --> but 16 bit value on stack, order: high byte, low byte */
#define BC_CMD_LOAD_16BIT (0x2f)
/* lower 4 bit: 15, upper 4 bit: 3  */
#define BC_CMD_RETURN_FROM_PROCEDURE (0x3f)
/* lower 4 bit: 15, upper 4 bit: 4  --> adr are next 16 bit*/
#define BC_CMD_JUMP_NOT_ZERO (0x4f)
/* lower 4 bit: 15, upper 4 bit: 5  --> adr are next 16 bit*/
#define BC_CMD_JUMP_ZERO (0x05f)

void bc_exec(bc_t *bc, uint8_t *code);


#endif
