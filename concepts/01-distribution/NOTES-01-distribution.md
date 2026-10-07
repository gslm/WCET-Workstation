# Concept 1: Execution time is a distribution

## Theory

Run the same code many times and you get a distribution of times, not one number. Two sources of variation:

1. **Input (software state)**: which path runs (branches taken, loop iterations).
2. **Hardware and system state**: cache contents, pipeline, branch predictor, clock speed, interrupts, OS preemption, other cores.

Terms:

| Term | Meaning |
|---|---|
| BCET | Best-case execution time: shortest possible. |
| ACET | Average-case execution time: what benchmarks report. Irrelevant for hard deadlines. |
| WCET | Worst-case execution time: longest possible. Usually unknown exactly. |
| Observed max | Longest time seen in tests (high-water mark). Always <= WCET. |
| WCET bound | Value produced by analysis, claimed >= WCET. |
| Safe | Bound >= true WCET. |
| Tight | Bound close to true WCET. The gap is called pessimism. |

```
 BCET      typical      observed max     true WCET     WCET bound
  |-----------|-------------|-----------------|-------------|
                            ^ what testing sees           ^ what analysis claims
```

Key points:

- Measurement gives a **lower** bound on WCET (the observed max), never an upper bound. The gap to the true WCET is unknown.
- Adding a margin (e.g. +20%) is an engineering judgement, not a proof.
- Hard real-time cares about the worst case, not the average: one missed deadline can be a failure. Schedulability analysis uses each task's WCET (concept 11).

## The example

`distribution.c` times an insertion sort of 256 ints on three inputs:

| Input | Inner-loop shifts | Role |
|---|---|---|
| sorted | 0 | known best case |
| random | about n(n-1)/4 = 16320 on average | what typical testing sees |
| reversed | n(n-1)/2 = 32640 | known worst case |

Measurement details (covered properly in concept 2):

- The three inputs are interleaved, so all see the same machine conditions (clock speed, background load).
- A first pass is a warm-up and is discarded.
- Compiler barriers keep the sort between the two timer reads.
- The result is checked, so the compiler cannot drop the sort.
- Fixed random seed, so every run uses the same inputs.

Build and run:

```bash
make run
```

## Results on the dev laptop

i7-11390H, `powersave` governor, no CPU pinning. Times in us.

```
input                             min      p50      p99      max
sorted   (best-case input)       0.34     0.60     0.86    67.57
random   (typical testing)      12.25    20.88    26.95   134.02
reversed (worst-case input)     21.66    33.96    42.82   132.48
```

## What to observe

1. **Input effect.** Medians: sorted 0.6, random 21, reversed 34. The random p99 (27) is below the reversed median (34): 20000 random tests never came close to the worst path. The chance that a random input is the reversed one is about 1 in 256!.
2. **System effect.** Maxima are 4 to over 100 times the medians. That comes from interrupts, preemption and clock changes, not from the code. In one run the random max (607) beat the reversed max (520): the observed max measured noise, not the worst path.
3. **Run-to-run.** Before interleaving and warm-up were added, the reversed median moved between 29 and 70 us across runs (CPU frequency scaling).
4. **Conclusion.** On this machine the observed max is neither safe nor tight. A WCET method must handle both dimensions: find the worst path (concepts 3 to 5) and bound the hardware state (concepts 2, 6 to 10).

## Questions

- With a 50 us deadline, would you accept this function based on the random tests alone?
- Which max values describe the code, and which describe the machine?
