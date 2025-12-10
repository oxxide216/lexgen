#include "lexgen/wstr.h"
#include "shl/shl-defs.h"
#include "shl/shl-str.h"

typedef struct {
  u32   prev_state;
  wchar_t min_char;
  wchar_t max_char;
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

WStr table_matches(TransitionTable *table, WStr *text, u64 *token_id);
