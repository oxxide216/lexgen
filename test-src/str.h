#ifndef STR_H
#define STR_H

#include <stdio.h>

#include "defs.h"

#define STR(ptr, len) ((Str) { ptr, len })
#define STR_LIT(ptr) ((Str) { ptr, sizeof(ptr) - 1 })
#define STR_FMT "%.*s"
#define STR_ARG(str) str.len, str.ptr

typedef struct {
  char *ptr;
  u32   len;
} Str;

Str str_new(char *str);
bool str_eq(Str a, Str b);
void str_fprint(FILE *stream, Str str);
void str_fprintln(FILE *stream, Str str);
void str_print(Str str);
void str_println(Str str);
i32  str_to_i32(Str str);
i64  str_to_i64(Str str);

typedef struct {
  char *buffer;
  u32   cap;
  u32   len;
} StringBuilder;

Str sb_to_str(StringBuilder sb);
void sb_push(StringBuilder *sb, char *str);
void sb_push_char(StringBuilder *sb, char ch);
void sb_push_str(StringBuilder *sb, Str str);
void sb_push_str_uppercase(StringBuilder *sb, Str str);
void sb_push_i32(StringBuilder *sb, i32 num);
void sb_push_i64(StringBuilder *sb, i64 num);
void sb_push_u32(StringBuilder *sb, u32 num);
void sb_push_u64(StringBuilder *sb, u64 num);

#endif // STR_H
