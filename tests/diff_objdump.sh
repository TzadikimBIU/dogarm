#!/bin/sh
set -eu

dogarm_bin=${1:-./build/dogarm-disasm}

if ! command -v arm-none-eabi-as >/dev/null 2>&1 ||
   ! command -v arm-none-eabi-objcopy >/dev/null 2>&1 ||
   ! command -v arm-none-eabi-objdump >/dev/null 2>&1; then
    echo "Skipping objdump differential test: arm-none-eabi binutils not found"
    exit 0
fi

tmp_base=${TMPDIR:-/tmp}/dogarm-diff-$$
asm_file=$tmp_base.s
obj_file=$tmp_base.o
bin_file=$tmp_base.bin
dogarm_out=$tmp_base.dogarm
objdump_out=$tmp_base.objdump

cleanup() {
    rm -f "$asm_file" "$obj_file" "$bin_file" "$dogarm_out" "$objdump_out"
}
trap cleanup EXIT

cat > "$asm_file" <<'ASM'
.syntax unified
.arch armv7-a
.arm
mrs r1, cpsr
msr cpsr_c, r0
movw r0, #0x1234
movt r0, #0x5678
ldrh r0, [r1, #2]
strh r2, [r3], #4
ldrsb r4, [r5, #-1]
ldrsh r6, [r7, r8]
ldrd r0, [r1, #8]
strd r2, [r3], -r4
bx lr
blx r3
swp r0, r1, [r2]
umull r0, r1, r2, r3
ASM

arm-none-eabi-as -o "$obj_file" "$asm_file" >/dev/null 2>&1
arm-none-eabi-objcopy -O binary --only-section=.text "$obj_file" "$bin_file"
arm-none-eabi-objdump -d "$obj_file" > "$objdump_out"
"$dogarm_bin" --no-header --format plain "$bin_file" > "$dogarm_out"

for mnemonic in mrs msr movw movt ldrh strh ldrsb ldrsh ldrd strd bx blx swp umull; do
    if ! grep -q "[[:space:]]$mnemonic" "$objdump_out"; then
        echo "objdump reference missing mnemonic: $mnemonic" >&2
        exit 1
    fi
    if ! grep -q ": $mnemonic" "$dogarm_out"; then
        echo "dogarm output missing mnemonic: $mnemonic" >&2
        exit 1
    fi
done
