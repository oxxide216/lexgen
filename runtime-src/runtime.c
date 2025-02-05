#include "runtime.h"

void tt_push_row(TransitionTable *tt, TransitionCol *cols, u32 cols_count) {
  TransitionRow new_row = { cols, cols_count };
  DA_APPEND(*tt, new_row);
}

static bool row_matches(TransitionRow *row, Str text, u32 *lexeme_len) {
  u32 state = 1;
  u32 col_index = 0;

  for (u32 i = 0; i <= (u32) text.len; ++i) {
    bool found = false;

    for (u32 j = col_index; j < row->cols_count; ++j) {
      TransitionCol *col = row->cols + j;

      if (col->prev_state != state)
        continue;

      if (col->min_char != -1 && (i == text.len ||
                                  col->min_char > text.ptr[i] ||
                                  col->max_char < text.ptr[i]))
        continue;

      found = true;
      state = col->next_state;
      if (state == 0) {
        *lexeme_len = i;
        if (*lexeme_len == 0)
          ++*lexeme_len;
        return true;
      }

      col_index += col->next_state - col->prev_state;
      break;
    }

    if (!found)
      break;
  }

  return false;
}

Str tt_matches(TransitionTable *tt, Str *text, u32 *token_id) {
  Str lexeme = { text->ptr, 0 };

  if (token_id)
  *token_id = (u32) -1;

  for (u32 i = 0; i < tt->len; ++i) {
    u32 new_lexeme_len = 0;
    bool row_match = row_matches(tt->items + i, *text,
                                 &new_lexeme_len);

    if (row_match) {
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
