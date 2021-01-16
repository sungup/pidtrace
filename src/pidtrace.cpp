#include <iostream>
#include <filesystem>

#include "include/process.h"

using namespace PIDTrace;

const int64_t MAX_PID_TASK_PATH_LEN = (6 + 7 + 6);

int main()
{
  ProcessID process, thread;
  int32_t pid, tid;
  char pid_task_home[MAX_PID_TASK_PATH_LEN];
  int counted = 0;

  for (const auto & pid_info : std::filesystem::directory_iterator("/proc")) {
    if ((0 <= (pid = parsePID(pid_info.path().filename().c_str()))) &&
        (process.load_process(pid))) {
      // c/c++ mixed manner
      sprintf(pid_task_home, "%s/task", pid_info.path().c_str());

      for (const auto& tid_info : std::filesystem::directory_iterator(
        pid_task_home)) {
        if ((0 <= (tid = parsePID(tid_info.path().filename().c_str()))) &&
            (thread.load_thread(process, tid))) {
          ++counted;

          std::cout << thread << std::endl;
        }
      }
    }
  }

  std::cout << "found: " << counted << std::endl;

  return 0;
}
