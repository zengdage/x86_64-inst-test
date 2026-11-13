/* AVX-512 opmask widths, modular arithmetic, shifts, and flag-setting tests. */
#include "avx512_test.h"

static void test_kmov_widths(void) {
    uint32_t out32;
    uint64_t out64;
    uint32_t in32 = UINT32_C(0x89abcdef);
    uint64_t in64 = UINT64_C(0x0123456789abcdef);
    __asm__ volatile("kmovb %1, %%k1\n\tkmovb %%k1, %0"
                     : "=r"(out32) : "r"(in32) : "k1");
    TEST_ASSERT(out32 == 0xefU, "KMOVB truncates to 8 bits: 0x%x", out32);
    __asm__ volatile("kmovd %1, %%k1\n\tkmovd %%k1, %0"
                     : "=r"(out32) : "r"(in32) : "k1");
    TEST_ASSERT(out32 == in32, "KMOVD round trip");
    __asm__ volatile("kmovq %1, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(out64) : "r"(in64) : "k1");
    TEST_ASSERT(out64 == in64, "KMOVQ round trip");
}

static void test_kadd_widths(void) {
    uint32_t a32 = UINT32_C(0xffffffff), b32 = 2, out32;
    uint64_t a64 = UINT64_MAX, b64 = 2, out64;
    __asm__ volatile(
        "kmovb %1, %%k1\n\tkmovb %2, %%k2\n\tkaddb %%k2, %%k1, %%k3\n\t"
        "kmovb %%k3, %0"
        : "=r"(out32) : "r"(a32), "r"(b32) : "k1", "k2", "k3");
    TEST_ASSERT(out32 == 1U, "KADDB wraps at 8 bits");
    __asm__ volatile(
        "kmovw %1, %%k1\n\tkmovw %2, %%k2\n\tkaddw %%k2, %%k1, %%k3\n\t"
        "kmovw %%k3, %0"
        : "=r"(out32) : "r"(a32), "r"(b32) : "k1", "k2", "k3");
    TEST_ASSERT(out32 == 1U, "KADDW wraps at 16 bits");
    __asm__ volatile(
        "kmovd %1, %%k1\n\tkmovd %2, %%k2\n\tkaddd %%k2, %%k1, %%k3\n\t"
        "kmovd %%k3, %0"
        : "=r"(out32) : "r"(a32), "r"(b32) : "k1", "k2", "k3");
    TEST_ASSERT(out32 == 1U, "KADDD wraps at 32 bits");
    __asm__ volatile(
        "kmovq %1, %%k1\n\tkmovq %2, %%k2\n\tkaddq %%k2, %%k1, %%k3\n\t"
        "kmovq %%k3, %0"
        : "=r"(out64) : "r"(a64), "r"(b64) : "k1", "k2", "k3");
    TEST_ASSERT(out64 == 1U, "KADDQ wraps at 64 bits");

    uint32_t value = 0x5aU;
    __asm__ volatile(
        "kmovb %1, %%k1\n\tkaddb %%k1, %%k1, %%k1\n\tkmovb %%k1, %0"
        : "=r"(out32) : "r"(value) : "k1");
    TEST_ASSERT(out32 == 0xb4U, "KADDB all operands aliased");
}

#define TEST_MASK_LOGIC32(SFX, MOV, MASK) do { \
    uint32_t a = UINT32_C(0x89abcdef) & (MASK); \
    uint32_t b = UINT32_C(0x5aa55aa5) & (MASK); \
    uint32_t andv, andnv, orv, xorv, xnorv, notv; \
    __asm__ volatile( \
        #MOV " %6, %%k1\n\t" #MOV " %7, %%k2\n\t" \
        "kand" #SFX " %%k2, %%k1, %%k3\n\t" \
        "kandn" #SFX " %%k2, %%k1, %%k4\n\t" \
        "kor" #SFX " %%k2, %%k1, %%k5\n\t" \
        "kxor" #SFX " %%k2, %%k1, %%k6\n\t" \
        "kxnor" #SFX " %%k2, %%k1, %%k7\n\t" \
        "knot" #SFX " %%k1, %%k1\n\t" \
        #MOV " %%k3, %0\n\t" #MOV " %%k4, %1\n\t" \
        #MOV " %%k5, %2\n\t" #MOV " %%k6, %3\n\t" \
        #MOV " %%k7, %4\n\t" #MOV " %%k1, %5" \
        : "=r"(andv), "=r"(andnv), "=r"(orv), "=r"(xorv), "=r"(xnorv), "=r"(notv) \
        : "r"(a), "r"(b) : "k1", "k2", "k3", "k4", "k5", "k6", "k7"); \
    TEST_ASSERT(andv == ((a & b) & (MASK)), "KAND" #SFX); \
    TEST_ASSERT(andnv == (((~a) & b) & (MASK)), "KANDN" #SFX); \
    TEST_ASSERT(orv == ((a | b) & (MASK)), "KOR" #SFX); \
    TEST_ASSERT(xorv == ((a ^ b) & (MASK)), "KXOR" #SFX); \
    TEST_ASSERT(xnorv == ((~(a ^ b)) & (MASK)), "KXNOR" #SFX); \
    TEST_ASSERT(notv == ((~a) & (MASK)), "KNOT" #SFX); \
} while (0)

static void test_logic_widths(void) {
    TEST_MASK_LOGIC32(b, kmovb, UINT32_C(0xff));
    TEST_MASK_LOGIC32(w, kmovw, UINT32_C(0xffff));
    TEST_MASK_LOGIC32(d, kmovd, UINT32_MAX);

    uint64_t a = UINT64_C(0x89abcdef01234567), b = UINT64_C(0x5aa55aa5a55aa55a);
    uint64_t andv, andnv, orv, xorv, xnorv, notv;
    __asm__ volatile(
        "kmovq %6, %%k1\n\tkmovq %7, %%k2\n\t"
        "kandq %%k2, %%k1, %%k3\n\tkandnq %%k2, %%k1, %%k4\n\t"
        "korq %%k2, %%k1, %%k5\n\tkxorq %%k2, %%k1, %%k6\n\t"
        "kxnorq %%k2, %%k1, %%k7\n\tknotq %%k1, %%k1\n\t"
        "kmovq %%k3, %0\n\tkmovq %%k4, %1\n\tkmovq %%k5, %2\n\t"
        "kmovq %%k6, %3\n\tkmovq %%k7, %4\n\tkmovq %%k1, %5"
        : "=r"(andv), "=r"(andnv), "=r"(orv), "=r"(xorv), "=r"(xnorv), "=r"(notv)
        : "r"(a), "r"(b) : "k1", "k2", "k3", "k4", "k5", "k6", "k7");
    TEST_ASSERT(andv == (a & b), "KANDQ");
    TEST_ASSERT(andnv == ((~a) & b), "KANDNQ");
    TEST_ASSERT(orv == (a | b), "KORQ");
    TEST_ASSERT(xorv == (a ^ b), "KXORQ");
    TEST_ASSERT(xnorv == ~(a ^ b), "KXNORQ");
    TEST_ASSERT(notv == ~a, "KNOTQ");
}

#undef TEST_MASK_LOGIC32

static void test_shift_widths(void) {
    uint32_t in = UINT32_C(0x80000081), byte_left, byte_right, dword_left, dword_right;
    __asm__ volatile(
        "kmovd %4, %%k1\n\t"
        "kshiftlb $7, %%k1, %%k2\n\tkshiftrb $7, %%k1, %%k3\n\t"
        "kshiftld $31, %%k1, %%k4\n\tkshiftrd $31, %%k1, %%k5\n\t"
        "kmovb %%k2, %0\n\tkmovb %%k3, %1\n\t"
        "kmovd %%k4, %2\n\tkmovd %%k5, %3"
        : "=r"(byte_left), "=r"(byte_right), "=r"(dword_left), "=r"(dword_right)
        : "r"(in) : "k1", "k2", "k3", "k4", "k5");
    TEST_ASSERT(byte_left == 0x80U && byte_right == 1U, "KSHIFTB width-1 boundaries");
    TEST_ASSERT(dword_left == UINT32_C(0x80000000) && dword_right == 1U,
                "KSHIFTD width-1 boundaries");
    uint64_t qin = UINT64_C(0x8000000000000001), qleft, qright;
    __asm__ volatile("kmovq %2, %%k1\n\tkshiftlq $63, %%k1, %%k2\n\t"
                     "kshiftrq $63, %%k1, %%k3\n\tkmovq %%k2, %0\n\tkmovq %%k3, %1"
                     : "=r"(qleft), "=r"(qright) : "r"(qin) : "k1", "k2", "k3");
    TEST_ASSERT(qleft == UINT64_C(0x8000000000000000), "KSHIFTLQ width-1");
    TEST_ASSERT(qright == 1, "KSHIFTRQ width-1");
}

static void test_unpack(void) {
    uint32_t a = UINT32_C(0x89abcdef), b = UINT32_C(0x76543210), bw, wd;
    uint64_t dq;
    __asm__ volatile("kmovw %3, %%k1\n\tkmovw %4, %%k2\n\t"
                     "kunpckbw %%k2, %%k1, %%k3\n\tkunpckwd %%k2, %%k1, %%k4\n\t"
                     "kmovd %3, %%k5\n\tkmovd %4, %%k6\n\tkunpckdq %%k6, %%k5, %%k7\n\t"
                     "kmovw %%k3, %0\n\tkmovd %%k4, %1\n\tkmovq %%k7, %2"
                     : "=r"(bw), "=r"(wd), "=r"(dq) : "r"(a), "r"(b)
                     : "k1", "k2", "k3", "k4", "k5", "k6", "k7");
    TEST_ASSERT(bw == (((a & 0xffU) << 8) | (b & 0xffU)), "KUNPCKBW");
    TEST_ASSERT(wd == (((a & 0xffffU) << 16) | (b & 0xffffU)), "KUNPCKWD");
    TEST_ASSERT(dq == (((uint64_t)a << 32) | b), "KUNPCKDQ");
}

static void run_ktest(uint32_t value, unsigned *zf, unsigned *cf) {
    unsigned char z, c;
    __asm__ volatile(
        "kmovw %2, %%k1\n\tktestw %%k1, %%k1\n\tsetz %0\n\tsetc %1"
        : "=qm"(z), "=qm"(c) : "r"(value) : "k1", "cc");
    *zf = z;
    *cf = c;
}

static void run_kortest(uint32_t a, uint32_t b, unsigned *zf, unsigned *cf) {
    unsigned char z, c;
    __asm__ volatile(
        "kmovw %2, %%k1\n\tkmovw %3, %%k2\n\t"
        "kortestw %%k2, %%k1\n\tsetz %0\n\tsetc %1"
        : "=qm"(z), "=qm"(c) : "r"(a), "r"(b) : "k1", "k2", "cc");
    *zf = z;
    *cf = c;
}

static void test_flags(void) {
    unsigned zf, cf;
    run_ktest(0, &zf, &cf);
    TEST_ASSERT(zf == 1 && cf == 1, "KTESTW zero sets ZF and CF");
    run_ktest(0x8001U, &zf, &cf);
    TEST_ASSERT(zf == 0 && cf == 1, "KTESTW self nonzero clears ZF and sets CF");

    run_kortest(0, 0, &zf, &cf);
    TEST_ASSERT(zf == 1 && cf == 0, "KORTESTW zero union");
    run_kortest(0x00ffU, 0xff00U, &zf, &cf);
    TEST_ASSERT(zf == 0 && cf == 1, "KORTESTW all-ones union");
    run_kortest(0x0001U, 0x8000U, &zf, &cf);
    TEST_ASSERT(zf == 0 && cf == 0, "KORTESTW mixed union");

#define TEST_FLAG_WIDTH(SFX, MOV, TYPE, ALL) do { \
    unsigned char z, c, oz, oc; TYPE value = (TYPE)1; \
    __asm__ volatile(#MOV " %4, %%k1\n\t" \
                     "ktest" #SFX " %%k1, %%k1\n\tsetz %0\n\tsetc %1\n\t" \
                     #MOV " %5, %%k2\n\t" \
                     "kortest" #SFX " %%k2, %%k1\n\tsetz %2\n\tsetc %3" \
                     : "=&qm"(z), "=&qm"(c), "=&qm"(oz), "=&qm"(oc) \
                     : "r"(value), "r"((TYPE)((ALL) ^ 1)) : "k1", "k2", "cc"); \
    TEST_ASSERT(z == 0 && c == 1, "KTEST" #SFX); \
    TEST_ASSERT(oz == 0 && oc == 1, "KORTEST" #SFX); \
} while (0)
    TEST_FLAG_WIDTH(b, kmovb, uint32_t, UINT8_MAX);
    TEST_FLAG_WIDTH(d, kmovd, uint32_t, UINT32_MAX);
    TEST_FLAG_WIDTH(q, kmovq, uint64_t, UINT64_MAX);
#undef TEST_FLAG_WIDTH
}

int main(void) {
    TEST_START("Extended AVX-512 opmask instructions");
    if (!CHECK_AVX512F()) return 0;
    test_kmov_widths();
    test_kadd_widths();
    test_logic_widths();
    test_shift_widths();
    test_unpack();
    test_flags();
    TEST_END();
}
