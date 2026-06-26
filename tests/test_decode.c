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

static void write_word_be(uint8_t *buf, uint32_t word) {
    buf[0] = (uint8_t)((word >> 24) & 0xffu);
    buf[1] = (uint8_t)((word >> 16) & 0xffu);
    buf[2] = (uint8_t)((word >> 8) & 0xffu);
    buf[3] = (uint8_t)(word & 0xffu);
}

static int check_sequence_with_options(const decode_case_t *cases, size_t count,
                                       const dogarm_options_t *options) {
    uint8_t bytes[sizeof(uint32_t) * 64] = {0};
    if (count > 64) {
        fprintf(stderr, "test harness capacity exceeded\n");
        return 1;
    }

    for (size_t i = 0; i < count; i++) {
        if (options && options->endian == DOGARM_ENDIAN_BIG) {
            write_word_be(bytes + (i * sizeof(uint32_t)), cases[i].word);
        } else {
            write_word_le(bytes + (i * sizeof(uint32_t)), cases[i].word);
        }
    }

    dogarm_disasm_t *disasm = dogarm_disasm_with_options(bytes, count * sizeof(uint32_t), options);
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

static int check_sequence(const decode_case_t *cases, size_t count) {
    return check_sequence_with_options(cases, count, NULL);
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
        {0xe10f1000u, "mrs r1, cpsr"},
        {0xe14f2000u, "mrs r2, spsr"},
        {0xe121f000u, "msr cpsr_c, r0"},
        {0xe16ff003u, "msr spsr_fsxc, r3"},
        {0xe328f102u, "msr cpsr_f, #2147483648"},
        {0xe3010234u, "movw r0, #4660"},
        {0xe3450678u, "movt r0, #22136"},
        {0xe1d100b2u, "ldrh r0, [r1, #2]"},
        {0xe0c320b4u, "strh r2, [r3], #4"},
        {0xe15540d1u, "ldrsb r4, [r5, #-1]"},
        {0xe19760f8u, "ldrsh r6, [r7, r8]"},
        {0xe1c100d8u, "ldrd r0, [r1, #8]"},
        {0xe00320f4u, "strd r2, [r3], -r4"},
        {0xe12fff1eu, "bx lr"},
        {0xe12fff33u, "blx r3"},
        {0xe1020091u, "swp r0, r1, [r2]"},
        {0xe1453094u, "swpb r3, r4, [r5]"},
        {0xe0810392u, "umull r0, r1, r2, r3"},
        {0xe0a54796u, "umlal r4, r5, r6, r7"},
        {0xe0c98b9au, "smull r8, r9, sl, fp"},
        {0xe0edc09eu, "smlal ip, sp, lr, r0"},
        {0xe1a00021u, "mov r0, r1, lsr #32"},
        {0xe1a02043u, "mov r2, r3, asr #32"},
        {0xe1a04065u, "mov r4, r5, rrx"},
    };
    const decode_case_t offset_branch[] = {
        {0xea000008u, "b #32"},
    };
    const decode_case_t based_branch[] = {
        {0xea000008u, "b 0x00001028"},
    };
    const decode_case_t big_endian[] = {
        {0xe3a00001u, "mov r0, #1"},
    };
    const decode_case_t armv5_gate[] = {
        {0xe3010234u, ".word 0xe3010234"},
    };

    int failures = check_sequence(cases, sizeof(cases) / sizeof(cases[0]));
    dogarm_options_t options = dogarm_default_options();

    options.branch_format = DOGARM_BRANCH_OFFSET;
    failures += check_sequence_with_options(offset_branch,
                                            sizeof(offset_branch) / sizeof(offset_branch[0]),
                                            &options);

    options = dogarm_default_options();
    options.base_address = 0x1000u;
    failures += check_sequence_with_options(based_branch,
                                            sizeof(based_branch) / sizeof(based_branch[0]),
                                            &options);

    options = dogarm_default_options();
    options.endian = DOGARM_ENDIAN_BIG;
    failures += check_sequence_with_options(big_endian,
                                            sizeof(big_endian) / sizeof(big_endian[0]),
                                            &options);

    options = dogarm_default_options();
    options.arch = DOGARM_ARCH_ARMV5;
    failures += check_sequence_with_options(armv5_gate,
                                            sizeof(armv5_gate) / sizeof(armv5_gate[0]),
                                            &options);

    if (failures != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
