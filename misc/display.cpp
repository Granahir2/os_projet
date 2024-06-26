#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "x86/asm_stubs.hh"
#include "display.h"


size_t Display::terminal_row;
size_t Display::terminal_column;
uint8_t Display::terminal_color;

/* Hardware text mode color constants. */
 
inline uint8_t Display::vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}
 
inline uint16_t Display::vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}
 
size_t Display::strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}
 
Display::Display() : terminal_buffer((uint16_t*)(terminal_buffer_addr)) {}

void Display::clear() {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}
 
void Display::terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}
 
void Display::terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}
 
void Display::terminal_putchar(char c) 
{
	if(c == '\n') {
		for(auto i = terminal_column; i < VGA_WIDTH; ++i) {
			terminal_putentryat(' ', terminal_color, i, terminal_row);
		}
		terminal_column = 0;
		if(++terminal_row == VGA_HEIGHT) terminal_row = 0;
		return;
	}
#ifdef KERNEL
	if(c == '\r') {
		x64::outb(0x3d4, 0x0d);
		x64::outb(0x3d5, 0);
		return;
	}
#endif

	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT) terminal_row = 0;
	}
}
 
void Display::terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void Display::print_number(unsigned int i, int base)
{
    if (base == 10)
    {
        if (i >= 10)
        {
            print_number(i / 10, 10);
            terminal_putchar((i % 10) + '0');
        }
        else
        {
            terminal_putchar(i + '0');
        }
    }
    else if (base == 16)
    {
        if (i >= 16)
        {
            print_number(i / 16, 16);
            i = i % 16;
            if (i < 10)
            {
                terminal_putchar(i + '0');
            }
            else
            {
                terminal_putchar(i - 10 + 'A');
            }
        }
        else
        {
            if (i < 10)
            {
                terminal_putchar(i + '0');
            }
            else
            {
                terminal_putchar(i - 10 + 'A');
            }
        }
    }

}

void Display::print(const char* str, ...) 
{
    va_list args;
    va_start(args, str);
    vprint(str, args);
    va_end(args);
}

void Display::vprint(const char* str, va_list args) {
    const char* p = str;
    int i;
    unsigned int u;
    char *s;

    for(; *p != '\0'; p++)
    {
        if(*p != '%')
        {
            terminal_putchar(*p);
            continue;
        }

        switch(*++p)
        {
            case 'c':
                i = va_arg(args, int);
                terminal_putchar(i);
                break;
            case 'd':
                i = va_arg(args, int);
                if(i < 0)
                {
                    u = -i;
                    terminal_putchar('-');
                }
                else 
                {
                    u = i;
                }
                print_number(u, 10);
                break;
            case 'u':
                u = va_arg(args, unsigned int);
                print_number(u, 10);
                break;
	    case 'p':{
		uint64_t x = va_arg(args, uint64_t);
		for(int i = 60; i >= 0; i -= 4) {
			constexpr char lut[16] = {'0','1','2','3','4','5','6','7',
						  '8','9','A','B','C','D','E','F'};
			terminal_putchar(lut[(x >> i)&0xf]);
		}
		} break;
            case 'x':
                u = va_arg(args, unsigned int);
                print_number(u, 16);
                break;
            case 's':
                s = va_arg(args, char*);
                terminal_write(s, strlen(s));
                break;
            case '%':
                terminal_putchar('%');
                break;
        }
    }
}
