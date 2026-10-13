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
