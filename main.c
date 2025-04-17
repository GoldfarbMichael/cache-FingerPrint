#define _GNU_SOURCE
#include "cpu-config.h"
#include "memorygrammer.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <time.h>
#define INTERVAL_NORMALIZER 1000 // 1 -> 1sec | 1000 -> 1ms | 1000000 -> microSec
#define PROBE_TIME_SEC 5 // probe time in seconds
#define INTERVAL_PROBE_MS 2 // the interval time in ms
#define DUMMY_TIME_SEC 1
#define DUMMY_PROBE_MS 1

void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("sched_setaffinity");
    }
}

int open_website(const char* url) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return 0;
    }

    if (pid == 0) {
        setpgid(0, 0);
        // Child process: open browser in incognito mode
        pin_to_core(2);
        execlp("google-chrome", "google-chrome",
              "--new-window",
              url, NULL);
        perror("execlp failed"); // if execlp returns
        exit(EXIT_FAILURE);
    }

    // Parent continues
    return pid; // return child PID
}

int heat_cache(memorygrammer_t* mg, const uint64_t intervalCycles, const uint64_t probe_cycles) {
    pid_t browser_pid = open_website("https://www.google.co.il/");
    if (browser_pid == 0) {
        free_memorygrammer(mg);
        return EXIT_FAILURE;
    }
    dummy_probe(mg, intervalCycles, probe_cycles);
    // Close the browser
    kill(-browser_pid, SIGKILL);
    waitpid(browser_pid, NULL, 0);
    return EXIT_SUCCESS;
}



int collect_data(memorygrammer_t* mg, const uint64_t intervalCycles, const uint64_t probeCycles, const char* url, int round) {
    //parse the site name from the URL
    char site_name[128];
    parse_site_name(url, site_name, sizeof(site_name));
    //print the site name
    printf("Probing site: %s\n", site_name);
    pid_t browser_pid = open_website(url);
    if (browser_pid == 0) {
        free_memorygrammer(mg);
        return EXIT_FAILURE;
    }
    printf("Probing...\n");
    run_probe(mg, intervalCycles, probeCycles);
    printf("Done.\n");

    kill(-browser_pid, SIGKILL);
    waitpid(browser_pid, NULL, 0);

    // Write results to CSV
    char csv_path[256];
    snprintf(csv_path, sizeof(csv_path), "%s.csv", site_name);
    if (!write_timings_to_csv(mg, csv_path)) {
        fprintf(stderr, "Failed to write CSV output.\n");
        free_memorygrammer(mg);
        return EXIT_FAILURE;
    }
    printf("Results written to: %s\n", csv_path);
    return EXIT_SUCCESS;
}


int main(int argc, char *argv[]) {
    pin_to_core(0);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    cpu_config_t config;
    if (detect_cpu_config(&config) != 1) {
        fprintf(stderr, "Failed to detect CPU configuration\n");
        return 1;
    }
    const char* urlWiki = "https://www.wikipedia.org";
    const char* urlBBC = "https://www.bbc.com/";
    char site1[128];
    char site2[128];
    char dummySite[128];
    parse_site_name(urlWiki, site1, sizeof(site1));
    parse_site_name(urlBBC, site2, sizeof(site2));
    parse_site_name("https://www.google.co.il/", dummySite, sizeof(dummySite));
    empty_csv(site1);
    empty_csv(site2);
    empty_csv(dummySite);



    print_cpu_config(&config);
    const uint32_t clockSpeed = get_clock_speed_hz(&config);
    uint64_t intervalCycles = (uint64_t)(clockSpeed / INTERVAL_NORMALIZER)* INTERVAL_PROBE_MS;
    uint64_t probeCycles = ((uint64_t)clockSpeed * PROBE_TIME_SEC);


    // Configure memorygrammer
    memorygrammer_t mg;

    if (!init_memorygrammer(&mg, &config)) {
        fprintf(stderr, "Failed to initialize memorygrammer.\n");
        return EXIT_FAILURE;
    }
    // heat_cache(&mg, intervalCycles, probeCycles);
    collect_data(&mg, intervalCycles, probeCycles, "https://www.google.co.il/", 0);

    // collect_data(&mg, intervalCycles, probeCycles, urlBBC, 0);
    for (int i = 0; i < 50; i++) {
        collect_data(&mg, intervalCycles, probeCycles, urlWiki, i);
    }
    collect_data(&mg, intervalCycles, probeCycles, urlWiki, 0);

    // collect_data(&mg, intervalCycles, probeCycles, urlWiki, 0);
    for (int i = 0; i < 50; i++) {
        collect_data(&mg, intervalCycles, probeCycles, urlBBC, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    // Calculate elapsed time in seconds and nanoseconds
    double elapsed_time = (end.tv_sec - start.tv_sec) +
                          (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Execution time: %.9f seconds\n", elapsed_time);

    // Cleanup
    free_memorygrammer(&mg);


    return EXIT_SUCCESS;

}