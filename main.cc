#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <jansson.h>

#include <list>
#include <deque>

#include <city.h>

#include <sparsehash/sparse_hash_map>
#include <sparsehash/sparse_hash_set>

///////////////////////////////////////////////////////////////////////////////
// Ruby heap management
///////////////////////////////////////////////////////////////////////////////

using namespace std;
using google::sparse_hash_set;
using google::sparse_hash_map;

enum ruby_value_type {
    RUBY_T_NONE   = 0x00,

    RUBY_T_OBJECT = 0x01,
    RUBY_T_CLASS  = 0x02,
    RUBY_T_MODULE = 0x03,
    RUBY_T_FLOAT  = 0x04,
    RUBY_T_STRING = 0x05,
    RUBY_T_REGEXP = 0x06,
    RUBY_T_ARRAY  = 0x07,
    RUBY_T_HASH   = 0x08,
    RUBY_T_STRUCT = 0x09,
    RUBY_T_BIGNUM = 0x0a,
    RUBY_T_FILE   = 0x0b,
    RUBY_T_DATA   = 0x0c,
    RUBY_T_MATCH  = 0x0d,
    RUBY_T_COMPLEX  = 0x0e,
    RUBY_T_RATIONAL = 0x0f,

    RUBY_T_NIL    = 0x11,
    RUBY_T_TRUE   = 0x12,
    RUBY_T_FALSE  = 0x13,
    RUBY_T_SYMBOL = 0x14,
    RUBY_T_FIXNUM = 0x15,

    RUBY_T_UNDEF  = 0x1b,
    RUBY_T_NODE   = 0x1c,
    RUBY_T_ICLASS = 0x1d,
    RUBY_T_ZOMBIE = 0x1e,

    RUBY_T_MASK   = 0x1f
};

#define RUBY_T_ROOT RUBY_T_MASK

enum ruby_flag_types {
    RUBY_FL_FROZEN          = 0x20,
    RUBY_FL_EMBEDDED        = 0x40,
    RUBY_FL_FSTRING         = 0x80,
    RUBY_FL_GC_WB_PROTECTED = 0x100,
    RUBY_FL_GC_OLD          = 0x200,
    RUBY_FL_GC_MARKED       = 0x400
};

struct ruby_heap_obj;

typedef list<struct ruby_heap_obj *> ruby_heap_obj_list_t;

typedef struct ruby_heap_obj {
  uint32_t flags;
  struct ruby_heap_obj **refs_to;
  ruby_heap_obj_list_t refs_from;

  union {
  struct {
    uint64_t addr;
    uint64_t class_addr;
    size_t memsize;
    union {
      const char *value;
      uint32_t size;
    } as;
  } obj;
  struct {
    const char *name;
  } root;
  } as;
} ruby_heap_obj_t;

struct eqstr {
  bool operator()(const char* s1, const char* s2) const {
    return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
  }
};

template<class T> class CityHasher;
template<> class CityHasher<const char *> {
public:
  std::size_t operator()(const char * s) const {
    return CityHash64(s, strlen(s));
  }
};

typedef CityHasher<const char *> CityHasherString;
typedef sparse_hash_set<const char *, CityHasherString, eqstr> string_set_t;
typedef sparse_hash_map<uint64_t, struct ruby_heap_obj *> ruby_heap_map_t;

static const size_t kRubyObjectSize = 40;

bool exit_ = false;
string_set_t intern_strings_;
ruby_heap_map_t heap_map_;
ruby_heap_obj_list_t root_objects;

static void
fatal_error(const char *fmt, ...) {
  va_list args;
  fprintf(stderr, "error: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(-1);
}

static ruby_heap_obj_t *
create_heap_object() {
  ruby_heap_obj_t *obj = new ruby_heap_obj_t();
  new (&obj->refs_from) list<struct ruby_heap_obj *>();
  return obj;
}

static ruby_heap_obj_t *
create_heap_object(uint64_t addr) {
  ruby_heap_obj_t *obj = create_heap_object();
  obj->as.obj.addr = addr;
  return obj;
}

static ruby_heap_obj_t *
get_heap_object(uint64_t addr, bool create) {
  ruby_heap_map_t::const_iterator it = heap_map_.find(addr);
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

static inline bool
is_root_object(ruby_heap_obj_t *obj) {
  return (obj->flags & RUBY_T_MASK) == RUBY_T_ROOT;
}

static const char *
get_string(const char *str) {
  assert(str);
  string_set_t::const_iterator it = intern_strings_.find(str);
  if (it != intern_strings_.end()) {
    return *it;
  }
  const char *dup = strdup(str);
  intern_strings_.insert(dup);
  return dup;
}

static uint32_t
get_heap_object_type(const char *type) {
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

static const char *
get_heap_object_type_string(uint32_t type) {
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

static void
build_obj_references(ruby_heap_obj_t *obj, json_t *refs_array) {
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

static void
parse_root_object(json_t *root_o) {
  uint32_t i;
  json_t *name_o = json_object_get(root_o, "root");
  assert(name_o);

  ruby_heap_obj_t *gc_root = create_heap_object();
  gc_root->flags = RUBY_T_ROOT;
  gc_root->as.root.name = get_string(json_string_value(name_o));
  root_objects.push_back(gc_root)
  build_obj_references(gc_root, json_object_get(root_o, "references"));
}

static void
parse_heap_object(json_t *heap_o, uint32_t type) {
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

static void
parse_file(const char *filename) {
  int i;

  FILE *f = fopen(filename, "r");
  if (!f) {
    fatal_error("unable to open %s: %d\n", filename, errno);
  }

  printf("parsing %s .", filename);

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

    if (i % 100000 == 0) { printf("."); }
  }
  printf(" done: %i heap objects\n", i);
}

static const char *
print_object_summary(ruby_heap_obj_t *obj, char *buf, size_t buf_sz) {
  uint32_t type = obj->flags & RUBY_T_MASK;
  if (type == RUBY_T_ROOT) {
    return "ROOT";
  }

  char value_buf[128];
  const char *value_bufp = value_buf;
  if (type == RUBY_T_ARRAY || type == RUBY_T_HASH) {
    sprintf(value_buf, "size %d", obj->as.obj.as.size);
  } else if (type == RUBY_T_OBJECT || type == RUBY_T_ICLASS) {
    ruby_heap_obj_t *clazz = heap_map_[obj->as.obj.class_addr];
    assert(clazz);
    value_bufp = clazz->as.obj.as.value;
  } else {
    value_bufp = obj->as.obj.as.value;
  }
  int ret = snprintf(buf, buf_sz, "%s: %s%s%s", get_heap_object_type_string(obj->flags),
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

static void
print_ref_object(ruby_heap_obj_t *obj) {
  char buf[64];
  if (is_root_object(obj)) {
    printf("%20s  ROOT (%s)\n", "", obj->as.root.name);
  } else {
    printf("%20s  0x%" PRIx64 " (%s)\n", "", obj->as.obj.addr, print_object_summary(obj, buf, sizeof(buf)));
  }
}

static void
print_object(ruby_heap_obj_t *obj) {
  uint32_t type = obj->flags & RUBY_T_MASK;
  if (type == RUBY_T_ROOT) {
    printf("ROOT (%s)\n", obj->as.root.name);
  } else {
    char buf[64] = { 0 };
    const char *p = buf;
    const char *name_title = NULL;
    sprintf(buf, "0x%" PRIx64, obj->as.obj.addr);
    printf("%18s: \"%s\"\n", buf, get_heap_object_type_string(obj->flags));
    if (type == RUBY_T_DATA) {
      name_title = "struct";
      p = obj->as.obj.as.value;
    } else if (type == RUBY_T_OBJECT || type == RUBY_T_ICLASS) {
      name_title = "name";
      ruby_heap_obj_t *clazz = heap_map_[obj->as.obj.class_addr];
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
        print_ref_object(obj->refs_to[i]);
      }
      printf("%18s  ]\n", "");
    }
    if (obj->refs_from.size() > 0) {
      printf("%18s: [\n", "referenced from");
      for (ruby_heap_obj_list_t::const_iterator it = obj->refs_from.begin();
           it != obj->refs_from.end();
           ++it) {
        print_ref_object(*it);
      }
      printf("%18s  ]\n", "");
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Commands
///////////////////////////////////////////////////////////////////////////////

typedef struct command {
  const char *name;
  void (*func)(const char *);
  const char *help;
} command_t;

static void cmd_quit(const char *);
static void cmd_help(const char *);
static void cmd_print(const char *);
static void cmd_rootpath(const char *);

command_t commands_[] = {
  { "print", cmd_print, "Prints heap info for the address specified" },
  { "rootpath", cmd_rootpath, "Display the root path for the object specified" },
  { "help", cmd_help, "Displays this message"},
  { "quit", cmd_quit, "Exits the program" },
  { NULL, NULL, NULL }
};

static void
cmd_quit(const char *) {
  exit_ = true;
}

static void
cmd_help(const char *) {
  printf("You can run the following commands:\n\n");
  for (int i = 0; commands_[i].name != NULL; ++i) {
    printf("\t%10s - %s\n", commands_[i].name, commands_[i].help);
  }
  printf("\n");
}

static ruby_heap_obj_t *
get_ruby_heap_obj_arg(const char *args) {
  if (args == NULL || strlen(args) == 0) {
    printf("error: you must specify an address\n");
    return NULL;
  }

  uint64_t addr = strtoull(args, NULL, 0);
  if (addr == 0) {
    printf("error: you must specify a valid heap address\n");
    return NULL;
  }

  ruby_heap_obj_t *obj = heap_map_[addr];
  if (!obj) {
    printf("error: no ruby object found at address 0x%" PRIx64 "\n", addr);
    return NULL;
  }

  return obj;
}

static void
cmd_print(const char *args) {
  ruby_heap_obj_t *obj = get_ruby_heap_obj_arg(args);
  if (!obj) {
    return;
  }

  print_object(obj);
}

static void
cmd_rootpath(const char *args) {
  bool found = false;
  ruby_heap_obj_t *obj = get_ruby_heap_obj_arg(args);
  if (!obj) {
    return;
  }

  ruby_heap_obj_t *cur;
  deque<ruby_heap_obj_t *> q;
  sparse_hash_set<ruby_heap_obj_t *> visited;
  sparse_hash_map<ruby_heap_obj_t *, ruby_heap_obj_t *> parent;

  q.push_back(obj);
  visited.insert(obj);

  while (!q.empty() && !found) {
    cur = q.front();
    q.pop_front();

    for (ruby_heap_obj_list_t::const_iterator it = cur->refs_from.begin();
         it != cur->refs_from.end();
         ++it) {
      if (visited.find(*it) == visited.end()) {
        visited.insert(*it);
        parent[*it] = cur;
        if (is_root_object(*it)) {
          cur = *it;
          found = true;
          break;
        }
        q.push_back(*it);
      }
    }
  }

  if (!found) {
    printf("error: could not find path to root for 0x%" PRIx64 "\n", obj->as.obj.addr);
    return;
  }

  printf("\nroot path to 0x%" PRIx64 ":\n", obj->as.obj.addr);
  while (cur != NULL) {
    print_ref_object(cur);
    cur = parent[cur];
  }
  printf("\n");
}

static void execute_command(char *line) {
  char *cmd = line;
  char *args;
  char *end = line + strlen(line) - 1;

  // Trim any whitespace from the command and point args
  // to the first argument after 'cmd'
  while (*cmd == ' ') {
    cmd++;
  }
  while (*end == ' ') {
    *end-- = '\0';
  }

  args = cmd;
  while (*args != ' ' && *args != '\0') {
    args++;
  }
  if (*args != '\0') {
    *args++ = '\0';
    while (*args == ' ') {
      args++;
    }
  }

  for (int i = 0; commands_[i].name != NULL; ++i) {
    command_t *c = &commands_[i];
    if (strcmp(c->name, cmd) == 0) {
      c->func(args);
      return;
    }
  }

  printf("unknown command: %s\n", cmd);
}

///////////////////////////////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv) {
  char *line;

  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc < 2) {
    fatal_error("objectspace json dump file required\n");
    return -1;
  }

  parse_file(argv[1]);

  while (!exit_) {
    line = readline("harb> ");

    if (line == NULL) {
      break;
    } else {
      add_history(line);
    }

    execute_command(line);

    free(line);
  }

  return 0;
}
