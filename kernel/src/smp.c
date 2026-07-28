// TODO: Refactor this out? Idk how necessary it is. I feel like this is just way simpler.
// Then again a preboot will just fix all of this anyway so I'm postponing :)
#include <limine.h>
#include "log.h"
#include "printk.h"
#include "kpanic.h"
#include "memory.h"

static volatile struct limine_smp_request limine_smp_request = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0,
    .flags = 0,
};
extern void ap_init(struct limine_smp_info*);
#define AP_STACK_SIZE 1*PAGE_SIZE
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/enable_arch_extra.h"
#include "apic.h"

static Mutex tss_sync = { 0 };
void ap_main(struct limine_smp_info* info) {
    reload_idt();
    reload_gdt();
    kernel_reload_gdt_registers();
    mutex_lock(&tss_sync);
    reload_tss();
    mutex_unlock(&tss_sync);
    __asm__ volatile(
            "movq %0, %%cr3\n"
            :
            : "r" ((uintptr_t)kernel.pml4 & ~KERNEL_MEMORY_MASK)
        );
    // APIC divider of 16
    printk("[CORE] Hello from logical processor %zu lapic_id %zu\n", info->lapic_id, get_lapic_id());
    enable_cpu_features();
    kernel.processors[info->lapic_id].initialised = true;
    lapic_timer_reload();
    irq_clear(kernel.task_switch_irq);
    enable_interrupts();
    __asm__ volatile("sti; nop; nop");
}

static void** ap_stacks = NULL;
static size_t ap_stack_count = 0;

void init_smp(void) {
    if(!limine_smp_request.response) return;

    if(!lapic_is_present()) {
        printk("[FAIL] LAPIC not detected, SMP disabled.\n");
        return;
    }

    size_t cpu_count = limine_smp_request.response->cpu_count;
    // Mark BSP as initialised
    kernel.processors[limine_smp_request.response->bsp_lapic_id].initialised = true;

    // Dynamically allocate stacks only for non-BSP cores
    size_t ap_count = 0;
    for(size_t i = 0; i < cpu_count; ++i) {
        struct limine_smp_info* info = limine_smp_request.response->cpus[i];
        if(info->lapic_id != limine_smp_request.response->bsp_lapic_id)
            ap_count++;
    }

    if(ap_count > 0) {
        ap_stacks = kernel_malloc(ap_count * sizeof(void*));
        if(!ap_stacks) {
            kpanic("Failed to allocate AP stacks");
        }
        ap_stack_count = ap_count;
    }

    size_t stack_idx = 0;
    for(size_t i = 0; i < cpu_count; ++i) {
        struct limine_smp_info* info = limine_smp_request.response->cpus[i];
        if(info->lapic_id == limine_smp_request.response->bsp_lapic_id)
            continue;
        if(kernel.max_processor_id < info->lapic_id)
            kernel.max_processor_id = info->lapic_id;

        void* stack = kernel_malloc(AP_STACK_SIZE);
        if(!stack) {
            kpanic("Failed to allocate AP stack for core %u", (unsigned)info->lapic_id);
        }
        memset(stack, 0, AP_STACK_SIZE);
        ap_stacks[stack_idx++] = stack;
        info->extra_argument = (uintptr_t)((char*)stack + AP_STACK_SIZE);
        info->goto_address = (void*)&ap_init;
    }

    printk("[SMP] Initialized %zu cores (BSP + %zu APs)\n", cpu_count, ap_count);
}
