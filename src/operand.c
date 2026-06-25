#include "operand.h"
#include <stdio.h>
#include <string.h>

static const char *reg_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "sl", "fp", "ip", "sp", "lr", "pc"
};

static const char *shift_names[] = {"lsl", "lsr", "asr", "ror"};

static const char *get_reg_name(uint32_t reg) {
    return reg_names[reg & 0xf];
}

static uint32_t rotate_immediate(uint32_t imm, uint32_t rot) {
    if (rot == 0) return imm;
    uint32_t rotation = (rot & 0xf) * 2;
    return (imm >> rotation) | (imm << (32 - rotation));
}

int operand_format(const operand_t *op, char *buf, size_t bufsize) {
    switch (op->type) {
        case OPERAND_REGISTER:
            return snprintf(buf, bufsize, "%s", get_reg_name(op->data.reg.reg));
            
        case OPERAND_IMMEDIATE:
            return snprintf(buf, bufsize, "#%u", op->data.imm.value);
            
        case OPERAND_REGISTER_SHIFTED: {
            const char *reg = get_reg_name(op->data.reg_shifted.reg);
            if (op->data.reg_shifted.amount == 0 && op->data.reg_shifted.shift == SHIFT_LSL) {
                return snprintf(buf, bufsize, "%s", reg);
            }
            if (op->data.reg_shifted.amount == 0 && op->data.reg_shifted.shift == SHIFT_ROR) {
                return snprintf(buf, bufsize, "%s, rrx", reg);
            }
            const char *shift = shift_names[op->data.reg_shifted.shift];
            uint32_t amount = op->data.reg_shifted.amount;
            if (amount == 0 && (op->data.reg_shifted.shift == SHIFT_LSR ||
                                op->data.reg_shifted.shift == SHIFT_ASR)) {
                amount = 32;
            }
            return snprintf(buf, bufsize, "%s, %s #%u", reg, shift, amount);
        }
        
        case OPERAND_REGISTER_REGISTER_SHIFTED: {
            const char *reg = get_reg_name(op->data.reg_reg_shifted.reg);
            const char *shift = shift_names[op->data.reg_reg_shifted.shift];
            const char *shift_reg = get_reg_name(op->data.reg_reg_shifted.shift_reg);
            return snprintf(buf, bufsize, "%s, %s %s", reg, shift, shift_reg);
        }
        
        default:
            return snprintf(buf, bufsize, "<?>");
    }
}

operand_t operand_parse_imm(uint32_t instr) {
    operand_t op;
    uint32_t imm = instr & 0x000000ff;
    uint32_t rot = (instr >> 8) & 0x0f;
    
    op.type = OPERAND_IMMEDIATE;
    op.data.imm.value = rotate_immediate(imm, rot);
    
    return op;
}

operand_t operand_parse_reg(uint32_t instr) {
    operand_t op;
    uint32_t rm = instr & 0x0000000f;
    uint32_t shift_type = (instr >> 5) & 3;
    uint32_t shift_imm = (instr >> 7) & 0x1f;
    uint32_t shift_reg = (instr >> 8) & 0x0f;
    uint32_t i_bit = (instr >> 4) & 1;
    
    if (i_bit == 0) {
        if (shift_imm == 0 && shift_type == SHIFT_LSL) {
            op.type = OPERAND_REGISTER;
            op.data.reg.reg = rm;
        } else {
            op.type = OPERAND_REGISTER_SHIFTED;
            op.data.reg_shifted.reg = rm;
            op.data.reg_shifted.shift = (shift_type_t)shift_type;
            op.data.reg_shifted.amount = shift_imm;
        }
    } else {
        op.type = OPERAND_REGISTER_REGISTER_SHIFTED;
        op.data.reg_reg_shifted.reg = rm;
        op.data.reg_reg_shifted.shift = (shift_type_t)shift_type;
        op.data.reg_reg_shifted.shift_reg = shift_reg;
    }
    
    return op;
}
