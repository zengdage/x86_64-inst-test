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
}

int main(void) {
    TEST_START("CMPXCHG instruction");
    test_cmpxchg_equal();
    test_cmpxchg_not_equal();
    test_cmpxchg_32bit();
    test_cmpxchg_8bit();
    TEST_END();
}
