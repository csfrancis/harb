#ifndef HARB__FIELDS_H
#define HARB__FIELDS_H

#include <functional>

#include "node/field.h"

namespace harb {
namespace field {

extern node::field<uint64_t> Address;

}

extern node::field_base* fields[];

node::field_base * find_field(const char *name);

}

#endif // HARB__FIELDS_H
