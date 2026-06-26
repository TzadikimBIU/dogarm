#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "../src/dogarm.h"

typedef enum {
    OUTPUT_TABLE,
    OUTPUT_PLAIN
} output_format_t;

static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s [options] <binary_file>\n", program);
    fprintf(stderr, "Disassemble ARM 32-bit instructions from a binary file\n");
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  --base <addr>             Base address for displayed addresses and branch targets\n");
    fprintf(stderr, "  --endian little|big       Input byte order\n");
    fprintf(stderr, "  --arch armv4t|armv5|armv6|armv7\n");
    fprintf(stderr, "  --branch-format target|offset\n");
    fprintf(stderr, "  --format table|plain\n");
    fprintf(stderr, "  --no-header               Suppress table header and summary\n");
    fprintf(stderr, "  --strict                  Reject files whose size is not a multiple of 4\n");
}

static int parse_u32(const char *text, uint32_t *value) {
    char *end = NULL;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 0);
    if (errno != 0 || !end || *end != '\0' || parsed > UINT32_MAX) {
        return -1;
    }

    *value = (uint32_t)parsed;
    return 0;
}

static const char *option_value(int *index, int argc, char *argv[], const char *arg,
                                const char *prefix) {
    size_t prefix_len = strlen(prefix);
    if (strncmp(arg, prefix, prefix_len) == 0 && arg[prefix_len] == '=') {
        return arg + prefix_len + 1u;
    }
    if (strcmp(arg, prefix) == 0) {
        if (*index + 1 >= argc) {
            return NULL;
        }
        (*index)++;
        return argv[*index];
    }
    return NULL;
}

static int parse_endian(const char *text, dogarm_endian_t *endian) {
    if (strcmp(text, "little") == 0) {
        *endian = DOGARM_ENDIAN_LITTLE;
        return 0;
    }
    if (strcmp(text, "big") == 0) {
        *endian = DOGARM_ENDIAN_BIG;
        return 0;
    }
    return -1;
}

static int parse_arch(const char *text, dogarm_arch_t *arch) {
    if (strcmp(text, "armv4t") == 0) {
        *arch = DOGARM_ARCH_ARMV4T;
        return 0;
    }
    if (strcmp(text, "armv5") == 0) {
        *arch = DOGARM_ARCH_ARMV5;
        return 0;
    }
    if (strcmp(text, "armv6") == 0) {
        *arch = DOGARM_ARCH_ARMV6;
        return 0;
    }
    if (strcmp(text, "armv7") == 0) {
        *arch = DOGARM_ARCH_ARMV7;
        return 0;
    }
    return -1;
}

static int parse_branch_format(const char *text, dogarm_branch_format_t *format) {
    if (strcmp(text, "target") == 0) {
        *format = DOGARM_BRANCH_TARGET;
        return 0;
    }
    if (strcmp(text, "offset") == 0) {
        *format = DOGARM_BRANCH_OFFSET;
        return 0;
    }
    return -1;
}

static int parse_output_format(const char *text, output_format_t *format) {
    if (strcmp(text, "table") == 0) {
        *format = OUTPUT_TABLE;
        return 0;
    }
    if (strcmp(text, "plain") == 0) {
        *format = OUTPUT_PLAIN;
        return 0;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    dogarm_options_t options = dogarm_default_options();
    output_format_t output_format = OUTPUT_TABLE;
    int strict = 0;
    int show_header = 1;
    const char *input_path = NULL;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        const char *value;

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }

        value = option_value(&i, argc, argv, arg, "--base");
        if (value) {
            if (parse_u32(value, &options.base_address) != 0) {
                fprintf(stderr, "Error: invalid --base value: %s\n", value);
                return EXIT_FAILURE;
            }
            continue;
        }

        value = option_value(&i, argc, argv, arg, "--endian");
        if (value) {
            if (parse_endian(value, &options.endian) != 0) {
                fprintf(stderr, "Error: invalid --endian value: %s\n", value);
                return EXIT_FAILURE;
            }
            continue;
        }

        value = option_value(&i, argc, argv, arg, "--arch");
        if (value) {
            if (parse_arch(value, &options.arch) != 0) {
                fprintf(stderr, "Error: invalid --arch value: %s\n", value);
                return EXIT_FAILURE;
            }
            continue;
        }

        value = option_value(&i, argc, argv, arg, "--branch-format");
        if (value) {
            if (parse_branch_format(value, &options.branch_format) != 0) {
                fprintf(stderr, "Error: invalid --branch-format value: %s\n", value);
                return EXIT_FAILURE;
            }
            continue;
        }

        value = option_value(&i, argc, argv, arg, "--format");
        if (value) {
            if (parse_output_format(value, &output_format) != 0) {
                fprintf(stderr, "Error: invalid --format value: %s\n", value);
                return EXIT_FAILURE;
            }
            continue;
        }

        if (strcmp(arg, "--no-header") == 0) {
            show_header = 0;
            continue;
        }

        if (strcmp(arg, "--strict") == 0) {
            strict = 1;
            continue;
        }

        if (arg[0] == '-') {
            fprintf(stderr, "Error: unknown option: %s\n", arg);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        if (input_path) {
            fprintf(stderr, "Error: multiple input files provided\n");
            return EXIT_FAILURE;
        }
        input_path = arg;
    }

    if (!input_path) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    struct stat st;
    if (stat(input_path, &st) != 0) {
        perror("stat");
        return EXIT_FAILURE;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: input is not a regular file\n");
        return EXIT_FAILURE;
    }
    if (st.st_size < 0 || (uintmax_t)st.st_size > (uintmax_t)SIZE_MAX) {
        fprintf(stderr, "Error: file is too large\n");
        return EXIT_FAILURE;
    }

    size_t file_size = (size_t)st.st_size;
    
    FILE *fp = fopen(input_path, "rb");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    if (file_size == 0) {
        fprintf(stderr, "Error: file is empty\n");
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (file_size % 4 != 0) {
        size_t truncated_size = file_size - (file_size % 4u);
        if (strict) {
            fprintf(stderr, "Error: file size (%zu bytes) is not a multiple of 4\n", file_size);
            fclose(fp);
            return EXIT_FAILURE;
        }
        fprintf(stderr, "Warning: file size (%zu bytes) is not a multiple of 4, truncating to %zu bytes\n",
                file_size, truncated_size);
        file_size = truncated_size;
        if (file_size == 0) {
            fprintf(stderr, "Error: no complete 32-bit instructions to disassemble\n");
            fclose(fp);
            return EXIT_FAILURE;
        }
    }

    uint8_t *buffer = malloc(file_size);
    if (!buffer) {
        fprintf(stderr, "Error: memory allocation failed\n");
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (fread(buffer, 1, file_size, fp) != file_size) {
        perror("fread");
        free(buffer);
        fclose(fp);
        return EXIT_FAILURE;
    }
    fclose(fp);

    dogarm_disasm_t *disassembly = dogarm_disasm_with_options(buffer, file_size, &options);
    if (!disassembly) {
        fprintf(stderr, "Error: disassembly failed\n");
        free(buffer);
        return EXIT_FAILURE;
    }

    if (show_header && output_format == OUTPUT_TABLE) {
        printf("dogarm - ARM 32-bit Disassembler\n");
        printf("File: %s (%zu bytes, %zu instructions)\n", input_path, file_size, disassembly->ninstr);
        printf("================================\n\n");
        printf("Address     Instruction  Disassembly\n");
        printf("----------  -----------  ----------------------------------------\n");
    }

    for (size_t i = 0; i < disassembly->ninstr; ++i) {
        if (output_format == OUTPUT_PLAIN) {
            printf("0x%08x: %s\n",
                   disassembly->instructions[i].address,
                   disassembly->instructions[i].disasm_instr);
        } else {
            printf("0x%08x  %08x    %s\n",
                   disassembly->instructions[i].address,
                   disassembly->instructions[i].instruction,
                   disassembly->instructions[i].disasm_instr);
        }
    }

    if (show_header && output_format == OUTPUT_TABLE) {
        printf("\nTotal instructions: %zu\n", disassembly->ninstr);
    }

    dogarm_free(disassembly);
    free(buffer);
    return EXIT_SUCCESS;
}
