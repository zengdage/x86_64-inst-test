/*
 * test_movhps_movlps.c - Test MOVHPS/MOVLPS/MOVHPD/MOVLPD instructions
 *
 * MOVLPS: Move low packed single-precision (64 bits) between xmm and memory.
 * MOVHPS: Move high packed single-precision (64 bits) between xmm and memory.
 * MOVLPD: Move low packed double-precision (64 bits) between xmm and memory.
 * MOVHPD: Move high packed double-precision (64 bits) between xmm and memory.
 * Load forms: load 64 bits from mem into low/high half of xmm, other half unchanged.
 * Store forms: store low/high half of xmm to memory.
 *
 * Compile: gcc -o test_movhps_movlps simd/test_movhps_movlps.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_movlps_load(void) {
    float mem[2] = { 1.0f, 2.0f };
    xmm_t init = { .f32 = { 10.0f, 20.0f, 30.0f, 40.0f } };
    xmm_t dst;

    __asm__ volatile (
        "movaps %2, %%xmm0\n\t"
        "movlps %1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst)
        : "m"(mem[0]), "m"(init)
        : "xmm0"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f && dst.f32[1] == 2.0f,
        "movlps load: low should be {1,2}, got {%f,%f}", dst.f32[0], dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 30.0f && dst.f32[3] == 40.0f,
        "movlps load: high unchanged, got {%f,%f}", dst.f32[2], dst.f32[3]);
}

static void test_movlps_store(void) {
    xmm_t src = { .f32 = { 5.0f, 6.0f, 7.0f, 8.0f } };
    float mem[2] = { 0.0f, 0.0f };

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movlps %%xmm0, %0"
        : "=m"(mem[0])
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(mem[0] == 5.0f && mem[1] == 6.0f,
        "movlps store: expected {5,6}, got {%f,%f}", mem[0], mem[1]);
}

static void test_movhps_load(void) {
    float mem[2] = { 100.0f, 200.0f };
    xmm_t init = { .f32 = { 1.0f, 2.0f, 3.0f, 4.0f } };
    xmm_t dst;

    __asm__ volatile (
        "movaps %2, %%xmm0\n\t"
        "movhps %1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst)
        : "m"(mem[0]), "m"(init)
        : "xmm0"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f && dst.f32[1] == 2.0f,
        "movhps load: low unchanged, got {%f,%f}", dst.f32[0], dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 100.0f && dst.f32[3] == 200.0f,
        "movhps load: high should be {100,200}, got {%f,%f}", dst.f32[2], dst.f32[3]);
}

static void test_movhps_store(void) {
    xmm_t src = { .f32 = { 5.0f, 6.0f, 7.0f, 8.0f } };
    float mem[2] = { 0.0f, 0.0f };

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movhps %%xmm0, %0"
        : "=m"(mem[0])
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(mem[0] == 7.0f && mem[1] == 8.0f,
        "movhps store: expected {7,8}, got {%f,%f}", mem[0], mem[1]);
}

static void test_movlpd_load(void) {
    double mem_val = 3.14;
    xmm_t init = { .f64 = { 1.0, 2.0 } };
    xmm_t dst;

    __asm__ volatile (
        "movapd %2, %%xmm0\n\t"
        "movlpd %1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst)
        : "m"(mem_val), "m"(init)
        : "xmm0"
    );
    TEST_ASSERT(dst.f64[0] == 3.14, "movlpd load: low expected 3.14, got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 2.0, "movlpd load: high unchanged, got %f", dst.f64[1]);
}

static void test_movlpd_store(void) {
    xmm_t src = { .f64 = { 2.718, 1.414 } };
    double mem_val = 0.0;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movlpd %%xmm0, %0"
        : "=m"(mem_val)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(mem_val == 2.718, "movlpd store: expected 2.718, got %f", mem_val);
}

static void test_movhpd_load(void) {
    double mem_val = 99.99;
    xmm_t init = { .f64 = { 1.0, 2.0 } };
    xmm_t dst;

    __asm__ volatile (
        "movapd %2, %%xmm0\n\t"
        "movhpd %1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst)
        : "m"(mem_val), "m"(init)
        : "xmm0"
    );
    TEST_ASSERT(dst.f64[0] == 1.0, "movhpd load: low unchanged, got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 99.99, "movhpd load: high expected 99.99, got %f", dst.f64[1]);
}

static void test_movhpd_store(void) {
    xmm_t src = { .f64 = { 11.11, 22.22 } };
    double mem_val = 0.0;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movhpd %%xmm0, %0"
        : "=m"(mem_val)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(mem_val == 22.22, "movhpd store: expected 22.22, got %f", mem_val);
}

static void test_partial_moves_all_unaligned_offsets(void) {
    uint8_t load_buf[32] __attribute__((aligned(16)));
    uint8_t store_buf[32] __attribute__((aligned(16)));
    const uint64_t value = UINT64_C(0xfff8123456789abc);
    xmm_t initial = { .u64 = {
        UINT64_C(0x8000000000000000), UINT64_C(0x0000000000000001)
    } };
    xmm_t result;

#define TEST_PARTIAL_MOVE(INSN, HIGH, LABEL, OFFSET) do {                    \
        memset(load_buf, 0xcc, sizeof(load_buf));                            \
        memcpy(load_buf + (OFFSET), &value, sizeof(value));                  \
        __asm__ volatile (                                                  \
            "movdqa %1, %%xmm0\n\t" INSN " (%2), %%xmm0\n\t"          \
            "movdqa %%xmm0, %0"                                           \
            : "=m"(result) : "m"(initial), "r"(load_buf + (OFFSET))      \
            : "xmm0", "memory");                                        \
        TEST_ASSERT(result.u64[(HIGH)] == value &&                          \
                    result.u64[1 - (HIGH)] == initial.u64[1 - (HIGH)],       \
                    LABEL " load offset %d updates only selected half", (OFFSET)); \
        memset(store_buf, 0x5a, sizeof(store_buf));                          \
        __asm__ volatile (                                                  \
            "movdqa %1, %%xmm0\n\t" INSN " %%xmm0, (%0)"                \
            : : "r"(store_buf + (OFFSET)), "m"(initial)                    \
            : "xmm0", "memory");                                        \
        uint64_t stored;                                                     \
        memcpy(&stored, store_buf + (OFFSET), sizeof(stored));               \
        int ok = stored == initial.u64[(HIGH)];                              \
        for (int i = 0; i < (OFFSET); i++) if (store_buf[i] != 0x5a) ok = 0; \
        for (int i = (OFFSET) + 8; i < 32; i++)                              \
            if (store_buf[i] != 0x5a) ok = 0;                               \
        TEST_ASSERT(ok, LABEL " store offset %d writes selected 8 bytes only", (OFFSET)); \
    } while (0)

    for (int offset = 1; offset < 8; offset++) {
        TEST_PARTIAL_MOVE("movlps", 0, "MOVLPS", offset);
        TEST_PARTIAL_MOVE("movhps", 1, "MOVHPS", offset);
        TEST_PARTIAL_MOVE("movlpd", 0, "MOVLPD", offset);
        TEST_PARTIAL_MOVE("movhpd", 1, "MOVHPD", offset);
    }
#undef TEST_PARTIAL_MOVE
}

int main(void) {
    TEST_START("MOVHPS/MOVLPS/MOVHPD/MOVLPD instructions");
    test_movlps_load();
    test_movlps_store();
    test_movhps_load();
    test_movhps_store();
    test_movlpd_load();
    test_movlpd_store();
    test_movhpd_load();
    test_movhpd_store();
    test_partial_moves_all_unaligned_offsets();
    TEST_END();
}
