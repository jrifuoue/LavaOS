#include "hpet.h"
#include "acpi.h"
#include "iomem.h"
#include "log.h"

#define HPET_ID_REG          0x00
#define HPET_CFG_REG         0x10
#define HPET_COUNTER_REG     0xF0

#define HPET_CFG_ENABLE      (1 << 1)

static HPET hpet;

typedef struct {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
} __attribute__((packed)) ACPIGas;

typedef struct {
    ACPISDTHeader header;
    uint32_t event_timer_block_id;
    ACPIGas address;
    uint8_t sequence;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed)) HPETTable;

bool hpet_init(void) {
    hpet.present = false;
    hpet.base = NULL;
    hpet.frequency = 0;

    ACPISDTHeader* header = acpi_find("HPET");
    if(!header) {
        kwarn("No HPET ACPI table found");
        return false;
    }

    HPETTable* table = (HPETTable*)header;
    if(table->address.address == 0) {
        kwarn("HPET address is NULL");
        return false;
    }

    void* mapped = iomap_bytes(table->address.address, 0x100, KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE | KERNEL_PFLAG_CACHE_DISABLE);
    if(!mapped) {
        kwarn("Failed to map HPET registers");
        return false;
    }

    hpet.base = (volatile uint64_t*)mapped;

    uint64_t id = hpet.base[HPET_ID_REG / 8];
    uint32_t rev = (uint32_t)(id & 0xFF);
    uint32_t comparator_count = (uint32_t)((id >> 8) & 0x1F);
    uint32_t counter_size = (uint32_t)((id >> 13) & 1);

    (void)rev;
    (void)comparator_count;
    (void)counter_size;

    // Frequency is derived from minimum_tick (period in femtoseconds)
    // freq = 10^15 / minimum_tick
    if(table->minimum_tick > 0) {
        hpet.frequency = (uint32_t)(1000000000000000ULL / table->minimum_tick);
    } else {
        kwarn("HPET minimum_tick is 0");
        iounmap_bytes(mapped, 0x100);
        hpet.base = NULL;
        return false;
    }

    if(hpet.frequency == 0) {
        kwarn("HPET frequency is 0");
        iounmap_bytes(mapped, 0x100);
        hpet.base = NULL;
        return false;
    }

    hpet.base[HPET_CFG_REG / 8] = HPET_CFG_ENABLE;
    hpet.present = true;

    kinfo("HPET: rev=%u counters=%u freq=%u Hz", rev, comparator_count, hpet.frequency);
    return true;
}

uint64_t hpet_read_counter(void) {
    if(!hpet.base) return 0;
    return hpet.base[HPET_COUNTER_REG / 8];
}

void hpet_busy_wait_us(uint64_t us) {
    if(!hpet.base) return;
    uint64_t ticks_per_us = hpet.frequency / 1000000;
    uint64_t start = hpet_read_counter();
    uint64_t target = start + us * ticks_per_us;
    while(hpet_read_counter() < target) {
        asm volatile("pause");
    }
}

bool hpet_is_present(void) {
    return hpet.present;
}

uint32_t hpet_get_frequency(void) {
    return hpet.frequency;
}
