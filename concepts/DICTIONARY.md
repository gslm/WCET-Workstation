# Dictionary

Terms to learn or expand on, sorted alphabetically. Each entry says what the term is, gives an example, and says why it matters for WCET.

Machine facts quoted as "this laptop" were read from `lscpu` and `/sys` on the dev laptop (Intel i7-11390H, Linux 6.8).

---

## Bounded loops

**In short:** a loop whose maximum number of iterations is known before the program runs.

WCET analysis adds up the time of everything that can execute. If a loop body can repeat an unknown number of times, the total is unknown. So every loop needs a known upper limit, called its **loop bound**. Some bounds are obvious from the code; others must be proven, or stated by the programmer as an annotation for the analysis tool.

Example:

```c
/* Bounded: always exactly 16 iterations. */
for (int i = 0; i < 16; i++)
    sum += buf[i];

/* Bounded only if len is bounded: depends on every caller. */
for (int i = 0; i < len; i++)
    sum += buf[i];

/* Unbounded: depends on how much data keeps arriving. */
while (uart_rx_ready())
    process(uart_read());

/* Made bounded: never more than MAX_RX bytes per call. */
for (int n = 0; n < MAX_RX && uart_rx_ready(); n++)
    process(uart_read());
```

In concept 1, insertion sort's outer loop runs N-1 times and the inner `while` loop at most `i` times, so the inner body runs at most N(N-1)/2 times in total. That is the loop bound that gives the worst case.

**Why it matters for WCET:** no loop bound, no WCET. Safety coding rules require it, e.g. rule 2 of NASA/JPL's "Power of Ten": all loops must have a fixed upper bound. Concept 4 covers loop bounds in depth.

---

## Caches

**In short:** small, fast memories close to the CPU that keep copies of recently used data from the large, slow main memory (RAM).

Main memory (DRAM) is far slower than the CPU: one read can cost hundreds of CPU cycles. A cache keeps recently used data close by, so repeated use is fast.

- Data moves in fixed-size blocks called **cache lines** (64 bytes on this laptop). Reading one byte loads its whole line.
- **Hit:** the data is already in the cache, so access is fast.
- **Miss:** the data must be fetched from the next, slower level.
- **Levels:** L1 (smallest, fastest, one per core), L2, L3 (largest, slowest, usually shared by all cores).
- When a cache is full, an old line is thrown out (**evicted**) to make room. The rule that picks which one is the replacement policy (e.g. LRU: least recently used).

This laptop: L1 data 48 KiB per core, L1 instruction 32 KiB per core, L2 1.25 MiB per core, L3 12 MiB shared by the 4 cores. Rough access costs on CPUs of this class: L1 about 5 cycles, L2 about 15, L3 about 50, DRAM several hundred (we measure the real values in concept 7).

Example: same instructions, same result, very different time:

```c
static int m[1024][1024];   /* 4 MiB; C stores it row by row */

/* Fast: walks consecutive addresses, one miss per 16 ints (64-byte line). */
for (int r = 0; r < 1024; r++)
    for (int c = 0; c < 1024; c++)
        sum += m[r][c];

/* Slow: each access jumps 4 KiB ahead, so most accesses miss L1. */
for (int c = 0; c < 1024; c++)
    for (int r = 0; r < 1024; r++)
        sum += m[r][c];
```

**Why it matters for WCET:** the same code takes different times depending on what is already in the cache (cold vs warm), and on what other code, or another core, evicted. This is the largest source of hardware timing variation and the hardest part of static WCET analysis. Safety systems often lock, partition or disable caches, or use TCM/scratchpad memory instead (concept 6).

---

## Clock turbo

**In short:** the CPU temporarily runs above its base clock speed while temperature and power allow it. Intel calls it Turbo Boost, AMD calls it Precision Boost.

The chip's firmware raises the clock when there is thermal and power headroom, and lowers it when the chip gets hot, when several cores are busy, or when the power budget is used up. This happens many times per second, and the program is not told.

This laptop: `lscpu` reports a maximum of 5.0 GHz, and the CPU name string says 3.40 GHz. Turbo is currently enabled (`/sys/devices/system/cpu/intel_pstate/no_turbo` is 0).

Example: a piece of code that needs 1,000,000 clock cycles takes 0.20 ms at 5.0 GHz and 0.29 ms at 3.4 GHz. Same code, about 47% longer, depending only on how hot and busy the chip is.

**Why it matters for WCET:** time in seconds changes for reasons that have nothing to do with the code. For measurements, turbo is usually switched off (on this laptop, as root: `echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo`). Avionics processors run at a fixed clock set at boot. See also: Frequency scaling.

---

## Fixed-priority scheduler

**In short:** a scheduler where each task is given a priority at design time that never changes, and the highest-priority task that is ready always runs.

This is what most RTOSes use (FreeRTOS, VxWorks, RTEMS, and others). It is usually **preemptive**: when a higher-priority task becomes ready (e.g. its timer expires), it immediately interrupts the running lower-priority task, which resumes later where it stopped.

A common way to choose priorities is **rate monotonic**: the shorter a task's period, the higher its priority.

Example: three periodic tasks.

| Task | Period | WCET | Priority |
|---|---|---|---|
| Sensor | 5 ms | 1 ms | high |
| Control | 20 ms | 4 ms | medium |
| Logging | 100 ms | 10 ms | low |

All three start at time 0. One character per millisecond:

```
ms        0    5    10   15   20
Sensor    S    S    S    S    S
Control    CCCC                C...
Logging         LLLL LLLL LL
```

- Sensor always runs as soon as it is released.
- Control waits 1 ms for Sensor, then runs.
- Logging only runs when nothing else is ready, and is preempted by Sensor at 10 and 15 ms. Its WCET is 10 ms, but it finishes at 18 ms. That 18 ms is its **response time**.

**Why it matters for WCET:** behaviour is predictable. Given each task's WCET, period and priority, you can calculate every task's worst-case response time and prove all deadlines are met (concept 11). This only works if you have a WCET for every task. See also: Scheduler.

---

## Frequency scaling

**In short:** the operating system (or the hardware) changes the CPU clock speed while running: slow when idle to save power, fast when busy. Also called DVFS (dynamic voltage and frequency scaling).

On Linux, a **cpufreq driver** controls the clock, and a **governor** sets the policy. This laptop uses the `intel_pstate` driver with the `powersave` governor (the alternative is `performance`). With `intel_pstate`, `powersave` still speeds up under load, but it reacts gradually and favours efficiency. The clock can go anywhere from 0.4 GHz to 5.0 GHz on this laptop.

Changing speed takes time and happens in steps. A benchmark that starts on an idle CPU first runs slowly, then speeds up.

Example: in concept 1, before the warm-up pass and interleaving were added, the median for the reversed input moved between 29 and 70 us from one run to the next. The code was identical; the clock speed was not.

For measurements, the clock is held still: `performance` governor plus turbo off, e.g. `sudo cpupower frequency-set -g performance` (`cpupower` is installed on this laptop).

**Why it matters for WCET:** a WCET is only valid for one clock speed. Time-critical systems fix the clock at boot and never change it. Difference from turbo: frequency scaling is the general mechanism of changing the clock; turbo is the specific feature of going above the base clock. See also: Clock turbo.

---

## Heap

**In short:** a pool of memory from which a program requests blocks at run time, of any size and in any order. In C: `malloc()` and `free()`.

Unlike local variables on the stack (freed automatically when the function returns), a heap block lives until the program explicitly frees it. The allocator (the code behind `malloc`) keeps track of which blocks are used and which are free.

```c
char *buf = malloc(1000);   /* ask for 1000 bytes */
if (buf == NULL) {
    /* out of memory: must be handled somehow */
}
/* ... use buf ... */
free(buf);                  /* give it back */
```

Problems for time-critical systems:

1. **Unpredictable time.** `malloc` may search lists of free blocks, split or merge blocks, or ask the OS for more memory. Its time depends on the whole history of earlier allocations.
2. **Fragmentation.** After many allocations and frees of different sizes, free memory is split into small pieces. A request can fail even though enough memory is free in total.
3. **Failure at run time.** `malloc` can return NULL in the middle of a flight.
4. **Bugs.** Leaks (forgetting `free`), use after free, double free.

**Why it matters for WCET:** heap operations have no useful time bound. Safety rules forbid them: Power of Ten rule 3 forbids dynamic allocation after initialisation, and MISRA C:2012 Rule 21.3 forbids `malloc`/`free` entirely. The alternatives are static allocation (arrays sized at compile time, as in concept 1) and fixed-size memory pools, which take constant time. See also: Stack.

---

## Memory bandwidth

**In short:** how much data per second can move between the CPU and main memory (RAM).

Bandwidth is not the same as latency:

- **Latency:** how long one access waits.
- **Bandwidth:** how much data can flow per second in total.

Think of a road: latency is how long one car takes to make the trip; bandwidth is how many cars per hour the road can carry.

All cores share the same memory controller and RAM, so they share the bandwidth. When one core moves a lot of data (e.g. copying a large buffer), the other cores' memory accesses queue up and wait longer.

Example (illustrative numbers; concept 10 measures real ones): task A on core 0 takes 100 us when it runs alone. A "memory hog" on core 1 starts copying a 100 MB buffer in a loop. Task A now takes noticeably longer, with no change to its code or input.

**Why it matters for WCET:** this is the main cause of **multicore interference**, which AMC 20-193 / AC 20-193 require you to account for. Mitigations: limit how much each core may access memory per time period (bandwidth regulation), partition the memory system, or keep other cores quiet while a critical task runs.

---

## Memory lock

**In short:** asking the OS to keep a program's memory permanently in physical RAM, so it is never moved out to disk and never loaded lazily on first use. On Linux: `mlock()` / `mlockall()`.

Background: Linux uses **virtual memory**. When a program allocates memory, the kernel often does not give it real RAM until the memory is first touched (**demand paging**). The first access to each 4 KiB page causes a **page fault**: the kernel pauses the program, finds a free page of RAM, clears it and maps it in. That costs microseconds. Under memory pressure the kernel may also move rarely used pages out to disk (**swap**); touching them again can cost milliseconds.

```c
#include <stdio.h>
#include <sys/mman.h>

int main(void)
{
    /* Lock all current and future pages in RAM. Needs root,
       CAP_IPC_LOCK, or a large enough RLIMIT_MEMLOCK. */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
        perror("mlockall");

    /* ... time-critical work, no page faults from here on ... */
    return 0;
}
```

Real-time Linux programs also touch their whole stack and buffers once at start-up ("prefaulting"), so no first-touch fault happens later.

**Why it matters for WCET:** a page fault in the middle of a task adds a large, rare delay that shows up only in the tail. Microcontroller RTOSes usually have no virtual memory, so this does not apply there; on Linux-based real-time systems it is standard practice. Not to be confused with **cache locking** (pinning chosen data in the cache, see Caches) or with a **mutex lock** (protecting shared data between tasks).

---

## Pipeline

**In short:** the CPU splits each instruction into steps and works on several instructions at once, each in a different step, like an assembly line.

Classic 5-stage pipeline: F = fetch, D = decode, E = execute, M = memory access, W = write back.

```
cycle:     1  2  3  4  5  6  7
instr 1:   F  D  E  M  W
instr 2:      F  D  E  M  W
instr 3:         F  D  E  M  W
```

Each instruction still takes 5 cycles, but once the pipeline is full, one instruction finishes every cycle.

The line stops moving (a **stall**) when:

- **Data hazard:** an instruction needs a result that is not ready yet.
- **Control hazard:** a branch. The CPU does not know which instruction comes next until the branch is resolved. It either waits, or guesses (see Speculative execution). A wrong guess throws away the work in progress.
- **Structural hazard:** two instructions need the same hardware unit at the same time.

```c
x = a[i];      /* load: several cycles, more on a cache miss */
y = x + 1;     /* needs x: may stall until the load completes */
```

**Why it matters for WCET:** an instruction's time depends on its neighbours, not only on itself, so you cannot just add up times from a table. Static WCET tools simulate the pipeline cycle by cycle. Simple in-order pipelines (Cortex-M, Cortex-R) can be modelled precisely; the deep out-of-order pipelines of x86 desktop CPUs, like this laptop's, practically cannot. Timing anomalies come from here (concept 8).

---

## Scheduler

**In short:** the part of an operating system that decides which task (thread) runs on each CPU core at each moment.

A core runs one thing at a time. When there are more tasks than cores, the scheduler switches between them. Each switch is a **context switch**: save the registers of one task, load those of another. The scheduler acts on a timer tick, when a task blocks (waiting for data, a lock or a delay), or when an interrupt makes another task ready.

Common policies:

- **Round robin:** each task gets a time slice in turn.
- **Fair share:** each task gets a fair share of CPU time over time. Linux's default for normal tasks (CFS, replaced by EEVDF in kernel 6.6; this laptop runs 6.8). Good for desktops, but a task can be delayed by an unpredictable amount.
- **Fixed priority:** see Fixed-priority scheduler. The usual RTOS policy. Linux also has it as `SCHED_FIFO` / `SCHED_RR` (set with `chrt`).
- **Earliest deadline first (EDF):** the task with the nearest deadline runs first. Linux has it as `SCHED_DEADLINE`.
- **Time-triggered (cyclic executive):** a fixed timetable computed before the system runs. Common in avionics; ARINC 653 partitions run in fixed time windows.

Example: in concept 1 the program is a normal Linux task, so the scheduler can pause it in the middle of a sort to run something else. That is one cause of the very large max values. `chrt -f 80 ./build/distribution` would run it under `SCHED_FIFO`, ahead of all normal tasks.

**Why it matters for WCET:** WCET is the time of one task running alone. The scheduler and the other tasks decide when it actually finishes (its response time). Real-time analysis combines both (concept 11).

---

## Scratchpad

**In short:** a small, fast on-chip memory that software controls directly. The program (or the linker) decides what lives there; nothing moves in or out automatically, unlike a cache.

A scratchpad has its own address range. To use it, you place code or data there at link time, or copy data in and out at run time (often with DMA).

Example (GCC on a microcontroller; the section names are defined in your linker script):

```c
__attribute__((section(".scratch_text")))
void control_law(void)
{
    /* ... runs from scratchpad ... */
}

__attribute__((section(".scratch_data")))
static float samples[256];   /* lives in scratchpad */
```

| | Cache | Scratchpad |
|---|---|---|
| Who decides what is inside | hardware, automatically | software, explicitly |
| Access time | hit or miss, depends on history | always the same |
| Effort | none | you must plan what fits |
| WCET analysis | hard | easy |

**Why it matters for WCET:** fixed access time gives tight, easy WCET bounds, at the price of planning memory by hand. TCM is ARM's name for this kind of memory. See also: TCM, Caches.

---

## Speculative execution

**In short:** the CPU guesses what the program will do next (e.g. which way a branch goes) and starts executing that path before knowing if the guess is right. A right guess saves time; a wrong guess throws the work away and costs time.

In a deep pipeline, waiting for every branch to be resolved would waste many cycles. A **branch predictor** learns from history (e.g. "this loop branch was taken the last 100 times, so it will probably be taken again"). A wrong guess (**misprediction**) costs roughly 15 to 20 cycles on recent x86 cores.

Example (concept 8 preview):

```c
for (int i = 0; i < n; i++)
    if (data[i] >= 128)      /* branch */
        sum += data[i];
```

- `data` sorted: the branch is false, false, ..., then true, true, .... The predictor is almost always right, so the loop is fast.
- `data` random: the branch is a coin toss, about half are mispredicted, and the loop can be several times slower.

Same instructions, same number of iterations. The compiler may turn this `if` into branchless code at `-O2`/`-O3`, which hides the effect.

The Spectre security flaws (2018) also come from speculative execution: the wrong-path work is undone, but it leaves traces in the cache.

**Why it matters for WCET:** a branch's time depends on the history of earlier branches (the predictor's state), so the same code with the same input can take different times depending on what ran before. This is very hard to model, so time-critical processors use simple predictors or none. See also: Pipeline.

---

## Stack

**In short:** the memory region that holds each function call's local variables, arguments and return address. It grows when a function is called and shrinks when it returns, automatically.

Each call adds a **frame** on top; each return removes it (last in, first out). Allocation is very fast: just moving the stack pointer register.

```c
int square(int x)
{
    int r = x * x;     /* r lives in square()'s frame (or a register) */
    return r;
}                      /* frame released here, automatically */

void f(void)
{
    char buf[256];     /* 256 bytes in f()'s frame */
    square(3);         /* adds square()'s frame on top of f()'s */
}
```

```
high addresses
 +-----------------+
 | main() frame    |
 +-----------------+
 | f() frame       |  buf[256]
 +-----------------+
 | square() frame  |  r, return address    <- stack pointer
 +-----------------+
 | free            |  grows downward on most CPUs
low addresses
```

**Stack overflow:** if calls nest too deeply (e.g. recursion) or local variables are too big, the stack runs past its region and overwrites other memory or crashes. Many microcontrollers have no protection, so the corruption is silent. In an RTOS every task has its own fixed-size stack.

**Why it matters for WCET:** certification asks for a worst-case stack usage, proven much like WCET: the deepest call chain times the frame sizes. Recursion makes both stack usage and WCET unbounded, which is why it is banned. GCC's `-fstack-usage` reports each function's frame size. See also: Heap.

---

## TCM (Tightly Coupled Memory)

**In short:** ARM's name for a small, fast RAM wired directly to the CPU core, with fixed, very short access time and no cache behaviour. A kind of scratchpad.

Found on Cortex-M7 and Cortex-R cores. Usually split into **ITCM** (for instructions) and **DTCM** (for data). It runs at full core speed, unlike flash (which needs wait states at high clock) or main SRAM reached over a shared bus (which competes with DMA and other masters). The Cortex-M7 in the STM32H7 family has both TCMs and caches.

Typical use: put interrupt handlers and the control loop in ITCM, and their data in DTCM. No cache misses, no flash wait states, no bus contention.

Example (STM32-style; the section names are defined in your linker script):

```c
__attribute__((section(".itcm_text")))
void ADC_IRQHandler(void)
{
    /* ... runs from ITCM ... */
}

__attribute__((section(".dtcm_data")))
static int16_t adc_buf[512];   /* lives in DTCM */
```

**Why it matters for WCET:** fixed access time removes cache variation, so WCET bounds are tight and easy to get. The cost is size (tens to hundreds of KiB) and choosing carefully what goes in. See also: Scratchpad, Caches.
