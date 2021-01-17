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

  size = 0;
  for (const auto & p_path : directory_iterator("/proc")) {
    Thread process;
    int32_t pid, tid;

    if ((0 <= (pid = parse_id(p_path))) && process.load_process(pid)) {
      char task_home[MAX_PID_TASK_PATH_LEN];

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

void control_thread(int send_fd, std::chrono::milliseconds sleep_duration) {
  char buffer(1);

  while (true) {
    std::this_thread::sleep_for(sleep_duration);

    write(send_fd, &buffer, 1);
  }

exit_control_thread:
  close(send_fd);
}

class ThreadTracker {
private:
  std::ostream& _tracker;
  int           _pipe;

public:
  ThreadTracker(std::ostream& output, int i_pipe)
    : _tracker(output),
      _pipe(i_pipe)
  { }

  [[nodiscard]] int pipe_fd() const {
    return _pipe;
  }

  std::ostream& fout() {
    return _tracker;
  }

  void Close() {
    close(this->_pipe);
  }
};

std::vector<PIDTrace::Thread> check_new_thread(std::vector<PIDTrace::Thread>& old, ThreadTracker* tracker) {
  std::vector<PIDTrace::Thread> now = load_thread_list();

  auto tick = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
  auto epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(tick.time_since_epoch());

  auto now_item = now.begin();
  auto old_item = old.begin();

  while (now_item != now.end() && old_item != old.end()) {
    if (old_item->tid() == now_item->tid()) {
      // 1. old.tid == new.tid
      if (*old_item != *now_item) {
        tracker->fout() << epoch.count() << ": " << *now_item << std::endl;
      }

      ++now_item; ++old_item;
    } else if (old_item->tid() < now_item->tid()) {
      // 2. old.tid < new.tid
      ++old_item;

    } else {
      // 3. old.tid > new.tid
      tracker->fout() << epoch.count() << ": " << *now_item << std::endl;
      ++now_item;
    }
  }

  while (now_item != now.end()) {
    tracker->fout() << epoch.count() << ": " << *now_item << std::endl;
    ++now_item;
  }

  return now;
}

void *pipe_handler_routine(void *data) {
  auto* tracker = (ThreadTracker*)data;

  constexpr int MAX_PIPE_BUFFER = 16;
  char  buffer[MAX_PIPE_BUFFER];
  int read_len;

  std::vector<PIDTrace::Thread> threads;

  while (true) {
    read_len = read(tracker->pipe_fd(), buffer, MAX_PIPE_BUFFER);

    for (int i = 0; i < read_len; ++i) {
      if (buffer[i] == '\0') {
        goto exit_pipe_handler;
      }

      threads = check_new_thread(threads, tracker);
    }

  }

  exit_pipe_handler:

  return nullptr;
}

int main() {
  pthread_t scan_thread;
  int       thread_status;
  int       pipe_fd[2];

  auto sleep_duration = std::chrono::milliseconds(100); // 1 second

  if (pipe(pipe_fd) < 0) {
    std::cerr << "pipe creation failed" << std::endl;
    exit(-1);
  }

  ThreadTracker tracker(std::cout, pipe_fd[0]);

  if (pthread_create(&scan_thread, nullptr, pipe_handler_routine, (void*)(&tracker)) != 0) {
    std::cerr << "creation failed" << std::endl;
    exit(-1);
  }

  control_thread(pipe_fd[1], sleep_duration);

  pthread_join(scan_thread, (void**)&thread_status);
  tracker.Close();

  return 0;
}
