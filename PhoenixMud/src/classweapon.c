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
#include "repair_life.h"
#include "classweapon.h"

extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct str_app_type str_app[];
int untrained_weapon_prof(struct char_data *ch, struct obj_data *obj);
int get_prof(struct char_data *ch, struct obj_data *wielded);
char *skill_name(int num);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int invalid_race(struct char_data *ch, struct obj_data *obj);
int invalid_align(struct char_data *ch, struct obj_data *obj);
int can_wear_lr(struct char_data *ch, struct obj_data *obj, int message);
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
#define FOCUS_DPS            0.25
#define FOCUS_HEAL           0.25
#define FOCUS_HOT_HP         12
#define FOCUS_HOT_MANA       12
#define FOCUS_HOT_TICKS      3
#define BLOWS_PER_WEAR       600
#define INDESTRUCTIBLE_PTS   175
#define INDESTRUCTIBLE_WEAR  2
#define MASTER_REFORGE       250000
/* the form's combat flags a class weapon takes on: glow, hum, magic, bless,
 * the three anti-alignments, two-handed, brittle, resistant */
#define CW_FORM_FLAGS ((1 << 0) | (1 << 1) | (1 << 6) | (1 << 8) | (1 << 9) | (1 << 10) | \
                       (1 << 11) | (1 << 16) | (1 << 20) | (1 << 21))
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
static const char *cw_commands[] = { "temper", "focus", "infuse", "shift", "refit", "claim", NULL };
/* the staff's, opened to immortals */
static const char *cw_staff_commands[] = { "classweapon", "remortreward", NULL };

/* the rewards ledger (etc/remort_rewards): the pieces set out at the last
 * mortal level for each class, and what each character has had */
#define CLAIM_LEVEL       104
#define CLAIM_KEEPER      27499
#define MAX_104_PIECES    8
struct cw_grant { long idnum; long weapon_at; long claim104; long claimed_at; struct cw_grant *next; };
static struct cw_grant *cw_grants = NULL;
static long cw_104[NUM_CLASSES][MAX_104_PIECES];
static int cw_104_n[NUM_CLASSES];

/* the focus: one at a time, each with its own growth points */
static const char *cw_focus_names[HERO_NUM_FOCUS] = { NULL, "dps", "heal", "tank", "cast" };
static const char *cw_focus_words[HERO_NUM_FOCUS] =
   { NULL, "strikes harder", "heals stronger", "mends you with each blow",
     "feeds your mana with each blow" };

/* the over-time pools a tank or cast focus fills: by character */
struct cw_pool { long idnum; int hp, mana, ticks; struct cw_pool *next; };
static struct cw_pool *cw_pools = NULL;
SPECIAL(repair_guy);
int get_repair_cost(struct obj_data *obj, struct char_data *keeper, struct char_data *ch);
extern struct descriptor_data *descriptor_list;

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

static void cw_read_ledger(void)
   {
   FILE *fl;
   char line[512], *p;
   struct cw_grant *g;
   long v, a, b, c, d;
   int cls, n;

   memset(cw_104_n, 0, sizeof(cw_104_n));
   while (cw_grants)
      {
      g = cw_grants->next;
      free(cw_grants);
      cw_grants = g;
      }
   if (!(fl = fopen("etc/remort_rewards", "r")))
      return;
   while (fgets(line, sizeof(line), fl))
      if (sscanf(line, "reward104 %d%n", &cls, &n) == 1 && cls >= 0 && cls < NUM_CLASSES)
         for (p = line + n; cw_104_n[cls] < MAX_104_PIECES && sscanf(p, " %ld%n", &v, &n) == 1; p += n)
            cw_104[cls][cw_104_n[cls]++] = v;
      else if (sscanf(line, "granted %ld %ld %ld %ld %ld", &a, &b, &c, &d, &v) >= 4)
         {
         CREATE(g, struct cw_grant, 1);
         g->idnum = a;
         g->weapon_at = b;
         g->claim104 = c;
         g->claimed_at = d;
         g->next = cw_grants;
         cw_grants = g;
         }
   fclose(fl);
   }

static void cw_write_ledger(void)
   {
   FILE *fl;
   struct cw_grant *g;
   int c, i;

   if (!(fl = fopen("etc/remort_rewards.tmp", "w")))
      return;
   for (c = 0; c < NUM_CLASSES; c++)
      if (cw_104_n[c])
         {
         fprintf(fl, "reward104 %d", c);
         for (i = 0; i < cw_104_n[c]; i++)
            fprintf(fl, " %ld", cw_104[c][i]);
         fprintf(fl, "\n");
         }
   for (g = cw_grants; g; g = g->next)
      fprintf(fl, "granted %ld %ld %ld %ld\n", g->idnum, g->weapon_at, g->claim104, g->claimed_at);
   fclose(fl);
   rename("etc/remort_rewards.tmp", "etc/remort_rewards");
   }

static struct cw_grant *cw_grant_of(long idnum, int create)
   {
   struct cw_grant *g;

   for (g = cw_grants; g; g = g->next)
      if (g->idnum == idnum)
         return g;
   if (!create)
      return NULL;
   CREATE(g, struct cw_grant, 1);
   g->idnum = idnum;
   g->next = cw_grants;
   cw_grants = g;
   return g;
   }

void classweapon_boot(void)
   {
   int i, j;

   cw_read_table();
   if (!class_weapons)
      return;
   cw_build_pool();
   cw_read_ledger();
   for (i = 0; cw_commands[i]; i++)
      for (j = 0; *cmd_info[j].command != '\n'; j++)
         if (!strcmp(cmd_info[j].command, cw_commands[i]))
            cmd_info[j].minimum_level = 0;
   for (i = 0; cw_staff_commands[i]; i++)
      for (j = 0; *cmd_info[j].command != '\n'; j++)
         if (!strcmp(cmd_info[j].command, cw_staff_commands[i]))
            cmd_info[j].minimum_level = LVL_IMMORT;
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

static int cw_level_for(double points)
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

/* the share of the full focus the one in use has reached: its own points,
 * on the weapon's level curve; 0 with none chosen */
static double cw_focus_share(struct hero_data *h)
   {
   if (h->focus <= HERO_FOCUS_NONE || h->focus >= HERO_NUM_FOCUS)
      return 0;
   return cw_share(cw_level_for(h->focus_points[h->focus]));
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

/* ---------------------------------------------------------------- forms */

static int cw_held_kind(struct obj_data *obj);

/* a form's weapon at the smiths: its slots, what a wear takes, and
 * whether a repairman may touch it; an indestructible one lives as the most
 * durable weapon in the world does */
static void cw_weapon_wear(long vnum, int *points, int *per, int *no_repair)
   {
   obj_rnum r = real_object(vnum);

   *points = r >= 0 ? GET_OBJ_OSLOTS(&obj_proto[r]) : 0;
   *per = 4;
   *no_repair = r >= 0 && IS_SET(GET_OBJ_EXTRA(&obj_proto[r]), ITEM_NO_REPAIR);
   if (r >= 0 && IS_SET(GET_OBJ_EXTRA(&obj_proto[r]), ITEM_RESISTANT))
      *per = 2;
   else if (r >= 0 && IS_SET(GET_OBJ_EXTRA(&obj_proto[r]), ITEM_BRITTLE))
      *per = 8;
   if (*points <= 0)
      {
      *points = INDESTRUCTIBLE_PTS;
      *per = INDESTRUCTIBLE_WEAR;
      *no_repair = FALSE;
      }
   }

/* a form's life is its weapon's whole life at the smiths (repair_life.c):
 * the repairs and reforges in all; *charge, the blows in the charge after k
 * of them; *at, where the weapon would stand at the k-th */
static int cw_form_life(long vnum, int k, long *charge, struct repair_service *at)
   {
   int points, per, no_repair;

   cw_weapon_wear(vnum, &points, &per, &no_repair);
   return repair_life(points, per, no_repair, BLOWS_PER_WEAR, k, charge, at);
   }

/* the blows in a new form's charge */
static long cw_form_charge(long vnum)
   {
   long charge = 1;

   cw_form_life(vnum, 0, &charge, NULL);
   return charge;
   }

/* the blows in this form's full charge, where it stands in its life */
static long cw_full_charge(struct hero_form *f)
   {
   long charge = 1;

   cw_form_life(f->vnum, MAX(0, f->refits), &charge, NULL);
   return charge;
   }

static int cw_refits_left(struct hero_form *f)
   {
   return MAX(0, cw_form_life(f->vnum, 0, NULL, NULL) - MAX(0, f->refits));
   }

/* the repair or reforge the form's next refit stands for: FALSE when none
 * is left */
static int cw_next_refit(struct hero_form *f, struct repair_service *at)
   {
   return MAX(0, f->refits) < cw_form_life(f->vnum, MAX(0, f->refits), NULL, at);
   }

/* what a spent form needs, for the messages */
static const char *cw_spent_hint(struct hero_form *f)
   {
   struct repair_service at;

   if (!f || !cw_next_refit(f, &at))
      return "it is worn out; infuse another of that weapon to make it new";
   return at.master ? "a master smith must reforge it to use it again"
                    : "refit it at a smith's station to use it again";
   }

/* the gold for a form's next refit: what keeper charges for the repair or
 * reforge it stands for (get_repair_cost, on a copy of the weapon as it
 * would stand then) */
static int cw_refit_price(struct char_data *ch, struct char_data *keeper,
                          struct hero_form *f, struct repair_service *at)
   {
   obj_rnum r = real_object(f->vnum);
   struct obj_data *o;
   int cost, reductions;

   if (r >= 0 && GET_OBJ_OSLOTS(&obj_proto[r]) > 0)
      {
      o = read_object(f->vnum, VIRTUAL);
      GET_OBJ_OSLOTS(o) = at->orig;
      GET_OBJ_TSLOTS(o) = at->max;
      GET_OBJ_CSLOTS(o) = at->cur;
      cost = get_repair_cost(o, keeper, ch);
      extract_obj(o);
      return cost;
      }
   /* an indestructible weapon, priced as the weapon it lives as */
   reductions = (int)(((float)INDESTRUCTIBLE_PTS - at->orig) / ((float)INDESTRUCTIBLE_PTS * 0.1));
   if (at->master)
      cost = (int)(MASTER_REFORGE * (1 + 0.5 * reductions));
   else
      cost = (reductions + 1) * at->orig * (at->max - at->cur);
   return price_adjust(ch, keeper, cost);
   }

/* the copy takes a base: the stats and traits of t, the class weapon's own
 * name and descriptions kept */
static void cw_apply_base(struct obj_data *obj, struct obj_data *t, struct obj_data *core)
   {
   int i;

   GET_OBJ_TYPE(obj) = GET_OBJ_TYPE(t);
   for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
      GET_OBJ_VAL(obj, i) = GET_OBJ_VAL(t, i);
   GET_OBJ_WEIGHT(obj) = GET_OBJ_WEIGHT(t);
   GET_OBJ_COST(obj) = GET_OBJ_COST(t);
   for (i = 0; i < MAX_OBJ_AFFECT; i++)
      obj->affected[i] = t->affected[i];
   for (i = 0; i < MAX_SPELL_AFFECT; i++)
      obj->spell_affect[i] = t->spell_affect[i];
   GET_OBJ_EXTRA(obj) = GET_OBJ_EXTRA(core) | (t == core ? 0 : (GET_OBJ_EXTRA(t) & CW_FORM_FLAGS));
   GET_OBJ_ANTI(obj) = GET_OBJ_ANTI(t);
   obj->obj_flags.bitvector = t->obj_flags.bitvector;
   }

/* rebuild the copy from its state: the form it is in (or its own), and the
 * wearer's applies off and on again around it */
static void cw_refresh(struct obj_data *obj)
   {
   struct hero_data *h = obj->hero;
   struct char_data *wearer = obj->worn_by;
   int pos = obj->worn_on;
   obj_rnum cr, fr;

   if (!h || (cr = real_object(h->core)) < 0)
      return;
   fr = h->form != HERO_CORE_FORM ? real_object(h->form) : -1;
   if (h->form != HERO_CORE_FORM && fr < 0)
      h->form = HERO_CORE_FORM;   /* a form gone from the world: its own */
   if (wearer)
      unequip_char(wearer, pos);
   cw_apply_base(obj, fr >= 0 ? &obj_proto[fr] : &obj_proto[cr], &obj_proto[cr]);
   if (wearer)
      equip_char(wearer, obj, pos);
   }

/* a loaded copy (rent, house, corpse) takes the form it was saved in */
void cw_loaded(struct obj_data *obj)
   {
   if (obj && obj->hero && obj->hero->form != HERO_CORE_FORM && !obj->worn_by)
      cw_refresh(obj);
   }

/* could the character use this as the class weapon's form: wield it (or,
 * for a held class weapon, hold it), as they stand; NULL when so */
static const char *cw_form_refusal(struct char_data *ch, struct hero_data *h,
                                   struct obj_data *o, char *why)
   {
   obj_rnum cr = real_object(h->core);
   int held = cr >= 0 && cw_held_kind(&obj_proto[cr]);
   int t = GET_OBJ_VAL(o, 3);

   if (held)
      {
      if (!CAN_WEAR(o, ITEM_WEAR_HOLD) || !(GET_OBJ_TYPE(o) == ITEM_ARMOR ||
          GET_OBJ_TYPE(o) == ITEM_WORN || GET_OBJ_TYPE(o) == ITEM_TREASURE ||
          GET_OBJ_TYPE(o) == ITEM_OTHER))
         sprintf(why, "%s is not something your class weapon can take the form of: it takes "
                 "things held, never weapons, lights, wands, staves or containers.",
                 o->short_description);
      else if (GET_LEVEL(ch) < LVL_IMMORT &&   /* as equip_char lets an immortal */
               (invalid_class(ch, o) || invalid_race(ch, o) || invalid_align(ch, o)))
         sprintf(why, "%s is not for your kind to hold.", o->short_description);
      else if (!can_wear_lr(ch, o, FALSE))
         sprintf(why, "You are not experienced enough to use %s.", o->short_description);
      else
         return NULL;
      return why;
      }
   if (GET_OBJ_TYPE(o) != ITEM_WEAPON && GET_OBJ_TYPE(o) != ITEM_FIREWEAPON)
      sprintf(why, "%s is no weapon.", o->short_description);
   else if (GET_OBJ_WEIGHT(o) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
      sprintf(why, "%s is too heavy for you to use.", o->short_description);
   else if ((IS_CLERIC(ch) || IS_DRUID(ch)) && (t == 2 || t == 3 || t == 6 || t == 7 ||
            t == 8 || t == 9 || t == 10 || t == 13 || t == 15))
      strcpy(why, "The Gods that grant your powers forbid you to cut flesh!!");
   else if (IS_MAGIC_USER(ch) && (t == 3 || t == 10 || t == 13 || t == 15))
      strcpy(why, "That weapon would interfere with your magic!");
   else if (untrained_weapon_prof(ch, o))
      sprintf(why, "You have no training with %s weapons.", skill_name(get_prof(ch, o)));
   else if (GET_LEVEL(ch) < LVL_IMMORT &&   /* as equip_char lets an immortal */
               (invalid_class(ch, o) || invalid_race(ch, o) || invalid_align(ch, o)))
      sprintf(why, "%s is not for your kind to wield.", o->short_description);
   else if (!can_wear_lr(ch, o, FALSE))
      sprintf(why, "You are not experienced enough to use %s.", o->short_description);
   else
      return NULL;
   return why;
   }

/* "Forms: core, the Holy Avenger 80% (95 refits), a dwarven hammer spent (0 refits)."
 * With a buyer and the repairman he deals with, each form's next price. */
static void cw_form_list(struct obj_data *obj, char *out, struct char_data *buyer,
                         struct char_data *keeper)
   {
   struct hero_data *h = obj->hero;
   struct repair_service at;
   obj_rnum r;
   long full;
   int i;

   if (!h->num_forms)
      {
      r = real_object(h->core);
      strcpy(out, r >= 0 && cw_held_kind(&obj_proto[r])
             ? "It knows no forms yet; infuse something you hold at a smith."
             : "It knows no forms yet; infuse a weapon at a smith.");
      return;
      }
   strcpy(out, "Forms: core");
   for (i = 0; i < h->num_forms; i++)
      {
      r = real_object(h->forms[i].vnum);
      full = cw_full_charge(&h->forms[i]);
      sprintf(out + strlen(out), ", %s ", r >= 0 ? obj_proto[r].short_description : "something lost");
      if (h->forms[i].charge > 0)
         sprintf(out + strlen(out), "%d%%",
                 (int)MIN(100, cw_floor(100.0 * h->forms[i].charge / full + 0.5)));
      else
         strcat(out, "spent");
      if (buyer && keeper && cw_next_refit(&h->forms[i], &at))
         sprintf(out + strlen(out), " (%d refits, the next %d gold%s)", cw_refits_left(&h->forms[i]),
                 cw_refit_price(buyer, keeper, &h->forms[i], &at), at.master ? " at a master smith" : "");
      else
         sprintf(out + strlen(out), " (%d refits)", cw_refits_left(&h->forms[i]));
      }
   strcat(out, ".");
   }

/* a form the weapon knows, by a word of that weapon's name */
static struct hero_form *cw_find_form(struct hero_data *h, char *arg)
   {
   obj_rnum r;
   int i;

   for (i = 0; i < h->num_forms; i++)
      if ((r = real_object(h->forms[i].vnum)) >= 0 &&
          (isname(arg, obj_proto[r].name) || strstr(obj_proto[r].short_description, arg)))
         return &h->forms[i];
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
   if (!obj)
      return;
   /* a tank or cast focus: each blow refreshes the over-time pool */
   if (obj->hero->focus == HERO_FOCUS_TANK || obj->hero->focus == HERO_FOCUS_CAST)
      {
      struct cw_pool *p;
      int per = MAX(1, (int)cw_floor((obj->hero->focus == HERO_FOCUS_TANK ? FOCUS_HOT_HP :
                    FOCUS_HOT_MANA) * cw_focus_share(obj->hero) + 0.5));

      for (p = cw_pools; p && p->idnum != GET_IDNUM(ch); p = p->next)
         ;
      if (!p)
         {
         CREATE(p, struct cw_pool, 1);
         p->idnum = GET_IDNUM(ch);
         p->next = cw_pools;
         cw_pools = p;
         }
      p->hp = obj->hero->focus == HERO_FOCUS_TANK ? per : 0;
      p->mana = obj->hero->focus == HERO_FOCUS_CAST ? per : 0;
      p->ticks = FOCUS_HOT_TICKS;
      }
   /* a borrowed form spends its charge, a blow at a time */
   if (obj->hero->form != HERO_CORE_FORM)
      {
      struct hero_form *f = NULL;
      int i;

      for (i = 0; i < obj->hero->num_forms; i++)
         if (obj->hero->forms[i].vnum == obj->hero->form)
            f = &obj->hero->forms[i];
      if (f && f->charge > 0)
         f->charge--;
      if (!f || f->charge <= 0)
         {
         obj->hero->form = HERO_CORE_FORM;
         cw_refresh(obj);
         send_to_char(ch, "&Y%s's borrowed form is spent and it is back to its own form: %s.&n\r\n",
                      obj->short_description, cw_spent_hint(f));
         }
      }
   cw_grow(ch, obj, POINTS_PER_HIT);
   }

/* every two seconds: the pools pay out */
void cw_focus_tick(void)
   {
   struct cw_pool *p, *next, *prev = NULL;
   struct descriptor_data *d;
   struct char_data *ch;

   if (!class_weapons || !cw_pools)
      return;
   for (p = cw_pools; p; p = next)
      {
      next = p->next;
      for (ch = NULL, d = descriptor_list; d && !ch; d = d->next)
         if (STATE(d) == CON_PLAYING && d->character && GET_IDNUM(d->character) == p->idnum)
            ch = d->character;
      if (ch && p->ticks > 0)
         {
         if (p->hp > 0 && GET_HIT(ch) < GET_MAX_HIT(ch))
            GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + p->hp);
         if (p->mana > 0 && GET_MANA(ch) < GET_MAX_MANA(ch))
            GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + p->mana);
         p->ticks--;
         }
      if (!ch || p->ticks <= 0)
         {
         if (prev)
            prev->next = next;
         else
            cw_pools = next;
         free(p);
         continue;
         }
      prev = p;
      }
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
   if (obj->hero->focus == HERO_FOCUS_HEAL)
      a *= 1.0 + FOCUS_HEAL * cw_focus_share(obj->hero);
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
   if (ch && (obj = cw_in_hand(ch)))
      {
      if (cw_knob(obj->hero->cls, obj->hero->level, &kind, &s) && kind == KIND_OUT)
         mul *= cw_mult(KIND_OUT, s);
      if (obj->hero->focus == HERO_FOCUS_DPS)
         mul *= 1.0 + FOCUS_DPS * cw_focus_share(obj->hero);
      }
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

/* the class weapon into a Hero's hands: at the pass, by a claim or by the
 * staff; once a character, kept in the ledger. force hands it again to one
 * who has had it (lost, say); never a second to one who has it on them. */
static int cw_grant_weapon(struct char_data *ch, int force, int at_pass, char *why)
   {
   struct obj_data *obj;
   struct cw_grant *g;
   long vnum;

   if (IS_NPC(ch) || GET_CLASS(ch) < 0 || GET_CLASS(ch) >= NUM_CLASSES)
      {
      strcpy(why, "no such character");
      return FALSE;
      }
   if (!at_pass && GET_LEVEL(ch) < LVL_HERO && REMORT_LEVEL(ch) < 1)
      {
      sprintf(why, "%s has not passed the Hero test", GET_NAME(ch));
      return FALSE;
      }
   if ((obj = cw_owned(ch)))
      {
      g = cw_grant_of(GET_IDNUM(ch), TRUE);
      if (!g->weapon_at)
         {
         g->weapon_at = (long)time(0);
         cw_write_ledger();
         }
      sprintf(why, "%s already has %s", GET_NAME(ch), obj->short_description);
      return FALSE;
      }
   if (!force && (g = cw_grant_of(GET_IDNUM(ch), FALSE)) && g->weapon_at)
      {
      sprintf(why, "%s has had the class weapon; force to hand it again", GET_NAME(ch));
      return FALSE;
      }
   vnum = cw_first[(int)GET_CLASS(ch)];
   if (vnum <= 0)
      {
      sprintf(why, "the %s has no class weapon set", pc_class_types[(int)GET_CLASS(ch)]);
      return FALSE;
      }
   if (real_object(vnum) < 0 || !(obj = read_object(vnum, VIRTUAL)))
      {
      sprintf(why, "the %s's weapon is not in the world", pc_class_types[(int)GET_CLASS(ch)]);
      return FALSE;
      }
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
   g = cw_grant_of(GET_IDNUM(ch), TRUE);
   g->weapon_at = (long)time(0);
   cw_write_ledger();
   strcpy(why, obj->short_description);
   return TRUE;
   }

void cw_give_first(struct char_data *ch)
   {
   char why[256];

   if (!class_weapons || IS_NPC(ch))
      return;
   cw_grant_weapon(ch, FALSE, TRUE, why);
   }

/* one weapon for life, through remorts: its bonus is the class the
 * character is, and its own form is that class's weapon. The character is
 * naked to remort, so it is carried, not worn. */
/* the copy becomes another template's: its name, descriptions and wear
 * flags follow, and the world's count of each */
static void cw_switch_template(struct obj_data *o, long vnum)
   {
   obj_rnum old = GET_OBJ_RNUM(o), nr = real_object(vnum);

   if (nr < 0 || old == nr)
      return;
   if (old >= 0)
      {
      if (o->name == obj_proto[old].name)
         o->name = obj_proto[nr].name;
      if (o->description == obj_proto[old].description)
         o->description = obj_proto[nr].description;
      if (o->short_description == obj_proto[old].short_description)
         o->short_description = obj_proto[nr].short_description;
      if (o->action_description == obj_proto[old].action_description)
         o->action_description = obj_proto[nr].action_description;
      if (o->ex_description == obj_proto[old].ex_description)
         o->ex_description = obj_proto[nr].ex_description;
      obj_index[old].number--;
      }
   o->item_number = nr;
   obj_index[nr].number++;
   o->obj_flags.wear_flags = obj_proto[nr].obj_flags.wear_flags;
   }

void cw_reclass(struct char_data *ch)
   {
   struct obj_data *o;
   obj_rnum fr;
   long next;
   char why[256];

   if (!class_weapons || IS_NPC(ch))
      return;
   for (o = ch->carrying; o; o = o->next_content)
      {
      if (!cw_mine(o, ch) || o->hero->cls == GET_CLASS(ch))
         continue;
      o->hero->cls = GET_CLASS(ch);
      next = cw_first[(int)GET_CLASS(ch)];
      if (next > 0 && real_object(next) >= 0)
         {
         o->hero->core = next;
         cw_switch_template(o, next);
         }
      /* a form the new class may not use goes back to the weapon's own */
      if (o->hero->form != HERO_CORE_FORM &&
          ((fr = real_object(o->hero->form)) < 0 || cw_form_refusal(ch, o->hero, &obj_proto[fr], why)))
         o->hero->form = HERO_CORE_FORM;
      cw_refresh(o);
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
   /* the 104 piece the ledger says was claimed, unbound by a round trip */
   {
   struct cw_grant *g = cw_grant_of(GET_IDNUM(ch), FALSE);
   int i;

   if (g && g->claim104 > 0)
      {
      for (o = ch->carrying; o; o = o->next_content)
         if (!o->bound_idnum && GET_OBJ_VNUM(o) == g->claim104)
            {
            o->bound_idnum = GET_IDNUM(ch);
            strncpy(o->bound_name, GET_NAME(ch), MAX_NAME_LENGTH);
            o->bound_name[MAX_NAME_LENGTH] = '\0';
            }
      for (i = 0; i < NUM_WEARS; i++)
         if ((o = GET_EQ(ch, i)) && !o->bound_idnum && GET_OBJ_VNUM(o) == g->claim104)
            {
            o->bound_idnum = GET_IDNUM(ch);
            strncpy(o->bound_name, GET_NAME(ch), MAX_NAME_LENGTH);
            o->bound_name[MAX_NAME_LENGTH] = '\0';
            }
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

/* ---------------------------------------------------------------- focus */

/* "Its focus: it strikes harder (63% of the full focus). Kept: heal 34%." */
static void cw_focus_line(struct hero_data *h, char *out)
   {
   char tail[160] = "";
   int f, n = 0;

   for (f = HERO_FOCUS_DPS; f < HERO_NUM_FOCUS; f++)
      if (f != h->focus && h->focus_points[f] > 0)
         sprintf(tail + strlen(tail), "%s%s %d%%", n++ ? ", " : " Kept: ", cw_focus_names[f],
                 (int)cw_floor(cw_share(cw_level_for(h->focus_points[f])) * 100 + 0.5));
   if (n)
      strcat(tail, ".");
   if (h->focus > HERO_FOCUS_NONE && h->focus < HERO_NUM_FOCUS)
      sprintf(out, "Its focus: it %s (%d%% of the full focus).%s", cw_focus_words[h->focus],
              (int)cw_floor(cw_focus_share(h) * 100 + 0.5), tail);
   else
      sprintf(out, "No focus chosen yet: focus <dps|heal|tank|cast>.%s", tail);
   }

/* would this repairman turn the piece away as outside his trade
 * (repair_guy's specialist check) */
static int cw_outside_trade(struct char_data *keeper, struct obj_data *piece)
   {
   int weap = MOB2_FLAGGED(keeper, MOB2_WEAPONSMITH) ? (ITEM_WEAR_WIELD | ITEM_WEAR_HOLD) : 0;
   int armr = MOB2_FLAGGED(keeper, MOB2_ARMORER) ?
              (ITEM_WEAR_BODY | ITEM_WEAR_HEAD | ITEM_WEAR_LEGS | ITEM_WEAR_FEET | ITEM_WEAR_HANDS |
               ITEM_WEAR_ARMS | ITEM_WEAR_SHIELD | ITEM_WEAR_ABOUT | ITEM_WEAR_WAIST |
               ITEM_WEAR_BACK | ITEM_WEAR_FACE) : 0;
   int jewl = MOB2_FLAGGED(keeper, MOB2_JEWELER) ?
              (ITEM_WEAR_HOLD | ITEM_WEAR_FINGER | ITEM_WEAR_NECK | ITEM_WEAR_WRIST |
               ITEM_WEAR_EAR | ITEM_WEAR_WAIST | ITEM_WEAR_FACE | ITEM_WEAR_ABOUT |
               ITEM_WEAR_HEAD) : 0;

   return (weap && !CAN_WEAR(piece, weap)) || (armr && !CAN_WEAR(piece, armr)) ||
          (jewl && !CAN_WEAR(piece, jewl));
   }

/* a repairman at work here (a smith's station, in this game); a master who
 * works on that piece when master is set (one who reforges) */
static struct char_data *cw_repairman(struct char_data *ch, struct obj_data *piece, int master)
   {
   struct char_data *k;

   for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
      {
      if (!IS_NPC(k) || GET_MOB_RNUM(k) < 0 || mob_index[GET_MOB_RNUM(k)].func != repair_guy ||
          !AWAKE(k) || FIGHTING(k))
         continue;
      if (master && (!MOB_FLAGGED(k, MOB_MASTER) || (piece && cw_outside_trade(k, piece))))
         continue;
      return k;
      }
   return NULL;
   }

static int cw_at_smith(struct char_data *ch)
   {
   return cw_repairman(ch, NULL, FALSE) != NULL;
   }

/* focus [dps|heal|tank|cast]: the first anywhere, a change at a smith's
 * station. Only the focus in use grows; one returned to keeps what it had;
 * the first chosen starts with the weapon's growth. */
ACMD(do_focus)
   {
   struct obj_data *obj = NULL, *o;
   struct hero_data *h;
   char *arg = get_buffer(MAX_INPUT_LENGTH), *line = get_buffer(512);
   int f, was;

   one_argument(argument, arg);
   for (o = ch->carrying; o && !obj; o = o->next_content)
      if (cw_mine(o, ch))
         obj = o;
   if (!obj)
      obj = cw_in_hand(ch);
   if (!class_weapons)
      send_to_char(ch, "Huh?!?\r\n");
   else if (!obj)
      send_to_char(ch, "You have no class weapon in hand.\r\n");
   else
      {
      h = obj->hero;
      for (f = HERO_FOCUS_DPS; f < HERO_NUM_FOCUS && str_cmp(arg, (char *)cw_focus_names[f]); f++)
         ;
      if (!*arg)
         {
         cw_focus_line(h, line);
         send_to_char(ch, "&Y%s&n\r\n", line);
         }
      else if (f >= HERO_NUM_FOCUS)
         send_to_char(ch, "focus <dps|heal|tank|cast>: dps, it strikes harder; heal, it heals "
                      "stronger; tank, it mends you with each blow; cast, it feeds your mana with "
                      "each blow.\r\n");
      else if (h->focus == f)
         send_to_char(ch, "The focus of %s is already %s.\r\n", obj->short_description,
                      cw_focus_names[f]);
      else if (h->focus != HERO_FOCUS_NONE && !cw_at_smith(ch))
         send_to_char(ch, "A focus is changed at a smith's station: a forge, a smithy, or a "
                      "smith at his work.\r\n");
      else
         {
         was = h->focus;
         h->focus = f;
         if (was == HERO_FOCUS_NONE)
            {
            h->focus_points[f] = MAX(h->focus_points[f], (double)cw_points(obj));
            send_to_char(ch, "&Y%s takes its focus: it %s, more as it grows.&n\r\n",
                         obj->short_description, cw_focus_words[f]);
            }
         else
            send_to_char(ch, "&Y%s turns its focus to %s: it %s. Each focus keeps what it has "
                         "gained; only the one in use grows.&n\r\n", obj->short_description,
                         cw_focus_names[f], cw_focus_words[f]);
         }
      }
   release_buffer(arg);
   release_buffer(line);
   }

/* ---------------------------------------------------------------- the smith */

static struct obj_data *cw_carried(struct char_data *ch)
   {
   struct obj_data *o;

   for (o = ch->carrying; o; o = o->next_content)
      if (cw_mine(o, ch))
         return o;
   return NULL;
   }

/* infuse <weapon>: at a smith's station, the carried class weapon absorbs a
 * weapon (for a held one, something held) the character could use as they
 * stand; it becomes a form at full charge, or a known form is made new */
ACMD(do_infuse)
   {
   struct obj_data *hero, *other;
   struct hero_data *h;
   struct hero_form *f;
   char *arg = get_buffer(MAX_INPUT_LENGTH), *why = get_buffer(512);
   char word[MAX_INPUT_LENGTH], what[MAX_STRING_LENGTH];
   long vnum;
   int i, known;

   one_argument(argument, arg);
   if (!class_weapons)
      send_to_char(ch, "Huh?!?\r\n");
   else if (!*arg)
      send_to_char(ch, "Infuse what? A weapon you carry (for a Monk, something you hold), into "
                   "your class weapon, at a smith's station.\r\n");
   else if (!cw_at_smith(ch))
      send_to_char(ch, "There is no smith's station here: a forge, a smithy, or a smith at his "
                   "work.\r\n");
   else if (!(hero = cw_carried(ch)))
      send_to_char(ch, "Your class weapon must be carried, not in your hands, to absorb "
                   "anything.\r\n");
   else if (!(other = get_obj_in_list_vis(ch, arg, ch->carrying)))
      send_to_char(ch, "You are not carrying %s %s.\r\n", AN(arg), arg);
   else if (other == hero || other->hero)
      send_to_char(ch, "A class weapon cannot absorb itself or another class weapon.\r\n");
   else if (other->bound_idnum > 0 && other->bound_idnum != GET_IDNUM(ch))
      send_to_char(ch, "%s is %s's.\r\n", other->short_description, other->bound_name);
   else if (cw_form_refusal(ch, (h = hero->hero), other, why))
      send_to_char(ch, "%s\r\n", why);
   else if ((vnum = GET_OBJ_VNUM(other)) == h->core)
      send_to_char(ch, "%s is the weapon's own form already.\r\n", other->short_description);
   else
      {
      for (f = NULL, i = 0; i < h->num_forms; i++)
         if (h->forms[i].vnum == vnum)
            f = &h->forms[i];
      if (!f && h->num_forms >= HERO_MAX_FORMS)
         send_to_char(ch, "%s can hold no more forms.\r\n", hero->short_description);
      else
         {
         strcpy(what, other->short_description);
         one_argument(other->name, word);
         obj_from_char(other);
         extract_obj(other);
         known = (f != NULL);
         if (!f)
            f = &h->forms[h->num_forms++];
         f->vnum = vnum;
         f->charge = cw_form_charge(vnum);
         f->refits = 0;   /* another of the same weapon: a new life */
         if (known)
            send_to_char(ch, "&Y%s absorbs %s: the form is new again.&n\r\n",
                         hero->short_description, what);
         else
            send_to_char(ch, "&Y%s absorbs %s: it can take that form now. shift %s.&n\r\n",
                         hero->short_description, what, word);
         }
      }
   release_buffer(arg);
   release_buffer(why);
   }

/* shift <form> | shift core: the carried class weapon takes a form it has
 * absorbed, with its levels and bonus on top; shift alone lists them */
ACMD(do_shift)
   {
   struct obj_data *obj;
   struct hero_data *h;
   struct hero_form *f;
   obj_rnum r;
   char *arg = get_buffer(MAX_INPUT_LENGTH), *buf = get_buffer(MAX_STRING_LENGTH);
   char why[512];

   one_argument(argument, arg);
   if (!class_weapons)
      send_to_char(ch, "Huh?!?\r\n");
   else if (!(obj = cw_carried(ch)))
      send_to_char(ch, "Your class weapon must be carried, not in your hands, to shift.\r\n");
   else if (!*arg)
      {
      cw_form_list(obj, buf, NULL, NULL);
      send_to_char(ch, "shift <form>: %s\r\n", buf);
      }
   else if (!str_cmp(arg, "core") || !str_cmp(arg, "own") || !str_cmp(arg, "self"))
      {
      if ((h = obj->hero)->form == HERO_CORE_FORM)
         send_to_char(ch, "%s is in its own form.\r\n", obj->short_description);
      else
         {
         h->form = HERO_CORE_FORM;
         cw_refresh(obj);
         send_to_char(ch, "&Y%s is back to its own form.&n\r\n", obj->short_description);
         }
      }
   else if (!(f = cw_find_form((h = obj->hero), arg)) || (r = real_object(f->vnum)) < 0)
      {
      cw_form_list(obj, buf, NULL, NULL);
      send_to_char(ch, "%s knows no form called '%s'. %s\r\n", obj->short_description, arg, buf);
      }
   else if (f->charge <= 0)
      send_to_char(ch, "The form of %s is spent; %s.\r\n", obj_proto[r].short_description,
                   cw_spent_hint(f));
   else if (cw_form_refusal(ch, h, &obj_proto[r], why))
      send_to_char(ch, "Not as you stand: %s\r\n", why);
   else
      {
      h->form = f->vnum;
      cw_refresh(obj);
      send_to_char(ch, "&Y%s takes the form of %s, with your levels and bonus on top; charge %d%%.&n\r\n",
                   obj->short_description, obj_proto[r].short_description,
                   (int)MIN(100, cw_floor(100.0 * f->charge / cw_full_charge(f) + 0.5)));
      }
   release_buffer(arg);
   release_buffer(buf);
   }

/* the masters who reforge, by name: "the Master Weaponsmith, the Master
 * Armourer, or the Master Goldsmith" */
static void cw_masters_line(char *out)
   {
   static const long masters[] = { 27498, 27497, 27496, 0 };
   mob_rnum m;
   int i, n = 0;

   *out = '\0';
   for (i = 0; masters[i]; i++)
      {
      if ((m = real_mobile(masters[i])) < 0)
         continue;
      if (n)
         strcat(out, masters[i + 1] ? ", " : ", or ");
      strcat(out, mob_proto[m].player.short_descr);
      n++;
      }
   if (!n)
      strcpy(out, "the master smiths");
   }

/* refit <form>: a spent form filled to a full charge again, standing for
 * the next repair of its weapon's life at a smith's station, or the next
 * reforge, which only a master does; for what that repair or reforge costs */
ACMD(do_refit)
   {
   struct obj_data *obj;
   struct hero_form *f;
   struct char_data *keeper, *master;
   struct repair_service at;
   obj_rnum r;
   char *arg = get_buffer(MAX_INPUT_LENGTH), *buf = get_buffer(MAX_STRING_LENGTH);
   int left, price;

   one_argument(argument, arg);
   if (!(obj = cw_carried(ch)))
      obj = cw_in_hand(ch);
   if (!class_weapons)
      send_to_char(ch, "Huh?!?\r\n");
   else if (!obj)
      send_to_char(ch, "You have no class weapon to refit.\r\n");
   else if (!(keeper = cw_repairman(ch, NULL, FALSE)))
      send_to_char(ch, "There is no smith's station here: a forge, a smithy, or a smith at his "
                   "work.\r\n");
   else if (!*arg)
      {
      cw_form_list(obj, buf, ch, keeper);
      send_to_char(ch, "Refit which form? %s\r\n", buf);
      }
   else if (!(f = cw_find_form(obj->hero, arg)) || (r = real_object(f->vnum)) < 0)
      {
      cw_form_list(obj, buf, ch, keeper);
      send_to_char(ch, "%s knows no form called '%s'. %s\r\n", obj->short_description, arg, buf);
      }
   else if (f->charge >= cw_full_charge(f))
      send_to_char(ch, "The form of %s is at full charge.\r\n", obj_proto[r].short_description);
   else if (!cw_next_refit(f, &at))
      send_to_char(ch, "The form of %s has been repaired and reforged as often as that weapon "
                   "could be; infuse another of it to make it new.\r\n", obj_proto[r].short_description);
   else if (at.master && !(master = cw_repairman(ch, &obj_proto[r], TRUE)))
      {
      cw_masters_line(buf);
      send_to_char(ch, "Only a master smith can reforge the form of %s now, for %d gold: %s.\r\n",
                   obj_proto[r].short_description, cw_refit_price(ch, keeper, f, &at), buf);
      }
   else if (GET_GOLD(ch) < (price = cw_refit_price(ch, at.master ? master : keeper, f, &at)))
      send_to_char(ch, "You need %d gold on hand to %s the form of %s.\r\n", price,
                   at.master ? "reforge" : "refit", obj_proto[r].short_description);
   else
      {
      GET_GOLD(ch) -= price;
      f->refits = MAX(0, f->refits) + 1;
      f->charge = cw_full_charge(f);
      left = cw_refits_left(f);
      if (left == 0)
         send_to_char(ch, "&YYou pay %d gold. The form of %s is %s to a full charge; it cannot be "
                      "refit again.&n\r\n", price, obj_proto[r].short_description,
                      at.master ? "reforged" : "refit");
      else
         send_to_char(ch, "&YYou pay %d gold. The form of %s is %s to a full charge; it can be refit "
                      "%d more time%s.&n\r\n", price, obj_proto[r].short_description,
                      at.master ? "reforged" : "refit", left, left == 1 ? "" : "s");
      }
   release_buffer(arg);
   release_buffer(buf);
   }

/* ---------------------------------------------------------------- claims */

static struct char_data *cw_keeper_here(struct char_data *ch)
   {
   struct char_data *k;

   for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
      if (IS_NPC(k) && GET_MOB_VNUM(k) == CLAIM_KEEPER && AWAKE(k) && !FIGHTING(k))
         return k;
   return NULL;
   }

/* the class weapon a claim would meet */
#define CLAIM_OWED     0
#define CLAIM_NOT_HERO 1
#define CLAIM_HAD      2
#define CLAIM_HAS      3
#define CLAIM_NONE     4
static int cw_weapon_claim(struct char_data *ch)
   {
   struct cw_grant *g;

   if (GET_LEVEL(ch) < LVL_HERO && REMORT_LEVEL(ch) < 1)
      return CLAIM_NOT_HERO;
   if (cw_owned(ch))
      return CLAIM_HAS;
   if ((g = cw_grant_of(GET_IDNUM(ch), FALSE)) && g->weapon_at)
      return CLAIM_HAD;
   return cw_first[(int)GET_CLASS(ch)] > 0 ? CLAIM_OWED : CLAIM_NONE;
   }

/* the 104 pieces this character may choose from: only those the world has */
static int cw_104_options(struct char_data *ch, long *out)
   {
   int c = GET_CLASS(ch), i, n = 0;

   if (c < 0 || c >= NUM_CLASSES)
      return 0;
   for (i = 0; i < cw_104_n[c]; i++)
      if (real_object(cw_104[c][i]) >= 0)
         out[n++] = cw_104[c][i];
   return n;
   }

/* claim [weapon | <number>]: before the Legendary Purveyor, a class weapon
 * owed from before it was handed out, then the piece set out at the last
 * mortal level for the character's class */
ACMD(do_claim)
   {
   char *arg = get_buffer(MAX_INPUT_LENGTH), why[256];
   long opts[MAX_104_PIECES];
   struct cw_grant *g;
   struct obj_data *obj;
   int c, n, i, pick, wants_weapon;

   one_argument(argument, arg);
   wants_weapon = !str_cmp(arg, "weapon");
   if (!class_weapons)
      send_to_char(ch, "Huh?!?\r\n");
   else if (!cw_keeper_here(ch))
      send_to_char(ch, "Claims are made before the Legendary Purveyor of Mystical Wares, east of "
                   "a small pool in Talam.\r\n");
   else if ((!*arg || wants_weapon) && (c = cw_weapon_claim(ch)) == CLAIM_OWED)
      {
      if (cw_grant_weapon(ch, FALSE, FALSE, why))
         mudlogf(NRM, LVL_IMMORT, TRUE, "%s claims the class weapon (%s).", GET_NAME(ch), why);
      else
         send_to_char(ch, "The Purveyor has nothing for you yet.\r\n");
      }
   else if (wants_weapon)
      {
      if (c == CLAIM_NOT_HERO)
         send_to_char(ch, "The class weapon is for those who have passed the Hero test.\r\n");
      else if (c == CLAIM_HAD)
         send_to_char(ch, "You have had your class weapon. If it is lost, an immortal can "
                      "replace it.\r\n");
      else if (c == CLAIM_HAS)
         send_to_char(ch, "You have your class weapon already.\r\n");
      else
         send_to_char(ch, "No class weapon is set out for %s yet.\r\n",
                      pc_class_types[(int)GET_CLASS(ch)]);
      }
   else if (GET_LEVEL(ch) < CLAIM_LEVEL)
      send_to_char(ch, "The reward is set out for those who reach %d.\r\n", CLAIM_LEVEL);
   else if (GET_LEVEL(ch) >= LVL_IMMORT)
      send_to_char(ch, "The reward is set out for mortals at %d.\r\n", CLAIM_LEVEL);
   else if ((g = cw_grant_of(GET_IDNUM(ch), FALSE)) && g->claim104 > 0 && real_object(g->claim104) >= 0)
      send_to_char(ch, "You have claimed your reward.\r\n");
   else if (!(n = cw_104_options(ch, opts)))
      send_to_char(ch, "Nothing is set out for %s yet.\r\n", pc_class_types[(int)GET_CLASS(ch)]);
   else if (!*arg)
      {
      send_to_char(ch, "&cSet out for a %s of %d: one piece, yours alone.&n\r\n\r\n",
                   pc_class_types[(int)GET_CLASS(ch)], CLAIM_LEVEL);
      for (i = 0; i < n; i++)
         send_to_char(ch, " %d) %s\r\n", i + 1, obj_proto[real_object(opts[i])].short_description);
      send_to_char(ch, "\r\n&Kclaim <number> takes it; it is bound to you and no one else can "
                   "use it.&n\r\n");
      }
   else if ((pick = atoi(arg)) < 1)
      send_to_char(ch, "claim <number>\r\n");
   else if (pick > n)
      send_to_char(ch, "Choose 1 to %d.\r\n", n);
   else if (!(obj = read_object(opts[pick - 1], VIRTUAL)))
      send_to_char(ch, "Piece %ld is not in the world; tell an immortal.\r\n", opts[pick - 1]);
   else
      {
      obj->bound_idnum = GET_IDNUM(ch);
      strncpy(obj->bound_name, GET_NAME(ch), MAX_NAME_LENGTH);
      obj->bound_name[MAX_NAME_LENGTH] = '\0';
      obj_to_char(obj, ch);
      g = cw_grant_of(GET_IDNUM(ch), TRUE);
      g->claim104 = opts[pick - 1];
      g->claimed_at = (long)time(0);
      cw_write_ledger();
      send_to_char(ch, "You take %s. It is bound to you.\r\n", obj->short_description);
      }
   release_buffer(arg);
   }

/* write etc/class_weapons back: the switch is on, or nobody could run this */
static void cw_write_table(void)
   {
   FILE *fl;
   int c;

   if (!(fl = fopen("etc/class_weapons.tmp", "w")))
      return;
   fprintf(fl, "enabled 1\n");
   for (c = 0; c < NUM_CLASSES; c++)
      fprintf(fl, "first %d %ld\n", c, cw_first[c]);
   fprintf(fl, "essence %ld\ncheckpoint %.4f %d\n", cw_essence, cw_chance, cw_max_per_100);
   fclose(fl);
   rename("etc/class_weapons.tmp", "etc/class_weapons");
   }

static int cw_class_of(char *word)
   {
   int c;

   if (isdigit(*word))
      return atoi(word);
   for (c = 0; c < NUM_CLASSES && *pc_class_types[c] != '\n'; c++)
      if (is_abbrev(word, pc_class_types[c]))
         return c;
   return -1;
   }

/* classweapon: the tables -- each class's weapon, the essence, the lock rule */
ACMD(do_classweapon)
   {
   char *a1 = get_buffer(MAX_INPUT_LENGTH), *a2 = get_buffer(MAX_INPUT_LENGTH);
   char *rest = get_buffer(MAX_INPUT_LENGTH);
   obj_rnum r;
   double d;
   long v;
   int c, n;

   half_chop(argument, a1, rest);
   if (!*a1)
      {
      send_to_char(ch, "&cThe class weapons (a Hero's, bound; grows by hits (%d), attack spells "
                   "(%d) and heals (%d) in hand, at most %d a MUD hour, scaled by remort tier; a "
                   "lock %d%% a level, %d in a hundred; the essence comes from a locked weapon's "
                   "boss, slain):&n\r\n", POINTS_PER_HIT, POINTS_PER_CAST, POINTS_PER_HEAL,
                   POINTS_PER_MUD_HOUR, (int)cw_floor(cw_chance * 100 + 0.5), cw_max_per_100);
      r = real_object(cw_essence);
      send_to_char(ch, " essence [%ld] %s\r\n", cw_essence,
                   cw_essence > 0 && r >= 0 ? obj_proto[r].short_description : "(none set)");
      for (c = 0; c < NUM_CLASSES; c++)
         if (cw_first[c] > 0)
            {
            r = real_object(cw_first[c]);
            send_to_char(ch, " %-12s [%ld] %s\r\n", pc_class_types[c], cw_first[c],
                         r >= 0 ? obj_proto[r].short_description : "(not in the world)");
            }
      }
   else if (!str_cmp(a1, "essence"))
      {
      one_argument(rest, a2);
      cw_essence = !str_cmp(a2, "none") ? 0 : atol(a2);
      cw_write_table();
      mudlogf(NRM, LVL_IMMORT, TRUE, "(GC) %s sets the class weapon essence to %ld.", GET_NAME(ch), cw_essence);
      send_to_char(ch, "The essence is %ld.\r\n", cw_essence);
      }
   else if (!str_cmp(a1, "checkpoint"))
      {
      if (sscanf(rest, "%lf %d", &d, &n) != 2)
         send_to_char(ch, "classweapon checkpoint <chance 0..1> <most in a hundred levels>\r\n");
      else
         {
         cw_chance = MAX(0.0, MIN(1.0, d));
         cw_max_per_100 = MAX(0, n);
         cw_write_table();
         mudlogf(NRM, LVL_IMMORT, TRUE, "(GC) %s sets the class weapon locks to %.4f, %d.", GET_NAME(ch), cw_chance, cw_max_per_100);
         send_to_char(ch, "Locks: %d%% a level, at most %d in a hundred.\r\n",
                      (int)cw_floor(cw_chance * 100 + 0.5), cw_max_per_100);
         }
      }
   else if ((c = cw_class_of(a1)) >= 0 && c < NUM_CLASSES)
      {
      half_chop(rest, a2, rest);
      if (str_cmp(a2, "first") || !*rest)
         send_to_char(ch, "classweapon <class> first <vnum>|none\r\n");
      else
         {
         v = !str_cmp(rest, "none") ? 0 : atol(rest);
         cw_first[c] = v;
         cw_write_table();
         mudlogf(NRM, LVL_IMMORT, TRUE, "(GC) %s sets the %s's class weapon to %ld.", GET_NAME(ch), pc_class_types[c], v);
         send_to_char(ch, "The %s's class weapon is %ld.\r\n", pc_class_types[c], v);
         }
      }
   else
      send_to_char(ch, "classweapon | classweapon <class> first <vnum>|none | classweapon essence "
                   "<vnum>|none | classweapon checkpoint <chance> <most in a hundred>\r\n");
   release_buffer(a1);
   release_buffer(a2);
   release_buffer(rest);
   }

/* remortreward: the 104 pieces each class may claim, the ledger, and a
 * class weapon granted to a player who is here */
ACMD(do_remortreward)
   {
   char *a1 = get_buffer(MAX_INPUT_LENGTH), *a2 = get_buffer(MAX_INPUT_LENGTH);
   char *a3 = get_buffer(MAX_INPUT_LENGTH), *rest = get_buffer(MAX_INPUT_LENGTH), why[256];
   struct char_data *victim;
   struct cw_grant *g;
   char *p;
   long v;
   int c, i, n;

   half_chop(argument, a1, rest);
   if (!str_cmp(a1, "grant"))
      {
      half_chop(rest, a2, rest);
      half_chop(rest, a3, rest);
      if (!*a2 || str_cmp(a3, "weapon"))
         send_to_char(ch, "remortreward grant <player> weapon [force]\r\n");
      else if (!(victim = get_player_vis(ch, a2, FIND_CHAR_WORLD)))
         send_to_char(ch, "%s is not in the world; grants go to a player who is here.\r\n", a2);
      else if (!cw_grant_weapon(victim, !str_cmp(rest, "force"), FALSE, why))
         send_to_char(ch, "No: %s.\r\n", why);
      else
         {
         mudlogf(NRM, LVL_IMMORT, TRUE, "(GC) %s grants %s the class weapon%s", GET_NAME(ch),
                 GET_NAME(victim), !str_cmp(rest, "force") ? " (force)" : "");
         send_to_char(ch, "%s has %s.\r\n", GET_NAME(victim), why);
         }
      }
   else if (!str_cmp(a1, "104"))
      {
      half_chop(rest, a2, rest);
      if (!*a2)
         {
         for (c = 0; c < NUM_CLASSES; c++)
            if (cw_104_n[c])
               {
               send_to_char(ch, " %-12s", pc_class_types[c]);
               for (i = 0; i < cw_104_n[c]; i++)
                  send_to_char(ch, " [%ld]", cw_104[c][i]);
               send_to_char(ch, "\r\n");
               }
         send_to_char(ch, "remortreward 104 <class> <vnum> [<vnum>...] | none\r\n");
         }
      else if ((c = cw_class_of(a2)) < 0 || c >= NUM_CLASSES)
         send_to_char(ch, "No class '%s'.\r\n", a2);
      else
         {
         cw_104_n[c] = 0;
         if (str_cmp(rest, "none"))
            for (p = rest; cw_104_n[c] < MAX_104_PIECES && sscanf(p, " %ld%n", &v, &n) == 1; p += n)
               if (v > 0)
                  cw_104[c][cw_104_n[c]++] = v;
         cw_write_ledger();
         mudlogf(NRM, LVL_IMMORT, TRUE, "(GC) %s sets the %s's 104 pieces.", GET_NAME(ch), pc_class_types[c]);
         send_to_char(ch, "The %s may claim %d piece%s.\r\n", pc_class_types[c], cw_104_n[c],
                      cw_104_n[c] == 1 ? "" : "s");
         }
      }
   else if (!str_cmp(a1, "list"))
      {
      for (g = cw_grants; g; g = g->next)
         send_to_char(ch, " #%-6ld weapon %s  104 %s\r\n", g->idnum, g->weapon_at ? "had" : "-",
                      g->claim104 > 0 ? "claimed" : "-");
      }
   else
      send_to_char(ch, "remortreward list | 104 [<class> <vnums>|none] | grant <player> weapon [force]\r\n");
   release_buffer(a1);
   release_buffer(a2);
   release_buffer(a3);
   release_buffer(rest);
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
   cw_focus_line(h, line);
   sprintf(buf + strlen(buf), "%s\r\n", line);
   if (h->form != HERO_CORE_FORM)
      {
      obj_rnum r = real_object(h->form);

      sprintf(buf + strlen(buf), "In the form of %s.\r\n",
              r >= 0 ? obj_proto[r].short_description : "something lost");
      }
   if (h->num_forms)
      {
      char *list = get_buffer(MAX_STRING_LENGTH);

      cw_form_list(obj, list, NULL, NULL);
      sprintf(buf + strlen(buf), "%s\r\n", list);
      release_buffer(list);
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
