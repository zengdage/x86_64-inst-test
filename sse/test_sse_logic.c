/* TARGETS: ANDPS ANDNPS ORPS XORPS ANDPD ANDNPD ORPD XORPD */
#include "../common.h"

#define RUN_LOGIC(NAME, INSN, EXPR) do {                                      \
    xmm_t out;                                                               \
    __asm__ volatile("movdqu %1, %%xmm0\n\t" INSN " %2, %%xmm0\n\t"      \
                     "movdqu %%xmm0, %0"                                    \
                     : "=m"(out) : "m"(a), "m"(b) : "xmm0");             \
    for (int i = 0; i < 2; i++) {                                            \
        uint64_t expected = (EXPR);                                          \
        TEST_ASSERT(out.u64[i] == expected, NAME " qword %d", i);           \
    }                                                                        \
} while (0)

int main(void) {
    const xmm_t a = { .u64 = {
        UINT64_C(0x0123456789abcdef), UINT64_C(0xffff0000aaaa5555)
    } };
    const xmm_t b = { .u64 = {
        UINT64_C(0xfedcba9876543210), UINT64_C(0x123456789abcdef0)
    } };

    TEST_START("SSE packed floating-point bit logic");
    RUN_LOGIC("ANDPS", "andps", a.u64[i] & b.u64[i]);
    RUN_LOGIC("ANDNPS", "andnps", ~a.u64[i] & b.u64[i]);
    RUN_LOGIC("ORPS", "orps", a.u64[i] | b.u64[i]);
    RUN_LOGIC("XORPS", "xorps", a.u64[i] ^ b.u64[i]);
    RUN_LOGIC("ANDPD", "andpd", a.u64[i] & b.u64[i]);
    RUN_LOGIC("ANDNPD", "andnpd", ~a.u64[i] & b.u64[i]);
    RUN_LOGIC("ORPD", "orpd", a.u64[i] | b.u64[i]);
    RUN_LOGIC("XORPD", "xorpd", a.u64[i] ^ b.u64[i]);
    TEST_END();
}
