#include <stdint.h>

typedef struct
{
    uintptr_t addr;
    uintptr_t toc;
} function_descriptor;

#if defined(__cplusplus)
extern "C"
{
#endif
void patch_call_(const uintptr_t src, const function_descriptor* dst, const function_descriptor* original, const char* desc);
void hex_dump(const void* data, const uint64_t size, const uintptr_t real);

#if defined(__cplusplus)
}
#endif

#define nstr(n) #n
#define nnstr(n) nstr(n)

#define patch_call(s, d, o) patch_call_(s, (function_descriptor*)d, (function_descriptor*)o, __FILE__ ", " nnstr(__LINE__) ": " #s ", " #d ", " #o)
