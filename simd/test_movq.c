/*
 * test_movq.c - Test MOVQ instruction
 *
 * MOVQ: Move 64-bit value between XMM register low quadword and GPR/memory.
 * When loading into XMM, the upper 64 bits are zeroed.
 *
 * Compile: gcc -o test_movq simd/test_movq.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_movq_gpr_to_xmm(void) {
    xmm_t dst;
    uint64_t val = 0xDEADBEEFCAFEBABEULL;

    __asm__ volatile (
        "movq %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst)
        : "r"(val)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == val, "movq gpr->xmm low: expected 0x%016lx, got 0x%016lx", val, dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0, "movq gpr->xmm high: expected 0, got 0x%016lx", dst.u64[1]);
}

static void test_movq_xmm_to_gpr(void) {
    xmm_t src = { .u64 = { 0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL } };
    uint64_t result;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movq %%xmm0, %0"
        : "=r"(result)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(result == src.u64[0], "movq xmm->gpr: expected 0x%016lx, got 0x%016lx",
        src.u64[0], result);
}

static void test_movq_mem_to_xmm(void) {
    uint64_t val = 0xAAAABBBBCCCCDDDDULL;
    xmm_t dst;

    __asm__ volatile (
        "movq %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst)
        : "m"(val)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == val, "movq mem->xmm low: expected 0x%016lx, got 0x%016lx", val, dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0, "movq mem->xmm high zeroed: got 0x%016lx", dst.u64[1]);
}

static void test_movq_xmm_to_mem(void) {
    xmm_t src = { .u64 = { 0x1111222233334444ULL, 0x5555666677778888ULL } };
    uint64_t result = 0;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movq %%xmm0, %0"
        : "=m"(result)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(result == src.u64[0], "movq xmm->mem: expected 0x%016lx, got 0x%016lx",
        src.u64[0], result);
}

static void test_movq_xmm_to_xmm(void) {
    xmm_t src = { .u64 = { 0xCAFEBABEDEADBEEFULL, 0xFFFFFFFFFFFFFFFFULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movq %%xmm0, %%xmm1\n\t"
        "movdqa %%xmm1, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(dst.u64[0] == src.u64[0], "movq xmm->xmm low: expected 0x%016lx, got 0x%016lx",
        src.u64[0], dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0, "movq xmm->xmm high zeroed: got 0x%016lx", dst.u64[1]);
}

static void test_movq_zero(void) {
    xmm_t dst = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL } };
    uint64_t val = 0;

    __asm__ volatile (
        "movq %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst)
        : "r"(val)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0 && dst.u64[1] == 0,
        "movq zero: expected all zeros, got 0x%016lx_%016lx", dst.u64[1], dst.u64[0]);
}

static void test_movq_max(void) {
    xmm_t dst;
    uint64_t val = 0xFFFFFFFFFFFFFFFFULL;

    __asm__ volatile (
        "movq %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst)
        : "r"(val)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL, "movq max low: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0, "movq max high zeroed: got 0x%016lx", dst.u64[1]);
}

static void test_vmovq_upper_ymm_clear_and_unaligned_memory(void) {
    ymm_t initial, result;
    uint64_t value = UINT64_C(0x8000000000000001);
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile (
        "vmovdqu %1, %%ymm2\n\t"
        "vmovq %2, %%xmm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(result) : "m"(initial), "r"(value) : "ymm2"
    );
    TEST_ASSERT(result.u64[0] == value, "VMOVQ GPR source low qword");
    TEST_ASSERT(result.u64[1] == 0 && result.u64[2] == 0 && result.u64[3] == 0,
                "VMOVQ GPR source clears XMM high qword and upper YMM");

    uint8_t load_buf[32] __attribute__((aligned(16)));
    uint8_t store_buf[32] __attribute__((aligned(16)));
    for (int offset = 1; offset < 8; offset++) {
        memset(load_buf, 0xcc, sizeof(load_buf));
        memcpy(load_buf + offset, &value, sizeof(value));
        __asm__ volatile (
            "vmovdqu %1, %%ymm2\n\t"
            "vmovq (%2), %%xmm2\n\t"
            "vmovdqu %%ymm2, %0"
            : "=m"(result) : "m"(initial), "r"(load_buf + offset)
            : "ymm2", "memory"
        );
        TEST_ASSERT(result.u64[0] == value && result.u64[1] == 0 &&
                    result.u64[2] == 0 && result.u64[3] == 0,
                    "VMOVQ memory load offset %d clears remaining destination bits", offset);

        memset(store_buf, 0x5a, sizeof(store_buf));
        __asm__ volatile (
            "vmovq %1, %%xmm0\n\t"
            "vmovq %%xmm0, (%0)"
            : : "r"(store_buf + offset), "r"(value) : "xmm0", "memory"
        );
        uint64_t stored;
        memcpy(&stored, store_buf + offset, sizeof(stored));
        int ok = stored == value;
        for (int i = 0; i < offset; i++) if (store_buf[i] != 0x5a) ok = 0;
        for (int i = offset + 8; i < 32; i++) if (store_buf[i] != 0x5a) ok = 0;
        TEST_ASSERT(ok, "VMOVQ store offset %d writes exactly 8 bytes", offset);
    }
}

int main(void) {
    TEST_START("MOVQ instruction");
    test_movq_gpr_to_xmm();
    test_movq_xmm_to_gpr();
    test_movq_mem_to_xmm();
    test_movq_xmm_to_mem();
    test_movq_xmm_to_xmm();
    test_movq_zero();
    test_movq_max();
    test_vmovq_upper_ymm_clear_and_unaligned_memory();
    TEST_END();
}
