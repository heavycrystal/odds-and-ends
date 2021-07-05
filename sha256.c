/*  A super simple SHA-256 implementation by Kevin K. Biju.
    Written in standard C89/ANSI-C, with endian agnosticness.    */

#ifndef __STDC__
#error "C89/ANSI-C support not complete [missing __STDC__], refusing to compile."
#endif

#include    <stdio.h>
#include    <stdlib.h>
#include    <limits.h>

#if CHAR_BIT != 8
#error "CHAR_BIT not 8, refusing to compile."
#endif

#define     AS_U32(input)               ((u32) input)                                          
#define     RIGHT_ROTATE(input, dist)   ((input >> dist) | (input << (32u - dist)))                                                         

#define     INVALID_DATA_TYPES_ERROR    "Invalid data-types provided. Please recompile with proper typedefs. Aborting.\n"                                          
#define     FILE_OPEN_ERROR             "File could not be opened. Aborting.\n"
#define     FILE_READ_ERROR             "Error reading file. Aborting.\n"

typedef     unsigned char               u8;
typedef     unsigned int                u32;

u8 working_buffer[256];
u32 hash_values[8] = 
{   0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
u32 round_hash_values[8];
const u32 round_constants[64] = 
{   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };  

void assert_processor(void)
{
    u8 u8_test = 0u;
    u32 u32_test = 0u;
    u8_test--;
    u32_test--;
    if((u8_test < 0) || (u32_test < 0) || (sizeof(u8) != 1) || (sizeof(u32) != 4))
    {
        fprintf(stderr, INVALID_DATA_TYPES_ERROR);
        exit(1);
    }
}

u32 construct_u32(u8 index)
{
    return ((((u32)working_buffer[(4u * index)]) << 24u) + (((u32)working_buffer[(4u * index) + 1u]) << 16u) + (((u32)working_buffer[(4u * index) + 2u]) << 8u) + ((u32)working_buffer[(4u * index) + 3u]));
}

void u32_to_buffer(u32 number, u8 index)
{
    u8 loop_var = 0u;

    for(; loop_var < 4u; loop_var++)
    {
        working_buffer[(4u * index) + loop_var] = (number) >> (8u * (3u - loop_var)); 
    }
}

void processor()
{
    u8 loop_var = 16u;
    u32 temp_vars[6u];

    for(; loop_var < 64u; loop_var++)
    {   
        temp_vars[0u] = RIGHT_ROTATE(construct_u32(loop_var - 15u), 7u) ^ RIGHT_ROTATE(construct_u32(loop_var - 15u), 18u) ^ (construct_u32(loop_var - 15u) >> 3u);
        temp_vars[1u] = RIGHT_ROTATE(construct_u32(loop_var - 2u), 17u) ^ RIGHT_ROTATE(construct_u32(loop_var - 2u), 19u) ^ (construct_u32(loop_var - 2u) >> 10u); 
        u32_to_buffer(construct_u32(loop_var - 16u) + temp_vars[0u] + construct_u32(loop_var - 7u) + temp_vars[1u], loop_var);       
    }
     
    for(loop_var = 0u; loop_var < 8u; loop_var++)
    {
        round_hash_values[loop_var] = hash_values[loop_var];
    }

    for(loop_var = 0u; loop_var < 64u; loop_var++)
    {
        temp_vars[0u] = RIGHT_ROTATE(round_hash_values[4u], 6u) ^ RIGHT_ROTATE(round_hash_values[4u], 11u) ^ RIGHT_ROTATE(round_hash_values[4u], 25u);
        temp_vars[1u] = (round_hash_values[4u] & round_hash_values[5u]) ^ (~(round_hash_values[4u]) & round_hash_values[6u]);
        temp_vars[2u] = round_hash_values[7u] + temp_vars[0u] + temp_vars[1u] + round_constants[loop_var] + construct_u32(loop_var);
        temp_vars[3u] = RIGHT_ROTATE(round_hash_values[0u], 2u) ^ RIGHT_ROTATE(round_hash_values[0u], 13u) ^ RIGHT_ROTATE(round_hash_values[0u], 22u);
        temp_vars[4u] = (round_hash_values[0u] & round_hash_values[1u]) ^ (round_hash_values[0u] & round_hash_values[2u]) ^ (round_hash_values[1u] & round_hash_values[2u]);
        temp_vars[5u] = temp_vars[3u] + temp_vars[4u];

        round_hash_values[7u] = round_hash_values[6u];
        round_hash_values[6u] = round_hash_values[5u];
        round_hash_values[5u] = round_hash_values[4u];
        round_hash_values[4u] = round_hash_values[3u] + temp_vars[2u];
        round_hash_values[3u] = round_hash_values[2u];
        round_hash_values[2u] = round_hash_values[1u];
        round_hash_values[1u] = round_hash_values[0u];
        round_hash_values[0u] = temp_vars[2u] + temp_vars[5u];
    }

    for(loop_var = 0u; loop_var < 8u; loop_var++)
    {
        hash_values[loop_var] = hash_values[loop_var] + round_hash_values[loop_var];
    }
}

int main(int argc, char** argv)
{   

    FILE* file_handle;
    u32 file_size_low = 0u;
    u32 file_size_high = 0u;
    u8 read_bytes;

    assert_processor();
    file_handle = fopen(argv[AS_U32(argc) - 1u], "rb");
    if(file_handle == NULL)
    {
        fprintf(stderr, FILE_OPEN_ERROR);
        exit(1);
    }

    while(!feof(file_handle))
    {
        read_bytes = fread(working_buffer, 1u, 64u, file_handle);
        if(ferror(file_handle))
        {
            fprintf(stderr, FILE_READ_ERROR);
            exit(1);
        }
        if((0xFFFFFFFFu - (8u * read_bytes)) <= file_size_low)
        {
            file_size_low = file_size_low - (0xFFFFFFFFu - (8u * read_bytes) + 1u);
            file_size_high++;
        }
        else
        {
            file_size_low = file_size_low + (8u * read_bytes);
        }
      
        if(read_bytes < 64u)
        {
            if(read_bytes < 56u)
            {
                working_buffer[read_bytes++] = 0x80u;
                for(; read_bytes < 56u; read_bytes++)
                {
                    working_buffer[read_bytes] = 0u; 
                }
                for(; read_bytes < 60u; read_bytes++)
                {
                    working_buffer[read_bytes] = (file_size_high) >> (8u * (59u - read_bytes));     
                }
                for(; read_bytes < 64u; read_bytes++)
                {
                    working_buffer[read_bytes] = (file_size_low) >> (8u * (63u - read_bytes));     
                }                
                processor();
            }
            else
            {
                working_buffer[read_bytes++] = 0x80u;
                for(; read_bytes < 64u; read_bytes++)
                {
                    working_buffer[read_bytes] = 0u; 
                }
                processor();

                for(read_bytes = 0u; read_bytes < 56u; read_bytes++)
                {
                    working_buffer[read_bytes] = 0u;
                }
                for(; read_bytes < 60u; read_bytes++)
                {
                    working_buffer[read_bytes] = (file_size_high) >> (8u * (59u - read_bytes));     
                }
                for(; read_bytes < 64u; read_bytes++)
                {
                    working_buffer[read_bytes] = (file_size_low) >> (8u * (63u - read_bytes));     
                }
                processor();              
            }
        }
        else
        {
            processor();
        }  
    }

    printf("%s  -   ", argv[AS_U32(argc) - 1u]);
    for(read_bytes = 0; read_bytes < 8u; read_bytes++)
    {
        printf("%08x", hash_values[read_bytes]);
    }
    printf("\n");

    fclose(file_handle);
    return 0;
}