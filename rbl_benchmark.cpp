#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>
#include <cmath>

using namespace std;

// Cấu trúc lưu trữ đồng thời 4 chỉ số thực nghiệm
struct Metrics {
    double time_ms = 0.0;
    long long comparisons = 0;
    long long moves = 0;
    size_t extra_memory_bytes = 0;
};

// ================= PHẦN 1: TOOL TỰ ĐỘNG SINH 6 BỘ DATASET =================

// 1. Random: Dữ liệu ngẫu nhiên phân phối đều (Edelkamp et al., 2018)
vector<int> genRandom(int n, unsigned seed = 42) {
    vector<int> a(n);
    mt19937 gen(seed);
    uniform_int_distribution<int> dist(1, n * 10);
    for (int i = 0; i < n; ++i) a[i] = dist(gen);
    return a;
}

// 2. Sorted: Dữ liệu đã sắp xếp tăng dần (Sedgewick, 1978)
vector<int> genSorted(int n) {
    vector<int> a(n);
    for (int i = 0; i < n; ++i) a[i] = i + 1;
    return a;
}

// 3. Reverse: Dữ liệu sắp xếp ngược giảm dần (Sedgewick, 1978)
vector<int> genReverse(int n) {
    vector<int> a(n);
    for (int i = 0; i < n; ++i) a[i] = n - i;
    return a;
}

// 4. NearlySorted: Gần như đã sắp xếp - nhiễu 2% phần tử (Auger et al., 2015)
vector<int> genNearlySorted(int n, unsigned seed = 42) {
    vector<int> a = genSorted(n);
    mt19937 gen(seed);
    uniform_int_distribution<int> dist(0, n - 1);
    int swaps = max(1, n / 50);
    for (int i = 0; i < swaps; ++i) swap(a[dist(gen)], a[dist(gen)]);
    return a;
}

// 5. Duplicates_01: Dữ liệu trùng lặp cao chỉ gồm 0 và 1 (Bentley & McIlroy, 1993)
vector<int> genDuplicates(int n, unsigned seed = 42) {
    vector<int> a(n);
    mt19937 gen(seed);
    uniform_int_distribution<int> dist(0, 1);
    for (int i = 0; i < n; ++i) a[i] = dist(gen);
    return a;
}

// 6. Mo3_Killer: Hoán vị K_2k làm suy thoái Median-of-3 (Musser, 1997)
vector<int> genMo3Killer(int n) {
    vector<int> a(n);
    int k = n / 2;
    for (int i = 0; i < n; ++i) a[i] = i + 1;
    for (int i = 1; i < k; i += 2) {
        a[i - 1] = i;
        a[i] = k + i;
    }
    if (k % 2 == 0) a[k - 1] = 2 * k - 1;
    for (int i = 0; i < k; ++i) a[k + i] = 2 * (i + 1);
    return a;
}

// ================= PHẦN 2: CÀI ĐẶT THUẬT TOÁN & ĐO BENCHMARK =================

// A. QuickSort_Basic: Chọn chốt đầu tiên + phân hoạch Lomuto (Bentley & McIlroy, 1993)
void quickSortBasicRec(vector<int>& a, int l, int r, Metrics& m, int depth, int& max_depth) {
    while (l < r) {
        max_depth = max(max_depth, depth);
        int pivot = a[l]; m.moves++;
        int j = l;
        for (int i = l + 1; i <= r; ++i) {
            m.comparisons++;
            if (a[i] < pivot) {
                ++j;
                swap(a[j], a[i]);
                m.moves += 3;
            }
        }
        swap(a[l], a[j]);
        m.moves += 3;
        // Đệ quy vào nhánh nhỏ hơn trước để chống tràn bộ nhớ Stack (Sedgewick, 1978)
        if (j - l < r - j) {
            quickSortBasicRec(a, l, j - 1, m, depth + 1, max_depth);
            l = j + 1;
        } else {
            quickSortBasicRec(a, j + 1, r, m, depth + 1, max_depth);
            r = j - 1;
        }
        depth++;
    }
}

Metrics runQuickSortBasic(vector<int> a) {
    Metrics m; int max_depth = 0;
    auto start = chrono::high_resolution_clock::now();
    if (!a.empty()) quickSortBasicRec(a, 0, (int)a.size() - 1, m, 1, max_depth);
    auto end = chrono::high_resolution_clock::now();
    m.time_ms = chrono::duration<double, milli>(end - start).count();
    m.extra_memory_bytes = max_depth * 32;
    return m;
}

// B. QuickSort_Mo3: Chọn trung vị (đầu, giữa, cuối) + phân hoạch Hoare (Musser, 1997)
int medianOf3Val(int x, int y, int z, Metrics& m) {
    m.comparisons++;
    if (x < y) {
        m.comparisons++;
        if (y < z) return y;
        m.comparisons++;
        return (x < z) ? z : x;
    } else {
        m.comparisons++;
        if (y > z) return y;
        m.comparisons++;
        return (x > z) ? z : x;
    }
}

void quickSortMo3Rec(vector<int>& a, int f, int b, Metrics& m, int depth, int& max_depth) {
    while (b - f > 1) {
        max_depth = max(max_depth, depth);
        if (b - f == 2) {
            m.comparisons++;
            if (a[f] > a[f + 1]) { swap(a[f], a[f + 1]); m.moves += 3; }
            return;
        }
        int pivot = medianOf3Val(a[f], a[f + (b - f) / 2], a[b - 1], m);
        int i = f, j = b - 1;
        while (true) {
            while (true) { m.comparisons++; if (a[i] < pivot) i++; else break; }
            while (true) { m.comparisons++; if (pivot < a[j]) j--; else break; }
            if (i >= j) break;
            swap(a[i], a[j]);
            m.moves += 3;
            i++; j--;
        }
        int p = i;
        if (p - f >= b - p) {
            quickSortMo3Rec(a, p, b, m, depth + 1, max_depth);
            b = p;
        } else {
            quickSortMo3Rec(a, f, p, m, depth + 1, max_depth);
            f = p;
        }
        depth++;
    }
}

Metrics runQuickSortMo3(vector<int> a) {
    Metrics m; int max_depth = 0;
    auto start = chrono::high_resolution_clock::now();
    if (!a.empty()) quickSortMo3Rec(a, 0, (int)a.size(), m, 1, max_depth);
    auto end = chrono::high_resolution_clock::now();
    m.time_ms = chrono::duration<double, milli>(end - start).count();
    m.extra_memory_bytes = max_depth * 32;
    return m;
}

// C. MergeSort: Chia để trị chuẩn với mảng phụ O(N) (Al-Kharabsheh et al., 2013)
void mergeSortRec(vector<int>& a, vector<int>& temp, int l, int r, Metrics& m) {
    if (l >= r) return;
    int mid = l + (r - l) / 2;
    mergeSortRec(a, temp, l, mid, m);
    mergeSortRec(a, temp, mid + 1, r, m);
    int i = l, j = mid + 1, k = l;
    while (i <= mid && j <= r) {
        m.comparisons++;
        if (a[i] <= a[j]) temp[k++] = a[i++];
        else temp[k++] = a[j++];
        m.moves++;
    }
    while (i <= mid) { temp[k++] = a[i++]; m.moves++; }
    while (j <= r) { temp[k++] = a[j++]; m.moves++; }
    for (int idx = l; idx <= r; ++idx) { a[idx] = temp[idx]; m.moves++; }
}

Metrics runMergeSort(vector<int> a) {
    Metrics m;
    auto start = chrono::high_resolution_clock::now();
    vector<int> temp(a.size());
    if (!a.empty()) mergeSortRec(a, temp, 0, (int)a.size() - 1, m);
    auto end = chrono::high_resolution_clock::now();
    m.time_ms = chrono::duration<double, milli>(end - start).count();
    m.extra_memory_bytes = a.size() * sizeof(int) + ((int)log2(max(1, (int)a.size())) + 2) * 32;
    return m;
}

int main() {
    vector<int> sizes = {10000, 20000, 30000};
    ofstream csv("benchmark_results.csv");
    csv << "Dataset,Size,Algorithm,Time_ms,Comparisons,Moves,Memory_Bytes\n";

    cout << left << setw(15) << "Dataset" << setw(8) << "Size" << setw(18) << "Algorithm" 
         << setw(12) << "Time(ms)" << setw(15) << "Comparisons" << setw(15) << "Moves" << "Memory(B)\n";
    cout << string(95, '-') << "\n";

    for (int n : sizes) {
        vector<pair<string, vector<int>>> datasets = {
            {"Random", genRandom(n)},
            {"Sorted", genSorted(n)},
            {"Reverse", genReverse(n)},
            {"NearlySorted", genNearlySorted(n)},
            {"Duplicates_01", genDuplicates(n)},
            {"Mo3_Killer", genMo3Killer(n)}
        };

        for (auto& ds : datasets) {
            string dname = ds.first;
            const vector<int>& arr = ds.second;

            vector<pair<string, Metrics>> results = {
                {"QuickSort_Basic", runQuickSortBasic(arr)},
                {"QuickSort_Mo3",   runQuickSortMo3(arr)},
                {"MergeSort",       runMergeSort(arr)}
            };

            for (auto& res : results) {
                cout << left << setw(15) << dname << setw(8) << n << setw(18) << res.first
                     << fixed << setprecision(3) << setw(12) << res.second.time_ms
                     << setw(15) << res.second.comparisons << setw(15) << res.second.moves 
                     << res.second.extra_memory_bytes << "\n";

                csv << dname << "," << n << "," << res.first << "," 
                    << fixed << setprecision(3) << res.second.time_ms << "," 
                    << res.second.comparisons << "," << res.second.moves << "," 
                    << res.second.extra_memory_bytes << "\n";
            }
        }
    }
    csv.close();
    cout << "\n[HOAN TAT] Da xuat toan bo so lieu ra file 'benchmark_results.csv'!\n";
    return 0;
}