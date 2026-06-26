#ifndef DOGARM_H
#define DOGARM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t address;
    uint32_t instruction;
    char disasm_instr[128];
} dogarm_instruction_t;

typedef struct {
    size_t ninstr;
    dogarm_instruction_t *instructions;
} dogarm_disasm_t;

dogarm_disasm_t *dogarm_disasm(const void *buffer, const size_t nbytes);

void dogarm_free(dogarm_disasm_t *disassembly);

#ifdef __cplusplus
}
#endif

#endif
