#include "dominator_tree.h"

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

namespace harb {

DominatorTree::DominatorTree(RubyHeapObj *root, int32_t num_nodes)
  : root(root), num_nodes(num_nodes + 1), count(0) {
  arr = new int32_t[num_nodes];
  rev = new int32_t[num_nodes];
  label = new int32_t[num_nodes];
  sdom = new int32_t[num_nodes];
  dom = new int32_t[num_nodes];
  parent = new int32_t[num_nodes];
  dsu = new int32_t[num_nodes];
  objs = new RubyHeapObj*[num_nodes];

  reverse_graph = new std::vector<int32_t>*[num_nodes];
  bucket = new std::vector<int32_t>*[num_nodes];
  tree = new std::vector<int32_t>*[num_nodes];

  for (int32_t i = 0; i < this->num_nodes; ++i) {
    reverse_graph[i] = new std::vector<int32_t>();
    bucket[i] = new std::vector<int32_t>();
    tree[i] = new std::vector<int32_t>();
  }

  progress = new harb::Progress("generating dominator tree", num_nodes * 3);
}

DominatorTree::~DominatorTree() {
  delete objs;

  for (int32_t i = 0; i < this->num_nodes; ++i) {
    delete tree[i];
  }

  delete tree;
}

void DominatorTree::dfs_child(RubyHeapObj *obj, RubyHeapObj *child) {
  int32_t u = obj->get_index();
  int32_t w = child->get_index();

  if (!arr[w]) {
    dfs(child);
    parent[arr[w]] = arr[u];
  }

  reverse_graph[arr[w]]->push_back(arr[u]);
}

void DominatorTree::dfs(RubyHeapObj *obj) {
  count++;
  arr[obj->get_index()] = count;
  rev[count] = obj->get_index();
  label[count] = count;
  sdom[count] = count;
  dsu[count] = count;
  objs[obj->get_index()] = obj;

  progress->increment();

  if (unlikely(obj == root)) {
    for(auto it = obj->get_root_children()->cbegin(); it != obj->get_root_children()->cend(); it++) {
      dfs_child(obj, *it);
    }
  } else if(obj->has_refs_to()) {
    for (uint32_t i = 0; obj->get_refs_to(i) != NULL; ++i) {
      dfs_child(obj, obj->get_refs_to(i));
    }
  }
}

int32_t DominatorTree::find(int32_t u, int32_t x) {
  if (u == dsu[u]) {
    return x ? -1 : u;
  }

  int32_t v = find(dsu[u], x + 1);
  if (v < 0) {
    return u;
  }

  if (sdom[label[dsu[u]]] < sdom[label[u]]) {
    label[u] = label[dsu[u]];
  }

  dsu[u] = v;
  return x ? v : label[u];
}

void DominatorTree::_union(int32_t u, int32_t v) {
  dsu[v] = u;
}

void DominatorTree::calculate_sdom() {
  for (int32_t i = count; i >= 1; i--) {
    for (uint32_t j = 0; j < reverse_graph[i]->size(); j++) {
      sdom[i] = std::min(sdom[i], sdom[find((*reverse_graph[i])[j])]);
    }

    if (i > 1) {
      bucket[sdom[i]]->push_back(i);
    }

    for (uint32_t j = 0; j < bucket[i]->size(); j++) {
      int32_t w = (*bucket[i])[j];
      int32_t v = find(w);

      if (sdom[v] == sdom[w]) {
        dom[w] = sdom[w];
      } else {
        dom[w] = v;
      }
    }

    if (i > 1) {
      _union(parent[i], i);
    }

    progress->increment();
  }
}

void DominatorTree::cleanup_intermediate_state() {
  delete arr;
  delete rev;
  delete label;
  delete sdom;
  delete parent;
  delete dsu;

  for (int32_t i = 0; i < this->num_nodes; ++i) {
    delete reverse_graph[i];
    delete bucket[i];
  }
  delete reverse_graph;
  delete bucket;

  delete progress;
}

void DominatorTree::calculate() {
  progress->start();

  dfs(root);

  progress->update(num_nodes);

  calculate_sdom();

  progress->update(num_nodes * 2);

  for (int32_t i = 2; i <= count; i++) {
    if (dom[i] != sdom[i]) {
      dom[i] = dom[dom[i]];
    }

    tree[rev[i]]->push_back(rev[dom[i]]);
    tree[rev[dom[i]]]->push_back(rev[i]);
    progress->increment();
  }

  cleanup_intermediate_state();

  progress->complete();
}

void DominatorTree::retained_size(RubyHeapObj *obj, size_t &size) {
  size += obj->is_root_object() ? 0 : obj->get_memsize();

  if (obj != root) {
    auto t = tree[obj->get_index()];
    if (t->size() == 1) {
      return;
    } else {
      for (size_t i = 1; i < t->size(); i++) {
        RubyHeapObj *obj = objs[(*t)[i]];
        retained_size(obj, size);
      }
    }
  }
}

}
