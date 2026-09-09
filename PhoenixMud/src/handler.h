/* ************************************************************************
*   File: handler.h                                     Part of CircleMUD *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* handling the affected-structures for chars */
void	affect_total(struct char_data *ch);
void	affect_modify(struct char_data *ch, byte loc, long mod, 
		      bitvector_t bitv, bool add);
void	affect_to_char(struct char_data *ch, struct affected_type *af);
void	affect_remove(struct char_data *ch, struct affected_type *af);
void    affect_remove_all(struct char_data *ch);
void	affect_from_char(struct char_data *ch, int type);
bool	affected_by_spell(struct char_data *ch, int type);
/*
 * Declared because magic.c calls it. Without this the compiler assumes a
 * function returning int, and the returned pointer survives only because the
 * build is -m32 and int and pointer are both 32 bits there. It would truncate
 * on any 64-bit build.
 */
struct affected_type* get_affected_by_spell(struct char_data *ch, int type);
void	affect_join(struct char_data *ch, struct affected_type *af,
		    bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);

/* handling the affected-structures for rooms */
void affect_modify_room(struct room_data *rm, byte loc, long mod, 
			bitvector_t bitv, bool add);
void affect_total_room(struct room_data *rm);
void affect_to_room(struct room_data *rm, struct room_affected_type * af);
void affect_remove_room(struct room_data *rm, struct room_affected_type * af);
void affect_from_room(struct room_data *rm, int type);
bool affected_by_spell_room(struct room_data *rm, int type);
void affect_join_room(struct room_data *rm, struct room_affected_type * af, 
		      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);

/* utility */
char *money_desc(int amount);
struct obj_data *create_money(int amount);
int	isname(const char *str, const char *namelist);
int	isname_all(const char *str, const char *namelist);
char	*fname(char *namelist);
int	get_number(char **name);

/* ******** objects *********** */

void	obj_to_char(struct obj_data *object, struct char_data *ch);
void	obj_from_char(struct obj_data *object);

void	equip_char(struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(struct char_data *ch, int pos);
struct obj_data *unequip_char_inner(struct char_data *ch, int pos);
int    invalid_align(struct char_data *ch, struct obj_data *obj);

struct obj_data *get_obj_in_list(char *name, struct obj_data *list);
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj(char *name);
struct obj_data *get_obj_num(obj_rnum nr);

void	obj_to_room(struct obj_data *object, room_rnum room);
void	obj_from_room(struct obj_data *object);
void	obj_to_objt(struct obj_data *obj, struct obj_data *obj_to,const char *func, const int line);
#define	obj_to_obj(obj, obj_to) obj_to_objt((obj),(obj_to),__FUNCTION__,__LINE__)
void	obj_from_obj(struct obj_data *obj);
void	object_list_new_owner(struct obj_data *list, struct char_data *ch);

void	extract_obj(struct obj_data *obj);
void    scrap_item(struct obj_data *obj, struct char_data *ch);

int     can_take_obj(struct char_data * ch, struct obj_data * obj);

/* ******* characters ********* */

struct char_data *get_char_room(char *name, room_rnum room);
struct char_data *get_char_num(mob_rnum nr);
struct char_data *get_char(char *name);

int get_char_gold(struct char_data *ch);
int charge_char_gold(struct char_data *ch, int ammount);

void	char_from_room(struct char_data *ch);
void	char_to_room(struct char_data *ch, room_rnum room);
void    char_to_object(struct char_data * ch, struct obj_data *obj) ;
void    char_from_object(struct char_data * ch, struct obj_data * obj);
void	extract_char(struct char_data *ch);
void set_racial_traits(struct char_data *ch);
void reset_racial_traits(struct char_data *ch);
/* find if character can see */
struct room_direction_data *get_room_dir(struct room_data * rd, char *name,
					 struct char_data *ch);
struct room_direction_data *get_other_room_dir(struct room_data *rd,char *name,
					       struct char_data *ch);
struct char_data *get_char_room_vis(struct char_data *ch, char *name);
struct char_data *get_player_vis(struct char_data *ch, char *name, int inroom);
struct char_data *get_char_vis(struct char_data *ch, char *name,int location);
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, 
				     struct obj_data *list);
struct obj_data *get_obj_vis(struct char_data *ch, char *name);
struct obj_data *get_object_in_equip_vis(struct char_data *ch,
					 char *arg, 
					 struct obj_data *equipment[], int *j);


/* find all dots */

int	find_all_dots(char *arg);

#define FIND_INDIV	0
#define FIND_ALL	1
#define FIND_ALLDOT	2


/* Generic Find */

int	generic_find(char *arg, bitvector_t bitvector, struct char_data *ch,
		     struct char_data **tar_ch, struct obj_data **tar_obj);

#define FIND_CHAR_ROOM     (1 << 0)
#define FIND_CHAR_WORLD    (1 << 1)
#define FIND_OBJ_INV       (1 << 2)
#define FIND_OBJ_ROOM      (1 << 3)
#define FIND_OBJ_WORLD     (1 << 4)
#define FIND_OBJ_EQUIP     (1 << 5)


/* prototypes from crash save system */

int	Crash_get_filename(char *orig_name, char *filename);
int	Crash_delete_file(char *name);
int	Crash_delete_crashfile(struct char_data *ch);
int	Crash_clean_file(char *name);
void	Crash_listrent(struct char_data *ch, char *name);
int	Crash_load(struct char_data *ch);
void	Crash_crashsave(struct char_data *ch);
void	Crash_idlesave(struct char_data *ch);
void	Crash_save_all(void);
int     Crash_load_xapobjs(struct char_data *ch);

/* prototypes from fight.c */
void	set_fighting(struct char_data *ch, struct char_data *victim);
void	stop_fighting(struct char_data *ch);
void	hit(struct char_data *ch, struct char_data *victim, int type);
void	forget(struct char_data *ch, struct char_data *victim);
void	remember(struct char_data *ch, struct char_data *victim);
int	damage(struct char_data *ch, struct char_data *victim, int dam, 
	       int attacktype, int imm_type);
int	skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype);
