#include <iostream>
#include <filesystem>

#include "include/process.h"

using namespace PIDTrace;

int main()
{
  ProcessID sample;

  int counted = 0;

  for (const auto & entry : std::filesystem::directory_iterator("/proc")) {
    int32_t pid = parsePID(entry.path().filename().c_str());

    if (pid < 0 || !sample.load(pid)) {
      continue;
    }

    counted++;
    std::cout << pid << ": " << sample.name() << std::endl;
  }

  std::cout << "found: " << counted << std::endl;

  return 0;
}
