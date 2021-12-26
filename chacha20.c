#ifndef __STDC__
#error "C89/ANSI-C support not complete [missing __STDC__], refusing to compile."
#endif

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <limits.h>

#if CHAR_BIT != 8
#error "CHAR_BIT not 8, refusing to compile."
#endif

typedef     unsigned char               u8;
typedef     unsigned int                u32;

#define     U32_LEFT_ROTATE(input, dist)        ((input << dist) | (input >> (32 - dist)))
#define     U32_RIGHT_ROTATE(input, dist)       ((input >> dist) | (input << (32 - dist)))

#define     INVALID_DATA_TYPES_ERROR    "Invalid data-types provided. Please recompile with proper typedefs.\n"
#define     TOO_FEW_ARGUMENTS_ERROR     "Usage: \n \
        File encryption/decryption using ChaCha20: [program filename] [input file name] [output file name] [password].\n \
        File hash computation using SHA256: [program filename] [input file name]\n"
#define     INPUT_FILE_OPEN_ERROR       "Could not open input file for reading. Aborting.\n"
#define     OUTPUT_FILE_OPEN_ERROR      "Could not open output file for writing. Aborting.\n"
#define     OUTPUT_FILE_EXISTS_ERROR    "Output file already exists. Aborting.\n"
#define     INPUT_FILE_READ_ERROR       "Error reading from input file. Aborting.\n"
#define     OUTPUT_FILE_WRITE_ERROR     "Error writing to output file. Aborting."

static const u32 sha256_constants[64] =
{   0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u };

static void assert_processor(void)
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

static u32 load_u32_le(const void* buffer)
{
    u8* buffer_u8 = (u8*) buffer;
    return (u32)buffer_u8[0u] | ((u32)buffer_u8[1u] << 8u) | ((u32)buffer_u8[2u] << 16u) | ((u32)buffer_u8[3u] << 24u);
}

static u32 load_u32_be(const void* buffer)
{
    u8* buffer_u8 = (u8*) buffer;
    return (u32)buffer_u8[3u] | ((u32)buffer_u8[2u] << 8u) | ((u32)buffer_u8[1u] << 16u) | ((u32)buffer_u8[0u] << 24u);
}

static void store_u32_be(const void* buffer, u32 input)
{
    u8 loop_var;
    u8* buffer_u8 = (u8*) buffer;

    for(loop_var = 0u; loop_var < 4u; loop_var++)
    {
        buffer_u8[loop_var] = (input) >> (8u * (3u - loop_var));
    }
}

static void sha256_processor(u8* working_buffer, u32* sha256_output)
{
    u8 loop_var;
    u32 temp_vars[6u];
    u32 round_hash_values[8u];

    for(loop_var = 16u; loop_var < 64u; loop_var++)
    {
        temp_vars[2u] = load_u32_be(working_buffer + (4u * (loop_var - 15u)));
        temp_vars[3u] = load_u32_be(working_buffer + (4u * (loop_var - 2u)));
        temp_vars[0u] = U32_RIGHT_ROTATE(temp_vars[2u], 7u) ^ U32_RIGHT_ROTATE(temp_vars[2u], 18u) ^ (temp_vars[2u] >> 3u);
        temp_vars[1u] = U32_RIGHT_ROTATE(temp_vars[3u], 17u) ^ U32_RIGHT_ROTATE(temp_vars[3u], 19u) ^ (temp_vars[3u] >> 10u);
        store_u32_be(working_buffer + (4u * loop_var), load_u32_be(working_buffer + (4 * (loop_var - 16u))) + temp_vars[0u] + load_u32_be(working_buffer + (4 * (loop_var - 7u))) + temp_vars[1u]);
    }

    for(loop_var = 0u; loop_var < 8u; loop_var++)
    {
        round_hash_values[loop_var] = sha256_output[loop_var];
    }

    for(loop_var = 0u; loop_var < 64u; loop_var++)
    {
        temp_vars[0u] = U32_RIGHT_ROTATE(round_hash_values[4u], 6u) ^ U32_RIGHT_ROTATE(round_hash_values[4u], 11u) ^ U32_RIGHT_ROTATE(round_hash_values[4u], 25u);
        temp_vars[1u] = (round_hash_values[4u] & round_hash_values[5u]) ^ (~(round_hash_values[4u]) & round_hash_values[6u]);
        temp_vars[2u] = round_hash_values[7u] + temp_vars[0u] + temp_vars[1u] + sha256_constants[loop_var] + load_u32_be(working_buffer + (4 * loop_var));
        temp_vars[3u] = U32_RIGHT_ROTATE(round_hash_values[0u], 2u) ^ U32_RIGHT_ROTATE(round_hash_values[0u], 13u) ^ U32_RIGHT_ROTATE(round_hash_values[0u], 22u);
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
        sha256_output[loop_var] = sha256_output[loop_var] + round_hash_values[loop_var];
    }
}

static void password_sha256_compute(const char* password, u32* sha256_output)
{
    size_t temp_password_length = strlen(password);
    size_t password_length = temp_password_length * 8u;
    u8 working_buffer[256u];
    u8 read_bytes;

    while(temp_password_length != 0u)
    {
        read_bytes = (temp_password_length >= 64u) ? 64u : temp_password_length;
        temp_password_length = temp_password_length - read_bytes;
        memcpy(working_buffer, password, read_bytes);
        password = password + read_bytes;

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
                    working_buffer[read_bytes] = (password_length >> 32u) >> (8u * (59u - read_bytes));
                }
                for(; read_bytes < 64u; read_bytes++)
                {
                    working_buffer[read_bytes] = (password_length & 0xFFFFFFFFu) >> (8u * (63u - read_bytes));
                }
                sha256_processor(working_buffer, sha256_output);
            }
            else
            {
                working_buffer[read_bytes++] = 0x80u;
                for(; read_bytes < 64u; read_bytes++)
                {
                    working_buffer[read_bytes] = 0u;
                }
                sha256_processor(working_buffer, sha256_output);

                for(read_bytes = 0u; read_bytes < 56u; read_bytes++)
                {
                    working_buffer[read_bytes] = 0u;
                }
                for(; read_bytes < 60u; read_bytes++)
                {
                    working_buffer[read_bytes] = (password_length >> 32u) >> (8u * (59u - read_bytes));
                }
                for(; read_bytes < 64u; read_bytes++)
                {
                    working_buffer[read_bytes] = (password_length & 0xFFFFFFFFu) >> (8u * (63u - read_bytes));
                }
                sha256_processor(working_buffer, sha256_output);
            }
        }
        else
        {
            sha256_processor(working_buffer, sha256_output);
        }
    }
}

static void chacha20_quarter_round(u32* chacha20_xor_block, u8 a_index, u8 b_index, u8 c_index, u8 d_index)
{
    chacha20_xor_block[a_index] = chacha20_xor_block[a_index] + chacha20_xor_block[b_index];
    chacha20_xor_block[d_index] = U32_LEFT_ROTATE((chacha20_xor_block[d_index] ^ chacha20_xor_block[a_index]), 16);

    chacha20_xor_block[c_index] = chacha20_xor_block[c_index] + chacha20_xor_block[d_index];
    chacha20_xor_block[b_index] = U32_LEFT_ROTATE((chacha20_xor_block[b_index] ^ chacha20_xor_block[c_index]), 12);

    chacha20_xor_block[a_index] = chacha20_xor_block[a_index] + chacha20_xor_block[b_index];
    chacha20_xor_block[d_index] = U32_LEFT_ROTATE((chacha20_xor_block[d_index] ^ chacha20_xor_block[a_index]), 8);

    chacha20_xor_block[c_index] = chacha20_xor_block[c_index] + chacha20_xor_block[d_index];
    chacha20_xor_block[b_index] = U32_LEFT_ROTATE((chacha20_xor_block[b_index] ^ chacha20_xor_block[c_index]), 7);
}

static void chacha20_state_block_init(u32* chacha20_state_block, u32* key)
{
    chacha20_state_block[0u] = load_u32_le("expa");
    chacha20_state_block[1u] = load_u32_le("nd 3");
    chacha20_state_block[2u] = load_u32_le("2-by");
    chacha20_state_block[3u] = load_u32_le("te k");
    memcpy(chacha20_state_block + 4u, key, sizeof(u32) * 8u);
    chacha20_state_block[14u] = load_u32_le("mist");
    chacha20_state_block[15u] = load_u32_le("rake");
}

static void gen_chacha20_xor_block(u32* chacha20_state_block, u32* chacha20_xor_block, u32 block_counter_low, u32 block_counter_high)
{
    u8 loop_var;

    chacha20_state_block[12u] = block_counter_low;
    chacha20_state_block[13u] = block_counter_high;

    memcpy(chacha20_xor_block, chacha20_state_block, sizeof(u32) * 16u);

    for (loop_var = 0u; loop_var < 10u; loop_var++) /* 20 rounds, 2 rounds per loop. */
    {
        chacha20_quarter_round(chacha20_xor_block, 0u, 4u, 8u, 12u);    /* column 0 */
        chacha20_quarter_round(chacha20_xor_block, 1u, 5u, 9u, 13u);    /* column 1 */
        chacha20_quarter_round(chacha20_xor_block, 2u, 6u, 10u, 14u);   /* column 2 */
        chacha20_quarter_round(chacha20_xor_block, 3u, 7u, 11u, 15u);   /* column 3 */
        chacha20_quarter_round(chacha20_xor_block, 0u, 5u, 10u, 15u);   /* diagonal 1 */
        chacha20_quarter_round(chacha20_xor_block, 1u, 6u, 11u, 12u);   /* diagonal 2 */
        chacha20_quarter_round(chacha20_xor_block, 2u, 7u, 8u, 13u);    /* diagonal 3 */
        chacha20_quarter_round(chacha20_xor_block, 3u, 4u, 9u, 14u);    /* diagonal 4 */
    }

    for(loop_var = 0u; loop_var < 16u; loop_var++) /* adding original block to scrambled block */
    {
        chacha20_xor_block[loop_var] = chacha20_xor_block[loop_var] + chacha20_state_block[loop_var];
    }
}

static void file_sha256_compute(const char* input_file_name, u32* sha256_output)
{
    FILE*   input_file_handle;
    u8      working_buffer[256u];
    u32     file_size_low = 0u;
    u32     file_size_high = 0u;
    u8      read_bytes;

    assert_processor();
    input_file_handle = fopen(input_file_name, "rb");
    if(input_file_handle == NULL)
    {
        fprintf(stderr, INPUT_FILE_OPEN_ERROR);
        exit(1);
    }

    while(!feof(input_file_handle))
    {
        read_bytes = fread(working_buffer, 1u, 64u, input_file_handle);
        if(ferror(input_file_handle))
        {
            fprintf(stderr, INPUT_FILE_READ_ERROR);
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
                sha256_processor(working_buffer, sha256_output);
            }
            else
            {
                working_buffer[read_bytes++] = 0x80u;
                for(; read_bytes < 64u; read_bytes++)
                {
                    working_buffer[read_bytes] = 0u; 
                }
                sha256_processor(working_buffer, sha256_output);

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
                sha256_processor(working_buffer, sha256_output);             
            }
        }
        else
        {
                sha256_processor(working_buffer, sha256_output);
        }  
    }
}

static void file_chacha20_compute(const char* input_file_name, const char* output_file_name, const char* password)
{
    FILE*   input_file_handle;
    FILE*   output_file_handle;
    u32     chacha20_state_block[16u];
    u32     chacha20_xor_block[16u];
    u8      file_read_block[64u];
    u32     sha256_output[8u] =
    {   0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au, 0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u };    
    u8      loop_var;
    u8      read_bytes;
    u32     block_counter_low = 0u;
    u32     block_counter_high = 0u;

    assert_processor();
/*  Checking for existence of the output file, failing safely for simplicity. This is vulnerable to TOCTOU exploits. Please do not do this in actual code.
    No standards compliant way of working around this until C11, where fopen() has an "x" mode. Keeping the code C89 compliant leads to this. */
    output_file_handle = fopen(output_file_name, "rb");
    if(output_file_handle != NULL)
    {
        fprintf(stderr, OUTPUT_FILE_EXISTS_ERROR);
        fclose(output_file_handle);
        exit(1);
    }

    input_file_handle = fopen(input_file_name, "rb");
    if(input_file_handle == NULL)
    {
        fprintf(stderr, INPUT_FILE_OPEN_ERROR);
        exit(1);
    }
    output_file_handle = fopen(output_file_name, "wb");
    if(output_file_handle == NULL)
    {
        fprintf(stderr, OUTPUT_FILE_OPEN_ERROR);
        exit(1);
    }
    password_sha256_compute(password, sha256_output);
    chacha20_state_block_init(chacha20_state_block, sha256_output);

    while(!feof(input_file_handle))
    {
        read_bytes = fread(file_read_block, sizeof(u8), sizeof(file_read_block), input_file_handle);
        if(ferror(input_file_handle))
        {
            fprintf(stderr, INPUT_FILE_READ_ERROR);
            exit(1);
        }
        gen_chacha20_xor_block(chacha20_state_block, chacha20_xor_block, block_counter_low, block_counter_high);
        block_counter_low = block_counter_low + 1u;
        if(block_counter_low == 0xFFFFFFFFu)
        {
            block_counter_high = block_counter_high + 1u;
            block_counter_low = 0u;
        }
        for(loop_var = 0u; loop_var < 64u; loop_var++)
        {
            file_read_block[loop_var] = file_read_block[loop_var] ^ (chacha20_xor_block[loop_var / 4u] >> ((loop_var % 4u) * 8u));
        }
        fwrite(file_read_block, sizeof(u8), read_bytes, output_file_handle);
        if(ferror(output_file_handle))
        {
            fprintf(stderr, OUTPUT_FILE_WRITE_ERROR);
            exit(1);
        }        
    }

    fclose(input_file_handle);
    fclose(output_file_handle);
}

int main(int argc, char** argv)
{
    u32     sha256_output[8u] =
    {   0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au, 0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u };
    u8      loop_var;

    if(argc >= 4)
    {
        file_chacha20_compute(argv[argc - 3u], argv[argc - 2u], argv[argc - 1u]);
    }
    else if(argc >= 2)
    {
        file_sha256_compute(argv[argc - 1u], sha256_output);
        printf("%s  -  ", argv[argc - 1u]);
        for(loop_var = 0u; loop_var < 8u; loop_var++)
        {
            printf("%08x", sha256_output[loop_var]);
        }
        printf("\n");
    }
    else
    {
        fprintf(stderr, TOO_FEW_ARGUMENTS_ERROR);
        exit(1);
    }

    return 0;
}

