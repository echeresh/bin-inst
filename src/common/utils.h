#pragma once
#include <cstdint>
#include <chrono>

inline uint64_t rdtsc() __attribute__((always_inline));
inline uint64_t rdtsc()
{
    uint64_t x;
    __asm__ volatile ("rdtsc\n\tshl $32, %%rdx\n\tor %%rdx, %%rax" : "=a" (x) : : "rdx");
    return x;
}

inline double dsecnd()
{
    return std::chrono::duration<double, std::ratio<1, 1>>
        (std::chrono::steady_clock::now() - std::chrono::steady_clock::time_point()).count();
}
