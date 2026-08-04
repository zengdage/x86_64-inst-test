CC      = gcc
CFLAGS  = -O2 -Wall -Wno-unused-result -s
LDFLAGS = -s

# Runtime CPUID guards are disabled by default so instruction tests execute
# unconditionally (important when validating a binary translator). Enable with
# `make -B ENABLE_RUNTIME_CPU_CHECKS=1` for safe native execution on mixed CPUs
# (`-B` is needed when switching modes because Make does not track flag changes).
ENABLE_RUNTIME_CPU_CHECKS ?= 0
ifeq ($(ENABLE_RUNTIME_CPU_CHECKS),1)
CFLAGS += -DENABLE_RUNTIME_CPU_CHECKS=1
endif

# Directories
DIRS    = integer float simd system crypto avx512 avx256

# Per-category extra flags
integer_FLAGS = -madx -mbmi -mbmi2 -mlzcnt -mpopcnt -mmovbe -mcrc32
float_FLAGS   = -lm
simd_FLAGS    = -mavx2 -mfma -mpclmul -msha -maes
system_FLAGS  =
crypto_FLAGS  = -maes -mpclmul -msha -mavx2 -mgfni
avx512_FLAGS  = -mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512ifma -mavx512vbmi -maes -mpclmul -msha -mgfni -mvaes -mvpclmulqdq
avx256_FLAGS  = -mavx2 -mfma

# Collect all sources per category
integer_SRCS := $(wildcard integer/*.c)
float_SRCS   := $(wildcard float/*.c)
simd_SRCS    := $(wildcard simd/*.c)
system_SRCS  := $(wildcard system/*.c)
crypto_SRCS  := $(wildcard crypto/*.c)
avx512_SRCS  := $(wildcard avx512/*.c)
avx256_SRCS  := $(wildcard avx256/*.c)

# Derive binary names (strip .c, prefix with bin/)
integer_BINS := $(patsubst integer/%.c, integer/bin/%, $(integer_SRCS))
float_BINS   := $(patsubst float/%.c,   float/bin/%,   $(float_SRCS))
simd_BINS    := $(patsubst simd/%.c,    simd/bin/%,    $(simd_SRCS))
system_BINS  := $(patsubst system/%.c,  system/bin/%,  $(system_SRCS))
crypto_BINS  := $(patsubst crypto/%.c,  crypto/bin/%,  $(crypto_SRCS))
avx512_BINS  := $(patsubst avx512/%.c,  avx512/bin/%,  $(avx512_SRCS))
avx256_BINS  := $(patsubst avx256/%.c,  avx256/bin/%,  $(avx256_SRCS))

ALL_BINS := $(integer_BINS) $(float_BINS) $(simd_BINS) $(system_BINS) $(crypto_BINS) $(avx512_BINS) $(avx256_BINS)

.PHONY: all run box64 clean $(addprefix run-, $(DIRS))

all: $(ALL_BINS)

# Pattern rules per category
integer/bin/%: integer/%.c | integer/bin
	$(CC) $(CFLAGS) $(integer_FLAGS) -o $@ $< $(LDFLAGS)

float/bin/%: float/%.c | float/bin
	$(CC) $(CFLAGS) $(float_FLAGS) -o $@ $< $(LDFLAGS) -lm

simd/bin/%: simd/%.c | simd/bin
	$(CC) $(CFLAGS) $(simd_FLAGS) -o $@ $< $(LDFLAGS)

system/bin/%: system/%.c | system/bin
	$(CC) $(CFLAGS) $(system_FLAGS) -o $@ $< $(LDFLAGS)

crypto/bin/%: crypto/%.c | crypto/bin
	$(CC) $(CFLAGS) $(crypto_FLAGS) -o $@ $< $(LDFLAGS)

avx512/bin/%: avx512/%.c | avx512/bin
	$(CC) $(CFLAGS) $(avx512_FLAGS) -o $@ $< $(LDFLAGS)

avx256/bin/%: avx256/%.c | avx256/bin
	$(CC) $(CFLAGS) $(avx256_FLAGS) -o $@ $< $(LDFLAGS)

# Create bin directories
integer/bin float/bin simd/bin system/bin crypto/bin avx512/bin avx256/bin:
	mkdir -p $@

# Run all tests (compile first)
run: all
	@PASS=0; FAIL=0; ERRORS=""; \
	for bin in $(ALL_BINS); do \
		echo "Running: $$bin"; \
		if $$bin > /dev/null 2>&1; then \
			PASS=$$((PASS+1)); \
		else \
			FAIL=$$((FAIL+1)); \
			ERRORS="$$ERRORS\n  FAILED: $$bin"; \
		fi; \
	done; \
	echo "========================================"; \
	echo "Total: $$((PASS+FAIL))  Passed: $$PASS  Failed: $$FAIL"; \
	if [ -n "$$ERRORS" ]; then printf "$$ERRORS\n"; exit 1; fi

# Run all pre-compiled bins via box64 without building
ALL_BIN_DIRS = integer/bin float/bin simd/bin system/bin crypto/bin avx512/bin avx256/bin
BOX64 ?= box64
box64:
	@PASS=0; FAIL=0; ERRORS=""; \
	for dir in $(ALL_BIN_DIRS); do \
		if [ -d "$$dir" ]; then \
			for bin in $$dir/*; do \
				[ -x "$$bin" ] || continue; \
				echo "Running: $(BOX64) $$bin"; \
				if $(BOX64) $$bin; then \
					PASS=$$((PASS+1)); \
				else \
					FAIL=$$((FAIL+1)); \
					ERRORS="$$ERRORS\n  FAILED: $$bin"; \
				fi; \
			done; \
		fi; \
	done; \
	echo "========================================"; \
	echo "Total: $$((PASS+FAIL))  Passed: $$PASS  Failed: $$FAIL"; \
	if [ -n "$$ERRORS" ]; then printf "$$ERRORS\n"; exit 1; fi

run-integer: $(integer_BINS)
	@PASS=0; FAIL=0; \
	for bin in $(integer_BINS); do \
		if $$bin > /dev/null 2>&1; then PASS=$$((PASS+1)); \
		else FAIL=$$((FAIL+1)); echo "FAILED: $$bin"; fi; \
	done; \
	echo "integer: $$PASS passed, $$FAIL failed"

run-float: $(float_BINS)
	@PASS=0; FAIL=0; \
	for bin in $(float_BINS); do \
		if $$bin > /dev/null 2>&1; then PASS=$$((PASS+1)); \
		else FAIL=$$((FAIL+1)); echo "FAILED: $$bin"; fi; \
	done; \
	echo "float: $$PASS passed, $$FAIL failed"

run-simd: $(simd_BINS)
	@PASS=0; FAIL=0; \
	for bin in $(simd_BINS); do \
		if $$bin > /dev/null 2>&1; then PASS=$$((PASS+1)); \
		else FAIL=$$((FAIL+1)); echo "FAILED: $$bin"; fi; \
	done; \
	echo "simd: $$PASS passed, $$FAIL failed"

run-system: $(system_BINS)
	@PASS=0; FAIL=0; \
	for bin in $(system_BINS); do \
		if $$bin > /dev/null 2>&1; then PASS=$$((PASS+1)); \
		else FAIL=$$((FAIL+1)); echo "FAILED: $$bin"; fi; \
	done; \
	echo "system: $$PASS passed, $$FAIL failed"

run-crypto: $(crypto_BINS)
	@PASS=0; FAIL=0; \
	for bin in $(crypto_BINS); do \
		if $$bin > /dev/null 2>&1; then PASS=$$((PASS+1)); \
		else FAIL=$$((FAIL+1)); echo "FAILED: $$bin"; fi; \
	done; \
	echo "crypto: $$PASS passed, $$FAIL failed"

run-avx512: $(avx512_BINS)
	@PASS=0; FAIL=0; \
	for bin in $(avx512_BINS); do \
		if $$bin > /dev/null 2>&1; then PASS=$$((PASS+1)); \
		else FAIL=$$((FAIL+1)); echo "FAILED: $$bin"; fi; \
	done; \
	echo "avx512: $$PASS passed, $$FAIL failed"

clean:
	rm -rf integer/bin float/bin simd/bin system/bin crypto/bin avx512/bin avx256/bin
