#ifndef CPU_CONFIG_H
#define CPU_CONFIG_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t llc_size_bytes;
    size_t cache_line_size;
    int llc_associativity;
    int num_logical_processors;
    size_t sets_per_slice;
    char model_name[128];
    int has_hyperthreading;
    uint32_t clock_speed_hz;
    // Huge page info
    int hugepages_total;
    int hugepages_free;
    size_t hugepage_size_kb;
} cpu_config_t;

int detect_cpu_config(cpu_config_t* config);
void print_cpu_config(const cpu_config_t* config);
int get_num_of_slices(const cpu_config_t* config);
size_t get_sets_per_slice(const cpu_config_t* config);
int get_associativity(const cpu_config_t* config);


#endif
