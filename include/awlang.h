#ifndef __AWLANG_H__
#define __AWLANG_H__

#include <stdio.h>

#define AW_TOKEN_MAX  256
#define AW_STRING_MAX 256
#define AW_IDENT_MAX  64

#define AW_ABS(x) ((x) < 0 ? -(x) : (x))

typedef enum aw_type_t aw_type_t;
typedef struct aw_token_t aw_token_t;
typedef struct aw_ctx_t aw_ctx_t;

enum aw_type_t {
  aw_eof,
  aw_ident,
  aw_int_lit,
  aw_str_lit,
  aw_begin,
  aw_end,
  aw_func,
  aw_add,
  aw_sub,
  aw_equ,
  aw_less,
  aw_gtr,
  aw_not,
  aw_decl,
  aw_set,
  aw_if,
  aw_else,
  aw_while,
  aw_asm
};

struct aw_token_t {
  aw_type_t type;

  union {
    char ident[AW_IDENT_MAX];
    char str_lit[AW_STRING_MAX];
    int int_lit;
  } data;
};

struct aw_ctx_t {
  aw_token_t *names;
  int *offsets;

  int count;

  int str_count, lbl_count;
};

aw_type_t aw_next(aw_token_t *token_ptr, const char **code_ptr, int consume);

void aw_parse(FILE *file, const char **code_ptr);
void aw_parse_expr(FILE *file, aw_ctx_t *ctx, const char **code_ptr);

#endif
