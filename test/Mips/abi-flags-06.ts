# Check generation of .MIPS.abiflags section under the following conditions:
# - There are multiple input object files
# - Not all input files have a .MIPS.abiflags section
# The .MIPS.abiflags section in the output file should reproduce result
# of merging input object file sections and data come from ELF header flags.

# RUN: yaml2obj -format=elf -docnum 1 %s > %t-abi.o
# RUN: yaml2obj -format=elf -docnum 2 %s > %t-elf.o
# RUN: %MCLinker -mtriple=mipsel-unknown-linux -shared \
# RUN:           -o %t.so %t-abi.o %t-elf.o
# RUN: llvm-readobj -mips-abi-flags %t.so | FileCheck %s

# CHECK:      MIPS ABI Flags {
# CHECK-NEXT:   Version: 0
# CHECK-NEXT:   ISA: MIPS32r2
# CHECK-NEXT:   ISA Extension: None (0x0)
# CHECK-NEXT:   ASEs [ (0x810)
# CHECK-NEXT:     MDMX (0x10)
# CHECK-NEXT:     microMIPS (0x800)
# CHECK-NEXT:   ]
# CHECK-NEXT:   FP ABI: Hard float (double precision) (0x1)
# CHECK-NEXT:   GPR size: 32
# CHECK-NEXT:   CPR1 size: 64
# CHECK-NEXT:   CPR2 size: 0
# CHECK-NEXT:   Flags 1 [ (0x0)
# CHECK-NEXT:   ]
# CHECK-NEXT:   Flags 2: 0x0
# CHECK-NEXT: }

# abi.o
---
FileHeader:
  Class:   ELFCLASS32
  Data:    ELFDATA2LSB
  Type:    ET_REL
  Machine: EM_MIPS
  Flags:   [EF_MIPS_CPIC, EF_MIPS_ABI_O32,
            EF_MIPS_ARCH_32, EF_MIPS_ARCH_ASE_MDMX]

Sections:
- Name: .MIPS.abiflags
  Type: SHT_MIPS_ABIFLAGS
  AddressAlign: 8
  ISA:          MIPS32
  ISARevision:  1
  ISAExtension: EXT_NONE
  ASEs:         [ MDMX ]
  FpABI:        FP_DOUBLE
  GPRSize:      REG_32
  CPR1Size:     REG_64
  CPR2Size:     REG_NONE
  Flags1:       [ ]
  Flags2:       0x0

# elf.o
---
FileHeader:
  Class:   ELFCLASS32
  Data:    ELFDATA2LSB
  Type:    ET_REL
  Machine: EM_MIPS
  Flags:   [EF_MIPS_CPIC, EF_MIPS_ABI_O32,
            EF_MIPS_ARCH_32R2, EF_MIPS_MICROMIPS]

Sections:
- Name:         .text
  Type:         SHT_PROGBITS
  Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
  Size:         4
  AddressAlign: 16

Symbols:
  Global:
    - Name:    T0
      Section: .text
      Type:    STT_FUNC
      Value:   0
      Size:    4
...
