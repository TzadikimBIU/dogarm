#include "disasm.h"
#include "decode.h"

uint32_t disasm_read_u32_le(const uint8_t *buf) {
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
}

uint32_t disasm_read_u32_be(const uint8_t *buf) {
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
}

uint32_t disasm_read_u32(const uint8_t *buf, dogarm_endian_t endian) {
    if (endian == DOGARM_ENDIAN_BIG) {
        return disasm_read_u32_be(buf);
    }
    return disasm_read_u32_le(buf);
}

int disasm_instruction(const uint8_t *buf, uint32_t address, const dogarm_options_t *options,
                       instruction_t *instr, char *output, size_t outsize) {
    if (!buf || !options || !instr || !output) {
        return -1;
    }
    
    instr->raw = disasm_read_u32(buf, options->endian);
    instr->address = address;
    instr->cond = (instr->raw >> 28) & 0xf;
    instr->type = decode_instruction_type(instr->raw);
    instr->arch = options->arch;
    instr->branch_format = options->branch_format;
    instr->syntax = options->syntax;
    
    return decode_instruction(instr, output, outsize);
}
