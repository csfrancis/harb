#ifndef HARB__NODE__BLOCK_H
#define HARB__NODE__BLOCK_H

#include "base.h"

#include <list>

namespace harb {
namespace node {

class block : public executable
{
public:
  block() { }

  virtual ~block() {
    for (node_list::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
      delete *it;
    }
  }

  virtual base * execute(base *in, Harb *h) {
    base *ret = in;
    for (node_list::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
      ret = (*it)->execute(ret, h);
    }
    return ret;
  }

  void append_node(executable *node) {
    nodes.push_back(node);
  }

private:
  typedef std::list<executable *> node_list;
  node_list nodes;
};

}
}

#endif // HARB__NODE__BLOCK_H