/***********************************************************************
*  File: repair_life.h                                                 *
*  Usage: a weapon's whole life at the smiths, as repair_guy runs it,  *
*         for the class weapon's forms (classweapon.c)                 *
***********************************************************************/

#ifndef REPAIR_LIFE_H
#define REPAIR_LIFE_H

/* where a weapon stands when it is handed over for one of its repairs */
struct repair_service
   {
   int orig;     /* its original slots then (a reforge takes a tenth off) */
   int max;      /* its total slots */
   int cur;      /* its current slots */
   int master;   /* only a master does this one: a reforge */
   };

int repair_life(int points, int per_wear, int no_repair, long blows_per_wear,
                int k, long *charge, struct repair_service *at);

#endif
