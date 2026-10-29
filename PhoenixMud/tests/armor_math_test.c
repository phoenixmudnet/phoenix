/***********************************************************************
*  File: armor_math_test.c                                             *
*  Usage: standalone checks for the class armor's arithmetic           *
*                                                                      *
*  Build and run with `make test` from the repository root.  Links     *
*  armor_math.o and hero_state.o.                                      *
***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../src/armor_math.h"

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

static double half(void) { return 0.5; }
static double zero(void) { return 0.0; }

static void add(struct obj_armor_data *a, int stat, int essences, int over)
   {
   a->pts[a->num].stat = stat;
   a->pts[a->num].essences = essences;
   a->pts[a->num].over = over;
   a->num++;
   }

int main(void)
   {
   struct obj_armor_data a;
   struct class_armor_state st;
   int pool[ARMOR_NUM_STATS], n, cls, slot, cost = 0, over = 0, totals[ARMOR_NUM_STATS], e, p, i;
   long gold;
   double brk = 0;

   /* the pieces: six a class, 27600 + 6 x class index + slot */
   CHECK(armor_vnum(0, ARMOR_HEAD) == 27600);
   CHECK(armor_vnum(16, ARMOR_ARMS) == 27689);
   CHECK(armor_vnum(7, ARMOR_HEAD) == -1);
   CHECK(armor_piece_of(27607, &cls, &slot) && cls == 1 && slot == ARMOR_BODY);
   CHECK(!armor_piece_of(27690, &cls, &slot));

   /* the weight drives the limit: two points a part, one a part past it */
   CHECK(armor_limit(ARMOR_HEAD) == 4 && armor_over_limit(ARMOR_HEAD) == 2);
   CHECK(armor_limit(ARMOR_BODY) == 6 && armor_over_limit(ARMOR_BODY) == 3);
   CHECK(armor_limit(ARMOR_FEET) == 2 && armor_over_limit(ARMOR_FEET) == 1);

   /* the pool: the slot's stats that fit half the piece, then hitroll and
    * damroll; strength only where it fits (body, legs) */
   n = armor_pool(ARMOR_HEAD, pool);
   CHECK(n == 6 && pool[0] == ARMOR_STAT_HP && pool[1] == ARMOR_STAT_MANA &&
         pool[2] == ARMOR_STAT_BREATHSAVE && pool[3] == ARMOR_STAT_SPELLSAVE &&
         pool[4] == ARMOR_STAT_HITROLL && pool[5] == ARMOR_STAT_DAMROLL);
   n = armor_pool(ARMOR_FEET, pool);
   CHECK(n == 6 && pool[0] == ARMOR_STAT_DEX && pool[1] == ARMOR_STAT_HP &&
         pool[2] == ARMOR_STAT_MANA && pool[3] == ARMOR_STAT_ARMOR);
   n = armor_pool(ARMOR_LEGS, pool);
   CHECK(n == 7 && pool[4] == ARMOR_STAT_STR);

   /* within the limit */
   memset(&a, 0, sizeof(a));
   CHECK(armor_check(ARMOR_HEAD, &a, ARMOR_STAT_HP, &cost, &brk, &over) == ATTUNE_OK &&
         cost == 100 && !over);
   CHECK(armor_check(ARMOR_BODY, &a, ARMOR_STAT_STR, &cost, &brk, &over) == ATTUNE_OK &&
         cost == 200);
   CHECK(armor_check(ARMOR_FEET, &a, ARMOR_STAT_STR, &cost, &brk, &over) == ATTUNE_NOT_IN_POOL);
   add(&a, ARMOR_STAT_HP, 100, 0);
   add(&a, ARMOR_STAT_HP, 100, 0);
   CHECK(armor_check(ARMOR_HEAD, &a, ARMOR_STAT_HP, &cost, &brk, &over) == ATTUNE_STAT_FULL);
   memset(&a, 0, sizeof(a));
   add(&a, ARMOR_STAT_HITROLL, 100, 0);
   add(&a, ARMOR_STAT_DAMROLL, 100, 0);
   CHECK(armor_check(ARMOR_HEAD, &a, ARMOR_STAT_DAMROLL, &cost, &brk, &over) == ATTUNE_OFFENSE_FULL);

   /* past the limit: dearer each point, a rising chance to break */
   memset(&a, 0, sizeof(a));
   add(&a, ARMOR_STAT_HP, 100, 0);
   add(&a, ARMOR_STAT_DEX, 100, 0);
   CHECK(armor_check(ARMOR_FEET, &a, ARMOR_STAT_HP, &cost, &brk, &over) == ATTUNE_OK &&
         over && cost == 200 && fabs(brk - 0.15) < 1e-9);
   CHECK(armor_check(ARMOR_FEET, &a, ARMOR_STAT_STR, &cost, &brk, &over) == ATTUNE_NOT_IN_POOL);
   add(&a, ARMOR_STAT_HP, 200, 1);
   CHECK(armor_check(ARMOR_FEET, &a, ARMOR_STAT_DEX, &cost, &brk, &over) == ATTUNE_OVER_FULL);
   CHECK(armor_over_cost(3) == 800 && fabs(armor_over_break(2) - 0.30) < 1e-9);

   /* totals and the armourer's terms */
   armor_totals(&a, totals);
   CHECK(totals[ARMOR_STAT_HP] == 20 && totals[ARMOR_STAT_DEX] == 1);
   armor_unattune_terms(&a, ARMOR_STAT_HP, &e, &gold, &p);
   CHECK(e == 225 && p == 2 && gold == 40000);
   memset(&a, 0, sizeof(a));
   add(&a, ARMOR_STAT_HP, 100, 0);
   add(&a, ARMOR_STAT_DEX, 200, 1);
   armor_settle_over(&a, ARMOR_FEET);
   CHECK(a.pts[1].over == 0);
   /* a body: damroll at half (3), hp to the limit, three damroll past it;
    * the hp taken out, the damroll past the limit could not have gone on
    * within it, so it stays past it and no more goes on */
   memset(&a, 0, sizeof(a));
   for (i = 0; i < 3; i++) add(&a, ARMOR_STAT_DAMROLL, 100, 0);
   for (i = 0; i < 3; i++) add(&a, ARMOR_STAT_DAMROLL, 200, 1);
   armor_settle_over(&a, ARMOR_BODY);
   CHECK(armor_points_over(&a) == 3);
   CHECK(armor_check(ARMOR_BODY, &a, ARMOR_STAT_DAMROLL, &cost, &brk, &over) != ATTUNE_OK);

   /* the set's limit costs the six pieces' work: one essence per 108 */
   CHECK(fabs(armor_work_per_essence() - 108.0) < 1e-9);
   CHECK(armor_essences_for(108.0 * 3, 0.5) == 3);
   CHECK(armor_essences_for(54.0, 0.4) == 1 && armor_essences_for(54.0, 0.6) == 0);

   /* the drop: the mark is the piece's parts of the total, spread */
   CHECK(armor_mark_for(ARMOR_BODY, 0.5) == 64800);
   CHECK(armor_mark_for(ARMOR_FEET, 0.0) == 10800);
   armor_state_fresh(&st, 0);
   st.next = ARMOR_FEET;
   st.mark = 10;
   CHECK(armor_add_work(&st, 12, half) == ARMOR_FEET && (st.got & (1 << ARMOR_FEET)));
   CHECK(armor_got_count(st.got) == 1);
   CHECK(armor_pick_next(0, 0.0) == ARMOR_HEAD);
   CHECK(armor_pick_next(0x3f, 0.5) == -1);
   armor_state_fresh(&st, 0);
   CHECK(armor_add_work(&st, 0, zero) == -1 && st.next == ARMOR_HEAD);

   /* the bonus: the armor's part on top of the weapon's */
   CHECK(fabs(armor_factor(0, 0.187, 0, armor_slot_fraction(ARMOR_BODY)) -
              (1 + 0.201 * (exp(0.187) - 1))) < 1e-9);
   CHECK(armor_factor(0, 0, 1, 0.5) == 1.0);
   CHECK(armor_set_focus_mult(2) == 1.0 && armor_set_focus_mult(3) == 1.25);

   /* stats by the names players type */
   CHECK(armor_stat_parse("hp") == ARMOR_STAT_HP && armor_stat_parse("hit points") == ARMOR_STAT_HP);
   CHECK(armor_stat_parse("Strength") == ARMOR_STAT_STR && armor_stat_parse("save spell") == ARMOR_STAT_SPELLSAVE);
   CHECK(armor_stat_parse("dam") == ARMOR_STAT_DAMROLL && armor_stat_parse("ac") == ARMOR_STAT_ARMOR);
   CHECK(armor_stat_parse("wisdom") == -1);

   printf("armor_math_test: %d checks, %d failures\n", checks, failures);
   return failures ? 1 : 0;
   }
