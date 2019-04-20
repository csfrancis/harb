#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "progress.h"

namespace harb {

Progress::Progress(const char *message, uint64_t total)
  : current(0), total(total), percentage(-1), message(message) {
  show_progress = isatty(STDOUT_FILENO) == 1;
}

void Progress::start() {
  update(0);
}

void Progress::complete() {
  update(total);
  if (show_progress)
    printf("\n");
}

void Progress::print() {
  if (show_progress)
    printf("\r%s (%d%%)", message, percentage);
}

void Progress::clear() {
  if (show_progress)
    printf("\r%*s\r", (int)strlen(message) + 7, "");
}

void Progress::increment(uint64_t amount) {
  update(current + amount);
}

void Progress::update(uint64_t progress) {
  this->current = progress;
  int new_percentage = (progress * 100) / total;
  if (percentage != new_percentage) {
    this->percentage = new_percentage;
    print();
  }
}

}

