#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    volatile uint64_t* base;
    uint32_t frequency;
    bool present;
} HPET;

bool hpet_init(void);
uint64_t hpet_read_counter(void);
void hpet_busy_wait_us(uint64_t us);
bool hpet_is_present(void);
uint32_t hpet_get_frequency(void);
