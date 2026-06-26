#ifndef DOGARM_H
#define DOGARM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DOGARM_INSTR_TYPE_UNDEFINED,
    DOGARM_INSTR_TYPE_DATA_PROC,
    DOGARM_INSTR_TYPE_MULTIPLY,
    DOGARM_INSTR_TYPE_MULTIPLY_LONG,
    DOGARM_INSTR_TYPE_SINGLE_DATA_TRANSFER,
    DOGARM_INSTR_TYPE_HALFWORD_DATA_TRANSFER,
    DOGARM_INSTR_TYPE_BLOCK_DATA_TRANSFER,
    DOGARM_INSTR_TYPE_BRANCH,
    DOGARM_INSTR_TYPE_SOFTWARE_INTERRUPT,
    DOGARM_INSTR_TYPE_COPROCESSOR,
    DOGARM_INSTR_TYPE_SWAP,
    DOGARM_INSTR_TYPE_PSR_TRANSFER,
    DOGARM_INSTR_TYPE_BRANCH_EXCHANGE,
    DOGARM_INSTR_TYPE_WIDE_IMMEDIATE
} dogarm_instruction_type_t;

typedef enum {
    DOGARM_ENDIAN_LITTLE,
    DOGARM_ENDIAN_BIG
} dogarm_endian_t;

typedef enum {
    DOGARM_ARCH_ARMV4T = 4,
    DOGARM_ARCH_ARMV5 = 5,
    DOGARM_ARCH_ARMV6 = 6,
    DOGARM_ARCH_ARMV7 = 7
} dogarm_arch_t;

typedef enum {
    DOGARM_BRANCH_TARGET,
    DOGARM_BRANCH_OFFSET
} dogarm_branch_format_t;

typedef enum {
    DOGARM_SYNTAX_UNIFIED
} dogarm_syntax_t;

typedef struct {
    uint32_t base_address;
    dogarm_endian_t endian;
    dogarm_arch_t arch;
    dogarm_branch_format_t branch_format;
    dogarm_syntax_t syntax;
} dogarm_options_t;

typedef struct {
    uint32_t address;
    uint32_t instruction;
    dogarm_instruction_type_t type;
    uint32_t cond;
    char disasm_instr[128];
} dogarm_instruction_t;

typedef struct {
    size_t ninstr;
    dogarm_instruction_t *instructions;
} dogarm_disasm_t;

dogarm_options_t dogarm_default_options(void);

dogarm_disasm_t *dogarm_disasm(const void *buffer, const size_t nbytes);

dogarm_disasm_t *dogarm_disasm_with_options(const void *buffer, const size_t nbytes,
                                            const dogarm_options_t *options);

void dogarm_free(dogarm_disasm_t *disassembly);

#ifdef __cplusplus
}
#endif

#endif
