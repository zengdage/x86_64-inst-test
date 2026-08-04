/*
 * test_movdqa_movdqu.c - Test MOVDQA and MOVDQU instructions
 *
 * MOVDQA: Move aligned double quadword (128-bit). Source/dest must be 16-byte aligned.
 * MOVDQU: Move unaligned double quadword (128-bit). No alignment requirement.
 *
 * Compile: gcc -o test_movdqa_movdqu simd/test_movdqa_movdqu.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_movdqa_xmm_to_xmm(void) {
    xmm_t src = { .u64 = { 0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %%xmm0, %%xmm1\n\t"
        "movdqa %%xmm1, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(dst.u64[0] == src.u64[0] && dst.u64[1] == src.u64[1],
        "movdqa xmm->xmm: expected 0x%016lx_%016lx, got 0x%016lx_%016lx",
        src.u64[1], src.u64[0], dst.u64[1], dst.u64[0]);
}

static void test_movdqa_mem_to_xmm(void) {
    xmm_t src = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == src.u64[0] && dst.u64[1] == src.u64[1],
        "movdqa mem->xmm: data mismatch");
}

static void test_movdqa_xmm_to_mem(void) {
    xmm_t src = { .u64 = { 0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == src.u64[0] && dst.u64[1] == src.u64[1],
        "movdqa xmm->mem: data mismatch");
}

static void test_movdqu_unaligned(void) {
    /* Use a buffer with offset to test unaligned access */
    uint8_t buf[48] __attribute__((aligned(16)));
    memset(buf, 0, sizeof(buf));
    /* Write pattern at offset 1 (unaligned) */
    for (int i = 0; i < 16; i++) buf[1 + i] = (uint8_t)(i + 1);

    xmm_t dst;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(dst)
        : "m"(buf[1])
        : "xmm0"
    );
    int ok = 1;
    for (int i = 0; i < 16; i++) {
        if (dst.u8[i] != (uint8_t)(i + 1)) { ok = 0; break; }
    }
    TEST_ASSERT(ok, "movdqu unaligned load: data mismatch");
}

static void test_movdqu_xmm_to_xmm(void) {
    xmm_t src = { .u64 = { 0xAAAABBBBCCCCDDDDULL, 0xEEEEFFFF00001111ULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqu %1, %%xmm2\n\t"
        "movdqu %%xmm2, %%xmm3\n\t"
        "movdqu %%xmm3, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm2", "xmm3"
    );
    TEST_ASSERT(dst.u64[0] == src.u64[0] && dst.u64[1] == src.u64[1],
        "movdqu xmm->xmm: data mismatch");
}

static void test_movdqu_store_unaligned(void) {
    xmm_t src = { .u8 = {10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160} };
    uint8_t buf[48] __attribute__((aligned(16)));
    memset(buf, 0, sizeof(buf));

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(buf[3])
        : "m"(src)
        : "xmm0"
    );
    int ok = 1;
    for (int i = 0; i < 16; i++) {
        if (buf[3 + i] != src.u8[i]) { ok = 0; break; }
    }
    TEST_ASSERT(ok, "movdqu unaligned store: data mismatch");
}

static void test_movdqa_all_zeros(void) {
    xmm_t src = { .u64 = { 0, 0 } };
    xmm_t dst = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL } };

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0 && dst.u64[1] == 0,
        "movdqa all zeros: expected 0, got 0x%016lx_%016lx", dst.u64[1], dst.u64[0]);
}

static void test_movdqa_all_ones(void) {
    xmm_t src = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL } };
    xmm_t dst = { .u64 = { 0, 0 } };

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL && dst.u64[1] == 0xFFFFFFFFFFFFFFFFULL,
        "movdqa all ones: data mismatch");
}

static void test_movdqu_all_unaligned_offsets_and_store_guards(void) {
    uint8_t load_buf[64] __attribute__((aligned(16)));
    uint8_t store_buf[64] __attribute__((aligned(16)));
    xmm_t pattern, result;
    for (int i = 0; i < 16; i++) pattern.u8[i] = (uint8_t)(i * 17 + 3);

    for (int offset = 1; offset < 16; offset++) {
        memset(load_buf, 0xcc, sizeof(load_buf));
        memcpy(load_buf + offset, &pattern, sizeof(pattern));
        __asm__ volatile (
            "movdqu (%1), %%xmm0\n\t"
            "movdqa %%xmm0, %0"
            : "=m"(result) : "r"(load_buf + offset) : "xmm0", "memory"
        );
        TEST_ASSERT(memcmp(&result, &pattern, sizeof(pattern)) == 0,
                    "movdqu load offset %d crossing alignment boundary", offset);

        memset(store_buf, 0x5a, sizeof(store_buf));
        __asm__ volatile (
            "movdqa %1, %%xmm0\n\t"
            "movdqu %%xmm0, (%0)"
            : : "r"(store_buf + offset), "m"(pattern) : "xmm0", "memory"
        );
        int data_ok = memcmp(store_buf + offset, &pattern, sizeof(pattern)) == 0;
        int guards_ok = 1;
        for (int i = 0; i < offset; i++) if (store_buf[i] != 0x5a) guards_ok = 0;
        for (int i = offset + 16; i < 64; i++) if (store_buf[i] != 0x5a) guards_ok = 0;
        TEST_ASSERT(data_ok && guards_ok,
                    "movdqu store offset %d writes exactly 16 bytes", offset);
    }
}

int main(void) {
    TEST_START("MOVDQA/MOVDQU instructions");
    test_movdqa_xmm_to_xmm();
    test_movdqa_mem_to_xmm();
    test_movdqa_xmm_to_mem();
    test_movdqu_unaligned();
    test_movdqu_xmm_to_xmm();
    test_movdqu_store_unaligned();
    test_movdqa_all_zeros();
    test_movdqa_all_ones();
    test_movdqu_all_unaligned_offsets_and_store_guards();
    TEST_END();
}
