/***********************************************************************
*  File: armor_math.h                                                  *
*  Usage: the class armor's arithmetic and tables -- six pieces a      *
*         class, part of the class weapon's system; their drops and    *
*         their attunement. Self-contained, so the tests link it alone.*
***********************************************************************/

#ifndef ARMOR_MATH_H
#define ARMOR_MATH_H

#include "hero_state.h"

/* the six pieces in vnum order; their parts of the armor's share */
#define ARMOR_HEAD   0
#define ARMOR_BODY   1
#define ARMOR_LEGS   2
#define ARMOR_FEET   3
#define ARMOR_HANDS  4
#define ARMOR_ARMS   5
#define ARMOR_NUM_SLOTS 6
#define ARMOR_PARTS_TOTAL 10

#define ARMOR_BASE_VNUM     27600
#define ARMOR_ESSENCE_VNUM  27690
#define ARMOR_NUM_CLASSES   15

/* all six pieces cost this much work: 360 online hours from a first remort
 * to level 103 of a second, at the benchmark's 600 an hour */
#define ARMOR_WORK_TOTAL    216000.0
#define ARMOR_MARK_SPREAD   0.5

/* the set: the pieces worn and the class weapon in hand */
#define SET_FOCUS_AT        3
#define SET_FOCUS_MULT      1.25
#define SET_SECOND_FOCUS_AT 5

/* attunement */
#define ARMOR_STAT_HP         0
#define ARMOR_STAT_MANA       1
#define ARMOR_STAT_STR        2
#define ARMOR_STAT_DEX        3
#define ARMOR_STAT_HITROLL    4
#define ARMOR_STAT_DAMROLL    5
#define ARMOR_STAT_SPELLSAVE  6
#define ARMOR_STAT_BREATHSAVE 7
#define ARMOR_STAT_ARMOR      8

#define ATTUNE_POINTS_PER_PART 2
#define ATTUNE_OVER_PER_PART   1
#define ESSENCES_PER_POINT     100
#define OVER_BREAK_STEP        0.15
#define MASTER_ARMOURER        27497
#define UNATTUNE_GOLD_PER_POINT 20000
#define UNATTUNE_KEEP          0.75

/* armor_check's answers */
#define ATTUNE_OK          0
#define ATTUNE_NOT_IN_POOL 1
#define ATTUNE_STAT_FULL   2
#define ATTUNE_OFFENSE_FULL 3
#define ATTUNE_NO_ROOM     4
#define ATTUNE_OVER_FULL   5
#define ATTUNE_NO_OVER     6

/* a character's class armor: the pieces had (a bit a slot), the work since
 * the last and the mark for the next, the class it is for, and the work a
 * kill has yet to pay (the class weapon's bank and MUD hour rules) */
struct class_armor_state
   {
   int cls;
   int got;
   double work;
   int next;                /* a slot, or -1 */
   long mark;
   double bank;
   long hour;
   long hour_points;
   };

const char *armor_slot_name(int slot);
int    armor_slot_parts(int slot);
double armor_work_per_essence(void);
int    armor_class_index(int cls);
long   armor_vnum(int cls, int slot);
int    armor_piece_of(long vnum, int *cls, int *slot);
int    armor_got_count(int got);

/* the drop */
void   armor_state_fresh(struct class_armor_state *st, int cls);
int    armor_pick_next(int got, double r);
long   armor_mark_for(int slot, double r);
int    armor_add_work(struct class_armor_state *st, double work, double (*rand01)(void));
int    armor_essences_for(double work, double r);

/* the bonus: deals (or heals) m - 1, takes 1 - m; the armor's part on top
 * of what the weapon already applies at its share */
double armor_slot_fraction(int slot);
double armor_factor(int in_kind, double s, double weapon_share, double fraction);
double armor_set_focus_mult(int count);

/* attunement */
const char *armor_stat_label(int stat);
int    armor_stat_parse(const char *typed);
int    armor_limit(int slot);
int    armor_over_limit(int slot);
int    armor_stat_points(int stat);
int    armor_max_units(int stat, int slot);
int    armor_pool(int slot, int out[ARMOR_NUM_STATS]);
int    armor_points_used(const struct obj_armor_data *a);
int    armor_points_over(const struct obj_armor_data *a);
long   armor_over_cost(int k);
double armor_over_break(int k);
int    armor_check(int slot, const struct obj_armor_data *a, int stat, int *cost, double *brk,
                   int *over);
int    armor_stat_per(int stat);
void   armor_totals(const struct obj_armor_data *a, int totals[ARMOR_NUM_STATS]);
void   armor_unattune_terms(const struct obj_armor_data *a, int stat, int *essences,
                            long *gold, int *points);
void   armor_settle_over(struct obj_armor_data *a, int slot);

#endif
