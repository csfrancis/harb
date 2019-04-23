#ifndef HARB_DOMINATOR_TREE_H
#define HARB_DOMINATOR_TREE_H

#include <unistd.h>

#include <vector>

#include "ruby_heap_obj.h"
#include "progress.h"

namespace harb {

class DominatorTree {
  public:
    DominatorTree(RubyHeapObj *root, int32_t num_nodes);
    ~DominatorTree();

    void calculate();

    void retained_size(RubyHeapObj *obj, size_t &size);

    RubyHeapObj * get_idom(RubyHeapObj *obj) {
      auto t = tree[obj->get_index()];
      return objs[(*t)[0]];
    }

    void get_dominators(RubyHeapObj *obj, std::vector<RubyHeapObj *> &dominators) {
      auto t = tree[obj->get_index()];
      for (size_t i = 1; i < t->size(); ++i) {
        dominators.push_back(objs[(*t)[i]]);
      }
    }

  private:
    RubyHeapObj *root;
    int32_t num_nodes;
    int32_t count;
    int32_t *arr;
    int32_t *rev;
    int32_t *label;
    int32_t *sdom;
    int32_t *dom;
    int32_t *parent;
    int32_t *dsu;
    RubyHeapObj **objs;
    std::vector<int32_t> **reverse_graph;
    std::vector<int32_t> **bucket;
    std::vector<int32_t> **tree;

    harb::Progress *progress;

    void dfs(RubyHeapObj *node);
    void dfs_child(RubyHeapObj *obj, RubyHeapObj *child);
    void calculate_sdom();
    void cleanup_intermediate_state();

    int32_t find(int32_t u, int32_t x = 0);
    void _union(int32_t u, int32_t v);
};

}

#endif // HARB_DOMINATOR_TREE_H
