#include "../common.h"

#define CHECK_CMOV(NAME, SET_FLAGS, INSN, EXPECT) do {                         \
    uint64_t dst = 0x1111, src = 0x8877665544332211, before, after;            \
    __asm__ volatile (SET_FLAGS "\n\tpushfq\n\tpopq %1\n\t" INSN            \
                      " %3, %0\n\tpushfq\n\tpopq %2"                        \
                      : "+r"(dst), "=&r"(before), "=&r"(after)              \
                      : "m"(src) : "rax", "cc");                            \
    TEST_ASSERT(dst == (EXPECT), NAME " result: %#" PRIx64, dst);             \
    TEST_ASSERT((before & 0x8d5) == (after & 0x8d5), NAME " preserves flags");\
} while (0)

static void test_missing_cmov_conditions(void) {
    CHECK_CMOV("cmovo true",  "movb $0x7f, %%al\n\taddb $1, %%al", "cmovoq", UINT64_C(0x8877665544332211));
    CHECK_CMOV("cmovno false", "movb $0x7f, %%al\n\taddb $1, %%al", "cmovnoq", UINT64_C(0x1111));
    CHECK_CMOV("cmovae true", "clc", "cmovaeq", UINT64_C(0x8877665544332211));
    CHECK_CMOV("cmovbe true", "movl $1, %%eax\n\tcmpl $1, %%eax", "cmovbeq", UINT64_C(0x8877665544332211));
    CHECK_CMOV("cmovp true", "xorl %%eax, %%eax", "cmovpq", UINT64_C(0x8877665544332211));
    CHECK_CMOV("cmovnp false", "xorl %%eax, %%eax", "cmovnpq", UINT64_C(0x1111));
}

#define CHECK_SET(NAME, SET_FLAGS, INSN, EXPECT) do {                          \
    uint8_t byte = 0xaa; uint64_t before, after;                               \
    __asm__ volatile (SET_FLAGS "\n\tpushfq\n\tpopq %1\n\t" INSN           \
                      " %0\n\tpushfq\n\tpopq %2"                            \
                      : "+qm"(byte), "=&r"(before), "=&r"(after)             \
                      : : "rax", "cc");                                      \
    TEST_ASSERT(byte == (EXPECT), NAME " result: %u", byte);                  \
    TEST_ASSERT((before & 0x8d5) == (after & 0x8d5), NAME " preserves flags");\
} while (0)

static void test_missing_set_conditions(void) {
    CHECK_SET("setae", "clc", "setae", 1);
    CHECK_SET("setbe", "movl $1, %%eax\n\tcmpl $2, %%eax", "setbe", 1);
    CHECK_SET("setp", "xorl %%eax, %%eax", "setp", 1);
    CHECK_SET("setnp", "xorl %%eax, %%eax", "setnp", 0);
    CHECK_SET("setge", "movl $2, %%eax\n\tcmpl $1, %%eax", "setge", 1);
    CHECK_SET("setle", "movl $-1, %%eax\n\tcmpl $1, %%eax", "setle", 1);
}

static void test_parity_branches(void) {
    uint64_t jp_taken, jnp_taken;
    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"
        "movq $0, %0\n\tjp 1f\n\tjmp 2f\n\t1: movq $1, %0\n\t2:"
        : "=&r"(jp_taken) : : "rax", "cc");
    __asm__ volatile (
        "movl $1, %%eax\n\ttestl %%eax, %%eax\n\t"
        "movq $0, %0\n\tjnp 1f\n\tjmp 2f\n\t1: movq $1, %0\n\t2:"
        : "=&r"(jnp_taken) : : "rax", "cc");
    TEST_ASSERT(jp_taken == 1, "jp branches when PF=1");
    TEST_ASSERT(jnp_taken == 1, "jnp branches when PF=0");
}

#define CHECK_BRANCH(NAME, SET_FLAGS, INSN) do {                               \
    uint64_t taken;                                                            \
    __asm__ volatile (SET_FLAGS "\n\tmovq $0, %0\n\t" INSN                  \
                      " 1f\n\tjmp 2f\n\t1: movq $1, %0\n\t2:"             \
                      : "=&r"(taken) : : "rax", "cc");                       \
    TEST_ASSERT(taken == 1, NAME " branches for its true condition");         \
} while (0)

static void test_remaining_direct_branches(void) {
    CHECK_BRANCH("jo", "movb $0x7f, %%al\n\taddb $1, %%al", "jo");
    CHECK_BRANCH("jb", "stc", "jb");
    CHECK_BRANCH("ja", "movl $2, %%eax\n\tcmpl $1, %%eax", "ja");
    CHECK_BRANCH("jl", "movl $-1, %%eax\n\tcmpl $1, %%eax", "jl");
    CHECK_BRANCH("jg", "movl $2, %%eax\n\tcmpl $1, %%eax", "jg");
}

int main(void) {
    TEST_START("missing CMOVcc/SETcc/Jcc conditions");
    test_missing_cmov_conditions();
    test_missing_set_conditions();
    test_parity_branches();
    test_remaining_direct_branches();
    TEST_END();
}
