#include "utils.h"
#include <stddef.h>
#include "assert.h"
#include "arch/x86_64/idt.h"
#include "devices/ps2/keyboard/keyboard.h"

void kabort(void) {
    disable_interrupts();
    for(;;) {
        asm volatile("hlt");
    }
}
