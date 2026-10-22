/***********************************************************************
*  File: help_match.h                                                  *
*  Usage: does a word name a help entry exactly (find_help,            *
*         act.informative.c)                                           *
***********************************************************************/

#ifndef HELP_MATCH_H
#define HELP_MATCH_H

/* 1 if str, less any trailing spaces, is one of the names on namelist,
 * case aside.  A name is a word, or a quoted phrase taken whole
 * ("MAGIC MISSILE").  With first_only set, only the first name counts. */
int is_help_exact(const char *str, const char *namelist, int first_only);

#endif
