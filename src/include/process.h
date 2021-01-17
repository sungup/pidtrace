//
// Created by sungup on 2021/01/13.
//

#ifndef PIDTRACE_PROCESS_H
#define PIDTRACE_PROCESS_H

#include <cstdarg>
#include <cstdint>
#include <cstdlib>

#include <filesystem>

#include <fcntl.h>
#include <unistd.h>

namespace PIDTrace {
  typedef std::filesystem::directory_entry dir_entry;

  const int  MAX_PROC_NAME     = 16;
  const int  MAX_COMM_PATH_LEN = (6 + 7 + 6 + 7 + 6);
  const int  MAX_LOADAVG_SIZE  = (7 + 7 + 7 + 8 + 8 + 7);
  const char LOAD_AVG_PATH[]   = "/proc/loadavg";

  class Thread {
    union _comm {
      char        name[MAX_PROC_NAME]{};
      __uint128_t hash;
    };

  private:
    int32_t _pid;
    int32_t _tid;
    _comm   _process;
    _comm   _thread;

    static inline bool _load(char* buffer, const char* format, ...) {
      char path[MAX_COMM_PATH_LEN] = { 0, };

      // get path
      {
        va_list lp_start;
        va_start(lp_start, format);
        vsprintf(path, format, lp_start);
        va_end(lp_start);
      }

      // load _comm file
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
    inline Thread()
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
      if (Thread::_load(this->_process.name, "/proc/%d/comm", pid)) {
        this->_pid = pid;
        return true;
      }

      return false;
    }

    [[nodiscard]] inline bool load_thread(const Thread& process, int tid) {
      if (!Thread::_load(this->_thread.name, "/proc/%d/task/%d/comm", process._pid, tid)) {
        return false;
      }

      this->_process.hash = process._process.hash;
      this->_pid          = process._pid;
      this->_tid          = tid;

      return true;
    }

    [[nodiscard]] inline bool operator<(const Thread& rhs) const {
      return this->_tid < rhs._tid;
    }

    [[nodiscard]] inline bool operator>(const Thread& rhs) const {
      return this->_tid > rhs._tid;
    }

    [[nodiscard]] inline bool operator<=(const Thread& rhs) const {
      return this->_tid <= rhs._tid;
    }

    [[nodiscard]] inline bool operator>=(const Thread& rhs) const {
      return this->_tid >= rhs._tid;
    }

    [[nodiscard]] inline bool operator==(const Thread& rhs) const {
      return this->_tid == rhs._tid && this->_thread.hash == rhs._thread.hash;
    }

    [[nodiscard]] inline bool operator!=(const Thread& rhs) const {
      return this->_tid != rhs._tid || this->_thread.hash != rhs._thread.hash;
    }
  };

  inline std::ostream& operator<<(std::ostream& os, const Thread& info) {
    os << "[" << info.pid() << ":" << info.tid() << "] "
       << info.process() << " / " << info.thread();

    return os;
  }

  inline int32_t parse_id(const dir_entry & directory) {
    const char* dir = directory.path().filename().c_str();
    int32_t pid = 0;

    while ('0' <= *dir && *dir <= '9') {
      pid = pid * 10 + (*dir++) - '0';
    }

    return (*dir == '\0') ? pid : -1;
  }

  inline int32_t total_threads() {
    char buffer[MAX_LOADAVG_SIZE]; int fd;

    int32_t threads = 0;
    if (0 <= (fd = open(LOAD_AVG_PATH, O_RDONLY))) {
      if (0 <= read(fd, buffer, MAX_LOADAVG_SIZE)) {
        char* ptr = buffer;

        while (*ptr++ != '/');
        while (*ptr != ' ') {
          threads = threads * 10 + (*ptr++) - '0';
        }
      }

      close(fd);
    }

    return threads;
  }
}

#endif //PIDTRACE_PROCESS_H
