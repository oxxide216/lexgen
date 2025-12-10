#include <wchar.h>
#include <locale.h>

#include "lexgen/runtime.h"
#define LEXGEN_TRANSITION_TABLE_IMPLEMENTATION
#include "grammar.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"
#include "lexgen/io.h"

int main(void) {
  setlocale(LC_ALL, "");

  Str text = STR_LIT("  капибара\n");
  TransitionTable *table = get_transition_table();

  Str lexeme;
  u64 token_id;
  u32 char_len;
  u32 row = 1, col = 1;

  while (true) {
    lexeme = table_matches(table, &text, &token_id, &char_len);

    if (token_id == (u64) -1) {
      if (text.len == 0)
        break;

      PERROR("%u:%u: ", "Unexpected something\n", row, col);
      exit(1);
    }

    col += lexeme.len;
    if (token_id == TT_NEWLINE) {
      ++row;
      col = 1;
      continue;
    }

    if (token_id == TT_SKIP)
      continue;

    printf("%lu :: "STR_FMT" :: %u:%u\n", token_id, STR_ARG(lexeme), lexeme.len, char_len);
  }

  return 0;
}
