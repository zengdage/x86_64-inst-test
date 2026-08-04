# AGENTS.md

This file applies to the entire repository. It describes the project conventions an AI coding agent should follow when inspecting, extending, or validating the instruction tests.

## Project purpose

This repository is an x86-64 instruction conformance suite, primarily intended to validate binary translators such as Box64. Tests use inline assembly to execute exact instructions and check architectural results: destination values, flags, masks, exception state, saved processor state, and fault-suppression behavior.

The suite currently contains 242 standalone C test programs:

| Directory | Tests | Scope |
| --- | ---: | --- |
| `integer/` | 59 | General-purpose integer instructions and flags |
| `float/` | 31 | x87, SSE scalar and packed floating point |
| `simd/` | 67 | SSE through AVX2 SIMD instructions |
| `system/` | 9 | User-mode system and extended-state instructions |
| `crypto/` | 14 | AES, SHA, PCLMUL and GFNI |
| `avx512/` | 45 | EVEX/AVX-512 instructions and mask registers |
| `avx256/` | 17 | Supplemental AVX/AVX2/FMA tests |

Each `directory/test_name.c` normally has a tracked precompiled counterpart at `directory/bin/test_name`. These binaries are intentionally versioned because `make box64` executes them without rebuilding.

## Important files

- `common.h`: shared `TEST_START`, `TEST_ASSERT`, `TEST_END`, flag constants, `xmm_t`, and `ymm_t`.
- `Makefile`: authoritative compiler flags, source discovery, build rules, native runner, and Box64 runner.
- `README.md`: user-facing overview. Counts or command lists in it may lag behind the Makefile; verify against the repository before relying on them.

## Build and run modes

Runtime CPU-feature checks are disabled by default. This is deliberate: a binary translator must be forced to execute the instruction being tested even if the host CPU reports different features.

```bash
make -j4 all
make run
```

The expected full native result on a machine supporting all instructions is:

```text
Total: 242  Passed: 242  Failed: 0
```

For safer native execution on a machine that may lack optional instruction sets, enable the guards and force recompilation because Make does not track flag changes:

```bash
make -B ENABLE_RUNTIME_CPU_CHECKS=1 run
```

Run tracked binaries through a translator without rebuilding:

```bash
make box64 BOX64=/path/to/box64
```

Implemented category runners are `run-integer`, `run-float`, `run-simd`, `run-system`, `run-crypto`, and `run-avx512`. There is currently no `run-avx256` rule; run those binaries directly or use `make run`.

Do not use static linking. The repository build uses dynamically linked, stripped binaries.

## CPU-check convention

Any optional runtime feature probe must use this form so it is disabled by default:

```c
#if ENABLE_RUNTIME_CPU_CHECKS
static int check_feature(void) {
    /* CPUID implementation */
}
#else
#define check_feature() 1
#endif
```

Do not add an unconditional CPUID-based skip. A test silently skipping in the default build defeats translator validation.

## Required workflow for source changes

1. Inspect the target source, `common.h`, and the relevant Makefile category flags.
2. Preserve unrelated user changes in a dirty worktree.
3. Edit source with focused changes; do not rewrite unrelated tests.
4. Strictly compile each changed test to `/tmp` before touching tracked binaries. Use `-O0 -Wall -Werror` plus the appropriate ISA flags, for example:

   ```bash
   gcc -O0 -Wall -Werror -msse4.2 simd/test_pcmpistri.c -o /tmp/test_pcmpistri
   gcc -O0 -Wall -Werror -mavx2 -mfma simd/test_vfmadd.c -o /tmp/test_vfmadd -lm
   gcc -O0 -Wall -Werror -mavx512f -mavx512bw -mavx512dq -mavx512vl \
       avx512/test_example.c -o /tmp/test_example
   ```

5. Run every temporary binary and resolve all failures.
6. Run `make -j4 all` to rebuild tracked binaries with repository flags. Keep the changed binaries corresponding to changed sources.
7. Run `make run`; the required final result is 242/242.
8. Run `git diff --check` and inspect `git status --short` and `git diff --stat`.

Do not commit unless the user explicitly asks for a commit.

## Test design requirements

A test that merely executes an instruction is insufficient. Assertions should distinguish a correct implementation from plausible incorrect implementations.

### Integer instructions

- Test operand widths independently where encodings exist: 8, 16, 32, and 64 bits.
- Include `0`, `1`, `-1`, signed minimum/maximum, unsigned maximum, carry/borrow boundaries, and wraparound.
- For shifts and rotates, include counts `0`, `width-1`, `width`, `width+1`, maximum immediate, and register-count high-bit behavior.
- Check every architecturally defined status flag that the instruction changes: CF, PF, AF, ZF, SF, and OF.
- Cover register, memory, immediate, and valid `LOCK` forms when applicable.

### Floating-point instructions

- Include QNaN, SNaN where meaningful, positive and negative infinity, `+0`, `-0`, overflow, minimum normal, subnormal, and underflow-to-zero cases.
- Check result sign for zero and infinity.
- For conversions, test ties-to-even, truncation, out-of-range integer-indefinite results, and MXCSR/x87 exception flags.
- Save and restore MXCSR or the x87 control word around tests that modify them.
- FMA tests must include a value that distinguishes fused single rounding from a separately rounded multiply followed by add/subtract. Hexadecimal floating constants are preferred for exact expectations.

### SIMD, AVX, and AVX-512

- Assert every output lane unless the test is intentionally focused on a single scalar lane.
- Verify 128-bit lane-local behavior for instructions such as pack, align, and shuffle.
- For indexes, test first/last lane, high-bit truncation, wraparound, repeated indexes, and source-boundary crossings.
- AVX-512 mask tests should normally include `k=0`, `k=all`, bit 0 only, highest valid bit only, and a mixed mask.
- Test both merge masking and `{z}` zero masking when both are available.
- Test that masked-off exceptional arithmetic or invalid memory addresses do not raise exceptions or faults.
- AVX2 gather tests should verify the post-instruction mask update as well as gathered data.
- Scalar VEX instructions must verify which source supplies preserved upper lanes.

### String comparison instructions

For PCMPISTRI/PCMPISTRM and PCMPESTRI/PCMPESTRM, cover:

- empty strings or explicit length zero;
- lane 0 and final-lane boundaries;
- full-width strings;
- explicit lengths below, at, and above the element count, including negative lengths;
- positive, negative, masked-positive, and masked-negative polarity;
- least/most significant index selection;
- bit-mask and unit-mask output;
- CF, ZF, SF, and OF after the same instruction invocation.

### Crypto instructions

- Use exact known-answer/golden outputs for AES, SHA, PCLMUL, and GFNI operations.
- Assertions such as “output changed,” “two executions match,” or `TEST_ASSERT(1, ...)` are only smoke checks and must not be the sole oracle.
- Test every meaningful immediate selector and ensure ignored/reserved immediate bits behave correctly.

### System and memory-state instructions

- Verify architectural state, not only that the instruction did not fault.
- XSAVE/XRSTOR coverage should validate relevant XSTATE bits and round-trip x87, XMM/YMM, opmask/ZMM, and PKRU when enabled.
- When changing PKRU, MXCSR, x87 state, FS/GS state, or other process state, restore it before returning.
- Fault-suppression tests may use an anonymous `PROT_NONE` guard page. Ensure all active lanes stay within valid memory and always `munmap` allocated pages.

## Inline assembly guidelines

- The source syntax is GCC AT&T syntax unless a file clearly establishes otherwise. Remember that source operands precede the destination.
- Instruction immediates must be compile-time constants. Use macro stringification or explicit cases instead of attempting a runtime immediate.
- List all modified registers and `cc`/`memory` clobbers accurately.
- Keep instruction execution and flag capture in the same `asm volatile` block; intervening C code may change flags.
- For masked instructions, verify operand order carefully. Masked-off merge lanes may retain the old destination, the first source, or zero depending on the instruction semantics.
- Use aligned storage when required by an aligned move or instruction. `xmm_t` is 16-byte aligned and `ymm_t` is 32-byte aligned; AVX-512 files commonly define a local 64-byte-aligned `zmm_t`.
- Avoid undefined C arithmetic when computing expectations. Use unsigned arithmetic, widened reference calculations, or explicit constants for overflow cases.
- Prefer exact bit comparisons for NaNs, signed zero, masks, and integer-indefinite values when payload/sign details matter.

## Repository hygiene

- Source and corresponding tracked binaries should be handed off together after successful verification.
- Temporary probes and diagnostic binaries belong in `/tmp`, not in the repository.
- Do not use destructive Git operations to discard user work.
- Before reporting completion, state which tests were run and include the aggregate pass/fail result.
