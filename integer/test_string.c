/*
 * test_string.c - Test x86-64 string instructions
 *
 * MOVSB/W/D/Q: Move string (copy from RSI to RDI)
 * STOSB/W/D/Q: Store string (store AL/AX/EAX/RAX to RDI)
 * LODSB/W/D/Q: Load string (load from RSI to AL/AX/EAX/RAX)
 * CMPSB/W:     Compare strings
 * SCASB:       Scan string (compare AL with byte at RDI)
 * REP prefix:  Repeat RCX times
 * DF flag:     Direction (0=forward, 1=backward)
 *
 * Compile: gcc -o test_string integer/test_string.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_movsb(void) {
    char src[] = "Hello, World!";
    char dst[16] = {0};

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "leaq %0, %%rdi\n\t"
        "movq $13, %%rcx\n\t"
        "cld\n\t"
        "rep movsb"
        : "=m"(dst)
        : "m"(src)
        : "rcx", "rsi", "rdi", "cc"
    );
    TEST_ASSERT(memcmp(dst, src, 13) == 0, "rep movsb: string copy");
}

static void test_movsw(void) {
    uint16_t src[] = {0x1111, 0x2222, 0x3333, 0x4444};
    uint16_t dst[4] = {0};

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "leaq %0, %%rdi\n\t"
        "movq $4, %%rcx\n\t"
        "cld\n\t"
        "rep movsw"
        : "=m"(dst)
        : "m"(src)
        : "rcx", "rsi", "rdi", "cc"
    );
    TEST_ASSERT(memcmp(dst, src, 8) == 0, "rep movsw: word copy");
}

static void test_movsd(void) {
    uint32_t src[] = {0x11111111, 0x22222222, 0x33333333};
    uint32_t dst[3] = {0};

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "leaq %0, %%rdi\n\t"
        "movq $3, %%rcx\n\t"
        "cld\n\t"
        "rep movsl"
        : "=m"(dst)
        : "m"(src)
        : "rcx", "rsi", "rdi", "cc"
    );
    TEST_ASSERT(memcmp(dst, src, 12) == 0, "rep movsd: dword copy");
}

static void test_movsq(void) {
    uint64_t src[] = {0xAAAABBBBCCCCDDDDUL, 0x1111222233334444UL};
    uint64_t dst[2] = {0};

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "leaq %0, %%rdi\n\t"
        "movq $2, %%rcx\n\t"
        "cld\n\t"
        "rep movsq"
        : "=m"(dst)
        : "m"(src)
        : "rcx", "rsi", "rdi", "cc"
    );
    TEST_ASSERT(dst[0] == src[0] && dst[1] == src[1], "rep movsq: qword copy");
}

static void test_stosb(void) {
    char buf[8] = {0};

    __asm__ volatile (
        "leaq %0, %%rdi\n\t"
        "movb $0x41, %%al\n\t"
        "movq $8, %%rcx\n\t"
        "cld\n\t"
        "rep stosb"
        : "=m"(buf)
        :
        : "rax", "rcx", "rdi", "cc"
    );
    int all_A = 1;
    for (int i = 0; i < 8; i++) {
        if (buf[i] != 'A') all_A = 0;
    }
    TEST_ASSERT(all_A, "rep stosb: filled with 'A'");
}

static void test_stosw(void) {
    uint16_t buf[4] = {0};

    __asm__ volatile (
        "leaq %0, %%rdi\n\t"
        "movw $0xBEEF, %%ax\n\t"
        "movq $4, %%rcx\n\t"
        "cld\n\t"
        "rep stosw"
        : "=m"(buf)
        :
        : "rax", "rcx", "rdi", "cc"
    );
    TEST_ASSERT(buf[0] == 0xBEEF && buf[3] == 0xBEEF, "rep stosw: filled with 0xBEEF");
}

static void test_stosd(void) {
    uint32_t buf[4] = {0};

    __asm__ volatile (
        "leaq %0, %%rdi\n\t"
        "movl $0xDEADBEEF, %%eax\n\t"
        "movq $4, %%rcx\n\t"
        "cld\n\t"
        "rep stosl"
        : "=m"(buf)
        :
        : "rax", "rcx", "rdi", "cc"
    );
    TEST_ASSERT(buf[0] == 0xDEADBEEF && buf[3] == 0xDEADBEEF, "rep stosd");
}

static void test_stosq(void) {
    uint64_t buf[2] = {0};

    __asm__ volatile (
        "leaq %0, %%rdi\n\t"
        "movabsq $0xCAFEBABEDEADBEEF, %%rax\n\t"
        "movq $2, %%rcx\n\t"
        "cld\n\t"
        "rep stosq"
        : "=m"(buf)
        :
        : "rax", "rcx", "rdi", "cc"
    );
    TEST_ASSERT(buf[0] == 0xCAFEBABEDEADBEEFUL && buf[1] == 0xCAFEBABEDEADBEEFUL,
                "rep stosq");
}

static void test_lodsb(void) {
    uint8_t src[] = {0xAA, 0xBB, 0xCC};
    uint8_t result;

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "cld\n\t"
        "lodsb\n\t"
        "movb %%al, %0"
        : "=r"(result)
        : "m"(src)
        : "rax", "rsi", "cc"
    );
    TEST_ASSERT(result == 0xAA, "lodsb: expected 0xAA, got 0x%x", result);
}

static void test_lodsw(void) {
    uint16_t src[] = {0x1234, 0x5678};
    uint16_t result;

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "cld\n\t"
        "lodsw\n\t"
        "movw %%ax, %0"
        : "=r"(result)
        : "m"(src)
        : "rax", "rsi", "cc"
    );
    TEST_ASSERT(result == 0x1234, "lodsw: expected 0x1234");
}

static void test_lodsd(void) {
    uint32_t src[] = {0xDEADBEEF};
    uint32_t result;

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "cld\n\t"
        "lodsl\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        : "m"(src)
        : "rax", "rsi", "cc"
    );
    TEST_ASSERT(result == 0xDEADBEEF, "lodsd: expected 0xDEADBEEF");
}

static void test_lodsq(void) {
    uint64_t src[] = {0x123456789ABCDEF0UL};
    uint64_t result;

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "cld\n\t"
        "lodsq\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        : "m"(src)
        : "rax", "rsi", "cc"
    );
    TEST_ASSERT(result == 0x123456789ABCDEF0UL, "lodsq");
}

static void test_cmpsb(void) {
    char s1[] = "abcdef";
    char s2[] = "abcXef";
    uint64_t flags;

    /* Compare equal prefix */
    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "leaq %2, %%rdi\n\t"
        "movq $3, %%rcx\n\t"
        "cld\n\t"
        "repe cmpsb\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(s1), "m"(s2)
        : "rcx", "rsi", "rdi", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "repe cmpsb: first 3 bytes equal, ZF set");

    /* Compare with mismatch */
    uint64_t count;
    __asm__ volatile (
        "leaq %2, %%rsi\n\t"
        "leaq %3, %%rdi\n\t"
        "movq $6, %%rcx\n\t"
        "cld\n\t"
        "repe cmpsb\n\t"
        "movq %%rcx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(count), "=r"(flags)
        : "m"(s1), "m"(s2)
        : "rcx", "rsi", "rdi", "cc"
    );
    /* Mismatch at index 3, so RCX should be 2 (6-1-3=2) */
    TEST_ASSERT(!(flags & ZF_FLAG), "repe cmpsb with mismatch: ZF clear");
}

static void test_cmpsw(void) {
    uint16_t s1[] = {0x1111, 0x2222, 0x3333};
    uint16_t s2[] = {0x1111, 0x2222, 0x3333};
    uint64_t flags;

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "leaq %2, %%rdi\n\t"
        "movq $3, %%rcx\n\t"
        "cld\n\t"
        "repe cmpsw\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(s1), "m"(s2)
        : "rcx", "rsi", "rdi", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "repe cmpsw: all equal");
}

static void test_scasb(void) {
    char str[] = "Hello";
    uint64_t flags;

    /* Scan for 'l' */
    __asm__ volatile (
        "leaq %1, %%rdi\n\t"
        "movb $'l', %%al\n\t"
        "movq $5, %%rcx\n\t"
        "cld\n\t"
        "repne scasb\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(str)
        : "rax", "rcx", "rdi", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "repne scasb: found 'l'");

    /* Scan for character not in string */
    __asm__ volatile (
        "leaq %1, %%rdi\n\t"
        "movb $'Z', %%al\n\t"
        "movq $5, %%rcx\n\t"
        "cld\n\t"
        "repne scasb\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(str)
        : "rax", "rcx", "rdi", "cc"
    );
    TEST_ASSERT(!(flags & ZF_FLAG), "repne scasb: 'Z' not found");
}

static void test_direction_flag(void) {
    char src[] = "ABCDE";
    char dst[6] = {0};

    /* Copy backward using STD */
    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "addq $4, %%rsi\n\t"     /* point to last byte */
        "leaq %0, %%rdi\n\t"
        "addq $4, %%rdi\n\t"
        "movq $5, %%rcx\n\t"
        "std\n\t"
        "rep movsb\n\t"
        "cld"                     /* restore DF=0 */
        : "=m"(dst)
        : "m"(src)
        : "rcx", "rsi", "rdi", "cc"
    );
    TEST_ASSERT(memcmp(dst, src, 5) == 0, "backward rep movsb with STD");
}

static void test_zero_count_rep(void) {
    uint8_t src[] = {0x11, 0x22};
    uint8_t dst[] = {0xaa, 0xbb};
    uintptr_t src_after, dst_after;
    uint64_t count_after;
    uint64_t flags;

    __asm__ volatile (
        "leaq %3, %%rsi\n\t"
        "leaq %4, %%rdi\n\t"
        "xorq %%rcx, %%rcx\n\t"
        "cld\n\t"
        "rep movsb\n\t"
        "movq %%rsi, %0\n\t"
        "movq %%rdi, %1\n\t"
        "movq %%rcx, %2"
        : "=r"(src_after), "=r"(dst_after), "=r"(count_after), "+m"(src), "+m"(dst)
        :
        : "rcx", "rsi", "rdi", "cc", "memory"
    );
    TEST_ASSERT(dst[0] == 0xaa && dst[1] == 0xbb,
                "rep movsb RCX=0: destination unchanged");
    TEST_ASSERT(src_after == (uintptr_t)src && dst_after == (uintptr_t)dst && count_after == 0,
                "rep movsb RCX=0: pointers and count unchanged");

    /* With RCX=0, REPE CMPSB performs no comparison and preserves flags. */
    __asm__ volatile (
        "cmpq %%rax, %%rax\n\t"      /* ZF=1 */
        "leaq %1, %%rsi\n\t"
        "leaq %2, %%rdi\n\t"
        "xorq %%rcx, %%rcx\n\t"     /* also leaves ZF=1 */
        "repe cmpsb\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(src), "m"(dst)
        : "rax", "rcx", "rsi", "rdi", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "repe cmpsb RCX=0: ZF preserved");
}

int main(void) {
    TEST_START("String instructions");
    test_movsb();
    test_movsw();
    test_movsd();
    test_movsq();
    test_stosb();
    test_stosw();
    test_stosd();
    test_stosq();
    test_lodsb();
    test_lodsw();
    test_lodsd();
    test_lodsq();
    test_cmpsb();
    test_cmpsw();
    test_scasb();
    test_direction_flag();
    test_zero_count_rep();
    TEST_END();
}
