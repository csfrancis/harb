#ifndef HARB__NODE__WHERE_H
#define HARB__NODE__WHERE_H

#include "base.h"

namespace harb {
namespace node {

class where : public executable
{
public:
  where(executable *cond) : cond_(cond) { }

  virtual ~where() {
    if (cond_) {
      delete cond_;
      cond_ = NULL;
    }
  }
  virtual base * execute(base * in, Harb *h) {
    return cond_->execute(in, h);
  }

private:

  executable *cond_;
};

}
}

#endif // HARB__NODE__WHERE_H