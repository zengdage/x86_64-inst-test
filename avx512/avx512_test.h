#ifndef X86_64_AVX512_TEST_H
#define X86_64_AVX512_TEST_H

#include "../common.h"

typedef union {
    int8_t i8[64];
    uint8_t u8[64];
    int16_t i16[32];
    uint16_t u16[32];
    int32_t i32[16];
    uint32_t u32[16];
    int64_t i64[8];
    uint64_t u64[8];
    float f32[16];
    double f64[8];
} zmm_test_t __attribute__((aligned(64)));

#if ENABLE_RUNTIME_CPU_CHECKS
static inline uint32_t avx512_cpuid7_ebx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(7), "c"(0));
    return ebx;
}

static inline uint32_t avx512_cpuid7_ecx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(7), "c"(0));
    return ecx;
}

static inline uint32_t avx512_cpuid7_edx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(7), "c"(0));
    return edx;
}

static inline uint32_t avx512_cpuid7_1_eax(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(7), "c"(1));
    return eax;
}

#define CHECK_AVX512F() ((avx512_cpuid7_ebx() & (1U << 16)) != 0)
#define CHECK_AVX512BW() ((avx512_cpuid7_ebx() & (1U << 30)) != 0)
#define CHECK_AVX512IFMA() ((avx512_cpuid7_ebx() & (1U << 21)) != 0)
#define CHECK_AVX512VBMI() ((avx512_cpuid7_ecx() & (1U << 1)) != 0)
#define CHECK_AVX512CD() ((avx512_cpuid7_ebx() & (1U << 28)) != 0)
#define CHECK_AVX512ER() ((avx512_cpuid7_ebx() & (1U << 27)) != 0)
#define CHECK_AVX512PF() ((avx512_cpuid7_ebx() & (1U << 26)) != 0)
#define CHECK_AVX512VBMI2() ((avx512_cpuid7_ecx() & (1U << 6)) != 0)
#define CHECK_AVX512VNNI() ((avx512_cpuid7_ecx() & (1U << 11)) != 0)
#define CHECK_AVX512BITALG() ((avx512_cpuid7_ecx() & (1U << 12)) != 0)
#define CHECK_AVX512VPOPCNTDQ() ((avx512_cpuid7_ecx() & (1U << 14)) != 0)
#define CHECK_AVX5124VNNIW() ((avx512_cpuid7_edx() & (1U << 2)) != 0)
#define CHECK_AVX5124FMAPS() ((avx512_cpuid7_edx() & (1U << 3)) != 0)
#define CHECK_AVX512VP2INTERSECT() ((avx512_cpuid7_edx() & (1U << 8)) != 0)
#define CHECK_AVX512FP16() ((avx512_cpuid7_edx() & (1U << 23)) != 0)
#define CHECK_AVX512BF16() ((avx512_cpuid7_1_eax() & (1U << 5)) != 0)
#else
#define CHECK_AVX512F() 1
#define CHECK_AVX512BW() 1
#define CHECK_AVX512IFMA() 1
#define CHECK_AVX512VBMI() 1
#define CHECK_AVX512CD() 1
#define CHECK_AVX512ER() 1
#define CHECK_AVX512PF() 1
#define CHECK_AVX512VBMI2() 1
#define CHECK_AVX512VNNI() 1
#define CHECK_AVX512BITALG() 1
#define CHECK_AVX512VPOPCNTDQ() 1
#define CHECK_AVX5124VNNIW() 1
#define CHECK_AVX5124FMAPS() 1
#define CHECK_AVX512VP2INTERSECT() 1
#define CHECK_AVX512FP16() 1
#define CHECK_AVX512BF16() 1
#endif

#endif
