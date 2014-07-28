#ifndef HARB__NODE__FIELD_H
#define HARB__NODE__FIELD_H

#include <functional>

#include "base.h"
#include "value.h"

namespace harb {
namespace node {

class field_base : public base
{
public:
  field_base(const char *name) : name_(name) { }

  const char * get_name() { return name_; }

private:
  const char *name_;
};

template <class T>
class field : public field_base
{
public:
  typedef std::function<T*(struct ruby_heap_obj *)> get_value_func;

  field(const char *name, get_value_func func) : field_base(name), func_(func) { }

  T* get_value(struct ruby_heap_obj * obj) { return func_(obj); }

  typedef value<T> value_type;

private:
  get_value_func func_;
};

}
}

#endif // HARB__NODE__FIELD_H