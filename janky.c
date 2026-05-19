long write(int fd, const void* buf, unsigned long count);
int printf(const char *format, ...);
void exit(int code);

#define uint unsigned int
#define bool char
#define true 1
#define false 0

#define STDIN 0
#define STDOUT 1
#define HEAP_SIZE 65536
#define MIN_NODE_SIZE 8

#define STR_MALLOC "malloc\n"
#define STR_HEAP_OOM "heap OOM!\n"
#define STR_COMPACT "compact!\n"

typedef struct HNode {
  bool free;
  uint size;
} HNode;

static bool did_init = false;
static char heap[HEAP_SIZE];
static HNode* hhead = (HNode*) &heap;
static HNode* hnext = (HNode*) &heap;
static HNode* hend = (HNode*) (&heap[0] + HEAP_SIZE);

void* malloc(uint size) {
  write(STDOUT, STR_MALLOC, sizeof(STR_MALLOC));

  if (!did_init) {
    hhead->free = true;
    hhead->size = HEAP_SIZE;
    did_init = true;
  }

  bool did_compact = false;
  HNode* next = hnext;
  uint total_free = 0;

  do {
    do {
      HNode* n = next;

      // move pointer to next node
      next += sizeof(HNode) + n->size;
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
        return n + sizeof(HNode);
      }
      
      // split node
      uint extra = n->size - size;
      n->free = false;
      n->size = size;
      hnext = n + sizeof(HNode) + size;
      hnext->free = true;
      hnext->size = extra - sizeof(HNode);
      return n + sizeof(HNode);
    } while (next != hnext);

    if (!did_compact && total_free >= size) {
      write(STDOUT, STR_COMPACT, sizeof(STR_COMPACT)); 
      exit(-1);
      did_compact = true;
      continue;
    }
  } while (!did_compact);

  write(STDOUT, STR_HEAP_OOM, sizeof(STR_HEAP_OOM));
  exit(-1);
  return 0;
}

int main(int argc, char const *argv[]) {
  char* str = malloc(16);

  for (uint i = 0; i < 15; ++i) {
    str[i] = 'A' + i;
  }
  str[15] = '\n';

  printf(str);

  return 0;
}
