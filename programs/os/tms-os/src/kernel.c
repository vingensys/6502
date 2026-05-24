#include "tmsos.h"

void kernel_main(void) {
    os_warm_start();
}

void kernel_warm_start(void) {
    os_puts("TMS-OS " TMSOS_VERSION "\n");
    os_puts("READY\n");
    shell_run();
}
