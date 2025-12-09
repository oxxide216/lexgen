#include "../runtime-src/runtime.h"
#define LEXGEN_TRANSITION_TABLE_IMPLEMENTATION
#include "grammar.h"
#include "shl_str.h"
#include "log.h"

int main(void) {
  Str text = STR_LIT("let main() =\n"
                     "  if true: 2 + 2\n"
                     "  else: 0\n");
  TransitionTable *table = get_transition_table();

  Str lexeme;
  u64 token_id;
  u32 row = 0, col = 0;

  while (true) {
    lexeme = table_matches(table, &text, &token_id);

    if (token_id == (u64) -1) {
      if (text.len == 0)
        break;

      ERROR("Unexpected %c at %u:%u\n", text.ptr[0], row + 1, col + 1);
      exit(1);
    }

    ++col;
    if (token_id == TT_NEWLINE) {
      ++row;
      col = 0;
      continue;
    }

    if (token_id == TT_SKIP)
      continue;

    printf("%lu :: "STR_FMT"\n", token_id, STR_ARG(lexeme));
  }

  return 0;
}
