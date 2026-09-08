/* ********************************************************************** * 
 *  File: magic.c                                       Part of CircleMUD * 
 * Usage: low-level functions for magic; spell template code              * 
 *                                                                        * 
 * All rights reserved.  See license.doc for complete information.        * 
 *                                                                        * 
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University * 
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
 * ********************************************************************** */ 
 
 
#include "../localHeader/conf.h" 
#include "../localHeader/sysdep.h" 
 
#include "vnum.h"
#include "structs.h" 
#include "buffer.h"
#include "utils.h" 
#include "comm.h"
#include <arpa/telnet.h>   /* IAC/SB/SE, for the Char.Combat heal frame */ 
#include "spells.h" 
#include "handler.h" 
#include "db.h" 
#include "vnum.h"
#include "dg_scripts.h"
#include "constants.h"
#include "interpreter.h"
 
extern struct room_data *world; 
extern struct obj_data *object_list; 
extern struct char_data *character_list; 
extern struct index_data *obj_index; 
extern struct weather_data weather_info; 
extern struct descriptor_data *descriptor_list; 
extern struct zone_data *zone_table; 
extern int mini_mud; 
extern int pk_allowed; 
extern struct default_mobile_stats *mob_defaults; 
extern int *max_ac_applys; 
extern struct apply_mod_defaults *apmd; 
extern struct spell_info_type *spells;
extern struct time_info_data time_info; 

void clearMemory(struct char_data * ch); 
void weight_change_object(struct obj_data * obj, int weight); 
void justify_mob(struct char_data *mob);
 
/* 
 * Saving throws for: 
 * MCTW 
 *   PARA, ROD, PETRI, BREATH, SPELL 
 *     Levels 0-40 
 */ 
/* 
 * use act() don't include the \r\n, act() will do it for you.
 */
static char *mag_summon_msgs[] = 
{ 
   "\r\n", 
   "$n makes a strange magical gesture; you feel a strong breeze!", 
   "$n animates a corpse!", 
   "$N appears from a cloud of thick blue smoke!", 
   "$N appears from a cloud of thick green smoke!", 
   "$N appears from a cloud of thick red smoke!", 
   "$N disappears in a thick black cloud!", 
   "As $n makes a strange magical gesture, you feel a strong breeze.", 
   "As $n makes a strange magical gesture, you feel a searing heat.", 
   "As $n makes a strange magical gesture, you feel a sudden chill.", 
   "As $n makes a strange magical gesture, you feel the dust swirl.", 
   "$n magically divides!", 
   "$n animates a corpse!",
   "An army rises out of the ground in response to $n's call.",
   "$N comes trotting into the room."
}
; 
 
/* 
 * Use send_to_char, keep \r\n
 */
static char *mag_summon_fail_msgs[] = 
{ 
   "\r\n", 
   "There are no such creatures.\r\n", 
   "You stutter during the invocation.\r\n", 
   "Your fingers fail to form the correct symbols.\r\n", 
   "Your magic fails you.\r\n", 
   "The elements resist!\r\n", 
   "You failed.\r\n", 
   "There is no corpse!\r\n" 
}
; 
 
const byte saving_throws[NUM_CLASSES+1][5][2] = 
{ 
   {{70, 15}, {90, 25}, {75, 20}, {85, 20}, {85, 30}},  /* Warrior */ 
   {{50, 10}, {85, 30}, {65, 25}, {80, 40}, {75, 20}},  /* Cleric */ 
   {{70, 15}, {90, 25}, {75, 20}, {85, 20}, {85, 30}},  /* Thief */ 
   {{70, 40}, {55, 15}, {65, 25}, {75, 35}, {60, 15}},  /* Magic User */ 
   {{70, 15}, {90, 25}, {75, 20}, {85, 20}, {85, 30}},  /* Ranger */ 
   {{65, 40}, {95, 20}, {60, 35}, {80, 55}, {75, 25}},  /* Bard */ 
   {{50, 10}, {85, 30}, {65, 25}, {80, 40}, {75, 35}},  /* Monk */ 
   {{50, 10}, {85, 30}, {65, 25}, {80, 40}, {75, 35}},  /* *UNUSED* */ 
   {{70, 15}, {90, 25}, {75, 20}, {85, 20}, {85, 30}},  /* Barbarian */ 
   {{70, 15}, {90, 25}, {75, 20}, {85, 20}, {85, 30}},  /* Paladin */ 
   {{70, 15}, {90, 25}, {75, 20}, {85, 20}, {85, 30}},  /* Anti-Paladin */ 
   {{50, 10}, {85, 30}, {65, 25}, {80, 40}, {75, 35}},  /* Druid */ 
   {{70, 15}, {90, 25}, {75, 20}, {85, 20}, {85, 30}},  /* Merchant */ 
   {{65, 15}, {80, 25}, {70, 20}, {75, 20}, {75, 30}},  /* Kensai */ 
   {{60, 35}, {80, 15}, {65, 30}, {75, 50}, {70, 20}},  /* Assassin */ 
   {{50, 10}, {55, 15}, {65, 25}, {75, 35}, {60, 20}},  /* Necromancer */
   {{50, 10}, {55, 15}, {65, 25}, {75, 35}, {60, 20}},  /* Deva */ 
   {{70, 15}, {90, 25}, {75, 20}, {85, 20}, {85, 30}},  /* Immortal */ 
   {{70, 15}, {90, 25}, {75, 20}, {85, 20}, {85, 30}},  /* God */ 
   {{75, 20}, {95, 30}, {80, 25}, {90, 25}, {90, 35}}   /* Basic Person */
};
      
      
int mag_savingthrow(struct char_data * ch, int type,int level,int modifier) 
{ 
   int save; 
 
  /*        High - ((Difference +1) * LEVEL) */
  /*               ------------------------- */
  /*                       MAX LEVEL */

   save = (int) (saving_throws[(int)GET_CLASS(ch)][type][0] -
		 (int)((float)(saving_throws[(int)GET_CLASS(ch)][type][0] 
			 - saving_throws[(int)GET_CLASS(ch)][type][1] +1)
		  * (float)((float)GET_LEVEL(ch)/(float)(LVL_HERO-1))));

  /* negative apply_saving_throw values make saving throws better! */ 
   save += GET_SAVE(ch, type)*4;
   save += level*2;
   save += modifier;
  /* throwing a 0 is always a failure */ 
   if (MAX(1, save) < number(0, 100)) 
      {
      if(!IS_NPC(ch))
	 send_to_char(ch,"You resist the magic!!!\r\n");
      act("$n resists the magic.", TRUE, ch, NULL, NULL, TO_ROOM);
      return TRUE; 
      }
   else
      { 
      return FALSE; 
      }
   return FALSE;
} 
 
 
/* affect_update: called from comm.c (causes spells to wear off) */ 
/*
 * is_combat_buff (4.2): the affects that a PLAYER keeps while out of combat.
 *
 * A combat buff you have to re-cast after every walk across a zone is not a
 * tactical choice, it is an upkeep tax, and the tax is paid entirely in
 * out-of-combat time. Debuffs are deliberately NOT here: freezing those would
 * mean a poisoned player could never wait it out, which inverts the game
 * rather than smoothing it.
 *
 * Judgement calls worth stating, since a list like this is where they hide:
 *   - invisibility DECAYS. Mechanically a buff, but it is stealth, and a
 *     permanent out-of-combat invis is a different game.
 *   - The detects, fly, levitate, waterwalk, pass door and water breathe all
 *     DECAY. They are exploration tools; their whole cost is the clock.
 *   - Both ward id ranges are covered (#174-180 plus ward evil #34 / ward
 *     good #181) so half the family cannot behave differently from the other.
 */
int is_combat_buff(int spellnum)
{
   switch (spellnum)
      {
      case SPELL_ZEAL:                  /* 206 activity reward */
      case SPELL_ARMOR:                 /* 1   */
      case SPELL_BLESS:                 /* 3   */
      case SPELL_PROT_FROM_EVIL:        /* 34  ward evil  */
      case SPELL_SANCTUARY:             /* 36  */
      case SPELL_STRENGTH:              /* 39  */
      case SPELL_HASTE:                 /* 52  */
      case SPELL_SHIELD:                /* 62  */
      case SPELL_STONE_SKIN:            /* 63  */
      case SPELL_DRAGON:                /* 67  */
      case SPELL_BARK_SKIN:             /* 75  */
      case SPELL_BLUR:                  /* 78  */
      case SPELL_EAGLE_CLAW:            /* 79  */
      case SPELL_ENHANCED:              /* 86  */
      case SPELL_PROT_FIRE:             /* 174 */
      case SPELL_PROT_COLD:             /* 175 */
      case SPELL_PROT_ELEC:             /* 176 */
      case SPELL_PROT_ENERGY:           /* 177 */
      case SPELL_PROT_ACID:             /* 178 */
      case SPELL_PROT_POISON:           /* 179 */
      case SPELL_PROT_DRAIN:            /* 180 */
      case SPELL_PROT_FROM_GOOD:        /* 181 ward good  */
      case SPELL_FIRESHIELD:            /* 196 */
      /*
       * FERN (53) is here for the same reason as the rest: it does nothing
       * outside combat, so that is where it should be spent. Its duration is
       * combat-denominated (24*level) -- listing it here without that would
       * have made it effectively permanent.
       */
      case SPELL_FERN:                  /* 53  */
         return TRUE;
      default:
         return FALSE;
      }
}

/*
 * 4.2: how long a gifted fern may be held, in days, whatever the holder does
 * with it. Owner ruling 2026-08-22. The in-combat duration (24*level ticks) is
 * the VALUE cap; this is the SHELF LIFE.
 */
#define FERN_WINDOW_DAYS 14

/** Has this affect outlived the fern window? Non-fern affects never have. */
int fern_expired(struct char_data *i, struct affected_type *af)
{
   if (af->type != SPELL_FERN)
      return FALSE;
   if (IS_NPC(i) || !i->player_specials)
      return FALSE;
   if (i->player_specials->fern_expiry <= 0)
      return FALSE;
   return time(0) >= i->player_specials->fern_expiry;
}

/*
 * is_removable_buff (4.2): may the CARRIER drop this affect at will?
 *
 * `unaffect` was an immortal tool that stripped everything. A player has no
 * way to end a buff they no longer want -- sneak past a shopkeeper, then walk
 * into a bar still sneaking; take a bless before a fight that never happens.
 * The buff is theirs, the cost was theirs, and ending it early only ever
 * costs them.
 *
 * The set is deliberately the OUT-OF-COMBAT-PERSISTENT one -- exactly
 * is_combat_buff() -- plus the self-applied skill affects, which behave the
 * same way from the player's side. Kept adjacent to that list on purpose: two
 * lists in two files drift, and the drift would be silent.
 *
 * NOT here, and each for a reason:
 *   - Every debuff. Being able to cancel poison, blind or curse at will is
 *     not a convenience, it is an immunity.
 *   - SKILL_RAGE. Rage is a bargain -- its downside is the price of its
 *     upside. Cancelling it on demand keeps the damage and refunds the cost.
 *     (Legacy already strips it on the idle-void, limits.c:503; that is a
 *     timeout, not a player choice.)
 *   - SKILL_TAME / SKILL_MOUNTED_ATTACK. State shared with another creature;
 *     removing one side leaves the other holding a relationship.
 *   - SKILL_DARKEN / SKILL_LIGHTEN. Room affects, not carried by a character.
 *   - invisibility. It already has its own off switch in `visible`
 *     (do_visible); a second one would be two doors to the same room.
 */
int is_removable_buff(int type)
{
   switch (type)
      {
      case SKILL_SNEAK:
      case SKILL_SHADOW:
      case SKILL_ROVE:
         return TRUE;
      default:
         return is_combat_buff(type);
      }
}

/*
 * Hold this affect's duration this tick? (4.2)
 *
 * PLAYERS ONLY. Mobs share affect_update's loop, and 21+ load/random triggers
 * self-cast armor/bless/sanctuary — freezing those would let every idle mob in
 * the world ratchet its buffs permanently, which is a world change rather than
 * a quality-of-life one.
 */
static int affect_frozen(struct char_data *i, struct affected_type *af)
{
   if (IS_NPC(i))
      return FALSE;
   if (FIGHTING(i))
      return FALSE;
   /* 4.2: frozen unless a whole buff hour of combat has accrued. The counter
      carries across fights, so 150 half-seconds cost an hour however they are
      sliced -- and anything short of that is carried, not forgiven. Sampling
      FIGHTING alone made a buff free whenever the window boundary happened to
      miss the fight. */
   if (COMBAT_PULSES(i) >= PULSES_PER_BUFF_HOUR)
      return FALSE;
   return is_combat_buff(af->type);
}

/* affect_update: called from comm.c (causes spells to wear off) */ 
void affect_update(void) 
{ 
   static struct affected_type *af, *next; 
   static struct char_data *i; 
 
   for (i = character_list; i; i = i->next) 
      for (af = i->affected; af; af = next) 
	 { 
	 next = af->next; 
	 if (af->duration >= 1) 
	    {
	    /* 4.2 fern window: the absolute ceiling outranks everything below,
	       so a fern that has run out of wall-clock lapses even while its
	       combat-denominated duration still has ticks left. */
	    if (fern_expired(i, af))
	       {
	       affect_remove(i, af);
	       continue;
	       }
	    /* 4.2: a player's combat buffs do not burn down out of combat. */
	    if (affect_frozen(i, af))
	       continue;
	    af->duration--; 
	    }
	 else if (af->duration == -1) /* No action */ 
	    af->duration = -1; /* GODs only! unlimited */ 
	 else 
	    { 
	    if ((af->type > 0) && (af->type < MAX_SPELLS)) 
	       if (!af->next || (af->next->type != af->type) || 
		   (af->next->duration > 0)) 
		  if (spells[af->type].wear_off) 
		     { 
		     send_to_char(i, "%s\r\n",spells[af->type].wear_off);
		     } 
	    affect_remove(i, af); 
	    } 

      /* 4.2: spend one buff hour's worth and CARRY the remainder - the
         half-seconds a short fight bought are not forgiven, they wait for
         the next one. Done for EVERY character after its affects have been
         walked: not inside the loop, or the first frozen affect would spend
         the pulses and the rest would ride free; and not only for buff
         holders, or the count would run away while nothing consumed it.
         affect_update runs every SECS_PER_MUD_HOUR, so at most one hour's
         worth can accrue per window and this subtracts at most once. */
      if (COMBAT_PULSES(i) >= PULSES_PER_BUFF_HOUR)
         COMBAT_PULSES(i) -= PULSES_PER_BUFF_HOUR;
	 } 
} 
 

struct mat_components_type mat_components[] = {
   {SPELL_WEB, BIT_WEB,EXTRACT,-1,0,-1,0,TRUE},
   {-1,        -1,     0,      -1,0,-1,0,FALSE}
};
 
/* 
 *  mag_materials: 
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory. 
 * 
 * No spells implemented in Circle 3.0 use mag_materials, but you can use 
 * it to implement your own spells which require ingredients (i.e., some 
 * heal spell which requires a rare herb or some such.) 
 */ 
int mag_materials(struct char_data * ch, int spellnum,int level)
{ 
   struct obj_data *tobj; 
   struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL; 
   int i;
   int check1,check2,check0;
   for(i=0;mat_components[i].spell_num!=-1;i++)
      if(mat_components[i].spell_num==spellnum)
	 break;
   if(mat_components[i].spell_num==-1)
      {
      log("SYSERR: unknown spell %d passed to mag_materials", spellnum);
      return FALSE;
      }
   
   check0=mat_components[i].item0;
   check1=mat_components[i].item1;
   check2=mat_components[i].item2;

   for (tobj = ch->carrying; tobj; tobj = tobj->next_content) 
      { 
      if ((check0 > 0) && 
	  (GET_OBJ_VNUM(tobj) == mat_components[i].item0)) 
	 { 
	 obj0 = tobj; 
	 check0 = -1; 
	 } 
      else if ((check1 > 0) && 
	       (GET_OBJ_VNUM(tobj) == mat_components[i].item1)) 
	 { 
	 obj1 = tobj; 
	 check1 = -1; 
	 } 
      else if ((check2 > 0) && 
	       (GET_OBJ_VNUM(tobj) == mat_components[i].item2)) 
	 { 
	 obj2 = tobj; 
	 check2 = -1; 
	 } 
      } 
   if ((check0 > 0) || (check1 > 0) || (check2 > 0)) 
      { 
      if (mat_components[i].verbose) 
	 { 
	 switch (number(0, 2)) 
	    { 
	     case 0: 
		send_to_char(ch, "A wart sprouts on your nose.\r\n"); 
		break; 
	     case 1: 
		send_to_char(ch,"Your hair falls out in clumps.\r\n"); 
		break; 
	     case 2: 
		send_to_char(ch,"A huge corn develops on your big toe.\r\n");
		break; 
	    } 
	 } 
      return (FALSE); 
      } 

   if(obj0!=NULL)
      {
      if(mat_components[i].wear0==EXTRACT)
	 GET_OBJ_CSLOTS(obj0)=-1;
      else if(mat_components[i].wear0 > 0)
	 GET_OBJ_CSLOTS(obj0)
	    =MAX(-1, GET_OBJ_CSLOTS(obj0)-(mat_components[i].wear0*level));
      if(GET_OBJ_CSLOTS(obj0) < 0)
	 {
	 obj_from_char(obj0); 
	 extract_obj(obj0); 
	 }
      }
   if(obj1!=NULL)
      {
      if(mat_components[i].wear1==EXTRACT)
	 GET_OBJ_CSLOTS(obj1)=-1;
      else if(mat_components[i].wear1 > 0)
	 GET_OBJ_CSLOTS(obj1)
	    =MAX(-1,GET_OBJ_CSLOTS(obj1)-(mat_components[i].wear1*level));
      if(GET_OBJ_CSLOTS(obj1) < 0)
	 {
	 obj_from_char(obj1); 
	 extract_obj(obj1); 
	 }
      }
   if(obj2!=NULL)
      {
      if(mat_components[i].wear2==EXTRACT)
	 GET_OBJ_CSLOTS(obj2)=-1;
      else if(mat_components[i].wear2 > 0)
	 GET_OBJ_CSLOTS(obj2)
	    =MAX(-1,GET_OBJ_CSLOTS(obj2)-(mat_components[i].wear2*level));
      if(GET_OBJ_CSLOTS(obj2) < 0)
	 {
	 obj_from_char(obj2); 
	 extract_obj(obj2); 
	 }
      }

   if (mat_components[i].verbose) 
      { 
      send_to_char(ch,"A puff of smoke rises from your pack.\r\n"); 
      act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM); 
      } 
   return (TRUE); 
} 
 
 
 
 
/* 
 * Every spell that does damage comes through here.  This calculates the 
 * amount of damage, adds in any modifiers, determines what the saves are, 
 * tests for save and calls damage(). 
 *
 * -1 = dead, otherwise the amount of damage done.
 */ 
 
int mag_damage(int level, struct char_data * ch, struct char_data * victim, 
		int spellnum, int savetype) 
{ 
   int dam = 0; 
   int imm_type;
   if (victim == NULL || ch == NULL) 
      return 0; 
   level = MAX(MIN(level, 10), 1); 
 
 
   switch (spellnum) 
      { 
     /* Mostly mages */ 
       case SPELL_MAGIC_MISSILE: 
	  if (IS_MAGIC_USER(ch)) 
	     dam = dice(level, 8) + 1; 
	  else 
	     dam = dice(level, 6) + 1; 
	  imm_type=IMM_PIERCE;
	  break; 
       case SPELL_CHILL_TOUCH: /* chill touch also has an affect */ 
	  if (IS_MAGIC_USER(ch)) 
	     dam = dice(level, 8) + 1; 
	  else 
	     dam = dice(level, 6) + 1; 
	  imm_type=IMM_COLD;
	  break; 
       case SPELL_BURNING_HANDS: 
	  if (IS_MAGIC_USER(ch)) 
	     dam = dice(2+level, 8) + 3; 
	  else 
	     dam = dice(2+level, 6) + 3; 
	  imm_type=IMM_FIRE;
	  break; 
       case SPELL_SHOCKING_GRASP: 
	  if (IS_MAGIC_USER(ch)) 
	     dam = dice(3+level, 8) + 5; 
	  else 
	     dam = dice(3+level, 6) + 5; 
	  imm_type=IMM_ELEC;
	  break; 
       case SPELL_LIGHTNING_BOLT: 
	  if (IS_MAGIC_USER(ch)) 
	     dam = dice(5+level, 8) + 7; 
	  else 
	     dam = dice(5+level, 6) + 7; 
	  imm_type=IMM_ELEC;
	  break; 
       case SPELL_COLOR_SPRAY: 
	  if (IS_MAGIC_USER(ch)) 
	     dam = dice(7+level, 8) + 9; 
	  else 
	     dam = dice(7+level, 6) + 9; 
	  imm_type=IMM_ENERGY;
	  break; 
       case SPELL_FIREBALL: 
	  if (IS_MAGIC_USER(ch)) 
	     dam = dice(9+level, 8) + 11; 
	  else 
	     dam = dice(9+level, 6) + 11; 
	  imm_type=IMM_FIRE;
	  break; 
 
       case SPELL_FIRE_SONG: 
	  dam = dice(12+level,14) + 15; 
	  imm_type=IMM_FIRE;
	  break; 
       case SPELL_GAS_BLAST: 
	  dam = dice(18+level,19) + 19; 
	  imm_type=IMM_POISON;
	  break; 
       case SPELL_FROST_BLAST: 
	  dam = dice(13+level,15) + 20; 
	  imm_type=IMM_COLD;
	  break; 
 
       case SPELL_FLAME_STRIKE: 
	  dam=dice(12+level,14)+20; 
	  imm_type=IMM_FIRE;
	  break; 
   
       case SPELL_ACID_BLAST: 
	  dam= dice(14+level,16)+20; 
	  imm_type=IMM_ACID;
	  break; 
 
	 /* Mostly clerics */ 
       case SPELL_DISPEL_EVIL: 
	  dam = dice(6+level, 8) + 6; 
	  if (IS_EVIL(ch)) 
	     { 
	     victim = ch; 
	     dam = (GET_HIT(ch)/(11-level)) - 1; 
	     } 
	  else if (IS_GOOD(victim)) 
	     { 
	     act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR); 
	     return 0; 
	     } 
	  imm_type=IMM_HOLY;
	  break; 
       case SPELL_DISPEL_GOOD: 
	  dam = dice(6+level, 8) + 6; 
	  if (IS_GOOD(ch)) 
	     { 
	     victim = ch; 
	     dam = (GET_HIT(ch)/(11-level)) - 1; 
	     } 
	  else if (IS_EVIL(victim)) 
	     { 
	     act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR); 
	     return 0; 
	     } 
	  imm_type=IMM_UNHOLY;
	  break; 
 
 
       case SPELL_CALL_LIGHTNING: 
	  dam = dice(10+level, 12) + 15; 
	  imm_type=IMM_ELEC;
	  break; 
 
       case SPELL_CAUSE_LIGHT: 
	  dam = dice(level, 6) + 1; 
	  imm_type=IMM_DRAIN;
	  break; 
 
       case SPELL_CAUSE_SERIOUS: 
	  dam = dice(2*level, 6) + 5; 
	  imm_type=IMM_DRAIN;
	  break; 
 
       case SPELL_CAUSE_CRITIC: 
	  dam = dice(3*level, 6) + 10; 
	  imm_type=IMM_DRAIN;
	  break; 
 
       case SPELL_HARM: 
	  dam = dice(4*level, 6) + 20; 
	  imm_type=IMM_UNHOLY;
	  break; 
 
       case SPELL_ENFEEBLE: 
	  dam = dice(12+level, 14) + 20; 
	  imm_type=IMM_DRAIN;
	  break; 
 
       case SPELL_WRATH_OF_GOD: 
	  dam = dice(19+level,19)+ 25; 
	  imm_type=IMM_HOLY;
	  break; 
	  
       case SPELL_GRANT_PEACE:
	  dam = (int)((float)GET_HIT(victim) * (float)( ((float)GET_LEVEL(ch)) * (float)(level)) /2000.0);
	  imm_type=IMM_HOLY;
	  break;

	 /* Area spells */ 
       case SPELL_EARTHQUAKE: 
	  dam = dice(1+level, 8) + level; 
	  imm_type=IMM_ENERGY;
	  break; 
       case SPELL_FIRE_BREATH: 
	  if(IS_NPC(ch))
	     dam = dice(14+level, 14) +15; 
	  else
	     dam = dice(9+level, 8) + 11; 
	  imm_type=IMM_FIRE;
	  break; 
       case SPELL_FROST_BREATH: 
	  dam = dice(14+level, 14) +15; 
	  imm_type=IMM_COLD;
	  break; 
       case SPELL_GAS_BREATH: 
	  dam = dice(14+level, 14) +15; 
	  imm_type=IMM_POISON;
	  break; 
       case SPELL_LIGHTNING_BREATH: 
	  dam = dice(14+level, 14) +15; 
	  imm_type=IMM_ELEC;
	  break; 
       case SPELL_ACID_BREATH: 
	  dam = dice(14+level, 14) +15; 
	  imm_type=IMM_ACID;
	  break; 

       case SPELL_SUNRAY: 
          //dam = dice(18+level, 15) + 20; 
	  dam = dice(10+2*level, 14) + 25; 
	  imm_type=IMM_FIRE;
	  break; 
       case SPELL_METEOR_STORM:
	  dam = dice(2*level, 15) +10;
	  imm_type = IMM_FIRE;
	  break;
       case SPELL_ICE_STORM:
	  dam = dice(2*level, 18) +20;
	  imm_type = IMM_COLD;
	  break;
       default: 
	  log("SYSERR: unknown spellnum %d passed to mag_damage",
		  spellnum); 
	  return 0; 
	  break; 
 
      } 
/* switch(spellnum) */ 
 
 
  /* divide damage by two if victim makes his saving throw */ 
   if (mag_savingthrow(victim, savetype, level,0)) 
      {
      dam /= 2; 
      }
  /* and finally, inflict the damage */ 
   return damage(ch, victim, dam, spellnum,imm_type); 
} 
 
 
/* 
 * Every spell that does an affect comes through here.  This determines 
 * the effect, whether it is added or replacement, whether it is legal or 
 * not, etc. 
 * 
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod) 
*/ 
 
#define MAX_SPELL_AFFECTS 5 /* change if more needed */ 
 
void mag_affects(int level, struct char_data * ch, struct char_data * victim, 
		 int spellnum, int savetype) 
{ 
   struct affected_type af[MAX_SPELL_AFFECTS]; 
   struct affected_type *aftemp;
   struct char_data *tch, *tch_next; 
   bool accum_affect = FALSE, accum_duration = FALSE; 
   char *to_vict = NULL, *to_room = NULL; 
   int i; 
   int save=0;
   int chance=0, roll=0; 


   if (victim == NULL || ch == NULL) 
      return; 
   level = MAX(MIN(level, 10), 1); 
 
 
   for (i = 0; i < MAX_SPELL_AFFECTS; i++) 
      { 
      af[i].type = spellnum; 
      af[i].bitvector = 0; 
      af[i].modifier = 0; 
      af[i].location = APPLY_NONE; 
      af[i].spell_level = level;
      } 
 
   switch (spellnum) 
      { 
 
       case SPELL_CHILL_TOUCH: 
	  af[0].location = APPLY_STR; 
	  if (mag_savingthrow(victim, savetype,level,0)) 
	     af[0].duration = 1; 
	  else 
	     af[0].duration = level; 
	  af[0].modifier = -1*level; 
	  accum_duration = TRUE; 
	  accum_affect = FALSE; 
	  to_vict = "You feel your strength wither!"; 
	  break; 
 
       case SPELL_ARMOR: 
	  af[0].location = APPLY_AC; 
	  af[0].modifier = -3*level; 
	  af[0].duration = 5*level; 
	  accum_duration = FALSE; 
	  to_vict = "You feel someone protecting you."; 
	  break; 
 
       case SPELL_SHIELD: 
	  af[0].location = APPLY_AC; 
	  af[0].duration =  5*level; 
	  af[0].modifier = -4*level; 
	  accum_duration=FALSE; 
	  to_vict = "A force field surrounds you."; 
	  to_room = "$n is surrounded by a shimmering light."; 
	  break; 
  
       case SPELL_STONE_SKIN: 
	  af[0].location = APPLY_AC; 
	  af[0].duration = 5*level; 
	  af[0].modifier = -8*level; 
	  af[1].location = APPLY_DEX;
	  af[1].duration = 5*level;
	  af[1].modifier = -1*(level/3+1);
	  accum_duration=FALSE; 
	  to_vict = "Your skin turns to stone."; 
	  to_room = "$n's skin looks like it has turned to stone."; 
	  break; 
 
       case SPELL_BARK_SKIN: 
	  af[0].location = APPLY_AC; 
	  af[0].duration =  5*level; 
	  af[0].modifier = -6*level; 
	  af[1].location = APPLY_DEX;
	  af[1].duration = 5*level;
	  af[1].modifier = -1*(level/5+1);
	  accum_duration=FALSE; 
	  to_vict = "Your skin turns hard and rough like bark"; 
	  to_room = "$n's skin has turned into bark!!"; 
	  break; 
 
       case SPELL_BLESS: 
	  af[0].location = APPLY_HITROLL; 
	  af[0].modifier = level/3+1; 
	  af[0].duration = 5*level; 
 
	  af[1].location = APPLY_SAVING_SPELL; 
	  af[1].modifier = -1*(1+level/3); 
	  af[1].duration = 5*level; 
 
	  accum_duration = FALSE; 
	  to_vict = "You feel righteous."; 
	  break; 
 
       case SPELL_BLINDNESS: 
	  if (MOB_FLAGGED(victim,MOB_NOBLIND) ||
	      mag_savingthrow(victim, savetype, level,0)) 
	     { 
	     send_to_char(ch,"You fail.\r\n"); 
	     return; 
	     } 
 
	  af[0].location = APPLY_HITROLL; 
	  af[0].modifier = -4; 
	  af[0].duration = level/2+1; 
	  af[0].bitvector = AFF_BLIND; 
 
	  af[1].location = APPLY_AC; 
	  af[1].modifier = 40; 
	  af[1].duration = level/2+1; 
	  af[1].bitvector = AFF_BLIND; 
	  if(IS_NPC(victim))
	     {
	     af[0].duration/=3;
	     af[1].duration/=3;
	     }
	  to_room = "$n seems to be blinded!"; 
	  to_vict = "You have been blinded!"; 
	  break; 
 
       case SPELL_CURSE: 
	  if (mag_savingthrow(victim, savetype, level,0)) 
	     { 
	     send_to_char(ch, "%s", NOEFFECT); 
	     return; 
	     } 
 
	  af[0].location = APPLY_HITROLL; 
	  af[0].duration = 1 + level*2; 
	  af[0].modifier = -1*(1+level/2); 
	  af[0].bitvector = AFF_CURSE; 
 
	  af[1].location = APPLY_DAMROLL; 
	  af[1].duration = 1 + level*2; 
	  af[1].modifier = -1*(1+level/2);
	  af[1].bitvector = AFF_CURSE; 
 
	  accum_duration = TRUE; 
	  accum_affect = FALSE; 
	  to_room = "$n briefly glows red!"; 
	  to_vict = "You feel very uncomfortable."; 
	  break; 
 
       case SPELL_DETECT_ALIGN: 
	  af[0].duration =level*5; 
	  af[0].bitvector = AFF_DETECT_ALIGN; 
	  accum_duration = FALSE; 
	  to_vict = "Your eyes tingle."; 
	  break; 
 
       case SPELL_DETECT_INVIS: 
	  af[0].duration = level*5; 
	  af[0].bitvector = AFF_DETECT_INVIS; 
	  accum_duration = FALSE; 
	  to_vict = "Your eyes tingle."; 
	  break; 
 
       case SPELL_DETECT_MAGIC: 
	  af[0].duration = level*5; 
	  af[0].bitvector = AFF_DETECT_MAGIC; 
	  accum_duration = FALSE; 
	  to_vict = "Your eyes tingle."; 
	  break; 
 
       case SPELL_INFRAVISION: 
	  af[0].duration = level*5; 
	  af[0].bitvector = AFF_INFRAVISION; 
	  accum_duration = FALSE; 
	  to_vict = "Your eyes glow red."; 
	  to_room = "$n's eyes glow red."; 
	  break; 
 
       case SPELL_INVISIBLE: 
	  if (!victim) 
	     victim = ch; 
 
	  af[0].duration = level*5; 
	  af[0].modifier = -40; 
	  af[0].location = APPLY_AC; 
	  af[0].bitvector = AFF_INVISIBLE; 
	  accum_duration = FALSE; 
	  to_vict = "You vanish."; 
	  to_room = "$n slowly fades out of existence."; 
	  break; 
 
       case SPELL_POISON: 
	  if (mag_savingthrow(victim, savetype,level,0)) 
	     { 
	     send_to_char(ch, "%s", NOEFFECT); 
	     return; 
	     } 
 
	  af[0].location = APPLY_STR; 
	  af[0].duration = level*3; 
	  af[0].modifier = -1*(1+level/3); 
	  af[0].bitvector = AFF_POISON; 
	  to_vict = "You feel very sick."; 
	  to_room = "$n gets violently ill!"; 
	  break; 
 
       case SPELL_PROT_FROM_EVIL: 
	  if(AFF_FLAGGED(victim,AFF_PROTECT_GOOD))
	     {
	     send_to_char(ch, "%s", NOEFFECT); 
	     return;
	     }
	  af[0].duration = level*2; 
	  af[0].bitvector = AFF_PROTECT_EVIL; 
	  accum_duration = FALSE; 
	  to_vict = "You take on a dark visage to conceal your true aura."; 
	  to_room = "$n takes on a dark visage.";
	  break; 
       case SPELL_PROT_FROM_GOOD: 
	  if(AFF_FLAGGED(victim,AFF_PROTECT_EVIL))
	     {
	     send_to_char(ch, "%s", NOEFFECT); 
	     return;
	     }
	  af[0].duration = level*2; 
	  af[0].bitvector = AFF_PROTECT_GOOD; 
	  accum_duration = FALSE; 
	  to_vict = "You take on a saintly visage to conceal your true aura.";
	  to_room = "$n takes on a saintly aura.";
	  break; 
 
       case SPELL_SANCTUARY: 
	  af[0].duration = 5+(level/2); 
	  af[0].bitvector = AFF_SANCTUARY; 
 
	  accum_duration = FALSE; 
	  to_vict = "A white aura momentarily surrounds you."; 
	  to_room = "$n is surrounded by a white aura."; 
	  break; 
 
       case SPELL_HASTE: 
	  af[0].duration = level; 
	  af[0].bitvector = AFF_HASTE; 
	  accum_duration = FALSE; 
	  to_vict = "Your speed increases tremendously."; 
	  to_room = "$n begins to move very rapidly."; 
	  break; 
 
       case SPELL_BLUR: 
	  af[0].bitvector = AFF_HASTE; 
	  af[0].duration = level+3; 
	  accum_duration = FALSE; 
	  to_vict = "You begin to move so fast that you blur!!"; 
	  to_room = "$n begins to move so fast that $e blurs!!"; 
	  break; 
 
       case SPELL_ZEAL:
          /*
           * Duration is fixed, not level-scaled: the potion is earned for
           * playing rather than bought or cast, so the reward must not read
           * differently to a level 5 and a level 100. 60 ticks is 1.25 hours
           * of actual combat -- it is combat-denominated (is_combat_buff), so
           * a character who logs off holding it still has it.
           *
           * No location or modifier. The experience bonus is read off the
           * affect's PRESENCE in gain_exp, so there is no stat for a display
           * to show and nothing for score to double-count.
           */
          af[0].duration = ZEAL_COMBAT_TICKS;
          accum_duration = FALSE;
          to_vict = "Fervour takes you. Your victories will teach you more.";
          to_room = "$n burns with a quiet fervour.";
          break;

       case SPELL_FERN:
          if (GET_LEVEL(ch) >= LVL_GRGOD)
             {
             /*
              * fern is denominated in COMBAT time, not wall time. Combat buffs
              * no longer burn out of combat (is_combat_buff), so leaving it
              * wall-denominated would have made it permanent for anyone who
              * simply stopped fighting. 24*level = 240 ticks at the level-10
              * grant, about 5 hours of actual combat. (Owner ruling
              * 2026-09-08 restored the multiplier to 24; the 4.2 change had
              * cut it to 6. The FERN_WINDOW_DAYS ceiling below is unchanged.)
              */
             af[0].location = APPLY_HITROLL;
             af[0].duration = 24*level;
             af[0].modifier = 10*level; /* 10hr/dr per level. */
             af[1].location = APPLY_DAMROLL;
             af[1].duration = 24*level;
             af[1].modifier = 10*level;
             /*
              * ...and a wall-clock ceiling on top, because a character who
              * never fights never burns a combat tick. This is the bound that
              * is spent whether the fern is used or not.
              */
             victim->player_specials->fern_expiry =
                time(0) + (time_t)FERN_WINDOW_DAYS * 24 * 60 * 60;
             to_vict = "You feel incredibly more powerful as you are blessed with FERN!!";
             to_room = "$n begins to radiate with incredible power!!";
             }
          else if (GET_LEVEL(ch) < LVL_GRGOD)
             {
             send_to_char(ch, "Sorry, but you can't give out ferns yet!\r\n");
             return;
             }
          break;
 
       case SPELL_SLOW: 
	  af[0].bitvector = AFF_SLOW; 
	  af[0].duration = level; 
	  to_vict = "You begin to feel very slow."; 
	  to_room = "$n begins to move very slowly."; 
	  break; 
 
       case SPELL_DEPRESSION: 
	  af[0].bitvector = AFF_SLOW; 
	  af[0].duration = level+3; 
	  to_vict = "You suddenly feel a wave of depression come over you."; 
	  to_room = "$n begins to look pretty bummed."; 
	  break; 
 
       case SPELL_WITHER: 
	  af[0].bitvector = AFF_SLOW; 
	  af[0].duration = level+3; 
	  af[1].bitvector = AFF_SLOW; 
	  af[1].location = APPLY_STR; 
	  af[1].duration = level+3; 
	  af[1].modifier = -1*(1+level/2); 
	  to_vict = "All of the energy flows out of your limbs."; 
	  to_room = "$n begins to look very weak."; 
	  break; 
 
 
       case SPELL_SLEEP: 
	  if (!pk_allowed && !IS_NPC(ch) && !IS_NPC(victim)) 
	     return; 
	  if (MOB_FLAGGED(victim, MOB_NOSLEEP)) 
	     return; 
	  if (mag_savingthrow(victim, savetype,level,0)) 
	     return; 
 
	  af[0].duration = level; 
	  af[0].bitvector = AFF_SLEEP; 
 
	  if (GET_POS(victim) > POS_SLEEPING) 
	     { 
	     if(FIGHTING(victim))
		stop_fighting(victim);
	     for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next) 
		{ 
		tch_next = tch->next_in_room; 
		if (tch == ch) 
		   continue; 
		if(FIGHTING(tch)==victim)
		   stop_fighting(tch);
		}
	     send_to_char(victim,"You feel very sleepy...  Zzzz......\r\n"); 
	     to_room = "$n goes to sleep."; 
	     GET_POS(victim) = POS_SLEEPING; 

	     } 
	  break; 
 
       case SPELL_LULLABY: 
	  if (!victim) 
	     return; 
	  if (!pk_allowed && !IS_NPC(ch) && !IS_NPC(victim)) 
	     return; 
          if (MOB_FLAGGED(victim, MOB_NOSLEEP))
             return;
	  if (mag_savingthrow(victim, savetype,level,0)) 
	     return; 
	  if (GET_LEVEL(victim) > GET_LEVEL(ch)) 
	     return; 
	  af[0].bitvector = AFF_SLEEP; 
	  af[0].duration = 4 + level; 
	  if (GET_POS(victim) > POS_SLEEPING) 
	     { 
	     if(FIGHTING(victim))
		stop_fighting(victim);
	     for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next) 
		{ 
		tch_next = tch->next_in_room; 
		if(FIGHTING(tch)==victim)
		   stop_fighting(tch);
		}
	     to_vict = "A song begins to make you feel very sleepy...zzzzzz"; 
	     to_room = "$n is sung to sleep."; 
	     GET_POS(victim) = POS_SLEEPING; 
	     } 
	  break; 
 
 
       case SPELL_STRENGTH: 
	  af[0].location = APPLY_STR; 
	  af[0].duration = level*3; 
	  af[0].modifier = (level/3)+1; 
	  accum_duration = FALSE; 
	  accum_affect = FALSE; 
	  to_vict = "You feel stronger!"; 
	  break; 
 
       case SPELL_ENHANCED: 
	  af[0].location = APPLY_STR; 
	  af[0].duration = level*2; 
	  af[0].modifier = level+3; 
	  accum_duration = FALSE; 
	  accum_affect = FALSE; 
	  to_vict = "Your muscles bulge to a tremendous size."; 
	  to_room = "$n's muscles ripple as they strengthen."; 
	  break; 
 
 
       case SPELL_SENSE_LIFE: 
	  to_vict = "Your feel your awareness improve."; 
	  af[0].duration = level*5; 
	  af[0].bitvector = AFF_SENSE_LIFE; 
	  accum_duration = FALSE; 
	  break; 
 
       case SPELL_WATERWALK: 
	  af[0].duration = level*5; 
	  af[0].bitvector = AFF_WATERWALK; 
	  accum_duration = FALSE; 
	  to_vict = "You feel webbing between your toes."; 
	  break; 
 
       case SPELL_WATER_BREATHE: 
	  af[0].bitvector = AFF_WATER_BREATHE; 
	  af[0].duration = level*5; 
	  accum_duration = FALSE; 
	  to_vict = "You begin to grow gills."; 
	  to_room = "$n begins to grow gills! How Odd!"; 
	  break; 
 
 
       case SPELL_LEVITATE: 
	  af[0].bitvector = AFF_LEV; 
	  af[0].duration = level*5; 
	  accum_duration = FALSE; 
	  to_vict = "Your feet rise off the ground."; 
	  to_room = "$n's feet rise an inch off the ground."; 
	  break; 

       case SPELL_FLY: 
	  af[0].location = APPLY_FLY; 
	  af[0].duration = level*5;
	  accum_duration = FALSE; 
	  to_vict = "Wings sprout from your back and you soar into the air!"; 
	  to_room = "Wings sprout from $n's back!";
          SET_BIT(AFF2_FLAGS(ch), AFF2_FLYING);  
	  break; 
 
       case SPELL_DREAM_SIGHT: 
	  af[0].bitvector = AFF_DREAM; 
	  af[0].duration = level*5; 
	  to_vict = "You are much more aware of your surroundings."; 
	  to_room = "$n's eyelids turn translucent."; 
	  break; 
 
       case SPELL_DRAGON: 
	  af[0].location = APPLY_HITROLL; 
	  af[0].duration = 5; 
	  af[0].modifier = 1+level/2; 
	  af[1].location = APPLY_DAMROLL; 
	  af[1].duration = 5; 
	  af[1].modifier = 1+level/2; 
	  to_vict = "You suddenly feel more powerful!"; 
	  to_room = "$n begins to glow with a golden light."; 
	  break;
 
       case SPELL_EAGLE_CLAW: 
	  af[0].location = APPLY_HITROLL; 
	  af[0].duration = 5; 
	  af[0].modifier = 2+level/2; 
	  af[1].location = APPLY_DAMROLL; 
	  af[1].duration = 5; 
	  af[1].modifier = level/2; 
	  to_vict = "You suddenly feel more powerful!"; 
	  to_room = "$n begins to glow with a blue aura."; 
	  break; 
 
 
 
 
       case SPELL_INSPIRE: 
	  chance = 0; 
	  roll = 1; 
	  af[0].location = APPLY_DAMROLL; 
	  af[0].duration = level*3; 
	  chance = dice(1, 10); 
	  if (chance > 9) 
	     roll = 7; 
 
	  if ((chance <= 9) && (chance >= 7)) 
	     roll = 5; 
 
	  if ((chance <= 6) && (chance >= 4)) 
	     roll = 3; 
 
	  if ((chance <= 3) && (chance >= 2)) 
	     roll = -3; 
	  if (chance <= 1) 
	     roll = -7; 
	  af[0].modifier = roll; 
	  af[0].location = APPLY_DAMROLL; 
	  if (roll > 0)
	     { 
	     to_vict = "You suddenly feel inspired."; 
	     to_room = "$n suddenly gets a look of inspiration."; 
	     } 
	  if (roll < 0)
	     { 
	     to_vict = "You suddenly feel very uninspired."; 
	     to_room = "$n suddenly looks very uninspired."; 
	     } 
	  break; 
 
       case SPELL_PIXIE_DUST: 
	  af[0].location = APPLY_HITROLL; 
	  af[0].duration = level; 
	  af[0].modifier = ((-1*level)/2)-1; 
	  af[1].location = APPLY_DAMROLL; 
	  af[1].duration = level; 
	  af[1].modifier = (-1*level)/2; 
	  to_vict = "Ouch, you suddenly feel like a chicken could beat you up!"; 
	  to_room = "$n is surrounded by a shower of sparkling light."; 
	  break; 
 
       case SPELL_DISPEL_MAGIC: 
	  if (!victim) 
	     return; 
	  if ((GET_LEVEL(ch)+level) < GET_LEVEL(victim))
	     { 
	     send_to_char(ch,"You are not powerful enough to dispel their magic.\r\n"); 
	     return; 
	     } 
	  if (!AFF_FLAGGED(victim, AFF_SANCTUARY))
	     { 
	     send_to_char(ch,"A flash of light and nothing happens.\r\n"); 
	     return; 
	     } 

          for (aftemp = victim->affected; aftemp; aftemp = aftemp->next)
             if (aftemp->bitvector == AFF_SANCTUARY)
                {
                   affect_remove(victim, aftemp); 
                   break;
                }
          affect_total(victim); /* put this after REMOVE_BIT to keep object affected SANC */
          REMOVE_BIT(victim->char_specials.saved.affected_by, AFF_SANCTUARY); 
	  send_to_char(victim,"You keel over in pain!\r\n"); 
	  act("The white light around $n slowly begins to fade.", TRUE, victim, 0, 0, TO_ROOM); 
	  return; 
	  break; 
 
       case SPELL_PASS_DOOR: 
	  af[0].bitvector = AFF_PASS_DOOR; 
	  af[0].duration = level; 
	  to_vict = "You slowly fade into a translucent form."; 
	  to_room = "$n's slowly becomes translucent."; 
	  break; 
 
       case SPELL_PLAGUE: 
	  af[0].bitvector = AFF_PLAGUE; 
	  af[0].duration = 80; 
	  accum_duration=TRUE;
	  to_vict = "You are suddenly overcome with pain as horrible sores burst out."; 
	  to_room = "$n's turns green as huge sores break out."; 
	  break; 
       case SPELL_WEB:
	  if (mag_savingthrow(victim, savetype,level,0))
	     save=level/2;
	  else
	     save = level;
	  save =MAX(save,1);
	  af[0].bitvector = AFF_NO_FLEE|AFF_SLOW; 
	  af[0].duration = save; 
	  af[1].location = APPLY_DEX;
	  af[1].duration = save;
	  af[1].modifier = -1*(save/2+1);

	  to_vict = "You are entrapped by the web shooting from $N's fingers.";
	  to_room = "$n is snared in $N's web."; 
	  break;
       case SPELL_ENLIVEN:
          if (mag_savingthrow(victim, savetype,level,0))
             save = 1;
          else
             save = (level/2)+2;
          af[0].bitvector = AFF_NO_FLEE;
          af[0].duration = save;
          af[1].location = APPLY_DEX;
          af[1].duration = save;
          af[1].modifier = -1*(save/3+1);

          to_vict = "You find yourself entangled in the vine's growth!";
          to_room = "$n is entangled in the vine's growth!";
          break;
       case SPELL_ENTANGLE:
	  if (mag_savingthrow(victim, savetype,level,0))
	     save=level/2;
	  else
	     save = level;
	  save =MAX(save,1);
	  af[0].bitvector = AFF_NO_FLEE; 
	  af[0].duration = save+4; 
	  af[1].location = APPLY_DEX;
	  af[1].duration = save+4;
	  af[1].modifier = -1*(save/3+1);

	  to_vict = "Vines sprout out of the floor and wrap around your legs.";
	  to_room = "Vines sprout out of the floor and snare $n."; 
	  break;

       case SPELL_FAERIE_FIRE:
	  if (mag_savingthrow(victim, savetype,level,0))
	     save=level/2;
	  else
	     save = level;
	  save = MAX(save,1);
	  af[0].location = APPLY_AC;
	  af[0].modifier = 5*save; 
	  af[0].duration = save*3; 
	  af[1].location = APPLY_LIGHT;
	  af[1].duration = save*3;
	  af[1].modifier = save*2;

	  to_vict = "You are surrounded by a rainbow eruption of light.";
	  to_room = "$n glows with a multicolored fire."; 
	  break;

       case SPELL_PROT_FIRE: 
	  af[0].location = APPLY_RESIST; 
	  af[0].modifier = IMM_FIRE; 
	  af[0].duration = 5*level; 
	  accum_duration = FALSE; 
	  to_vict = "You have a nice cool feeling."; 
	  break; 
       case SPELL_PROT_COLD: 
	  af[0].location = APPLY_RESIST; 
	  af[0].modifier = IMM_COLD; 
	  af[0].duration = 5*level; 
	  accum_duration = FALSE; 
	  to_vict = "You have a nice warm feeling."; 
	  break; 
       case SPELL_PROT_ELEC: 
	  af[0].location = APPLY_RESIST; 
	  af[0].modifier = IMM_ELEC; 
	  af[0].duration = 5*level; 
	  accum_duration = FALSE; 
	  to_vict = "You feel grounded."; 
	  break; 
       case SPELL_PROT_ENERGY: 
	  af[0].location = APPLY_RESIST; 
	  af[0].modifier = IMM_ENERGY; 
	  af[0].duration = 5*level; 
	  accum_duration = FALSE; 
	  to_vict = "You feel a little reflective."; 
	  break; 
       case SPELL_PROT_ACID: 
	  af[0].location = APPLY_RESIST; 
	  af[0].modifier = IMM_ACID; 
	  af[0].duration = 5*level; 
	  accum_duration = FALSE; 
	  to_vict = "Your skin takes on a yellow tinge."; 
	  break; 
       case SPELL_PROT_POISON: 
	  af[0].location = APPLY_RESIST; 
	  af[0].modifier = IMM_POISON; 
	  af[0].duration = 5*level; 
	  accum_duration = FALSE; 
	  to_vict = "Your skin takes on a green tinge."; 
	  break; 
       case SPELL_PROT_DRAIN: 
	  af[0].location = APPLY_RESIST; 
	  af[0].modifier = IMM_DRAIN; 
	  af[0].duration = 5*level; 
	  accum_duration = FALSE; 
	  to_vict = "A bright shadow passes before your eyes."; 
	  break; 
       case SPELL_FIRESHIELD: 
	  af[0].duration = 5+(level/2); 
	  af[0].bitvector = AFF2_FIRESHIELD; 
	  af[0].location = APPLY_AFF2;
	  accum_duration = FALSE; 
	  to_vict = "A flaming aura momentarily surrounds you."; 
	  to_room = "$n is surrounded by a flaming aura."; 
	  break; 
       default: 
	  log("SYSERR: unknown spellnum %d passed to mag_affects",
		  spellnum); 
	  return; 
	  break; 
 
      } 
 
  /* 
   * If this is a mob that has this affect set in its mob file, do not 
   * perform the affect.  This prevents people from un-sancting mobs 
   * by sancting them and waiting for it to fade, for example. 
   */ 
   if (IS_NPC(victim) && !affected_by_spell(victim, spellnum)) 
      { 
      for (i=0;i<MAX_SPELL_AFFECTS;i++)
	 if(AFF_FLAGGED(victim,af[i].bitvector))
	    {
            if (spellnum!=SPELL_ENLIVEN)
	       send_to_char(ch, "%s", NOEFFECT); 
	    return; 
	    }
      }

   /* If the victim is already affected by the spell, but at a lower spell
    * level, remove the existing affect in preparation for overwriting it
    * with this new one.
    */
   struct affected_type* aff = get_affected_by_spell(victim, spellnum);
   if (aff != 0x0 && aff->spell_level <= level)
   {
     if( spellnum == SPELL_INSPIRE && aff->spell_level + 2 > level )
     {
       send_to_char(ch,  "%s", NOEFFECT);
       return;
     }

     affect_remove(victim, aff);
   }
   else
   {
     /* 
      * If the victim is already affected by this spell, and the spell does 
      * not have an accumulative effect, then fail the spell. 
      */
     if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect)) 
     { 
       if (spellnum!=SPELL_ENLIVEN)
         send_to_char(ch, "%s", NOEFFECT); 
       return; 
     } 
   }
 
   for (i = 0; i < MAX_SPELL_AFFECTS; i++) 
      if (af[i].bitvector || (af[i].location != APPLY_NONE)) 
	 affect_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE); 
 
   if (to_vict != NULL) 
      act(to_vict, FALSE, victim, 0, ch, TO_CHAR); 
   if (to_room != NULL) 
      act(to_room, TRUE, victim, 0, ch, TO_ROOM); 
} 
 
/* 
 * This function is used to halt the execution of spell affects if there
 * is a condition that hasn't been met (no storm for call lightning
 */
int mag_check(int level, struct char_data *ch, struct char_data *victim,
	      struct obj_data *ovict, int spellnum, int savetype)
{
   char *to_vict = NULL;
   char *to_room = NULL; 
   char *to_char = NULL;
   int pass = TRUE;

   switch (spellnum)
      {
       case SPELL_GOODBERRY:
	  if(!OUTSIDE(ch))
	     {
	     to_char = "You need to be outside to call on your powers!";
	     pass= FALSE;
	     }
	  break;
       case SPELL_CALL_LIGHTNING:
	  if(!OUTSIDE(ch))
	     {
	     to_char = "You need to be outside to call on the elements!";
	     pass= FALSE;
	     } 
	  else if(weather_info.sky<SKY_RAINING)
	     {
	     to_char = "There is no storm to call the lightning from!";
	     pass = FALSE;
	     }
	  break;
       case SPELL_SUNRAY:
	  if(!OUTSIDE(ch))
	     {
	     to_char = "You need to be outside to call on the sun!";
	     pass= FALSE;
	     }
	  else if(weather_info.sunlight==SUN_DARK)
	     {
	     to_char="The sun need to be out for you to call upon its power!";
	     pass= FALSE;
	     }
	  break;    
       case SPELL_ATONEMENT: 
	  if(!AFF_FLAGGED(ch,AFF_GROUP) || !AFF_FLAGGED(victim,AFF_GROUP) || 
	     (ch != victim->master))
	     {
	     to_char = "$N needs to follow and group with you before you can "
		       "help $N atone.";
	     to_vict = "You need to follow and be in $N's group before you "
		       "can atone for your sins."; 
	     pass = FALSE;
	     }
	  break;
       case SPELL_ANIMATE_DEAD:
	  if(ovict==NULL||!IS_CORPSE(ovict)||
	     (strncmp(ovict->name,"pile",4)==0))
	     {
	     to_char=mag_summon_fail_msgs[7];
	     pass=FALSE;
	     }
	  else if(GET_OBJ_VAL(ovict, 6) !=0)
	     {
	     to_char = "The spirit inhabiting that corpse prevents "
		       "reanimation.";
	     pass=FALSE;
	     }
	  else if(GET_LEVEL(ch) < GET_OBJ_VAL(ovict,7))
	     {
	     to_char = "The spirit inhabiting that corpse proves to be "
		       "too strong for your umtamed powers.";
	     pass=FALSE;
	     }
          else if((int)(((float)GET_LEVEL(ch)/2.0)+(((float)GET_LEVEL(ch)/20.0)*(float)level))< 
               GET_OBJ_VAL(ovict,7))
             {
             to_char = "Your spell is too weak to raise this corpse.";
             pass=FALSE;
             }
          break;
       case SPELL_GRANT_PEACE:
	  if(!IS_UNDEAD(victim))
	     {
	     to_char = "$N is not dead yet!  You can only grant peace "
		       "to the undead.";
	     pass= FALSE;
	     }
	  break;
       case SPELL_DRAGON:
	  if(affected_by_spell(victim,SPELL_EAGLE_CLAW))
	     {
	     affect_from_char(victim, SPELL_EAGLE_CLAW); 
	     pass=FALSE;
	     to_char = "The powers of dragon and eagle cancel each other out.";
	     to_vict = "The powers of dragon and eagle cancel each other out.";
	     to_room = "There is a blinding flash as the power of eagle and dragon clash.";
	     }
	  break;
       case SPELL_EAGLE_CLAW:
	  if(affected_by_spell(victim,SPELL_DRAGON))
	     {
	     affect_from_char(victim, SPELL_DRAGON); 
	     pass=FALSE;
	     to_char = "The powers of dragon and eagle cancel each other out.";
	     to_vict = "The powers of dragon and eagle cancel each other out.";
	     to_room = "There is a blinding flash as the power of eagle and dragon clash.";
	     }
	  break;

       case SPELL_INSPIRE:
	  if(AFF_FLAGGED(victim,AFF_RAGE))
	     {
	     pass=FALSE;
	     to_char = "$N is too angry to listen to you heartlifting prose.";
	     to_vict = "$N looks like $E is singing something but you are "
		       "too enraged to care.";
	     to_room = "$N's inspiring prose falls short against $n's rage.";
	     }
	  break;
       case SPELL_STONE_SKIN:
	  if(affected_by_spell(victim,SPELL_BARK_SKIN))
	     {
	     pass=FALSE;
	     to_char = "The flesh you wish to change is but wood!";
	     }
	  break;
       case SPELL_BARK_SKIN:
	  if(affected_by_spell(victim,SPELL_STONE_SKIN))
	     {
	     pass=FALSE;
	     to_char = "The flesh you wish to change is made of stone!!";
	     }
	  break;
       case SPELL_FIRESHIELD:
	  if(affected_by_spell(victim,SPELL_STONE_SKIN) || 
	     affected_by_spell(victim,SPELL_BARK_SKIN) ||
	     affected_by_spell(victim,SPELL_ARMOR) || 
	     affected_by_spell(victim,SPELL_SHIELD))
	     {
	     pass = FALSE;
	     to_char = "$N's magical protection blocks your spell.";
	     }
	  break;
       case SPELL_WEB:
	  if(affected_by_spell(victim,SPELL_FIRESHIELD))
	     {
	     pass = FALSE;
	     to_char = "The flames surrounding $N burns up your webs.";
	     to_vict = "Your fireshield burns $N's webbing.";
	     to_room = "$N's webbing is incinerated by $n's fireshield.";
	     }
	  break;
       default:
	  pass=TRUE;
      }
   if(pass)
      return pass;

   if(to_char!=NULL)
      act(to_char, FALSE, ch, 0, victim, TO_CHAR); 
   if (to_vict != NULL && victim!=ch) 
      act(to_vict, FALSE, victim, 0, ch, TO_CHAR); 
   if (to_room != NULL) 
      act(to_room, TRUE, victim, 0, ch, TO_ROOM); 
   return pass;
   
}
 
/* 
 * This function is used to provide services to mag_groups.  This function 
 * is the one you should change to add new group spells. 
 */ 
 
void perform_mag_groups(int level, struct char_data * ch, 
			struct char_data * tch, int spellnum, int savetype) 
{ 
   level = MAX(MIN(level, 10), 1); 
   switch (spellnum) 
      { 
       case SPELL_GROUP_HEAL: 
	  mag_points(level, ch, tch, SPELL_HEAL, savetype); 
	  break; 
       case SPELL_GROUP_ARMOR: 
	  mag_affects(level, ch, tch, SPELL_ARMOR, savetype); 
	  break; 
       case SPELL_GROUP_RECALL: 
	  spell_recall(level, ch, tch, NULL, NULL,NULL); 
	  break; 
       case SPELL_GROUP_INFRAVISION: 
	  mag_affects(level,ch,tch, SPELL_INFRAVISION, savetype); 
	  break; 
       case SPELL_GROUP_REFRESH: 
	  mag_points(level, ch, tch, SPELL_REFRESH, savetype); 
	  break; 
       case SPELL_GROUP_SANC: 
	  mag_affects(level,ch,tch, SPELL_SANCTUARY, savetype); 
	  break; 
       case SPELL_GROUP_LEVITATE: 
	  mag_affects(level,ch,tch, SPELL_LEVITATE, savetype); 
	  break; 
       case SPELL_GROUP_SUMMON:
          spell_summon(level, ch, tch, NULL, NULL, NULL);
          break;
       default: 
	  log("SYSERR: unknown spellnum %d passed to perform_mag_groups",
		  spellnum); 
	  return; 
	  break; 
 
 
   
      } 
} 
 
 
/* 
 * Every spell that affects the group should run through here 
 * perform_mag_groups contains the switch statement to send us to the right 
 * magic. 
 * 
 * group spells affect everyone grouped with the caster who is in the room, 
 * caster last. 
 * 
 * To add new group spells, you shouldn't have to change anything in 
 * mag_groups -- just add a new case to perform_mag_groups. 
 */ 
 
void mag_groups(int level, struct char_data * ch, int spellnum, int savetype) 
{ 
   struct char_data *tch, *k; 
   struct follow_type *f, *f_next; 
 
   if (ch == NULL) 
      return; 

   level = MAX(MIN(level, 10), 1); 
 
   if (!AFF_FLAGGED(ch, AFF_GROUP)) 
      return; 
   if (ch->master != NULL) 
      k = ch->master; 
   else 
      k = ch; 
   for (f = k->followers; f; f = f_next) 
      { 
      f_next = f->next; 
      tch = f->follower; 
      if ((IN_ROOM(tch) != IN_ROOM(ch)) && (spellnum != SPELL_GROUP_SUMMON)) 
	 continue; 
      if ((IN_ROOM(tch) == IN_ROOM(ch)) && (spellnum == SPELL_GROUP_SUMMON))
         continue;
      if (!AFF_FLAGGED(tch, AFF_GROUP)) 
	 continue; 
      if (ch == tch) 
	 continue; 
      perform_mag_groups(level, ch, tch, spellnum, savetype); 
      } 
 
   if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
      {
      if (((IN_ROOM(ch) != IN_ROOM(k)) && (spellnum == SPELL_GROUP_SUMMON)) ||
          ((IN_ROOM(ch) == IN_ROOM(k)) && (spellnum != SPELL_GROUP_SUMMON)))
         perform_mag_groups(level, ch, k, spellnum, savetype); 
      }
   if (spellnum != SPELL_GROUP_SUMMON)
      perform_mag_groups(level, ch, ch, spellnum, savetype); 
} 
 
 
/* 
 * mass spells affect every creature in the room except the caster. 
 * 
 * No spells of this class currently implemented as of Circle 3.0. 
 */ 
 
void mag_masses(int level, struct char_data * ch, int spellnum, int savetype) 
{ 
   struct char_data *tch, *tch_next; 
 
   level = MAX(MIN(level, 10), 1); 
   for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next) 
      { 
      tch_next = tch->next_in_room; 
      if (tch == ch) 
	 continue; 
 
      switch (spellnum) 
	 { 
	  default: 
	     log("SYSERR: unknown spellnum %d passed to mag_masses",
		 spellnum); 
	     return; 
	     break; 
	 } 
      } 
} 
 
 
/* 
 * Every spell that affects an area (room) runs through here.  These are 
 * generally offensive spells.  This calls mag_damage to do the actual 
 * damage -- all spells listed here must also have a case in mag_damage() 
 * in order for them to work. 
 * 
 *  area spells have limited targets within the room. 
*/ 
 
void mag_areas(int level, struct char_data * ch, int spellnum, int savetype) 
{ 
   struct char_data *tch=0, *next_tch; 
   char *to_char = NULL; 
   char *to_room = NULL; 
   int violent_affects = FALSE;
   int breath_spell = FALSE;
   if (ch == NULL) 
      return; 
   level = MAX(MIN(level, 10), 1); 
 
  /* 
   * to add spells to this fn, just add the message here plus an entry 
   * in mag_damage for the damaging part of the spell. 
   */ 
   switch (spellnum) 
      { 
       case SPELL_EARTHQUAKE: 
	  to_char = "You gesture and the earth begins to shake all around you!"; 
	  to_room ="$n gracefully gestures and the earth begins to shake violently!"; 
	  break; 
       case SPELL_METEOR_STORM: 
	  to_char = "With a scream upward you cause a shower of meteors to strike your opponent!";
	  to_room = "$n screams upward and meteors start to streak towards you!";
	  break; 
       case SPELL_ICE_STORM: 
	  to_char = "With a call to Thor, you bring a mighty hail storm down on your foes' heads!";
	  to_room = "With a call to Thor, $n brings a mighty hail storm down on your head!";
	  break; 
       case SPELL_ACID_BREATH:
	  to_char = "You bellow a cloud of green-yellow gas towards your victims.";
	  to_room = "$n bellows a cloud of green-yellow gas towards you.";
          breath_spell = TRUE;
	  break; 
       case SPELL_GAS_BREATH:
	  to_char ="You bellow a cloud of blood red gas towards your victims.";
	  to_room ="$n bellow a cloud of blood red gas towards you.";
          breath_spell = TRUE;
	  break; 
       case SPELL_FIRE_BREATH:
	  to_char="You bellow a cloud of searing flames towards your victims.";
	  to_room="$n bellow a cloud of searing flames towards you.";
          breath_spell = TRUE;
	  break; 
       case SPELL_FROST_BREATH:
	  to_char="You bellow a cloud of freezing vapor towards your victims.";
	  to_room="$n bellow a cloud of freezing vapor towards you.";
          breath_spell = TRUE;
	  break; 
       case SPELL_ENLIVEN:
          to_char="At your command, vines sprout and entangle the area.";
          to_room="$n commands, and whip-thin vines sprout to entangle the area!";
          violent_affects = TRUE;
          break;
       default: 
	  log("SYSERR: unknown spellnum %d passed to mag_areas",
		  spellnum); 
	  return; 
	  break; 
      } 
 
   if (to_char != NULL) 
      act(to_char, FALSE, ch, 0, 0, TO_CHAR); 
   if (to_room != NULL) 
      act(to_room, FALSE, ch, 0, 0, TO_ROOM); 
   
 
   for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) 
      { 
      next_tch = tch->next_in_room; 
 
     /* 
      * The skips: 1: the caster
      *            2: victims in different rooms 
      *            3: immortals 
      *            4: if no pk on this mud, skips over all players 
      *            5: pets (charmed NPCs)
      *            6: NPC followers of NPCs
      *            7: dragons if type BREATH 
      *            8: component mobs
      * players can only hit players in PKILL rooms
      */ 
 
      if (tch == ch) 
	 continue; 
      if (IN_ROOM(tch) != IN_ROOM(ch))
         continue;
      if (!IS_NPC(tch) && (GET_LEVEL(tch) >= LVL_IMMORT)) 
	 continue; 
      if (!pk_allowed&&!IS_NPC(ch)&&!IS_NPC(tch)&&!ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&& 
              !Z_FLAGGED(IN_ROOM(ch),Z_PKILL))
	 continue; 
      if (!IS_NPC(ch) && !IS_NPC(tch) && !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL) && !Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
         !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(tch, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(tch))<=10))
         continue;
      if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM)) 
	 continue; 
      if (IS_NPC(ch) && IS_NPC(tch) && (tch->master == ch))
         continue; 
      if (IS_NPC(ch) && breath_spell && IS_DRAGON(tch))
         continue;
      if (IS_NPC(ch) && (tch->master == ch) && MOB2_FLAGGED(tch, MOB2_COMPONENT))
         continue; 
      if (!IS_NPC(ch) && (AFF_FLAGGED(tch, AFF_GROUP) && (tch->master==ch)))
         continue;
      if (!IS_NPC(ch) && (AFF_FLAGGED(tch, AFF_GROUP) && (ch->master==tch)))
         continue;
      if (!IS_NPC(ch) && (AFF_FLAGGED(tch, AFF_GROUP) && (tch->master!=NULL) && (tch->master==ch->master)))
         continue;
      if ((violent_affects) && !(AFF_FLAGGED(tch, AFF_GROUP)&&(tch->master==ch)))
         {
         mag_affects(level, ch, tch, spellnum, savetype);
         if (IS_NPC(tch) || ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)||
              Z_FLAGGED(IN_ROOM(ch),Z_PKILL))
            hit(tch, ch, TYPE_UNDEFINED);
         }
      else /* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
         mag_damage(level, ch, tch, spellnum, 1); 
      } 
} 
 
 
/* 
 *  Every spell which summons/gates/conjours a mob comes through here. 
 * 
 *  None of these spells are currently implemented in Circle 3.0; these 
 *  were taken as examples from the JediMUD code.  Summons can be used 
 *  for spells like clone, ariel servant, etc. 
 */ 

 
 
void mag_summons(int level, struct char_data * ch, struct obj_data * obj, 
		 int spellnum, int savetype) 
{ 
   struct char_data *mob = NULL; 
   struct obj_data *tobj, *next_obj; 
   int pfail = 0; 
   int msg=0, fmsg=0; 
   int num = 1; 
   int a, i; 
   mob_vnum mob_num = 0; 
   int handle_corpse = FALSE; 
   int m_level=0;

   if (ch == NULL) 
      return; 
   level = MAX(MIN(level, 10), 1); 
 
   switch (spellnum) 
      { 
       case SPELL_CLONE:
	  msg = 10;
	  fmsg = number(2, 6);       /* Random fail message. */
	  mob_num = MOB_CLONE;
	  pfail = 70-(level*5);
	  break;

       case SPELL_ANIMATE_DEAD: 
	  handle_corpse = TRUE; 
	  msg = 12;
	  fmsg=number(2,6);
	  mob_num = MOB_ZOMBIE; 
 	  a = number(1, 30);
   	  if (a < 3)
 	     mob_num += a;
	  pfail = 15-(level*2); 
	  m_level = GET_OBJ_VAL(obj,7);
	  break; 
       case SPELL_CONJURE_INFANTRY:
	  mob_num = MOB_INFANTRY;
	  handle_corpse=FALSE;
	  msg = 13;
	  fmsg = number(2,4);
	  m_level = (GET_LEVEL(ch)*7)/10;
	  num = 2;
	  pfail = 12 - (level * 2);
	  break;
 
       case SPELL_SUMMON_MOUNT:
	  mob_num = MOB_SUMNMOUNT;
	  handle_corpse = FALSE;
	  msg = 14;
	  fmsg = number(2,4);
	  m_level = level * 8;
	  num = 1;
	  pfail = 12 - (level * 2);
	  break;

       default: 
	  log("SYSERR: unknown spellnum %d passed to mag_summons",
		  spellnum); 
	  return; 
	  break; 
      } 
 
   if (AFF_FLAGGED(ch, AFF_CHARM)) 
      { 
      send_to_char(ch,"You are too giddy to have any followers!\r\n"); 
      return; 
      } 
   if (number(0, 101) < pfail) 
      { 
      send_to_char(ch, "%s", mag_summon_fail_msgs[fmsg]); 
      return; 
      } 
   for (i = 0; i < num; i++) 
      { 
      if(ch==NULL)
	 return;
      if (!(mob = read_mobile(mob_num, VIRTUAL))) 
	 {
	 send_to_char(ch,"You don't quite remember how to make that"
		      " creature.\r\n");
	 mudlogf(BRF,LVL_IMMORT,TRUE,
		 "SYSERR: Spell %s(#%d) had problems creatinge mob vnum %ld."
		 "CREATE THE DAMN MOB WOULD YA!?!?!",
		 spells[spellnum].spell_name,spellnum,mob_num);
	 return;
	 }
      GET_MOB_VAL(mob,0)=GET_ROOM_VNUM(IN_ROOM(ch));
      mob->orig_room=IN_ROOM(ch);
      char_to_room(mob, IN_ROOM(ch)); 
      IS_CARRYING_W(mob) = 0; 
      IS_CARRYING_N(mob) = 0; 
      REMOVE_BIT(MOB_FLAGS(mob),MOB_AGGRESSIVE);
      REMOVE_BIT(MOB_FLAGS(mob),MOB_HUNT_KILLER);
      REMOVE_BIT(MOB_FLAGS(mob),MOB_HUNT_MEMORY);
      
      if(m_level!=0)
	 {
	 GET_LEVEL(mob)=m_level;
	 justify_mob(mob);
	 GET_MAX_HIT(mob)=dice(GET_HIT(mob),GET_MANA(mob))+GET_MOVE(mob);
	 GET_HIT(mob)=GET_MAX_HIT(mob);
	 GET_MANA(mob)=(GET_MAX_MANA(mob)=100);
	 GET_MOVE(mob)=(GET_MAX_MOVE(mob)=150);
	 GET_EXP(mob)/=5;
	 GET_GOLD(mob)=0;
	 }
      else if (spellnum == SPELL_CLONE) 
	 { 
	 char *buf=get_buffer(256);
	 sprintf(buf,"%s %s is standing here.\r\n",GET_NAME(ch),GET_TITLE(ch));
/* 	 if(mob->player.long_descr) */
/* 	    free(mob->player.long_descr); */
	 mob->player.long_descr=str_dup(buf);
	 sprintf(buf,"%s clone",GET_NAME(ch));
/* 	 if(mob->player.name) */
/* 	    free(mob->player.name); */
	 mob->player.name = str_dup(buf);
/* 	 if(mob->player.short_descr) */
/* 	    free(mob->player.short_descr); */
	 mob->player.short_descr = str_dup(GET_NAME(ch));
	 GET_MAX_HIT(mob)=((GET_MAX_HIT(ch)/3)*2*level)/10;
	 GET_HIT(mob)=GET_MAX_HIT(mob);
	 GET_LEVEL(mob)=(GET_LEVEL(ch)*level)/10;
	 GET_RACE(mob)=GET_RACE(ch);
	 GET_CLASS(mob)=GET_CLASS(ch);
	 GET_SEX(mob)=GET_SEX(ch);
	 GET_AC(mob)=(GET_AC(ch)*level)/10;
	 GET_MAX_MOVE(mob)=GET_MAX_MOVE(ch);
	 GET_MOVE(mob)=GET_MOVE(ch);
	 GET_MAX_MANA(mob)=100;/*GET_MAX_MANA(ch);*/
	 GET_MANA(mob)=100;/*GET_MANA(ch);*/
	 GET_HITROLL(mob)=(GET_HITROLL(ch)*level)/10;
	 GET_DAMROLL(mob)=(GET_DAMROLL(ch)/2*level)/10;
	 GET_ALIGNMENT(mob)=GET_ALIGNMENT(ch);
	 release_buffer(buf);
	 } 
      act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM); 
    /*  if(num_charmies(ch) >= (NUM_CHARM_ADJ_CHA(ch, mob) + \
			      (GET_LEVEL(ch)/10))) */
        if(num_charmies(ch) >= MAX_CHARMIES)
	 {
	 act("$N resists your call.", FALSE,ch,0,mob,TO_CHAR);
	 act("$N resists $n and attacks!",FALSE,ch,0,mob,TO_ROOM);
	 set_fighting(mob,ch);
	 load_mtrigger(mob);
	 }
      else if(m_level<=(GET_LEVEL(ch)+level))
	 {
	 SET_BIT(AFF_FLAGS(mob), AFF_CHARM); 
	 load_mtrigger(mob);
	 add_follower(mob, ch); 
	 }
      else if(ch)
	 {
	 act("$N resists your call.", FALSE,ch,0,mob,TO_CHAR);
	 act("$N resists $n and attacks!",FALSE,ch,0,mob,TO_ROOM);
         if(!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
	   set_fighting(mob,ch);
	 load_mtrigger(mob);
	 }
      else
	 load_mtrigger(mob);

      } 
   
   if (handle_corpse) 
      { 
      for (tobj = obj->contains; tobj; tobj = next_obj) 
	 { 
	 next_obj = tobj->next_content; 
	 obj_from_obj(tobj); 
	 obj_to_char(tobj, mob); 
	 } 
      extract_obj(obj); 
      } 
} 
 
 
void mag_points(int level, struct char_data * ch, struct char_data * victim, 
		int spellnum, int savetype) 
{ 
   int hitp = 0; 
   int move = 0; 
   int energy = 0; 
   int old_hit=0;
   if (victim == NULL) 
      return; 
   level = MAX(MIN(level, 10), 1); 
 
   switch (spellnum) 
      { 
       case SPELL_CURE_LIGHT: 
	  hitp = dice(level, 8) + 1;
	  send_to_char(victim, "You feel better.\r\n"); 
	  break; 
       case SPELL_CURE_SERIOUS: 
	  hitp = dice(2*level,8) + 20; 
	  send_to_char(victim, "You feel much better!\r\n"); 
	  break; 
       case SPELL_CURE_CRITIC: 
	  hitp = dice(3*level, 8) + 50;
	  send_to_char(victim, "You feel a lot better!\r\n"); 
	  break; 
       case SPELL_HEAL: 
	  hitp = dice(4*level, 8)+100; 
	  send_to_char(victim, "A warm feeling floods your body.\r\n");
	  break; 
 
       case SPELL_GIVE_LIFE: 
	  hitp = ((int)(GET_MANA(ch) / (11-level))); 
	  send_to_char(victim,"You suddenly feel new life come into you.\r\n");
	  GET_MANA(ch) -= ((int)(GET_MANA(ch) / (11-level))); 
	  send_to_char(ch, "You feel your life force draining away.\r\n"); 
	  break; 
 
       case SPELL_REFRESH: 
	  move = (level * 10); 
	  send_to_char(victim, "You suddenly feel refreshed.\r\n"); 
	  break; 

       case SPELL_ENERGY: 
	  energy = 10*level; 
	  send_to_char(victim, "You suddenly feel more energetic.\r\n"); 
	  break; 
       case SPELL_ATONEMENT:
	  GET_EXP(victim) -= (5000 + (level*500));
	  if(IS_EVIL(ch))
	     {
	     GET_ALIGNMENT(victim) -= level * 50;
	     }
	  else if (IS_GOOD(ch))
	     {
	     GET_ALIGNMENT(victim) += level * 50;
	     }
          else
             {
             if (GET_ALIGNMENT(victim) > 0)
                {
                GET_ALIGNMENT(victim) -= level * 50;
                if (GET_ALIGNMENT(victim) < 0)
                   GET_ALIGNMENT(victim) = 0;
                }
             else if (GET_ALIGNMENT(victim) < 0)
                {
                GET_ALIGNMENT(victim) += level * 50;
                if (GET_ALIGNMENT(victim) > 0)
                   GET_ALIGNMENT(victim) = 0;
                }
             } 
	  GET_ALIGNMENT(victim)=MAX(-1000,MIN(GET_ALIGNMENT(victim),1000));
	  act("You help $N atone for $S sins.",TRUE,ch,0,victim,TO_CHAR);
	  act("$n helps you atone for your sins.",TRUE,ch,0,victim,TO_VICT);
	  act("$n helps $N atone for $S sins.",TRUE,ch,0,victim,TO_NOTVICT);
	  break;
       default: 
	  log("SYSERR: unknown spellnum %d passed to mag_points",
		  spellnum); 
	  return; 
	  break; 
 
      } 
   old_hit=GET_HIT(victim);
   GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + hitp); 
   GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move); 
   GET_MANA(victim) = MIN(GET_MAX_MANA(victim), GET_MANA(victim) + energy); 

   /* What was RESTORED, not what was rolled: old_hit is captured above, so a
    * heal into a full bar reports nothing. */
   if (GET_HIT(victim) != old_hit)
      gmcp_combat_event(ch, victim, "heal", GET_HIT(victim) - old_hit, "spell");

   if(hitp>0)
      if((savetype==SAVING_SPELL)&&FIGHTING(ch))
	 gain_exp(ch,(GET_HIT(victim)-old_hit)*4);
   update_pos(victim);
} 
 
 
void mag_unaffects(int level, struct char_data * ch, struct char_data * victim, 
		   int spellnum, int type) 
{ 
   int spell = 0; 
   char *to_vict = NULL, *to_room = NULL; 
   struct obj_data *obj;
   int i=0;
   int count=0;
   if (victim == NULL) 
      return; 
   level = MAX(MIN(level, 10), 1); 
 
   switch (spellnum) 
      { 
       case SPELL_CURE_BLIND: 
       case SPELL_HEAL: 
	  spell = SPELL_BLINDNESS; 
	  to_vict = "Your vision returns!"; 
	  to_room = "There's a momentary gleam in $n's eyes."; 
	  break; 
       case SPELL_REMOVE_POISON: 
	  spell = SPELL_POISON; 
	  to_vict = "A warm feeling runs through your body!"; 
	  to_room = "$n looks better."; 
	  break; 
 
       case SPELL_PURIFY: 
	  spell = SPELL_POISON; 
	  to_vict = "You feel pure again."; 
	  to_room ="$n is surrounded by a white light and then it fades away.";
	  break; 
 
       case SPELL_REMOVE_CURSE: 
	  spell = SPELL_CURSE; 
	  to_vict = "You don't feel so unlucky."; 
	  count=0;
	  for(i=0;i<NUM_WEARS;i++)
	     {
	     if(((obj=GET_EQ(victim,i))!=NULL)&& CAN_SEE_OBJ(ch,obj))
		{
		if (IS_OBJ_STAT(obj, ITEM_NODROP)) 
		   { 
		   count++;
		   REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP); 
		   if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
                      {
                      struct obj_data *temp_obj;
                      temp_obj = read_object(real_object(GET_OBJ_VNUM(obj)), REAL);
                      if (GET_OBJ_VAL(temp_obj, 2) > 1)
                         GET_OBJ_VAL(obj, 2)++;
                      extract_obj(temp_obj);
                      } 
		   act("$p briefly glows blue.",FALSE,victim,obj,0,TO_CHAR);
		   act("$n's $p briefly glows blue.",FALSE,victim,
		       obj,0,TO_ROOM);
		   if(count>=level)
		      break;
		   } 
		}
	     }
	  for(obj=victim->carrying;obj && (count<level);obj=obj->next_content)
	     {
	     if(CAN_SEE_OBJ(ch,obj)&&IS_OBJ_STAT(obj,ITEM_NODROP))
		{
		count++;
		REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP); 
		if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
                   {
                   struct obj_data *temp_obj;
                   temp_obj = read_object(real_object(GET_OBJ_VNUM(obj)), REAL);
                   if (GET_OBJ_VAL(temp_obj, 2) > 1)
                      GET_OBJ_VAL(obj, 2)++;
		   extract_obj(temp_obj);
                   }
		act("$p briefly glows blue.",FALSE,victim,obj,0,TO_CHAR);
		act("$n's $p briefly glows blue.",FALSE,victim,
		    obj,0,TO_ROOM);
		if(count>=level)
		   break;
		}

	     }

	  break; 
       case SPELL_CURE_PLAGUE:
	  spell = SPELL_PLAGUE;
	  to_vict = "The sores on your body fade away.";
	  to_room = "$n's sores slowly heal.";
	  break;
       default: 
	  log("SYSERR: unknown spellnum %d passed to mag_unaffects",
		  spellnum); 
	  return; 
	  break; 
      } 
 
   if (!affected_by_spell(victim, spell)&&(count<1))
      { 
      if(spellnum!=SPELL_HEAL)
	 send_to_char(ch, "%s", NOEFFECT); 
      return; 
      } 
 
   affect_from_char(victim, spell); 
   if (to_vict != NULL) 
      act(to_vict, FALSE, victim, 0, ch, TO_CHAR); 
   if (to_room != NULL) 
      act(to_room, TRUE, victim, 0, ch, TO_ROOM); 
 
} 
 
 
void mag_alter_objs(int level, struct char_data * ch, struct obj_data * obj, 
		    int spellnum, int savetype) 
{ 
   char *to_char = NULL; 
   char *to_room = NULL; 
 
   if (obj == NULL) 
      return; 
   level = MAX(MIN(level, 10), 1); 
 
   switch (spellnum) 
      { 
       case SPELL_BLESS: 
	  if (!IS_OBJ_STAT(obj, ITEM_BLESS) && 
	      (GET_OBJ_WEIGHT(obj) <= (5 * level))) 
	     { 
	     SET_BIT(GET_OBJ_EXTRA(obj), ITEM_BLESS); 
	     to_char = "$p glows briefly."; 
	     } 
	  break; 
       case SPELL_CURSE: 
	  if (!IS_OBJ_STAT(obj, ITEM_NODROP)) 
	     { 
	     SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP); 
	     if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) 
		{
		GET_OBJ_VAL(obj, 2)-=1+level/2; 
		GET_OBJ_VAL(obj,2)=MAX(1,GET_OBJ_VAL(obj,2));
		}
	     to_char = "$p briefly glows red."; 
	     } 
	  break; 
       case SPELL_INVISIBLE: 
	  if (!IS_OBJ_STAT(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) 
	     { 
	     SET_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE); 
	     to_char = "$p vanishes."; 
	     } 
	  break; 
       case SPELL_POISON: 
	  if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) || 
	       (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) || 
	       (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3)) 
	     { 
	     GET_OBJ_VAL(obj, 3) = 1; 
	     to_char = "$p steams briefly."; 
	     } 
	  break; 
       case SPELL_REMOVE_CURSE: 
	  if (IS_OBJ_STAT(obj, ITEM_NODROP)) 
	     { 
	     REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP); 
	     if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
                {
                struct obj_data *temp_obj;
                temp_obj = read_object(real_object(GET_OBJ_VNUM(obj)), REAL);
                if (GET_OBJ_VAL(temp_obj, 2) > 1)
                   GET_OBJ_VAL(obj, 2)++;
                extract_obj(temp_obj);
                } 
	     to_char = "$p briefly glows blue."; 
	     } 
	  break; 
       case SPELL_REMOVE_POISON: 
	  if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) || 
	       (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) || 
	       (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3)) 
	     { 
	     GET_OBJ_VAL(obj, 3) = 0; 
	     to_char = "$p steams briefly."; 
	     } 
	  break; 
       default: 
	  log("SYSERR: unknown spellnum %d passed to mag_alter_objects",
		  spellnum); 
	  return; 
	  break; 
      } 
 
   if (to_char == NULL) 
      send_to_char(ch, "%s", NOEFFECT); 
   else 
      act(to_char, TRUE, ch, obj, 0, TO_CHAR); 
 
   if (to_room != NULL) 
      act(to_room, TRUE, ch, obj, 0, TO_ROOM); 
   else if (to_char != NULL) 
      act(to_char, TRUE, ch, obj, 0, TO_ROOM); 
 
} 
 
 
 
void mag_creations(int level, struct char_data * ch, int spellnum) 
{ 
   struct obj_data *tobj; 
   obj_vnum z;
   int i; 
   int loop;

   if (ch == NULL) 
      return; 
   level = MAX(MIN(level, 10), 1); 

   switch (spellnum) 
      { 
       case SPELL_CREATE_FOOD: 
	  z = VNUM_CREATE_FOOD; 
	  loop=level;
	  break;  
       case SPELL_CREATE_WATER: 
	  z = VNUM_CREATE_WATER; 
	  loop=level;
	  break; 
       case SPELL_CREATE_LIGHT: 
	  z = VNUM_CREATE_LIGHT; 
	  loop=1;
	  break; 
       case SPELL_CONTINUAL_LIGHT: 
	  z = VNUM_CONT_LIGHT; 
	  loop=1;
	  break; 
       case SPELL_GOODBERRY:
	  z = VNUM_GOODBERRY;
	  loop = level;
	  break;
       default: 
	  log("SYSERR: unknown spellnum %d passed to creations",
		  spellnum); 
	  return; 
	  break; 
      } 
 
   for(i=0;i<loop;i++)
      {
      if (!(tobj = read_object(z, VIRTUAL))) 
	 { 
	 send_to_char(ch, "I seem to have goofed.\r\n"); 
	 log("SYSERR: spell_creations, spell %d, obj %ld: obj not found", 
	     spellnum, z); 
	 return; 
	 } 
      obj_to_char(tobj, ch); 
      act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM); 
      act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR); 
      load_otrigger(tobj);

      switch(spellnum)
	 {
	  case SPELL_CREATE_LIGHT:
	     GET_OBJ_VAL(tobj,2)=level*4;
	     break;
	  case SPELL_CONTINUAL_LIGHT:
	     GET_OBJ_VAL(tobj,2)=level*12;
	     break;
	  case SPELL_GOODBERRY:
	     tobj->affected[0].location = APPLY_EAT_SPELL;
	     tobj->affected[0].modifier = SPELL_CURE_LIGHT;
	     break;
	 }
      }   
} 
 


void mag_forceful(int level, struct char_data * ch, struct char_data * victim,
                 int spellnum, int savetype)
{
   char *force_cmd = NULL;
   char *to_room = NULL, *to_vict = NULL;
   level = MAX(MIN(level, 10), 1);

   /* these messages might not work for all situations, adjust if necessary - nomi */
   if ((GET_LEVEL(victim) >= LVL_IMMORT) && (GET_LEVEL(ch) < GET_LEVEL(victim)))
      {
      send_to_char(ch, "Your attempt at being forceful with %s backfires, leaving "
                   "you whimpering like a small child.\r\n", GET_NAME(victim));
      GET_HIT(ch) = GET_HIT(ch)/2;
      return;
      }

   if (!FIGHTING(victim))
      {
      send_to_char(ch, "This only works if the target is in battle.\r\n");
      return;
      }

   /* if both PCs, and it's a PKill room, bail */
   if (!IS_NPC(victim) && !IS_NPC(ch) && !(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL) || Z_FLAGGED(IN_ROOM(ch),Z_PKILL)) &&
      !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(victim, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(victim))<=10))
      {
      send_to_char(ch, "You can't cast that on a player outside a pk room!!\r\n");
      return;
      }

   if (IS_NPC(victim) && MOB2_FLAGGED(victim, MOB2_COMPONENT))
      {
      send_to_char(ch, "You try to force %s but fail!\r\n", GET_NAME(victim));
      return;
      }

   if ((GET_LEVEL(ch)+level) < GET_LEVEL(victim))
      {
      send_to_char(ch, "%s has no fear of your petty intimidation.\r\n", 
                   GET_NAME(victim));
      return;
      }

   switch (spellnum)
      {
      case SPELL_FEAR:
         if (mag_savingthrow(victim, savetype,level,10))
            {
            to_vict = "$N attempts to scare you away, but fails miserably.";
            to_room = "$n laughs at $N's feeble attempt to scare $m away.";
            }
         else
            {
            force_cmd = "flee";
            to_vict = "$N puts some righteous fear into you and your "
                      "legs move before you have a chance to think!";
            to_room = "$n is stricken with a look of terror and attempts to flee!";
            }
         break;
      default:
         log("SYSERR: Someone screwed up, this ain't a spell!!! (mag_forceful() "
             "spell:%d person:%s)", spellnum, GET_NAME(ch));
      }


   if (to_vict != NULL)  
      act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
   if (to_room != NULL)
      act(to_room, TRUE, victim, 0, ch, TO_ROOM);

   /* now the fun part: FORCE!!! */
   if (force_cmd != NULL)
      command_interpreter(victim, force_cmd);
   return;
}
