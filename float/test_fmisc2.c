#include "../common.h"
#include <math.h>

int main(void) {
    TEST_START("x87 misc2: FCLEX/FNCLEX/FICOM/FICOMP/FSINCOS/FUCOM/FWAIT/FYL2XP1/FLDENV/FRSTOR/FSTSW AX");

    /* FNCLEX: clear FPU exceptions without checking for pending unmasked exceptions */
    {
        uint16_t sw = 0;
        __asm__ volatile(
            "fnclex\n\t"
            "fnstsw %0"
            : "=m"(sw));
        /* exception bits [5:0] and ES(7) should be clear */
        TEST_ASSERT((sw & 0x80BF) == 0, "fnclex: sw=0x%x", sw);
    }

    /* FCLEX: same but checks for pending unmasked exceptions first */
    {
        uint16_t sw = 0;
        __asm__ volatile(
            "fclex\n\t"
            "fnstsw %0"
            : "=m"(sw));
        TEST_ASSERT((sw & 0x80BF) == 0, "fclex: sw=0x%x", sw);
    }

    /* FWAIT/WAIT: synchronize FPU */
    {
        double val = 1.0;
        __asm__ volatile(
            "fldl %0\n\t"
            "fwait\n\t"
            "fstp %%st(0)"
            : : "m"(val));
        TEST_ASSERT(1, "fwait: no fault");
    }

    /* FICOM: compare ST(0) with 16-bit integer memory */
    {
        double st0 = 3.0;
        int16_t mem16 = 2;
        uint16_t sw = 0;
        /* ST(0) > mem => C3=0, C2=0, C0=0 */
        __asm__ volatile(
            "fldl %1\n\t"
            "ficoms %2\n\t"
            "fnstsw %0\n\t"
            "fstp %%st(0)"
            : "=m"(sw)
            : "m"(st0), "m"(mem16));
        uint8_t c3 = (sw >> 14) & 1, c2 = (sw >> 10) & 1, c0 = (sw >> 8) & 1;
        TEST_ASSERT(c3 == 0 && c2 == 0 && c0 == 0, "ficom st0>mem16: c3=%u c2=%u c0=%u", c3, c2, c0);
    }

    /* FICOM: ST(0) < mem16 => C0=1 */
    {
        double st0 = 1.0;
        int16_t mem16 = 5;
        uint16_t sw = 0;
        __asm__ volatile(
            "fldl %1\n\t"
            "ficoms %2\n\t"
            "fnstsw %0\n\t"
            "fstp %%st(0)"
            : "=m"(sw)
            : "m"(st0), "m"(mem16));
        uint8_t c0 = (sw >> 8) & 1;
        TEST_ASSERT(c0 == 1, "ficom st0<mem16: c0=%u", c0);
    }

    /* FICOM: ST(0) == mem16 => C3=1, C0=0 */
    {
        double st0 = 4.0;
        int16_t mem16 = 4;
        uint16_t sw = 0;
        __asm__ volatile(
            "fldl %1\n\t"
            "ficoms %2\n\t"
            "fnstsw %0\n\t"
            "fstp %%st(0)"
            : "=m"(sw)
            : "m"(st0), "m"(mem16));
        uint8_t c3 = (sw >> 14) & 1, c0 = (sw >> 8) & 1;
        TEST_ASSERT(c3 == 1 && c0 == 0, "ficom st0==mem16: c3=%u c0=%u", c3, c0);
    }

    /* FICOMP: compare and pop, 32-bit integer memory */
    {
        double st0 = 7.0;
        int32_t mem32 = 7;
        uint16_t sw = 0;
        __asm__ volatile(
            "fldl %1\n\t"
            "ficompl %2\n\t"
            "fnstsw %0"
            : "=m"(sw)
            : "m"(st0), "m"(mem32));
        uint8_t c3 = (sw >> 14) & 1, c0 = (sw >> 8) & 1;
        TEST_ASSERT(c3 == 1 && c0 == 0, "ficomp st0==mem32: c3=%u c0=%u", c3, c0);
    }

    /* FICOMP with NaN: sets C3=C2=C0=1 (unordered) */
    {
        double st0 = NAN;
        int32_t mem32 = 7;
        uint16_t sw = 0;
        __asm__ volatile(
            "fldl %1\n\t"
            "ficompl %2\n\t"
            "fnstsw %0"
            : "=m"(sw)
            : "m"(st0), "m"(mem32));
        uint8_t c3 = (sw >> 14) & 1, c2 = (sw >> 10) & 1, c0 = (sw >> 8) & 1;
        TEST_ASSERT(c3 == 1 && c2 == 1 && c0 == 1, "ficomp with NaN (unordered): c3=%u c2=%u c0=%u", c3, c2, c0);
    }

    /* FSINCOS: compute sin and cos simultaneously */
    {
        double angle = 0.0; /* sin(0)=0, cos(0)=1 */
        double sin_val, cos_val;
        __asm__ volatile(
            "fldl %2\n\t"
            "fsincos\n\t"       /* ST(0)=cos, ST(1)=sin */
            "fstpl %0\n\t"      /* pop cos */
            "fstpl %1"          /* pop sin */
            : "=m"(cos_val), "=m"(sin_val)
            : "m"(angle));
        TEST_ASSERT(fabs(sin_val - 0.0) < 1e-12, "fsincos sin(0)=%g", sin_val);
        TEST_ASSERT(fabs(cos_val - 1.0) < 1e-12, "fsincos cos(0)=%g", cos_val);
    }

    {
        double angle = M_PI / 2.0;
        double sin_val, cos_val;
        __asm__ volatile(
            "fldl %2\n\t"
            "fsincos\n\t"
            "fstpl %0\n\t"
            "fstpl %1"
            : "=m"(cos_val), "=m"(sin_val)
            : "m"(angle));
        TEST_ASSERT(fabs(sin_val - 1.0) < 1e-12, "fsincos sin(pi/2)=%g", sin_val);
        TEST_ASSERT(fabs(cos_val - 0.0) < 1e-12, "fsincos cos(pi/2)=%g", cos_val);
    }

    /* FUCOM: unordered compare ST(0) with ST(1), sets C3/C2/C0 */
    {
        double a = 3.0, b = 2.0;
        uint16_t sw = 0;
        __asm__ volatile(
            "fldl %1\n\t"   /* ST(0) = b */
            "fldl %2\n\t"   /* ST(0) = a, ST(1) = b */
            "fucom\n\t"     /* compare ST(0) with ST(1): a > b */
            "fnstsw %0\n\t"
            "fstp %%st(0)\n\t"
            "fstp %%st(0)"
            : "=m"(sw)
            : "m"(b), "m"(a));
        uint8_t c3 = (sw >> 14) & 1, c2 = (sw >> 10) & 1, c0 = (sw >> 8) & 1;
        TEST_ASSERT(c3 == 0 && c2 == 0 && c0 == 0, "fucom a>b: c3=%u c2=%u c0=%u", c3, c2, c0);
    }

    /* FUCOM with NaN: sets C3=C2=C0=1 (unordered) */
    {
        double a = NAN, b = 2.0;
        uint16_t sw = 0;
        __asm__ volatile(
            "fldl %1\n\t"
            "fldl %2\n\t"
            "fucom\n\t"
            "fnstsw %0\n\t"
            "fstp %%st(0)\n\t"
            "fstp %%st(0)"
            : "=m"(sw)
            : "m"(b), "m"(a));
        uint8_t c3 = (sw >> 14) & 1, c2 = (sw >> 10) & 1, c0 = (sw >> 8) & 1;
        TEST_ASSERT(c3 == 1 && c2 == 1 && c0 == 1, "fucom with NaN (unordered): c3=%u c2=%u c0=%u", c3, c2, c0);
    }

    /* FUCOMP: compare and pop */
    {
        double a = 1.0, b = 1.0;
        uint16_t sw = 0;
        __asm__ volatile(
            "fldl %1\n\t"
            "fldl %2\n\t"
            "fucomp\n\t"    /* compare ST(0)==ST(1), pop */
            "fnstsw %0\n\t"
            "fstp %%st(0)"
            : "=m"(sw)
            : "m"(b), "m"(a));
        uint8_t c3 = (sw >> 14) & 1, c0 = (sw >> 8) & 1;
        TEST_ASSERT(c3 == 1 && c0 == 0, "fucomp a==b: c3=%u c0=%u", c3, c0);
    }

    /* FUCOMPP: compare and pop twice */
    {
        double a = 1.0, b = 2.0;
        uint16_t sw = 0;
        __asm__ volatile(
            "fldl %1\n\t"   /* ST(0)=b */
            "fldl %2\n\t"   /* ST(0)=a, ST(1)=b; a < b => C0=1 */
            "fucompp\n\t"
            "fnstsw %0"
            : "=m"(sw)
            : "m"(b), "m"(a));
        uint8_t c0 = (sw >> 8) & 1;
        TEST_ASSERT(c0 == 1, "fucompp a<b: c0=%u", c0);
    }

    /* FYL2XP1: ST(1) * log2(ST(0)+1), result in ST(1), pop ST(0) */
    /* y=1, x=1: 1 * log2(2) = 1.0 */
    {
        double x = 1.0, y = 1.0, result;
        __asm__ volatile(
            "fldl %1\n\t"   /* ST(0) = y */
            "fldl %2\n\t"   /* ST(0) = x, ST(1) = y */
            "fyl2xp1\n\t"   /* ST(0) = y * log2(x+1) */
            "fstpl %0"
            : "=m"(result)
            : "m"(y), "m"(x));
        TEST_ASSERT(fabs(result - 1.0) < 1e-12, "fyl2xp1 1*log2(2)=%g", result);
    }

    /* FSTSW AX: store FPU status word to AX register */
    {
        double val = 0.0;
        uint16_t ax_val = 0;
        __asm__ volatile(
            "fldl %1\n\t"
            "fnclex\n\t"
            "fstsw %%ax\n\t"
            "movw %%ax, %0\n\t"
            "fstp %%st(0)"
            : "=m"(ax_val)
            : "m"(val)
            : "ax");
        /* just verify it ran and exception bits are clear */
        TEST_ASSERT((ax_val & 0x003F) == 0, "fstsw ax: exception bits=0x%x", ax_val & 0x3F);
    }

    /* FNSTENV / FLDENV: save and restore FPU environment */
    {
        uint8_t env1[28], env2[28];
        memset(env1, 0, sizeof(env1));
        memset(env2, 0xFF, sizeof(env2));
        double val = 1.0;
        __asm__ volatile(
            "fldl %2\n\t"
            "fnclex\n\t"
            "fnstenv %0\n\t"    /* save env to env1 */
            "fldenv %0\n\t"     /* restore from env1 */
            "fnstenv %1\n\t"    /* save again to env2 */
            "fstp %%st(0)"
            : "=m"(env1), "=m"(env2) : "m"(val));
        /* control word (bytes 0-1) should match */
        uint16_t cw1, cw2;
        memcpy(&cw1, env1, 2);
        memcpy(&cw2, env2, 2);
        TEST_ASSERT(cw1 == cw2, "fldenv/fnstenv: cw1=0x%x cw2=0x%x", cw1, cw2);
    }

    /* FNSAVE / FRSTOR: save and restore full FPU state */
    {
        uint8_t state[108] __attribute__((aligned(4)));
        memset(state, 0, sizeof(state));
        double val = 2.5, result = 0.0;
        __asm__ volatile(
            "fldl %1\n\t"
            "fnsave %0\n\t"     /* saves state and reinitializes FPU */
            "frstor %0\n\t"     /* restores state; ST(0) = 2.5 again */
            "fstpl %2"
            : "=m"(state)
            : "m"(val), "m"(result));
        /* read result back */
        __asm__ volatile("" : "=m"(result));
        double r;
        memcpy(&r, &result, sizeof(r));
        /* fnsave pops the stack, frstor restores it */
        TEST_ASSERT(fabs(r - 2.5) < 1e-12, "fnsave/frstor: result=%g", r);
    }

    TEST_END();
}
