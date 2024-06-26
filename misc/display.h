#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

class Display {
public:
    Display();
    void clear();
    void print(const char* str, ...);
    void vprint(const char* str, va_list args);
private:
    inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg);
    inline uint16_t vga_entry(unsigned char uc, uint8_t color);
    size_t strlen(const char* str);

    static const size_t VGA_WIDTH = 80;
    static const size_t VGA_HEIGHT = 25;
    
    static uint16_t current_offset;
    static size_t terminal_row;
    static size_t terminal_column;
    static uint8_t terminal_color;

    static const intptr_t terminal_buffer_addr = 0xb8000;
    
    uint16_t* const terminal_buffer;
    
    void terminal_setcolor(uint8_t color);
    void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);
    void terminal_putchar(char c);
    void terminal_write(const char* data, size_t size);
    void print_number(unsigned int i, int base);
};

