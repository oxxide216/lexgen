#include "runtime.h"

static bool row_matches(TransitionRow *row, Str text, u32 *lexeme_len) {
  u32 state = 1;

  for (u32 i = 0; i <= (u32) text.len; ++i) {
    bool result = false;

    for (u32 j = 0; j < row->cols_count; ++j) {
      TransitionCol *col = row->cols + j;

      if (col->prev_state == state &&
          (col->min_char == -1 ||
           (col->min_char <= text.ptr[i] &&
            col->max_char >= text.ptr[i]))) {
        if (col->min_char == -1)
          --i;

        state = col->next_state;
        if (state == 0) {
          *lexeme_len = i + 1;
          return true;
        }

        result = true;
        break;
      }
    }

    if (!result)
      break;
  }

  return false;
}

void tt_push_row(TransitionTable *tt, TransitionCol *cols, u32 cols_count) {
  TransitionRow new_row = { cols, cols_count };
  DA_APPEND(*tt, new_row);
}

Str tt_match(TransitionTable *tt, Str *text, u32 *token_id) {
  Str lexeme = { text->ptr, 0 };

  *token_id = (u32) -1;

  for (u32 i = 0; i < tt->len; ++i) {
    u32 new_lexeme_len = 0;
    if (row_matches(tt->items + i, *text, &new_lexeme_len)) {
      lexeme.len = new_lexeme_len;
      if (token_id)
        *token_id = i;
      break;
    }
  }

  if (*token_id != (u32) -1) {
    text->ptr += lexeme.len;
    text->len -= lexeme.len;
  }

  return lexeme;
}
