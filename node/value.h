#ifndef HARB__NODE__VALUE_H
#define HARB__NODE__VALUE_H

#include <string>

#include "base.h"
#include "../harb.h"

namespace harb {
namespace node {

class value_base : public base { };

template <class T>
class value : public value_base
{
public:
  value(const T val) : val_(val) { }

  T* get_value() { return &val_; }
  void set_value(const T val) { val_ = val; }

  typedef T value_type;

private:
  T val_;
};

typedef value<uint64_t> number;
typedef value<std::string> string;
typedef value<std::string> ident;
typedef value<bool> boolean;
typedef value<void *> pointer;
typedef value<struct ruby_heap_obj *> record;
typedef value<ruby_heap_map *> record_set;

extern boolean * True;
extern boolean * False;
extern pointer * Nil;

}
}

#endif // HARB__NODE__STRING_H