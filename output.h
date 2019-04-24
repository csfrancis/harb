#include <stdio.h>
#include <unistd.h>

namespace harb {

class Output {
  static bool use_pager_;

public:
  static void initialize() {
    if (isatty(fileno(stdin))) {
      use_pager_ = true;
    } else {
      use_pager_ = false;
    }
  }

  template<typename Func> static void with_handle(Func func) {
    FILE *output;
    if (use_pager_) {
      output = popen("/usr/bin/less -XF", "w");
    } else {
      output = stdout;
    }

    func(output);

    if (output != stdout) {
      pclose(output);
    }
  }
};


}

