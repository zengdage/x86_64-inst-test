/*
 * test_bt.c - Test x86-64 BT/BTS/BTR/BTC instructions
 *
 * BT:  Tests a bit and copies it to CF
 * BTS: Tests a bit, sets it, copies old to CF
 * BTR: Tests a bit, resets it, copies old to CF
 * BTC: Tests a bit, complements it, copies old to CF
 * Affects flags: CF
 *
 * Compile: gcc -o test_bt integer/test_bt.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_bt_reg(void) {
    uint64_t flags;

    /* Test bit 0 of 0x01 => CF=1 */
    __asm__ volatile (
        "movq $0x01, %%rax\n\t"
        "btq $0, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "bt bit 0 of 0x01: CF should be set");

    /* Test bit 1 of 0x01 => CF=0 */
    __asm__ volatile (
        "movq $0x01, %%rax\n\t"
        "btq $1, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "bt bit 1 of 0x01: CF should be clear");

    /* Test bit 63 */
    __asm__ volatile (
        "movq $0x8000000000000000, %%rax\n\t"
        "btq $63, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "bt bit 63: CF should be set");
}

static void test_bts(void) {
    uint64_t result;
    uint64_t flags;

    /* BTS: set bit 4 in 0 */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "btsq $4, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x10, "btsq bit 4: expected 0x10, got 0x%lx", result);
    TEST_ASSERT(!(flags & CF_FLAG), "btsq: old bit was 0, CF should be clear");

    /* BTS on already-set bit */
    __asm__ volatile (
        "movq $0x10, %%rax\n\t"
        "btsq $4, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x10, "btsq already-set: value unchanged");
    TEST_ASSERT(flags & CF_FLAG, "btsq: old bit was 1, CF should be set");
}

static void test_btr(void) {
    uint64_t result;
    uint64_t flags;

    /* BTR: clear bit 0 in 0xFF */
    __asm__ volatile (
        "movq $0xFF, %%rax\n\t"
        "btrq $0, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0xFE, "btrq bit 0: expected 0xFE, got 0x%lx", result);
    TEST_ASSERT(flags & CF_FLAG, "btrq: old bit was 1, CF should be set");

    /* BTR on already-clear bit */
    __asm__ volatile (
        "movq $0xFE, %%rax\n\t"
        "btrq $0, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0xFE, "btrq already-clear: value unchanged");
    TEST_ASSERT(!(flags & CF_FLAG), "btrq: old bit was 0, CF should be clear");
}

static void test_btc(void) {
    uint64_t result;
    uint64_t flags;

    /* BTC: complement bit 3 in 0x00 */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "btcq $3, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 8, "btcq bit 3 of 0: expected 8, got %lu", result);
    TEST_ASSERT(!(flags & CF_FLAG), "btcq: old bit was 0, CF clear");

    /* BTC again: complement back */
    __asm__ volatile (
        "movq $8, %%rax\n\t"
        "btcq $3, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0, "btcq complement back: expected 0");
    TEST_ASSERT(flags & CF_FLAG, "btcq: old bit was 1, CF set");
}

static void test_bt_mem(void) {
    uint64_t flags;
    uint64_t val = 0x8000000000000000UL;

    __asm__ volatile (
        "btq $63, %1\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(val)
        : "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "bt mem bit 63: CF should be set");
}

int main(void) {
    TEST_START("BT/BTS/BTR/BTC instructions");
    test_bt_reg();
    test_bts();
    test_btr();
    test_btc();
    test_bt_mem();
    TEST_END();
}
