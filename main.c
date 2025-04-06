#include "cpu-config.h"
#include "memorygrammer.h"
#include "utils.h"  // assuming rdtscp64() is here
#include <stdio.h>
#include <stdlib.h>

#define INTERVAL_NORMALIZER 1000 // 1 -> 1sec | 1000 -> 1ms | 1000000 -> microSec

int main(int argc, char *argv[]) {
    cpu_config_t config;
    if (detect_cpu_config(&config) != 1) {
        fprintf(stderr, "Failed to detect CPU configuration\n");
        return 1;
    }

    print_cpu_config(&config);

    // Configure memorygrammer
    memorygrammer_t mg;
    size_t num_samples = 10;

    if (!init_memorygrammer(&mg, &config, num_samples)) {
        fprintf(stderr, "Failed to initialize memorygrammer.\n");
        return EXIT_FAILURE;
    }

    // Calculate probe interval in cycles (e.g., 1ms)
    uint64_t interval_cycles = (uint64_t)(config.clock_speed_hz / INTERVAL_NORMALIZER);

    printf("Probing...\n");
    run_probe(&mg, interval_cycles, INTERVAL_NORMALIZER);
    printf("Done.\n");

    // Write results to CSV
    const char* csv_path = "memorygram_output.csv";
    if (!write_timings_to_csv(&mg, csv_path)) {
        fprintf(stderr, "Failed to write CSV output.\n");
        free_memorygrammer(&mg);
        return EXIT_FAILURE;
    }

    printf("Results written to: %s\n", csv_path);

    // Cleanup
    free_memorygrammer(&mg);
    return EXIT_SUCCESS;

    return 0;
}