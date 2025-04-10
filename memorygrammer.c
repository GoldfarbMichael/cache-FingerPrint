#include "memorygrammer.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Fisher-Yates shuffle to randomize node access order
static void shuffle(probe_node_t** array, size_t n) {
    if (n <= 1) return;
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        probe_node_t* tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}

int allocate_timing_arr(memorygrammer_t* mg, size_t num_samples) {
    if (!mg) return 0;
    mg->timings = calloc(num_samples, sizeof(double));
    if (!mg->timings) {
        perror("Failed to allocate timings array");
        return 0;
    }
    return 1;
}

/**
 * Allocates an array of probe nodes
 */
int allocate_nodes_arr(memorygrammer_t* mg, size_t num_nodes) {
    if (!mg) return 0;
    mg->nodes_arr = malloc(num_nodes * sizeof(probe_node_t*));
    if (!mg->nodes_arr) {
        perror("Failed to allocate node pointer array");
        return 0;
    }
    return 1;
}

/**
 * Allocates a buffer for each probe node (for each line)
 * node.next is at node[0]    =    *(probe_node_t**)
 * the rest of the allocated memory is just to fill the cache line
 */
int allocate_probe_nodes(cpu_config_t* config, memorygrammer_t* mg, size_t num_nodes) {
    if (!mg || !config) return 0;
    for (size_t i = 0; i < num_nodes; ++i) {
        probe_node_t* node = aligned_alloc(config->cache_line_size, config->cache_line_size); //allocate memory as the size of the line aligned to the line size
        if (!node) {
            perror("Failed to allocate probe_node");
            return 0;
        }
        node->next = NULL;
        mg->nodes_arr[i] = node;
    }
    return 1;
}



int init_memorygrammer(memorygrammer_t* mg, cpu_config_t* config, size_t num_samples) {
    if (!mg || !config) return 0;

    memset(mg, 0, sizeof(memorygrammer_t));
    mg->config = config;
    mg->num_samples = num_samples;

    // Calculate number of cache lines (nodes) needed to cover LLC
    size_t num_nodes = config->llc_size_bytes / config->cache_line_size;
    mg->num_nodes = num_nodes;

    if (!allocate_timing_arr(mg, num_samples)) {
        return 0;
    }

    if (!allocate_nodes_arr(mg, num_nodes)) {
        return 0;
    }

    if (!allocate_probe_nodes(config, mg, num_nodes)) {
        return 0;
    }

    // Shuffle node order to randomize traversal
    srand(time(NULL));
    shuffle(mg->nodes_arr, num_nodes);

    // Link nodes into a circular linked list
    for (size_t i = 0; i < num_nodes - 1; ++i) {
        mg->nodes_arr[i]->next = mg->nodes_arr[i + 1];
    }
    mg->nodes_arr[num_nodes - 1]->next = mg->nodes_arr[0]; // make it circular
    mg->head = mg->nodes_arr[0]; // set head to the first node

    return 1;
}


void run_probe(memorygrammer_t* mg, uint64_t interval_cycles) {
    if (!mg || !mg->head || !mg->timings) return;
    for (int i = 0; i < mg->num_samples; i++) {
        uint64_t t_start = rdtscp64();
        uint64_t t_target = t_start + interval_cycles;

        // Measure time to traverse the linked list
        uint64_t traverse_start = rdtscp64();
        volatile probe_node_t* curr = mg->head;
        for (size_t j = 0; j < mg->num_nodes; ++j) {
            curr = curr->next;
        }
        uint64_t traverse_end = rdtscp64();

        // Store number of cycles it took
        mg->timings[i] = (double)(traverse_end - traverse_start);

        // Busy-wait until next cycle window
        while (rdtscp64() < t_target);
    }
}

int write_timings_to_csv(memorygrammer_t* mg, const char* path) {
    if (!mg || !mg->timings || !path) return 0;

    FILE* f = fopen(path, "a");
    if (!f) {
        perror("Failed to open CSV file");
        return 0;
    }



    for (size_t i = 0; i < mg->num_samples; ++i) {
        fprintf(f, "%.0f\n", mg->timings[i]);
    }

    fclose(f);
    return 1;
}

void free_memorygrammer(memorygrammer_t* mg) {
    if (!mg) return;

    // Free each node individually
    if (mg->nodes_arr) {
        for (size_t i = 0; i < mg->num_nodes; ++i) {
            if (mg->nodes_arr[i]) {
                free(mg->nodes_arr[i]);
            }
        }
        free(mg->nodes_arr);
        mg->nodes_arr = NULL;
    }

    // Free timings array
    if (mg->timings) {
        free(mg->timings);
        mg->timings = NULL;
    }

    // Clear remaining fields
    mg->head = NULL;
    mg->num_nodes = 0;
    mg->num_samples = 0;
    mg->config = NULL;
}

