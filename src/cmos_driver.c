#include "header/driver/cmos_driver.h"

static struct clock Clock;

int8_t get_update_in_process_flag()
{
    out(CMOS_ADDRESS, 0x0A);
    return (in(CMOS_DATA) & 0x80);
}

uint8_t get_rtc_register(int8_t reg)
{
    out(CMOS_ADDRESS, reg);
    return in(CMOS_DATA);
}

void set_cmos(int8_t reg, uint8_t val)
{
    out(CMOS_ADDRESS, reg);
    out(CMOS_DATA, val);
}

void read_CMOS()
{
    while (get_update_in_process_flag())
        ;
    Clock.second = get_rtc_register(REG_SECONDS);
    Clock.minute = get_rtc_register(REG_MINUTES);
    Clock.hour = get_rtc_register(REG_HOURS);

    uint8_t regCode = get_rtc_register(0x0B);
    if (!(regCode & 0x04))
    {
        Clock.second = ((Clock.second & 0x0F) + (Clock.second / 16) * 10);
        Clock.minute = ((Clock.minute & 0x0F) + (Clock.minute / 16) * 10);
        Clock.hour = ((Clock.hour & 0x0F) + (((Clock.hour & 0x70) / 16) * 10)) | (Clock.hour & 0x80);
    }
}

void write_CMOS(struct clock *cl)
{
    set_cmos(REG_SECONDS, cl->second);
    set_cmos(REG_MINUTES, cl->minute);
    set_cmos(REG_HOURS, cl->hour);
}

struct clock get_clock()
{
    return Clock;
}