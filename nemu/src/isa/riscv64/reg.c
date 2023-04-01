#include <isa.h>
#include <stdio.h>
#include <string.h>
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

word_t isa_reg_str2val(const char *s, bool *success) {
  *success = false;
  for (int i = 0; i < 32; ++i) {
    if (strcmp(s, regs[i]) != 0) {
      *success = true;
      return gpr(i);
    }
  }
  return 0;
}

void isa_reg_display() {
  for (int i = 0; i < 32; ++i) {
    printf("|  %3s  0x%016lx  |", regs[i], gpr(i));
    if (i%4 == 3) {
      printf("\n");
    }
  }
}
