#include "defs.h"
#include "str.h"

typedef struct {
  u32 prev_state;
  i8  min_char;
  i8  max_char;
  u32 next_state;
} TransitionRow;

typedef struct {
  TransitionRow *rows;
  u32            rows_count;
} TransitionTable;

typedef Da(TransitionTable) TransitionTables;

typedef struct {
  TransitionTables tables;
} Matcher;

void matcher_push_table(Matcher *matcher, TransitionRow *rows, u32 rows_count);
Str  matcher_match(Matcher *matcher, Str *text, u32 *lexeme_len);
