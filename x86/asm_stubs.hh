#pragma once
#include "x86/descriptors.hh"
#include "x86/memory.hh"

namespace x64 {
void lgdt(uint64_t desc_ptr);
void reload_descriptors(); // assumes a gdt layout
void reload_tss(uint16_t desc);

void enable_sse();

void lidt(uint64_t desc_ptr);
void outb(uint16_t port, uint8_t x);
uint8_t inb(uint16_t port);
void outl(uint16_t port, uint32_t x);
uint32_t inl(uint16_t port);
void cli();
void sei();

uint64_t rdmsr(uint32_t msr);

x64::phaddr get_cr3();
void load_cr3(x64::phaddr);
}
