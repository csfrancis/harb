#ifndef HARB__NODE__EXECUTABLE_H
#define HARB__NODE__EXECUTABLE_H

#include "base.h"

class Harb;

namespace harb {
namespace node {

class executable : public base {
public:
  virtual base * execute(base *input, Harb *h) = 0;
};

}
}

#endif // HARB__NODE__EXECUTABLE_H