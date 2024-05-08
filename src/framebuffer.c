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
struct Cursor framebuffer_get_cursor()
{
    out(CURSOR_PORT_CMD, 0x0E);
    int offset = in(CURSOR_PORT_DATA) << 8;
    out(CURSOR_PORT_CMD, 0x0F);
    offset += in(CURSOR_PORT_DATA);
    struct Cursor c =
        {
            .row = offset / 80,
            .col = offset % 80};
    return c;
};
void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    uint16_t attrib = (bg << 4) | (fg & 0x0F);
    volatile uint16_t *where;
    where = (volatile uint16_t *)FRAMEBUFFER_MEMORY_OFFSET + (row * 80 + col);
    *where = c | (attrib << 8);
}

void framebuffer_clear(void)
{
    uint16_t space = 0x20 | (0x07 << 8);
    uint16_t i;
    volatile uint16_t *where;
    where = (volatile uint16_t *)FRAMEBUFFER_MEMORY_OFFSET;
    for (i = 0; i < 80 * 25; i++)
    {
        *where = space;
        where++;
    }
}

void putchar(char* ebx, uint8_t ecx)
{
    char kata = *ebx;
    struct Cursor c = framebuffer_get_cursor();
    int newrow = c.row;
    int newcol = c.col;
    // int offset = c.row * 80 + c.col;
    if (kata!='\n'){
        framebuffer_write(c.row, c.col, kata, ecx, 0);
        newcol += 1;
    }else{
        newrow += 1;
        newcol = 0;
    }
    if (kata!=0){
        if (c.col == 79)
            {
                framebuffer_set_cursor(newcol + 1, 0);
            }   
        else
            {
                framebuffer_set_cursor(newrow, newcol);
            }
    }
    
}
void puts(char *ebx, uint8_t length, uint8_t textcolor)
{
    struct Cursor c = framebuffer_get_cursor();
    int offset = c.row * 80 + c.col;
    for (uint8_t i = 0; i < length; i++)
    {
        if (offset >= 80 * (25))
        {
            int new_off = offset_framebuffer(offset);
            offset = new_off;
        }
        if (ebx[i] == '\n')
        {
            offset = (offset / 80 + 1) * 80;
        }
        else
        {
            framebuffer_write(offset / 80, offset % 80, ebx[i], textcolor, 0x00); // hitam
            offset += 1;
        }
    }
}
int offset_framebuffer(int offset)
{
    memcpy(
        (void *)FRAMEBUFFER_MEMORY_OFFSET,
        (void *)(FRAMEBUFFER_MEMORY_OFFSET + 80 * 2),
        80 * 2 * (25));

    for (int i = 0; i < 80; i++)
    {
        framebuffer_write(25 - 1, i, ' ', 0x0F, 0x00);
    }

    return (offset) - (1 * (80));
}