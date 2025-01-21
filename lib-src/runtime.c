#include "runtime.h"

static bool table_matches(TransitionTable *table, Str text, u32 *lexeme_len) {
  u32 state = 1;

  u32 i = 0;
  for (u32 j = 0; j < table->rows_count; ++j) {
    TransitionRow *row = table->rows + j;

    if (row->prev_state == state &&
        (row->next_char == text.ptr[i] ||
         row->next_char == (i8) -1)) {
      state = row->new_state;
      if (state == 0) {
        *lexeme_len = i + 1;
        return true;
      }

      i += row->new_state - row->prev_state;
    }
  }

  return false;
}

void matcher_push_table(Matcher *matcher, TransitionRow *rows, u32 rows_count) {
  TransitionTable new_table = { rows, rows_count };
  DA_APPEND(matcher->tables, new_table);
}

Str matcher_match(Matcher *matcher, Str *text, u32 *matched_table_id) {
  Str lexeme = { text->ptr, 0 };

  for (u32 i = 0; i < matcher->tables.len; ++i) {
    u32 new_lexeme_len = 0;
    if (table_matches(matcher->tables.items + i, *text, &new_lexeme_len) &&
        new_lexeme_len > lexeme.len) {
      lexeme.len = new_lexeme_len;
      if (matched_table_id)
        *matched_table_id = i;
    }
  }

  text->ptr += lexeme.len;
  text->len -= lexeme.len;

  return lexeme;
}
