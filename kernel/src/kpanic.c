#include "kpanic.h"
#include "serial.h"
#include "log.h"
#include "printk.h"
#include <stdarg.h>

void kpanic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kernel.logger = &serial_logger;
    kfatal_va(fmt, args);
    va_end(args);
    va_start(args, fmt);
    printk_set_color(0xFF0000, 0x000000);

    printk("[BUG!] ");
    vprintk(fmt, args);
    printk("\n");

    printk_reset_color();
    va_end(args);
    kabort();
}
