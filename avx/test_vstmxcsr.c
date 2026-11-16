/* TARGET: VSTMXCSR */
#include "../common.h"

int main(void) {
    uint32_t original, requested, observed = UINT32_C(0xdeadbeef);
    TEST_START("VSTMXCSR");
    __asm__ volatile("stmxcsr %0" : "=m"(original));
    requested = (original & ~UINT32_C(0x6000)) | UINT32_C(0x2000);
    __asm__ volatile("ldmxcsr %1\n\tvstmxcsr %0"
                     : "=m"(observed) : "m"(requested) : "memory");
    __asm__ volatile("ldmxcsr %0" : : "m"(original));
    TEST_ASSERT(observed == requested,
                "VSTMXCSR expected %#x got %#x", requested, observed);
    TEST_END();
}
