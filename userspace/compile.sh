#!/usr/bin/sh
x86_64-elf-g++ -ffreestanding -nostartfiles -mcmodel=large -ggdb -c user_libc.cc
x86_64-elf-g++ -ffreestanding -nostartfiles -mcmodel=large -ggdb -c start.s
x86_64-elf-g++ -ffreestanding -nostartfiles -mcmodel=large -ggdb -c test.cc
x86_64-elf-g++ -ffreestanding -nostartfiles -mcmodel=large -no-pie -fno-pie -nostdlib -ggdb user_libc.o test.o start.o -o test.elf
cp test.elf ../fatimage
