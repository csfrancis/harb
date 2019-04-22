#include <stdio.h>

#include <jansson.h>

#include "progress.h"
#include "graph.h"
#include "parser.h"

namespace harb {

Graph::Graph(FILE *f) {
  fseeko(f, 0, SEEK_END);
  Progress progress("parsing", ftello(f));
  fseeko(f, 0, SEEK_SET);
  progress.start();

  parser_ = new Parser(f);

  root_ = parser_->create_heap_object(RUBY_T_ROOT);

  parser_->parse([&] (RubyHeapObj *obj) {
    obj->graph = this;
    if (obj->is_root_object()) {
      root_->as.root.children->push_back(obj);
    } else {
      heap_map_[obj->as.obj.addr] = obj;
    }
    progress.update(ftello(f));
  });

  progress.complete();

  update_references();

  build_dominator_tree();
}

void Graph::add_inverse_obj_references(RubyHeapObj *obj) {
  if (obj->refs_to.obj == NULL) {
    return;
  }

  for (int i = 0; obj->refs_to.obj[i] != NULL; ++i) {
    obj->refs_to.obj[i]->refs_from.push_back(obj);
  }
}

void Graph::update_obj_references(RubyHeapObj *obj) {
  if (obj->refs_to.addr) {
    size_t count = 0;
    for (size_t i = 0; obj->refs_to.addr[i]; ++i) {
      RubyHeapObj *ref = get_heap_object(obj->refs_to.addr[i]);
      if (ref) {
        obj->refs_to.obj[count++] = ref;
      } else {
        // TODO: warn here?
      }
    }
    for (size_t i = count; obj->refs_to.addr[i]; ++i) {
      obj->refs_to.addr[i] = 0;
    }
  }

  obj->as.obj.clazz.obj = get_heap_object(obj->as.obj.clazz.addr);
}

void Graph::update_references() {
  size_t num_heap_objects = heap_map_.size();
  RubyHeapObjList *roots = root_->as.root.children;
  harb::Progress progress("updating references", num_heap_objects + roots->size());
  progress.start();
  for (auto it = heap_map_.begin(); it != heap_map_.end(); ++it) {
    update_obj_references(it->second);
    add_inverse_obj_references(it->second);
    progress.increment();
  }
  for (auto it = roots->begin(); it != roots->end(); ++it) {
    update_obj_references(*it);
    add_inverse_obj_references(*it);
    progress.increment();
  }
  progress.complete();
}

void Graph::build_dominator_tree() {
  dominator_tree_ = new DominatorTree(root_, parser_->get_heap_object_count());
  dominator_tree_->calculate();
}

RubyHeapObj* Graph::get_heap_object(uint64_t addr) {
  auto it = heap_map_.find(addr);
  if (it == heap_map_.end()) {
    return NULL;
  }
  return it->second;
}

size_t Graph::get_retained_size(RubyHeapObj *obj) {
  size_t size = 0;
  dominator_tree_->retained_size(obj, size);
  return size;
}

}

