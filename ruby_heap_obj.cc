#include <inttypes.h>

#include "ruby_heap_obj.h"
#include "graph.h"

namespace harb {

RubyHeapObj::RubyHeapObj(Graph *graph, RubyValueType t, int32_t idx)
  : flags(t), idx(idx), graph(graph) {
  refs_to.addr = NULL;
  as.obj.clazz.addr = 0;

  if (t == RUBY_T_ROOT) {
    as.root.children = new RubyHeapObjList();
  }
}

RubyValueType RubyHeapObj::get_value_type(const char *type) {
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
  } else if (strcmp(type, "IMEMO") == 0) {
    return RUBY_T_IMEMO;
  }
  return RUBY_T_NONE;
}

const char * RubyHeapObj::get_value_type_string(uint32_t type) {
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
  } else if (t == RUBY_T_IMEMO) {
    return "IMEMO";
  }
  return "NONE";
}

const char * RubyHeapObj::get_object_summary(char *buf, size_t buf_sz) {
  uint32_t type = flags & RUBY_T_MASK;
  if (type == RUBY_T_ROOT) {
    return "ROOT";
  }

  char value_buf[128];
  const char *value_bufp = value_buf;
  if (type == RUBY_T_ARRAY || type == RUBY_T_HASH) {
    sprintf(value_buf, "size %d", get_size());
  } else if (type == RUBY_T_OBJECT || type == RUBY_T_ICLASS) {
    value_bufp = get_class_obj()->get_value();
  } else if (type == RUBY_T_STRING && flags & RUBY_FL_SHARED) {
    value_bufp = get_refs_to(0)->get_value();
  } else {
    value_bufp = get_value();
  }
  int ret = snprintf(buf, buf_sz, "%s: %s%s%s", get_value_type_string(flags),
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

void RubyHeapObj::print_ref_object() {
  char buf[64];
  if (is_root_object()) {
    printf("%20s  ROOT (%s)\n", "", get_root_name());
  } else {
    printf("%20s  0x%" PRIx64 " (%s)\n", "", get_addr(), get_object_summary(buf, sizeof(buf)));
  }
}

void RubyHeapObj::print_object() {
  uint32_t type = flags & RUBY_T_MASK;
  if (type == RUBY_T_ROOT) {
    printf("ROOT (%s)\n", get_root_name());
  } else {
    char buf[64] = { 0 };
    const char *p = buf;
    const char *name_title = NULL;
    sprintf(buf, "0x%" PRIx64, get_addr());
    printf("%18s: \"%s\"\n", buf, get_value_type_string(flags));
    if (type == RUBY_T_DATA) {
      name_title = "struct";
      p = get_value();
    } else if (type == RUBY_T_OBJECT || type == RUBY_T_ICLASS) {
      name_title = type == RUBY_T_OBJECT ? "class" : "name";
      p = get_class_obj()->get_value();
    } else if (type == RUBY_T_STRING) {
      name_title = "value";
      p = get_value();
    } else if (type == RUBY_T_CLASS || type == RUBY_T_MODULE) {
      name_title = "name";
      p = get_value();
    } else if (type == RUBY_T_HASH || type == RUBY_T_ARRAY) {
      name_title = "length";
      sprintf(buf, "%'d", get_size());
      p = buf;
    } else if (type == RUBY_T_IMEMO) {
      name_title = "imemo_type";
      p = get_value();
    }
    if (name_title) {
      printf("%18s: %s%s%s\n", name_title,
        type == RUBY_T_STRING ? "\"" : "",
        p,
        type == RUBY_T_STRING ? "\"" : "");
    }

    printf("%18s: %'zu\n", "memsize", get_memsize());

    printf("%18s: %'zu\n", "retained memsize", graph->get_retained_size(this));

    if (flags & RUBY_FL_SHARED) {
      printf("%18s: %s\n", "shared", "true");
    }

    if (flags & RUBY_FL_FROZEN) {
      printf("%18s: %s\n", "frozen", "true");
    }

    if (has_refs_to()) {
      printf("%18s: [\n", "references to");
      for (uint32_t i = 0; refs_to.obj[i]; ++i) {
        refs_to.obj[i]->print_ref_object();
      }
      printf("%18s  ]\n", "");
    }
    if (refs_from.size() > 0) {
      printf("%18s: [\n", "referenced from");
      for (auto it = refs_from.begin(); it != refs_from.end(); ++it) {
        (*it)->print_ref_object();
      }
      printf("%18s  ]\n", "");
    }
  }
}
}
