#include "cpu-config.h"
#include "memorygrammer.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define INTERVAL_NORMALIZER 1000 // 1 -> 1sec | 1000 -> 1ms | 1000000 -> microSec
#define PROBE_TIME_SEC 20 // probe time in seconds
#define INTERVAL_PROBE_MS 2 // the interval time in ms

int open_website(const char* url) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return 0;
    }

    if (pid == 0) {
        setpgid(0, 0);
        // Child process: open browser in incognito mode
        execlp("google-chrome", "google-chrome",
              "--incognito",
              "--new-window",
              "--no-sandbox",
              "--disable-popup-blocking",
              url, NULL);
        perror("execlp failed"); // if execlp returns
        exit(EXIT_FAILURE);
    }

    // Parent continues
    return pid; // return child PID
}



int main(int argc, char *argv[]) {
    cpu_config_t config;
    if (detect_cpu_config(&config) != 1) {
        fprintf(stderr, "Failed to detect CPU configuration\n");
        return 1;
    }

    print_cpu_config(&config);
    const uint32_t clockSpeed = get_clock_speed_hz(&config);
    uint64_t intervalCycles = (uint64_t)(clockSpeed / INTERVAL_NORMALIZER)* INTERVAL_PROBE_MS;
    uint64_t probeCycles = (clockSpeed * PROBE_TIME_SEC);

    // Configure memorygrammer
    memorygrammer_t mg;


    if (!init_memorygrammer(&mg, &config, 100)) {
        fprintf(stderr, "Failed to initialize memorygrammer.\n");
        return EXIT_FAILURE;
    }

        const char* url = "https://www.wikipedia.org";
        pid_t browser_pid = open_website(url);
        if (browser_pid == 0) {
            free_memorygrammer(&mg);
            return EXIT_FAILURE;
        }
    uint64_t probeStart = rdtscp64();
    printf("StartProbe %lu\n", probeStart);

    printf("Probing...\n");
    run_probe(&mg, intervalCycles);
    printf("Done.\n");
    printf("End time   : %lu\n", rdtscp64());
    printf("Target Time: %lu\n", probeCycles+probeStart);



        kill(-browser_pid, SIGKILL);
        waitpid(browser_pid, NULL, 0);


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

}