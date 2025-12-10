#include <stdbool.h>

#include "lexgen/wstr.h"
#include "shl/shl-str.h"

WStr read_file(char *path);
bool write_file(char *path, WStr content);
