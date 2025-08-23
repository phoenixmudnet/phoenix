/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */



#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"

#include "structs.h"
#include "db.h"
#include "buffer.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "interpreter.h"
#include "constants.h"
#include "handler.h"

extern struct player_index_element *player_table;
extern int siteok_everyone;

void obj_to_char(struct obj_data * object, struct char_data * ch);
int find_id(long id);
void perform_remove(struct char_data * ch, int pos);

/* Names first */

const char *class_abbrevs[] =
  {
    "Wa",
    "Cl",
    "Th",
    "Mu",
    "Ra",
    "Bd",
    "Mo",
    "**",
    "Ba",
    "Pa",
    "AP",
    "Dr",
    "Me",
    "Ke",
    "As",
    "Ne",
    "De",
    "Im",
    "Gd",
    "SL",
    "\n"
  }
  ;

const char *scr_class_abbrevs[] =
  {
    "El",
    "Pr",
    "Rg",
    "Wz",
    "Wd",
    "Tr",
    "Se",
    "**",
    "Bz",
    "Tm",
    "DT",
    "Ad",
    "Me",
    "Ke",
    "As",
    "Ne",
    "De",
    "Im",
    "Gd",
    "SL",
    "\n"
  }
  ;


const char* scr_male_pc_class_types[] =
{
  "Elite Warrior",
  "Priest",
  "Rogue",
  "Wizard",
  "Warder",
  "Troubadour",
  "Sensei",
  "*UNUSED*",
  "Berserker",
  "Templar",
  "Dark Templar",
  "Archdruid",
  "*UNUSED*",
  "*UNUSED*",
  "*UNUSED*",
  "*UNUSED*",
  "*UNUSED*",
  "*UNUSED*",
  "*UNUSED*",
  "\n"
};

const char* scr_female_pc_class_types[] =
{
  "Elite Warrior",
  "Priestess",
  "Rogue",
  "Wizard",
  "Warder",
  "Troubadour",
  "Sensei",
  "*UNUSED*",
  "Berserker",
  "Templar",
  "Dark Templar",
  "Archdruid",
  "*UNUSED*",
  "*UNUSED*",
  "*UNUSED*",
  "*UNUSED*",
  "*UNUSED*",
  "*UNUSED*",
  "*UNUSED*",
  "\n"
};

const char *pc_class_types[] =
  {
    "Warrior",
    "Cleric",
    "Thief",
    "Magic-User",
    "Ranger",
    "Bard",
    "Monk",
    "*UNUSED*",
    "Barbarian",
    "Paladin",
    "Anti-Paladin",
    "Druid",
    "Merchant",
    "Kensai",
    "Assassin",
    "Necromancer",
    "Deva",
    "Immortal",
    "God",
    "\n"
  }
  ;


/* 10/26/96, Echo - class restrictions by race */

const int LEGAL_CLASS[NUM_RACES][NUM_CLASSES] =
  {
 /*  Wa Cl Th Mu Ra Bd Mo ** Ba Pa AP Dr Me Ke As Ne De Im Gd */
    { 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0} , /* Human */
    { 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0} , /* Elf  */
    { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0} , /* Half-Elf */
    { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0} , /* Dark-Elf  */
    { 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0} , /* Dwarf */
    { 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0} , /* Halfling */
    { 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0} , /* Sprite  */
    { 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0} , /* Minotaur  */
    { 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0} , /* Avian */
    { 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} , /* Half-Ogre */
    { 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0} , /* Half-Orc */
    { 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0} , /* Draconian*/
    { 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0} , /* Shadow */
    { 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0} , /* Titan */
    { 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0} /* Aesir */
  };

char *class_menu_header =
  "\r\nThe following classes are available to the race you've chosen:\r\n";

char *class_menu_choices[] =
  {
    "  [W]arrior        --Unspecialized fighters.\r\n",
    "  [C]leric         --Keepers of the faith; Healers.\r\n",
    "  [T]hief          --Experts at criminal skills.\r\n",
    "  [M]agic-user     --Experts in the magical arts.\r\n",
    "  [R]anger         --Warriors devoted to nature.\r\n",
    "  [B]ard           --Jacks of all trades.\r\n",
    "  M[o]nk           --Masters of martial arts.\r\n",
    "  *UNUSED*         --Future class of who knows what\r\n",
    "  B[a]rbarian      --Uncivilized tribal warriors.\r\n",
    "  [P]aladin        --Holy warriors.\r\n",
    "  A[n]ti-Paladin   --Evil holy warriors.\r\n",
    "  [D]ruid          --Nature oriented clerics.\r\n",
    "  M[e]rchant       --Master traders that trade their way to the top.\r\n\n",
    "  Remort1  - not selectable from class menu.\r\n",
    "  Remort2  - not selectable from class menu.\r\n",
    "  Remort3  - not selectable from class menu.\r\n",
    "  Remort4  - not selectable from class menu.\r\n",
    "  Immortal - not selectable from class menu.\r\n",
    "  God      - not selectable from class menu.\r\n",

  }
  ;

char *class_prompt =
  "\r\nPlease select a class or type '? <class>' for help ";

/* The menu for choosing a class in interpreter.c: */
/* 10/27/96, Echo - replaced this menu with
    class_menu_header, class_menu_choices[], and class_prompt
    in order to display class selections based on race.

char *class_menu =
"\r\n"
"Select a class:\r\n"
"  [C]leric\r\n"
"  [T]hief\r\n"
"  [W]arrior\r\n"
"  [M]agic-user\r\n";

*/  /* End commented class_menu */


/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class_for_menu(char arg)
  {
  arg = LOWER(arg);

  switch (arg)
    {
    case 'm':
      return CLASS_MAGIC_USER;
      break;
    case 'c':
      return CLASS_CLERIC;
      break;
    case 'w':
      return CLASS_WARRIOR;
      break;
    case 't':
      return CLASS_THIEF;
      break;
    case 'r':
      return CLASS_RANGER;
      break;
    case 'b':
      return CLASS_BARD;
      break;
    case 'o':
      return CLASS_MONK;
      break;
      /* case 's': -- REMOVED FOR REMORT
         return CLASS_ASSASSIN;
         break;
         */
    case 'a':
      return CLASS_BARBARIAN;
      break;
    case 'p':
      return CLASS_PALADIN;
      break;
    case 'n':
      return CLASS_ANTI_PALADIN;
      break;
    case 'd':
      return CLASS_DRUID;
      break;
    case 'e':
      return CLASS_MERCHANT;
      break;
    case '1': /* Remort class not selectable from class menu.   */
    case '2': /* Remort class not selectable from class menu.   */
    case '3': /* Remort class not selectable from class menu.   */
    case '4': /* Remort class not selectable from class menu.   */
    case 'i': /* Immortal class not selectable from class menu. */
    case 'g': /* God class not selectable from class menu.      */
    default:
      return CLASS_UNDEFINED;
      break;
    }
  }

int parse_class(char *arg)
  {
  int i=0;

  for(i=0;*(pc_class_types[i])!='\n';i++)
    {
    if(is_abbrev(arg,pc_class_types[i]))
      return i;
    }
  return CLASS_UNDEFINED;
  }

long find_class_bitvector(char *arg)
  {
  int class;

  class = parse_class(arg);

  if(class>=0)
    return(1 << class);
  return 0;
  }


/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 *
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 *
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL 0
#define SKILL 1

/* #define LEARNED_LEVEL 0  % known which is considered "learned" */
/* #define MAX_PER_PRAC  1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC  2  min percent gain in skill per practice */

int prac_params[3][NUM_CLASSES] =
  {
    /* MAG CLE THE WAR RAN BAD MON *** BAR PAL APL DRU MER Ke  As  Ne  De Im  Gd*/
    { 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95} ,/* learned level */
    { 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20} ,/* max per prac */
    { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10} ,/* min per pac */
  };


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 */
int guild_info[][3] =
  {
    /* Midgaard */
    { CLASS_MAGIC_USER, 3017, SCMD_SOUTH } ,
    { CLASS_CLERIC, 3004, SCMD_NORTH } ,
    { CLASS_THIEF,  3027, SCMD_EAST } ,
    { CLASS_WARRIOR, 3021, SCMD_EAST } ,

    /* Brass Dragon */
    { -999 /* all */ , 5065, SCMD_WEST } ,

    /* New Sparta */
    { CLASS_MAGIC_USER, 21075, SCMD_NORTH } ,
    { CLASS_CLERIC, 21019, SCMD_WEST } ,
    { CLASS_THIEF,  21014, SCMD_SOUTH } ,
    { CLASS_WARRIOR, 21023, SCMD_SOUTH } ,

    /* this must go last -- add new guards above! */
    { -1, -1, -1 }
  }
  ;




/* THAC0 for classes and levels.  (To Hit Armor Class 0) */

/* 10/07/96, Echo - added new function base_thaco()
 *   base_thaco() calculates thaco parametrically for each class,
 *   using the constant LEVEL_IMMORT defined in structs.h
 *   and replaces thaco[][] in fight.c
 */
/* interpolating function */
int between(int lvl, int lvl_a, int lvl_b, int lvl_c, int lvl_d, int lvl_e,
            int val_a, int val_b, int val_c, int val_d, int val_e)
  {
  const int default_thaco = 100;   /* if class or level undefined  */
  int temp = default_thaco;
  int lvl_fr = 0,              lvl_to = LVL_IMMORT - 1,
                                        val_fr = default_thaco,  val_to = 1;
  float bindex;

  if (lvl < 1 || lvl > 401)
    {
    temp = 1;
    return(temp);
    }
  else if (lvl >= lvl_a)
    {
    lvl_to = lvl_a;
    val_to = val_a;

    }
  else if (lvl >= lvl_b)
    {
    lvl_fr = lvl_a;
    val_fr = val_a;
    lvl_to = lvl_b;
    val_to = val_b;
    }
  else if (lvl >= lvl_c)
    {
    lvl_fr = lvl_b;
    val_fr = val_b;
    lvl_to = lvl_c;
    val_to = val_c;
    }
  else if (lvl >= lvl_d)
    {
    lvl_fr = lvl_c;
    val_fr = val_c;
    lvl_to = lvl_d;
    val_to = val_d;
    }
  else if (lvl >= lvl_e)
    {
    lvl_fr = lvl_d;
    val_fr = val_d;
    lvl_to = lvl_e;
    val_to = val_e;
    }
  /*   else
        {
        lvl_fr = lvl_e;
        val_fr = val_e;
        }
        */
  bindex = (float) (lvl - lvl_fr)/(lvl_to - lvl_fr);
  temp = (int) (val_fr + (bindex * (val_to - val_fr)));
  return(temp);
  }

int base_thaco(int ch_class, int targ_ac)
  {
  int temp_thaco;
  if(targ_ac>200)
    targ_ac = 200;
  else if (targ_ac<-200)
    targ_ac = -200;
  /* can't have a divide by zero, now can we? */
  targ_ac+=201;

  switch(ch_class)
    {
    case CLASS_MAGIC_USER:
      /* if level is in   [1, 10)  then thaco is in [20, 17);
       * if level is in  [10, 40)  then thaco is in [17, 15);
       * if level is in  [40, 120) then thaco is in [15, 8);
       * if level is in [120, 175) then thaco is in  [8, 5);
       * if level is in [175, 200) then thaco is in  [5, 1).
       * New values for 100 levels -Anduin
       * if level is in   [1, 5)  then thaco is in [20, 17);
       * if level is in  [5, 20)  then thaco is in [17, 15);
       * if level is in  [20, 60) then thaco is in [15, 8);
       * if level is in [60, 80) then thaco is in  [8, 5);
       * if level is in [80, 100) then thaco is in  [5, 1).
       */
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           72,52,32,12,1);
      break;
    case CLASS_CLERIC:
      temp_thaco = between(targ_ac,  401,301,201,101,1,
                           80,55,35,15,5);
      break;
    case CLASS_THIEF:
      temp_thaco = between(targ_ac,  401,301,201,101,1,
                           80,60,40,20,5);
      break;
    case CLASS_WARRIOR:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           95,75,55,35,15);
      break;
    case CLASS_RANGER:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           90,70,50,30,15);
      break;
    case CLASS_BARD:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           85,65,45,25,10);
      break;
    case CLASS_MONK:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           90,68,48,28,13);
      break;
      /*
        case CLASS_ASSASSIN:  -- MOVED TO REMORT
        temp_thaco = between(targ_ac,  401,301,201,101,1,
        17, 14, 12, 5,  1);
        break;
        */
    case CLASS_BARBARIAN:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           100,80,60,40,20);
      break;
    case CLASS_PALADIN:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           90,70,50,30,15);
      break;
    case CLASS_ANTI_PALADIN:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           90,70,50,30,15);
      break;
    case CLASS_DRUID:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           80,55,35,15,2);
      break;
    case CLASS_MERCHANT:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           70,50,30,10,1);
      break;

    case CLASS_KENSAI:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           95,75,55,35,15);
      break;
    case CLASS_ASSASSIN:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           95,75,55,35,15);
      break;
    case CLASS_NECROMANCER:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           80,60,40,20,5);
      break;
    case CLASS_DEVA:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           85,65,45,25,5);
      break;

    case MCLASS_NONE:
      temp_thaco = between(targ_ac, 401,301,201,101,1,
                           75,55,35,15,1);
      break;

    case CLASS_IMMORTAL:
      temp_thaco = 100;
      break;
    case CLASS_GOD:
      temp_thaco = 100;
      break;

    default:
      mudlogf(BRF,LVL_IMMORT,TRUE, "SYSERR:tried to get the thaco for "
              "an unknown class: %d/%s. Use show fighting to see who "
              "needs to be fixed", ch_class,class_abbrevs[ch_class]);
      temp_thaco = 90;
      break;
    }
  return(temp_thaco);
  }


/* 10/08/96, Echo - original thaco[][] table commented out.
 * const int thaco[NUM_CLASSES][LVL_IMPL + 1] =
  {
...
  }
;
 */


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(struct char_data * ch)
  {
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[4];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++)
    {

    for (j = 0; j < 4; j++)
      rolls[j] = number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
           MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp)
        {
        temp ^= table[k];
        table[k] ^= temp;
        temp ^= table[k];
        }
    }

  ch->real_abils.str_add = 0;

  switch (GET_CLASS(ch))
    {
    case CLASS_MAGIC_USER:
      ch->real_abils.intel = table[0];
      ch->real_abils.wis = table[1];
      ch->real_abils.dex = table[2];
      ch->real_abils.str = table[3];
      ch->real_abils.con = table[4];
      ch->real_abils.cha = table[5];
      break;
    case CLASS_CLERIC:
      ch->real_abils.wis = table[0];
      ch->real_abils.intel = table[1];
      ch->real_abils.str = table[2];
      ch->real_abils.dex = table[3];
      ch->real_abils.con = table[4];
      ch->real_abils.cha = table[5];
      break;
    case CLASS_THIEF:
      ch->real_abils.dex = table[0];
      ch->real_abils.str = table[1];
      ch->real_abils.con = table[2];
      ch->real_abils.intel = table[3];
      ch->real_abils.wis = table[4];
      ch->real_abils.cha = table[5];
      break;
    case CLASS_WARRIOR:
      ch->real_abils.str = table[0];
      ch->real_abils.dex = table[1];
      ch->real_abils.con = table[2];
      ch->real_abils.wis = table[3];
      ch->real_abils.intel = table[4];
      ch->real_abils.cha = table[5];
      if (ch->real_abils.str == 18)
        ch->real_abils.str_add = number(0, 100);
      break;
    case CLASS_RANGER:
      ch->real_abils.str = table[0];
      ch->real_abils.dex = table[1];
      ch->real_abils.con = table[2];
      ch->real_abils.wis = table[3];
      ch->real_abils.intel = table[4];
      ch->real_abils.cha = table[5];
      if (ch->real_abils.str == 18)
        ch->real_abils.str_add = number(0, 100);
      break;
    case CLASS_BARD:
      ch->real_abils.wis = table[0];
      ch->real_abils.intel = table[1];
      ch->real_abils.str = table[2];
      ch->real_abils.dex = table[3];
      ch->real_abils.con = table[4];
      ch->real_abils.cha = table[5];
      break;
    case CLASS_MONK:
      ch->real_abils.dex = table[0];
      ch->real_abils.str = table[1];
      ch->real_abils.con = table[2];
      ch->real_abils.intel = table[3];
      ch->real_abils.wis = table[4];
      ch->real_abils.cha = table[5];
      break;
    case CLASS_BARBARIAN:
      ch->real_abils.str = table[0];
      ch->real_abils.dex = table[1];
      ch->real_abils.con = table[2];
      ch->real_abils.wis = table[3];
      ch->real_abils.intel = table[4];
      ch->real_abils.cha = table[5];
      if (ch->real_abils.str == 18)
        ch->real_abils.str_add = number(0, 100);
      break;
      /*
        case CLASS_ASSASSIN: -- MOVED TO REMORT
        ch->real_abils.dex = table[0];
        ch->real_abils.str = table[1];
        ch->real_abils.con = table[2];
        ch->real_abils.intel = table[3];
        ch->real_abils.wis = table[4];
        ch->real_abils.cha = table[5];
        break;
        */
    case CLASS_PALADIN:
      ch->real_abils.str = table[0];
      ch->real_abils.dex = table[1];
      ch->real_abils.con = table[2];
      ch->real_abils.wis = table[3];
      ch->real_abils.intel = table[4];
      ch->real_abils.cha = table[5];
      if (ch->real_abils.str == 18)
        ch->real_abils.str_add = number(0, 100);
      break;
    case CLASS_ANTI_PALADIN:
      ch->real_abils.str = table[0];
      ch->real_abils.dex = table[1];
      ch->real_abils.con = table[2];
      ch->real_abils.wis = table[3];
      ch->real_abils.intel = table[4];
      ch->real_abils.cha = table[5];
      if (ch->real_abils.str == 18)
        ch->real_abils.str_add = number(0, 100);
      break;
    case CLASS_DRUID:
      ch->real_abils.wis = table[0];
      ch->real_abils.intel = table[1];
      ch->real_abils.str = table[2];
      ch->real_abils.dex = table[3];
      ch->real_abils.con = table[4];
      ch->real_abils.cha = table[5];
      break;
    }
  ch->aff_abils = ch->real_abils;
  }


/* I know this looks wierd, but it works :) Nomikos */
#define WEAR_BIT(pos) ((pos== 1||pos== 2) ? (1<< 1)      : ( \
                       (pos== 3||pos== 4) ? (1<< 2)      : ( \
                       (pos > 4&&pos <14) ? (1<<(pos-2)) : ( \
                       (pos==14||pos==15) ? (1<<12)      : ( \
                       (pos==16||pos==17) ? (1<<13)      : ( \
                       (pos==18||pos==19) ? (1<<14)      : ( \
                       (pos==20||pos==21) ? (1<<15)      : ( \
                       (pos >21)          ? (1<<(pos-4))  : -1) ) ) ) ) ) ) )

/* Some initializations for characters, including initial skills */
/* 10/27/96, Echo - added 'from_scratch' parameter to facilitate being
 *   able to use do_start more elegantly both for creating new characters
 *   as well as in do_advance (in act.wizard.c)
 */
void do_start(struct char_data * ch, bool from_scratch)
  {
  int i, j;

  int eqlistnew[13][9] =
    {
    /*  weapon worn   worn   worn   worn   worn   extra  extra  extra */
      { 21667, 21668, 21669, 21670, 21648, 21655, 21625, 21672,     0 }, /* warrior      */
      { 21685, 21686, 21687, 21688, 21689, 21647, 21690, 21691, 21663 }, /* cleric       */
      { 21627, 21628, 21629, 21630, 21631, 21632, 21633, 21634,     0 }, /* thief        */
      { 21659, 21661, 21660, 21662, 21613, 21666, 21664, 21665, 21663 }, /* magic-user   */
      { 21636, 21637, 21638, 21639, 21640, 21613, 21642, 21618, 21641 }, /* ranger       */
      { 21643, 21644, 21645, 21646, 21648, 21647, 21650, 21649,     0 }, /* bard         */
      {     0, 21673, 21647, 21675, 21676, 21677, 21625, 21678,     0 }, /* monk         */
      {     0,     0,     0,     0,     0,     0,     0,     0,     0 }, /* unused       */
      { 21652, 21653, 21654, 21655, 21656, 21657, 21625, 21658,     0 }, /* barbarian    */
      { 21679, 21680, 21681, 21682, 21683, 21668, 21625, 21684,     0 }, /* paladin      */
      { 21619, 21620, 21621, 21622, 21623, 21617, 21625, 21624,     0 }, /* anti-paladin */
      { 21610, 21612, 21611, 21613, 21614, 21615, 21616, 21625, 21618 }, /* druid        */
      {     0,  3043,  3076,  3081,  5428,  3032,     0,     0,     0 }  /* merchant     */
    };

  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;

  if (from_scratch)
    {
    set_title(ch, NULL); /* only reset title if first time around    */
    ch->points.max_hit = 20;
    for(i=0;i<MAX_SKILLS;i++)
      {
      GET_SKILL(ch,i)=0;
      GET_SKILL_LEARN(ch,i)=0;
      }
    GET_LAST_LEARN(ch)=0;
    GET_LEARN_TIC(ch)=0;
    }
  /* end if (from_scratch) */

  /* cause the characters max_hit, max_mana, and max_move to increase */
  advance_level(ch, from_scratch);  /* log the advance only if from_scratch */

  /* 'restore' the character */
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  /* give him/her a good meal */
  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  GET_OLD_MOBKILLS(ch)=0;
  GET_MOBKILLS(ch)=0;
  GET_DEATHS(ch)=0;
  GET_PKILLS(ch)=0;
  GET_QPOINTS(ch)=0;

  ch->player.time.played = 0;
  ch->player.time.logon = time(0);
  if (siteok_everyone)
    SET_BIT(PLR_FLAGS(ch), PLR_SITEOK);

  /* Give starting EQ and gold, including weapon based on class, if first time */
  if (from_scratch)
  {
    GET_GOLD(ch) = 150;

    for (int neweqidx = 0; neweqidx < 9; neweqidx++) {

      int neweqnum = eqlistnew[(int)GET_CLASS(ch)][neweqidx];
      if (neweqnum == 0) continue;

      obj_rnum rnum = real_object(neweqnum);
      if (rnum < 0) continue;  // object doesn't exist

      struct obj_data *obj = read_object(rnum, REAL);
      if (obj == NULL) continue; // object doesn't exist

      int pos;
      for (pos = 1; pos < NUM_WEARS; pos++)
        if (OBJWEAR_FLAGGED(obj, WEAR_BIT(pos)))
          break;

      if (neweqnum == 21642) // quiver
        for (j = 0; j < 5; j++) {
          struct obj_data *arrow = read_object(real_object(21651), REAL);

          obj_to_obj(arrow, obj); // arrows
        }

      if ((pos == NUM_WEARS) || (neweqidx > 5))
        obj_to_char(obj, ch);
      else
        equip_char(ch, obj, pos);
    }

    /* food */
    int bread_vobj = real_object(21694);
    if (bread_vobj > 0) {
      for (int z = 0; z < 3; z++) {
        struct obj_data *bread_obj = read_object(bread_vobj, REAL);
        if (bread_obj) {
          obj_to_char(bread_obj, ch);
        }
      }
    }

    /* map */
    obj_to_char(read_object(real_object(11), REAL), ch);

    GET_SKILL(ch, PROF_FISTICUFFS) = 40;
    GET_SKILL(ch, SKILL_READ_MAGIC) = 50;
    /*    GET_SKILL(ch,SKILL_RIDING)=50;*/
    GET_SKILL(ch, SKILL_FISHING) = 50;
    switch (GET_CLASS(ch))
    {
    case CLASS_MAGIC_USER:
      GET_SKILL(ch, SKILL_READ_MAGIC) = 90;
      GET_SKILL(ch, PROF_CLUB) = 35;
      break;
    case CLASS_CLERIC:
      GET_SKILL(ch, SKILL_READ_MAGIC) = 90;
      GET_SKILL(ch, PROF_CLUB) = 35;
      GET_ALIGNMENT(ch) = 1000;
      break;
    case CLASS_DRUID:
      GET_SKILL(ch, SKILL_READ_MAGIC) = 90;
      GET_SKILL(ch, PROF_CLUB) = 35;
      GET_ALIGNMENT(ch) = 0;
      break;
    case CLASS_BARD:
      GET_SKILL(ch, PROF_SWORD) = 35;
      GET_SKILL(ch, PROF_THROW) = 35;
      break;
    case CLASS_THIEF:
      /*   case CLASS_ASSASSIN: -- MOVED TO REMORT */
      GET_SKILL(ch, PROF_DAGGER) = 35;
      GET_SKILL(ch, PROF_THROW) = 35;
      break;
    case CLASS_WARRIOR:
      GET_SKILL(ch, PROF_SWORD) = 35;
      GET_SKILL(ch, PROF_DAGGER) = 35;
      break;
    case CLASS_RANGER:
      GET_SKILL(ch, PROF_SWORD) = 35;
      GET_SKILL(ch, PROF_DAGGER) = 35;
      GET_SKILL(ch, PROF_BOW) = 35;
      GET_ALIGNMENT(ch) = 500;
      break;
    case CLASS_PALADIN:
      GET_SKILL(ch, PROF_SWORD) = 35;
      GET_SKILL(ch, PROF_DAGGER) = 35;
      GET_ALIGNMENT(ch) = 1000;
      break;
    case CLASS_ANTI_PALADIN:
      GET_SKILL(ch, PROF_SWORD) = 35;
      GET_SKILL(ch, PROF_DAGGER) = 35;
      GET_ALIGNMENT(ch) = -1000;
      break;
    case CLASS_BARBARIAN:
      GET_SKILL(ch, PROF_HAMMER) = 35;
      GET_SKILL(ch, PROF_AXE) = 35;
      break;
    case CLASS_MONK:
      GET_SKILL(ch, PROF_THROW) = 35;
      break;
    case CLASS_KENSAI:
      GET_SKILL(ch, PROF_SWORD) = 35;
      GET_SKILL(ch, PROF_HAMMER) = 35;
      GET_SKILL(ch, PROF_AXE) = 35;
      GET_SKILL(ch, PROF_DAGGER) = 35;
      break;
    case CLASS_ASSASSIN:
      GET_SKILL(ch, PROF_SWORD) = 35;
      GET_SKILL(ch, PROF_HAMMER) = 35;
      GET_SKILL(ch, PROF_AXE) = 35;
      GET_SKILL(ch, PROF_DAGGER) = 35;
      break;
    case CLASS_NECROMANCER:
      GET_SKILL(ch, SKILL_READ_MAGIC) = 90;
      GET_SKILL(ch, PROF_CLUB) = 35;
      GET_SKILL(ch, PROF_DAGGER) = 35;
      GET_ALIGNMENT(ch) = -1000;
      break;
    case CLASS_DEVA:
      GET_SKILL(ch, SKILL_READ_MAGIC) = 90;
      GET_SKILL(ch, PROF_CLUB) = 35;
      GET_SKILL(ch, PROF_DAGGER) = 35;
      GET_ALIGNMENT(ch) = 1000;
      break;
    default:
      break;
    } /* end switch */

    /* Equip the heart of a noob. */
    obj_rnum heart_rnum = real_object(3047);
    if (heart_rnum > 0) {
      struct obj_data* heart = read_object(heart_rnum, REAL);
      if (heart) {
        equip_char(ch, heart, WEAR_HEART);
      }
    }

    /* Set up autoexit--everyone uses anyway */

    SET_BIT(PRF_FLAGS(ch), PRF_AUTOEXIT | PRF_AUTOSPLIT | PRF_AUTOLOOT | PRF_AUTOGOLD | PRF_AUTOSAC | PRF_SUMMONABLE | PRF_DISPMANA | PRF_DISPHP | PRF_DISPMOVE);
    SET_BIT(PRF2_FLAGS(ch), PRF2_DISPGOLD | PRF2_DISPEXP | PRF2_DISPALIGN | PRF2_DISPMAX | PRF2_PAGE_OK | PRF2_RECALLABLE);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DELETED);
    /* Send a nifty message */
    send_to_char(ch, "\r\nAs you are born into the realm,"
                     " you gain possession of items\r\n"
                     "that have been passed down in your"
                     " family generation after\r\n"
                     "generation... May the gods protect you.\r\n\r\n");
  }
  /* end if (from_scratch) */
  }



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(struct char_data * ch, bool show)
  {
  int add_hp = 0, add_mana = 0, add_move = 0, i;

  int con = GET_CON(ch);
  int wis = GET_WIS(ch);
  int intel = GET_INT(ch);
  int dex = GET_DEX(ch);
  int stat_bonus = 0;
  if (!IS_NPC(ch) && REMORT_LEVEL(ch) == 0 && GET_LEVEL(ch) < 40) {
    if (GET_LEVEL(ch) <= 15) {
      stat_bonus = 15;
    } else if (GET_LEVEL(ch) <= 30) {
      stat_bonus = 10;
    } else if (GET_LEVEL(ch) <= 40) {
      stat_bonus = 5;
    }
  }

  // Remort happens automatically when the mob is defeated. Give those folks a
  // minor break and assume they had good leveling gear.
  if (GET_LEVEL(ch) >= 100 && GET_LEVEL(ch) <= 103) {
      stat_bonus = 25;
  }

  if (show) {
    mudlogf(CMP, LVL_IMMORT, TRUE, "Levels: %s gained with base stats %d int, %d wis, %d dex, %d con",
	    GET_NAME(ch), intel, wis, dex, con);
  }

  if (stat_bonus > 0) {
    con = MIN(25, con+stat_bonus);
    wis = MIN(25, wis+stat_bonus);
    intel = MIN(25, intel+stat_bonus);
    dex = MIN(25, dex+stat_bonus);
    if (show) {
      mudlogf(CMP, LVL_IMMORT, TRUE, "Levels: %s gained with actual stats %d int, %d wis, %d dex, %d con using the newbie stat bonuses",
	      GET_NAME(ch), intel, wis, dex, con);
    }
  }

  con = stat_index(con);
  intel = stat_index(intel);
  wis = stat_index(wis);
  dex = stat_index(dex);

  add_hp = con_app[con].hitp;
  add_mana = (con_app[intel].hitp+con_app[wis].hitp) /3;
  add_move = con_app[dex].hitp;

  switch (GET_CLASS(ch))
    {

    case CLASS_MAGIC_USER:
      add_hp += number(1, 3);
      add_mana += number(2,9);
      add_move += number(1, 2);
      break;
    case CLASS_CLERIC:
      add_hp += number(1, 4);
      add_mana += number(2,8);
      add_move += number(1, 2);
      break;
    case CLASS_THIEF:
      add_hp += number(2, 5);
      add_mana += number(1, 4);
      add_move += number(1, 2);
      break;
    case CLASS_WARRIOR:
      add_hp += number(1, 8);
      add_mana += number(1, 4);
      add_move += number(1, 2);
      break;
    case CLASS_RANGER:
      add_hp += number(2, 6);
      add_mana += number(1, 5);
      add_move += number(1, 2);
      break;
    case CLASS_BARD:
      add_hp += number(1, 5);
      add_mana += number(1, 6);
      add_move += number(1, 2);
      break;
    case CLASS_MONK:
      add_hp += number(1, 5);
      add_mana += number(1, 5);
      add_move += number(1, 2);
      break;
    case CLASS_BARBARIAN:
      add_hp += number(1, 8);
      add_mana += number(1, 4);
      add_move += number(1, 2);
      break;
      /*
        case CLASS_ASSASSIN: -- MOVED TO REMORT
        add_hp += number(1, 5);
        add_mana += number(1, 5);
        add_move += number(1, 2);
        break;
        */
    case CLASS_PALADIN:
      add_hp += number(1, 7);
      add_mana += number(1, 5);
      add_move += number(1, 2);
      break;
    case CLASS_ANTI_PALADIN:
      add_hp += number(1, 7);
      add_mana += number(1, 5);
      add_move += number(1, 2);
      break;
    case CLASS_DRUID:
      add_hp += number(1, 5);
      add_mana += number(2, 7);
      add_move += number(1, 2);
      break;
    case CLASS_MERCHANT:
      add_hp += number(1, 2);
      add_mana += number(1, 2);
      add_move += number(1, 8);
      break;
    case CLASS_KENSAI:
      add_hp += 1+number(1, 8);
      add_mana += 1+number(1, 6);
      add_move += 1+number(1, 2);
      break;
    case CLASS_ASSASSIN:
      add_hp += 1+number(1, 7);
      add_mana += 1+number(1, 6);
      add_move += 1+number(1, 2);
      break;
    case CLASS_NECROMANCER:
      add_hp += 1+number(1, 5);
      add_mana += 1+number(2, 9);
      add_move += 1+number(1, 2);
      break;
    case CLASS_DEVA:
      add_hp += 1+number(1, 5);
      add_mana += 1+number(2, 9);
      add_move += 1+number(1, 2);
      break;
    default:
      log("Unknown class %d in advance_level",GET_CLASS(ch));
      break;

    }

  if (IS_SCR(ch)) {
    add_hp += 1;
    add_mana += 1;
    add_move += 1;
  }

  if (REMORT_LEVEL(ch) >= TRIPLE_REMORT)
  {
    add_hp += 1;
    add_mana += 1;
    add_move += 1;
  }

  add_hp += racial_info[GET_RACE(ch)].hp_bonus;
  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  add_mana += racial_info[GET_RACE(ch)].mana_bonus;

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += MAX(1, add_mana);

  if (show) {
      mudlogf(CMP, LVL_IMMORT, TRUE, "Levels: %s received stat bonuses %d hitp, %d mana, %d move",
	      GET_NAME(ch), add_hp, add_mana, add_move);

  }

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    }

  if(GET_LEVEL(ch)>LVL_NEWBIE){
    for (i = NUM_WEARS-1; i >=0 ; i--)
         {
         if (GET_EQ(ch, i))
            {
            if(IS_OBJ_STAT(GET_EQ(ch, i), ITEM_NEWBIE))
            	{
            	perform_remove(ch, i);
				}
            }
         }
  }
  save_char(ch, IN_ROOM(ch));

  if (show)
    {
    mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
            "Levels: %s has advanced to level %d.", GET_NAME(ch),GET_LEVEL(ch));
    }
  }


/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
/* 2/27/97, Anduin- Changed multiplier to suit 100 levels */
int backstab_mult(int level)
  {
  if (level <= 0)
    return 1;    /* level 0 */
  else if (level <= 10)
    return 2;
  else if (level <= 20)
    return 3;
  else if (level <= 30)
    return 4;
  else if (level <= 40)
    return 5;
  else if (level <= 50)
    return 6;
  else if (level <= 60)
    return 7;
  else if (level <= 80)
    return 8;
  else if (level <= 90)
    return 9;
  else if (level < LVL_IMMORT)
    return 10;   /* all remaining mortal levels */
  else if (level < LVL_GRGOD)
    return 1;    /* lower immortals */
  else
    return 20;   /* higher gods */
  }


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_
  {
class
  }
bitvectors.
 */

int invalid_class(struct char_data *ch, struct obj_data *obj)
  {
     if (REMORT_LEVEL(ch) == TRIPLE_REMORT)
     {
       return FALSE;
     }

  if ( (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_WAR) && IS_WARRIOR(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_CLE) && IS_CLERIC(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_THI) && IS_THIEF(ch))||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_MAG) && IS_MAGIC_USER(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_RAN) && IS_RANGER(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_BAD) && IS_BARD(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_MON) && IS_MONK(ch)) ||
       /* (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_ASS) && IS_ASSASSIN(ch)) || */
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_BAR) && IS_BARBARIAN(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_PAL) && IS_PALADIN(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_APA) && IS_ANTI_PALADIN(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_DRU) && IS_DRUID(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_MER) && IS_MERCHANT(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_KEN) && IS_KENSAI(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_ASS) && IS_ASSASSIN(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_NEC) && IS_NECROMANCER(ch)) ||
       (OBJ_ANTI_FLAGGED(obj, ITEM_ANTI_DEV) && IS_DEVA(ch)) )
    return 1;
  else
    return 0;
  }

int valid_class_align(struct char_data *ch)
  {
    if (IS_SCR(ch))
    {
      return 1;
    }
     if (REMORT_LEVEL(ch) == TRIPLE_REMORT)
     {
       return 1;
     }

  switch(GET_CLASS(ch))
    {
    case CLASS_RANGER:
      if(IS_EVIL(ch))
        return 0;
      break;
    case CLASS_PALADIN:
      if(!IS_GOOD(ch))
        return 0;
      break;
    case CLASS_ANTI_PALADIN:
      if(!IS_EVIL(ch))
        return 0;
      break;
    case CLASS_DRUID:
      if((GET_ALIGNMENT(ch)>500)||(GET_ALIGNMENT(ch)<-500))
        return 0;
      break;
    case CLASS_DEVA:
      if(IS_EVIL(ch))
        return 0;
      break;
    case CLASS_NECROMANCER:
      if(IS_GOOD(ch))
        return 0;
      break;
    default:
      return 1;
    }
  return 1;
  }




/* 10/08/96, Echo - borrowed from current version of Phoenix.
 *                  Modified to reflect existing class balance.
 */
const float class_exp_multipliers[] =
  {
    1.4, /* WA */
    1.4, /* CL */
    1.4, /* TH */
    1.4, /* MU */
    1.4, /* RA */
    1.4, /* BD */
    1.4, /* MO */
    0.0, /* *UNUSED*  */
    1.4, /* BA */
    1.4, /* PA */
    1.4, /* AP */
    1.4, /* DR */
    0.0, /* Me */
    2.2, /* KEN */
    2.2, /* ASS */
    2.2, /* NEC */
    2.2, /* DEV */
    4.0, /* Immortal */
    4.0, /* God */
    1.0  /* basic nothing class */
  }
  ;

/* 10/08/96, Echo - borrowed from current version of Phoenix,
 *                  for 212 levels.  Typographical errors corrected.
 */
/* 2/27/97, Anduin - Changing back to 100 levels */
const int exp_table[] =
  {
    1,              /* 0 */
    500,
    1000,
    1500,
    2000,
    3000,   /* 5 */
    4000,
    5000,
    6000,
    7000,
    8000,           /* 10 */
    9000,
    10000,
    11000,
    12000,
    13000,   /* 15 */
    20000,
    30000,
    40000,
    50000,
    60000,         /* 20 */
    70000,
    80000,
    90000,
    100000,
    120000,   /* 25 */
    160000,
    200000,
    240000,
    280000,
    320000,       /* 30 */
    360000,
    400000,
    440000,
    480000,
    520000,   /* 35 */
    560000,
    600000,
    640000,
    680000,
    720000,     /* 40 */
    760000,
    800000,
    840000,
    880000,
    920000,   /* 45 */
    960000,
    1140000,
    1580000,
    1820000,
    2000000,     /* 50 */
    2100000,
    2200000,
    2300000,
    2400000,
    2500000,   /* 55 */
    2600000,
    2700000,
    2800000,
    2900000,
    3000000,     /* 60 */
    3300000,
    3600000,
    3900000,
    4200000,
    4500000,   /* 65 */
    4800000,
    5100000,
    5400000,
    5700000,
    6000000,     /* 70 */
    6100000,
    6200000,
    6300000,
    6400000,
    6500000,   /* 75 */
    6600000,
    6700000,
    6800000,
    6900000,
    7000000,     /* 80 */
    7100000,
    7200000,
    7300000,
    7400000,
    7500000,   /* 85 */
    7600000,
    7700000,
    7800000,
    7900000,
    8000000,     /* 90 */
    8500000,
    10000000,
    11000000,
    12000000,
    14000000,   /* 95 */
    16000000,
    18000000,
    20000000,
    25000000,
    30000000,    /* 100 */
    50000000,
    51000000,
    52000000,
    53000000,
    54000000,    /* 105 */
    55000000,
    56000000,
    57000000,
    58000000,
    59000000,   /* 110 */
    60000000,
    60000001,
    60000002,
    60000003,
    60000004,   /* 115 */
    60000005,
    60000006,
    60000007,
    60000008,
    60000009,   /* 120 */
    60000010,
    60000020,
    60000030,
    60000040,
    60000050   /* 125 */

  }
  ;


/* 10/08/96, Echo - original titles[] array removed.
 *   Replaced throughout with
 *       const char *default_title  for titles, and
 *       const float exp_multipliers[NUM_CLASSES],
 *       const int exp_table[LVL_IMPL+1] for experience.
 *
 * original declaration:
 * const struct title_type titles[NUM_CLASSES][LVL_IMPL + 1] =
 {
 ...
 }
;
 */


struct scr_skills_struct scr_skills[] =
{
  // CLASS_WARRIOR       0
  //{CLASS_WARRIOR, SKILL_GUARD, 60},
  // CLASS_CLERIC        1
  // CLASS_THIEF         2
  // CLASS_MAGIC_USER    3
  // CLASS_RANGER        4
  //{CLASS_RANGER, SPELL_ICE_STORM, 50},
  //{CLASS_RANGER, SPELL_CURE_SERIOUS, 70},
  // CLASS_BARD          5
  //{CLASS_BARD, SKILL_TRIP, 51}, 
  //{CLASS_BARD, SPELL_CURE_LIGHT, 30}, 
  // CLASS_MONK          6
  // CLASS_BARBARIAN     8
  //{CLASS_BARBARIAN, SKILL_RAGE, 62},
  // CLASS_PALADIN       9
  // CLASS_ANTI_PALADIN 10
  // CLASS_DRUID        11
  {-1,-1,-1}
};

int find_scr_skill(int class, int spell_num)
{
  int i;
  
  for (i = 0; scr_skills[i].class != CLASS_UNDEFINED; i++)
  {
    if (class != scr_skills[i].class) {
      continue;
    }
    if (spell_num != scr_skills[i].spell_num) {
      continue;
    }
    return scr_skills[i].level;
  }

  return LVL_IMPL+1;
}
