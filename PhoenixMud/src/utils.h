/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


/* external declarations and prototypes **********************************/
extern struct weather_data weather_info;
extern int cha_align_table[][21];
#define DODGER_DEBUG 1
#undef log

#ifdef CIRCLE_WINDOWS
#define log(x) basic_mud_log(x)
#endif

#define mudlog(a,b,c,d)        mudlogf(b,c,d,"%s",a)

/* public functions in utils.c */
int   str_cmp(char *arg1, char *arg2);
int   strn_cmp(char *arg1, char *arg2, int n);
int   touch(char *path);
int   stat_index(int stat_to_check);
int   min_level(struct char_data *ch,int spellnum);

void count_items(struct char_data * ch, struct obj_data * obj, long *nitems);

int     num_charmies(struct char_data * ch);
void    add_follower(struct char_data *ch, struct char_data *leader);
void    stop_follower(struct char_data *ch);
void    die_follower(struct char_data * ch);
void    check_follower(struct char_data *leader, struct char_data *ch);
bool	circle_follow(struct char_data *ch, struct char_data * victim);

int     num_fighting(struct char_data *ch);
int     num_pcs_fighting(struct char_data *ch);

int     num_npcs_in_room(struct char_data *ch);

int     is_remort_level(struct char_data *ch, int remort_type);

void check_weapon_weight(struct char_data *ch);
int price_adjust (struct char_data * ch, struct char_data * vendor, int price);

#if defined(__GNUC__)
void   log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
#else
void   log(const char *format, ...);
#endif

void    core_dump_unix(const char *, ush_int);
#define core_dump() core_dump_unix(__FILE__,__LINE__)
/*
 * The attribute specifies that mudlogf() is formatted just like printf() is.
 * 4,5 means the format string is the fourth parameter and start checking
 * the fifth parameter for the types in the format string.  Not certain
 * if this is a GNU extension so if you want to try it, put #if 1 below.
 */
#if defined(__GNUC__)
void    mudlogf(char type, int level, byte file, const char *format, ...)
   __attribute__ ((format (printf, 4, 5)));
#else
void    mudlogf(char type, int level, byte file, const char *format, ...);
#endif
void	log_death_trap(struct char_data *ch);
int	number(int from, int to);
int	dice(int die_number, int size);
void	sprintbit(bitvector_t vektor, char *names[], char *result);
void	sprintbit2(bitvector_t vektor, char *names[], char *result,
       int start_pos);
void	sprinttype(int type, char *names[], char *result);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *orig_name, char *filename, int mode);
struct time_info_data *age(struct char_data *ch);
int	num_pc_in_room(struct room_data *room);

int     replace_str(char **string, char *pattern, char *replacement,
        int rep_all, int max_size);
void    format_text(char **ptr_string, int mode, struct descriptor_data *d,
        int maxlen);
char   *stripcr(char *dest, const char *src);
char   *stolower(char *str);
const char *stristr(const char *string, const char *key);

int starts_with(const char *string, const char *key);

int is_olc_set(struct char_data *ch, int zone);

/* random functions in random.c */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);



/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
char *CAP(char *txt);
char *CAP_LINE(char *txt);

/* in act.informative.c */
void	look_at_room(struct char_data *ch, int mode);

/* in act.movmement.c */
int	do_simple_move(struct char_data *ch, int dir, int need_specials_check);
int	perform_move(struct char_data *ch, int dir, int need_specials_check,
         int following);

/* in limits.c */
int	mana_gain(struct char_data *ch);
int	hit_gain(struct char_data *ch);
int	move_gain(struct char_data *ch);
void	advance_level(struct char_data *ch, bool show);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, long gain);
void	gain_exp_regardless(struct char_data *ch, long gain, bool show);
void	gain_condition(struct char_data *ch, int condition, int value);
void	check_idling(struct char_data *ch);
void	point_update(void);
void	update_pos(struct char_data *victim);


/* various constants *****************************************************/


/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE	 0
#define ETEXT_FILE	 1
#define ALIAS_FILE	 2	/* Alias mod */
#define SCRIPT_VARS_FILE 3
#define NEW_OBJ_FILES    4
#define REIMB_FILE       5
#define COMMS_FILE    6

/* breadth-first searching */
#define BFS_ERROR		-1
#define BFS_ALREADY_THERE	-2
#define BFS_NO_PATH		-3

/* mud-life time */
#define SECS_PER_MUD_HOUR	75
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(35*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(17*SECS_PER_MUD_MONTH)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)


/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')
#define IF_STR(st) ((st) ? (st) : "\0")
#define LOW(st)  (*(st) = LOWER(*(st)), st)

#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")


/* memory utils **********************************************************/

#if BUFFER_MEMORY
#define CREATE(result, type, number)  do {\
       if (!((result) = (type *) debug_calloc ((number), sizeof(type),#result, __FUNCTION__, __LINE__)))\
               { perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
       if (!((result) = (type *) debug_realloc ((result), sizeof(type) * (number),#result,__FUNCTION__, __LINE__)))\
               { perror("realloc failure"); abort(); } } while(0)

#define free(variable) debug_free((variable), __FUNCTION__, __LINE__)
#define str_dup(variable) debug_str_dup((variable), #variable,__FUNCTION__,__LINE__)
#else
char	*str_dup(const char *source);
#define CREATE(result, type, number)  do {\
  if (!((result) = (type *) calloc ((number), sizeof(type))))\
    { log("%s %s %d", __FILE__,__FUNCTION__,__LINE__);\
                  perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
    { perror("realloc failure"); abort(); } } while(0)
#define really_free(variable) free((variable))
#endif
/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
   temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\


/* basic bitvector utils *************************************************/

#define  ZONE_FLAGS(i)  (zone_table[(i)].bitvector)
#define  ZONE_FLAGGED(i, flag)  (IS_SET(ZONE_FLAGS((i)), flag))
#define  Z_FLAGGED(loc, flag) (IS_SET(ZONE_FLAGS(world[(loc)].zone), (flag)))

#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))

/* Subtle bug, but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
   (*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))


#define MOB_FLAGS(ch) ((ch)->char_specials.saved.act)
#define MOB2_FLAGS(ch) ((ch)->char_specials.saved.act2)
#define PLR_FLAGS(ch) ((ch)->char_specials.saved.act)
/*
  #define PRF_FLAGS(ch) ((ch)->player_specials->saved.pref)
  #define PRF2_FLAGS(ch) ((ch)->player_specials->saved.pref2)
  */
#define PRF_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.pref))
#define PRF2_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.pref2))
#define AFF_FLAGS(ch) ((ch)->char_specials.saved.affected_by)
#define AFF2_FLAGS(ch) ((ch)->char_specials.saved.affected_by2)
#define ROOM_FLAGS(loc) (world[(loc)].room_flags)
#define ROOM2_FLAGS(loc) (world[(loc)].room2_flags)

/* add affected3_bits when needed */
#define WHICH_BITS(loc) ( ((loc) == APPLY_AFF2) ? affected2_bits : affected_bits )

#define IS_NPC(ch)  (IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)  (IS_NPC(ch) && GET_MOB_RNUM(ch) >=0)

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define MOB2_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB2_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag) (IS_SET(AFF_FLAGS(ch), (flag)))
#define AFF2_FLAGGED(ch, flag) (IS_SET(AFF2_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) (IS_SET(PRF_FLAGS(ch), (flag)))
#define PRF2_FLAGGED(ch, flag) (IS_SET(PRF2_FLAGS(ch), (flag)))
#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS(loc), (flag)))
#define ROOM2_FLAGGED(loc, flag) (IS_SET(ROOM2_FLAGS(loc), (flag)))
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define OBJWEAR_FLAGGED(obj, flag) (IS_SET((obj)->obj_flags.wear_flags,(flag)))
#define OBJ_FLAGGED(obj, flag) (IS_SET(GET_OBJ_EXTRA(obj), (flag)))
#define OBJ_ANTI_FLAGGED(obj, flag) (IS_SET(GET_OBJ_ANTI(obj), (flag)))

#define SPELL_ROUTINES(spl)    (spells[spl].routines)
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK2(ch,flag) ((TOGGLE_BIT(PRF2_FLAGS(ch), (flag))) & (flag))



/* room utils ************************************************************/
#define SECT(room)	(world[(room)].sector_type)

#define IS_SHADE(ch)  ( world[((ch)->in_room)].light <= -10 )

#define IS_DARK(room)  ((room)>0 && (( world[room].light<VALUE_LIGHT && \
                         (ROOM_FLAGGED(room, ROOM_DARK) || \
        (world[room].light < -5) || \
                          ( ( SECT(room) != SECT_INSIDE && \
                              SECT(room) != SECT_CITY ) && \
                            (weather_info.sunlight == SUN_SET || \
           weather_info.sunlight == SUN_DARK)) ) )))

#define IS_LIGHT(room)  (!IS_DARK(room))
#define VALID_RNUM(rnum)       ((rnum) >= 0 && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
      ((room_vnum)(VALID_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
#define GET_ROOM_SPEC(room) (VALID_RNUM(room) ? world[(room)].func : NULL)

#define GET_LOCK_LEVEL(dir) (EXIT_FLAGGED(dir,EX_WIZLOCK)?dir->lcklevl:0)

#define CAN_ROVE(room) ( (SECT(room) != SECT_INSIDE) && \
                         (SECT(room) != SECT_CITY) && \
                         (SECT(room) != SECT_INSIDE) && \
                         (SECT(room) != SECT_UNDERWATER) && \
                         (SECT(room) != SECT_INSIDE) && \
                         (SECT(room) != SECT_WATER_NOSWIM) )

/* char utils ************************************************************/

#define REMORT_LEVEL(ch) ((ch)->player_specials->saved.times_remorted)
/* used in interpreter.c */
#define IS_REMORT_OLD(ch)   ((GET_CLASS((ch)) >= CLASS_KENSAI) && \
                             (GET_CLASS((ch)) <= CLASS_DEVA))
#define IS_DBLREMORT_OLD(ch)((GET_RACE((ch)) >= RACE_DRACONIAN) && \
                             (GET_RACE((ch)) <= RACE_AESIR))

/* Only CLASS_WARRIOR through CLASS_DRUID are considered SCR's. */
#define IS_SCR(ch) (GET_CLASS((ch)) < CLASS_KENSAI && REMORT_LEVEL((ch)) > NON_REMORT)
#define IS_SRR(ch) (GET_CLASS((ch)) < CLASS_KENSAI && REMORT_LEVEL((ch)) > SINGLE_REMORT)

#define MUTE_CHANNELS(ch) ((ch)->player_specials->saved.mute_channels)
#define IN_ROOM(ch)	((ch)->in_room)
#define GET_ID(x)       ((x)->id)
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_PFILEPOS(ch)((ch)->pfilepos)
#define GET_AGE(ch)     (age(ch)->year)

#define GET_PC_NAME(ch)        ((ch)->player.name)
#define GET_NAME(ch)    (IS_NPC(ch) ? \
       (ch)->player.short_descr : GET_PC_NAME(ch))
#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_LEVEL(ch)   ((ch)->player.level)
#define GET_PASSWD(ch)	((ch)->player.passwd)
#define GET_RACE(ch)    ((ch)->player.race)	/* 10/26/96, Echo */
#define GET_CLASS(ch)   ((ch)->player.class)
#define GET_HOME(ch)	((ch)->player.hometown)
#define GET_HEIGHT(ch)	((ch)->player.height)
#define GET_WEIGHT(ch)	((ch)->player.weight)
#define GET_SEX(ch)	((ch)->player.sex)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define GOD_LOG(ch)     (IS_NPC(ch)?LVL_DETY:MAX(LVL_DETY,MAX(GET_LEVEL(ch),GET_INVIS_LEV(ch))))

#define GET_STR(ch)     ((ch)->aff_abils.str)
#define GET_ADD(ch)     ((ch)->aff_abils.str_add)
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)

#define GET_EXP(ch)	  ((ch)->points.exp)
#define GET_AC(ch)        ((ch)->points.armor)
#define GET_HIT(ch)	  ((ch)->points.hit)
#define GET_MAX_HIT(ch)	  ((ch)->points.max_hit)
#define GET_MOVE(ch)	  ((ch)->points.move)
#define GET_MAX_MOVE(ch)  ((ch)->points.max_move)
#define GET_MANA(ch)	  ((ch)->points.mana)
#define GET_MAX_MANA(ch)  ((ch)->points.max_mana)
#define GET_GOLD(ch)	  ((ch)->points.gold[0])
#define GET_BANK_GOLD(ch) ((ch)->points.bank_gold[0])
#define GET_HITROLL(ch)	  ((ch)->points.hitroll)
#define GET_DAMROLL(ch)   ((ch)->points.damroll)

/** GET_BATTLE anduin **/
#define GET_BATTLE(ch)	   ((ch)->char_specials.in_battle)
#define GET_POS(ch)	   ((ch)->char_specials.position)
#define IS_CARRYING_W(ch)  ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch)  ((ch)->char_specials.carry_items)
#define FIGHTING(ch)	   ((ch)->char_specials.fighting)
#define LAST_HAND_USED(ch) ((ch)->char_specials.last_hand_used)
#define NEXT_HIT(ch)       ((ch)->char_specials.speed)
#define TIMER(ch)	   ((ch)->char_specials.timer)
#define GET_LIGHT(ch)	   ((ch)->char_specials.light)
#define HUNTING(ch)	   ((ch)->char_specials.hunting)
#define FURNITURE(ch)	   ((ch)->char_specials.furniture)
#define RIDING(ch)	   ((ch)->char_specials.riding)		/* // (DAK) */
#define RIDDEN_BY(ch)	   ((ch)->char_specials.ridden_by)	/* // (DAK) */
#define GET_DAM_AMT(ch,i)  ((ch)->char_specials.damage_amount[(i)])
#define GET_DODGED(ch)     ((ch)->char_specials.dodged)
#define IS_CASTING(ch)     ((ch)->char_specials.casting)
#define CAST_SPELLNUM(ch)  ((ch)->char_specials.spellnum)
#define CAST_TCH(ch)       ((ch)->char_specials.tch)
#define CAST_TOBJ(ch)      ((ch)->char_specials.tobj)
#define CAST_TDR(ch)       ((ch)->char_specials.tdr)
#define CAST_TDR2(ch)      ((ch)->char_specials.tdr2)
#define CAST_LEVEL(ch)     ((ch)->char_specials.cast_level)
#define CAST_TIME(ch)      ((ch)->char_specials.cast_time)
#define CAST_ARG(ch)       ((ch)->char_specials.cast_arg)
#define IMMUNE(ch)         ((ch)->char_specials.immune)
#define RESIST(ch)         ((ch)->char_specials.resist)
#define SUCCEPT(ch)        ((ch)->char_specials.succept)
#define GET_IDNUM(ch)	   ((ch)->char_specials.saved.idnum)
#define GET_SAVE(ch, i)	   ((ch)->char_specials.saved.apply_saving_throw[i])
#define GET_ALIGNMENT(ch)  ((ch)->char_specials.saved.alignment)
#define GET_SPELL_FAIL(ch) ((ch)->char_specials.saved.spell_fail)
#define GET_EXPLORED(ch)   ((ch)->player_specials->explored_total)
#define GET_EMAIL(ch)      ((ch)->player_specials->email)
#define GET_GUARDING(ch)   ((ch)->guarding)
#define GET_GUARDING_ME(ch) ((ch)->guarding_me)
#define GET_NUM_GUARDING_ME(ch) ((ch)->num_guarding_me)

#define IS_IMMUNE(ch,bit)  (IS_SET(IMMUNE(ch),(bit)))
#define IS_RESIST(ch,bit)  (IS_SET(RESIST(ch),(bit)))
#define IS_SUCCEPT(ch,bit) (IS_SET(SUCCEPT(ch),(bit)))

#define GET_COND(ch, i)		CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.conditions[(i)])
#define GET_LOADROOM(ch)	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.load_room)
#define GET_LAST_LEARN(ch)	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.last_learnt)
#define GET_LEARN_TIC(ch)       CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.learn_tic)
#define GET_INVIS_LEV(ch)	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.invis_level)
#define GET_WIMP_LEV(ch)	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.wimp_level)
#define GET_QPOINTS(ch)  	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.q_points)
#define GET_OLD_MOBKILLS(ch)    CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.old_mobkills)
/* temporary until pfile change */
#define GET_MOBKILLS(ch)        CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.mobkills)
#define GET_PKILLS(ch)   	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.pkills)
#define GET_DEATHS(ch)   	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.deaths)
#define GET_FREEZE_LEV(ch)	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.freeze_level)
#define GET_BAD_PWS(ch)		CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.bad_pws)
#define GET_TALK(ch, i)		CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.talks[i])
#define POOFIN(ch)		CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.poofin)
#define POOFOUT(ch)		CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.poofout)
#define GET_LAST_OLC_TARG(ch)	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->last_olc_targ)
#define GET_LAST_OLC_MODE(ch)	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->last_olc_mode)
#define GET_ALIASES(ch)		CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->aliases)
#define GET_LAST_TELL(ch)	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->last_tell)
#define GET_IGNORED(ch, i)      CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->ignored[i])
#define TAGGED(ch)              CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->tagged)

#define GET_SKILL(ch, i)	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.skills[i])
#define SET_SKILL(ch, i, pct)	do { CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.skills[i]) = pct; } while(0)

//#define SCR_SKILLCHECK(ch, i)        ((!IS_SCR(ch) || GET_LEVEL(ch) >= min_level(ch, i)))
#define SCR_SKILLCHECK(ch, i)        (!(IS_SCR(ch)||REMORT_LEVEL(ch) >= DOUBLE_REMORT) || GET_LEVEL(ch) >= min_level(ch, i))

#define GET_SKILL_LEARN(ch, i)	CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.skills_learn[i])

#define GET_KILLS_VNUM(ch,i)    CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.kills_vnum[i])
#define GET_KILLS_AMMOUNT(ch,i) CHECK_PLAYER_SPECIAL((ch),(ch)->player_specials->saved.kills_ammount[i])
#define GET_MOB_MAXFACTOR(ch)   ((ch)->mob_specials.maxfactor)

#define GET_EQ(ch, i)		((ch)->equipment[i])

#define GET_MOB_SPEC(ch) (IS_MOB(ch) ? (mob_index[(ch->nr)].func) : NULL)
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
         mob_index[GET_MOB_RNUM(mob)].vnum : -1)

#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
#define MEMORY(ch)		((ch)->mob_specials.memory)
#define GET_MOOD(ch)            ((ch)->mob_specials.mood)
#define GET_MOB_VAL(ch, val)	((ch)->mob_specials.special_value[(val)])

#define STRENGTH_APPLY_INDEX(ch) \
      (stat_index(GET_STR(ch)))
/*         ( ((GET_ADD(ch)==0) || (GET_STR(ch) != 18)) ? stat_index(GET_STR(ch)):\ */
/*           (GET_ADD(ch) <= 50) ? 46 :( \ */
/*           (GET_ADD(ch) <= 75) ? 47 :( \ */
/*           (GET_ADD(ch) <= 90) ? 48 :( \ */
/*           (GET_ADD(ch) <= 99) ? 49 :  50 ) ) )                   \ */
/*         ) */

#define CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w)
#define CAN_WIELD_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
#define CAN_CARRY_N(ch) (5 + (GET_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define HAS_INFRA(ch) (AFF_FLAGGED(ch,AFF_INFRAVISION))

#define CAN_SEE_IN_DARK(ch) \
   (HAS_INFRA(ch) || (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))

/* 10/26/96, Echo */
#define GET_EXP_FOR_LEVEL(race, class, level, remortlev) (long int) ( \
  (remortlev == TRIPLE_REMORT ? 4.0f : 1.0f) \
  * (remortlev == DOUBLE_REMORT ? 2.0f : (float)race_exp_multipliers[(int)(race)]) \
  * (remortlev >= SINGLE_REMORT ? 2.2f : (float)class_exp_multipliers[(int)(class)]) \
  * exp_table[(int)(level)] \
)

#define GET_EXP_FOR_CH(ch) (long int)( \
  (REMORT_LEVEL(ch) == TRIPLE_REMORT ? 4.0f : 1.0f) \
  * (REMORT_LEVEL((ch)) == DOUBLE_REMORT ? 2.0f : (float)race_exp_multipliers[(int)GET_RACE((ch))]) \
  * (REMORT_LEVEL((ch)) >= SINGLE_REMORT ? 2.2f : (float)class_exp_multipliers[(int)GET_CLASS((ch))]) \
  * exp_table[(int)(GET_LEVEL((ch)))] \
  )

/* These three deprecated. */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (IS_NPC(ch)?(4*cycle):(20*cycle)); } while(0)
#define CHECK_WAIT(ch)                ((ch)->wait > 0)
#define GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch)    ((ch)->wait)

/* lag experienced that will prevent you from fighting */
/* lag for player reduced from 20 to 10, until discussion can be made on subject */
#define STUN_STATE(ch, cycle) do { GET_STUN_STATE(ch) = (IS_NPC(ch)?(4*cycle):(10*cycle)); } while(0)
#define CHECK_STUN(ch)        ((ch)->stun_lag > 0)
#define GET_STUN_STATE(ch)    ((ch)->stun_lag)

/*
  #define WAIT_STATE(ch, cycle) { \
  if ((ch)->desc) (ch)->desc->wait += ((cycle)*20); \
  else if (IS_NPC(ch)) GET_MOB_WAIT(ch) += ((cycle)*4); }

  #define CHECK_WAIT(ch)	((!IS_NPC(ch))? \
  (((ch)->desc) ? ((ch)->desc->wait > 1) : 0 ):\
  (GET_MOB_WAIT(ch)>0))
  #define GET_MOB_WAIT(ch)	((ch)->mob_specials.wait_state)
  */
/* descriptor-based utils ************************************************/
#define STATE(d)	((d)->connected)


/* object utils **********************************************************/

/* 10/17/96, Echo - added GET_OBJ_NAME for refuelable lights. See note below.
 */
#define GET_OBJ_NAME(obj)       ((obj)->short_description)
#define GET_OBJ_VROOM(obj)      ((obj)->vroom)
#define GET_OBJ_TYPE(obj)	((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)	((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)	((obj)->obj_flags.cost_per_day)
#define GET_OBJ_EXTRA(obj)	((obj)->obj_flags.extra_flags)
#define GET_OBJ_EXTRA2(obj)	((obj)->obj_flags.extra_flags2)
#define GET_OBJ_EXTRA3(obj)	((obj)->obj_flags.extra_flags3)
#define GET_OBJ_ANTI(obj)	((obj)->obj_flags.anti_flags)
#define GET_OBJ_WEAR(obj)	((obj)->obj_flags.wear_flags)
#define GET_OBJ_VAL(obj, val)	((obj)->obj_flags.value[(val)])
#define GET_OBJ_LR(obj)         ((obj)->obj_flags.value[4])
#define GET_OBJ_WEIGHT(obj)	((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj)	((obj)->obj_flags.timer)
#define GET_OBJ_DGTIMER(obj)	((obj)->obj_flags.dg_timer)
#define GET_OBJ_CSLOTS(obj)     ((obj)->obj_flags.curr_dam_slots)
#define GET_OBJ_TSLOTS(obj)     ((obj)->obj_flags.total_dam_slots)
#define GET_OBJ_OSLOTS(obj)     ((obj)->obj_flags.orig_dam_slots)
#define GET_OBJ_SHOP_ORDER(obj) ((obj)->obj_flags.shop_order)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
#define GET_OBJ_VNUM(obj)	(GET_OBJ_RNUM(obj) >= 0 ? \
         obj_index[GET_OBJ_RNUM(obj)].vnum : -1)
#define IS_OBJ_STAT(obj,stat)	(IS_SET((obj)->obj_flags.extra_flags,stat))
#define IS_OBJ_STAT2(obj,stat) (IS_SET((obj)->obj_flags.extra_flags2,stat))
#define IS_CORPSE(obj)          (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
         GET_OBJ_VAL((obj), 3) == 1 && \
         (IS_OBJ_STAT(obj, ITEM_PC_CORPSE) || \
                                IS_OBJ_STAT(obj, ITEM_NPC_CORPSE)))

#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? \
  (obj_index[(obj)->item_number].func) : NULL)

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))

#define TWO_HANDED(obj)         (GET_OBJ_TYPE((obj)) == ITEM_WEAPON && \
                                 IS_OBJ_STAT((obj), ITEM_TWO_HAND))

#define ONE_HAND_FULL(ch)       (GET_EQ(ch, WEAR_WIELD_1) || \
                                 GET_EQ(ch, WEAR_WIELD_2) || \
                                 GET_EQ(ch, WEAR_SHIELD)  ||  \
                                 GET_EQ(ch, WEAR_HOLD_1)  || \
                                 GET_EQ(ch, WEAR_HOLD_2))

/* Refuelable light mod--Aleks */
#define GET_FUEL_TYPE(obj)	((obj)->obj_flags.value[1])
#define GET_MAX_FUEL(obj)	((obj)->obj_flags.value[3])
#define GET_CUR_FUEL(obj)	((obj)->obj_flags.value[2])

/* Remort and Double remort item restrictions--Nomikos */
#define IS_REMORT_ITEM(obj)     (IS_SET((obj)->obj_flags.extra_flags2, ITEM2_REMORT))
#define IS_DBLREMORT_ITEM(obj)  (IS_SET((obj)->obj_flags.extra_flags2, ITEM2_DBLREMORT))


/* compound utilities and other macros **********************************/


#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
      (((major) << 16) + ((minor) << 8) + (patchlevel))

/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT((sub)->in_room) || HAS_INFRA(sub)))

#define LIGHT_OBJ(ch,obj)  (!AFF_FLAGGED(ch,AFF_BLIND) &&\
          (IS_OBJ_STAT(obj,ITEM_GLOW)||\
           (GET_OBJ_TYPE(obj)==ITEM_LIGHT)))
/*
 * INVIS_OK
 * target can be seen if... 
 * it isn't invis or you have detect invis
 * if it isn't hiding or you have sense life
 * if the target isn't shadowed and in low light or you have sense life and are of equal or greater level
 * if the target is in a pkill room and shadowed they can be seen by anyone with sense life
 * if target is shadowed any mob with sense life can see them
 */
#define INVIS_OK(sub, obj)                                                                          \
  ((!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED((sub) ,AFF_DETECT_INVIS))                      \
    && ((!AFF_FLAGGED((obj), AFF_HIDE) && !(AFF2_FLAGGED((obj), AFF2_SHADOW)                        \
    && IS_SHADE(obj))) || AFF_FLAGGED((sub), AFF_SENSE_LIFE))                                       \
    && (!AFF2_FLAGGED((obj), AFF2_SHADOW) || GET_LEVEL((sub)) >= GET_LEVEL((obj)) ||                \
    IS_NPC((sub)) || ROOM_FLAGGED(IN_ROOM((obj)),ROOM_PKILL) || Z_FLAGGED(IN_ROOM((obj)),Z_PKILL)))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub)&&PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj)?0:GET_INVIS_LEV(obj))) && IMM_CAN_SEE(sub, obj)))

/* End of CAN_SEE */


#define MOB_NO_LDESC(mob) (is_abbrev((mob)->player.long_descr, "INVIS\r\n"))

#define INVIS_OK_OBJ(sub, obj) \
  (!IS_OBJ_STAT((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) \
   ((!obj->carried_by || (CAN_SEE(sub, obj->carried_by)||sub==obj->carried_by)) &&      \
    (!obj->worn_by || (CAN_SEE(sub, obj->worn_by)||sub==obj->worn_by)))


#define MORT_CAN_SEE_OBJ(sub, obj) \
  ((LIGHT_OK(sub)||LIGHT_OBJ(sub,obj)) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))

#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub)&&PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))


#define PERS(ch, vict) (CAN_SEE(vict, ch) ? GET_NAME(ch) : ((GET_LEVEL(ch) < LVL_IMMORT) \
    ? "someone" : (IS_NPC(ch) ? "someone" : "an immortal")))

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
  (obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
  fname((obj)->name) : "something")


#define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door])
/* Following two defines added for Scan code */
#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door])
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])

#define CAN_GO(ch, door) (EXIT(ch,door) && \
       (EXIT(ch,door)->to_room != NOWHERE) && \
       !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
#define EXIT2(roomnum, door) (world[(roomnum)].dir_option[door])
#define CAN_GO2(roomnum, door) (EXIT2(roomnum, door) && \
          (EXIT2(roomnum, door)->to_room != NOWHERE) && \
     !IS_SET(EXIT2(roomnum,door)->exit_info, EX_CLOSED))
#define CAN_FLY(ch) (AFF_FLAGGED((ch),AFF_FLY)||trait_info[GET_RACE(ch)].can_fly)


#define RACE_ABBR(ch)  (race_abbrevs[(int)GET_RACE(ch)])
#define CLASS_ABBR(ch) (IS_SCR(ch) ? scr_class_abbrevs[(int)GET_CLASS(ch)] : class_abbrevs[(int)GET_CLASS(ch)])

#define IS_MAGIC_USER(ch)	(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_MAGIC_USER))
#define IS_CLERIC(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_CLERIC))
#define IS_THIEF(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_THIEF))
#define IS_WARRIOR(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_WARRIOR))
#define IS_RANGER(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_RANGER))
#define IS_BARD(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_BARD))
#define IS_MONK(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_MONK))
#define IS_UNUSED_CLASS(ch)	(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_UNUSED))
#define IS_BARBARIAN(ch)	(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_BARBARIAN))
#define IS_PALADIN(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_PALADIN))
#define IS_ANTI_PALADIN(ch)	(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_ANTI_PALADIN))
#define IS_DRUID(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_DRUID))
#define IS_MERCHANT(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_MERCHANT))
#define IS_KENSAI(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_KENSAI))
#define IS_ASSASSIN(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_ASSASSIN))
#define IS_NECROMANCER(ch)	(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_NECROMANCER))
#define IS_DEVA(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_DEVA))

#define IS_HUMAN(ch)            (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_HUMAN))
#define IS_ELF(ch)              (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_ELF))
#define IS_H_ELF(ch)            (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_H_ELF))
#define IS_D_ELF(ch)            (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_D_ELF))
#define IS_DWARF(ch)            (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_DWARF))
#define IS_HALFLING(ch)         (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_HALFLING))
#define IS_SPRITE(ch)           (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_SPRITE))
#define IS_MINOTAUR(ch)         (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_MINOTAUR))
#define IS_AVIAN(ch)            (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_AVIAN))
#define IS_H_OGRE(ch)           (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_H_OGRE))
#define IS_H_ORC(ch)            (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_H_ORC))
#define IS_DRAC(ch)             (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_DRACONIAN))
#define IS_SHADOW(ch)           (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_SHADOW))
#define IS_TITAN(ch)            (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_TITAN))
#define IS_AESIR(ch)            (!IS_NPC(ch) && \
         (GET_RACE(ch) == RACE_AESIR))

#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS))


#define IS_HUMANOID(ch)  (!IS_NPC(ch)||((GET_RACE(ch)<=(NUM_RACES))||\
          (GET_RACE(ch)==MRACE_DEMON)||\
          (GET_RACE(ch)==MRACE_UNDEAD)))
#define IS_UNDEAD(ch) (IS_NPC(ch) && ((GET_RACE(ch) == MRACE_UNDEAD)||\
              (GET_RACE(ch)==MRACE_GHOST)))
#define IS_ANIMAL(ch) (IS_NPC(ch) && (GET_RACE(ch) == MRACE_ANIMAL))
#define IS_DRAGON(ch) (IS_NPC(ch) && (GET_RACE(ch) == MRACE_DRAGON))


/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

#if defined(NOCRYPT) || !defined(HAVE_CRYPT)
#define CRYPT(a,b) (a)
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

#define SENDOK(ch) ((IS_NPC(ch) || (ch)->desc) && (AWAKE(ch)||to_sleeping) && \
                    (IS_NPC(ch)||!PLR_FLAGGED((ch), PLR_WRITING)))

/* This figures alignment differences into charisma */
/* Eventually it might figure in race modifications */
#define ADJUSTED_CHA(ch,tch) ( \
        MAX(0,MIN(35,(stat_index(GET_CHA(ch)))+ \
        (cha_align_table[(10+(int)((GET_ALIGNMENT(ch))/100))] \
        [(10+(int)((GET_ALIGNMENT(tch))/100))]))) \
)

/* Adjust shopkeeper prices */
#define PRICE_ADJ_CHA(ch,tch) (cha_app[ADJUSTED_CHA(ch,tch)].adj_price)
/* Charmie stuff */
#define CHARM_TIME_ADJ_CHA(ch,tch) (cha_app[ADJUSTED_CHA(ch,tch)].adj_num_charm-4)
#define REACT_ADJ_CHA(ch,tch) (cha_app[ADJUSTED_CHA(ch,tch)].adj_reactions)
#define NUM_CHARM_ADJ_CHA(ch,tch) (cha_app[ADJUSTED_CHA(ch,tch)].adj_num_charm)
