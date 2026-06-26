#include "decode.h"
#include "operand.h"
#include <inttypes.h>
#include <stdio.h>

#define ARM_COND_MASK     0xf0000000
#define ARM_COND_SHIFT    28
#define ARM_OPCODE_MASK   0x01e00000
#define ARM_OPCODE_SHIFT  21
#define ARM_S_MASK        0x00100000
#define ARM_RN_MASK       0x000f0000
#define ARM_RN_SHIFT      16
#define ARM_RD_MASK       0x0000f000
#define ARM_RD_SHIFT      12
#define ARM_RM_MASK       0x0000000f
#define ARM_I_MASK        0x02000000
#define ARM_I_SHIFT       25
#define ARM_MULTIPLY_MASK       0x0fc000f0
#define ARM_MULTIPLY_BITS       0x00000090
#define ARM_MULTIPLY_LONG_MASK  0x0f8000f0
#define ARM_MULTIPLY_LONG_BITS  0x00800090
#define ARM_SWAP_MASK           0x0fb00ff0
#define ARM_SWAP_BITS           0x01000090

static const char *cond_codes[] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "",   "nv"
};

static const char *reg_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "sl", "fp", "ip", "sp", "lr", "pc"
};

static const char *dp_opcodes[] = {
    "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
    "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"
};

static const char *get_cond_suffix(uint32_t cond) {
    return cond_codes[cond & 0xf];
}

static const char *get_reg_name(uint32_t reg) {
    return reg_names[reg & 0xf];
}

static int32_t sign_extend_24(uint32_t val) {
    return ((int32_t)(val << 8)) >> 8;
}

instr_type_t decode_instruction_type(uint32_t instr) {
    uint32_t bits_27_26 = (instr >> 26) & 3;
    uint32_t bit_25 = (instr >> 25) & 1;
    uint32_t bits_27_24 = (instr >> 24) & 0xf;
    uint32_t bits_25_20 = (instr >> 20) & 0x3f;
    uint32_t opcode = (instr & ARM_OPCODE_MASK) >> ARM_OPCODE_SHIFT;
    uint32_t s = (instr & ARM_S_MASK) >> 20;
    
    if (bits_27_26 == 0) {
        if ((instr & ARM_MULTIPLY_LONG_MASK) == ARM_MULTIPLY_LONG_BITS) {
            return INSTR_TYPE_MULTIPLY_LONG;
        }
        if ((instr & ARM_MULTIPLY_MASK) == ARM_MULTIPLY_BITS) {
            return INSTR_TYPE_MULTIPLY;
        }
        if ((instr & ARM_SWAP_MASK) == ARM_SWAP_BITS) {
            return INSTR_TYPE_SWAP;
        }
        if ((bits_25_20 & 0x30) == 0x10 && (instr & 0x0ff00ff0) == 0x01200000) {
            return INSTR_TYPE_PSR_TRANSFER;
        }
        if (opcode >= 8 && opcode <= 11 && !s) {
            return INSTR_TYPE_UNDEFINED;
        }
        return INSTR_TYPE_DATA_PROC;
    } else if (bits_27_26 == 1) {
        return INSTR_TYPE_SINGLE_DATA_TRANSFER;
    } else if (bits_27_26 == 2) {
        if (bit_25) {
            return INSTR_TYPE_BRANCH;
        }
        return INSTR_TYPE_BLOCK_DATA_TRANSFER;
    } else if (bits_27_26 == 3) {
        if (bits_27_24 == 0xf) {
            return INSTR_TYPE_SOFTWARE_INTERRUPT;
        }
        return INSTR_TYPE_COPROCESSOR;
    }
    
    return INSTR_TYPE_UNDEFINED;
}

static int disasm_data_proc(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t opcode = (raw & ARM_OPCODE_MASK) >> ARM_OPCODE_SHIFT;
    uint32_t s = (raw & ARM_S_MASK) >> 20;
    uint32_t rn = (raw & ARM_RN_MASK) >> ARM_RN_SHIFT;
    uint32_t rd = (raw & ARM_RD_MASK) >> ARM_RD_SHIFT;
    uint32_t i = (raw & ARM_I_MASK) >> ARM_I_SHIFT;
    
    const char *opcode_str = dp_opcodes[opcode];
    const char *cond_str = get_cond_suffix(instr->cond);
    const char *flags_str = (s && !(opcode >= 8 && opcode <= 11)) ? "s" : "";
    const char *rd_str = get_reg_name(rd);
    char op2_buf[64];
    operand_t op2;
    
    if (i) {
        op2 = operand_parse_imm(raw);
    } else {
        op2 = operand_parse_reg(raw);
    }
    operand_format(&op2, op2_buf, sizeof(op2_buf));
    
    int len;
    
    if (opcode == 8 || opcode == 9 || opcode == 10 || opcode == 11) {
        const char *rn_str = get_reg_name(rn);
        len = snprintf(output, outsize, "%s%s %s, %s", opcode_str, cond_str, rn_str, op2_buf);
    } else {
        if (opcode == 13 || opcode == 15) {
            len = snprintf(output, outsize, "%s%s%s %s, %s", opcode_str, flags_str, cond_str, rd_str, op2_buf);
        } else {
            const char *rn_str = get_reg_name(rn);
            len = snprintf(output, outsize, "%s%s%s %s, %s, %s", opcode_str, flags_str, cond_str, rd_str, rn_str, op2_buf);
        }
    }

    return len;
}

static int disasm_multiply(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t a = (raw >> 21) & 1;
    uint32_t s = (raw >> 20) & 1;
    uint32_t rd = (raw & ARM_RD_MASK) >> ARM_RD_SHIFT;
    uint32_t rn = (raw & ARM_RN_MASK) >> ARM_RN_SHIFT;
    uint32_t rs = (raw >> 8) & 0xf;
    uint32_t rm = raw & 0xf;
    
    const char *cond_str = get_cond_suffix(instr->cond);
    const char *flags_str = s ? "s" : "";
    const char *rd_str = get_reg_name(rd);
    const char *rm_str = get_reg_name(rm);
    const char *rs_str = get_reg_name(rs);
    
    int len;
    
    if (a) {
        const char *rn_str = get_reg_name(rn);
        len = snprintf(output, outsize, "mla%s%s %s, %s, %s, %s", flags_str, cond_str, rd_str, rm_str, rs_str, rn_str);
    } else {
        len = snprintf(output, outsize, "mul%s%s %s, %s, %s", flags_str, cond_str, rd_str, rm_str, rs_str);
    }
    
    return len;
}

static int disasm_single_data_transfer(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t i = (raw >> 25) & 1;
    uint32_t p = (raw >> 24) & 1;
    uint32_t u = (raw >> 23) & 1;
    uint32_t b = (raw >> 22) & 1;
    uint32_t w = (raw >> 21) & 1;
    uint32_t l = (raw >> 20) & 1;
    uint32_t rn = (raw & ARM_RN_MASK) >> ARM_RN_SHIFT;
    uint32_t rd = (raw & ARM_RD_MASK) >> ARM_RD_SHIFT;
    uint32_t offset = raw & 0xfff;
    
    const char *cond_str = get_cond_suffix(instr->cond);
    const char *rd_str = get_reg_name(rd);
    const char *rn_str = get_reg_name(rn);
    const char *op_str = l ? "ldr" : "str";
    const char *size_str = b ? "b" : "";
    
    char addr_buf[128];
    
    if (p) {
        if (i) {
            operand_t op = operand_parse_reg(raw);
            char offset_buf[64];
            operand_format(&op, offset_buf, sizeof(offset_buf));
            if (u) {
                snprintf(addr_buf, sizeof(addr_buf), "[%s, %s]%s", rn_str, offset_buf, w ? "!" : "");
            } else {
                snprintf(addr_buf, sizeof(addr_buf), "[%s, -%s]%s", rn_str, offset_buf, w ? "!" : "");
            }
        } else {
            if (offset == 0) {
                snprintf(addr_buf, sizeof(addr_buf), "[%s]%s", rn_str, w ? "!" : "");
            } else {
                snprintf(addr_buf, sizeof(addr_buf), "[%s, #%s%u]%s", rn_str, u ? "" : "-", offset, w ? "!" : "");
            }
        }
    } else {
        if (i) {
            operand_t op = operand_parse_reg(raw);
            char offset_buf[64];
            operand_format(&op, offset_buf, sizeof(offset_buf));
            if (u) {
                snprintf(addr_buf, sizeof(addr_buf), "[%s], %s", rn_str, offset_buf);
            } else {
                snprintf(addr_buf, sizeof(addr_buf), "[%s], -%s", rn_str, offset_buf);
            }
        } else {
            if (offset == 0) {
                snprintf(addr_buf, sizeof(addr_buf), "[%s]", rn_str);
            } else {
                snprintf(addr_buf, sizeof(addr_buf), "[%s], #%s%u", rn_str, u ? "" : "-", offset);
            }
        }
    }
    
    return snprintf(output, outsize, "%s%s%s %s, %s", op_str, size_str, cond_str, rd_str, addr_buf);
}

static int disasm_branch(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t l = (raw >> 24) & 1;
    int32_t offset = sign_extend_24(raw & 0x00ffffff) * 4;
    uint32_t target = instr->address + 8u + (uint32_t)offset;
    
    const char *cond_str = get_cond_suffix(instr->cond);
    const char *op_str = l ? "bl" : "b";
    
    return snprintf(output, outsize, "%s%s 0x%08" PRIx32, op_str, cond_str, target);
}

static int disasm_software_interrupt(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t comment = raw & 0x00ffffff;
    const char *cond_str = get_cond_suffix(instr->cond);
    
    return snprintf(output, outsize, "svc%s #%u", cond_str, comment);
}

static int disasm_block_data_transfer(const instruction_t *instr, char *output, size_t outsize) {
    uint32_t raw = instr->raw;
    uint32_t p = (raw >> 24) & 1;
    uint32_t u = (raw >> 23) & 1;
    uint32_t w = (raw >> 21) & 1;
    uint32_t l = (raw >> 20) & 1;
    uint32_t rn = (raw & ARM_RN_MASK) >> ARM_RN_SHIFT;
    uint32_t reg_list = raw & 0xffff;
    
    const char *cond_str = get_cond_suffix(instr->cond);
    const char *rn_str = get_reg_name(rn);
    const char *op_str = l ? "ldm" : "stm";
    const char *mode_str;
    
    if (p && u) mode_str = "ib";
    else if (p && !u) mode_str = "db";
    else if (!p && u) mode_str = "ia";
    else mode_str = "da";
    
    char reg_list_buf[128] = "";
    size_t used = 0;
    int first = 1;
    
    for (uint32_t i = 0; i < 16; i++) {
        if (reg_list & (1u << i)) {
            size_t remaining = sizeof(reg_list_buf) - used;
            int written = snprintf(reg_list_buf + used, remaining, "%s%s",
                                   first ? "" : ", ", get_reg_name(i));
            if (written < 0) {
                return written;
            }
            if ((size_t)written >= remaining) {
                used = sizeof(reg_list_buf) - 1u;
                break;
            }
            used += (size_t)written;
            first = 0;
        }
    }
    
    return snprintf(output, outsize, "%s%s%s %s%s, {%s}", op_str, mode_str, cond_str, rn_str, w ? "!" : "", reg_list_buf);
}

int decode_instruction(const instruction_t *instr, char *output, size_t outsize) {
    switch (instr->type) {
        case INSTR_TYPE_DATA_PROC:
            return disasm_data_proc(instr, output, outsize);
            
        case INSTR_TYPE_MULTIPLY:
            return disasm_multiply(instr, output, outsize);
            
        case INSTR_TYPE_SINGLE_DATA_TRANSFER:
            return disasm_single_data_transfer(instr, output, outsize);
            
        case INSTR_TYPE_BRANCH:
            return disasm_branch(instr, output, outsize);
            
        case INSTR_TYPE_SOFTWARE_INTERRUPT:
            return disasm_software_interrupt(instr, output, outsize);
            
        case INSTR_TYPE_BLOCK_DATA_TRANSFER:
            return disasm_block_data_transfer(instr, output, outsize);
            
        case INSTR_TYPE_MULTIPLY_LONG:
        case INSTR_TYPE_SWAP:
        case INSTR_TYPE_PSR_TRANSFER:
        case INSTR_TYPE_HALFWORD_DATA_TRANSFER:
        case INSTR_TYPE_COPROCESSOR:
        case INSTR_TYPE_UNDEFINED:
        default:
            return snprintf(output, outsize, ".word 0x%08x", instr->raw);
    }
}
