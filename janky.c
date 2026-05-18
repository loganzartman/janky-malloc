long write(int fd, const void* buf, unsigned long count);
void exit(int code);

#define STDIN 0
#define STDOUT 1
#define HEAP_SIZE 2048
#define uint unsigned int

static uint hptr = 0;
static char heap[HEAP_SIZE];

void* malloc(uint size) {
  if (HEAP_SIZE - hptr < size) {
    exit(-1);
  }
  void* addr = &heap + hptr;
  hptr += size;
  return addr;
}

int main(int argc, char const *argv[]) {
  char* str = malloc(16);

  for (uint i = 0; i < 16; ++i) {
    str[i] = 'A' + i;
  }

  write(STDOUT, str, 16);
  write(STDOUT, "\n", 1);

  return 0;
}
