#ifndef HARB__NODE__BINARY_OP_H
#define HARB__NODE__BINARY_OP_H

#include <string>

#include "base.h"
#include "executable.h"

namespace harb {
namespace node {

class binary_op : public executable {
public:
  binary_op(base *l, base *r, const char *op);
  virtual ~binary_op();

  virtual base * execute(base *in, ::Harb *h);

private:
  bool copy_pred(base *op1, base *op2, const ruby_heap_map_entry &entry);
  template <class T> base * compare_op(T v1, T v2);
  template <class T> base * try_compare(base *op1, base *op2, struct ruby_heap_obj *obj);
  void validate_operands();
  void get_compare_operands(base **o1, base **o2);

  typedef enum op_type {
    OP_EQ,
    OP_NEQ,
    OP_GT,
    OP_LT,
    OP_GTE,
    OP_LTE
  } op_type;

  op_type type_;
  base *left_, *right_;
};

}
}

#endif // HARB__NODE__BINARY_OP_H