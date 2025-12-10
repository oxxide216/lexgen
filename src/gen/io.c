#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "lexgen/io.h"
#include "lexgen/wstr.h"
#include "shl/shl-defs.h"

WStr read_file(char *path) {
  FILE *file = fopen(path, "r");
  if (!file)
    return (WStr) {0};

  WStringBuilder wsb = {0};

  wchar _char;
  while ((_char = fgetwc(file)) != (wchar) EOF)
    wsb_push_wchar(&wsb, _char);

  fclose(file);

  WStr content = wsb_to_wstr(wsb);

  return content;
}

bool write_file(char *path, WStr content) {
  FILE *file = fopen(path, "w");
  if (!file) {
    return false;
  }

  for (u32 i = 0; i < content.len * sizeof(wchar); ++i)
    if (((char *) content.ptr)[i])
      fputc(((char *) content.ptr)[i], file);

  fclose(file);

  return true;
}
