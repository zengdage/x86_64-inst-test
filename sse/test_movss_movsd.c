#include "../common.h"

static void test_movss(void) {
    const uint32_t bits = UINT32_C(0x80000000);
    const xmm_t initial = {.u32 = {1, 2, 3, 4}};
    xmm_t result;
    __asm__ volatile (
        "movdqu %[initial], %%xmm0\n\tmovss %[source], %%xmm0\n\t"
        "movdqu %%xmm0, %[result]"
        : [result] "=m"(result)
        : [initial] "m"(initial), [source] "m"(bits) : "xmm0");
    TEST_ASSERT(result.u32[0] == bits, "movss memory load preserves -0 bits");
    TEST_ASSERT(result.u32[1] == 0 && result.u32[2] == 0 && result.u32[3] == 0,
                "movss memory load clears bits 127:32");

    const xmm_t source = {.u32 = {UINT32_C(0x7fc12345), 20, 30, 40}};
    const xmm_t destination = {.u32 = {10, 11, 12, 13}};
    __asm__ volatile (
        "movdqu %[source], %%xmm1\n\tmovdqu %[destination], %%xmm0\n\t"
        "movss %%xmm1, %%xmm0\n\tmovdqu %%xmm0, %[result]"
        : [result] "=m"(result)
        : [source] "m"(source), [destination] "m"(destination) : "xmm0", "xmm1");
    TEST_ASSERT(result.u32[0] == source.u32[0], "movss register copy preserves NaN payload");
    TEST_ASSERT(result.u32[1] == 11 && result.u32[2] == 12 && result.u32[3] == 13,
                "legacy movss register form preserves destination upper lanes");

    struct { uint32_t guard0, value, guard1; } store = {0xaaaaaaaa, 0, 0xbbbbbbbb};
    __asm__ volatile ("movdqu %1, %%xmm0\n\tmovss %%xmm0, %0"
                      : "=m"(store.value) : "m"(source) : "xmm0");
    TEST_ASSERT(store.value == source.u32[0] && store.guard0 == 0xaaaaaaaa &&
                store.guard1 == 0xbbbbbbbb, "movss stores exactly four bytes");
}

static void test_movsd(void) {
    const uint64_t bits = UINT64_C(0x8000000000000000);
    const xmm_t initial = {.u64 = {1, 2}};
    xmm_t result;
    __asm__ volatile (
        "movdqu %[initial], %%xmm0\n\tmovsd %[source], %%xmm0\n\t"
        "movdqu %%xmm0, %[result]"
        : [result] "=m"(result)
        : [initial] "m"(initial), [source] "m"(bits) : "xmm0");
    TEST_ASSERT(result.u64[0] == bits, "movsd memory load preserves -0 bits");
    TEST_ASSERT(result.u64[1] == 0, "movsd memory load clears bits 127:64");

    const xmm_t source = {.u64 = {UINT64_C(0x7ff8123456789abc), 0x2222}};
    const xmm_t destination = {.u64 = {0x1111, UINT64_C(0x8877665544332211)}};
    __asm__ volatile (
        "movdqu %[source], %%xmm1\n\tmovdqu %[destination], %%xmm0\n\t"
        "movsd %%xmm1, %%xmm0\n\tmovdqu %%xmm0, %[result]"
        : [result] "=m"(result)
        : [source] "m"(source), [destination] "m"(destination) : "xmm0", "xmm1");
    TEST_ASSERT(result.u64[0] == source.u64[0], "movsd register copy preserves NaN payload");
    TEST_ASSERT(result.u64[1] == destination.u64[1],
                "legacy movsd register form preserves destination high qword");

    struct { uint64_t guard0, value, guard1; } store = {
        UINT64_C(0xaaaaaaaaaaaaaaaa), 0, UINT64_C(0xbbbbbbbbbbbbbbbb)};
    __asm__ volatile ("movdqu %1, %%xmm0\n\tmovsd %%xmm0, %0"
                      : "=m"(store.value) : "m"(source) : "xmm0");
    TEST_ASSERT(store.value == source.u64[0] &&
                store.guard0 == UINT64_C(0xaaaaaaaaaaaaaaaa) &&
                store.guard1 == UINT64_C(0xbbbbbbbbbbbbbbbb),
                "movsd stores exactly eight bytes");
}

int main(void) {
    TEST_START("legacy MOVSS/MOVSD");
    test_movss();
    test_movsd();
    TEST_END();
}
