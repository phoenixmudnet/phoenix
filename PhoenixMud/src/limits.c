/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD * 
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      * 
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
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"
#include "queue.h"
#include "dg_scripts.h"

extern struct char_data *character_list;
extern struct obj_data *object_list;
extern const float class_exp_multipliers[];
extern const float race_exp_multipliers[];
extern const int exp_table[LVL_IMPL + 1];
extern struct mob_defaults mob_def_stats[];
extern struct room_data *world;
extern int max_exp_gain;
extern int max_exp_loss;
extern int min_kills;
extern int mob_exp_divisor;
extern int free_rent;
extern int use_autowiz;
extern int min_wizlist_lev;
extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
extern struct zone_data *zone_table;              /*. db.c .*/
extern int top_of_zone_table;                     /*. db.c .*/

/* External Functions */
void save_corpses(void);
void update_char_objects(struct char_data * ch); /* handler.c */
void extract_obj(struct obj_data * obj); /* handler.c */
pid_t getpid(void);
void Crash_rentsave(struct char_data *ch, int cost);
void Crash_count_items(struct obj_data * obj, long *nitems);
void Crash_extract_norents_from_equipped(struct char_data * ch);
void Crash_extract_norents(struct obj_data * obj);



/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int how_old, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
   {

   if (how_old < 15)
      return (p0);  /* < 15   */
   else if (how_old <= 29)
      return (int)(p1 + (((how_old - 15)*(p2 - p1)) / 15)); /* 15..29 */
   else if (how_old <= 44)
      return (int)(p2 + (((how_old - 30) * (p3 - p2)) / 15)); /* 30..44 */
   else if (how_old <= 59)
      return (int)(p3 + (((how_old - 45) * (p4 - p3)) / 15)); /* 45..59 */
   else if (how_old <= 79)
      return (int)(p4 + (((how_old - 60) * (p5 - p4)) / 20)); /* 60..79 */
   else
      return (p6);  /* >= 80 */
   return (p0);
   }


/*
* The hit_limit, mana_limit, and move_limit functions are gone.  They 
* added an unnecessary level of complexity to the internal structure, 
* weren't particularly useful, and led to some annoying bugs.  From the 
* players' point of view, the only difference the removal of these 
* functions will make is that a character's age will now only affect 
* the HMV gain per tick, and _not_ the HMV maximums. 
*/

/*
* manapoint gain pr. game hour 
*/
int mana_gain(struct char_data * ch)
   {
   int gain;

   if(GET_MANA(ch)==GET_MAX_MANA(ch))
      return 0;

   if(AFF_FLAGGED(ch,AFF_PLAGUE))
      {
      if(GET_POS(ch)==POS_SLEEPING)
         return 3;
      else
         return 1;
      }

   if (IS_NPC(ch))
      {
      gain = graf(25, 4, 8, 12, 16, 12, 10, 8);
      /* Neat and fast */
      }
   else
      {
      gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 12, 12);
      }

   /* stat calculations */
   gain+=(point_gain[stat_index(GET_WIS(ch))].manap/2);
   gain+=(point_gain[stat_index(GET_INT(ch))].manap/2);

   if (AFF_FLAGGED(ch, AFF_POISON))
      return gain/2;

   /* Skill/Spell calculations */

   /* Furniture calculations */
   if (FURNITURE(ch))
      gain += GET_OBJ_VAL(FURNITURE(ch), 2);

   /* Position calculations    */
   switch (GET_POS(ch))
      {
   case POS_SLEEPING:
      gain *= 2;
      break;
   case POS_CHANT:
   case POS_MEDITATE:
      gain += 30;
      break;
   case POS_RESTING:
      gain += (gain/2); /* Divide by 2 */
      break;
   case POS_SITTING:
      gain += (gain/4); /* Divide by 4 */
      break;
      }

   /* Class calculations */
   if (IS_MAGIC_USER(ch) || IS_CLERIC(ch)||IS_DEVA(ch)||IS_NECROMANCER(ch)||IS_DRUID(ch))
      gain *= 2;

   /* Condition calculations */
   if(!IS_NPC(ch)&& ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0)))
      gain /= 4;


   if (IS_SET(ROOM_FLAGS(IN_ROOM(ch)), ROOM_REGEN))
      gain += gain * 2;

   if (gain < 1)
      gain = 0;

   return (gain);
   }


/*
* Hitpoint gain pr. game hour 
*/
int hit_gain(struct char_data * ch)
   {
   int gain;

   if(GET_HIT(ch)==GET_MAX_HIT(ch))
      return 0;

   if (IS_NPC(ch))
      {
      gain = graf(25, 8, 12, 20, 32, 16, 10, 4);
      /* Neat and fast */
      }
   else
      {
      gain = graf(age(ch)->year, 8, 12, 20, 32, 16, 16, 16);
      }

   /* Stat calculations */
   gain+=point_gain[stat_index(GET_CON(ch))].hitp;

   if (AFF_FLAGGED(ch, AFF_POISON))
      return gain/2;

   /* Skill/Spell calculations */

   /* Furniture calculations */
   if (FURNITURE(ch))
      gain += GET_OBJ_VAL(FURNITURE(ch), 2);

   /* Position calculations    */

   switch (GET_POS(ch))
      {
   case POS_BANDAGE:
      gain+=(gain/2)+30;
      break;
   case POS_SLEEPING:
      gain += (gain/2); /* Divide by 2 */
      break;
   case POS_MEDITATE:
   case POS_CHANT:
   case POS_RESTING:
      gain += (gain/4); /* Divide by 4 */
      break;
   case POS_SITTING:
      gain += (gain/8); /* Divide by 8 */
      break;
      }

   /* Class/Level calculations */
   if (IS_MAGIC_USER(ch) || IS_CLERIC(ch)||IS_DEVA(ch)||IS_NECROMANCER(ch)||IS_DRUID(ch))
      gain /= 2;


   /* Condition calculations */
   if (!IS_NPC(ch)&&((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0)))
      gain /= 4;


   if(AFF_FLAGGED(ch,AFF_PLAGUE))
      {
      if(IS_NPC(ch)&&GET_POS(ch)==POS_SLEEPING)
         gain = 21;
      else if(GET_POS(ch)==POS_SLEEPING)
         gain = 3;
      else
         gain=0;
      }
   else if (IS_SET(ROOM_FLAGS(IN_ROOM(ch)), ROOM_REGEN))
      gain += gain * 2;

   return (gain);
   }



/*
* move gain pr. game hour 
*/
int move_gain(struct char_data * ch)
   {
   int gain;

   if(GET_MOVE(ch)==GET_MAX_MOVE(ch))
      return 0;

   if (IS_NPC(ch))
      {
      gain = graf(25, 16, 20, 24, 20, 16, 12, 10);
      /* Neat and fast */
      }
   else
      {
      gain = graf(age(ch)->year, 16, 20, 24, 20, 16, 16, 16);
      }

   /* Stat calculations */
   gain+=point_gain[stat_index(GET_DEX(ch))].mvp;
   if (AFF_FLAGGED(ch, AFF_POISON))
      return gain/2;
   /* Class/Level calculations */

   /* Skill/Spell calculations */

   /* Furniture calculations */
   if (FURNITURE(ch))
      gain += GET_OBJ_VAL(FURNITURE(ch), 2);

   /* Position calculations    */
   switch (GET_POS(ch))
      {
   case POS_SLEEPING:
      gain += (gain/2); /* Divide by 2 */
      break;
   case POS_BANDAGE:
   case POS_MEDITATE:
   case POS_CHANT:
   case POS_RESTING:
      gain += (gain/4); /* Divide by 4 */
      break;
   case POS_SITTING:
      gain += (gain/8); /* Divide by 8 */
      break;
      }
   if (!IS_NPC(ch)&&((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0)))
      gain /= 4;

   if (IS_SET(ROOM_FLAGS(IN_ROOM(ch)), ROOM_REGEN))
      gain += gain * 2;

   if(AFF_FLAGGED(ch,AFF_PLAGUE))
      {
      if(GET_POS(ch)==POS_SLEEPING)
         gain=6;
      else
         gain=1;
      }

   return (gain);
   }



void set_title(struct char_data * ch, char *title)
   {
   if (title == NULL)
      {
      CREATE(title, char, strlen("has yet to use the title command "));
      strcpy(title, "has yet to use the title command");
      }

   if (strlen(title) > MAX_TITLE_LENGTH)
      title[MAX_TITLE_LENGTH] = '\0';

   if (GET_TITLE(ch) != NULL)
      free(GET_TITLE(ch));

   GET_TITLE(ch) = str_dup(title);
   }

extern void update_wizlist(void);

void check_autowiz(struct char_data * ch)
   {
   if (use_autowiz && GET_LEVEL(ch) >= LVL_IMMORT)
      {
      char *buf=get_buffer(100);
      /*sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev,
              WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());
	      mudlog("Initiating autowiz.", CMP, LVL_IMMORT, FALSE);*/
      update_wizlist();
      system(buf);
      release_buffer(buf);
      }
   }



void gain_exp(struct char_data * ch, long gain)
   {
   float multiplier;
   long max_gain;
   int tmp_exp;

   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)
           /*||Z_FLAGGED(IN_ROOM(ch),Z_PKILL)*/)
      return;
   if (IS_NPC(ch))
      {
      GET_EXP(ch)+=(gain/5);
      }
   else if((GET_LEVEL(ch) < LVL_IMMORT) && (GET_LEVEL(ch) > 0)&&(gain>0))
      {
      multiplier = (float)((float)class_exp_multipliers[(int)GET_CLASS(ch)]
                           *(float)race_exp_multipliers[(int)GET_RACE(ch)]);

      if( REMORT_LEVEL( ch ) == TRIPLE_REMORT )
      {
        multiplier *= 4;
      }

      /*
       * ZEAL (4.2): the activity-reward potion. Multiplicative with the
       * class/race multipliers rather than added after, so it is worth the
       * same PROPORTION to every class -- a flat addition would be worth
       * least to exactly the classes that already gain slowest.
       *
       * Inside the (level < LVL_IMMORT && gain > 0) arm on purpose: it can
       * never soften a loss, and staff do not earn progression.
       */
      if (affected_by_spell(ch, SPELL_ZEAL))
      {
        multiplier *= ZEAL_EXP_BONUS;
      }

      /*
       * A scheduled experience window (events.c). Percent, so 200 is double
       * and a live 100 is a visible no-op. Multiplicative with class and race
       * for the reason zeal is: a flat addition is worth least to the classes
       * that already gain slowest.
       */
      {
        int evpct = event_value(EVENT_XP);
        if (evpct > 0)
          multiplier *= (float) evpct / 100.0f;
      }

      if (gain > 0)
         {
         if(GET_LEVEL(ch)<16)
            max_gain=(int)((float)exp_table[15]
                           *(float)multiplier)
                     /min_kills;
         else
            max_gain=((int)((float)exp_table[GET_LEVEL(ch)]
                            *(float)multiplier)
                      /min_kills);

         gain = MIN(max_gain, gain);
         /*
         if((gain+GET_EXP(ch))>(((int)((float)exp_table[(int)GET_LEVEL(ch)]
                                       * (float)multiplier))+
                                ((int)((float)exp_table[(int)GET_LEVEL(ch)+1]
                                * (float)multiplier))))*/
         tmp_exp = GET_EXP_FOR_LEVEL(GET_RACE(ch),GET_CLASS(ch),GET_LEVEL(ch),REMORT_LEVEL(ch))
           + GET_EXP_FOR_LEVEL(GET_RACE(ch),GET_CLASS(ch),1+GET_LEVEL(ch),REMORT_LEVEL(ch));
         if ((gain+GET_EXP(ch)) > tmp_exp)
            {
            if(GET_LEVEL(ch)<100)
               {
               send_to_char(ch,"You must visit your guild before you can gain more experience.\r\n");
               }
            /*GET_EXP(ch)=((int)((float)exp_table[(int)GET_LEVEL(ch)]
                               * (float)multiplier))+
                        ((int)((float)exp_table[GET_LEVEL(ch)+1]
                        * (float)multiplier))//-5; */
            GET_EXP(ch) = tmp_exp;
            }
         else
            {
            if(AFF_FLAGGED(ch,AFF_CHARM))
               gain/=5;
            GET_EXP(ch) += gain;
            }
         }
      }
   else if ((gain < 0) &&!IS_NPC(ch))
      {
      gain = MAX(-500000, gain);  /* Never loose more than 1/2 mil */
      GET_EXP(ch) += gain;
      if (GET_EXP(ch) < -1000000)
         GET_EXP(ch) = -1000000;
      }
   }


void gain_exp_regardless(struct char_data * ch, long gain, bool show)
   {
   long tmp_exp;
   if (!IS_NPC(ch))
      {
      tmp_exp = GET_EXP_FOR_LEVEL(GET_RACE(ch), GET_CLASS(ch),     GET_LEVEL(ch), REMORT_LEVEL(ch))
              + GET_EXP_FOR_LEVEL(GET_RACE(ch), GET_CLASS(ch), 1 + GET_LEVEL(ch), REMORT_LEVEL(ch));
      if ((gain + GET_EXP(ch)) > tmp_exp)
         {
         if(GET_LEVEL(ch) < 100)
            send_to_char(ch, "You must visit your guild before you can gain more experience.\r\n");
         GET_EXP(ch) = tmp_exp;
         }
	  else
         GET_EXP(ch) += gain;
		  
      if (GET_EXP(ch) < -1000000)
         GET_EXP(ch) = -1000000;
      }
   }


void gain_condition(struct char_data * ch, int condition, int value)
   {
   bool intoxicated;

   if (IS_NPC(ch)||GET_COND(ch, condition) == -1) /* No change */
      return;

   intoxicated = (GET_COND(ch, DRUNK) > 0);

   GET_COND(ch, condition) += value;

   GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
   GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

   if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
      return;

   switch (condition)
      {
   case FULL:
      send_to_char(ch,"You are hungry.\r\n");
      return;
   case THIRST:
      send_to_char(ch,"You are thirsty.\r\n");
      return;
   case DRUNK:
      if (intoxicated)
         send_to_char(ch,"You are now sober.\r\n");
      return;
   default:
      break;
      }

   }

void check_idling(struct char_data *ch)
   {
   if (ch->char_specials.timer > idle_void)
      {
      /* Moved immortal check here, allows imms to be seen as AFK - Nomi 10/25/25*/
      if (GET_LEVEL(ch) >= idle_max_level)
         {
         if (!IS_SET(PRF2_FLAGS(ch), PRF2_AFK))
            send_to_char(ch,"You have been idle, and are now AFK.\r\n");
         SET_BIT(PRF2_FLAGS(ch), PRF2_AFK);
         }
      else if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE)
         {
         GET_WAS_IN(ch) = IN_ROOM(ch);
         if (FIGHTING(ch))
            {
            stop_fighting(FIGHTING(ch));
            stop_fighting(ch);
            }
         act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
         send_to_char(ch,"You have been idle, and are pulled into a void.\r\n");
         save_char(ch, GET_WAS_IN(ch));
         Crash_crashsave(ch);
         affect_from_char(ch,SKILL_RAGE);
         SET_BIT(PRF2_FLAGS(ch), PRF2_AFK);
         char_from_room(ch);
         char_to_room(ch, 1);
         }
      else if (ch->char_specials.timer > idle_rent_time)
         {
         char *buf=get_buffer(128);
         if (IN_ROOM(ch) != NOWHERE)
            char_from_room(ch);
         char_to_room(ch, GET_WAS_IN(ch));
         sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
         mudlog(buf, CMP, LVL_DGOD, TRUE);
         
         Crash_extract_norents_from_equipped(ch);
         Crash_extract_norents(ch->carrying);

         if (ch->desc)
            {
            STATE(ch->desc)=CON_DISCONNECT;
            ch->desc->character=NULL;
            ch->desc = NULL;
            }
         if (free_rent)
            Crash_rentsave(ch, 0);
         else
            Crash_idlesave(ch);
         release_buffer(buf);
         extract_char(ch);
         }
      }
   }


/* Update PCs, NPCs, and objects */
void point_update(void)
   {
   struct char_data *i, *next_char;
   struct obj_data *j, *next_thing, *jj, *next_thing2;

   /* characters */
   for (i = character_list; i; i = next_char)
      {
      next_char = i->next;

      gain_condition(i, FULL, -1);
      gain_condition(i, DRUNK, -1);
      gain_condition(i, THIRST, -1);

      if (!IS_NPC(i))
         {
         update_char_objects(i);
         ++i->char_specials.timer;
         check_idling(i);
         GET_LEARN_TIC(i)=0;

         }
      }

   /* objects */
   for (j = object_list; j; j = next_thing)
      {
      next_thing = j->next; /* Next in object list */

      if (IS_OBJ_STAT(j, ITEM_NORENT)) {
	if (GET_OBJ_TIMER(j) < 0) {
	  GET_OBJ_TIMER(j) = 1152; /* 24 hours. */
	}
      }

      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0) {
	GET_OBJ_TIMER(j)--;
      }

      if (GET_OBJ_DGTIMER(j) > 0)
         GET_OBJ_DGTIMER(j)--;

      if (!GET_OBJ_DGTIMER(j))
         {
         GET_OBJ_DGTIMER(j)=-1;
         timer_otrigger(j);
         }

      if (GET_OBJ_DGTIMER(j) > 0)
         continue;


      if (!GET_OBJ_TIMER(j))
         {
         long nitems=0;
         if (j->carried_by) {
            act("$p decays in your hands.", FALSE, j->carried_by, j,0,TO_CHAR);

            if (IS_CORPSE(j) && GET_OBJ_VAL(j,6)) {
                Crash_count_items(j->contains,&nitems);
                mudlogf(CMP,LVL_DETY,TRUE,"CORPSE: %s's corpse has decayed at %ld while being held by %s.(%ld items)",
                  get_name_by_id(GET_OBJ_VAL(j,6)),GET_ROOM_VNUM(IN_ROOM(j->carried_by)),GET_NAME(j->carried_by),nitems);
               }
            }
         else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people))
            {
            act("$p decays into the ground.",
                TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
            act("$p decays into the ground.",
                TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);

            if (IS_CORPSE(j) && GET_OBJ_VAL(j,6)) {
                Crash_count_items(j->contains,&nitems);
                mudlogf(CMP,LVL_DETY,TRUE,"CORPSE: %s's corpse has decayed at %ld.(%ld items)",    
                  get_name_by_id(GET_OBJ_VAL(j,6)),GET_ROOM_VNUM(IN_ROOM(j)),nitems);
               }
            }
         for (jj = j->contains; jj; jj = next_thing2)
            {
            next_thing2 = jj->next_content; /* Next in inventory */
            obj_from_obj(jj);

            if (j->in_obj)
               obj_to_obj(jj, j->in_obj);
            else if (j->carried_by)
               obj_to_room(jj, IN_ROOM(j->carried_by));
            else if (IN_ROOM(j) != NOWHERE)
               obj_to_room(jj, IN_ROOM(j));
            else {
               mudlogf(BRF,LVL_IMMORT,FALSE,"core_dump being called because %s is NOWHERE",
                       GET_OBJ_NAME(j));
               core_dump();
               extract_obj(jj); /* nowhere to place it; free it instead of leaking */
                 }
            }
         extract_obj(j);
         }
      }
   save_corpses();
   }

void MEV_update(void)
   {
   struct char_data *i, *next_char, *tch;
   int time_to_tic;
   int chance;
   struct affected_type af;
   int dam =0;

   for (i = character_list; i; i = next_char)
      {
      next_char = i->next;
      if(OUTSIDE(i) && (GET_LEVEL(i)<LVL_IMMORT) &&
              !ROOM_FLAGGED(IN_ROOM(i), ROOM_DARK) &&
              (IS_D_ELF(i)))
         {
         if((weather_info.sunlight==SUN_SET) ||
                 (weather_info.sunlight == SUN_RISE) ||
                 (weather_info.sunlight == SUN_LIGHT))
            {
            dam=GET_LEVEL(i)/10;
            dam *= 5;
            if(dam<2)
               dam=2;
            if(damage(i,i,dam,SPELL_SUNBURN,IMM_FIRE)==-1) /* they died */
               continue;
            }
         }
      if(!IS_NPC(i) && (SECT(IN_ROOM(i))==SECT_UNDERWATER) && ROOM_FLAGGED(IN_ROOM(i),ROOM_NO_WATERBREATHE))
         {
         dam = GET_MAX_HIT(i)*5;
         dam /=100;
         if(damage(i, i, MAX(1,dam), SPELL_DROWN,IMM_DRAIN)==-1)
            continue; /* whoops, they died */
         }
      if (AFF_FLAGGED(i,AFF_PLAGUE)&&IS_SET(ROOM_FLAGS(IN_ROOM(i)), ROOM_REGEN))
         {
         affect_from_char(i, SPELL_PLAGUE); 
         send_to_char(i,"The sores on your body fade away.\r\n"); 
         act("$n's sores slowly heal.", TRUE, i, 0, i, TO_ROOM); 
         }


      if (GET_POS(i) >= POS_STUNNED)
         {
         GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
         GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
         GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));

         if (AFF_FLAGGED(i, AFF_POISON))
            {
            dam = GET_MAX_HIT(i) *3;
            dam /=100;

            if(damage(i, i, MAX(1,dam), SPELL_POISON,IMM_POISON)==-1)
               continue; /* whoops, they died */
            }
         if(AFF_FLAGGED(i,AFF_PLAGUE))
            {
            if(damage(i,i,7,SPELL_PLAGUE,IMM_POISON)==-1)
               continue; /* whoops, they died */
            }

         if (GET_POS(i) <= POS_STUNNED)
            update_pos(i);
         }
      else if (GET_POS(i) == POS_INCAP&&!FIGHTING(i))
         {
         if(damage(i, i, 1, TYPE_SUFFERING,0)==-1)
            continue;
         }
      else if ((GET_POS(i) == POS_MORTALLYW) &&!FIGHTING(i))
         {
         if(damage(i, i, 2, TYPE_SUFFERING,0)==-1)
            continue;
         }
      if (AFF_FLAGGED(i, AFF_PLAGUE)&&
              !ROOM_FLAGGED(IN_ROOM(i),ROOM_PEACEFUL) &&
              !ROOM_FLAGGED(IN_ROOM(i),ROOM_REGEN))
         {
         for (tch = world[IN_ROOM(i)].people;tch;tch = tch->next_in_room)
            {
            if(!(IS_NPC(tch)&&ROOM_FLAGGED(IN_ROOM(tch),ROOM_NOMAGIC)))
               if (!AFF_FLAGGED(tch, AFF_PLAGUE)&&(GET_LEVEL(tch)>15)&&
                       (IS_NPC(tch)||!PRF_FLAGGED(tch, PRF_NOHASSLE))&&
                       (GET_RACE(tch)!=MRACE_STATUE))
                  {
                  chance = 0;
                  if(IS_NPC(tch))
                     chance = number(0, 7);
                  else
                     chance = number(0, 11);
                  if (chance > 2)
                     {
                     af.type = SPELL_PLAGUE;
                     af.location =  APPLY_NONE;
                     af.modifier = 0;
                     af.duration = 60;
                     af.bitvector = AFF_PLAGUE;
                     affect_join(tch, &af, FALSE, FALSE, FALSE, FALSE);
                     send_to_char(tch,"You have suddenly contracted the "
                                  "plague!!\r\n");
                     act("$n turns green as huge sores break out.",TRUE,tch,
                         0,tch,TO_ROOM);
                     }
                  }
            }
         }
      }
   time_to_tic=number(50,101);
   add_function_to_queue(time_to_tic,NULL,0,0,MEV_update);
   /* mudlogf(CMP,LVL_IMMORT,FALSE,"Time to next tic: %d",time_to_tic); */
   }



#define GET_NDD(mob) ((mob)->mob_specials.damnodice)
#define GET_SDD(mob) ((mob)->mob_specials.damsizedice)

void justify_mob(struct char_data *mob)
   {
   int mlevel;

   /*
   * Put a flag check in here so that, if we don't want to change
   * this mob's stats, we won't
   */
   SET_BIT(IMMUNE(mob),trait_info[GET_RACE(mob)].immune);
   SET_BIT(RESIST(mob),trait_info[GET_RACE(mob)].resist);
   SET_BIT(SUCCEPT(mob),trait_info[GET_RACE(mob)].susceptible);
   mlevel = GET_LEVEL(mob);

   GET_HIT(mob) = mob_def_stats[mlevel].num_hit;
   GET_MANA(mob)=mob_def_stats[mlevel].sze_hit;
   GET_MOVE(mob)=mob_def_stats[mlevel].bon_hit;

   GET_HITROLL(mob)=mob_def_stats[mlevel].to_hit;
   GET_DAMROLL(mob)=mob_def_stats[mlevel].to_dam;

   GET_AC(mob)=mob_def_stats[mlevel].ac;

   GET_NDD(mob)=mob_def_stats[mlevel].num_dam;
   GET_SDD(mob)=mob_def_stats[mlevel].sze_dam;

   GET_GOLD(mob)=mob_def_stats[mlevel].gold;

   GET_EXP(mob) = (exp_table[mlevel]*3)/mob_exp_divisor;
   }






