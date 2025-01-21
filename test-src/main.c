#include "../lib-src/runtime.h"
#include "grammar.h"
#include "str.h"

int main(void) {
  //Str text = STR_LIT("2 + 2 * bruh / capybara");
  Str text = STR_LIT("textty");
  Matcher matcher = {0};
  matcher_push_table(&matcher,
                     transition_table_test0,
                     ARRAY_LEN(transition_table_test0));
  matcher_push_table(&matcher,
                     transition_table_test1,
                     ARRAY_LEN(transition_table_test1));

  Str lexeme;
  u32 matched_table_id;
  while (true) {
    lexeme = matcher_match(&matcher, &text, &matched_table_id);
    if (lexeme.len == 0)
      break;

    if (text.len == 0)
      text = STR_LIT("EOF");
    printf("%u :: "STR_FMT" :: "STR_FMT"\n",
           matched_table_id, STR_ARG(lexeme),
           STR_ARG(text));
  }

  return 0;
}
