/***********************************************************************
*  File: repair_life.c                                                 *
*  Usage: a weapon's whole life at the smiths, as repair_guy runs it   *
*         (spec_procs.c), for the class weapon's forms (classweapon.c) *
***********************************************************************/

#include "repair_life.h"

/* The weapon is repaired whenever the next wear would leave it at half or
 * below. A repairman repairs it while its total slots are above two, the
 * total a slot lower (two when below half). Then only a master can: a
 * reforge takes a tenth of its original slots off and makes it whole. A
 * no-repair weapon goes to a master every time. The life ends when a
 * reforge would leave nothing.
 *
 * Returns the repairs and reforges in all. *charge (when given) is the
 * blows in the charge after k of them: each stretch between reforges is cut
 * into equal charges, one a repair and one before the first. *at (when
 * given, and k is less than the total) is where the weapon stands when it
 * is handed over for the k-th, counting from 0. */
int repair_life(int points, int per_wear, int no_repair, long blows_per_wear,
                int k, long *charge, struct repair_service *at)
   {
   int step = points / 10 > 0 ? points / 10 : 1;
   int orig, max, cur, shop, n = 0;
   long wears;

   for (orig = points; orig >= 1; orig -= step)
      {
      max = cur = orig;
      wears = 0;
      shop = 0;
      for (;;)
         {
         if (cur - per_wear >= 0 && (double)(cur - per_wear) / max > 0.5)
            {
            cur -= per_wear;
            wears++;
            continue;
            }
         /* too small to wear above half: wear, then repair below it */
         if (cur == max)
            {
            cur -= per_wear;
            wears++;
            }
         if (no_repair || max <= 2)
            break;
         if (at && n + shop == k)
            {
            at->orig = orig;
            at->max = max;
            at->cur = cur > 0 ? cur : 0;
            at->master = 0;
            }
         shop++;
         max -= ((double)cur / max > 0.5) ? 1 : 2;
         cur = max;
         }
      if (charge && k >= n && k <= n + shop)
         {
         *charge = (long)((double)wears * blows_per_wear / (shop + 1) + 0.5);
         if (*charge < 1)
            *charge = 1;
         }
      n += shop;
      if (orig - step >= 1)
         {
         if (at && n == k)
            {
            at->orig = orig;
            at->max = max;
            at->cur = cur > 0 ? cur : 0;
            at->master = 1;
            }
         n++;
         }
      }
   return n;
   }
