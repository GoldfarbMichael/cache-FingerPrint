#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>

static inline uint64_t rdtscp64() {
    uint32_t low, high;
    asm volatile ("rdtscp": "=a" (low), "=d" (high) :: "ecx");
    return (((uint64_t)high) << 32) | low;
}


#endif //UTILS_H
