/* ************************************************************************
 *   File: spells.h                                      Part of CircleMUD *
 *  Usage: header file: constants and fn prototypes for spell system       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#define DEFAULT_STAFF_LVL	5
#define DEFAULT_WAND_LVL	5

#define CAST_UNDEFINED	-1
#define CAST_SPELL	0
#define CAST_POTION	1
#define CAST_WAND	2
#define CAST_STAFF	3
#define CAST_SCROLL	4
#define CAST_PILL       5 /* Pill modification--Aleks */
#define CAST_BREATH     6 /* still able to be done in a !MAGIC ROOM */

#define MAG_DAMAGE	(1 << 0)
#define MAG_AFFECTS	(1 << 1)
#define MAG_UNAFFECTS	(1 << 2)
#define MAG_POINTS	(1 << 3)
#define MAG_ALTER_OBJS	(1 << 4)
#define MAG_GROUPS	(1 << 5)
#define MAG_MASSES	(1 << 6)
#define MAG_AREAS	(1 << 7)
#define MAG_SUMMONS	(1 << 8)
#define MAG_CREATIONS	(1 << 9)
#define MAG_MANUAL	(1 << 10)
#define MAG_CHECK       (1 << 11)
#define MAG_MATERIALS   (1 << 12)
#define MAG_FORCEFUL    (1 << 13) /* nomikos addition 10-8-02 */

#define VIOLENT  1
#define NON_VIOLENT 0

#define TYPE_UNDEFINED               -1
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

#define SPELL_ARMOR                   1 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_TELEPORT                2 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLESS                   3 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLINDNESS               4 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BURNING_HANDS           5 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CALL_LIGHTNING          6 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHARM                   7 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHILL_TOUCH             8 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CLONE                   9 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_COLOR_SPRAY            10 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CONTROL_WEATHER        11 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_FOOD            12 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_WATER           13 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_BLIND             14 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_CRITIC            15 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_LIGHT             16 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURSE                  17 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_ALIGN           18 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_INVIS           19 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_MAGIC           20 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_POISON          21 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_EVIL            22 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EARTHQUAKE             23 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENCHANT_WEAPON         24 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENERGY_DRAIN           25 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FIREBALL               26 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HARM                   27 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL                   28 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INVISIBLE              29 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LIGHTNING_BOLT         30 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LOCATE_OBJECT          31 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MAGIC_MISSILE          32 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_POISON                 33 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_PROT_FROM_EVIL         34 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_CURSE           35 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SANCTUARY              36 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SHOCKING_GRASP         37 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SLEEP                  38 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STRENGTH               39 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SUMMON                 40 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VENTRILOQUATE          41 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WORD_OF_RECALL         42 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_POISON          43 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SENSE_LIFE             44 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ANIMATE_DEAD	     45 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_GOOD	     46 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_ARMOR	     47 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_HEAL	     48 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_RECALL	     49 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INFRAVISION	     50 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WATERWALK		     51 /* Reserved Skill[] DO NOT CHANGE */
/* What follows are non-stock spells.  Can have up to constant MAX_SPELLS */
#define SPELL_HASTE		     52 
/* This slot used to be Identify on Phoenix 3.0 */
#define SPELL_FERN		     53 
#define SPELL_SLOW		     54
#define SPELL_ACID_BLAST	     55
#define SPELL_FIRE_BREATH            56
#define SPELL_GAS_BREATH             57
#define SPELL_FROST_BREATH           58
#define SPELL_ACID_BREATH            59
#define SPELL_LIGHTNING_BREATH       60

#define SPELL_GROUP_INFRAVISION      61
#define SPELL_SHIELD                 62
#define SPELL_STONE_SKIN             63
#define SPELL_FLAME_STRIKE           64
#define SPELL_LEVITATE		     65
#define SPELL_DISPEL_MAGIC           66
#define SPELL_DRAGON                 67
#define SPELL_PIXIE_DUST             68
#define SPELL_INSPIRE                69
#define SPELL_DREAM_SIGHT            70
#define SPELL_ENCHANT_ARMOR          71
#define SPELL_GROUP_REFRESH          72
#define SPELL_REFRESH                73
#define SPELL_GIVE_LIFE              74
#define SPELL_BARK_SKIN              75
#define SPELL_DEPRESSION             76
#define SPELL_LULLABY                77
#define SPELL_BLUR                   78
#define SPELL_EAGLE_CLAW             79
#define SPELL_ENFEEBLE               80
#define SPELL_FIRE_SONG              81
#define SPELL_WRATH_OF_GOD           82
#define SPELL_WITHER                 83
#define SPELL_PURIFY                 84
#define SPELL_WATER_BREATHE          85
#define SPELL_ENHANCED               86
#define SPELL_GATE                   87
#define SPELL_GAS_BLAST              88
#define SPELL_FROST_BLAST            89
#define SPELL_PLAGUE                 90
#define SPELL_PASS_DOOR              91
#define SPELL_CALM                   92
#define SPELL_METEOR_STORM           93
#define SPELL_ICE_STORM              94
#define SPELL_CHANGE_SEX             95
#define SPELL_CURE_PLAGUE            96
#define SPELL_SUNBURN                97
#define SPELL_CURE_SERIOUS           98
#define SPELL_ENERGY                 99
#define SPELL_GROUP_SANC             100
#define SPELL_GROUP_LEVITATE         101
#define SPELL_CREATE_LIGHT           102
#define SPELL_CONTINUAL_LIGHT        103
#define SPELL_PORTAL		     104
#define SPELL_IDENTIFY               105
#define SKILL_BACKSTAB               106 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BASH                   107 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HIDE                   108 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_KICK                   109 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PICK_LOCK              110 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PUNCH                  111 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_RESCUE                 112 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SNEAK                  113 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_STEAL                  114 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_TRACK		     115 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_MOUNT		     116 /* // Mounting (DAK) */
#define SKILL_RIDING		     117 /* // Riding (DAK) */
#define SKILL_TAME		     118 /* // Ability to tame (DAK) */
#define SKILL_SECOND_ATTACK          119 /* Change ordering as necessary */
#define SKILL_THIRD_ATTACK           120
#define SKILL_FOURTH_ATTACK          121
#define SKILL_RAGE                   122
#define PROF_THROW                   123   
#define PROF_BOW                     124
#define PROF_SLING                   125
#define PROF_CROSSBOW                126
#define SKILL_DUAL_WIELD             127
#define SKILL_REPAIR                 128
#define SPELL_FLY                    129


#define PROF_FISTICUFFS              130 /* hit */
#define PROF_SWORD                   131 /* slashing */
#define PROF_2H_SWORD                132 /* slash 2h */
#define PROF_DAGGER                  133 /* pierce */
#define PROF_CLUB                    134 /* bludgeon */
#define PROF_2H_CLUB                 135 /* bludgeon 2h */
#define PROF_HAMMER                  136 /* smite */
#define PROF_2H_HAMMER               137 /* smite 2h */
#define PROF_AXE                     138 /* cleave */
#define PROF_2H_AXE                  139 /* 2h cleave */
#define PROF_SPEAR                   140 /* stab 2h pierce  */
#define PROF_WHIP                    141 /* whip (not in yet) */
#define PROF_CLAW                    142 /* claw (not in yet) */

#define SKILL_BREW                   143
#define SKILL_SCRIBE                 144

#define SPELL_KNOCK                  145
#define SPELL_WIZARDLOCK             146
#define SKILL_AMBUSH                 147
#define SKILL_TRIP                   148
#define SKILL_SWEEP                  149
#define SKILL_QUIVERING_PALM         150
#define SKILL_STOMP                  151
#define SKILL_HEADBUTT               152
#define SKILL_DISARM                 153
#define SKILL_BERSERK                154
#define SKILL_CIRCLE                 155
#define SKILL_SHOCK                  156
#define SKILL_STUN                   157
#define SKILL_SHADOW                 158
#define SKILL_CAMOUFLAGE             159
#define SKILL_PALM                   160
#define SKILL_LAY_HANDS              161
#define SKILL_BANDAGE                162
#define SKILL_DARKEN                 163
#define SKILL_LIGHTEN                164
#define SKILL_CHANT                  165
#define SKILL_MEDITATE               166
#define SKILL_DODGE                  167
#define SKILL_BLOCK                  168
#define SKILL_PARRY                  169
#define SPELL_CONJURE_INFANTRY       170
#define SPELL_WEB                    171
#define SKILL_GORE                   172
#define SPELL_FAERIE_FIRE            173
#define SPELL_PROT_FIRE              174
#define SPELL_PROT_COLD              175
#define SPELL_PROT_ELEC              176
#define SPELL_PROT_ENERGY            177
#define SPELL_PROT_ACID              178
#define SPELL_PROT_POISON            179
#define SPELL_PROT_DRAIN             180
#define SPELL_PROT_FROM_GOOD         181
#define SPELL_SUNRAY                 182
#define SKILL_REDIRECT               183
#define SPELL_CAUSE_LIGHT            184
#define SPELL_CAUSE_SERIOUS          185
#define SPELL_CAUSE_CRITIC           186
#define SPELL_ATONEMENT              187
#define SPELL_SUMMON_MOUNT           188
#define SPELL_GOODBERRY              189
#define SKILL_READ_MAGIC             190
#define SPELL_CLAN_RECALL            191
#define SPELL_ENTANGLE               192
#define SPELL_GRANT_PEACE            193
#define SKILL_DIG                    194
#define SKILL_SKIN                   195
#define SPELL_FIRESHIELD             196
#define SPELL_DROWN                  197
#define SPELL_ENLIVEN                198
#define SPELL_FEAR                   199
#define SKILL_MOUNTED_ATTACK         200
#define SKILL_FISHING                201
#define SPELL_GROUP_SUMMON           202
#define SKILL_ROVE                   203
#define SKILL_PECK                   204
#define SKILL_GUARD                  205
#define SKILL_TIGER_STANCE 207 /* 4.2 monk stance: bare hands claw (slash) */
#define SKILL_MANTIS_STANCE 208 /* 4.2 monk stance: bare hands stab (pierce) */
/*
 * ZEAL (4.2) -- the activity-reward affect. Not castable by anything: no
 * class table lists it and no guild teaches it. It arrives by quaffing a
 * potion, and grants a flat experience bonus for its duration.
 *
 * 206 is the LAST id at or below MAX_SPELLS. spelledit's loader exits on
 * nr > MAX_SPELLS, and spells and skills share one id space (top used was
 * 205), so this is the only free slot that loads. Adding another needs
 * MAX_SPELLS raised, which widens the listing loops in act.informative.c
 * and act.other.c -- output-neutral but bench-visible, so it is a change
 * to make deliberately rather than in passing.
 */
#define SPELL_ZEAL                   206

/*
 * How long a quaffed zeal lasts, in COMBAT ticks (is_combat_buff), and what
 * it multiplies experience by. Here rather than in magic.c because gain_exp
 * (limits.c) reads the bonus and mag_affects reads the duration.
 */
#define ZEAL_COMBAT_TICKS            60
#define ZEAL_EXP_BONUS               1.10f

#define MAX_SPELLS 209

#define TOP_SPELL_DEFINE	     600
/* NEW NPC/OBJECT SPELLS (non saved) can be inserted here up to 599 */


/* WEAPON ATTACK TYPES */

#define TYPE_HIT                     600 /*  */
#define TYPE_BLUDGEON		     601 /*  */
#define TYPE_PIERCE		     602 /*  */
#define TYPE_SLASH                   603 /*  */
#define TYPE_BLAST		     604
#define TYPE_WHIP                    605 /*  */
#define TYPE_PIERCE_NO_BS            606 /*  */
#define TYPE_CLAW                    607 /*  */
#define TYPE_BITE                    608
#define TYPE_STING                   609
#define TYPE_CLEAVE                  610 /*  */
#define TYPE_POUND                   611 /*  */
#define TYPE_MAUL                    612 /*  */
#define TYPE_THRASH                  613 /*  */
#define TYPE_PUNCH		     614
#define TYPE_STAB		     615 /*  */
#define TYPE_MAXWEP                  615 /* always the max wepon type */
/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING		     699




#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4


#define TAR_IGNORE        (1<<0)
#define TAR_CHAR_ROOM     (1<<1)
#define TAR_CHAR_WORLD    (1<<2)
#define TAR_FIGHT_SELF    (1<<3)
#define TAR_FIGHT_VICT    (1<<4)
#define TAR_SELF_ONLY     (1<<5)/* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF      (1<<6)/* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV       (1<<7)
#define TAR_OBJ_ROOM      (1<<8)
#define TAR_OBJ_WORLD     (1<<9)
#define TAR_OBJ_EQUIP     (1<<10)
#define TAR_DOOR          (1<<11)

#define IS_UNUSED 0
#define IS_SPELL  1
#define IS_SKILL  2

struct spell_info_type {
   char *spell_name;
   int mana_max;	/* Max amount of mana used by a spell (lowest lev) */
   int mana_min;	/* Min amount of mana used by a spell (highest lev) */
   int mana_change;	/* Change in mana used by spell from lev to lev */
   byte min_position;	/* Position for caster	 */
   int targets;         /* See below for use with TAR_XXX  */
   byte violent;
   int routines;
   int min_level[NUM_CLASSES];
   int is_spell;
   int cast_time;
   char *wear_off;
};

struct race_skills_struct {
   int race;
   int spell_num;
   int level;
};

struct mat_components_type {
   int spell_num;
   int item0;
   int wear0;
   int item1;
   int wear1;
   int item2;
   int wear2;
   bool verbose;
};

#define EXTRACT   -1
#define KEEP_ITEM  0

/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4

#define USE_FAIL           1
#define USE_PASS           2
#define AUTO_FAIL          3
#define AUTO_PASS          4
#define PROF_FAIL          5
#define PROF_PASS          6

/* Attacktypes with grammar */

struct attack_hit_type {
   char	*singular;
   char	*plural;
};


#define ASPELL(spellname) \
int	spellname(int level, struct char_data *ch, \
		  struct char_data *victim, struct obj_data *obj, \
                  struct room_direction_data *dr,\
		  struct room_direction_data *dr2)

#define MANUAL_SPELL(spellname) spellname(level, caster, cvict, ovict,\
					      dvict, dvict2);

ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_crecall);
ASPELL(spell_teleport);
ASPELL(spell_portal);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);
ASPELL(spell_gate);
ASPELL(spell_enchant_armor);
ASPELL(spell_knock);
ASPELL(spell_wizardlock);
ASPELL(spell_energy_drain);

/* basic magic calling functions */

int find_skill_num(char *name);

int mag_materials(struct char_data * ch, int spellnum,int level);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
		int spellnum, int savetype);

int mag_check(int level, struct char_data *ch, struct char_data *victim,
	       struct obj_data *ovict, int spellnum, int savetype);

/* 4.2: may the carrier drop this affect at will? Defined in magic.c beside
 * is_combat_buff so the two lists stay adjacent. */
int is_removable_buff(int type);
/* Shared with spell_parser.c: buff cast time follows the practice level. */
int is_combat_buff(int spellnum);

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
		 int spellnum, int savetype);

void mag_groups(int level, struct char_data *ch, int spellnum, int savetype);

void mag_masses(int level, struct char_data *ch, int spellnum, int savetype);

void mag_areas(int level, struct char_data *ch, int spellnum, int savetype);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
		 int spellnum, int savetype);

void mag_points(int level, struct char_data *ch, struct char_data *victim,
		int spellnum, int savetype);

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
		   int spellnum, int type);

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
		    int spellnum, int type);

void mag_creations(int level, struct char_data *ch, int spellnum);

void mag_forceful(int level, struct char_data *ch, struct char_data *victim,
                 int spellnum, int savetype);

int	call_magic(struct char_data *caster, struct char_data *cvict,
		   struct obj_data *ovict, struct room_direction_data *dvict,
		   struct room_direction_data *dvict2,
		   int spellnum, int level, int casttype);

void	mag_objectmagic(struct char_data *ch, struct obj_data *obj,
			char *argument);

int	cast_spell(struct char_data *ch, struct char_data *tch,
		   struct obj_data *tobj, struct room_direction_data *tdr, 
		   struct room_direction_data *tdr2, 
		   int spellnum, int cast_level);


/* other prototypes */
void spell_level(int spell, int class, int level);
void init_spell_levels(void);
char *skill_name(int num);
int  improve_skill(struct char_data *ch, int skill, int passcheck);
int mag_manacost(struct char_data * ch, int spellnum, int cast_level) ;
int mag_savingthrow(struct char_data * ch, int type,int level,int modifier);
