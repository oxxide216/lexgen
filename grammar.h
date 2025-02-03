#ifndef LEXGEN_TRANSITION_TABLE
#define LEXGEN_TRANSITION_TABLE

#define TT_PROC 0
TransitionRow tt_proc[] = { 
  { 1, 'p', 'p', 2 },
  { 2, 'r', 'r', 3 },
  { 3, 'o', 'o', 4 },
  { 4, 'c', 'c', 0 },
};

#define TT_IDENT 1
TransitionRow tt_ident[] = { 
  { 1, 'a', 'z', 2 },
  { 2, 'A', 'A', 3 },
  { 1, '0', '9', 2 },
  { 3, -1, -1, 4 },
  { 4, 'a', 'a', 5 },
  { 5, 'b', 'b', 6 },
  { 6, 'c', 'c', 0 },
};

#endif // LEXGEN_TRANSITION_TABLE
