#ifndef LEXGEN_TRANSITION_TABLE
#define LEXGEN_TRANSITION_TABLE

TransitionRow transition_table_test0[] = { 
  { 1, 't', 2 },
  { 2, 'e', 3 },
  { 3, 'x', 4 },
  { 4, 't', 5 },
  { 5, 't', 5 },
  { 5, (i8) -1, 0 },
};

TransitionRow transition_table_test1[] = { 
  { 1, 't', 1 },
  { 1, (i8) -1, 1 },
  { 1, 'y', 0 },
};

#endif // LEXGEN_TRANSITION_TABLE
