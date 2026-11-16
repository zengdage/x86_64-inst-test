#include "../common.h"

static void test_cmpsl(void) {
    uint32_t left[] = {1, 2, UINT32_C(0x80000000), 4};
    uint32_t right[] = {1, 2, UINT32_C(0x7fffffff), 4};
    uint64_t count = 4, flags;
    uint32_t *rsi = left, *rdi = right;
    __asm__ volatile (
        "cld\n\trepe cmpsl\n\tpushfq\n\tpopq %[flags]"
        : "+S"(rsi), "+D"(rdi), "+c"(count), [flags] "=r"(flags)
        : : "cc", "memory");
    TEST_ASSERT(count == 1, "repe cmpsd stops at third element");
    TEST_ASSERT(rsi == &left[3] && rdi == &right[3],
                "cmpsd advances RSI/RDI by four bytes");
    TEST_ASSERT(!(flags & ZF_FLAG) && !(flags & CF_FLAG) && !(flags & SF_FLAG) &&
                (flags & OF_FLAG),
                "cmpsd subtraction flags for 0x80000000-0x7fffffff");
}

static void test_cmpsq_backward(void) {
    uint64_t left[] = {9, 2, 3};
    uint64_t right[] = {8, 2, 3};
    uint64_t count = 3, flags;
    uint64_t *rsi = &left[2], *rdi = &right[2];
    __asm__ volatile (
        "std\n\trepe cmpsq\n\tpushfq\n\tpopq %[flags]\n\tcld"
        : "+S"(rsi), "+D"(rdi), "+c"(count), [flags] "=r"(flags)
        : : "cc", "memory");
    TEST_ASSERT(count == 0 && !(flags & ZF_FLAG),
                "backward repe cmpsq reaches mismatch at first element");
    TEST_ASSERT((uintptr_t)rsi == (uintptr_t)left - sizeof(*left) &&
                (uintptr_t)rdi == (uintptr_t)right - sizeof(*right),
                "backward cmpsq decrements RSI/RDI by eight bytes");
}

#define DEFINE_SCAS(NAME, SUFFIX, TYPE, ACC, ACONSTRAINT)                      \
static void test_##NAME(void) {                                                \
    TYPE data[] = {(TYPE)1, (TYPE)2, (TYPE)0x55, (TYPE)4};                    \
    uint64_t count = 4, flags; TYPE *rdi = data;                               \
    TYPE needle = (TYPE)0x55;                                                  \
    __asm__ volatile ("cld\n\trepne scas" SUFFIX                             \
                      "\n\tpushfq\n\tpopq %[flags]"                         \
                      : "+D"(rdi), "+c"(count), [flags] "=r"(flags)         \
                      : ACONSTRAINT(needle) : "cc", "memory");                \
    TEST_ASSERT(count == 1 && rdi == &data[3] && (flags & ZF_FLAG),            \
                #NAME " finds third element and updates count/pointer");      \
}

DEFINE_SCAS(scasw, "w", uint16_t, ax, "a")
DEFINE_SCAS(scasd, "l", uint32_t, eax, "a")
DEFINE_SCAS(scasq, "q", uint64_t, rax, "a")

static void test_scasq_not_found(void) {
    uint64_t data[] = {2, 4, 6};
    uint64_t *rdi = data;
    uint64_t count = 3, flags;
    __asm__ volatile (
        "cld\n\trepne scasq\n\tpushfq\n\tpopq %[flags]"
        : "+D"(rdi), "+c"(count), [flags] "=r"(flags)
        : "a"(UINT64_C(5)) : "cc", "memory");
    TEST_ASSERT(count == 0 && rdi == &data[3] && !(flags & ZF_FLAG),
                "repne scasq not-found state");
    TEST_ASSERT(flags & CF_FLAG, "final scasq computes 5-6 with borrow");
}

int main(void) {
    TEST_START("CMPSD/CMPSQ and SCASW/SCASD/SCASQ");
    test_cmpsl();
    test_cmpsq_backward();
    test_scasw();
    test_scasd();
    test_scasq();
    test_scasq_not_found();
    TEST_END();
}
