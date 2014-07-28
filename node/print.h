#ifndef HARB__NODE__PRINT_H
#define HARB__NODE__PRINT_H

#include "executable.h"

namespace harb {
namespace node {

class print : public executable
{
public:
  virtual base * execute(base *in, Harb *h);

protected:
  void print_ref_object(Harb *h, struct ruby_heap_obj *obj);

private:
  void print_object(Harb *h, struct ruby_heap_obj *obj);
  const char * print_object_summary(Harb *h, struct ruby_heap_obj *obj, char *buf, size_t buf_sz);
};

}
}

#endif // HARB__NODE__PRINT_H