//
// Created by sungup on 2021/01/17.
//

#ifndef PIDTRACE_TRACKER_H
#define PIDTRACE_TRACKER_H

#include <chrono>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include <unistd.h>
#include "include/process.h"

namespace PIDTrace {
  class PIDTraceError: public std::exception {
  private:
    std::string _message{};

  public:
    explicit PIDTraceError(std::string message) noexcept;
    ~PIDTraceError() override = default;

    [[nodiscard]] const char* what() const noexcept override;
  };

  class ThreadTracker {
  private:
    const int MAX_PIPE_BUFFER = 16;
    const int READ  = 0;
    const int WRITE = 1;

    bool _ready;

    std::ostream& _out;

    int       _pipe[2]{};
    pthread_t _scanner{};

    std::vector<Thread> _latest;

    void _run_issuer(std::chrono::milliseconds sleep_duration);
    void _trace();
    void _trace_new_thread();

    static std::vector<Thread> _load_threads();
    static void *_tracking_handler(void* data);

  public:
    explicit ThreadTracker(std::ostream& output);
    ~ThreadTracker();

    void start(std::chrono::milliseconds sleep_duration);
    void stop();
  };
}


#endif //PIDTRACE_TRACKER_H
