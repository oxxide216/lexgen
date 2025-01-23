#include <ctype.h>

#include "io.h"
#include "defs.h"
#include "log.h"

typedef enum {
  AtomKindChar = 0,
  AtomKindRange,
  AtomKindLoop,
  AtomKindWhitespace,
  AtomKindOr,
  AtomKindBlock,
} AtomKind;

typedef struct Atom Atom;

typedef struct {
  i8 min, max;
} Range;

typedef struct {
  Atom *body;
  bool  is_from_star;
} Loop;

typedef struct {
  Atom *lhs;
  Atom *rhs;
} Or;

typedef union {
  char   _char;
  Range  range;
  Loop   loop;
  Or     or;
  Atom  *block;
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

Atom *parse(Str source_text, u32 *i, bool is_in_block) {
  Atom *result = NULL;
  Atom *result_end = NULL;
  bool is_escaped = false;

  while (*i < source_text.len &&
         (source_text.ptr[*i] != '\n'&&
         (source_text.ptr[*i] != ')' ||
          is_in_block))) {
    i8 _char = source_text.ptr[*i];

    ++*i;

    if (is_escaped) {
      is_escaped = false;
      LL_PREPEND(result, result_end, Atom);
      *result_end = (Atom) {
        AtomKindChar,
        { _char = _char },
        NULL
      };
      continue;
    } else if (_char == '\\') {
      is_escaped = true;
      continue;
    }

    switch (_char) {
    case '+': {
      if (!result_end) {
        ERROR("No operand was found\n");
        exit(1);
      }
      if (result_end->kind == AtomKindLoop) {
        ERROR("Loop recursion detected\n");
        exit(1);
      }

      Atom *new_atom = aalloc(sizeof(Atom));
      *new_atom = (Atom) { result_end->kind, result_end->as, NULL };
      LL_PREPEND(result, result_end, Atom);
      *result_end = (Atom) { AtomKindLoop, { .loop = { new_atom, false } }, NULL };
    } break;

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
      *result_end = (Atom) { AtomKindLoop, { .loop = { new_atom, true } }, NULL };
    } break;

    case '$': {
      LL_PREPEND(result, result_end, Atom);
      *result_end = (Atom) { AtomKindWhitespace, {0}, NULL };
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
        { .range = { result_end->as._char, ' ' } },
        NULL
      };
    } break;

    case '|': {
      if (!result || *i + 1 >= source_text.len) {
        ERROR("No operand was found\n");
        exit(1);
      }

      Atom *rhs = parse(source_text, i, is_in_block);
      --*i;
      Atom *new_atom = aalloc(sizeof(Atom));
      *new_atom = (Atom) { AtomKindOr, { .or = { result, rhs } }, NULL };
      result = new_atom;
      result_end = new_atom;
    } break;

    case '(': {
      Atom *body = parse(source_text, i, true);
      --*i;
      LL_PREPEND(result, result_end, Atom);
      *result_end = (Atom) { AtomKindBlock, { .block = body }, NULL };
    } break;

    case ')': break;

    default: {
      Atom new_atom = { AtomKindChar, { _char = _char }, NULL };

      if (result_end && result_end->kind == AtomKindRange) {
        result_end->as.range.max = _char;
      } else {
        LL_PREPEND(result, result_end, Atom);
        *result_end = new_atom;
      }
    } break;
    }
  }

  ++*i;

  return result;
}

Defs create_defs(Str source_text) {
  Defs defs = {0};

  u32 i = 0;
  while (i < source_text.len) {
    if (source_text.ptr[i] == '\n')
      continue;

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
    if (source_text.ptr[i++] != '=') {
      ERROR("Expected `=`, but got `%c`\n", source_text.ptr[i]);
      exit(1);
    }

    Atom *atoms = parse(source_text, &i, false);

    Atom *last_atom = NULL;
    Atom *atom = atoms;
    while (atom) {
      if (!atom->next)
        last_atom = atom;
      atom = atom->next;
    }
    if (last_atom &&
        last_atom->kind == AtomKindLoop &&
        last_atom->as.loop.is_from_star) {
      ERROR("Last atom should not be star\n");
      exit(1);
    }

    Def new_def = { name, atoms };
    DA_APPEND(defs, new_def);
  }

  return defs;
}

void sb_push_atoms(StringBuilder *sb, Atom *atoms, u32 state, bool is_in_loop) {
  Atom *atom = atoms;
  while (atom) {
    switch (atom->kind) {
    case AtomKindChar: {
      sb_push(sb, "  { ");
      sb_push_u32(sb, state);
      sb_push(sb, ", '");
      sb_push_char(sb, atom->as._char);
      sb_push(sb, "', '");
      sb_push_char(sb, atom->as._char);
      sb_push(sb, "', ");

      if (!is_in_loop)
        ++state;

      if (atom->next || is_in_loop)
        sb_push_u32(sb, state);
      else
        sb_push_char(sb, '0');

      sb_push(sb, " },\n");
    } break;

    case AtomKindRange: {
      sb_push(sb, "  { ");
      sb_push_u32(sb, state);
      sb_push(sb, ", '");
      sb_push_char(sb, atom->as.range.min);
      sb_push(sb, "', '");
      sb_push_char(sb, atom->as.range.max);
      sb_push(sb, "', ");

      if (!is_in_loop)
        ++state;

      if (atom->next || is_in_loop)
        sb_push_u32(sb, state);
      else
        sb_push_char(sb, '0');

      sb_push(sb, " },\n");
    } break;

    case AtomKindLoop: {
      u32 prev_state = state;
      sb_push_atoms(sb, atom->as.loop.body, state, true);
      state = prev_state;

      sb_push(sb, "  { ");
      sb_push_u32(sb, state);
      sb_push(sb, ", -1, -1, ");

      if (atom->next)
        sb_push_u32(sb, ++state);
      else
        sb_push_char(sb, '0');

      sb_push(sb, " },\n");
    } break;

    case AtomKindWhitespace: {
      char *whitespace[] = { " ", "\\n", "\\t" };

      for (u32 i = 0; i < ARRAY_LEN(whitespace); ++i) {
        sb_push(sb, "  { ");
        sb_push_u32(sb, state);
        sb_push(sb, ", '");
        sb_push(sb, whitespace[i]);
        sb_push(sb, "', '");
        sb_push(sb, whitespace[i]);
        sb_push(sb, "', ");

        if (!is_in_loop)
          ++state;

        if (atom->next || is_in_loop)
          sb_push_u32(sb, state);
        else
          sb_push_u32(sb, 0);

        sb_push(sb, " },\n");
      }
    } break;

    case AtomKindOr: {
      sb_push_atoms(sb, atom->as.or.lhs, state, is_in_loop);
      sb_push_atoms(sb, atom->as.or.rhs, state, is_in_loop);
    } break;

    case AtomKindBlock: {
      sb_push_atoms(sb, atom->as.block, state, is_in_loop);
    } break;
    }

    atom = atom->next;
  }
}

Str defs_gen_code(Defs *defs) {
  StringBuilder sb = {0};

  sb_push(&sb, "#ifndef LEXGEN_TRANSITION_TABLE\n");
  sb_push(&sb, "#define LEXGEN_TRANSITION_TABLE\n\n");

  for (u32 i = 0; i < defs->len; ++i) {
    sb_push(&sb, "TransitionRow transition_table_");
    sb_push_str(&sb, defs->items[i].name);
    sb_push(&sb, "[] = { \n");

    u32 state = 1;
    sb_push_atoms(&sb, defs->items[i].atoms, state, false);

    sb_push(&sb, "};\n\n");
  }

  sb_push(&sb, "#endif // LEXGEN_TRANSITION_TABLE\n");

  return sb_to_str(sb);
}

int main(i32 argv, char **argc) {
  if (argv < 2) {
    ERROR("output file was not provided\n");
    exit(1);
  }

  if (argv < 3) {
    ERROR("input file was not provided\n");
    return 1;
  }

  Str source_text = read_file(argc[2]);
  Defs defs = create_defs(source_text);
  write_file(argc[1], defs_gen_code(&defs));

  return 0;
}
