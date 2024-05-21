#include <stdint.h>

// Implemnetation of 64-bit CRC checksum
uint32_t CRC32() {
    uint32_t crc32 = 0xFFFFFFFF;
    uint32_t data;
    while(fread(&data, 4, 1, stdin))
    {
        crc32 ^= data;
        for(int i = 0; i < 32; i++)
        {
            if(crc32 & 1)
                crc32 = (crc32 >> 1) ^ 0xEDB88320u;
            else
                crc32 >>= 1;
        }
    }
    crc32 ^= 0xFFFFFFFF;
    return crc32;
}