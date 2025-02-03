#include "runtime.h"

static bool table_matches(TransitionTable *table, Str text, u32 *lexeme_len) {
  u32 state = 1;
  u32 last_used_row_index = 0;

  for (u32 i = 0; i <= (u32) text.len; ++i) {
    bool result = false;

    for (u32 j = last_used_row_index; j < table->rows_count; ++j) {
      TransitionRow *row = table->rows + j;

      if (state == row->prev_state &&
          (row->min_char == -1 ||
           (text.ptr[i] >= row->min_char &&
            text.ptr[i] <= row->max_char))) {
        if (row->min_char == -1)
          --i;
        state = row->next_state;
        if (state == 0) {
          *lexeme_len = i + 1;
          return true;
        }
        last_used_row_index = j;
        result = true;
        break;
      }
    }

    if (!result)
      break;
  }

  return false;
}

void matcher_push_table(Matcher *matcher, TransitionRow *rows, u32 rows_count) {
  TransitionTable new_table = { rows, rows_count };
  DA_APPEND(matcher->tables, new_table);
}

Str matcher_match(Matcher *matcher, Str *text, u32 *matched_table_id) {
  Str lexeme = { text->ptr, 0 };

  *matched_table_id = (u32) -1;

  for (u32 i = 0; i < matcher->tables.len; ++i) {
    u32 new_lexeme_len = 0;
    if (table_matches(matcher->tables.items + i, *text, &new_lexeme_len)) {
      lexeme.len = new_lexeme_len;
      if (matched_table_id)
        *matched_table_id = i;
      break;
    }
  }

  if (*matched_table_id != (u32) -1) {
    text->ptr += lexeme.len;
    text->len -= lexeme.len;
  }

  return lexeme;
}
