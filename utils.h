#pragma once
#include <cstdint>

inline uint64_t rdtsc() __attribute__((always_inline));
inline uint64_t rdtsc()
{
    uint64_t x;
    __asm__ volatile ("rdtsc\n\tshl $32, %%rdx\n\tor %%rdx, %%rax" : "=a" (x) : : "rdx");
    return x;
}