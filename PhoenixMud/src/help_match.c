/***********************************************************************
*  File: help_match.c                                                  *
*  Usage: does a word name a help entry exactly (find_help,            *
*         act.informative.c)                                           *
***********************************************************************/

#include <ctype.h>
#include <string.h>
#include "help_match.h"

int is_help_exact(const char *str, const char *namelist, int first_only)
   {
   size_t len = strlen(str);
   const char *p = namelist;

   while (len > 0 && isspace((unsigned char) str[len - 1]))
      len--;
   if (len == 0)
      return 0;

   for (;;)
      {
      const char *start, *end;
      size_t i;

      while (*p == ' ')
         p++;
      if (!*p)
         return 0;
      if (*p == '"')
         {
         start = ++p;
         while (*p && *p != '"')
            p++;
         end = p;
         if (*p)
            p++;
         }
      else
         {
         start = p;
         while (*p && *p != ' ')
            p++;
         end = p;
         }
      if ((size_t) (end - start) == len)
         {
         for (i = 0; i < len && tolower((unsigned char) start[i]) == tolower((unsigned char) str[i]); i++)
            ;
         if (i == len)
            return 1;
         }
      if (first_only)
         return 0;
      }
   }
