#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#ifndef NTHREADS
#define NTHREADS 2
#endif

#ifdef PADDED
// Pad to cache line size (64 bytes) to avoid false sharing
struct {
    volatile uint64_t value; // 8 bytes
    uint8_t padding[56];  // 64 - 8 = 56 bytes padding
} counter[NTHREADS] __attribute__((aligned(64))); // Most modern CPUs have 64-byte cache lines (x86, ARM)
#else
// Adjacent counters will likely share a cache line (false sharing)
volatile uint64_t counter[NTHREADS];
#endif

void* thread_func(void* arg) {
    int id = *(int*)arg;
    for (uint64_t i = 0; i < 1e9; i++)
#ifdef PADDED
        counter[id].value++;
#else
        counter[id]++;
#endif
    return NULL;
}

int main() {
    pthread_t threads[NTHREADS];
    int thread_ids[NTHREADS];
    
    // Create threads
    for (int i = 0; i < NTHREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Print results
#ifdef PADDED
    printf("Results (PADDED - no false sharing, %d threads):\n", NTHREADS);
#else
    printf("Results (FALSE SHARING, %d threads):\n", NTHREADS);
#endif
    
    for (int i = 0; i < NTHREADS; i++) {
#ifdef PADDED
        printf("Counter[%d]: %lu\n", i, counter[i].value);
#else
        printf("Counter[%d]: %lu\n", i, counter[i]);
#endif
    }
    
    return 0;
}
