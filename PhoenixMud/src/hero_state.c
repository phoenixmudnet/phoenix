/***********************************************************************
*  File: hero_state.c                                                  *
*  Usage: the class weapon's per-copy state and an object's binding,   *
*         and the H lines a rent record carries them in                *
*                                                                      *
*  Every line is short (under 128 characters), starts with H and is    *
*  skipped by a binary that does not know it:                          *
*                                                                      *
*    HB <idnum> <name>            bound to one character               *
*    HC <boss vnum>               the essence a boss marked            *
*    HT <points> <hits> <hour> <hour hits> <hour points>  attunement   *
*    HW <class> <core> <level> <essences> <cleared> <waiting> <form>   *
*       <focus>                   the class weapon                     *
*    HF <vnum> <charge> <refits> [<sitting>]  a form it knows; sitting *
*       is when it was lent from a !rent piece (0 or none: for good)  *
*    HH <level>                   a level a lock held it at            *
*    HP <dps> <heal> <tank> <cast>  each focus's own points            *
*    HG <key>                     a grant applied to it                *
*    HS <focus>                   the second focus (the class set)     *
*    HA <stat> <essences> <over>  a unit attuned on class armor        *
*    HE <count>                   an armor essence copy's count        *
*                                                                      *
*  Self-contained (no game globals) so the tests link it alone.        *
***********************************************************************/

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"
#include "structs.h"
#include "utils.h"
#include "hero_state.h"

static void *hero_calloc(size_t n, size_t size)
   {
   void *p = calloc(n, size);

   if (!p)
      {
      perror("malloc failure");
      abort();
      }
   return p;
   }

struct hero_data *hero_create(void)
   {
   struct hero_data *h = hero_calloc(1, sizeof(struct hero_data));

   h->waiting = HERO_NOT_LOCKED;
   h->form = HERO_CORE_FORM;
   h->focus = HERO_FOCUS_NONE;
   return h;
   }

struct obj_attune_data *attune_create(void)
   {
   return hero_calloc(1, sizeof(struct obj_attune_data));
   }

struct obj_armor_data *armor_create(void)
   {
   return hero_calloc(1, sizeof(struct obj_armor_data));
   }

static const char *armor_stat_words[ARMOR_NUM_STATS] =
   { "hp", "mana", "str", "dex", "hitroll", "damroll", "spellsave", "breathsave", "armor" };

const char *armor_stat_word(int stat)
   {
   return (stat >= 0 && stat < ARMOR_NUM_STATS) ? armor_stat_words[stat] : "?";
   }

int armor_stat_of(const char *word)
   {
   int i;

   for (i = 0; word && i < ARMOR_NUM_STATS; i++)
      if (!strcmp(word, armor_stat_words[i]))
         return i;
   return -1;
   }

void obj_hero_free(struct obj_data *obj)
   {
   if (!obj)
      return;
   if (obj->hero)
      free(obj->hero);
   if (obj->attune)
      free(obj->attune);
   if (obj->armor)
      free(obj->armor);
   obj->hero = NULL;
   obj->attune = NULL;
   obj->armor = NULL;
   obj->bound_idnum = 0;
   obj->bound_name[0] = '\0';
   obj->checkpoint_of = 0;
   }

void obj_hero_copy(struct obj_data *dst, const struct obj_data *src)
   {
   if (!dst || !src || dst == src)
      return;
   obj_hero_free(dst);
   if (src->hero)
      {
      dst->hero = hero_calloc(1, sizeof(struct hero_data));
      *dst->hero = *src->hero;
      }
   if (src->attune)
      {
      dst->attune = hero_calloc(1, sizeof(struct obj_attune_data));
      *dst->attune = *src->attune;
      }
   if (src->armor)
      {
      dst->armor = hero_calloc(1, sizeof(struct obj_armor_data));
      *dst->armor = *src->armor;
      }
   dst->bound_idnum = src->bound_idnum;
   strncpy(dst->bound_name, src->bound_name, MAX_NAME_LENGTH);
   dst->bound_name[MAX_NAME_LENGTH] = '\0';
   dst->checkpoint_of = src->checkpoint_of;
   }

int obj_hero_write(FILE *fp, const struct obj_data *obj)
   {
   const struct hero_data *h = obj->hero;
   const struct obj_attune_data *a = obj->attune;
   const struct obj_armor_data *r = obj->armor;
   int i, err = 0;

   if (obj->bound_idnum > 0)
      if (fprintf(fp, "HB %ld %s\n", obj->bound_idnum,
                  obj->bound_name[0] ? obj->bound_name : "-") < 0)
         err = -1;
   if (obj->checkpoint_of > 0)
      if (fprintf(fp, "HC %ld\n", (long)obj->checkpoint_of) < 0)
         err = -1;
   if (a)
      {
      if (a->banked > 0)
         {
         if (fprintf(fp, "HT %.4f %ld %ld %ld %ld %.4f\n", a->points, a->hits,
                     a->hour, a->hour_hits, a->hour_points, a->banked) < 0)
            err = -1;
         }
      else if (fprintf(fp, "HT %.4f %ld %ld %ld %ld\n", a->points, a->hits,
                       a->hour, a->hour_hits, a->hour_points) < 0)
         err = -1;
      }
   if (r)
      {
      for (i = 0; i < r->num && i < ARMOR_MAX_POINTS; i++)
         if (fprintf(fp, "HA %s %d %d\n", armor_stat_word(r->pts[i].stat), r->pts[i].essences,
                     r->pts[i].over ? 1 : 0) < 0)
            err = -1;
      if (r->essences > 0)
         if (fprintf(fp, "HE %ld\n", r->essences) < 0)
            err = -1;
      }
   if (!h)
      return err;
   if (fprintf(fp, "HW %d %ld %d %d %d %ld %ld %d\n", h->cls, h->core,
               h->level, h->essences, h->cleared, h->waiting, h->form,
               h->focus) < 0)
      err = -1;
   for (i = 0; i < h->num_forms && i < HERO_MAX_FORMS; i++)
      if ((h->forms[i].sitting > 0
           ? fprintf(fp, "HF %ld %ld %d %ld\n", h->forms[i].vnum, h->forms[i].charge,
                     h->forms[i].refits, h->forms[i].sitting)
           : fprintf(fp, "HF %ld %ld %d\n", h->forms[i].vnum, h->forms[i].charge,
                     h->forms[i].refits)) < 0)
         err = -1;
   for (i = 0; i < h->num_holds && i < HERO_MAX_HOLDS; i++)
      if (fprintf(fp, "HH %d\n", h->holds[i]) < 0)
         err = -1;
   if (h->focus_points[HERO_FOCUS_DPS] > 0 ||
       h->focus_points[HERO_FOCUS_HEAL] > 0 ||
       h->focus_points[HERO_FOCUS_TANK] > 0 ||
       h->focus_points[HERO_FOCUS_CAST] > 0)
      if (fprintf(fp, "HP %.4f %.4f %.4f %.4f\n",
                  h->focus_points[HERO_FOCUS_DPS],
                  h->focus_points[HERO_FOCUS_HEAL],
                  h->focus_points[HERO_FOCUS_TANK],
                  h->focus_points[HERO_FOCUS_CAST]) < 0)
         err = -1;
   for (i = 0; i < h->num_grants && i < HERO_MAX_GRANTS; i++)
      if (fprintf(fp, "HG %s\n", h->grants[i]) < 0)
         err = -1;
   if (h->focus2 > HERO_FOCUS_NONE && h->focus2 < HERO_NUM_FOCUS)
      if (fprintf(fp, "HS %d\n", h->focus2) < 0)
         err = -1;
   return err;
   }

static struct hero_data *hero_of(struct obj_data *obj)
   {
   if (!obj->hero)
      obj->hero = hero_create();
   return obj->hero;
   }

static struct obj_armor_data *armor_of(struct obj_data *obj)
   {
   if (!obj->armor)
      obj->armor = armor_create();
   return obj->armor;
   }

int obj_hero_read_line(struct obj_data *obj, const char *line)
   {
   struct hero_data *h;
   long l1, l2, l3, l4;
   int i1, i2, i3, i4, i5, i6;
   double d1, d2, d3, d4;
   char word[HERO_GRANT_LEN + 1];

   if (!obj || !line || line[0] != 'H' || !line[1] || line[2] != ' ')
      return FALSE;
   switch (line[1])
      {
      case 'B':
         if (sscanf(line, "HB %ld %20s", &l1, word) == 2 && l1 > 0)
            {
            obj->bound_idnum = l1;
            if (strcmp(word, "-"))
               {
               strncpy(obj->bound_name, word, MAX_NAME_LENGTH);
               obj->bound_name[MAX_NAME_LENGTH] = '\0';
               }
            }
         return TRUE;
      case 'C':
         if (sscanf(line, "HC %ld", &l1) == 1 && l1 > 0)
            obj->checkpoint_of = l1;
         return TRUE;
      case 'T':
         d2 = 0;
         if (sscanf(line, "HT %lf %ld %ld %ld %ld %lf", &d1, &l1, &l2, &l3, &l4, &d2) >= 5)
            {
            if (!obj->attune)
               obj->attune = attune_create();
            obj->attune->points = d1;
            obj->attune->hits = l1;
            obj->attune->hour = l2;
            obj->attune->hour_hits = l3;
            obj->attune->hour_points = l4;
            obj->attune->banked = d2 > 0 ? d2 : 0;
            }
         return TRUE;
      case 'W':
         if (sscanf(line, "HW %d %ld %d %d %d %ld %ld %d", &i1, &l1, &i2, &i3,
                    &i4, &l2, &l3, &i5) == 8)
            {
            h = hero_of(obj);
            h->cls = i1;
            h->core = l1;
            h->level = i2;
            h->essences = i3;
            h->cleared = i4;
            h->waiting = l2;
            h->form = l3;
            h->focus = (i5 >= HERO_FOCUS_NONE && i5 < HERO_NUM_FOCUS)
                       ? i5 : HERO_FOCUS_NONE;
            }
         return TRUE;
      case 'F':
         l3 = 0;
         if (sscanf(line, "HF %ld %ld %d %ld", &l1, &l2, &i6, &l3) >= 3)
            {
            h = hero_of(obj);
            if (h->num_forms < HERO_MAX_FORMS)
               {
               h->forms[h->num_forms].vnum = l1;
               h->forms[h->num_forms].charge = l2 < 0 ? 0 : l2;
               h->forms[h->num_forms].refits = i6 < 0 ? HERO_REFITS_UNSET : i6;
               h->forms[h->num_forms].sitting = l3 > 0 ? l3 : 0;
               h->num_forms++;
               }
            }
         return TRUE;
      case 'H':
         if (sscanf(line, "HH %d", &i1) == 1)
            {
            h = hero_of(obj);
            if (h->num_holds < HERO_MAX_HOLDS)
               h->holds[h->num_holds++] = i1;
            }
         return TRUE;
      case 'P':
         if (sscanf(line, "HP %lf %lf %lf %lf", &d1, &d2, &d3, &d4) == 4)
            {
            h = hero_of(obj);
            h->focus_points[HERO_FOCUS_DPS] = d1;
            h->focus_points[HERO_FOCUS_HEAL] = d2;
            h->focus_points[HERO_FOCUS_TANK] = d3;
            h->focus_points[HERO_FOCUS_CAST] = d4;
            }
         return TRUE;
      case 'S':
         if (sscanf(line, "HS %d", &i1) == 1 && i1 > HERO_FOCUS_NONE && i1 < HERO_NUM_FOCUS)
            hero_of(obj)->focus2 = i1;
         return TRUE;
      case 'A':
         if (sscanf(line, "HA %20s %d %d", word, &i1, &i2) == 3 && armor_stat_of(word) >= 0 &&
             i1 >= 0)
            {
            struct obj_armor_data *r = armor_of(obj);

            if (r->num < ARMOR_MAX_POINTS)
               {
               r->pts[r->num].stat = armor_stat_of(word);
               r->pts[r->num].essences = i1;
               r->pts[r->num].over = i2 ? 1 : 0;
               r->num++;
               }
            }
         return TRUE;
      case 'E':
         if (sscanf(line, "HE %ld", &l1) == 1 && l1 > 0)
            armor_of(obj)->essences = l1;
         return TRUE;
      case 'G':
         if (sscanf(line, "HG %47s", word) == 1)
            {
            h = hero_of(obj);
            if (h->num_grants < HERO_MAX_GRANTS)
               {
               strncpy(h->grants[h->num_grants], word, HERO_GRANT_LEN - 1);
               h->grants[h->num_grants][HERO_GRANT_LEN - 1] = '\0';
               h->num_grants++;
               }
            }
         return TRUE;
      }
   return TRUE;   /* an H line of a later version: skipped */
   }
