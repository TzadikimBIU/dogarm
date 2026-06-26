#include "dogarm.h"
#include "disasm.h"
#include "instr.h"
#include <stdint.h>
#include <stdlib.h>

dogarm_options_t dogarm_default_options(void) {
    dogarm_options_t options;
    options.base_address = 0;
    options.endian = DOGARM_ENDIAN_LITTLE;
    options.arch = DOGARM_ARCH_ARMV7;
    options.branch_format = DOGARM_BRANCH_TARGET;
    options.syntax = DOGARM_SYNTAX_UNIFIED;
    return options;
}

static dogarm_options_t normalize_options(const dogarm_options_t *options) {
    dogarm_options_t normalized = options ? *options : dogarm_default_options();

    if (normalized.endian != DOGARM_ENDIAN_LITTLE && normalized.endian != DOGARM_ENDIAN_BIG) {
        normalized.endian = DOGARM_ENDIAN_LITTLE;
    }
    if (normalized.arch != DOGARM_ARCH_ARMV4T &&
        normalized.arch != DOGARM_ARCH_ARMV5 &&
        normalized.arch != DOGARM_ARCH_ARMV6 &&
        normalized.arch != DOGARM_ARCH_ARMV7) {
        normalized.arch = DOGARM_ARCH_ARMV7;
    }
    if (normalized.branch_format != DOGARM_BRANCH_TARGET &&
        normalized.branch_format != DOGARM_BRANCH_OFFSET) {
        normalized.branch_format = DOGARM_BRANCH_TARGET;
    }
    if (normalized.syntax != DOGARM_SYNTAX_UNIFIED) {
        normalized.syntax = DOGARM_SYNTAX_UNIFIED;
    }

    return normalized;
}

dogarm_disasm_t *dogarm_disasm_with_options(const void *buffer, const size_t nbytes,
                                            const dogarm_options_t *options) {
    if (!buffer || nbytes == 0 || nbytes % 4 != 0) {
        return NULL;
    }
    
    dogarm_options_t normalized = normalize_options(options);
    const uint8_t *buf = (const uint8_t *)buffer;
    size_t ninstr = nbytes / 4;
    if (ninstr > ((size_t)(UINT32_MAX - normalized.base_address) / 4u) + 1u) {
        return NULL;
    }
    
    dogarm_disasm_t *disasm = malloc(sizeof(dogarm_disasm_t));
    if (!disasm) {
        return NULL;
    }
    
    disasm->ninstr = ninstr;
    disasm->instructions = calloc(ninstr, sizeof(dogarm_instruction_t));
    
    if (!disasm->instructions) {
        free(disasm);
        return NULL;
    }
    
    for (size_t i = 0; i < ninstr; i++) {
        uint32_t addr = normalized.base_address + (uint32_t)(i * 4u);
        instruction_t instr;
        
        disasm->instructions[i].address = addr;
        disasm->instructions[i].instruction = disasm_read_u32(buf + i * 4, normalized.endian);
        
        disasm_instruction(buf + i * 4, addr, &normalized, &instr,
                          disasm->instructions[i].disasm_instr,
                          sizeof(disasm->instructions[i].disasm_instr));
        disasm->instructions[i].type = instr.type;
        disasm->instructions[i].cond = instr.cond;
    }
    
    return disasm;
}

dogarm_disasm_t *dogarm_disasm(const void *buffer, const size_t nbytes) {
    dogarm_options_t options = dogarm_default_options();
    return dogarm_disasm_with_options(buffer, nbytes, &options);
}

void dogarm_free(dogarm_disasm_t *disassembly) {
    if (disassembly) {
        free(disassembly->instructions);
        free(disassembly);
    }
}
