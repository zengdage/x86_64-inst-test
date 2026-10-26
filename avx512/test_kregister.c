/* AVX-512 opmask register instructions and their 16-bit edge cases. */
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx512f(void) {
    uint32_t eax, ebx, ecx, edx;
    uint32_t xcr0_lo, xcr0_hi;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1), "c"(0));
    if (!(ecx & (1U << 27)) || !(ecx & (1U << 28))) return 0;
    __asm__ volatile("xgetbv" : "=a"(xcr0_lo), "=d"(xcr0_hi) : "c"(0));
    if ((xcr0_lo & 0xe6U) != 0xe6U) return 0;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(7), "c"(0));
    return (ebx & (1U << 16)) != 0;
}
#else
#define check_avx512f() 1
#endif

static uint16_t kmov_roundtrip(uint32_t value) {
    uint32_t out;
    __asm__ volatile("kmovw %1, %%k1\n\tkmovw %%k1, %0"
                     : "=r"(out) : "r"(value) : "k1");
    return (uint16_t)out;
}

static void test_kmov_and_logic(void) {
    const uint16_t values[] = {0x0000, 0x0001, 0x8000, 0xffff, 0xa55a};
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); i++)
        TEST_ASSERT(kmov_roundtrip(values[i] | 0xffff0000U) == values[i],
                    "kmovw retains only low 16 bits for 0x%04x", values[i]);

    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        for (unsigned j = 0; j < sizeof(values) / sizeof(values[0]); j++) {
            uint16_t a = values[i], b = values[j];
            uint32_t out;
            __asm__ volatile("kmovw %1, %%k1\n\tkmovw %2, %%k2\n\t"
                             "kandw %%k2, %%k1, %%k3\n\tkmovw %%k3, %0"
                             : "=r"(out) : "r"((uint32_t)a), "r"((uint32_t)b)
                             : "k1", "k2", "k3");
            TEST_ASSERT((uint16_t)out == (uint16_t)(a & b), "kandw pair %u/%u", i, j);
            __asm__ volatile("kmovw %1, %%k1\n\tkmovw %2, %%k2\n\t"
                             "korw %%k2, %%k1, %%k3\n\tkmovw %%k3, %0"
                             : "=r"(out) : "r"((uint32_t)a), "r"((uint32_t)b)
                             : "k1", "k2", "k3");
            TEST_ASSERT((uint16_t)out == (uint16_t)(a | b), "korw pair %u/%u", i, j);
            __asm__ volatile("kmovw %1, %%k1\n\tkmovw %2, %%k2\n\t"
                             "kxorw %%k2, %%k1, %%k3\n\tkmovw %%k3, %0"
                             : "=r"(out) : "r"((uint32_t)a), "r"((uint32_t)b)
                             : "k1", "k2", "k3");
            TEST_ASSERT((uint16_t)out == (uint16_t)(a ^ b), "kxorw pair %u/%u", i, j);
            __asm__ volatile("kmovw %1, %%k1\n\tkmovw %2, %%k2\n\t"
                             "kxnorw %%k2, %%k1, %%k3\n\tkmovw %%k3, %0"
                             : "=r"(out) : "r"((uint32_t)a), "r"((uint32_t)b)
                             : "k1", "k2", "k3");
            TEST_ASSERT((uint16_t)out == (uint16_t)~(a ^ b), "kxnorw pair %u/%u", i, j);
        }
    }

    /* Destination/source register aliasing is legal for the mask ALU. */
    uint32_t out;
    uint16_t value = 0xa55a;
    __asm__ volatile("kmovw %1, %%k1\n\tkandw %%k1, %%k1, %%k1\n\tkmovw %%k1, %0"
                     : "=r"(out) : "r"((uint32_t)value) : "k1");
    TEST_ASSERT((uint16_t)out == value, "kandw all operands aliased");
    __asm__ volatile("kmovw %1, %%k1\n\tknotw %%k1, %%k1\n\tkmovw %%k1, %0"
                     : "=r"(out) : "r"((uint32_t)value) : "k1");
    TEST_ASSERT((uint16_t)out == (uint16_t)~value, "knotw aliased destination");
}

static void test_kshifts(void) {
    const uint16_t values[] = {0, 1, 0x8000, 0xffff, 0xa55a};
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        uint32_t left, right, zero;
        __asm__ volatile("kmovw %3, %%k1\n\t"
                         "kshiftlw $1, %%k1, %%k2\n\t"
                         "kshiftrw $1, %%k1, %%k3\n\t"
                         "kshiftlw $16, %%k1, %%k4\n\t"
                         "kmovw %%k2, %0\n\tkmovw %%k3, %1\n\tkmovw %%k4, %2"
                         : "=r"(left), "=r"(right), "=r"(zero)
                         : "r"((uint32_t)values[i]) : "k1", "k2", "k3", "k4");
        TEST_ASSERT((uint16_t)left == (uint16_t)(values[i] << 1), "kshiftlw value %u", i);
        TEST_ASSERT((uint16_t)right == (uint16_t)(values[i] >> 1), "kshiftrw value %u", i);
        TEST_ASSERT((uint16_t)zero == 0, "kshiftlw count >= width value %u", i);
    }
}

int main(void) {
    TEST_START("AVX-512 opmask register instructions");
    if (!check_avx512f()) {
        printf("AVX-512F not supported, skipping\n");
        return 0;
    }
    test_kmov_and_logic();
    test_kshifts();
    TEST_END();
}
