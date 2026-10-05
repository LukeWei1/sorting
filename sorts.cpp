// sorting.cpp
// Insertion, selection, merge, and quick sort, plus a benchmark.
// Uses only four standard headers; everything else is written by hand.
//
// Compile: g++ -std=c++17 -O1 -o sorting sorting.cpp
// Run:     ./sorting      (prints tables, writes results.csv)

#include <vector>   // std::vector
#include <chrono>   // std::chrono::steady_clock for timing
#include <cstdio>   // printf, fopen, fprintf
#include <random>   // std::mt19937 random number generator

using std::vector;

// ===========================================================================
// Helpers
// ===========================================================================

// Random number generator with a fixed seed, so every run of the
// experiment uses the same lists and is reproducible.
std::mt19937 rng(42);

// Random int in [0, maxValue)
int randomInt(int maxValue) {
    std::uniform_int_distribution<int> dist(0, maxValue - 1);
    return dist(rng);
}

void swapInts(int& x, int& y) {
    int t = x;
    x = y;
    y = t;
}

bool isSorted(const vector<int>& a) {
    for (size_t i = 1; i < a.size(); ++i)
        if (a[i - 1] > a[i]) return false;
    return true;
}

// ===========================================================================
// Insertion sort: O(n^2) average/worst, O(n) on already-sorted input.
// Grows a sorted prefix by sliding each new element left into its place.
// ===========================================================================
void insertionSort(vector<int>& a) {
    for (size_t i = 1; i < a.size(); ++i) {
        int key = a[i];
        size_t j = i;
        while (j > 0 && a[j - 1] > key) {
            a[j] = a[j - 1];   // shift larger element one step right
            --j;
        }
        a[j] = key;
    }
}

// ===========================================================================
// Selection sort: O(n^2) in every case.
// Repeatedly finds the minimum of the unsorted part and swaps it to the front.
// ===========================================================================
void selectionSort(vector<int>& a) {
    size_t n = a.size();
    for (size_t i = 0; i + 1 < n; ++i) {
        size_t minIdx = i;
        for (size_t j = i + 1; j < n; ++j)
            if (a[j] < a[minIdx]) minIdx = j;
        if (minIdx != i) swapInts(a[i], a[minIdx]);
    }
}

// ===========================================================================
// Merge sort: O(n log n) in every case, O(n) extra memory, stable.
// Split in half, sort each half recursively, merge the two sorted halves.
// One scratch buffer is allocated once and reused by every merge.
// ===========================================================================

// Merge sorted ranges a[lo, mid) and a[mid, hi) using tmp, result back in a.
void merge(vector<int>& a, vector<int>& tmp, size_t lo, size_t mid, size_t hi) {
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (a[i] <= a[j]) tmp[k++] = a[i++];   // "<=" keeps the sort stable
        else              tmp[k++] = a[j++];
    }
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi)  tmp[k++] = a[j++];
    for (size_t t = lo; t < hi; ++t) a[t] = tmp[t];
}

void mergeSortRec(vector<int>& a, vector<int>& tmp, size_t lo, size_t hi) {
    if (hi - lo < 2) return;                   // 0 or 1 element: already sorted
    size_t mid = lo + (hi - lo) / 2;
    mergeSortRec(a, tmp, lo, mid);
    mergeSortRec(a, tmp, mid, hi);
    merge(a, tmp, lo, mid, hi);
}

void mergeSort(vector<int>& a) {
    vector<int> tmp(a.size());
    mergeSortRec(a, tmp, 0, a.size());
}

// ===========================================================================
// Quick sort: O(n log n) average, O(n^2) worst case.
// ===========================================================================

// Partition a[lo..hi] (inclusive). Returns p such that every element of
// a[lo..p] is <= every element of a[p+1..hi].
size_t hoarePartition(vector<int>& a, size_t lo, size_t hi) {
    std::uniform_int_distribution<size_t> dist(lo, hi);   // random pivot index
    int pivot = a[dist(rng)];
    size_t i = lo - 1;   // may wrap around when lo == 0; incremented before use
    size_t j = hi + 1;
    while (true) {
        do { ++i; } while (a[i] < pivot);
        do { --j; } while (a[j] > pivot);
        if (i >= j) return j;
        swapInts(a[i], a[j]);
    }
}

void quickSortRec(vector<int>& a, size_t lo, size_t hi) {
    while (lo < hi) {
        size_t p = hoarePartition(a, lo, hi);
        if (p - lo < hi - p) {      // left side smaller: recurse on it
            quickSortRec(a, lo, p);
            lo = p + 1;
        } else {                    // right side smaller: recurse on it
            quickSortRec(a, p + 1, hi);
            hi = p;
        }
    }
}

void quickSort(vector<int>& a) {
    if (a.size() > 1) quickSortRec(a, 0, a.size() - 1);
}

// ===========================================================================
// Benchmarking
// ===========================================================================

typedef void (*SortFunction)(vector<int>&);   // pointer to a sort function

struct Algo {
    const char* name;
    SortFunction sort;
    size_t maxSize;   // skip sizes above this (O(n^2) sorts get too slow)
};

const int NUM_ALGOS = 4;
const Algo algos[NUM_ALGOS] = {
    {"Insertion", insertionSort, 100000},
    {"Selection", selectionSort, 100000},
    {"Merge",     mergeSort,     10000000},
    {"Quick",     quickSort,     10000000},
};

// List of n random ints in [0, maxValue)
vector<int> randomList(size_t n, int maxValue = 1000000) {
    vector<int> v(n);
    for (size_t i = 0; i < n; ++i) v[i] = randomInt(maxValue);
    return v;
}

// Sorts a copy of `input` and returns the time in seconds.
// The copy is made before the timer starts so it isn't counted.
// Returns -1 if the result isn't sorted.
double timeOnce(SortFunction sort, const vector<int>& input) {
    vector<int> work = input;
    auto start = std::chrono::steady_clock::now();
    sort(work);
    auto stop = std::chrono::steady_clock::now();
    if (!isSorted(work)) return -1.0;
    return std::chrono::duration<double>(stop - start).count();
}

// Median of a list of times (insertion-sorts the small list first)
double median(vector<double> v) {
    for (size_t i = 1; i < v.size(); ++i) {
        double key = v[i];
        size_t j = i;
        while (j > 0 && v[j - 1] > key) { v[j] = v[j - 1]; --j; }
        v[j] = key;
    }
    size_t n = v.size();
    return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

double minOf(const vector<double>& v) {
    double m = v[0];
    for (double x : v) if (x < m) m = x;
    return m;
}

double maxOf(const vector<double>& v) {
    double m = v[0];
    for (double x : v) if (x > m) m = x;
    return m;
}

// Correctness check. Insertion sort is the reference (it's the simplest, and
// we also verify its own output is sorted); every algorithm must match it.
bool runTests() {
    vector<vector<int>> cases = {
        {}, {5}, {2, 1}, {1, 2}, {3, 3, 3, 3}, {5, 4, 3, 2, 1}, {1, 2, 3, 4, 5},
        {-3, 7, 0, -3, 12, 7, 1}, {2, 1, 2, 1, 2, 1, 2, 1}
    };
    for (int t = 0; t < 200; ++t) cases.push_back(randomList(randomInt(300), 50));
    cases.push_back(randomList(5000, 10));   // lots of duplicates

    for (const vector<int>& c : cases) {
        vector<int> expected = c;
        insertionSort(expected);
        if (!isSorted(expected)) { printf("Insertion sort FAILED a test\n"); return false; }

        for (int k = 0; k < NUM_ALGOS; ++k) {
            vector<int> actual = c;
            algos[k].sort(actual);
            if (actual != expected) {
                printf("%s sort FAILED a correctness test\n", algos[k].name);
                return false;
            }
        }
    }
    printf("All correctness tests passed.\n\n");
    return true;
}

int main() {
    if (!runTests()) return 1;

    // ---------------- Experiment 1: runtime vs. list size (random input) ----
    const size_t sizes[] = {10, 100, 1000, 10000, 100000, 1000000};

    FILE* csv = fopen("results.csv", "w");
    if (!csv) { printf("Could not open results.csv\n"); return 1; }
    fprintf(csv, "algorithm,size,trials,median_seconds,min_seconds,max_seconds\n");

    printf("Random input, median runtime (seconds)\n");
    printf("%-10s", "n");
    for (int k = 0; k < NUM_ALGOS; ++k) printf("%14s", algos[k].name);
    printf("\n");

    for (size_t n : sizes) {
        // Tiny inputs are noisy, so they get many more repetitions.
        int trials = n <= 1000 ? 201 : (n <= 100000 ? 11 : 5);

        // Every algorithm sorts the SAME lists, so comparisons are fair.
        vector<vector<int>> inputs;
        for (int t = 0; t < trials; ++t) inputs.push_back(randomList(n));

        printf("%-10zu", n);
        for (int k = 0; k < NUM_ALGOS; ++k) {
            const Algo& algo = algos[k];
            if (n > algo.maxSize) { printf("%14s", "skipped"); continue; }

            int algoTrials = trials;
            if (algo.maxSize <= 100000 && n == 100000) algoTrials = 3;   // ~seconds each

            vector<double> times;
            for (int t = 0; t < algoTrials; ++t) {
                double secs = timeOnce(algo.sort, inputs[t]);
                if (secs < 0) { printf("\n%s produced unsorted output!\n", algo.name); return 1; }
                times.push_back(secs);
            }
            double med = median(times);
            printf("%14.3e", med);
            fflush(stdout);
            fprintf(csv, "%s,%zu,%d,%g,%g,%g\n", algo.name, n, algoTrials,
                    med, minOf(times), maxOf(times));
        }
        printf("\n");
    }
    fclose(csv);
    return 0;
}