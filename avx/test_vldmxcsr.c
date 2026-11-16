/* TARGET: VLDMXCSR */
#include "../common.h"

int main(void) {
    uint32_t original, requested, observed;
    TEST_START("VLDMXCSR");
    __asm__ volatile("stmxcsr %0" : "=m"(original));
    requested = (original & ~UINT32_C(0x6000)) | UINT32_C(0x4000);
    __asm__ volatile("vldmxcsr %1\n\tstmxcsr %0"
                     : "=m"(observed) : "m"(requested) : "memory");
    __asm__ volatile("ldmxcsr %0" : : "m"(original));
    TEST_ASSERT(observed == requested,
                "VLDMXCSR expected %#x got %#x", requested, observed);
    TEST_END();
}
