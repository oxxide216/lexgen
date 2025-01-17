#include <ctype.h>

#include "io.h"
#include "defs.h"
#include "log.h"

typedef enum {
  AtomKindChar = 0,
  AtomKindRange,
  AtomKindPlus,
  AtomKindStar,
} AtomKind;

typedef struct {
  char min, max;
} Range;

typedef struct Atom Atom;

typedef union {
  char   _char;
  Range  range;
  Atom  *plus;
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

    Atom *new_atom = malloc(sizeof(Atom));

    switch (_char) {
    case '+': {
      if (!result_end) {
        ERROR("No operand was found\n");
        exit(1);
      }

      *new_atom = *result_end;
      *result_end = (Atom) { AtomKindPlus, { .plus = new_atom }, NULL };
    } break;

    case '*': {
      if (!result_end) {
        ERROR("No operand was found\n");
        exit(1);
      }

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

    case AtomKindPlus: {
      print_atoms(atom->as.plus);
      putc('+', stdout);
    } break;

    case AtomKindStar: {
      print_atoms(atom->as.star);
      putc('*', stdout);
    } break;
    }

    atom = atom->next;
  }
}

int main(i32 argv, char **argc) {
  if (argv < 2) {
    ERROR("input file was not provided\n");
    return 1;
  }

  Str source_text = read_file(argc[1]);
  Da(Def) defs = {0};
  u32 i = 0;
  while (i < source_text.len) {
    if (source_text.ptr[i] == '\n')
      continue;

    Str name = STR(source_text.ptr + i, 0);
    while (isalnum(source_text.ptr[i])) {
      ++name.len;
      ++i;
    }

    if (source_text.ptr[i++] != '=') {
      ERROR("Expected `=`, but got `%c`", source_text.ptr[i]);
    }

    Atom *atoms = lex_line(source_text, '\n', &i);

    Def new_def = { name, atoms };
    DA_APPEND(defs, new_def);
  }

  for (u32 i = 0; i < defs.len; ++i) {
    str_print(defs.items[i].name);
    fputs(" = `", stdout);
    print_atoms(defs.items[i].atoms);
    fputs("`\n", stdout);
  }

  return 0;
}
