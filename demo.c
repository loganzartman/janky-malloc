/* a throwaway driver that exercises the janky allocator. */
#include <stdio.h>
#include <stdlib.h>

#define DEBUG
#include "janky.h"

int main(int argc, char const *argv[]) {
  (void) argc;
  (void) argv;

  int result = 0;
  do {
    printf("> ");

    char* line = NULL;
    size_t size = 0;
    result = getline(&line, &size, stdin);

    printf(line);
    printf("\n");

    free(line);
  } while (result > 0);

  janky_print_heap();
  return 0;
}

