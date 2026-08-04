# x86-64 Instruction Test Suite

This is a AI-generated project.

A comprehensive test suite for verifying x86-64 instruction behavior through binary translation. Each test case uses inline assembly to exercise specific instructions and validates results including output values and FLAGS registers.

This project is designed for testing x86-64 binary translators such as [box64](https://github.com/ptitSeb/box64).

## Project Structure

```
x86_64-inst-test/
├── common.h          # Shared test macros and type definitions
├── Makefile
├── integer/          # 59 tests — General-purpose integer instructions
├── float/            # 30 tests — x87 FPU and SSE scalar/packed float
├── simd/             # 62 tests — SSE/SSSE3/SSE4/AVX/AVX2
├── avx256/           # 16 tests — AVX/AVX2 256-bit supplemental
├── avx512/           # 45 tests — AVX-512 (EVEX-encoded)
├── system/           #  9 tests — User-mode system instructions
└── crypto/           # 14 tests — AES/SHA/PCLMUL/GF2P8
```

**Total: 235 test programs** covering ~950 x86-64 instructions.

## Instruction Coverage

| Category | Instructions | Examples |
|----------|-------------|----------|
| Integer | ADD, SUB, MUL, DIV, IMUL, IDIV, CMP, TEST, MOV, LEA, CMOV, SETcc, Jcc, BT/BTS/BTR/BTC, BSF/BSR, POPCNT, TZCNT, LZCNT, BSWAP, XCHG, CMPXCHG, CMPXCHG8B/16B, SHL/SHR/SAR/ROL/ROR, SHLD/SHRD, ADCX/ADOX, BMI1/BMI2, CRC32, etc. |
| Float | x87 FPU (FLD/FST/FADD/FSUB/FMUL/FDIV/FSIN/FCOS/FSQRT/FCOM/FCOMI, etc.), SSE scalar (ADDSS/SD, MULSS/SD, SQRTSS/SD, UCOMISS/SD, etc.), SSE packed (ADDPS/PD, MULPS/PD, MINPS/PD, MAXPS/PD, ROUNDPS/PD, etc.) |
| SIMD | Data movement (MOVDQA/U, MOVAPS/UPS, MOVNTDQ), integer arithmetic (PADDB/W/D/Q, PMULL, PMADDWD), comparison (PCMPEQ, PCMPGT, CMPPS/PD), shuffle (PSHUFB, PSHUFD, SHUFPS), pack/extend (PACK*, PMOVSX/ZX), string (PCMPISTRI/PCMPISTRM), dot product (DPPS/DPPD), etc. |
| AVX-256 | VADDPS/PD, VMULPS/PD, VCVT*, VMASKMOV, VPSHIFT, VPGATHER, VFMADD/VFMSUB/VFNMADD/VFNMSUB (FMA3), VBLEND, VBROADCAST, VPERM, etc. |
| AVX-512 | 512-bit arithmetic/logical/compare/shift/shuffle/permute/blend/pack/extend, k-register masking (merge and zeroing), VSCALEF, VEXTRACTF32X4/64X2, VPALIGNR zmm, etc. |
| System | CPUID, RDTSC/RDTSCP, RDRAND/RDSEED, RDFSBASE/WRFSBASE, XSAVE/XRSTOR, XGETBV, LFENCE/MFENCE/SFENCE, etc. |
| Crypto | AESENC/AESDEC/AESIMC/AESKEYGENASSIST, PCLMULQDQ, SHA1/SHA256, GF2P8MULB/AFFINEQB/AFFINEINVQB, VEX and EVEX variants |

## Requirements

- **Compiler**: GCC with support for the target instruction sets
- **Architecture**: x86-64 (compilation host)
- **CPU features needed at runtime**: SSE4.2, AVX2, FMA, AES-NI, SHA, AVX-512 (F/BW/DQ/VL/IFMA/VBMI), GFNI, VAES, VPCLMULQDQ, ADX, BMI1/BMI2

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
Total: 235  Passed: 235  Failed: 0
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
