/* Minimal Xbox 360 Xenon probe built in GitHub Actions.
 * This is a toolchain smoke test, not the Dreamcast emulator itself.
 */
#include <stdint.h>

volatile uint32_t dreamkai360_cloud_probe_signature = 0x444B3336u;

int main(void)
{
    volatile uint32_t heartbeat = 0;
    for (;;) {
        heartbeat++;
        __asm__ volatile ("" : "+r"(heartbeat) : : "memory");
    }
    return 0;
}
