/*
 * test_cmpxchg.c - Test x86-64 CMPXCHG instruction
 *
 * CMPXCHG compares AL/AX/EAX/RAX with DEST.
 *   If equal: ZF=1, DEST = SRC
 *   If not equal: ZF=0, AL/AX/EAX/RAX = DEST
 * Affects flags: CF, PF, AF, ZF, SF, OF (same as CMP)
 *
 * Compile: gcc -o test_cmpxchg integer/test_cmpxchg.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void assert_cmpxchg_flags(uint64_t flags, int cf, int pf, int af,
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

static void test_cmpxchg_equal(void) {
    uint64_t rax_val, dest_val;
    uint64_t flags;

    /* RAX == DEST => exchange happens, ZF set */
    dest_val = 42;
    __asm__ volatile (
        "movq $42, %%rax\n\t"
        "movq $99, %%rcx\n\t"
        "cmpxchgq %%rcx, %2\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(rax_val), "=r"(flags), "+m"(dest_val)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(dest_val == 99, "cmpxchgq equal: dest should be 99, got %lu", dest_val);
    TEST_ASSERT(rax_val == 42, "cmpxchgq equal: rax unchanged at 42");
    TEST_ASSERT(flags & ZF_FLAG, "cmpxchgq equal: ZF should be set");
}

static void test_cmpxchg_not_equal(void) {
    uint64_t rax_val, dest_val;
    uint64_t flags;

    /* RAX != DEST => no exchange, DEST loaded into RAX, ZF clear */
    dest_val = 77;
    __asm__ volatile (
        "movq $42, %%rax\n\t"
        "movq $99, %%rcx\n\t"
        "cmpxchgq %%rcx, %2\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(rax_val), "=r"(flags), "+m"(dest_val)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(dest_val == 77, "cmpxchgq not equal: dest unchanged at 77");
    TEST_ASSERT(rax_val == 77, "cmpxchgq not equal: rax loaded with dest value 77, got %lu", rax_val);
    TEST_ASSERT(!(flags & ZF_FLAG), "cmpxchgq not equal: ZF should be clear");
}

static void test_cmpxchg_32bit(void) {
    uint32_t dest_val = 100;
    uint32_t eax_val;
    uint64_t flags;

    __asm__ volatile (
        "movl $100, %%eax\n\t"
        "movl $200, %%ecx\n\t"
        "cmpxchgl %%ecx, %2\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(eax_val), "=r"(flags), "+m"(dest_val)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(dest_val == 200, "cmpxchgl equal: dest updated to 200");
    TEST_ASSERT(flags & ZF_FLAG, "cmpxchgl equal: ZF set");
}

static void test_cmpxchg_8bit(void) {
    uint8_t dest_val = 0xAA;
    uint8_t al_val;
    uint64_t flags;

    __asm__ volatile (
        "movb $0xBB, %%al\n\t"
        "movb $0xCC, %%cl\n\t"
        "cmpxchgb %%cl, %2\n\t"
        "movb %%al, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(al_val), "=r"(flags), "+m"(dest_val)
        :
        : "rax", "rcx", "cc"
    );
    /* AL(0xBB) != dest(0xAA) => no exchange, AL = dest */
    TEST_ASSERT(dest_val == 0xAA, "cmpxchgb not equal: dest unchanged");
    TEST_ASSERT(al_val == 0xAA, "cmpxchgb not equal: al = dest = 0xAA, got 0x%x", al_val);
    TEST_ASSERT(!(flags & ZF_FLAG), "cmpxchgb not equal: ZF clear");
    assert_cmpxchg_flags(flags, 0, 1, 0, 0, 0, 0,
                         "cmpxchgb 0xbb-0xaa failure");
}

static void test_cmpxchg_16bit_overflow(void) {
    uint16_t dest = INT16_MAX, ax_value;
    uint64_t flags;
    __asm__ volatile (
        "movw $0x8000, %%ax\n\t"
        "movw $0x1234, %%cx\n\t"
        "cmpxchgw %%cx, %2\n\t"
        "pushfq\n\tpopq %1\n\t"
        "movw %%ax, %0"
        : "=m"(ax_value), "=r"(flags), "+m"(dest)
        : : "rax", "rcx", "cc", "memory");
    TEST_ASSERT(dest == INT16_MAX && ax_value == INT16_MAX,
                "cmpxchgw failure keeps destination and loads AX");
    assert_cmpxchg_flags(flags, 0, 0, 1, 0, 0, 1,
                         "cmpxchgw INT16_MIN-INT16_MAX");
}

static void test_cmpxchg_32bit_failure_zero_extension(void) {
    uint32_t dest = UINT32_C(0x89abcdef);
    uint64_t rax_value, flags;
    __asm__ volatile (
        "movabsq $0xffffffff00000000, %%rax\n\t"
        "movl $0x12345678, %%ecx\n\t"
        "cmpxchgl %%ecx, %2\n\t"
        "pushfq\n\tpopq %1\n\t"
        "movq %%rax, %0"
        : "=r"(rax_value), "=r"(flags), "+m"(dest)
        : : "rax", "rcx", "cc", "memory");
    TEST_ASSERT(dest == UINT32_C(0x89abcdef),
                "cmpxchgl failure leaves destination unchanged");
    TEST_ASSERT(rax_value == UINT64_C(0x0000000089abcdef),
                "cmpxchgl failure loads EAX and zero-extends RAX: %#" PRIx64,
                rax_value);
    assert_cmpxchg_flags(flags, 1, 1, 1, 0, 0, 0,
                         "cmpxchgl 0-0x89abcdef");
}

static void test_cmpxchg_register_and_lock_forms(void) {
    uint64_t accumulator, destination, flags;
    __asm__ volatile (
        "movq $3, %%rax\n\t"
        "movq $5, %%rdx\n\t"
        "movq $9, %%rcx\n\t"
        "cmpxchgq %%rcx, %%rdx\n\t"
        "pushfq\n\tpopq %2\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(accumulator), "=r"(destination), "=r"(flags)
        : : "rax", "rcx", "rdx", "cc");
    TEST_ASSERT(accumulator == 5 && destination == 5,
                "cmpxchgq register failure loads RAX and preserves destination");
    assert_cmpxchg_flags(flags, 1, 0, 1, 0, 1, 0,
                         "cmpxchgq register 3-5");

    uint64_t memory = UINT64_MAX;
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "xorq %%rcx, %%rcx\n\t"
        "lock cmpxchgq %%rcx, %2\n\t"
        "pushfq\n\tpopq %1\n\t"
        "movq %%rax, %0"
        : "=r"(accumulator), "=r"(flags), "+m"(memory)
        : : "rax", "rcx", "cc", "memory");
    TEST_ASSERT(memory == 0 && accumulator == UINT64_MAX,
                "lock cmpxchgq equal path atomically stores source");
    assert_cmpxchg_flags(flags, 0, 1, 0, 1, 0, 0,
                         "lock cmpxchgq equal UINT64_MAX");
}

int main(void) {
    TEST_START("CMPXCHG instruction");
    test_cmpxchg_equal();
    test_cmpxchg_not_equal();
    test_cmpxchg_32bit();
    test_cmpxchg_8bit();
    test_cmpxchg_16bit_overflow();
    test_cmpxchg_32bit_failure_zero_extension();
    test_cmpxchg_register_and_lock_forms();
    TEST_END();
}
