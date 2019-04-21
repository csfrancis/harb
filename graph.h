#ifndef HARB_GRAPH_H
#define HARB_GRAPH_H

#include <stdio.h>

#include <jansson.h>

#include <city.h>
#include <sparsehash/sparse_hash_map>
#include <sparsehash/sparse_hash_set>

#include "ruby_heap_obj.h"
#include "dominator_tree.h"

namespace harb {

class Graph {
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
  typedef google::sparse_hash_set<const char *, CityHasherString, eqstr> string_set_t;
  typedef google::sparse_hash_map<uint64_t, RubyHeapObj *> ruby_heap_map_t;

  RubyHeapObj *root_;
  int32_t heap_obj_count_;
  string_set_t intern_strings_;
  ruby_heap_map_t heap_map_;
  dominator_tree *dominator_tree_;

  const char * get_intern_string(const char *str);

  RubyHeapObj* create_heap_object(RubyValueType type);
  RubyHeapObj* read_heap_object(FILE *f, json_t **json_obj);
  RubyHeapObj* parse_root_object(json_t *root_o);
  RubyHeapObj* parse_heap_object(json_t *heap_o, RubyValueType type);
  void parse_obj_references(RubyHeapObj *obj, json_t *refs_array);
  uint32_t parse_flags(json_t *heap_o, RubyValueType type);

  void add_inverse_obj_references(RubyHeapObj *obj);
  void update_obj_references(RubyHeapObj *obj);
  void update_references();
  void build_dominator_tree();

public:
  Graph();
  Graph(FILE *f);

  RubyHeapObj* get_heap_object(uint64_t addr);
};

}

#endif // HARB_GRAPH_H
