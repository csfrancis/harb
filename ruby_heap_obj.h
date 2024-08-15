#ifndef HARB_RUBY_HEAP_OBJ_H
#define HARB_RUBY_HEAP_OBJ_H

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#include <vector>

namespace harb {

enum RubyValueType {
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
    RUBY_T_UNDEF  = 0x16,

    RUBY_T_IMEMO  = 0x1a,
    RUBY_T_NODE   = 0x1b,
    RUBY_T_ICLASS = 0x1c,
    RUBY_T_ZOMBIE = 0x1d,
    RUBY_T_MOVED  = 0x1e,

    RUBY_T_MASK   = 0x1f
};

#define RUBY_T_ROOT RUBY_T_MASK

enum RubyFlagType {
    RUBY_FL_FROZEN          = 0x20,
    RUBY_FL_EMBEDDED        = 0x40,
    RUBY_FL_FSTRING         = 0x80,
    RUBY_FL_GC_WB_PROTECTED = 0x100,
    RUBY_FL_GC_OLD          = 0x200,
    RUBY_FL_GC_MARKED       = 0x400,
    RUBY_FL_SHARED          = 0x800
};

class RubyHeapObj;
class Graph;
class Parser;

typedef std::vector<RubyHeapObj *> RubyHeapObjList;
typedef std::vector<uint64_t> RubyHeapAddrList;

class RubyHeapObj {
private:
  friend class Graph;
  friend class Parser;

  uint32_t flags;
  uint32_t idx; // unique node index

  Graph *graph;

  union {
    uint64_t *addr;
    RubyHeapObj **obj;
  } refs_to;
  RubyHeapObjList refs_from;

  union {
  struct {
    uint64_t addr;
    union {
      uint64_t addr;
      RubyHeapObj *obj;
    } clazz;
    size_t memsize;
    union {
      const char *value;
      uint32_t size;
    } as;
  } obj;
  struct {
    const char *name;
    RubyHeapObjList *children; // only used by the root_ object
  } root;
  } as;

public:
  RubyHeapObj(Graph *graph, RubyValueType t, int32_t idx);

  bool is_root_object() { return (flags & RUBY_T_MASK) == RUBY_T_ROOT; }

  uint32_t get_flags() { return flags; }

  uint32_t get_index() { return idx; }

  RubyValueType get_type() { return (RubyValueType) (flags & RUBY_T_MASK); }

  bool has_refs_to() { return refs_to.obj != NULL; }

  RubyHeapObj * get_refs_to(size_t index) { return refs_to.obj[index]; }

  const RubyHeapObjList * get_refs_from() { return &refs_from; }

  uint64_t get_addr() { return as.obj.addr; }

  RubyHeapObj * get_class_obj() { return as.obj.clazz.obj; }

  size_t get_memsize() { return as.obj.memsize; }

  const char * get_value() { return as.obj.as.value; }

  uint32_t get_size() { return as.obj.as.size; }

  const char * get_root_name() { return as.root.name; }

  const RubyHeapObjList * get_root_children() { return as.root.children; }

  const char * get_object_summary(char *buf, size_t buf_sz);

  void print_ref_object(FILE *);

  void print_object(FILE *);

  static RubyValueType get_value_type(const char *str);
  static const char * get_value_type_string(uint32_t type);
};

}

#endif // HARB_RUBY_HEAP_OBJ_H
