/***********************************************************************
*  File: help_match_test.c                                             *
*  Usage: standalone checks for a word naming a help entry exactly     *
*                                                                      *
*  Build and run with `make test` from the repository root.  Links     *
*  only help_match.o.                                                  *
***********************************************************************/

#include <stdio.h>
#include "../src/help_match.h"

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

/* any name on the line; the vectors are shared with the TS engine's test */
static void test_any_name(void)
   {
   CHECK(is_help_exact("get", "TAKE GET", 0));
   CHECK(is_help_exact("GET", "TAKE GET", 0));
   CHECK(!is_help_exact("get", "GET_OUT_OF_DEATH_FREE_TOKEN", 0));
   CHECK(is_help_exact("up", "NORTH SOUTH EAST WEST UP DOWN", 0));
   CHECK(!is_help_exact("u", "NORTH SOUTH EAST WEST UP DOWN", 0));
   CHECK(is_help_exact(".", "GOSSIP . GRATS OOC", 0));
   CHECK(is_help_exact("at", "WIZAT \"AT\"", 0));
   CHECK(is_help_exact("magic missile", "\"MAGIC MISSILE\"", 0));
   CHECK(!is_help_exact("magic", "\"MAGIC MISSILE\"", 0));
   CHECK(is_help_exact("get  ", "TAKE GET", 0));
   CHECK(!is_help_exact("", "TAKE GET", 0));
   CHECK(!is_help_exact("   ", "TAKE GET", 0));
   }

/* the first name only */
static void test_first_name(void)
   {
   CHECK(is_help_exact("credits", "CREDITS", 1));
   CHECK(!is_help_exact("credits", "CIRCLEMUD CREDITS PHOENIXMUD", 1));
   CHECK(is_help_exact("circlemud", "CIRCLEMUD CREDITS PHOENIXMUD", 1));
   CHECK(is_help_exact("lay hands", "\"LAY HANDS\" LAY_HANDS", 1));
   CHECK(!is_help_exact("lay_hands", "\"LAY HANDS\" LAY_HANDS", 1));
   }

int main(void)
   {
   test_any_name();
   test_first_name();
   printf("help_match: %d checks, %d failures\n", checks, failures);
   return failures ? 1 : 0;
   }
