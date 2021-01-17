//
// Created by sungup on 2021/01/17.
//

#include <thread>
#include <vector>
#include <filesystem>

#include "tracker.h"

using namespace PIDTrace;
using namespace std::filesystem;

// exception handling
PIDTraceError::PIDTraceError(std::string message) noexcept
  : _message(std::move(message))
{ }

const char* PIDTraceError::what() const noexcept
{
  return this->_message.c_str();
}


// main ThreadTracker object
ThreadTracker::ThreadTracker(std::ostream& output)
  : _out(output)
{
  if (pipe(this->_pipe) < 0) {
    throw PIDTraceError("create _pipe has been failed");
  }
}

ThreadTracker::~ThreadTracker()
{
  close(_pipe[READ]);
  close(_pipe[WRITE]);
}

void ThreadTracker::_run_issuer(std::chrono::milliseconds sleep_duration)
{
  char buffer(1);

  while (true) {
    std::this_thread::sleep_for(sleep_duration);

    write(this->_pipe[WRITE], &buffer, 1);
  }
}

void ThreadTracker::_trace()
{
  constexpr int MAX_PIPE_BUFFER = 16;
  char buffer[MAX_PIPE_BUFFER];

  std::vector<Thread> threads;

  while (true) {
    int len = read(this->_pipe[READ], buffer, MAX_PIPE_BUFFER);

    for (int i = 0; i < len; ++i) {
      if (buffer[i] == '\0') {
        return;
      }

      threads = this->_trace_new_thread(threads);
    }
  }
}

std::vector<Thread> ThreadTracker::_trace_new_thread(std::vector<Thread>& old)
{
  auto now = ThreadTracker::_load_threads();

  auto tick = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
  auto epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(tick.time_since_epoch());

  auto now_item = now.begin();
  auto old_item = old.begin();

  while (now_item != now.end() && old_item != old.end()) {
    if (old_item->tid() == now_item->tid()) {
      // 1. old.tid == new.tid
      if (*old_item != *now_item) {
        this->_out << epoch.count() << ": " << *now_item << std::endl;
      }

      ++now_item; ++old_item;
    } else if (old_item->tid() < now_item->tid()) {
      // 2. old.tid < new.tid
      ++old_item;

    } else {
      // 3. old.tid > new.tid
      this->_out << epoch.count() << ": " << *now_item << std::endl;
      ++now_item;
    }
  }

  while (now_item != now.end()) {
    this->_out << epoch.count() << ": " << *now_item << std::endl;
    ++now_item;
  }

  return now;
}

std::vector<Thread> ThreadTracker::_load_threads()
{
  const int64_t MAX_PID_TASK_PATH_LEN = (6 + 7 + 6);
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

void* ThreadTracker::_tracking_handler(void *data)
{
  ((ThreadTracker *) data)->_trace();

  return nullptr;
}

void ThreadTracker::start(std::chrono::milliseconds sleep_duration)
{
  if (
    pthread_create(&this->_scanner, nullptr, _tracking_handler, (void *) (this)) != 0) {
    throw PIDTraceError("create thread failed");
  }

  this->_run_issuer(sleep_duration);

  int thread_status;
  pthread_join(this->_scanner, (void**)&thread_status);
}
