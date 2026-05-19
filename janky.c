#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "janky.h"

#ifndef HEAP_SIZE
#define HEAP_SIZE 4096
#endif

#ifndef MIN_NODE_SIZE
#define MIN_NODE_SIZE 8
#endif

#ifdef DEBUG
#define LOG(S) write(1, S, sizeof(S) - 1)
#else
#define LOG(S)
#endif

typedef struct HNode {
  bool free;
  size_t size;
} HNode;

static bool did_init = false;
static char heap[HEAP_SIZE];
static HNode* hhead = (HNode*) &heap;
static HNode* hnext = (HNode*) &heap;
static HNode* hend = (HNode*) (&heap[0] + HEAP_SIZE);

static HNode* next_node(HNode* node) {
  node = ((void*) node) + sizeof(HNode) + node->size;
  if (node >= hend) {
    node = hhead;
  }
  return node;
}

static void merge_free_after(HNode* n) {
  HNode* start = n;
  n = next_node(n);
  size_t reclaimed = 0;

  while (n != start) {
    if (!n->free) {
      break;
    }
    reclaimed += n->size + sizeof(HNode);
    n = next_node(n);
  }

  start->size += reclaimed;
}

void* malloc(size_t size) {
  LOG("malloc\n");

  if (!did_init) {
    hhead->free = true;
    hhead->size = HEAP_SIZE - sizeof(HNode);
    did_init = true;
  }

  HNode* next = hnext;

  do {
    HNode* n = next;
    next = next_node(next);

    if (!n->free) {
      continue;
    }

    merge_free_after(n);

    if (n->size < size) {
      continue;
    }

    // exact-size node (or not enough to split)
    if (n->size < size + sizeof(HNode) + MIN_NODE_SIZE) {
      hnext = next;
      n->free = false;
      return ((void*) n) + sizeof(HNode);
    }
    
    // split node
    size_t extra = n->size - size;
    n->free = false;
    n->size = size;
    hnext = ((void*) n) + sizeof(HNode) + size;
    hnext->free = true;
    hnext->size = extra - sizeof(HNode);
    return ((void*) n) + sizeof(HNode);
  } while (next != hnext);

  LOG("heap oom!\n");
  return NULL;
}

void free(void* ptr) {
  LOG("free\n");

  HNode* n = (HNode*) (ptr - sizeof(HNode));
  n->free = true;

  merge_free_after(n);
}

void* realloc(void* ptr, size_t new_size) {
  LOG("realloc\n");

  void* allocated = malloc(new_size);

  if (ptr != NULL) {
    HNode* existing = (HNode*) (ptr - sizeof(HNode));

    size_t size = existing->size < new_size 
      ? existing->size 
      : new_size;

    memcpy(allocated, ptr, size);
    free(ptr);
  }

  return allocated;
}

void janky_print_heap() {
  HNode* n = hhead;
  do {
    printf(" | ");
    printf(n->free ? " " : "\x1b[7m ");
    printf("%d", n->size);
    printf(n->free ? " " : " \x1b[0m");

    n = next_node(n);
  } while (n != hhead);
  printf(" | \n");
}

#ifdef JANKY_TEST
void janky_reset(void) {
  did_init = false;
  hhead = (HNode*) &heap;
  hnext = (HNode*) &heap;
  hend = (HNode*) (&heap[0] + HEAP_SIZE);
}
#endif

