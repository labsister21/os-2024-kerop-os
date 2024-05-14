#ifndef _CMOS_DRIVER_H
#define _CMOS_DRIVER_H

#include "../cpu/portio.h"
#include "../cpu/interrupt.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

#define REG_SECONDS 0x00
#define REG_MINUTES 0x02
#define REG_HOURS 0x04
#define REG_DAY_OF_MONTH 0x07
#define REG_MONTH 0x08
#define REG_YEAR 0x09

struct clock
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
} __attribute__((packed));

int8_t get_update_in_process_flag();
uint8_t get_rtc_register(int8_t reg);
void set_cmos(int8_t reg, uint8_t val);

void read_CMOS();
void write_CMOS(struct clock *cl);
struct clock get_clock();

#endif