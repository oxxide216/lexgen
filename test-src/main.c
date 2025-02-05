#include "../runtime-src/runtime.h"
#include "grammar.h"
#include "str.h"

int main(void) {
  Str text = STR_LIT("228 + 1337 * bruh / capybara : xy - yx +-*/ Alde_rak");
  TransitionTable table = {0};

  for (u32 i = 0; i < ARRAY_LEN(tt); ++i)
    tt_push_row(&table, tt[i].cols, tt[i].cols_count);

  Str lexeme;
  u32 token_id;
  while (true) {
    lexeme = tt_matches(&table, &text, &token_id);
    if (token_id == (u32) -1)
      break;

    if (token_id == TT_SKIP)
      continue;

    printf("%u :: "STR_FMT, token_id, STR_ARG(lexeme));
    if (text.len > 0)
      printf(" :: "STR_FMT, STR_ARG(text));
    putc('\n', stdout);
  }

  return 0;
}
