/***********************************************************************
*  File: armor_math.c                                                  *
*  Usage: the class armor's arithmetic and tables (armor_math.h); the  *
*         same numbers, curves and rules as TS's classArmor.ts.        *
*                                                                      *
*  The weight drives a piece: head and legs 2 parts, the body 3, feet, *
*  hands and arms 1, of 10. It sets the piece's part of the class      *
*  bonus, how rarely it drops, its attunement limit and how far past.  *
***********************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "armor_math.h"

#define HERO_WEAPON_BONUS 0.33

static const char *slot_names[ARMOR_NUM_SLOTS] = { "head", "body", "legs", "feet", "hands", "arms" };
static const int slot_parts[ARMOR_NUM_SLOTS] = { 2, 3, 2, 1, 1, 1 };
/* the classes with a set, in vnum order */
static const int armor_classes[ARMOR_NUM_CLASSES] = { 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 13, 14, 15, 16 };

const char *armor_slot_name(int slot)
   {
   return (slot >= 0 && slot < ARMOR_NUM_SLOTS) ? slot_names[slot] : "piece";
   }

int armor_slot_parts(int slot)
   {
   return (slot >= 0 && slot < ARMOR_NUM_SLOTS) ? slot_parts[slot] : 0;
   }

double armor_work_per_essence(void)
   {
   return ARMOR_WORK_TOTAL / (ATTUNE_POINTS_PER_PART * ARMOR_PARTS_TOTAL * ESSENCES_PER_POINT);
   }

int armor_class_index(int cls)
   {
   int i;

   for (i = 0; i < ARMOR_NUM_CLASSES; i++)
      if (armor_classes[i] == cls)
         return i;
   return -1;
   }

long armor_vnum(int cls, int slot)
   {
   int i = armor_class_index(cls);

   if (i < 0 || slot < 0 || slot >= ARMOR_NUM_SLOTS)
      return -1;
   return ARMOR_BASE_VNUM + ARMOR_NUM_SLOTS * i + slot;
   }

int armor_piece_of(long vnum, int *cls, int *slot)
   {
   long n = vnum - ARMOR_BASE_VNUM;

   if (n < 0 || n >= ARMOR_NUM_SLOTS * ARMOR_NUM_CLASSES)
      return 0;
   if (cls)
      *cls = armor_classes[n / ARMOR_NUM_SLOTS];
   if (slot)
      *slot = (int)(n % ARMOR_NUM_SLOTS);
   return 1;
   }

int armor_got_count(int got)
   {
   int s, n = 0;

   for (s = 0; s < ARMOR_NUM_SLOTS; s++)
      if (got & (1 << s))
         n++;
   return n;
   }

/* ---------------------------------------------------------------- the drop */

void armor_state_fresh(struct class_armor_state *st, int cls)
   {
   memset(st, 0, sizeof(*st));
   st->cls = cls;
   st->next = -1;
   }

/* the repeat part of what the next kill of vnum pays: in full for a type's
 * first two kills, then KILL_UNDER_MF (fight.c), 3% less a kill, to a floor
 * of one in ten thousand; a type pushed off by 64 others pays in full again */
double class_kills_repeat(const struct class_kills *k, long vnum)
   {
   int i, n = 0, pct;

   for (i = 0; i < k->n; i++)
      if (k->vnum[i] == vnum)
         {
         n = k->count[i];
         break;
         }
   if (n <= 1)
      return 1.0;
   if (n > 101)
      n = 101;
   pct = (int)((float)((float)(101 - (n * 3 > 101 ? 101 : n * 3)) / (float)100.0) * (float)10000);
   return (pct < 1 ? 1 : pct) / 10000.0;
   }

/* a kill of vnum goes on the buffer: to the front, counted; a new type
 * pushes the oldest off */
void class_kills_note(struct class_kills *k, long vnum)
   {
   int i, at = -1, count = 0;

   for (i = 0; i < k->n; i++)
      if (k->vnum[i] == vnum)
         {
         at = i;
         break;
         }
   if (at >= 0)
      count = k->count[at];
   else if (k->n < CLASS_KILLS_SIZE)
      at = k->n++;
   else
      at = CLASS_KILLS_SIZE - 1;
   for (i = at; i > 0; i--)
      {
      k->vnum[i] = k->vnum[i - 1];
      k->count[i] = k->count[i - 1];
      }
   k->vnum[0] = vnum;
   k->count[0] = count < 255 ? count + 1 : 255;
   }

/* the armor's class bonus by the wearer's level: the gap was measured in a
 * level-100 duel, so a first life or a remort climbing back gets none of it
 * at 50 or below and more each level to the whole at 100; the pieces' armor,
 * base hitroll and damroll and attuned stats are whole at every level */
double armor_level_scale(int level)
   {
   double x = (double)(level - ARMOR_BONUS_FROM_LEVEL) / (ARMOR_BONUS_FULL_LEVEL - ARMOR_BONUS_FROM_LEVEL);

   return x < 0 ? 0 : x > 1 ? 1 : x;
   }

/* the next piece among those not had: the lighter, the likelier (1/parts) */
int armor_pick_next(int got, double r)
   {
   double total = 0, x;
   int s, last = -1;

   for (s = 0; s < ARMOR_NUM_SLOTS; s++)
      if (!(got & (1 << s)))
         total += 1.0 / slot_parts[s];
   if (total <= 0)
      return -1;
   x = r * total;
   for (s = 0; s < ARMOR_NUM_SLOTS; s++)
      if (!(got & (1 << s)))
         {
         last = s;
         x -= 1.0 / slot_parts[s];
         if (x < 0)
            return s;
         }
   return last;
   }

/* the work a piece waits for: its parts of the total, spread */
long armor_mark_for(int slot, double r)
   {
   double mean = ARMOR_WORK_TOTAL * slot_parts[slot] / ARMOR_PARTS_TOTAL;

   return (long)floor(mean * (1 - ARMOR_MARK_SPREAD + 2 * ARMOR_MARK_SPREAD * r) + 0.5);
   }

/* a kill's work in; the slot that drops when the mark is reached, else -1 */
int armor_add_work(struct class_armor_state *st, double work, double (*rand01)(void))
   {
   int slot;

   if (st->next < 0)
      {
      st->next = armor_pick_next(st->got, rand01());
      if (st->next < 0)
         return -1;
      st->mark = armor_mark_for(st->next, rand01());
      }
   if (!(work > 0))
      return -1;
   st->work += work;
   if (st->work < st->mark)
      return -1;
   slot = st->next;
   st->got |= 1 << slot;
   st->work = 0;
   st->next = armor_pick_next(st->got, rand01());
   st->mark = st->next >= 0 ? armor_mark_for(st->next, rand01()) : 0;
   return slot;
   }

/* the essences a kill's work brings: whole ones, and a chance at the rest */
int armor_essences_for(double work, double r)
   {
   double e;
   int n;

   if (!(work > 0))
      return 0;
   e = work / armor_work_per_essence();
   n = (int)floor(e);
   return n + (r < e - n ? 1 : 0);
   }

/* ---------------------------------------------------------------- the bonus */

double armor_slot_fraction(int slot)
   {
   return (1.0 - HERO_WEAPON_BONUS) * armor_slot_parts(slot) / ARMOR_PARTS_TOTAL;
   }

static double bonus_of(int in_kind, double s)
   {
   return in_kind ? 1.0 - exp(-s) : exp(s) - 1.0;
   }

double armor_factor(int in_kind, double s, double weapon_share, double fraction)
   {
   double weapon, armor;

   if (!(s > 0) || !(fraction > 0))
      return 1.0;
   if (weapon_share < 0)
      weapon_share = 0;
   if (weapon_share > 1)
      weapon_share = 1;
   weapon = HERO_WEAPON_BONUS * bonus_of(in_kind, s * weapon_share);
   armor = fraction * bonus_of(in_kind, s);
   return in_kind ? (1 - weapon - armor) / (1 - weapon) : (1 + weapon + armor) / (1 + weapon);
   }

double armor_set_focus_mult(int count)
   {
   return count >= SET_FOCUS_AT ? SET_FOCUS_MULT : 1.0;
   }

/* ---------------------------------------------------------------- attunement */

static const char *stat_labels[ARMOR_NUM_STATS] =
   { "hit points", "mana", "strength", "dexterity", "hitroll", "damroll", "save vs spell",
     "save vs breath", "armor class" };
/* what a unit gives, and what it costs of the limit */
static const int stat_per[ARMOR_NUM_STATS] = { 10, 10, 1, 1, 1, 1, -1, -1, -3 };
static const int stat_points[ARMOR_NUM_STATS] = { 1, 1, 2, 1, 1, 1, 1, 1, 1 };
/* the stats each slot carries on what heroes wear; every piece also takes
 * hitroll and damroll, and any class takes any of them */
static const int slot_stats[ARMOR_NUM_SLOTS][6] =
   {
   { ARMOR_STAT_HP, ARMOR_STAT_MANA, ARMOR_STAT_BREATHSAVE, ARMOR_STAT_SPELLSAVE, -1 },
   { ARMOR_STAT_HP, ARMOR_STAT_STR, ARMOR_STAT_DEX, ARMOR_STAT_SPELLSAVE, ARMOR_STAT_ARMOR, -1 },
   { ARMOR_STAT_DEX, ARMOR_STAT_MANA, ARMOR_STAT_SPELLSAVE, ARMOR_STAT_HP, ARMOR_STAT_STR, -1 },
   { ARMOR_STAT_DEX, ARMOR_STAT_STR, ARMOR_STAT_HP, ARMOR_STAT_MANA, ARMOR_STAT_ARMOR, -1 },
   { ARMOR_STAT_STR, ARMOR_STAT_MANA, ARMOR_STAT_DEX, ARMOR_STAT_HP, -1 },
   { ARMOR_STAT_DEX, ARMOR_STAT_MANA, ARMOR_STAT_STR, ARMOR_STAT_SPELLSAVE, -1 },
   };

const char *armor_stat_label(int stat)
   {
   return (stat >= 0 && stat < ARMOR_NUM_STATS) ? stat_labels[stat] : "?";
   }

int armor_stat_per(int stat)
   {
   return (stat >= 0 && stat < ARMOR_NUM_STATS) ? stat_per[stat] : 0;
   }

int armor_stat_points(int stat)
   {
   return (stat >= 0 && stat < ARMOR_NUM_STATS) ? stat_points[stat] : 1;
   }

/* a typed stat name: "hp", "hit points", "str", "save spell" */
int armor_stat_parse(const char *typed)
   {
   static const struct { int stat; const char *w; } words[] =
      {
      { ARMOR_STAT_HP, "hp" }, { ARMOR_STAT_HP, "hitpoints" }, { ARMOR_STAT_HP, "hits" },
      { ARMOR_STAT_HP, "health" }, { ARMOR_STAT_MANA, "mana" }, { ARMOR_STAT_MANA, "mp" },
      { ARMOR_STAT_STR, "str" }, { ARMOR_STAT_STR, "strength" },
      { ARMOR_STAT_DEX, "dex" }, { ARMOR_STAT_DEX, "dexterity" },
      { ARMOR_STAT_HITROLL, "hitroll" }, { ARMOR_STAT_HITROLL, "hr" }, { ARMOR_STAT_HITROLL, "hit" },
      { ARMOR_STAT_DAMROLL, "damroll" }, { ARMOR_STAT_DAMROLL, "dr" }, { ARMOR_STAT_DAMROLL, "dam" },
      { ARMOR_STAT_DAMROLL, "damage" },
      { ARMOR_STAT_SPELLSAVE, "spellsave" }, { ARMOR_STAT_SPELLSAVE, "savespell" },
      { ARMOR_STAT_SPELLSAVE, "svspell" }, { ARMOR_STAT_SPELLSAVE, "spell" },
      { ARMOR_STAT_SPELLSAVE, "save" }, { ARMOR_STAT_SPELLSAVE, "savevsspell" },
      { ARMOR_STAT_BREATHSAVE, "breathsave" }, { ARMOR_STAT_BREATHSAVE, "savebreath" },
      { ARMOR_STAT_BREATHSAVE, "svbreath" }, { ARMOR_STAT_BREATHSAVE, "breath" },
      { ARMOR_STAT_BREATHSAVE, "savevsbreath" },
      { ARMOR_STAT_ARMOR, "armor" }, { ARMOR_STAT_ARMOR, "armour" }, { ARMOR_STAT_ARMOR, "ac" },
      { ARMOR_STAT_ARMOR, "armorclass" },
      { -1, NULL }
      };
   char w[64];
   int i, n = 0;

   for (i = 0; typed && typed[i] && n < 63; i++)
      if (isalpha((unsigned char)typed[i]))
         w[n++] = (char)tolower((unsigned char)typed[i]);
   w[n] = '\0';
   if (!n)
      return -1;
   for (i = 0; words[i].w; i++)
      if (!strcmp(w, words[i].w))
         return words[i].stat;
   if (n >= 2)
      for (i = 0; words[i].w; i++)
         if (strlen(words[i].w) >= 3 && !strncmp(words[i].w, w, n))
            return words[i].stat;
   return -1;
   }

int armor_limit(int slot)
   {
   return ATTUNE_POINTS_PER_PART * armor_slot_parts(slot);
   }

int armor_over_limit(int slot)
   {
   return ATTUNE_OVER_PER_PART * armor_slot_parts(slot);
   }

/* a stat's cap on a piece within the limit: half its points (body 3, head
 * and legs 2, the rest 1), strength one */
int armor_max_units(int stat, int slot)
   {
   int m = armor_limit(slot) / 2 / armor_stat_points(stat);

   if (stat == ARMOR_STAT_STR && m > 1)
      m = 1;
   return m;
   }

/* a piece's pool: its slot's stats that fit under its cap, then hitroll and
 * damroll; returns how many */
int armor_pool(int slot, int out[ARMOR_NUM_STATS])
   {
   int i, n = 0;

   if (slot < 0 || slot >= ARMOR_NUM_SLOTS)
      return 0;
   for (i = 0; i < 6 && slot_stats[slot][i] >= 0; i++)
      if (armor_max_units(slot_stats[slot][i], slot) > 0)
         out[n++] = slot_stats[slot][i];
   out[n++] = ARMOR_STAT_HITROLL;
   out[n++] = ARMOR_STAT_DAMROLL;
   return n;
   }

int armor_points_used(const struct obj_armor_data *a)
   {
   int i, n = 0;

   for (i = 0; a && i < a->num; i++)
      if (!a->pts[i].over)
         n += armor_stat_points(a->pts[i].stat);
   return n;
   }

int armor_points_over(const struct obj_armor_data *a)
   {
   int i, n = 0;

   for (i = 0; a && i < a->num; i++)
      if (a->pts[i].over)
         n++;
   return n;
   }

/* the k-th point past the limit: its essences, and its chance to break */
long armor_over_cost(int k)
   {
   return (long)(ESSENCES_PER_POINT * pow(2.0, k));
   }

double armor_over_break(int k)
   {
   double b = OVER_BREAK_STEP * k;

   return b > 1 ? 1 : b;
   }

/* may one more unit of stat go on the piece, and at what price */
int armor_check(int slot, const struct obj_armor_data *a, int stat, int *cost, double *brk,
                int *over)
   {
   int pool[ARMOR_NUM_STATS], n = armor_pool(slot, pool), i, in = 0, have = 0, off = 0, k;
   int lim = armor_limit(slot), used = armor_points_used(a);

   for (i = 0; i < n; i++)
      if (pool[i] == stat)
         in = 1;
   if (!in)
      return ATTUNE_NOT_IN_POOL;
   *over = 0;
   *brk = 0;
   if (used + armor_stat_points(stat) <= lim)
      {
      for (i = 0; a && i < a->num; i++)
         if (!a->pts[i].over)
            {
            if (a->pts[i].stat == stat)
               have++;
            if (a->pts[i].stat == ARMOR_STAT_HITROLL || a->pts[i].stat == ARMOR_STAT_DAMROLL)
               off++;
            }
      if (have >= armor_max_units(stat, slot))
         return ATTUNE_STAT_FULL;
      if ((stat == ARMOR_STAT_HITROLL || stat == ARMOR_STAT_DAMROLL) && 2 * (off + 1) > lim)
         return ATTUNE_OFFENSE_FULL;
      *cost = ESSENCES_PER_POINT * armor_stat_points(stat);
      return ATTUNE_OK;
      }
   if (used < lim)
      return ATTUNE_NO_ROOM;
   k = armor_points_over(a) + 1;
   if (k > armor_over_limit(slot))
      return ATTUNE_OVER_FULL;
   if (armor_stat_points(stat) > 1)
      return ATTUNE_NO_OVER;
   *over = 1;
   *cost = (int)armor_over_cost(k);
   *brk = armor_over_break(k);
   return ATTUNE_OK;
   }

/* what the attunement adds, a total a stat */
void armor_totals(const struct obj_armor_data *a, int totals[ARMOR_NUM_STATS])
   {
   int i;

   for (i = 0; i < ARMOR_NUM_STATS; i++)
      totals[i] = 0;
   for (i = 0; a && i < a->num; i++)
      if (a->pts[i].stat >= 0 && a->pts[i].stat < ARMOR_NUM_STATS)
         totals[a->pts[i].stat] += stat_per[a->pts[i].stat];
   }

/* taking a stat's points out (stat < 0: all of them): the essences handed
 * back and the gold asked */
void armor_unattune_terms(const struct obj_armor_data *a, int stat, int *essences,
                          long *gold, int *points)
   {
   long e = 0;
   int i, p = 0;

   for (i = 0; a && i < a->num; i++)
      if (stat < 0 || a->pts[i].stat == stat)
         {
         e += a->pts[i].essences;
         p += a->pts[i].over ? 1 : armor_stat_points(a->pts[i].stat);
         }
   *essences = (int)floor(e * UNATTUNE_KEEP);
   *gold = (long)UNATTUNE_GOLD_PER_POINT * p;
   *points = p;
   }

/* as room opens under the limit, the oldest points past it come within,
 * each only if it could have gone on within the limit (no stat past its
 * cap on the piece, hitroll and damroll one cap): moved any other way, an
 * unattune would launder points past the limit over the caps and start
 * their price and break chance again */
void armor_settle_over(struct obj_armor_data *a, int slot)
   {
   struct obj_armor_data rest;
   int i, j, cost, over;
   double brk;

   for (i = 0; a && i < a->num; i++)
      {
      if (!a->pts[i].over)
         continue;
      rest = *a;
      for (j = i; j + 1 < rest.num; j++)
         rest.pts[j] = rest.pts[j + 1];
      rest.num--;
      if (armor_check(slot, &rest, a->pts[i].stat, &cost, &brk, &over) == ATTUNE_OK && !over)
         a->pts[i].over = 0;
      }
   }
