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

int main(void) {
    TEST_START("XADD instruction");
    test_xadd_basic();
    test_xadd_zero();
    test_xadd_32bit();
    test_xadd_16bit();
    TEST_END();
}
