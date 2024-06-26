#include <cstdint>

uint64_t* md5() {
    /* fread, stdin */
    uint32_t buffer[16];
    uint8_t s[64] = { 7, 12, 17, 22,  7, 12, 17, 22, 
                      7, 12, 17, 22,  7, 12, 17, 22, 
                      5,  9, 14, 20,  5,  9, 14, 20,
                      5,  9, 14, 20,  5,  9, 14, 20,
                      4, 11, 16, 23,  4, 11, 16, 23,
                      4, 11, 16, 23,  4, 11, 16, 23,
                      6, 10, 15, 21,  6, 10, 15, 21,
                      6, 10, 15, 21,  6, 10, 15, 21 };
    uint32_t k[64] = { 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
                       0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                       0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
                       0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                       0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                       0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                       0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
                       0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                       0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
                       0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                       0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                       0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                       0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
                       0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                       0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                       0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
    uint32_t a0 = 0x67452301;
    uint32_t b0 = 0xefcdab89;
    uint32_t c0 = 0x98badcfe;
    uint32_t d0 = 0x10325476;
    uint32_t A, B, C, D, f, g, temp;

    bool end = false;
    bool additional_block = false;
    uint64_t file_size = 0;

    auto hash_iteration = [&] ()
    {
        A = a0;
        B = b0;
        C = c0;
        D = d0;

        for (uint8_t i = 0; i < 64; i++)
        {
            if(i < 16)
            {
                f = (B & C) | ((~B) & D);
                g = i;
            }
            else if(i < 32)
            {
                f = (D & B) | ((~D) & C);
                g = (5 * i + 1) % 16;
            }
            else if(i < 48)
            {
                f = B ^ C ^ D;
                g = (3 * i + 5) % 16;
            }
            else
            {
                f = C ^ (B | (~D));
                g = (7 * i) % 16;
            }

            temp = D;
            D = C;
            C = B;
            B = B + ((A + f + k[i] + buffer[g]) << s[i]);
            A = temp;
        }
    };

    while(!end)
    {
        size_t number_of_bytes_read = fread(&buffer, 1, 64, stdin);
        file_size += number_of_bytes_read;
        
        if (number_of_bytes_read < 64)
        {
            buffer[number_of_bytes_read] = 0x80;
            if (number_of_bytes_read < 62)
            {
                for (size_t i = number_of_bytes_read + 1; i < 64; i++)
                    buffer[i] = 0;
                buffer[14] = file_size << 3;
                buffer[15] = file_size >> 29;
                file_size = 0;
            }
            else
            {
                additional_block = true;
            }
            end = true;

            hash_iteration();
        }
        hash_iteration();
    }
    if (additional_block)
    {
        for (size_t i = 0; i < 14; i++)
            buffer[i] = 0;
        buffer[14] = file_size << 3;
        buffer[15] = file_size >> 29;
        hash_iteration();
    }

    uint64_t* result = new uint64_t[4];
    result[0] = (a0 << 32) | b0;
    result[1] = (c0 << 32) | d0;
    return result;
}
