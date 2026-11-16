#include "../common.h"

#if !defined(__i386__)
#error "Build this test with -m32"
#endif

struct far_pointer32 {
    uint32_t offset;
    uint16_t selector;
} __attribute__((packed));

#define CHECK_SEGMENT_LOAD(NAME, INSN, SEGMENT, VALUE) do {                    \
    uint16_t selector_before, selector_after; uint32_t offset;                 \
    __asm__ volatile ("movw %%" SEGMENT ", %0" : "=rm"(selector_before));    \
    struct far_pointer32 pointer = {(VALUE), selector_before};                 \
    __asm__ volatile (INSN " %2, %0\n\tmovw %%" SEGMENT ", %1"              \
                      : "=r"(offset), "=rm"(selector_after)                   \
                      : "m"(pointer) : "memory");                             \
    TEST_ASSERT(offset == (VALUE), NAME " loads 32-bit offset");              \
    TEST_ASSERT(selector_after == selector_before, NAME " loads selector");   \
} while (0)

static void test_data_segment_loads(void) {
    CHECK_SEGMENT_LOAD("lds", "ldsl", "ds", UINT32_C(0x11223344));
    CHECK_SEGMENT_LOAD("les", "lesl", "es", UINT32_C(0x22334455));
    CHECK_SEGMENT_LOAD("lfs", "lfsl", "fs", UINT32_C(0x33445566));
    CHECK_SEGMENT_LOAD("lgs", "lgsl", "gs", UINT32_C(0x44556677));
}

__attribute__((naked, noinline))
static uint32_t execute_lss(struct far_pointer32 *pointer) {
    __asm__ volatile (
        "movl 4(%esp), %eax\n\t"
        "movl %esp, 0(%eax)\n\t"
        "lssl (%eax), %esp\n\t"
        "movl $1, %eax\n\tret");
}

static void test_lss(void) {
    uint16_t ss;
    __asm__ volatile ("movw %%ss, %0" : "=rm"(ss));
    struct far_pointer32 pointer = {0, ss};
    uint32_t completed = execute_lss(&pointer);
    uint16_t after;
    __asm__ volatile ("movw %%ss, %0" : "=rm"(after));
    TEST_ASSERT(completed == 1 && after == ss, "lss loads the current SS:ESP and returns");
}

int main(void) {
    TEST_START("LDS/LES/LFS/LGS/LSS (32-bit)");
    test_data_segment_loads();
    test_lss();
    TEST_END();
}
