#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "include/process.h"

using namespace PIDTrace;
using namespace std::filesystem;

const int64_t MAX_PID_TASK_PATH_LEN = (6 + 7 + 6);

std::vector<PIDTrace::Thread> load_thread_list() {
  ssize_t size;
  if ((size = PIDTrace::total_threads()) <= 0) {
    return std::vector<PIDTrace::Thread>();
  }

  const auto processor_count = std::thread::hardware_concurrency();
  auto threads = std::vector<PIDTrace::Thread>(size + processor_count);

  Thread process;
  int32_t pid, tid;
  char task_home[MAX_PID_TASK_PATH_LEN];

  size = 0;
  for (const auto & p_path : directory_iterator("/proc")) {
    if ((0 <= (pid = parse_id(p_path))) && process.load_process(pid)) {
      // c/c++ mixed manner
      sprintf(task_home, "%s/task", p_path.path().c_str());

      for (const auto& t_path : directory_iterator(task_home)) {
        if ((0 <= (tid = parse_id(t_path))) &&
            threads[size].load_thread(process, tid)) {
          ++size;
        }
      }
    }
  }

  threads.resize(size);

  return threads;
}

bool load_thread_list(std::vector<PIDTrace::Thread>& threads) {
  ssize_t size;
  if ((size = PIDTrace::total_threads()) <= 0) {
    return false;
  }

  const auto processor_count = std::thread::hardware_concurrency();
  threads.resize(size + processor_count);

  Thread process;
  int32_t pid, tid;
  char task_home[MAX_PID_TASK_PATH_LEN];

  size = 0;
  for (const auto & p_path : directory_iterator("/proc")) {
    if ((0 <= (pid = parse_id(p_path))) && process.load_process(pid)) {
      // c/c++ mixed manner
      sprintf(task_home, "%s/task", p_path.path().c_str());

      for (const auto& t_path : directory_iterator(task_home)) {
        if ((0 <= (tid = parse_id(t_path))) &&
            threads[size].load_thread(process, tid)) {
          ++size;
        }
      }
    }
  }

  threads.resize(size);

  return true;
}

std::chrono::microseconds return_with_sort(int loop_count) {
  std::chrono::steady_clock::time_point begin, end;

  begin = std::chrono::steady_clock::now();
  for (int i = 0; i < loop_count; ++i) {
    auto threads = load_thread_list();

    std::sort(threads.begin(), threads.end());
  }
  end = std::chrono::steady_clock::now();

  return std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
}

std::chrono::microseconds call_by_ref_with_sort(int loop_count) {
  std::chrono::steady_clock::time_point begin, end;
  std::vector<Thread> threads;

  begin = std::chrono::steady_clock::now();
  for (int i = 0; i < loop_count; ++i) {
    load_thread_list(threads);

    std::sort(threads.begin(), threads.end());
  }
  end = std::chrono::steady_clock::now();

  return std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
}

int main() {
  const int LOOP_COUNT = 100;
  const int SAMPLING_COUNT = 100;

  std::ofstream csv("test_type.csv");

  csv << ",return vector"
      << ",call by ref vector"
      << ",return vector"
      << ",call by ref vector"
      << std::endl;

  for (int i = 0; i < SAMPLING_COUNT; ++i) {
    auto micro_return_with_sort         = return_with_sort(LOOP_COUNT);
    auto micro_call_by_ref_with_sort    = call_by_ref_with_sort(LOOP_COUNT);

    csv << "Try " << i << ","
        << micro_return_with_sort.count() << ","
        << micro_call_by_ref_with_sort.count() << ","
        << micro_return_with_sort.count() / LOOP_COUNT << ","
        << micro_call_by_ref_with_sort.count() / LOOP_COUNT << ","
        << std::endl;
  }

  csv.close();

  return 0;
}
