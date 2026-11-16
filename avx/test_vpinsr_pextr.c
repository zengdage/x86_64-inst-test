/* TARGETS: VPINSRB VPINSRW VPINSRD VPINSRQ VPEXTRB VPEXTRW VPEXTRD VPEXTRQ */
#include "../common.h"

static void assert_upper_zero(const ymm_t *out, const char *name) {
    TEST_ASSERT(out->u64[2] == 0 && out->u64[3] == 0,
                "%s VEX.128 clears upper YMM", name);
}

static void assert_insert_b(const ymm_t *out, const xmm_t *base, int index,
                            uint8_t value, const char *form) {
    for (int i = 0; i < 16; i++) {
        uint8_t expected = i == index ? value : base->u8[i];
        TEST_ASSERT(out->u8[i] == expected, "VPINSRB %s lane %d", form, i);
    }
    assert_upper_zero(out, "VPINSRB");
}

static void assert_insert_w(const ymm_t *out, const xmm_t *base, int index,
                            uint16_t value, const char *form) {
    for (int i = 0; i < 8; i++) {
        uint16_t expected = i == index ? value : base->u16[i];
        TEST_ASSERT(out->u16[i] == expected, "VPINSRW %s lane %d", form, i);
    }
    assert_upper_zero(out, "VPINSRW");
}

static void assert_insert_d(const ymm_t *out, const xmm_t *base, int index,
                            uint32_t value, const char *form) {
    for (int i = 0; i < 4; i++) {
        uint32_t expected = i == index ? value : base->u32[i];
        TEST_ASSERT(out->u32[i] == expected, "VPINSRD %s lane %d", form, i);
    }
    assert_upper_zero(out, "VPINSRD");
}

static void assert_insert_q(const ymm_t *out, const xmm_t *base, int index,
                            uint64_t value, const char *form) {
    for (int i = 0; i < 2; i++) {
        uint64_t expected = i == index ? value : base->u64[i];
        TEST_ASSERT(out->u64[i] == expected, "VPINSRQ %s lane %d", form, i);
    }
    assert_upper_zero(out, "VPINSRQ");
}

static void test_vpinsr(void) {
    const xmm_t base = { .u32 = {
        UINT32_C(0x03020100), UINT32_C(0x07060504),
        UINT32_C(0x0b0a0908), UINT32_C(0x0f0e0d0c)
    } };
    const ymm_t poison = { .u64 = {
        UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX
    } };
    ymm_t out;

    const uint32_t rb = UINT32_C(0xdeadbe80);
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vpinsrb $0, %3, %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(base), "m"(poison), "r"(rb)
                     : "xmm0", "ymm1");
    assert_insert_b(&out, &base, 0, (uint8_t)rb, "register first");
    const uint8_t mb = UINT8_C(0xf5);
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vpinsrb $0xff, %3, %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(base), "m"(poison), "m"(mb)
                     : "xmm0", "ymm1");
    assert_insert_b(&out, &base, 15, mb, "memory high immediate");

    const uint32_t rw = UINT32_C(0xdead8001);
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vpinsrw $0, %3, %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(base), "m"(poison), "r"(rw)
                     : "xmm0", "ymm1");
    assert_insert_w(&out, &base, 0, (uint16_t)rw, "register first");
    const uint16_t mw = UINT16_C(0xfedc);
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vpinsrw $0xff, %3, %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(base), "m"(poison), "m"(mw)
                     : "xmm0", "ymm1");
    assert_insert_w(&out, &base, 7, mw, "memory high immediate");

    const uint32_t rd = UINT32_C(0x80000001);
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vpinsrd $0, %3, %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(base), "m"(poison), "r"(rd)
                     : "xmm0", "ymm1");
    assert_insert_d(&out, &base, 0, rd, "register first");
    const uint32_t md = UINT32_C(0xfedcba98);
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vpinsrd $0xff, %3, %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(base), "m"(poison), "m"(md)
                     : "xmm0", "ymm1");
    assert_insert_d(&out, &base, 3, md, "memory high immediate");

    const uint64_t rq = UINT64_C(0x8000000000000001);
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vpinsrq $0, %3, %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(base), "m"(poison), "r"(rq)
                     : "xmm0", "ymm1");
    assert_insert_q(&out, &base, 0, rq, "register first");
    const uint64_t mq = UINT64_C(0xfedcba9876543210);
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vpinsrq $0xff, %3, %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(base), "m"(poison), "m"(mq)
                     : "xmm0", "ymm1");
    assert_insert_q(&out, &base, 1, mq, "memory high immediate");
}

static void test_vpextr_register(void) {
    const xmm_t src = { .u32 = {
        UINT32_C(0x03020100), UINT32_C(0x07060504),
        UINT32_C(0x0b0a0908), UINT32_C(0x8f0e8d0c)
    } };
    uint64_t b = UINT64_MAX, w = UINT64_MAX, d = UINT64_MAX, q = UINT64_MAX;

    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvpextrb $0xff, %%xmm0, %k0"
                     : "+r"(b) : "m"(src) : "xmm0");
    TEST_ASSERT(b == src.u8[15], "VPEXTRB high immediate and GPR zero extension");
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvpextrw $0xff, %%xmm0, %k0"
                     : "+r"(w) : "m"(src) : "xmm0");
    TEST_ASSERT(w == src.u16[7], "VPEXTRW high immediate and GPR zero extension");
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvpextrd $0xff, %%xmm0, %k0"
                     : "+r"(d) : "m"(src) : "xmm0");
    TEST_ASSERT(d == src.u32[3], "VPEXTRD high immediate and GPR zero extension");
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvpextrq $0xff, %%xmm0, %0"
                     : "+r"(q) : "m"(src) : "xmm0");
    TEST_ASSERT(q == src.u64[1], "VPEXTRQ high immediate");
}

static void test_vpextr_memory(void) {
    const xmm_t src = { .u64 = {
        UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210)
    } };
    struct { uint8_t before, value, after; } b = {0xa5, 0xcc, 0x5a};
    struct { uint16_t before, value, after; } w = {0xa55a, 0xcccc, 0x5aa5};
    struct { uint32_t before, value, after; } d = {
        UINT32_C(0xa55aa55a), UINT32_C(0xcccccccc), UINT32_C(0x5aa55aa5)
    };
    struct { uint64_t before, value, after; } q = {
        UINT64_C(0xa55aa55aa55aa55a), UINT64_C(0xcccccccccccccccc),
        UINT64_C(0x5aa55aa55aa55aa5)
    };

    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvpextrb $0, %%xmm0, %0"
                     : "=m"(b.value) : "m"(src) : "xmm0", "memory");
    TEST_ASSERT(b.value == src.u8[0], "VPEXTRB memory value");
    TEST_ASSERT(b.before == 0xa5 && b.after == 0x5a,
                "VPEXTRB memory writes exactly one byte");

    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvpextrw $0, %%xmm0, %0"
                     : "=m"(w.value) : "m"(src) : "xmm0", "memory");
    TEST_ASSERT(w.value == src.u16[0], "VPEXTRW memory value");
    TEST_ASSERT(w.before == 0xa55a && w.after == 0x5aa5,
                "VPEXTRW memory writes exactly one word");

    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvpextrd $0, %%xmm0, %0"
                     : "=m"(d.value) : "m"(src) : "xmm0", "memory");
    TEST_ASSERT(d.value == src.u32[0], "VPEXTRD memory value");
    TEST_ASSERT(d.before == UINT32_C(0xa55aa55a) &&
                d.after == UINT32_C(0x5aa55aa5),
                "VPEXTRD memory writes exactly one dword");

    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvpextrq $0, %%xmm0, %0"
                     : "=m"(q.value) : "m"(src) : "xmm0", "memory");
    TEST_ASSERT(q.value == src.u64[0], "VPEXTRQ memory value");
    TEST_ASSERT(q.before == UINT64_C(0xa55aa55aa55aa55a) &&
                q.after == UINT64_C(0x5aa55aa55aa55aa5),
                "VPEXTRQ memory writes exactly one qword");
}

int main(void) {
    TEST_START("AVX VEX.128 packed insert/extract instructions");
    test_vpinsr();
    test_vpextr_register();
    test_vpextr_memory();
    __asm__ volatile("vzeroupper");
    TEST_END();
}
