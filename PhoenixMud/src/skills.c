/***************************************************************************
 *  File: skills.c                                      Part of PhoenixMud *
 *  Usage: ACMD's and utility functions for skills.  The old method of     *
 *         putting all these functions in separate files was hard to       *
 *         maintain and distribute to other coders.                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *  AAM Jul 98                                                             *
 *                                                                         *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 *  PhoenixMUD is based on CircleMUD, Copyright (C) 1996-98.               *
 ***************************************************************************/

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"


#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "queue.h"
#include "dg_scripts.h"
#include "vnum.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct zone_data *zone_table;              /*. db.c .*/
extern int top_of_zone_table;                     /*. db.c .*/
extern int pk_allowed;
extern int pt_allowed;
extern struct index_data *mob_index;
extern struct index_data *obj_index;              /*. db.c .*/ 
extern int min_level(struct char_data *ch,int spellnum);

SPECIAL(shop_keeper);
/* extern functions */
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
int  skill_roll(struct char_data *ch, int skill_num, int penalty);
void get_check_money(struct char_data * ch, struct obj_data * obj, int subcmd);
void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);
void process_skills(struct char_data *ch, struct char_data *tch, struct obj_data *item,
                    int t_alt, int skill, int state);
void perform_wear(struct char_data * ch, struct obj_data * obj, int where_pos, int visible);
ACMD(do_show_queue) ;

/*
 * Ambush skill code
 */
ACMD(do_ambush)
   {
   struct char_data *vict;
   char *arg;


   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   if (!(vict = get_char_vis(ch, arg,FIND_CHAR_ROOM)))
      {
      send_to_char(ch,"Bushwhack who?\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"How can you sneak up on yourself?\r\n");
      return;
      }

   if (!GET_EQ(ch,WEAR_WIELD_1))
      {
      send_to_char(ch,"You need to wield a weapon to make it a success.\r\n");
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   WAIT_STATE(ch, SKILL_LAG * 2);

   if((MOB_FLAGGED(vict, MOB_AWARE))||
           (!IS_NPC(ch)&& (!SCR_SKILLCHECK(ch, SKILL_AMBUSH) || GET_SKILL(ch, SKILL_AMBUSH) <= 0)))
      {
      act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
      act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
      act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
      hit(vict, ch, TYPE_UNDEFINED);
      return;
      }
   else if (AFF2_FLAGGED(vict, AFF2_WARY) && (number(1, 3) > 1))
      {
      act("You are wary of the ambush from $n and attack!",
          FALSE, ch, 0, vict, TO_VICT);  
      act("$N notices $n trying to ambush $M and gets in the first attack!",
          FALSE, ch, 0, vict, TO_NOTVICT);
      act("$N is wary, and notices your attempted ambush!",
          FALSE, ch, 0, vict, TO_CHAR);
      hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   LAST_HAND_USED(ch)=0;
   if (AWAKE(vict) && !skill_roll(ch,SKILL_AMBUSH,5*dex_app[stat_index(GET_DEX(ch))].reaction))
      {
      damage(ch, vict, 0, SKILL_AMBUSH,IMM_NONMAG);
      }
   else
      {
      if (IS_NPC(vict))
         SET_BIT(AFF2_FLAGS(vict), AFF2_WARY);
      hit(ch, vict, SKILL_AMBUSH);
      }
   }

/*
 * Backstab skill code
 */
ACMD(do_backstab)
   {
   struct char_data *vict;
   char *arg;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);
   release_buffer(arg);
   if (vict==NULL)
      {
      send_to_char(ch,"Backstab who?\r\n");
      return;
      }
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"How can you sneak up on yourself?\r\n");
      return;
      }
   if (!GET_EQ(ch, WEAR_WIELD_1))
      {
      send_to_char(ch,"You need to wield a weapon to make it a success.\r\n");
      return;
      }
   if (sharp[GET_OBJ_VAL(GET_EQ(ch,WEAR_WIELD_1),3)]!=1)
      {
      send_to_char(ch,"Only piercing weapons in your primary hand can be used for backstabbing.\r\n");
      return;
      }
   if (FIGHTING(vict)&&!AFF_FLAGGED(ch,AFF_SNEAK))
      {
      send_to_char(ch,"You can't backstab a fighting person -- they're too alert!\r\n");
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   WAIT_STATE(ch, SKILL_LAG * 2);

   if ((MOB_FLAGGED(vict, MOB_AWARE)) ||
       (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_BACKSTAB)||GET_SKILL(ch,SKILL_BACKSTAB)<=0)))
      {
      act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
      act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
      act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
      hit(vict, ch, TYPE_UNDEFINED);
      return;
      }
   else if (AFF2_FLAGGED(vict, AFF2_WARY) && (number(1, 3) > 1))
      {
      act("You are wary of the backstab from $n and attack!",
          FALSE, ch, 0, vict, TO_VICT);
      act("$N notices $n trying to backstab $M and gets in the first attack!", 
          FALSE, ch, 0, vict, TO_NOTVICT);
      act("$N is wary, and notices your attempted backstab!",
          FALSE, ch, 0, vict, TO_CHAR);
      hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   LAST_HAND_USED(ch)=0;

   if (AWAKE(vict) && !skill_roll(ch,SKILL_BACKSTAB,5*dex_app[stat_index(GET_DEX(ch))].reaction))
      {
      damage(ch, vict, 0, SKILL_BACKSTAB,IMM_PIERCE);
      }
   else
      {
      if (IS_NPC(vict))
         SET_BIT(AFF2_FLAGS(vict), AFF2_WARY);
      hit(ch, vict, SKILL_BACKSTAB);
      }
   }


/*
 * Bandage skill code
 */
ACMD(do_bandage)
   {
   struct char_data *vict;
   int band_self = FALSE;
   char *arg;


   if (ch == NULL)
      return;
   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(FIGHTING(ch))
      {
      send_to_char(ch,"You are too busy to bandage.\n\r");
      return;
      }
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_BANDAGE)||GET_SKILL(ch, SKILL_BANDAGE) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }

   arg=get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);
   /* if the argument is blank - apply bandage to self */
   if (!*arg)
      band_self = TRUE;
   else if (!(vict = get_char_vis(ch, arg,FIND_CHAR_ROOM)))
      {
      send_to_char(ch,"Whom do you wish to heal???\n\r");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   /* if argument corresponds to the player attempting the bandage */
   if (ch == vict)
      band_self = TRUE;

   if (band_self)
      vict = ch;

   if(GET_POS(vict)>POS_SITTING)
      {
      if (!band_self)
         {
         act("$N must not be standing or fighting for this to work.",TRUE,ch,
             0,vict,TO_CHAR);
         act("$n tries to bandage your wounds, but you are moving around "
             "too much.",TRUE,ch,0,vict,TO_VICT);
         return;
         }
      else
         {
         send_to_char(ch,"You must not be standing or fighting for "
                      "this to work.\n\r");
         return;
         }
      }

   if(GET_POS(vict)==POS_BANDAGE)
      {
      send_to_char(ch,"You add more bandages.\r\n");
      return;
      }

   if (!skill_roll(ch,SKILL_BANDAGE,0))
      {
      if (!band_self)
         {
         send_to_char(ch,"You don't seem to be able to wrap the bandages "
                      "correctly.\n\r");
         act("$n tries to bandage you, but fumbles the wrappings.",FALSE,
             ch,0,vict,TO_VICT);
         act("$n tries to bandage $N, but just fumbles around.",FALSE,ch,
             0,vict,TO_NOTVICT);
         }
      else
         {
         send_to_char(ch,"You don't seem to be able to wrap the bandages "
                      "correctly.\n\r");
         act("$n tries to bandage $mself, but just fumbles around.",FALSE,
             ch,0,vict,TO_NOTVICT);
         }
      }
   else
      {
      if (!band_self)
         {
         act("You bandage $N's wounds.",FALSE,ch,0,vict,TO_CHAR);
         act("$n bandages your injuries, stay still for a while.",FALSE,ch,
             0,vict,TO_VICT);
         act("$n bandages $N's wounds and says 'Lie still for a bit'.",
             FALSE,ch,0,vict,TO_NOTVICT);
         GET_POS(vict)=POS_BANDAGE;
         }
      else
         {
         send_to_char(ch,"You bandage your own wounds.\n\r");
         act("$n bandages $s wounds.",FALSE,ch,0,vict,TO_NOTVICT);
         GET_POS(vict)=POS_BANDAGE;
         }
      }
   }


/*
 * Bash skill code
 */
ACMD(do_bash)
   {
   struct char_data *vict;
   char *arg;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Bash who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if(IN_ROOM(ch)!=IN_ROOM(vict))
      return;
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"Aren't we funny today...\r\n");
      return;
      }

   if (!GET_EQ(ch, WEAR_SHIELD))
      {
      send_to_char(ch,"You need to hold a shield to make it a success.\r\n");
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   if ((IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOBASH)) ||
       (!IS_NPC(ch)&& (!SCR_SKILLCHECK(ch, SKILL_BASH) || GET_SKILL(ch, SKILL_BASH)<=0)))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   WAIT_STATE(ch, SKILL_LAG * 3);
   if (!skill_roll(ch,SKILL_BASH,0))
      {
      damage(ch, vict, 0, SKILL_BASH,IMM_BLUNT);
      GET_POS(ch) = POS_SITTING;
      hit(vict,ch,TYPE_UNDEFINED);
      }
   else
      {
      damage(ch, vict, GET_LEVEL(ch)*2, SKILL_BASH,IMM_BLUNT);
      }
   }


/*
 * Berserk skill code
 */
ACMD(do_berserk)
   {
   struct char_data *tmp_vict,*next_vict;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_BERSERK) ||GET_SKILL(ch, SKILL_BERSERK) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }


   if (GET_MOVE(ch) < 20)
      {
      send_to_char(ch,"You are too exhausted!\r\n");
      return;
      }
   GET_MOVE(ch) -= 20;
   
   if(skill_roll(ch,SKILL_BERSERK,0))
      {
      send_to_char(ch,"You go BERSERK!!!\r\n");
      act("$n goes BERSERK!!!", FALSE, ch, 0, 0, TO_NOTVICT);

      for(tmp_vict=world[IN_ROOM(ch)].people;tmp_vict;tmp_vict=next_vict)
         {
         next_vict=tmp_vict->next_in_room;
	 if (!IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT && !MORT_CAN_SEE(ch, tmp_vict)) {
	   /* If you can't see the victim, do a skill roll to see if you hit anyway (flailing blindly will get you lucky sometimes. */
	   if (number(0, 101) > GET_SKILL(ch, SKILL_BERSERK)/2) {
	     continue;
	   }
	 }
         /* only PC's can't berserk each other ... arena? */
         if ((IS_NPC(tmp_vict)&&!IS_NPC(ch)) ||
                 (!IS_NPC(tmp_vict)&&IS_NPC(ch)) ||
                 (IS_NPC(tmp_vict)&&IS_NPC(ch)))
            {
            if (tmp_vict->master!=ch && tmp_vict!=ch)
               {
               hit(ch, tmp_vict, TYPE_UNDEFINED);
               }
            }
         }
      }
   else
      {
      send_to_char(ch,"You attempt to berserk but fail miserably.\r\n");
      act("$n tries to hit everyone in the room and MISSES!!!", FALSE, ch, 0,
          0, TO_NOTVICT);
      }
   WAIT_STATE(ch, SKILL_LAG * 1 );
   }

/*
 * Camouflage skill code
 */
ACMD(do_camouflage)
   {
   int penalty=0;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      {
      send_to_char(ch,"You try to hide from your master but fail.\r\n");
      return;
      }
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_CAMOUFLAGE)||GET_SKILL(ch, SKILL_CAMOUFLAGE) <= 0))
      {
      send_to_char(ch,"You try to camouflage yourself, but find you "
                      "haven't a clue where to start!\r\n");
      return;
      }

   send_to_char(ch,"You attempt to camouflage yourself.\r\n");

   if (AFF_FLAGGED(ch, AFF_HIDE))
      REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
   switch(SECT(IN_ROOM(ch)))
      {
   case SECT_INSIDE:
   case SECT_FLYING:
      return;
      break;
   case SECT_CITY:
      penalty += 50;
      break;
   case SECT_FIELD:
      penalty = 0;
      break;
   case SECT_FOREST:
      penalty -= 20;
      break;
   case SECT_HILLS:
      penalty -= 20;
      break;
   case SECT_MOUNTAIN:
      penalty -= 10;
      break;
   case SECT_WATER_SWIM:
      penalty -= 20;
      break;
   case SECT_WATER_NOSWIM:
      penalty += 30;
      break;
   case SECT_UNDERWATER:
      penalty += 10;
      break;
   case SECT_DESERT:
      penalty = 0;
      break;
   case SECT_SWAMP:
      penalty -= 20;
      break;
   case SECT_TUNDRA:
      penalty = 0;
      break;
   case SECT_JUNGLE:
      penalty -= 20;
      break;
   default:
      break;
      }


   penalty -= dex_app_skill[stat_index(GET_DEX(ch))].hide;
   if (!skill_roll(ch,SKILL_CAMOUFLAGE,penalty))
      return;

   SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
   }

/*
 * Chant skill code
 */
ACMD(do_chant)
   {
   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_CHANT)||GET_SKILL(ch, SKILL_CHANT) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }
   if(GET_POS(ch)==POS_CHANT)
      {
      send_to_char(ch,"You are already chanting twerp!\r\n");
      return;
      }
   if(FIGHTING(ch))
      {
      send_to_char(ch,"But you are fighting!\r\n");
      return;
      }

   if (!skill_roll(ch,SKILL_CHANT,0))
      {
      act("You don't have the mental capacity to chant.", FALSE, ch,
          0, 0, TO_CHAR);
      return;
      }

   switch (GET_POS(ch))
      {
   case POS_STANDING :
      act("You sit down and begin chanting.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sits down and begins reciting 'Oh Wa Ta Jer Ki Am' over "
          "and over again.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_CHANT;
      break;
   case POS_SITTING :
      act("You begin chanting.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n begins chanting 'Oh Wa Ta Jer Ki Am'.", TRUE, ch,
          0, 0, TO_ROOM);
      GET_POS(ch) = POS_CHANT;
      break;
   case POS_MEDITATE       :
      act("You stop meditating, and start chanting.", FALSE, ch,
          0, 0, TO_CHAR);
      act("$n begins chanting 'Oh Wa Ta Jer Ki Am.'", TRUE, ch,
          0, 0, TO_ROOM);
      GET_POS(ch) = POS_CHANT;
      break;
   case POS_CHANT  :
      act("You are already chanting", FALSE, ch, 0, 0, TO_CHAR);
      break;
   case POS_BANDAGE        :
      act("You take off your bandages, and start chanting.", FALSE, ch,
          0, 0, TO_CHAR);
      act("$n begins chanting 'Oh Wa Ta Jer Ki Am'.", TRUE, ch,
          0, 0, TO_ROOM);
      GET_POS(ch) = POS_CHANT;
      break;
   case POS_RESTING :
      act("You sit up and begin chanting.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n begins chanting 'Oh Wa Ta Jer Ki Am'.",TRUE,ch,0,0,TO_ROOM);
      GET_POS(ch) = POS_CHANT;
      break;
   case POS_SLEEPING :
      act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
      break;
   case POS_FIGHTING :
      act("Chant while fighting?  Are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
      break;
   default :
      act("You stop floating around, and begin chanting.",
          FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops floating around, and chants 'Oh Wa Ta Jer Ki Am'.",
          FALSE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_CHANT;
      break;
      }
   }


/*
 * Circle skill code
 */
ACMD(do_circle)
   {
   struct char_data *vict;
   char *arg;


   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Circle who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_CIRCLE)||GET_SKILL(ch, SKILL_CIRCLE) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"How can you circle yourself?!!\r\n");
      return;
      }
   if (!GET_EQ(ch,WEAR_WIELD_1))
      {
      send_to_char(ch,"Do you think you can use your bare hands to circle??\r\n");
      return;
      }

   if (sharp[GET_OBJ_VAL(GET_EQ(ch,WEAR_WIELD_1),3)]!=1)
      {
      send_to_char(ch,"Only piercing weapons can be used for circling.\r\n");
      return;
      }

   if(FIGHTING(vict)==ch)
      {
      send_to_char(ch,"Your opponent is too alert for you to sneak "
                   "behind him!!!\r\n");
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))   
      return;
   
   WAIT_STATE(ch, SKILL_LAG * 2);
   if (FIGHTING(vict) && !skill_roll(ch,SKILL_CIRCLE,dex_app[stat_index(GET_DEX(ch))].reaction))
      {
      damage(ch, vict, 0, SKILL_CIRCLE,IMM_PIERCE);
      }
   else
      {
      hit(ch, vict, SKILL_CIRCLE);
      }
   }



/*
 * Darken skill code
 */
ACMD(do_darken)
   {
   struct room_affected_type af;
   struct room_data *rm;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_DARKEN)||GET_SKILL(ch, SKILL_DARKEN) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }

   if (GET_MANA(ch) < 20)
      {
      send_to_char(ch,"You do not have the energy to darken.\n\r");
      return;
      }

   if (!skill_roll(ch,SKILL_DARKEN,0))
      {
      send_to_char(ch,"You fail to darken the room.\n\r");
      GET_WAIT_STATE(ch) = 1 RL_SEC;
      return;
      }


   GET_MANA(ch) = GET_MANA(ch) - 20;

   rm=&world[IN_ROOM(ch)];
   if(rm->affected)
      {
      affect_from_room(rm,SKILL_DARKEN);
      }
   if(rm->affected)
      {
      affect_from_room(rm,SKILL_LIGHTEN);
      }

   af.bitvector = 0;
   af.type = SKILL_DARKEN;
   af.duration =(int)(GET_LEVEL(ch)/10);
   af.modifier = (-1*VALUE_LIGHT);
   af.location = APPLY_LIGHT;
   send_to_char(ch,"The room darkens.\n\r");
   affect_join_room(rm, &af, FALSE, FALSE, FALSE, FALSE);
   return;
   }



/*
 * dig skill code
 */
void dig_update(struct char_data *pxChar,int iState)
   {
   int i;
   struct obj_data *pxDirt;

   if(!AFF2_FLAGGED(pxChar,AFF2_DIGGING))
      return;
   if(iState==0)
      {
      send_to_char(pxChar,"You start to make a dent into the ground.\r\n");
      act("$n starts making a little hole.", FALSE, pxChar, 0, 0, TO_ROOM);
      }
   else
      {
      REMOVE_BIT(AFF2_FLAGS(pxChar), AFF2_DIGGING);
      send_to_char(pxChar,"Hey, You got something!\r\n");
      if (!skill_roll(pxChar,SKILL_DIG,0))
         {
         if (!(pxDirt = read_object(MINE_DIRT, VIRTUAL)))
            {
            send_to_char(pxChar,"Dirt object not found.\r\n");
            log("SYSERR: MINE_DIRT not found %d", MINE_DIRT);
            return;
            }
         obj_to_char(pxDirt, pxChar);
         act("$n mines $p.", FALSE, pxChar, pxDirt, 0, TO_ROOM);
         act("You mine $p.", FALSE, pxChar, pxDirt, 0, TO_CHAR);
         if(GET_LEVEL(pxChar)>=LVL_IMMORT)
            send_to_char(pxChar,"Skill Failure\r\n");
         load_otrigger(pxDirt);

         return;
         }

      for(i=0;i<NUM_ORE_SLOTS;i++)
         {
         if(world[IN_ROOM(pxChar)].ore_types[i]!=NOTHING)
            if(number(0,100)<= world[IN_ROOM(pxChar)].ore_percent[i])
               {
               if (!(pxDirt = read_object(world[IN_ROOM(pxChar)].ore_types[i], VIRTUAL)))
                  {
                  send_to_char(pxChar,"ERROR! Mine object not found.\r\n");
                  log("SYSERR: MINE object not found %d",
                      world[IN_ROOM(pxChar)].ore_types[i]);
                  return;
                  }
               obj_to_char(pxDirt, pxChar);
               act("$n mines $p.", FALSE, pxChar, pxDirt, 0, TO_ROOM);
               act("You mine $p.", FALSE, pxChar, pxDirt, 0, TO_CHAR);
               load_otrigger(pxDirt);
               if(GET_LEVEL(pxChar)>=LVL_IMMORT)
                  send_to_char(pxChar,"Skill Success: Ore found\r\n");
               return;
               }
         }
      if (!(pxDirt = read_object(MINE_DIRT, VIRTUAL)))
         {
         send_to_char(pxChar,"Dirt object not found.\r\n");
         log("SYSERR: MINE_DIRT not found %d", MINE_DIRT);
         return;
         }
      obj_to_char(pxDirt, pxChar);
      act("$n mines $p.", FALSE, pxChar, pxDirt, 0, TO_ROOM);
      act("You mine $p.", FALSE, pxChar, pxDirt, 0, TO_CHAR);
      if(GET_LEVEL(pxChar)>=LVL_IMMORT)
         send_to_char(pxChar,"Skill Success: Default case, no ore found\r\n");
      load_otrigger(pxDirt);
      return;
      }
   iState++;
   add_function_to_queue(5,pxChar,0,2,dig_update,pxChar,iState);
   }


ACMD(do_dig)
   {
   int iHasShovel=0;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(!ROOM_FLAGGED(IN_ROOM(ch),ROOM_MINE))
      {
      send_to_char(ch,"The ground is not suitable for digging here.\r\n");
      return;
      }
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_DIG)||GET_SKILL(ch, SKILL_DIG) <= 0))
      {
      send_to_char(ch,"You can't seem to figure out how to dig.\r\n");
      return;
      }

   if(GET_EQ(ch,WEAR_WIELD_1) || GET_EQ(ch,WEAR_WIELD_2))
      {
      send_to_char(ch,"Your weapon keeps you from digging properly.\r\n");
      return;
      }

   if(GET_EQ(ch,WEAR_HOLD_1))
      if(GET_OBJ_TYPE(GET_EQ(ch,WEAR_HOLD_1))==ITEM_SHOVEL)
         iHasShovel=1;
   if(!iHasShovel && GET_EQ(ch,WEAR_HOLD_2))
      if(GET_OBJ_TYPE(GET_EQ(ch,WEAR_HOLD_2))==ITEM_SHOVEL)
         iHasShovel=1;
   if(!iHasShovel)
      {
      send_to_char(ch,"You don't seem to be holding a digging implement.\r\n");
      return;
      }

   send_to_char(ch,"You start digging.\r\n");
   act("$n starts digging.", FALSE, ch, 0, 0, TO_ROOM);
   SET_BIT(AFF2_FLAGS(ch), AFF2_DIGGING);
   add_function_to_queue(5,ch,0,2,dig_update, ch,0);
   }


/*
 * Disarm skill code
 */
ACMD(do_disarm)
   {
   int penalty;
   struct char_data *vict;
   struct obj_data *target;
   char *arg;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument,arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Disarm who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if(vict==ch)
      {
      send_to_char(ch,"Yeah, ok stupid, try again.\r\n");
      return;
      }
   
   if (!ok_damage_shopkeeper(ch, vict))
      return;

   if (GET_MOVE(ch) < 10)
      {
      send_to_char(ch,"You are too exhausted!\r\n");
      return;
      }

   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_DISARM)||GET_SKILL(ch, SKILL_DISARM) <= 0))
      {
      send_to_char(ch,"You are not proficient with disarm!\r\n");
      if(!FIGHTING(vict))
         {
         send_to_char(ch,"%s notices your petty attempt and attacks!\r\n",
                   CAP(GET_NAME(vict)));
         hit(vict, ch, TYPE_UNDEFINED);
         }
      return;
      }

   if(!ch->equipment[WEAR_WIELD_1])
      {
      send_to_char(ch,"You need a weapon to try to disarm.\r\n");
      if(!FIGHTING(vict))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   if(!vict->equipment[WEAR_WIELD_1])
      {
      act("You try to disarm $N, but $E doesn't have a weapon.",
          FALSE, ch, 0, vict, TO_CHAR);
      act("$n makes an impressive fighting move, but does little more.",
          FALSE, ch, 0, 0, TO_ROOM);
      if(!FIGHTING(vict))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }
   penalty  = 25;
   if(TWO_HANDED(GET_EQ(vict,WEAR_WIELD_1)))
      penalty+=50;

   if(IS_OBJ_STAT(GET_EQ(vict,WEAR_WIELD_1),ITEM_NODROP))
      penalty+=50;

   penalty += dex_app[stat_index(GET_DEX(ch))].reaction*10;
   penalty -= dex_app[stat_index(GET_DEX(vict))].reaction*10;
   penalty -= (GET_LEVEL(ch)-GET_LEVEL(vict))*4;
   if(!skill_roll(ch,SKILL_DISARM,penalty))
      {
      GET_MOVE(ch) -= 5;
      act("You try to disarm $N, but fail miserably.",
          FALSE, ch, 0, vict, TO_CHAR);
      act("$n does a nifty fighting move, but then falls on $s butt.",
          FALSE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      if (IS_NPC(vict) && (GET_POS(vict) > POS_SLEEPING) &&
              !FIGHTING(vict))
         {
         set_fighting(vict, ch);
         if(!FIGHTING(ch))
            {
            set_fighting(ch,vict);
            }
         }
      WAIT_STATE(ch, SKILL_LAG*2);
      }
   else
      {
      GET_MOVE(ch) -= 10;
      if (MOB2_FLAGGED(vict, MOB2_NODISARM))
         {
         send_to_char(ch, "You attempt to disarm %s, but fail miserably.\r\n",
                      GET_NAME(vict));
         return;
         }
      else if (vict->equipment[WEAR_WIELD_1])
         {
         target = unequip_char(vict, WEAR_WIELD_1);
         act("$n makes an impressive fighting move.",FALSE, ch, 0, 0, TO_ROOM);
         act("You send $p flying from $N's grasp.",FALSE,ch,target,
             vict, TO_CHAR);
         act("$p flies from your grasp.", FALSE, ch, target, vict, TO_VICT);
         obj_to_char(target, vict);
         WAIT_STATE(vict, SKILL_LAG);
         }

      if ((IS_NPC(vict)) && (GET_POS(vict) > POS_SLEEPING) &&
              (!FIGHTING(vict)))
         {
         set_fighting(vict, ch);
         if(!FIGHTING(ch))
            {
            set_fighting(ch,vict);
            }
         }
      WAIT_STATE(ch, SKILL_LAG*2);
      }
   }

/*
 * Gore skill code
 */
ACMD(do_gore)
   {
   struct char_data *vict;
   int penalty;
   char *arg;


   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Gore who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"Aren't we funny today...\r\n");
      return;
      }
      
   if (!ok_damage_shopkeeper(ch, vict))
      return;
   
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_GORE)||GET_SKILL(ch, SKILL_GORE) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   WAIT_STATE(ch, SKILL_LAG * 3);
   penalty = (10 - (GET_AC(vict) / 20)) * 2;
   if (!skill_roll(ch,SKILL_GORE,penalty))
      {
      damage(ch, vict, 0, SKILL_GORE,IMM_SLASH);
      }
   else
      {
      damage(ch, vict, (2*GET_LEVEL(ch)), SKILL_GORE,IMM_SLASH);
      }
   }
/*
 * Headbutt skill code
 */
ACMD(do_headbutt)
   {
   struct char_data *vict;
   int penalty;
   char *arg;


   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Headbutt who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"Aren't we funny today...\r\n");
      return;
      }
      
   if (!ok_damage_shopkeeper(ch, vict))
      return;

   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_HEADBUTT)||GET_SKILL(ch, SKILL_HEADBUTT) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   WAIT_STATE(ch, SKILL_LAG * 3);
   penalty = (10 - (GET_AC(vict) / 20)) * 2;
   if (!skill_roll(ch,SKILL_HEADBUTT,penalty))
      {
      damage(ch, vict, 0, SKILL_HEADBUTT,IMM_BLUNT);
      }
   else
      {
      damage(ch, vict, (3*GET_LEVEL(ch))/2, SKILL_HEADBUTT,IMM_BLUNT);
      }
   }


/*
 * Kick skill code
 */
ACMD(do_kick)
   {
   struct char_data *vict;
   char *arg;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }
   arg=get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Kick who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }
   
   if (vict == ch)
      {
      send_to_char(ch,"Aren't we funny today...\r\n");
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_KICK)||GET_SKILL(ch, SKILL_KICK) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   WAIT_STATE(ch, SKILL_LAG*2);
   if (!skill_roll(ch,SKILL_KICK,((20-(GET_AC(vict)/20))*2)))
      {
      damage(ch, vict, 0, SKILL_KICK,IMM_BLUNT);
      }
   else
      {
      damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_KICK,IMM_BLUNT);
      }
   }

/*
 * Lay Hands skill code
 */
ACMD(do_lay_hands)
   {
   struct char_data *vict;
   char *arg;
   int heal;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(FIGHTING(ch))
      {
      send_to_char(ch,"You are too busy to heal.\r\n");
      return;
      }
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_LAY_HANDS)||GET_SKILL(ch, SKILL_LAY_HANDS) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);
   if (!(vict = get_char_vis(ch, arg,FIND_CHAR_ROOM)))
      {
      send_to_char(ch,"Who do you wish to heal???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if (vict == NULL || ch == NULL)
      return;

   if(GET_MAX_HIT(vict)==GET_HIT(vict))
      {
      send_to_char(ch,"But how can you heal someone who is not hurt?\r\n");
      return;
      }

   if(GET_POS(vict)>POS_SITTING)
      {
      act("$N must not be standing or fighting for this to work.",TRUE,
          ch,0,vict,TO_CHAR);
      act("$n tries to heal you with $s hand, but you are moving "
          "around too much.",TRUE,ch,0,vict,TO_VICT);
      return;
      }

   if (!skill_roll(ch,SKILL_LAY_HANDS,0))
      {
      send_to_char(ch,"You can't seem to be able to summon the energy "
                   "to heal.\r\n");
      act("$n tries to heal you, but can't seem to get a good grip.",FALSE,
          ch,0,vict,TO_VICT);
      act("$n tries to heal $N with $s hands, but nothing happens.",FALSE,
          ch,0,vict,TO_NOTVICT);
      }
   else
      {
      act("The wounds on $N disappear, but reappear on you.",FALSE,ch,0,
          vict,TO_CHAR);
      act("Your wounds disappear, but $n is now strangely injured.",FALSE,ch,
          0,vict,TO_VICT);
      act("$N's wounds seem to shift to $n as $n touches $N.",FALSE,ch,0,
          vict,TO_NOTVICT);
      if(GET_HIT(ch)<51)
         heal=GET_HIT(ch)-1;
      else
         heal=50;
      GET_HIT(ch)-=heal;
      GET_HIT(vict)+=heal;
      if(GET_HIT(vict)>GET_MAX_HIT(vict))
         GET_HIT(vict)=GET_MAX_HIT(vict);
      update_pos(vict);
      }
   }


/*
 * Lighten skill code
 */
ACMD(do_lighten)
   {
   struct room_affected_type af;
   struct room_data *rm;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_LIGHTEN)||GET_SKILL(ch, SKILL_LIGHTEN) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }

   if (GET_MANA(ch) < 20)
      {
      send_to_char(ch,"You do not have the energy to lighten.\n\r");
      return;
      }

   if (!skill_roll(ch,SKILL_LIGHTEN,0))
      {
      send_to_char(ch,"You fail to lighten the room.\n\r");
      GET_WAIT_STATE(ch) = 1 RL_SEC;
      return;
      }


   GET_MANA(ch) = GET_MANA(ch) - 20;

   rm=&world[IN_ROOM(ch)];
   if(rm->affected)  
      {
      affect_from_room(rm,SKILL_LIGHTEN);
      }
   if(rm->affected)
      {
      affect_from_room(rm,SKILL_DARKEN);
      }

   af.bitvector = 0;
   af.type = SKILL_LIGHTEN;
   af.duration =(int)(GET_LEVEL(ch)/10);
   af.modifier = (VALUE_LIGHT);
   af.location = APPLY_LIGHT;
   send_to_char(ch,"The room brightens.\n\r");
   act("$n lightens the room.",FALSE,ch,0,0,TO_ROOM);
   affect_join_room(rm, &af, FALSE, FALSE, FALSE, FALSE);
   return;
   }

/*
 * Meditate skill code
 */
ACMD(do_meditate)
   {
   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_MEDITATE)||GET_SKILL(ch, SKILL_MEDITATE) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }
   if(FIGHTING(ch))
      {
      send_to_char(ch,"But you are fighting!\r\n");
      return;
      }

   if(GET_POS(ch)==POS_MEDITATE)
      {
      send_to_char(ch,"You are already meditating twerp!\r\n");
      return;
      }

   if (!skill_roll(ch,SKILL_MEDITATE,0))
      {
      act("You don't have the mental capacity to meditate.", FALSE, ch,
          0, 0, TO_CHAR);
      return;
      }

   switch (GET_POS(ch))
      {
   case POS_STANDING :
      act("You sit down and meditate.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sits down and goes 'Ohmmmmm...'.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_MEDITATE;
      break;
   case POS_SITTING :
      act("You start meditating.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n goes 'Ohmmmm...'.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_MEDITATE;
      break;
   case POS_MEDITATE       :
      act("You are already meditating.", FALSE, ch, 0, 0, TO_CHAR);
      break;
   case POS_CHANT  :
      act("You stop chanting, and meditate.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n goes 'Ohmmmm...'.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_MEDITATE;
      break;
   case POS_BANDAGE        :
      act("You take off your bandages, and meditate.",FALSE,ch,0,0,
          TO_CHAR);
      act("$n goes 'Ohmmmm...'.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_MEDITATE;
      break;
   case POS_RESTING :
      act("You sit up and meditate.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n goes 'Ohmmmm...'.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_MEDITATE;
      break;
   case POS_SLEEPING :
      act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
      break;
   case POS_FIGHTING :
      act("Meditate while fighting?  Are you MAD?",FALSE,ch,0, 0, TO_CHAR);
      break;
   default :
      act("You stop floating around, and meditate.",
          FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops floating around, and goes 'Ohmmmm...'.", FALSE,
          ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_MEDITATE;
      break;
      }
   }



/*
 * Palm skill code
 */
ACMD(do_palm)
   {
   char *arg;
   struct obj_data *obj;
   struct char_data *tch;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;


   if ((IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))||
           (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch)))
      {
      send_to_char(ch,"Your arms are already full!\r\n");
      return;
      }
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_PALM)||GET_SKILL(ch, SKILL_PALM) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   if (!*arg)
      send_to_char(ch,"Palm what?\r\n");
   else
      {
      if (!(obj = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents)))
         {
         send_to_char(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
         }
      else
         {
         if (can_take_obj(ch, obj) && get_otrigger(obj,ch))
            {
            obj_from_room(obj);
            obj_to_char(obj, ch);
            act("You slyly palm $p.", FALSE, ch, obj, 0, TO_CHAR);
            if (!skill_roll(ch,SKILL_PALM,(GET_OBJ_WEIGHT(obj)/2)))
               act("$n tries to slyly palm $p.", TRUE, ch, obj, 0, TO_ROOM);
            else
               {
               for(tch=world[IN_ROOM(ch)].people;tch;tch=tch->next_in_room)
                  {
                  if((GET_LEVEL(tch)>=LVL_IMMORT) && (ch!=tch))
                     {
                     send_to_char(tch, "IMM: %s palms %s.\r\n",GET_NAME(ch),
                                  GET_OBJ_NAME(obj));
                     }
                  }
               }
            if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
               {
               mudlogf(NRM,  GOD_LOG(ch),TRUE,
                       "(GC) %s has picked up %s [%ld] at [%ld].",
                       GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj),
                       GET_ROOM_VNUM(IN_ROOM(ch)));
               }
            get_check_money(ch, obj,0);
            }
         else
            {
            send_to_char(ch,"You can't palm that.\r\n");
            }
         }
      }
   release_buffer(arg);
   }

/*
 * Peck skill code
 */
ACMD(do_peck)
   {   
   struct char_data *vict;
   char *arg;
   int crit;
   struct affected_type af;
   
   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }
   arg=get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);
   
   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);
   
   if(vict==NULL)
      {
      send_to_char(ch,"Peck who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }
   
   if (vict == ch)
      {
      send_to_char(ch,"Aren't we funny today...\r\n");
      return;
      }
      
   if (!ok_damage_shopkeeper(ch, vict))
      return;
       
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_PECK)||!GET_SKILL(ch, SKILL_PECK)))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }
   crit = number(1, 20); /* calculates for critical hit */
    
   WAIT_STATE(ch, SKILL_LAG*2);
   if (!skill_roll(ch,SKILL_PECK,((20-(GET_AC(vict)/20))*2)))
      {
      damage(ch, vict, 0, SKILL_PECK,IMM_PIERCE);
      }
   else if (crit == 1) {
     if (!AFF_FLAGGED(vict, AFF_BLIND))
      {
      send_to_char(ch, "You peck them in the eyes!\r\n");
      send_to_char(vict, "You have been pecked in the eyes! You're Blind!!\r\n");
      af.type = SPELL_BLINDNESS;
      af.location = APPLY_HITROLL;
      af.modifier = -2;
      af.duration = 1/2;
      af.bitvector = AFF_BLIND;
      affect_to_char(vict, &af);
      damage(ch, vict, 20, SKILL_PECK,IMM_PIERCE);
      } 
     else
      damage(ch, vict, 20, SKILL_PECK,IMM_PIERCE);
      }
      else
      {
      damage(ch, vict, 10, SKILL_PECK,IMM_PIERCE);
      }
   }

/*
 * Quivering Palm skill code
 */
ACMD(do_quivering_palm)
   {
   struct char_data *vict;
   char *arg;
   int penalty;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Quivering palm who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"Aren't we funny today...\r\n");
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_QUIVERING_PALM)||GET_SKILL(ch, SKILL_QUIVERING_PALM) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   WAIT_STATE(ch, SKILL_LAG * 2);
   penalty = ((10 - (GET_AC(vict) / 20)) * 2);
   if (!skill_roll(ch,SKILL_QUIVERING_PALM,penalty))
      {
      damage(ch, vict, 0, SKILL_QUIVERING_PALM,IMM_ENERGY);
      }
   else
      {
      damage(ch, vict, GET_LEVEL(ch) * 2, SKILL_QUIVERING_PALM,IMM_ENERGY);
      }
   }


/*
 * Rage skill code
 */
ACMD(do_rage)
   {
   struct affected_type af;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_RAGE)||GET_SKILL(ch, SKILL_RAGE) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }

   if(AFF_FLAGGED(ch, AFF_RAGE))
      {
      send_to_char(ch,"You are already enraged!\r\n");
      return;
      }

   if(affected_by_spell(ch,SPELL_INSPIRE))
      {
      send_to_char(ch,"You are too inspired to get angry.\r\n");
      return;
      }

   if (!FIGHTING(ch))
      {
      send_to_char(ch,"That would be a waste since you are not fighting.\r\n");
      return;
      }

   if (IN_ROOM(ch)!=IN_ROOM(FIGHTING(ch)))
      {
      send_to_char(ch,"Your opponent isn't here anymore...\r\n");
      return;
      }

   if (GET_MOVE(ch) < 20)
      {
      send_to_char(ch,"You are too exhausted!\r\n");
      return;
      }

   GET_MOVE(ch) -= 20;
   if(skill_roll(ch,SKILL_RAGE,0))
      {
      send_to_char(ch,"You go into an unstoppable RAGE!!!!\r\n");
      act("$n goes into a RAGE!!!!", FALSE, ch, 0, 0, TO_NOTVICT);
      af.type = SKILL_RAGE;
      af.duration = -1;
      af.modifier = 5;
      af.location = APPLY_DAMROLL;
      af.bitvector = AFF_RAGE;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      af.modifier = 7;
      af.location = APPLY_HITROLL;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      af.modifier = 60;
      af.location = APPLY_AC;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      }
   else
      {
      send_to_char(ch,"You attempt go into a rage but fail miserably.\r\n");
      act("$n gets a little hot under the collar!!!", FALSE, ch, 0, 0,
          TO_NOTVICT);
      }
   WAIT_STATE(ch, SKILL_LAG * 1 );
   }


/*
 * redirect skill code
 */
ACMD(do_redirect)
   {
   struct char_data *vict;
   char *arg;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(!FIGHTING(ch))
      {
      send_to_char(ch,"How can you switch targets if you aren't fighting someone "
                   "in the first place?\r\n");
      return;
      }
   else if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }
   else if(AFF_FLAGGED(ch,AFF_RAGE))
      {
      send_to_char(ch,"WHAT!?!? You have a perfectly good target in front of you.\r\n");
      return;
      }
   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);
   if (!(vict = get_char_vis(ch, arg,FIND_CHAR_ROOM)))
      {
      send_to_char(ch,"Who do you wish to start hitting?\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_REDIRECT)||GET_SKILL(ch, SKILL_REDIRECT) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch)||!FIGHTING(vict))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   if (vict == FIGHTING(ch))
      {
      send_to_char(ch,"But you're already fighting %s!\r\n", GET_NAME(FIGHTING(ch)));
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"Aren't we funny today...\r\n");
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   if (!skill_roll(ch,SKILL_REDIRECT,0))
      {
      send_to_char(ch,"You try to change targets, but can't!\r\n");
      }
   else
      {
      stop_fighting(ch);
      set_fighting(ch,vict);
      act("You change opponents to $N!",FALSE,ch,0,vict,TO_CHAR);
      act("With a deft combat manuever $n starts fighting you!",
          FALSE,ch,0,vict,TO_VICT);
      act("With a deft combat manuever $n switches opponents and starts to "
          "fight $N!",FALSE,ch,0,vict,TO_NOTVICT);
      }
   WAIT_STATE(ch, SKILL_LAG*2);
   }


/*
 * Rescue skill code
 */
ACMD(do_rescue)
   {
   struct char_data *vict, *tmp_ch;
   char *arg;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }
   arg=get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   if (!(vict = get_char_vis(ch, arg,FIND_CHAR_ROOM)))
      {
      if(FIGHTING(ch) && FIGHTING(FIGHTING(ch)))
         vict=FIGHTING(FIGHTING(ch));
      else
         {
         send_to_char(ch,"Whom do you want to rescue?\r\n");
         release_buffer(arg);
         return;
         }
      }
   release_buffer(arg);

   if (vict == ch)
      {
      send_to_char(ch,"What about fleeing instead?\r\n");
      return;
      }
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_RESCUE)||GET_SKILL(ch, SKILL_RESCUE)<=0))
      {
      send_to_char(ch,"But you have no idea how!\r\n");
      return;
      }
   if (FIGHTING(ch) == vict)
      {
      send_to_char(ch,"How can you rescue someone you are trying to kill?\r\n");
      return;
      }
   if(AFF_FLAGGED(vict,AFF_RAGE))
      {
      act("$N is so enraged, $E won't let you interfere.",FALSE,ch,0,vict,
          TO_CHAR);
      act("$n tries to rescue you, but you knock $m away.",FALSE,ch,0,vict,
          TO_VICT);
      act("$n tries to rescue $N but $E is too enraged to allow it.",FALSE,ch,
          0,vict,TO_NOTVICT);
      return;
      }
   for (tmp_ch = world[IN_ROOM(ch)].people;tmp_ch&&(FIGHTING(tmp_ch) != vict);
           tmp_ch = tmp_ch->next_in_room)
      ;

   if (!tmp_ch)
      {
      act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
      return;
      }
   WAIT_STATE(ch, 1 * SKILL_LAG);
   if (!skill_roll(ch,SKILL_RESCUE,0))
      {
      send_to_char(ch,"You fail the rescue!\r\n");
      return;
      }
   else
      {
      send_to_char(ch,"Banzai!  To the rescue...\r\n");
      act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

      if (FIGHTING(vict) == tmp_ch)
         stop_fighting(vict);
      if (FIGHTING(tmp_ch))
         stop_fighting(tmp_ch);
      if (FIGHTING(ch))
         stop_fighting(ch);

      LAST_HAND_USED(ch)=0;
      set_fighting(ch, tmp_ch);
      set_fighting(tmp_ch, ch);
      WAIT_STATE(vict, 1 * SKILL_LAG);
      }
   }


/*
 * Shadow skill code
 */
ACMD(do_shadow)
   {
   int penalty=0;
   struct affected_type af;

   if (IS_NPC(ch) &&AFF_FLAGGED(ch, AFF_CHARM))
      {
      send_to_char(ch,"You try to hide in the shadows, but "
                      "your master catches you!\r\n");
      return;
      }
   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_SHADOW)||GET_SKILL(ch, SKILL_SHADOW) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      return;
      }

   

   send_to_char(ch,"You attempt to withdraw into the shadows.\r\n");

   if (AFF2_FLAGGED(ch, AFF2_SHADOW))
      affect_from_char(ch, SKILL_SHADOW);

   penalty=world[IN_ROOM(ch)].light;

   if (!skill_roll(ch,SKILL_SHADOW,penalty))
      {
      return;
      }
   else
      {
      af.type = SKILL_SHADOW;
      af.duration = GET_LEVEL(ch) / 2;
      af.modifier = 0;
      af.location = APPLY_AFF2;
      af.bitvector = AFF2_SHADOW;
      affect_to_char(ch, &af);
      }

/*   SET_BIT(AFF_FLAGS(ch), AFF_HIDE);*/
   }



/*
 * Shock skill code
 */
ACMD(do_shock)
   {
   struct char_data *vict;
   char *arg;
   int penalty;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg=get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Shock who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_SHOCK)||GET_SKILL(ch, SKILL_SHOCK) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"You wanna zorch yourself?!!\r\n");
      return;
      }

   if (GET_MANA(ch) < 10)
      {
      send_to_char(ch,"You don't have the energy to do it.");
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   penalty =str_app[STRENGTH_APPLY_INDEX(ch)].tohit*10;
   penalty -=con_app[stat_index(GET_CON(vict))].hitp*10;
   if(!skill_roll(ch,SKILL_SHOCK,penalty))
      damage(ch,vict,0,SKILL_SHOCK,IMM_ENERGY);
   else
      {
      if(damage(ch,vict,GET_LEVEL(ch),SKILL_SHOCK,IMM_ENERGY) > 1)
         {
         if(GET_POS(vict)==POS_STUNNED)
            {
            WAIT_STATE(vict,SKILL_LAG);
            }
         else
            {
            GET_POS(vict)=POS_STUNNED;
            WAIT_STATE(vict,SKILL_LAG*2);
            }
         }
      }
   WAIT_STATE(ch,SKILL_LAG*2);
   GET_MANA(ch) = GET_MANA(ch) - 10;
   }

/*
 * skin skill code
 */
void skin_update(struct char_data *pxChar,
                 struct obj_data *pxCarcass,
                 int iState)
   {
   struct obj_data *pxSkin;

   if(!AFF2_FLAGGED(pxChar,AFF2_SKINNING))
      return;
   if(iState==0)
      {
      send_to_char(pxChar,"You work at skinning the carcass, making a mess.\r\n");
      act("$n works at skinning the carcass, making a mess.", FALSE,
          pxChar, 0, 0, TO_ROOM);
      }
   else
      {
      REMOVE_BIT(AFF2_FLAGS(pxChar), AFF2_SKINNING);
      send_to_char(pxChar,"You finish skinning the carcass and take a look at "
                   "your work.\r\n");
      if(GET_OBJ_VAL(pxCarcass,4)==NOTHING)
         {
         send_to_char(pxChar,"It seems that someone has already gotten this "
                      "hide.\r\n");
         return;
         }
      if (!skill_roll(pxChar,SKILL_SKIN,0))
         {
         send_to_char(pxChar,"You seem to have botched the job and are left "
                      "with only shreds.\r\n");
         GET_OBJ_VAL(pxCarcass,4)=NOTHING;
         if(GET_LEVEL(pxChar)>=LVL_IMMORT)
            send_to_char(pxChar,"IMM: Skill Failure\r\n");
         return;
         }
      if (!(pxSkin = read_object(GET_OBJ_VAL(pxCarcass,4), VIRTUAL)))
         {
         send_to_char(pxChar,"Hide object not found.\r\n");
         log("SYSERR: Hide not found %ld", GET_OBJ_VAL(pxCarcass,4));
         return;
         }
      obj_to_char(pxSkin, pxChar);
      act("$n skins $p.", FALSE, pxChar, pxSkin, 0, TO_ROOM);
      act("You skin $p.", FALSE, pxChar, pxSkin, 0, TO_CHAR);
      GET_OBJ_VAL(pxCarcass,4)=NOTHING;
      if(GET_LEVEL(pxChar)>=LVL_IMMORT)
         send_to_char(pxChar,"IMM: Skill Success\r\n");
      load_otrigger(pxSkin);
      return;
      }
   iState++;
   add_function_to_queue(3,pxChar,0,3,skin_update,pxChar,pxCarcass,iState);
   }


ACMD(do_skin)
   {
   int iHasKnife=0;
   int iWeaponType=0;
   struct obj_data *pxCarcass;
   char *buf=get_buffer(MAX_INPUT_LENGTH);

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   /* find the corpse! */

   any_one_arg(argument, buf);

   if (*buf)
      {
      if((pxCarcass=get_obj_in_list_vis(ch,buf,world[IN_ROOM(ch)].contents))
              != NULL)
         {
         if((GET_OBJ_TYPE(pxCarcass)!=ITEM_CONTAINER) ||
                 !GET_OBJ_VAL(pxCarcass,3))
            {
            send_to_char(ch,"But that isn't a corpse.\r\n");
            release_buffer(buf);
            return;
            }
         if(GET_OBJ_VAL(pxCarcass,4)==NOTHING)
            {
            send_to_char(ch,"This corpse's hide is not worth anything.\r\n");
            release_buffer(buf);
            return;
            }
         }
      else   /* no obj */
         {
         release_buffer(buf);
         send_to_char(ch,"Skin WHAT!?!\r\n");
         return;
         }
      }
   else
      {
      release_buffer(buf);
      send_to_char(ch,"USAGE: skin corpse or skin 1.corpse\r\n");
      return;
      }
   release_buffer(buf);


   if (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_SKIN)||GET_SKILL(ch, SKILL_SKIN) <= 0))
      {
      send_to_char(ch,"You can't seem to figure out how to skin this "
                   "carcass.\r\n");
      return;
      }

   if(GET_EQ(ch,WEAR_WIELD_1))
      {
      iWeaponType=GET_OBJ_VAL(GET_EQ(ch,WEAR_WIELD_1),3)+TYPE_HIT;
      if((iWeaponType==TYPE_PIERCE)||
              (iWeaponType==TYPE_SLASH)||
              (iWeaponType==TYPE_PIERCE_NO_BS)||
              (iWeaponType==TYPE_CLEAVE)||
              (iWeaponType==TYPE_STAB))
         {
         iHasKnife=1;
         }
      }

   if(!iHasKnife)
      {
      send_to_char(ch,"You don't seem to be wielding anything that "
                   "could remove this hide.\r\n");
      return;
      }

   send_to_char(ch,"You kneel down and start skinning the carcass.\r\n");
   act("$n kneels down and starts skinning the carcass.",
       FALSE, ch, 0, 0, TO_ROOM);
   SET_BIT(AFF2_FLAGS(ch), AFF2_SKINNING);
   add_function_to_queue(3,ch,0,3,skin_update, ch,pxCarcass,0);
   }

/*
 * Steal skill code
 */
ACMD(do_steal)
   {
   struct char_data *vict;
   struct obj_data *obj;
   char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
   int gold, eq_pos, pcsteal = 0, ohoh = 0;
   int penality=0;

   /* players want penalty to be VERY VERY negative. */

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (!SCR_SKILLCHECK(ch, SKILL_STEAL)||GET_SKILL(ch, SKILL_STEAL) < 25) {
     send_to_char(ch, "You have no idea how.\r\n");
     return;
   }

   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   two_arguments(argument,obj_name,vict_name);

   if ((strlen(vict_name)==0)||
           !(vict = get_char_vis(ch, vict_name, FIND_CHAR_ROOM)))
      {
      send_to_char(ch,"Steal what from who?\r\n");
      return;
      }
   else if (vict == ch)
      {
      send_to_char(ch,"Come on now, that's rather stupid!\r\n");
      return;
      }

   if (!pt_allowed && !IS_NPC(vict)&&!IS_NPC(ch))
      {
      send_to_char(ch,"Come on now, that's rather stupid!\r\n");
      return;
      }
   /* 101% is a complete failure */
   /* penality =  dex_app_skill[stat_index(GET_DEX(ch))].p_pocket; */
   penality = 25;
   penality -= dex_app_skill[stat_index(GET_DEX(ch))].p_pocket/5;
   penality -= MIN(10, 4*(GET_LEVEL(ch) - GET_LEVEL(vict)));

   if (AFF_FLAGGED(ch, AFF_SNEAK)) {
     penality -= GET_LEVEL(ch)/10;
   }

   if (GET_POS(vict) < POS_SLEEPING)
     penality -= 50;
     /* penality = -500; */  /* ALWAYS SUCCESS */
   if(!AWAKE(vict))
      penality -=25;

   if(GET_LEVEL(ch)>=LVL_DETY && !IS_NPC(ch) && PRF_FLAGGED(ch,PRF_NOHASSLE))
      penality = -500;

   /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
   if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal ||
           GET_MOB_SPEC(vict) == shop_keeper)
      penality = 500;  /* Failure */

   if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold"))
      {
      if (!(obj = get_obj_in_list_vis(ch, obj_name, vict->carrying)))
         {
         for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
            if (GET_EQ(vict, eq_pos) &&
                    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
                    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos)))
               {
               obj = GET_EQ(vict, eq_pos);
               break;
               }
         if (!obj)
            {
            act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
            return;
            }
         else
            {
            /* It is equipment */
            if ((GET_POS(vict) > POS_STUNNED)&&!PRF_FLAGGED(ch,PRF_NOHASSLE))
               {
               send_to_char(ch,"Steal the equipment now?  Impossible!\r\n");
               return;
               }
            else
               {
               act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
               act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
               obj_to_char(unequip_char(vict, eq_pos), ch);
               }
            }
         }
      else if (IS_SET(GET_OBJ_EXTRA2(obj), ITEM2_BODYPART))
         {
         /* can't steal body parts */

         act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
         return;
         }
      else
         {
         /* obj found in inventory */

         penality += GET_OBJ_WEIGHT(obj); /* Make heavy harder */

         if (AWAKE(vict) && !skill_roll(ch,SKILL_STEAL,penality))
            {
            ohoh = TRUE;
            send_to_char(ch,"Oops..\r\n");
            act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
            act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
            improve_skill(ch,SKILL_STEAL,USE_FAIL);
            }
         else
            {
            /* Steal the item */

            if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)))
               {
               improve_skill(ch,SKILL_STEAL,USE_PASS);
               if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch))
                  {
                  obj_from_char(obj);
                  obj_to_char(obj, ch);
                  send_to_char(ch,"Got it!\r\n");
		  if (IS_NPC(vict)) {
		    act("You seem to be missing something.", TRUE, ch, 0, vict, TO_VICT);
		  }
                  }
               }
            else
               send_to_char(ch,"You cannot carry that much.\r\n");
            }
         }
      }
   else
      {
      /* Steal some coins */
      if (AWAKE(vict) && !skill_roll(ch,SKILL_STEAL,penality))
         {
         ohoh = TRUE;
         send_to_char(ch,"Oops..\r\n");
         act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
         act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
         improve_skill(ch,SKILL_STEAL,USE_FAIL);
         }
      else
         {
         /* Steal some gold coins */
         improve_skill(ch,SKILL_STEAL,USE_PASS);
         gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
         gold = MIN(1782, gold);
         if (gold > 0)
            {
            GET_GOLD(ch) += gold;
            GET_GOLD(vict) -= gold;
            if (gold > 1)
               {
               send_to_char(ch, "Bingo!  You got %d gold coins.\r\n", gold);
               }
            else
               {
               send_to_char(ch,"You manage to swipe a solitary gold coin.\r\n");
               }
            }
         else
            {
            send_to_char(ch,"You couldn't get any gold...\r\n");
            }
         }
      }

   if (ohoh && IS_NPC(vict) && AWAKE(vict))
      hit(vict, ch, TYPE_UNDEFINED);
   }


/*
 * Stomp skill code
 */
ACMD(do_stomp)
   {
   struct char_data *vict;
   int penalty;
   char *arg;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Stomp who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"Aren't we funny today...\r\n");
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   if (!IS_NPC(ch) && (!SCR_SKILLCHECK(ch, SKILL_STOMP)||GET_SKILL(ch, SKILL_STOMP) <= 0))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   WAIT_STATE(ch, SKILL_LAG * 2);
   penalty = (10 - (GET_AC(vict) / 20)) * 2;
   if (!skill_roll(ch,SKILL_STOMP,penalty))
      {
      damage(ch, vict, 0, SKILL_STOMP,IMM_BLUNT);
      }
   else
      {
      damage(ch, vict, GET_LEVEL(ch), SKILL_STOMP,IMM_BLUNT);
      }

   }


/*
 * Stun skill code
 */
ACMD(do_stun)
   {
   int penalty;
   struct char_data *vict;
   char *arg;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg=get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Stun who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if(IN_ROOM(ch)!=IN_ROOM(vict))
      return;
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }
   
   if (vict == ch)
      {
      send_to_char(ch,"You wanna whack yourself?!!\r\n");
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   if (MOB2_FLAGGED(vict, MOB2_NOSTUN) ||
       (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_STUN)||GET_SKILL(ch, SKILL_STUN) <= 0)))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   penalty =con_app[stat_index(GET_CON(vict))].hitp*5;
   penalty -=str_app[STRENGTH_APPLY_INDEX(ch)].tohit*3;
   WAIT_STATE(ch,SKILL_LAG*3);
   if(!skill_roll(ch,SKILL_STUN,penalty))
      {
      damage(ch,vict,0,SKILL_STUN,IMM_HOLD);
      }
   else
      {
      if(damage(ch,vict,GET_LEVEL(ch),SKILL_STUN,IMM_HOLD)>0)
         {
         if(!CHECK_STUN(vict))
            {
            if(GET_POS(vict)==POS_STUNNED)
               {
               /* if both are PCs, then give command lag instead - nomi*/
               if (!IS_NPC(ch) && !IS_NPC(vict))
                  WAIT_STATE(vict, SKILL_LAG);    
               else
                  STUN_STATE(vict, STUN_LAG);
               }
            else
               {
               GET_POS(vict)=POS_STUNNED;
               /* if both are PCs, then give command lag instead - nomi*/
               if (!IS_NPC(ch) && !IS_NPC(vict))  
                  WAIT_STATE(vict, SKILL_LAG);  
               else
                  STUN_STATE(vict,STUN_LAG*2);
               }
            }
         }
      }
   }



/*
 * Sweep skill code
 */
ACMD(do_sweep)
   {
   struct char_data *vict;
   char *arg;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Sweep who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if(IN_ROOM(ch)!=IN_ROOM(vict))
      return;
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"Aren't we funny today...\r\n");
      return;
      }

   if (MOB2_FLAGGED(vict, MOB2_NOSWEEP) ||
       (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_SWEEP)||GET_SKILL(ch, SKILL_SWEEP) <= 0)))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   WAIT_STATE(ch, SKILL_LAG*3);
   if (!skill_roll(ch,SKILL_SWEEP,0))
      {
      damage(ch, vict, 0, SKILL_SWEEP,IMM_HOLD);
      GET_POS(ch) = POS_SITTING;
      hit(vict,ch,TYPE_UNDEFINED);
      }
   else
      {
      damage(ch, vict, GET_LEVEL(ch), SKILL_SWEEP,IMM_HOLD);
      }
   }


/*
 * Tame skill code
 */
ACMD(do_tame)
   {
   struct char_data *vict;
   char *arg;
   struct affected_type af;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);
   if (!(vict = get_char_vis(ch, arg,FIND_CHAR_ROOM)))
      {
      send_to_char(ch,"Who do you wish to tame???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if (vict == NULL || ch == NULL)
      return;

   if(!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_TAME)||GET_SKILL(ch,SKILL_TAME)<=0))
      {
      send_to_char(ch,"You fail.\r\n");
      return;
      }

   if (GET_MANA(ch) < 20)
      {
      send_to_char(ch,"You do not have the energy to tame this beast.");
      return;
      }
   GET_MANA(ch) = GET_MANA(ch) - 20;

   WAIT_STATE(ch,SKILL_LAG);
   if (vict == ch)
      send_to_char(ch,"You like yourself even better!\r\n");
   else if(!trait_info[GET_RACE(vict)].can_tame)
      send_to_char(ch,"This being can't be tamed.\r\n");
   else if (!IS_NPC(vict) && !PRF_FLAGGED(vict, PRF_SUMMONABLE))
      send_to_char(ch,"You fail because SUMMON protection is on!\r\n");
   else if (AFF_FLAGGED(vict, AFF_SANCTUARY))
      send_to_char(ch,"Your vict is protected by sanctuary!\r\n");
   else if (AFF_FLAGGED(ch, AFF_CHARM))
      send_to_char(ch,"You can't have any followers of your own!\r\n");
   else if (MOB_FLAGGED(vict, MOB_NOCHARM))
      send_to_char(ch,"You can't seem to communicate to this beast!\r\n");
   else if (AFF_FLAGGED(vict, AFF_CHARM) || GET_LEVEL(ch) < GET_LEVEL(vict))
      send_to_char(ch,"You fail.\r\n");
   else if (!pk_allowed && !IS_NPC(vict))
      send_to_char(ch,"You fail - shouldn't be doing it anyway.\r\n");
   else if (circle_follow(vict, ch))
      send_to_char(ch,"Sorry, following in circles can not be allowed.\r\n");
   else if (IS_NPC(vict) && (IS_IMMUNE(vict, IMM_BLUNT) || IS_IMMUNE(vict, IMM_PIERCE) || IS_IMMUNE(vict, IMM_SLASH) || IS_IMMUNE(vict, IMM_PLUS1) || IS_IMMUNE(vict, IMM_PLUS2) || IS_IMMUNE(vict, IMM_PLUS3) || IS_IMMUNE(vict, IMM_PLUS4) || IS_IMMUNE(vict, IMM_NONMAG)))
     send_to_char(ch, "You fail.\r\n");
   else if (mag_savingthrow(vict, SAVING_PARA,GET_LEVEL(ch)/10,0))
      {
      send_to_char(ch,"The beast resists!\r\n");
      send_to_char(ch,"You enrage the beast!!\r\n");
      hit(vict,ch,TYPE_UNDEFINED);
      }
   /*else if (num_charmies(ch) >= (NUM_CHARM_ADJ_CHA(ch, vict) + \
     (GET_LEVEL(ch)/10))) */
   else if (num_charmies(ch) >= MAX_CHARMIES)
      {
      send_to_char(ch,"Sorry, your skills are already taxed with this "
                   "many charmies.\r\n");
      send_to_char(ch,"You enrage the beast!!\r\n");
      hit(vict,ch,TYPE_UNDEFINED);
      }
   else if (!skill_roll(ch,SKILL_TAME,0))
      {
      send_to_char(ch,"You enrage the beast.\r\n");
      hit(vict,ch,TYPE_UNDEFINED);
      }
   else
      {
      if (vict->master)
         stop_follower(vict);
      add_follower(vict, ch);

      af.type = SKILL_TAME;

      af.duration=(12 * 18 / MAX(1,GET_INT(vict)))+CHARM_TIME_ADJ_CHA(ch,vict);

      af.modifier = 0;
      af.location = 0;
      af.bitvector = AFF_CHARM;
      affect_to_char(vict, &af);
      act("Isn't $n just such a nice fellow?", FALSE, ch, 0, vict, TO_VICT);
      if (IS_NPC(vict))
         {
         REMOVE_BIT(MOB_FLAGS(vict), MOB_AGGRESSIVE);
         REMOVE_BIT(MOB_FLAGS(vict), MOB_SPEC);
         }
      }
   }



/*
 * Trip skill code
 */
ACMD(do_trip)
   {
   struct char_data *vict;
   char *arg;

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL)&&GET_LEVEL(ch)<LVL_ADMIN)
      {
      send_to_char(ch,"You can not bring yourself to disturb the peace in "
                   "this area\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);

   vict=NULL;
   if ((!*arg)&&FIGHTING(ch))
      vict = FIGHTING(ch);
   else if(*arg)
      vict = get_char_vis(ch, arg,FIND_CHAR_ROOM);

   if(vict==NULL)
      {
      send_to_char(ch,"Trip who???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if(IN_ROOM(ch)!=IN_ROOM(vict))
      return;
   if (!pk_allowed && !IS_NPC(vict)&&!IS_NPC(ch)&&
           !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
      {
      act("Use 'murder' if you really want to attack $N.", FALSE,
          ch, 0, vict, TO_CHAR);
      return;
      }

   if (vict == ch)
      {
      send_to_char(ch,"Aren't we funny today...\r\n");
      return;
      }

   if (MOB2_FLAGGED(vict, MOB2_NOTRIP) ||
       (!IS_NPC(ch)&&(!SCR_SKILLCHECK(ch, SKILL_TRIP)||GET_SKILL(ch, SKILL_TRIP) <= 0)))
      {
      send_to_char(ch,"You fail!\r\n");
      if(!FIGHTING(ch))
         hit(vict, ch, TYPE_UNDEFINED);
      return;
      }

   if (!ok_damage_shopkeeper(ch, vict))
      return;

   WAIT_STATE(ch, SKILL_LAG*3);
   if (!skill_roll(ch,SKILL_TRIP,0))
      {
      damage(ch, vict, 0, SKILL_TRIP,IMM_HOLD);
      GET_POS(ch) = POS_SITTING;
      hit(vict,ch,TYPE_UNDEFINED);
      }
   else
      {
      damage(ch, vict, GET_LEVEL(ch), SKILL_TRIP,IMM_HOLD);
      }
   }

void check_fishing(void) 
   {
   struct descriptor_data *d;
   struct char_data *ch;
   int bite;

   for (d = descriptor_list; d; d = d->next) 
      {
      if (d->connected) 
         continue;

      ch = d->character;

      if (PLR_FLAGGED(ch, PLR_FISHING))
         {
         if (!ROOM2_FLAGGED(IN_ROOM(ch), ROOM2_SALTWATER_FISH) &&
             !ROOM2_FLAGGED(IN_ROOM(ch), ROOM2_FRESHWATER_FISH))
            {
            /* uhoh, can't fish here */
            REMOVE_BIT(PLR_FLAGS(ch), PLR_FISHING | PLR_FISH_ON);
            continue;
            }

	 int found_pole = 0;
	 if (GET_EQ(ch, WEAR_HOLD_1) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD_1)) == ITEM_POLE) {
	   found_pole = 1;
	 } else if (GET_EQ(ch, WEAR_HOLD_2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD_2)) == ITEM_POLE) {
	   found_pole = 1;
	 }
	 if (!found_pole) {
            /* now where did that pole go? */
            REMOVE_BIT(PLR_FLAGS(ch), PLR_FISHING | PLR_FISH_ON);
            continue;
	 }

         if (PLR_FLAGGED(ch, PLR_FISH_ON))
            {
            /* waited too long, fish bails */
            REMOVE_BIT(PLR_FLAGS(ch), PLR_FISH_ON);
            }
         bite = number(1, 10);

         if (bite > 8)
            {
            send_to_char(ch, "Your line suddenly jumps to life, FISH ON!!!\r\n");
            SET_BIT(PLR_FLAGS(ch), PLR_FISH_ON);
            }
         else if (bite > 6) 
            {
            send_to_char(ch, "You feel a very solid pull on your line!\r\n");
            SET_BIT(PLR_FLAGS(ch), PLR_FISH_ON);
            }
         else if (bite > 4)
            {
            send_to_char(ch, "You feel a slight jiggle on your line.\r\n");
            }
         else if (bite > 2)
            {
            send_to_char(ch, "Time goes by... not even a nibble.\r\n");
            }
         }
      }
   }


#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
 (OBJVAL_FLAGGED(obj,CONT_PICKPROOF)) : \
 (EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))

void process_skills(struct char_data *ch, struct char_data *tch, struct obj_data *item, 
                    int t_alt, int skill, int state) 
   {
 
  /* process skill and whine if we're not supposed to be here */
   switch (skill)
      {
      case SKILL_PICK_LOCK:
		  /* process_skills is called during the pick lock phases. Inbetween those phases
		   * the actor can abandon picking the lock (by moving rooms, etc.) This function
		   * is still called, but there is nothing more to pick, so return immediately.
		   */
		  if (!AFF2_FLAGGED(ch, AFF2_PICKING) && !AFF2_FLAGGED(ch, AFF2_PICKING_STAY)) {
			  return;
		  }
		/*
         if (!EXIT(ch, t_alt))
            {
            send_to_char(ch, "You abandon picking the lock.\r\n");
            return;
            }
		*/
         /* give an in-between message, then call again in 5 seconds */
         if (state == 0)
            {
            state++;
            send_to_char(ch, "You continue working dilligently on the lock.\r\n");
            add_function_to_queue(HALF_SKILL_COUNT, ch, 0, 6,
                  process_skills, ch, NULL, item, t_alt, SKILL_PICK_LOCK, state);
            }
         else
            {
            int percent = number(1, 101);
            if (DOOR_IS_PICKPROOF(ch, item, t_alt)) {
               send_to_char(ch,"You cannot pick this lock.\r\n");
               REMOVE_BIT(AFF2_FLAGS(ch), AFF2_PICKING_STAY);
               REMOVE_BIT(AFF2_FLAGS(ch), AFF2_PICKING); }
            else if (percent > GET_SKILL(ch, SKILL_PICK_LOCK))
               {
               improve_skill(ch,SKILL_PICK_LOCK,USE_FAIL);
               send_to_char(ch,"You failed to pick the lock.\r\n");
               }
            else
               {
               do_doorcmd(ch, item, t_alt, SCMD_PICK);
               improve_skill(ch,SKILL_PICK_LOCK,USE_PASS);
               }
            }
         break;
      default:
         log("SYSERR: process_skills, bad skill value(%d)", skill);
         return;
      }

   return;
   }


/*
 * Draw command for blade weapons worn at belt.
 * Torgny Bjers <artovil@arcanerealms.org>, 2002-12-16
 */
ACMD(do_draw)
   {
   struct obj_data *obj = NULL;
   char *arg = get_buffer(MAX_INPUT_LENGTH);

   any_one_arg(argument, arg);

   if (!*arg)
      send_to_char(ch, "Draw what?\r\n");
   else if ((GET_EQ(ch, WEAR_WIELD_1) && OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD_1), ITEM_TWO_HAND)) || 
            (GET_EQ(ch, WEAR_WIELD_1) && GET_EQ(ch, WEAR_WIELD_2)))
      send_to_char (ch, "You are already wielding weapons in both hands.\n\r");
   else if (FIGHTING(ch))
      send_to_char(ch, "You are too busy fighting to attempt that!\r\n");
   else 
      {
      int wear;
      /* Default to dual wield, if not, then wield normally. */
      if (!GET_EQ(ch, WEAR_WIELD_2) && GET_EQ(ch, WEAR_WIELD_1))
         {
         if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DUAL_WIELD) || !SCR_SKILLCHECK(ch, SKILL_DUAL_WIELD))
            {
            send_to_char(ch, "You are already wielding a weapon.\r\n");
            release_buffer(arg);
            return;
            }
         wear = WEAR_WIELD_2;
         }
      else
         wear = WEAR_WIELD_1;
      obj = GET_EQ(ch, WEAR_WAIST);
      /* If there is no sheath, we abort. */
      if (!obj) 
         {
         send_to_char(ch, "You do not seem to have a sheath at your waist.\r\n");
         release_buffer(arg);
         return;
         }
      else if (!isname("sheath", obj->name))
         {
         send_to_char(ch, "You do not have a sheath at your waist.\r\n");
         release_buffer(arg);
         return;
         }
      /* Check the object names. */
      if (!(obj->contains && isname(arg, obj->contains->name)))
         {
         /* Message if first item isn't weapon */
         if (obj->contains && obj->contains->next_content)
            send_to_char(ch, "You fumble through your sheath, looking for a weapon.\r\n");
         else
            send_to_char(ch, "There seems to be no such weapon in your sheath.\r\n");
         }
      else
         {
         /* We now assume that a weapon was found. */
         struct obj_data *weapon = obj->contains;

         /* Must be a weapon to be able to draw it. */
         if (GET_OBJ_TYPE(weapon) != ITEM_WEAPON)
            send_to_char(ch, "You can't wield that.\r\n");
         /* Must be able to wield to be able to draw. */
         else if (!CAN_WEAR(weapon, ITEM_WEAR_WIELD))
            send_to_char(ch, "You can't wield that.\r\n");
         /* Make sure that they can actually carry it too. */
         else if (GET_OBJ_WEIGHT(weapon) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
            send_to_char(ch, "It's too heavy for you to use.\r\n");
         else
            {
            if (!remove_otrigger(weapon, ch))
               return;
            /* Remove the object from sheath. */
            obj_from_obj(weapon);
            obj_to_char(weapon, ch);
            /* Wield the object. */
            perform_wear(ch, weapon, wear, FALSE);
            /* Let the player know that they did something. */
            act("You draw $p.", FALSE, ch, weapon, 0, TO_CHAR);
            act("$n draws $p.", TRUE, ch, weapon, 0, TO_ROOM);
            }
         }
      }
   release_buffer(arg);
   }

ACMD(do_guard)
{
  if (!argument || !*argument) {
    send_to_char(ch, "Guard who?\r\n");
    return;
  }
  skip_spaces(&argument);

  char *arg = get_buffer(MAX_INPUT_LENGTH);
  any_one_arg(argument, arg);

  if (!arg || !*arg) {
    send_to_char(ch, "Guard who?\r\n");
    release_buffer(arg);
    return;
  }

  struct char_data *target = get_char_vis(ch, arg, FIND_CHAR_ROOM);
  release_buffer(arg);

  if (!target) {
    send_to_char(ch, "That person is not here.\r\n");
    return;
  }

  if (!IS_NPC(ch) && IS_NPC(target)) {
    /* PCs can only guard PCs.  Mobs can guard PCs or NPCs. */
    send_to_char(ch, "That person is not here.\r\n");
    return;
  }

  if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM)) {
    /* Charmies can't guard. */
    send_to_char(ch, "Sorry.\r\n");
    return;
  }

  if (!IS_NPC(ch) && (!SCR_SKILLCHECK(ch, SKILL_GUARD)||GET_SKILL(ch, SKILL_GUARD) < 25)) {
    /* I'm leaving this so ANY mob that isn't charmed can "guard".  But PCs must know the guard skill. */
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  if (GET_GUARDING(ch)) {
    /* Remove "ch" as a guarder of its target. */
    struct char_data *tch = GET_GUARDING(ch);
    act("You stop guarding $N.", TRUE, ch, 0, tch, TO_CHAR);
    act("$n stops guarding you.", TRUE, ch, 0, tch, TO_VICT);
    act("$n stops guarding $N.", TRUE, ch, 0, tch, TO_NOTVICT);
    int i;
    for (i = 0; i < GET_NUM_GUARDING_ME(tch) && ch != GET_GUARDING_ME(tch)[i]; i++);
    if (i == GET_NUM_GUARDING_ME(tch)) {
      log("SYSERR: do_guard(%s) is supposed to be guarding %s, but was not found on the list!", GET_NAME(ch), GET_NAME(tch));
    } else {
      for (; i < GET_NUM_GUARDING_ME(tch)-1; i++) {
	GET_GUARDING_ME(tch)[i] = GET_GUARDING_ME(tch)[i+1];
      }
      GET_NUM_GUARDING_ME(tch)--;
      GET_GUARDING_ME(tch) = (struct char_data **)realloc(GET_GUARDING_ME(tch), GET_NUM_GUARDING_ME(tch) * sizeof(struct char_data *));
      /* log("%s stopped guarding %s.", GET_NAME(ch), GET_NAME(tch)); */
    }
    if (ch == target) {
      /* "guard self" -> no more guarding. */
      GET_GUARDING(ch) = NULL;
      return;
    }
  }

  if (ch == target) {
    send_to_char(ch, "You're already looking out for number one!\r\n");
    return;
  }
  
  GET_GUARDING(ch) = target;
  act("You start guarding $N.", TRUE, ch, 0, target, TO_CHAR);
  act("$n starts guarding you.", TRUE, ch, 0, target, TO_VICT);
  act("$n starts guarding $N.", TRUE, ch, 0, target, TO_NOTVICT);
  /* log("%s starts guarding %s.", GET_NAME(ch), GET_NAME(target)); */

  GET_NUM_GUARDING_ME(target)++;
  GET_GUARDING_ME(target) = (struct char_data **)realloc(GET_GUARDING_ME(target), GET_NUM_GUARDING_ME(target) * sizeof(struct char_data *));
  GET_GUARDING_ME(target)[GET_NUM_GUARDING_ME(target)-1] = ch;
}
