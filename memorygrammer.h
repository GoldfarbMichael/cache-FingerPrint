#ifndef MEMORYGRAMMER_H
#define MEMORYGRAMMER_H

#include <stddef.h>
#include "cpu-config.h"

/**
 * struct that represents a probe node.
 * each node represents a line in the cache set
 */
typedef struct probe_node {
    struct probe_node* next;
} probe_node_t;

typedef struct {
    cpu_config_t* config;       // Pointer to machine-specific config
    probe_node_t** nodes_arr;       // Array of all probe nodes
    size_t num_nodes;           // Number of nodes (cache lines)
    probe_node_t* head;         // Starting point for traversal (randomized)
    double* timings;            // Result timings in microseconds (or ns)
    size_t num_samples;         // Number of samples that exist in the timings array
} memorygrammer_t;


/**
 * Initialize the memorygrammer
 * Allocates all probe nodes, and links them randomly
 */
int init_memorygrammer(memorygrammer_t* mg, cpu_config_t* config);

/**
 * Records the time to probe every round into mg->timings[]
 *Traverses the linked list at fixed intervals, logs timing
 *interval_cycles: sampling interval in cycles
 */
void run_probe(memorygrammer_t* mg, uint64_t interval_cycles, uint64_t probe_cycles);

/**
 Write timings to a CSV file
*/
int write_timings_to_csv(memorygrammer_t* mg, const char* path);

/**
 *Clean up resources (timings, nodes)
 */
void free_memorygrammer(memorygrammer_t* mg);

int shuffle_linked_list(memorygrammer_t* mg, size_t num_nodes);

#endif //MEMORYGRAMMER_H
