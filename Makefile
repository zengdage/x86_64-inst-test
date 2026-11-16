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

# MXCSR exception and rounding checks are disabled by default. Enable with
# `make -B ENABLE_MXCSR_CHECK=1` when validating MXCSR state handling.
ENABLE_MXCSR_CHECK ?= 0
ifeq ($(ENABLE_MXCSR_CHECK),1)
CFLAGS += -DENABLE_MXCSR_CHECK=1
endif

# Per-category extra flags follow the encoding boundary represented by each
# directory. AVX-512 rare tests add their feature flags below.
scalar_FLAGS = -madx -mbmi -mbmi2 -mlzcnt -mpopcnt -mmovbe -mcrc32 -mprfchw -mclwb
x87_LIBS     = -lm
sse_FLAGS    = -msse4.2 -maes -mpclmul -msha -mgfni
sse_LIBS     = -lm
avx_FLAGS    = -mavx2 -mfma -mf16c -maes -mpclmul -msha -mgfni
avx_LIBS     = -lm
system_FLAGS =
avx512_FLAGS = -mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512ifma -mavx512vbmi -mavx512cd -mavx512vbmi2 -mavx512vnni -mavx512bitalg -maes -mpclmul -msha -mgfni -mvaes -mvpclmulqdq
avx512_LIBS  = -lm

scalar_SRCS      := $(wildcard scalar/*.c)
compat32_SRCS    := $(wildcard compat32/*.c)
x87_SRCS         := $(wildcard x87/*.c)
sse_SRCS         := $(wildcard sse/*.c)
avx_SRCS         := $(wildcard avx/*.c)
system_SRCS      := $(wildcard system/*.c)
avx512_core_SRCS := $(wildcard avx512/core/*.c)
avx512_rare_SRCS := $(wildcard avx512/rare/*.c)

scalar_BINS      := $(patsubst scalar/%.c,scalar/bin/%,$(scalar_SRCS))
compat32_BINS    := $(patsubst compat32/%.c,compat32/bin/%,$(compat32_SRCS))
x87_BINS         := $(patsubst x87/%.c,x87/bin/%,$(x87_SRCS))
sse_BINS         := $(patsubst sse/%.c,sse/bin/%,$(sse_SRCS))
avx_BINS         := $(patsubst avx/%.c,avx/bin/%,$(avx_SRCS))
system_BINS      := $(patsubst system/%.c,system/bin/%,$(system_SRCS))
avx512_core_BINS := $(patsubst avx512/core/%.c,avx512/core/bin/%,$(avx512_core_SRCS))
avx512_rare_BINS := $(patsubst avx512/rare/%.c,avx512/rare/bin/%,$(avx512_rare_SRCS))

ALL_BINS := $(scalar_BINS) $(compat32_BINS) $(x87_BINS) $(sse_BINS) \
	$(avx_BINS) $(system_BINS) $(avx512_core_BINS) $(avx512_rare_BINS)
NATIVE_BINS := $(scalar_BINS) $(compat32_BINS) $(x87_BINS) $(sse_BINS) \
	$(avx_BINS) $(system_BINS) $(avx512_core_BINS)

BIN_DIRS := scalar/bin compat32/bin x87/bin sse/bin avx/bin system/bin \
	avx512/core/bin avx512/rare/bin

.PHONY: all run box64 clean run-scalar run-compat32 run-x87 run-sse run-avx \
	run-system run-avx512 run-avx512-all

all: $(ALL_BINS)

scalar/bin/%: scalar/%.c | scalar/bin
	$(CC) $(CFLAGS) $(scalar_FLAGS) -o $@ $< $(LDFLAGS)

compat32/bin/%: compat32/%.c | compat32/bin
	$(CC) $(CFLAGS) -m32 -o $@ $< $(LDFLAGS)

x87/bin/%: x87/%.c | x87/bin
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(x87_LIBS)

sse/bin/%: sse/%.c | sse/bin
	$(CC) $(CFLAGS) $(sse_FLAGS) -o $@ $< $(LDFLAGS) $(sse_LIBS)

avx/bin/%: avx/%.c | avx/bin
	$(CC) $(CFLAGS) $(avx_FLAGS) -o $@ $< $(LDFLAGS) $(avx_LIBS)

system/bin/%: system/%.c | system/bin
	$(CC) $(CFLAGS) $(system_FLAGS) -o $@ $< $(LDFLAGS)

avx512/core/bin/%: avx512/core/%.c | avx512/core/bin
	$(CC) $(CFLAGS) $(avx512_FLAGS) -o $@ $< $(LDFLAGS) $(avx512_LIBS)

avx512/rare/bin/%: avx512/rare/%.c | avx512/rare/bin
	$(CC) $(CFLAGS) $(avx512_FLAGS) -o $@ $< $(LDFLAGS) $(avx512_LIBS)

avx512_PF_NAMES := test_vgatherpf0dpd test_vgatherpf0dps test_vgatherpf0qpd \
	test_vgatherpf0qps test_vgatherpf1dpd test_vgatherpf1dps \
	test_vgatherpf1qpd test_vgatherpf1qps test_vscatterpf0dpd \
	test_vscatterpf0dps test_vscatterpf0qpd test_vscatterpf0qps \
	test_vscatterpf1dpd test_vscatterpf1dps test_vscatterpf1qpd \
	test_vscatterpf1qps
avx512_ER_NAMES := test_vexp2pd test_vexp2ps test_vrcp28pd test_vrcp28ps \
	test_vrcp28sd test_vrcp28ss test_vrsqrt28pd test_vrsqrt28ps \
	test_vrsqrt28sd test_vrsqrt28ss

avx512/rare/bin/test_avx5124fmaps: avx512_FLAGS += -mavx5124fmaps
avx512/rare/bin/test_vp4dpwssd avx512/rare/bin/test_vp4dpwssds: avx512_FLAGS += -mavx5124vnniw
avx512/rare/bin/test_avx512bf16: avx512_FLAGS += -mavx512bf16
$(addprefix avx512/rare/bin/,$(avx512_ER_NAMES)): avx512_FLAGS += -mavx512er
$(addprefix avx512/rare/bin/,$(avx512_PF_NAMES)): avx512_FLAGS += -mavx512pf
avx512/rare/bin/test_vp2intersectd avx512/rare/bin/test_vp2intersectq: avx512_FLAGS += -mavx512vp2intersect
avx512/rare/bin/test_vpopcntd avx512/rare/bin/test_vpopcntq: avx512_FLAGS += -mavx512vpopcntdq

$(BIN_DIRS):
	mkdir -p $@

define RUN_LIST
	@PASS=0; FAIL=0; ERRORS=""; \
	for bin in $(1); do \
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
endef

run: all
	$(call RUN_LIST,$(NATIVE_BINS))

run-scalar: $(scalar_BINS)
	$(call RUN_LIST,$(scalar_BINS))

run-compat32: $(compat32_BINS)
	$(call RUN_LIST,$(compat32_BINS))

run-x87: $(x87_BINS)
	$(call RUN_LIST,$(x87_BINS))

run-sse: $(sse_BINS)
	$(call RUN_LIST,$(sse_BINS))

run-avx: $(avx_BINS)
	$(call RUN_LIST,$(avx_BINS))

run-system: $(system_BINS)
	$(call RUN_LIST,$(system_BINS))

run-avx512: $(avx512_core_BINS)
	$(call RUN_LIST,$(avx512_core_BINS))

run-avx512-all: $(avx512_core_BINS) $(avx512_rare_BINS)
	$(call RUN_LIST,$(avx512_core_BINS) $(avx512_rare_BINS))

# Run all precompiled binaries through a translator without rebuilding.
BOX64 ?= box64
BOX64_BIN_DIRS := scalar/bin compat32/bin x87/bin sse/bin avx/bin system/bin \
	avx512/core/bin avx512/rare/bin
box64:
	@PASS=0; FAIL=0; ERRORS=""; \
	for dir in $(BOX64_BIN_DIRS); do \
		if [ -d "$$dir" ]; then \
			for bin in $$dir/*; do \
				[ -x "$$bin" ] || continue; \
				echo "Running: $(BOX64) $$bin"; \
				if $(BOX64) "$$bin"; then PASS=$$((PASS+1)); \
				else FAIL=$$((FAIL+1)); ERRORS="$$ERRORS\n  FAILED: $$bin"; fi; \
			done; \
		fi; \
	done; \
	echo "========================================"; \
	echo "Total: $$((PASS+FAIL))  Passed: $$PASS  Failed: $$FAIL"; \
	if [ -n "$$ERRORS" ]; then printf "$$ERRORS\n"; exit 1; fi

clean:
	rm -rf $(BIN_DIRS)
