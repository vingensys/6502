#ifndef INC_6502_KINPUT_H
#define INC_6502_KINPUT_H

#include <stdint.h>

void kinput_listen(void);
uint8_t kinput_should_quit(void);
uint8_t kinput_is_running(void);
void kinput_set_running(uint8_t running);

#endif
