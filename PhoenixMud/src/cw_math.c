/***********************************************************************
*  File: cw_math.c                                                     *
*  Usage: the class weapon's curves (classweapon.c), kept apart from   *
*         utils.h: its log() is the game's logger, not the logarithm.  *
*         The game's global log() also shadows libm's at link time, so *
*         the logarithm is taken through logl().                       *
***********************************************************************/

#include <math.h>

double cw_exp(double x) { return exp(x); }
double cw_ln(double x) { return (double)logl((long double)x); }
double cw_pow(double x, double y) { return pow(x, y); }
double cw_floor(double x) { return floor(x); }

/* what a cast is worth to a class weapon: base at `full` mana or more, and
 * its share below, rounded down; nothing for a cast that cost nothing */
int cw_cast_points(int base, int mana, int full)
{
   if (full <= 0 || mana <= 0 || base <= 0)
      return 0;
   if (mana > full)
      mana = full;
   return base * mana / full;
}
