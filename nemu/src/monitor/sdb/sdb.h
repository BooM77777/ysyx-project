#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

word_t expr(char *e, bool *success);

void new_wp(char* expr);
void free_wp(word_t no);

void print_all_wp();
void check_all_wp();

#endif
