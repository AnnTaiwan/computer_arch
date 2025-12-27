#include <stdint.h>
#include "newlib.h"
#define CODE_OPT_VER 1
extern uint64_t get_cycles(void);
extern uint64_t get_instret(void);


/* ============= Test QR code Declaration ============= */
extern int generate_qrcode(void);
extern int generate_qrcode_opt_v1(void);
extern int generate_qrcode_opt_v2(void);
/* ============= Test Suite ============= */
static void test_generate_qrcode(void)
{
    TEST_LOGGER("Generate_qrcode...\n");
#if CODE_OPT_VER == 0
    int ret = generate_qrcode();
#elif CODE_OPT_VER == 1
    int ret = generate_qrcode_opt_v1();
#elif CODE_OPT_VER == 2
    int ret = generate_qrcode_opt_v2();
#endif
    if(ret == 0)
    {

        TEST_LOGGER("Exit Successfully.\n");
    }
    else
    {
        char exit_msg[25] = "Exit with error code ";
        sprintf(exit_msg + str_len(exit_msg), "%d.\n", ret); // add '\0' automatically
        printstr(exit_msg, str_len(exit_msg));
    }
}
int main(void)
{
    uint64_t start_cycles, end_cycles, cycles_elapsed;
    uint64_t start_instret, end_instret, instret_elapsed;
    TEST_LOGGER("\n=== QR code Tests ===\n\n");
#if CODE_OPT_VER == 0
    TEST_LOGGER("Test 0: QR code (Original reference C code)\n");
#elif CODE_OPT_VER == 1
    TEST_LOGGER("Test 1: QR code (Optimize code v1, use risc-v assembly to implement _rs_mul)\n");
#elif CODE_OPT_VER == 2
    TEST_LOGGER("Test 2: QR code (Optimize code v1, use risc-v assembly to implement _rs_mul, and exclude unnecessary mul_loop)\n");
#endif
    start_cycles = get_cycles();
    start_instret = get_instret();

    test_generate_qrcode();

    end_cycles = get_cycles();
    end_instret = get_instret();
    cycles_elapsed = end_cycles - start_cycles;
    instret_elapsed = end_instret - start_instret;

    TEST_LOGGER("  Cycles: ");
    print_dec((unsigned long) cycles_elapsed);
    TEST_LOGGER("  Instructions: ");
    print_dec((unsigned long) instret_elapsed);
    TEST_LOGGER("\n");

    TEST_LOGGER("\n=== All Tests Completed ===\n");

    return 0;
}
