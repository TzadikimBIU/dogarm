#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/dogarm.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <binary_file>\n", argv[0]);
        fprintf(stderr, "Disassemble ARM 32-bit instructions from a binary file\n");
        return EXIT_FAILURE;
    }

    struct stat st;
    if (stat(argv[1], &st) != 0) {
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
    
    FILE *fp = fopen(argv[1], "rb");
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

    dogarm_disasm_t *disassembly = dogarm_disasm(buffer, file_size);
    if (!disassembly) {
        fprintf(stderr, "Error: disassembly failed\n");
        free(buffer);
        return EXIT_FAILURE;
    }

    printf("dogarm - ARM 32-bit Disassembler\n");
    printf("File: %s (%zu bytes, %zu instructions)\n", argv[1], file_size, disassembly->ninstr);
    printf("================================\n\n");
    printf("Address  Instruction  Disassembly\n");
    printf("--------  -----------  ----------------------------------------\n");

    for (size_t i = 0; i < disassembly->ninstr; ++i) {
        printf("0x%04x    %08x    %s\n",
               disassembly->instructions[i].address,
               disassembly->instructions[i].instruction,
               disassembly->instructions[i].disasm_instr);
    }

    printf("\nTotal instructions: %zu\n", disassembly->ninstr);

    dogarm_free(disassembly);
    free(buffer);
    return EXIT_SUCCESS;
}
