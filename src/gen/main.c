#include <wctype.h>
#include <locale.h>

#define SHL_DEFS_LL_ALLOC(size) arena_alloc(&arena, size)

#include "lexgen/io.h"
#include "lexgen/arena.h"
#include "shl/shl-defs.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"
#include "shl/shl-log.h"

typedef enum {
  AtomKindChar = 0,
  AtomKindRange,
  AtomKindLoop,
  AtomKindOr,
  AtomKindBlock,
  AtomKindOptional,
} AtomKind;

typedef struct Atom Atom;

typedef struct {
  wchar _char;
} Char;

typedef struct {
  wchar min, max;
} Range;

typedef struct {
  Atom *lhs;
  Atom *rhs;
} Or;

typedef union {
  Char   _char;
  Range  range;
  Atom  *loop;
  Or     or;
  Atom  *block;
  Atom  *optional;
} AtomAs;

struct Atom {
  AtomKind  kind;
  AtomAs    as;
  Atom     *next;
};

typedef struct {
  WStr  name;
  Atom* atoms;
} Def;

typedef Da(Def) Defs;

typedef struct {
  wchar src;
  wchar dest;
} EscapeChar;

static EscapeChar escape_chars[] = {
  { U'n', U'\n' },
  { U'r', U'\r' },
  { U't', U'\t' },
};

static Arena arena = {0};

void atoms_push_char(Atom **begin, Atom **end, wchar _char) {
  for (u32 i = 0; i < ARRAY_LEN(escape_chars); ++i) {
    if (escape_chars[i].src == _char) {
      _char = escape_chars[i].dest;
      break;
    }
  }

  Atom new_atom = {
    AtomKindChar,
    { { _char } },
    NULL,
  };

  if (*end && (*end)->kind == AtomKindRange && (*end)->as.range.max == 0) {
    (*end)->as.range.max = _char;
  } else {
    LL_PREPEND(*begin, *end, Atom);
    **end = new_atom;
  }
}

Atom *parse(WStr source_text, u32 *i, bool is_in_block) {
  Atom *result = NULL;
  Atom *result_end = NULL;
  bool is_escaped = false;

  while (*i < source_text.len &&
         (source_text.ptr[*i] != U'\n'&&
          (source_text.ptr[*i] != U')' ||
           is_in_block || is_escaped))) {
    wchar _char = source_text.ptr[*i];

    if (is_escaped) {
      atoms_push_char(&result, &result_end, _char);
      is_escaped = false;
      ++*i;
      continue;
    } else if (_char == U'\\') {
      is_escaped = true;
      ++*i;
      continue;
    }

    switch (_char) {
    case U'+':
    case U'*': {
      if (!result_end) {
        ERROR("No operand was found\n");
        exit(1);
      }
      if (result_end->kind == AtomKindLoop) {
        ERROR("Loop recursion detected\n");
        exit(1);
      }

      Atom *new_atom = arena_alloc(&arena, sizeof(Atom));
      *new_atom = *result_end;
      if (_char == U'+')
        LL_PREPEND(result, result_end, Atom);
      *result_end = (Atom) { AtomKindLoop, { .loop = new_atom }, NULL };
    } break;

    case U'-': {
      if (!result_end || *i + 1 >= source_text.len) {
        ERROR("No operand was found\n");
        exit(1);
      }
      if (result_end->kind != AtomKindChar) {
        ERROR("Operands of range should be characters\n");
        exit(1);
      }

      *result_end = (Atom) {
        AtomKindRange,
        { { result_end->as._char._char } },
        NULL,
      };
    } break;

    case U'|': {
      if (!result || *i + 1 >= source_text.len) {
        ERROR("No operand was found\n");
        exit(1);
      }

      ++*i;
      Atom *rhs = parse(source_text, i, is_in_block);
      --*i;
      Atom *new_atom = arena_alloc(&arena, sizeof(Atom));
      *new_atom = (Atom) { AtomKindOr, { .or = { result, rhs } }, NULL };
      result = new_atom;
      result_end = new_atom;
    } break;

    case U'(': {
      ++*i;
      Atom *body = parse(source_text, i, true);
      LL_PREPEND(result, result_end, Atom);
      *result_end = (Atom) { AtomKindBlock, { .block = body }, NULL };
    } break;

    case U')': {
      if (is_in_block)
        return result;

      ERROR("Non expected `)`\n");
      exit(1);
    } break;

    case U'?': {
      if (!result_end || *i + 1 >= source_text.len) {
        ERROR("No operand was found\n");
        exit(1);
      }

      Atom *new_atom = arena_alloc(&arena, sizeof(Atom));
      *new_atom = *result_end;
      *result_end = (Atom) {
        AtomKindOptional,
        { .optional = new_atom },
        NULL,
      };


    } break;

    default: {
      atoms_push_char(&result, &result_end, _char);
    } break;
    }

    is_escaped = false;
    ++*i;
  }

  return result;
}

Defs create_defs(WStr source_text) {
  Defs defs = {0};

  u32 row = 1;
  u32 col = 1;
  u32 i = 0;
  while (i < source_text.len) {
    if (source_text.ptr[i] == U'\n') {
      ++row;
      col = 1;
      ++i;
      continue;
    } else if (source_text.ptr[i] == U'#') {
      while (i < source_text.len && source_text.ptr[i] != U'\n')
        ++i;
      ++row;
      col = 1;
      ++i;
    }

    WStr name = WSTR(source_text.ptr + i, 0);
    while (i < source_text.len &&
           (iswalnum(source_text.ptr[i]) ||
            source_text.ptr[i] == U'_')) {
      ++name.len;
      ++i;
    }

    if (name.len == 0) {
      wchar _char[2] = { source_text.ptr[i], 0 };
      PERROR("%u:%u: ", "Expected identifier, but got `%s`\n", row, col, (char *) _char);
      exit(1);
    }

    if (i == source_text.len) {
      PERROR("%u:%u: ", "Expected `=`, but got EOF\n", row, col);
      exit(1);
    }

    if (source_text.ptr[i] != U'=') {
      wchar _char[2] = { source_text.ptr[i], 0 };
      PERROR("%u:%u: ", "expected `=`, but got `%s`\n", row, col, (char *) _char);
      exit(1);
    }

    ++col;
    ++i;

    Atom *atoms = parse(source_text, &i, false);
    Def new_def = { name, atoms };
    DA_APPEND(defs, new_def);
  }

  return defs;
}

void atom_max_state(Atom *atom, u32 *max_state) {
  switch (atom->kind) {
  case AtomKindChar:
  case AtomKindRange:
  case AtomKindLoop:
  case AtomKindOptional: {
    ++*max_state;
  } break;

  case AtomKindOr: {
    u32 lhs_max_state = 0, rhs_max_state = 0;
    atom_max_state(atom->as.or.lhs, &lhs_max_state);
    atom_max_state(atom->as.or.rhs, &rhs_max_state);

    if (rhs_max_state > lhs_max_state)
      *max_state += rhs_max_state - 1;
    else
      *max_state += lhs_max_state - 1;
  } break;

  case AtomKindBlock: {
    atom_max_state(atom->as.block, max_state);
  } break;

  }

  if (atom->next)
    atom_max_state(atom->next, max_state);
}


u32 wsb_push_atoms(WStringBuilder *wsb, Atom *atoms, u32 current_state,
                  u32 expected_target_state, bool is_on_top_level,
                  bool is_in_loop) {
  if (current_state == 0) {
    ERROR("Lnreachable\n");
    exit(1);
  }

  u32 target_state = current_state;
  if (is_in_loop)
    target_state = expected_target_state;

  Atom *atom = atoms;
  while (atom) {
    if (!is_in_loop) {
      ++target_state;
      if (!atom->next)
        target_state = expected_target_state;
    }

    switch (atom->kind) {
    case AtomKindChar: {
      wsb_push(wsb, U"  { ");
      wsb_push_u32(wsb, current_state);
      wsb_push(wsb, U", ");
      wsb_push_u32(wsb, atom->as._char._char);
      wsb_push(wsb, U", ");
      wsb_push_u32(wsb, atom->as._char._char);
      wsb_push(wsb, U", ");
      wsb_push_u32(wsb, target_state);
      wsb_push(wsb, U" },\n");
    } break;

    case AtomKindRange: {
      wsb_push(wsb, U"  { ");
      wsb_push_u32(wsb, current_state);
      wsb_push(wsb, U", ");
      wsb_push_u32(wsb, atom->as.range.min);
      wsb_push(wsb, U", ");
      wsb_push_u32(wsb, atom->as.range.max);
      wsb_push(wsb, U", ");
      wsb_push_u32(wsb, target_state);
      wsb_push(wsb, U" },\n");
    } break;

    case AtomKindLoop: {
      if (is_in_loop) {
        ERROR("Recursive loop detected\n");
        exit(1);
      }

      u32 state_offset = 0;
      atom_max_state(atom->as.loop, &state_offset);
      if (expected_target_state - target_state >= state_offset)
        target_state += state_offset;

      wsb_push_atoms(wsb, atom->as.loop, current_state,
                    current_state, false, true);

      wsb_push(wsb, U"  { ");
      wsb_push_u32(wsb, current_state);
      wsb_push(wsb, U", -1, -1, ");
      wsb_push_u32(wsb, target_state);
      wsb_push(wsb, U" },\n");
    } break;

    case AtomKindOr: {
      u32 lhs_state_offset = 0, rhs_state_offset = 0;
      atom_max_state(atom->as.or.lhs, &lhs_state_offset);
      atom_max_state(atom->as.or.rhs, &rhs_state_offset);

      u32 state_offset = lhs_state_offset;
      if (rhs_state_offset > lhs_state_offset)
        state_offset = rhs_state_offset;

      if (!is_in_loop && expected_target_state - target_state >= state_offset)
        target_state += state_offset;

      wsb_push_atoms(wsb, atom->as.or.lhs, current_state,
                    target_state, is_on_top_level, false);
      wsb_push_atoms(wsb, atom->as.or.rhs, current_state,
                    target_state, is_on_top_level, false);
    } break;

    case AtomKindBlock: {
      target_state =wsb_push_atoms(wsb, atom->as.block, current_state,
                                   target_state, false, false);
    } break;

    case AtomKindOptional: {
      wsb_push_atoms(wsb, atom->as.optional, current_state,
                    target_state, false, false);

      wsb_push(wsb, U"  { ");
      wsb_push_u32(wsb, current_state);
      wsb_push(wsb, U", -1, -1, ");
      wsb_push_u32(wsb, target_state);
      wsb_push(wsb, U" },\n");
    } break;
    }

    atom = atom->next;
    current_state = target_state;
  }

  return target_state;
}

WStr defs_gen_table(Defs *defs) {
  WStringBuilder  wsb = {0};

   wsb_push(&wsb, U"#ifndef LEXGEN_TRANSITION_TABLE\n");
   wsb_push(&wsb, U"#define LEXGEN_TRANSITION_TABLE\n\n");

  for (u32 i = 0; i < defs->len; ++i) {
     wsb_push(&wsb, U"#define TT_");
     wsb_push_wstr_uppercase(&wsb, defs->items[i].name);
     wsb_push_wchar(&wsb, U' ');
     wsb_push_u32(&wsb, i);
     wsb_push(&wsb, (wchar *) U"\n");
  }

   wsb_push(&wsb, U"\n#define TTS_COLNT U");
   wsb_push_u32(&wsb, defs->len);
   wsb_push(&wsb, U"\n\n");

   wsb_push(&wsb, U"TransitionTable *get_transition_table(void);\n\n");

   wsb_push(&wsb, U"#ifdef LEXGEN_TRANSITION_TABLE_IMPLEMENTATION\n\n");

  for (u32 i = 0; i < defs->len; ++i) {
     wsb_push(&wsb, U"TransitionCol table_col_");
     wsb_push_wstr(&wsb, defs->items[i].name);
     wsb_push(&wsb, U"[] = {\n");
     wsb_push_atoms(&wsb, defs->items[i].atoms, 1, 0, true, false);
     wsb_push(&wsb, U"};\n\n");
  }

  wsb_push(&wsb, U"TransitionRow table_rows[] = {\n");
  for (u32 i = 0; i < defs->len; ++i) {
    wsb_push(&wsb, U"  { table_col_");
    wsb_push_wstr(&wsb, defs->items[i].name);
    wsb_push(&wsb, U", sizeof(");
    wsb_push(&wsb, U"table_col_");
    wsb_push_wstr(&wsb, defs->items[i].name);
    wsb_push(&wsb, U") / sizeof(TransitionCol) },\n");
  }
  wsb_push(&wsb, U"};\n\n");

  wsb_push(&wsb, U"TransitionTable table = {\n");
  wsb_push(&wsb, U"  table_rows,\n");
  wsb_push(&wsb, U"  sizeof(table_rows) / sizeof(TransitionRow),\n");
  wsb_push(&wsb, U"};\n\n");

  wsb_push(&wsb, U"TransitionTable *get_transition_table(void) {\n");
  wsb_push(&wsb, U"  return &table;\n");
  wsb_push(&wsb, U"};\n\n");

  wsb_push(&wsb, U"#endif // LEXGEN_TRANSITION_TABLE_IMPLEMENTATION\n\n");
  wsb_push(&wsb, U"#endif // LEXGEN_TRANSITION_TABLE\n");

  return wsb_to_wstr(wsb);
}

int main(i32 argc, char **argv) {
  if (argc < 2) {
    ERROR("Output file was not provided\n");
    return 1;
  }

  if (argc < 3) {
    ERROR("Input file was not provided\n");
    return 1;
  }

  setlocale(LC_ALL, "");

  WStr source_text = read_file(argv[2]);
  Defs defs = create_defs(source_text);
  WStr dest_text = defs_gen_table(&defs);
  write_file(argv[1], dest_text);

  free(source_text.ptr);
  arena_free(&arena);
  if (defs.items)
    free(defs.items);
  free(dest_text.ptr);

  return 0;
}
