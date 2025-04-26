#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

using namespace std;
using std::chrono::high_resolution_clock;

int thread_counts[] = { 4, 8, 16, 32, 64, 128, 256 };
int array_sizes[] = { 100000, 1000000, 10000000, 100000000 };

vector<int> generate_random_array(int size) {
    vector<int> arr(size);
    srand(time(0));

    for (int& num : arr) {
        num = rand() % 10000 + 1;
    }

    return arr;
}

void without_parallelization(const vector<int>& arr, long long& sum, int& min_multiple_of_13) {
    sum = 0;
    min_multiple_of_13 = INT_MAX;

    for (int num : arr) {
        if (num % 13 == 0) {
            sum += num;
            if (num < min_multiple_of_13) {
                min_multiple_of_13 = num;
            }
        }
    }
}

void with_mutex(const vector<int>& arr, long long& sum, int& min_multiple_of_13, int num_threads) {
    sum = 0;
    min_multiple_of_13 = INT_MAX;
    mutex mtx;

    auto process_chunk = [&](int start, int end) {
        long long local_sum = 0;
        int local_min = INT_MAX;

        for (int i = start; i < end; ++i) {
            if (arr[i] % 13 == 0) {
                local_sum += arr[i];
                if (arr[i] < local_min) {
                    local_min = arr[i];
                }
            }
        }

        lock_guard<mutex> lock(mtx);
        sum += local_sum;
        if (local_min < min_multiple_of_13) {
            min_multiple_of_13 = local_min;
        }
    };

    vector<thread> threads;
    int chunk_size = arr.size() / num_threads;

    for (int i = 0; i < num_threads; ++i) {
        int start = i * chunk_size;
        int end = (i == num_threads - 1) ? arr.size() : start + chunk_size;
        threads.push_back(thread(process_chunk, start, end));
    }

    for (auto& t : threads) {
        t.join();
    }
}

void with_atomic(const vector<int>& arr, atomic<long long>& sum, atomic<int>& min_multiple_of_13, int num_threads) {
    sum = 0;
    min_multiple_of_13 = INT_MAX;

    auto process_chunk = [&](int start, int end) {
        long long local_sum = 0;
        int local_min = INT_MAX;

        for (int i = start; i < end; ++i) {
            if (arr[i] % 13 == 0) {
                local_sum += arr[i];
                if (arr[i] < local_min) {
                    local_min = arr[i];
                }
            }
        }

        sum.fetch_add(local_sum, memory_order_relaxed);
        int current_min = min_multiple_of_13.load(memory_order_relaxed);
        while (local_min < current_min &&
            !min_multiple_of_13.compare_exchange_weak(current_min, local_min, memory_order_relaxed, memory_order_relaxed)) {}
    };

    vector<thread> threads;
    int chunk_size = arr.size() / num_threads;

    for (int i = 0; i < num_threads; ++i) {
        int start = i * chunk_size;
        int end = (i == num_threads - 1) ? arr.size() : start + chunk_size;
        threads.push_back(thread(process_chunk, start, end));
    }

    for (auto& t : threads) {
        t.join();
    }
}

int main() {
    for (int array_size : array_sizes) {
        vector<int> arr = generate_random_array(array_size);
        cout << "\nArray size: " << array_size << endl;

        auto start = high_resolution_clock::now();
        long long sum = 0;
        int min_multiple_of_13 = INT_MAX;
        without_parallelization(arr, sum, min_multiple_of_13);
        auto end = high_resolution_clock::now();
        chrono::duration<double> duration = end - start;
        cout << "Without parallelization: Time = " << duration.count() << " seconds" << endl;

        for (int threads : thread_counts) {
            start = high_resolution_clock::now();
            sum = 0;
            min_multiple_of_13 = INT_MAX;
            with_mutex(arr, sum, min_multiple_of_13, threads);
            end = high_resolution_clock::now();
            duration = end - start;
            cout << "[" << threads << " threads] With mutex: Time = " << duration.count() << " seconds" << endl;

            atomic<long long> atomic_sum = 0;
            atomic<int> atomic_min_multiple_of_13 = INT_MAX;
            start = high_resolution_clock::now();
            with_atomic(arr, atomic_sum, atomic_min_multiple_of_13, threads);
            end = high_resolution_clock::now();
            duration = end - start;
            cout << "[" << threads << " threads] With atomic: Time = " << duration.count() << " seconds" << endl;
        }
    }

    return 0;
}
