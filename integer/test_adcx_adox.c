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

    /* ADOX: basic, uses OF */
    {
        uint64_t dst = 1, src = 2, of = 0;
        __asm__ volatile(
            "clc\n\t"  /* clear OF via clc doesn't work; use add to clear OF */
            "xorq %%rax, %%rax\n\t"  /* clears OF */
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

    /* 128-bit addition chain: ADCX preserves OF, ADOX preserves CF */
    {
        uint64_t lo1 = UINT64_MAX, hi1 = 0;
        uint64_t lo2 = 1,         hi2 = 0;
        uint64_t rlo, rhi;
        /* result should be 2^64: lo=0, hi=1 */
        __asm__ volatile(
            "xorq %%rax, %%rax\n\t"   /* clear OF */
            "clc\n\t"                  /* clear CF */
            "adcx %2, %0\n\t"
            "adcx %3, %1\n\t"
            : "=r"(rlo), "=r"(rhi)
            : "r"(lo2), "r"(hi2), "0"(lo1), "1"(hi1)
            : "rax");
        TEST_ASSERT(rlo == 0 && rhi == 1, "128-bit adcx chain: lo=%lu hi=%lu", rlo, rhi);
    }

    /* ADCX preserves OF: set OF, run ADCX, OF should still be set */
    {
        uint64_t dst = 1, src = 1;
        uint64_t of_after = 0;
        __asm__ volatile(
            /* set OF by signed overflow: INT64_MAX + 1 */
            "movq $0x7fffffffffffffff, %%rax\n\t"
            "addq $1, %%rax\n\t"   /* sets OF */
            "clc\n\t"
            "adcx %2, %0\n\t"      /* ADCX must not touch OF */
            "seto %b1"
            : "+r"(dst), "+r"(of_after)
            : "r"(src)
            : "rax");
        TEST_ASSERT(of_after == 1, "adcx preserves OF: of=%lu", of_after);
    }

    /* ADOX preserves CF: set CF, run ADOX, CF should still be set */
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

    TEST_END();
}
