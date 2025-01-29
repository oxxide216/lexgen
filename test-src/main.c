#include "../runtime-src/runtime.h"
#include "grammar.h"
#include "str.h"

int main(void) {
  Str text = STR_LIT("228 + 1337 * bruh / capybara : xy - yx +-*/ Alderak");
  Matcher matcher = {0};
  matcher_push_table(&matcher, tt_skip, ARRAY_LEN(tt_skip));
  matcher_push_table(&matcher, tt_ident, ARRAY_LEN(tt_ident));
  matcher_push_table(&matcher, tt_number, ARRAY_LEN(tt_number));
  matcher_push_table(&matcher, tt_op_add, ARRAY_LEN(tt_op_add));
  matcher_push_table(&matcher, tt_op_sub, ARRAY_LEN(tt_op_sub));
  matcher_push_table(&matcher, tt_op_mul, ARRAY_LEN(tt_op_mul));
  matcher_push_table(&matcher, tt_op_div, ARRAY_LEN(tt_op_div));

  Str lexeme;
  u32 matched_table_id;
  while (true) {
    lexeme = matcher_match(&matcher, &text, &matched_table_id);
    if (matched_table_id == (u32) -1)
      break;
    if (matched_table_id == 0)
      continue;

    if (text.len == TT_SKIP)
      printf("%u :: "STR_FMT" :: EOF\n",
             matched_table_id, STR_ARG(lexeme));
    else
      printf("%u :: "STR_FMT" :: "STR_FMT"\n",
             matched_table_id, STR_ARG(lexeme),
             STR_ARG(text));
  }

  return 0;
}
