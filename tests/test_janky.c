/* Unit tests for the janky allocator (malloc / free / realloc).
 *
 * janky.c defines *global* malloc/free/realloc. We compile it for tests with
 *   -Dmalloc=j_malloc -Dfree=j_free -Drealloc=j_realloc -DJANKY_NO_MAIN -DJANKY_TEST
 * so that:
 *   - Unity and libc keep using the real system allocator (no recursion), and
 *   - the allocator under test is reachable as j_malloc/j_free/j_realloc.
 *
 * The allocator stores all bookkeeping in file-static state, so janky_reset()
 * is called in setUp() to give every test a pristine heap.
 */
#include <stdint.h>
#include <string.h>
#include "unity.h"

#define HEAP_SIZE 4096 /* keep in sync with janky.c */

void *j_malloc(size_t size);
void  j_free(void *ptr);
void *j_realloc(void *ptr, size_t new_size);
void  janky_reset(void);

void setUp(void)    { janky_reset(); }
void tearDown(void) {}

/* --- malloc ------------------------------------------------------------- */

void test_malloc_returns_usable_nonnull(void) {
  unsigned char *p = j_malloc(32);
  TEST_ASSERT_NOT_NULL(p);
  /* Whole region must be writable without corrupting the allocator. */
  memset(p, 0xAB, 32);
  for (int i = 0; i < 32; i++) TEST_ASSERT_EQUAL_HEX8(0xAB, p[i]);
}

void test_malloc_distinct_allocations_do_not_overlap(void) {
  unsigned char *a = j_malloc(64);
  unsigned char *b = j_malloc(64);
  TEST_ASSERT_NOT_NULL(a);
  TEST_ASSERT_NOT_NULL(b);
  TEST_ASSERT_NOT_EQUAL(a, b);

  memset(a, 0x11, 64);
  memset(b, 0x22, 64);
  for (int i = 0; i < 64; i++) {
    TEST_ASSERT_EQUAL_HEX8(0x11, a[i]); /* b's write didn't bleed into a */
    TEST_ASSERT_EQUAL_HEX8(0x22, b[i]);
  }
}

void test_malloc_too_large_returns_null(void) {
  TEST_ASSERT_NULL(j_malloc((size_t)HEAP_SIZE * 1000));
}

void test_malloc_many_small_then_exhaust(void) {
  int got = 0;
  while (got < 4096) {
    void *p = j_malloc(16);
    if (p == NULL) break;
    got++;
  }
  TEST_ASSERT_GREATER_THAN(0, got);          /* at least some succeeded */
  TEST_ASSERT_NULL(j_malloc(HEAP_SIZE));      /* and the heap is now full */
}

/* --- free --------------------------------------------------------------- */

void test_free_makes_space_reusable(void) {
  /* Fill the heap, free one block, and require that the heap can once again
   * satisfy a request of that size. This asserts the actual contract (freed
   * memory is reclaimed) without over-specifying the returned address. */
  void *blocks[512];
  int n = 0;
  while (n < (int)(sizeof(blocks) / sizeof(blocks[0]))) {
    void *p = j_malloc(128);
    if (p == NULL) break;
    blocks[n++] = p;
  }
  TEST_ASSERT_GREATER_THAN(2, n);
  TEST_ASSERT_NULL(j_malloc(128)); /* heap is full */

  j_free(blocks[n / 2]);
  TEST_ASSERT_NOT_NULL(j_malloc(128)); /* freed block is reusable */
}

void test_free_coalesces_adjacent_blocks(void) {
  /* Carve three adjacent blocks, free the middle then the first (the order
   * the forward-coalescing free() expects). A request larger than any single
   * original block must then be satisfiable only if a+b were merged. */
  unsigned char *a = j_malloc(64);
  unsigned char *b = j_malloc(64);
  unsigned char *c = j_malloc(64);
  TEST_ASSERT_NOT_NULL(a);
  TEST_ASSERT_NOT_NULL(b);
  TEST_ASSERT_NOT_NULL(c);

  /* Exhaust the rest of the heap so the only way to serve `big` below is by
   * reusing the coalesced a+b hole. */
  while (j_malloc(64) != NULL) { }

  j_free(b);
  j_free(a);

  unsigned char *big = j_malloc(140); /* > 64; needs the merged a+b hole */
  TEST_ASSERT_NOT_NULL(big);
  memset(big, 0x5A, 140);             /* writable across the merged span */
  for (int i = 0; i < 140; i++) TEST_ASSERT_EQUAL_HEX8(0x5A, big[i]);
}

/* --- realloc ------------------------------------------------------------ */

void test_realloc_null_behaves_like_malloc(void) {
  unsigned char *p = j_realloc(NULL, 48);
  TEST_ASSERT_NOT_NULL(p);
  memset(p, 0x7E, 48);
  for (int i = 0; i < 48; i++) TEST_ASSERT_EQUAL_HEX8(0x7E, p[i]);
}

void test_realloc_grow_preserves_contents(void) {
  unsigned char *p = j_malloc(32);
  TEST_ASSERT_NOT_NULL(p);
  for (int i = 0; i < 32; i++) p[i] = (unsigned char)i;

  unsigned char *q = j_realloc(p, 256);
  TEST_ASSERT_NOT_NULL(q);
  for (int i = 0; i < 32; i++) TEST_ASSERT_EQUAL_HEX8((unsigned char)i, q[i]);
}

void test_realloc_shrink_preserves_prefix(void) {
  unsigned char *p = j_malloc(200);
  TEST_ASSERT_NOT_NULL(p);
  for (int i = 0; i < 200; i++) p[i] = (unsigned char)(i * 7);

  unsigned char *q = j_realloc(p, 40);
  TEST_ASSERT_NOT_NULL(q);
  for (int i = 0; i < 40; i++)
    TEST_ASSERT_EQUAL_HEX8((unsigned char)(i * 7), q[i]);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_malloc_returns_usable_nonnull);
  RUN_TEST(test_malloc_distinct_allocations_do_not_overlap);
  RUN_TEST(test_malloc_too_large_returns_null);
  RUN_TEST(test_malloc_many_small_then_exhaust);
  RUN_TEST(test_free_makes_space_reusable);
  RUN_TEST(test_free_coalesces_adjacent_blocks);
  RUN_TEST(test_realloc_null_behaves_like_malloc);
  RUN_TEST(test_realloc_grow_preserves_contents);
  RUN_TEST(test_realloc_shrink_preserves_prefix);
  return UNITY_END();
}
