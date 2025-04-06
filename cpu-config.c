#include "cpu-config.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MB_NORMALIZER 1048576 //2^20
#define KB_NORMALIZER 1024 //2^10

int detect_cores(cpu_config_t* config) {
    if (!config) return 0;

    config->num_logical_processors = sysconf(_SC_NPROCESSORS_ONLN);
    FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
    if (!cpuinfo) return 0;

    char line[256];
    double ghz = 0.0;

    while (fgets(line, sizeof(line), cpuinfo)) {
        if (strncmp(line, "model name", 10) == 0){
            sscanf(line, "model name : %127[^\n]", config->model_name);

            // Try to extract clock from model name string
            // Look for "@ <float>GHz"
            char* at_ptr = strstr(config->model_name, "@ ");
            if (at_ptr != NULL) {
                double ghz_temp = 0.0;
                if (sscanf(at_ptr, "@ %lfGHz", &ghz_temp) == 1) {
                    ghz = ghz_temp;
                }
            }
        }
    }

    fclose(cpuinfo);
    config->clock_speed_hz = ghz * 1e9; // Convert GHz to Hz
    return 1;
}

int detect_LLC_size(cpu_config_t* config, FILE* memInfo) {
    if (!config) return 0;
    memInfo = fopen("/sys/devices/system/cpu/cpu0/cache/index3/size", "r");
    if (memInfo) {
        int size_kb;
        fscanf(memInfo, "%dK", &size_kb);
        config->llc_size_bytes = size_kb * KB_NORMALIZER;
        fclose(memInfo);
        return 1;
    }
    return 0;
}

/**
 * Assumes that if it doesn't find the file so the line size is 64 bytes.
 */
void detect_line_size(cpu_config_t* config, FILE* memInfo) {
    if (!config) return;

    memInfo = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
    if (memInfo) {
        int line_size;
        fscanf(memInfo, "%d", &line_size);
        config->cache_line_size = (size_t)line_size;
        fclose(memInfo);
    } else {
        config->cache_line_size = 64 * KB_NORMALIZER; // Default to 64 bytes
    }
}

int check_hyperthreading(cpu_config_t* config, FILE* memInfo) {
    if (!config) return 0;
    memInfo = fopen("/sys/devices/system/cpu/smt/active", "r");
    if (memInfo) {
        int active;
        if (fscanf(memInfo, "%d", &active) == 1) {
            config->has_hyperthreading = active;
        }
        fclose(memInfo);
        return 1;
    }
    return 0;
}

int detect_associativity(cpu_config_t* config, FILE* memInfo) {
    if (!config) return 0;
    memInfo = fopen("/sys/devices/system/cpu/cpu0/cache/index3/ways_of_associativity", "r");
    if (memInfo) {
        fscanf(memInfo, "%d", &config->llc_associativity);
        fclose(memInfo);
        return 1;
    }
    return 0;
}

int check_huge_pages(cpu_config_t* config, FILE* memInfo) {
    if (!config) return 0;
    memInfo = fopen("/proc/meminfo", "r");
    if (memInfo) {
        char label[64];
        int value;
        char unit[16];
        while (fscanf(memInfo, "%63s %d %15s\n", label, &value, unit) == 3) {
            if (strcmp(label, "HugePages_Total:") == 0)
                config->hugepages_total = value;
            else if (strcmp(label, "HugePages_Free:") == 0)
                config->hugepages_free = value;
            else if (strcmp(label, "Hugepagesize:") == 0)
                config->hugepage_size_kb = (size_t)value;
        }
        fclose(memInfo);
        return 1;
    }
    return 0;
}


int detect_cpu_config(cpu_config_t* config) {
    if (!config) return 0;
    memset(config, 0, sizeof(cpu_config_t));
    FILE* memInfo = NULL;

    if (!detect_cores(config)) {
        printf("Detecting cores failed\n");
    }

    if (!detect_LLC_size(config, memInfo)) {
        printf("Failed to detect LLC size\n");
    }

    detect_line_size(config, memInfo);

    if (!check_hyperthreading(config, memInfo)) {
        printf("Failed to check hyperthreading\n");
    }

    if (!detect_associativity(config, memInfo)) {
        printf("Failed to detect associativity\n");
    }

    if (!check_huge_pages(config, memInfo)) {
        printf("Failed to check huge pages\n");
    }

    size_t sliceSize = (config->llc_size_bytes)/(config->num_logical_processors);
    size_t linesInSlice = sliceSize/config->cache_line_size;
    config->sets_per_slice = linesInSlice/config->llc_associativity;

    return 1;

}

void print_cpu_config(const cpu_config_t* config) {
    if (!config) return;
    printf("Model: %s\n", config->model_name);
    printf("Logical CPUs: %d\n", config->num_logical_processors);
    printf("Hyper-Threading: %s\n", config->has_hyperthreading ? "Enabled" : "Disabled");
    printf("LLC Size: %zu MB\n", config->llc_size_bytes/MB_NORMALIZER);
    printf("Cache Line Size: %zu bytes\n", config->cache_line_size);
    if (config->llc_associativity > 0) {
        printf("LLC Associativity: %d-way\n", config->llc_associativity);
    } else {
        printf("LLC Associativity: Unknown\n");
    }
    printf("Sets per slice: %zu sets\n", config->sets_per_slice);
    if (config->hugepages_total > 0) {
        printf("Hugepages Enabled:  Yes (Total: %d, Free: %d, Size: %zu KB)\n",
               config->hugepages_total, config->hugepages_free, config->hugepage_size_kb);
    } else {
        printf("Hugepages Enabled:  No\n");
    }
}

int get_num_of_slices(const cpu_config_t* config) {
    if (!config) return 0;
    return config->num_logical_processors;
}

size_t get_sets_per_slice(const cpu_config_t* config) {
    if (!config) return 0;
    return config->sets_per_slice;
}

int get_associativity(const cpu_config_t* config) {
    if (!config) return 0;
    return config->llc_associativity;
}
