/***********************************************************************
*  File: size_fit.c                                                    *
*  Usage: a height or weight brought inside a race's range, from the   *
*         character's idnum (fit_size_to_race, db.c)                   *
***********************************************************************/

#include "size_fit.h"

long size_fit(long current, long lo, long hi, unsigned int id, unsigned int salt)
   {
   if (hi < lo || (current >= lo && current <= hi))
      return current;
   return lo + (long) ((id * salt) % (unsigned int) (hi - lo + 1));
   }
