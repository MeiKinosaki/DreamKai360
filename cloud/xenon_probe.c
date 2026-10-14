/* Xenon cross-build smoke test that links and exercises the DreamKai360 core.
 * This proves compilation/linking only; it is not a complete Dreamcast emulator
 * and does not produce a default.xex package.
 */
#include "dreamkai360.h"
#include <stdint.h>

static uint8_t test_ram[64u * 1024u];
static dk_machine_t machine;
volatile uint32_t dreamkai360_cloud_probe_signature = 0x444B3336u;

int main(void)
{
    unsigned i;
    if (dk_machine_init(&machine, test_ram, sizeof(test_ram), 0) != 0)
        return 1;

    /* Fill a short SH-4 program with NOP (0x0009, big-endian). */
    for (i = 0; i < 2048; i += 2) {
        test_ram[i] = 0x00;
        test_ram[i + 1] = 0x09;
    }

    for (i = 0; i < 512; ++i) {
        if (dk_machine_step(&machine, 0, 2) != 0)
            return 2;
    }

    if (machine.cpu.halted || machine.cpu.cycles != 1024u)
        return 3;

    dreamkai360_cloud_probe_signature ^= (uint32_t)machine.cpu.cycles;
    return 0;
}
