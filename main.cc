#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "harb.h"
#include "nodes.h"


///////////////////////////////////////////////////////////////////////////////
// Commands
///////////////////////////////////////////////////////////////////////////////

#if 0
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

#endif

///////////////////////////////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////////////////////////////



int
main(int argc, char **argv) {
  char *line;

  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc < 2) {
    fprintf(stderr, "objectspace json dump file required\n");
    return -1;
  }

  Harb h;
  h.load_file(argv[1]);

#if 0
  harb::node::record_set records(&heap_map_);
  harb::node::block root;
  ha_parse("0x7f248b357f80", &root);

  harb::node::base *out = root.execute(&records);
  harb::node::record_set *rs = dynamic_cast<harb::node::record_set *>(out);
  if (rs) {
    printf("got record set\n");
    ruby_heap_map_t *out_map = *(rs->get_value());
    ruby_heap_map_t::const_iterator it = out_map->begin();
    for (; it != out_map->end(); ++it) {
      printf("got one\n");
    }
  }

  exit(0);
#endif

  while (true) {
    line = readline("harb> ");

    if (line == NULL) {
      break;
    } else {
      add_history(line);
    }

    harb::node::block root;
    Harb::parse_command(line, &root);

    free(line);
  }

  return 0;
}
