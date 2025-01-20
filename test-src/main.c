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

  u32 matched;
  while (true) {
    u32 lexeme_len = 0;
    matched = matcher_match(&matcher, text, &lexeme_len);
    if (matched == (u32) -1)
      break;

    printf(STR_FMT"\n", STR_ARG(text));
    text.ptr += lexeme_len;
    text.len -= lexeme_len;
  }

  return 0;
}
