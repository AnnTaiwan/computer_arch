# False Sharing Performance Analysis

## Experimental Setup

- **Workload**: Each thread increments its own counter 1 billion times (1e9)
- **Hardware**: CPU with 64-byte cache lines
- **Compiler**: GCC with -O2 optimization
- **Test Configurations**: 2 threads and 8 threads

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

## Conclusions

1. **False sharing is a silent killer** - High CPU usage but poor performance
2. **Impact scales with thread count** - More threads = worse false sharing penalty
3. **Padding is essential** for multi-threaded performance on shared cache line boundaries
4. **Always align hot data structures** to cache line boundaries in concurrent code
5. **Monitor elapsed time, not just CPU%** - CPU can be busy doing unproductive work

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

## References

- Cache line size: 64 bytes (x86-64, ARM Cortex-A)
- MESI Protocol: Modified-Exclusive-Shared-Invalid
- Alignment: `__attribute__((aligned(N)))` where N is power of 2
