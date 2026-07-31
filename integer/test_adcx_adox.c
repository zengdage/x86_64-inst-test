#include "../common.h"

static int check_adx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(7), "c"(0));
    return (ebx >> 19) & 1;
}

int main(void) {
    TEST_START("ADCX/ADOX");

    if (!check_adx()) {
        printf("ADX not supported, skipping\n");
        return 0;
    }

    /* Note on flag setup: every test below assembles CF/OF setup *inside*
     * the same __asm__ block as the ADCX/ADOX under test.  Splitting
     * "clc" or "xorq %%rax,%%rax" into a separate asm volatile is not
     * enough — the compiler is free to emit flag-touching code between
     * two asm volatiles, and the test would observe a clobbered flag. */

    /* basic ADCX: no carry in, no carry out */
    {
        uint64_t dst = 1, src = 2, cf = 0;
        __asm__ volatile(
            "clc\n\t"
            "adcx %2, %0\n\t"
            "setc %b1"
            : "+r"(dst), "+r"(cf)
            : "r"(src));
        TEST_ASSERT(dst == 3 && cf == 0, "adcx basic: dst=%lu cf=%lu", dst, cf);
    }

    /* ADCX with carry in */
    {
        uint64_t dst = 1, src = 2, cf = 0;
        __asm__ volatile(
            "stc\n\t"
            "adcx %2, %0\n\t"
            "setc %b1"
            : "+r"(dst), "+r"(cf)
            : "r"(src));
        TEST_ASSERT(dst == 4 && cf == 0, "adcx carry-in: dst=%lu cf=%lu", dst, cf);
    }

    /* ADCX carry out */
    {
        uint64_t dst = UINT64_MAX, src = 1, cf = 0;
        __asm__ volatile(
            "clc\n\t"
            "adcx %2, %0\n\t"
            "setc %b1"
            : "+r"(dst), "+r"(cf)
            : "r"(src));
        TEST_ASSERT(dst == 0 && cf == 1, "adcx carry-out: dst=%lu cf=%lu", dst, cf);
    }

    /* ADOX: basic, uses OF.  Clear OF inside the same block via xor. */
    {
        uint64_t dst = 1, src = 2, of = 0;
        __asm__ volatile(
            "xorq %%rax, %%rax\n\t"   /* clears OF */
            "adox %2, %0\n\t"
            "seto %b1"
            : "+r"(dst), "+r"(of)
            : "r"(src)
            : "rax");
        TEST_ASSERT(dst == 3 && of == 0, "adox basic: dst=%lu of=%lu", dst, of);
    }

    /* ADOX overflow out */
    {
        uint64_t dst = UINT64_MAX, src = 1, of = 0;
        __asm__ volatile(
            "xorq %%rax, %%rax\n\t"
            "adox %2, %0\n\t"
            "seto %b1"
            : "+r"(dst), "+r"(of)
            : "r"(src)
            : "rax");
        TEST_ASSERT(dst == 0 && of == 1, "adox overflow-out: dst=%lu of=%lu", dst, of);
    }

    /* 128-bit addition chain via ADCX: lo+lo+CF feeds hi+hi+CF. */
    {
        uint64_t rlo = UINT64_MAX, rhi = 0;
        uint64_t lo2 = 1, hi2 = 0;
        /* result should be 2^64: lo=0, hi=1 */
        __asm__ volatile(
            "xorq %%rax, %%rax\n\t"   /* clear OF (adox/adox chain not used) */
            "clc\n\t"                  /* clear CF */
            "adcx %2, %0\n\t"
            "adcx %3, %1\n\t"
            : "+r"(rlo), "+r"(rhi)
            : "r"(lo2), "r"(hi2)
            : "rax");
        TEST_ASSERT(rlo == 0 && rhi == 1, "128-bit adcx chain: lo=%lu hi=%lu", rlo, rhi);
    }

    /* ADCX preserves OF: set OF, run ADCX, OF should still be set. */
    {
        uint64_t dst = 1, src = 1;
        uint64_t of_after = 0;
        __asm__ volatile(
            /* set OF by signed overflow: INT64_MAX + 1 */
            "movq $0x7fffffffffffffff, %%rax\n\t"
            "addq $1, %%rax\n\t"
            "clc\n\t"
            "adcx %2, %0\n\t"      /* ADCX must not touch OF */
            "seto %b1"
            : "+r"(dst), "+r"(of_after)
            : "r"(src)
            : "rax");
        TEST_ASSERT(of_after == 1, "adcx preserves OF: of=%lu", of_after);
    }

    /* ADOX preserves CF: set CF, run ADOX, CF should still be set. */
    {
        uint64_t dst = 1, src = 1;
        uint64_t cf_after = 0;
        __asm__ volatile(
            "xorq %%rax, %%rax\n\t"  /* clear OF */
            "stc\n\t"                 /* set CF */
            "adox %2, %0\n\t"         /* ADOX must not touch CF */
            "setc %b1"
            : "+r"(dst), "+r"(cf_after)
            : "r"(src)
            : "rax");
        TEST_ASSERT(cf_after == 1, "adox preserves CF: cf=%lu", cf_after);
    }

    /* ===== ADCX boundary conditions ===== */

    /* ADCX: 0 + 0, CF=0 → 0, CF=0 (zero boundary) */
    {
        uint64_t dst = 0, src = 0, cf = 0;
        __asm__ volatile(
            "clc\n\t"
            "adcx %2, %0\n\t"
            "setc %b1"
            : "+r"(dst), "+r"(cf)
            : "r"(src));
        TEST_ASSERT(dst == 0 && cf == 0, "adcx 0+0 CF=0: dst=%lu cf=%lu", dst, cf);
    }

    /* ADCX: 0 + 0, CF=1 → 1, CF=0 (carry-in only, no carry-out) */
    {
        uint64_t dst = 0, src = 0, cf = 0;
        __asm__ volatile(
            "stc\n\t"
            "adcx %2, %0\n\t"
            "setc %b1"
            : "+r"(dst), "+r"(cf)
            : "r"(src));
        TEST_ASSERT(dst == 1 && cf == 0, "adcx 0+0 CF=1: dst=%lu cf=%lu", dst, cf);
    }

    /* ADCX: UINT64_MAX + UINT64_MAX, CF=0 → UINT64_MAX-1, CF=1 (max+max) */
    {
        uint64_t dst = UINT64_MAX, src = UINT64_MAX, cf = 0;
        __asm__ volatile(
            "clc\n\t"
            "adcx %2, %0\n\t"
            "setc %b1"
            : "+r"(dst), "+r"(cf)
            : "r"(src));
        TEST_ASSERT(dst == UINT64_MAX - 1 && cf == 1,
                    "adcx MAX+MAX: dst=%lu cf=%lu", dst, cf);
    }

    /* ADCX: UINT64_MAX + 0, CF=1 → 0, CF=1 (both carry-in and carry-out) */
    {
        uint64_t dst = UINT64_MAX, src = 0, cf = 0;
        __asm__ volatile(
            "stc\n\t"
            "adcx %2, %0\n\t"
            "setc %b1"
            : "+r"(dst), "+r"(cf)
            : "r"(src));
        TEST_ASSERT(dst == 0 && cf == 1, "adcx MAX+0+CF: dst=%lu cf=%lu", dst, cf);
    }

    /* ADCX: sign-bit-only + sign-bit-only, CF=0 → 0, CF=1 (carry at the
     *  high bit only, no propagation through the lower 63 bits). */
    {
        uint64_t dst = 0x8000000000000000ULL;
        uint64_t src = 0x8000000000000000ULL;
        uint64_t cf = 0;
        __asm__ volatile(
            "clc\n\t"
            "adcx %2, %0\n\t"
            "setc %b1"
            : "+r"(dst), "+r"(cf)
            : "r"(src));
        TEST_ASSERT(dst == 0 && cf == 1,
                    "adcx high+high: dst=%lu cf=%lu", dst, cf);
    }

    /* ADCX 128-bit boundary: UINT128_MAX + 1 → 0, CF=1 (both halves wrap) */
    {
        uint64_t rlo = UINT64_MAX, rhi = UINT64_MAX;
        uint64_t src_lo = 1, src_hi = 0;
        uint64_t cf = 0;
        /* =&r forces the compiler to keep the output registers separate from
         * the input registers, and "0" matches the output operand 0 (=rlo)
         * to the input that provides rlo's initial value.  Without the
         * early-clobber, GCC may share a register between rlo and src_lo. */
        __asm__ volatile(
            "xorq %%rax, %%rax\n\t"   /* clear OF */
            "clc\n\t"
            "adcx %3, %0\n\t"        /* src_lo, rlo */
            "adcx %4, %1\n\t"        /* src_hi, rhi */
            "setc %b2"
            : "=&r"(rlo), "=&r"(rhi), "+r"(cf)
            : "r"(src_lo), "r"(src_hi), "0"(UINT64_MAX), "1"(UINT64_MAX)
            : "rax");
        TEST_ASSERT(rlo == 0 && rhi == 0 && cf == 1,
                    "adcx UINT128_MAX+1: rlo=%lu rhi=%lu cf=%lu",
                    rlo, rhi, cf);
    }

    /* ADCX preserves ZF: pre-set ZF inside the same asm block. */
    {
        uint64_t dst = 0, src = 1;  /* 0 + 1 → dst=1, ZF must remain 1 */
        uint64_t zf_after = 0;
        __asm__ volatile(
            /* pre-set ZF=1 by computing 1 + (-1) */
            "movq $1, %%rax\n\t"
            "addq $-1, %%rax\n\t"
            "clc\n\t"
            "adcx %2, %0\n\t"
            "setz %b1"
            : "+r"(dst), "+r"(zf_after)
            : "r"(src)
            : "rax");
        TEST_ASSERT(zf_after == 1, "adcx preserves ZF: zf=%lu", zf_after);
    }

    /* ===== ADOX boundary conditions ===== */

    /* ADOX: 0 + 0, OF=0 → 0, OF=0 (zero boundary) */
    {
        uint64_t dst = 0, src = 0, of = 0;
        __asm__ volatile(
            "xorq %%rax, %%rax\n\t"  /* clear OF */
            "adox %2, %0\n\t"
            "seto %b1"
            : "+r"(dst), "+r"(of)
            : "r"(src)
            : "rax");
        TEST_ASSERT(dst == 0 && of == 0, "adox 0+0 OF=0: dst=%lu of=%lu", dst, of);
    }

    /* ADOX: 0 + 0, OF=1 → 1, OF=0 (OF used as carry-in) */
    {
        uint64_t dst = 0, src = 0, of = 0;
        __asm__ volatile(
            /* set OF via signed overflow: INT64_MAX + 1 */
            "movq $0x7fffffffffffffff, %%rax\n\t"
            "addq $1, %%rax\n\t"
            "adox %2, %0\n\t"
            "seto %b1"
            : "+r"(dst), "+r"(of)
            : "r"(src)
            : "rax");
        TEST_ASSERT(dst == 1 && of == 0, "adox 0+0 OF=1: dst=%lu of=%lu", dst, of);
    }

    /* ADOX: INT64_MAX + INT64_MAX → 0xFFFE..FFFE, OF=0 (no unsigned
     * carry out — the sum fits in 64 bits despite signed overflow.)
     * NOTE: src and dst share the same bit pattern, so we must force
     * separate registers with =&r — otherwise GCC will fold them into
     * the same register and the adox degenerates into 2*x+OF.  We also
     * need a "0" matching input to make sure dst's *initial* value is
     * actually loaded (with =r alone GCC will reuse whatever happens to
     * be in the picked register from a prior test.) */
    {
        uint64_t dst = 0x7fffffffffffffffULL;
        uint64_t src = 0x7fffffffffffffffULL;
        uint64_t of = 0;
        __asm__ volatile(
            "xorq %%rax, %%rax\n\t"
            "adox %2, %0\n\t"
            "seto %b1"
            : "=&r"(dst), "+r"(of)
            : "r"(src), "0"(0x7fffffffffffffffULL)
            : "rax", "cc");
        TEST_ASSERT(dst == 0xfffffffffffffffeULL && of == 0,
                    "adox INT64_MAX+INT64_MAX: dst=0x%lx of=%lu", dst, of);
    }

    /* ADOX: INT64_MIN + INT64_MIN → 0, OF=1 (signed-underflow boundary;
     * the two most-negative values still set OF.)  Same anti-aliasing
     * precaution as above. */
    {
        uint64_t dst = 0x8000000000000000ULL;
        uint64_t src = 0x8000000000000000ULL;
        uint64_t of = 0;
        __asm__ volatile(
            "xorq %%rax, %%rax\n\t"
            "adox %2, %0\n\t"
            "seto %b1"
            : "=&r"(dst), "+r"(of)
            : "r"(src), "0"(0x8000000000000000ULL)
            : "rax");
        TEST_ASSERT(dst == 0 && of == 1,
                    "adox INT64_MIN+INT64_MIN: dst=%lu of=%lu", dst, of);
    }

    /* ADOX: INT64_MAX + INT64_MIN → UINT64_MAX (-1), OF=0 (extremes with
     * opposite sign cancel; no overflow even though both are extreme.) */
    {
        uint64_t dst = 0x7fffffffffffffffULL;
        uint64_t src = 0x8000000000000000ULL;
        uint64_t of = 0;
        __asm__ volatile(
            "xorq %%rax, %%rax\n\t"
            "adox %2, %0\n\t"
            "seto %b1"
            : "+r"(dst), "+r"(of)
            : "r"(src)
            : "rax");
        TEST_ASSERT(dst == 0xffffffffffffffffULL && of == 0,
                    "adox INT64_MAX+INT64_MIN: dst=0x%lx of=%lu", dst, of);
    }

    /* ADOX preserves ZF: pre-set ZF=1, ADOX must not clear it. */
    {
        uint64_t dst = 0, src = 1;  /* 0 + 1 → dst=1, ZF must remain 1 */
        uint64_t zf_after = 0;
        __asm__ volatile(
            "movq $1, %%rax\n\t"
            "addq $-1, %%rax\n\t"   /* set ZF=1 */
            "adox %2, %0\n\t"
            "setz %b1"
            : "+r"(dst), "+r"(zf_after)
            : "r"(src)
            : "rax");
        TEST_ASSERT(zf_after == 1, "adox preserves ZF: zf=%lu", zf_after);
    }

    /* ===== Mixed ADCX + ADOX chain =====
     *
     * Both flags carry independently because ADCX only modifies CF and
     * ADOX only modifies OF.  Layout: ADCX on the lo 64 bits, ADOX on
     * the hi 64 bits.  This is the carry-pattern big-integer schoolbook
     * multiplication uses to defer propagation.
     *
     * Operands: a = UINT128_MAX (lo=hi=UINT64_MAX), b = 1 (lo=1, hi=0).
     *   ADCX path: rlo = 0, CF=1.
     *   ADOX path: rhi = UINT64_MAX, OF=0  (the lo CF does NOT feed the
     *              hi ADOX, by design — that's the whole point of two flags.)
     */
    {
        uint64_t rlo = UINT64_MAX, rhi = UINT64_MAX;
        uint64_t add_lo = 1, add_hi = 0;
        uint64_t cf = 0, of = 0;
        __asm__ volatile(
            "clc\n\t"                 /* CF = 0 (feeds the ADCX) */
            "xorq %%rax, %%rax\n\t"   /* OF = 0 (feeds the ADOX) */
            "adcx %4, %0\n\t"         /* rlo += add_lo + CF */
            "adox %5, %1\n\t"         /* rhi += add_hi + OF (already 0) */
            "setc %b2\n\t"
            "seto %b3"
            : "+r"(rlo), "+r"(rhi), "+r"(cf), "+r"(of)
            : "r"(add_lo), "r"(add_hi)
            : "rax");
        TEST_ASSERT(rlo == 0 && rhi == UINT64_MAX && cf == 1 && of == 0,
                    "mixed 128-bit add: rlo=%lu rhi=0x%lx cf=%lu of=%lu",
                    rlo, rhi, cf, of);
    }

    TEST_END();
}
