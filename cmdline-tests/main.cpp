#include <stdio.h>
#include <sys/timer.h>

extern "C" int _sys_printf(const char* fmt, ...);

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

int main(int argc, char** argv)
{
    _sys_printf("lv2 printf\n");
    printf("argc %d\n", argc);
    printf("argv %p\n", argv);
    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d]: %s\n", i, argv[i]);
    }
    hex_dump(argv, 128, (uintptr_t)argv);
    while (1)
    {
        //printf("cmdline-tests started\n");
        sys_timer_sleep(1);
    }
    return 0;
}
