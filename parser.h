#ifndef HARB_PARSER_H
#define HARB_PARSER_H

#include "ruby_heap_obj.h"

#include <city.h>
#include <sparsehash/sparse_hash_set>

#include <jansson.h>

namespace harb {

class Parser {
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
  typedef google::sparse_hash_set<const char *, CityHasherString, eqstr> StringSet;

  int32_t heap_obj_count_;
  StringSet intern_strings_;
  FILE *f_;

  const char * get_intern_string(const char *str);

  RubyHeapObj* read_heap_object(FILE *f, json_t **json_obj);
  RubyHeapObj* parse_root_object(json_t *root_o);
  RubyHeapObj* parse_heap_object(json_t *heap_o, RubyValueType type);
  void parse_obj_references(RubyHeapObj *obj, json_t *refs_array);
  uint32_t parse_flags(json_t *heap_o, RubyValueType type);

public:

  Parser(FILE *f);

  RubyHeapObj* create_heap_object(RubyValueType type);

  int32_t get_heap_object_count() { return heap_obj_count_; }

  template<typename Func> void parse(Func func) {
    fseeko(f_, 0, SEEK_SET);
    RubyHeapObj *obj = NULL;
    json_t *json_obj = NULL;
    while ((obj = read_heap_object(f_, &json_obj)) != NULL) {
      func(obj, json_obj);
      json_decref(json_obj);
    }
  }
};

}

#endif // HARB_PARSER_H
