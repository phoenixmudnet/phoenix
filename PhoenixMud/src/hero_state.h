/***********************************************************************
*  File: hero_state.h                                                  *
*  Usage: the class weapon's per-copy state and what binds an object   *
*         to one character, and the rent lines that carry them         *
***********************************************************************/

#ifndef HERO_STATE_H
#define HERO_STATE_H

#define HERO_MAX_FORMS    32
#define HERO_MAX_HOLDS    32
#define HERO_MAX_GRANTS   8
#define HERO_GRANT_LEN    48

/* hero->focus */
#define HERO_FOCUS_NONE   0
#define HERO_FOCUS_DPS    1
#define HERO_FOCUS_HEAL   2
#define HERO_FOCUS_TANK   3
#define HERO_FOCUS_CAST   4
#define HERO_NUM_FOCUS    5

/* hero->waiting: HERO_NOT_LOCKED, HERO_NO_BOSS (locked, no boss named
 * yet), or the boss's mob vnum. hero->form: HERO_CORE_FORM or a vnum. */
#define HERO_NOT_LOCKED   (-1)
#define HERO_NO_BOSS      0
#define HERO_CORE_FORM    (-1)

/* forms[].refits: how often the form was refit; HERO_REFITS_UNSET on a
 * form charged before forms were cut into charges. */
#define HERO_REFITS_UNSET (-1)

struct hero_form
   {
   long vnum;
   long charge;
   int refits;
   long sitting;            /* when it was lent from a !rent piece, for one
                             * sitting (time(0)); 0: held for good */
   };

struct hero_data
   {
   int cls;                 /* the class it serves */
   long core;               /* the class weapon's own vnum */
   int level;
   int essences;            /* essences set in it (each opened a lock) */
   int cleared;             /* locks opened */
   long waiting;            /* see HERO_NOT_LOCKED */
   long form;               /* the form it is in; see HERO_CORE_FORM */
   int focus;
   double focus_points[HERO_NUM_FOCUS];
   int num_forms;
   struct hero_form forms[HERO_MAX_FORMS];
   int num_holds;
   int holds[HERO_MAX_HOLDS];  /* the levels a lock held it at */
   int num_grants;
   char grants[HERO_MAX_GRANTS][HERO_GRANT_LEN];
   };

/* growth points and the MUD hour's count, on any attuned copy */
struct obj_attune_data
   {
   double points;
   long hits;               /* an earlier save's counted blows */
   long hour;               /* the MUD hour the counts below belong to */
   long hour_hits;
   long hour_points;
   };

struct obj_data;
struct char_data;

struct hero_data *hero_create(void);
struct obj_attune_data *attune_create(void);
/* the copy's weapon state, attunement and binding freed or cleared */
void obj_hero_free(struct obj_data *obj);
/* dst takes its own copy of src's weapon state, attunement and binding */
void obj_hero_copy(struct obj_data *dst, const struct obj_data *src);
/* the H lines of one rent record; 0, or -1 on a write error */
int obj_hero_write(FILE *fp, const struct obj_data *obj);
/* one rent line: TRUE when it was an H line (taken, or skipped if odd) */
int obj_hero_read_line(struct obj_data *obj, const char *line);

#endif
