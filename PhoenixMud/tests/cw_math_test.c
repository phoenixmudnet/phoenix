/***********************************************************************
*  File: cw_math_test.c                                                *
*  Usage: standalone checks for the class weapon's arithmetic          *
*                                                                      *
*  Build and run with `make test` from the repository root.  Links     *
*  only cw_math.o.                                                     *
***********************************************************************/

#include <stdio.h>

int cw_cast_points(int base, int mana, int full);

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

int main(void)
   {
   /* a full cast: the whole weight, and no more for a dearer one */
   CHECK(cw_cast_points(25, 40, 40) == 25);
   CHECK(cw_cast_points(25, 90, 40) == 25);
   /* a cheap cast: its share, rounded down */
   CHECK(cw_cast_points(25, 20, 40) == 12);
   CHECK(cw_cast_points(25, 24, 40) == 15);
   CHECK(cw_cast_points(25, 10, 40) == 6);
   CHECK(cw_cast_points(25, 1, 40) == 0);
   /* nothing cast, or nothing to weigh */
   CHECK(cw_cast_points(25, 0, 40) == 0);
   CHECK(cw_cast_points(25, -5, 40) == 0);
   CHECK(cw_cast_points(0, 40, 40) == 0);
   CHECK(cw_cast_points(25, 40, 0) == 0);

   printf("cw_math_test: %d checks, %d failed\n", checks, failures);
   return failures ? 1 : 0;
   }
