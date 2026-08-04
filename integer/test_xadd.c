/*
 * test_xadd.c - Test x86-64 XADD instruction
 *
 * XADD exchanges and adds: TEMP=DEST; DEST=DEST+SRC; SRC=TEMP
 * Affects flags: same as ADD (CF, PF, AF, ZF, SF, OF)
 *
 * Compile: gcc -o test_xadd integer/test_xadd.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void assert_xadd_flags(uint64_t flags, int cf, int pf, int af,
                              int zf, int sf, int of, const char *name) {
    TEST_ASSERT(!!(flags & CF_FLAG) == cf, "%s CF=%d expected %d", name,
                !!(flags & CF_FLAG), cf);
    TEST_ASSERT(!!(flags & PF_FLAG) == pf, "%s PF=%d expected %d", name,
                !!(flags & PF_FLAG), pf);
    TEST_ASSERT(!!(flags & AF_FLAG) == af, "%s AF=%d expected %d", name,
                !!(flags & AF_FLAG), af);
    TEST_ASSERT(!!(flags & ZF_FLAG) == zf, "%s ZF=%d expected %d", name,
                !!(flags & ZF_FLAG), zf);
    TEST_ASSERT(!!(flags & SF_FLAG) == sf, "%s SF=%d expected %d", name,
                !!(flags & SF_FLAG), sf);
    TEST_ASSERT(!!(flags & OF_FLAG) == of, "%s OF=%d expected %d", name,
                !!(flags & OF_FLAG), of);
}

static void test_xadd_basic(void) {
    uint64_t dest_val = 10;
    uint64_t src_val;
    uint64_t flags;

    __asm__ volatile (
        "movq $20, %%rax\n\t"
        "xaddq %%rax, %2\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(src_val), "=r"(flags), "+m"(dest_val)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(dest_val == 30, "xaddq: dest = 10+20 = 30, got %lu", dest_val);
    TEST_ASSERT(src_val == 10, "xaddq: src = old dest = 10, got %lu", src_val);
}

static void test_xadd_zero(void) {
    uint64_t dest_val = 0;
    uint64_t src_val;
    uint64_t flags;

    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "xaddq %%rax, %2\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(src_val), "=r"(flags), "+m"(dest_val)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(dest_val == 0, "xaddq 0+0: dest = 0");
    TEST_ASSERT(src_val == 0, "xaddq 0+0: src = 0");
    TEST_ASSERT(flags & ZF_FLAG, "xaddq 0+0: ZF should be set");
}

static void test_xadd_32bit(void) {
    uint32_t dest_val = 0xFFFFFFFF;
    uint32_t src_val;
    uint64_t flags;

    __asm__ volatile (
        "movl $1, %%eax\n\t"
        "xaddl %%eax, %2\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(src_val), "=r"(flags), "+m"(dest_val)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(dest_val == 0, "xaddl overflow: 0xFFFFFFFF+1 = 0");
    TEST_ASSERT(src_val == 0xFFFFFFFF, "xaddl: src = old dest");
    TEST_ASSERT(flags & CF_FLAG, "xaddl overflow: CF should be set");
    TEST_ASSERT(flags & ZF_FLAG, "xaddl overflow: ZF should be set");
}

static void test_xadd_16bit(void) {
    uint16_t dest_val = 100;
    uint16_t src_val;

    __asm__ volatile (
        "movw $50, %%ax\n\t"
        "xaddw %%ax, %1\n\t"
        "movw %%ax, %0"
        : "=r"(src_val), "+m"(dest_val)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(dest_val == 150, "xaddw: dest = 100+50 = 150");
    TEST_ASSERT(src_val == 100, "xaddw: src = old dest = 100");
}

static void test_xadd_width_boundaries_and_flags(void) {
    uint64_t flags;
    uint8_t dest8 = UINT8_MAX, old8;
    __asm__ volatile (
        "movb $1, %%al\n\t"
        "xaddb %%al, %2\n\t"
        "pushfq\n\tpopq %1\n\t"
        "movb %%al, %0"
        : "=m"(old8), "=r"(flags), "+m"(dest8)
        : : "rax", "cc", "memory");
    TEST_ASSERT(dest8 == 0 && old8 == UINT8_MAX,
                "xaddb UINT8_MAX+1 wraps and returns old destination");
    assert_xadd_flags(flags, 1, 1, 1, 1, 0, 0, "xaddb UINT8_MAX+1");

    uint16_t dest16 = INT16_MAX, old16;
    __asm__ volatile (
        "movw $1, %%ax\n\t"
        "xaddw %%ax, %2\n\t"
        "pushfq\n\tpopq %1\n\t"
        "movw %%ax, %0"
        : "=m"(old16), "=r"(flags), "+m"(dest16)
        : : "rax", "cc", "memory");
    TEST_ASSERT(dest16 == UINT16_C(0x8000) && old16 == INT16_MAX,
                "xaddw INT16_MAX+1 reaches signed minimum and returns old destination");
    assert_xadd_flags(flags, 0, 1, 1, 0, 1, 1, "xaddw INT16_MAX+1");

    uint32_t dest32 = UINT32_MAX, old32;
    __asm__ volatile (
        "movl $1, %%eax\n\t"
        "xaddl %%eax, %2\n\t"
        "pushfq\n\tpopq %1\n\t"
        "movl %%eax, %0"
        : "=m"(old32), "=r"(flags), "+m"(dest32)
        : : "rax", "cc", "memory");
    TEST_ASSERT(dest32 == 0 && old32 == UINT32_MAX,
                "xaddl UINT32_MAX+1 wraps and returns old destination");
    assert_xadd_flags(flags, 1, 1, 1, 1, 0, 0, "xaddl UINT32_MAX+1");

    uint64_t dest64 = INT64_MAX, old64;
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "xaddq %%rax, %2\n\t"
        "pushfq\n\tpopq %1\n\t"
        "movq %%rax, %0"
        : "=m"(old64), "=r"(flags), "+m"(dest64)
        : : "rax", "cc", "memory");
    TEST_ASSERT(dest64 == UINT64_C(0x8000000000000000) && old64 == INT64_MAX,
                "xaddq INT64_MAX+1 reaches signed minimum and returns old destination");
    assert_xadd_flags(flags, 0, 1, 1, 0, 1, 1, "xaddq INT64_MAX+1");
}

static void test_xadd_register_and_lock_forms(void) {
    uint32_t dest_reg, src_reg;
    __asm__ volatile (
        "movl $5, %%eax\n\t"
        "movl $7, %%ecx\n\t"
        "xaddl %%ecx, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "movl %%ecx, %1"
        : "=r"(dest_reg), "=r"(src_reg)
        : : "rax", "rcx", "cc");
    TEST_ASSERT(dest_reg == 12 && src_reg == 5,
                "xaddl register form exchanges old destination and stores sum");

    uint64_t dest_mem = UINT64_MAX, old_src, flags;
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "lock xaddq %%rax, %2\n\t"
        "pushfq\n\tpopq %1\n\t"
        "movq %%rax, %0"
        : "=r"(old_src), "=r"(flags), "+m"(dest_mem)
        : : "rax", "cc", "memory");
    TEST_ASSERT(dest_mem == 0 && old_src == UINT64_MAX,
                "lock xaddq UINT64_MAX+1 atomically wraps and returns old value");
    assert_xadd_flags(flags, 1, 1, 1, 1, 0, 0,
                      "lock xaddq UINT64_MAX+1");
}

int main(void) {
    TEST_START("XADD instruction");
    test_xadd_basic();
    test_xadd_zero();
    test_xadd_32bit();
    test_xadd_16bit();
    test_xadd_width_boundaries_and_flags();
    test_xadd_register_and_lock_forms();
    TEST_END();
}
