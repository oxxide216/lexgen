#include "../runtime-src/runtime.h"
#include "grammar.h"
#include "shl_str.h"
#include "log.h"

int main(void) {
  Str text = STR_LIT("let main() =\n"
                     "  if true: 2 + 2\n"
                     "  else: 0\n");
  TransitionTable table = {0};

  for (u32 i = 0; i < ARRAY_LEN(tt); ++i)
    tt_push_row(&table, tt[i].cols, tt[i].cols_count);

  Str lexeme;
  u32 token_id;
  u32 row = 0, col = 0;

  while (true) {
    lexeme = tt_matches(&table, &text, &token_id);

    if (token_id == (u32) -1) {
      if (text.len == 0)
        break;

      ERROR("Unexpected %c at %u:%u\n", text.ptr[0], row + 1, col + 1);
      exit(1);
    }

    ++col;
    if (token_id == TT_NEWLINE) {
      ++row;
      col = 0;
    }

    if (token_id == TT_SKIP)
      continue;

    printf("%u :: "STR_FMT"\n", token_id, STR_ARG(lexeme));
  }

  return 0;
}
