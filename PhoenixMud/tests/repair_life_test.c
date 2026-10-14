/***********************************************************************
*  File: repair_life_test.c                                            *
*  Usage: standalone checks for a weapon's life at the smiths          *
*                                                                      *
*  Build and run with `make test` from the repository root.  Links     *
*  only repair_life.o.                                                 *
***********************************************************************/

#include <stdio.h>
#include "../src/repair_life.h"

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

#define BLOWS 600

/* the first service only a master does, or -1 */
static int first_master(int points, int per, int no_repair)
   {
   struct repair_service at;
   int n = repair_life(points, per, no_repair, BLOWS, 0, NULL, NULL), k;

   for (k = 0; k < n; k++)
      {
      repair_life(points, per, no_repair, BLOWS, k, NULL, &at);
      if (at.master)
         return k;
      }
   return -1;
   }

/* a 100-slot weapon at 4 a wear: 95 repairs, then nine reforges */
static void test_ordinary(void)
   {
   struct repair_service at;
   long charge = 0;
   int n = repair_life(100, 4, 0, BLOWS, 0, &charge, &at);

   CHECK(n == 509);
   CHECK(charge == 3625);
   /* handed over at 52 of 100: the next wear would leave it at half */
   CHECK(at.orig == 100 && at.max == 100 && at.cur == 52 && !at.master);
   CHECK(first_master(100, 4, 0) == 95);
   repair_life(100, 4, 0, BLOWS, 95, NULL, &at);
   CHECK(at.master && at.orig == 100 && at.max <= 2);
   /* after the first reforge: a weapon of 90, whole, a shorter charge */
   repair_life(100, 4, 0, BLOWS, 96, &charge, &at);
   CHECK(at.orig == 90 && at.max == 90 && !at.master);
   CHECK(charge < 3625);
   repair_life(100, 4, 0, BLOWS, n, &charge, NULL);
   CHECK(charge == 600);
   }

/* the others, as the TS port counts them */
static void test_counts(void)
   {
   long charge = 0;

   CHECK(repair_life(110, 4, 0, BLOWS, 0, &charge, NULL) == 564 && charge == 3996);
   CHECK(repair_life(125, 2, 0, BLOWS, 0, &charge, NULL) == 692 && charge == 9234);
   CHECK(repair_life(175, 4, 0, BLOWS, 0, &charge, NULL) == 947 && charge == 6425);
   CHECK(repair_life(175, 2, 0, BLOWS, 0, &charge, NULL) == 967 && charge == 12982);
   CHECK(first_master(110, 4, 0) == 105);
   CHECK(first_master(125, 2, 0) == 122);
   CHECK(first_master(175, 4, 0) == 170);
   }

/* a no-repair weapon goes to a master every time */
static void test_no_repair(void)
   {
   struct repair_service at;
   long charge = 0;
   int k, n = repair_life(40, 4, 1, BLOWS, 0, &charge, NULL);

   CHECK(n == 9);
   CHECK(charge == 2400);
   for (k = 0; k < n; k++)
      {
      repair_life(40, 4, 1, BLOWS, k, NULL, &at);
      CHECK(at.master && at.orig == 40 - 4 * k);
      }
   CHECK(repair_life(40, 8, 1, BLOWS, 0, &charge, NULL) == 9 && charge == 1200);
   }

/* the charges add up to the whole life, give or take the rounding */
static void test_whole_life(void)
   {
   long charge, sum = 0;
   int k, n = repair_life(100, 4, 0, BLOWS, 0, NULL, NULL);

   for (k = 0; k <= n; k++)
      {
      repair_life(100, 4, 0, BLOWS, k, &charge, NULL);
      sum += charge;
      }
   /* 2,178 wears in all */
   CHECK(sum - 2178L * BLOWS <= n + 1 && 2178L * BLOWS - sum <= n + 1);
   }

int main(void)
   {
   test_ordinary();
   test_counts();
   test_no_repair();
   test_whole_life();
   printf("repair_life: %d checks, %d failures\n", checks, failures);
   return failures ? 1 : 0;
   }
