/*
 * test_mov.c - Test x86-64 MOV, MOVSX, MOVSXD, MOVZX instructions
 *
 * MOV copies data between registers, memory, and immediates.
 * MOVSX sign-extends the source to fill the destination.
 * MOVZX zero-extends the source to fill the destination.
 * MOVSXD sign-extends a 32-bit value to 64 bits.
 * MOV does not affect any flags.
 *
 * Compile: gcc -o test_mov integer/test_mov.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_mov_reg_reg(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $0xDEADBEEFCAFEBABE, %%rax\n\t"
        "movq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == 0xDEADBEEFCAFEBABEUL, "movq reg,reg");
}

static void test_mov_reg_imm(void) {
    uint64_t r64;
    uint32_t r32;
    uint16_t r16;
    uint8_t r8;

    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(r64)
        :
        : "rax"
    );
    TEST_ASSERT(r64 == 0x123456789ABCDEF0UL, "movq imm64");

    __asm__ volatile (
        "movl $0xDEADBEEF, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(r32)
        :
        : "rax"
    );
    TEST_ASSERT(r32 == 0xDEADBEEF, "movl imm32");

    __asm__ volatile (
        "movw $0x1234, %%ax\n\t"
        "movw %%ax, %0"
        : "=r"(r16)
        :
        : "rax"
    );
    TEST_ASSERT(r16 == 0x1234, "movw imm16");

    __asm__ volatile (
        "movb $0xAB, %%al\n\t"
        "movb %%al, %0"
        : "=r"(r8)
        :
        : "rax"
    );
    TEST_ASSERT(r8 == 0xAB, "movb imm8");
}

static void test_mov_reg_mem(void) {
    uint64_t src = 0x1122334455667788UL;
    uint64_t dst;

    __asm__ volatile (
        "movq %1, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(dst)
        : "m"(src)
        : "rax"
    );
    TEST_ASSERT(dst == src, "movq reg,mem");
}

static void test_mov_mem_reg(void) {
    uint64_t dst = 0;

    __asm__ volatile (
        "movq $0xAABBCCDD, %%rax\n\t"
        "movq %%rax, %0"
        : "=m"(dst)
        :
        : "rax"
    );
    TEST_ASSERT(dst == 0xAABBCCDD, "movq mem,reg");
}

static void test_mov_sib_extended_index(void) {
    /* 4a 89 0c d7    movq %rcx, (%rdi,%r10,8)
     * REX.WX (0x4a) selects an extended index register (r10) in the SIB byte
     * with scale 8, base rdi and no displacement.
     */
    uint64_t buf[4] = {0, 0, 0, 0};

    __asm__ volatile (
        "movq $0xFEEDFACECAFEBEEF, %%rcx\n\t"
        "movq $2, %%r10\n\t"
        "movq %%rcx, (%%rdi,%%r10,8)"
        :
        : "D"(buf)
        : "rcx", "r10", "memory"
    );
    TEST_ASSERT(buf[2] == 0xFEEDFACECAFEBEEFUL,
                "movq %%rcx,(%%rdi,%%r10,8) store: got 0x%" PRIx64, buf[2]);
    TEST_ASSERT(buf[0] == 0 && buf[1] == 0 && buf[3] == 0,
                "movq %%rcx,(%%rdi,%%r10,8) must not touch neighbours");

    /* Load back through the same addressing form */
    uint64_t loaded;
    __asm__ volatile (
        "movq $2, %%r10\n\t"
        "movq (%%rdi,%%r10,8), %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(loaded)
        : "D"(buf)
        : "rcx", "r10"
    );
    TEST_ASSERT(loaded == 0xFEEDFACECAFEBEEFUL,
                "movq (%%rdi,%%r10,8),%%rcx load: got 0x%" PRIx64, loaded);

    /* Index 0 exercises the same encoding with a different offset */
    __asm__ volatile (
        "movq $0x1122334455667788, %%rcx\n\t"
        "xorq %%r10, %%r10\n\t"
        "movq %%rcx, (%%rdi,%%r10,8)"
        :
        : "D"(buf)
        : "rcx", "r10", "memory"
    );
    TEST_ASSERT(buf[0] == 0x1122334455667788UL,
                "movq %%rcx,(%%rdi,%%r10,8) index 0: got 0x%" PRIx64, buf[0]);
}

static void test_mov_32bit_zero_extends(void) {
    uint64_t result;

    /* mov to 32-bit register zero-extends to 64 bits */
    __asm__ volatile (
        "movq $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
        "movl $1, %%eax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 1, "movl zero-extends to 64 bits: got 0x%lx", result);
}

static void test_movsx(void) {
    int64_t result;

    /* MOVSX byte to 32-bit (sign-extend) */
    int32_t r32;
    __asm__ volatile (
        "movb $0x80, %%al\n\t"
        "movsbl %%al, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(r32)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(r32 == -128, "movsbl 0x80 expected -128, got %d", r32);

    /* MOVSX byte to 64-bit */
    __asm__ volatile (
        "movb $0xFF, %%al\n\t"
        "movsbq %%al, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == -1, "movsbq 0xFF expected -1");

    /* MOVSX word to 64-bit */
    __asm__ volatile (
        "movw $0x8000, %%ax\n\t"
        "movswq %%ax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == -32768, "movswq 0x8000 expected -32768");

    /* Positive value should zero-extend */
    __asm__ volatile (
        "movb $0x7F, %%al\n\t"
        "movsbq %%al, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == 127, "movsbq 0x7F expected 127");
}

static void test_movsxd(void) {
    int64_t result;

    /* MOVSXD: sign-extend 32-bit to 64-bit */
    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "movslq %%eax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == (int64_t)(int32_t)0x80000000, "movsxd 0x80000000 expected INT32_MIN sign-extended");

    __asm__ volatile (
        "movl $100, %%eax\n\t"
        "movslq %%eax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == 100, "movsxd 100 expected 100");
}

static void test_movzx(void) {
    uint64_t result;

    /* MOVZX byte to 32-bit */
    uint32_t r32;
    __asm__ volatile (
        "movb $0xFF, %%al\n\t"
        "movzbl %%al, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(r32)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(r32 == 255, "movzbl 0xFF expected 255, got %u", r32);

    /* MOVZX word to 64-bit */
    __asm__ volatile (
        "movw $0xFFFF, %%ax\n\t"
        "movzwq %%ax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == 0xFFFF, "movzwq 0xFFFF expected 65535");

    /* MOVZX byte to 64-bit */
    __asm__ volatile (
        "movb $0x80, %%al\n\t"
        "movzbq %%al, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == 128, "movzbq 0x80 expected 128 (no sign extension)");
}

int main(void) {
    TEST_START("MOV/MOVSX/MOVSXD/MOVZX instructions");
    test_mov_reg_reg();
    test_mov_reg_imm();
    test_mov_reg_mem();
    test_mov_mem_reg();
    test_mov_sib_extended_index();
    test_mov_32bit_zero_extends();
    test_movsx();
    test_movsxd();
    test_movzx();
    TEST_END();
}
