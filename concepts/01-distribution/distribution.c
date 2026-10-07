/*
 * Concept 1: execution time is a distribution, not a number.
 *
 * Times insertion sort on three kinds of input:
 *   sorted   - known best case  (inner loop never runs)
 *   random   - what typical testing sees
 *   reversed - known worst case (inner loop runs fully every time)
 *
 * Build and run: make run
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N       2560     /* elements to sort (default: 256) */
#define RUNS    20000   /* timed runs per input kind (default: 20000) */
#define BINS    20      /* histogram rows (default: 20) */
#define BAR_MAX 50      /* histogram bar width (default: 50) */

/* Stop the compiler from moving code across the timer reads. */
#define BARRIER() __asm__ volatile("" ::: "memory")

enum kind { SORTED, RANDOM, REVERSED, KINDS };

static const char *kind_name[KINDS] = {
    "sorted   (best-case input)",
    "random   (typical testing)",
    "reversed (worst-case input)",
};

static int input[KINDS][N];
static int work[N];
static uint64_t t_ns[KINDS][RUNS];

/* Code under test. */
static void insertion_sort(int *a, int n)
{
    for (int i = 1; i < n; i++) {
        int key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

static void make_input(enum kind k)
{
    for (int i = 0; i < N; i++) {
        switch (k) {
        case SORTED:   input[k][i] = i;      break;
        case REVERSED: input[k][i] = N - i;  break;
        default:       input[k][i] = rand(); break;
        }
    }
}

static int is_sorted(const int *a, int n)
{
    for (int i = 1; i < n; i++)
        if (a[i - 1] > a[i])
            return 0;
    return 1;
}

/* Time one sort of input[k], in ns. */
static uint64_t time_sort(enum kind k)
{
    memcpy(work, input[k], sizeof work);

    BARRIER();
    uint64_t t0 = now_ns();
    BARRIER();
    insertion_sort(work, N);
    BARRIER();
    uint64_t t1 = now_ns();
    BARRIER();

    if (!is_sorted(work, N)) {
        fprintf(stderr, "sort failed\n");
        exit(1);
    }
    return t1 - t0;
}

/*
 * The three kinds are interleaved so they all see the same machine
 * conditions (clock speed, background load). Pass 0 is a warm-up and
 * its samples are overwritten by pass 1.
 */
static void measure(void)
{
    for (int k = 0; k < KINDS; k++)
        make_input(k);

    for (int pass = 0; pass < 2; pass++) {
        for (int r = 0; r < RUNS; r++) {
            make_input(RANDOM);     /* fresh random input every run */
            for (int k = 0; k < KINDS; k++)
                t_ns[k][r] = time_sort(k);
        }
    }
}

static int cmp_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

/* Value below which a fraction q of the samples fall. s must be sorted. */
static uint64_t percentile(const uint64_t *s, double q)
{
    return s[(size_t)(q * (RUNS - 1))];
}

/* Sorted copy of the samples of kind k. */
static void sort_samples(enum kind k, uint64_t *s)
{
    memcpy(s, t_ns[k], RUNS * sizeof s[0]);
    qsort(s, RUNS, sizeof s[0], cmp_u64);
}

static void print_stats(enum kind k)
{
    static uint64_t s[RUNS];
    sort_samples(k, s);

    printf("%-28s %8.2f %8.2f %8.2f %8.2f\n", kind_name[k],
           s[0] / 1e3, percentile(s, 0.50) / 1e3,
           percentile(s, 0.99) / 1e3, s[RUNS - 1] / 1e3);
}

/* Bins cover min..p99.9; the rare samples above are counted separately. */
static void print_histogram(enum kind k)
{
    static uint64_t s[RUNS];
    sort_samples(k, s);
    uint64_t lo = s[0];
    uint64_t hi = percentile(s, 0.999);

    int count[BINS] = {0};
    int above = 0;
    int peak = 0;
    double width = (double)(hi - lo + 1) / BINS;
    for (int r = 0; r < RUNS; r++) {
        if (t_ns[k][r] > hi)
            above++;
        else
            count[(int)((t_ns[k][r] - lo) / width)]++;
    }
    for (int b = 0; b < BINS; b++)
        if (count[b] > peak)
            peak = count[b];

    printf("\nHistogram, %s, min to p99.9 (bin start in us, count):\n",
           kind_name[k]);
    for (int b = 0; b < BINS; b++) {
        int len = count[b] * BAR_MAX / peak;
        if (count[b] > 0 && len == 0)
            len = 1;    /* keep rare samples visible */
        printf("%8.2f |", (lo + b * width) / 1e3);
        for (int i = 0; i < len; i++)
            putchar('#');
        printf("%*s %d\n", BAR_MAX - len, "", count[b]);
    }
    printf("   above | %d samples, up to %.2f\n", above, s[RUNS - 1] / 1e3);
}

int main(void)
{
    srand(1);   /* same random inputs on every run */

    measure();

    printf("Insertion sort, N=%d, %d runs per input kind. Times in us.\n\n",
           N, RUNS);
    printf("%-28s %8s %8s %8s %8s\n", "input", "min", "p50", "p99", "max");
    for (int k = 0; k < KINDS; k++)
        print_stats(k);

    print_histogram(RANDOM);
    return 0;
}
