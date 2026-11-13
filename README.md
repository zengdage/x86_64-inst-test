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
├── avx512/           # 78 tests — AVX-512 (EVEX-encoded)
├── system/           # 10 tests — User-mode system instructions
└── crypto/           # 14 tests — AES/SHA/PCLMUL/GF2P8
```

**Total: 292 test programs** covering more than 1,000 x86-64 instruction forms.

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

- EVEX floating point and conversion: `VADDPS/VADDPD`, `VSUBPS/VSUBPD`, `VMULPS/VMULPD`, `VDIVPS/VDIVPD`, `VSQRTPS`, `VMAXPS/VMAXPD`, `VMINPS/VMINPD`, `VCMPPS/VCMPPD/VCMPSS/VCMPSD`, `VSCALEF*`, `VRCP14PS`, `VRSQRT14PS`, `VGETEXPPS`, `VGETMANTPS`, `VRNDSCALEPS`, `VREDUCEPS`, `VRANGEPS`, and `VFPCLASSPS`.
- EVEX FMA and conversion boundaries: `VFMADD132/213/231PS/PD`, `VFMSUB132/213/231PS`, `VFNMADD132PS`, `VFNMSUB231PS`, `VCVTPS2PD/VCVTPD2PS`, signed and unsigned `VCVT*DQ/QQ`, and truncating `VCVTT*DQ/QQ` forms. Tests include fused-rounding discriminators, NaNs, infinities, signed zero, ties-to-even, and integer precision boundaries.
- EVEX integer arithmetic and saturation: `VPADDB/VPADDW/VPADDD/VPADDQ`, `VPSUBB/VPSUBW/VPSUBD/VPSUBQ`, `VPADDSB/VPADDSW/VPADDUSB/VPADDUSW`, `VPSUBSB/VPSUBSW/VPSUBUSB/VPSUBUSW`, `VPABS*`, `VPMULLW/VPMULHUW/VPMULHW/VPMULLD/VPMULUDQ`, `VPMADDWD/VPMADDUBSW`, `VPSADBW`, and `VPSIGNB/VPSIGNW/VPSIGND`.
- EVEX shifts, rotates, and ternary logic: `VPSLLW/D/Q`, `VPSRLW/D/Q`, `VPSRAW/D/Q`, variable word/dword/qword shifts, `VPROLD/Q`, variable dword/qword rotates, and `VPTERNLOGD/Q`, including counts at and beyond the element width and fully overlapping operands.
- EVEX compare, min/max, multiply, and masks: legacy-style `VPCMPEQ*`/`VPCMPGT*`, all eight signed and unsigned `VPCMPB/W/D/Q` predicates, `VPMIN*`, `VPMAX*`, `VPMULLQ`, `VPMULDQ`, `VPMULHRSW`, `VPTESTM*`, and `VPMOV{B,D,Q,W}2M/VPMOVM2{B,D,Q,W}`.
- EVEX shuffle, permute, pack, align, and extend: `VPSHUFB/VPSHUFD/VPSHUFHW/VPSHUFLW`, `VPERMB/VPERMW/VPERMD/VPERMQ`, `VPERMI2B/D/Q/W`, `VPERMUTB/D/Q/W`, `VPALIGNR`, `VPUNPCKL*/VPUNPCKH*`, `VPACKSSWB/VPACKUSWB`, and `VPMOVSX*/VPMOVZX*`.
- EVEX data distribution and indexed memory: scalar and block `VBROADCAST*` forms, `VPBROADCASTB/W/D/Q`, mask broadcasts, 128/256-bit `VINSERTF*/VINSERTI*` forms, integer and floating-point `VPERMI2*`/`VPERMT2*`, `VPCOMPRESSD/Q`, `VPEXPANDD`, `VCOMPRESSPS`, `VEXPANDPD`, `VPGATHERDD/QQ`, and `VPSCATTERDD`. Empty-mask tests use `PROT_NONE` pages to verify fault suppression.
- EVEX narrowing and extension: truncating, signed-saturating, and unsigned-saturating `VPMOV*` forms across qword/dword/word/byte widths, plus signed/unsigned byte-to-word extension with boundary and overflow cases.
- Extension-specific arithmetic: `VPMADD52LUQ/VPMADD52HUQ` (AVX-512IFMA) and `VPMULTISHIFTQB` (AVX-512VBMI), checked against 104-bit and bit-window software references.
- Conflict, bit, and neural-network extensions: `VPCONFLICTD/Q` and `VPLZCNTD/Q` (AVX-512CD), `VPOPCNTB/W` and `VPSHUFBITQMB` (AVX-512BITALG), `VPOPCNTD/Q` (AVX-512VPOPCNTDQ), and the four `VPDPBUSD/VPDPBUSDS/VPDPWSSD/VPDPWSSDS` AVX-512VNNI dot products.
- Additional permute and set operations: byte/word `VPCOMPRESS*`/`VPEXPAND*` plus `VPSHLD*`/`VPSHRD*` (AVX-512VBMI2), and `VP2INTERSECTD/Q` with both architectural result masks (AVX-512VP2INTERSECT).
- Cross-generation numeric extensions: `VEXP2*`, `VRCP28*`, and `VRSQRT28*` (AVX-512ER); gather/scatter prefetch forms (AVX-512PF); `V4FMADD*`/`V4FNMADD*` (AVX-512_4FMAPS); and `VP4DPWSSD/VP4DPWSSDS` (AVX-512_4VNNIW).
- Low-precision arithmetic: `VCVTNEPS2BF16`, `VCVTNE2PS2BF16`, and `VDPBF16PS` (AVX-512BF16), plus packed-half `VADDPH`, `VMULPH`, `VFMADD231PH`, `VSQRTPH`, and `VCMPPH` (AVX-512FP16). FP16 reference values use an integer IEEE binary16 implementation rather than compiler intrinsics.
- EVEX blend, insert, extract, and crypto: `VBLENDMB/MD/MQ/MW`, `VBLENDMPS/VBLENDMPD`, `VPINSRB/VPINSRW/VPINSRD/VPINSRQ`, `VEXTRACTF32X4/VEXTRACTF64X2/VEXTRACTI32X4/VEXTRACTI64X2`, `VPCLMULQDQ`, `VAES*`, `VGF2P8*`, and `VSHA*`.
- Opmask instructions and masking behavior: `KMOVB/W/D/Q`, `KADD*`, `KANDW`, `KORW`, `KXORW`, `KXNORW`, `KNOTW`, `KSHIFT*`, `KTESTW`, and `KORTESTW`, plus merge and zero masking across vector arithmetic, logical, comparison, shuffle, pack, permute, gather, scatter, and memory operations.

### System and cryptographic instructions

- System/user-mode state: `CPUID`, `RDTSC`, `RDTSCP`, `RDRAND`, `RDSEED`, `RDPID`, `RDFSBASE/RDGSBASE`, `RDPKRU/WRPKRU`, `XGETBV`, `XSAVE/XRSTOR`, `STMXCSR/VSTMXCSR`, and `LFENCE/MFENCE/SFENCE`.
- AES and carry-less multiply: `AESENC/AESENCLAST`, `AESDEC/AESDECLAST`, `AESIMC`, `AESKEYGENASSIST`, `VAESENC/VAESENCLAST`, `VAESDEC/VAESDECLAST`, `PCLMULQDQ`, and `VPCLMULQDQ`.
- SHA and GFNI: `SHA1RNDS4/SHA1MSG1/SHA1MSG2/SHA1NEXTE`, `SHA256RNDS2/SHA256MSG1/SHA256MSG2`, `GF2P8MULB/GF2P8AFFINEQB/GF2P8AFFINEINVQB`, and VEX/EVEX variants.

## Requirements

- **Compiler**: GCC with support for the target instruction sets
- **32-bit tests**: GCC multilib and 32-bit libc development/runtime support
- **Architecture**: x86-64 (compilation host)
- **CPU features needed for the default native run**: SSE2/SSE3/SSSE3/SSE4.1/SSE4.2, AVX/AVX2, FMA, AES-NI, PCLMULQDQ, SHA, ADX, BMI1/BMI2, CRC32, FSGSBASE, RDPID, PKU, XSAVE/XGETBV, AVX-512F/BW/CD/DQ/VL/IFMA/VBMI/VBMI2/VNNI/BITALG, GFNI, VAES, and VPCLMULQDQ
- **Additional features covered by translator/full-extension runs**: AVX-512ER, AVX-512PF, AVX-512_4FMAPS, AVX-512_4VNNIW, AVX-512VPOPCNTDQ, AVX-512VP2INTERSECT, AVX-512BF16, and AVX-512FP16

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
make run-avx512-all  # requires every AVX-512 extension or a capable translator
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
Total: 282  Passed: 282  Failed: 0
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
