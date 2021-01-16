//
// Created by sungup on 2021/01/13.
//

#ifndef PIDTRACE_PROCESS_H
#define PIDTRACE_PROCESS_H

#include <cstdarg>
#include <cstdint>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

namespace PIDTrace {
  const int MAX_PROC_NAME     = 16;
  const int MAX_COMM_PATH_LEN = (6 + 7 + 6 + 7 + 6);

  union ProcessInfo {
    char        name[MAX_PROC_NAME]{};
    __uint128_t hash;
  };

  class ProcessID {
  private:
    int32_t     _pid;
    int32_t     _tid;
    ProcessInfo _process;
    ProcessInfo _thread;

    static inline bool _load(char* buffer, const char* format, ...) {
      char path[MAX_COMM_PATH_LEN] = { 0, };

      // get path
      {
        va_list lp_start;
        va_start(lp_start, format);
        vsprintf(path, format, lp_start);
        va_end(lp_start);
      }

      // load comm file
      {
        int fd;
        ssize_t len;

        if ((fd = open(path, O_RDONLY)) < 0) {
          return false;
        }

        if (0 < (len = read(fd, buffer, MAX_PROC_NAME))) {
          buffer[len - 1] = '\0';
        }

        close(fd);

        return 0 < len;
      }
    }

  public:
    inline ProcessID()
      : _pid(-1),
        _tid(-1),
        _process(),
        _thread()
    { }

    [[nodiscard]] inline const char* process() const {
      return this->_process.name;
    }

    [[nodiscard]] inline const char* thread() const {
      return this->_thread.name;
    }

    [[nodiscard]] inline int32_t pid() const {
      return this->_pid;
    }

    [[nodiscard]] inline int32_t tid() const {
      return this->_tid;
    }

    [[nodiscard]] inline bool load_process(int pid) {
      if (ProcessID::_load(this->_process.name, "/proc/%d/comm", pid)) {
        this->_pid = pid;
        return true;
      }

      return false;
    }

    [[nodiscard]] inline bool load_thread(const ProcessID& process, int tid) {
      if (!ProcessID::_load(this->_thread.name, "/proc/%d/task/%d/comm", process._pid, tid)) {
        return false;
      }

      this->_process.hash = process._process.hash;
      this->_pid          = process._pid;
      this->_tid          = tid;

      return true;
    }
  };

  std::ostream& operator<<(std::ostream& os, const ProcessID& info) {
    os << "[" << info.pid() << ":" << info.tid() << "] "
       << info.process() << " / " << info.thread();

    return os;
  }

  inline int32_t parsePID(const char* dir) {
    int32_t pid = 0;

    while ('0' <= *dir && *dir <= '9') {
      pid = pid * 10 + (*dir++) - '0';
    }

    return (*dir == '\0') ? pid : -1;
  }
}

#endif //PIDTRACE_PROCESS_H
