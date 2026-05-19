#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define bool char
#define true 1
#define false 0

#define HEAP_SIZE 4096
#define MIN_NODE_SIZE 8

#define DEBUG
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

HNode* next_node(HNode* node) {
  node = ((void*) node) + sizeof(HNode) + node->size;
  if (node > hend) {
    node = hhead;
  }
  return node;
}

void* malloc(size_t size) {
  LOG("malloc\n");

  if (!did_init) {
    hhead->free = true;
    hhead->size = HEAP_SIZE;
    did_init = true;
  }

  HNode* next = hnext;

  do {
    HNode* n = next;
    next = next_node(next);

    if (!n->free) {
      continue;
    }

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

void print_heap() {
  HNode* n = hhead;
  bool first = true;
  do {
    printf(" | ");
    printf(n->free ? " " : "\x1b[7m ");
    printf("%d", n->size);
    printf(n->free ? " " : " \x1b[0m");

    n = next_node(n);
  } while (n != hhead);
  printf(" | \n");
}

int main(int argc, char const *argv[]) {
  int result = 0;
  do {
    printf("> ");

    char* line;
    size_t size;
    result = getline(&line, &size, stdin);

    printf(line);
    printf("\n");

    free(line);
  } while (result > 0);

  print_heap();
  return 0;
}
