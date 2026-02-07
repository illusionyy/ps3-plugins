#if defined(_USRPRX)
#include "../game_patch/lv2_stdio.h"
#else
#include <vshlib.hpp>
#endif
#include "../game_patch_vsh_data/Memory/Memory.h"
#include <sys/process.h>
#include <stdint.h>

#include "memory.h"

#if !defined(printf)
#define printf vsh::printf
#endif

#define MAKE_JUMP_VALUE(addr, to) (((0x12 << 26) | ((((to - (uint64_t)(addr)) >> 2) & 0xFFFFFF) << 2)))
#define MAKE_CALL_VALUE(addr, to) (((0x12 << 26) | ((((to - (uint64_t)(addr)) >> 2) & 0xFFFFFF) << 2)) | 1)

#define READ_JUMP_OFFSET(value) ((int32_t)(((value) & 0x03FFFFFC) << 6) >> 6)
#define READ_CALL_OFFSET(value) ((int32_t)((((value) & ~1) & 0x03FFFFFC) << 6) >> 6)

#if defined(__cplusplus)
extern "C"
{
#endif
    void patch_call_(const uintptr_t src, const function_descriptor* dst, const function_descriptor* original, const char* desc)
{
    printf("%s src 0x%x to dst 0x%x toc 0x%0x\n", desc, src, dst->addr, dst->toc);
    const uint32_t bl_to = MAKE_CALL_VALUE(src, dst->addr);
    const uint32_t vsh_pid = sys_process_getpid();
    uint32_t bl_original = 0;
    ReadProcessMemory(vsh_pid, (void*)src, &bl_original, sizeof(bl_original));
    if (original && original->addr)
    {
        const uintptr_t original_call = src + READ_CALL_OFFSET(bl_original);
        const uint32_t bl_to_original = MAKE_JUMP_VALUE(original->addr, original_call);
        WriteProcessMemory(vsh_pid, (void*)original->addr, &bl_to_original, sizeof(bl_to_original));
        printf("%s has original call to 0x%x\n", desc, original_call);
    }
    WriteProcessMemory(vsh_pid, (void*)src, &bl_to, sizeof(bl_to));
}

static int isprint(int c)
{
    return ((unsigned)c - 0x20) < 0x5f;
}

void hex_dump(const void* data, const uint64_t size, const uintptr_t real)
{
    if (real)
    {
        printf("offset: %lx\n", real);
    }
    const unsigned char* p = (const unsigned char*)data;
    uintptr_t i = 0, j = 0;

    for (i = 0; i < size; i += 16)
    {
        printf("%08x: ", real + i);  // Print address offset

        for (j = 0; j < 16; j++)
        {
            if (i + j < size)
            {
                printf("%02x ", p[i + j]);  // Print hex value
            }
            else
            {
                printf("   ");  // Pad with spaces if less than 16 bytes
            }
        }

        printf("  |");

        for (j = 0; j < 16; j++)
        {
            if (i + j < size)
            {
                if (isprint(p[i + j]))
                {
                    printf("%c", p[i + j]);  // Print printable ASCII characters
                }
                else
                {
                    printf(".");  // Replace non-printable characters with '.'
                }
            }
        }

        printf("|\n");
    }
}

#if (__cplusplus)
}
#endif
