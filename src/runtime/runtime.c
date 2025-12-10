#include "lexgen/runtime.h"

wchar get_next_wchar(Str text, u32 index, u32 *len) {
  if (text.len <= index)
    return U'\0';

  u8 byte = text.ptr[index];
  wchar result = byte;

  if ((result & 0x80) == 0) {
    *len = 1;
  } else if ((result & 0xE0) == 0xC0) {
    *len = 2;
    result &= 0x1F;
  } else if ((result & 0xF0) == 0xE0) {
    *len = 3;
    result &= 0x0F;
  } else if ((result & 0xF8) == 0xF0) {
    *len = 4;
    result &= 0x07;
  } else {
    *len = 1;
    return U'\0';
  }

  if (text.len - index < *len)
    return U'\0';

  for (u32 i = 1; i < *len; ++i) {
    byte = text.ptr[index + i];
    if ((byte & 0xC0) != 0x80) {
      *len = i;
      return U'\0';
    }

    result <<= 6;
    result |= byte & 0b00111111;
  }

  if (*len == 2 && result < 0x80)
    return U'\0';
  if (*len == 3 && result < 0x800)
    return U'\0';
  if (*len == 4 && result < 0x10000)
    return U'\0';

  if (result >= 0xD800 && result <= 0xDFFF)
    return U'\0';

  if (result > 0x10FFFF)
    return U'\0';

  return result;
}

static bool row_matches(TransitionRow *row, Str text, u32 *lexeme_len, u32 *char_len) {
  u32 state = 1;
  u32 byte_index = 0;
  u32 char_index = 0;
  u32 wchar_len;
  wchar _wchar;

  while ((_wchar = get_next_wchar(text, byte_index, &wchar_len)) != U'\0') {
    bool found = false;

    for (u32 j = 0; j < row->cols_count; ++j) {
      TransitionCol *col = row->cols + j;

      if (col->prev_state != state)
        continue;

      if (col->min_char != (wchar) -1 &&
          (_wchar < col->min_char ||
           _wchar > col->max_char))
        continue;

      if (col->min_char != (wchar) -1) {
        byte_index += wchar_len;
        ++char_index;
      }

      found = true;
      state = col->next_state;
      if (state == 0) {
        *lexeme_len = byte_index;
        *char_len = char_index;
        return true;
      }

      break;
    }

    if (!found)
      break;
  }

  return false;
}

Str table_matches(TransitionTable *table, Str *text, u64 *token_id, u32 *char_len) {
  Str lexeme = { text->ptr, 0 };
  u64 longest_token_id = (u64) -1;
  *char_len = 0;

  for (u32 i = 0; i < table->len; ++i) {
    u32 new_lexeme_len = 0;
    u32 new_char_len = 0;
    bool row_match = row_matches(table->items + i, *text,
                                 &new_lexeme_len, &new_char_len);

    if (row_match && new_char_len > *char_len) {
      lexeme.len = new_lexeme_len;
      *char_len = new_char_len;
      longest_token_id = i;
    }
  }

  if (longest_token_id != (u64) -1) {
    text->ptr += lexeme.len;
    text->len -= lexeme.len;
  }

  if (token_id)
    *token_id = longest_token_id;

  return lexeme;
}
