#ifndef WSTR_H
#define WSTR_H

#include <stdio.h>

#include "shl/shl-defs.h"
#include "shl/shl-str.h"

#define WSTR(ptr, len) ((WStr) { ptr, len })
#define WSTR_LIT(lit) ((WStr) { lit, (sizeof(lit) - 1) / sizeof(wchar_t) })
#define WSTR_FMT "%.*ls"
#define WSTR_ARG(wstr) wstrlenu((wstr).ptr, (wstr).len), (wstr).ptr

typedef struct {
  wchar_t *ptr;
  u32      len;
} WStr;

typedef struct {
  wchar_t *buffer;
  u32      cap;
  u32      len;
} WStringBuilder;

typedef void *(*LexgenAlloc)(u64 size);

u32  wstrlen(wchar_t *wstr);
u32  wstrlenu(wchar_t *wstr, u32 len);
void wtou(char *dest, WStr wstr);
Str  wtou_alloc(WStr wstr, LexgenAlloc alloc);
WStr wsb_to_wstr(WStringBuilder wsb);
void wsb_push(WStringBuilder *wsb, wchar_t *wstr);
void wsb_push_wchar(WStringBuilder *wsb, wchar_t _char);
void wsb_push_wstr(WStringBuilder *wsb, WStr wstr);
void wsb_push_wstr_uppercase(WStringBuilder *wsb, WStr wstr);
void wsb_push_u32(WStringBuilder *wsb, u32 num);

#endif // WSTR_H
