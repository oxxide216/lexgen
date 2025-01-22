#ifndef LEXGEN_TRANSITION_TABLE
#define LEXGEN_TRANSITION_TABLE

TransitionRow transition_table_skip[] = { 
  { 1, ' ', ' ', 1 },
  { 1, '\n', '\n', 1 },
  { 1, '\t', '\t', 1 },
  { 1, -1, -1, 0 },
};

TransitionRow transition_table_ident[] = { 
  { 1, 'a', 'z', 2 },
  { 2, 'a', 'z', 2 },
  { 2, -1, -1, 0 },
};

TransitionRow transition_table_number[] = { 
  { 1, '0', '9', 2 },
  { 2, '0', '9', 2 },
  { 2, -1, -1, 0 },
};

TransitionRow transition_table_op_add[] = { 
  { 1, '+', '+', 0 },
};

TransitionRow transition_table_op_sub[] = { 
  { 1, '-', '-', 0 },
};

TransitionRow transition_table_op_mul[] = { 
  { 1, '*', '*', 0 },
};

TransitionRow transition_table_op_div[] = { 
  { 1, '/', '/', 0 },
};

#endif // LEXGEN_TRANSITION_TABLE
