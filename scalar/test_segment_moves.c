#define _GNU_SOURCE
#include "../common.h"
#include <asm/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

static void test_push_pop_fs(void) {
    unsigned long base;
    uint16_t selector, after;
    uint64_t pushed;
    TEST_ASSERT(syscall(SYS_arch_prctl, ARCH_GET_FS, &base) == 0,
                "read FS base before PUSH/POP FS");
    __asm__ volatile ("movw %%fs, %0\n\tpush %%fs\n\tpopq %1"
                      : "=rm"(selector), "=r"(pushed) : : "memory");
    TEST_ASSERT((uint16_t)pushed == selector, "PUSH FS stores the current selector");

    uint64_t selector64 = selector;
    long restore_result;
    __asm__ volatile (
        "pushq %[selector]\n\tpop %%fs\n\t"
        "syscall"
        : "=a"(restore_result)
        : "0"((long)SYS_arch_prctl), "D"((long)ARCH_SET_FS), "S"(base),
          [selector] "r"(selector64)
        : "rcx", "r11", "cc", "memory");
    TEST_ASSERT(restore_result == 0, "restore FS base immediately after POP FS");
    __asm__ volatile ("movw %%fs, %0" : "=rm"(after));
    TEST_ASSERT(after == selector, "POP FS loads the selector");
}

static void test_push_pop_gs(void) {
    unsigned long base;
    uint16_t selector, after;
    uint64_t pushed;
    TEST_ASSERT(syscall(SYS_arch_prctl, ARCH_GET_GS, &base) == 0,
                "read GS base before PUSH/POP GS");
    __asm__ volatile ("movw %%gs, %0\n\tpush %%gs\n\tpopq %1"
                      : "=rm"(selector), "=r"(pushed) : : "memory");
    TEST_ASSERT((uint16_t)pushed == selector, "PUSH GS stores the current selector");

    uint64_t selector64 = selector;
    long restore_result;
    __asm__ volatile (
        "pushq %[selector]\n\tpop %%gs\n\t"
        "syscall"
        : "=a"(restore_result)
        : "0"((long)SYS_arch_prctl), "D"((long)ARCH_SET_GS), "S"(base),
          [selector] "r"(selector64)
        : "rcx", "r11", "cc", "memory");
    TEST_ASSERT(restore_result == 0, "restore GS base immediately after POP GS");
    __asm__ volatile ("movw %%gs, %0" : "=rm"(after));
    TEST_ASSERT(after == selector, "POP GS loads the selector");
}

static void test_mov_data_segments(void) {
    uint16_t ds, es, ss, after_ds, after_es, after_ss;
    __asm__ volatile ("movw %%ds, %0\n\tmovw %%es, %1\n\tmovw %%ss, %2"
                      : "=rm"(ds), "=rm"(es), "=rm"(ss));
    __asm__ volatile (
        "movw %[ds], %%ds\n\tmovw %[es], %%es\n\tmovw %[ss], %%ss\n\t"
        "movw %%ds, %[after_ds]\n\tmovw %%es, %[after_es]\n\t"
        "movw %%ss, %[after_ss]"
        : [after_ds] "=rm"(after_ds), [after_es] "=rm"(after_es),
          [after_ss] "=rm"(after_ss)
        : [ds] "rm"(ds), [es] "rm"(es), [ss] "rm"(ss) : "memory");
    TEST_ASSERT(after_ds == ds, "MOV DS loads the current data selector");
    TEST_ASSERT(after_es == es, "MOV ES loads the current data selector");
    TEST_ASSERT(after_ss == ss, "MOV SS loads the current stack selector");
}

static void test_mov_fs_gs(void) {
    unsigned long fs_base, gs_base;
    uint16_t fs, gs, after_fs, after_gs;
    TEST_ASSERT(syscall(SYS_arch_prctl, ARCH_GET_FS, &fs_base) == 0,
                "read FS base before MOV FS");
    TEST_ASSERT(syscall(SYS_arch_prctl, ARCH_GET_GS, &gs_base) == 0,
                "read GS base before MOV GS");
    __asm__ volatile ("movw %%fs, %0\n\tmovw %%gs, %1" : "=rm"(fs), "=rm"(gs));

    long restore_result;
    __asm__ volatile (
        "movw %w[selector], %%fs\n\t"
        "syscall"
        : "=a"(restore_result)
        : "0"((long)SYS_arch_prctl), "D"((long)ARCH_SET_FS), "S"(fs_base),
          [selector] "r"(fs)
        : "rcx", "r11", "cc", "memory");
    TEST_ASSERT(restore_result == 0, "restore FS base immediately after MOV FS");

    __asm__ volatile (
        "movw %w[selector], %%gs\n\t"
        "syscall"
        : "=a"(restore_result)
        : "0"((long)SYS_arch_prctl), "D"((long)ARCH_SET_GS), "S"(gs_base),
          [selector] "r"(gs)
        : "rcx", "r11", "cc", "memory");
    TEST_ASSERT(restore_result == 0, "restore GS base immediately after MOV GS");
    __asm__ volatile ("movw %%fs, %0\n\tmovw %%gs, %1"
                      : "=rm"(after_fs), "=rm"(after_gs));
    TEST_ASSERT(after_fs == fs, "MOV FS loads the selector");
    TEST_ASSERT(after_gs == gs, "MOV GS loads the selector");
}

int main(void) {
    TEST_START("PUSH/POP FS/GS and MOV segment registers");
    test_push_pop_fs();
    test_push_pop_gs();
    test_mov_data_segments();
    test_mov_fs_gs();
    TEST_END();
}
