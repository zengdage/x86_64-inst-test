#include "../common.h"

#if !defined(__i386__)
#error "Build this test with -m32"
#endif

__attribute__((naked, noinline))
static void capture_pusha(uint32_t *result) {
    __asm__ volatile (
        "pushal\n\t"
        "movl $0x11111111, %eax\n\tmovl $0x22222222, %ecx\n\t"
        "movl $0x33333333, %edx\n\tmovl $0x44444444, %ebx\n\t"
        "movl $0x55555555, %ebp\n\tmovl $0x66666666, %esi\n\t"
        "movl $0x77777777, %edi\n\tpushal\n\t"
        "movl 68(%esp), %edi\n\tleal 32(%esp), %eax\n\tmovl %eax, 32(%edi)\n\t"
        "movl %esp, %esi\n\tmovl $8, %ecx\n\t"
        "cld\n\trep movsl\n\tpopal\n\tpopal\n\tret");
}

__attribute__((naked, noinline))
static void capture_popa(uint32_t *result) {
    __asm__ volatile (
        "pushal\n\tsubl $32, %esp\n\t"
        "movl $0x77777777, 0(%esp)\n\tmovl $0x66666666, 4(%esp)\n\t"
        "movl $0x55555555, 8(%esp)\n\tmovl $0xdeadbeef, 12(%esp)\n\t"
        "movl $0x44444444, 16(%esp)\n\tmovl $0x33333333, 20(%esp)\n\t"
        "movl $0x22222222, 24(%esp)\n\tmovl $0x11111111, 28(%esp)\n\t"
        "popal\n\tpushal\n\t"
        "movl 68(%esp), %edi\n\tleal 32(%esp), %eax\n\tmovl %eax, 32(%edi)\n\t"
        "movl %esp, %esi\n\tmovl $8, %ecx\n\t"
        "cld\n\trep movsl\n\tpopal\n\tpopal\n\tret");
}

static void check_register_image(const uint32_t *image, const char *name) {
    TEST_ASSERT(image[0] == UINT32_C(0x77777777), "%s EDI", name);
    TEST_ASSERT(image[1] == UINT32_C(0x66666666), "%s ESI", name);
    TEST_ASSERT(image[2] == UINT32_C(0x55555555), "%s EBP", name);
    TEST_ASSERT(image[4] == UINT32_C(0x44444444), "%s EBX", name);
    TEST_ASSERT(image[5] == UINT32_C(0x33333333), "%s EDX", name);
    TEST_ASSERT(image[6] == UINT32_C(0x22222222), "%s ECX", name);
    TEST_ASSERT(image[7] == UINT32_C(0x11111111), "%s EAX", name);
}

int main(void) {
    uint32_t pusha_image[9], popa_image[9];
    TEST_START("PUSHA/POPA (32-bit)");
    capture_pusha(pusha_image);
    check_register_image(pusha_image, "pusha");
    TEST_ASSERT(pusha_image[3] == pusha_image[8],
                "pusha records the exact pre-instruction ESP");
    capture_popa(popa_image);
    check_register_image(popa_image, "popa");
    TEST_ASSERT(popa_image[3] == popa_image[8] &&
                popa_image[3] != UINT32_C(0xdeadbeef),
                "popa skips the saved ESP slot and retains current ESP");
    TEST_END();
}
