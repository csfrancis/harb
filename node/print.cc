#include <cinttypes>

#include "../harb.h"
#include "print.h"
#include "value.h"

namespace harb {
namespace node {

base *
print::execute(base *in, Harb *h) {
  record_set *rs = dynamic_cast<record_set *>(in);
  assert(rs);

  ruby_heap_map *in_map = *(rs->get_value());
  ruby_heap_map::const_iterator it = in_map->begin();
  for (; it != in_map->end(); ++it) {

  }

  if (in_map != h->get_heap_map()) {
    delete in_map;
  }
  return Nil;
}

void
print::print_object(Harb *h, struct ruby_heap_obj *obj) {
  uint32_t type = obj->flags & RUBY_T_MASK;
  if (type == RUBY_T_ROOT) {
    printf("ROOT (%s)\n", obj->as.root.name);
  } else {
    char buf[64] = { 0 };
    const char *p = buf;
    const char *name_title = NULL;
    sprintf(buf, "0x%" PRIx64, obj->as.obj.addr);
    printf("%18s: \"%s\"\n", buf, Harb::get_heap_object_type_string(obj->flags));
    if (type == RUBY_T_DATA) {
      name_title = "struct";
      p = obj->as.obj.as.value;
    } else if (type == RUBY_T_OBJECT || type == RUBY_T_ICLASS) {
      name_title = "name";
      ruby_heap_obj_t *clazz = (*h->get_heap_map())[obj->as.obj.class_addr];
      assert(clazz);
      p = clazz->as.obj.as.value;
    } else if (type == RUBY_T_STRING) {
      name_title = "value";
      p = obj->as.obj.as.value;
    } else if (type == RUBY_T_CLASS || type == RUBY_T_MODULE) {
      name_title = "name";
      p = obj->as.obj.as.value;
    } else if (type == RUBY_T_HASH || type == RUBY_T_ARRAY) {
      name_title = "length";
      sprintf(buf, "%d", obj->as.obj.as.size);
      p = buf;
    }
    if (name_title) {
      printf("%18s: %s%s%s\n", name_title,
        type == RUBY_T_STRING ? "\"" : "",
        p,
        type == RUBY_T_STRING ? "\"" : "");
    }
    printf("%18s: %zu\n", "size", obj->as.obj.memsize);
    if (obj->refs_to) {
      printf("%18s: [\n", "references to");
      for (uint32_t i = 0; obj->refs_to[i] != NULL; ++i) {
        if (obj->as.obj.addr == obj->refs_to[i]->as.obj.addr) continue;
        print_ref_object(h, obj->refs_to[i]);
      }
      printf("%18s  ]\n", "");
    }
    if (obj->refs_from.size() > 0) {
      printf("%18s: [\n", "referenced from");
      for (ruby_heap_obj_list::const_iterator it = obj->refs_from.begin();
           it != obj->refs_from.end();
           ++it) {
        if (obj->as.obj.addr == (*it)->as.obj.addr) continue;
        print_ref_object(h, *it);
      }
      printf("%18s  ]\n", "");
    }
  }
}

const char *
print::print_object_summary(Harb *h, ruby_heap_obj_t *obj, char *buf, size_t buf_sz) {
  uint32_t type = obj->flags & RUBY_T_MASK;
  if (type == RUBY_T_ROOT) {
    return "ROOT";
  }

  char value_buf[128];
  const char *value_bufp = value_buf;
  if (type == RUBY_T_ARRAY || type == RUBY_T_HASH) {
    sprintf(value_buf, "size %d", obj->as.obj.as.size);
  } else if (type == RUBY_T_OBJECT || type == RUBY_T_ICLASS) {
    ruby_heap_obj_t *clazz = (*h->get_heap_map())[obj->as.obj.class_addr];
    assert(clazz);
    value_bufp = clazz->as.obj.as.value;
  } else {
    value_bufp = obj->as.obj.as.value;
  }
  int ret = snprintf(buf, buf_sz, "%s: %s%s%s", Harb::get_heap_object_type_string(obj->flags),
    type == RUBY_T_STRING ? "\"" : "",
    value_bufp,
    type == RUBY_T_STRING ? "\"" : "");
  if (ret < 0) {
    return NULL;
  }
  if ((unsigned) ret >= buf_sz && buf_sz > 3) {
    buf[buf_sz - 2] = '.';
    buf[buf_sz - 3] = '.';
    buf[buf_sz - 4] = '.';
  }
  return buf;
}

void
print::print_ref_object(Harb *h, struct ruby_heap_obj *obj) {
  char buf[64];
  if (h->is_root_object(obj)) {
    printf("%20s  ROOT (%s)\n", "", obj->as.root.name);
  } else {
    printf("%20s  0x%" PRIx64 " (%s)\n", "", obj->as.obj.addr, print_object_summary(h, obj, buf, sizeof(buf)));
  }
}

}
}
