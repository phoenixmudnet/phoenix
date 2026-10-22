/***********************************************************************
*  File: size_fit.h                                                    *
*  Usage: a height or weight brought inside a race's range, from the   *
*         character's idnum (fit_size_to_race, db.c)                   *
***********************************************************************/

#ifndef SIZE_FIT_H
#define SIZE_FIT_H

/* the multipliers that spread idnums over a range, one per measure */
#define SIZE_SALT_HEIGHT 2654435761u
#define SIZE_SALT_WEIGHT 2246822519u

/* current if it lies in lo..hi, otherwise a value in lo..hi from id;
 * draws no RNG */
long size_fit(long current, long lo, long hi, unsigned int id, unsigned int salt);

#endif
