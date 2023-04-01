#include "common.h"
#include "debug.h"
#include "sdb.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define NR_WP 32

// class WatchPoint {};
// class WatchPointManager {};

typedef struct watchpoint 
{
  int NO;
  struct watchpoint *next;

  char expr[1024];
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() 
{
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

void new_wp(char *expr)
{
  if (free_ == NULL) {
    Warn("no more watch point, we only support %d points now\n", NR_WP);
    return;
  }

  WP *ret = free_;
  free_ = free_->next;
  ret->next = NULL;

  if (head == NULL) {
    head = ret;
  } else {
    WP *iter = head;
    for (; iter->next != NULL; iter = iter->next);
    iter->next = ret;
  }

  strcpy(ret->expr, expr);
}

void free_wp(word_t no)
{
  if (no >= NR_WP) {
    Warn("no more watch point, we only support %d points now\n", NR_WP);
  }
  WP *wp = &wp_pool[no];

  if (head == NULL) { return; }

  if (head == wp) {
    head = head->next;
 } else {
    WP *iter = head;
    for (; iter && iter->next != wp; iter = iter->next);
    if (iter && iter->next == wp) {
      iter->next = wp->next;
    }
  }

  wp->next = free_;
  free_ = wp;
}

void print_all_wp()
{
  for (WP *iter = head; iter != NULL; iter = iter->next) {
    printf("watch point %d : %s\n", iter->NO, iter->expr);
  }
}

void check_all_wp()
{
  bool stop_flag = false;
  word_t value;
  bool success;
  for (WP *iter = head; iter != NULL; iter = iter->next) {
    value = expr(iter->expr, &success);
    if (!success) {
      TODO();
    }
    if (value == 1) {
      printf("watch point %d (expr : %s) meet its condition\n", iter->NO, iter->expr);
      stop_flag = true;
    }
  }
  if (stop_flag) {
    nemu_state.state = NEMU_STOP; 
  }
}
