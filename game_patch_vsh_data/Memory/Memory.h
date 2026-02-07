#pragma once
#include <stdint.h>

#if defined(__cplusplus)
extern "C"
{
#endif

int WriteProcessMemory(uint32_t pid, void* address, const void* data, size_t size);
int ReadProcessMemory(uint32_t pid, void* address, void* data, size_t size);

#if defined(__cplusplus)
}
#endif
