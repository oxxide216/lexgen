#include <wctype.h>
#include <wchar.h>

#include "lexgen/wstr.h"

static void wsb_reserve_space(WStringBuilder *wsb, u32 amount) {
  if (amount > wsb->cap - wsb->len) {
    if (wsb->cap != 0) {
      while (amount > wsb->cap - wsb->len)
        wsb->cap *= 2;

      wsb->buffer = realloc(wsb->buffer, (wsb->cap + 1) * sizeof(wchar));
    } else {
      wsb->cap += amount;
      wsb->buffer = malloc((wsb->cap + 1) * sizeof(wchar));
    }
  }
}

u32 wstrlen(wchar *wstr) {
  u32 len = 0;

  while (wstr[len])
    ++len;

  return len;
}

u32 wstrlenu(wchar *wstr, u32 len) {
  u32 _len = 0;

  for (u32 i = 0; i < len * sizeof(wchar); ++i)
    if (((char *) wstr)[i])
      ++_len;

  return _len;
}

void wtou(char *dest, WStr wstr) {
  for (u32 i = 0, j = 0; i < wstr.len * sizeof(wchar); ++i)
    if (((char *) wstr.ptr)[i])
      dest[j++] = ((char *) wstr.ptr)[i];
}

WStr wsb_to_wstr(WStringBuilder wsb) {
  return (WStr) {
    .ptr = wsb.buffer,
    .len = wsb.len,
  };
}

void wsb_push(WStringBuilder *wsb, wchar *wstr) {
  wsb_push_wstr(wsb, WSTR(wstr, wstrlen(wstr)));
}

void wsb_push_wchar(WStringBuilder *wsb, wchar wchar) {
  wsb_reserve_space(wsb, 1);
  wsb->buffer[wsb->len++] = wchar;
}

void wsb_push_wstr(WStringBuilder *wsb, WStr wstr) {
  wsb_reserve_space(wsb, wstr.len);
  memcpy(wsb->buffer + wsb->len, wstr.ptr, wstr.len * sizeof(wchar));
  wsb->len += wstr.len;
}

void wsb_push_wstr_uppercase(WStringBuilder *wsb, WStr wstr) {
  u32 prev_len = wsb->len;

  wsb_push_wstr(wsb, wstr);

  for (u32 i = prev_len; i < wsb->len; ++i)
    wsb->buffer[i] = towupper(wsb->buffer[i]);
}

void wsb_push_u32(WStringBuilder *wsb, u32 num) {
  u32 _num = num;
  u32 len = 1;

  while (_num >= 10) {
    ++len;
    _num /= 10;
  }

  wsb_reserve_space(wsb, len);
  wsb->len += len;

  _num = num;

  for (u32 i = 1; i <= len; ++i) {
    wsb->buffer[wsb->len - i] = '0' + _num % 10;
    _num /= 10;
  }
}
