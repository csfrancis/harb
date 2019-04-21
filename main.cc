#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <locale.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <deque>

#include <sparsehash/sparse_hash_map>
#include <sparsehash/sparse_hash_set>

#include "graph.h"
#include "ruby_heap_obj.h"
#include "progress.h"

using namespace harb;

bool exit_ = false;
FILE *out_ = stdout;
Graph *graph_;

static void
fatal_error(const char *fmt, ...) {
  va_list args;
  fprintf(stderr, "error: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(-1);
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
static void cmd_summary(const char *);
static void cmd_diff(const char *);

command_t commands_[] = {
  { "quit", cmd_quit, "Exits the program" },
  { "print", cmd_print, "Prints heap info for the address specified" },
  { "rootpath", cmd_rootpath, "Display the root path for the object specified" },
  { "help", cmd_help, "Displays this message"},
  /* { "summary", cmd_summary, "Display a heap dump summary" }, */
  /* { "diff", cmd_diff, "Diff current heap dump with specifed dump" }, */
  { NULL, NULL, NULL }
};

/* static void */
/* cmd_summary(const char *) { */
/*   typedef google::sparse_hash_map<uint32_t, size_t> type_map_t; */
/*   type_map_t type_map; */
/*   size_t total_size = 0; */
/*   size_t num_heap_objects = heap_map_.size(); */
/*   for (auto it = heap_map_.begin(); it != heap_map_.end(); ++it) { */
/*     ruby_heap_obj *obj = it->second; */
/*     total_size += obj->as.obj.memsize; */

/*     uint32_t type = obj->flags & RUBY_T_MASK; */
/*     if (type_map[type]) { */
/*       type_map[type] += obj->as.obj.memsize; */
/*     } else { */
/*       type_map[type] = obj->as.obj.memsize; */
/*     } */
/*   } */
/*   fprintf(out_, "total objects: %'zu\n", num_heap_objects); */
/*   fprintf(out_, "total heap memsize: %'zu bytes\n", total_size); */
/*   for (auto it = type_map.begin(); it != type_map.end(); ++it) { */
/*     fprintf(out_, "  %s: %'zu bytes\n", RubyHeapObj::get_value_type_string(it->first), */
/*         it->second); */
/*   } */
/* } */

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

/* static void */
/* cmd_diff(const char *args) { */
/*   if (args == NULL || strlen(args) == 0) { */
/*     printf("error: you must specify a heap dump file\n"); */
/*     return; */
/*   } */

/*   FILE *f = fopen(args, "r"); */
/*   if (!f) { */
/*     printf("unable to open %s: %d\n", args, errno); */
/*     return; */
/*   } */

/*   char template_name[] = "harb_diff-XXXXXX"; */
/*   int fd = mkstemp(template_name); */
/*   if (fd == -1) { */
/*     printf("unable to create tempfile: %d\n", errno); */
/*     return; */
/*   } */

/*   FILE *out = fdopen(fd, "w"); */
/*   if (!out) { */
/*     printf("unable to open temp fd: %d", errno); */
/*     return; */
/*   } */

/*   ruby_heap_obj_t *obj = NULL; */
/*   json_t *json_obj = NULL; */
/*   while ((obj = read_heap_object(f, &json_obj)) != NULL) { */
/*     if (!is_root_object(obj) && heap_map_[obj->as.obj.addr] == NULL) { */
/*       char *s = json_dumps(json_obj, 0); */
/*       fprintf(out, "%s\n", s); */
/*       free(s); */
/*     } */
/*     json_decref(json_obj); */
/*   } */

/*   fclose(out); */
/*   fclose(f); */
/* } */

static RubyHeapObj *
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

  RubyHeapObj *obj = graph_->get_heap_object(addr);
  if (!obj) {
    printf("error: no ruby object found at address 0x%" PRIx64 "\n", addr);
    return NULL;
  }

  return obj;
}

static void
cmd_print(const char *args) {
  RubyHeapObj *obj = get_ruby_heap_obj_arg(args);
  if (!obj) {
    return;
  }

  obj->print_object();
}

static void
cmd_rootpath(const char *args) {
  bool found = false;
  RubyHeapObj *obj = get_ruby_heap_obj_arg(args);
  if (!obj) {
    return;
  }

  RubyHeapObj *cur;
  std::deque<RubyHeapObj *> q;
  google::sparse_hash_set<RubyHeapObj *> visited;
  google::sparse_hash_map<RubyHeapObj *, RubyHeapObj *> parent;

  q.push_back(obj);
  visited.insert(obj);

  while (!q.empty() && !found) {
    cur = q.front();
    q.pop_front();

    for (auto ref : *(cur->get_refs_from())) {
      if (visited.find(ref) == visited.end()) {
        visited.insert(ref);
        parent[ref] = cur;
        if (ref->is_root_object()) {
          cur = ref;
          found = true;
          break;
        }
        q.push_back(ref);
      }
    }
  }

  if (!found) {
    printf("error: could not find path to root for 0x%" PRIx64 "\n", obj->get_addr());
    return;
  }

  printf("\nroot path to 0x%" PRIx64 ":\n", obj->get_addr());
  while (cur != NULL) {
    cur->print_ref_object();
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

  setlocale(LC_ALL, "");

  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc < 2) {
    fatal_error("objectspace json dump file required\n");
    return -1;
  }

  const char *heap_filename = argv[1];
  FILE *heap_file = fopen(heap_filename, "r");
  if (!heap_file) {
    fatal_error("unable to open %s: %d\n", heap_filename, errno);
  }

  graph_ = new Graph(heap_file);

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
