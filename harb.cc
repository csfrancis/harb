#include <cstdio>
#include <jansson.h>
#include <city.h>
#include <sparsehash/sparse_hash_set>

#include "harb.h"

using google::sparse_hash_set;
using namespace std;

static const size_t kRubyObjectSize = 40;


Harb::Harb() {
  intern_strings_ = new string_set();
}

Harb::~Harb() {
  if (intern_strings_) {
    delete intern_strings_;
    intern_strings_ = NULL;
  }
}

ruby_heap_obj *
Harb::create_heap_object() {
  ruby_heap_obj *obj = new ruby_heap_obj();
  new (&obj->refs_from) list<ruby_heap_obj *>();
  return obj;
}

ruby_heap_obj *
Harb::create_heap_object(uint64_t addr) {
  ruby_heap_obj *obj = create_heap_object();
  obj->as.obj.addr = addr;
  return obj;
}

ruby_heap_obj *
Harb::get_heap_object(uint64_t addr, bool create) {
  ruby_heap_map::const_iterator it = heap_map_.find(addr);
  if (it == heap_map_.end()) {
    if (!create) {
      return NULL;
    }
    ruby_heap_obj_t *obj = create_heap_object(addr);
    heap_map_[addr] = obj;
    return obj;
  }
  return it->second;
}

const char *
Harb::get_string(const char *str) {
  assert(str);
  string_set::const_iterator it = intern_strings_->find(str);
  if (it != intern_strings_->end()) {
    return *it;
  }
  const char *dup = strdup(str);
  intern_strings_->insert(dup);
  return dup;
}

uint32_t
Harb::get_heap_object_type(const char *type) {
  assert(type);
  if (strcmp(type, "OBJECT") == 0) {
    return RUBY_T_OBJECT;
  } else if (strcmp(type, "STRING") == 0) {
    return RUBY_T_STRING;
  } else if (strcmp(type, "HASH") == 0) {
    return RUBY_T_HASH;
  } else if (strcmp(type, "ARRAY") == 0) {
    return RUBY_T_ARRAY;
  } else if (strcmp(type, "CLASS") == 0) {
    return RUBY_T_CLASS;
  } else if (strcmp(type, "ICLASS") == 0) {
    return RUBY_T_ICLASS;
  } else if (strcmp(type, "DATA") == 0) {
    return RUBY_T_DATA;
  } else if (strcmp(type, "MODULE") == 0) {
    return RUBY_T_MODULE;
  } else if (strcmp(type, "STRUCT") == 0) {
    return RUBY_T_STRUCT;
  } else if (strcmp(type, "NODE") == 0) {
    return RUBY_T_NODE;
  } else if (strcmp(type, "REGEXP") == 0) {
    return RUBY_T_REGEXP;
  } else if (strcmp(type, "BIGNUM") == 0) {
    return RUBY_T_BIGNUM;
  } else if (strcmp(type, "FLOAT") == 0) {
    return RUBY_T_FLOAT;
  } else if (strcmp(type, "FILE") == 0) {
    return RUBY_T_FILE;
  } else if (strcmp(type, "MATCH") == 0) {
    return RUBY_T_MATCH;
  } else if (strcmp(type, "COMPLEX") == 0) {
    return RUBY_T_COMPLEX;
  } else if (strcmp(type, "RATIONAL") == 0) {
    return RUBY_T_RATIONAL;
  } else if (strcmp(type, "NIL") == 0) {
    return RUBY_T_NIL;
  } else if (strcmp(type, "TRUE") == 0) {
    return RUBY_T_TRUE;
  } else if (strcmp(type, "FALSE") == 0) {
    return RUBY_T_FALSE;
  } else if (strcmp(type, "SYMBOL") == 0) {
    return RUBY_T_SYMBOL;
  } else if (strcmp(type, "FIXNUM") == 0) {
    return RUBY_T_FIXNUM;
  } else if (strcmp(type, "UNDEF") == 0) {
    return RUBY_T_UNDEF;
  } else if (strcmp(type, "ZOMBIE") == 0) {
    return RUBY_T_ZOMBIE;
  } else if (strcmp(type, "ROOT") == 0) {
    return RUBY_T_ROOT;
  }
  return RUBY_T_NONE;
}

const char *
Harb::get_heap_object_type_string(uint32_t type) {
  uint32_t t = type & RUBY_T_MASK;
  if (t == RUBY_T_OBJECT) {
    return "OBJECT";
  } else if (t == RUBY_T_STRING) {
    return "STRING";
  } else if (t == RUBY_T_HASH) {
    return "HASH";
  } else if (t == RUBY_T_ARRAY) {
    return "ARRAY";
  } else if (t == RUBY_T_CLASS) {
    return "CLASS";
  } else if (t == RUBY_T_ICLASS) {
    return "ICLASS";
  } else if (t == RUBY_T_DATA) {
    return "DATA";
  } else if (t == RUBY_T_MODULE) {
    return "MODULE";
  } else if (t == RUBY_T_STRUCT) {
    return "STRUCT";
  } else if (t == RUBY_T_NODE) {
    return "NODE";
  } else if (t == RUBY_T_REGEXP) {
    return "REGEXP";
  } else if (t == RUBY_T_BIGNUM) {
    return "BIGNUM";
  } else if (t == RUBY_T_FLOAT) {
    return "FLOAT";
  } else if (t == RUBY_T_FILE) {
    return "FILE";
  } else if (t == RUBY_T_MATCH) {
    return "MATCH";
  } else if (t == RUBY_T_COMPLEX) {
    return "COMPLEX";
  } else if (t == RUBY_T_RATIONAL) {
    return "RATIONAL";
  } else if (t == RUBY_T_NIL) {
    return "NIL";
  } else if (t == RUBY_T_TRUE) {
    return "TRUE";
  } else if (t == RUBY_T_FALSE) {
    return "FALSE";
  } else if (t == RUBY_T_SYMBOL) {
    return "SYMBOL";
  } else if (t == RUBY_T_FIXNUM) {
    return "FIXNUM";
  } else if (t == RUBY_T_UNDEF) {
    return "UNDEF";
  } else if (t == RUBY_T_ZOMBIE) {
    return "ZOMBIE";
  } else if (t == RUBY_T_ROOT) {
    return "ROOT";
  }
  return "NONE";
}

void
Harb::load_file(const char *filename) {
  int i;

  FILE *f = fopen(filename, "r");
  if (!f) {
    throw runtime_error("unable to open file");
  }

  json_t *o = json_loadf(f, JSON_DISABLE_EOF_CHECK | JSON_ALLOW_NUL, NULL);
  for (i = 0; o != NULL; json_decref(o), o = json_loadf(f, JSON_DISABLE_EOF_CHECK | JSON_ALLOW_NUL, NULL), ++i) {
    assert(json_typeof(o) == JSON_OBJECT);

    json_t *type_o = json_object_get(o, "type");
    assert(type_o);
    uint32_t type = get_heap_object_type(json_string_value(type_o));

    switch (type) {
      case RUBY_T_ROOT:
      {
        parse_root_object(o);
        break;
      }
      default:
      {
        parse_heap_object(o, type);
        break;
      }
    }
  }
}

void
Harb::build_obj_references(ruby_heap_obj_t *obj, json_t *refs_array) {
  if (!refs_array) {
    return;
  }
  assert(json_typeof(refs_array) == JSON_ARRAY);
  assert(obj->refs_to == NULL);
  size_t size = json_array_size(refs_array);
  if (size == 0) {
    return;
  }

  uint32_t i;
  obj->refs_to = new ruby_heap_obj_t*[size + 1];
  for (i = 0; i < size; ++i) {
    json_t *child_o = json_array_get(refs_array, i);
    assert(child_o);
    assert(json_typeof(child_o) == JSON_STRING);
    uint64_t child_addr = strtoull(json_string_value(child_o), NULL, 0);
    assert(child_addr != 0);
    ruby_heap_obj_t *child = get_heap_object(child_addr, true);
    assert(child);
    child->refs_from.push_front(obj);
    obj->refs_to[i] = child;
  }
  obj->refs_to[i] = NULL;
}

void
Harb::parse_root_object(json_t *root_o) {
  json_t *name_o = json_object_get(root_o, "root");
  assert(name_o);

  ruby_heap_obj_t *gc_root = create_heap_object();
  gc_root->flags = RUBY_T_ROOT;
  gc_root->as.root.name = get_string(json_string_value(name_o));
  root_objects_.push_back(gc_root);
  build_obj_references(gc_root, json_object_get(root_o, "references"));
}

void
Harb::parse_heap_object(json_t *heap_o, uint32_t type) {
  json_t *addr_o = json_object_get(heap_o, "address");
  if (!addr_o) return;

  assert(json_typeof(addr_o) == JSON_STRING);
  const char *addr_s = json_string_value(addr_o);
  assert(addr_s != NULL);

  uint64_t addr = strtoull(addr_s, NULL, 0);
  assert(addr != 0);
  ruby_heap_obj_t *obj = get_heap_object(addr, true);
  obj->flags = type;

  if (type == RUBY_T_STRING) {
    json_t *value_o = json_object_get(heap_o, "value");
    if (value_o) {
      assert(json_typeof(value_o) == JSON_STRING);
      obj->as.obj.as.value = get_string(json_string_value(value_o));
    }
  } else if (type == RUBY_T_OBJECT || type == RUBY_T_ICLASS) {
    json_t *class_o = json_object_get(heap_o, "class");
    if (class_o) {
      assert(json_typeof(class_o) == JSON_STRING);
      obj->as.obj.class_addr = strtoull(json_string_value(class_o), NULL, 0);
    }
  } else if (type == RUBY_T_CLASS || type == RUBY_T_MODULE) {
    json_t *name_o = json_object_get(heap_o, "name");
    if (name_o) {
      assert(json_typeof(name_o) == JSON_STRING);
      obj->as.obj.as.value = get_string(json_string_value(name_o));
    }
  } else if (type == RUBY_T_DATA) {
    json_t *struct_o = json_object_get(heap_o, "struct");
    if (struct_o) {
      assert(json_typeof(struct_o) == JSON_STRING);
      obj->as.obj.as.value = get_string(json_string_value(struct_o));
    }
  } else if (type == RUBY_T_HASH) {
    json_t *size_o = json_object_get(heap_o, "size");
    if (size_o) {
      assert(json_typeof(size_o) == JSON_INTEGER);
      obj->as.obj.as.size = json_integer_value(size_o);
    }
  } else if (type == RUBY_T_ARRAY) {
    json_t *length_o = json_object_get(heap_o, "length");
    if (length_o) {
      assert(json_typeof(length_o) == JSON_INTEGER);
      obj->as.obj.as.size = json_integer_value(length_o);
    }
  }

  json_t * memsize_o = json_object_get(heap_o, "memsize");
  if (memsize_o) {
    assert(json_typeof(memsize_o) == JSON_INTEGER);
    obj->as.obj.memsize = json_integer_value(memsize_o);
  }
  obj->as.obj.memsize += kRubyObjectSize;

  build_obj_references(obj, json_object_get(heap_o, "references"));
}

