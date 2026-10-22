/***********************************************************************
*  File: size_fit_test.c                                               *
*  Usage: standalone checks for a size brought inside a race's range   *
*                                                                      *
*  Build and run with `make test` from the repository root.  Links     *
*  only size_fit.o.                                                    *
***********************************************************************/

#include <stdio.h>
#include "../src/size_fit.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond) check((cond), #cond, __LINE__)

static void check(int cond, const char *what, int line)
   {
   checks++;
   if (!cond)
      {
      failures++;
      printf("FAIL %s:%d  %s\n", __FILE__, line, what);
      }
   }

/* a size already in range is kept */
static void test_in_range(void)
   {
   CHECK(size_fit(70, 62, 80, 434, SIZE_SALT_HEIGHT) == 70);
   CHECK(size_fit(62, 62, 80, 434, SIZE_SALT_HEIGHT) == 62);
   CHECK(size_fit(80, 62, 80, 434, SIZE_SALT_HEIGHT) == 80);
   }

/* out of range: inside it, the same for the same id every time.  These
 * vectors are shared with the TS engine's test. */
static void test_out_of_range(void)
   {
   CHECK(size_fit(64, 84, 108, 434, SIZE_SALT_HEIGHT) == 105);    /* a Titan kept at Half-Elf size */
   CHECK(size_fit(119, 250, 350, 434, SIZE_SALT_WEIGHT) == 312);
   CHECK(size_fit(0, 74, 94, 1, SIZE_SALT_HEIGHT) == 93);          /* never rolled */
   CHECK(size_fit(0, 424, 813, 1, SIZE_SALT_WEIGHT) == 573);       /* a Half-Ogre past one byte */
   CHECK(size_fit(13, 213, 304, 123456, SIZE_SALT_WEIGHT) == 297); /* a wrapped weight */
   CHECK(size_fit(0, 62, 72, 0, SIZE_SALT_HEIGHT) == 62);
   }

/* every id lands inside the range */
static void test_bounds(void)
   {
   unsigned int id;
   int ok = 1;

   for (id = 0; id < 5000; id++)
      {
      long h = size_fit(0, 84, 108, id, SIZE_SALT_HEIGHT);
      long w = size_fit(0, 424, 813, id, SIZE_SALT_WEIGHT);
      if (h < 84 || h > 108 || w < 424 || w > 813)
         ok = 0;
      }
   CHECK(ok);
   }

int main(void)
   {
   test_in_range();
   test_out_of_range();
   test_bounds();
   printf("size_fit: %d checks, %d failures\n", checks, failures);
   return failures ? 1 : 0;
   }
