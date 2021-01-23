#include <cstdlib>
#include <iostream>
#include <fstream>

#include <getopt.h>

#include "tracker.h"

const uint64_t default_interval = 100;
const char*    default_output   = "stdout";

struct ArgOpts {
  uint64_t    interval = 100;
  std::string dump_to;

  ArgOpts(uint64_t interval, const char* dump_to)
    : interval(interval), dump_to(dump_to)
  { }
};

void print_opts(char* argv0, int exit_code) {
  using namespace std;
  cout
    << "pidtrace" << endl
    << "pidtrace is a thread creation tracking tool using linux /proc. This tool will" << endl
    << "be monitoring the directory structure under /proc periodically, so it can be"  << endl
    << "missing some threads. If you cannot use the ftrace or strace on your system,"  << endl
    << "this tool will be helpful for tracking new threads. But, if you are familiar"  << endl
    << "with ftrace or strace, that tools are more powerful than pidtrace! :)"         << endl
    << endl;

  cout
    << "Usage: " << argv0 << " [OPTIONS]"                                  << endl
    << "  -i,--interval [usec] PID scanning interval."                     << endl
    << "                       (Default: " << default_interval << " usec)" << endl
    << "  -o,--output [path]   Dump output target or stdout/stderr"        << endl
    << "                       (Default: " << default_output << ")"        << endl
    << "  -h,--help            Print help message."                        << endl
    << endl;

  cout
    << "Author: Sungup Moon <sungup@me.com>" << endl;

  exit(exit_code);
}

ArgOpts parse_opts(int argc, char* argv[]) {
  ArgOpts opts = {default_interval, default_output};

  static struct option long_options[] = {
    {"help",     no_argument,       nullptr, 'h'},
    {"interval", required_argument, nullptr, 'i'},
    {"output",   required_argument, nullptr, 'o'},
    {nullptr,    no_argument,       nullptr, 0}
  };

  while (true) {
    int opt_index = 0;
    int c = getopt_long(argc, argv, "hi:o:", long_options, &opt_index);

    switch (c) {
      case -1:
      case 0:
        return opts;

      case 'h':
        print_opts(argv[0], 0);
        break;

      case 'i':
        opts.interval = std::strtol(optarg, nullptr, 10);
        break;

      case 'o':
        opts.dump_to = std::string(optarg);
        break;

      default:
        print_opts(argv[0], -1);
        break;
    }
  }
}


int main(int argc, char* argv[]) {
  auto opts = parse_opts(argc, argv);

  std::ofstream f_out;
  std::ostream* out;
  if (opts.dump_to == "stdout") {
    out = &std::cout;
  } else if (opts.dump_to == "stderr") {
    out = &std::cerr;
  } else {
    f_out.open(opts.dump_to, std::ios_base::out);
    out = &f_out;
  }

  // TODO add signal handler to stop threads and close files gracefully

  PIDTrace::ThreadTracker tracker(*out);
  tracker.start(std::chrono::milliseconds(opts.interval));

  if (f_out.is_open()) {
    f_out.close();
  }

  return 0;
}
