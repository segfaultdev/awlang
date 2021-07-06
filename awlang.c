#include <awlang.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int aw_isident(const char *buffer) {
  if (!isalpha(*buffer) && *buffer != '_') return 0;
  buffer++;

  while (*buffer) {
    if (!isalnum(*buffer) && *buffer != '_') return 0;
    buffer++;
  }

  return 1;
}

static aw_type_t aw_typeof(const char *buffer) {
  if (!strcmp(buffer, "begin")) return aw_begin;
  else if (!strcmp(buffer, "end")) return aw_end;
  else if (!strcmp(buffer, "func")) return aw_func;
  else if (!strcmp(buffer, "add")) return aw_add;
  else if (!strcmp(buffer, "sub")) return aw_sub;
  else if (!strcmp(buffer, "equ")) return aw_equ;
  else if (!strcmp(buffer, "less")) return aw_less;
  else if (!strcmp(buffer, "gtr")) return aw_gtr;
  else if (!strcmp(buffer, "not")) return aw_not;
  else if (!strcmp(buffer, "decl")) return aw_decl;
  else if (!strcmp(buffer, "set")) return aw_set;
  else if (!strcmp(buffer, "if")) return aw_if;
  else if (!strcmp(buffer, "else")) return aw_else;
  else if (!strcmp(buffer, "while")) return aw_while;
  else if (!strcmp(buffer, "asm")) return aw_asm;
  else if (aw_isident(buffer)) return aw_ident;
  else if (isdigit(*buffer)) return aw_int_lit;

  return aw_str_lit;
}

static int aw_find(aw_ctx_t *ctx, aw_token_t token) {
  for (int i = 0; i < ctx->count; i++) {
    if (!strcmp(ctx->names[i].data.ident, token.data.ident))
      return ctx->offsets[i];
  }
  
  return 0;
}

aw_type_t aw_next(aw_token_t *token_ptr, const char **code_ptr, int consume) {
  const char *code = *code_ptr;

  aw_token_t token;
  token.type = aw_eof;

  if (!code) return token.type;
  while (isspace(*code)) code++;

  char buffer[AW_TOKEN_MAX] = {0};
  int length = 0;

  int in_string = 0;

  while (*code && !(isspace(*code) && !in_string)) {
    if (*code == '"') {
      in_string = !in_string;
    } else {
      buffer[length++] = *code;
    }

    code++;
  }

  if (length) token.type = aw_typeof(buffer);

  if (token.type == aw_ident) {
    memset(token.data.ident, 0, AW_IDENT_MAX);
    strncpy(token.data.ident, buffer, AW_IDENT_MAX - 1);
  } else if (token.type == aw_int_lit) {
    token.data.int_lit = (int)(strtol(buffer, NULL, 0));
  } else if (token.type == aw_str_lit) {
    memset(token.data.str_lit, 0, AW_STRING_MAX);
    strncpy(token.data.str_lit, buffer, AW_STRING_MAX - 1);
  }

  // fprintf(file, "'%s': %d\n", buffer, token.type);

  if (consume) *code_ptr = code;
  if (token_ptr) *token_ptr = token;
  return token.type;
}

void aw_parse(FILE *file, const char **code_ptr) {
  for (;;) {
    aw_type_t type = aw_next(NULL, code_ptr, 1);

    if (type == aw_func) {
      aw_token_t token;
      aw_next(&token, code_ptr, 1);

      fprintf(file, "%s:\n", token.data.ident);

      int arg_cnt = 0, var_cnt = 0;

      aw_ctx_t ctx;
      ctx.names = NULL;
      ctx.offsets = NULL;
      ctx.count = 0;
      ctx.lbl_count = 0;

      if (token.type != aw_ident) return;

      while (aw_next(NULL, code_ptr, 0) == aw_ident) {
        aw_next(&token, code_ptr, 1);

        ctx.names = realloc(ctx.names, sizeof(aw_token_t) * (ctx.count + 1));
        ctx.offsets = realloc(ctx.offsets, sizeof(int) * (ctx.count + 1));

        ctx.names[ctx.count] = token;
        ctx.offsets[ctx.count] = (2 + arg_cnt) * 8;

        ctx.count++;
        arg_cnt++;
      }

      if (aw_next(NULL, code_ptr, 0) == aw_decl) {
        aw_next(NULL, code_ptr, 1);

        while (aw_next(NULL, code_ptr, 0) == aw_ident) {
          aw_next(&token, code_ptr, 1);

          ctx.names = realloc(ctx.names, sizeof(aw_token_t) * (ctx.count + 1));
          ctx.offsets = realloc(ctx.offsets, sizeof(int) * (ctx.count + 1));

          ctx.names[ctx.count] = token;
          ctx.offsets[ctx.count] = (1 + var_cnt) * -8;
          
          ctx.count++;
          var_cnt++;
        }
      }

      if (var_cnt) {
        fprintf(file, "  push rbp\n");
        fprintf(file, "  mov rbp, rsp\n");
        fprintf(file, "  sub rsp, %d\n", var_cnt * 8);
      }

      aw_parse_expr(file, &ctx, code_ptr);

      if (var_cnt) {
        fprintf(file, "  mov rsp, rbp\n");
        fprintf(file, "  pop rbp\n");
      }

      fprintf(file, "  ret\n");
    } else return;
  }
}

void aw_parse_expr(FILE *file, aw_ctx_t *ctx, const char **code_ptr) {
  aw_token_t token;
  aw_type_t type = aw_next(&token, code_ptr, 1);

  if (type == aw_begin) {
    for (;;) {
      type = aw_next(NULL, code_ptr, 0);

      if (type == aw_end) break;
      else aw_parse_expr(file, ctx, code_ptr);
    }

    aw_next(NULL, code_ptr, 1);
  } else if (type == aw_add) {
    fprintf(file, "  push rdx\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  mov rdx, rax\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  add rdx, rax\n");
    fprintf(file, "  mov rax, rdx\n");
    fprintf(file, "  pop rdx\n");
  } else if (type == aw_sub) {
    fprintf(file, "  push rdx\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  mov rdx, rax\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  sub rdx, rax\n");
    fprintf(file, "  mov rax, rdx\n");
    fprintf(file, "  pop rdx\n");
  } else if (type == aw_equ) {
    fprintf(file, "  push rdx\n");
    fprintf(file, "  push rbx\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  mov rdx, rax\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  mov rbx, rax\n");
    fprintf(file, "  xor rax, rax\n");
    fprintf(file, "  cmp rdx, rbx\n");
    fprintf(file, "  jne .lbl_%d\n", ctx->lbl_count);
    fprintf(file, "  inc rax\n");
    fprintf(file, ".lbl_%d:\n", ctx->lbl_count++);
    fprintf(file, "  pop rbx\n");
    fprintf(file, "  pop rdx\n");
  } else if (type == aw_less) {
    fprintf(file, "  push rdx\n");
    fprintf(file, "  push rbx\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  mov rdx, rax\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  mov rbx, rax\n");
    fprintf(file, "  xor rax, rax\n");
    fprintf(file, "  cmp rdx, rbx\n");
    fprintf(file, "  jge .lbl_%d\n", ctx->lbl_count);
    fprintf(file, "  inc rax\n");
    fprintf(file, ".lbl_%d:\n", ctx->lbl_count++);
    fprintf(file, "  pop rbx\n");
    fprintf(file, "  pop rdx\n");
  } else if (type == aw_gtr) {
    fprintf(file, "  push rdx\n");
    fprintf(file, "  push rbx\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  mov rdx, rax\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  mov rbx, rax\n");
    fprintf(file, "  xor rax, rax\n");
    fprintf(file, "  cmp rdx, rbx\n");
    fprintf(file, "  jle .lbl_%d\n", ctx->lbl_count);
    fprintf(file, "  inc rax\n");
    fprintf(file, ".lbl_%d:\n", ctx->lbl_count++);
    fprintf(file, "  pop rbx\n");
    fprintf(file, "  pop rdx\n");
  } else if (type == aw_not) {
    fprintf(file, "  push rdx\n");
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  mov rdx, rax\n");
    fprintf(file, "  xor rax, rax\n");
    fprintf(file, "  test rdx, rdx\n");
    fprintf(file, "  jnz .lbl_%d\n", ctx->lbl_count);
    fprintf(file, "  inc rax\n");
    fprintf(file, ".lbl_%d:\n", ctx->lbl_count++);
    fprintf(file, "  pop rdx\n");
  } else if (type == aw_int_lit) {
    fprintf(file, "  mov rax, %d\n", token.data.int_lit);
  } else if (type == aw_str_lit) {
    fprintf(file, "  mov rax, %d\n", ctx->lbl_count);
    fprintf(file, "  jmp .lbl_%d\n", ctx->lbl_count + 1);
    fprintf(file, ".lbl_%d:\n", ctx->lbl_count++);
    fprintf(file, "  db \"%s\", 0\n", token.data.str_lit);
    fprintf(file, ".lbl_%d:\n", ctx->lbl_count++);
  } else if (type == aw_set) {
    if (aw_next(&token, code_ptr, 1) != aw_ident) return;
    int offset = aw_find(ctx, token);

    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  mov [rbp %c %d], rax\n", offset < 0 ? '-' : '+', AW_ABS(offset));
  } else if (type == aw_ident) {
    int offset = aw_find(ctx, token);

    fprintf(file, "  mov rax, [rbp %c %d]\n", offset < 0 ? '-' : '+', AW_ABS(offset));
  } else if (type == aw_if) {
    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  test rax, rax\n");
    fprintf(file, "  jz .lbl_%d\n", ctx->lbl_count);
    aw_parse_expr(file, ctx, code_ptr);

    if (aw_next(NULL, code_ptr, 0) == aw_else) {
      fprintf(file, "  jmp .lbl_%d\n", ctx->lbl_count + 1);
      fprintf(file, ".lbl_%d:\n", ctx->lbl_count++);

      aw_next(NULL, code_ptr, 1);
      aw_parse_expr(file, ctx, code_ptr);
    }

    fprintf(file, ".lbl_%d:\n", ctx->lbl_count++);
  } else if (type == aw_while) {
    fprintf(file, ".lbl_%d:\n", ctx->lbl_count++);

    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  test rax, rax\n");
    fprintf(file, "  jz .lbl_%d\n", ctx->lbl_count);

    aw_parse_expr(file, ctx, code_ptr);
    fprintf(file, "  jmp .lbl_%d\n", ctx->lbl_count - 1);
    fprintf(file, ".lbl_%d:\n", ctx->lbl_count++);
  } else if (type == aw_asm) {
    aw_next(&token, code_ptr, 1);

    fprintf(file, "  %s\n", token.data.str_lit);
  }
}
