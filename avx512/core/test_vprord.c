/* AVX-512F immediate dword rotate right. */
#include "../include/avx512_test.h"

static uint32_t reference(uint32_t value, unsigned count) {
    count &= 31U;
    return count ? (value >> count) | (value << (32U - count)) : value;
}

int main(void) {
    TEST_START("AVX-512F VPRORD");
    if (!CHECK_AVX512F()) return 0;
    zmm_test_t value, r0, r7, r31, r32, r33;
    for (int i = 0; i < 16; i++)
        value.u32[i] = UINT32_C(0x81234567) ^ (uint32_t)(i * 0x11111111U);
    __asm__ volatile("vmovdqa32 %5, %%zmm0\n\tvprord $0, %%zmm0, %%zmm1\n\t"
                     "vprord $7, %%zmm0, %%zmm2\n\tvprord $31, %%zmm0, %%zmm3\n\t"
                     "vprord $32, %%zmm0, %%zmm4\n\tvprord $33, %%zmm0, %%zmm5\n\t"
                     "vmovdqa32 %%zmm1, %0\n\tvmovdqa32 %%zmm2, %1\n\t"
                     "vmovdqa32 %%zmm3, %2\n\tvmovdqa32 %%zmm4, %3\n\t"
                     "vmovdqa32 %%zmm5, %4"
                     : "=m"(r0), "=m"(r7), "=m"(r31), "=m"(r32), "=m"(r33)
                     : "m"(value)
                     : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5");
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(r0.u32[i] == reference(value.u32[i], 0), "VPRORD count 0 lane %d", i);
        TEST_ASSERT(r7.u32[i] == reference(value.u32[i], 7), "VPRORD count 7 lane %d", i);
        TEST_ASSERT(r31.u32[i] == reference(value.u32[i], 31), "VPRORD count 31 lane %d", i);
        TEST_ASSERT(r32.u32[i] == reference(value.u32[i], 32), "VPRORD count 32 lane %d", i);
        TEST_ASSERT(r33.u32[i] == reference(value.u32[i], 33), "VPRORD count 33 lane %d", i);
    }
    TEST_END();
}
