// From https://wiki.osdev.org/Bare_Bones, used with discretion
// Placeholder 64 bit kernel

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "./misc/display.h"

extern "C" void kernel_main(void)  {
    	Display terminal = Display();
	terminal.print("Hello, %d bit kernel World!", 64);
}
