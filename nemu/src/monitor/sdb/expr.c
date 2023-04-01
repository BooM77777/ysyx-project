#include "common.h"
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdio.h>
#include <memory/vaddr.h>

enum 
{
  TK_NOTYPE,
  TK_EQ,
  TK_NEQ,
  TK_NUM,
  TK_HEX,
  TK_REG,
  TK_PLUS,
  TK_MINUS,
  TK_MULTIPLY,
  TK_DIV,
  TK_LB,
  TK_RB,
  TK_GETVAL,
  TK_OR,
  TK_AND,
};

static struct rule 
{
  const char *regex;
  int token_type;
} 
rules[] = 
{
  {"0[xX][0-9a-f]+", TK_HEX},
  {"[0-9]+", TK_NUM},
  {"\\$[0-9A-Za-z]+", TK_REG},
  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"-", TK_MINUS},
  {"\\*", TK_MULTIPLY},
  {"/", TK_DIV},
  {"\\(", TK_LB},
  {"\\)", TK_RB},
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},
  {"&&", TK_AND},
  {"||", TK_OR},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token 
{
  int type;
  int val;
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) 
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  char reg[2];
  bool success;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        switch (rules[i].token_type) {

        case TK_NUM:
          sscanf(substr_start, "%d", &tokens[nr_token].val);
          tokens[nr_token++].type = rules[i].token_type;
          break;
        case TK_HEX:
          sscanf(substr_start, "%x", &tokens[nr_token].val);
          tokens[nr_token++].type = rules[i].token_type;
          break;
        case TK_REG:
          memcpy(reg, substr_start, substr_len);
          tokens[nr_token].val = isa_reg_str2val(reg, &success); 
          tokens[nr_token++].type = TK_REG;
          if (!success) {
            TODO();
          }
          break;
        case TK_MINUS:
          if (nr_token == 0 
            || (tokens[nr_token-1].type != TK_NUM
            && tokens[nr_token-1].type != TK_HEX
            && tokens[nr_token-1].type != TK_RB)) {

            tokens[nr_token].val = -1;
            tokens[nr_token++].type = TK_NUM;
            tokens[nr_token++].type = TK_MULTIPLY;
          } else {
            tokens[nr_token++].type = rules[i].token_type;
          }
          break;
        case TK_MULTIPLY:
          if (nr_token == 0
            || (tokens[nr_token-1].type != TK_HEX
            && tokens[nr_token-1].type != TK_NUM
            && tokens[nr_token-1].type != TK_RB)) {
              printf("???\n");
            tokens[nr_token++].type = TK_GETVAL;
          } else {
            printf("%d\n", tokens[nr_token-1].type);
            tokens[nr_token++].type = rules[i].token_type;
          }
          break;
        case TK_PLUS:
        case TK_DIV:
        case TK_LB:
        case TK_RB:
        case TK_AND:
        case TK_OR:
        case TK_EQ:
        case TK_NEQ:
          tokens[nr_token++].type = rules[i].token_type;
          break;
        case TK_NOTYPE:
          break;
        default: 
          TODO();
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

inline int updateDepth(int i) 
{
  switch (tokens[i].type) {
    case TK_RB:
      return 1;
    case TK_LB:
      return -1;
  }
  return 0;
}

word_t get_tokens_res(int l, int r) 
{

  assert(l <= r);

  if (l == r) {
    if (tokens[l].type == TK_NUM || tokens[l].type == TK_HEX || tokens[l].type == TK_REG) {
      return tokens[l].val;
    }
    TODO();
  }

  if (tokens[l].type == TK_LB && tokens[r].type == TK_RB) {
    return get_tokens_res(l+1, r-1);
  }

  int ret = 0, depth = 0, rValue;

  for (int i = r; i >= l; --i) {

    depth += updateDepth(i);
    assert(depth >= 0);

    if (depth == 0) {
      switch (tokens[i].type) {
      case TK_PLUS:
        return get_tokens_res(l, i-1) + get_tokens_res(i+1, r);
        break;
      case TK_MINUS:
        return get_tokens_res(l, i-1) - get_tokens_res(i+1, r);
        break;
      }
    }
  }

  assert(depth == 0);

  for (int i = r; i >= l; --i) {

    depth += updateDepth(i);
    assert(depth >= 0);

    if (depth == 0) {
      switch (tokens[i].type) {
      case TK_MULTIPLY:
        return get_tokens_res(l, i-1) * get_tokens_res(i+1, r);
      case TK_DIV:
        rValue = get_tokens_res(i+1, r);
        assert(rValue != 0);
        return get_tokens_res(l, i-1) / rValue;
      }
    }
  }

  for (int i = r; i >= l; --i) {

    depth += updateDepth(i);
    assert(depth >= 0);

    if (depth == 0) {
      switch (tokens[i].type) {
      case TK_AND:
        return get_tokens_res(l, i-1) && get_tokens_res(i+1, r);
      case TK_OR:
        return get_tokens_res(l, i-1) || get_tokens_res(i+1, r);
      case TK_EQ:
        return get_tokens_res(l, i-1) == get_tokens_res(i+1, r);
      case TK_NEQ:
        return get_tokens_res(l, i-1) != get_tokens_res(i+1, r);
      }
    }
  }

  for (int i = r; i >= l; --i) {
    switch (tokens[i].type) {
    case TK_GETVAL:
      return vaddr_read(get_tokens_res(i+1, r), sizeof(word_t));
    }
  }

  TODO();  

  return ret;
}

word_t expr(char *e, bool *success) {

  if (!make_token(e)) {
    *success = false;
    return 0;
  }

//  for (int i = 0; i < nr_token; ++i) {
//    printf("token type = %d, value = %d\n", tokens[i].type, tokens[i].val);
//  }

  *success = true;
  return get_tokens_res(0, nr_token - 1);
}
