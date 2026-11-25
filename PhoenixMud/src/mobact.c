/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD * 
*  Usage: Functions for generating intelligent (?) behavior in mobiles    * 
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "queue.h"
#include "screen.h"
#include "spells.h"
#include "constants.h"

/* external structs */
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct room_data *world;
extern int no_specials;
extern int pulse;
extern int cmd_growl;
extern struct zone_data *zone_table;

int find_first_step(room_rnum src, room_rnum target,long iFlag);
void hunt_victim(struct char_data * ch);
void mprog_random_trigger(struct char_data * mob);
void mprog_wordlist_check(char *arg, struct char_data * mob,
                          struct char_data * actor, struct obj_data * obj,
                          void *vo, int type);
int mob_go_path(struct char_data *ch, room_vnum dest);
int is_empty(int zone_nr);
int has_object(struct char_data *ch, obj_rnum obj_real_num);
void death_cry(struct char_data *ch);
int handleGetOutOfDeathFree(struct char_data*);

ACMD(do_action);
ACMD(do_gen_comm);
ACMD(do_get);
ACMD(do_hide);
ACMD(do_look);
ACMD(do_move);
ACMD(do_recall);
ACMD(do_say);
ACMD(do_sneak);

SPECIAL(citizen);
SPECIAL(cityguard);
SPECIAL(moods);
SPECIAL(shop_keeper);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

void mob_citizen_help(struct char_data *ch)
   {
   char *buf=get_buffer(256);
   struct char_data *tch, *tch_next;

   if(trait_info[GET_RACE(ch)].can_talk==0)
      act("$n squeals out in alarm!",TRUE,ch,0,0,TO_ROOM);
   else
      {
      act("$n yells 'HELP!! MURDER!! Someone help me!'",TRUE, ch,0,0,TO_ROOM);
      sprintf(buf,"Help, I'm being attacked by %s!", GET_NAME(FIGHTING(ch)));
      do_gen_comm(ch,buf,0,SCMD_SHOUT);
      }
   release_buffer(buf);
   for (tch = character_list;tch;tch= tch_next)
      {
      tch_next=tch->next;
      if(!ch||!FIGHTING(ch))
         break;
      if(FIGHTING(tch))
         continue;
      if (GET_MOB_RNUM(tch) != -1 && (world[IN_ROOM(tch)].zone == world[IN_ROOM(ch)].zone)
              && (mob_index[GET_MOB_RNUM(tch)].func == cityguard)
              && (1==number(1,2)))
         {
         HUNTING(tch) = FIGHTING(ch);
         GET_MOB_VAL(tch,1)=10;
         if(IN_ROOM(tch)!=IN_ROOM(FIGHTING(ch)))
            hunt_victim(tch);
         }
      }
   }

/* a rager with no fight left calms down, and the rage wears off */
void rage_calm(struct char_data *ch)
   {
   if (!ch || FIGHTING(ch))
      return;
   if(AFF_FLAGGED(ch,AFF_RAGE))
      {
      send_to_char(ch, "You slowly calm down.\r\n");
      act("$n slowly calms down.",TRUE,ch,0,0,TO_ROOM);
      }
   affect_from_char(ch,SKILL_RAGE);
   }

void rage_check(struct char_data *ch)
   {
   struct char_data *tmp_victim;
   if(!ch)
      return;
   if(AFF_FLAGGED(ch,AFF_RAGE))
      for(tmp_victim=world[IN_ROOM(ch)].people;tmp_victim;
              tmp_victim=tmp_victim->next_in_room)
         {
         if(IS_NPC(tmp_victim))
            if((!AFF_FLAGGED(tmp_victim,AFF_CHARM) && !MOB2_FLAGGED(tmp_victim,MOB2_COMPONENT))||
                    (AFF_FLAGGED(tmp_victim,AFF_CHARM)&&
                     IS_NPC(tmp_victim->master)))
               {
               if(FIGHTING(ch))
                  stop_fighting(ch);
               set_fighting(ch,tmp_victim);
               if(!FIGHTING(tmp_victim))
                  {
                  set_fighting(tmp_victim,ch);
                  NEXT_HIT(tmp_victim)=7;
                  }
               break;
               }
         }
   rage_calm(ch);

   }

void mob_scavenge(struct char_data *ch)
   {
   int max;
   struct obj_data *obj, *best_obj;

   if (world[IN_ROOM(ch)].contents && (number(0, 10)<5))
      {
      max = 1;
      best_obj = NULL;
      for (obj = world[IN_ROOM(ch)].contents; obj; obj=obj->next_content)
         if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max)
            {
            best_obj = obj;
            max = GET_OBJ_COST(obj);
            }
      if (best_obj != NULL)
         {
         obj_from_room(best_obj);
         obj_to_char(best_obj, ch);
         act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
         }
      }
   }

void mob_aggro(struct char_data *ch)
   {
   int found;
   struct char_data *vict;

   found = FALSE;

   /* no attacks in peace rooms */
   if(ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
      return;

   for (vict = world[IN_ROOM(ch)].people;vict&&!found;vict=vict->next_in_room)
      {
      /* can we see them? */
      if (IS_NPC(vict) || !CAN_SEE(ch, vict)||PRF_FLAGGED(vict,PRF_NOHASSLE))
         continue;

      /* wimpy agg only attack sleeping people */
      if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
         continue;

      /* make sure both bits aren't set (items helping spells) */
      if(!(AFF_FLAGGED(vict,AFF_PROTECT_GOOD)&&
              AFF_FLAGGED(vict,AFF_PROTECT_EVIL)))
         {
         if(IS_EVIL(ch)&&AFF_FLAGGED(vict,AFF_PROTECT_EVIL)&& number(0,9)>2)
            continue;
         if(IS_GOOD(ch)&&AFF_FLAGGED(vict,AFF_PROTECT_GOOD)&& number(0,9)>2)
            continue;
         }

      /* Align check */
      if (!MOB_FLAGGED(ch, MOB_AGGR_TO_ALIGN) ||
              (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
              (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
              (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict)))
         {
         if(GET_POS(ch)<POS_STANDING)
            {
            act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch)=POS_STANDING;
            }
         hit(ch, vict, TYPE_UNDEFINED);
         found = TRUE;
         }
      }
   }

int mob_memory(struct char_data *ch)
   {
   int found;
   struct char_data *vict;
   memory_rec *names;

   found = FALSE;

   for(vict=world[IN_ROOM(ch)].people;vict&&!found;vict=vict->next_in_room)
      {
      if(IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict,PRF_NOHASSLE))
         continue;
      for(names = MEMORY(ch); names && !found; names = names->next)
         if (names->id == GET_IDNUM(vict))
            {
            found = TRUE;
            if(trait_info[GET_RACE(ch)].can_talk==0)
               do_action(ch,GET_NAME(vict),cmd_growl,0);
            else
               act("'Hey! You're the fiend that attacked me!!!', exclaims $n.",
                   FALSE, ch, 0, 0, TO_ROOM);
            if(GET_POS(ch)<POS_STANDING)
               {
               act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
               GET_POS(ch)=POS_STANDING;
               }
            if(ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
               act("$n fumes with rage!!",TRUE,ch,0,0,TO_ROOM);
            else
               hit(ch, vict, TYPE_UNDEFINED);
            return found;
            }
      }
   return 0;
   }

int mob_hunt_killer(struct char_data *ch)
   {
   int found,ranum,temple,dir;
   struct char_data *vict;
   char *buf;


   for (found = 0, vict = character_list; vict && !found; vict = vict->next)
      if ((!IS_NPC(vict) && IS_SET(PLR_FLAGS(vict), PLR_KILLER)) ||
              (!IS_NPC(vict) && IS_SET(PLR_FLAGS(vict), PLR_THIEF)))
         {
         found = 1;
         break;
         }

   if (found == 1)
      {
      if((GET_POS(ch)<POS_STANDING)&&(GET_HIT(ch)>(3*(GET_MAX_HIT(ch)/4))))
         {
         act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
         GET_POS(ch)=POS_STANDING;
         }
      else if(GET_POS(ch)<POS_STANDING)
         return 0;
      dir = find_first_step(IN_ROOM(ch), IN_ROOM(vict),
                            IGNORE_NOTRACK|IGNORE_ZNOTRACK);
      if (dir < 0)
         {
         ranum = number(0,100);
         if (ranum > 80)
            {
            buf=get_buffer(256);
            sprintf(buf, "Where the hell did %s go!!!", GET_NAME(vict));
            do_say(ch, buf, 0, 0);
            release_buffer(buf);
            }
         temple=real_room(3001);
         if (IN_ROOM(ch) != temple)
            do_recall(ch,NULL,0,0);
         return 1;
         }
      else
         {
         perform_move(ch, dir, 0,0);
         temple = real_room(3001);
         if (IN_ROOM(ch) != temple)
            if (IN_ROOM(vict) == temple)
               do_recall(ch,NULL,0,0);
         ranum = number(0,500);
         if ((ranum > 90) && (ranum < 100))
            {
            buf=get_buffer(256);
            sprintf(buf, "You WILL DIE %s. How dare you Player kill!!",
                    GET_NAME(vict));
            do_gen_comm(ch, buf, 0, SCMD_GOSSIP);
            release_buffer(buf);
            }
         else if ((ranum < 80) && (ranum >70))
            {
            buf=get_buffer(256);
            sprintf(buf, "%s, You are going to die a HORRIBLE DEATH!!",
                    GET_NAME(vict));
            do_gen_comm(ch, buf, 0, SCMD_OOC);
            release_buffer(buf);
            }
         else if ((ranum < 60) && (ranum >50))
            {
            buf=get_buffer(256);
            sprintf(buf, "%s, You have been CONDEMNED!! by the gods to DIE!!",
                    GET_NAME(vict));
            do_gen_comm(ch, buf, 0, SCMD_SHOUT);
            release_buffer(buf);
            }
         else if ((ranum < 40) && (ranum > 30))
            {
            buf=get_buffer(256);
            sprintf(buf, "You can not escape me %s!",
                    GET_NAME(vict));
            do_gen_comm(ch, buf, 0, SCMD_SHOUT);
            release_buffer(buf);
            }

         if (IN_ROOM(ch) == IN_ROOM(vict))
            {
            buf=get_buffer(256);
            if(trait_info[GET_RACE(ch)].can_talk==0)
               do_action(ch,GET_NAME(vict),cmd_growl,0);
            else
               {
               sprintf(buf, "Now you WILL DIE, %s!!", GET_NAME(vict));
               do_say(ch, buf, 0, 0);
               }
            release_buffer(buf);
            if(ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
               act("$n fumes with rage!!",TRUE,ch,0,0,TO_ROOM);
            else
               hit(ch, vict, 0);
            }
         return 1;
         }
      }
   return 0;
   }


int mob_hunt_memory(struct char_data *ch)
   {
   int found,ranum,way;
   struct char_data *vict;
   memory_rec *names;
   char *buf;

   for (found = 0, vict = character_list; vict && !found; vict = vict->next)
      {
      if (!IS_NPC(vict))
         for (names = MEMORY(ch); names&& !found; names = names->next)
            if (names->id == GET_IDNUM(vict))
               {
               found = 1;
               break;
               }
      if(found)
         break;
      }

   if (found == 1)
      {
      if((GET_POS(ch)<POS_STANDING)&&(GET_HIT(ch)>(3*(GET_MAX_HIT(ch)/4))))
         {
         act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
         GET_POS(ch)=POS_STANDING;
         }
      else if(GET_POS(ch)<POS_STANDING)
         return 0;

      way = find_first_step(IN_ROOM(ch), IN_ROOM(vict),
                            IGNORE_NOTRACK|IGNORE_ZNOTRACK);
      if (way < 0)
         {
         ranum = number(0,95);
         if (ranum > 90)
            {
            buf=get_buffer(256);
            sprintf(buf, "Come back here %s and fight like a man!!",
                    GET_NAME(vict));
            do_gen_comm(ch, buf, 0, SCMD_SHOUT);
            release_buffer(buf);
            }
         if ((ranum > 85) && (ranum < 90))
            {
            buf=get_buffer(256);
            sprintf(buf, "Oh %s where for art thou!!", GET_NAME(vict));
            do_gen_comm(ch, buf, 0, SCMD_SHOUT);
            release_buffer(buf);
            }
         if (IN_ROOM(ch) != ch->orig_room)
            {
            char_from_room(ch);
            char_to_room(ch, ch->orig_room);
            }
         return 1;
         }
      else
         {
         perform_move(ch, way, 0,0);
         ranum = number(0,500);
         if ((ranum > 90) && (ranum < 100))
            {
            buf=get_buffer(256);
            sprintf(buf, "That loser %s tried to kill me! Better watch your back!", GET_NAME(vict));
            do_gen_comm(ch, buf, 0, SCMD_SHOUT);
            release_buffer(buf);
            }
         else if ((ranum < 80) && (ranum > 70))
            {
            buf=get_buffer(256);
            sprintf(buf, "I am coming for you %s!!", GET_NAME(vict));
            do_gen_comm(ch, buf, 0, SCMD_SHOUT);
            release_buffer(buf);
            }
         else if ((ranum < 70) && (ranum >60))
            {
            buf=get_buffer(256);
            sprintf(buf, "You can not escape me %s!!", GET_NAME(vict));
            do_gen_comm(ch, buf, 0, SCMD_SHOUT);
            release_buffer(buf);
            }
         else if ((ranum < 60) && (ranum >50))
            {
            buf=get_buffer(256);
            sprintf(buf, "Stand still and fight you coward!! (what a wuss)");
            do_gen_comm(ch, buf, 0, SCMD_SHOUT);
            release_buffer(buf);
            }

         if (IN_ROOM(ch) == IN_ROOM(vict))
            {
            buf=get_buffer(256);
            if(trait_info[GET_RACE(ch)].can_talk==0)
               do_action(ch,GET_NAME(vict),cmd_growl,0);
            else
               {
               sprintf(buf, "How dare you attack me!, %s. Now you will DIE!!",
                       GET_NAME(vict));
               do_say(ch, buf, 0, 0);
               }
            if(ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
               act("$n fumes with rage!!",TRUE,ch,0,0,TO_ROOM);
            else
               hit(ch, vict, 0);
            release_buffer(buf);
            }
         return 1;
         }
      }
   return 0;
   }


int mob_go_path(struct char_data *ch, room_vnum dest)
   {
   int way;

   if((GET_POS(ch)<POS_STANDING)&&(GET_HIT(ch)>(3*(GET_MAX_HIT(ch)/4))))
      {
      act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch)=POS_STANDING;
      }
   else if(GET_POS(ch)<POS_STANDING)
      return 0;

   way = find_first_step(IN_ROOM(ch), real_room(dest),
                         IGNORE_NOTRACK|IGNORE_ZNOTRACK);
   if((way==-1)||(way==-3))
      {
      mudlogf(CMP,LVL_IMMORT,TRUE, "SYSERR:PATH:%d:  %s [%ld] could not find the path to %ld, make sure the room exists and a path exists.",
              way,GET_NAME(ch),GET_MOB_VNUM(ch),dest);
      GET_MOB_VAL(ch,9)=0;
      REMOVE_BIT(MOB_FLAGS(ch),MOB_GOPATH);
      return 0;
      }
   else if(way==-2)
      {
      GET_MOB_VAL(ch,9)=0;
      REMOVE_BIT(MOB_FLAGS(ch),MOB_GOPATH);
      return 0;
      }
   else
      {
      perform_move(ch,way,0,0);
      if(GET_ROOM_VNUM(IN_ROOM(ch))==dest)
         {
         GET_MOB_VAL(ch,9)=0;
         REMOVE_BIT(MOB_FLAGS(ch),MOB_GOPATH);
         }
      else
         {
         add_function_to_queue(FIGHTING(ch)?15:4,ch,0,2,(void (*) ()) mob_go_path,ch,dest);
         }
      if(mob_index[GET_MOB_RNUM(ch)].func == cityguard)
         GET_MOB_VAL(ch,1)=2;
      return 1;
      }
   return 1;

   }

void mob_helper(struct char_data *ch)
   {
   int found;
   struct char_data *vict;

   if(AFF_FLAGGED(ch,AFF_BLIND|AFF_CHARM))
      return;
   found = FALSE;

   for (vict = world[IN_ROOM(ch)].people;vict&&!found;vict=vict->next_in_room)
      {
      if((ch==vict) || !IS_NPC(vict) || !FIGHTING(vict))
         continue;
      if(IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
         continue;
      if(!CAN_SEE(ch,vict))
         continue;
      if(GET_POS(ch)<POS_STANDING)
         {
         if (!MOB2_FLAGGED(ch, MOB2_COMPONENT))
            act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
         GET_POS(ch)=POS_STANDING;
         }
      if (!MOB2_FLAGGED(ch, MOB2_COMPONENT))
         act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
      if(ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
         {
         if (!MOB2_FLAGGED(ch, MOB2_COMPONENT))
            act("$n fumes with rage!!",TRUE,ch,0,0,TO_ROOM);
         }
      else
         hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
      found = TRUE;
      }
   }

int mob_race_action(struct char_data *ch,struct char_data *vict)
   {
   return FALSE;
   }

int mob_class_action(struct char_data *ch,struct char_data *vict)
   {
   switch(GET_CLASS(ch))
      {
   case MCLASS_THIEF:
      if(!FIGHTING(ch))
         {
         if(GET_LEVEL(ch)>10&&
                 !AFF_FLAGGED(ch,AFF_SNEAK) &&
                 number(1,4)==1)
            {
            do_sneak(ch, "\0", 0, 0);
            return TRUE;
            }
         else if(GET_LEVEL(ch)>20 &&
                 !AFF_FLAGGED(ch,AFF_HIDE) &&
                 number(1,4)==1)
            {
            do_hide(ch,"\0",0,0);
            return TRUE;
            }
         }
      break;
   default:
      break;
      }
   return FALSE;
   }


void mob_position(struct char_data *ch)
   {
   if (MOB2_FLAGGED(ch, MOB2_COMPONENT))
      return;
   if((GET_POS(ch)==POS_STANDING)&&(GET_HIT(ch)<(2*(GET_MAX_HIT(ch)/3))))
      {
      if(AFF_FLAGGED(ch,AFF_PLAGUE) && !MOB_FLAGGED(ch, MOB_NOSLEEP))
         {
         act("$n sleeps to allow $s wounds to heal.",TRUE,ch,0,0,TO_ROOM);
         GET_POS(ch)=POS_SLEEPING;
         }
      else if(GET_LEVEL(ch)<40 || !trait_info[GET_RACE(ch)].has_hands)
         {
         act("$n rests to allow $s wounds to heal.",TRUE,ch,0,0,TO_ROOM);
         GET_POS(ch)=POS_RESTING;
         }
      else
         {
         act("$n bandages $s wounds and rests.",TRUE,ch,0,0,TO_ROOM);
         GET_POS(ch)=POS_BANDAGE;
         }
      }
   else if ((GET_POS(ch)!=GET_DEFAULT_POS(ch))&&
            (GET_HIT(ch) > (GET_MAX_HIT(ch)-10)))
      {
      switch(GET_DEFAULT_POS(ch))
         {
      case POS_STANDING:
         act("$n stands up and looks around.",TRUE,ch,0,0,TO_ROOM);
         GET_POS(ch)=POS_STANDING;
         break;
      case POS_RESTING:
         act("$n sits down for a rest.",TRUE,ch,0,0,TO_ROOM);
         GET_POS(ch)=POS_RESTING;
         break;
      case POS_SITTING:
         act("$n sits down.",TRUE,ch,0,0,TO_ROOM);
         GET_POS(ch)=POS_SITTING;
         break;
      case POS_BANDAGE:
         act("$n bandages $s wounds and rests.",TRUE,ch,0,0,TO_ROOM);
         GET_POS(ch)=POS_BANDAGE;
         break;
      case POS_MEDITATE:
         act("$n sits down and starts to meditate.",TRUE,ch,0,0,TO_ROOM);
         GET_POS(ch)=POS_MEDITATE;
         break;
      case POS_CHANT:
         act("$n sits down and starts to chant.",TRUE,ch,0,0,TO_ROOM);
         GET_POS(ch)=POS_CHANT;
         break;
      case POS_SLEEPING:
         act("$n lies down and goes to sleep",TRUE,ch,0,0,TO_ROOM);
         GET_POS(ch)=POS_SLEEPING;
         break;
      default:
         mudlogf(BRF,LVL_IMMORT,TRUE,
                 "SYSERR: Bad default position: %d on mob: %s[%ld].",
                 GET_DEFAULT_POS(ch),GET_NAME(ch),GET_MOB_VNUM(ch));
         break;
         }
      }
   }

/* the main loop */
void mobile_activity(void)
   {
   struct char_data *ch,*next_ch;
   int door, i, dir;
   static int mobs_to_run=0;


   if(++mobs_to_run>MOBILE_PERCENT)
      mobs_to_run=1;

   for (ch = character_list,i=1; ch; ch = next_ch,i++)
      {
      next_ch = ch->next;

      /* run every MOBILE_PERCENT'th mob, we are going through this loop */
      /* once per second */
      if(i>MOBILE_PERCENT)
         i=1;
      if(i!=mobs_to_run)
         continue;


      if (!IS_MOB(ch))
         {
         if(!FIGHTING(ch))
            LAST_HAND_USED(ch)=0;
         continue;
         }

      if(GET_WAIT_STATE(ch)>0)
         {
         GET_WAIT_STATE(ch)=MAX(0,GET_WAIT_STATE(ch)-20);
         continue;
         }

      /* all fighting mob_activity goes here */
      if(FIGHTING(ch))
         {
         if(mob_race_action(ch,FIGHTING(ch)))
            continue;
         else if(mob_class_action(ch,FIGHTING(ch)))
            continue;

         if(MOB_FLAGGED(ch,MOB_CITIZEN)&& (1==number(1,3)) &&
                 (FIGHTING(FIGHTING(ch))==ch))
            {
            mob_citizen_help(ch);
            }
         continue;
         }
      else
         {
         if(mob_race_action(ch,ch))
            continue;
         else if (mob_class_action(ch,ch))
            continue;
         }

      if(AFF_FLAGGED(ch,AFF_RAGE))
         rage_check(ch);

      LAST_HAND_USED(ch)=0;

      /* Examine call for special procedure */
      if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials)
         {
         if (mob_index[GET_MOB_RNUM(ch)].func == NULL)
            {
            log("SYSERR: %s (#%ld): Attempting to call non-existing mob func",
                GET_NAME(ch), GET_MOB_VNUM(ch));
            REMOVE_BIT(MOB_FLAGS(ch), MOB_SPEC);
            }
         else
            {
            if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, ""))
               continue;  /* go to next char */
            }
         }

      if(!AWAKE(ch))
         continue;

      if(!is_empty(world[IN_ROOM(ch)].zone))
         mprog_random_trigger(ch);

      /* Scavenger (picking up objects) */
      if (MOB_FLAGGED(ch, MOB_SCAVENGER) && AWAKE(ch))
         mob_scavenge(ch);

      /* Mob Movement */
      if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
              ((door = number(0, 18)) < NUM_OF_DIRS) && CAN_GO(ch, door) &&
              !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB | ROOM_DEATH) &&
              !ROOM2_FLAGGED(EXIT(ch, door)->to_room, ROOM2_NEVERMOB) && (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
               (world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone)) &&
              (!MOB_FLAGGED(ch, MOB_STAY_TERRAIN) ||
               (world[EXIT(ch, door)->to_room].sector_type == world[IN_ROOM(ch)].sector_type)))
         {
         if(ch->mob_specials.last_direction==door)
            {
            ch->mob_specials.last_direction=-1;
            }
         else
            {
            ch->mob_specials.last_direction=door;
            perform_move(ch, door, 1,0);
            }
         }

      /* MOB Prog foo */
      if(IS_NPC(ch) && ch->mpactnum > 0 && !is_empty(world[IN_ROOM(ch)].zone))
         {
         MPROG_ACT_LIST *tmp_act, *tmp2_act;
         for(tmp_act = ch->mpact; tmp_act != NULL; tmp_act=tmp_act->next)
            {
            mprog_wordlist_check(tmp_act->buf, ch, tmp_act->ch,
                                 tmp_act->obj, tmp_act->vo, ACT_PROG);
            free(tmp_act->buf);
            }
         for(tmp_act = ch->mpact; tmp_act != NULL; tmp_act=tmp2_act)
            {
            tmp2_act = tmp_act->next;
            free(tmp_act);
            }
         ch->mpactnum = 0;
         ch->mpact = NULL;
         }

      /* Aggressive Mobs */
      if (MOB_FLAGGED(ch, MOB_AGGRESSIVE | MOB_AGGR_TO_ALIGN))
         mob_aggro(ch);

      /* Mob Memory */
      if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch))
         if(mob_memory(ch))
            continue;

      /* Helper Mobs */
      if (MOB_FLAGGED(ch, MOB_HELPER))
         mob_helper(ch);

      /* pkiller hunter */
#if 0

      if(MOB_FLAGGED(ch, MOB_HUNT_KILLER)&&
              !MOB_FLAGGED(ch, MOB_AGGRESSIVE | MOB_AGGR_TO_ALIGN))
         if(mob_hunt_killer(ch))
            continue;
#endif
      /* Hunt people that attacked me */
      if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch) &&
              MOB_FLAGGED(ch, MOB_HUNT_MEMORY))
         if(mob_hunt_memory(ch))
            continue;

      /* Go to a specified location. */
      if(MOB_FLAGGED(ch,MOB_GOPATH)&&(GET_MOB_VAL(ch,9)>0))
         if(mob_go_path(ch,GET_MOB_VAL(ch,9)))
            {
            GET_MOB_VAL(ch,9)=0;
            continue;
            }

      /* go to your master */
      if(MOB_FLAGGED(ch,MOB_GUARD) &&
              AFF_FLAGGED(ch,AFF_GROUP) &&
              ch->master &&
              (IN_ROOM(ch)!=IN_ROOM(ch->master)) &&
              !FIGHTING(ch) &&
              !((mob_index[GET_MOB_RNUM(ch)].func==cityguard)&&
                (GET_MOB_VAL(ch,1)>0)))
         {
         dir=find_first_step(IN_ROOM(ch),IN_ROOM(ch->master),
                             IGNORE_NOTRACK|IGNORE_ZNOTRACK);
         if(dir>=0)
            perform_move(ch,dir,1,0);
         continue;
         }


      /* Do stuff that my class and race allow */
      if(mob_race_action(ch,0))
         continue;
      else if(mob_class_action(ch,0))
         continue;

      /* have me rest when hurt and stand when better */
      if(!AFF_FLAGGED(ch,AFF_CHARM))
         mob_position(ch);

      /* Cymy's favorite code, this stuff allows the mobs to seem a bit */
      /* more alive */
      if(MOB_FLAGGED(ch,MOB_CITIZEN)&&!number(0,65))
         citizen(ch,ch,SPEC_MOBACT,"");
      else if(IS_HUMANOID(ch)&&!number(0,100))
         moods(ch,ch,SPEC_MOBACT,"");

      /* Add new mobile actions here */
      /* please add a function for each type */

      }
   /* end for() */
   }



/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data * ch, struct char_data * victim)
   {
   memory_rec *tmp;
   bool present = FALSE;

   if (!IS_NPC(ch) || IS_NPC(victim))
      return;

   for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
      if (tmp->id == GET_IDNUM(victim))
         present = TRUE;

   if (!present)
      {
      CREATE(tmp, memory_rec, 1);
      tmp->next = MEMORY(ch);
      tmp->id = GET_IDNUM(victim);
      MEMORY(ch) = tmp;
      }
   }


/* make ch forget victim */
void forget(struct char_data * ch, struct char_data * victim)
   {
   memory_rec *curr, *prev = NULL;

   if (!(curr = MEMORY(ch)))
      return;

   while (curr && curr->id != GET_IDNUM(victim))
      {
      prev = curr;
      curr = curr->next;
      }

   if (!curr)
      return;   /* person wasn't there at all. */

   if (curr == MEMORY(ch))
      MEMORY(ch) = curr->next;
   else
      prev->next = curr->next;

   free(curr);
   }


/* erase ch's memory */
void clearMemory(struct char_data * ch)
   {
   memory_rec *curr, *next;

   curr = MEMORY(ch);

   while (curr)
      {
      next = curr->next;
      free(curr);
      curr = next;
      }

   MEMORY(ch) = NULL;
   }

/*
 * begin add - Bon 07/25/97 
 * room teleport code from Phoenix 3 
 */
void teleport(int room_to, int room_in, struct char_data *ch)
   {
   int tmp = 0;
   int target_rnum;

   if (IN_ROOM(ch) == room_in)
      {
      tmp = real_room(room_to);
      if(tmp<0)
         {
         log("ERROR IN TELEPORT CODE.  unknown room");
         tmp=0;
         }

      if(world[IN_ROOM(ch)].tele->obj!=-1)
         {
         if((target_rnum=real_object(world[IN_ROOM(ch)].tele->obj))!=-1)
            {
            if(IS_SET(world[IN_ROOM(ch)].tele->bitvector,TELE_OBJ)&&
                    !has_object(ch,target_rnum))
               return;
            else if(IS_SET(world[IN_ROOM(ch)].tele->bitvector,TELE_NOOBJ)&&
                    has_object(ch,target_rnum))
               return;
            }
         }

      if(!IS_SET(world[room_in].tele->bitvector,TELE_NOMSG))
         {
         if(world[room_in].tele->to_source_room&&
                 (strlen(world[room_in].tele->to_source_room)>1))
            act(world[room_in].tele->to_source_room,FALSE,ch,0,0,TO_ROOM);
         else
            act("$n has left.", FALSE, ch, 0, 0, TO_ROOM);

         if(world[room_in].tele->to_char&&
                 (strlen(world[room_in].tele->to_char)>1))
            act(world[room_in].tele->to_char,FALSE,ch,0,0,TO_CHAR);
         else
            act("You have a very strange feeling.",FALSE,ch,0,0,TO_CHAR);
         }
      char_from_room(ch);
      char_to_room(ch, tmp);
      if(!IS_SET(world[room_in].tele->bitvector,TELE_NOMSG))
         {
         if(world[room_in].tele->to_targ_room&&
                 (strlen(world[room_in].tele->to_targ_room)>1))
            act(world[room_in].tele->to_targ_room,TRUE,ch,0,0,TO_ROOM);
         else
            act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);
         }
      if(IS_SET(world[room_in].tele->bitvector,TELE_LOOK)&&!IS_NPC(ch))
         {
         if(!IS_SET(world[room_in].tele->bitvector,TELE_NOMSG))
            send_to_char(ch,"\r\n");
         do_look(ch, "\0", 0, 0);
	 if (!IS_NPC(ch)) {
	   ch->char_specials.timer = 0;
	 }
         }
      if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH))
         {
         if (GET_LEVEL(ch) < LVL_IMMORT)
            {
	      if (handleGetOutOfDeathFree(ch)) {
		return;
	      }

            log_death_trap(ch);
            death_cry(ch);
            extract_char(ch);
            }
         }
      }
   else
      return;
   }

void  teleport_update(struct char_data *ch)
   {
   int when;
   int target_rnum;

   if(!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE))
      return;
   if ((world[IN_ROOM(ch)].tele!= NULL) &&
           (world[IN_ROOM(ch)].tele->targ != 0))
      {
      if(IS_SET(world[IN_ROOM(ch)].tele->bitvector,TELE_NOMOB)&&IS_MOB(ch))
         return;
      if(IS_SET(world[IN_ROOM(ch)].tele->bitvector,TELE_NOPC)&&!IS_MOB(ch))
         return;
      if(IS_SET(world[IN_ROOM(ch)].tele->bitvector,TELE_NVRMOB)&&IS_MOB(ch))
         return;
      if(world[IN_ROOM(ch)].tele->obj!=-1)
         {
         if((target_rnum=real_object(world[IN_ROOM(ch)].tele->obj))!=-1)
            {
            if(IS_SET(world[IN_ROOM(ch)].tele->bitvector,TELE_OBJ)&&
                    !has_object(ch,target_rnum))
               return;
            else if(IS_SET(world[IN_ROOM(ch)].tele->bitvector,TELE_NOOBJ)&&
                    has_object(ch,target_rnum))
               return;
            }
         }


      if(IS_SET(world[IN_ROOM(ch)].tele->bitvector,TELE_COUNT))
         when = world[IN_ROOM(ch)].tele->time;
      else if(IS_SET(world[IN_ROOM(ch)].tele->bitvector,TELE_RANDOM))
         when = number(0,world[IN_ROOM(ch)].tele->time);
      else
         when = world[IN_ROOM(ch)].tele->time;
      add_function_to_queue(when,
                            ch,0,3,teleport,
                            world[IN_ROOM(ch)].tele->targ,
                            IN_ROOM(ch),ch);

      }
   }



