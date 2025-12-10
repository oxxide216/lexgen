#include "lexgen/wstr.h"
#include "shl/shl-defs.h"
#include "shl/shl-str.h"

typedef struct {
  u32   prev_state;
  wchar min_char;
  wchar max_char;
  u32   next_state;
} TransitionCol;

typedef struct {
  TransitionCol *cols;
  u32            cols_count;
} TransitionRow;

typedef struct {
  TransitionRow *items;
  u32            len;
} TransitionTable;

wchar get_next_wchar(Str text, u32 index, u32 *len);
Str   table_matches(TransitionTable *table, Str *text, u64 *token_id, u32 *char_len);
