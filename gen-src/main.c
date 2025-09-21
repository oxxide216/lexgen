#include <ctype.h>

#include "shl_defs.h"
#define SHL_ARENA_IMPLEMENTATION
#include "shl_arena.h"
#define SHL_STR_IMPLEMENTATION
#include "shl_str.h"
#include "shl_log.h"
#include "io.h"

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
  i8   _char;
  bool is_escaped;
} Char;

typedef struct {
  i8 min, max;
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
  Str   name;
  Atom* atoms;
} Def;

typedef Da(Def) Defs;

i8 escape_chars[] = { 'n', 'r', 't', '\\' };

void atoms_push_char(Atom **begin, Atom **end,
                     i8 _char, bool is_escaped) {
  Atom new_atom = {
    AtomKindChar,
    { { _char, is_escaped } },
    NULL,
  };

  if (*end && (*end)->kind == AtomKindRange && (*end)->as.range.max == 0) {
    (*end)->as.range.max = _char;
  } else {
    LL_PREPEND((*begin), (*end), Atom);
    **end = new_atom;
  }
}

Atom *parse(Str source_text, u32 *i, bool is_in_block) {
  Atom *result = NULL;
  Atom *result_end = NULL;
  bool is_escaped = false;

  while (*i < source_text.len &&
         (source_text.ptr[*i] != '\n'&&
          (source_text.ptr[*i] != ')' ||
           is_in_block || is_escaped))) {
    i8 _char = source_text.ptr[*i];

    if (is_escaped) {
      bool is_char_escaped = false;

      for (u32 j = 0; j < ARRAY_LEN(escape_chars); ++j) {
        if (_char == escape_chars[j]) {
          is_char_escaped = true;
          break;
        }
      }

      atoms_push_char(&result, &result_end,
                      _char, is_char_escaped);
      is_escaped = false;
      ++*i;
      continue;
    } else if (_char == '\\') {
      is_escaped = true;
      ++*i;
      continue;
    }

    switch (_char) {
    case '+':
    case '*': {
      if (!result_end) {
        ERROR("No operand was found\n");
        exit(1);
      }
      if (result_end->kind == AtomKindLoop) {
        ERROR("Loop recursion detected\n");
        exit(1);
      }

      Atom *new_atom = aalloc(sizeof(Atom));
      *new_atom = *result_end;
      if (_char == '+')
        LL_PREPEND(result, result_end, Atom);
      *result_end = (Atom) { AtomKindLoop, { .loop = new_atom }, NULL };
    } break;

    case '-': {
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
        { { result_end->as._char._char, 0 } },
        NULL,
      };
    } break;

    case '|': {
      if (!result || *i + 1 >= source_text.len) {
        ERROR("No operand was found\n");
        exit(1);
      }

      ++*i;
      Atom *rhs = parse(source_text, i, is_in_block);
      --*i;
      Atom *new_atom = aalloc(sizeof(Atom));
      *new_atom = (Atom) { AtomKindOr, { .or = { result, rhs } }, NULL };
      result = new_atom;
      result_end = new_atom;
    } break;

    case '(': {
      ++*i;
      Atom *body = parse(source_text, i, true);
      LL_PREPEND(result, result_end, Atom);
      *result_end = (Atom) { AtomKindBlock, { .block = body }, NULL };
    } break;

    case ')': {
      if (is_in_block)
        return result;

      ERROR("Non expected `)`\n");
      exit(1);
    } break;

    case '?': {
      if (!result_end || *i + 1 >= source_text.len) {
        ERROR("No operand was found\n");
        exit(1);
      }

      Atom *new_atom = aalloc(sizeof(Atom));
      *new_atom = *result_end;
      *result_end = (Atom) {
        AtomKindOptional,
        { .optional = new_atom },
        NULL,
      };


    } break;

    default: {
      atoms_push_char(&result, &result_end,
                      _char, false);
    } break;
    }

    is_escaped = false;
    ++*i;
  }

  return result;
}

Defs create_defs(Str source_text) {
  Defs defs = {0};

  u32 i = 0;
  while (i < source_text.len) {
    if (source_text.ptr[i] == '\n') {
      ++i;
      continue;
    } else if (source_text.ptr[i] == '#') {
      while (i < source_text.len && source_text.ptr[i] != '\n')
        ++i;
      ++i;
    }

    Str name = STR(source_text.ptr + i, 0);
    while (i < source_text.len &&
           (isalnum(source_text.ptr[i]) ||
            source_text.ptr[i] == '_')) {
      ++name.len;
      ++i;
    }

    if (name.len == 0) {
      ERROR("Expected identifier, but got %c\n", source_text.ptr[i]);
      exit(1);
    }

    if (i == source_text.len) {
      ERROR("Expected `=`, but got EOF\n");
      exit(1);
    }
    if (source_text.ptr[i] != '=') {
      ERROR("Expected `=`, but got `%c`\n", source_text.ptr[i]);
      exit(1);
    }

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


u32 sb_push_atoms(StringBuilder *sb, Atom *atoms, u32 current_state,
                  u32 expected_target_state, bool is_on_top_level,
                  bool is_in_loop) {
  if (current_state == 0) {
    ERROR("Unreachable\n");
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
      sb_push(sb, "  { ");
      sb_push_u32(sb, current_state);
      sb_push(sb, ", '");
      if (atom->as._char.is_escaped ||
          atom->as._char._char == '\'' ||
          atom->as._char._char == '\\')
        sb_push_char(sb, '\\');
      sb_push_char(sb, atom->as._char._char);
      sb_push(sb, "', '");
      if (atom->as._char.is_escaped ||
          atom->as._char._char == '\'' ||
          atom->as._char._char == '\\')
        sb_push_char(sb, '\\');
      sb_push_char(sb, atom->as._char._char);
      sb_push(sb, "', ");
      sb_push_u32(sb, target_state);
      sb_push(sb, " },\n");
    } break;

    case AtomKindRange: {
      sb_push(sb, "  { ");
      sb_push_u32(sb, current_state);
      sb_push(sb, ", '");
      if (atom->as.range.min == '\'')
        sb_push_char(sb, '\\');
      sb_push_char(sb, atom->as.range.min);
      sb_push(sb, "', '");
      if (atom->as.range.max == '\'')
        sb_push_char(sb, '\\');
      sb_push_char(sb, atom->as.range.max);
      sb_push(sb, "', ");
      sb_push_u32(sb, target_state);
      sb_push(sb, " },\n");
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

      sb_push_atoms(sb, atom->as.loop, current_state,
                    current_state, false, true);

      sb_push(sb, "  { ");
      sb_push_u32(sb, current_state);
      sb_push(sb, ", -1, -1, ");
      sb_push_u32(sb, target_state);
      sb_push(sb, " },\n");
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

      sb_push_atoms(sb, atom->as.or.lhs, current_state,
                    target_state, is_on_top_level, false);
      sb_push_atoms(sb, atom->as.or.rhs, current_state,
                    target_state, is_on_top_level, false);
    } break;

    case AtomKindBlock: {
      target_state = sb_push_atoms(sb, atom->as.block, current_state,
                                   target_state, false, false);
    } break;

    case AtomKindOptional: {
      sb_push_atoms(sb, atom->as.optional, current_state,
                    target_state, false, false);

      sb_push(sb, "  { ");
      sb_push_u32(sb, current_state);
      sb_push(sb, ", -1, -1, ");
      sb_push_u32(sb, target_state);
      sb_push(sb, " },\n");

    } break;
    }

    atom = atom->next;
    current_state = target_state;
  }

  return target_state;
}

void sb_push_str_uppercase(StringBuilder *sb, Str str) {
  for (u32 i = 0; i < str.len; ++i) {
    char ch = str.ptr[i];
    if (str.ptr[i] >= 97 && str.ptr[i] <=122)
      ch -= 32;
    sb_push_char(sb, ch);
  }
}

Str defs_gen_table(Defs *defs) {
  StringBuilder sb = {0};

  sb_push(&sb, "#ifndef LEXGEN_TRANSITION_TABLE\n");
  sb_push(&sb, "#define LEXGEN_TRANSITION_TABLE\n\n");

  for (u32 i = 0; i < defs->len; ++i) {
    sb_push(&sb, "#define TT_");
    sb_push_str_uppercase(&sb, defs->items[i].name);
    sb_push_char(&sb, ' ');
    sb_push_u32(&sb, i);
    sb_push(&sb, "\n");
  }

  sb_push(&sb, "\n#define TTS_COUNT ");
  sb_push_u32(&sb, defs->len);
  sb_push(&sb, "\n\n");

  sb_push(&sb, "TransitionTable *get_transition_table(void);\n\n");

  sb_push(&sb, "#ifdef LEXGEN_TRANSITION_TABLE_IMPLEMENTATION\n\n");

  for (u32 i = 0; i < defs->len; ++i) {
    sb_push(&sb, "TransitionCol table_col_");
    sb_push_str(&sb, defs->items[i].name);
    sb_push(&sb, "[] = {\n");
    sb_push_atoms(&sb, defs->items[i].atoms, 1, 0, true, false);
    sb_push(&sb, "};\n\n");
  }

  sb_push(&sb, "TransitionRow table_rows[] = {\n");
  for (u32 i = 0; i < defs->len; ++i) {
    sb_push(&sb, "  { table_col_");
    sb_push_str(&sb, defs->items[i].name);
    sb_push(&sb, ", sizeof(");
    sb_push(&sb, "table_col_");
    sb_push_str(&sb, defs->items[i].name);
    sb_push(&sb, ") / sizeof(TransitionCol) },\n");
  }
  sb_push(&sb, "};\n\n");

  sb_push(&sb, "TransitionTable table = {\n");
  sb_push(&sb, "  table_rows,\n");
  sb_push(&sb, "  sizeof(table_rows) / sizeof(TransitionRow),\n");
  sb_push(&sb, "};\n\n");

  sb_push(&sb, "TransitionTable *get_transition_table(void) {\n");
  sb_push(&sb, "  return &table;\n");
  sb_push(&sb, "};\n\n");

  sb_push(&sb, "#endif // LEXGEN_TRANSITION_TABLE_IMPLEMENTATION\n\n");
  sb_push(&sb, "#endif // LEXGEN_TRANSITION_TABLE\n");

  return sb_to_str(sb);
}

int main(i32 argv, char **argc) {
  if (argv < 2) {
    ERROR("Output file was not provided\n");
    exit(1);
  }

  if (argv < 3) {
    ERROR("Input file was not provided\n");
    return 1;
  }

  Str source_text = read_file(argc[2]);
  Defs defs = create_defs(source_text);
  write_file(argc[1], defs_gen_table(&defs));

  return 0;
}
