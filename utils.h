#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>
#include <stdint.h>

static inline uint64_t rdtscp64() {
    uint32_t low, high;
    asm volatile ("rdtscp": "=a" (low), "=d" (high) :: "ecx");
    return (((uint64_t)high) << 32) | low;
}
void parse_site_name(const char* url, char* site_name, size_t size);
void empty_csv(const char* site_name);

#endif //UTILS_H
