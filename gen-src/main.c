#include <ctype.h>

#include "io.h"
#include "defs.h"
#include "log.h"

typedef enum {
  AtomKindChar = 0,
  AtomKindRange,
  AtomKindStar,
} AtomKind;

typedef struct {
  char min, max;
} Range;

typedef struct Atom Atom;

typedef union {
  char   _char;
  Range  range;
  Atom  *star;
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

Atom *lex_line(Str source_text, i8 delimeter, u32 *i) {
  Atom *result = NULL;
  Atom *result_end = NULL;
  bool is_escaped = false;

  while (*i < source_text.len && source_text.ptr[*i] != delimeter) {
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

      Atom *new_atom = aalloc(sizeof(Atom));
      *new_atom = (Atom) { result_end->kind, result_end->as, NULL };

      LL_PREPEND(result, result_end, Atom);
      *result_end = (Atom) { AtomKindStar, { .star = new_atom }, NULL };
    } break;

    case '*': {
      if (!result_end) {
        ERROR("No operand was found\n");
        exit(1);
      }

      Atom *new_atom = aalloc(sizeof(Atom));

      *new_atom = *result_end;
      *result_end = (Atom) { AtomKindStar, { .star = new_atom }, NULL };
    } break;

    default: {
      LL_PREPEND(result, result_end, Atom);
      *result_end = (Atom) { AtomKindChar, { _char = _char }, NULL };
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

    if (i == source_text.len) {
      ERROR("Expected `=`, but got EOF\n");
      exit(1);
    }
    if (source_text.ptr[i++] != '=') {
      ERROR("Expected `=`, but got `%c`\n", source_text.ptr[i]);
      exit(1);
    }

    Atom *atoms = lex_line(source_text, '\n', &i);

    Def new_def = { name, atoms };
    DA_APPEND(defs, new_def);
  }

  return defs;
}

void print_atoms(Atom *atoms) {
  Atom *atom = atoms;

  while (atom) {
    if (atom != atoms)
      putc(' ', stdout);

    switch (atom->kind) {
    case AtomKindChar: {
      putc(atom->as._char, stdout);
    } break;

    case AtomKindRange: {
      printf("%c-%c", atom->as.range.min, atom->as.range.max);
    } break;

    case AtomKindStar: {
      print_atoms(atom->as.star);
      putc('*', stdout);
    } break;
    }

    atom = atom->next;
  }
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
      sb_push(sb, "', ");

      if (!is_in_loop)
        ++state;

      if (atom->next || is_in_loop)
        sb_push_u32(sb, state);
      else
        sb_push_u32(sb, 0);

      sb_push(sb, " },\n");
    } break;

    case AtomKindRange: {
      ERROR("not implemented\n");
      exit(1);
    } break;

    case AtomKindStar: {
      u32 prev_state = state;
      sb_push_atoms(sb, atom->as.star, state, true);
      state = prev_state;

      sb_push(sb, "  { ");
      sb_push_u32(sb, state);
      sb_push(sb, ", ");
      sb_push(sb, "(i8) -1");
      sb_push(sb, ", ");

      if (atom->next)
        sb_push_u32(sb, state);
      else
        sb_push_u32(sb, 0);

      sb_push(sb, " },\n");
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
