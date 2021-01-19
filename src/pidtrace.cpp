#include <iostream>

#include <boost/program_options.hpp>
#include <fstream>

#include "tracker.h"

namespace po = boost::program_options;

struct ArgOpts {
  int         interval = 100;
  std::string dump_to;
};

ArgOpts parse_opts(int argc, char* argv[]) {
  const int   default_interval = 100;
  const char* default_output   = "stdout";

  ArgOpts opts;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "produce help message")
    ("interval,i", po::value<int>(&opts.interval)->default_value(default_interval), "run interval (millisecond)")
    ("file,f", po::value<std::string>(&opts.dump_to)->default_value(default_output), "default output file path");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << "pidtrace" << std::endl
              << "The pidtrace is a thread creation tracking tool using linux /proc. This tool" << std::endl
              << "will be monitoring the directory structure under /proc periodically, so it can" << std::endl
              << "be missing some threads. If you cannot use the ftrace or strace on your system," << std::endl
              << "this tool will be helpful for tracking new threads. But, if you are familiar" << std::endl
              << "with ftrace or strace, that tools are more powerful than pidtrace! :)" << std::endl
              << std::endl;
    std::cout << desc << std::endl;

    std::cout << "Author: Sungup Moon <sungup@me.com>" << std::endl;
    exit(1);
  }

  return opts;
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
