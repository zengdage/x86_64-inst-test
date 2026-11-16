#define _GNU_SOURCE
#include "../common.h"
#include <asm/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

struct far_pointer64 {
    uint64_t offset;
    uint16_t selector;
} __attribute__((packed));

static void test_lfs(void) {
    unsigned long saved_base;
    uint16_t selector;
    TEST_ASSERT(syscall(SYS_arch_prctl, ARCH_GET_FS, &saved_base) == 0,
                "read FS base before LFS");
    __asm__ volatile ("movw %%fs, %0" : "=rm"(selector));
    struct far_pointer64 pointer = {UINT64_C(0x1122334455667788), selector};
    uint64_t loaded = 0;
    long restore_result = -1;
    __asm__ volatile (
        "leaq %[pointer], %%rdi\n\t"
        ".byte 0x48, 0x0f, 0xb4, 0x07\n\t" /* LFS RAX, m16:64 [RDI] */
        "movq %%rax, %[loaded]\n\t"
        "movq %[sys_arch_prctl], %%rax\n\t"
        "movq %[arch_set_fs], %%rdi\n\t"
        "movq %[saved_base], %%rsi\n\t"
        "syscall\n\t"
        "movq %%rax, %[restore_result]"
        : [loaded] "=m"(loaded), [restore_result] "=m"(restore_result)
        : [pointer] "m"(pointer), [saved_base] "r"(saved_base),
          [sys_arch_prctl] "i"((long)SYS_arch_prctl),
          [arch_set_fs] "i"((long)ARCH_SET_FS)
        : "rax", "rdi", "rsi", "rcx", "r11", "cc", "memory");
    TEST_ASSERT(restore_result == 0, "restore FS base immediately after LFS");
    TEST_ASSERT(loaded == pointer.offset, "lfs loads 64-bit offset");
    uint16_t after;
    __asm__ volatile ("movw %%fs, %0" : "=rm"(after));
    TEST_ASSERT(after == selector, "lfs loads the requested FS selector");
}

static void test_lgs(void) {
    unsigned long saved_base;
    uint16_t selector;
    TEST_ASSERT(syscall(SYS_arch_prctl, ARCH_GET_GS, &saved_base) == 0,
                "read GS base before LGS");
    __asm__ volatile ("movw %%gs, %0" : "=rm"(selector));
    struct far_pointer64 pointer = {UINT64_C(0x8877665544332211), selector};
    uint64_t loaded = 0;
    long restore_result = -1;
    __asm__ volatile (
        "leaq %[pointer], %%rdi\n\t"
        ".byte 0x48, 0x0f, 0xb5, 0x07\n\t" /* LGS RAX, m16:64 [RDI] */
        "movq %%rax, %[loaded]\n\t"
        "movq %[sys_arch_prctl], %%rax\n\t"
        "movq %[arch_set_gs], %%rdi\n\t"
        "movq %[saved_base], %%rsi\n\t"
        "syscall\n\t"
        "movq %%rax, %[restore_result]"
        : [loaded] "=m"(loaded), [restore_result] "=m"(restore_result)
        : [pointer] "m"(pointer), [saved_base] "r"(saved_base),
          [sys_arch_prctl] "i"((long)SYS_arch_prctl),
          [arch_set_gs] "i"((long)ARCH_SET_GS)
        : "rax", "rdi", "rsi", "rcx", "r11", "cc", "memory");
    TEST_ASSERT(restore_result == 0, "restore GS base immediately after LGS");
    TEST_ASSERT(loaded == pointer.offset, "lgs loads 64-bit offset");
    uint16_t after;
    __asm__ volatile ("movw %%gs, %0" : "=rm"(after));
    TEST_ASSERT(after == selector, "lgs loads the requested GS selector");
}

__attribute__((naked, noinline))
static uint64_t execute_lss(struct far_pointer64 *pointer) {
    __asm__ volatile (
        "movq %rsp, 0(%rdi)\n\t"
        ".byte 0x48, 0x0f, 0xb2, 0x27\n\t" /* LSS RSP, m16:64 [RDI] */
        "movl $1, %eax\n\t"
        "ret");
}

static void test_lss(void) {
    uint16_t selector, after;
    __asm__ volatile ("movw %%ss, %0" : "=rm"(selector));
    struct far_pointer64 pointer = {0, selector};
    uint64_t completed = execute_lss(&pointer);
    __asm__ volatile ("movw %%ss, %0" : "=rm"(after));
    TEST_ASSERT(completed == 1, "lss resumes on the loaded stack");
    TEST_ASSERT(after == selector, "lss loads the requested SS selector");
}

int main(void) {
    TEST_START("long-mode LFS/LGS/LSS");
    test_lfs();
    test_lgs();
    test_lss();
    TEST_END();
}
