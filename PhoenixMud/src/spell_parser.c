/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD * 
*  Usage: top-level magic routines; outside points of entry to magic sys. * 
*                                                                         * 
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         * 
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University * 
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
************************************************************************ */


#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"


#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "vnum.h"

extern struct spell_info_type *spells;
int valid_class_align(struct char_data *ch);
int find_race_skill(int race,int spell_num);
const char *unused_spellname = "!UNUSED!";
extern struct room_data *world;
extern int spell_sort_info[];
extern struct zone_data *zone_table;
extern struct index_data *mob_index;
extern SPECIAL(shop_keeper);

#define SINFO spells[spellnum]
#define UU LVL_IMMORT
#define XX LVL_ADMIN
 /*=
      {
         {"!RESERVED!",0,0,0,0,0,0,0,
         {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
         IS_UNUSED,5}
      ,
      {"armor",  30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU, 8,UU,10,UU,12,UU,UU,UU,15,15,UU,UU,15, 8, 9, 4,UU,UU},
       IS_SPELL,5},

      {"teleport", 80,50,3, POS_STANDING, TAR_SELF_ONLY | TAR_CHAR_ROOM, NON_VIOLENT, MAG_MANUAL,
       {UU,UU,UU,62,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,43,UU,UU,UU},
       IS_SPELL,5},

      {"bless",  30, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, NON_VIOLENT, MAG_AFFECTS | MAG_ALTER_OBJS,
       {UU, 2,UU,UU,UU,UU,15,UU,UU,18,UU,10,UU,UU,UU,UU, 2,UU,UU},
       IS_SPELL,5},

      {"blindness",  30, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,32,UU,UU,UU,UU,UU,UU,38,UU,UU,UU,29,25,UU,UU,UU},
       IS_SPELL,5},

      {"burning hands",  30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,14,UU,16,22,UU,UU,UU,UU,UU,UU,UU,11,11,UU,UU,UU},
       IS_SPELL,5},

      {"call lightning",  40, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE|MAG_CHECK,
       {UU,46,UU,UU,UU,UU,UU,UU,UU,UU,UU,35,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"charm person",  75, 50, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, VIOLENT, MAG_MANUAL,
       {UU,UU,UU,45,UU,55,UU,UU,UU,UU,UU,UU,UU,UU,UU,35,45,UU,UU},
       IS_SPELL,5},

      {"chill touch",  30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE | MAG_AFFECTS,
       {UU,UU,UU, 7,UU,15,UU,UU,UU,UU,20,UU,UU,UU,21, 5,UU,UU,UU},
       IS_SPELL,5},

      {"clone", 80, 65, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, NON_VIOLENT, MAG_SUMMONS,
       {UU,UU,UU,73,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,60,UU,UU,UU},
       IS_SPELL,5},

      {"color spray", 30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,35,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,31,UU,UU,UU,UU},
       IS_SPELL,5},

      {"control weather",  75, 25, 5, POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_MANUAL,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_UNUSED,5},

      {"create food",  30, 5, 4, POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_CREATIONS,
       {UU,10,UU,UU,10,UU, 6,UU,UU,UU,UU, 5,UU,UU,UU,UU, 8,UU,UU},
       IS_SPELL,5},

      {"create water",  30, 5, 4, POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_CREATIONS,
       {UU,10,UU,UU,12,UU, 6,UU,UU,UU,UU, 5,UU,UU,UU,UU, 8,UU,UU},
       IS_SPELL,5},

      {"cure blind", 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_UNAFFECTS,
       {UU,23,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,17,UU,UU},
       IS_SPELL,5},

      {"cure critic",  50, 30, 2, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_POINTS,
       {UU,35,UU,UU,UU,UU,45,UU,UU,UU,UU,59,UU,UU,UU,UU,30,UU,UU},
       IS_SPELL,5},

      {"cure light",  30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_POINTS,
       {UU, 1,UU,UU,40,UU,10,UU,UU, 9,19,18,UU,21,UU,UU, 1,UU,UU},
       IS_SPELL,5},

      {"curse", 80, 50, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV|TAR_OBJ_ROOM, VIOLENT, MAG_AFFECTS | MAG_ALTER_OBJS,
       {UU,UU,UU,47,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,36,UU,UU,UU},
       IS_SPELL,5},

      {"detect alignment",  20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, NON_VIOLENT, MAG_AFFECTS,
       {UU,27,UU,UU,UU,UU, 9,UU,UU,17,17, 8,UU,UU,UU,UU,12,UU,UU},
       IS_SPELL,5},

      {"detect invisibility",  20, 10, 2, POS_STANDING, TAR_CHAR_ROOM , NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU, 8,UU,10,UU,UU,UU,UU,UU, 9,UU,UU,10, 4,UU,UU,UU},
       IS_SPELL,5},

      {"detect magic",  20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU, 6,UU, 8,UU,UU,UU,UU,UU,UU,UU,UU,UU, 3,UU,UU,UU},
       IS_SPELL,5},

      {"detect poison",  15, 5, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, NON_VIOLENT, MAG_MANUAL,
       {UU,24,UU,UU,30,UU,UU,UU,UU,20,UU,11,UU,UU,UU,UU,11,UU,UU},
       IS_SPELL,5},

      {"dispel evil",  40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,34,UU,UU,UU,UU,UU,UU,UU,52,UU,25,UU,UU,UU,UU,19,UU,UU},
       IS_SPELL,5},

      {"earthquake", 40, 25, 3, POS_FIGHTING, TAR_IGNORE, VIOLENT, MAG_AREAS,
       {UU,31,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,25,UU,UU},
       IS_SPELL,5},

      {"enchant weapon",  150, 100, 10, POS_STANDING, TAR_OBJ_INV |TAR_OBJ_ROOM, NON_VIOLENT, MAG_MANUAL,
       {UU,UU,UU,23,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,12,18,UU,UU},
       IS_SPELL,15},

      {"energy drain", 25, 10, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_MANUAL,
       {UU,UU,UU,42,UU,UU,UU,UU,UU,UU,46,UU,UU,UU,UU,27,UU,UU,UU},
       IS_SPELL,5},

      {"fireball", 30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,46,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,41,33,UU,UU,UU},
       IS_SPELL,5},

      {"harm",  75, 45, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,76,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"heal",  60, 40, 3, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_POINTS | MAG_UNAFFECTS,
       {UU,48,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,36,UU,UU},
       IS_SPELL,5},

      {"invisibility",  35, 25, 1, POS_STANDING, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM,NON_VIOLENT,MAG_AFFECTS|MAG_ALTER_OBJS,
       {UU,UU,UU,12,UU,14,UU,UU,UU,UU,UU,19,UU,UU,UU, 9,UU,UU,UU},
       IS_SPELL,5},

      {"lightning bolt", 30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,28,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,15,UU,UU,UU},
       IS_SPELL,5},

      {"locate object", 25, 20, 1, POS_STANDING, TAR_OBJ_WORLD, NON_VIOLENT, MAG_MANUAL,
       {UU,38,UU,18,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,14,29,UU,UU},
       IS_SPELL,5},

      {"magic missile", 20, 10, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU, 1,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU, 1,UU,UU,UU},
       IS_SPELL,5},

      {"poison",  50, 20, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, VIOLENT, MAG_AFFECTS | MAG_ALTER_OBJS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,40,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"ward evil",  40, 10, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,33,UU,UU},
       IS_SPELL,5},

      {"remove curse", 45, 25, 5, POS_STANDING, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_EQUIP, NON_VIOLENT, MAG_UNAFFECTS|MAG_ALTER_OBJS,
       {UU,47,UU,UU,UU,UU,UU,UU,UU,55,UU,UU,UU,UU,UU,UU,35,UU,UU},
       IS_SPELL,5},

      {"sanctuary", 110, 85, 5, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,54,UU,UU,UU,71,88,UU,UU,UU,UU,UU,UU,UU,UU,UU,44,UU,UU},
       IS_SPELL,5},

      {"shocking grasp",  30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,21,UU,23,32,UU,UU,UU,UU,UU,UU,UU,UU,16,UU,UU,UU},
       IS_SPELL,5},

      {"sleep", 40, 25, 5, POS_STANDING, TAR_CHAR_ROOM, VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,43,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,35,UU,UU,UU},
       IS_SPELL,5},

      {"strength", 35, 30, 1, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,20,UU,25,UU,UU,UU,UU,UU,UU,UU,UU,UU,16,UU,UU,UU},
       IS_SPELL,5},

      {"summon", 50, 20, 6, POS_STANDING, TAR_CHAR_WORLD | TAR_NOT_SELF, NON_VIOLENT, MAG_MANUAL,
       {UU,30,UU,40,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,30,20,UU,UU},
       IS_SPELL,5},

      {"ventriloquate", 0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_UNUSED,5},

      {"word of recall",  20, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_MANUAL,
       {UU,17,UU,48,UU,UU,UU,UU,UU,UU,UU,17,UU,UU,UU,27,17,UU,UU},
       IS_SPELL,5},

      {"remove poison", 40, 8, 4, POS_STANDING, TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM,NON_VIOLENT,MAG_UNAFFECTS|MAG_ALTER_OBJS,
       {UU,36,UU,UU,UU,UU,UU,UU,UU,23,UU,UU,UU,UU,UU,UU,24,UU,UU},
       IS_SPELL,5},

      {"sense life", 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM|TAR_SELF_ONLY, NON_VIOLENT, MAG_AFFECTS,
       {UU,28,UU,UU,35,UU,UU,UU,UU,UU,UU,30,UU,UU,10,UU,19,UU,UU},
       IS_SPELL,5},

      {"animate dead", 70, 20, 6,POS_STANDING, TAR_OBJ_ROOM, NON_VIOLENT, MAG_SUMMONS|MAG_CHECK,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,17,UU,UU,UU},
       IS_SPELL,5},

      {"dispel good", 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,60,29,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"group armor", 50, 30, 2, POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_GROUPS,
       {UU,60,UU,36,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,24,50,UU,UU},
       IS_SPELL,8},

      {"group heal", 80, 60, 5, POS_FIGHTING, TAR_IGNORE, NON_VIOLENT, MAG_GROUPS,
       {UU,65,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,55,UU,UU},
       IS_SPELL,8},

      {"group recall", 50,20,5,POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_GROUPS,
       {UU,61,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,48,UU,UU},
       IS_SPELL,8},

      {"infravision",  25, 10, 1, POS_STANDING, TAR_CHAR_ROOM|TAR_SELF_ONLY, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,30,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,17,14,UU,UU},
       IS_SPELL,5},

      {"waterwalk",60,40,4,POS_STANDING, TAR_CHAR_ROOM,NON_VIOLENT,MAG_AFFECTS,
       {UU,29,UU, 4,UU,UU,UU,UU,UU,UU,UU,25,UU,UU,UU, 5,18,UU,UU},
       IS_SPELL,5},

      {"haste",  80, 30, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,56,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,41,UU,UU,UU},
       IS_SPELL,5},

      {"fern", 2000,1950,5, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_UNUSED,5},

      {"slow",  60, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,35,55,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,27,25,UU,UU},
       IS_SPELL,5},

      {"acid blast",  60, 40, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,58,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,38,UU,UU,UU},
       IS_SPELL,5},

      {"fire breath",  0,0,0, POS_FIGHTING, TAR_IGNORE, VIOLENT, MAG_AREAS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"gas breath", 0,0,0, POS_FIGHTING, TAR_IGNORE, VIOLENT, MAG_AREAS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"frost breath",  0,0,0, POS_FIGHTING, TAR_IGNORE, VIOLENT, MAG_AREAS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"acid breath", 0,0,0, POS_FIGHTING, TAR_IGNORE, VIOLENT, MAG_AREAS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"lightning breath", 0,0,0, POS_FIGHTING, TAR_IGNORE, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"group infravision",  50, 30, 2, POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_GROUPS,
       {UU,UU,UU,44,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,31,UU,UU,UU},
       IS_SPELL,8},

      {"shield",  70, 40, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,42,UU,UU,UU,UU,UU,UU,UU,UU,UU,45,38,28,UU,UU,UU},
       IS_SPELL,5},

      {"stone skin",  80, 40, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS|MAG_CHECK,
       {UU,UU,UU,UU,55,UU,UU,UU,UU,UU,UU,40,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"flame strike",  60, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,56,UU,UU,UU,UU,UU,UU,UU,UU,UU,67,UU,UU,UU,UU,45,UU,UU},
       IS_SPELL,5},

      {"levitate",  60, 20, 5, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,26,UU,27,28,UU,UU,UU,UU,UU,UU,UU,UU,18,UU,UU,UU},
       IS_SPELL,5},

      {"dispel magic",  80, 40, 2, POS_STANDING, TAR_CHAR_ROOM|TAR_NOT_SELF, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,34,UU,47,UU,UU,UU,UU,UU,UU,UU,UU,UU,22,UU,UU,UU},
       IS_SPELL,5},

      {"dragon", 60, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS|MAG_CHECK,
       {UU,UU,UU,75,UU,86,UU,UU,UU,UU,UU,UU,UU,UU,UU,65,UU,UU,UU},
       IS_SPELL,5},

      {"pixie dust",  60, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM|TAR_FIGHT_VICT, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,39,UU,UU,UU,UU,UU,UU,UU,29,UU,UU,UU,29,UU,UU,UU},
       IS_SPELL,5},

      {"inspire", 60, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS|MAG_CHECK,
       {UU,UU,UU,UU,UU,57,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"dream sight",  60, 20, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,15,UU,35,UU,UU,UU,UU,UU,32,UU,UU,UU, 9,UU,UU,UU},
       IS_SPELL,5},

      {"enchant armor",  150, 100, 5, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_ROOM, NON_VIOLENT, MAG_MANUAL,
       {UU,UU,UU,26,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,14,19,UU,UU},
       IS_SPELL,15},

      {"group refresh", 40, 20, 4, POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_GROUPS,
       {UU,55,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,42,UU,UU},
       IS_SPELL,8},

      {"refresh",  30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_POINTS,
       {UU,18,UU,UU,UU,UU,23,UU,UU,40,57,UU,UU,UU,UU,UU,10,UU,UU},
       IS_SPELL,5},

      {"give life",  0, 0, 0, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_POINTS,
       {UU,80,UU,UU,UU,UU,70,UU,UU,65,UU,UU,UU,UU,UU,UU,62,UU,UU},
       IS_SPELL,5},

      {"bark skin",  80, 40, 5, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS|MAG_CHECK,
       {UU,UU,UU,UU,25,UU,UU,UU,UU,UU,UU,15,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"depression",  50, 30, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT,VIOLENT,MAG_AFFECTS,
       {UU,UU,UU,UU,UU,50,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"lullaby",  50, 20, 2, POS_STANDING, TAR_CHAR_ROOM, VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU, 6,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"blur", 60, 40, 2, POS_STANDING, TAR_CHAR_ROOM,NON_VIOLENT,MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,43,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"eagle claw",  80, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS|MAG_CHECK,
       {UU,UU,UU,UU,65,UU,UU,UU,UU,UU,UU,55,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"enfeeble",  60, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,UU,UU,58,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"fire song",  60, 40, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,UU,UU,33,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"wrath of god",  80, 40, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,75,UU,UU,UU,UU,90,UU,UU,85,UU,UU,UU,UU,UU,UU,66,UU,UU},
       IS_SPELL,5},

      {"wither", 60, 40, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT,VIOLENT,MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,48,UU,UU,UU,UU,UU,UU,UU,UU,UU,64,UU,UU},
       IS_SPELL,5},

      {"purify", 60, 20, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, NON_VIOLENT, MAG_UNAFFECTS,
       {UU,UU,UU,UU,UU,40,36,UU,UU,UU,UU,UU,UU,UU,UU,UU,27,UU,UU},
       IS_SPELL,5},

      {"water breathe",  60, 40, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,32,UU,29,28,UU,UU,UU,UU,UU,UU,26,UU,UU,UU,15,21,UU,UU},
       IS_SPELL,5},

      {"enhanced strength",  60, 20, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,53,UU,72,UU,UU,UU,UU,UU,UU,UU,UU,UU,40,UU,UU,UU},
       IS_SPELL,5},

      {"gate",  60, 40, 5, POS_STANDING, TAR_NOT_SELF| TAR_CHAR_WORLD, NON_VIOLENT, MAG_MANUAL,
       {UU,60,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,50,UU,52,UU,UU},
       IS_SPELL,5},

      {"gas blast", 60, 40, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,66,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,45,UU,UU,UU},
       IS_SPELL,5},

      {"frost blast", 60, 40, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,54,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,35,UU,UU,UU},
       IS_SPELL,5},

      {"plague",20,10,1,POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, NON_VIOLENT,MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"pass door", 0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_UNUSED,5},

      {"calm", 0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_UNUSED,5},

      {"meteor storm",70, 40, 2, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS,
       {UU,UU,UU,50,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,39,UU,UU,UU},
       IS_SPELL,5},

      {"ice storm", 90, 60, 2, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS,
       {UU,UU,UU,72,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,51,UU,UU,UU},
       IS_SPELL,5},

      {"change sex",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_UNUSED,5},

      {"cure plague",  30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_UNAFFECTS,
       {UU,UU,UU,UU,UU,UU,65,UU,UU,UU,UU,UU,UU,UU,UU,UU,46,UU,UU},
       IS_SPELL,5},

      {"sunburn", 0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_UNUSED,5},

      {"cure serious", 50,20, 2, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_POINTS,
       {UU,29,UU,UU,UU,UU,30,UU,UU,29,UU,38,UU,58,UU,UU,14,UU,UU},
       IS_SPELL,5},

      {"energy regeneration", 50,20,3,POS_FIGHTING, TAR_CHAR_ROOM,NON_VIOLENT,MAG_POINTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"group sanctuary", 120,90,5,POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_GROUPS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,80,UU,UU},
       IS_SPELL,8},

      {"group levitate", 60,40,5,POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_GROUPS,
       {UU,UU,UU,45,UU,62,UU,UU,UU,UU,UU,UU,UU,UU,UU,32,UU,UU,UU},
       IS_SPELL,8},

      {"create light", 30,10,3,POS_STANDING, TAR_IGNORE,NON_VIOLENT,MAG_CREATIONS,
       {UU,15,UU, 1,UU, 2,UU,UU,UU,UU,UU,12,UU,UU,UU, 1,12,UU,UU},
       IS_SPELL,5},

      {"continual light",50,10,5,POS_STANDING, TAR_IGNORE,NON_VIOLENT,MAG_CREATIONS,
       {UU,39,UU,37,UU,60,UU,UU,UU,UU,UU,UU,UU,UU,UU,28,28,UU,UU},
       IS_SPELL,5},

      {"portal", 130,70,5,POS_STANDING, TAR_CHAR_WORLD|TAR_NOT_SELF,NON_VIOLENT,MAG_MANUAL,
       {UU,UU,UU,60,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,40,UU,UU,UU},
       IS_SPELL,10},

      {"identify", 70,20,5,POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, NON_VIOLENT, MAG_MANUAL,
       {UU,UU,UU,47,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,29,UU,UU,UU},
       IS_SPELL,5},

      {"backstab",0,0,0,0,0,0,0,
       {UU,UU, 3,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU, 1,UU,UU,UU,UU},
       IS_SKILL,0},

      {"bash", 0,0,0,0,0,0,0,
       {5,UU,UU,UU,UU,UU,UU,UU, 3,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"hide", 0,0,0,0,0,0,0,
       {UU,UU, 4,UU,UU,UU,30,UU,UU,UU,UU,UU,UU,UU, 2,UU,UU,UU,UU},
       IS_SKILL,0},

      {"kick", 0,0,0,0,0,0,0,
       {1,UU,UU,UU, 1, 8, 1,UU, 1, 5, 2,UU,UU, 1,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"pick lock", 0,0,0,0,0,0,0,
       {UU,UU, 4,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU, 4,UU,UU,UU,UU},
       IS_SKILL,0},

      {"punch", 0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"rescue", 0,0,0,0,0,0,0,
       {17,UU,UU,UU,19,UU,UU,UU,20,15,19,UU,UU,14,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"sneak", 0,0,0,0,0,0,0,
       {UU,UU, 1,UU,UU,23,UU,UU,UU,UU,UU,UU,UU,UU, 1,UU,UU,UU,UU},
       IS_SKILL,0},

      {"steal", 0,0,0,0,0,0,0,
       {UU,UU, 7,UU,UU,26,UU,UU,UU,UU,UU,UU,UU,UU, 7,UU,UU,UU,UU},
       IS_SKILL,0},

      {"track", 0,0,0,0,0,0,0,
       {UU,UU,UU,UU,14,UU,UU,UU,UU,UU,UU,36,UU,UU,50,UU,UU,UU,UU},
       IS_SKILL,0},

      {"mount",0,0,0,0,0,0,0,
       {13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13},
       IS_SKILL,0},

      {"riding",  0,0,0,0,0,0,0,
       {13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13},
       IS_SKILL,0},

      {"tame",  0,0,0,0,0,0,0,
       {UU,UU,UU,UU,19,UU,UU,UU,UU,UU,UU,11,UU,UU,UU,UU,18,UU,UU},
       IS_SKILL,0},

      {"second attack",  0,0,0,0,0,0,0,
       {25,35,30,40,28,30,25,35,28,25,25,35,UU,20,30,40,30,UU,UU},
       IS_SKILL,0},

      {"third attack", 0,0,0,0,0,0,0,
       {50,65,55,UU,53,55,50,65,53,50,50,65,UU,40,55,70,60,UU,UU},
       IS_SKILL,0},

      {"fourth attack",  0,0,0,0,0,0,0,
       {75,UU,95,UU,78,UU,UU,UU,78,75,75,UU,UU,60,85,UU,UU,UU,UU},
       IS_SKILL,0},

      {"rage",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,29,UU,UU,UU,UU,UU,62,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"throw",0,0,0,0,0,0,0,
       {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
       IS_SKILL,0},

      {"bow",0,0,0,0,0,0,0,
       {15,UU,10,UU, 1,30,UU,UU,12,UU,UU,UU,UU,10, 8,UU,UU,UU,UU},
       IS_SKILL,0},

      {"sling",  0,0,0,0,0,0,0,
       {UU,UU, 1,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU, 1,UU,UU,UU,UU},
       IS_SKILL,0},

      {"crossbow",0,0,0,0,0,0,0,
       {30,UU,20,UU,25,15,UU,UU,UU,UU,35,UU,UU,15,20,UU,UU,UU,UU},
       IS_SKILL,0},

      {"dual wield",0,0,0,0,0,0,0,
       {35,UU,15,UU,43,52,UU,UU,66,48,41,UU,UU,28,14,UU,UU,UU,UU},
       IS_SKILL,0},

      {"repair",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"fly",  60, 20, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,29,UU,UU,UU,UU,UU,UU,UU,33,UU,UU,UU,19,UU,UU,UU},
       IS_SPELL,5},

      {"fisticuffs", 0,0,0,0,0,0,0,
       {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
       IS_SKILL,5},

      {"sword", 0,0,0,0,0,0,0,
       {1,UU, 1,UU, 1, 1,UU,UU, 1, 1, 1,UU,UU, 1, 1, 3, 3, 3, 3},
       IS_SKILL,5},

      {"two handed sword", 0,0,0,0,0,0,0,
       {4,UU,UU,UU, 4, 4,UU,UU, 4, 4, 4,UU,UU, 2,UU,UU,UU,UU,UU},
       IS_SKILL,5},

      {"dagger", 0,0,0,0,0,0,0,
       {1,UU, 1, 1, 1, 1,UU, 1, 1, 1, 1, UU, UU, 1, 1, 1, 1, 1, 1},
       IS_SKILL,5},

      {"club", 0,0,0,0,0,0,0,
       {1, 1, 1, 1, 1, 1,UU, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
       IS_SKILL,5},

      {"two handed club", 0,0,0,0,0,0,0,
       {4, 4, 4, 4, 4, 4,UU, 4, 4, 4, 4, 4,UU, 2, 2, 2, 2,UU,UU},
       IS_SKILL,5},

      {"hammer", 0,0,0,0,0,0,0,
       {1, 1, 1, 1, 1, 1,UU, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,UU,UU},
       IS_SKILL,5},

      {"two handed hammer", 0,0,0,0,0,0,0,
       {4, 4, 4, 4, 4, 4,UU, 4, 4, 4, 4, 4,UU, 2, 2, 2, 2,UU,UU},
       IS_SKILL,5},

      {"axe", 0,0,0,0,0,0,0,
       {1,UU,UU,UU, 1, 1,UU,UU, 1, 1, 1,UU,UU, 1,UU,UU,UU,UU,UU},
       IS_SKILL,5},

      {"two handed axe", 0,0,0,0,0,0,0,
       {4,UU,UU,UU, 4, 4,UU,UU, 4, 4, 4,UU,UU, 2,UU,UU,UU,UU,UU},
       IS_SKILL,5},

      {"spear", 0,0,0,0,0,0,0,
       {1,UU, 1,UU, 1, 1,UU,UU, 1, 1, 1,UU,UU, 1, 1,UU,UU,UU,UU},
       IS_SKILL,5},

      {"whip", 0,0,0,0,0,0,0,
       {1, 1, 1, 1, 1, 1,UU,UU, 1, 1, 1, 1, 1, 1, 1, 1, 1,UU,UU},
       IS_SKILL,5},

      {"claw", 0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,5},

      {"brew", 0,0,0,0,0,0,0,
       {UU,29,UU,27,UU,34,UU,UU,UU,UU,UU,29,UU,UU,UU,24,24,UU,UU},
       IS_SKILL,5},

      {"scribe", 0,0,0,0,0,0,0,
       {UU,29,UU,27,UU,34,UU,UU,UU,UU,UU,29,UU,UU,UU,24,24,UU,UU},
       IS_SKILL,5},

      {"knock",   60, 20, 2, POS_STANDING, TAR_DOOR, NON_VIOLENT, MAG_MANUAL,
       {UU,UU,UU,33,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,22,UU,UU,UU},
       IS_SPELL,5},

      {"wizard lock",   60, 20, 2, POS_STANDING, TAR_DOOR, NON_VIOLENT, MAG_MANUAL,
       {UU,UU,UU,34,UU,46,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"ambush",0,0,0,0,0,0,0,
       {UU,UU,40,UU,65,UU,UU,UU,65,UU,UU,UU,UU,UU,20,UU,UU,UU,UU},
       IS_SKILL,0},

      {"trip",0,0,0,0,0,0,0,
       {UU,UU,42,UU,UU,UU,UU,UU,44,UU,45,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"sweep",0,0,0,0,0,0,0,
       {43,UU,UU,UU,48,UU,UU,UU,UU,45,UU,UU,UU,41,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"quivering palm",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,57,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"stomp",0,0,0,0,0,0,0,
       {15,UU,UU,UU,17,UU,UU,UU,16,UU,UU,UU,UU,14,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"headbutt",0,0,0,0,0,0,0,
       {10,UU,UU,UU,15,UU,UU,UU, 8,UU,15,UU,UU, 7,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"disarm",0,0,0,0,0,0,0,
       {28,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,25,30,UU,UU,UU,UU},
       IS_SKILL,0},

      {"berserk",0,0,0,0,0,0,0,
       {40,UU,UU,UU,UU,UU,UU,55,UU,35,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"circle",0,0,0,0,0,0,0,
       {UU,UU,38,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,33,UU,UU,UU,UU},
       IS_SKILL,0},

      {"unused",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_UNUSED,0},

      {"stun",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,51,UU,UU,UU,UU,UU,UU,UU,47,UU,UU,UU,UU},
       IS_SKILL,0},

      {"shadow",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"camouflage",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,11,UU,UU,UU,23,UU,UU,17,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"palm",0,0,0,0,0,0,0,
       {UU,UU,12,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU, 7,UU,UU,UU,UU},
       IS_SKILL,0},

      {"lay hands",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,22,UU,UU,33,UU,UU,UU,UU,UU,UU,42,UU,UU},
       IS_SKILL,0},

      {"bandage",0,0,0,0,0,0,0,
       {30,UU,45,UU,32,35,UU,UU,40,28,32,UU,UU,20,45,UU,UU,UU,UU},
       IS_SKILL,0},

      {"darken",0,0,0,0,0,0,0,
       {UU,UU,51,UU,UU,UU,35,UU,UU,UU,45,UU,UU,UU,46,40,UU,UU,UU},
       IS_SKILL,0},

      {"lighten",0,0,0,0,0,0,0,
       {UU,37,UU,UU,UU,UU,35,UU,UU,40,UU,UU,UU,UU,UU,UU,30,UU,UU},
       IS_SKILL,0},

      {"chant",0,0,0,0,0,0,0,
       {UU,40,UU,UU,UU,UU,UU,UU,UU,UU,UU,40,UU,UU,UU,UU,36,UU,UU},
       IS_SKILL,0},

      {"meditate",0,0,0,0,0,0,0,
       {UU,UU,UU,40,UU,50,25,UU,UU,UU,UU,UU,UU,UU,UU,36,UU,UU,UU},
       IS_SKILL,0},

      {"dodge",0,0,0,0,0,0,0,
       {UU,42,17,42,UU,26,18,UU,UU,UU,UU,42,UU,UU,17,38,38,UU,UU},
       IS_SKILL,0},

      {"block",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,21,21,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"parry",0,0,0,0,0,0,0,
       {21,UU,UU,UU,21,UU,UU,UU,27,UU,UU,UU,UU,21,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"conjure infantry", 70, 20, 5,POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_SUMMONS,
       {UU,UU,UU,38,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"web",  60, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_AFFECTS|MAG_CHECK,
       {UU,UU,UU,55,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,46,UU,UU,UU},
       IS_SPELL,5},

      {"gore",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"faerie fire",  30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_AFFECTS,
       {UU,UU,UU, 2,20,13,UU,UU,UU,UU,UU,UU,UU,UU,UU, 2,UU,UU,UU},
       IS_SPELL,5},

      {"ward fire", 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,54,UU,UU},
       IS_SPELL,5},

      {"ward cold", 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,47,UU,UU},
       IS_SPELL,5},

      {"ward electricity", 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,59,UU,UU},
       IS_SPELL,5},

      {"ward energy", 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,64,UU,UU},
       IS_SPELL,5},

      {"ward acid", 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,74,UU,UU},
       IS_SPELL,5},

      {"ward poison", 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,41,UU,UU},
       IS_SPELL,5},

      {"ward drain", 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,63,UU,UU},
       IS_SPELL,5},

      {"ward good",  40, 10, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, NON_VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,59,UU,UU,UU},
       IS_SPELL,5},

      {"sunray",  60, 40, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,65,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"redirect",0,0,0,0,0,0,0,
       {22,UU,UU,UU,27,48,48,UU,36,25,25,UU,UU,19,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"cause light",  40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU, 4,UU,UU,UU,UU,UU,UU,UU,UU, 8,UU,UU,UU,UU,UU, 3,UU,UU},
       IS_SPELL,5},

      {"cause serious",  40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,23,UU,UU,UU,UU,UU,UU,UU,UU,25,UU,UU,UU,UU,UU,18,UU,UU},
       IS_SPELL,5},

      {"cause critic",  40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,54,UU,UU,UU,UU,UU,40,UU,UU},
       IS_SPELL,5},

      {"atonement", 50,20,3,POS_FIGHTING, TAR_CHAR_ROOM|TAR_NOT_SELF,NON_VIOLENT,MAG_POINTS|MAG_CHECK,
       {UU,43,UU,UU,UU,UU,50,UU,UU,45,45,48,UU,UU,UU,40,25,UU,UU},
       IS_SPELL,5},

      {"summon mount", 35, 10, 3,POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_SUMMONS,
       {UU,UU,UU,UU,48,UU,UU,UU,UU,64,81,52,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"goodberry", 30,10,3,POS_STANDING, TAR_IGNORE,NON_VIOLENT,MAG_CREATIONS|MAG_CHECK,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,13,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"read magic",0,0,0,0,0,0,0,
       {1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1,UU, 1, 1, 1, 1,UU,UU},
       IS_SKILL,0},

      {"clan recall",  20, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_MANUAL,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"entangle",  60, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_AFFECTS,
       {UU,UU,UU,UU,80,UU,UU,UU,UU,UU,UU,57,UU,UU,UU,UU,45,UU,UU},
       IS_SPELL,5},

      {"grant peace",  60, 50, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, VIOLENT, MAG_DAMAGE | MAG_CHECK,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,10,UU,UU},
       IS_SPELL,10},

      {"dig", 0,0,0,0,0,0,0,
       {5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5},
       IS_SKILL,0},

      {"skin", 0,0,0,0,0,0,0,
       {5, UU, 5, 5, 5, 5, UU, 5, 5, 5, 5, UU, 5, 5, 5, 5, 5, 5, 5},
       IS_SKILL,0},

      {"fire shield",  80, 40, 2, POS_STANDING, TAR_CHAR_ROOM, NON_VIOLENT, MAG_AFFECTS|MAG_CHECK,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,7},

      {"drown", 0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_UNUSED,5},

      {"enliven", 80, 50, 3, POS_FIGHTING, TAR_IGNORE, VIOLENT, MAG_AREAS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,72,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"fear", 80, 50, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, NON_VIOLENT, MAG_FORCEFUL,
       {UU,UU,UU,65,UU,UU,UU,UU,UU,UU,50,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SPELL,5},

      {"mounted attack", 0,0,0,0,0,0,0,
       {UU,UU,UU,UU,51,UU,UU,UU,UU,30,30,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"fishing", 0,0,0,0,0,0,0,
       { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
       IS_SKILL,0},

      {"group summon", 50, 20, 2, POS_STANDING, TAR_IGNORE, NON_VIOLENT, MAG_GROUPS,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,63,UU,UU},
       IS_SPELL,10},

      {"rove", 0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_SKILL,0},

      {"\n",0,0,0,0,0,0,0,
       {UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU,UU},
       IS_UNUSED,0}
      } ;
*/
    /* SKILLEND SPELLEND */
/*
 * This arrangement is pretty stupid, but the number of skills is limited by 
 * the playerfile.  We can arbitrarily increase the number of skills by 
 * increasing the space in the playerfile. Meanwhile, this should provide 
 * ample slots for skills. 
 */



struct syllable
   {
   const char *org;
   const char *news;
   } ;


struct syllable syls[] =
      {
         {" " ,     " " } ,
      {"ar",     "abra" } ,
      {"ate",    "i" } ,
      {"cau",    "kada" } ,
      {"blind",  "nose" } ,
      {"bur",    "mosa" } ,
      {"cu",     "judi" } ,
      {"de",     "oculo" } ,
      {"dis",    "mar" } ,
      {"ect",    "kamina" } ,
      {"en",     "uns" } ,
      {"gro",    "cra" } ,
      {"light",  "dies" } ,
      {"lo",     "hi" } ,
      {"magi",   "kari" } ,
      {"mon",    "bar" } ,
      {"mor",    "zak" } ,
      {"move",   "sido" } ,
      {"ness",   "lacri" } ,
      {"ning",   "illa" } ,
      {"per",    "duda" } ,
      {"ra",     "gru" } ,
      {"re",     "candus" } ,
      {"son",    "sabru" } ,
      {"tect",   "infra" } ,
      {"tri",    "cula" } ,
      {"ven",    "nofo" } ,
      {"word of","inset" } ,
      {"a", "i" } ,
      {"b", "v" } ,
      {"c", "q" } ,
      {"d", "m" } ,
      {"e", "o" } ,
      {"f", "y" } ,
      {"g", "t" } ,
      {"h", "p" } ,
      {"i", "u" } ,
      {"j", "y" } ,
      {"k", "t" } ,
      {"l", "r" } ,
      {"m", "w" } ,
      {"n", "b" } ,
      {"o", "a" } ,
      {"p", "s" } ,
      {"q", "d" } ,
      {"r", "f" } ,
      {"s", "g" } ,
      {"t", "h" } ,
      {"u", "e" } ,
      {"v", "z" } ,
      {"w", "x" } ,
      {"x", "n" } ,
      {"y", "l" } ,
      {"z", "k" } ,
      {"" , "" }
      } ;


extern int find_scr_skill(int class, int spell_num);
extern int find_srr_skill(int race, int spell_num);

int min_level(struct char_data *ch,int spellnum)
{
  int base_level = LVL_IMPL+1;
  base_level = MIN(base_level, spells[spellnum].min_level[(int) GET_CLASS(ch)]);
  base_level = MIN(base_level, find_race_skill(GET_RACE(ch),spellnum));
  if (IS_SCR(ch)) {
    base_level = MIN(base_level, 20*find_scr_skill(GET_CLASS(ch), spellnum)/17);
  }

  if (IS_SCR(ch) && base_level < LVL_IMMORT)
  {
    return 1 + (int)(17 * base_level / 20);
  }
  /*
  if (IS_SRR(ch)) {
    base_level = MIN(base_level, 4*find_srr_skill(GET_RACE(ch), spellnum)/3);
    }
  if (base_level <= 0 || base_level >= LVL_IMMORT) {
    return base_level;
    }*/
    return base_level;

  /*  if (IS_SCR(ch)) {
    return 1+(int)(3*base_level/4);
    } else {
    return base_level;
    }*/
}

int mag_manacost(struct char_data * ch, int spellnum, int cast_level)
   {
   int mana;
   int change;
   int spl_level;
   int level_dif;
   /* get the base mana level */
   spl_level = min_level(ch,spellnum)+(cast_level-1);

   /* figure the char's relation to it */
   level_dif = GET_LEVEL(ch)-spl_level;

   change = (int)((float)level_dif * (float)0.5);

   /*    log("spel_lvl: %d  level_dif: %d  change: %d",spl_level,level_dif,change); */

   if(cast_level>GET_SKILL(ch,spellnum))
      change+=SINFO.mana_change*(-4)*(cast_level-GET_SKILL(ch,spellnum));
   else
      change+=SINFO.mana_change*(GET_SKILL(ch,spellnum)-cast_level);
   /*mana = MIN(MAX(SINFO.mana_max - change, SINFO.mana_min),SINFO.mana_max);*/
   mana = MAX(SINFO.mana_max - change, SINFO.mana_min);
   /*    log("Change: %d  Mana: %d  Max: %d  Min: %d",change,mana,SINFO.mana_max,SINFO.mana_min); */
   return mana;
   }

/* say_spell erodes buf, buf1, buf2 */
void say_spell(struct char_data * ch, int spellnum, struct char_data * tch,
               struct obj_data * tobj)
   {
   char *lbuf=get_buffer(256);
   char *buf=get_buffer(256);
   char *buf1=get_buffer(256);
   char *buf2=get_buffer(256);
   const char *format;
   struct char_data *i;
   int j, ofs = 0;

   *buf = '\0';
   strcpy(lbuf, spells[spellnum].spell_name);

   while (lbuf[ofs])
      {
      for (j = 0; *(syls[j].org); j++)
         {
         if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org)))
            {
            strcat(buf, syls[j].news);
            ofs += strlen(syls[j].org);
            break;
            }
         }
      /* i.e., we didn't find a match in syls[] */
      if (!*syls[j].org)
         {
         log("No entry in syllable table for substring of '%s'", lbuf);
         ofs++;
         }
      }

   if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch))
      {
      if (tch == ch)
         format = "$n closes $s eyes and utters the words, '%s'.";
      else
         format = "$n stares at $N and utters the words, '%s'.";
      }
   else if (tobj != NULL &&
            ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
      format = "$n stares at $p and utters the words, '%s'.";
   else
      format = "$n utters the words, '%s'.";

   sprintf(buf1, format, spells[spellnum].spell_name);
   sprintf(buf2, format, buf);

   for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
      {
      if (i == ch || i == tch || !i->desc || !AWAKE(i))
         continue;
      if (GET_CLASS(ch) == GET_CLASS(i))
         perform_act(buf1, ch, tobj, tch, i);
      else
         perform_act(buf2, ch, tobj, tch, i);
      }

   if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch))
      {
      sprintf(buf1, "$n stares at you and utters the words, '%s'.",
              GET_CLASS(ch) == GET_CLASS(tch) ? spells[spellnum].spell_name : buf);
      act(buf1, FALSE, ch, NULL, tch, TO_VICT);
      }

   release_buffer(buf2);
   release_buffer(buf1);
   release_buffer(buf);
   release_buffer(lbuf);
   }


/********************* Added so bards sing songs ***************************/

char *sing(int spell)
   {
   switch (spell)
      {
   case 1:
      return("a tune about a blacksmith who made the finest armor in the land.");
   case 19:
      return("a song about a sprite who thought he was invisible but wasn't.");
   case 20:
      return("about a land where magic fills the air.");
   case 66:
      return("of a time where there was no magic in the realm.");
   case 29:
      return("about a lonely man who was not able to be seen by other men.");
   case 65:
      return("a tune about a winged horse named pegasus.");
   case 92:
      return("of a peaceful time when everyone was friends.");
   case 17:
      return("an ancient tune of bitter enemies.");
   case 24:
      return("of great battles won and lost by the weapons the warriors carried!");
   case 77:
      return("a soft lullaby to put even the greatest beast to sleep.");
   case 37:
      return("of a great man named Hercules with a powerful grip!");
   case 5:
      return("about a man who lost his hands to the flames of a dragon.");
   case 69:
      return("a lofty tale of the knights that have conqured this realm.");
   case 84:
      return("about men who are pure in heart and in mind!\n\r.");
   case 76:
      return("a wrenching song filled with pain and sorrow.");
   case 83:
      return("a twisted song that drains your soul.");
   case 7:
      return("a cute little tune of companionship and enchantment.");
   case 80:
      return("the most horrible sound you have EVER heard!!");
   case 81:
      return("of an ancient red dragon named Fina and his deadly breath!");
   case 67:
      return("about a dragon named Erlic who rallied a town to destroy a king!");
   case 33:
      return("about the lifeless black seas far to the north in kindria.");
   case 70:
      return("about men who sleep lightly and live longer in this realm.");
   case 101:
      return("the tale of Korash who led his band to the City in the Sky.");
   case 102:
      return("of Belinor's globe which brought light to the besieged.");
   case 103:
      return("a song of Cora Nir, land of eternal light!");
   default:
      return("a strange song of glittering phrase.");
      }
   }
void sing_spell(struct char_data *ch, int spellnum)
   {
   char *lbuf=get_buffer(256);

   send_to_char(ch, "You sing %s\r\n", sing(spellnum));

   sprintf(lbuf, "$n sings %s",  sing(spellnum));
   act(lbuf,FALSE,ch,0,0,TO_ROOM);
   release_buffer(lbuf);
   }

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE.
 */

char *skill_name(int num)
   {
   if (num > 0 && num <= TOP_SPELL_DEFINE)
      return (spells[num].spell_name);
   else if (num == -1)
      return "UNUSED";
   else
      return ("UNDEFINED");
   }


int find_skill_num(char *name)
   {
   int skindex = 0, ok;
   char *temp, *temp2;
   char *first;
   char *first2;

   if(*name=='!')
      return -1;
   first=get_buffer(256);
   first2=get_buffer(256);
   for(skindex=1;skindex<MAX_SPELLS;skindex++)
      {
      if (is_abbrev(name, spells[spell_sort_info[skindex]].spell_name))
         {
         release_buffer(first2);
         release_buffer(first);
         return spell_sort_info[skindex];
         }

      ok = 1;
      temp = any_one_arg(spells[spell_sort_info[skindex]].spell_name, first);
      temp2 = any_one_arg(name, first2);
      while (*first && *first2 && ok)
         {
         if (!is_abbrev(first2, first))
            ok = 0;
         temp = any_one_arg(temp, first);
         temp2 = any_one_arg(temp2, first2);
         }

      if (ok && !*first2)
         {
         release_buffer(first2);
         release_buffer(first);
         return spell_sort_info[skindex];
         }
      }
   release_buffer(first2);
   release_buffer(first);
   return -1;
   }



/*
 * This function is the very heart of the entire magic system.  All 
 * invocations of all types of magic -- objects, spoken and unspoken PC 
 * and NPC spells, the works -- all come through this function eventually. 
 * This is also the entry point for non-spoken or unrestricted spells. 
 * Spellnum 0 is legal but silently ignored here, to make callers simpler. 
 */
int call_magic(struct char_data * caster, struct char_data * cvict,
               struct obj_data * ovict, struct room_direction_data *dvict,
               struct room_direction_data *dvict2,
               int spellnum, int level, int casttype)
   {
   int savetype;
   int target=FALSE;
   int should_prac=TRUE;

   if (spellnum < -1 || spellnum > MAX_SPELLS||spellnum==0)
      {
      log("SYSERR: Bad spell number passed to call_magic: %d",spellnum);
      CAST_ARG(caster)[0]='\0';
      return 0;
      }

   if (caster && cvict) {
     /*log("%s cast a spell on %s.", GET_NAME(caster), GET_NAME(cvict));*/
     long id = IS_NPC(caster) ? GET_MOB_RNUM(caster) + 500000 : GET_IDNUM(caster);
     int i = 0;
     while (i < cvict->num_casters && cvict->casting_on_me[i] != id) {
       i++;
     }
     if (i < cvict->num_casters) {
       /*log("(call_magic) Removing %ld from %s's casting list.", cvict->casting_on_me[i], GET_NAME(cvict));*/
       for (; i < cvict->num_casters-1; i++) {
	 cvict->casting_on_me[i] = cvict->casting_on_me[i+1];
       }
       cvict->num_casters--;
       cvict->casting_on_me = (long *)realloc(cvict->casting_on_me, cvict->num_casters * sizeof(long));
     }
   }

   if(spellnum==-1)
      {
      CAST_ARG(caster)[0]='\0';
      return 0;
      }

   if(caster->nr!=real_mobile(DG_CASTER_PROXY))
      {
      if(casttype !=CAST_BREATH || (casttype==CAST_BREATH && !IS_NPC(caster)))
         {
         if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC)&&
                 (GET_LEVEL(caster) < LVL_IMPL))
            {
            send_to_char(caster,"Your magic fizzles out and dies.\r\n");
            act("$n's magic fizzles out and dies.",FALSE,caster,0,0,TO_ROOM);
            CAST_ARG(caster)[0]='\0';
            return 0;
            }
         }
      if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
              (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))&&
              (GET_LEVEL(caster) < LVL_IMPL))
         {
         send_to_char(caster,"A flash of white light fills the room, dispelling your violent magic!\r\n");
         act("White light from no particular source suddenly fills the room, "
             "then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
         CAST_ARG(caster)[0]='\0';
         return 0;
         }
      }
   /* determine the type of saving throw */
   if (IS_SET(SINFO.targets, TAR_IGNORE))
      {
      target = TRUE;
      }
   else if((ovict!=NULL)||(cvict!=NULL)||(dvict!=NULL))
      {
      if(cvict&&IN_ROOM(cvict)==NOWHERE)
         cvict=NULL;

      if (!target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM)))
         if(cvict&&(IN_ROOM(caster)==IN_ROOM(cvict)))
            target = TRUE;

      if (!target && IS_SET(SINFO.targets, TAR_OBJ_INV))
         if (ovict&&(ovict->carried_by==caster))
            target = TRUE;

      if (!target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP))
         if(ovict&&(ovict->worn_by==caster))
            target = TRUE;

      if (!target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
         if (ovict&&(IN_ROOM(ovict)==IN_ROOM(caster)))
            target = TRUE;

      if (!target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
         if (ovict)
            target = TRUE;

      if (!target && IS_SET(SINFO.targets, TAR_DOOR))
         if (dvict)
            target = TRUE;

      if (!target && (IS_SET(SINFO.targets, TAR_CHAR_WORLD)||
                      ((IS_SET(SINFO.targets, TAR_CHAR_ROOM))&&
                       (GET_LEVEL(caster)>=LVL_IMMORT) &&!IS_NPC(caster))))
         if (cvict)
            target = TRUE;


      }

   if(!target)
      {
      send_to_char(caster,"Your target seems to have vanished\r\n");
      CAST_ARG(caster)[0]='\0';
      return 0;
      }

   switch (casttype)
      {
   case CAST_STAFF:
   case CAST_SCROLL:
   case CAST_PILL: /* New case for Pill Modification--Aleks */
   case CAST_POTION:
   case CAST_WAND:
      savetype = SAVING_ROD;
      break;
   case CAST_SPELL:
      savetype = SAVING_SPELL;
      break;
   default:
      savetype = SAVING_BREATH;
      break;
      }

   if (cvict && GET_POS(cvict) <= POS_DEAD) {
     if (caster) {
       send_to_char(caster, "Nothing seems to happen.\r\n");
       CAST_ARG(caster)[0] = '\x0';
     }
     return 0;
   }

   if (cvict && IS_NPC(cvict) && GET_MOB_SPEC(cvict) == shop_keeper) {
     if (caster) {
       send_to_char(caster, "Nothing seems to happen.\r\n");
       CAST_ARG(caster)[0] = '\x0';
     }
     return 0;
   }

   if (IS_SET(SINFO.routines, MAG_CHECK))
      if(!mag_check(level, caster, cvict, ovict, spellnum, savetype))
         {
         CAST_ARG(caster)[0]='\0';
         return 0;
         }
   if (IS_SET(SINFO.routines, MAG_MATERIALS))
      if(!mag_materials(caster, spellnum, level))
         {
         CAST_ARG(caster)[0]='\0';
         return 0;
         }
   if (IS_SET(SINFO.routines, MAG_AFFECTS))
      mag_affects(level, caster, cvict, spellnum, savetype);

   if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
      mag_unaffects(level, caster, cvict, spellnum, savetype);

   if (IS_SET(SINFO.routines, MAG_POINTS))
      mag_points(level, caster, cvict, spellnum, savetype);

   if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
      mag_alter_objs(level, caster, ovict, spellnum, savetype);

   if (IS_SET(SINFO.routines, MAG_GROUPS))
      mag_groups(level, caster, spellnum, savetype);

   if (IS_SET(SINFO.routines, MAG_MASSES))
      mag_masses(level, caster, spellnum, savetype);

   if (IS_SET(SINFO.routines, MAG_AREAS))
      mag_areas(level, caster, spellnum, savetype);

   if (IS_SET(SINFO.routines, MAG_SUMMONS))
      mag_summons(level, caster, ovict, spellnum, savetype);

   if (IS_SET(SINFO.routines, MAG_CREATIONS))
      mag_creations(level, caster, spellnum);

   if (IS_SET(SINFO.routines, MAG_DAMAGE))
      if(mag_damage(level, caster, cvict, spellnum, savetype)==-1)
         {
         if(casttype==CAST_SPELL)
            improve_skill(caster,spellnum,USE_PASS);
         CAST_ARG(caster)[0]='\0';
         return 1;
         }

   if (IS_SET(SINFO.routines, MAG_MANUAL))
      switch (spellnum)
         {
      case SPELL_CHARM:
         should_prac=MANUAL_SPELL(spell_charm);
         break;
      case SPELL_CREATE_WATER:
         should_prac=MANUAL_SPELL(spell_create_water);
         break;
      case SPELL_DETECT_POISON:
         should_prac=MANUAL_SPELL(spell_detect_poison);
         break;
      case SPELL_ENCHANT_WEAPON:
         should_prac=MANUAL_SPELL(spell_enchant_weapon);
         break;
      case SPELL_ENCHANT_ARMOR:
         should_prac=MANUAL_SPELL(spell_enchant_armor);
         break;
      case SPELL_IDENTIFY:
         should_prac=MANUAL_SPELL(spell_identify);
         break;
      case SPELL_LOCATE_OBJECT:
         should_prac=MANUAL_SPELL(spell_locate_object);
         break;
      case SPELL_SUMMON:
         should_prac=MANUAL_SPELL(spell_summon);
         break;
      case SPELL_WORD_OF_RECALL:
         should_prac=MANUAL_SPELL(spell_recall);
         break;
      case SPELL_CLAN_RECALL:
         should_prac=MANUAL_SPELL(spell_crecall);
         break;
      case SPELL_PORTAL:
         should_prac=MANUAL_SPELL(spell_portal);
         break;
      case SPELL_GATE:
         should_prac=MANUAL_SPELL(spell_gate);
         break;
      case SPELL_TELEPORT:
         should_prac=MANUAL_SPELL(spell_teleport);
         break;
      case SPELL_KNOCK:
         should_prac=MANUAL_SPELL(spell_knock);
         break;
      case SPELL_WIZARDLOCK:
         should_prac=MANUAL_SPELL(spell_wizardlock);
         break;
      case SPELL_ENERGY_DRAIN:
         should_prac=MANUAL_SPELL(spell_energy_drain);
         break;
      default:
         log("SYSERR: unknown spellnum %d passed to mag_manual", spellnum);
         }

   if (IS_SET(SINFO.routines, MAG_FORCEFUL))
      mag_forceful(level, caster, cvict, spellnum, savetype);

   if(should_prac)
      if(casttype==CAST_SPELL)
         improve_skill(caster,spellnum,USE_PASS);

   CAST_ARG(caster)[0]='\0';
   return 1;
   }

int tarbit_to_findbit(int spellnum)
   {
   int retval=0;

   if(IS_SET(spells[spellnum].targets,TAR_IGNORE))
      return 0;
   if(IS_SET(spells[spellnum].targets,TAR_CHAR_ROOM))
      SET_BIT(retval,FIND_CHAR_ROOM);
   if(IS_SET(spells[spellnum].targets,TAR_CHAR_WORLD))
      SET_BIT(retval,FIND_CHAR_WORLD);
   if(IS_SET(spells[spellnum].targets,TAR_OBJ_INV))
      SET_BIT(retval,FIND_OBJ_INV);
   if(IS_SET(spells[spellnum].targets,TAR_OBJ_ROOM))
      SET_BIT(retval,FIND_OBJ_ROOM);
   if(IS_SET(spells[spellnum].targets,TAR_OBJ_WORLD))
      SET_BIT(retval,FIND_OBJ_WORLD);
   if(IS_SET(spells[spellnum].targets,TAR_OBJ_EQUIP))
      SET_BIT(retval,FIND_OBJ_EQUIP);
   return retval;
   }

/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should 
 * only be called by the 'quaff', 'use', 'recite', 'swallow' etc. routines. 
 * 
 * For reference, object values 0-3: 
 * staff  - [0] level [1] max charges [2] num charges [3] spell num 
 * wand   - [0] level [1] max charges [2] num charges [3] spell num 
 * scroll - [0] level [1] spell num [2] spell num [3] spell num 
 * potion - [0] level [1] spell num [2] spell num [3] spell num 
 * pill   - [0] level [1] spell num [2] spell num [3] spell num 
 * 
 * Staves and wands will default to level 14 if the level is not specified; 
 * the DikuMUD format did not specify staff and wand levels in the world 
 * files (this is a CircleMUD enhancement). 
 */

void mag_objectmagic(struct char_data * ch, struct obj_data * obj,
                     char *argument)
   {
   int i, k;
   int find_bits;
   struct char_data *tch = NULL, *next_tch;
   struct obj_data *tobj = NULL;
   struct room_direction_data *tdr = NULL;
   struct room_direction_data *tdr2 = NULL;
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   struct char_data *kmaster;

   one_argument(argument, arg);
   /*
    * begin add - Bon 11/06/97
    * allow wand to summon someone not in same room, added FIND_CHAR_WORLD
    */
   switch (GET_OBJ_TYPE(obj))
      {
   case ITEM_STAFF:
      act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
      if (obj->action_description && !IS_OBJ_STAT(obj,ITEM_DO_ACT))
         act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
      else
         act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);

      if (GET_OBJ_VAL(obj, 2) <= 0)
         {
         act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
         act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
         }
      else
         {
         GET_OBJ_VAL(obj, 2)--;
         WAIT_STATE(ch, SKILL_LAG);
         if(IS_SET(spells[GET_OBJ_VAL(obj,3)].routines,MAG_AREAS))
            {
            if (GET_OBJ_VAL(obj, 0))
               call_magic(ch, ch, NULL, NULL,NULL,GET_OBJ_VAL(obj, 3),
                          GET_OBJ_VAL(obj, 0), CAST_STAFF);
            else
               call_magic(ch, ch, NULL, NULL,NULL,GET_OBJ_VAL(obj, 3),
                          DEFAULT_STAFF_LVL, CAST_STAFF);
            }
         else
            {
            if(AFF_FLAGGED(ch,AFF_GROUP))
               {
               if(ch->master!=NULL)
                  kmaster=ch->master;
               else
                  kmaster=ch;
               }
            else
               kmaster=NULL;

            for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
               {
               next_tch = tch->next_in_room;
               if (ch == tch)
                  continue;
               if (!IS_NPC(tch) && GET_INVIS_LEV(tch))
                  continue;
               if (!IS_NPC(tch) && !PLR_FLAGGED(tch,PLR_KILLER) &&
                   !(ROOM_FLAGGED(IN_ROOM(tch),ROOM_PKILL) ||
                   Z_FLAGGED(IN_ROOM(tch),Z_PKILL)) &&
                   (!PLR_FLAGGED(ch, PLR_PK) && !PLR_FLAGGED(tch, PLR_PK) &&
                   !(abs(GET_LEVEL(ch)-GET_LEVEL(tch))<=10)))
                  continue;

               if(IS_SET(spells[GET_OBJ_VAL(obj,3)].violent,VIOLENT))
                  {
                  if((kmaster!=NULL)&&(AFF_FLAGGED(tch,AFF_GROUP)))
                     {
                     if(tch==kmaster)
                        continue;
                     if(tch->master==kmaster)
                        continue;
                     }
                  }
               if (GET_OBJ_VAL(obj, 0))
                  call_magic(ch, tch, NULL, NULL,NULL,GET_OBJ_VAL(obj, 3),
                             GET_OBJ_VAL(obj, 0), CAST_STAFF);
               else
                  call_magic(ch, tch, NULL, NULL,NULL,GET_OBJ_VAL(obj, 3),
                             DEFAULT_STAFF_LVL, CAST_STAFF);
               }
            }
         }


      if((obj!=NULL)&&(GET_OBJ_VAL(obj,2)<=0))
         {
         obj=unequip_char(ch,obj->worn_on);
         obj_to_room(obj,IN_ROOM(ch));
         scrap_item(obj,ch);
         }

      break;
      /*
       * begin add - Bon 11/06/97
       * allow wand to summon someone not in same room
       */
   case ITEM_WAND:
      if(!*arg)
         {
         if((spells[GET_OBJ_VAL(obj,3)].violent==VIOLENT)&&FIGHTING(ch))
            {
            tch=FIGHTING(ch);
            k=FIND_CHAR_ROOM;
            }
         else if(spells[GET_OBJ_VAL(obj,3)].violent==NON_VIOLENT)
            {
            k=FIND_CHAR_ROOM;
            tch=ch;
            }
         else
            tch=NULL;
         }
      if(tch==NULL)
         {
         find_bits=tarbit_to_findbit(GET_OBJ_VAL(obj,3));
         k = generic_find(arg, find_bits, ch, &tch, &tobj);
         }

      if ((k == FIND_CHAR_ROOM) ||((k==FIND_CHAR_WORLD) &&
                                   (GET_OBJ_VAL(obj,3)==SPELL_SUMMON)))
         {
         if (tch == ch)
            {
            if(spells[GET_OBJ_VAL(obj,3)].violent==VIOLENT)
               {
               act("You point $p at yourself, but then realize that you "
                   "really don't want to do that.", FALSE,
                   ch, obj, 0, TO_CHAR);
               release_buffer(arg);
               return;
               }
            act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
            act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
            }
         else
            {
            if(IS_SET(spells[GET_OBJ_VAL(obj,3)].violent,VIOLENT))
               {
               if (!IS_NPC(tch) && !PLR_FLAGGED(tch,PLR_KILLER) &&
                   !(ROOM_FLAGGED(IN_ROOM(tch),ROOM_PKILL) ||
                   Z_FLAGGED(IN_ROOM(tch),Z_PKILL)) &&
                   (!PLR_FLAGGED(ch, PLR_PK) && !PLR_FLAGGED(tch, PLR_PK) &&
                   !(abs(GET_LEVEL(ch)-GET_LEVEL(tch))<=10))) 
                  {
                  act("Pointing $p at $N suddenly doesn't "
                      "seem like such a good idea.",
                      FALSE, ch, obj, tch, TO_CHAR);
                  release_buffer(arg);
                  return;
                  }
               }
            act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
            if ((obj->action_description != NULL)  &&
                    !IS_OBJ_STAT(obj,ITEM_DO_ACT))
               act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
            else
               act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
            }
         }
      else if (tobj != NULL)
         {
         act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
         if ((obj->action_description != NULL)  &&
                 !IS_OBJ_STAT(obj,ITEM_DO_ACT))
            act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
         else
            act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
         }
      else
         {
         act("At what should $p be pointed?", FALSE, ch, obj,NULL,TO_CHAR);
         release_buffer(arg);
         return;
         }

      if (GET_OBJ_VAL(obj, 2) <= 0)
         {
         act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
         act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
         release_buffer(arg);
         return;
         }
      GET_OBJ_VAL(obj, 2)--;
      WAIT_STATE(ch, SKILL_LAG);
      if(arg!=NULL && *arg)
         {
         strncpy(CAST_ARG(ch),arg,120);
         CAST_ARG(ch)[120]='\0';
         }
      if (GET_OBJ_VAL(obj, 0))
         call_magic(ch, tch, tobj, tdr,tdr2,GET_OBJ_VAL(obj, 3),
                    GET_OBJ_VAL(obj, 0), CAST_WAND);
      else
         call_magic(ch, tch, tobj, tdr,tdr2,GET_OBJ_VAL(obj, 3),
                    DEFAULT_WAND_LVL, CAST_WAND);

      if((obj!=NULL)&&(GET_OBJ_VAL(obj,2)<=0))
         {
         obj=unequip_char(ch,obj->worn_on);
         obj_to_room(obj,IN_ROOM(ch));
         scrap_item(obj,ch);
         }

      break;
   case ITEM_SCROLL:
      if (FIGHTING(ch) && (GET_DEX(ch) < number(0, 30)))
         {
         act("You try to recite $p.", FALSE, ch, obj, NULL, TO_CHAR);
         act("But a sudden move in combat causes you to rip it.", FALSE, ch, obj, NULL, TO_CHAR);
         act("$n tries to recite $p but combat distracts $m.", TRUE, ch, obj, NULL, TO_ROOM);
         WAIT_STATE(ch, SKILL_LAG);
         }
      else
         {
         act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
         if (obj->action_description && !IS_OBJ_STAT(obj,ITEM_DO_ACT))
            act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
         else
            act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);

         WAIT_STATE(ch, SKILL_LAG);

         for (i = 1; i < 4&&GET_OBJ_VAL(obj,i)>0; i++)
            {
            find_bits=tarbit_to_findbit(GET_OBJ_VAL(obj,i));
            k = generic_find(arg, find_bits, ch, &tch, &tobj);
            if (*arg)
               {
               if (!k)
                  {
                  act("There is nothing to here to affect with $p.", FALSE,
                      ch, obj, NULL, TO_CHAR);
                  release_buffer(arg);
                  return;
                  }
               }
            else
               tch = ch;

            if(arg!=NULL && *arg)
               {
               strncpy(CAST_ARG(ch),arg,120);
               CAST_ARG(ch)[120]='\0';
               }
            if (!(call_magic(ch, tch, tobj, tdr,tdr2, GET_OBJ_VAL(obj, i),
                             GET_OBJ_VAL(obj, 0), CAST_SCROLL)))
               break;
            }
         }
      if (obj != NULL)
         extract_obj(obj);
      break;
   case ITEM_POTION:
      tch = ch;
      if (FIGHTING(ch) && (GET_DEX(ch) < number(0, 30)))
         {
         act("You try to quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
         act("But it slips from your grasp in combat and smashes to shards.", FALSE, ch, obj, NULL, TO_CHAR);
         act("$n tries to quaff $p but it slips and smashes on the ground.", TRUE, ch, obj, NULL, TO_ROOM);
         WAIT_STATE(ch, SKILL_LAG);
         }
      else if (IS_OBJ_STAT(obj, ITEM_NEWBIE) && (REMORT_LEVEL(ch) > 0 || GET_LEVEL(ch) > LVL_POTION_NEWBIE))
	{
	  act("You try to quaff $p, but are too advanced for a newbie potion.", FALSE, ch, obj, NULL, TO_CHAR);
	  WAIT_STATE(ch, SKILL_LAG);
	}
      else
         {
         act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
         if (obj->action_description && !IS_OBJ_STAT(obj,ITEM_DO_ACT))
            act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
         else
            act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);

         WAIT_STATE(ch, SKILL_LAG);
         for (i = 1; i < 4&&GET_OBJ_VAL(obj,i)>0; i++)
            {
            if (!(call_magic(ch, ch, NULL, NULL,NULL, GET_OBJ_VAL(obj, i),
                             GET_OBJ_VAL(obj, 0), CAST_POTION)))
               break;
            if(!ch)
               break;
            }

         }
      if (obj != NULL)
         extract_obj(obj);
      break;
      /* New case for Pill modification--Aleks */
   case ITEM_PILL:
      tch = ch;
      act("You swallow $p.", FALSE, ch, obj, NULL, TO_CHAR);
      if (obj->action_description && !IS_OBJ_STAT(obj,ITEM_DO_ACT))
         act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
      else
         act("$n swallows $p.", TRUE, ch, obj, NULL, TO_ROOM);

      WAIT_STATE(ch, SKILL_LAG);
      for (i = 1; i < 4; i++)
         if (!(call_magic(ch, ch, NULL, NULL,NULL, GET_OBJ_VAL(obj, i),
                          GET_OBJ_VAL(obj, 0), CAST_PILL)))
            break;

      if (obj != NULL)
         extract_obj(obj);
      break;
   default:
      log("SYSERR: Unknown object_type %d in mag_objectmagic",
          GET_OBJ_TYPE(obj));
      break;
      }
   release_buffer(arg);
   }


/*
 * cast_spell is used generically to cast any spoken spell, assuming we 
 * already have the target char/obj and spell number.  It checks all 
 * restrictions, etc., prints the words, etc. 
 * 
 * Entry point for NPC casts.  Recommended entry point for spells cast 
 * by NPCs via specprocs. 
 */

int cast_spell(struct char_data * ch, struct char_data * tch,
               struct obj_data * tobj, struct room_direction_data *tdr,
               struct room_direction_data *tdr2,
               int spellnum,int cast_level)
   {
   int min_time;

   if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE)
      {
      log("SYSERR: cast_spell trying to call spellnum %d\n",spellnum);
      return 0;
      }

   if (GET_POS(ch) < SINFO.min_position)
      {
      switch (GET_POS(ch))
         {
      case POS_SLEEPING:
         send_to_char(ch,"You dream about great magical powers.\r\n");
         break;
      case POS_RESTING:
         send_to_char(ch,"You cannot concentrate while resting.\r\n");
         break;
      case POS_SITTING:
         send_to_char(ch,"You can't do this sitting!\r\n");
         break;
      case POS_FIGHTING:
         send_to_char(ch,"Impossible!  You can't concentrate enough!\r\n");
         break;
      default:
         send_to_char(ch,"You can't do much of anything like this!\r\n");
         break;
         }
      return 0;
      }
   if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch))
      {
      send_to_char(ch,"You are afraid you might hurt your master!\r\n");
      return 0;
      }
   if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY))
      {
      send_to_char(ch,"You can only cast this spell upon yourself!\r\n");
      return 0;
      }
   if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF))
      {
      send_to_char(ch,"You cannot cast this spell upon yourself!\r\n");
      return 0;
      }
   if (IS_SET(SINFO.routines, MAG_GROUPS) && !AFF_FLAGGED(ch, AFF_GROUP))
      {
      send_to_char(ch,"You can't cast this spell if you're not in a group!\r\n");
      return 0;
      }

   if(IS_BARD(ch))
      {
      send_to_char(ch,"You start singing...\r\n");
      act("$n starts singing...", TRUE, ch, 0, 0, TO_ROOM);
      }
   else
      {
      send_to_char(ch,"You start chanting...\r\n");
      act("$n starts chanting...", TRUE, ch, 0, 0, TO_ROOM);
      }
   IS_CASTING(ch) = TRUE;
   CAST_SPELLNUM(ch) = spellnum;
   CAST_TCH(ch)  = tch;
   CAST_TOBJ(ch) = tobj;
   CAST_TDR(ch)  = tdr;
   CAST_TDR2(ch)  = tdr2;
   CAST_LEVEL(ch) = cast_level;

   if (tch) {
     tch->num_casters++;
     tch->casting_on_me = (long *)realloc(tch->casting_on_me, tch->num_casters * sizeof(long));
     long id = IS_NPC(ch) ? GET_MOB_RNUM(ch) + 500000 : GET_IDNUM(ch);
     tch->casting_on_me[tch->num_casters-1] = id;
     /*log("%s(%ld) is casting a spell on %s.", GET_NAME(ch), id, GET_NAME(tch));*/
   }

   if(spells[spellnum].violent==NON_VIOLENT)
      min_time=0;
   else
      min_time=1;

   if(IS_NPC(ch))
      CAST_TIME(ch)=spells[spellnum].cast_time;
   else
      {
      if(PRF_FLAGGED(ch,PRF_NOHASSLE))
         CAST_TIME(ch) = min_time;
      else if(cast_level<=GET_SKILL(ch,spellnum))
         CAST_TIME(ch) = MAX(min_time,spells[spellnum].cast_time-(GET_SKILL(ch,spellnum)-cast_level));
      else
         CAST_TIME(ch) =spells[spellnum].cast_time + ((cast_level-GET_SKILL(ch,spellnum))*3);
      }
   /*    log("time:%d  level:%d  skill:%d",CAST_TIME(ch), CAST_LEVEL(ch),GET_SKILL(ch,spellnum)); */

   return 1;
   }


/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments, 
 * determines the spell number and finds a target, throws the die to see if 
 * the spell can be cast, checks for sufficient mana and subtracts it, and 
 * passes control to cast_spell(). 
 */

ACMD(do_cast)
   {
   struct char_data *tch = NULL;
   struct obj_data *tobj = NULL;
   struct room_direction_data *tdr = NULL;
   struct room_direction_data *tdr2 = NULL;
   char *s, *t,*arg;
   char *mybuf;
   char cnum[4];
   int mana, spellnum, i, target = 0,cast_level=0,percentage;
   int chance, dam=0;

   if (IS_NPC(ch))
      return;

   if(!valid_class_align(ch))
      {
      send_to_char(ch,"You recieve a searing pain from you own magic as your god punishes you for straying from the true path!\r\n");
      act("The gods punish $n for straying.",TRUE,ch,0,0,TO_ROOM);
      damage(ch,ch,GET_LEVEL(ch)*2,TYPE_SUFFERING,IMM_DRAIN);
      return;
      }
   if ((GET_CLASS(ch) == CLASS_BARD) && subcmd == SCMD_CAST)
      {
      send_to_char(ch,"You must sing your songs...\r\n");
      return;
      }

   if ((GET_CLASS(ch) != CLASS_BARD) && subcmd == SCMD_SING)
      {
      send_to_char(ch,"You must cast your spells...\r\n");
      return;
      }

   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC)&&
           (GET_LEVEL(ch) < LVL_IMPL))
      {
      send_to_char(ch,"Your magic fizzles out and dies.\r\n");
      act("$n's magic fizzles out and dies.",FALSE,ch,0,0,TO_ROOM);
      return;
      }
   mybuf=get_buffer(MAX_STRING_LENGTH);
   strcpy(mybuf,argument);
   skip_spaces(&argument);

   /*
    * get the cast_level and strip the number out
    * and place s on the spell name or just place s
    * on the spell name
    */
   if(isdigit((int)argument[0]))
      {
      cnum[0]=argument[0];
      if(isdigit((int)argument[1]))
         {
         cnum[1]=argument[1];
         cnum[2]='\0';
         }
      else
         cnum[1]='\0';
      cast_level=MAX(1,MIN(10,atoi(cnum)));

      s = strtok(argument, "'");

      if (s == NULL)
         {
         if (subcmd == SCMD_CAST)
            send_to_char(ch,"Cast what where?\r\n");
         else
            send_to_char(ch,"Sing what where?\r\n");
         release_buffer(mybuf);
         return;
         }
      s = strtok(NULL, "'");
      }
   else
      s = strtok(argument, "'");


   if (s == NULL)
      {
      if (subcmd == SCMD_CAST)
         send_to_char(ch,"Spell names must be enclosed in the Mystic Symbols: '\r\n");
      else
         send_to_char(ch,"Song names must be enclosed in the Magic Notes: '\r\n");
      release_buffer(mybuf);
      return;
      }
   t = strtok(NULL, "\0");


   /* spellnum = search_block(s, spells, 0); */
   spellnum = find_skill_num(s);

   if ((spellnum < 1) || (spellnum > MAX_SPELLS))
      {
      if (subcmd == SCMD_CAST)
         send_to_char(ch,"Cast what?!?\r\n");
      else
         send_to_char(ch,"Sing what?!?\r\n");
      release_buffer(mybuf);
      return;
      }

   if(SINFO.is_spell!=IS_SPELL)
      {
      if (subcmd == SCMD_CAST)
         send_to_char(ch,"Cast what?!?\r\n");
      else
         send_to_char(ch,"Sing what?!?\r\n");
      release_buffer(mybuf);
      return;
      }

   /* RACE_SPELLS CHECK */
   if (GET_LEVEL(ch) < min_level(ch,spellnum))
      {
      if (subcmd == SCMD_CAST)
         send_to_char(ch,"You do not know that spell!\r\n");
      else
         send_to_char(ch,"You do not know that song!\r\n");
      release_buffer(mybuf);
      return;
      }

   if (GET_SKILL(ch, spellnum) == 0)
      {
      if (subcmd == SCMD_CAST)
         send_to_char(ch,"You are unfamiliar with that spell.(help learn)\r\n");
      else
         send_to_char(ch,"You are unfamiliar with that song.(help learn)\r\n");
      release_buffer(mybuf);
      return;
      }
   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) &&
           (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))&&
           (GET_LEVEL(ch) < LVL_IMPL))
      {
      send_to_char(ch,"A flash of white light fills the room, dispelling your "
                   "violent magic!\r\n");
      act("White light from no particular source suddenly fills the room, "
          "then vanishes.", FALSE, ch, 0, 0, TO_ROOM);
      release_buffer(mybuf);
      return;
      }
   /* Find the target */
   if (t != NULL)
      {
      arg=get_buffer(MAX_INPUT_LENGTH);
      one_argument(strcpy(arg, t), t);
      skip_spaces(&t);
      release_buffer(arg);
      }
   if (IS_SET(SINFO.targets, TAR_IGNORE))
      {
      target = TRUE;
      }
   else if (t != NULL && *t)
      {
      strncpy(CAST_ARG(ch),t,120);
      CAST_ARG(ch)[120]='\0';

      if (!target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM)))
         {
         if ((tch = get_char_vis(ch, t,FIND_CHAR_ROOM)) != NULL)
            target = TRUE;
         }

      if(!target && IS_SET(SINFO.targets,TAR_DOOR))
         {
         if((tdr = get_room_dir(world,t,ch))!=NULL)
            {
            tdr2=get_other_room_dir(world,t,ch);
            target=TRUE;
            }
         }

      if (!target && IS_SET(SINFO.targets, TAR_OBJ_INV))
         if ((tobj = get_obj_in_list_vis(ch, t, ch->carrying)))
            target = TRUE;

      if (!target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP))
         {
         for (i = 0; !target && i < NUM_WEARS; i++)
            if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->name))
               {
               tobj = GET_EQ(ch, i);
               target = TRUE;
               }
         }
      if (!target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
         if ((tobj = get_obj_in_list_vis(ch, t, world[IN_ROOM(ch)].contents)))
            target = TRUE;

      if (!target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
         if ((tobj = get_obj_vis(ch, t)))
            target = TRUE;

      if (!target && (IS_SET(SINFO.targets, TAR_CHAR_WORLD)||
                      (IS_SET(SINFO.targets, TAR_CHAR_ROOM)&&
                       (GET_LEVEL(ch)>=LVL_IMMORT))))
         if ((tch = get_char_vis(ch, t,FIND_CHAR_WORLD)))
            target = TRUE;
      }
   else
      {
      /* if target string is empty */
      if (!target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
         if (FIGHTING(ch) != NULL)
            {
            tch = ch;
            target = TRUE;
            }
      if (!target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
         if (FIGHTING(ch) != NULL)
            {
            tch = FIGHTING(ch);
            target = TRUE;
            }
      /* if no target specified, and the spell isn't violent, default to self */
      if (!target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
              !SINFO.violent)
         {
         tch = ch;
         target = TRUE;
         }
      if (!target)
         {
         char *buf=get_buffer(128);
         if (subcmd == SCMD_CAST)
            send_to_char(ch, "Upon %s should the spell be cast?\r\n",
                         IS_SET(SINFO.targets,TAR_OBJ_ROOM|TAR_OBJ_INV|TAR_OBJ_WORLD|TAR_DOOR)?
                         "what" : "who");
         else
            send_to_char(ch, "Upon %s should the song be sung?\r\n",
                         IS_SET(SINFO.targets,TAR_OBJ_ROOM|TAR_OBJ_INV|TAR_OBJ_WORLD|TAR_DOOR)?
                         "what" : "who");
         release_buffer(buf);
         release_buffer(mybuf);
         return;
         }
      }

   if (target && (tch == ch) && SINFO.violent)
      {
      if (subcmd == SCMD_CAST)
         send_to_char(ch,"You shouldn't cast that on yourself -- could be bad for your health!\r\n");
      else
         send_to_char(ch,"You shouldn't sing that to yourself -- could be bad for your health!\r\n");
      release_buffer(mybuf);
      return;
      }
   if (!target)
      {
      if (subcmd == SCMD_CAST)
         send_to_char(ch,"Cannot find the target of your spell!\r\n");
      else
         send_to_char(ch,"Cannot find the target of your song!\r\n");
      release_buffer(mybuf);
      return;
      }

   /* Offensive spell check. Prevent PCs from casting harmful spells on each other. */
   if (tch && !IS_NPC(ch) && !IS_NPC(tch) && (tch!=ch) /* Target exists and Both are players. */
      && (
      SINFO.violent /* And the spell is violent. */
      || spellnum == SPELL_PIXIE_DUST /* Or it's pixie or blind. */
      || spellnum == SPELL_BLINDNESS
      || spellnum == SPELL_DISPEL_MAGIC
      ) 
      && (
      !PLR_FLAGGED(ch, PLR_PK) /* Caster's not set (PK). */
      || !PLR_FLAGGED(tch, PLR_PK) /* Or target's not set PK. */
      || (abs(GET_LEVEL(ch)-GET_LEVEL(tch)) > 10) /* Or they're more than 10 levels apart. */
      )
      && !PLR_FLAGGED(tch, PLR_KILLER) /* target is not a (KILLER). */
      && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_PKILL) /* And you're not in a PK room. */
      && !Z_FLAGGED(IN_ROOM(tch),Z_PKILL) /* And you're not in a PK zone. */
      && (GET_LEVEL(ch) < LVL_IMMORT) /* And you're not an imm. */
      ) {   
      send_to_char(ch, "You can't cast that on a player outside a pk room!!\r\n");
      release_buffer(mybuf);
      return;
      }

   /* If you don't have a PK flag, you can't cast a spell on someone who does, if that someone is in a PK fight. */
   if (tch /* Target exists. */
      &&!IS_NPC(tch)                               /* Target is a player. */
      && ch != tch                                 /* And it's not yourself. */
      && PLR_FLAGGED(tch, PLR_PK)                  /* And the target has a PK flag. */
      && FIGHTING(tch)	                           /* And the target is fighting. */
      && !IS_NPC(FIGHTING(tch))	                   /* And the target is fighting another PK'er. */
      && PLR_FLAGGED(FIGHTING(tch), PLR_PK)        /* And the target's target has a PK flag. */
      && (!PLR_FLAGGED(ch, PLR_PK)                 /* And you don't have a PK flag. */
      || ((abs(GET_LEVEL(ch)-GET_LEVEL(tch)) > 10) /* Or they're more than 10 levels apart. */
      || (abs(GET_LEVEL(ch)-GET_LEVEL(FIGHTING(tch))) > 10)))
      && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_PKILL)   /* And you're not in a PK room. */
      && !Z_FLAGGED(IN_ROOM(tch),Z_PKILL)          /* And you're not in a PK zone. */
      && (GET_LEVEL(ch) < LVL_IMMORT)              /* And you're not an imm. */
      ) {
      send_to_char(ch, "You can't interfere with a PK fight without buying a PK flag.\r\n");
      release_buffer(mybuf);
      return;
      }	

   if(cast_level==0)
      cast_level=MAX(1,MIN(10,GET_SKILL(ch,spellnum)));

   mana = mag_manacost(ch, spellnum, cast_level);
   if ((mana > 0) && (GET_MANA(ch) < mana) && (GET_LEVEL(ch) < LVL_IMMORT))
      {
      if (subcmd == SCMD_CAST)
         send_to_char(ch,"You haven't the energy to cast that spell!\r\n");
      else
         send_to_char(ch,"You haven't the energy to sing that song!\r\n");
      release_buffer(mybuf);
      return;
      }

   /*
    * you get a bonus for casting below your level and a 15% hit for
    * casting above it 
    */
   percentage=90;
   if(cast_level>GET_SKILL(ch,spellnum))
      percentage=percentage-(15*(cast_level-GET_SKILL(ch,spellnum)));
   else if(cast_level<GET_SKILL(ch,spellnum))
      percentage=percentage+(GET_SKILL(ch,spellnum)-cast_level);

   /* You throws the dice and you takes your chances.. 101% is total failure */
   if(PRF_FLAGGED(ch,PRF_NOHASSLE))
      chance=0;
   else
      {
      chance=number(1,101);
      if(chance!=101)
         chance+=GET_SPELL_FAIL(ch);
      }


   if (chance> percentage)
      {
      WAIT_STATE(ch, SKILL_LAG);
      improve_skill(ch,spellnum,USE_FAIL);
      /*
       * just to make sure someone casting below their level doesn't get
       * wammied too often 
       */
      if((chance>(percentage+5))&&(cast_level>GET_SKILL(ch,spellnum)))
         {
         if (subcmd == SCMD_CAST)
            send_to_char(ch,"Something went VERY wrong!!\r\n");
         else
            send_to_char(ch,"Your first note catches in your throat causing the energy of your song to backfire!!\r\n");
         GET_MANA(ch)/=2;
         dam  = GET_MAX_HIT(ch)/10;
         dam  = dam * (cast_level-GET_SKILL(ch,spellnum)) * 2;
         dam += 5;
         damage(ch,ch,dam,TYPE_SUFFERING,IMM_DRAIN);
         if(!ch||(GET_HIT(ch)<1))
            {
            /* whoops, killed the caster */
            release_buffer(mybuf);
            return;
            }
         }
      else
         {
         if (!tch || !skill_message(0, ch, tch, spellnum))
            {
            send_to_char(ch,"Your loss of concentration stuns you momentarily!\r\n");
            act("$n's loss of concentration stuns $m momentarily.", FALSE, ch, 0, 0, TO_ROOM);
            }
         }
      if (mana > 0)
         GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch)-(mana/2)));
      }
   else
      {
      if(!IS_NPC(ch)&&(GET_LEVEL(ch)>=LVL_IMMORT))/*&&(GET_LEVEL(ch)<LVL_IMPL))*/
         mudlogf(CMP,GOD_LOG(ch),TRUE,
                 "(GC) %s cast %s with arg %s at %ld.",GET_NAME(ch),
                 spells[spellnum].spell_name, mybuf,
                 world[IN_ROOM(ch)].number);

      /* cast spell returns 1 on success; subtract mana & set waitstate */
      if (cast_spell(ch, tch, tobj, tdr,tdr2,spellnum,cast_level))
         {
         if (mana > 0)
            GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
         }
      }
   release_buffer(mybuf);
   /* you dared to cast at me, WHACK! */
   if (SINFO.violent && tch && IS_NPC(tch)&&!FIGHTING(tch)&&(tch->master!=ch))
      hit(tch, ch, TYPE_UNDEFINED);
   }



void spell_level(int spell, int class, int level)
   {
   int bad = 0;

   if (spell < 0 || spell > TOP_SPELL_DEFINE)
      {
      log("SYSERR: attempting assign to illegal spellnum %d", spell);
      return;
      }

   if (class < 0 || class >= NUM_CLASSES)
      {
      log("SYSERR: assigning '%s' to illegal class %d", skill_name(spell),
          class);
      bad = 1;
      }

   if (level < 1 || level > LVL_IMPL)
      {
      log("SYSERR: assigning '%s' to illegal level %d",
          skill_name(spell), level);
      }

   if (!bad)
      spells[spell].min_level[class] = level;
   }



void reset_casting_data(struct char_data *ch)
   {
   IS_CASTING(ch) = FALSE;
   CAST_TCH(ch) = NULL;
   CAST_TOBJ(ch) = NULL;
   CAST_TDR(ch)=NULL;
   CAST_TDR2(ch)=NULL;
   CAST_SPELLNUM(ch) = 0;
   CAST_TIME(ch) = 0;
   CAST_LEVEL(ch) = 0;
   *(CAST_ARG(ch))='\0';
   }

void update_casting_time(struct char_data *ch)
   {
   int x;
   int cast_time = CAST_TIME(ch);
   int spellnum = CAST_SPELLNUM(ch);
   struct char_data *tch = CAST_TCH(ch);
   struct obj_data *tobj = CAST_TOBJ(ch);
   struct room_direction_data *tdr = CAST_TDR(ch);
   struct room_direction_data *tdr2 = CAST_TDR2(ch);

   if (cast_time > 0)
      {
      if ((GET_POS(ch) != POS_STANDING && GET_POS(ch) != POS_FIGHTING))
         {
         send_to_char(ch,"You lost your concentration!\r\n");
         reset_casting_data(ch);
         }
      else
         {
         char *buf=get_buffer(256);
         CAST_TIME(ch)--;
         if(GET_CLASS(ch)!=CLASS_BARD)
            sprintf(buf, "Casting: %s", spells[spellnum].spell_name);
         else
            sprintf(buf, "Singing: %s", spells[spellnum].spell_name);
         for (x = cast_time; x > 0; x--)
            strcat(buf, "*");
         strcat(buf, "\r\n");
         send_to_char(ch,"%s",buf);
         release_buffer(buf);
         }
      }
   else
      {
      if ((GET_POS(ch) != POS_STANDING && GET_POS(ch) != POS_FIGHTING))
         {
         send_to_char(ch,"You lost your concentration!\r\n");
         reset_casting_data(ch);
         }
      else
         {
         if(GET_CLASS(ch)!=CLASS_BARD)
            {
            send_to_char(ch,"You complete your spell...\r\n");
            act("$n finishes chanting...", FALSE, ch, 0, 0, TO_ROOM);
            say_spell(ch, spellnum, tch, tobj);
            }
         else
            {
            send_to_char(ch,"You complete your song...\r\n");
            act("$n finishes singing...", FALSE, ch, 0, 0, TO_ROOM);
            sing_spell(ch, spellnum);
            }

         call_magic(ch, tch, tobj, tdr, tdr2,spellnum,CAST_LEVEL(ch),
                    CAST_SPELL);
         reset_casting_data(ch);
         }
      }
   }

