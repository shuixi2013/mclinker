# Check handling of R_MIPS_PC26_S2 relocation.

# RUN: yaml2obj -format=elf %s > %t.o
# RUN: %MCLinker -mtriple=mipsel-unknown-linux -e T0 -o %t.exe %t.o
# RUN: llvm-objdump -s -t %t.exe | FileCheck %s

# CHECK: Contents of section .text:
# CHECK-NEXT: 4000a0 01000000 00000000 00000000
#                    ^ V
#                    A = -1 << 2 = -4 =>
#                    V = (T1 - 4 - T0) >> 2 =>
#                    V => 4 >> 2 = 1

# CHECK: SYMBOL TABLE:
# CHECK: 004000a0 g  F .text  00000008 T0
# CHECK: 004000a8 g  F .text  00000004 T1

FileHeader:
  Class:   ELFCLASS32
  Data:    ELFDATA2LSB
  Type:    ET_REL
  Machine: EM_MIPS
  Flags:   [EF_MIPS_CPIC, EF_MIPS_ABI_O32, EF_MIPS_ARCH_32R6]

Sections:
- Name:         .text
  Type:         SHT_PROGBITS
  Content:      "ffffff030000000000000000"
#                                ^ T1
#                ^ T0 A := 0x3ffffff == -1
  AddressAlign: 16
  Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]

- Name:         .rel.text
  Type:         SHT_REL
  Info:         .text
  AddressAlign: 4
  Relocations:
    - Offset: 0
      Symbol: T1
      Type:   R_MIPS_PC26_S2

Symbols:
  Global:
    - Name:    T0
      Section: .text
      Type:    STT_FUNC
      Value:   0
      Size:    8
    - Name:    T1
      Section: .text
      Type:    STT_FUNC
      Value:   8
      Size:    4