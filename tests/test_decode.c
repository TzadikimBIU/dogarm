#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/dogarm.h"

typedef struct {
    uint32_t word;
    const char *expected;
} decode_case_t;

static void write_word_le(uint8_t *buf, uint32_t word) {
    buf[0] = (uint8_t)(word & 0xffu);
    buf[1] = (uint8_t)((word >> 8) & 0xffu);
    buf[2] = (uint8_t)((word >> 16) & 0xffu);
    buf[3] = (uint8_t)((word >> 24) & 0xffu);
}

static int check_sequence(const decode_case_t *cases, size_t count) {
    uint8_t bytes[sizeof(uint32_t) * 16];
    if (count > 16) {
        fprintf(stderr, "test harness capacity exceeded\n");
        return 1;
    }

    for (size_t i = 0; i < count; i++) {
        write_word_le(bytes + (i * sizeof(uint32_t)), cases[i].word);
    }

    dogarm_disasm_t *disasm = dogarm_disasm(bytes, count * sizeof(uint32_t));
    if (!disasm) {
        fprintf(stderr, "dogarm_disasm returned NULL\n");
        return 1;
    }

    int failures = 0;
    for (size_t i = 0; i < count; i++) {
        const char *actual = disasm->instructions[i].disasm_instr;
        if (strcmp(actual, cases[i].expected) != 0) {
            fprintf(stderr, "case %zu failed for 0x%08x\n  expected: %s\n  actual:   %s\n",
                    i, cases[i].word, cases[i].expected, actual);
            failures++;
        }
    }

    dogarm_free(disasm);
    return failures;
}

int main(void) {
    const decode_case_t cases[] = {
        {0xe0900000u, "adds r0, r0, r0"},
        {0xe89de004u, "ldmia sp, {r2, sp, lr, pc}"},
        {0xea000008u, "b 0x00000030"},
        {0xebfffffcu, "bl 0x00000004"},
        {0xef000000u, "svc #0"},
        {0x12900001u, "addsne r0, r0, #1"},
        {0xe8bdffffu, "ldmia sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr, pc}"},
        {0xe1001000u, ".word 0xe1001000"},
    };

    int failures = check_sequence(cases, sizeof(cases) / sizeof(cases[0]));
    if (failures != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
