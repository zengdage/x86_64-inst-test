/*
 * test_insertps_extractps.c - Test SSE4.1 INSERTPS/EXTRACTPS instructions
 *
 * INSERTPS : Insert a 32-bit value from xmm/mem into a dword position of an xmm
 *            destination, with a zero-mask. imm8[7:6]=src dword select,
 *            imm8[5:4]=dst dword select, imm8[3:0]=zero mask (Z3..Z0).
 * EXTRACTPS: Extract a 32-bit dword from an xmm to a GPR/mem. imm8[1:0]=src dword select.
 *
 * Compile: gcc -o test_insertps_extractps simd/test_insertps_extractps.c -O2 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_insertps_from_xmm(void) {
    xmm_t dst = { .u32 = {0xAA, 0xBB, 0xCC, 0xDD} };
    xmm_t src = { .u32 = {0x111, 0x222, 0x333, 0x444} };
    xmm_t r;
    /* imm8 = 0b00_01_0000 = 0x10: src lane 0, dst lane 1, no zeroing */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "insertps $0x10, %%xmm1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(r) : "m"(dst), "m"(src) : "xmm0", "xmm1"
    );
    TEST_ASSERT(r.u32[0] == 0xAAu,  "insertps xmm [0] preserved: got 0x%x", r.u32[0]);
    TEST_ASSERT(r.u32[1] == 0x111u, "insertps xmm [1]=src0: got 0x%x", r.u32[1]);
    TEST_ASSERT(r.u32[2] == 0xCCu,  "insertps xmm [2] preserved: got 0x%x", r.u32[2]);
    TEST_ASSERT(r.u32[3] == 0xDDu,  "insertps xmm [3] preserved: got 0x%x", r.u32[3]);
}

static void test_insertps_zeromask(void) {
    xmm_t dst = { .u32 = {0xAA, 0xBB, 0xCC, 0xDD} };
    xmm_t src = { .u32 = {0x111, 0x222, 0x333, 0x444} };
    xmm_t r;
    /* imm8 = 0b00_01_1001 = 0x19: src lane 0, dst lane 1, zero dwords 0 and 3 */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "insertps $0x19, %%xmm1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(r) : "m"(dst), "m"(src) : "xmm0", "xmm1"
    );
    TEST_ASSERT(r.u32[0] == 0u,     "insertps zeromask [0] zeroed: got 0x%x", r.u32[0]);
    TEST_ASSERT(r.u32[1] == 0x111u, "insertps zeromask [1]=src0: got 0x%x",    r.u32[1]);
    TEST_ASSERT(r.u32[2] == 0xCCu,  "insertps zeromask [2] preserved: got 0x%x", r.u32[2]);
    TEST_ASSERT(r.u32[3] == 0u,     "insertps zeromask [3] zeroed: got 0x%x", r.u32[3]);
}

static void test_insertps_from_mem(void) {
    xmm_t dst = { .u32 = {0xAA, 0xBB, 0xCC, 0xDD} };
    uint32_t mem = 0x7777;
    xmm_t r;
    /* For mem src, src dword select is ignored (mem is a single dword).
       imm8 = 0b00_10_0000 = 0x20: dst lane 2 */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "insertps $0x20, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(r) : "m"(dst), "m"(mem) : "xmm0"
    );
    TEST_ASSERT(r.u32[0] == 0xAAu,   "insertps mem [0] preserved: got 0x%x", r.u32[0]);
    TEST_ASSERT(r.u32[1] == 0xBBu,   "insertps mem [1] preserved: got 0x%x", r.u32[1]);
    TEST_ASSERT(r.u32[2] == 0x7777u, "insertps mem [2]=mem: got 0x%x",       r.u32[2]);
    TEST_ASSERT(r.u32[3] == 0xDDu,   "insertps mem [3] preserved: got 0x%x", r.u32[3]);
}

static void test_insertps_src_lane_select(void) {
    xmm_t dst = { .u32 = {0, 0, 0, 0} };
    xmm_t src = { .u32 = {0x111, 0x222, 0x333, 0x444} };
    xmm_t r;
    /* imm8 = 0b10_00_0000 = 0x80: src lane 2, dst lane 0 */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "insertps $0x80, %%xmm1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(r) : "m"(dst), "m"(src) : "xmm0", "xmm1"
    );
    TEST_ASSERT(r.u32[0] == 0x333u, "insertps src lane 2 -> dst 0: got 0x%x", r.u32[0]);
    TEST_ASSERT(r.u32[1] == 0u,     "insertps [1] zeroed (dst was 0): got 0x%x", r.u32[1]);
}

static void test_extractps_to_reg(void) {
    xmm_t src = { .u32 = {0xDEADBEEF, 0x12345678, 0xCAFEBABE, 0xFEEDFACE} };
    uint32_t out = 0;
    /* imm8[1:0]=2 -> extract dword 2 */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "extractps $2, %%xmm0, %0\n\t"
        : "=r"(out) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(out == 0xCAFEBABEu, "extractps $2 to reg: expected 0xCAFEBABE, got 0x%x", out);
}

static void test_extractps_to_mem(void) {
    xmm_t src = { .u32 = {0xDEADBEEF, 0x12345678, 0xCAFEBABE, 0xFEEDFACE} };
    uint32_t mem = 0;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "extractps $1, %%xmm0, %0\n\t"
        : "=m"(mem) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(mem == 0x12345678u, "extractps $1 to mem: expected 0x12345678, got 0x%x", mem);
}

static void test_extractps_lane0(void) {
    xmm_t src = { .u32 = {0xDEADBEEF, 0x12345678, 0xCAFEBABE, 0xFEEDFACE} };
    uint32_t out = 0;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "extractps $0, %%xmm0, %0\n\t"
        : "=r"(out) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(out == 0xDEADBEEFu, "extractps $0 to reg: expected 0xDEADBEEF, got 0x%x", out);
}

static void test_insertps_extractps_immediate_boundaries(void) {
    xmm_t dst = { .u32 = {0x10,0x20,0x30,0x40} };
    xmm_t src = { .u32 = {0x111,0x222,0x333,0x444} };
    xmm_t reg_result, mem_select0, mem_select3;
    uint32_t mem = UINT32_C(0xa5a5a5a5);
    __asm__ volatile (
        "movdqu %3, %%xmm0\n\t" "movdqu %4, %%xmm1\n\t"
        "insertps $0xf0, %%xmm1, %%xmm0\n\t" "movdqu %%xmm0, %0\n\t"
        "movdqu %3, %%xmm0\n\t" "insertps $0x20, %5, %%xmm0\n\t" "movdqu %%xmm0, %1\n\t"
        "movdqu %3, %%xmm0\n\t" "insertps $0xe0, %5, %%xmm0\n\t" "movdqu %%xmm0, %2"
        : "=m"(reg_result), "=m"(mem_select0), "=m"(mem_select3)
        : "m"(dst), "m"(src), "m"(mem) : "xmm0", "xmm1");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(reg_result.u32[i] == (i == 3 ? src.u32[3] : dst.u32[i]),
                    "insertps src3/dst3 lane %d", i);
    TEST_ASSERT(memcmp(&mem_select0, &mem_select3, sizeof(mem_select0)) == 0,
                "insertps memory source ignores imm8 source-select bits");

    uint64_t extracted;
    __asm__ volatile (
        "movq $-1, %%rax\n\t" "movdqu %1, %%xmm0\n\t"
        "extractps $0xff, %%xmm0, %%eax\n\t" "movq %%rax, %0"
        : "=r"(extracted) : "m"(src) : "rax", "xmm0");
    TEST_ASSERT(extracted == src.u32[3],
                "extractps imm=ff selects lane3 and zero-extends RAX");
}

int main(void) {
    TEST_START("INSERTPS/EXTRACTPS");
    test_insertps_from_xmm();
    test_insertps_zeromask();
    test_insertps_from_mem();
    test_insertps_src_lane_select();
    test_extractps_to_reg();
    test_extractps_to_mem();
    test_extractps_lane0();
    test_insertps_extractps_immediate_boundaries();
    TEST_END();
}
