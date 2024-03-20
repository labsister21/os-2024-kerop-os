#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "header/text/framebuffer.h"
#include "header/cpu/portio.h"
#include "header/stdlib/string.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c)
{
    uint16_t pos = r * 80 + c;

    out(0x3D4, 0x0F);
    out(0x3D5, (uint8_t)(pos & 0xFF));
    out(0x3D4, 0x0E);
    out(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    uint16_t attrib = (bg << 4) | (fg & 0x0F);
    volatile uint16_t *where;
    where = (volatile uint16_t *)0xB8000 + (row * 80 + col);
    *where = c | (attrib << 8);
}

void framebuffer_clear(void)
{
    // TODO : Implement
    size_t size = 80 * 25 * 2;
    memset(FRAMEBUFFER_MEMORY_OFFSET, 0x0, size);
}
