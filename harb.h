#ifndef __HARB_H
#define __HARB_H

#include <city.h>

#include <list>
#include <cstdint>
#include <sparsehash/sparse_hash_map>
#include <sparsehash/sparse_hash_set>

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

typedef std::list<struct ruby_heap_obj *> ruby_heap_obj_list;

typedef struct ruby_heap_obj {
  uint32_t flags;
  struct ruby_heap_obj **refs_to;
  ruby_heap_obj_list refs_from;

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
} ruby_heap_obj;

typedef std::pair<uint64_t, struct ruby_heap_obj *> ruby_heap_map_entry;
typedef google::sparse_hash_map<uint64_t, struct ruby_heap_obj *> ruby_heap_map;

struct json_t;

namespace harb { namespace node { class block; }}

template<class T> class CityHasher;
template<> class CityHasher<const char *> {
public:
  std::size_t operator()(const char * s) const {
    return CityHash64(s, strlen(s));
  }
};

class Harb {
 public:
  Harb();
  ~Harb();

  void load_file(const char *filename);
  static int parse_command(const char *str, harb::node::block *root);

  const char * get_string(const char *str);
  static uint32_t get_heap_object_type(const char *type);
  static const char * get_heap_object_type_string(uint32_t type);

  ruby_heap_obj * get_heap_object(uint64_t addr, bool create);
  ruby_heap_map * get_heap_map() { return &heap_map_; }

  static inline bool is_root_object(ruby_heap_obj *obj) {
    return (obj->flags & RUBY_T_MASK) == RUBY_T_ROOT;
  }

 private:
  struct eqstr {
    bool operator()(const char* s1, const char* s2) const {
      return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
    }
  };

  typedef CityHasher<const char *> CityHasherString;
  typedef google::sparse_hash_set<const char *, CityHasherString, eqstr> string_set;

  static ruby_heap_obj * create_heap_object();
  static ruby_heap_obj * create_heap_object(uint64_t addr);

  void build_obj_references(ruby_heap_obj *obj, json_t *refs_array);
  void parse_root_object(json_t *root_o);
  void parse_heap_object(json_t *heap_o, uint32_t type);

  ruby_heap_map heap_map_;
  ruby_heap_obj_list root_objects_;
  string_set *intern_strings_;
};

#endif // __HARB_H