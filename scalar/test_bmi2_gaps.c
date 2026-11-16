#include "../common.h"

static const uint64_t flag_mask =
    CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;

#define CHECK_SHIFT64(NAME, INSN, VALUE, COUNT, EXPECTED) do {                 \
    uint64_t result, before, after;                                             \
    __asm__ volatile (                                                          \
        "movq $0x8d5, %%r11\n\tpushq %%r11\n\tpopfq\n\t"                  \
        "pushfq\n\tpopq %1\n\t" INSN " %3, %4, %0\n\t"                  \
        "pushfq\n\tpopq %2"                                                   \
        : "=&r"(result), "=&r"(before), "=&r"(after)                        \
        : "r"((uint64_t)(COUNT)), "r"((uint64_t)(VALUE)) : "r11", "cc"); \
    TEST_ASSERT(result == (uint64_t)(EXPECTED),                                 \
                NAME " value=%#" PRIx64 " count=%u: got %#" PRIx64,          \
                (uint64_t)(VALUE), (unsigned)(COUNT), result);                  \
    TEST_ASSERT((before & flag_mask) == (after & flag_mask),                    \
                NAME " preserves flags for count %u", (unsigned)(COUNT));     \
} while (0)

#define CHECK_SHIFT32(NAME, INSN, VALUE, COUNT, EXPECTED) do {                 \
    uint32_t result; uint64_t before, after;                                    \
    __asm__ volatile (                                                          \
        "movq $0x8d5, %%r11\n\tpushq %%r11\n\tpopfq\n\t"                  \
        "pushfq\n\tpopq %1\n\t" INSN " %k3, %k4, %k0\n\t"               \
        "pushfq\n\tpopq %2"                                                   \
        : "=&r"(result), "=&r"(before), "=&r"(after)                        \
        : "r"((uint32_t)(COUNT)), "r"((uint32_t)(VALUE)) : "r11", "cc"); \
    TEST_ASSERT(result == (uint32_t)(EXPECTED),                                 \
                NAME " value=%#x count=%u: got %#x",                          \
                (uint32_t)(VALUE), (unsigned)(COUNT), result);                  \
    TEST_ASSERT((before & flag_mask) == (after & flag_mask),                    \
                NAME " preserves flags for count %u", (unsigned)(COUNT));     \
} while (0)

static void test_variable_shifts(void) {
    static const unsigned counts64[] = {0, 1, 63, 64, 65, 255};
    static const unsigned counts32[] = {0, 1, 31, 32, 33, 255};
    const uint64_t u64 = UINT64_C(0x8123456789abcdef);
    const uint32_t u32 = UINT32_C(0x89abcdef);

    for (unsigned i = 0; i < sizeof(counts64) / sizeof(counts64[0]); ++i) {
        unsigned count = counts64[i] & 63;
        CHECK_SHIFT64("shlxq", "shlxq", u64, counts64[i], u64 << count);
        CHECK_SHIFT64("shrxq", "shrxq", u64, counts64[i], u64 >> count);
        CHECK_SHIFT64("sarxq", "sarxq", u64, counts64[i],
                      (uint64_t)((int64_t)u64 >> count));
    }
    for (unsigned i = 0; i < sizeof(counts32) / sizeof(counts32[0]); ++i) {
        unsigned count = counts32[i] & 31;
        CHECK_SHIFT32("shlxl", "shlxl", u32, counts32[i], u32 << count);
        CHECK_SHIFT32("shrxl", "shrxl", u32, counts32[i], u32 >> count);
        CHECK_SHIFT32("sarxl", "sarxl", u32, counts32[i],
                      (uint32_t)((int32_t)u32 >> count));
    }
}

static void test_memory_sources(void) {
    const uint64_t src = UINT64_C(0xf00000000000000f);
    uint64_t left, right, arithmetic;
    __asm__ volatile (
        "shlxq %[count], %[src], %[left]\n\t"
        "shrxq %[count], %[src], %[right]\n\t"
        "sarxq %[count], %[src], %[arithmetic]"
        : [left] "=&r"(left), [right] "=&r"(right),
          [arithmetic] "=&r"(arithmetic)
        : [src] "m"(src), [count] "r"(4UL));
    TEST_ASSERT(left == UINT64_C(0x00000000000000f0), "shlx memory source");
    TEST_ASSERT(right == UINT64_C(0x0f00000000000000), "shrx memory source");
    TEST_ASSERT(arithmetic == UINT64_C(0xff00000000000000), "sarx memory source");
}

static void test_mulx(void) {
    static const struct { uint64_t a, b; } cases[] = {
        {0, UINT64_MAX}, {1, UINT64_MAX}, {UINT64_MAX, UINT64_MAX},
        {UINT64_C(0x8000000000000000), 2},
        {UINT64_C(0x123456789abcdef0), UINT64_C(0xfedcba9876543210)}
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        uint64_t lo, hi, before, after;
        unsigned __int128 product =
            (unsigned __int128)cases[i].a * cases[i].b;
        __asm__ volatile (
            "movq $0x8d5, %%r11\n\tpushq %%r11\n\tpopfq\n\t"
            "pushfq\n\tpopq %[before]\n\t"
            "mulxq %[src], %[lo], %[hi]\n\t"
            "pushfq\n\tpopq %[after]"
            : [lo] "=&r"(lo), [hi] "=&r"(hi), [before] "=&r"(before),
              [after] "=&r"(after)
            : [src] "rm"(cases[i].b), "d"(cases[i].a) : "r11", "cc");
        TEST_ASSERT(lo == (uint64_t)product && hi == (uint64_t)(product >> 64),
                    "mulxq case %u: hi:lo=%#" PRIx64 ":%016" PRIx64, i, hi, lo);
        TEST_ASSERT((before & flag_mask) == (after & flag_mask),
                    "mulxq case %u preserves flags", i);
    }

    {
        uint32_t lo, hi;
        uint64_t product = (uint64_t)UINT32_C(0xffffffff) * UINT32_C(0x80000001);
        __asm__ volatile ("mulxl %2, %0, %1"
                          : "=&r"(lo), "=&r"(hi)
                          : "rm"(UINT32_C(0x80000001)), "d"(UINT32_C(0xffffffff)));
        TEST_ASSERT(lo == (uint32_t)product && hi == (uint32_t)(product >> 32),
                    "mulxl high and low halves");
    }
}

int main(void) {
    TEST_START("BMI2 MULX/SHLX/SHRX/SARX");
    test_variable_shifts();
    test_memory_sources();
    test_mulx();
    TEST_END();
}
