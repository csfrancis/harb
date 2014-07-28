#include "harb.h"
#include "fields.h"

namespace harb {
namespace field {

node::field<uint64_t> Address("address", [](struct ruby_heap_obj *obj) {
  return &(obj->as.obj.addr);
});

}

node::field_base *
find_field(const char *name)
{
  node::field_base **field = &fields[0];
  while (*field != NULL) {
    if (strcmp(name, (*field)->get_name()) == 0) return *field;
  }
  return NULL;
}

node::field_base* fields[] = {
  &field::Address,
  NULL
};

}
