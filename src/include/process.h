//
// Created by sungup on 2021/01/13.
//

#ifndef PIDTRACE_PROCESS_H
#define PIDTRACE_PROCESS_H

#include <cstdint>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

#define MAX_COMM_PATH_LEN (1 + 4 + 1 + 7 + 1 + 4 + 1)

namespace PIDTrace {
  union ProcessInfo {
    char        name[16]{};
    __uint128_t hash;
  };

  class ProcessID {
  private:
    int32_t     _pid;
    ProcessInfo _process;

  public:
    inline ProcessID()
      : _pid(-1),
        _process()
    { }

    [[nodiscard]] inline const char* name() const {
      return this->_process.name;
    }

    [[nodiscard]] inline int32_t pid() const {
      return this->_pid;
    }

    // load process file for pid
    [[nodiscard]] inline bool load(int p_id) {
      char comm_path[MAX_COMM_PATH_LEN];

      // read comm file
      sprintf(comm_path, "/proc/%d/comm", p_id);

      int comm_fd = open(comm_path, O_RDONLY);
      if (comm_fd < 0) {
        return false;
      }

      ssize_t comm_len = read(comm_fd, this->_process.name, MAX_COMM_PATH_LEN);
      if (comm_len < 0) {
        close(comm_fd);
        return false;
      }
      this->_process.name[comm_len - 1] = '\0';
      close(comm_fd);

      this->_pid = p_id;
      return true;
    }
  };

  inline int32_t parsePID(const char* dir) {
    int32_t pid = 0;

    while ('0' <= *dir && *dir <= '9') {
      pid = pid * 10 + (*dir++) - '0';
    }

    return (*dir == '\0') ? pid : -1;
  }
}

#endif //PIDTRACE_PROCESS_H
