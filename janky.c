#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define bool char
#define true 1
#define false 0

#define HEAP_SIZE 65536
#define MIN_NODE_SIZE 8

#define STR_MALLOC "malloc\n"
#define STR_FREE "free\n"
#define STR_REALLOC "realloc\n"
#define STR_HEAP_OOM "heap OOM!\n"
#define STR_COMPACT "compact!\n"

typedef struct HNode {
  bool free;
  size_t size;
} HNode;

static bool did_init = false;
static char heap[HEAP_SIZE];
static HNode* hhead = (HNode*) &heap;
static HNode* hnext = (HNode*) &heap;
static HNode* hend = (HNode*) (&heap[0] + HEAP_SIZE);

void* malloc(size_t size) {
  write(1, STR_MALLOC, sizeof(STR_MALLOC));

  if (!did_init) {
    hhead->free = true;
    hhead->size = HEAP_SIZE;
    did_init = true;
  }

  bool did_compact = false;
  HNode* next = hnext;
  size_t total_free = 0;

  do {
    do {
      HNode* n = next;

      // move pointer to next node
      next = ((void*) next) + sizeof(HNode) + n->size;
      if (next > hend) {
        next = hhead;
      }

      if (!n->free) {
        continue;
      }

      if (n->size < size) {
        total_free += n->size;
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

    if (!did_compact && total_free >= size) {
      write(1, STR_COMPACT, sizeof(STR_COMPACT));
      exit(-1);
      did_compact = true;
      continue;
    }
  } while (!did_compact);

  write(1, STR_HEAP_OOM, sizeof(STR_HEAP_OOM));
  exit(-1);
  return 0;
}

void free(void* ptr) {
  write(1, STR_FREE, sizeof(STR_FREE));
  HNode* n = (HNode*) (ptr - sizeof(HNode));
  n->free = true;
}

void* realloc(void* ptr, size_t new_size) {
  write(1, STR_REALLOC, sizeof(STR_REALLOC));

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

int main(int argc, char const *argv[]) {
  while (true) {
    printf("> ");

    char* line = NULL;
    size_t size = 0;
    getline(&line, &size, stdin);

    printf(line);
    printf("\n");
  }

  return 0;
}
