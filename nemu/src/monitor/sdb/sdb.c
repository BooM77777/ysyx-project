#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include "common.h"
#include "sdb.h"
#include <memory/vaddr.h>

static int is_batch_mode = false;

int str_to_int(const char* input_str, bool* success) {
  int ret = 0;
  *success = true;
  for (int i = 0; input_str[i] != 0 && input_str[i] != ' '; ++i) {
    if (input_str[i] < '0' || input_str[i] > '9') {
      *success = false;
      break;
    }
    ret = ret * 10 + (input_str[i] - '0');
  }
  return ret;
}

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
  int n = 1;
  if (sscanf(args, "%d", &n) != 1) {
    TODO();
  }
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
  switch (args[0]) {
  case 'r':
    isa_reg_display();
    break; 
  case 'w':
    print_all_wp();
    break;
  default:
    TODO();
  }
  return 0;
}

static int cmd_x(char *args) {

  int n;
  char exp[1024];
  if (sscanf(args, "%d %s", &n, exp) != 2) {
    TODO();
  }

  bool success;
  vaddr_t base_addr = expr(exp, &success);
  if (!success) {
    TODO();
  }

  for (int i = 0; i < n; i += 4) {
    printf("0x%016lx:", base_addr);
    for (int j = 0; j < 4 && (i + j) < n; ++j) {
      printf("\t0x%08lx", vaddr_read(base_addr + j * 4, 4));
    }
    printf("\n");
    base_addr += 16;
  }
  return 0;
}

static int cmd_p(char *args) {
  bool success;
  word_t value = expr(args, &success);
  if (!success) {
    TODO();
  }
  printf("Decimal format = %lu\t\tHexadecimal format = 0x%016lx\n", value, value);
  return 0;
}

static int cmd_w(char *args) {
  new_wp(args);
  return 0;
}

static int cmd_d(char *args) {
  word_t watchpoint_id;
  sscanf(args, "%lu", &watchpoint_id);
  free_wp(watchpoint_id);
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  {    "c",             "Continue the execution of the program", cmd_c    },
  {    "q",                                         "Exit NEMU", cmd_q    },
  {   "si",                                                  "", cmd_si   },
  { "info",                                                  "", cmd_info },
  {    "x",                                                  "", cmd_x    },
  {    "p",                                                  "", cmd_p    },
  {    "w",                                                  "", cmd_w    },
  {    "d",                                                  "", cmd_d    },
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { 
      continue; 
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { 
          return; 
        }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
