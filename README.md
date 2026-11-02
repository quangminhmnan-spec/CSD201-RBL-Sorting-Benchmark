# CSD201-RBL-Sorting-Benchmark
# Empirical Performance Degradation Analysis: QuickSort vs. MergeSort (CSD201 RBL)

## 1. Research Overview
* **Topic:** Experimental Evaluation of Performance Degradation in QuickSort Variants Compared to MergeSort Under Adverse Input Distributions.
* **Research Question (RQ):** How do specific input distributions (`Random`, `Sorted`, `Reverse`, `NearlySorted`, `Duplicates_01`, and `Mo3_Killer`) and pivot selection strategies affect the execution time (runtime), comparison counts, data moves, and memory consumption of QuickSort compared to MergeSort?

## 2. Repository Structure
* `rbl_benchmark.cpp`: Complete C++11 source code containing both the automated **Dataset Generator** (6 input distributions) and the **Experimental Benchmark Engine** (`QuickSort_Basic`, `QuickSort_Mo3`, and `MergeSort`).
* `benchmark_results.csv`: Raw experimental output data across $N \in \{10,000, 20,000, 30,000\}$ measuring `Time_ms`, `Comparisons`, `Moves`, and `Memory_Bytes`.
* `benchmark_charts_en.png`: Visualized comparison charts generated from the empirical benchmark data.

## 3. How to Compile and Run
To reproduce the experimental benchmark and generate `benchmark_results.csv`:
```bash
g++ -O2 rbl_benchmark.cpp -o rbl_benchmark
./rbl_benchmark
