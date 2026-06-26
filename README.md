<div align="center">
  <img src="images/image.png" alt="dogarm" />
</div>

By James.

# dogarm

dogarm is an ARM ISA disassembly engine that does real time decoding of 32 bit ARM opcodes into human readable assembly mnemonics. The library implements instruction format parsing, operand extraction and encoding translation for the ARM architectures unified instruction set and supports data processing, memory transfer, branch and control flow operations w/ full conditional execution semantics

The disassembler uses a decoder pipeline implementing instruction classification via bitfield pattern matching that's followed by operand parsing using ARMs second operand (shifter operand) semantics. The decoder handles immediate value rotation, register shifts as well as addressing mode interpretation for load/store operations

The disassembler implements instruction classification through hierarchical bitfield pattern matching to enable dispatch to specialised decoding routines. Operand extraction handles ARM's addressing modes.

The decoder pipeline uses instruction type identification by examining bits [27:26] and [25:20] then dispatches to format handlers that extract operands and generate mnemonic strings following standard ARM assembly syntax conventions

## API

### `dogarm_disasm_t *dogarm_disasm(const void *buffer, const size_t nbytes)`

Decodes a contiguous stream of ARM 32 bit little endian opcodes and transforms raw instruction words into canonical assembly mnemonics w/ full operand deconstruction. Internally this performs strict instruction boundary checks and validates stream alignment as well as walking the opcode sequence with bitfield pattern matching. For each opcode the decoder pipeline issues micro parsers for operand extraction, field decoding and encoding class translation then emitting a symbolic disassembly inclusive of shifter operands, addressing modes and conditional execution annotations

**Parameters:**

`buffer`  
&nbsp;&nbsp;&nbsp;&nbsp;Pointer to the instruction buffer containing raw ARM opcodes (must be word aligned)

`nbytes`  
&nbsp;&nbsp;&nbsp;&nbsp;Byte count of the instruction stream (must be a multiple of 4 bytes i.e complete 32 bit words)

**Returns:**

Pointer to a `dogarm_disasm_t` struture containing decoded instruction metadata and formatted disassembly strings. Returns `NULL` if allocation fails or input parameters are invalid

### `void dogarm_free(dogarm_disasm_t *disassembly)`

This deallocates memory associated w/ a disassembly result structure releasing all internal buffers and metadata allocated during the disassembly process

`disassembly`  
Pointer to the disassembly structure returned by `dogarm_disasm()`. It's safe to pass `NULL`

### Building

The build system generates both static and shared library artefacts:

```bash
make
make test
```

For the same strict build used by CI:

```bash
make ci
```

The legacy script remains available:

```bash
./build.sh
```

**Usage:**
```bash
./build/dogarm-disasm <binary_file>
```

The utility reads a contiguous byte stream from the specified file that validates instruction boundary alignment (requiring word-aligned 32-bit opcodes) and performs full instruction decoding with operand extraction. Output is formatted as tabular disassembly listing showing virtual addresses, raw opcode encodings and canonical ARM assembly mnemonics

**Raw Binary Files (ready to disassemble):**
```bash
./build/dogarm-disasm penguin.bin
./build/dogarm-disasm firmware-image.bin
```

**Extracting Code from ELF Executables:**
If you have a compiled ARM ELF binary, extract the executable code section first:
```bash
objcopy -O binary --only-section=.text your_program.elf code-section.bin
./build/dogarm-disasm code-section.bin
```
Note: `objcopy` is only needed when extracting from ELF files. Raw binary files like `penguin.bin` can be disassembled directly.

**Memory Dumps:**
```bash
./build/dogarm-disasm memdump.bin
```

**Hex to Binary Conversion:**
```bash
echo "e3a00000 e1a05008" | xxd -r -p > instructions.bin
./build/dogarm-disasm instructions.bin
```

**Limitations:**
- Instruction addresses and branch targets are relative to file start, not actual load addresses. For position dependent analysis you can compute actual addresses from ELF base addresses or runtime addresses
- Undefined/unsupported instruction encodings are emitted as `.word` directives w/ raw hex values
