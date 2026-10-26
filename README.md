# x86-64 Instruction Test Suite

This is a AI-generated project.

A comprehensive x86-64 instruction conformance suite for validating binary translators. Each test uses inline assembly to execute a specific instruction form and checks architectural results, flags, masks, exceptions, and saved state where applicable.

This project is designed for testing x86-64 binary translators such as [box64](https://github.com/ptitSeb/box64).

## Project Structure

```
x86_64-inst-test/
├── common.h          # Shared test macros and type definitions
├── Makefile
├── integer/          # 61 tests — General-purpose integer instructions
├── integer32/        # 32-bit-only instructions (AAM/AAD, built with -m32)
├── float/            # 32 tests — x87 FPU and SSE scalar/packed float
├── simd/             # 71 tests — SSE/SSSE3/SSE4/AVX/AVX2
├── avx256/           # 25 tests — AVX/AVX2 256-bit supplemental
├── avx512/           # 46 tests — AVX-512 (EVEX-encoded)
├── system/           # 10 tests — User-mode system instructions
└── crypto/           # 14 tests — AES/SHA/PCLMUL/GF2P8
```

**Total: 260 test programs** covering more than 1,000 x86-64 instruction forms.

## Instruction Coverage

The list below describes the instruction forms exercised by the current source tree. A mnemonic appearing in multiple sections is intentional: legacy SSE/XMM, VEX/AVX2, and EVEX/AVX-512 encodings are tested separately.

### Integer and control flow

- Arithmetic and logic: `ADD`, `ADC`, `SUB`, `SBB`, `INC`, `DEC`, `NEG`, `MUL`, `IMUL`, `DIV`, `IDIV`, `AND`, `OR`, `XOR`, `NOT`, `CMP`, `TEST`.
- Data movement and exchange: `MOV`, `MOVSX`, `MOVSXD`, `MOVZX`, `MOVBE`, `LEA`, `BSWAP`, `XCHG`, `XADD`.
- Bit and shift operations: `BT`, `BTS`, `BTR`, `BTC`, `BSF`, `BSR`, `POPCNT`, `TZCNT`, `LZCNT`, `SHL/SAL`, `SHR`, `SAR`, `SHLD`, `SHRD`, `ROL`, `ROR`, `RCL`, `RCR`, `RORX`.
- BMI/ADX/CRC: `ANDN`, `BEXTR`, `BZHI`, `BLSI`, `BLSMSK`, `BLSR`, `PDEP`, `PEXT`, `ADCX`, `ADOX`, `CRC32`.
- Branches, flags, and atomics: `CMOVcc`, `SETcc`, `Jcc`, `LOOP/LOOPE/LOOPNE`, `CALL/RET`, `LAHF/SAHF`, `CLC/STC/CMC`, `CLD/STD`, `CMPXCHG`, `CMPXCHG8B`, `CMPXCHG16B`.
- Stack, system-adjacent, and cache operations: `PUSH/POP/PUSHFQ/POPFQ`, `ENTER`, `NOP`, `PAUSE`, `UD2`, `CPUID`, `LFENCE/MFENCE/SFENCE`, `CLFLUSH/CLFLUSHOPT`, and `PREFETCH*`.
- 32-bit-only: `AAM` and `AAD` in `integer32/`.

### x87 and scalar SSE floating point

- x87 load/store and arithmetic: `FLD`, `FST`, `FSTP`, `FILD`, `FIST`, `FISTP`, `FISTTP`, `FBLD`, `FBSTP`, `FADD(P)`, `FSUB(P/R)`, `FMUL(P)`, `FDIV(P/R)`, and `FCOM*`/`FUCOM*`.
- x87 constants and math: `FLD1`, `FLDZ`, `FLDPI`, `FLDL2E`, `FLDL2T`, `FLDLG2`, `FLDLN2`, `FSQRT`, `FABS`, `FCHS`, `FRNDINT`, `FSIN`, `FCOS`, `FPTAN`, `FPATAN`, `F2XM1`, `FSCALE`, `FYL2X`, `FYL2XP1`, `FPREM`, `FPREM1`, and environment/control instructions.
- Scalar SSE: `ADDSS/ADDSD`, `SUBSS/SUBSD`, `MULSS/MULSD`, `DIVSS/DIVSD`, `SQRTSS/SQRTSD`, `RCPSS/RCPPS`, `RSQRTSS/RSQRTPS`, `MINSS/MINSD`, `MAXSS/MAXSD`, `UCOMISS/UCOMISD`, and `COMISS/COMISD`.
- Packed SSE/SSE4.1 floating point: `ADDPS/ADDPD`, `SUBPS/SUBPD`, `MULPS/MULPD`, `DIVPS/DIVPD`, `SQRTPS/SQRTPD`, `MINPS/MINPD`, `MAXPS/MAXPD`, `ROUNDPS/ROUNDPD`, `ROUNDSS/ROUNDSD`, and `CVTPD2PS`.

### SSE/SSSE3/SSE4 SIMD (128-bit XMM)

- Data movement and masks: `MOVAPS/MOVUPS`, `MOVAPD/MOVUPD`, `MOVDQA/MOVDQU`, `MOVHPS/MOVLPS`, `MOVHPD/MOVLPD`, `MOVMSKPS/MOVMSKPD`, `MASKMOVDQU`, `MOVNTDQ`, `MOVNTDQA`, `MOVNTI`, `MOVNTPD`, `MOVNTPS`, `PEXTR*`, `PINSR*`, and `INSERTPS/EXTRACTPS`.
- Integer arithmetic and saturation: `PADDB/PADDW/PADDD/PADDQ`, `PSUBB/PSUBW/PSUBD/PSUBQ`, `PADDSB/PADDSW/PADDUSB/PADDUSW`, `PSUBSB/PSUBSW/PSUBUSB/PSUBUSW`, `PMULLW/PMULHW/PMULHUW/PMULDQ/PMULUDQ/PMULLD`, `PMADDWD/PMADDUBSW`, `PMULHRSW`, `PAVGB/PAVGW`, and `PSADBW`.
- Compare, min/max, and test: `PCMPEQ*`, `PCMPGT*`, `PMIN*`, `PMAX*`, `PTEST`, and packed floating `CMPPS/CMPPD/CMPSS/CMPSD`.
- Shuffle, pack, unpack, and extend: `PSHUFB`, `PSHUFD`, `PSHUFHW`, `PSHUFLW`, `SHUFPS/SHUFPD`, `PALIGNR`, `PHADD*`, `PHSUB*`, `PUNPCKH*`, `PUNPCKL*`, `PACK*`, `PMOVSX*`, `PMOVZX*`, `PABS*`, and `PSIGN*`.
- Blend, dot product, strings, and polynomial multiply: `PBLENDW`, `PBLENDVB`, `BLENDPS/BLENDPD`, `BLENDVPS/BLENDVPD`, `DPPS/DPPD`, `MPSADBW`, `PHMINPOSUW`, `PCMPISTRI/PCMPISTRM`, `PCMPESTRI/PCMPESTRM`, and `PCLMULQDQ`.

### AVX/AVX2 and FMA (128/256-bit XMM/YMM)

- Floating-point arithmetic and comparisons: `VADDPS/VADDPD`, `VSUBPS/VSUBPD`, `VADDSUBPS/VADDSUBPD`, `VMULPS/VMULPD`, `VDIVPS/VDIVPD`, `VADDSD`, `VMAXPD`, `VSQRTPD`, `VCMPPS/VCMPPD`, `VCMPSD/VCMPSS`, `VCOMISD`, and `VTESTPS/VTESTPD`.
- FMA3: `VFMADD132/213/231PS/PD`, `VFMSUB132/213/231PS/PD`, `VFNMADD132/213/231PS/PD`, and `VFNMSUB132/213/231PS/PD`.
- Data movement and lane operations: `VMOVAPS/VMOVUPS`, `VMOVAPD/VMOVUPD`, `VMOVDQU`, `VMOVQ`, `VMOVSD`, `VEXTRACTF128/VEXTRACTI128`, `VPERM2F128/VPERM2I128`, `VBROADCASTSS/VBROADCASTSD`, `VBROADCASTF128/VBROADCASTI128`, `VZEROALL/VZEROUPPER`, and `VMASKMOVPD`.
- AVX2 integer and gather operations: `VPBROADCASTB/W/D/Q`, `VPSHUFB`, `VPMULLD`, `VPMULUDQ`, `VPMADDUBSW`, `VPMINSD`, `VPMAXSD`, `VPMAXUB`, `VPSLLD`, `VPGATHERDD`, `VPERMD`, `VPERMQ`, `VPERMPD`, `VPERMPS`, `VPERMILPS`, `VPHADD*`, `VPHSUB*`, and `VPSIGN*`.
- AVX packed helpers: `VBLENDPS/VBLENDPD`, `VDPPD`, `VPHMINPOSUW`, `VPACKSSDW`, `VPACKUSDW`, `VMOVMSKPD`, and `VMOVNTDQ`.

### AVX-512 and opmask registers (XMM/YMM/ZMM EVEX)

- EVEX floating point and conversion: `VADDPS/VADDPD`, `VSUBPS/VSUBPD`, `VMULPS/VMULPD`, `VDIVPS/VDIVPD`, `VMAXPS/VMAXPD`, `VMINPS/VMINPD`, `VSCALEFPS/VSCALEFPD/VSCALEFSS/VSCALEFSD`, and `VCVTPS2PD/VCVTPD2PS/VCVTDQ2PD/VCVTDQ2PS/VCVTPD2DQ/VCVTPS2DQ`.
- EVEX integer arithmetic and saturation: `VPADDB/VPADDW/VPADDD/VPADDQ`, `VPSUBB/VPSUBW/VPSUBD/VPSUBQ`, `VPADDSB/VPADDSW/VPADDUSB/VPADDUSW`, `VPSUBSB/VPSUBSW/VPSUBUSB/VPSUBUSW`, `VPABS*`, `VPMULLW/VPMULHUW/VPMULHW/VPMULUDQ`, `VPMADDWD`, `VPSADBW`, and `VPSIGNB/VPSIGNW/VPSIGND`.
- EVEX shifts: `VPSLLW/VPSLLD/VPSLLQ`, `VPSRLW/VPSRLD/VPSRLQ`, and `VPSRAW/VPSRAD/VPSRAQ`.
- EVEX compare, min/max, and masks: `VPCMPEQB/W/D/Q`, `VPCMPGTB/W/D/Q`, `VPMINSB/SW/SD/SQ/UB/UW/UD/UQ`, `VPMAXSB/SW/SD/SQ/UB/UW/UD/UQ`, `VPTESTMD/VPTESTMQ`, and `VPMOVMSKB/D/W`.
- EVEX shuffle, permute, pack, align, and extend: `VPSHUFB/VPSHUFD/VPSHUFHW/VPSHUFLW`, `VPERMB/VPERMW/VPERMD/VPERMQ`, `VPERMI2B/D/Q/W`, `VPERMUTB/D/Q/W`, `VPALIGNR`, `VPUNPCKL*/VPUNPCKH*`, `VPACKSSWB/VPACKUSWB`, and `VPMOVSX*/VPMOVZX*`.
- EVEX blend, insert, extract, and crypto: `VBLENDMB/MD/MQ/MW`, `VBLENDMPS/VBLENDMPD`, `VPINSRB/VPINSRW/VPINSRD/VPINSRQ`, `VEXTRACTF32X4/VEXTRACTF64X2/VEXTRACTI32X4/VEXTRACTI64X2`, `VPCLMULQDQ`, `VAES*`, `VGF2P8*`, and `VSHA*`.
- Opmask instructions and masking behavior: `KMOVW/KMOVQ`, `KANDW`, `KORW`, `KXORW`, `KXNORW`, `KNOTW`, `KSHIFTLW`, and `KSHIFTRW`, plus merge and zero masking across vector arithmetic, logical, comparison, shuffle, pack, permute, gather, and memory operations.

### System and cryptographic instructions

- System/user-mode state: `CPUID`, `RDTSC`, `RDTSCP`, `RDRAND`, `RDSEED`, `RDPID`, `RDFSBASE/RDGSBASE`, `RDPKRU/WRPKRU`, `XGETBV`, `XSAVE/XRSTOR`, `STMXCSR/VSTMXCSR`, and `LFENCE/MFENCE/SFENCE`.
- AES and carry-less multiply: `AESENC/AESENCLAST`, `AESDEC/AESDECLAST`, `AESIMC`, `AESKEYGENASSIST`, `VAESENC/VAESENCLAST`, `VAESDEC/VAESDECLAST`, `PCLMULQDQ`, and `VPCLMULQDQ`.
- SHA and GFNI: `SHA1RNDS4/SHA1MSG1/SHA1MSG2/SHA1NEXTE`, `SHA256RNDS2/SHA256MSG1/SHA256MSG2`, `GF2P8MULB/GF2P8AFFINEQB/GF2P8AFFINEINVQB`, and VEX/EVEX variants.

## Requirements

- **Compiler**: GCC with support for the target instruction sets
- **32-bit tests**: GCC multilib and 32-bit libc development/runtime support
- **Architecture**: x86-64 (compilation host)
- **CPU features needed at runtime**: SSE2/SSE3/SSSE3/SSE4.1/SSE4.2, AVX/AVX2, FMA, AES-NI, PCLMULQDQ, SHA, ADX, BMI1/BMI2, CRC32, FSGSBASE, RDPID, PKU, XSAVE/XGETBV, AVX-512F/BW/DQ/VL/IFMA/VBMI, GFNI, VAES, and VPCLMULQDQ

Runtime CPUID guards are disabled by default, so instruction tests execute
unconditionally. This is the preferred mode when testing a binary translator.
The guards can be enabled for native execution on machines with mixed feature
support.

## Build

```bash
# Build all test binaries (compiled with -O2, stripped)
make

# Build with parallel jobs
make -j$(nproc)

# Enable runtime CPUID checks and skip unsupported optional instructions.
# Use -B when switching modes so binaries are rebuilt with the new flags.
make -B ENABLE_RUNTIME_CPU_CHECKS=1

# Equivalent for a directly compiled test
gcc -DENABLE_RUNTIME_CPU_CHECKS=1 ...

# Clean all binaries
make clean
```

## Run

### Native execution

```bash
# Build and run all tests unconditionally
make run

# Native run with CPUID/SIGILL guards enabled
make -B ENABLE_RUNTIME_CPU_CHECKS=1 run

# Run a specific category
make run-integer
make run-integer32
make run-float
make run-simd
make run-system
make run-crypto
make run-avx512
make run-avx256
```

### Run via box64 (binary translator testing)

```bash
# Run all pre-compiled binaries through box64 (no recompilation)
make box64

# Specify a custom box64 path
make box64 BOX64=/path/to/box64
```

### Run a single test

```bash
# Native
./integer/bin/test_add

# Via box64
box64 ./integer/bin/test_add
```

## Output Format

Each test prints its results to stdout:

```
=== Testing ADD ===
Results: 42 passed, 0 failed
```

The `make run` and `make box64` targets aggregate results:

```
Running: integer/bin/test_add
Running: integer/bin/test_sub
...
========================================
Total: 260  Passed: 260  Failed: 0
```

## Test Design

- **Inline assembly only** — no compiler intrinsics, ensuring the exact instruction encoding is tested
- **Boundary conditions** — zero, max/min, overflow, underflow, NaN, infinity, denormals
- **Operand coverage** — reg-reg, reg-mem, reg-imm, different sizes (8/16/32/64-bit)
- **FLAGS verification** — CF, ZF, SF, OF, PF, AF checked where applicable
- **CPUID guards** — tests for optional features skip gracefully on unsupported CPUs
- **No static linking** — all binaries are dynamically linked

## License

This project is provided as-is for testing and validation purposes.
