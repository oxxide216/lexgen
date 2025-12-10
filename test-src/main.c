#include <locale.h>

#include "lexgen/runtime.h"
#define LEXGEN_TRANSITION_TABLE_IMPLEMENTATION
#include "grammar.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"
#include "lexgen/io.h"

int main(void) {
  setlocale(LC_ALL, "");

  WStr text = WSTR_LIT(L"let main() =\n"
                       L"  if true: 2 + 2\n"
                       L"  else: 0\n"
                       L"  капибара\n");
  TransitionTable *table = get_transition_table();

  WStr lexeme;
  u64 token_id;
  u32 row = 1, col = 1;

  while (true) {
    lexeme = table_matches(table, &text, &token_id);

    if (token_id == (u64) -1) {
      if (text.len == 0)
        break;

      wchar_t _char[2] = { text.ptr[0], 0 };
      PERROR("%u:%u: ", "Unexpected %s\n", row, col, (char *) _char);
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

    printf("%lu :: "WSTR_FMT"\n", token_id, WSTR_ARG(lexeme));
  }

  return 0;
}
