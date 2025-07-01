/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD * 
*  Usage: implementation of special procedures for mobiles/objects/rooms  * 
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
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "clan.h"
#include "constants.h"
#include "dg_scripts.h"
#include "shop.h"
#include "vnum.h"
/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern struct command_info cmd_info[];
extern struct zone_data *zone_table;
extern int min_kills;
extern const int exp_table[];
extern const float class_exp_multipliers[];
extern const float race_exp_multipliers[];
extern struct spell_info_type *spells;
extern int guild_info[][3];
extern room_vnum mortal_start_room;
extern int top_of_zone_table;

extern int cmd_slap;
extern int cmd_tell;
/* extern functions */
void save_corpses(void);
void write_clan_file (void);
void str_add_spaces(char *source,int total_length);
char *fname(char *namelist);
int real_zone(int vnumber);
int find_first_step(room_rnum src, room_rnum target,long iFlag);
int min_level(struct char_data *ch,int spellnum);
int can_give_gold(struct char_data *ch, int amount);
char *stolower(char *str);

extern struct queue_event *add_function_to_queue(time_t, struct char_data *,
                                                 long, int num_variables,
                                                 void (*)(), ...);


int total_repair = 0;
int total_recharge = 0;

ACMD(do_move);
ACMD(do_tell);
ACMD(do_say);
ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_gen_comm);
ACMD(do_action);
ACMD(do_insult);
ACMD(do_say);
ACMD(do_bash);
ACMD(do_kick);
ACMD(do_gen_com);
SPECIAL(moods);

struct social_type
   {
   char *cmd;
   int next_line;
   } ;


/* ********************************************************************
*  Special procedures for mobiles                                     * 
******************************************************************** */

int spell_sort_info[MAX_SPELLS+1];


void sort_spells(void)
   {
   int a, b, tmp;

   /* initialize array */
   for (a = 1; a < MAX_SPELLS; a++)
      spell_sort_info[a] = a;

   /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
   for (a = 1; a < MAX_SPELLS - 1; a++)
      for (b = a + 1; b < MAX_SPELLS; b++)
         if (strcmp(spells[spell_sort_info[a]].spell_name, spells[spell_sort_info[b]].spell_name)>0)
            {
            tmp = spell_sort_info[a];
            spell_sort_info[a] = spell_sort_info[b];
            spell_sort_info[b] = tmp;
            }
   }

int gold_check(int cost,struct char_data *ch)
   {
   if((GET_GOLD(ch)>=cost)||(GET_LEVEL(ch)>LVL_IMMORT))
      return(1);
   send_to_char(ch,"You cannot afford that.\r\n");
   return(0);
   }

/* The following is the archer code from the snippets page --Erika */

#define NUM_ARCHERS      2              /* # of rooms archers can shoot from */
#define NUM_TARGETS      3              /* # of rooms an archer can shoot at */

SPECIAL(archer)
   {
   struct char_data *targ;
   int i, j, k;
   int ar_damage;

   int fire_val[]={0,4,5};

   char *mssgs[] =
      {
         "You feel a sharp pain in your side as an arrow finds its mark!",
         "You hear a dull thud as an arrow pierces $N!",
         "An arrow whistles by your ear, barely missing you!",
         "An arrow narrowly misses $N!"
      }
      ;

   if(cmd)
      return FALSE;

   if(GET_POS(ch) != POS_STANDING)
      return FALSE;

   for(i = 0; i < 3; i++)
      {
      if(real_room(GET_MOB_VAL(ch,fire_val[i])) == IN_ROOM(ch))
         {
         for(j = 6; j <= 8; j++)
            {
            if((GET_MOB_VAL(ch,j)>0) && ((k = real_room(GET_MOB_VAL(ch,j))) > 0))
               {
               for(targ = world[k].people; targ; targ = targ->next_in_room)
                  {
                  if(!IS_NPC(targ) && (GET_LEVEL(targ) < LVL_IMMORT) &&
                          (!number(0, 3)))
                     {
                     act("$n fires an arrow at $N!",TRUE,ch,0,targ,TO_ROOM);
                     act("$N fires an arrow at $n!",TRUE,targ,0,ch,TO_NOTVICT);
                     act("$n fires an arrow at you!",TRUE,ch,0,targ,TO_VICT);
                     if(number(1, 100) <= GET_MOB_VAL(ch,1))
                        {
                        act(mssgs[0], 1, ch, 0, targ, TO_VICT);
                        act(mssgs[1], 1, targ, 0, targ, TO_NOTVICT);
                        ar_damage = dice(GET_MOB_VAL(ch,2),GET_MOB_VAL(ch,3));
                        ar_damage += (number(1, 5));
                        damage(ch,targ,ar_damage,-1,IMM_PIERCE);
                        /*  these above numbers can be changed for different
                        *  damage levels. 
                        */
                        return TRUE;
                        }
                     else
                        {
                        act(mssgs[2], 1, ch, 0, targ, TO_VICT);
                        act(mssgs[3], 1, targ, 0, targ, TO_NOTVICT);
                        return TRUE;
                        }
                     }
                  }
               }
            }
         }
      }
   return FALSE;
   }


SPECIAL(home_keeper)
   {
   char *buf;
   int cost,zone;
   struct char_data *keeper =(struct char_data *)me;

   if((cmd<1)||(!CMD_IS("move")&&!CMD_IS("cost")&&!CMD_IS("show")))
      return FALSE;

   if (IS_NPC(ch) && ch->master)
      return TRUE;

   if (CMD_IS("move"))
      {
      zone=real_zone(GET_MOB_VAL(keeper,1));
      if(GET_HOME(ch)==GET_MOB_VAL(keeper,1))
         {
         buf=get_buffer(256);
         sprintf(buf,"%s You twit, you already live in %s!!",
                 GET_NAME(ch),zone_table[zone].name);
         do_tell(keeper,buf,cmd_tell,0);
         release_buffer(buf);
         return TRUE;
         }
      if(zone>0)
         {
         if(real_room(GET_MOB_VAL(keeper,1))<1)
            {
            log("SYSERR: bad room %ld for home_keeper: %ld",
                GET_MOB_VAL(keeper,1), GET_MOB_VNUM(keeper));
            return FALSE;
            }
         buf=get_buffer(256);
         cost=GET_LEVEL(ch)*GET_MOB_VAL(keeper,2);

         /* odinian, 10/25/99, CHA price adjustment */
         cost = price_adjust(ch, keeper, cost);

         if(GET_GOLD(ch)<cost)
            {
            sprintf(buf,"%s You don't have enough gold to move to %s!",
                    GET_NAME(ch),zone_table[zone].name);
            do_tell(keeper,buf,cmd_tell,0);
            }
         else
            {
            sprintf(buf,"%s Ok, your new home town is now %s.",GET_NAME(ch),
                    zone_table[zone].name);
            do_tell(keeper,buf,cmd_tell,0);
            GET_GOLD(ch)-=cost;
            GET_HOME(ch)=GET_MOB_VAL(keeper,1);
            }
         release_buffer(buf);
         }
      else
         {
         log("SYSERR: bad zone %ld for home_keeper: %ld",GET_MOB_VAL(keeper,1),
             GET_MOB_VNUM(keeper));
         return FALSE;
         }
      return TRUE;
      }
   else if(CMD_IS("cost"))
      {
      zone=real_zone(GET_MOB_VAL(keeper,1));
      if(zone>0)
         {
         buf=get_buffer(256);
         cost=GET_LEVEL(ch)*GET_MOB_VAL(keeper,2);

         /* odinian, 10/25/99, CHA price adjustmant */
         cost = price_adjust(ch, keeper, cost);

         sprintf(buf,"%s It costs %d coins for you to move to %s.",
                 GET_NAME(ch),cost,zone_table[zone].name);
         do_tell(keeper,buf,cmd_tell,0);
         release_buffer(buf);
         }
      else
         {
         log("SYSERR: bad zone %ld for home_keeper: %ld",GET_MOB_VAL(keeper,1),
             GET_MOB_VNUM(keeper));
         return FALSE;
         }
      return TRUE;
      }
   else if(CMD_IS("show"))
      {
      zone=real_zone(GET_HOME(ch));
      if(zone<1)
         {
         log("SYSERR: bad home town %ld for player: %s, resetting",
             GET_HOME(ch),GET_NAME(ch));
         GET_HOME(ch)=mortal_start_room;
         }
      buf=get_buffer(256);
      zone=real_zone(GET_HOME(ch));
      sprintf(buf,"%s Your present residence is %s.",GET_NAME(ch),
              zone_table[zone].name);
      do_tell(keeper,buf,cmd_tell,0);
      release_buffer(buf);
      return TRUE;
      }
   else
      {
      return FALSE;
      }
   return FALSE;
   }




SPECIAL(mayor)
   {

   static char open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

   static char close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

   static char *path;
   static int mindex;
   static bool move = FALSE;

   if (!move)
      {
      if (time_info.hours == 6)
         {
         move = TRUE;
         path = open_path;
         mindex = 0;
         }
      else if (time_info.hours == 20)
         {
         move = TRUE;
         path = close_path;
         mindex = 0;
         }
      }
   if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
           (GET_POS(ch) == POS_FIGHTING)||(cmd<1))
      return FALSE;

   switch (path[mindex])
      {
   case '0':
   case '1':
   case '2':
   case '3':
      perform_move(ch, path[mindex] - '0', 1,0);
      break;

   case 'W':
      GET_POS(ch) = POS_STANDING;
      act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
      break;

   case 'S':
      GET_POS(ch) = POS_SLEEPING;
      act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
      break;

   case 'a':
      act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
      act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
      break;

   case 'b':
      act("$n says 'What a view!  I must get something done about that dump!'",
          FALSE, ch, 0, 0, TO_ROOM);
      break;

   case 'c':
      act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
          FALSE, ch, 0, 0, TO_ROOM);
      break;

   case 'd':
      act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
      break;

   case 'e':
      act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
      break;

   case 'E':
      act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
      break;

   case 'O':
      do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
      do_gen_door(ch, "gate", 0, SCMD_OPEN);
      break;

   case 'C':
      do_gen_door(ch, "gate", 0, SCMD_CLOSE);
      do_gen_door(ch, "gate", 0, SCMD_LOCK);
      break;

   case '.':
      move = FALSE;
      break;

      }

   mindex++;
   return FALSE;
   }


/* ********************************************************************
*  General special procedures for mobiles                             * 
******************************************************************** */


void npc_steal(struct char_data * ch, struct char_data * victim)
   {
   int gold;

   if (IS_NPC(victim))
      return;
   if (GET_LEVEL(victim) >= LVL_IMMORT)
      return;

   if (AWAKE(victim) && (number(0, GET_LEVEL(ch)) == 0))
      {
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
      }
   else
      {
      /* Steal some gold coins */
      gold = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
      if (gold > 0)
         {
         GET_GOLD(ch) += gold;
         GET_GOLD(victim) -= gold;
         }
      }
   }


SPECIAL(snake)
   {
   if (cmd)
      return FALSE;

   if (GET_POS(ch) != POS_FIGHTING)
      return FALSE;

   if (FIGHTING(ch) && (IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch)) &&
           (number(0, LVL_IMPL - GET_LEVEL(ch)) == 0))
      {
      act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
      act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
      call_magic(ch, FIGHTING(ch), 0,0,0, SPELL_POISON, (1+(GET_LEVEL(ch)/10)), CAST_SPELL);
      return TRUE;
      }
   return FALSE;
   }


SPECIAL(thief)
   {
   struct char_data *cons;

   if (cmd)
      return FALSE;

   if (GET_POS(ch) != POS_STANDING)
      return FALSE;

   for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
      if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_IMMORT) && (!number(0, 4)))
         {
         npc_steal(ch, cons);
         return TRUE;
         }
   return FALSE;
   }


SPECIAL(magic_user)
   {
   struct char_data *vict;

   if (cmd || GET_POS(ch) != POS_FIGHTING)
      return FALSE;

   if(IS_CASTING(ch))
      return FALSE;

   /* pseudo-randomly choose someone in the room who is fighting me */
   for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
      if (FIGHTING(vict) == ch && !number(0, 4))
         break;

   /* if I didn't pick any of those, then just slam the guy I'm fighting */
   if ((vict == NULL) && IN_ROOM(FIGHTING(ch))==IN_ROOM(ch))
      vict = FIGHTING(ch);

   if(vict==NULL)
      return TRUE;

   if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
      cast_spell(ch, vict, NULL, NULL,NULL, SPELL_SLEEP,((GET_LEVEL(ch)/10)+1));

   if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
      cast_spell(ch, vict, NULL, NULL,NULL, SPELL_BLINDNESS,((GET_LEVEL(ch)/10)+1));

   if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0))
      {
      if (IS_EVIL(ch))
         cast_spell(ch, vict, NULL, NULL,NULL, SPELL_ENERGY_DRAIN,((GET_LEVEL(ch)/10)+1));
      else if (IS_GOOD(ch))
         cast_spell(ch, vict, NULL, NULL,NULL, SPELL_DISPEL_EVIL,((GET_LEVEL(ch)/10)+1));
      }
   if (number(0, 4))
      return TRUE;

   switch (GET_LEVEL(ch))
      {
   case 4:
   case 5:
      cast_spell(ch, vict, NULL, NULL,NULL, SPELL_MAGIC_MISSILE,((GET_LEVEL(ch)/10)+1));
      break;
   case 6:
   case 7:
      cast_spell(ch, vict, NULL, NULL,NULL, SPELL_CHILL_TOUCH,((GET_LEVEL(ch)/10)+1));
      break;
   case 8:
   case 9:
      cast_spell(ch, vict, NULL, NULL,NULL, SPELL_BURNING_HANDS,((GET_LEVEL(ch)/10)+1));
      break;
   case 10:
   case 11:
      cast_spell(ch, vict, NULL, NULL,NULL, SPELL_SHOCKING_GRASP,((GET_LEVEL(ch)/10)+1));
      break;
   case 12:
   case 13:
      cast_spell(ch, vict, NULL, NULL,NULL, SPELL_LIGHTNING_BOLT,((GET_LEVEL(ch)/10)+1));
      break;
   case 14:
   case 15:
   case 16:
   case 17:
      cast_spell(ch, vict, NULL, NULL,NULL, SPELL_COLOR_SPRAY,((GET_LEVEL(ch)/10)+1));
      break;
   default:
      cast_spell(ch, vict, NULL, NULL, NULL,SPELL_FIREBALL,((GET_LEVEL(ch)/10)+1));
      break;
      }
   return TRUE;

   }

SPECIAL(priest)
   {
   struct char_data *vict;

   if (cmd || GET_POS(ch) != POS_FIGHTING)
      return FALSE;

   /* pseudo-randomly choose someone in the room who is fighting me */
   for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
      if (FIGHTING(vict) == ch && !number(0, 4))
         break;

   /* if I didn't pick any of those, then just slam the guy I'm fighting */
   if (vict == NULL)
      vict = FIGHTING(ch);

   if(IS_CASTING(ch))
      return FALSE;



   switch (GET_LEVEL(ch))
      {
   case 8:
   case 9:
      cast_spell(ch, ch, NULL, NULL, NULL,SPELL_CURE_LIGHT,((GET_LEVEL(ch)/10)+1));
      break;
   case 10:
   case 11:
      cast_spell(ch, ch, NULL, NULL, NULL,SPELL_BLESS,((GET_LEVEL(ch)/10)+1));
      break;
   case 12:
   case 13:
      cast_spell(ch, vict, NULL, NULL, NULL,SPELL_POISON,((GET_LEVEL(ch)/10)+1));
      break;
   case 16:
      if (number(0, 4))
         if (affected_by_spell(ch, SPELL_POISON))
            cast_spell(ch, ch, NULL,NULL,  NULL,SPELL_REMOVE_POISON,((GET_LEVEL(ch)/10)+1));
   case 17:
      cast_spell(ch, vict, NULL, NULL, NULL,SPELL_CALL_LIGHTNING,((GET_LEVEL(ch)/10)+1));
      break;
   case 18:
   case 19:
      if (number(0, 4))
         if (affected_by_spell(ch, SPELL_CURSE))
            cast_spell(ch, ch, NULL, NULL, NULL,SPELL_REMOVE_CURSE,((GET_LEVEL(ch)/10)+1));
   case 20:
   case 21:
      cast_spell(ch, vict, NULL, NULL,NULL, SPELL_LIGHTNING_BOLT,((GET_LEVEL(ch)/10)+1));
      break;
   case 22:
   case 23:
   case 24:
      if (number(0, 4))
         cast_spell(ch, ch, NULL, NULL, NULL,SPELL_HEAL,((GET_LEVEL(ch)/10)+1));
      else
         cast_spell(ch, vict, NULL, NULL, NULL,SPELL_LIGHTNING_BOLT,((GET_LEVEL(ch)/10)+1));
      break;
   default:
      cast_spell(ch, vict, NULL, NULL, NULL,SPELL_HARM,((GET_LEVEL(ch)/10)+1));
      if ((number(0,4)) == 4)
         do_say(ch, "May god have mercy on your soul.", 0, 0);
      break;
      }
   return TRUE;

   }


/* ********************************************************************
*  Special procedures for mobiles                                      * 
******************************************************************** */

SPECIAL(guild_guard)
   {
   struct char_data *guard = (struct char_data *) me;
   char *buf = "$n humiliates you, and blocks your way.\r\n";
   char *buf2 = "$n humiliates $N, and blocks $S way.";

   if ((cmd<1)||!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
      return FALSE;

   if (GET_LEVEL(ch) >= LVL_IMMORT)
      return FALSE;

   if ((((GET_CLASS(ch)+1)!=GET_MOB_VAL(guard,3)) &&
           ((GET_CLASS(ch)+1)!=GET_MOB_VAL(guard,4))&&
           ((GET_CLASS(ch)+1)!=GET_MOB_VAL(guard,5))&&
           ((GET_CLASS(ch)+1)!=GET_MOB_VAL(guard,6))&&
           ((GET_CLASS(ch)+1)!=GET_MOB_VAL(guard,7))&&
           ((GET_CLASS(ch)+1)!=GET_MOB_VAL(guard,8)))&&
           (GET_ROOM_VNUM(IN_ROOM(ch)) == GET_MOB_VAL(guard,1)) &&
           (cmd ==GET_MOB_VAL(guard,2))&&
           CAN_SEE(guard,ch))
      {
      act(buf, FALSE, guard, 0, ch, TO_CHAR);
      act(buf2, FALSE, guard, 0, ch, TO_ROOM);
      return TRUE;
      }
   return FALSE;
   }



SPECIAL(puff)
   {

   if (cmd)
      return (0);

   switch (number(0, 60))
      {
   case 0:
      do_say(ch, "My god!  It's full of stars!", 0, 0);
      return (1);
   case 1:
      do_say(ch, "How'd all those fish get up here?", 0, 0);
      return (1);
   case 2:
      do_say(ch, "I'm a very female dragon.", 0, 0);
      return (1);
   case 3:
      do_say(ch, "I've got a peaceful, easy feeling.", 0, 0);
      return (1);
   default:
      return (0);
      }
   }



SPECIAL(fido)
   {

   struct obj_data *i, *temp, *next_obj;
   int count = 0, tempcount;
   char *buf;

   if (cmd || !AWAKE(ch))
      return (FALSE);

   for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
      {
      if (GET_OBJ_VAL(i, 3) && (GET_OBJ_TYPE(i)==ITEM_CONTAINER) &&
          IS_OBJ_STAT(i, ITEM_PC_CORPSE))
         {
         if ((temp = i->contains) && (GET_LEVEL(ch) > 16) &&
             (GET_OBJ_VAL(i,7) > 15) && (number(1, 3) == 1))
            {
            buf = get_buffer(256);
            sprintf(buf,"$n sneaks up and savagely takes a bite out of %s, then scampers off.", 
                    GET_OBJ_NAME(i));
            act(buf, FALSE, ch, 0, 0, TO_ROOM);
            release_buffer(buf);
            for (temp = i->contains; temp; temp = temp->next_content)
               count++;
            tempcount = number(1, count);
            count = 1;
            for (temp = i->contains; temp&&(count!=tempcount); temp = temp->next_content)
               count++;
            obj_from_obj(temp);
            obj_to_char(temp, ch); 
            mudlogf(CMP,LVL_DETY,TRUE,"CORPSE: %s has nibbled %s from %s.", GET_NAME(ch),
                    GET_OBJ_NAME(temp), GET_OBJ_NAME(i));
            save_corpses();
            return (TRUE);
            }
         return (FALSE);
         }         
      if (IS_CORPSE(i))
         {
         act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
         for (temp = i->contains; temp; temp = next_obj)
            {
            next_obj = temp->next_content;
            obj_from_obj(temp);
            obj_to_room(temp, IN_ROOM(ch));
            }
         extract_obj(i);
         return (TRUE);
         }
      }
   return (FALSE);
   }



SPECIAL(janitor)
   {
   struct obj_data *i;

   if (cmd || !AWAKE(ch))
      return (FALSE);

   for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
      {
      if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
         continue;
      if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
         continue;
      if (GET_OBJ_VAL(i, 3) && (GET_OBJ_TYPE(i)==ITEM_CONTAINER))
         continue;
      act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
      obj_from_room(i);
      obj_to_char(i, ch);
      return TRUE;
      }

   return FALSE;
   }


SPECIAL(cityguard)
   {
   struct char_data *tch, *evil,*subj,*tmp,*victim;
   int dir;

   subj=(struct char_data *) me;
   if(!cmd)
      {
      if(GET_MOB_VAL(subj,1)>0)
         GET_MOB_VAL(subj,1)--;
      else if(MOB_FLAGGED(subj,MOB_SENTINEL) && !AFF_FLAGGED(subj,AFF_GROUP) &&
              !FIGHTING(subj) && (IN_ROOM(subj)>=0) &&
              (GET_MOB_VAL(subj,0)!=GET_ROOM_VNUM(IN_ROOM(subj))))
         {
         dir=find_first_step(IN_ROOM(subj),real_room(GET_MOB_VAL(subj,0)),
                             IGNORE_NOTRACK|IGNORE_ZNOTRACK);
         if(dir>=0)
            perform_move(subj,dir,1,0);
         return FALSE;
         }
      }

   if(GET_MOB_VAL(ch,1)==1)
      return FALSE;

   if (((cmd>0)&&!CMD_IS("steal")) || !AWAKE(subj) || FIGHTING(subj))
      return FALSE;

   evil = 0;


   if((ch==subj)&&(cmd==SPEC_ARRIVE))
      {
      for (tch = world[IN_ROOM(subj)].people; tch; tch = tch->next_in_room)
         {
         if(tch==subj)
            continue;
         if(IS_MOB(tch)&&(mob_index[GET_MOB_RNUM(tch)].func==cityguard))
            {
            if(GET_LEVEL(subj)>GET_LEVEL(tch))
               {
               act("$n salutes $N.",FALSE,tch,0,subj,TO_ROOM);
               act("$n returns the salute.",FALSE,subj,0,0,TO_ROOM);
               return FALSE;
               }
            else if(GET_LEVEL(subj) < GET_LEVEL(tch))
               {
               act("$n salutes $N.",TRUE,subj,0,tch,TO_ROOM);
               act("$n returns the salute.",TRUE,tch,0,0,TO_ROOM);
               return FALSE;
               }
            else if(!number(0,5))
               {
               act("$n asks $N 'Is everything ok?'",TRUE,subj,0,tch,TO_ROOM);
               act("$n nods his head.",TRUE,tch,0,0,TO_ROOM);
               return FALSE;
               }
            }
         }
      return FALSE;
      }
   else if(cmd<0)
      return FALSE;

   if((cmd>0)&&CMD_IS("steal") && !number(0,3))
      {
      char *victim_name=get_buffer(256);
      char *obj_name=get_buffer(256);
      char *buf=get_buffer(512);

      strcpy(buf, argument);
      one_argument(one_argument(buf, obj_name), victim_name);
      release_buffer(buf);
      release_buffer(obj_name);
      if (!(victim = get_char_room_vis(ch, victim_name)))
         {
         release_buffer(victim_name);
         return(FALSE);
         }
      release_buffer(victim_name);
      if (!IS_NPC(victim))
         {
         return(FALSE);
         }
      if (IS_NPC(victim) && !MOB_FLAGGED(victim, MOB_CITIZEN))
         {
         return(FALSE);
         }
      else if ((subj == victim)&&number(0,3))
         {
         buf=get_buffer(512);
         sprintf(buf,"Don't be such a twerp %s.",GET_NAME(ch));
         do_say(subj,buf,0,0);
         release_buffer(buf);
         return(TRUE);
         }
      else
         {
         act("$n screams, 'STOP THIEF!'",TRUE,subj,0,0,TO_ROOM);
         hit(subj, ch, TYPE_UNDEFINED);
         return(FALSE);
         }
      }


   for (tch = world[IN_ROOM(subj)].people; tch; tch = tch->next_in_room)
      {
      if (tch == subj)
         continue;
      if (!IS_NPC(tch) && (GET_LEVEL(tch) < LVL_IMMORT))
         {
         if(PLR_FLAGGED(tch,PLR_KILLER))
            {
            act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
            hit(subj, tch, TYPE_UNDEFINED);
            return (TRUE);
            }
         else if(PLR_FLAGGED(tch,PLR_THIEF))
            {
            act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
            hit(subj, tch, TYPE_UNDEFINED);
            return (TRUE);
            }
         }
      if (IS_UNDEAD(tch))
         {
         do_gen_comm(subj,"DIE UNCLEAN CREATURE!!!!",0,SCMD_SHOUT);
         hit(subj,tch, TYPE_UNDEFINED);
         return(TRUE);
         }
      if(AFF_FLAGGED(tch,AFF_PLAGUE))
         {
         if(GET_LEVEL(tch) > GET_LEVEL(subj)+20)
            {
            char *buf=get_buffer(512);
            sprintf(buf,"Alert!! %s has the Plague!!! Send Help!!.",GET_NAME(tch));
            do_gen_comm(subj,buf,0,SCMD_SHOUT);
            release_buffer(buf);
            }
         else
            {
            do_gen_comm(subj,"YOU SHALL NOT SPREAD YOUR FILTH FURTHER!!!!",0,SCMD_SHOUT);
            hit(subj,tch, TYPE_UNDEFINED);
            return(TRUE);
            }
         }
      }


   for (tch = world[IN_ROOM(subj)].people; tch; tch = tch->next_in_room)
      {
      if (IS_NPC(tch) && CAN_SEE(subj, tch) && FIGHTING(tch) &&
              (MOB_FLAGGED(tch,MOB_CITIZEN) ||
               (mob_index[GET_MOB_RNUM(tch)].func==cityguard))&&
               !AFF_FLAGGED(tch,AFF_PLAGUE))
         {
         for(tmp=world[IN_ROOM(subj)].people;tmp;tmp=tmp->next_in_room)
            {
            if(IS_NPC(tmp)&&FIGHTING(tmp)&&(FIGHTING(tmp)==tch)&& 
               !(mob_index[GET_MOB_RNUM(tmp)].func==cityguard))
               {
               evil=tmp;
               break;
               }
            }
         }
      }

   if (evil && (GET_ALIGNMENT(FIGHTING(evil)) >= 0))
      {
      act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  SPOON!!!!'",
          FALSE, subj, 0, 0, TO_ROOM);
      hit(subj, evil, TYPE_UNDEFINED);
      return (TRUE);
      }
   return (FALSE);
   }


SPECIAL(leviathan)
   {
   struct char_data *vict;

   if (cmd || GET_POS(ch) != POS_FIGHTING)
      return FALSE;


   /* pseudo-randomly choose someone in the room who is fighting me */
   for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
      if (FIGHTING(vict) == ch && !number(0, 4))
         break;

   /* if I didn't pick any of those, then just slam the guy I'm fighting */
   if (vict == NULL)
      vict = FIGHTING(ch);

   if (!vict)
      return FALSE;

   if(IS_CASTING(ch))
      return FALSE;

   switch(number(0,20))
      {
   case 1:
      act("$n utters the words, 'transvecta aqua'.",1,ch,0,0,TO_ROOM);
      cast_spell(ch, vict, NULL, NULL, NULL,SPELL_ICE_STORM,((GET_LEVEL(ch)/10)+1));
      break;
   case 2:
      act("$n utters the words, 'transvecta dirizt'.",1,ch,0,0,TO_ROOM);
      cast_spell(ch, vict, NULL, NULL, NULL,SPELL_METEOR_STORM,((GET_LEVEL(ch)/10)+1));
      break;
   case 3:
      act("$n utters the words, 'transvecta aerhta'.",1,ch,0,0,TO_ROOM);
      cast_spell(ch, vict, NULL, NULL, NULL,SPELL_EARTHQUAKE,((GET_LEVEL(ch)/10)+1));
      break;
   case 6:
      act("$n looks at you with the deepest sorrow.",1,ch,0,0,TO_ROOM);
      break;
   case 12:
      act("$n utters the words, 'transvecta talon'.", 1,ch,0,0,TO_ROOM);
      cast_spell(ch, ch, NULL, NULL, NULL,SPELL_HEAL,((GET_LEVEL(ch)/10)+1));
      break;
   default:
      break;

      }
   return(TRUE);
   }

SPECIAL(sea_serpent)
   {
   struct char_data *vict;

   if (cmd || GET_POS(ch) != POS_FIGHTING)
      return FALSE;


   /* pseudo-randomly choose someone in the room who is fighting me */
   for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
      if (FIGHTING(vict) == ch && !number(0, 4))
         break;

   /* if I didn't pick any of those, then just slam the guy I'm fighting */
   if (vict == NULL)
      vict = FIGHTING(ch);

   if (!vict)
      return FALSE;

   if(IS_CASTING(ch))
      return FALSE;

   if ((GET_LEVEL(ch)<10) && (number(0,100)==10))
      {
      act("$n utters the words, 'hisssssss'.",1, ch, 0, 0, TO_ROOM);
      cast_spell(ch, vict, NULL, NULL, NULL,SPELL_FIREBALL,((GET_LEVEL(ch)/10)+1));
      }
   else if (number(0,100)<=15)
      {
      act("$n utters the words, 'hissssss'.",1, ch, 0,0, TO_ROOM);
      cast_spell(ch, vict, NULL, NULL, NULL,SPELL_FIREBALL,((GET_LEVEL(ch)/10)+3));
      }
   return(TRUE);
   }




/* The following is butcher code from the snippets page --Erika */

#define NUM_BODY_PARTS 5

static char *headers[] =
   {
      "the corpse of The ",
      "the corpse of the ",
      "the corpse of an ",
      "the corpse of An ",
      "the corpse of a ",
      "the corpse of A ",
      "the corpse of "
   } ;

char *body_part[]=
   {
      "an arm of",
      "a leg of",
      "a foot of",
      "a rib of",
      "a hand of",
      "\n"
   } ;

char *body_word[]=
   {
      "arm",
      "leg",
      "foot",
      "rib",
      "hand",
      "\n"
   } ;

SPECIAL(butcher)
   {
   struct obj_data *obj;
   char *loc_buf;
   char *loc_buf2;
   char *buf;
   int i=0, len=0, total_len=0, done=FALSE, part=0;

   if(!cmd&&!FIGHTING((struct char_data *)me))
      {
      if(number(1,50)<2)
         {
         act("$n says 'Anyone have a corpse for me? I can butcher them "
             "for food!'", FALSE, me, 0, 0, TO_ROOM);
         return 1;
         }
      return 0;
      }

   if(CMD_IS("butcher"))
      {
      buf=get_buffer(MAX_INPUT_LENGTH);
      argument=one_argument(argument,buf);
      if(strcmp(buf,"corpse")==0)
         {
         release_buffer(buf);
         obj=get_obj_in_list_vis(ch,"corpse",ch->carrying);
         if(!obj)
            {
            act("$N says to you 'Sorry. You don't seem to have"
                " any corpses.'",FALSE,ch,0,me,TO_CHAR);
            return TRUE;
            }

         if(GET_GOLD(ch)<500)
            {
            act("$N says to you 'Sorry. You don't have"
                " enough gold.'",FALSE,ch,0,me,TO_CHAR);
            return TRUE;
            }
         else
            {
            part=number(1,NUM_BODY_PARTS)-1;
            for (i = 0; (i < 7) && (!done); i++)
               {
               len=strlen(headers[i]);
               if(memcmp(obj->short_description,headers[i],len)==0)
                  {

                  total_len=strlen(obj->short_description);

                  loc_buf=get_buffer(MAX_STRING_LENGTH);
                  loc_buf2=get_buffer(MAX_STRING_LENGTH);

                  strncpy(loc_buf,obj->short_description+len,
                          total_len-len);

                  free(obj->name);
                  sprintf(loc_buf2,"%s %s",body_word[part],loc_buf);
                  obj->name=str_dup(loc_buf2);

                  sprintf(loc_buf2,"%s %s lies here.",
                          body_part[part],loc_buf);
                  free(obj->description);
                  obj->description=str_dup(loc_buf2);

                  sprintf(loc_buf2,"%s %s",body_part[part],loc_buf);
                  free(obj->short_description);
                  obj->short_description=str_dup(loc_buf2);

                  GET_OBJ_TYPE(obj)=ITEM_FOOD;
                  GET_OBJ_WEAR(obj)=ITEM_WEAR_TAKE;
                  GET_OBJ_VAL(obj,0)=10; /* adjust this for fill */
                  GET_OBJ_VAL(obj,1)=0;
                  GET_OBJ_VAL(obj,2)=0;
                  GET_OBJ_VAL(obj,3)=0; /* adjust this for poison */
                  GET_OBJ_WEIGHT(obj)=5;  /* adjust this for weight */
                  GET_OBJ_COST(obj)=500;
                  GET_OBJ_RENT(obj)=100;  /* shouldn't be rentable..
                                              unless you make a change 
                                              to your mud to do so, 
                                              because corpses have a 
                                              vnum&rnum of -1 */
                  GET_OBJ_TIMER(obj)=-1;

                  GET_GOLD(ch)-=500;
                  done=TRUE;

                  act("$N says to you 'Done! Enjoy your meal!'",
                      FALSE,ch,0,me,TO_CHAR);
                  release_buffer(loc_buf2);
                  release_buffer(loc_buf);
                  return 1;
                  }
               }

            act("$N says to you 'Sorry, I can't make cuts with "
                "this.'",FALSE,ch,0,me,TO_CHAR);
            return 1;
            }
         }
      release_buffer(buf);
      return 0; /* if it wasn't corpse... it wasn't meant for me... */

      }
   return 0;

   }


/*************************************************************************
 *             General Procedures for 5 different types of breathing     *
 *              dragons, and one proc for a dragon which randomly        *
 *                breaths any of the five types. (great for tiamat)      *
 *************************************************************************/
SPECIAL(breath_fire)
   {
   if(cmd || GET_POS(ch) != POS_FIGHTING)
      return FALSE;
   if(number(0,4))
      return TRUE;
   call_magic(ch, NULL, NULL, NULL,NULL, SPELL_FIRE_BREATH, number(5,10),
              CAST_BREATH);
   return TRUE;
   }



SPECIAL(breath_gas)
   {
   if(cmd || GET_POS(ch) != POS_FIGHTING)
      return FALSE;
   if(number(0,4))
      return TRUE;
   call_magic(ch, NULL, NULL, NULL,NULL, SPELL_GAS_BREATH, number(5,10),
              CAST_BREATH);
   return TRUE;
   }

SPECIAL(breath_frost)
   {
   if(cmd || GET_POS(ch) != POS_FIGHTING)
      return FALSE;
   if(number(0,4))
      return TRUE;
   call_magic(ch, NULL, NULL,NULL,NULL,  SPELL_FROST_BREATH, number(5,10),
              CAST_BREATH);
   return TRUE;
   }

SPECIAL(breath_acid)
   {
   if(cmd || GET_POS(ch) != POS_FIGHTING)
      return FALSE;
   if(number(0,4))
      return TRUE;
   call_magic(ch, NULL, NULL, NULL,NULL, SPELL_ACID_BREATH, number(5,10),
              CAST_BREATH);
   return TRUE;
   }

SPECIAL(breath_lightning)
   {
   if(cmd || GET_POS(ch) != POS_FIGHTING)
      return FALSE;
   if(number(0,4))
      return TRUE;
   call_magic(ch, NULL, NULL,NULL,NULL, SPELL_LIGHTNING_BREATH,number(5,10),
              CAST_BREATH);
   return TRUE;
   }

SPECIAL(dragon)
   {
   struct char_data *vict;
   int i;

   if (cmd)
      return FALSE;

   if (GET_POS(ch) != POS_FIGHTING)
      return FALSE;

   if (!FIGHTING(ch))
      return FALSE;

   /* pseudo-randomly choose someone in the room who is fighting me */
   for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
      if (FIGHTING(vict) == ch && !number(0, 4))
         break;

   /* if I didn't pick any of those, then just slam the guy I'm fighting */
   if (vict == NULL)
      vict = FIGHTING(ch);

   if(IS_CASTING(ch))
      return FALSE;

   if (!vict)
      return FALSE;

   i = number(0,8);

   if (i == 0)
      {
      cast_spell(ch,vict,NULL,NULL, NULL,SPELL_FIRE_BREATH,((GET_LEVEL(ch)/10)+1));
      return TRUE;
      }

   if (i == 2)
      {
      cast_spell(ch,vict,NULL,NULL, NULL,SPELL_ACID_BREATH,((GET_LEVEL(ch)/10)+1));
      return TRUE;
      }

   if (i == 4)
      {
      cast_spell(ch,vict,NULL,NULL, NULL,SPELL_GAS_BREATH,((GET_LEVEL(ch)/10)+1));
      return TRUE;
      }

   if (i == 6)
      {
      cast_spell(ch,vict,NULL,NULL, NULL,SPELL_FROST_BREATH,((GET_LEVEL(ch)/10)+1));
      return TRUE;
      }

   if (i == 8)
      {
      cast_spell(ch,vict,NULL,NULL, NULL,SPELL_LIGHTNING_BREATH,((GET_LEVEL(ch)/10)+1));
      return TRUE;
      }

   return FALSE;

   }

/**************************************************************************
 *   Special Procedures for Heliopolis, Zone 30                             *
 *           incl. Robinhood, butch, trainer, healer, etc                 *
 **************************************************************************/
SPECIAL(robin_hood)
   {
   struct char_data *vict, *next_vict;
   int messg;
   int gold = 0;
   int to_room;
   char *buf;
   char *buf2;
   if (GET_POS(ch) == POS_STUNNED)
      GET_POS(ch) = POS_STANDING;

   if (number(0,100) >2)
      return FALSE;
   if ( (cmd) || (ch->master) )
      return FALSE;
   if (FIGHTING(ch))
      return FALSE;
   if (!ch || IN_ROOM(ch) == NOWHERE)
      return FALSE;
   buf=get_buffer(MAX_STRING_LENGTH);
   buf2=get_buffer(MAX_STRING_LENGTH);
   for (vict = character_list; vict; vict = next_vict)
      {
      next_vict = vict->next;
      if (IS_NPC(vict) ||
              (GET_LEVEL(vict) >= LVL_IMMORT) ||
              IN_ROOM(vict) == NOWHERE ||
              ROOM_FLAGGED(IN_ROOM(vict), ROOM_GODROOM) ||
              ROOM_FLAGGED(IN_ROOM(vict), ROOM_PRIVATE) ||
              !vict->desc ||
              GET_GOLD(vict)<200 ||
              GET_LEVEL(vict)<=4)
         continue;

      /* 11/07/96, Echo - empty buffer to eliminate 'noise' */
      *buf = '\0';
      if (number(0, 200) == 0)
         {
         char_from_room(ch);
         char_to_room(ch, IN_ROOM(vict));
         messg=number(0,4);
         if (messg==0 && mob_index[ch->nr].vnum==25901)
            sprintf(buf,"%s jumps down from above you.", GET_NAME(ch));
         else if (messg==1 && mob_index[ch->nr].vnum==25901)
            sprintf(buf,"%s swings in from above.",GET_NAME(ch));
         else if (messg==2 && mob_index[ch->nr].vnum==25901)
            sprintf(buf,"%s steps out in front of you with an arrow pointed at your chest.", GET_NAME(ch));
         else if (messg==3 && mob_index[ch->nr].vnum==25901)
            sprintf(buf,"%s springs up before you from the ground.",GET_NAME(ch));

         else if (messg==4 && mob_index[ch->nr].vnum==25901)
            sprintf(buf, "%s jumps out of the trees around you.",GET_NAME(ch));
         else if (mob_index[ch->nr].vnum==25902)
            sprintf(buf,"%s drives up in a mule drawn cart filled with beer.",
                    GET_NAME(ch));
         act(buf, FALSE, ch, 0, 0, TO_ROOM);

         messg=number(0,5);
         if (messg==0)
            sprintf(buf, "%s says, 'Thank you for contributing a small amount toward the less fortunate people of this realm.'",GET_NAME(ch));
         else if (messg==1)
            sprintf(buf,"%s says, 'Thank you ever so much we shall have a feast tonight fit for a king,'", GET_NAME(ch));
         else if (messg==2 || messg== 3)
            sprintf(buf,"%s says, 'Thanks and praises go out to the rich who aid my just cause.'",GET_NAME(ch));
         else
            sprintf(buf, "%s says, 'Your purse appears to be a bit full my good man'",GET_NAME(ch));

         act(buf, FALSE, ch, 0, 0, TO_ROOM);

         if (GET_GOLD(vict)>100)
            {
            gold = (int)(GET_GOLD(vict)/2);
            GET_GOLD(ch) = (GET_GOLD(ch) + gold);
            GET_GOLD(vict) -= gold;
            /* 11/07/96, Echo - show how much money was taken. */
            sprintf(buf2, "%s has borrowed %d coin%s from %s.",
                    GET_NAME(ch), gold, ((gold > 1) ? "s" :""),GET_NAME(vict));
            mudlog(buf2, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
            if (mob_index[ch->nr].vnum==25902)
               {
               sprintf(buf,"%s says, 'Here is a bottle of ale for your trouble my good man'", GET_NAME(ch));
               act(buf, FALSE, ch, 0, 0, TO_ROOM);
               sprintf(buf,"%s says, 'May the saints be with you.", GET_NAME(ch));
               act(buf, FALSE, ch, 0, 0, TO_ROOM);
               obj_to_char(read_object(real_object(1649),REAL),vict);
               }
            }
         if (mob_index[ch->nr].vnum==25902)
            sprintf(buf,"%s drives away on a mule drawn cart filled with beer.", GET_NAME(ch));
         else
            sprintf(buf,"The foliage covers %s as he leaps away. ",
                    GET_NAME(ch));
         act(buf, FALSE, ch, 0, 0, TO_ROOM);
         char_from_room(ch);
         char_to_room(ch, ch->orig_room);
         if (GET_GOLD(ch) > 100000)
            GET_GOLD(ch) = 100000;

         do
            {
            to_room = number(0, top_of_world);
            }
         while (IS_SET(world[to_room].room_flags, ROOM_PRIVATE | ROOM_DEATH));
         act("$n slowly fades out of existence and is gone.",FALSE, ch, 0,
             0, TO_ROOM);
         char_from_room(ch);
         char_to_room(ch, to_room);
         release_buffer(buf2);
         release_buffer(buf);
         return TRUE;
         }
      }
   if (GET_GOLD(ch)>0)
      for (vict = character_list; vict; vict = next_vict)
         {
         next_vict = vict->next;
         if (IS_NPC(vict) ||
                 (GET_LEVEL(vict) >= LVL_IMMORT) ||
                 IN_ROOM(vict) == NOWHERE ||
                 ROOM_FLAGGED(IN_ROOM(vict), ROOM_GODROOM) ||
                 ROOM_FLAGGED(IN_ROOM(vict), ROOM_PRIVATE) ||
                 !vict->desc ||
                 GET_GOLD(vict)>200  ||
                 GET_LEVEL(vict)>5     ||
                 GET_BANK_GOLD(vict)>1000 ||
                 TIMER(vict) > 1)
            continue;
         if (number(0, 300) == 0)
            {
            gold = 0;
            char_from_room(ch);
            char_to_room(ch, IN_ROOM(vict));
            if (mob_index[ch->nr].vnum==25902)
               sprintf(buf,"%s drives up on a mule drawn cart filled with beer.", GET_NAME(ch));
            else
               sprintf(buf, "%s jumps out of the trees around you.",
                       GET_NAME(ch));
            act(buf, FALSE, ch, 0, 0, TO_ROOM);
            sprintf(buf, "%s says, 'Well met %s. Your pockets appear to be a bit empty. Please let me take care of that.'", GET_NAME(ch), GET_NAME(vict));
            act(buf, FALSE, ch, 0, 0, TO_ROOM);
            sprintf(buf2, "%s has given money to %s.",GET_NAME(ch),
                    GET_NAME(vict));
            mudlog(buf2, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);

            if (GET_GOLD(ch)>200)
               {
               gold = 200;
               GET_GOLD(vict) += gold;
               GET_GOLD(ch) -= gold;
               }
            if (mob_index[ch->nr].vnum==25902)
               sprintf(buf,"%s drives away on a mule drawn cart filled with beer.", GET_NAME(ch));
            else
               sprintf(buf,"The foliage covers %s as he leaps away. ",
                       GET_NAME(ch));
            act(buf, FALSE, ch, 0, 0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, ch->orig_room);
            do
               {
               to_room = number(0, top_of_world);
               }
            while (IS_SET(world[to_room].room_flags,ROOM_PRIVATE|ROOM_DEATH));
            act("$n slowly fades out of existence and is gone.",FALSE, ch, 0,
                0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, to_room);
            release_buffer(buf2);
            release_buffer(buf);
            return TRUE;
            }
         }
   release_buffer(buf2);
   release_buffer(buf);
   return FALSE;
   }

/* Give the repair cost based on the status of the object.
   Added master and !repair costs - Nomikos 6/8/2025 */
int get_repair_cost(struct obj_data *obj,struct char_data *keeper,
                    struct char_data *ch)
   {
   int cost, original, master_base, reductions;
   float one_log_reduction;
   
   /* Base cost is here for now. Should probably put it somewhere more useful */
   master_base = 50000;
   /* Cost is doubled for !repair items */
   if (IS_OBJ_STAT(obj, ITEM_NO_REPAIR))
      master_base *= 2;
   
   /* Compare with the original material slots of the item */
   /* Note: If material strength changes that could cause an issue here */
   original = material_affs[obj->material].default_dam_slots;
   /* One log reduction is technically 90% but who cares */
   one_log_reduction = (float)original * 0.1;
   /* Each reduction is 10% off the _original_ strength, so can do it 10 times */
   reductions = (int)(((float)original - (float)GET_OBJ_OSLOTS(obj)) / one_log_reduction);
   
   /* If the item has total slots worn all the way down, or is !repair pay the master cost */
   /* Note that value of 2 will go down to -1, so if it less than 3 a master is needed */
   if ((GET_OBJ_TSLOTS(obj) < 3) || IS_OBJ_STAT(obj, ITEM_NO_REPAIR))
      {
      /* If the master has repaired it already, increase the price by an extra 50% each time.
      If not, then it's just the base price. */
      cost = master_base * (1 + (0.5 * reductions));
      }
   else
      {
      /* Added in a multiplier for each reduction. Starts at normal, then goes up */
      cost = (reductions + 1) * (GET_OBJ_OSLOTS(obj) + 5 * 
	     (GET_OBJ_TSLOTS(obj) - GET_OBJ_CSLOTS(obj)));
      }
   
   /* odinian, 10/25/99, CHA price adjustment */
   cost = price_adjust(ch, keeper, cost); /* -46% up to +20% based on Charisma */
   
   return cost;
   }

SPECIAL(repair_guy)
   {
   struct char_data *keeper = (struct char_data *) me;
   char *buf, *buf2;
   struct obj_data *obj;
   int cost,  original, reduction, condition;
   int weap_flags = 0, armr_flags = 0, jewl_flags = 0;

   if(cmd<1)
      return FALSE;
  
   if (!AWAKE(keeper))
      return (FALSE);

   if (CMD_IS("steal"))
      {
      char *argm = get_buffer(MAX_INPUT_LENGTH);
      sprintf(argm, "$N shouts '%s'", MSG_NO_STEAL_HERE);
      do_action(keeper, GET_NAME(ch), cmd_slap, 0);
      act(argm, FALSE, ch, 0, keeper, TO_CHAR);
      release_buffer(argm);
      return (TRUE);
      }

   /* Check if the repair guy is being coerced */
   if(IS_NPC(ch) && ch->master)
      return FALSE;

   /* Check for specialization. Some of these overlap */
   weap_flags = MOB2_FLAGGED(keeper, MOB2_WEAPONSMITH) ? 
          (ITEM_WEAR_WIELD | ITEM_WEAR_HOLD) : 0;
   armr_flags = MOB2_FLAGGED(keeper, MOB2_ARMORER) ?
          (ITEM_WEAR_BODY   | ITEM_WEAR_HEAD  | ITEM_WEAR_LEGS  | 
	   ITEM_WEAR_FEET   | ITEM_WEAR_HANDS | ITEM_WEAR_ARMS  | 
	   ITEM_WEAR_SHIELD | ITEM_WEAR_ABOUT | ITEM_WEAR_WAIST | 
	   ITEM_WEAR_BACK   | ITEM_WEAR_FACE) : 0;
   jewl_flags = MOB2_FLAGGED(keeper, MOB2_JEWELER) ? 
          (ITEM_WEAR_HOLD  | ITEM_WEAR_FINGER | ITEM_WEAR_NECK  | 
	   ITEM_WEAR_WRIST | ITEM_WEAR_EAR    | ITEM_WEAR_WAIST |
           ITEM_WEAR_FACE  | ITEM_WEAR_ABOUT  | ITEM_WEAR_HEAD) : 0;

   /* Fix, evaluate, or list? */
   if (CMD_IS("fix"))
      {
      buf = get_buffer(MAX_STRING_LENGTH);

      if (!(*argument))
         sprintf(buf, "%s What do you want me to evaluate??", 
	         GET_NAME(ch));
      else
	 {
         one_argument(argument, buf);
	  
         if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
            sprintf(buf,"%s Are you really sure you have that?", 
		    GET_NAME(ch));
	 else if (GET_OBJ_TSLOTS(obj) == INDESTRUCTABLE)
            sprintf(buf,"%s What a lucky find, that will never need repairing.",
                    GET_NAME(ch));
         else if (IS_OBJ_STAT(obj, ITEM_NO_REPAIR) && 
		  !MOB_FLAGGED(keeper, MOB_MASTER))
            sprintf(buf,"%s I am not skilled enough to repair %s.", 
		    GET_NAME(ch), GET_OBJ_NAME(obj));
	 else if ((GET_OBJ_CSLOTS(obj) == GET_OBJ_TSLOTS(obj)) && 
		  (GET_OBJ_TSLOTS(obj) > 0))
            sprintf(buf,"%s But %s is not damaged!", 
		    GET_NAME(ch), GET_OBJ_NAME(obj));
         else if ((weap_flags && !CAN_WEAR(obj, weap_flags)) || 
		  (armr_flags && !CAN_WEAR(obj, armr_flags)) ||
		  (jewl_flags && !CAN_WEAR(obj, jewl_flags)))
	    act("$n looks at $p and shakes $s head. They do not specialize in this type of equipment.", 
	        TRUE, keeper, obj, 0, TO_ROOM);
         else
	    {
            cost = get_repair_cost(obj,keeper,ch);

            if (GET_GOLD(ch) < cost)
               sprintf(buf,"%s You need at least %d gold on-hand to repair %s.",
                       GET_NAME(ch), cost, GET_OBJ_NAME(obj));
            else
	       {
	       *buf = '\0';

	       /* Subtract total slots. If all out, have a master fix it */
               GET_OBJ_TSLOTS(obj) -= 2;

               /* Added a fix to prevent the repair guy from getting confused */
	       /* that TSLOTS == 0 is indestructable */
	       if (GET_OBJ_TSLOTS(obj) == 0)
	          GET_OBJ_TSLOTS(obj)--;

	       /* Master-type skillz */
               if ((GET_OBJ_TSLOTS(obj) < 1) || (IS_OBJ_STAT(obj, ITEM_NO_REPAIR)))
	          {
                  if (!MOB_FLAGGED(keeper, MOB_MASTER))
                     act("$n tries to repair $p, but $e is not skilled enough!",
			 TRUE, keeper, obj, 0, TO_ROOM);
		  else
	             {
		     /* Master fixer dude */
		     original = material_affs[obj->material].default_dam_slots;
		     reduction = (float)original * 0.1;

		     /* Subtract 10% of default dam_slots, set the rest as max */
		     GET_OBJ_OSLOTS(obj) -= reduction;
		     GET_OBJ_CSLOTS(obj) = GET_OBJ_TSLOTS(obj) = GET_OBJ_OSLOTS(obj);

		     GET_GOLD(ch) -= cost;
                     total_repair += cost;

		     act("$N takes a small mountain of gold and $p from $n.",
		         TRUE, ch, obj, keeper, TO_ROOM);
                     send_to_char(ch,"%s takes %d gold and %s from you.\r\n",
		                  CAP(strdup(GET_NAME(keeper))), cost, GET_OBJ_NAME(obj));
					 
		     /* figure out the new condition for messaging */
		     condition = ((GET_OBJ_OSLOTS(obj) * 10) / original) + 2;

		     buf2 = get_buffer(MAX_STRING_LENGTH);
					 
		     /* Adjust these messages for master if needed */
		     if (condition > 9)
			sprintf(buf2, "$n repairs $p, restoring it to %s condition again!", 
			        stolower(strdup(item_condition_no_color[condition])));
		     else if (condition > 6)
			sprintf(buf2, "$n frowns at the %s condition of $p after completing $s work.", 
				stolower(strdup(item_condition_no_color[condition])));
		     else if (condition > 4)
			sprintf(buf2, "$p looks %s after $n repairs it, causing $m to shrug and chuckle.", 
			        stolower(strdup(item_condition_no_color[condition])));
		     else if (condition > 0)
			sprintf(buf2, "$p appears to be %s after $n does whatever $e does with it.", 
			        stolower(strdup(item_condition_no_color[condition])));
		     else
			/* Should never see this. Sorry in advance if you do... */
			sprintf(buf2, "$n shoves $p up your arse -better tell an imm-",
		                stolower(strdup(item_condition_no_color[condition])));
			
		     act(buf2, TRUE, keeper, obj, 0, TO_ROOM);
		     act("$N hands $p back to $n.", TRUE, ch, obj, keeper, TO_ROOM);
		     act("$N hands $p back to you.", TRUE, ch, obj, keeper, TO_CHAR);

		     release_buffer(buf2);
		     }
                  }
	       else
	          {
	          /* regular repair muchacho */
	          original = GET_OBJ_OSLOTS(obj);

	          /* Subtract current slots after total slots check */
	          GET_OBJ_CSLOTS(obj) = GET_OBJ_TSLOTS(obj);

	          /* Let's charge the player ONLY when we can fix it - Nomikos 5/10/25 */
                  GET_GOLD(ch) -= cost;
                  total_repair += cost;
				  
                  act("$N takes some gold and $p from $n.",
		      TRUE, ch, obj, keeper, TO_ROOM);
                  send_to_char(ch,"%s takes %d gold and %s from you.\r\n",
		               CAP(strdup(GET_NAME(keeper))), cost, GET_OBJ_NAME(obj));

	          /* figure out the new condition for messaging */
	          condition = ((GET_OBJ_TSLOTS(obj) * 10) / original) + 2;
	 
	          buf2 = get_buffer(MAX_STRING_LENGTH);
	          if (condition > 9)
	             sprintf(buf2, "$n repairs $p, restoring it to %s condition again!",  
		             stolower(strdup(item_condition_no_color[condition])));
	          else if (condition > 6)
	             sprintf(buf2, "$n frowns at the %s condition of $p after completing $s work.", 
	                     stolower(strdup(item_condition_no_color[condition])));
	          else if (condition > 4)
	              sprintf(buf2, "$p looks %s after $n repairs it, causing $m to shrug and chuckle.",  
		              stolower(strdup(item_condition_no_color[condition])));
	          else if (condition > 0)
		      sprintf(buf2, "$p appears to be %s after $n does whatever $e does with it.", 
		   	      stolower(strdup(item_condition_no_color[condition])));
	          else
		     /* Should never see this. */
		     sprintf(buf2, "$n broke something, but not $p. -tell an imm-",
			     stolower(strdup(item_condition_no_color[condition])));

	          act(buf2, TRUE, keeper, obj, 0, TO_ROOM);
	          act("$N hands $p back to $n.", TRUE, ch, obj, keeper, TO_ROOM);
	          act("$N hands $p back to you.", TRUE, ch, obj, keeper, TO_CHAR);

	          release_buffer(buf2);
	          }
               }
            }
         }
	  
      /* Send message. */
      if (*buf)
         do_tell(keeper, buf, cmd_tell, 0);
	  
      release_buffer(buf);
      return (TRUE);
      }
   else if (CMD_IS("value"))
      {
      buf=get_buffer(MAX_STRING_LENGTH);
      if(!(*argument))
         {
         sprintf(buf, "%s What do you want me to evaluate??", GET_NAME(ch));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      one_argument(argument,buf);
      if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
         {
         sprintf(buf,"%s Are you really sure you have that?",GET_NAME(ch));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      if (IS_OBJ_STAT(obj, ITEM_NO_REPAIR) && !MOB_FLAGGED(keeper, MOB_MASTER))
         {
         sprintf(buf,"%s I am not skilled enough to repair %s.", GET_NAME(ch),
                    GET_OBJ_NAME(obj));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      if ((weap_flags && !CAN_WEAR(obj, weap_flags)) || 
          (armr_flags && !CAN_WEAR(obj, armr_flags)) ||
	  (jewl_flags && !CAN_WEAR(obj, jewl_flags)))
	 {
	 act("$n looks at $p and shakes $s head. They do not specialize in this type of equipment.", 
	     TRUE, keeper, obj, 0, TO_ROOM);
         release_buffer(buf);
         return TRUE;
	 }
      if (GET_OBJ_TSLOTS(obj) == INDESTRUCTABLE)
         {
         sprintf(buf,"%s What a lucky find, that will never need repairing.",
                 GET_NAME(ch));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      if ((GET_OBJ_CSLOTS(obj) == GET_OBJ_TSLOTS(obj)) && (GET_OBJ_TSLOTS(obj) > 2))
         {
         sprintf(buf,"%s But %s is not damaged! STOP BOTHERING ME!",
                 GET_NAME(ch), GET_OBJ_NAME(obj));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }

      cost = get_repair_cost(obj,keeper,ch);

      sprintf(buf,"%s It will cost you %d gold to repair %s.",GET_NAME(ch),
              cost,GET_OBJ_NAME(obj));
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      return (TRUE);
      }
   else if (CMD_IS("list"))
      {
      send_to_char(ch, "Use fix <object> to get an object in your inventory fixed.\r\n");
      send_to_char(ch, "Use value <object> to see how much it will cost to fix an object.\r\n");
      if(GET_LEVEL(ch)>=LVL_IMMORT)
         {
         send_to_char(ch, "Total charged to the players: %d\r\n",total_repair);
         }
      return (TRUE);
      }

   return (FALSE);
   }

int get_recharge_cost(struct obj_data *obj, struct char_data *keeper,
                      struct char_data *ch)
   {
   int cost = 0;
   int base = 0;
   int num_charges = 0;
   int keeper_level = 0;
   int obj_level = 5;

   base = MAX(1,GET_MOB_VAL(keeper,1));
   num_charges = GET_OBJ_VAL(obj,1) - GET_OBJ_VAL(obj,2);
   keeper_level = min_level(keeper,GET_OBJ_VAL(obj,3));
   if(GET_OBJ_VAL(obj,0))
      obj_level = GET_OBJ_VAL(obj,0);

   cost = base * num_charges * keeper_level * obj_level;
   cost = price_adjust(ch,keeper,cost);
   return cost;
   }


SPECIAL(recharge_guy)
   {
   struct char_data *keeper = (struct char_data *) me;
   char *buf;
   struct obj_data *obj;
   int cost;

   if(cmd<1)
      return FALSE;
   if (!AWAKE(keeper))
      return (FALSE);

   if (CMD_IS("steal"))
      {
      char *argm = get_buffer(MAX_INPUT_LENGTH);
      sprintf(argm, "$N shouts '%s'", MSG_NO_STEAL_HERE);
      do_action(keeper, GET_NAME(ch), cmd_slap, 0);
      act(argm, FALSE, ch, 0, keeper, TO_CHAR);
      release_buffer(argm);
      return (TRUE);
      }
   if (CMD_IS("recharge"))
      {
      buf=get_buffer(MAX_STRING_LENGTH);
      if(!(*argument))
         {
         sprintf(buf, "%s What do you want me to evaluate??", GET_NAME(ch));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      one_argument(argument,buf);
      if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
         {
         sprintf(buf,"%s Are you really sure you have that?",GET_NAME(ch));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }

      if((GET_OBJ_TYPE(obj)!=ITEM_WAND)&&
              (GET_OBJ_TYPE(obj)!=ITEM_STAFF))
         {
         sprintf(buf,"%s But %s isn't a charged item.",
                 GET_NAME(ch),GET_OBJ_NAME(obj));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }

      if((cost = get_recharge_cost(obj,keeper,ch)) < 0)
         {
         sprintf(buf,"%s I don't know how to work the magic needed to "
                 "recharge %s.",GET_NAME(ch),GET_OBJ_NAME(obj));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }

      if(GET_GOLD(ch)<cost)
         {
         sprintf(buf,"%s You need at least %d gold on-hand to recharge %s.",
                 GET_NAME(ch),cost, GET_OBJ_NAME(obj));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }


      GET_GOLD(ch) -= cost;
      total_recharge += cost;
      act("$N take some gold and $p from $n.",TRUE,ch,obj,keeper,TO_ROOM);
      send_to_char(ch,"%s takes %d gold and %s from you.\r\n",GET_NAME(keeper),
                   cost,GET_OBJ_NAME(obj));


      if(GET_OBJ_TSLOTS(obj) != INDESTRUCTABLE)
         {
         GET_OBJ_TSLOTS(obj) -= 2;
         if(GET_OBJ_CSLOTS(obj) > GET_OBJ_TSLOTS(obj))
            {
            GET_OBJ_CSLOTS(obj) = GET_OBJ_TSLOTS(obj);
            }
         }
      GET_OBJ_VAL(obj,2)=GET_OBJ_VAL(obj,1);

      if(GET_OBJ_TSLOTS(obj) < 1)
         {
         act("$n tries to recharge $p, but it crumbles away!",TRUE,keeper,
             obj,0,TO_ROOM);
         release_buffer(buf);
         extract_obj(obj);
         return (TRUE);
         }
      act("$n recharges $p, making it as good as new again!", TRUE, keeper,
          obj, 0, TO_ROOM);
      act("$n hands $p back to $N.",TRUE,keeper,obj,ch,TO_ROOM);
      release_buffer(buf);
      return (TRUE);
      }
   else if (CMD_IS("value"))
      {
      buf=get_buffer(MAX_STRING_LENGTH);
      if(!(*argument))
         {
         sprintf(buf, "%s What do you want me to evaluate??", GET_NAME(ch));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      one_argument(argument,buf);
      if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
         {
         sprintf(buf,"%s Are you really sure you have that?",GET_NAME(ch));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }

      if((GET_OBJ_TYPE(obj)!=ITEM_WAND)&&
              (GET_OBJ_TYPE(obj)!=ITEM_STAFF))
         {
         sprintf(buf,"%s But %s isn't a charged item.",
                 GET_NAME(ch),GET_OBJ_NAME(obj));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }

      if((cost = get_recharge_cost(obj,keeper,ch)) < 0)
         {
         sprintf(buf,"%s I don't know how to work the magic needed to "
                 "recharge %s.",GET_NAME(ch),GET_OBJ_NAME(obj));
         do_tell(keeper, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }

      sprintf(buf,"%s It will cost you %d gold to recharge %s.",GET_NAME(ch),
              cost,GET_OBJ_NAME(obj));
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      return (TRUE);
      }
   else if (CMD_IS("list"))
      {
      send_to_char(ch, "Use recharge <object> to get an object in your inventory recharged.\r\n");
      send_to_char(ch, "Use value <object> to see how much it will cost to recharge an object.\r\n");
      if(GET_LEVEL(ch)>=LVL_IMMORT)
         {
         send_to_char(ch,"Total charged to the players: %d\r\n",
                      total_recharge);
         }
      return (TRUE);
      }

   return (FALSE);
   }


SPECIAL(stable_master)
   {
   struct char_data *stable_guy = (struct char_data*) me;
   struct char_data *horse;
   struct obj_data *obj;
   char *buf;
   /* make sure that cmd is in the cmd array and that it is one of the
   commands we wish to run, otherwise this proc will say that it did
   nothing (returns false) */

   if((cmd < 1) || (!CMD_IS("exchange") && !CMD_IS("stable")))
      return FALSE;
   buf=get_buffer(MAX_STRING_LENGTH);

   if(CMD_IS("exchange"))
      {
      if(!(*argument))
         {
         sprintf(buf, "%s What do you want me to exchange??", GET_NAME(ch));
         do_tell(stable_guy, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      one_argument(argument,buf);
      if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
         {
         sprintf(buf,"%s Are you really sure you have that?",GET_NAME(ch));
         do_tell(stable_guy, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      if(GET_OBJ_TYPE(obj)!=ITEM_STABLE_TICKET)
         {
         sprintf(buf,"%s I only exchange stable tickets.",GET_NAME(ch));
         do_tell(stable_guy, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      if ((horse = read_mobile(GET_OBJ_VAL(obj,0), VIRTUAL))==NULL)
         {
         sprintf(buf,"%s We're sorry that ticket doesn't seem to be working.",
                 GET_NAME(ch));
         log("SYSERR: Broken ticket: %ld", GET_OBJ_VNUM(obj));
         release_buffer(buf);
         return TRUE;
         }
      /* take & extract ticket (look at the junk code) */
      /* set horse to charmed, create the correct master/follower links. */
      act("You give $p to $N.",FALSE,ch,obj,stable_guy,TO_CHAR);
      act("$n gives $p to $N.",TRUE,ch,obj,stable_guy,TO_NOTVICT);
      obj_from_char(obj);
      extract_obj(obj);
      horse->orig_room=IN_ROOM(ch);
      char_to_room(horse,IN_ROOM(ch));
      act("$N calls out a number and a stable boy leads $n into the room.",FALSE,horse,0,stable_guy,TO_ROOM);
      load_mtrigger(horse);
      add_follower(horse, ch);
      release_buffer(buf);
      return TRUE;
      }
   else if(CMD_IS("stable"))
      {
      if(!(*argument))
         {
         sprintf(buf, "%s What do you want me to put into the stable??",
                 GET_NAME(ch));
         do_tell(stable_guy, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      one_argument(argument,buf);
      if(!(horse = get_char_room_vis(stable_guy,buf)))
         {
         sprintf(buf, "%s Sorry. You don't seem to have a horse.",
                 GET_NAME(ch));

         do_tell(stable_guy, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      if(!IS_NPC(horse)||!MOB_FLAGGED(horse,MOB_MOUNT))
         {
         sprintf(buf,"%s But %s isn't a mount.",GET_NAME(ch),
                 GET_NAME(horse));
         do_tell(stable_guy, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }


      if(horse->master!=ch)
         {
         sprintf(buf,"%s But that isn't your mount.",GET_NAME(ch));
         do_tell(stable_guy, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      /* might want to make this more sophisticated later*/
      obj = create_obj();
      obj->item_number = NOTHING;
      obj->name = str_dup("stable ticket");
      sprintf(buf,"a stable ticket for %s",GET_NAME(horse));
      obj->short_description = str_dup(buf);
      obj->description = str_dup("Someone has left a stable ticket here.");

      GET_OBJ_TYPE(obj) = ITEM_STABLE_TICKET;
      GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE | ITEM_WEAR_HOLD;
      GET_OBJ_WEIGHT(obj) = 1;
      GET_OBJ_RENT(obj) = 10;
      GET_OBJ_COST(obj) = 100; /*improve this later, have it be based off the horse */
      obj->action_description = NULL;
      obj->material = MATERIAL_PAPER;
      GET_OBJ_CSLOTS(obj) = material_affs[MATERIAL_PAPER].default_dam_slots;
      GET_OBJ_TSLOTS(obj) = material_affs[MATERIAL_PAPER].default_dam_slots;
      GET_OBJ_OSLOTS(obj) = material_affs[MATERIAL_PAPER].default_dam_slots;
      /* so it saves */
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);

      GET_OBJ_VAL(obj,0)=GET_MOB_VNUM(horse);

      act("$N snaps $S fingers and a stable boy comes out and leads $n into a stall.",TRUE,horse,0,stable_guy,
          TO_ROOM);
      REMOVE_BIT(AFF_FLAGS(horse), AFF_CHARM);
      stop_follower(horse);
      extract_char(horse);
      obj_to_char(obj, ch);
      act("$N hands you $p.",FALSE,ch,obj,stable_guy,TO_CHAR);
      act("$N hands $p to $n.",TRUE,ch,obj,stable_guy,TO_NOTVICT);
      /* remove and extract horse */
      /* give ticket character */
      release_buffer(buf);
      return TRUE;
      }
   release_buffer(buf);
   return FALSE;
   }

SPECIAL(sage)
   {
   struct char_data *sage_guy = (struct char_data*) me;
   char *buf;
   int base_cost = GET_MOB_VAL(sage_guy,1);
   int cost = 0;
   static int allchk=0,strchk=0,dexchk=0,conchk=0,intchk=0,wischk=0,chachk=0;
   struct obj_data *device = NULL;
   /* make sure that cmd is in the cmd array and that it is one of the
   commands we wish to run, otherwise this proc will say that it did
   nothing (returns false) */

   if((cmd < 1) || (!CMD_IS("list") && !CMD_IS("divine") && !CMD_IS("check")))
      return FALSE;

   if(CMD_IS("list"))
      {
      send_to_char(ch,"\r\n   ----------------------------------------------------------\r\n");
      send_to_char(ch, "  The cost to see ALL your stats is:    %d\r\n",
                   price_adjust(ch,sage_guy,base_cost));
      send_to_char(ch, "  The cost to see your STRENGTH is:     %d\r\n",
                   price_adjust(ch,sage_guy,base_cost/4));
      send_to_char(ch, "  The cost to see your DEXTERITY is:    %d\r\n",
                   price_adjust(ch,sage_guy,base_cost/4));
      send_to_char(ch, "  The cost to see your CONSTITUTION is: %d\r\n",
                   price_adjust(ch,sage_guy,base_cost/3));
      send_to_char(ch, "  The cost to see your WISDOM is:       %d\r\n",
                   price_adjust(ch,sage_guy,base_cost/4));
      send_to_char(ch, "  The cost to see your INTELLIGENCE is: %d\r\n",
                   price_adjust(ch,sage_guy,base_cost/4));
      send_to_char(ch, "  The cost to see your CHARISMA is:     %d\r\n",
                   price_adjust(ch,sage_guy,base_cost/5));
      send_to_char(ch,
                   "  Use \"divine [stat]\" to discover something about yourself\r\n");
      send_to_char(ch, "\r\n   ----------------------------------------------------------\r\n");
      return TRUE;
      }
   else if(CMD_IS("divine"))
      {
      buf=get_buffer(MAX_STRING_LENGTH);
      if(!(*argument))
         {
         sprintf(buf, "%s What do you want me to tell you about??",
                 GET_NAME(ch));
         do_tell(sage_guy, buf, cmd_tell, 0);
         release_buffer(buf);
         return TRUE;
         }
      if(GET_EQ(sage_guy,WEAR_HOLD_1))
         device = GET_EQ(sage_guy,WEAR_HOLD_1);
      one_argument(argument,buf);

      /*
      if(holding somethin)
      The Tarot Reader lays out the cards and studies them
      else
      ___ looks closely at you.
      */
      if(is_abbrev(buf,"all"))
         {
         cost = price_adjust(ch,sage_guy,base_cost);
         if(!gold_check(cost,ch))
            {
            release_buffer(buf);
            return TRUE;
            }
         if(device)
            {
            act("$n gazes at $s $o.",FALSE, sage_guy,device,ch,TO_ROOM);
            }
         else
            {
            act("$n gazes into your eyes.",FALSE, sage_guy,device,ch,TO_VICT);
            act("$n gazes into $N's eyes.",FALSE, sage_guy,device,ch,TO_NOTVICT);
            }
         sprintf(buf,"$n says, 'I see that your strength is %d, your dexterity is %d, your constitution is %d, your intelligence is %d, your wisdom is %d, and your charisma is %d.'\r\n",GET_STR(ch),GET_DEX(ch),GET_CON(ch),GET_INT(ch),GET_WIS(ch),GET_CHA(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_VICT);
         sprintf(buf,"$n says, 'I see that $N's strength is %d, $S dexterity is %d, $S constitution is %d, $S intelligence is %d, $S wisdom is %d, and $S charisma is %d.'\r\n",GET_STR(ch),GET_DEX(ch),GET_CON(ch),GET_INT(ch),GET_WIS(ch),GET_CHA(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_NOTVICT);
         allchk++;
         }
      else if(is_abbrev(buf,"strength"))
         {
         cost = price_adjust(ch,sage_guy,base_cost/4);
         if(!gold_check(cost,ch))
            {
            release_buffer(buf);
            return TRUE;
            }
         if(device)
            {
            act("$n gazes at $s $o.",FALSE, sage_guy,device,ch,TO_ROOM);
            }
         else
            {
            act("$n gazes into your eyes.",FALSE, sage_guy,device,ch,TO_VICT);
            act("$n gazes into $N's eyes.",FALSE, sage_guy,device,ch,TO_NOTVICT);
            }
         sprintf(buf,"$n says, 'I see that your strength is %d.'\r\n",
                 GET_STR(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_VICT);
         sprintf(buf,"$n says, 'I see that $N's strength is %d.'\r\n",
                 GET_STR(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_NOTVICT);
         strchk++;
         }
      else if(is_abbrev(buf,"dexterity"))
         {
         cost = price_adjust(ch,sage_guy,base_cost/4);
         if(!gold_check(cost,ch))
            {
            release_buffer(buf);
            return TRUE;
            }
         if(device)
            {
            act("$n gazes at $s $o.",FALSE, sage_guy,device,ch,TO_ROOM);
            }
         else
            {
            act("$n gazes into your eyes.",FALSE, sage_guy,device,ch,TO_VICT);
            act("$n gazes into $N's eyes.",FALSE, sage_guy,device,ch,TO_NOTVICT);
            }
         sprintf(buf,"$n says, 'I see that your dexterity is %d.'\r\n",
                 GET_DEX(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_VICT);
         sprintf(buf,"$n says, 'I see that $N's dexterity is %d.'\r\n",
                 GET_DEX(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_NOTVICT);
         dexchk++;
         }
      else if(is_abbrev(buf,"constitution"))
         {
         cost = price_adjust(ch,sage_guy,base_cost/3);
         if(!gold_check(cost,ch))
            {
            release_buffer(buf);
            return TRUE;
            }
         if(device)
            {
            act("$n gazes at $s $o.",FALSE, sage_guy,device,ch,TO_ROOM);
            }
         else
            {
            act("$n gazes into your eyes.",FALSE, sage_guy,device,ch,TO_VICT);
            act("$n gazes into $N's eyes.",FALSE, sage_guy,device,ch,TO_NOTVICT);
            }
         sprintf(buf,"$n says, 'I see that your constitution is %d.'\r\n",
                 GET_CON(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_VICT);
         sprintf(buf,"$n says, 'I see that $N's constitution is %d.'\r\n",
                 GET_CON(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_NOTVICT);
         conchk++;
         }
      else if(is_abbrev(buf,"wisdom"))
         {
         cost = price_adjust(ch,sage_guy,base_cost/4);
         if(!gold_check(cost,ch))
            {
            release_buffer(buf);
            return TRUE;
            }
         if(device)
            {
            act("$n gazes at $s $o.",FALSE, sage_guy,device,ch,TO_ROOM);
            }
         else
            {
            act("$n gazes into your eyes.",FALSE, sage_guy,device,ch,TO_VICT);
            act("$n gazes into $N's eyes.",FALSE, sage_guy,device,ch,TO_NOTVICT);
            }
         sprintf(buf,"$n says, 'I see that your wisdom is %d.'\r\n",
                 GET_WIS(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_VICT);
         sprintf(buf,"$n says, 'I see that $N's wisdom is %d.'\r\n",
                 GET_WIS(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_NOTVICT);
         wischk++;
         }
      else if(is_abbrev(buf,"intelligence"))
         {
         cost = price_adjust(ch,sage_guy,base_cost/4);
         if(!gold_check(cost,ch))
            {
            release_buffer(buf);
            return TRUE;
            }
         if(device)
            {
            act("$n gazes at $s $o.",FALSE, sage_guy,device,ch,TO_ROOM);
            }
         else
            {
            act("$n gazes into your eyes.",FALSE, sage_guy,device,ch,TO_VICT);
            act("$n gazes into $N's eyes.",FALSE, sage_guy,device,ch,TO_NOTVICT);
            }
         sprintf(buf,"$n says, 'I see that your intelligence is %d.'\r\n",
                 GET_INT(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_VICT);
         sprintf(buf,"$n says, 'I see that $N's intelligence is %d.'\r\n",
                 GET_INT(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_NOTVICT);
         intchk++;
         }
      else if(is_abbrev(buf,"charisma"))
         {
         cost = price_adjust(ch,sage_guy,base_cost/5);
         if(!gold_check(cost,ch))
            {
            release_buffer(buf);
            return TRUE;
            }
         if(device)
            {
            act("$n gazes at $s $o.",FALSE, sage_guy,device,ch,TO_ROOM);
            }
         else
            {
            act("$n gazes into your eyes.",FALSE, sage_guy,device,ch,TO_VICT);
            act("$n gazes into $N's eyes.",FALSE, sage_guy,device,ch,TO_NOTVICT);
            }
         sprintf(buf,"$n says, 'I see that your charisma is %d.'\r\n",
                 GET_CHA(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_VICT);
         sprintf(buf,"$n says, 'I see that $N's charisma is %d.'\r\n",
                 GET_CHA(ch));
         act(buf,FALSE,sage_guy,device,ch,TO_NOTVICT);
         chachk++;
         }
      else
         {
         release_buffer(buf);
         return TRUE;
         }
      GET_GOLD(ch) = GET_GOLD(ch)-cost;
      if(GET_GOLD(ch)<0)
         GET_GOLD(ch)=0;

      release_buffer(buf);
      return TRUE;
      }
   else if(CMD_IS("check") && GET_LEVEL(ch)>=LVL_IMMORT)
      {
      send_to_char(ch, "Stats: All: %d   Str: %d   Dex: %d   Con: %d   Int: %d   Wis: %d   Cha: %d\r\n",allchk,strchk,dexchk,conchk,intchk,wischk,chachk);
      return TRUE;
      }
   return FALSE;
   }





SPECIAL(trainer)
   {
   /*   static char  buf[256];
        struct char_data *train =0;
        struct char_data *tch;
        char arg1[100];
        char arg2[100];
        char *argtemp;
      */
   return(FALSE);
   /*   if ((cmd<1)||(!ch->desc) || IS_NPC(ch)||!CMD_IS("train"))
        return(FALSE);
     
        for (tch = world[IN_ROOM(ch)].people; (tch) && (!train); tch = tch->next_in_room)
        if (IS_MOB(tch))
        if(mob_index[tch->nr].func == trainer)
        train = tch;
    
     
        if (!train)
        {
        sprintf(buf,"SYSERR: Fubar'd trainer. room: %d",IN_ROOM(ch));
        log(buf);
        return(FALSE); 
        }
        train->player.class=1;
    
        argtemp = one_argument(argument,arg1);
        if(argtemp)
        one_argument(argtemp,arg2);
         
       
        if(!CMD_IS("train"))
        {
        return(FALSE);
        }
        else
        {
        if (!AWAKE(train))
        {
        send_to_char("The trainer is sleeping, shhh....\r\n", ch);
        return(TRUE);
        }
        else if (!CAN_SEE(train, ch))
        {
        act("$n says, 'I can't train people I can't see!'", FALSE,\
        train, 0, 0, TO_ROOM);
        return(TRUE);  
        }
        else if (GET_PRACTICES(ch) <= 0)
        {
        act("$n says, 'Come back when you have some practice sessions left!'", FALSE, train, 0, 0, TO_ROOM);
        return(TRUE);
        }
        else if(!strcmp(arg1,"hitpoints"))
        {
        GET_MAX_HIT(ch) = GET_MAX_HIT(ch) + 2;
        send_to_char("You train your hitpoints for a while...\r\n", ch);
        GET_PRACTICES(ch)--;
        return(TRUE);
        }
        else if(!strcmp(arg1,"energy"))
        {
        GET_MAX_MANA(ch) = GET_MAX_MANA(ch) + 2;
        send_to_char("You train your energy for a while...\r\n", ch);
        GET_PRACTICES(ch)--;
        return(TRUE);
        }
        else
        {
        send_to_char("\r\n   -----------------------------------------------------\r\n",ch);
        send_to_char("   I can train your Hitpoints or Energy higher in\r\n",ch);
        send_to_char("   exchange for one of your practice sessions. I will\r\n",ch);
        send_to_char("   give you 2 energy points or 2 hitpoints for each\r\n",ch);
        send_to_char("   practice session you trade me. BUT be warned!! The\r\n",ch);
        send_to_char("   gods have told me that new spells or skills can pop\r\n",ch);
        send_to_char("   up at any time so if you trade me ALL your pracitices\r\n",ch);
        send_to_char("   you are gonna be out of luck when a new spell or\r\n",ch);
        send_to_char("   skill comes in. For me to train you, you must type:\r\n",ch);
        send_to_char("   'train hitpoints' or 'train energy'.\r\n",ch);
        send_to_char("   -----------------------------------------------------\r\n",ch);
        sprintf(buf, "   You have %d practice session%s remaining.\r\n\r\n",
        GET_PRACTICES(ch), (GET_PRACTICES(ch) == 1 ? "":"s"));
        send_to_char(ch,"%s",buf);
        return(TRUE);
        }
          
        if (GET_PRACTICES(ch) < 0)
        GET_PRACTICES(ch) = 0;
        return(TRUE);
        }
                        
        */
   }

SPECIAL(questmob)
   {
   struct char_data *quest =0;
   struct char_data *tch;
   char arg1[100];
   char arg2[100];
   char *argtemp;
   int race;

   if ((cmd<1)||(!ch->desc) || IS_NPC(ch))
      return(FALSE);

   for (tch = world[IN_ROOM(ch)].people; (tch) && (!quest); tch = tch->next_in_room)
      if (IS_MOB(tch))
         if(mob_index[tch->nr].func == questmob)
            quest = tch;

   if (!quest)
      {
      log("SYSERR: Fubar'd quest mob. room: %ld",IN_ROOM(ch));
      return(FALSE);
      }

   quest->player.class=1;

   argtemp = one_argument(argument,arg1);

   if(argtemp)
      one_argument(argtemp,arg2);

   if(!CMD_IS("reward"))
      {
      return(FALSE);
      }
   else
      {
      race = GET_RACE(ch);
      if (!AWAKE(quest))
         {
         send_to_char(ch,"The Questor is sleeping, shhh....\r\n");
         return(TRUE);
         }
      else if (!CAN_SEE(quest, ch))
         {
         act("$n says, 'I can't reward people I can't see!'", FALSE,\
             quest, 0, 0, TO_ROOM);
         return(TRUE);
         }
      else if (GET_QPOINTS(ch) <= 0)
         {
         act("$n says, 'Come back when you have some Quest points saved up!'",
             FALSE, quest, 0, 0, TO_ROOM);
         return(TRUE);
         }
      else if(!strcmp(arg1,"hitpoints"))
         {

         if (GET_QPOINTS(ch) < 2)
            {
            act("$n says, 'You don't have enough Quest points saved up!'",
                FALSE, quest, 0, 0, TO_ROOM);
            return(TRUE);
            }
         else
            {
            GET_MAX_HIT(ch) = (GET_MAX_HIT(ch) + 5);
            GET_QPOINTS(ch) = (GET_QPOINTS(ch) - 2);
            send_to_char(ch,"You have been rewarded 5 hitpoints!\r\n");
            save_char(ch, IN_ROOM(ch));
            mudlogf(BRF, LVL_IMMORT, TRUE, "QUEST_MOB: %s bought 5 hps for "           
                    "2 qps, %s now has a total of %d qps/%d hps.", GET_NAME(ch),
                    GET_NAME(ch), GET_QPOINTS(ch), GET_MAX_HIT(ch));
            return(TRUE);
            }

         }
      else if(!strcmp(arg1,"energy"))
         {

         if (GET_QPOINTS(ch) < 2)
            {
            act("$n says, 'You don't have enough Quest points saved up!'",
                FALSE, quest, 0, 0, TO_ROOM);
            return(TRUE);
            }
         else
            {
            GET_MAX_MANA(ch) = (GET_MAX_MANA(ch) + 5);
            GET_QPOINTS(ch) = (GET_QPOINTS(ch) - 2);
            send_to_char(ch,"You have been rewarded 5 Energy points!\r\n");
            save_char(ch, IN_ROOM(ch));
            mudlogf(BRF, LVL_IMMORT, TRUE, "QUEST_MOB: %s bought 5 mana for "
                    "2 qps, %s now has a total of %d qps/%d mana.", GET_NAME(ch),
                    GET_NAME(ch), GET_QPOINTS(ch), GET_MAX_MANA(ch)); 
            return(TRUE);
            }
         }
      else if(!strcmp(arg1,"dexterity"))
         {

         if (GET_QPOINTS(ch) < 15)
            {
            act("$n says, 'You don't have enough Quest points saved up!'",
                FALSE, quest, 0, 0, TO_ROOM);
            return(TRUE);
            }
         else
            {

            if (ch->real_abils.dex >= race_max_stats[race][3])
               {
               send_to_char(ch,"You already have %ld dexterity!\r\n",
				race_max_stats[race][3]);
               return(TRUE);
               }
            else
               {
               ch->real_abils.dex =  (ch->real_abils.dex + 1);
               affect_total(ch);
               }
            GET_QPOINTS(ch) = (GET_QPOINTS(ch) - 15);
            send_to_char(ch,"You have been rewarded 1 Dexterity point!\r\n");
            save_char(ch, IN_ROOM(ch));
            mudlogf(BRF, LVL_IMMORT, TRUE, "QUEST_MOB: %s bought 1 DEX for "
                    "15 qps, %s now has a total of %d qps/%d dex.", GET_NAME(ch),
                    GET_NAME(ch), GET_QPOINTS(ch), ch->real_abils.dex);
            return(TRUE);
            }
         }
      else if(!strcmp(arg1,"charisma"))
         {

         if (GET_QPOINTS(ch) < 15)
            {
            act("$n says, 'You don't have enough Quest points saved up!'", FALSE,\
                quest, 0, 0, TO_ROOM);
            return(TRUE);
            }
         else
            {
            if (ch->real_abils.cha >= race_max_stats[race][5])
               {
               send_to_char(ch,"You already have %ld Charisma!\r\n",
				race_max_stats[race][5]);
               return(TRUE);
               }
            else
               {
               ch->real_abils.cha =  (ch->real_abils.cha + 1);
               affect_total(ch);
               }
            GET_QPOINTS(ch) = (GET_QPOINTS(ch) - 15);
            send_to_char(ch,"You have been rewarded 1 Charisma point!\r\n");
            save_char(ch, IN_ROOM(ch));
            mudlogf(BRF, LVL_IMMORT, TRUE, "QUEST_MOB: %s bought 1 CHA for " 
                    "15 qps, %s now has a total of %d qps/%d cha.", GET_NAME(ch),
                    GET_NAME(ch), GET_QPOINTS(ch), ch->real_abils.cha);
            return(TRUE);
            }
         }
      else if(!strcmp(arg1,"constitution"))
         {

         if (GET_QPOINTS(ch) < 15)
            {
            act("$n says, 'You don't have enough Quest points saved up!'", FALSE,\
                quest, 0, 0, TO_ROOM);
            return(TRUE);
            }
         else
            {
            if (ch->real_abils.con >= race_max_stats[race][4])
               {
               send_to_char(ch,"You already have %ld Constitution!\r\n",
				race_max_stats[race][4]);
               return(TRUE);
               }
            else
               {
               ch->real_abils.con =  (ch->real_abils.con + 1);
               affect_total(ch);
               }
            GET_QPOINTS(ch) = (GET_QPOINTS(ch) - 15);
            send_to_char(ch,"You have been rewarded 1 Constitution point!\r\n");
            save_char(ch, IN_ROOM(ch));
            mudlogf(BRF, LVL_IMMORT, TRUE, "QUEST_MOB: %s bought 1 CON for " 
                    "15 qps, %s now has a total of %d qps/%d con.", GET_NAME(ch),
                    GET_NAME(ch), GET_QPOINTS(ch), ch->real_abils.con);
            return(TRUE);
            }
         }
      else if(!strcmp(arg1,"intelligence"))
         {

         if (GET_QPOINTS(ch) < 15)
            {
            act("$n says, 'You don't have enough Quest points saved up!'", FALSE,\
                quest, 0, 0, TO_ROOM);
            return(TRUE);
            }
         else
            {
            if (ch->real_abils.intel >= race_max_stats[race][1])
               {
               send_to_char(ch,"You already have %ld Intelligence!\r\n",
				race_max_stats[race][1]);
               return(TRUE);
               }
            else
               {
               ch->real_abils.intel =  (ch->real_abils.intel + 1);
               affect_total(ch);
               }
            GET_QPOINTS(ch) = (GET_QPOINTS(ch) - 15);
            send_to_char(ch,"You have been rewarded 1 Intelligence point!\r\n");
            save_char(ch, IN_ROOM(ch));
            mudlogf(BRF, LVL_IMMORT, TRUE, "QUEST_MOB: %s bought 1 INT for " 
                    "15 qps, %s now has a total of %d qps/%d int.", GET_NAME(ch),
                    GET_NAME(ch), GET_QPOINTS(ch), ch->real_abils.intel);
            return(TRUE);
            }
         }
      else if(!strcmp(arg1,"wisdom"))
         {

         if (GET_QPOINTS(ch) < 15)
            {
            act("$n says, 'You don't have enough Quest points saved up!'", FALSE,\
                quest, 0, 0, TO_ROOM);
            return(TRUE);
            }
         else
            {
            if (ch->real_abils.wis >= race_max_stats[race][2])
               {
               send_to_char(ch,"You already have %ld Wisdom!\r\n",
				race_max_stats[race][2]);
               return(TRUE);
               }
            else
               {
               ch->real_abils.wis =  (ch->real_abils.wis + 1);
               affect_total(ch);
               }
            GET_QPOINTS(ch) = (GET_QPOINTS(ch) - 15);
            send_to_char(ch,"You have been rewarded 1 Wisdom point!\r\n");
            save_char(ch, IN_ROOM(ch));
            mudlogf(BRF, LVL_IMMORT, TRUE, "QUEST_MOB: %s bought 1 WIS for " 
                    "15 qps, %s now has a total of %d qps/%d wis.", GET_NAME(ch),
                    GET_NAME(ch), GET_QPOINTS(ch), ch->real_abils.wis);
            return(TRUE);
            }
         }
      else if(!strcmp(arg1,"strength"))
         {

         if (GET_QPOINTS(ch) < 15)
            {
            act("$n says, 'You don't have enough Quest points saved up!'", FALSE,\
                quest, 0, 0, TO_ROOM);
            return(TRUE);
            }
         else
            {
            if (ch->real_abils.str >= race_max_stats[race][0])
               {
               send_to_char(ch,"You already have %ld Strength!\r\n",
				race_max_stats[race][0]);
               return(TRUE);
               }
            else
               {
               ch->real_abils.str =  (ch->real_abils.str + 1);
               ch->real_abils.str_add = 0;
               affect_total(ch);
               }
            GET_QPOINTS(ch) = (GET_QPOINTS(ch) - 15);
            send_to_char(ch,"You have been rewarded 1 Strength point!\r\n");
            save_char(ch, IN_ROOM(ch));
            mudlogf(BRF, LVL_IMMORT, TRUE, "QUEST_MOB: %s bought 1 STR for " 
                    "15 qps, %s now has a total of %d qps/%d str.", GET_NAME(ch),
                    GET_NAME(ch), GET_QPOINTS(ch), ch->real_abils.str);
            return(TRUE);
            }

         }
      else
         {
         send_to_char(ch,"\r\n   ----------------------------------------------------------\r\n"
                      "   To get a reward from me you need to have earned quest\r\n"
                      "   points by completing quests run by the gods. I can \r\n"
                      "   increase your hitpoints or energy by 5 points in \r\n"
                      "   exchange for 2 quest points, or I can increase any \r\n"
                      "   of your stats by 1 point in exchange for 15 quest \r\n"
                      "   points. For me to do this you must type one of the\r\n"
                      "   following: 'reward hitpoints' 'reward energy'\r\n"
                      "   'reward strength' 'reward constitution' 'reward wisdom'\r\n"
                      "   'reward charisma' 'reward dexterity' 'reward intelligence'\r\n"
                      "   ----------------------------------------------------------\r\n");
         send_to_char(ch, "   You have %d Quest Point%s remaining.\r\n\r\n",
                      GET_QPOINTS(ch), (GET_QPOINTS(ch) == 1 ? "" : "s"));
         return(TRUE);
         }

      if (GET_QPOINTS(ch) < 0)
         GET_QPOINTS(ch) = 0;
      return(TRUE);
      }

   }


/* Healer spec_proc added by masque 3/8/95 */
SPECIAL(healer)
   {
   struct char_data *heal =0;
   struct char_data *tch;
   struct obj_data *tobj=NULL;
   /* odinian 10/25/99
      added castLevel 
      */
   int cost, i, castLevel;
   char *arg1;
   char *arg2;
   char *arg3;
   char *buf;
   char *argtemp=NULL, *argtemp2=NULL;

   if ((cmd<1)||(!ch->desc) || IS_NPC(ch))
      return(FALSE);

   for (tch = world[IN_ROOM(ch)].people; (tch) && (!heal); tch = tch->next_in_room)
      if (IS_MOB(tch))
         if(mob_index[tch->nr].func == healer)
            heal = tch;
   cost = 0;

   if (!heal)
      {
      log("SYSERR: Fubar'd Healer. room: %ld",IN_ROOM(ch));
      return(FALSE);
      }
   heal->player.class=1;
   arg1=get_buffer(256);
   arg2=get_buffer(256);
   arg3=get_buffer(256);
   buf=get_buffer(1024);
   argtemp = one_argument(argument,arg1);
   if(argtemp)
      argtemp2 = one_argument(argtemp,arg2);
   if(argtemp2)
      one_argument(argtemp2,arg3);

   /*  odinian 10/25/99
       Determine the spell level to cast
       based on level of the character
       The higher the level character, the lower the spell
       level gets cast.
        
       *** NOTE: Change all spells for the healer from level 10 to castLevel ****
       */
   castLevel = MAX((int)((LVL_IMMORT - GET_LEVEL(ch)) / 10) , 1);

   if(!CMD_IS("heal"))
      {
      release_buffer(arg1);
      release_buffer(arg2);
      release_buffer(arg3);
      release_buffer(buf);
      return(FALSE);
      }
   else
      {
      if (IS_SET(world[IN_ROOM(heal)].room_flags, ROOM_NOMAGIC))
         {
         act("$n says, 'I can't help you in this room, Please wait.'", FALSE,\
             heal, 0, 0, TO_ROOM);
         release_buffer(arg1);
         release_buffer(arg2);
         release_buffer(arg3);
         release_buffer(buf);

         return(TRUE);
         }
      if (IS_CASTING(heal))
         {
         send_to_char(ch,"The healer is busy at the moment, please wait your turn.\r\n");
         release_buffer(arg1);
         release_buffer(arg2);
         release_buffer(arg3);
         release_buffer(buf);
         return(TRUE);
         }
      if (!AWAKE(heal))
         {
         send_to_char(ch,"The healer is napping, shhh....\r\n");
         release_buffer(arg1);
         release_buffer(arg2);
         release_buffer(arg3);
         release_buffer(buf);
         return(TRUE);
         }
      else if (!CAN_SEE(heal, ch))
         {
         act("$n says, 'I don't deal with people I can't see!'", FALSE,\
             heal, 0, 0, TO_ROOM);
         release_buffer(arg1);
         release_buffer(arg2);
         release_buffer(arg3);
         release_buffer(buf);
         return(TRUE);
         }
      else if(!strcmp(arg1,"armor"))
         {
         cost = 800;
         cost = price_adjust(ch, heal, cost);
         if(!gold_check(cost,ch))
            {
            release_buffer(arg1);
            release_buffer(arg2);
            release_buffer(arg3);
            release_buffer(buf);
            return(TRUE);
            }
         cast_spell(heal,ch,NULL,NULL, NULL,SPELL_ARMOR,castLevel);
         }
      else if(!strcmp(arg1,"bless"))
         {
         cost=200;
         cost = price_adjust(ch, heal, cost);
         if(!gold_check(cost,ch))
            {
            release_buffer(arg1);
            release_buffer(arg2);
            release_buffer(arg3);
            release_buffer(buf);
            return(TRUE);
            }
         cast_spell(heal,ch,NULL,NULL, NULL,SPELL_BLESS,castLevel);
         }
      else if(!strcmp(arg1,"cure"))
         {
         if(!strcmp(arg2,"light"))
            {
            cost=300;
            if(!gold_check(cost,ch))
               {
               release_buffer(arg1);
               release_buffer(arg2);
               release_buffer(arg3);
               release_buffer(buf);
               return(TRUE);
               }
            cast_spell(heal,ch,NULL,NULL,NULL,  SPELL_CURE_LIGHT,castLevel);
            }
         else if(!strcmp(arg2,"critic"))
            {
            cost=2000;
            cost = price_adjust(ch, heal, cost);
            if(!gold_check(cost,ch))
               {
               release_buffer(arg1);
               release_buffer(arg2);
               release_buffer(arg3);
               release_buffer(buf);
               return(TRUE);
               }
            cast_spell(heal,ch,NULL,NULL, NULL,SPELL_CURE_CRITIC,castLevel);
            }
         else if(!strcmp(arg2,"blind"))
            {
            cost=500;
            cost = price_adjust(ch, heal, cost);
            if(!gold_check(cost,ch))
               {
               release_buffer(arg1);
               release_buffer(arg2);
               release_buffer(arg3);
               release_buffer(buf);
               return(TRUE);
               }
            cast_spell(heal,ch,NULL,NULL, NULL,SPELL_CURE_BLIND,castLevel);
            }
         else if(!strcmp(arg2,"plague"))
            {
            cost=3200;
            cost = price_adjust(ch, heal, cost);
            if(!gold_check(cost,ch))
               {
               release_buffer(arg1);
               release_buffer(arg2);
               release_buffer(arg3);
               release_buffer(buf);
               return(TRUE);
               }
            cast_spell(heal,ch,NULL,NULL,NULL, SPELL_CURE_PLAGUE,castLevel);
            }
         }
      else if(!strcmp(arg1,"heal"))
         {
         cost=4000;
         cost = price_adjust(ch, heal, cost);
         if(!gold_check(cost,ch))
            {
            release_buffer(arg1);
            release_buffer(arg2);
            release_buffer(arg3);
            release_buffer(buf);
            return(TRUE);
            }
         if (!FIGHTING(ch))
            {
            cast_spell(heal,ch,NULL,NULL, NULL,SPELL_HEAL,castLevel);
            }
         else
            {
            send_to_char(ch,"\r\nThe Healer says, I will not heal people who are fighting.\r\n");
            }
         }
      else if(!strcmp(arg1,"remove"))
         {
         if(!strcmp(arg2,"poison"))
            {
            cost=1500;
            cost = price_adjust(ch, heal, cost);
            if(!gold_check(cost,ch))
               {
               release_buffer(arg1);
               release_buffer(arg2);
               release_buffer(arg3);
               release_buffer(buf);
               return(TRUE);
               }
            cast_spell(heal,ch,NULL,NULL, NULL,SPELL_REMOVE_POISON,castLevel);
            }
         else if(!strcmp(arg2,"curse"))
            {
            if (*arg3)
               {    /* On an item */
               if (!(tobj = get_object_in_equip_vis(ch, arg3,
                                                    ch->equipment, &i)))
                  tobj = get_obj_in_list_vis(ch, arg3, ch->carrying);
               if (tobj)
                  {
                  cost=750;
                  cost = price_adjust(ch, heal, cost);
                  if (!gold_check(cost, ch))
                     {
                     release_buffer(arg1);
                     release_buffer(arg2);
                     release_buffer(arg3);
                     release_buffer(buf);
                     return TRUE;
                     }
                  cast_spell(heal,ch,tobj,NULL, NULL,SPELL_REMOVE_CURSE,castLevel);
                  }
               else
                  send_to_char(ch, "\r\nThe Healer says, 'I don't see the %s on you anywhere.'\r\n", arg3);
               release_buffer(arg1);
               release_buffer(arg2);
               release_buffer(arg3);
               release_buffer(buf);
               return(TRUE);
               }
            else
               {    /* On a player */
               cost=750;
               cost = price_adjust(ch, heal, cost);
               if(!gold_check(cost,ch))
                  {
                  release_buffer(arg1);
                  release_buffer(arg2);
                  release_buffer(arg3);
                  release_buffer(buf);
                  return(TRUE);
                  }
               cast_spell(heal,ch,NULL,NULL, NULL,SPELL_REMOVE_CURSE,castLevel);
               }
            }
         }
      else if(!strcmp(arg1,"sanctuary"))
         {
         cost=5400;
         cost = price_adjust(ch, heal, cost);
         if(!gold_check(cost,ch))
            {
            release_buffer(arg1);
            release_buffer(arg2);
            release_buffer(arg3);
            release_buffer(buf);
            return(TRUE);
            }
         cast_spell(heal,ch,NULL,NULL, NULL,SPELL_SANCTUARY,castLevel);
         }

      else
         {
         /* odinian 10/25/99 - removed non-cleric/deva spells
            odinian 11/1/99 - adjusted prices for cha
            */
         send_to_char(ch,"\r\nThe Healer says 'I will cast the following spells for a cost.'\r\n"
                      "\r\n(actual prices may vary depending on how much I like you)'\r\n"
                      "   Armor               800 gold coins\r\n"
                      "   Bless               200 gold coins\r\n"
                      "   Cure Light          300 gold coins\r\n"
                      "   Cure Critic        2000 gold coins\r\n"
                      "   Heal               4000 gold coins\r\n"
                      "   Sanctuary          5400 gold coins\r\n"
                      "   Cure Blind          500 gold coins\r\n"
                      "   Cure Plague        3200 gold coins\r\n"
                      "   Remove Poison      1500 gold coins\r\n"
                      "   Remove Curse        750 gold coins\r\n"
                      "   Remove Curse (item) 750 gold coins\r\n"
                      "   ** Tell healer to: 'heal remove curse item'\r\n"
                      "   ** where 'item' is the cursed item's name*\r\n"
                      "  __________________________________________________________________\r\n"
                      "  Type: heal <spell_name>  to have a certain spell cast upon you.\r\n"
                      "  WARNING!  You will be charged even if the spell doesn't affect you\r\n"
                      "  because you are already affected by the same spell.\r\n"
                      "  ------------------------------------------------------------------\r\n\r\n");
         release_buffer(arg1);
         release_buffer(arg2);
         release_buffer(arg3);
         release_buffer(buf);
         return(TRUE);
         }

      GET_GOLD(ch) = GET_GOLD(ch)-cost;
      if(GET_GOLD(ch)<0)
         GET_GOLD(ch)=0;
      release_buffer(arg1);
      release_buffer(arg2);
      release_buffer(arg3);
      release_buffer(buf);
      return(TRUE);
      }

   }


SPECIAL(executioner)
   {
   struct char_data *tch;

   if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
      return (FALSE);

   for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
      {
      if (!IS_NPC(tch) &&  (GET_LEVEL(tch) < LVL_IMMORT))
         {
         if(IS_SET(PLR_FLAGS(tch), PLR_KILLER))
            {
            act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
            hit(ch, tch, TYPE_UNDEFINED);
            return(TRUE);
            }
         else if (IS_SET(PLR_FLAGS(tch), PLR_THIEF))
            {
            act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
            hit(ch, tch, TYPE_UNDEFINED);
            return(TRUE);
            }
         }
      }
   return(FALSE);
   }







SPECIAL(moods)
   {
   int mood, which, i,length;
   char **mp;
   struct char_data *vict=0;

   vict = (struct char_data *) me;

   if(cmd!=SPEC_MOBACT)
      return FALSE;
   if (AFF_FLAGGED(ch, AFF_PARALYSIS))
      return(FALSE);
   if(MOB_FLAGGED(ch,MOB_NOMOOD))
      return FALSE;
   if(GET_POS(ch) < POS_RESTING)
      return FALSE;
   if (FIGHTING(ch))
      return FALSE;
   if (vict == ch)
      {
      for (vict = world[IN_ROOM(ch)].people;
              vict;
              vict = vict->next_in_room )
         /*  if ((!IS_NPC(vict)
           || IS_SET(vict->specials2.act, MOB_CAN_SPEAK))
           && !number(0,1) && vict != ch)
           */
         if (!number(0,2) && vict != ch)
            break;
      }

   if (GET_MOOD(ch) > 900)
      mp = hyper_soc;
   else if (GET_MOOD(ch) > 750)
      {
      if (number(0,1))
         mp = happy_soc;
      else
         mp = hyper_soc;
      }
   else if (GET_MOOD(ch) > 500)
      {
      switch(number(0,2))
         {
      case 0:
         mp = happy_soc;
         break;
      case 1:
         mp = cheery_soc;
         break;
      case 2:
         mp = hyper_soc;
         break;
         }
      }
   else if (GET_MOOD(ch) > 250)
      {
      switch(number(0,2))
         {
      case 0:
         mp = cheery_soc;
         break;
      case 1:
         mp = happy_soc;
         break;
      case 2:
         mp = neutral_soc;
         break;
         }
      }
   else if (GET_MOOD(ch) > 0)
      {
      switch(number(0,2))
         {
      case 0:
         mp = cheery_soc;
         break;
      case 1:
         mp = sad_soc;
         break;
      case 2:
         mp = neutral_soc;
         break;
         }
      }
   else if (GET_MOOD(ch) > -250)
      {
      switch(number(0,2))
         {
      case 0:
         mp = grouchy_soc;
         break;
      case 1:
         mp = sad_soc;
         break;
      case 2:
         mp = neutral_soc;
         break;
         }
      }
   else if (GET_MOOD(ch) > -500)
      {
      switch(number(0,2))
         {
      case 0:
         mp = grouchy_soc;
         break;
      case 1:
         mp = sad_soc;
         break;
      case 2:
         mp = homicidal_soc;
         break;
         }
      }
   else if (GET_MOOD(ch) > -900)
      {
      switch(number(0,1))
         {
      case 0:
         mp = grouchy_soc;
         break;
      case 1:
         mp = homicidal_soc;
         break;
         }
      }
   else
      mp = homicidal_soc;

   mood = 0;
   while (mp[mood][0] !='\n')
      mood++;
   which = number(0,mood);
   if (vict && GET_SEX(vict) == GET_SEX(ch))
      {
      i = 0;
      while(opp_sex_soc[i][0] != '\n')
         {
         if (!strcmp(opp_sex_soc[i], *(mp +which)))
            return(0);
         i++;
         }
      }
   if (vict
           && (GET_LEVEL(vict) < GET_LEVEL(ch))
           && ((!strcmp(*(mp +which),"bow") || !strcmp(*(mp +which),"worship") ||
                !strcmp(*(mp +which),"grovel"))))
      return(0);

   if (!vict && (!strcmp(*(mp +which),"grovel") ||
                 !strcmp(*(mp +which),"bow")))
      return(0);

   if (!strcmp(*(mp +which),"curtsey") &&    GET_SEX(ch) != SEX_FEMALE)
      return(0);
   if (!strcmp(*(mp +which),"bow") &&     GET_SEX(ch) != SEX_MALE)
      return(0);

   for (length=strlen(*(mp+which)),cmd=0;*cmd_info[cmd].command!='\n';cmd++)
      if (!strncmp(cmd_info[cmd].command, *(mp+which), length))
         break;

   if(!ch)
      return 0;

   if (vict)
      do_action(ch,GET_NAME(vict),cmd,0);
   else
      do_action(ch,"",cmd,0);

   return 0;
   }


SPECIAL(citizen)
   {
   if (cmd >= 0)
      return(0);
   if(!AWAKE(ch) || FIGHTING(ch))
      return(FALSE);
   if (AFF_FLAGGED(ch, AFF_PARALYSIS))
      return(FALSE);

   moods(ch, ch,SPEC_MOBACT, "");
   return 0;
   }


SPECIAL(mirror)
   {
   struct obj_data *wielded ;
   struct char_data *vict;
   struct char_data *ch_mirror;
   char *buf;

   if (cmd!=0)
      return FALSE;

   ch_mirror=(struct char_data *) me;

   if (!(vict=FIGHTING(ch_mirror)))
      return FALSE;

   if ((vict) && (IN_ROOM(vict) == IN_ROOM(ch_mirror)))
      {
      buf=get_buffer(256);
      vict=FIGHTING(ch_mirror);
      sprintf(buf,"An image of %s %s is standing here.\r\n",GET_NAME(vict),
              GET_TITLE(vict));
      if (strcmp(ch_mirror->player.long_descr, buf) != 0)
         {
         if(ch_mirror->player.long_descr&&(GET_MOB_VAL(ch_mirror,1)!=0))
            free(ch_mirror->player.long_descr);
         ch_mirror->player.long_descr=str_dup(buf);

         if(ch_mirror->player.name&&(GET_MOB_VAL(ch_mirror,1)!=0))
            free(ch_mirror->player.name);
         sprintf(buf,"%s image",GET_NAME(vict));
         ch_mirror->player.name = str_dup(buf);

         if(ch_mirror->player.short_descr&&(GET_MOB_VAL(ch_mirror,1)!=0))
            free(ch_mirror->player.short_descr);
         ch_mirror->player.short_descr = str_dup(GET_NAME(vict));

         GET_MAX_HIT(ch_mirror)=(GET_MAX_HIT(vict)*2);
         if (AFF_FLAGGED(vict, AFF_SANCTUARY))
            GET_MAX_HIT(ch_mirror)=GET_MAX_HIT(ch_mirror)*4;
         GET_HIT(ch_mirror)=GET_MAX_HIT(ch_mirror);
         GET_MAX_MOVE(ch_mirror)=GET_MAX_MOVE(vict);
         GET_MOVE(ch_mirror)=GET_MAX_MOVE(ch_mirror);
         GET_MAX_MANA(ch_mirror)=GET_MAX_MANA(vict);
         GET_MANA(ch_mirror)=GET_MAX_MANA(ch_mirror);

         GET_MOB_VAL(ch_mirror,1)++;
         GET_LEVEL(ch_mirror)=GET_LEVEL(vict);
         GET_CLASS(ch_mirror)=GET_CLASS(vict);
         GET_SEX(ch_mirror)=GET_SEX(vict);
         GET_AC(ch_mirror)=GET_AC(vict);
         GET_HITROLL(ch_mirror)=GET_HITROLL(vict);
         GET_DAMROLL(ch_mirror)=GET_DAMROLL(vict);
         GET_ALIGNMENT(ch_mirror)=GET_ALIGNMENT(vict);
         GET_EXP(ch_mirror)=(GET_LEVEL(ch_mirror)*GET_LEVEL(ch_mirror))*50;
         wielded = GET_EQ(vict, WEAR_WIELD_1);

         if (wielded)
            {
            ch_mirror->mob_specials.damnodice=GET_OBJ_VAL(wielded, 1);
            ch_mirror->mob_specials.damsizedice=GET_OBJ_VAL(wielded, 2);
            }
         else
            {
            ch_mirror->mob_specials.damnodice=GET_LEVEL(ch_mirror)/10;
            ch_mirror->mob_specials.damsizedice=GET_LEVEL(ch_mirror)/10;
            }

         release_buffer(buf);
         return TRUE;
         }
      release_buffer(buf);
      return FALSE;

      }
   return FALSE;
   }






/**************************************************************************
 *   Special Procedures for objects                                       *
 **************************************************************************/

SPECIAL(bank)
   {
   int amount;
   struct clan_data *pxClan;
   if(cmd<1)
      return FALSE;

   if (CMD_IS("balance"))
      {
      if (GET_BANK_GOLD(ch) > 0)
         send_to_char(ch, "Your current balance is %ld coins.\r\n",
                      GET_BANK_GOLD(ch));
      else
         send_to_char(ch, "You currently have no money deposited.\r\n");
      return 1;
      }
   else if (CMD_IS("deposit"))
      {
      if ((amount = atoi(argument)) <= 0)
         {
         send_to_char(ch,"How much do you want to deposit?\r\n");
         return 1;
         }
      if (GET_GOLD(ch) < amount)
         {
         send_to_char(ch, "You don't have that many coins!\r\n");
         return 1;
         }
      GET_GOLD(ch) -= amount;
      GET_BANK_GOLD(ch) += amount;
      send_to_char(ch, "You deposit %d coins.\r\n", amount);
      act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
      mudlogf(CMP, LVL_DGOD, TRUE, "Gold: %s has deposited %d gold into %s account.",
              GET_NAME(ch), amount, HSHR(ch));
      return 1;
      }
   else if (CMD_IS("withdraw"))
      {
      if ((amount = atoi(argument)) <= 0)
         {
         send_to_char(ch,"How much do you want to withdraw?\r\n");
         return 1;
         }
      if (GET_BANK_GOLD(ch) < amount)
         {
         send_to_char(ch,"You don't have that many coins deposited!\r\n");
         return 1;
         }
      GET_GOLD(ch) += amount;
      GET_BANK_GOLD(ch) -= amount;
      send_to_char(ch, "You withdraw %d coins.\r\n", amount);
      act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
      mudlogf(CMP, LVL_DGOD, TRUE, "Gold: %s has withdrawn %d gold from %s account.",
              GET_NAME(ch), amount, HSHR(ch));
      return 1;
      }
   else if (CMD_IS("cbalance"))
      {
      if(GET_CLAN(ch)<=0)
         {
         send_to_char(ch,"You need to be in a clan to do this.\r\n");
         return TRUE;
         }
      for(pxClan=clan_list;pxClan;pxClan=pxClan->next)
         if(pxClan->cl_number==GET_CLAN(ch))
            break;
      if(pxClan!=NULL)
         {
         send_to_char(ch, "%s's balance is %ld.\r\n",pxClan->cl_name,
                      pxClan->cl_bank);
         }
      else
         send_to_char(ch,"Your clan doesn't seem to exist!\r\n");
      return TRUE;

      }
   else if (CMD_IS("cdeposit"))
      {
      if(GET_CLAN(ch)<=0)
         {
         send_to_char(ch,"You need to be in a clan to do this.\r\n");
         return TRUE;
         }
      if ((amount = atoi(argument)) <= 0)
         {
         send_to_char(ch,"How much do you want to deposit?\r\n");
         return TRUE;
         }
      if (GET_GOLD(ch) < amount)
         {
         send_to_char(ch,"You don't have that many coins!\r\n");
         return TRUE;
         }
      if (!can_give_gold(ch, amount))
         {
         send_to_char(ch, "The auctioneer smacks you upside the head.\r\n");
         return TRUE;
         }
      for(pxClan=clan_list;pxClan;pxClan=pxClan->next)
         if(pxClan->cl_number==GET_CLAN(ch))
            break;

      if(pxClan!=NULL)
         {
         GET_GOLD(ch)-=amount;
         pxClan->cl_bank+=amount;
         send_to_char(ch, "You deposit %d coins into %s's account.\r\n",
                      amount, pxClan->cl_name);
         mudlogf(CMP, LVL_ADMIN, TRUE, "Gold: %s has deposited %d gold into %s's account.",
                GET_NAME(ch), amount, pxClan->cl_name); 
         write_clan_file();
         }
      else
         send_to_char(ch,"Your clan doesn't seem to exist!\r\n");
      return TRUE;
      }
   else if (CMD_IS("cwithdraw"))
      {
      if(GET_CLAN(ch)<=0)
         {
         send_to_char(ch,"You need to be in a clan to do this.\r\n");
         return TRUE;
         }
      if(GET_LEADER(ch)!=1)
         {
         send_to_char(ch,"You need to be the leader of a clan to do this.\r\n");
         return TRUE;
         }
      if ((amount = atoi(argument)) <= 0)
         {
         send_to_char(ch,"How much do you want to withdraw?\r\n");
         return TRUE;
         }
      for(pxClan=clan_list;pxClan;pxClan=pxClan->next)
         if(pxClan->cl_number==GET_CLAN(ch))
            break;

      if(pxClan!=NULL)
         {
         if(pxClan->cl_bank<amount)
            {
            send_to_char(ch,"You clan doesn't have that much cash.\r\n");
            return TRUE;
            }
         GET_GOLD(ch)+=amount;
         pxClan->cl_bank-=amount;
         send_to_char(ch, "You withdraw %d from %s's account.\r\n",amount,
                      pxClan->cl_name);
         mudlogf(CMP, LVL_ADMIN, TRUE, "Gold: %s has withdrawn %d gold from %s's account.",
                GET_NAME(ch), amount, pxClan->cl_name);
         write_clan_file();
         }
      else
         send_to_char(ch,"Your clan doesn't seem to exist!\r\n");
      return TRUE;
      }
   else
      return 0;
   }

SPECIAL(portal)
   {
   struct obj_data *obj = (struct obj_data *) me;
   struct obj_data *port;
   char *obj_name;

   if ((cmd<1)||!CMD_IS("enter"))
      return FALSE;

   obj_name=get_buffer(MAX_STRING_LENGTH);
   argument = one_argument(argument,obj_name);
   if (!(port = get_obj_in_list_vis(ch, obj_name,world[IN_ROOM(ch)].contents)))
      {
      release_buffer(obj_name);
      return(FALSE);
      }
   release_buffer(obj_name);

   if (port != obj)
      {
      return(FALSE);
      }

   GET_OBJ_VAL(port,0)--;
   if (port->obj_flags.value[1] <= 0 ||
           port->obj_flags.value[1] > 330000)
      {
      send_to_char(ch,"The portal leads nowhere\r\n");
      return TRUE;
      }

   act("$n enters $p, and vanishes!", FALSE, ch, port, 0, TO_ROOM);
   act("You enter $p, and you are transported elsewhere", FALSE, ch,
       port, 0, TO_CHAR);
   char_from_room(ch);
   char_to_room(ch, real_room(port->obj_flags.value[1]));
   look_at_room(ch,0);
   act("$n appears from thin air!", FALSE, ch, 0, 0, TO_ROOM);
   if(GET_OBJ_VAL(port,0)<1)
      {
      act("$p vanishes in a puff of smoke!", FALSE, 0, port, 0, TO_ROOM);
      extract_obj(port);
      }
   return TRUE;
   }

int convert_letter(char s)
   {
   if(s >= 'a' && s <= 'l')
      return s - 'a';
   else
      return 0;
   }

SPECIAL(roulette)
   {
   struct obj_data *obj = (struct obj_data *) me;
   static char *slot_color[37] =
      { "green", "red", "black", "red",
        "black", "red", "black", "red", "black", "red",
        "black", "black", "red", "black", "red", "black",
        "red", "black", "red", "red", "black", "red",
        "black", "red", "black", "red", "black", "red",
        "black", "black", "red", "black", "red", "black",
        "red", "black", "red"
      };
   char bet_on[38] = "......................................";
   int amount;
   char *bet;
   int res;
   register int l;
   int payout;

   if (IS_NPC(ch))
      return 0;
   if(!CMD_IS("bet") && !CMD_IS("check"))
      return 0;

   bet=get_buffer(256);
   if(CMD_IS("check")&&GET_LEVEL(ch)>=LVL_IMMORT)
      {
      send_to_char(ch, "The table took in:  %ld gold.\r\n",GET_OBJ_VAL(obj,6));
      send_to_char(ch, "The table paid out: %ld gold.\r\n",GET_OBJ_VAL(obj,7));
      release_buffer(bet);
      return 1;
      }
   if(!*argument)
      {
      send_to_char(ch,"The croupier says, 'You must wager something.'\r\n");
      release_buffer(bet);
      return 1;
      }
   skip_spaces(&argument);
   res = sscanf(argument, "%d on %s", &amount, bet);
   if(res != 2)
      {
      send_to_char(ch,
                   "The croupier says, 'I don't understand your bet of '%s''\r\n",
                   argument);
      release_buffer(bet);
      return 1;
      }

   if(amount < 10)
      {
      send_to_char(ch,"The croupier says, 'You must wager at least 10 gold.'\r\n");
      release_buffer(bet);
      return 1;
      }
   if(amount > 10000)
      {
      send_to_char(ch,"The croupier says, 'You may wager at most 10000 gold.'\r\n");
      release_buffer(bet);
      return 1;
      }

   if(GET_GOLD(ch) < amount)
      {
      send_to_char(ch,"The croupier says, 'Don't waste my time!  Come back with money!'\r\n");
      release_buffer(bet);
      return 1;
      }

   /* Make into lower case, and get length of string */
   for (l = 0; *(bet + l); l++)
      *(bet + l) = LOWER(*(bet + l));

   /* Okay.. process the bet */
   if(strcmp(bet, "00") == 0)
      {
      bet_on[37] = '+';
      payout = 36*amount;
      }
   else if (strcmp(bet, "0-line") == 0)
      {
      bet_on[0] = '+';
      bet_on[1] = '+';
      bet_on[2] = '+';
      bet_on[3] = '+';
      bet_on[37] = '+';
      payout = 7*amount;
      }
   else if (strcmp(bet, "row1") == 0)
      {
      bet_on[1] = '+';
      bet_on[4] = '+';
      bet_on[7] = '+';
      bet_on[10] = '+';
      bet_on[13] = '+';
      bet_on[16] = '+';
      bet_on[19] = '+';
      bet_on[22] = '+';
      bet_on[25] = '+';
      bet_on[28] = '+';
      bet_on[31] = '+';
      bet_on[34] = '+';
      payout = 3*amount;
      }
   else if (strcmp(bet, "row2") == 0)
      {
      bet_on[2] = '+';
      bet_on[5] = '+';
      bet_on[8] = '+';
      bet_on[11] = '+';
      bet_on[14] = '+';
      bet_on[17] = '+';
      bet_on[20] = '+';
      bet_on[23] = '+';
      bet_on[26] = '+';
      bet_on[29] = '+';
      bet_on[32] = '+';
      bet_on[35] = '+';
      payout = 3*amount;
      }
   else if (strcmp(bet, "row3") == 0)
      {
      bet_on[3] = '+';
      bet_on[6] = '+';
      bet_on[9] = '+';
      bet_on[12] = '+';
      bet_on[15] = '+';
      bet_on[18] = '+';
      bet_on[21] = '+';
      bet_on[24] = '+';
      bet_on[27] = '+';
      bet_on[30] = '+';
      bet_on[33] = '+';
      bet_on[36] = '+';
      payout = 3*amount;
      }
   else if (strcmp(bet, "even") == 0)
      {
      for(l = 2; l < 37; l+=2)
         bet_on[l] = '+';
      payout = 2*amount;
      }
   else if (strcmp(bet, "odd") == 0)
      {
      for(l = 1; l < 36; l+=2)
         bet_on[l] = '+';
      payout = 2*amount;
      }
   else if (strcmp(bet, "red") == 0)
      {
      bet_on[1] = '+';
      bet_on[3] = '+';
      bet_on[5] = '+';
      bet_on[7] = '+';
      bet_on[9] = '+';
      bet_on[12] = '+';
      bet_on[14] = '+';
      bet_on[16] = '+';
      bet_on[18] = '+';
      bet_on[19] = '+';
      bet_on[21] = '+';
      bet_on[23] = '+';
      bet_on[25] = '+';
      bet_on[27] = '+';
      bet_on[30] = '+';
      bet_on[32] = '+';
      bet_on[34] = '+';
      bet_on[36] = '+';
      payout = 2*amount;
      }
   else if (strcmp(bet, "black") == 0)
      {
      bet_on[2] = '+';
      bet_on[4] = '+';
      bet_on[6] = '+';
      bet_on[8] = '+';
      bet_on[10] = '+';
      bet_on[11] = '+';
      bet_on[13] = '+';
      bet_on[15] = '+';
      bet_on[17] = '+';
      bet_on[20] = '+';
      bet_on[22] = '+';
      bet_on[24] = '+';
      bet_on[26] = '+';
      bet_on[28] = '+';
      bet_on[29] = '+';
      bet_on[31] = '+';
      bet_on[33] = '+';
      bet_on[35] = '+';
      payout = 2*amount;
      }
   else if (strcmp(bet, "low") == 0)
      {
      for(l = 1; l < 13; l++)
         bet_on[l] = '+';
      payout = 3*amount;
      }
   else if (strcmp(bet, "mid") == 0)
      {
      for(l = 13; l < 25; l++)
         bet_on[l] = '+';
      payout = 3*amount;
      }
   else if (strcmp(bet, "high") == 0)
      {
      for(l = 25; l < 37; l++)
         bet_on[l] = '+';
      payout = 3*amount;
      }
   else if ((strcmp(bet, "top") == 0) || (strcmp(bet, "1-18") == 0))
      {
      for(l = 1; l < 19; l++)
         bet_on[l] = '+';
      payout = 2*amount;
      }
   else if ((strcmp(bet, "bottom") == 0) || (strcmp(bet, "19-36") == 0))
      {
      for(l = 19; l < 37; l++)
         bet_on[l] = '+';
      payout = 2*amount;
      }
   else if ((strlen(bet) == 1) && (*bet >= 'a') && (*bet <= 'l'))
      {
      int point = convert_letter(*bet) * 3;
      bet_on[point+1] = '+';
      bet_on[point+2] = '+';
      bet_on[point+3] = '+';
      payout = 12*amount;
      }
   else
      {
      int a, b;
      char c, d;
      /* these are all the oddball bets */
      if(sscanf(bet, "%c/%c", &c, &d) == 2)
         {
         /* adjacent streets */
         a = convert_letter(c);
         b = convert_letter(d);
         if((a < 0) || (a > 10) || (b < 1) || (b > 11))
            {
            send_to_char(ch, "The croupier says, 'I don't understand your bet of '%s''\r\n", argument);
            return 1;
            }
         if((a+1) != b)
            {
            send_to_char(ch,"The croupier says, 'You must bet on adjacent streets.'\r\n");
            release_buffer(bet);
            return 1;
            }
         a *= 3;
         b *= 3;
         bet_on[a+1] = '+';
         bet_on[a+2] = '+';
         bet_on[a+3] = '+';
         bet_on[b+1] = '+';
         bet_on[b+2] = '+';
         bet_on[b+3] = '+';
         payout = 6*amount;
         }
      else if(sscanf(bet, "%d/%d", &a, &b)==2)
         {
         if((a < 1) || (a > 35) || (b < 2) || (b > 36))
            {
            send_to_char(ch, "The croupier says, 'I don't understand your bet of '%s''\r\n", argument);
            release_buffer(bet);
            return 1;
            }
         if(((a + 1) != b) && ((a+3) != b))
            {
            send_to_char(ch,"The croupier says, 'You must bet on adjacent numbers.'\r\n");
            release_buffer(bet);
            return 1;
            }
         bet_on[a] = '+';
         bet_on[b] = '+';
         payout = 18*amount;
         }
      else if(sscanf(bet, "corner %d", &a) == 1)
         {
         /* corner bet */
         if(((a % 3) == 0) || (a < 1) || (a > 32))
            {
            send_to_char(ch,"The croupier says, 'That is not a valid corner bet.'\r\n");
            release_buffer(bet);
            return 1;
            }
         bet_on[a] = '+';
         bet_on[a+1] = '+';
         bet_on[a+3] = '+';
         bet_on[a+4] = '+';
         payout = 9 * amount;
         }
      else if (sscanf(bet, "%d", &a) == 1)
         {
         if((a < 0) || (a > 36))
            {
            send_to_char(ch, "The croupier says, 'I don't understand your bet of '%s''\r\n", argument);
            release_buffer(bet);
            return 1;
            }
         bet_on[a] = '+';
         payout = 36*amount;
         }
      else
         {
         send_to_char(ch, "The croupier says, 'I don't understand your bet of '%s''\r\n", argument);
         release_buffer(bet);
         return 1;
         }
      }

   GET_GOLD(ch) -= amount;

   act("$n places a bet at the roulette table.", FALSE, ch, 0, ch, TO_ROOM);
   send_to_char(ch,"You place your bet.\r\n");

   res = number(0, 37);

   if(res == 37)
      {
      send_to_char(ch, "The croupier says, 'The ball stopped in slot '00' (green)'\r\n");
      }
   else
      {
      send_to_char(ch, "The croupier says, 'The ball stopped in slot '%d' (%s)'\r\n", res, slot_color[res]);
      }
   if(bet_on[res] == '+')
      {
      act("$n wins money at the roulette table.", FALSE, ch, 0, ch, TO_ROOM);
      GET_GOLD(ch) += payout;
      GET_OBJ_VAL(obj,6)+=payout;
      send_to_char(ch, "The croupier pays you %d coins.\r\n", payout);
      }
   else
      {
      GET_OBJ_VAL(obj,7)+=amount;
      act("$n loses money at the roulette table.", FALSE, ch, 0, ch, TO_ROOM);
      }
   release_buffer(bet);
   return 1;
   }
/**************************************************************************
 *   Special Procedures for rooms                                         *
 **************************************************************************/
SPECIAL(dump)
   {
   struct obj_data *k;
   int value = 0;

   for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents)
      {
      act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
      extract_obj(k);
      }

   if ((cmd<1)||!CMD_IS("drop"))
      return 0;

   do_drop(ch, argument, cmd, 0);

   for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents)
      {
      act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
      value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
      extract_obj(k);
      }

   if (value)
      {
      send_to_char(ch,"You are awarded for outstanding performance.\r\n");
      act("$n has been awarded for being a good citizen.", TRUE,
          ch, 0, 0, TO_ROOM);

      if (GET_LEVEL(ch) < 10)
         gain_exp(ch, value);
      else
         GET_GOLD(ch) += (value/2);
      }
   return 1;
   }



#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)
#define MAX_PETS 2
/* #define MAX_PETS(ch)   (MAX(1, GET_CHA(ch)))  */

SPECIAL(pet_shops)
   {
   char *buf;
   char *pet_name;
   room_rnum pet_room;
   struct char_data *pet;
   struct follow_type *f;
   int pet_counter = 0; /* Initialize this variable! */

   if(cmd<1)
      return FALSE;

   pet_room = IN_ROOM(ch) + 1;

   if (CMD_IS("list"))
      {
      send_to_char(ch,"Available pets are:\r\n");
      for (pet = world[pet_room].people; pet; pet = pet->next_in_room)
         {
         if(IS_NPC(pet))
            {
            send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
            }
         }
      return (TRUE);
      }
   else if (CMD_IS("buy"))
      {
      buf=get_buffer(MAX_STRING_LENGTH);
      pet_name=get_buffer(256);
      two_arguments(argument, buf, pet_name);

      if (!(pet = get_char_room(buf, pet_room)))
         {
         send_to_char(ch,"There is no such pet!\r\n");
         release_buffer(pet_name);
         release_buffer(buf);
         return (TRUE);
         }
      if(!IS_NPC(pet))
         {
         send_to_char(ch,"There is no such pet!\r\n");
         release_buffer(pet_name);
         release_buffer(buf);
         return (TRUE);
         }
      if (GET_GOLD(ch) < PET_PRICE(pet))
         {
         send_to_char(ch,"You don't have enough gold!\r\n");
         release_buffer(pet_name);
         release_buffer(buf);
         return (TRUE);
         }

      for (f = ch->followers; f; f = f->next)
         {
	   /*if (GET_MOB_VNUM(f->follower) == GET_MOB_VNUM(pet))*/
	   if (IS_NPC(f->follower)) {
	     pet_counter++;
	   }
         }
      if (pet_counter >= MAX_PETS/*(ch)*/)
         {
         send_to_char(ch,"You trying to open a pet store or what?\r\n");
         release_buffer(pet_name);
         release_buffer(buf);
         return (TRUE);
         }

      GET_GOLD(ch) -= PET_PRICE(pet);

      pet = read_mobile(GET_MOB_RNUM(pet), REAL);
      GET_EXP(pet) = 0;
      SET_BIT(AFF_FLAGS(pet), AFF_CHARM);

      if (*pet_name)
         {
         sprintf(buf, "%s %s", pet->player.name, pet_name);
         /* free(pet->player.name); don't free the prototype! */
         pet->player.name = str_dup(buf);

         sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
                 pet->player.description, pet_name);
         /* free(pet->player.description); don't free the prototype! */
         pet->player.description = str_dup(buf);
         }
      GET_MOB_VAL(pet,0)=GET_ROOM_VNUM(IN_ROOM(ch));
      pet->orig_room=IN_ROOM(ch);
      char_to_room(pet, IN_ROOM(ch));
      add_follower(pet, ch);
      load_mtrigger(pet);

      /* Be certain that pets can't get/carry/use/wield/wear items */
      IS_CARRYING_W(pet) = 1000;
      IS_CARRYING_N(pet) = 100;

      send_to_char(ch,"May you enjoy your pet.\r\n");
      act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

      release_buffer(pet_name);
      release_buffer(buf);
      return 1;
      }
   /* All commands except list and buy */
   return 0;
   }

SPECIAL(battle_master)
   {
   if(cmd<1)
       return FALSE;

   if (CMD_IS("list")) {
       send_to_char(ch, "It is free to learn &Rhow&n-to battle with other players.\r\n");
       send_to_char(ch, "It will cost you 50k to be able to &Rfight&n other players outside of the Battle field.\r\n");
       send_to_char(ch, "It will cost you 500k to be &Rsafe&n from other players.\r\n");
       return (TRUE);
       }
   else if (CMD_IS("fight")) {
       if (PLR_FLAGGED(ch, PLR_PK)) {
           send_to_char(ch, "You can already fight with others.\r\n");  
           return (TRUE);
           }
       else if (GET_GOLD(ch) < 50000) {
           send_to_char(ch, "You need 50k gold coins to be able to fight with other players.\r\n");
           return (TRUE);
           }
       else
           {
           GET_GOLD(ch) -= 50000;
           send_to_char(ch, "LETS' GET READY TO RUMBLE!!!!\r\n");
           SET_BIT(PLR_FLAGS(ch),PLR_PK);
           return (TRUE);
           }
       }
   else if (CMD_IS("safe")) {
       if (!PLR_FLAGGED(ch, PLR_PK)) {
           send_to_char(ch, "You are already safe from other players.\r\n");
           return (TRUE);
           }
       else if (GET_GOLD(ch) < 500000) {
           send_to_char(ch, "You need 500k gold coins to be safe from players.\r\n");
           return (TRUE);
           }
       else
           {
           GET_GOLD(ch) -= 500000;
           send_to_char(ch, "You are now safe from other players.\r\n");
           REMOVE_BIT(PLR_FLAGS(ch),PLR_PK);
           return (TRUE);
           }
       }
   else if (CMD_IS("how")) {
       send_to_char(ch, "   If you wish to fight with other players outside of the Battle field\r\n");
       send_to_char(ch, "and arenas you will need to purchase the PK flag. This flag will allow\r\n");
       send_to_char(ch, "you to fight with any player that is within 10 levels of yourself in\r\n");
       send_to_char(ch, "any place in the realm. The fights will work the same as if you were in\r\n");
       send_to_char(ch, "the battle arena. You will not lose experience or eq when you die, and\r\n");
       send_to_char(ch, "during the fight your eq will not get damaged. As there are no penalties\r\n");
       send_to_char(ch, "there are also no gains, you will not gain exp for the fight or the kill,\r\n");
       send_to_char(ch, "and you will not gain knowledge in your skills. Using the command who -p\r\n");
       send_to_char(ch, "you will be able to see any players that have the PK flag. If you chose\r\n");
       send_to_char(ch, "to buy the flag it will cost you 50k gold coins and if you chose to get\r\n");
       send_to_char(ch, "rid of the flag it will cost you 500k gold coins. If you notice any thing\r\n");
       send_to_char(ch, "strange or something that doesn't seem to work right please send a mudmail\r\n");
       send_to_char(ch, "to Ceria. Happy hunting and good luck.\r\n");
       return (TRUE);
       }
   return 0;
   }

SPECIAL(explore_seer)
{
  if (cmd < 1) {
    return 0;
  } else if (!CMD_IS("list")) {
    return 0;
  }

  struct char_data *mob = (struct char_data *)me;

  char *buf = get_buffer(32750);
  char *buf2 = get_buffer(MAX_STRING_LENGTH);
  sprintf(buf, "%s tells you, 'Here are the places you have visited'\r\n", GET_NAME(mob));
  buf[0] = toupper(buf[0]);

  int i, j, tzone;
  for (i = 0; i <= MIN(EXPLORED_TOP_VNUM/100, top_of_zone_table+1); i++) {
    for (j = 0; j < 100; j++) {
      int vnum = 100*i + j;
      if (ch->player_specials->explored_vnums[vnum/8] & (1 << (vnum%8))) {
	for (tzone = 0; tzone <= top_of_zone_table && zone_table[tzone].number != i; tzone++);
	sprintf(buf2, "   %s\r\n", zone_table[tzone].name);
	if (strlen(buf) + strlen(buf2) < 32700) {
	  strcat(buf, buf2);
	}
	break;
      }
    }
  }

  /* WAIT_STATE(ch, SKILL_LAG); */
  page_string(ch->desc, buf, TRUE, "");
  release_buffer(buf2);
  release_buffer(buf);

  return 1;
}


