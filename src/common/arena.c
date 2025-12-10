#include <stdlib.h>
#include <stdio.h>

#include "lexgen/arena.h"

void *arena_alloc(Arena *arena, u32 size) {
  Segment *segment = arena->segments;
  Segment **segment_next = &arena->segments;
  while (segment) {
    if (segment->len + size <= segment->cap) {
      void *result = segment->space + segment->len;
      segment->len += size;

      return result;
    }

    segment_next = &segment->next;
    segment = segment->next;
  }

  u32 new_cap = DEFAULT_ARENA_SEGMENT_SIZE;
  if (new_cap < size)
    new_cap = size;

  (*segment_next) = malloc(sizeof(Segment) + new_cap);
  (*segment_next)->space = (*segment_next) + 1;
  (*segment_next)->len = size;
  (*segment_next)->cap = new_cap;
  (*segment_next)->next = NULL;

  memset((*segment_next)->space, 0, (*segment_next)->cap);

  return (*segment_next)->space;
}

void arena_reset(Arena *arena) {
  Segment *segment = arena->segments;
  while (segment) {
    segment->len = 0;

    memset(segment->space, 0, segment->cap);

    segment = segment->next;
  }
}

void arena_free(Arena *arena) {
  Segment *segment = arena->segments;
  while (segment) {
    Segment *next = segment->next;
    free(segment);
    segment = next;
  }

  arena->segments = NULL;
}
