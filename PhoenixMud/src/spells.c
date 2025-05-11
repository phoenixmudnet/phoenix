/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD * 
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    * 
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
#include "comm.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "dg_scripts.h"
#include "vnum.h"
#include "clan.h"
#include "queue.h"
#include "screen.h"
#include "constants.h"

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
extern int cmd_faint;

char *acstr(int x);
char *naturestr(int x);
char *bonstr(int x);
void clearMemory(struct char_data * ch);
void weight_change_object(struct obj_data * obj, int weight);
void name_to_drinkcon(struct obj_data * obj, int type);
void name_from_drinkcon(struct obj_data * obj);
void stun_by_spell(struct char_data *ch,int iPhase);
void dismount_char(struct char_data *ch);
int compute_armor_class(struct char_data *ch);
ACMD(do_look);
ACMD(do_action);
struct char_data *item_owner(struct obj_data *obj);

/*
 * Special spells appear below. 
 */

ASPELL(spell_create_water)
   {
   int water;


   if (ch == NULL || obj == NULL)
      return FALSE;
   level = MAX(MIN(level,10), 1);

   if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
      {
      if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0))
         {
         name_from_drinkcon(obj);
         GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
         name_to_drinkcon(obj, LIQ_SLIME);
         return FALSE;
         }
      else
         {
         water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
         if (water > 0)
            {
            if(GET_OBJ_VAL(obj,1)>=1)
               name_from_drinkcon(obj);
            GET_OBJ_VAL(obj, 2) = LIQ_WATER;
            GET_OBJ_VAL(obj, 1) += water;
            name_to_drinkcon(obj, LIQ_WATER);
            weight_change_object(obj, water);
            act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
            }
         }
      }
   return TRUE;
   }

ASPELL(spell_portal)
   {
   /* create a magic portal */
   struct obj_data *tmp_obj;
   /*
    * struct obj_data *tmp_obj2; 
    */
   struct extra_descr_data *ed;
   struct room_data *rp, *nrp;
   struct char_data *tmp_ch = (struct char_data *) victim;
   char *buf;

   assert(ch);
   assert((level >= 0) && (level <=10));
   level = MAX(MIN(level,10), 1);


   /*
     check target room for legality. 
    */
   rp = &world[IN_ROOM(ch)];
   tmp_obj = read_object(VNUM_PORTAL, VIRTUAL);
   if (!rp || !tmp_obj)
      {
      send_to_char(ch,"The magic fails\r\n");
      extract_obj(tmp_obj);
      return FALSE;
      }

   if (IS_SET(rp->room_flags, ROOM_NOMAGIC))
      {
      send_to_char(ch,"Eldritch wizardry obstructs thee.\r\n");
      extract_obj(tmp_obj);
      return FALSE;
      }

   if (IS_SET(rp->room_flags, ROOM_TUNNEL))
      {
      send_to_char(ch,"There is no room in here to create a portal!\r\n");
      extract_obj(tmp_obj);
      return FALSE;
      }

   if (!(nrp = &world[IN_ROOM(tmp_ch)]))
      {
      log("%s not in any room (spell portal)", GET_NAME(tmp_ch));
      send_to_char(ch,"The magic cannot locate the target\n");
      extract_obj(tmp_obj);
      return FALSE;
      }

   if (ROOM_FLAGGED(IN_ROOM(tmp_ch), ROOM_NOMAGIC)||
           ROOM_FLAGGED(IN_ROOM(tmp_ch), ROOM_HOUSE))
      {
      send_to_char(ch,"Your target is protected against your magic.\r\n");
      extract_obj(tmp_obj);
      return FALSE;
      }

   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_NO_SUMMON)||
           ROOM_FLAGGED(IN_ROOM(tmp_ch),ROOM_NO_SUMMON)||
           Z_FLAGGED(IN_ROOM(ch),Z_NO_SUMMON)||
           Z_FLAGGED(IN_ROOM(tmp_ch),Z_NO_SUMMON)||
           MOB_FLAGGED(tmp_ch, MOB_NOSUMMON))
      {
      send_to_char(ch,"Something seems to block your magic!!\r\n");
      extract_obj(tmp_obj);
      return FALSE;
      }

   if(ROOM_FLAGGED(IN_ROOM(victim),ROOM_CLAN)&&((GET_CLAN(ch)!=GET_CLAN(victim))||
            !GET_CLAN(ch)))
      {
      send_to_char(ch,"Your target is protected against your magic.\r\n");
      extract_obj(tmp_obj);
      return FALSE;
      }

   if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
           !PLR_FLAGGED(victim, PLR_KILLER))
      {
      send_to_char(victim,"%s just tried to open a portal to you from: %s.\r\n"
                   "%s failed because you have summon protection on.\r\n"
                   "Type NOSUMMON to allow other players to summon you.\r\n",
                   GET_NAME(ch), world[IN_ROOM(ch)].name,
                   (ch->player.sex == SEX_MALE) ? "He" : "She");

      send_to_char(ch, "You failed because %s has summon protection on.\r\n",
                   GET_NAME(victim));

      mudlogf(BRF, LVL_IMMORT, TRUE,
              "%s failed portaling to %s from %s.",
              GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
      return FALSE;
      }

   buf=get_buffer(256);
   sprintf(buf, "Through the mists of the portal, you can faintly see %s\r\n",
           nrp->name);

   CREATE(ed , struct extra_descr_data, 1);
   ed->next = tmp_obj->ex_description;
   tmp_obj->ex_description = ed;
   CREATE(ed->keyword, char, strlen(tmp_obj->name) + 1);
   strcpy(ed->keyword, tmp_obj->name);
   ed->description = str_dup(buf);
   release_buffer(buf);

   tmp_obj->obj_flags.value[0] = ((level-1)*2)+1;
   tmp_obj->obj_flags.value[1] = GET_ROOM_VNUM(IN_ROOM(tmp_ch));

   obj_to_room(tmp_obj,IN_ROOM(ch));

   GET_OBJ_TIMER(tmp_obj)=3;
   act("$p suddenly appears.",TRUE,ch,tmp_obj,0,TO_ROOM);
   act("$p suddenly appears.",TRUE,ch,tmp_obj,0,TO_CHAR);
   return TRUE;

   /* Portal at other side */
   /*
      rp = &world[IN_ROOM(ch)]; 
      tmp_obj2 = read_object(VNUM_PORTAL, VIRTUAL); 
      if (!rp || !tmp_obj2) 
         { 
         send_to_char("The magic fails\r\n", ch); 
         extract_obj(tmp_obj2); 
         return FALSE; 
         } 
      buf=get_buffer(256);
      sprintf(buf,  
       "Through the mists of the portal, you can faintly see %s\r\n",rp->name);
    
      CREATE(ed , struct extra_descr_data, 1); 
      ed->next = tmp_obj2->ex_description; 
      tmp_obj2->ex_description = ed; 
      CREATE(ed->keyword, char, strlen(tmp_obj2->name) + 1); 
      strcpy(ed->keyword, tmp_obj2->name); 
      ed->description = str_dup(buf); 
      release_buffer(buf);
    
      tmp_obj2->obj_flags.value[0] = ((level-1)*2)+1; 
      tmp_obj2->obj_flags.value[1] =GET_ROOM_VNUM(IN_ROOM(ch)); 
    
      obj_to_room(tmp_obj2,IN_ROOM(tmp_ch)); 
      GET_OBJ_TIMER(tmp_obj2)=12;
      act("$p suddenly appears.",TRUE,tmp_ch,tmp_obj2,0,TO_ROOM); 
      act("$p suddenly appears.",TRUE,tmp_ch,tmp_obj2,0,TO_CHAR); 
      */
   }

ASPELL(spell_recall)
   {
   struct follow_type *f, *f_next;

   if (victim == NULL)
      return FALSE;

   if(GET_LEVEL(ch)<LVL_IMMORT && GET_LEVEL(victim)>=LVL_IMMORT)
      {
      send_to_char(ch,"You fail!\r\n");
      return FALSE;
      }
   level = MAX(MIN(level,10), 1);


   if (victim->char_specials.in_battle == TRUE)
      {
      send_to_char(victim,"You can not recall out of the battle zone!!\r\n");
      return FALSE;
      }

   if (ch->char_specials.in_battle == TRUE)
      {
      send_to_char(ch,"You can not recall out of the battle zone!!\r\n");
      return FALSE;
      }

   if (IS_NPC(victim)&&(FIGHTING(victim)||GET_POS(victim)==POS_FIGHTING))
      {
      send_to_char(ch,"You failed recalling %s.\r\n", GET_NAME(victim));
      return FALSE;
      }

   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_NO_RECALL)||
           ROOM_FLAGGED(IN_ROOM(victim),ROOM_NO_RECALL)||
           Z_FLAGGED(IN_ROOM(ch),Z_NO_RECALL)||
           Z_FLAGGED(IN_ROOM(victim),Z_NO_RECALL))
      {
      send_to_char(ch,"Something seems to block your magic!!\r\n");
      return FALSE;
      }

   if (!IS_NPC(ch) && !IS_NPC(victim) && (!PRF2_FLAGGED(victim, PRF2_RECALLABLE)) && 
       !ROOM_FLAGGED(IN_ROOM(ch),ROOM_CLAN) && (ch != victim))
      {
      send_to_char(victim, "%s just tried to recall you.\r\n"
                   "%s failed because you have recall protection on.\r\n"
                   "Type NORECALL to allow other players to recall you.\r\n",
                   GET_NAME(ch),(ch->player.sex == SEX_MALE) ? "He" : "She");

      send_to_char(ch,"You failed because %s has recall protection on.\r\n",
                   GET_NAME(victim));

      mudlogf(BRF, LVL_IMMORT, TRUE,
              "%s failed recalling %s at %s.",
              GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
      return FALSE;
      }

   if (IS_NPC(victim) && (MOB2_FLAGGED(victim, MOB2_COMPONENT)))
      {
      send_to_char(ch, "How'd you like to get YOUR limb recalled, hmm?\r\n");
      return FALSE;
      }

   if (FIGHTING(victim)&&IS_NPC(FIGHTING(victim)))
      {
      if(num_fighting(victim)<2)
         if(GET_HIT(FIGHTING(victim))<(GET_MAX_HIT(FIGHTING(victim))/2))
            GET_HIT(FIGHTING(victim))=GET_MAX_HIT(FIGHTING(victim))/2;
      }

   act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
   dismount_char(victim);
   char_from_room(victim);
   if(IS_NPC(victim))
      {
      char_to_room(victim, victim->orig_room);
      }
   else
      {
      char_to_room(victim, real_room(GET_HOME(victim)));
      look_at_room(victim, 0);
      }
   /* component mobs */
   if (IS_NPC(victim) && victim->followers)
      for (f = victim->followers; f; f = f_next)
         {
         f_next = f->next;
         if (MOB2_FLAGGED(f->follower, MOB2_COMPONENT))
            {
            char_from_room(f->follower);
            char_to_room(f->follower, IN_ROOM(victim));
            }
         }

   act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
   entry_memory_mtrigger(victim);
   greet_mtrigger(victim, -1);
   greet_memory_mtrigger(victim);
   return TRUE;
   }


ASPELL(spell_crecall)
   {
   if (victim == NULL)
      return FALSE;

   level = MAX(MIN(level,10), 1);

   if(!GET_CLAN(victim))
      {
      send_to_char(victim,"How to you expect to clan recall when you aren't"
                   " in a clan!\r\n");
      return FALSE;
      }
   if (victim->char_specials.in_battle == TRUE)
      {
      send_to_char(victim,"You can not recall out of the battle zone!!\r\n");
      return FALSE;
      }

   if (ch->char_specials.in_battle == TRUE)
      {
      send_to_char(ch,"You can not recall out of the battle zone!!\r\n");
      return FALSE;
      }

   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_NO_RECALL)||
           ROOM_FLAGGED(IN_ROOM(victim),ROOM_NO_RECALL)||
           Z_FLAGGED(IN_ROOM(ch),Z_NO_RECALL)||
           Z_FLAGGED(IN_ROOM(victim),Z_NO_RECALL))
      {
      send_to_char(ch,"Something seems to block your magic!!\r\n");
      return FALSE;
      }

  if (!IS_NPC(ch) && !IS_NPC(victim) && (!PRF2_FLAGGED(victim, PRF2_RECALLABLE)) &&
       !ROOM_FLAGGED(IN_ROOM(ch),ROOM_CLAN) && (ch != victim))
      {
      send_to_char(victim, "%s just tried to crecall you.\r\n"
                   "%s failed because you have recall protection on.\r\n"
                   "Type NORECALL to allow other players to crecall you.\r\n",
                   GET_NAME(ch),(ch->player.sex == SEX_MALE) ? "He" : "She");
   
      send_to_char(ch,"You failed because %s has recall protection on.\r\n",
                   GET_NAME(victim));
      
      mudlogf(BRF, LVL_IMMORT, TRUE,
              "%s failed crecalling %s at %s.",
              GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
      return FALSE;
      }

   if (FIGHTING(victim)&&IS_NPC(FIGHTING(victim)))
      {
      if(num_fighting(victim)<2)
         if(GET_HIT(FIGHTING(victim))<(GET_MAX_HIT(FIGHTING(victim))/2))
            GET_HIT(FIGHTING(victim))=GET_MAX_HIT(FIGHTING(victim))/2;
      }


   act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
   char_from_room(victim);
   if(IS_NPC(victim))
      {
      char_to_room(victim, victim->orig_room);
      }
   else
      {
      char_to_room(victim, real_room(GET_CLAN_ROOM(victim)));
      look_at_room(victim, 0);
      }
   act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
   entry_memory_mtrigger(victim);
   greet_mtrigger(victim, -1);
   greet_memory_mtrigger(victim);
   return TRUE;
   }


ASPELL(spell_gate)
   {

   if (GET_LEVEL(victim) >= LVL_IMMORT)
      {
      send_to_char(ch,"Gating to immortals can be hazardous to your health!\n\r");
      return FALSE;
      }

   if (victim == NULL || IS_NPC(victim))
      {
      send_to_char(ch,"You dont seem to be able to open a gate to this person.\n\r");
      return FALSE;
      }

   if (FIGHTING(ch))
      {
      stop_fighting(ch);
      }
   if(ROOM_FLAGGED(IN_ROOM(victim),ROOM_CLAN)&&(GET_CLAN(ch)!=GET_CLAN(victim)))
      {
      send_to_char(ch,"You can not gate to a member of another clan while they are in their clan hall!!\r\n");
      return FALSE;
      }
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_NO_SUMMON)||
           ROOM_FLAGGED(IN_ROOM(victim),ROOM_NO_SUMMON)||
           Z_FLAGGED(IN_ROOM(ch),Z_NO_SUMMON)||
           Z_FLAGGED(IN_ROOM(victim),Z_NO_SUMMON)||
           ROOM_FLAGGED(IN_ROOM(victim), ROOM_HOUSE))
      {
      send_to_char(ch,"Something seems to block your magic!!\r\n");
      return FALSE;
      }

   if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
           !PLR_FLAGGED(victim, PLR_KILLER))
      {
      send_to_char(victim, "%s just tried to gate to you from: %s.\r\n"
                   "%s failed because you have summon protection on.\r\n"
                   "Type NOSUMMON to allow other players to summon you.\r\n",
                   GET_NAME(ch), world[IN_ROOM(ch)].name,
                   (ch->player.sex == SEX_MALE) ? "He" : "She");

      send_to_char(ch,"You failed because %s has summon protection on.\r\n",
                   GET_NAME(victim));

      mudlogf(BRF, LVL_IMMORT, TRUE,
              "%s failed gating to %s from %s.",
              GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
      return FALSE;
      }
   act("$n opens a swirling gate of fire and disappears.",TRUE,ch,0,0,TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, IN_ROOM(victim));
   act("$n appears out of a swirling gate of fire.", TRUE, ch, 0, 0, TO_ROOM);
   do_look(ch,"",0,0);
   entry_memory_mtrigger(ch);
   greet_mtrigger(ch, -1);
   greet_memory_mtrigger(ch);
   return TRUE;
   }





ASPELL(spell_teleport)
   {
   room_rnum to_room;

   if (victim != ch)
      if (!IS_NPC(ch))
         return FALSE;

   level = MAX(MIN(level,10), 1);

   if(Z_FLAGGED(IN_ROOM(ch),Z_NO_TELEPORT))
      {
      send_to_char(ch,"Something seems to block your magic!!\r\n");
      return FALSE;
      }

   do
      {
      to_room = number(0, top_of_world);
      }
   while (ROOM_FLAGGED(to_room, ROOM_NO_SUMMON|ROOM_CLAN|ROOM_GODROOM|ROOM_PRIVATE|ROOM_DEATH) ||
          Z_FLAGGED(to_room,Z_NO_TELEPORT) || (IN_ROOM(ch)==to_room));

   act("$n slowly fades out of existence and is gone.",
       FALSE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, to_room);
   act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
   look_at_room(ch, 0);
   entry_memory_mtrigger(ch);
   greet_mtrigger(ch, -1);
   greet_memory_mtrigger(ch);
   return TRUE;
   }

#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_summon)
   {
   struct follow_type *f, *f_next;
   int iEscapeFromFight=0;
   if (ch == NULL || victim == NULL)
      return FALSE;
   level = MAX(MIN(level,10), 1);

   if (GET_LEVEL(victim) > MIN((LVL_IMMORT - 1), (50+(level*6)) ))
      {
      send_to_char(ch,SUMMON_FAIL);
      return FALSE;
      }

   /*if (IS_NPC(victim) && !MOB2_FLAGGED(victim, MOB2_SUMMONABLE))
      {
      send_to_char(ch,SUMMON_FAIL);
      return FALSE;
      }
   */

   if (IS_NPC(victim) && MOB2_FLAGGED(victim, MOB2_COMPONENT))
      {
      send_to_char(ch,SUMMON_FAIL);
      return FALSE;
      }

   if (!pk_allowed)
      {
      if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)||
              MOB_FLAGGED(victim, MOB_AGGR_GOOD)||
              MOB_FLAGGED(victim, MOB_AGGR_EVIL)||
              MOB_FLAGGED(victim, MOB_AGGR_NEUTRAL))
         {
         act("As the words escape your lips and $N travels\r\n"
             "through time and space towards you, you realize that $E is\r\n"
             "aggressive and might harm you, so you wisely send $M back.",
             FALSE, ch, 0, victim, TO_CHAR);
         return FALSE;
         }


      if (victim->char_specials.in_battle == TRUE)
         {
         send_to_char(ch,"That person is fighting for his life right now in battle!!\r\n");
         return FALSE;
         }

      if (ch->char_specials.in_battle == TRUE)
         {
         send_to_char(ch,"You can not summon while in battle!!\r\n");
         return FALSE;
         }
      if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_NO_SUMMON)||
              ROOM_FLAGGED(IN_ROOM(victim),ROOM_NO_SUMMON)||
              Z_FLAGGED(IN_ROOM(ch),Z_NO_SUMMON)||
              Z_FLAGGED(IN_ROOM(victim),Z_NO_SUMMON))
         {
         send_to_char(ch,"Something seems to block your magic!!\r\n");
         return FALSE;
         }

      if(IS_NPC(victim) && ROOM_FLAGGED(IN_ROOM(ch),ROOM_CLAN))
         {
         send_to_char(ch,"Something seems to block your magic!!\r\n");
         return FALSE;
         }

      if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
              !PLR_FLAGGED(victim, PLR_KILLER))
         {
         send_to_char(victim, "%s just tried to summon you to: %s.\r\n"
                      "%s failed because you have summon protection on.\r\n"
                      "Type NOSUMMON to allow other players to summon you.\r\n",
                      GET_NAME(ch), world[IN_ROOM(ch)].name,
                      (ch->player.sex == SEX_MALE) ? "He" : "She");

         send_to_char(ch,"You failed because %s has summon protection on.\r\n",
                      GET_NAME(victim));

         mudlogf(BRF, LVL_IMMORT, TRUE,
                 "%s failed summoning %s to %s.",
                 GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
         return FALSE;
         }
      }

   if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
           (IS_NPC(victim) && mag_savingthrow(victim, SAVING_SPELL,level,0)))
      {
      send_to_char(ch,SUMMON_FAIL);
      return FALSE;
      }

   if(FIGHTING(victim) && IS_NPC(victim))
      {
      send_to_char(ch,SUMMON_FAIL);
      return FALSE;
      }

   
   int rnum = IN_ROOM(ch);
   if (IN_ROOM(ch) > 0 && GET_LEVEL(ch) < LVL_IMMORT && IS_NPC(victim)) {
     int num = num_npcs_in_room(ch);
     printf("rnum: %d, num: %d\n", rnum, num);
     if (num >= 3) {
       send_to_char(ch,"This area is too crowded for a summon.\r\n");
       return FALSE;
     }
   }

   if (FIGHTING(victim)&&IS_NPC(FIGHTING(victim)))
      {
      iEscapeFromFight=1;
      if(num_fighting(victim)<2)
         if(GET_HIT(FIGHTING(victim))<(GET_MAX_HIT(FIGHTING(victim))/2))
            GET_HIT(FIGHTING(victim))=GET_MAX_HIT(FIGHTING(victim))/2;
      }
   act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);
   if(victim!=ch)
      {
      dismount_char(victim);
      char_from_room(victim);
      char_to_room(victim, IN_ROOM(ch));
      }
   /* component mobs */
   if (IS_NPC(victim) && victim->followers)
      for (f = victim->followers; f; f = f_next)
         {
         f_next = f->next;
         if (MOB2_FLAGGED(f->follower, MOB2_COMPONENT))
            {
            char_from_room(f->follower);
            char_to_room(f->follower, IN_ROOM(victim));
            }          
         }

   act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
   act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT | TO_SLEEP);
   if (GET_POS(victim) == POS_SLEEPING)
      {
      GET_POS(victim) = POS_STANDING;
      act("$n wakes up and peers around.", TRUE, victim, 0, 0, TO_ROOM);      
      act("You awaken from your slumber.", TRUE, victim, 0, 0, TO_CHAR);
      }
   look_at_room(victim, 0);
   entry_memory_mtrigger(victim);
   greet_mtrigger(victim, -1);
   greet_memory_mtrigger(victim);
   if(iEscapeFromFight==1)
      {
      act("The stress of pulling $N through the ether momentarily stuns you!",
          TRUE,ch,0,victim,TO_CHAR);
      act("The stress being pulled through the ether momentarily stuns you!",
          TRUE,ch,0,victim,TO_VICT);
      SET_BIT(PLR_FLAGS(ch), PLR_STUNNED);
      SET_BIT(PLR_FLAGS(victim), PLR_STUNNED);
      add_function_to_queue(15,ch,0,2,stun_by_spell,ch,1);
      add_function_to_queue(10,victim,0,2,stun_by_spell,victim,1);
      }
   return TRUE;
   }



ASPELL(spell_locate_object)
   {
   struct obj_data *i;
   char *name=get_buffer(MAX_INPUT_LENGTH);
   char *buf = get_buffer(512);

   int j;

   level = MAX(MIN(level,10), 1);

   strcpy(name, CAST_ARG(ch));
   j = level*2;
   log("Locate-Obj: %s cast locate obj on |%s|.",GET_NAME(ch),name);
   for (i = object_list; i && (j > 0); i = i->next)
      {
      if (!isname(name, i->name))
         continue;
      /* cannot locate for body parts on NPCs */
      if (IS_SET(GET_OBJ_EXTRA2(i), ITEM2_BODYPART) &&
          ((i->carried_by && IS_NPC(i->carried_by)) ||
           (i->worn_by && IS_NPC(i->worn_by))))
         continue;
      /* Objs flagged !locate don't show up on locate object. */
      if (IS_SET(GET_OBJ_EXTRA2(i), ITEM2_NOLOCATE)) {
	continue;
      }
      /* odinian 1/3/2000 */
      /* check caster's level v. item's LR */
      if ((GET_OBJ_VAL(i,4) >= LVL_IMMORT)&&(GET_LEVEL(ch) < LVL_IMMORT))
         continue;
      if ((GET_OBJ_VAL(i,4) == LVL_AVATAR)&&(!is_remort_level(ch, DOUBLE_REMORT))&&
              (GET_LEVEL(ch)<LVL_IMMORT))
         continue;
      if ((GET_OBJ_VAL(i,4) == LVL_ANGEL)&&(is_remort_level(ch, NON_REMORT))&&  
              (GET_LEVEL(ch)<LVL_IMMORT))
         continue;
      if ((GET_OBJ_VAL(i,4) > GET_LEVEL(ch))&&(is_remort_level(ch, NON_REMORT)))
         continue;
      /*if (IS_OBJ_STAT(i, ITEM_NEWBIE) && 
               ((GET_LEVEL(ch) > LVL_NEWBIE) || (!is_remort_level(ch, NON_REMORT))))
         continue;*/ /* let us help the newbies :-) */

      strcpy(buf,i->short_description);
      CAP(buf);
      if (i->carried_by)
         {
         if(i->carried_by==ch)
            continue;
         if (!CAN_SEE(ch, i->carried_by))
            continue;
         send_to_char(ch, "%s is being carried by %s.\r\n",
                      buf, GET_NAME(i->carried_by));
         }
      else if (IN_ROOM(i) != NOWHERE)
         {
         if(ROOM_FLAGGED(IN_ROOM(i),ROOM_HOUSE) && 
           !ROOM_FLAGGED(IN_ROOM(i),ROOM_DONATION))
            continue;
         send_to_char(ch, "%s is in %s.\r\n", buf,
                      world[IN_ROOM(i)].name);
         }
      else if (i->in_obj)
         {
         /* if item owner is invisible to you, skip */
         if (item_owner(i) && !IS_NPC(item_owner(i)) &&
                (GET_INVIS_LEV(item_owner(i)) > GET_LEVEL(ch)))
            continue;
         send_to_char(ch, "%s is in %s.\r\n", buf,
                      i->in_obj->short_description);
         }
      else if (i->worn_by)
         {
         if (!CAN_SEE(ch, i->worn_by))
            continue;
         if(i->worn_by==ch)
            continue;
         send_to_char(ch, "%s is being %s by %s.\r\n", buf,
                 (GET_EQ(i->worn_by, WEAR_WIELD_1)==i)||
                 (GET_EQ(i->worn_by, WEAR_WIELD_2)==i)?"wielded":"worn",
                      GET_NAME(i->worn_by));
         }
      else
         send_to_char(ch, "%s's location is uncertain.\r\n", buf);

      j--;
      }
   release_buffer(buf);
   release_buffer(name);
   if (j == level*2)
      {
      send_to_char(ch,"You sense nothing.\r\n");
      return FALSE;
      }
   return TRUE;
   }



ASPELL(spell_charm)
   {
   struct affected_type af;

   if (victim == NULL || ch == NULL)
      return FALSE;
   level = MAX(MIN(level,10), 1);

   if (victim == ch)
      send_to_char(ch,"You like yourself even better!\r\n");
   else if (AFF_FLAGGED(victim, AFF_SANCTUARY))
      send_to_char(ch,"Your victim is protected by sanctuary!\r\n");
   else if (MOB_FLAGGED(victim, MOB_NOCHARM))
      send_to_char(ch,"Your victim resists!\r\n");
   else if (AFF_FLAGGED(ch, AFF_CHARM))
      send_to_char(ch,"You can't have any followers of your own!\r\n");
   else if (AFF_FLAGGED(victim, AFF_CHARM) || level*10 < GET_LEVEL(victim))
      send_to_char(ch,"You fail.\r\n");
   else if (GET_LEVEL(ch) < GET_LEVEL(victim))
      send_to_char(ch,"You fail.\r\n");
   /* player charming another player - no legal reason for this */
   else if (!pk_allowed && !IS_NPC(victim))
      send_to_char(ch,"You fail - shouldn't be doing it anyway.\r\n");
   else if (circle_follow(victim, ch))
      send_to_char(ch,"Sorry, following in circles can not be allowed.\r\n");
   else if (mag_savingthrow(victim, SAVING_PARA,level,0))
      send_to_char(ch,"Your victim resists!\r\n");
   /*   else if (num_charmies(ch) >= (NUM_CHARM_ADJ_CHA(ch, victim) + \
	(GET_LEVEL(ch)/10)))*/
   else if (num_charmies(ch) >= MAX_CHARMIES)
      send_to_char(ch,"You fail.\r\n");
   else if (IS_NPC(victim) && (MOB2_FLAGGED(victim, MOB2_COMPONENT)))
      send_to_char(ch,"The limb is not fooled by your shennanigans.\r\n");
   else if (IS_NPC(victim) && (IS_IMMUNE(victim, IMM_BLUNT) || IS_IMMUNE(victim, IMM_PIERCE) || IS_IMMUNE(victim, IMM_SLASH) || IS_IMMUNE(victim, IMM_PLUS1) || IS_IMMUNE(victim, IMM_PLUS2) || IS_IMMUNE(victim, IMM_PLUS3) || IS_IMMUNE(victim, IMM_PLUS4) || IS_IMMUNE(victim, IMM_NONMAG)))
     send_to_char(ch, "You fail.\r\n");
   else
      {
      if (victim->master)
         stop_follower(victim);

      if(FIGHTING(victim))
         stop_fighting(victim);
      if(FIGHTING(ch)==victim)
         stop_fighting(ch);

      add_follower(victim, ch);

      af.type = SPELL_CHARM;

      if (GET_INT(victim))
         af.duration = 24 * 18 / GET_INT(victim);
      else
         af.duration = 24 * 18;

      af.modifier = 0;
      af.location = 0;
      af.bitvector = AFF_CHARM;
      affect_to_char(victim, &af);

      act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
      if (IS_NPC(victim))
         {
         REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
         REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
         }
      return TRUE;
      }
   return FALSE;
   }


void id_obj_to_char(struct char_data *ch, struct obj_data *obj)
{
  char *buf2=get_buffer(2048);
  int i, found, condition,percent;
  int wcondition,wpercent;
  
  sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
  send_to_char(ch, "Object '%s', Item type: %s\r\n",
	       obj->short_description,buf2);
  
  if(obj->obj_flags.wear_flags)
  {
    sprintbit2(obj->obj_flags.wear_flags, wear_strings,buf2,1);
    send_to_char(ch, "Item can be worn %s.\r\n",buf2);
  }

  if (obj->obj_flags.bitvector)
  {
    sprintbit(obj->obj_flags.bitvector, affected_bits, buf2);
    send_to_char(ch, "Item will give you following abilities:  %s\r\n",
		 buf2);
  }

  if(GET_OBJ_EXTRA(obj) || GET_OBJ_EXTRA2(obj))
  {
    char *buf1 = get_buffer(MAX_STRING_LENGTH);
    sprintf(buf1, "Item is: ");
    if (GET_OBJ_EXTRA(obj)) {
      sprintbit(GET_OBJ_EXTRA(obj), extra_bits_id, buf2);
      strcat(buf1, buf2);
    }
    if (GET_OBJ_EXTRA2(obj)) {
      sprintbit(GET_OBJ_EXTRA2(obj), extra_bits2_id, buf2);
      strcat(buf1, buf2);
    }
    send_to_char(ch, "%s\r\n", buf1);
    release_buffer(buf1);
  }

   if(GET_OBJ_ANTI(obj)!=0)
      {
      sprintbit(GET_OBJ_ANTI(obj), anti_bits_id, buf2);
      send_to_char(ch, "Race/Class Restrict: %s\r\n",buf2);
      }

   send_to_char(ch, "Weight: %d, Value: %d\r\n",
                GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj));
   switch (GET_OBJ_TYPE(obj))
      {
   case ITEM_SCROLL:
   case ITEM_POTION:
   case ITEM_PILL:
     sprintf(buf2,"This %s casts at level %s%d%s: ",
             item_types[(int)GET_OBJ_TYPE(obj)],
             NCYN, GET_OBJ_VAL( obj, 0 ), NNRM );
      if ((GET_OBJ_VAL(obj, 1) >= 1) && (GET_OBJ_VAL(obj,1)<MAX_SPELLS))
         sprintf(buf2+strlen(buf2), " %s",
                 spells[GET_OBJ_VAL(obj, 1)].spell_name);
      if ((GET_OBJ_VAL(obj, 2) >= 1) && (GET_OBJ_VAL(obj,2)<MAX_SPELLS))
         sprintf(buf2+strlen(buf2), " %s",
                 spells[GET_OBJ_VAL(obj, 2)].spell_name);
      if ((GET_OBJ_VAL(obj, 3) >= 1) && (GET_OBJ_VAL(obj,3)<MAX_SPELLS))
         sprintf(buf2+strlen(buf2), " %s",
                 spells[GET_OBJ_VAL(obj, 3)].spell_name);
      send_to_char(ch,"%s\r\n", buf2);
      break;
   case ITEM_WAND:
   case ITEM_STAFF:
      send_to_char(ch, "This %s casts at level %s%ld%s: ",
                   item_types[(int)GET_OBJ_TYPE(obj)],
                   NCYN, GET_OBJ_VAL( obj, 0 ), NNRM );
      if ((GET_OBJ_VAL(obj, 3) >= 1) && (GET_OBJ_VAL(obj,3)<MAX_SPELLS))
         send_to_char(ch, " %s\r\n",
                      spells[GET_OBJ_VAL(obj, 3)].spell_name);
      else
         send_to_char(ch, "nothing, tell a builder\r\n");
      send_to_char(ch,
                   "It has %ld maximum charge%s and %ld remaining.\r\n",
                   GET_OBJ_VAL(obj, 1),
                   GET_OBJ_VAL(obj, 1) == 1 ? "" : "s", GET_OBJ_VAL(obj, 2));
      break;
   case ITEM_WEAPON:
      send_to_char(ch, "Damage Dice is '%ldD%ld'", GET_OBJ_VAL(obj, 1),
                   GET_OBJ_VAL(obj, 2));
      send_to_char(ch,
                   " for an average per-round damage of %.1f.\r\n",
                   (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj,1)));
      switch (obj->obj_flags.value[3])
         {
      case 0  :
         send_to_char(ch, "Weapon Type: Hit\r\n");
         break;
      case 1  :
         send_to_char(ch, "Weapon Type: Bludgeoning\r\n");
         break;
      case 2  :
         send_to_char(ch, "Weapon Type: Piercing\r\n");
         break;
      case 3  :
         send_to_char(ch, "Weapon Type: Slashing\r\n");
         break;
      case 4  :
         send_to_char(ch, "Weapon Type: Blasting\r\n");
         break;
      case 5  :
         send_to_char(ch, "Weapon Type: Whipping\r\n");
         break;
      case 6  :
         send_to_char(ch, "Weapon Type: Piercing\r\n");
         break;
      case 7  :
         send_to_char(ch, "Weapon Type: Clawing\r\n");
         break;
      case 8  :
         send_to_char(ch, "Weapon Type: Biting\r\n");
         break;
      case 9  :
         send_to_char(ch, "Weapon Type: Stinging\r\n");
         break;
      case 10  :
         send_to_char(ch, "Weapon Type: Cleaving\r\n");
         break;
      case 11 :
         send_to_char(ch, "Weapon Type: Pounding\r\n");
         break;
      case 12 :
         send_to_char(ch, "Weapon Type: Mauling\r\n");
         break;
      case 13 :
         send_to_char(ch, "Weapon Type: Thrashing\r\n");
         break;
      case 14 :
         send_to_char(ch, "Weapon Type: Punching\r\n");
         break;
      case 15 :
         send_to_char(ch, "Weapon Type: Stabbing\r\n");
         break;
      default :
         send_to_char(ch, "Weapon Type: Unknown\r\n");
         break;
         }
      break;
   case ITEM_THROW:
   case ITEM_ROCK:
   case ITEM_BOLT:
   case ITEM_ARROW:
      send_to_char(ch, "Damage Dice is '%ldD%ld'", GET_OBJ_VAL(obj, 1),
                   GET_OBJ_VAL(obj, 2));
      send_to_char(ch,
                   " for an average per-hit damage of %.1f.\r\n",
                   (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj,1)));
      break;
   case ITEM_ARMOR:
      send_to_char(ch, "AC-apply is %ld\r\n", GET_OBJ_VAL(obj, 0));
      break;
   case ITEM_LIGHT:
      if(GET_OBJ_VAL(obj,2)==-1)
         send_to_char(ch, "This light will burn forever.\r\n");
      else if(GET_OBJ_VAL(obj,0)==0)
         send_to_char(ch, "Will burn for %ld hours.\r\n",
                      GET_OBJ_VAL(obj,2));
      else
         send_to_char(ch,
                      "Will burn %s for the next %ld hours, and can "
                      "hold %ld hours of %s.\r\nIt %s be refueled.\r\n",
                      fuels[GET_OBJ_VAL(obj,1)],GET_OBJ_VAL(obj,2),
                      GET_OBJ_VAL(obj,3),fuels[GET_OBJ_VAL(obj,1)],
                      GET_OBJ_VAL(obj,0)?"can":"can't");
      break;
   case ITEM_FUEL:
      send_to_char(ch, "Holds %ld hours of %s.\r\n",GET_OBJ_VAL(obj,0),
                   fuels[GET_OBJ_VAL(obj,1)]);
      break;
   default:
      break;
      }

   if ((obj->obj_flags.value[4] > 1) || (GET_OBJ_EXTRA2(obj) & 3))
      {
      sprintbit((GET_OBJ_EXTRA2(obj) & 3), extra_bits2_id, buf2);
      send_to_char(ch, "Level Restriction: %s%ld\r\n",
                   !strcmp(buf2, "NOBITS ")?"":buf2,
                    obj->obj_flags.value[4]);
      }
   else
      {
      send_to_char(ch, "Level Restriction: None\r\n");
      }
   found = FALSE;
   for (i = 0; i < MAX_OBJ_AFFECT; i++)
      {
      if ((obj->affected[i].location != APPLY_NONE) &&
              (obj->affected[i].modifier != 0))
         {
         if (!found)
            {
            send_to_char(ch,"Can affect you as :\r\n");
            found = TRUE;
            }
         if(obj->affected[i].location==APPLY_IMMUNE)
            {
            sprintbit(obj->affected[i].modifier,immunity_names,buf2);
            send_to_char(ch,"   Makes Immune to %s%s%s\r\n",
                         NCYN,buf2,NNRM);
            }
         else if(obj->affected[i].location==APPLY_RESIST)
            {
            sprintbit(obj->affected[i].modifier,immunity_names,buf2);
            send_to_char(ch,"   Makes Resistant to %s%s%s\r\n",
                         NCYN,buf2,NNRM);
            }
         else if(obj->affected[i].location==APPLY_SUSC)
            {
            sprintbit(obj->affected[i].modifier,immunity_names,buf2);
            send_to_char(ch,"   Makes Susceptable to %s%s%s\r\n",
                         NCYN,buf2,NNRM);
            }
         else
            {
            sprinttype(obj->affected[i].location, apply_types, buf2);
            send_to_char(ch,"   Affects: %s By %ld\r\n", buf2,
                         obj->affected[i].modifier);
            }
         }
      }

   if(GET_OBJ_TSLOTS(obj) == 0)
      percent = 0;
   else
      /* Base equipment status off original, not total - Nomi 5/10/25 */
      percent = (GET_OBJ_CSLOTS(obj) * 100) / GET_OBJ_OSLOTS(obj); 

   if ((GET_OBJ_CSLOTS(obj) == 0) && (GET_OBJ_TSLOTS(obj) == 0))
      condition = 0;
   else if (GET_OBJ_CSLOTS(obj) < 0)
      /* Added a broken status at position 1 - Nomi 5/10/25 */
      condition = 1;
   else
      /* Position 2 is the lowest before it breaks */
      condition = (percent / 10) + 2;

   if(GET_OBJ_OSLOTS(obj) == 0)
      wpercent = 0;
   else
      wpercent = (GET_OBJ_TSLOTS(obj) * 100) / GET_OBJ_OSLOTS(obj);

   if ((GET_OBJ_TSLOTS(obj) == 0) && (GET_OBJ_OSLOTS(obj) == 0))
      wcondition = 0;
   else
      wcondition = (wpercent / 10) + 1;

   if( GET_OBJ_TYPE( obj ) == ITEM_WEAPON )
   {
     int j;
     for ( j = 0; j < MAX_SPELL_AFFECT; j++ )
     {
       if( obj->spell_affect[ j ].spelltype > 0 )
       {
         send_to_char( ch, "Casts: %s%s%s level %d at %d%%\r\n",
                       NCYN, skill_name( obj->spell_affect[ j ].spelltype ), NNRM,
                       obj->spell_affect[ j ].level,
                       obj->spell_affect[ j ].percentage );
       }
     }
   }

   if(GET_LEVEL(ch) >= LVL_IMMORT)
      {
      send_to_char(ch,"Made out of: %s\r\n"
                   "Quality: %d(cur)/%d(tot)/%d(orig)\r\n"
                   "Condition: %d percent, %s\r\n"
                   "Wear: %d percent, %s\r\n"
                   "Vnum: %ld\r\n",
                   material_types_lower[obj->material],
                   GET_OBJ_CSLOTS(obj),
                   GET_OBJ_TSLOTS(obj),
                   GET_OBJ_OSLOTS(obj),
                   percent,
                   item_condition[condition],
                   wpercent,
                   item_wear[wcondition],
                   GET_OBJ_VNUM(obj));
      }
   else
      {
      send_to_char(ch,"Made out of: %s\r\n"
                   "Quality: %s\r\n"
                   "Wear: %s\r\n",
                   material_types_lower[obj->material],
                   item_condition[condition],
                   item_wear[wcondition]);
      }
   release_buffer(buf2);
   }

void id_char_to_char(struct char_data *ch, struct char_data *victim)
   {
   send_to_char(ch, "Name: %s\r\n", GET_NAME(victim));
   if (!IS_NPC(victim))
      {
      send_to_char(ch,
                   "%s is %d years, %d months, %d days and %d hours old\r\n",
                   GET_NAME(victim), age(victim)->year, age(victim)->month,
                   age(victim)->day, age(victim)->hours);
      }
   send_to_char(ch, "Height: %d feet %d inches, Weight: %d pounds\r\n",
                GET_HEIGHT(victim)/12, GET_HEIGHT(victim)%12, GET_WEIGHT(victim));
   send_to_char(ch, "Level: %d, Hits: %d, Mana: %d\r\n",
                GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
   send_to_char(ch, "AC: %s %s, Hitroll: %s, Damroll: %s\r\n",
                ch==victim?"You are":"They are",
                acstr(compute_armor_class(victim)),
                bonstr(GET_HITROLL(victim)),
                bonstr(GET_DAMROLL(victim)));
   send_to_char(ch, "Str: %s, Int: %s, Wis: %s, Dex: %s, Con: %s, Cha: %s\r\n",
                str_strings[MIN(9,(GET_STR(victim)/3))],
                int_strings[MIN(9,(GET_INT(victim)/3))],
                wis_strings[MIN(9,(GET_WIS(victim)/3))],
                dex_strings[MIN(9,(GET_DEX(victim)/3))],
                con_strings[MIN(9,(GET_CON(victim)/3))],
                cha_strings[MIN(9,(GET_CHA(victim)/3))]);
   }

ASPELL(spell_identify)
   {
   level = MAX(MIN(level,10), 1);

   if (obj)
      {
      send_to_char(ch,"You feel informed:\r\n");

      id_obj_to_char(ch,obj);
      }
   else if (victim)
      {
      id_char_to_char(ch,victim);
      }
   return TRUE;
   }



ASPELL(spell_enchant_weapon)
   {
   int i;

   if (ch == NULL || obj == NULL)
      return FALSE;
   level = MAX(MIN(level,10), 1);
   level = MIN(level,(GET_LEVEL(ch)/10));

   if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || OBJ_FLAGGED(obj, ITEM_MAGIC))
      {
      send_to_char(ch,"This weapon can't be enchanted!\n\r");
      return FALSE;
      }

   for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
         return FALSE;

   SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
   SET_BIT(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

   obj->affected[0].location = APPLY_HITROLL;
   obj->affected[0].modifier = (level+1)/2;
   if(obj->affected[0].modifier>=3)
      obj->affected[0].modifier--;
   obj->affected[1].location = APPLY_DAMROLL;
   obj->affected[1].modifier = (level)/2;
   if(obj->affected[1].modifier>=2)
      obj->affected[1].modifier--;

   if (IS_GOOD(ch))
      {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
      act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
      }
   else if (IS_EVIL(ch))
      {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
      act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
      }
   else
      {
      act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
      }
   return TRUE;
   }



ASPELL(spell_enchant_armor)
   {
   int i;

   if (ch == NULL || obj == NULL)
      return FALSE;
   level = MAX(MIN(level,10), 1);
   level = MIN(level,(GET_LEVEL(ch)/10));

   if (GET_OBJ_TYPE(obj) != ITEM_ARMOR || OBJ_FLAGGED(obj, ITEM_MAGIC))
      {
      send_to_char(ch,"This armor can't be enchanted!\n\r");
      return FALSE;
      }

   for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
         {
         send_to_char(ch,"This armor can't be enchanted!\n\r");
         return FALSE;
         }

   SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
   SET_BIT(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

   obj->affected[0].location = APPLY_AC;

   obj->affected[0].modifier = 0;
   if (level >= 1)
      obj->affected[0].modifier--; /* -1 */
   if (level >= 2)
      obj->affected[0].modifier--; /* -2 */
   if (level >= 3)
      obj->affected[0].modifier--; /* -3 */
   if (level >= 5)
      obj->affected[0].modifier--; /* -4 */
   if (level >= 7)
      obj->affected[0].modifier--; /* -5 */
   if (level >= 9)
      obj->affected[0].modifier--; /* -6 */
   if (level >= 10)
      obj->affected[0].modifier--; /* -7 */

   act("$n enchants $p.", FALSE, ch, obj, 0, TO_ROOM);

   if (IS_GOOD(ch))
      {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
      act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
      }
   else if (IS_EVIL(ch))
      {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
      act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
      }
   else
      {
      act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
      }
   return TRUE;
   }


ASPELL(spell_detect_poison)
   {
   level = MAX(MIN(level,10), 1);
   if (victim)
      {
      if (victim == ch)
         {
         if (AFF_FLAGGED(victim, AFF_POISON))
            send_to_char(ch,"You can sense poison in your blood.\r\n");
         else
            send_to_char(ch,"You feel healthy.\r\n");
         }
      else
         {
         if (AFF_FLAGGED(victim, AFF_POISON))
            act("You sense that $E is poisoned.", FALSE, ch,0,victim,TO_CHAR);
         else
            act("You sense that $E is healthy.", FALSE, ch, 0, victim,TO_CHAR);
         }
      }

   if (obj)
      {
      switch (GET_OBJ_TYPE(obj))
         {
      case ITEM_DRINKCON:
      case ITEM_FOUNTAIN:
      case ITEM_FOOD:
         if (GET_OBJ_VAL(obj, 3))
            act("You sense that $p has been contaminated.",FALSE,ch,obj,0,TO_CHAR);
         else
            act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0,
                TO_CHAR);
         break;
      default:
         send_to_char(ch,"You sense that it should not be consumed.\r\n");
         }
      }
   return TRUE;
   }


ASPELL(spell_knock)
   {
   level = MAX(MIN(level,10), 1);
   if (dr == NULL)
      {
      send_to_char(ch,"Door Null. ERROR!!\r\n");
      }
   else
      {
      if (IS_SET(dr->exit_info, EX_WIZLOCK))
         {
         if (level >= dr->lcklevl)
            {
            act("$n shatters the magic lock, with a powerful blow.", TRUE,
                ch, 0, 0, TO_ROOM);
            send_to_char(ch,"Your knock shatters the magic lock.\r\n");
            TOGGLE_BIT((dr->exit_info), EX_WIZLOCK);
            dr->lcklevl = 0;
            if(dr2!=NULL)
               {
               TOGGLE_BIT((dr2->exit_info), EX_WIZLOCK);
               dr2->lcklevl = 0;
               }
            return TRUE;
            }
         else
            {
            act("$n smashes at the magic lock, but it holds.", TRUE, ch,
                0, 0, TO_ROOM);
            send_to_char(ch,"You smash at the magic lock, but it holds.\r\n");
            return FALSE;
            }
         }


      if (IS_SET(dr->exit_info, EX_CLOSED))
         {
         if (IS_SET(dr->exit_info, EX_LOCKED))
            {
            if (IS_SET(dr->exit_info, EX_PICKPROOF)&&
                    ((level<9)||(number(1,15)>level)))
               {
               act("$n smashes at the lock, but it holds.", TRUE, ch,
                   0, 0, TO_ROOM);
               send_to_char(ch,"You smash at the lock, but it holds.\r\n");
               return FALSE;
               }
            else
               {
               REMOVE_BIT((dr->exit_info), EX_LOCKED);
               if(dr2!=NULL)
                  REMOVE_BIT((dr2->exit_info), EX_LOCKED);
               act("$n defeats the lock with a minor cantrip.", TRUE,
                   ch, 0, 0, TO_ROOM);
               send_to_char(ch,"The lock yields to your magic.\r\n");
               return TRUE;
               }
            }
         else
            send_to_char(ch,"That door is not locked.\r\n");
         }
      else
         send_to_char(ch,"There is an open passage already in that"
                      " direction.\r\n");
      }
   return FALSE;
   }

ASPELL(spell_wizardlock)
   {
   if (dr == NULL)
      send_to_char(ch,"Door Null. ERROR!!\r\n");
   else
      {
      if (IS_SET(dr->exit_info, EX_ISDOOR))
         {
         if ((dr->lcklevl > level) || (dr2 && dr2->lcklevl > level))
             {
             send_to_char(ch, "A wizard better than you'll ever be "
                          "wizlocked it before you!\r\n");
             return FALSE;
             }
         if (!IS_SET(dr->exit_info, EX_CLOSED))
            TOGGLE_BIT((dr->exit_info), EX_CLOSED);
         act("$n places a magical padlock around the door.", TRUE,
             ch, 0, 0, TO_ROOM);
         send_to_char(ch,"You place a glowing magical padlock around"
                      " the door.\r\n");
         if (!IS_SET(dr->exit_info, EX_WIZLOCK))
            TOGGLE_BIT((dr->exit_info), EX_WIZLOCK);
         dr->lcklevl = level;
         if (dr2&&IS_SET(dr2->exit_info, EX_ISDOOR))
            {
            if (!IS_SET(dr2->exit_info, EX_CLOSED))
               TOGGLE_BIT((dr2->exit_info), EX_CLOSED);
            if (!IS_SET(dr2->exit_info, EX_WIZLOCK))
               TOGGLE_BIT((dr2->exit_info), EX_WIZLOCK);
            dr2->lcklevl = level;
            }
         return TRUE;
         }
      else
         send_to_char(ch,"That Exit has no door to lock.\r\n");
      }
   return FALSE;
   }

void stun_by_spell(struct char_data *ch,int iPhase)
   {
   if(iPhase==1)
      {
      send_to_char(ch,"You become more aware of your surroundings, but then you are hit by a wave of darkness!\r\n");
      do_action(ch,"",cmd_faint,0);
      add_function_to_queue(10,ch,0,2,stun_by_spell,ch,0);
      }
   else if(iPhase==0)
      {
      send_to_char(ch,"You start to feel better.\r\n");
      act("$n starts to look better.",TRUE,ch,0,0,TO_ROOM);
      REMOVE_BIT(PLR_FLAGS(ch), PLR_STUNNED);
      }
   }
ASPELL(spell_energy_drain)
   {
   level = MAX(MIN(level,10), 1);

   if (IS_NPC(victim) || (ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL) || Z_FLAGGED(IN_ROOM(ch),Z_PKILL)))
     {
     if (GET_MANA(victim) <= (level*10) && (GET_MANA(victim) + GET_MANA(ch) <= GET_MAX_MANA(ch))) 
       {
       GET_MANA(ch) += GET_MANA(victim);
       GET_MANA(victim) = 0;
       send_to_char(ch, "You drain some of %s's energy!\r\n",GET_NAME(victim));
       send_to_char(victim, "%s drains some of your energy!\r\n",GET_NAME(ch));
       }
     else if (GET_MANA(victim) >= (level*10) && ((level*10) + GET_MANA(ch) <= GET_MAX_MANA(ch)))
       {
       GET_MANA(ch) += (level*10);
       GET_MANA(victim) -= (level*10);
       send_to_char(ch, "You drain some of %s's energy!\r\n",GET_NAME(victim));
       send_to_char(victim, "%s drains some of your energy!\r\n",GET_NAME(ch));
       }
     else if (GET_MANA(victim) <= (level*10) && (GET_MANA(ch) + GET_MANA(victim) > GET_MAX_MANA(ch)))
       {
       GET_MANA(ch) = GET_MAX_MANA(ch);
       GET_MANA(victim) = 0;
       send_to_char(ch, "You drain some of %s's energy!\r\n",GET_NAME(victim));
       send_to_char(victim, "%s drains some of your energy!\r\n",GET_NAME(ch));
       }
     else if (GET_MANA(victim) >= (level*10) && ((level*10) + GET_MANA(ch) > GET_MAX_MANA(ch)))
       {
       GET_MANA(ch) = GET_MAX_MANA(ch);
       GET_MANA(victim) -= (level*10);
       send_to_char(ch, "You drain some of %s's energy!\r\n",GET_NAME(victim));
       send_to_char(victim, "%s drains some of your energy!\r\n",GET_NAME(ch));
       }
     return TRUE;
     }
   else
     {
     send_to_char(ch, "You must be in a battle room to cast this on another player.\r\n");
     return FALSE;
     }
   }
