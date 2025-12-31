# False Sharing Performance Analysis

## Experimental Setup

- **Workload**: Each thread increments its own counter 1 billion times (1e9)
- **Hardware**: CPU with 64-byte cache lines
- **Compiler**: GCC with -O2 optimization
- **Test Configurations**: 2 threads and 8 threads

### Hardware configurations
* **Hardware**
    * CPU: 13th Gen Intel(R) Core(TM) i7-13620H
    * Architecture: x86_64
    * Caches
        * L1d: 416 KiB (10 instances)
        * L1i: 448 KiB (10 instances)
        * L2: 9.5 MiB (7 instances)
        * L3: 24 MiB (1 instance)
    * Memory: 30 GB RAM (≈26 GB available), 14 GB swap 
    * Storage: 512GB SSD + 1TB SSD
* **Software**
    * Operating System: Ubuntu 24.04.3 LTS
    * Kernel: 6.14.0-37-generic
    * Compiler:
        * gcc: 13.3.0
        * g++: 13.3.0
    * Build tool:
        * GNU Make 4.3
### How to Reproduce?
* Download code: [source code](https://github.com/AnnTaiwan/computer_arch/tree/main/ca25_quiz6_problemX)
```c=
cd ca25_quiz6_problemX
make clean all

# run two version one times to compare
make run

# run several times to compare
make compare

# Test with different thread counts (2, 4, 8)
make test-threads

# Adjust thread count => compile and use 'time' to conduct experiment
make clean NTHREADS=2 run

# Use 'perf' to conduct experiment
make clean NTHREADS=2 perf_false_sharing
make clean NTHREADS=2 perf_non_false_sharing
```
### Code explanation
```c=
#ifdef PADDED
// Pad to cache line size (64 bytes) to avoid false sharing
struct {
    volatile uint64_t value; // 8 bytes
    uint8_t padding[56];  // 64 - 8 = 56 bytes padding
} counter[NTHREADS] __attribute__((aligned(64))); // Most modern CPUs have 64-byte cache lines (x86, ARM)
#else
// Adjacent counters will likely share a cache line (false sharing)
volatile uint64_t counter[NTHREADS]; //  8*NTHREADS bytes in sequence
#endif
```
The padded one will make sure the data stored in `counter` array is not in the same cache line.
And, then create `NTHREADS` threads to do the addition:
```c=
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
```
Create several threads to modify the variable `counter`.
```c=
// Create threads
    for (int i = 0; i < NTHREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
```
Actually, the threads are modifying the different parts of memory pointed by `counter`, but the false sharing issue will cause non-padding version poor performance.
* `__attribute__((aligned(64)))` is a GCC compiler directive that forces the structure to be aligned to a 64-byte boundary in memory.
    * Most modern CPUs have 64-byte cache lines (x86, ARM)
    * When data crosses cache line boundaries or shares a cache line with other thread's data, false sharing occurs
    * 64 bytes ensures each counter sits on its own cache line
* see my cpu's cache line size
    ```c
    $ cat /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size
    64
    ```
### **Use `time` to get statistic**
Use `time ./program` to test the time and cpu usage. 
#### stdout result
* Terminal result (**2 threads**: false sharing vs non false sharing)
    ```c
    $ make clean NTHREADS=2 run
    rm -f false_sharing no_false_sharing
    gcc -O2 -Wall -pthread -DNTHREADS=2 -o false_sharing false_sharing.c -pthread
    gcc -O2 -Wall -pthread -DNTHREADS=2 -DPADDED -o no_false_sharing false_sharing.c -pthread
    === Running with FALSE SHARING ===
    time ./false_sharing
    Results (FALSE SHARING, 2 threads):
    Counter[0]: 1000000000
    Counter[1]: 1000000000
    3.77user 0.00system 0:01.89elapsed 199%CPU (0avgtext+0avgdata 1560maxresident)k
    0inputs+0outputs (0major+80minor)pagefaults 0swaps

    === Running WITHOUT FALSE SHARING (padded) ===
    time ./no_false_sharing
    Results (PADDED - no false sharing, 2 threads):
    Counter[0]: 1000000000
    Counter[1]: 1000000000
    1.07user 0.00system 0:00.54elapsed 198%CPU (0avgtext+0avgdata 1496maxresident)k
    0inputs+0outputs (0major+80minor)pagefaults 0swaps
    ```
* Terminal result (**8 threads**: false sharing vs non false sharing)
    ```c
    $ make clean NTHREADS=8 run
    rm -f false_sharing no_false_sharing
    gcc -O2 -Wall -pthread -DNTHREADS=8 -o false_sharing false_sharing.c -pthread
    gcc -O2 -Wall -pthread -DNTHREADS=8 -DPADDED -o no_false_sharing false_sharing.c -pthread
    === Running with FALSE SHARING ===
    time ./false_sharing
    Results (FALSE SHARING, 8 threads):
    Counter[0]: 1000000000
    Counter[1]: 1000000000
    Counter[2]: 1000000000
    Counter[3]: 1000000000
    Counter[4]: 1000000000
    Counter[5]: 1000000000
    Counter[6]: 1000000000
    Counter[7]: 1000000000
    18.93user 0.00system 0:03.29elapsed 575%CPU (0avgtext+0avgdata 1376maxresident)k
    0inputs+0outputs (0major+93minor)pagefaults 0swaps

    === Running WITHOUT FALSE SHARING (padded) ===
    time ./no_false_sharing
    Results (PADDED - no false sharing, 8 threads):
    Counter[0]: 1000000000
    Counter[1]: 1000000000
    Counter[2]: 1000000000
    Counter[3]: 1000000000
    Counter[4]: 1000000000
    Counter[5]: 1000000000
    Counter[6]: 1000000000
    Counter[7]: 1000000000
    4.35user 0.00system 0:00.66elapsed 656%CPU (0avgtext+0avgdata 1624maxresident)k
    0inputs+0outputs (0major+93minor)pagefaults 0swaps
    ```

## Results Summary

### Time Metrics Explained

- **User Time**: Total CPU time spent executing user-space code across all threads (sum of all thread CPU times)
- **Elapsed Time**: Wall-clock time from start to finish (real time that passed)
- **CPU Usage**: (User Time / Elapsed Time) × 100% - Shows how many CPU cores are actively utilized
  - 200% = 2 cores fully utilized
  - 800% = 8 cores fully utilized
  - Lower than expected indicates threads are waiting (e.g., cache coherency, I/O)

### 2 Threads Performance

| Configuration | User Time | Elapsed Time | CPU Usage | Speedup |
|--------------|-----------|--------------|-----------|---------|
| **With False Sharing** | 3.77s | 1.89s | 199% | 1.0× (baseline) |
| **Without False Sharing (Padded)** | 1.07s | 0.54s | 198% | **3.5×** |

### 8 Threads Performance

| Configuration | User Time | Elapsed Time | CPU Usage | Speedup |
|--------------|-----------|--------------|-----------|---------|
| **With False Sharing** | 18.93s | 3.29s | 575% | 1.0× (baseline) |
| **Without False Sharing (Padded)** | 4.35s | 0.66s | 656% | **5.0×** |

## Key Observations

### 1. **Dramatic Performance Impact**

- **2 threads**: False sharing causes **3.5× slowdown** (1.89s vs 0.54s)
- **8 threads**: False sharing causes **5.0× slowdown** (3.29s vs 0.66s)
- The penalty **increases with more threads** due to increased cache coherency traffic

### 2. **Wasted CPU Cycles**

Look at the user time vs elapsed time ratio:

**2 Threads:**
- False sharing: 3.77s user / 1.89s elapsed = 2.0 (expected for 2 threads)
- Padded: 1.07s user / 0.54s elapsed = 2.0 (expected for 2 threads)

**8 Threads:**
- False sharing: 18.93s user / 3.29s elapsed = **5.8** (should be ~8!)
- Padded: 4.35s user / 0.66s elapsed = **6.6** (close to 8)

With false sharing, threads spend more time waiting for cache coherency, reducing parallel efficiency.

### 3. **Cache Coherency Overhead**

**Why is CPU% high but performance poor?**

The CPUs are 100% busy, but with false sharing they're doing **unproductive work**:
- Invalidating each other's cache lines
- Waiting for exclusive cache line ownership (MESI protocol)
- Memory barriers and synchronization overhead
- This still counts as "CPU time" but doesn't progress the computation

Think of it like a traffic jam: engines running (high CPU%), but nobody moving fast (slow elapsed time).

### 4. **Scaling Behavior**

**Without False Sharing (Ideal):**
- 2 threads: 0.54s
- 8 threads: 0.66s
- Nearly perfect scaling (slight overhead from thread management)

**With False Sharing (Poor Scaling):**
- 2 threads: 1.89s
- 8 threads: 3.29s
- Performance **degrades** with more threads due to cache line contention

## Technical Explanation

### What is False Sharing?

When multiple threads write to different variables that happen to reside on the **same cache line** (64 bytes on most modern CPUs), the CPU's cache coherency protocol (MESI) forces unnecessary synchronization:

```
Cache Line (64 bytes):
[counter[0]] [counter[1]] [counter[2]] ... [counter[7]]
    ↑            ↑            ↑
Thread 0     Thread 1     Thread 2

Each write invalidates the entire cache line for other cores!
```

### Cache Coherency Protocol (MESI)

1. Thread 0 writes to `counter[0]` → Cache line becomes **Modified** in Core 0
2. Thread 1 wants to write to `counter[1]` → Must invalidate Core 0's cache line
3. Core 0 must flush the line to memory
4. Core 1 loads the cache line and marks it **Modified**
5. Thread 0 wants to write again → Repeat from step 2

This "ping-pong" effect destroys performance!

### Solution: Cache Line Padding

```c
struct {
    volatile uint64_t value;  // 8 bytes
    uint8_t padding[56];      // 56 bytes padding
} counter[N] __attribute__((aligned(64)));
```

Each counter now occupies its own 64-byte cache line:
```
Cache Line 0: [counter[0].value] [padding...] (64 bytes)
Cache Line 1: [counter[1].value] [padding...] (64 bytes)
Cache Line 2: [counter[2].value] [padding...] (64 bytes)
...
```

No sharing → No false sharing!

## Performance Analysis

### Efficiency Metrics

**Parallel Efficiency = (Sequential Time) / (N × Parallel Time)**

Assuming sequential time ≈ 2 seconds (single-threaded baseline):

**8 Threads:**
- False sharing efficiency: 2.0 / (8 × 3.29) = **7.6%** 😱
- Padded efficiency: 2.0 / (8 × 0.66) = **37.9%** ✓

The padded version is **5× more efficient**!

### Cache Line Bouncing Frequency

With 1 billion increments per thread:
- **False sharing**: Each increment potentially causes cache invalidation
- **Estimated cache misses**: Hundreds of millions
- **Memory bus traffic**: Massive (18.93s user time with only computational work)



## Use `perf`
* system won't allow it to use `perf` defaultly.
  * Error message: [Doc](https://www.kernel.org/doc/html/v5.0/admin-guide/perf-security.html)
    ```c
    Error:
    Access to performance monitoring and observability operations is limited.
    Consider adjusting /proc/sys/kernel/perf_event_paranoid setting to open
    access to performance monitoring and observability operations for processes
    without CAP_PERFMON, CAP_SYS_PTRACE or CAP_SYS_ADMIN Linux capability.
    More information can be found at 'Perf events and tool security' document:
    https://www.kernel.org/doc/html/latest/admin-guide/perf-security.html
    perf_event_paranoid setting is 4:
      -1: Allow use of (almost) all events by all users
          Ignore mlock limit after perf_event_mlock_kb without CAP_IPC_LOCK
    >= 0: Disallow raw and ftrace function tracepoint access
    >= 1: Disallow CPU event access
    >= 2: Disallow kernel profiling
    To make the adjusted perf_event_paranoid setting permanent preserve it
    in /etc/sysctl.conf (e.g. kernel.perf_event_paranoid = <setting>)
    ```
  * See current setting
  ```
  cat /proc/sys/kernel/perf_event_paranoid
  ```
  * Set to 1 temporarily.
  ```c
  sudo sysctl -w kernel.perf_event_paranoid=1 # safe for user-space CPU events 
  ```
  * Do experiment
  ```
  perf stat ./false_sharing
  ```
## Complete `perf` result
* **false sharing - 2 threads**
```c
Results (FALSE SHARING, 2 threads):
Counter[0]: 1000000000
Counter[1]: 1000000000

 Performance counter stats for './false_sharing':

          6,641.67 msec task-clock                       #    1.998 CPUs utilized             
                44      context-switches                 #    6.625 /sec                      
                 5      cpu-migrations                   #    0.753 /sec                      
                68      page-faults                      #   10.238 /sec                      
    21,400,792,005      cpu_atom/instructions/           #    1.65  insn per cycle              (0.07%)
    18,056,009,040      cpu_core/instructions/           #    1.30  insn per cycle              (99.90%)
    12,971,286,497      cpu_atom/cycles/                 #    1.953 GHz                         (0.07%)
    13,897,302,659      cpu_core/cycles/                 #    2.092 GHz                         (99.90%)
     4,465,600,147      cpu_atom/branches/               #  672.361 M/sec                       (0.07%)
     4,009,400,425      cpu_core/branches/               #  603.674 M/sec                       (99.90%)
        11,638,865      cpu_atom/branch-misses/          #    0.26% of all branches             (0.10%)
            42,392      cpu_core/branch-misses/          #    0.00% of all branches             (99.90%)
             TopdownL1 (cpu_core)                 #     66.1 %  tma_backend_bound      
                                                  #      7.0 %  tma_bad_speculation    
                                                  #      2.7 %  tma_frontend_bound     
                                                  #     24.2 %  tma_retiring             (99.90%)
                                                  #     17.7 %  tma_bad_speculation    
                                                  #     33.1 %  tma_retiring             (0.10%)
                                                  #     39.4 %  tma_backend_bound      
                                                  #      9.9 %  tma_frontend_bound       (0.10%)

       3.324510024 seconds time elapsed

       6.641331000 seconds user
       0.002000000 seconds sys
```
* **non-false sharing - 2 threads**
```c
Results (PADDED - no false sharing, 2 threads):
Counter[0]: 1000000000
Counter[1]: 1000000000

 Performance counter stats for './no_false_sharing':

          1,870.22 msec task-clock                       #    1.927 CPUs utilized             
                19      context-switches                 #   10.159 /sec                      
                 4      cpu-migrations                   #    2.139 /sec                      
                67      page-faults                      #   35.825 /sec                      
    15,774,242,885      cpu_atom/instructions/           #    4.32  insn per cycle              (0.11%)
    18,021,014,641      cpu_core/instructions/           #    4.60  insn per cycle              (99.84%)
     3,652,159,376      cpu_atom/cycles/                 #    1.953 GHz                         (0.15%)
     3,913,707,147      cpu_core/cycles/                 #    2.093 GHz                         (99.84%)
     3,542,898,260      cpu_atom/branches/               #    1.894 G/sec                       (0.16%)
     4,003,737,477      cpu_core/branches/               #    2.141 G/sec                       (99.84%)
           670,705      cpu_atom/branch-misses/          #    0.02% of all branches             (0.16%)
            20,693      cpu_core/branch-misses/          #    0.00% of all branches             (99.84%)
             TopdownL1 (cpu_core)                 #      5.2 %  tma_backend_bound      
                                                  #      0.0 %  tma_bad_speculation    
                                                  #      9.5 %  tma_frontend_bound     
                                                  #     85.3 %  tma_retiring             (99.84%)
                                                  #      2.4 %  tma_bad_speculation    
                                                  #     87.6 %  tma_retiring             (0.16%)
                                                  #      1.5 %  tma_backend_bound      
                                                  #      8.5 %  tma_frontend_bound       (0.16%)

       0.970456566 seconds time elapsed

       1.869619000 seconds user
       0.002000000 seconds sys

```
* **false sharing - 4 threads**
```c
Results (FALSE SHARING, 4 threads):
Counter[0]: 1000000000
Counter[1]: 1000000000
Counter[2]: 1000000000
Counter[3]: 1000000000

 Performance counter stats for './false_sharing':

          7,499.81 msec task-clock                       #    3.866 CPUs utilized             
                49      context-switches                 #    6.533 /sec                      
                17      cpu-migrations                   #    2.267 /sec                      
                70      page-faults                      #    9.334 /sec                      
    13,762,672,048      cpu_atom/instructions/           #    0.79  insn per cycle              (0.39%)
    36,173,611,964      cpu_core/instructions/           #    1.28  insn per cycle              (99.54%)
    17,351,370,971      cpu_atom/cycles/                 #    2.314 GHz                         (0.39%)
    28,254,433,006      cpu_core/cycles/                 #    3.767 GHz                         (99.54%)
     3,060,179,743      cpu_atom/branches/               #  408.034 M/sec                       (0.39%)
     8,034,542,685      cpu_core/branches/               #    1.071 G/sec                       (99.54%)
           743,924      cpu_atom/branch-misses/          #    0.02% of all branches             (0.39%)
            93,714      cpu_core/branch-misses/          #    0.00% of all branches             (99.54%)
             TopdownL1 (cpu_core)                 #     69.6 %  tma_backend_bound      
                                                  #      4.3 %  tma_bad_speculation    
                                                  #      2.1 %  tma_frontend_bound     
                                                  #     23.9 %  tma_retiring             (99.54%)
                                                  #      0.1 %  tma_bad_speculation    
                                                  #     15.8 %  tma_retiring             (0.41%)
                                                  #     83.7 %  tma_backend_bound      
                                                  #      0.4 %  tma_frontend_bound       (0.40%)

       1.940139945 seconds time elapsed

       7.498968000 seconds user
       0.001999000 seconds sys

```
* **non-false sharing - 4 threads**
```c
Results (PADDED - no false sharing, 4 threads):
Counter[0]: 1000000000
Counter[1]: 1000000000
Counter[2]: 1000000000
Counter[3]: 1000000000

 Performance counter stats for './no_false_sharing':

          1,918.63 msec task-clock                       #    3.959 CPUs utilized             
                23      context-switches                 #   11.988 /sec                      
                 9      cpu-migrations                   #    4.691 /sec                      
                72      page-faults                      #   37.527 /sec                      
    16,282,611,170      cpu_atom/instructions/           #    4.36  insn per cycle              (0.18%)
    36,066,207,522      cpu_core/instructions/           #    5.01  insn per cycle              (99.76%)
     3,733,463,957      cpu_atom/cycles/                 #    1.946 GHz                         (0.18%)
     7,198,170,085      cpu_core/cycles/                 #    3.752 GHz                         (99.76%)
     3,628,844,576      cpu_atom/branches/               #    1.891 G/sec                       (0.19%)
     8,013,606,579      cpu_core/branches/               #    4.177 G/sec                       (99.76%)
           480,221      cpu_atom/branch-misses/          #    0.01% of all branches             (0.24%)
            33,128      cpu_core/branch-misses/          #    0.00% of all branches             (99.76%)
             TopdownL1 (cpu_core)                 #      0.2 %  tma_backend_bound      
                                                  #      0.0 %  tma_bad_speculation    
                                                  #      6.8 %  tma_frontend_bound     
                                                  #     93.0 %  tma_retiring             (99.76%)
                                                  #     -0.9 %  tma_bad_speculation    
                                                  #     87.5 %  tma_retiring             (0.24%)
                                                  #      4.4 %  tma_backend_bound      
                                                  #      8.9 %  tma_frontend_bound       (0.24%)

       0.484660594 seconds time elapsed

       1.918586000 seconds user
       0.001000000 seconds sys

```
* **false sharing - 8 threads**
```c
Results (FALSE SHARING, 8 threads):
Counter[0]: 1000000000
Counter[1]: 1000000000
Counter[2]: 1000000000
Counter[3]: 1000000000
Counter[4]: 1000000000
Counter[5]: 1000000000
Counter[6]: 1000000000
Counter[7]: 1000000000

 Performance counter stats for './false_sharing':

         21,704.81 msec task-clock                       #    5.482 CPUs utilized             
               156      context-switches                 #    7.187 /sec                      
                37      cpu-migrations                   #    1.705 /sec                      
                78      page-faults                      #    3.594 /sec                      
     5,853,407,050      cpu_atom/instructions/           #    0.10  insn per cycle              (17.50%)
    88,780,334,805      cpu_core/instructions/           #    1.08  insn per cycle              (80.00%)
    58,145,821,539      cpu_atom/cycles/                 #    2.679 GHz                         (17.50%)
    82,195,956,306      cpu_core/cycles/                 #    3.787 GHz                         (80.00%)
     1,291,095,039      cpu_atom/branches/               #   59.484 M/sec                       (17.50%)
    19,717,807,720      cpu_core/branches/               #  908.454 M/sec                       (80.00%)
           583,237      cpu_atom/branch-misses/          #    0.05% of all branches             (17.50%)
           273,129      cpu_core/branch-misses/          #    0.00% of all branches             (80.00%)
             TopdownL1 (cpu_core)                 #     76.0 %  tma_backend_bound      
                                                  #      3.1 %  tma_bad_speculation    
                                                  #      0.7 %  tma_frontend_bound     
                                                  #     20.2 %  tma_retiring             (80.00%)
                                                  #      2.6 %  tma_bad_speculation    
                                                  #      2.0 %  tma_retiring             (17.50%)
                                                  #     94.8 %  tma_backend_bound      
                                                  #      0.5 %  tma_frontend_bound       (17.51%)

       3.959622406 seconds time elapsed

      21.701641000 seconds user
       0.003999000 seconds sys
```
* **non-false sharing - 8 threads**
```c
Results (PADDED - no false sharing, 8 threads):
Counter[0]: 1000000000
Counter[1]: 1000000000
Counter[2]: 1000000000
Counter[3]: 1000000000
Counter[4]: 1000000000
Counter[5]: 1000000000
Counter[6]: 1000000000
Counter[7]: 1000000000

 Performance counter stats for './no_false_sharing':

          4,204.21 msec task-clock                       #    6.368 CPUs utilized             
                73      context-switches                 #   17.364 /sec                      
                18      cpu-migrations                   #    4.281 /sec                      
                80      page-faults                      #   19.029 /sec                      
    49,828,894,258      cpu_atom/instructions/           #    4.45  insn per cycle              (21.00%)
    79,064,747,383      cpu_core/instructions/           #    4.99  insn per cycle              (75.95%)
    11,195,174,863      cpu_atom/cycles/                 #    2.663 GHz                         (21.01%)
    15,841,474,614      cpu_core/cycles/                 #    3.768 GHz                         (75.95%)
    11,081,399,626      cpu_atom/branches/               #    2.636 G/sec                       (21.03%)
    17,567,642,735      cpu_core/branches/               #    4.179 G/sec                       (75.95%)
           128,492      cpu_atom/branch-misses/          #    0.00% of all branches             (21.04%)
            71,330      cpu_core/branch-misses/          #    0.00% of all branches             (75.95%)
             TopdownL1 (cpu_core)                 #      0.2 %  tma_backend_bound      
                                                  #      0.0 %  tma_bad_speculation    
                                                  #      6.8 %  tma_frontend_bound     
                                                  #     93.1 %  tma_retiring             (75.95%)
                                                  #      0.2 %  tma_bad_speculation    
                                                  #     89.0 %  tma_retiring             (21.07%)
                                                  #      1.2 %  tma_backend_bound      
                                                  #      9.5 %  tma_frontend_bound       (21.08%)

       0.660257369 seconds time elapsed

       4.205806000 seconds user
       0.000000000 seconds sys
```
#### `perf` Results Summary
##### Key Metrics
| Metric              | Meaning                                                  |
| ------------------- | -------------------------------------------------------- |
| **Elapsed time**    | Wall-clock runtime (what users feel)                     |
| **CPU core cycles** | Total cycles spent on big cores                          |
| **CPU core IPC**    | Instructions per cycle (efficiency)                      |
| **Backend bound**   | % of time stalled waiting for memory / cache / coherence :arrow_right: It is a key metric to see the effect of false-sharing which needs to do a lot of cache coherence work. **Backend bound means: The CPU pipeline is ready, but execution cannot proceed because data is not available.**|
| **Retiring**        | % of cycles doing useful work                            |
* The instruction count among non-false/false sharing is similar.
    * False sharing does not change instruction count, it explodes cycle count.
    * Hence, the IPC will exit a gap.
##### 2 Threads
| Metric          | False Sharing | No False Sharing | Explanation                               |
| --------------- | ------------- | ---------------- | ----------------------------------------- |
| Elapsed time    | **3.32 s**    | **0.97 s**       | False sharing causes cache line ping-pong |
| CPU core cycles | **13.9B**     | **3.9B**         | ~3.6× more cycles wasted                  |
| CPU core IPC    | **1.30**      | **4.60**         | Backend stalls kill IPC                   |
| Backend bound   | **66.1%**     | **5.2%**         | Cache coherence dominates                 |
| Retiring        | **24.2%**     | **85.3%**        | CPU mostly idle vs productive             |

##### 4 Threads
| Metric          | False Sharing | No False Sharing | Explanation                       |
| --------------- | ------------- | ---------------- | --------------------------------- |
| Elapsed time    | **1.94 s**    | **0.48 s**       | Scaling breaks with false sharing |
| CPU core cycles | **28.3B**     | **7.2B**         | 4× extra cycles                   |
| CPU core IPC    | **1.28**      | **5.01**         | Near peak IPC without sharing     |
| Backend bound   | **69.6%**     | **0.2%**         | Severe cache-line bouncing        |
| Retiring        | **23.9%**     | **93.0%**        | Almost perfect execution          |

##### 8 Threads
| Metric          | False Sharing | No False Sharing | Explanation                     |
| --------------- | ------------- | ---------------- | ------------------------------- |
| Elapsed time    | **3.96 s**    | **0.66 s**       | More threads → worse contention |
| CPU core cycles | **82.2B**     | **15.8B**        | ~5× wasted cycles               |
| CPU core IPC    | **1.08**      | **4.99**         | Pipeline starved by memory      |
| Backend bound   | **76.0%**     | **0.2%**         | Coherence traffic explodes      |
| Retiring        | **20.2%**     | **93.1%**        | CPU mostly waiting              |

#### Observations
As thread count increases, false sharing significantly increases backend-bound cycles due to cache coherence traffic, leading to higher CPU cycles, lower IPC, and worse elapsed time. In contrast, the no false sharing version maintains high IPC and retiring rates, demonstrating near-ideal scalability.


## Recommendations

### For Performance-Critical Code:

1. **Identify hot paths** - Use profiling tools (perf, VTune)
2. **Align thread-local data** - Use `__attribute__((aligned(64)))`
3. **Add padding** - Ensure each thread's data is on separate cache lines
4. **Measure cache misses** - Use `perf stat -e cache-misses,cache-references`
5. **Test scalability** - Verify performance improves with more threads

### Example `perf` Command:

```bash
perf stat -e cache-misses,cache-references,L1-dcache-load-misses ./false_sharing
perf stat -e cache-misses,cache-references,L1-dcache-load-misses ./no_false_sharing
```

Compare cache miss rates to confirm false sharing elimination!
## Conclusions

1. **False sharing is a silent killer** - High CPU usage but poor performance
2. **Impact scales with thread count** - More threads = worse false sharing penalty
3. **Padding is essential** for multi-threaded performance on shared cache line boundaries
4. **Always align hot data structures** to cache line boundaries in concurrent code
5. **Monitor elapsed time, not just CPU%** - CPU can be busy doing unproductive work

## References

- Cache line size: 64 bytes (x86-64, ARM Cortex-A)
- MESI Protocol: Modified-Exclusive-Shared-Invalid
- Alignment: `__attribute__((aligned(N)))` where N is power of 2
- [perf usage 1](https://blog.eastonman.com/blog/2021/02/use-perf/)
- [perf usage 2](https://blog.csdn.net/Summer0828/article/details/142208327)
