#include "../harb.h"
#include "binary_op.h"
#include "value.h"
#include "field.h"

namespace harb {
namespace node {

binary_op::binary_op(base *l, base *r, const char *op) :
  left_(l), right_(r) {
  if (strcmp(op, "==") == 0) {
    type_ = OP_EQ;
  } else if (strcmp(op, "!=") == 0) {
    type_ = OP_NEQ;
  } else if (strcmp(op, ">") == 0) {
    type_ = OP_GT;
  } else if (strcmp(op, "<") == 0) {
    type_ = OP_LT;
  } else if (strcmp(op, ">=") == 0) {
    type_ = OP_GTE;
  } else if (strcmp(op, "<=") == 0) {
    type_ = OP_LTE;
  }
}

binary_op::~binary_op() {
  if (left_ && dynamic_cast<field_base *>(left_) == NULL) {
    delete left_;
  }
  left_ = NULL;
  if (right_ && dynamic_cast<field_base *>(right_) == NULL) {
    delete right_;
    right_ = NULL;
  }
}

base *
binary_op::execute(base *in, ::Harb *h) {
  base *op1, *op2;
  record_set *rs = dynamic_cast<record_set *>(in);

  if (!rs) {
    // todo raise
  }
  validate_operands();
  get_compare_operands(&op1, &op2);

  ruby_heap_map *in_map = *(rs->get_value());
  ruby_heap_map *out_map = new ruby_heap_map();
  auto pred = std::bind(&binary_op::copy_pred, this, op1, op2, std::placeholders::_1);
  std::copy_if(in_map->begin(), in_map->end(), std::inserter(*out_map, out_map->begin()), pred);

  rs->set_value(out_map);
  if (in_map != h->get_heap_map()) {
    delete in_map;
  }

  return rs;
}

bool
binary_op::copy_pred(base *op1, base *op2, const ruby_heap_map_entry &entry) {
  base *ret;

  ret = try_compare<number>(op1, op2, entry.second);
  if (ret != Nil) return ret == True ? true : false;

  ret = try_compare<string>(op1, op2, entry.second);
  if (ret != Nil) return ret == True ? true : false;

  return false;
}

template <class T> base *
binary_op::compare_op(T v1, T v2) {
  switch(type_) {
  case OP_EQ:
    return *v1 == *v2 ? True : False;
  case OP_NEQ:
    return *v1 != *v2 ? True : False;
  case OP_GT:
    return *v1 > *v2 ? True : False;
  case OP_LT:
    return *v1 < *v2 ? True : False;
  case OP_GTE:
    return *v1 >= *v2 ? True : False;
  case OP_LTE:
    return *v1 <= *v2 ? True : False;
  default:
    // todo raise
    return Nil;
  }
}

template <class T> base *
binary_op::try_compare(base *op1, base *op2, struct ruby_heap_obj *obj) {
  T *v1, *v2;
  field<typename T::value_type> *f1;
  if ((v1 = dynamic_cast<T*>(op1)) == NULL) return Nil;

  if ((f1 = dynamic_cast<field<typename T::value_type> *>(op2))) {
    return compare_op(v1->get_value(), f1->get_value(obj));
  } else if ((v2 = dynamic_cast<T*>(op2))) {
    return compare_op(v1->get_value(), v2->get_value());
  }
  printf("unable to compare types\n");
  // todo raise
  return Nil;
}

void
binary_op::validate_operands() {
  if ((dynamic_cast<value_base *>(left_) == NULL && dynamic_cast<field_base *>(left_) == NULL)
       || (dynamic_cast<value_base *>(right_) == NULL && dynamic_cast<field_base *>(right_) == NULL)) {
    printf("cannot compare nodes: %s, %s\n", typeid(left_).name(), typeid(right_).name());
    // todo raise
  }
}

void
binary_op::get_compare_operands(base **o1, base **o2) {
  value_base *op1 = dynamic_cast<value_base *>(right_);
  if (!op1) {
    op1 = dynamic_cast<value_base *>(left_);
    if (!op1) {
      printf("must compare at least one literal value\n");
      // todo raise
    }
  }
  base *op2 = op1 == left_ ? right_ : left_;
  *o1 = op1;
  *o2 = op2;
}

}
}
