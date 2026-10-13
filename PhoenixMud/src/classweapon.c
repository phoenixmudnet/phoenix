/***********************************************************************
*  File: classweapon.c                                                 *
*  Usage: the class weapon -- one weapon for life                      *
*                                                                      *
*  A first Hero is handed the class's weapon, bound. It grows by what  *
*  its owner does with it in hand: a melee hit, an attack spell, a     *
*  heal on someone hurt, under a cap each MUD hour, scaled by the      *
*  remort tier as the exp tables are. Level n needs 200 x n^1.5 growth *
*  points; each level buys share 1 - 0.92^n of the class bonus. A new  *
*  level may be LOCKED (a roll, at most a few in a hundred levels): it *
*  names a boss of the world, whose essence opens it. The owner kills  *
*  that boss to win the essence; a master crafter sets it (temper).    *
*                                                                      *
*  All of it sits behind class_weapons, off unless etc/class_weapons   *
*  says "enabled 1". Off, nothing here runs and the commands are out   *
*  of reach, so a server without the file plays as before. The lock    *
*  roll uses rand(), never number(): it draws nothing from the game's  *
*  own random stream.                                                  *
***********************************************************************/

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"
#include <stdint.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "hero_state.h"
#include "classweapon.h"

extern struct char_data *mob_proto;
extern struct index_data *obj_index;
extern const int exp_table[];
extern const float class_exp_multipliers[];
extern const float race_exp_multipliers[];
/* cw_math.c: the logarithm and its kin, apart from utils.h's log() */
double cw_exp(double x);
double cw_ln(double x);
double cw_pow(double x, double y);
double cw_floor(double x);
extern struct index_data *mob_index;
extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;
extern struct room_data *world;
extern struct time_info_data time_info;
extern const char *pc_class_types[];
extern struct command_info cmd_info[];
const char *mob_spec_name(SPECIAL(*f));
SPECIAL(shop_keeper);

int class_weapons = FALSE;

#define CLASS_WEAPON_BASE    26600
#define DEFAULT_ESSENCE      26620
#define POINTS_PER_HIT       2
#define POINTS_PER_CAST      25
#define POINTS_PER_HEAL      25
#define POINTS_PER_MUD_HOUR  100
#define LEVEL_COST_A         200.0
#define SHARE_R              0.08
#define HERO_WEAPON_BONUS    0.33
#define ANCHOR_BOOST         0.05
#define BOSS_BAND            10
#define BOSS_BAND_MIN        20

static long cw_first[NUM_CLASSES];
static long cw_essence = DEFAULT_ESSENCE;
static double cw_chance = 0.05;
static int cw_max_per_100 = 5;

/* the class bonus each class's weapon buys toward (the arena's gap to the
 * anchor, the Deva): out = deals more, in = takes less, heal = heals more */
#define KIND_OUT  0
#define KIND_IN   1
#define KIND_HEAL 2
static const struct { const char *knob; int kind; double s; } cw_raise[NUM_CLASSES] =
   {
   { "weapon mastery", KIND_OUT, 0.187 },   /* Warrior */
   { "heal", KIND_HEAL, 0.279 },            /* Cleric */
   { "evasion", KIND_IN, 0.544 },           /* Thief */
   { "arcane ward", KIND_IN, 1.091 },       /* Magic-User */
   { "hunter's mark", KIND_OUT, 0.384 },    /* Ranger */
   { "battle hymn", KIND_OUT, 0.571 },      /* Bard */
   { "cure critic", KIND_HEAL, 0.414 },     /* Monk */
   { NULL, 0, 0 },                          /* unused */
   { "rage", KIND_OUT, 0.177 },             /* Barbarian */
   { "cure serious", KIND_HEAL, 0.851 },    /* Paladin */
   { "unholy might", KIND_OUT, 0.259 },     /* Anti-Paladin */
   { "cure critic", KIND_HEAL, 0.578 },     /* Druid */
   { NULL, 0, 0 },                          /* Merchant, retired */
   { "blade work", KIND_OUT, 0.015 },       /* Kensai */
   { "blade work", KIND_OUT, 0.034 },       /* Assassin */
   { "bone armor", KIND_IN, 0.599 },        /* Necromancer */
   { "divine might", KIND_OUT, 0 },         /* Deva: the anchor */
   { NULL, 0, 0 },
   { NULL, 0, 0 }
   };

/* the boss pool: every mob a zone reset loads, that keeps no shop and runs
 * no service, with the room it is loaded into */
struct cw_boss { long vnum; int level; room_rnum room; };
static struct cw_boss *cw_pool = NULL;
static int cw_pool_size = 0;

static const char *cw_service_stems[] =
   {
   "shop_keeper", "guild", "receptionist", "cryogenicist", "postmaster",
   "bank", "stable", "repair", "healer", "sage", "home", "janitor", "fido",
   "cleaning", "mayor", "portal", "quest", "ferry", "captain", "coach",
   "innkeeper", "armorer", "weaponsmith", "jeweler", "battle_master", NULL
   };

/* the four who set an essence in a class weapon */
static const long cw_crafters[] = { 27498, 27497, 27496, 27499, 0 };

/* the class weapon commands, opened at boot when the switch is on */
static const char *cw_commands[] = { "temper", NULL };

/* ---------------------------------------------------------------- tables */

static void cw_defaults(void)
   {
   int c;

   for (c = 0; c < NUM_CLASSES; c++)
      cw_first[c] = (c == 7 || c == 12 || c >= 17) ? 0 : CLASS_WEAPON_BASE + c;
   cw_essence = DEFAULT_ESSENCE;
   cw_chance = 0.05;
   cw_max_per_100 = 5;
   }

/* etc/class_weapons:
 *   enabled 1
 *   first <class> <vnum>     (0 = none)
 *   essence <vnum>
 *   checkpoint <chance> <most in a hundred levels> */
static void cw_read_table(void)
   {
   FILE *fl;
   char line[256];
   int c, i;
   long v;
   double d;

   cw_defaults();
   class_weapons = FALSE;
   if (!(fl = fopen("etc/class_weapons", "r")))
      return;
   while (fgets(line, sizeof(line), fl))
      {
      if (sscanf(line, "enabled %d", &i) == 1)
         class_weapons = i ? TRUE : FALSE;
      else if (sscanf(line, "first %d %ld", &c, &v) == 2 && c >= 0 && c < NUM_CLASSES)
         cw_first[c] = v;
      else if (sscanf(line, "essence %ld", &v) == 1)
         cw_essence = v;
      else if (sscanf(line, "checkpoint %lf %d", &d, &i) == 2)
         {
         cw_chance = MAX(0.0, MIN(1.0, d));
         cw_max_per_100 = MAX(0, i);
         }
      }
   fclose(fl);
   }

static int cw_is_service(struct index_data *ix)
   {
   const char *name;
   char low[64];
   int i;

   if (!ix->func)
      return FALSE;
   if (ix->func == shop_keeper)
      return TRUE;
   if (!(name = mob_spec_name(ix->func)))
      return FALSE;
   for (i = 0; name[i] && i < 63; i++)
      low[i] = LOWER(name[i]);
   low[i] = '\0';
   for (i = 0; cw_service_stems[i]; i++)
      if (!strncmp(low, cw_service_stems[i], strlen(cw_service_stems[i])))
         return TRUE;
   return FALSE;
   }

static void cw_build_pool(void)
   {
   int zone, cmd_no, i, seen;
   mob_rnum m;

   if (cw_pool)
      free(cw_pool);
   cw_pool = NULL;
   cw_pool_size = 0;
   for (zone = 0; zone <= top_of_zone_table; zone++)
      for (cmd_no = 0; zone_table[zone].cmd && zone_table[zone].cmd[cmd_no].command != 'S'; cmd_no++)
         {
         struct reset_com *z = &zone_table[zone].cmd[cmd_no];

         if (z->command != 'M' || z->arg1 < 0 || z->arg3 < 0)
            continue;
         m = z->arg1;
         if (GET_LEVEL(&mob_proto[m]) <= 0 || GET_LEVEL(&mob_proto[m]) > 127)
            continue;
         if (cw_is_service(&mob_index[m]))
            continue;
         for (i = seen = 0; i < cw_pool_size; i++)
            if (cw_pool[i].vnum == mob_index[m].vnum)
               seen = 1;
         if (seen)
            continue;
         if (cw_pool)
            RECREATE(cw_pool, struct cw_boss, cw_pool_size + 1);
         else
            CREATE(cw_pool, struct cw_boss, 1);
         cw_pool[cw_pool_size].vnum = mob_index[m].vnum;
         cw_pool[cw_pool_size].level = GET_LEVEL(&mob_proto[m]);
         cw_pool[cw_pool_size].room = z->arg3;
         cw_pool_size++;
         }
   }

void classweapon_boot(void)
   {
   int i, j;

   cw_read_table();
   if (!class_weapons)
      return;
   cw_build_pool();
   for (i = 0; cw_commands[i]; i++)
      for (j = 0; *cmd_info[j].command != '\n'; j++)
         if (!strcmp(cmd_info[j].command, cw_commands[i]))
            cmd_info[j].minimum_level = 0;
   log("Class weapons: on (%d bosses in the pool).", cw_pool_size);
   }

/* ---------------------------------------------------------------- curves */

static long cw_mud_hour(void)
   {
   return (((long)time_info.year * 17 + time_info.month) * 35 + time_info.day) * 24
          + time_info.hours;
   }

static long cw_level_cost(int n)
   {
   return (long)cw_floor(LEVEL_COST_A * cw_pow((double)n, 1.5) + 0.5);
   }

static int cw_level_for(long points)
   {
   int n = 0;

   while (cw_level_cost(n + 1) <= points)
      n++;
   return n;
   }

static double cw_share(int level)
   {
   return 1.0 - cw_pow(1.0 - SHARE_R, level);
   }

static double cw_mult(int kind, double s)
   {
   return kind == KIND_IN ? cw_exp(-s) : cw_exp(s);
   }

static double cw_scale(int kind, double s, double f)
   {
   double m = cw_mult(kind, s);

   return kind == KIND_IN ? -cw_ln(1.0 - f * (1.0 - m)) : cw_ln(1.0 + f * (m - 1.0));
   }

/* the weapon's knob at a level's share: a third of the class's gap plus the
 * anchor's boost, in knob (log) space; FALSE for a class with none */
static int cw_knob(int cls, int level, int *kind, double *s)
   {
   double share = cw_share(level), gap = 0, boost;

   if (cls < 0 || cls >= NUM_CLASSES || !cw_raise[cls].knob || share <= 0)
      return FALSE;
   if (cw_raise[cls].s)
      gap = cw_scale(cw_raise[cls].kind, cw_raise[cls].s * share, HERO_WEAPON_BONUS);
   boost = cw_scale(KIND_OUT, cw_ln(1.0 + ANCHOR_BOOST / HERO_WEAPON_BONUS) * share,
                    HERO_WEAPON_BONUS);
   *kind = cw_raise[cls].kind;
   *s = gap + boost;
   return TRUE;
   }

/* a percentage to two places, trailing zeros dropped: 12.12, 51.5, 5 */
static void cw_pct(char *out, double p)
   {
   char *e;

   sprintf(out, "%.2f", p);
   if (strchr(out, '.'))
      {
      e = out + strlen(out) - 1;
      while (*e == '0')
         *e-- = '\0';
      if (*e == '.')
         *e = '\0';
      }
   if (!strcmp(out, "-0"))
      strcpy(out, "0");
   }

/* "weapon mastery, deals 11.03% more damage", or "none yet" */
static void cw_bonus_line(struct hero_data *h, char *out)
   {
   int kind;
   double s, m;
   char p[32];

   if (!cw_knob(h->cls, h->level, &kind, &s))
      {
      strcpy(out, "none yet");
      return;
      }
   m = cw_mult(kind, s);
   if (kind == KIND_IN)
      {
      cw_pct(p, (1.0 - m) * 100.0);
      sprintf(out, "%s, takes %s%% less damage", cw_raise[h->cls].knob, p);
      }
   else
      {
      cw_pct(p, (m - 1.0) * 100.0);
      sprintf(out, kind == KIND_OUT ? "%s, deals %s%% more damage" : "%s, heals %s%% more",
              cw_raise[h->cls].knob, p);
      }
   }

static long cw_points(struct obj_data *obj)
   {
   return obj->attune ? (long)cw_floor(obj->attune->points) + obj->attune->hits : 0;
   }

/* how much a growth point counts at the owner's remort tier (1 at a single
 * remort): the exp tables' own ratio for levels 101 and 102 */
static double cw_tier(struct char_data *ch)
   {
   double mine, single;

   mine = (double)GET_EXP_FOR_LEVEL(GET_RACE(ch), GET_CLASS(ch), 101, REMORT_LEVEL(ch))
        + (double)GET_EXP_FOR_LEVEL(GET_RACE(ch), GET_CLASS(ch), 102, REMORT_LEVEL(ch));
   single = (double)GET_EXP_FOR_LEVEL(GET_RACE(ch), GET_CLASS(ch), 101, 1)
          + (double)GET_EXP_FOR_LEVEL(GET_RACE(ch), GET_CLASS(ch), 102, 1);
   return mine > 0 && single > 0 ? single / mine : 1.0;
   }

/* ---------------------------------------------------------------- the copy */

static int cw_mine(struct obj_data *obj, struct char_data *ch)
   {
   return obj && obj->hero && !IS_NPC(ch) && obj->bound_idnum == GET_IDNUM(ch);
   }

/* the class weapon in its owner's hands: wielded, or held */
static struct obj_data *cw_in_hand(struct char_data *ch)
   {
   static const int slots[] = { WEAR_WIELD_1, WEAR_WIELD_2, WEAR_HOLD_1, WEAR_HOLD_2 };
   int i;

   if (IS_NPC(ch))
      return NULL;
   for (i = 0; i < 4; i++)
      if (cw_mine(GET_EQ(ch, slots[i]), ch))
         return GET_EQ(ch, slots[i]);
   return NULL;
   }

static struct obj_data *cw_held(struct char_data *ch)
   {
   if (cw_mine(GET_EQ(ch, WEAR_HOLD_1), ch))
      return GET_EQ(ch, WEAR_HOLD_1);
   if (cw_mine(GET_EQ(ch, WEAR_HOLD_2), ch))
      return GET_EQ(ch, WEAR_HOLD_2);
   return NULL;
   }

/* every class weapon the character carries or wears */
static struct obj_data *cw_owned(struct char_data *ch)
   {
   struct obj_data *o;
   int i;

   for (i = 0; i < NUM_WEARS; i++)
      if (cw_mine(GET_EQ(ch, i), ch))
         return GET_EQ(ch, i);
   for (o = ch->carrying; o; o = o->next_content)
      if (cw_mine(o, ch))
         return o;
   return NULL;
   }

/* ---------------------------------------------------------------- bosses */

static uint32_t cw_mix(long a, long b)
   {
   uint32_t h = 2166136261u, x;
   int i;

   for (i = 0; i < 2; i++)
      {
      x = (uint32_t)(i ? b : a);
      h ^= x;
      h *= 16777619u;
      h ^= (x >> 16);
      h *= 16777619u;
      }
   return h;
   }

static int cw_cmp_vnum(const void *a, const void *b)
   {
   long x = ((const struct cw_boss *)a)->vnum, y = ((const struct cw_boss *)b)->vnum;

   return x < y ? -1 : x > y ? 1 : 0;
   }

/* the boss for this character's lock at hold_level: a band around the
 * character's level, widened until it holds enough, sorted, and picked by
 * a hash of (character, lock) so a reboot names the same one */
static long cw_choose_boss(long idnum, int hold_level, int level)
   {
   struct cw_boss *band;
   int below = BOSS_BAND, above = BOSS_BAND, n = 0, i, want;
   long vnum;

   if (!cw_pool_size)
      return 0;
   CREATE(band, struct cw_boss, cw_pool_size);
   want = MIN(BOSS_BAND_MIN, cw_pool_size);
   for (;;)
      {
      n = 0;
      for (i = 0; i < cw_pool_size; i++)
         if (cw_pool[i].level >= level - below && cw_pool[i].level <= level + above)
            band[n++] = cw_pool[i];
      if (n >= want || (below >= 130 && above >= 130))
         break;
      below += 5;
      above += 5;
      }
   if (!n)
      {
      for (i = 0; i < cw_pool_size; i++)
         band[i] = cw_pool[i];
      n = cw_pool_size;
      }
   qsort(band, n, sizeof(struct cw_boss), cw_cmp_vnum);
   vnum = band[cw_mix(idnum, hold_level) % (uint32_t)n].vnum;
   free(band);
   return vnum;
   }

static struct cw_boss *cw_pool_entry(long vnum)
   {
   int i;

   for (i = 0; i < cw_pool_size; i++)
      if (cw_pool[i].vnum == vnum)
         return &cw_pool[i];
   return NULL;
   }

/* a lock with no boss named, or one gone from the world, names one */
static void cw_ensure_boss(struct char_data *ch, struct hero_data *h)
   {
   if (h->waiting == HERO_NOT_LOCKED)
      return;
   if (h->waiting > 0 && real_mobile(h->waiting) >= 0 && cw_pool_entry(h->waiting))
      return;
   h->waiting = cw_choose_boss(GET_IDNUM(ch), h->level + 1, GET_LEVEL(ch));
   }

/* "the essence of the frost giant jarl, found in Heliopolis" */
static void cw_boss_line(struct hero_data *h, char *out)
   {
   struct cw_boss *b;
   mob_rnum m;

   if (h->waiting <= 0 || (m = real_mobile(h->waiting)) < 0)
      {
      strcpy(out, "the essence of a boss");
      return;
      }
   b = cw_pool_entry(h->waiting);
   if (b && b->room >= 0 && world[b->room].zone >= 0 && zone_table[world[b->room].zone].name)
      sprintf(out, "the essence of %s, found in %s", mob_proto[m].player.short_descr,
              zone_table[world[b->room].zone].name);
   else
      sprintf(out, "the essence of %s", mob_proto[m].player.short_descr);
   }

/* ---------------------------------------------------------------- growth */

#define SETTLE_NONE  0
#define SETTLE_LEVEL 1
#define SETTLE_HELD  2

static int cw_lock_roll(void)
   {
   return (double)rand() / ((double)RAND_MAX + 1.0) < cw_chance;
   }

/* the growth as it stands, through the lock rule */
static int cw_settle(struct char_data *ch, struct obj_data *obj)
   {
   struct hero_data *h = obj->hero;
   int target, next, recent, i, result = SETTLE_NONE;

   if (!h || h->waiting != HERO_NOT_LOCKED)
      return SETTLE_NONE;
   target = cw_level_for(cw_points(obj));
   while (h->level < target)
      {
      next = h->level + 1;
      for (i = recent = 0; i < h->num_holds; i++)
         if (h->holds[i] > next - 100)
            recent++;
      if (cw_chance > 0 && recent < cw_max_per_100 && cw_lock_roll())
         {
         h->waiting = HERO_NO_BOSS;
         if (h->num_holds < HERO_MAX_HOLDS)
            h->holds[h->num_holds++] = next;
         cw_ensure_boss(ch, h);
         return SETTLE_HELD;
         }
      h->level = next;
      result = SETTLE_LEVEL;
      }
   return result;
   }

static void cw_grow(struct char_data *ch, struct obj_data *obj, int base)
   {
   struct obj_attune_data *a;
   struct hero_data *h = obj->hero;
   long hour = cw_mud_hour();
   int take, r;
   char line[256];

   if (!obj->attune)
      {
      obj->attune = attune_create();
      obj->attune->hour = hour;
      }
   a = obj->attune;
   if (a->hour != hour)
      {
      a->hour = hour;
      a->hour_hits = 0;
      a->hour_points = 0;
      }
   take = MIN(base, MAX(0, POINTS_PER_MUD_HOUR - (int)a->hour_points));
   if (take <= 0)
      return;
   a->hour_points += take;
   a->points += take * cw_tier(ch);
   if (h->focus != HERO_FOCUS_NONE)
      h->focus_points[h->focus] += take * cw_tier(ch);
   if (h->waiting != HERO_NOT_LOCKED)
      cw_ensure_boss(ch, h);
   r = cw_settle(ch, obj);
   if (r == SETTLE_LEVEL)
      {
      cw_bonus_line(h, line);
      send_to_char(ch, "&Y%s reaches level %d. Class bonus: %s.&n\r\n",
                   obj->short_description, h->level, line);
      }
   else if (r == SETTLE_HELD)
      {
      cw_boss_line(h, line);
      send_to_char(ch, "&Y%s is locked at level %d: it needs %s. Go there and slay it "
                   "to win its essence.&n\r\n", obj->short_description, h->level, line);
      }
   }

void cw_note_hit(struct char_data *ch, struct obj_data *wielded)
   {
   struct obj_data *obj;

   if (!class_weapons || IS_NPC(ch))
      return;
   obj = (wielded && cw_mine(wielded, ch)) ? wielded : cw_held(ch);
   if (obj)
      cw_grow(ch, obj, POINTS_PER_HIT);
   }

void cw_note_cast(struct char_data *ch)
   {
   struct obj_data *obj;

   if (!class_weapons || IS_NPC(ch) || !(obj = cw_in_hand(ch)))
      return;
   cw_grow(ch, obj, POINTS_PER_CAST);
   }

int cw_heal_mod(struct char_data *ch, struct char_data *victim, int amount)
   {
   struct obj_data *obj;
   int kind;
   double a = amount, s;

   if (!class_weapons || IS_NPC(ch) || amount <= 0 || !(obj = cw_in_hand(ch)))
      return amount;
   if (victim && GET_HIT(victim) < GET_MAX_HIT(victim))
      cw_grow(ch, obj, POINTS_PER_HEAL);
   if (cw_knob(obj->hero->cls, obj->hero->level, &kind, &s) && kind == KIND_HEAL)
      a *= cw_mult(KIND_HEAL, s);
   return (int)cw_floor(a);
   }

int cw_damage_mod(struct char_data *ch, struct char_data *victim, int dam,
                  int melee_or_skill)
   {
   struct obj_data *obj;
   double mul = 1.0, s;
   int kind;

   if (!class_weapons || !melee_or_skill || dam <= 0)
      return dam;
   if (ch && (obj = cw_in_hand(ch)) &&
       cw_knob(obj->hero->cls, obj->hero->level, &kind, &s) && kind == KIND_OUT)
      mul *= cw_mult(KIND_OUT, s);
   if (victim && (obj = cw_in_hand(victim)) &&
       cw_knob(obj->hero->cls, obj->hero->level, &kind, &s) && kind == KIND_IN)
      mul *= cw_mult(KIND_IN, s);
   if (mul == 1.0)
      return dam;
   return (int)cw_floor(dam * mul + 0.5);
   }

/* ---------------------------------------------------------------- the hand */

static int cw_held_kind(struct obj_data *obj)
   {
   return CAN_WEAR(obj, ITEM_WEAR_HOLD) && !CAN_WEAR(obj, ITEM_WEAR_WIELD);
   }

void cw_give_first(struct char_data *ch)
   {
   struct obj_data *obj;
   long vnum;

   if (!class_weapons || IS_NPC(ch) || GET_CLASS(ch) < 0 || GET_CLASS(ch) >= NUM_CLASSES)
      return;
   vnum = cw_first[(int)GET_CLASS(ch)];
   if (vnum <= 0 || real_object(vnum) < 0 || cw_owned(ch))
      return;
   if (!(obj = read_object(vnum, VIRTUAL)))
      return;
   obj->bound_idnum = GET_IDNUM(ch);
   strncpy(obj->bound_name, GET_NAME(ch), MAX_NAME_LENGTH);
   obj->bound_name[MAX_NAME_LENGTH] = '\0';
   obj->attune = attune_create();
   obj->attune->hour = cw_mud_hour();
   obj->hero = hero_create();
   obj->hero->cls = GET_CLASS(ch);
   obj->hero->core = vnum;
   obj_to_char(obj, ch);
   if (cw_held_kind(obj))
      send_to_char(ch, "You receive %s, your class weapon, bound to you. Hold it: it gains "
                   "levels as you fight while holding it, and takes the form of anything you "
                   "hold that you infuse into it at a smith.\r\n", obj->short_description);
   else
      send_to_char(ch, "You receive %s, your class weapon, bound to you. It gains levels as "
                   "you fight with it, and takes the form of any weapon you infuse into it at "
                   "a smith.\r\n", obj->short_description);
   }

/* one weapon for life, through remorts: its bonus is the class the
 * character is, and its own form is that class's weapon. The character is
 * naked to remort, so it is carried, not worn. */
void cw_reclass(struct char_data *ch)
   {
   struct obj_data *o;
   long next;

   if (!class_weapons || IS_NPC(ch))
      return;
   for (o = ch->carrying; o; o = o->next_content)
      {
      if (!cw_mine(o, ch) || o->hero->cls == GET_CLASS(ch))
         continue;
      o->hero->cls = GET_CLASS(ch);
      next = cw_first[(int)GET_CLASS(ch)];
      if (next > 0 && real_object(next) >= 0)
         o->hero->core = next;
      }
   }

/* at login: the class's own weapon, carried and bound to its owner but
 * without its state (a hand-made copy), is started at level 0 */
void cw_login(struct char_data *ch)
   {
   struct obj_data *o;
   long first;

   if (!class_weapons || IS_NPC(ch))
      return;
   first = cw_first[(int)GET_CLASS(ch)];
   for (o = ch->carrying; o; o = o->next_content)
      if (!o->hero && first > 0 && GET_OBJ_VNUM(o) == first &&
          o->bound_idnum == GET_IDNUM(ch))
         {
         o->hero = hero_create();
         o->hero->cls = GET_CLASS(ch);
         o->hero->core = first;
         if (!o->attune)
            {
            o->attune = attune_create();
            o->attune->hour = cw_mud_hour();
            }
         }
   cw_reclass(ch);
   }

/* ---------------------------------------------------------------- essences */

static struct obj_data *cw_carried_essence(struct char_data *ch, long boss, int marked)
   {
   struct obj_data *o;

   if (cw_essence <= 0)
      return NULL;
   for (o = ch->carrying; o; o = o->next_content)
      {
      if (GET_OBJ_VNUM(o) != cw_essence)
         continue;
      if (!marked && !o->checkpoint_of)
         return o;
      if (marked && o->checkpoint_of && (boss == 0 || o->checkpoint_of == boss))
         return o;
      }
   return NULL;
   }

/* the locked owner slays the boss its weapon names: the essence, marked and
 * bound, one at a time */
void cw_boss_killed(struct char_data *killer, struct char_data *victim)
   {
   struct obj_data *w, *ess;
   long vnum;

   if (!class_weapons || !killer || !victim || IS_NPC(killer) || !IS_NPC(victim))
      return;
   vnum = GET_MOB_VNUM(victim);
   if (!(w = cw_owned(killer)) || w->hero->waiting != vnum || vnum <= 0)
      return;
   if (cw_carried_essence(killer, vnum, TRUE))
      return;
   if (real_object(cw_essence) < 0 || !(ess = read_object(cw_essence, VIRTUAL)))
      return;
   ess->bound_idnum = GET_IDNUM(killer);
   strncpy(ess->bound_name, GET_NAME(killer), MAX_NAME_LENGTH);
   ess->bound_name[MAX_NAME_LENGTH] = '\0';
   ess->checkpoint_of = vnum;
   obj_to_char(ess, killer);
   send_to_char(killer, "&YWhere %s stood lies %s: the essence your weapon needs. It is yours "
                "alone; a master crafter sets it in your weapon (temper).&n\r\n",
                victim->player.short_descr, ess->short_description);
   }

static struct char_data *cw_crafter_here(struct char_data *ch)
   {
   struct char_data *k;
   int i;

   for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
      {
      if (!IS_NPC(k) || FIGHTING(k) || !AWAKE(k))
         continue;
      for (i = 0; cw_crafters[i]; i++)
         if (GET_MOB_VNUM(k) == cw_crafters[i])
            return k;
      }
   return NULL;
   }

/* temper <weapon>: before a master crafter, an essence set in a LOCKED class
 * weapon opens the lock; the boss's own essence first, or a plain one kept
 * from before. An essence never raises the weapon. */
ACMD(do_temper)
   {
   struct obj_data *obj, *ess;
   struct char_data *crafter;
   struct hero_data *h;
   char *arg = get_buffer(MAX_INPUT_LENGTH), *line = get_buffer(512);
   int r;

   one_argument(argument, arg);
   if (!class_weapons)
      send_to_char(ch, "Huh?!?\r\n");
   else if (!*arg)
      send_to_char(ch, "Temper what? Your class weapon, carried, before a master crafter.\r\n");
   else if (!(crafter = cw_crafter_here(ch)))
      send_to_char(ch, "There is no master crafter here. Only the four master crafters set "
                   "an essence in a class weapon.\r\n");
   else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
      send_to_char(ch, "You are not carrying %s %s; take your class weapon out of your "
                   "hands first.\r\n", AN(arg), arg);
   else if (!cw_mine(obj, ch))
      send_to_char(ch, "%s is not your class weapon to temper.\r\n", obj->short_description);
   else if (cw_essence <= 0)
      send_to_char(ch, "No essence is set out for the class weapons yet.\r\n");
   else if ((h = obj->hero)->waiting == HERO_NOT_LOCKED)
      send_to_char(ch, "%s is not locked. An essence only unlocks a locked level; keep it "
                   "until then.\r\n", obj->short_description);
   else
      {
      cw_ensure_boss(ch, h);
      if (!(ess = cw_carried_essence(ch, h->waiting, TRUE)))
         ess = cw_carried_essence(ch, 0, FALSE);
      if (!ess)
         {
         cw_boss_line(h, line);
         send_to_char(ch, "%s is locked: it needs %s.\r\n", obj->short_description, line);
         }
      else
         {
         obj_from_char(ess);
         extract_obj(ess);
         h->waiting = HERO_NOT_LOCKED;
         h->cleared++;
         h->essences++;
         r = cw_settle(ch, obj);
         strncpy(line, crafter->player.short_descr ? crafter->player.short_descr : "The crafter", 255);
         line[255] = '\0';
         line[0] = UPPER(line[0]);
         send_to_char(ch, "&Y%s sets the essence in your weapon. ", line);
         if (r == SETTLE_HELD)
            {
            cw_boss_line(h, line);
            send_to_char(ch, "%s is unlocked, and locked again at level %d: it needs %s "
                         "next.&n\r\n", obj->short_description, h->level, line);
            }
         else
            {
            cw_bonus_line(h, line);
            send_to_char(ch, "%s is unlocked: level %d. Class bonus: %s.&n\r\n",
                         obj->short_description, h->level, line);
            }
         }
      }
   release_buffer(arg);
   release_buffer(line);
   }

/* ---------------------------------------------------------------- identify */

static void cw_commas(char *out, long n)
   {
   char raw[32];
   int len, i, j = 0;

   sprintf(raw, "%ld", n);
   len = strlen(raw);
   for (i = 0; i < len; i++)
      {
      out[j++] = raw[i];
      if (raw[i] != '-' && (len - i - 1) > 0 && (len - i - 1) % 3 == 0)
         out[j++] = ',';
      }
   out[j] = '\0';
   }

void cw_identify_lines(struct obj_data *obj, char *buf)
   {
   struct hero_data *h = obj->hero;
   char line[256], a[32], b[32];

   if (obj->bound_idnum > 0 && obj->bound_name[0])
      sprintf(buf + strlen(buf), "Bound to %s, and no other.\r\n", obj->bound_name);
   if (!h)
      return;
   cw_bonus_line(h, line);
   cw_commas(a, cw_points(obj));
   cw_commas(b, cw_level_cost(h->level + 1));
   sprintf(buf + strlen(buf), "Class weapon (%s): level %d. Class bonus: %s. %s of %s points "
           "to level %d.\r\n",
           (h->cls >= 0 && h->cls < NUM_CLASSES) ? pc_class_types[h->cls] : "hero",
           h->level, line, a, b, h->level + 1);
   if (h->waiting != HERO_NOT_LOCKED)
      {
      cw_boss_line(h, line);
      sprintf(buf + strlen(buf), "Locked: it needs %s. Go there and slay it to win its "
              "essence.\r\n", line);
      }
   }

int cw_bound_ok(struct char_data *ch, struct obj_data *obj, int message)
   {
   if (!obj || obj->bound_idnum <= 0 || IS_NPC(ch) || GET_IDNUM(ch) == obj->bound_idnum ||
       GET_LEVEL(ch) >= LVL_IMMORT)
      return TRUE;
   if (message)
      send_to_char(ch, "%s is bound to %s; it will not serve you.\r\n",
                   obj->short_description, obj->bound_name);
   return FALSE;
   }
