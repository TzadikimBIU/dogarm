#include "dogarm.h"
#include "disasm.h"
#include "instr.h"
#include <stdlib.h>
#include <string.h>

dogarm_disasm_t *dogarm_disasm(const void *buffer, const size_t nbytes) {
    if (!buffer || nbytes == 0 || nbytes % 4 != 0) {
        return NULL;
    }
    
    const uint8_t *buf = (const uint8_t *)buffer;
    size_t ninstr = nbytes / 4;
    if (ninstr > ((size_t)UINT32_MAX / 4u) + 1u) {
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
        uint32_t addr = (uint32_t)(i * 4u);
        instruction_t instr;
        
        disasm->instructions[i].address = addr;
        disasm->instructions[i].instruction = disasm_read_u32_le(buf + i * 4);
        
        disasm_instruction(buf + i * 4, addr, &instr,
                          disasm->instructions[i].disasm_instr,
                          sizeof(disasm->instructions[i].disasm_instr));
    }
    
    return disasm;
}

void dogarm_free(dogarm_disasm_t *disassembly) {
    if (disassembly) {
        free(disassembly->instructions);
        free(disassembly);
    }
}
