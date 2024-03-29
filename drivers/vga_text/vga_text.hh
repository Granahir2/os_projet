#pragma once
#include "x86/memory.hh"

namespace vga {

// *Wants*. The constructor makes sure this is the case
constexpr static x64::phyaddr buff_addr  = 0xb8000;

namespace graphics {
constexpr static int16_t aport = 0x3ce;
constexpr static int16_t dport = 0x3cf;
enum {SR, SR_EN, CC, DATA_ROT, READMAP_SEL, MODE, MISC, COLOR_DONTCARE, MASK} reg;
}

namespace seq {
constexpr static int16_t aport = 0x3c4;
constexpr static int16_t dport = 0x3c5;
enum {RESET, CLOCKMODE, MAPMASK, CHARMAP_SELECT, MEMMODE} reg;
}

namespace crtc {
constexpr static aport = 0x3d4; 
constexpr static dport = 0x3d5;
enum {ST_ADDR_HI = 0x0c, ST_ADDR_LO = 0x0d} reg;
}


enum color {
	BLACK = 0,
	BLUE = 1,
	GREEN = 2,
	CYAN = 3,
	RED = 4,
	MAGENTA = 5,
	BROWN = 6,
	LIGHT_GREY = 7,
	DARK_GREY = 8,
	LIGHT_BLUE = 9,
	LIGHT_GREEN = 10,
	LIGHT_CYAN = 11,
	LIGHT_RED = 12,
	LIGHT_MAGENTA = 13,
	LIGHT_BROWN = 14,
	WHITE = 15,
};

class text_adapter {
public:
	static int8_t  curr_row;
	static int8_t  curr_column;
	static int16_t curr_offset;

	int8_t curr_flags; // Current status of flag byte (not synchronised among instances)

	void reset();
	void print(char* str); // Prints, interpreting escape codes

};
}
