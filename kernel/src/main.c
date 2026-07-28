#include "../../config.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "port.h"
#include "serial.h"
#include "logger.h"
#include "log.h"
#include "assert.h"
#include "print.h"
#include "print_base.h"
#include "utils.h"
#include "memory.h"
#include "mem/bitmap.h"
#include "kernel.h"
#include "page.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/enable_arch_extra.h"
#include "arch/x86_64/exception.h"
#include "vfs.h"
#include "rootfs.h"
#include "mem/slab.h"
#include "string.h"
#include "process.h"
#include "task.h"
#include "task_switch.h"
#include "exec.h"
#include "pic.h"
#include "devices.h"
#include "./devices/tty/tty.h"
#include <minos/keycodes.h>
#include <minos/key.h>
#include <sync.h>
#include "cmdline.h"
#include "charqueue.h"
#include "filelog.h"
#include "iomem.h"
#include "acpi.h"
#include "apic.h"
#include "pci.h"
#include "interrupt.h"
#include "general_caches.h"
#include "epoll.h"
#include "sockets/minos.h"
#include "smp.h"
#include "mem/shared_mem.h"
#include "printk.h"
#include "term/fb/fb.h"
#include "hpet.h"

#include "fblogger.h"
#include "hash_table.h"
#include "kht.h"

static void set_version() {
    kernel.kname = KNAME;
    kernel.kver = KVER;

    kernel.dname = DNAME;
    kernel.dver = DVER;
    kernel.dcode = DCODE;
}
static void handle_interrupts() {
    for(;;) {
        asm volatile("hlt");
    }
}
void spawn_init(void) {
    intptr_t e = 0;
    const char* epath = NULL;
    Args args;
    Args env;
    epath = "/init";
    const char* argv[] = {epath, NULL};
    args = create_args(argv);
    const char* envv[] = {NULL};
    env  = create_args(envv);
    if((e = exec_new(epath, &args, &env)) < 0) kpanic("Failed to exec %s : %s",epath,status_str(e));
}
void _start() {
    disable_interrupts();
    BREAKPOINT();

    set_version();

    printk("Using %s kernel v%s\n", kernel.kname, kernel.kver);
    printk("Now booting %s %s v%s\n", kernel.dname, kernel.dcode, kernel.dver);

    printk("\n");

    printk("[WAIT] Initializing serial...\n");
    serial_init();
    kernel.logger = &serial_logger;
    kernel.logger->level = LOG_ALL;
    printk("[ OK ] Initialized serial.\n");
    printk("[WAIT] Initializing cmdline...\n");
    init_cmdline();
    printk("[ OK ] Initialized cmdline.\n");
    printk("[WAIT] Initializing loggers...\n");
    init_loggers();

    init_fb_logger();

    printk("[ OK ] Initialized loggers.\n");
    printk("[WAIT] Initializing GDT and IDT...\n");
    init_gdt();
    disable_interrupts();
    init_idt();
    printk("[ OK ] Initialized GDT and IDT.\n");
    printk("[WAIT] Initializing essential components and devices...\n");
    init_exceptions();
    reload_tss();
    init_bitmap();
    init_paging();
    KERNEL_SWITCH_VTABLE();
    enable_cpu_features();
    printk("[ OK ] Initialized essential components and devices.\n");
    printk("[WAIT] Starting Interrupt controller...\n");
    init_pic();
    init_acpi();
    printk("[ OK ] Started Interrupt controller.\n");
    printk("[WAIT] Initializing HPET...\n");
    hpet_init();
    printk("[ OK ] Initialized HPET.\n");
    enable_interrupts();
    printk("[WAIT] Configuring caches...\n");
    init_cache_cache();
    minos_socket_init_cache();
    init_epoll_cache();
    init_general_caches();
    init_charqueue();
    printk("[ OK ] Configured caches.\n");
    printk("[WAIT] Configuring and testing kernel hash table.\n");
    ht_init(&kernel_ht);
    printk("[ OK ] Kernel hash table is fine.\n");
    printk("[VERB] Loading PCI...\n");
    init_pci();
    printk("[VERB] Loading SMP...\n");
    init_smp();
    printk("[VERB] Initializing load balancer lock...\n");
    spinlock_init(&kernel.load_balancer_lock);
    printk("[VERB] Configuring memory, starting core tasks and scheduler...\n");
    printk("[VERB] Memregion...\n");
    init_memregion();
    printk("[VERB] Processes...\n");
    init_processes();
    printk("[VERB] Tasks...\n");
    init_tasks();
    printk("[VERB] Kernel task...\n");
    init_kernel_task();
    printk("[VERB] Schedulers...\n");
    init_schedulers();
    printk("[VERB] Task switch...\n");
    init_task_switch();
    printk("[VERB] Resources...\n");
    init_resources();
    printk("[VERB] SHM Cache...\n");
    init_shm_cache();
    enable_interrupts();
    printk("[WAIT] Initializing filesystms...\n");
    printk("[VERB] Initializing VFS...\n");
    init_vfs();
    printk("[VERB] Initializing rootfs...\n");
    init_rootfs();
    printk("[ OK ] Initialized filesystms.\n");
    printk("[VERB] Initializing devices...\n");
    init_devices();
    printk("[VERB] Initializing TTY...\n");
    init_tty();

    spawn_init();

    disable_interrupts();
    irq_clear(kernel.task_switch_irq);
    enable_interrupts();
    handle_interrupts();
}
