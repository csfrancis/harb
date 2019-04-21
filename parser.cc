#include "parser.h"

namespace harb {

Parser::Parser(FILE *f) : heap_obj_count_(0), f_(f) {}

const char * Parser::get_intern_string(const char *str) {
  assert(str);
  auto it = intern_strings_.find(str);
  if (it != intern_strings_.end()) {
    return *it;
  }
  const char *dup = strdup(str);
  intern_strings_.insert(dup);
  return dup;
}

RubyHeapObj * Parser::create_heap_object(RubyValueType type) {
  return new RubyHeapObj(NULL, type, ++heap_obj_count_);
}

void Parser::parse_obj_references(RubyHeapObj *obj, json_t *refs_array) {
  if (!refs_array) {
    return;
  }
  assert(json_typeof(refs_array) == JSON_ARRAY);
  assert(obj->refs_to.addr == NULL);
  size_t size = json_array_size(refs_array);
  if (size == 0) {
    return;
  }

  uint32_t i;
  obj->refs_to.addr = new uint64_t[size + 1];
  for (i = 0; i < size; ++i) {
    json_t *child_o = json_array_get(refs_array, i);
    assert(child_o);
    assert(json_typeof(child_o) == JSON_STRING);
    uint64_t child_addr = strtoull(json_string_value(child_o), NULL, 0);
    assert(child_addr != 0);
    obj->refs_to.addr[i] = child_addr;
  }
  obj->refs_to.addr[i] = 0;
}

RubyHeapObj * Parser::parse_root_object(json_t *root_o) {
  json_t *name_o = json_object_get(root_o, "root");
  assert(name_o);

  RubyHeapObj *gc_root = create_heap_object(RUBY_T_ROOT);
  gc_root->as.root.name = get_intern_string(json_string_value(name_o));
  parse_obj_references(gc_root, json_object_get(root_o, "references"));
  return gc_root;
}

RubyHeapObj* Parser::parse_heap_object(json_t *heap_o, RubyValueType type) {
  json_t *addr_o = json_object_get(heap_o, "address");
  if (!addr_o) return NULL;

  assert(json_typeof(addr_o) == JSON_STRING);
  const char *addr_s = json_string_value(addr_o);
  assert(addr_s != NULL);

  uint64_t addr = strtoull(addr_s, NULL, 0);
  assert(addr != 0);
  RubyHeapObj *obj = create_heap_object(type);
  obj->flags = parse_flags(heap_o, type);
  obj->as.obj.addr = addr;

  if (type == RUBY_T_STRING) {
    json_t *value_o = json_object_get(heap_o, "value");
    if (value_o) {
      assert(json_typeof(value_o) == JSON_STRING);
      obj->as.obj.as.value = get_intern_string(json_string_value(value_o));
    }
  } else if (type == RUBY_T_OBJECT || type == RUBY_T_ICLASS) {
    json_t *class_o = json_object_get(heap_o, "class");
    if (class_o) {
      assert(json_typeof(class_o) == JSON_STRING);
      obj->as.obj.clazz.addr = strtoull(json_string_value(class_o), NULL, 0);
    }
  } else if (type == RUBY_T_CLASS || type == RUBY_T_MODULE) {
    json_t *name_o = json_object_get(heap_o, "name");
    if (name_o) {
      assert(json_typeof(name_o) == JSON_STRING);
      obj->as.obj.as.value = get_intern_string(json_string_value(name_o));
    }
  } else if (type == RUBY_T_DATA) {
    json_t *struct_o = json_object_get(heap_o, "struct");
    if (struct_o) {
      assert(json_typeof(struct_o) == JSON_STRING);
      obj->as.obj.as.value = get_intern_string(json_string_value(struct_o));
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
  } else if (type == RUBY_T_IMEMO) {
    json_t *name_o = json_object_get(heap_o, "imemo_type");
    if (name_o) {
      assert(json_typeof(name_o) == JSON_STRING);
      obj->as.obj.as.value = get_intern_string(json_string_value(name_o));
    }
  }

  json_t * memsize_o = json_object_get(heap_o, "memsize");
  if (memsize_o) {
    assert(json_typeof(memsize_o) == JSON_INTEGER);
    obj->as.obj.memsize = json_integer_value(memsize_o);
  }

  parse_obj_references(obj, json_object_get(heap_o, "references"));
  return obj;
}

uint32_t Parser::parse_flags(json_t *heap_o, RubyValueType type) {
  uint32_t flags = type;

  if (json_object_get(heap_o, "frozen") == json_true()) {
    flags |= RUBY_FL_FROZEN;
  }

  if (json_object_get(heap_o, "shared") == json_true()) {
    flags |= RUBY_FL_SHARED;
  }

  return flags;
}

RubyHeapObj* Parser::read_heap_object(FILE *f, json_t **json_obj) {
  *json_obj = NULL;

  json_t *o = json_loadf(f, JSON_DISABLE_EOF_CHECK | JSON_ALLOW_NUL, NULL);
  if (o == NULL) {
    return NULL;
  }

  assert(json_typeof(o) == JSON_OBJECT);

  json_t *type_o = json_object_get(o, "type");
  assert(type_o);
  RubyValueType type = RubyHeapObj::get_value_type(json_string_value(type_o));

  RubyHeapObj *heap_obj;
  if (type == RUBY_T_ROOT) {
    heap_obj = parse_root_object(o);
  } else {
    heap_obj = parse_heap_object(o, type);
  }

  *json_obj = o;

  return heap_obj;
}

}
