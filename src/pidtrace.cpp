#include <iostream>

#include "tracker.h"

int main() {
  PIDTrace::ThreadTracker tracker(std::cout);

  tracker.start(std::chrono::milliseconds(100));

  return 0;
}
