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

static void sb_reserve_space(StringBuilder *sb, u32 amount) {
  if (amount > sb->cap - sb->len) {
    if (sb->cap != 0) {
      while (amount > sb->cap - sb->len)
        sb->cap *= 2;

      sb->buffer = realloc(sb->buffer, sb->cap + 1);
    } else {
      sb->cap += amount;
      sb->buffer = malloc(sb->cap + 1);
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

void wsb_push_wchar(WStringBuilder *wsb, wchar _wchar) {
  wsb_reserve_space(wsb, 1);
  wsb->buffer[wsb->len++] = _wchar;
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

void sb_push_wchar(StringBuilder *sb, wchar _wchar) {
  if (_wchar < 0x80) {
    sb_reserve_space(sb, 1);
    sb->buffer[sb->len++] = (char) _wchar;
  } else if (_wchar < 0x800) {
    sb_reserve_space(sb, 2);
    sb->buffer[sb->len++] = (_wchar >> 6) | 0xC0;
    sb->buffer[sb->len++] = (_wchar & 0x3F) | 0x80;
  } else if (_wchar < 0x10000) {
    sb_reserve_space(sb, 3);
    sb->buffer[sb->len++] = (_wchar >> 12) | 0xE0;
    sb->buffer[sb->len++] = ((_wchar >> 6) & 0x3F) | 0x80;
    sb->buffer[sb->len++] = (_wchar & 0x3F) | 0x80;
  } else if (_wchar < 0x110000) {
    sb_reserve_space(sb, 4);
    sb->buffer[sb->len++] = (_wchar >> 18) | 0xF0;
    sb->buffer[sb->len++] = ((_wchar >> 12) & 0x3F) | 0x80;
    sb->buffer[sb->len++] = ((_wchar >> 6) & 0x3F) | 0x80;
    sb->buffer[sb->len++] = (_wchar & 0x3F) | 0x80;
  }
}
