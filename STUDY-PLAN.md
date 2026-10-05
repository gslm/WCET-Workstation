# WCET Study Plan

Goal: learn WCET (Worst-Case Execution Time) core concepts, hardware-agnostic, before joining an aircraft software project whose target processor is not yet known.

## 1. Orientation

**What WCET is.** A guaranteed upper bound on the execution time of a piece of code (usually a task) on a specific hardware platform, over all inputs and all initial hardware states. It feeds schedulability analysis (proving every task meets its deadline).

```
 BCET      typical      observed max     true WCET     WCET bound
  |-----------|-------------|-----------------|-------------|
                            ^ what testing sees           ^ what analysis claims
```

- Bound must be **safe** (never below the true WCET) and **tight** (not too pessimistic).

**Why it is hard.**
1. Paths: time depends on which branches and loop iterations inputs trigger. Needs loop bounds and exclusion of infeasible paths.
2. Hardware state: caches, pipelines, branch predictors, DRAM refresh, bus contention, other cores. Timing anomalies: a local speed-up can cause a global slow-down.

**Method families.**
1. Static analysis: models the processor, analyses the binary without running it. Safe bound, needs a precise hardware model. Example tool: AbsInt aiT.
2. Measurement-based: run under stress, take the observed maximum plus a margin. Simple, but not proven safe; only as good as test coverage.
3. Hybrid: measure small blocks on real hardware, combine them along the worst path. Common in industry. Example tool: Rapita RVS.
4. Probabilistic (MBPTA): extreme-value statistics on measurements. Mostly research or niche.

**Certification context.**
- DO-178C (airborne software): verification includes timing analysis, including worst-case execution time.
- Multicore: FAA AC 20-193 / EASA AMC 20-193 (successors of CAST-32A) add interference requirements. Currently the hardest area.

**Note on this platform.** A general-purpose Linux workstation (x86, out-of-order, frequency scaling, SMT, interrupts) is a poor platform for safe WCET; only measurement-based work is possible there. Good for learning concepts, not how certified bounds are produced.

## 2. Platform decision

- **This Linux workstation, in C**: measurement, paths, cache effects. Noisy; mitigate with CPU pinning, fixed frequency governor, isolated cores.
- **QEMU**: not for timing (no cache/pipeline model). Exception: `-icount` mode gives deterministic instruction counts, useful for path analysis.
- **OTAWA** (open-source static WCET tool, supports ARM): used when reaching static analysis, to compare its bound against measurements.
- **MCU boards** (optional, later): Cortex-M with a cycle counter is close to real practice; a dual-core part (e.g. STM32H755) can show multicore interference.

## 3. Core concepts

| # | Concept | Coding example | Status |
|---|---|---|---|
| **Foundations** | | | |
| 1 | Execution time as a distribution: BCET/ACET/WCET, observed max vs bound, safe vs tight | Time one function over many inputs, plot a histogram | Next |
| 2 | Measuring correctly: timers, overhead, warm-up, noise | `clock_gettime` vs `rdtsc`, measure an empty region, pinned vs unpinned | Pending |
| **Paths** | | | |
| 3 | Control-flow graph, basic blocks, worst-case path | Branchy function where random inputs miss the worst path | Pending |
| 4 | Loop bounds, flow facts, infeasible paths | Data-dependent loop; mutually exclusive branches | Pending |
| 5 | IPET: WCET as an integer linear program | Hand-built ILP for a small CFG, solved with GLPK | Pending |
| **Hardware effects** | | | |
| 6 | Caches: hit/miss, cold vs warm, must/may analysis idea | Row- vs column-major traversal; flushed vs warm cache | Pending |
| 7 | Memory hierarchy: L1/L2/L3/DRAM, TLB | Pointer chasing over growing working sets (latency staircase) | Pending |
| 8 | Pipelines, branch prediction, timing anomalies | Sorted vs unsorted array with a branch; branchless version | Pending |
| **System level** | | | |
| 9 | Interference: preemption, interrupts, cache-related preemption delay | Task preempted by a cache-thrashing task | Pending |
| 10 | Multicore interference: shared cache, bus, DRAM | Task on core 0 while a memory hog runs on core 1 | Pending |
| 11 | From WCET to schedulability: utilisation, RM, response-time analysis | Small RTA calculator for a task set | Pending |
| **Practice** | | | |
| 12 | Static analysis tools | Run OTAWA on a small ARM binary, compare with measurement | Pending |
| 13 | Hybrid analysis: per-block timing combined with the worst path | Instrument basic blocks, combine their maxima | Pending |
| 14 | WCET-friendly coding: bounded loops, no recursion/heap, single-path code | Rewrite a function in single-path form, compare variance | Pending |
| 15 | Certification context: DO-178C, AMC 20-193, ARINC 653 partitioning | Reading only | Pending |
| 16 | (Optional) Probabilistic WCET with extreme-value theory | Fit a Gumbel distribution to measured maxima | Pending |

## 4. Reading to look up

- Wilhelm et al., "The Worst-Case Execution-Time Problem - Overview of Methods and Survey of Tools" (ACM TECS, 2008).
- G. Buttazzo, "Hard Real-Time Computing Systems" (scheduling and response-time analysis).
- TACLeBench / Malardalen WCET benchmarks (standard example programs).
