/* ************************************************************************
*   File: act.movement.c                                Part of CircleMUD * 
*  Usage: movement commands, door handling, & sleep/rest/etc state        * 
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
#include "house.h"
#include "dg_scripts.h"
#include "clan.h"
#include "constants.h"
#include "queue.h"

/* external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;

/* external functs */
int  special(struct char_data *ch, int cmd, char *arg);
void death_cry(struct char_data *ch);
int  find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);
void dismount_char(struct char_data * ch);
void mount_char(struct char_data *ch, struct char_data *mount);
void mprog_greet_trigger(struct char_data * ch);
void mprog_entry_trigger(struct char_data * mob);
int has_key(struct char_data *ch, obj_vnum key);
void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door,int scmd);
void close_door(room_rnum was_in, int door);
void process_skills(struct char_data *ch, struct char_data *tch, struct obj_data *item, 
                    int t_alt, int skill, int state);
int handleGetOutOfDeathFree(struct char_data*);

/* simple function to determine if char can walk on water */
int has_boat(struct char_data *ch)
   {
   struct obj_data *obj;
   int i;
   /*
      if (ROOM_IDENTITY(IN_ROOM(ch)) == DEAD_SEA) 
      return (1); 
      */
   if (AFF_FLAGGED(ch, AFF_WATERWALK)||
           (!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE)))
      return (1);
   if(AFF_FLAGGED(ch,AFF_LEV)||AFF2_FLAGGED(ch,AFF2_FLYING))
      return (1);

   /* non-wearable boats in inventory will do it */
   for (obj = ch->carrying; obj; obj = obj->next_content)
      if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
         return (1);

   /* and any boat you're wearing will do it too */
   for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
         return (1);

   if(IS_NPC(ch)&&MOB_FLAGGED(ch,MOB_GOPATH))
      return (1);
   return (0);
   }



/* do_simple_move assumes
 *    1. That there is no master and no followers. 
 *    2. That the direction exists. 
 * 
 *   Returns : 
 *   1 : If succes. 
 *   0 : If fail 
 */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
   {
   int riding = 0, ridden_by = 0;
   room_rnum was_in;
   int need_movement;
   room_vnum vnum;
   struct char_data *tch;
   char *buf3;

   /*
    * Check for special routines (North is 1 in command list, but 0 here) Note 
    * -- only check if following; this avoids 'double spec-proc' bug 
    */
   if (need_specials_check && special(ch, dir + 1, ""))
      return (0);

   /*   check if they're mounted */
   /* if not in same room, dismount - nomikos 10/17/02 */
   if (RIDING(ch))
      {
      if (IN_ROOM(RIDING(ch)) == IN_ROOM(ch))
         riding = 1;
      else
         dismount_char(ch);
      }
   if (RIDDEN_BY(ch))
      {
      if (IN_ROOM(RIDDEN_BY(ch)) == IN_ROOM(ch))
         ridden_by = 1;
      else
         dismount_char(RIDDEN_BY(ch));
      }

   /*   tamed mobiles cannot move about (DAK) */
   if (ridden_by && AFF_FLAGGED(ch, AFF_TAMED))
      {
      send_to_char(ch, "You've been tamed.  Now act it!\r\n");
      return (0);
      }

   /*   charmed? */
   if(AFF_FLAGGED(ch,AFF_CHARM)&& ch->master&&
           (IN_ROOM(ch)==IN_ROOM(ch->master)))
      {
      send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
      act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
      return (0);
      }

   if( IS_NPC( ch ) && ( !EXIT( ch, dir ) || !( EXIT( ch, dir )->to_room ) ) )
     {
       send_to_char( ch, "There doesn't seem to be an exit there...\r\n" );
       return 0;
     }


   /* if this room or the one we're going to needs a boat, check for one */
   if (((SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) ||
           (SECT(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM)) &&
           !(!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      if ((riding && !has_boat(RIDING(ch))) || !has_boat(ch))
         {
         send_to_char(ch,"You need a boat to go there.\r\n");
         return (0);
         }
      }

   if((SECT(EXIT(ch,dir)->to_room) == SECT_UNDERWATER) &&
           !(!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      if(!ROOM_FLAGGED(EXIT(ch,dir)->to_room,ROOM_NO_WATERBREATHE))
         {
         if(riding && !AFF_FLAGGED(RIDING(ch),AFF_WATER_BREATHE))
            {
            send_to_char(ch, "Your mount can't breathe under water!\r\n");
            return (0);
            }
         if(!AFF_FLAGGED(ch,AFF_WATER_BREATHE))
            {
            send_to_char(ch,"You need air to breathe, not water!\r\n");
            return (0);
            }
         }
      }

   if((ROOM_FLAGGED(EXIT(ch,dir)->to_room,ROOM_NO_LEV))&&
           !(!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      if(riding && AFF_FLAGGED(RIDING(ch),AFF_LEV))
         {
         send_to_char(ch,"Your mount can't levitate there!\r\n");
         return (0);
         }
      else if(AFF_FLAGGED(ch,AFF_LEV))
         {
         send_to_char(ch,"You cannot levitate there!\r\n");
         return (0);
         }
      }
   if((ROOM_FLAGGED(EXIT(ch,dir)->to_room,ROOM_NO_FLY))&&
           !(!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      if(riding && AFF2_FLAGGED(RIDING(ch),AFF2_FLYING))
         {
         send_to_char(ch,"Your mount can't fly there!\r\n");
         return (0);
         }
      else if(AFF2_FLAGGED(ch,AFF2_FLYING))
         {
         send_to_char(ch,"You cannot fly there!\r\n");
         return (0);
         }
      }
   if(riding&&(ROOM_FLAGGED(EXIT(ch,dir)->to_room,ROOM_NO_MOUNT))&&
           !(!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      send_to_char(ch,"You need to dismount to go there!\r\n");
      return (0);
      }

   if((ROOM2_FLAGGED(EXIT(ch,dir)->to_room,ROOM2_NEVERMOB)) && IS_NPC(ch))
      return (0);

   /* If you're about to move rooms, and you're picking an object on the ground or a door exit, you give up. */
   if (AFF2_FLAGGED(ch, AFF2_PICKING_STAY)) {
      send_to_char(ch, "You abandon picking the lock.\r\n");
      REMOVE_BIT(AFF2_FLAGS(ch), AFF2_PICKING_STAY);
      /* Clear the pick lock skill lag. */
      GET_WAIT_STATE(ch) = 0;
      }

   /* move points needed is avg. move loss for src and destination sect type */
   need_movement = movement_loss[SECT(IN_ROOM(ch))];
   need_movement += movement_loss[SECT(EXIT(ch, dir)->to_room)];

   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_TRAVEL)&&!(riding||ridden_by))
      need_movement += number(10, 14);

   if(riding || AFF2_FLAGGED(ch,AFF2_FLYING))
      need_movement=1;
   else if(AFF_FLAGGED(ch,AFF_LEV))
      need_movement /=4;
   else
      need_movement /=2;


   if (riding)
      {
      if (GET_MOVE(RIDING(ch)) < need_movement)
         {
         send_to_char(ch,"Your mount is too exhausted.\r\n");
         return (0);
         }
      else if (GET_MOVE(ch)<2)
         {
         send_to_char(ch, "You are too exhausted to ride any further.\r\n");
         return (0);
         }
      }
   else
      {
      if (GET_MOVE(ch) < need_movement && !IS_NPC(ch))
         {
         if (need_specials_check && ch->master)
            send_to_char(ch, "You are too exhausted to follow.\r\n");
         else
            send_to_char(ch, "You are too exhausted.\r\n");
         return (0);
         }
      }

   if((EXIT_FLAGGED(EXIT(ch,dir),EX_FLY)||
           (SECT(EXIT(ch, dir)->to_room) == SECT_FLYING)) &&
           !(!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      if(riding&&!AFF2_FLAGGED(RIDING(ch),AFF2_FLYING))
         {
         send_to_char(ch,"Your mount needs to fly to go there.\r\n");
         return (0);
         }
      else if(!riding && !AFF2_FLAGGED(ch,AFF2_FLYING))
         {
         send_to_char(ch, "You need to be flying to go there.\r\n");
         return (0);
         }
      }

   /*
      if (riding)
         {
         if (GET_SKILL(ch, SKILL_RIDING)<number(1, 65+movement_loss[SECT(IN_ROOM(ch))]*5))
            {
            if (number(1, 4) == 1)
               {
               act("$N rears backwards, throwing you to the ground.", FALSE, ch, 0, 
                   RIDING(ch), TO_CHAR); 
               act("You rear backwards, throwing $n to the ground.", FALSE, ch, 0,
                   RIDING(ch), TO_VICT); 
               act("$N rears backwards, throwing $n to the ground.", FALSE, ch, 0,
                   RIDING(ch), TO_NOTVICT); 
               dismount_char(ch); 
               damage(ch, ch, dice(1,6), -1,IMM_BLUNT); 
               improve_skill(ch,SKILL_RIDING,AUTO_FAIL);
               return (0); 
               }
            else
               {
               act("You lose balance and almost fall off your mount, but quickly "
                   "regain control.", FALSE, ch, 0, 0, TO_CHAR);
               act("$n loses balance and almost falls off $s mount, but quickly "
                   "regains control.", FALSE, ch, 0, RIDING(ch), TO_NOTVICT);
               }
            } 
         else
            improve_skill(ch,SKILL_RIDING,AUTO_PASS);
         }
   */

   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM))
      {
      vnum = GET_ROOM_VNUM(EXIT(ch, dir)->to_room);
      if (!House_can_enter(ch, vnum))
         {
         send_to_char(ch,"That's private property -- no trespassing!\r\n");
         return (0);
         }
      }

   if((ROOM_FLAGGED(EXIT(ch,dir)->to_room,ROOM_CLAN))&&
           (IS_NPC(ch)||(!PRF_FLAGGED(ch,PRF_NOHASSLE)&&(GET_CLAN(ch)==0))))
      {
      send_to_char(ch,"That is a clan only room!\r\n");
      return(FALSE);
      }


   if((riding||ridden_by)&&ROOM_FLAGGED(EXIT(ch,dir)->to_room, ROOM_TUNNEL))
      {
      send_to_char(ch,"There isn't enough room there, while mounted.\r\n");
      return (0);
      }
   else
      {
      if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_TUNNEL) && (GET_LEVEL(ch)<LVL_IMMORT) &&
              num_pc_in_room(&(world[EXIT(ch, dir)->to_room])) >= 1)
         {
         send_to_char(ch,"There isn't enough room there for more than one person!\r\n");
         return (0);
         }
      }

   /* Mortals and low level gods cannot enter greater god rooms. */
   if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM) &&
           GET_LEVEL(ch) < LVL_GOD)
      {
      send_to_char(ch,"You aren't godly enough to use that room!\r\n");
      return (0);
      }

   int zone = GET_ROOM_VNUM(EXIT(ch, dir)->to_room)/100;
   if (!IS_NPC(ch) && zone != 0 && zone != 12 && zone != 30 && GET_LEVEL(ch) < WALK_INTO_LEVEL && !is_olc_set(ch, zone)) {
     send_to_char(ch, "You do not have permissions to walk there.\r\n");
     return 0;
   }   

   /* see if an entry trigger disallows the move */
   if (!entry_mtrigger(ch))
      return (0);
   if (!enter_wtrigger(&world[EXIT(ch, dir)->to_room], ch, dir))
      return (0);

   /* exit if the player died for some reason */
   if (IN_ROOM(ch) == NOWHERE)
      return (0);

   if(!IS_NPC(ch)&&!PRF_FLAGGED(ch,PRF_NOHASSLE))
      {
      if (!IS_NPC(ch) && !(riding || ridden_by))
         GET_MOVE(ch) -= need_movement;
      else if (riding)
         {
         GET_MOVE(ch)-=2;
         GET_MOVE(RIDING(ch)) -= 1;
         }
      else if (ridden_by)
         GET_MOVE(RIDDEN_BY(ch)) -= need_movement;
      }

   if (riding)
      {
      char *buf2=get_buffer(SMALL_BUFSIZE);
      sprintf(buf2, "$n rides $N %s.", dirs[dir]);
      act(buf2, TRUE, ch, 0, RIDING(ch), TO_NOTVICT);
      release_buffer(buf2);
      }
   else if (ridden_by)
      {
      char *buf2=get_buffer(SMALL_BUFSIZE);
      sprintf(buf2, "$n rides $N %s.", dirs[dir]);
      act(buf2, TRUE, RIDDEN_BY(ch), 0, ch, TO_NOTVICT);
      release_buffer(buf2);
      }
   else if (!AFF_FLAGGED(ch, AFF_SNEAK) &&
            !(AFF2_FLAGGED(ch, AFF2_ROVE) && CAN_ROVE(IN_ROOM(ch))))
      {
      if (!(IS_NPC(ch)&&MOB_NO_LDESC(ch)))
         {
         char *buf2=get_buffer(SMALL_BUFSIZE);
         sprintf(buf2, "$n leaves %s.", dirs[dir]);
         act(buf2, TRUE, ch, 0, 0, TO_ROOM);
         release_buffer(buf2);
         }
      }
   else
      {
      for(tch=world[IN_ROOM(ch)].people;tch;tch=tch->next_in_room)
         {
         if ((GET_LEVEL(tch) >= LVL_IMMORT) && (tch != ch))
            {
            buf3=get_buffer(SMALL_BUFSIZE);
            sprintf(buf3, "$n sneaks %s.", dirs[dir]);
            if (IS_NPC(ch)) /* to avoid mob using player specials error */
               {
               act(buf3, TRUE, ch, 0, tch, TO_VICT);
               }
            else
               {
               if (GET_INVIS_LEV(ch) <= GET_LEVEL(tch))
                  {
                  act(buf3, TRUE, ch, 0, tch, TO_VICT);
                  }
               }
            release_buffer(buf3);
            }
         }
      }

   if ((ROOM2_FLAGGED(ch->in_room, ROOM2_SALTWATER_FISH) ||
        ROOM2_FLAGGED(ch->in_room, ROOM2_FRESHWATER_FISH)) &&
        (PLR_FLAGGED(ch, PLR_FISHING) || PLR_FLAGGED(ch, PLR_FISH_ON)))
      {
      REMOVE_BIT(PLR_FLAGS(ch), PLR_FISHING | PLR_FISH_ON);
      send_to_char(ch, "\r\nYou pack up your fishing gear and move on.\r\n\r\n");
      }

   was_in = IN_ROOM(ch);
   char_from_room(ch);
   char_to_room(ch, world[was_in].dir_option[dir]->to_room);

   if (riding && IN_ROOM(RIDING(ch)) != IN_ROOM(ch))
      {
      char_from_room(RIDING(ch));
      char_to_room(RIDING(ch), IN_ROOM(ch));
      }
   else if (ridden_by && IN_ROOM(RIDDEN_BY(ch)) != IN_ROOM(ch))
      {
      char_from_room(RIDDEN_BY(ch));
      char_to_room(RIDDEN_BY(ch), IN_ROOM(ch));
      }

   if (riding)
      {
      char *buf2=get_buffer(SMALL_BUFSIZE);
      sprintf(buf2, "$n arrives from %s%s, riding $N.",
              (dir < UP  ? "the " : ""),
              (dir == UP ? "below":dir==DOWN?"above":dirs[rev_dir[dir]]));
      act(buf2, TRUE, ch, 0, RIDING(ch), TO_ROOM);
      release_buffer(buf2);
      }
   else if (ridden_by)
      {
      char *buf2=get_buffer(SMALL_BUFSIZE);
      sprintf(buf2, "$n arrives from %s%s, ridden by $N.",
              (dir < UP  ? "the " : ""),
              (dir==UP?"below":dir== DOWN ? "above" : dirs[rev_dir[dir]]));
      act(buf2, TRUE, ch, 0, RIDDEN_BY(ch), TO_ROOM);
      release_buffer(buf2);
      }
   else if (!AFF_FLAGGED(ch, AFF_SNEAK) &&
            !(AFF2_FLAGGED(ch, AFF2_ROVE) && CAN_ROVE(IN_ROOM(ch))))
      {
      if (!(IS_NPC(ch)&&MOB_NO_LDESC(ch)))
         act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);
      }
   else
      {
      for(tch=world[IN_ROOM(ch)].people;tch;tch=tch->next_in_room)
         {
         if ((GET_LEVEL(tch) >= LVL_IMMORT) && (tch != ch))
            {
            buf3=get_buffer(SMALL_BUFSIZE);
            sprintf(buf3, "$n sneaks in from %s%s.",
                    (dir < UP  ? "the " : ""),
                    (dir==UP?"below":(dir==DOWN?"above":dirs[rev_dir[dir]])));
            if (IS_NPC(ch)) /* to avoid mob using player specials error */
               {
               act(buf3, TRUE, ch, 0, tch, TO_VICT);
               }
            else
               {
               if (GET_INVIS_LEV(ch) <= GET_LEVEL(tch))
                  {
                  act(buf3, TRUE, ch, 0, tch, TO_VICT);
                  }
               }
            release_buffer(buf3);
            }
         }
      }

   if (ch->desc != NULL)
      look_at_room(ch, 0);

   for(tch=world[IN_ROOM(ch)].people;tch;tch=tch->next_in_room)
      {
      if(IS_MOB(tch)&&mob_index[GET_MOB_RNUM(tch)].func)
         (mob_index[GET_MOB_RNUM(tch)].func)(ch,tch,SPEC_ARRIVE,"");
      }

   /*   DT! (Hopefully these are rare in your MUD) -dak */
   if ((ROOM_FLAGGED(IN_ROOM(ch),ROOM_DEATH))&&(GET_LEVEL(ch)<LVL_IMMORT))
      {
	if (handleGetOutOfDeathFree(ch)) {
	  return 1;
	}
	  
      log_death_trap(ch);
      death_cry(ch);
      /* transfer gold */
      if (GET_GOLD(ch) > 0)
         {
         /* following 'if' clause added to fix gold duplication loophole */
         if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc))
            {
            struct obj_data *money = create_money(GET_GOLD(ch));
            obj_to_room(money, IN_ROOM(ch));
            }
         GET_GOLD(ch) = 0;
         }
      extract_char(ch);
      return (0);
      }

   entry_memory_mtrigger(ch);
   if (!greet_mtrigger(ch, dir))
      {
      char_from_room(ch);
      char_to_room(ch, was_in);
      look_at_room(ch, 0);
      }
   else
      greet_memory_mtrigger(ch);

   mprog_entry_trigger(ch);
   mprog_greet_trigger(ch);
   return (1);
   }


int perform_move(struct char_data *ch, int dir, int need_specials_check,
                 int following)
   {
   room_rnum was_in;
   struct follow_type *k, *next;
   int result;

   if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
      return (0);
   else if (!EXIT(ch, dir) ||
            EXIT(ch, dir)->to_room == NOWHERE)
      send_to_char(ch,"Alas, you cannot go that way...\r\n");
   else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)&&
            (GET_LEVEL(ch)<LVL_IMMORT))
      {
      if(EXIT_FLAGGED(EXIT(ch,dir), EX_SECRET))
         send_to_char(ch,"Alas, you cannot go that way...\r\n");
      else if(EXIT(ch, dir)->keyword)
         {
         send_to_char(ch, "The %s seems to be closed.\r\n",
                      fname(EXIT(ch, dir)->keyword));
         }
      else
         {
         send_to_char(ch, "It seems to be closed.\r\n");
         }
      }
   else if (EXIT_FLAGGED(EXIT(ch, dir), EX_NOPASS) && GET_LEVEL(ch) < LVL_IMMORT) {
     send_to_char(ch, "Alas, you cannot go that way...\r\n");
   }
   else
      {
      was_in = IN_ROOM(ch);
      result=do_simple_move(ch, dir, need_specials_check);

      if(!result)
         return (0);
      if(ch->followers)
         for (k = ch->followers; k; k = next)
            {
            next = k->next;
            if ((IN_ROOM(k->follower) == was_in) &&
                    (GET_POS(k->follower) >= POS_STANDING))
               {
               act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
               perform_move(k->follower, dir, 1,1);
               }
            }
      if((following==0)&&
              EXIT_FLAGGED(world[was_in].dir_option[dir],EX_ISDOOR)&&
              EXIT_FLAGGED(world[was_in].dir_option[dir],EX_AUTOCLOSE))
         if(!EXIT_FLAGGED(world[was_in].dir_option[dir],EX_CLOSED))
            close_door(was_in,dir);

      return (1);
      }
   return (0);
   }


ACMD(do_move)
   {
   /*
    * This is basically a mapping of cmd numbers to perform_move indices. 
    * It cannot be done in perform_move because perform_move is called 
    * by other functions which do not require the remapping. 
    */

   /* add in speed trap for speed walkers: 1 second lag per command over 10 */
   if (ch->desc && !IS_NPC(ch) && !PRF_FLAGGED(ch,PRF_NOHASSLE))
      ch->desc->speed_buffer += (ch->desc->speed_buffer < 10);
   perform_move(ch, subcmd - 1, 0,0);
   }


int find_door(struct char_data *ch, char *type, char *dir, char *cmdname)
   {
   int door;

   if (*dir)
      {
      /* a direction was specified */
      if ((door = search_block(dir, dirs, FALSE)) == -1)
         {
         /* Partial Match */
         send_to_char(ch, "That's not a direction.\r\n");
         return (-1);
         }
      if (EXIT(ch, door))
         {
         if (EXIT(ch, door)->keyword)
            {
            if (isname(type, EXIT(ch, door)->keyword))
               return (door);
            else
               {
               send_to_char(ch, "I see no %s there.\r\n", type);
               return (-1);
               }
            }
         else
            return (door);
         }
      else
         {
         send_to_char(ch,"I really don't see how you can %s anything there.\r\n", cmdname);
         return (-1);
         }
      }
   else      /* try to locate the keyword */
      {
      if (!*type)
         {
         send_to_char(ch, "What is it you want to %s?\r\n", cmdname);
         return (-1);
         }
      for (door = 0; door < NUM_OF_DIRS; door++)
         if (EXIT(ch, door))
            if (EXIT(ch, door)->keyword)
               if (isname(type, EXIT(ch, door)->keyword))
                  return (door);

      return (-1);
      }
   }


int has_key(struct char_data *ch, obj_vnum key)
   {
   struct obj_data *o;

   for (o = ch->carrying; o; o = o->next_content)
      if (GET_OBJ_VNUM(o) == key)
         return (1);

   if (GET_EQ(ch, WEAR_HOLD_1))
      if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_1)) == key)
         return (1);
   if (GET_EQ(ch, WEAR_HOLD_2))
      if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_2)) == key)
         return (1);

   return (0);
   }



#define NEED_OPEN     (1 << 0)
#define NEED_CLOSED   (1 << 1)
#define NEED_UNLOCKED (1 << 2)
#define NEED_LOCKED   (1 << 3)

char *cmd_door[] =
   {
      "open",
      "close",
      "unlock",
      "lock",
      "pick"
   }
   ;

const int flags_door[] =
   {
      NEED_CLOSED | NEED_UNLOCKED,
      NEED_OPEN,
      NEED_CLOSED | NEED_LOCKED,
      NEED_CLOSED | NEED_UNLOCKED,
      NEED_CLOSED | NEED_LOCKED
   }
   ;


#define EXITN(room, door)  (world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door) ((obj) ?\
        (TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
        (TOGGLE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door) ((obj) ?\
        (TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
        (TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))

void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
   {
   int other_room = 0;
   struct room_direction_data *back = 0;
   char *buf=get_buffer(SMALL_BUFSIZE);

   sprintf(buf, "$n %ss ", cmd_door[scmd]);
   if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
      if ((back = world[other_room].dir_option[rev_dir[door]])!=NULL)
         if (back->to_room != IN_ROOM(ch))
            back = 0;

   switch (scmd)
      {
      case SCMD_OPEN:
         if(!obj&&IS_SET(EXITN(IN_ROOM(ch),door)->exit_info,EX_WIZLOCK))
            {
            send_to_char(ch,"That door is magically sealed.\r\n");
            release_buffer(buf);
            return;
            }
      case SCMD_CLOSE:
         OPEN_DOOR(IN_ROOM(ch), obj, door);
         if (back)
            OPEN_DOOR(other_room, obj, rev_dir[door]);
         send_to_char(ch,"%s",OK);
         break;
      case SCMD_UNLOCK:
      case SCMD_LOCK:
         LOCK_DOOR(IN_ROOM(ch), obj, door);
         if (back)
            LOCK_DOOR(other_room, obj, rev_dir[door]);
         send_to_char(ch,"*Click*\r\n");
         break;
      case SCMD_PICK:
         LOCK_DOOR(IN_ROOM(ch), obj, door);
         if (back)
            LOCK_DOOR(other_room, obj, rev_dir[door]);
         send_to_char(ch,"The lock quickly yields to your skills.\r\n");
         strcpy(buf, "$n skillfully picks the lock on ");
         REMOVE_BIT(AFF2_FLAGS(ch), AFF2_PICKING_STAY);
         REMOVE_BIT(AFF2_FLAGS(ch), AFF2_PICKING);
         break;
      }

   /* Notify the room */
   sprintf(buf + strlen(buf), "%s%s.", ((obj) ? "" : "the "), (obj) ? "$p" :
           (EXIT(ch, door)->keyword ? "$F" : "door"));
   if (!(obj) || (IN_ROOM(obj) != NOWHERE))
      act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

   /* Notify the other room */
   if ((scmd == SCMD_OPEN || scmd == SCMD_CLOSE) && back)
      {
      sprintf(buf, "The %s is %s%s from the other side.",
              (back->keyword ? fname(back->keyword) :"door"),cmd_door[scmd],
              (scmd == SCMD_CLOSE) ? "d" : "ed");
      if (world[EXIT(ch, door)->to_room].people)
         {
         act(buf, FALSE, world[EXIT(ch, door)->to_room].people,0,0,TO_ROOM);
         act(buf, FALSE, world[EXIT(ch, door)->to_room].people,0,0,TO_CHAR);
         }
      }
   release_buffer(buf);
   }

void close_door(room_rnum was_in, int door)
   {
   room_rnum other_room = 0;
   struct room_direction_data *back = 0;
   if ((other_room = world[was_in].dir_option[door]->to_room) != NOWHERE)
      if ((back = world[other_room].dir_option[rev_dir[door]])!=NULL)
         if (back->to_room != was_in)
            back = 0;
   TOGGLE_BIT(world[was_in].dir_option[door]->exit_info,EX_CLOSED);
   if(back)
      TOGGLE_BIT(world[other_room].dir_option[rev_dir[door]]->exit_info,EX_CLOSED);


   }


#define DOOR_IS_OPENABLE(ch, obj, door) ((obj) ? \
 ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
  OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
 (EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door) ((obj) ? \
 (!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :\
 (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door) ((obj) ? \
 (!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :\
 (!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
 (OBJVAL_FLAGGED(obj,CONT_PICKPROOF)) : \
 (EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))
#define DOOR_IS_WIZLOCK(ch,door)\
 (EXIT_FLAGGED(EXIT(ch,door),EX_WIZLOCK))

#define DOOR_IS_CLOSED(ch, obj, door) (!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door) (!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)  ((obj) ? (GET_OBJ_VAL(obj, 2)) : \
      (EXIT(ch, door)->key))
#define DOOR_LOCK(ch, obj, door) ((obj) ? (GET_OBJ_VAL(obj, 1)) : \
      (EXIT(ch, door)->exit_info))

ACMD(do_gen_door)
   {
   int door = -1;
   obj_vnum keynum;
   char *type, *dir;
   struct obj_data *obj = NULL;
   struct char_data *victim = NULL;
   int pick_stationary = 0;

   skip_spaces(&argument);
   if (!*argument)
      {
      type = get_buffer(128);
      strcpy(type,cmd_door[subcmd]);
      send_to_char(ch, "%s what?\r\n", CAP(type));
      release_buffer(type);
      return;
      }

   type=get_buffer(MAX_INPUT_LENGTH);
   dir=get_buffer(MAX_INPUT_LENGTH);
   two_arguments(argument, type, dir);

   if ((door = find_door(ch, type, dir, cmd_door[subcmd]))==-1)
   {
		generic_find(type, FIND_OBJ_INV, ch, &victim, &obj);
		if (obj != NULL) {
			pick_stationary = 0;
		} else {
			generic_find(type, FIND_OBJ_ROOM, ch, &victim, &obj);
			pick_stationary = 1;
		}
   } else {
	   pick_stationary = 1;
   }

   if ((obj) || (door >= 0))
      {
      keynum = DOOR_KEY(ch, obj, door);
      if (!(DOOR_IS_OPENABLE(ch, obj, door)))
         act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
      else if (!DOOR_IS_OPEN(ch, obj, door) &&
               IS_SET(flags_door[subcmd], NEED_OPEN))
         send_to_char(ch,"But it's already closed!\r\n");
      else if (!DOOR_IS_CLOSED(ch, obj, door) &&
               IS_SET(flags_door[subcmd], NEED_CLOSED))
         send_to_char(ch,"But it's currently open!\r\n");
      else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
               IS_SET(flags_door[subcmd], NEED_LOCKED))
         send_to_char(ch,"Oh.. it wasn't locked, after all..\r\n");
      else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
               IS_SET(flags_door[subcmd], NEED_UNLOCKED))
         send_to_char(ch,"It seems to be locked.\r\n");
      else if (!has_key(ch, keynum) &&
               (IS_NPC(ch)||!PRF_FLAGGED(ch,PRF_NOHASSLE)) &&
               ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
         send_to_char(ch,"You don't seem to have the proper key.\r\n");
      else
         {
         if (subcmd == SCMD_PICK)
            {
            if (GET_SKILL(ch, SKILL_PICK_LOCK) > 0)
               {
               send_to_char(ch,"You start picking the lock.\r\n");
               if (pick_stationary == 1) {
                  SET_BIT(AFF2_FLAGS(ch), AFF2_PICKING_STAY);
                  } else {
                  SET_BIT(AFF2_FLAGS(ch), AFF2_PICKING);
                  }
               add_function_to_queue(HALF_SKILL_COUNT, ch, 0, 6, 
               process_skills, ch, NULL, obj, door, SKILL_PICK_LOCK, 0);
               WAIT_STATE(ch,SKILL_LAG);
               }
            else
               {
               send_to_char(ch, "You have no idea how.\r\n");
               }
            }
         else
            {
            do_doorcmd(ch, obj, door, subcmd);
            }
         }
      }
   else
      {
      send_to_char(ch,"There doesn't seem to be %s %s here.\r\n",
                   AN(type), type);
      }
   release_buffer(dir);
   release_buffer(type);
   return;
   }


ACMD(do_enter)
   {
   int door;
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   struct obj_data *entrance=NULL;
   int j = 0, vnumber;
   room_rnum to_room;
   struct room_data *to_room_data;

   one_argument(argument, buf);

   if (*buf)
      {
      char *tmp =get_buffer(MAX_INPUT_LENGTH);
      strcpy(tmp, buf);

      if (!(vnumber = get_number(&tmp)))
         {
         release_buffer(tmp);
         release_buffer(buf);
         return;
         }

      for (entrance = world[IN_ROOM(ch)].contents; entrance && (j <= vnumber);
              entrance = entrance->next_content)
         {
         if ((isname(tmp, entrance->name)) &&
                 (GET_OBJ_TYPE(entrance)==ITEM_PORTAL))
            {
            if (CAN_SEE_OBJ(ch, entrance))
               if (++j == vnumber)
                  {
                  break;
                  }
            }
         }
      /* an argument was supplied, first check for an ITEM_PORTAL */
      if(entrance)
         {
         to_room = real_room(GET_OBJ_VAL(entrance,0));
         if ((GET_OBJ_VAL(entrance,0)<=0) ||
                 (GET_OBJ_VAL(entrance,0)>330000) ||
                 (to_room == NOWHERE))
            {
            log("SYSERR: obj: %ld has an destination room %ld",
                GET_OBJ_VNUM(entrance),GET_OBJ_VAL(entrance,0));
            if(GET_LEVEL(ch)<LVL_IMMORT)
               send_to_char(ch,"A mysterious force blocks your way\n\r");
            else
               send_to_char(ch,"Some nitwit builder set the destination value "
                            "wrong!\n\rGo whap them!\n\r");
            release_buffer(tmp);
            release_buffer(buf);
            return;
            }

	 to_room_data = &world[to_room];
	 if (!enter_wtrigger(to_room_data, ch, 5)) { /* Entering a portal doesn't have a "direction" field.  Default it to 5 (down). */
	   /*
	    * The trigger said not to let you enter.  Return, and don't send any message
	    * to the player/room, because the trigger author should have sent something
	    * anyway.
	    *
	    * act("You try to enter $p, but are blocked!", FALSE, ch, entrance, 0, TO_CHAR);
	    * act("$p tries to enter $p, but is blocked!", FALSE, ch, entrance, 0, TO_ROOM);
	   */
	   release_buffer(tmp);
	   release_buffer(buf);
	   return;
	 }

         act("$n steps into $p", FALSE, ch, entrance, 0, TO_ROOM);
         act("You step into $p.", FALSE, ch, entrance, 0, TO_CHAR);
         char_from_room(ch);
         char_to_room(ch, to_room);
         look_at_room(ch,0);
         act("$n has arrived", FALSE, ch, 0, 0, TO_ROOM);
         }
      else
         {
         /* search for door keyword */
         for (door = 0; door < NUM_OF_DIRS; door++)
            if (EXIT(ch, door))
               if (EXIT(ch, door)->keyword)
                  if (!str_cmp(EXIT(ch, door)->keyword, buf))
                     {
                     perform_move(ch, door, 1,0);
                     release_buffer(tmp);
                     release_buffer(buf);
                     return;
                     }
         send_to_char(ch, "That is not an entrance.\r\n");
         }
      release_buffer(tmp);
      }
   else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
      send_to_char(ch,"You are already indoors.\r\n");
   else
      {
      /* try to locate an entrance */
      for (door = 0; door < NUM_OF_DIRS; door++)
         if (EXIT(ch, door))
            if (EXIT(ch, door)->to_room != NOWHERE)
               if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
                       ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS))
                  {
                  perform_move(ch, door, 1,0);
                  release_buffer(buf);
                  return;
                  }
      send_to_char(ch,"You can't seem to find anything to enter.\r\n");
      }
   release_buffer(buf);
   }


ACMD(do_leave)
   {
   int door;

   if (!OUTSIDE(ch))
      send_to_char(ch,"You are outside.. where do you want to go?\r\n");
   else
      {
      for (door = 0; door < NUM_OF_DIRS; door++)
         if (EXIT(ch, door))
            if (EXIT(ch, door)->to_room != NOWHERE)
               if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
                       !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS) &&
                       (SECT(EXIT(ch, door)->to_room) != SECT_INSIDE))
                  {
                  perform_move(ch, door, 1,0);
                  return;
                  }
      send_to_char(ch,"I see no obvious exits to the outside.\r\n");
      }
   }

void furniture_ok(struct char_data * ch, char *arg, char *text, int pos)
   {
   struct char_data *tch, *next_tch;
   struct obj_data *furniture;
   int num = 0;
   char *buf;

   /* Can the char see the object requested? */
   if (!(furniture = get_obj_in_list_vis(ch, arg,world[IN_ROOM(ch)].contents)))
      {
      send_to_char(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
      return;
      }

   /* Make sure they can actually sit on the object */
   if ((GET_OBJ_TYPE(furniture) != ITEM_FURNITURE) ||
           (pos < GET_OBJ_VAL(furniture, 0)))
      {
      buf = get_buffer(MAX_STRING_LENGTH);
      strcpy(buf,furniture->short_description);
      send_to_char(ch, "%s isn't for %s on!\r\n",
                   CAP(buf),
                   position_types[(int) pos]);
      release_buffer(buf);
      return;
      }

   /* Check if there's room on this piece of furniture */
   for (tch = furniture->people; tch; tch = next_tch)
      {
      next_tch = tch->next_in_furniture;
      num++;
      }

   /* Mob only check */
   /* This can be modified for size in your MUD */
   if (GET_OBJ_VAL(furniture, 1) == 0 ||
           ((!(IS_MOB(ch))) && GET_OBJ_VAL(furniture, 3) == -1))
      {
      send_to_char(ch, "You can't %s on that!\r\n", text);
      return;
      }

   /* Tell them why they can't fit on */
   if (GET_OBJ_VAL(furniture, 1) == num)
      {
      if (num == 1)
         {
         tch = furniture->people;
         send_to_char(ch, "%s is already %s on %s.\r\n", GET_NAME(tch),
                      position_types[(int) GET_POS(tch)],
                      furniture->short_description);
         }
      else
         send_to_char(ch, "There are already %d people using %s.\r\n", num,
                      furniture->short_description);
      return;
      }

   GET_POS(ch) = pos;
   char_to_object(ch, furniture);

   buf = get_buffer(MAX_STRING_LENGTH);
   sprintf(buf, "You %s on $p.", text);
   act(buf, FALSE, ch, furniture, 0, TO_CHAR | TO_SLEEP);

   sprintf(buf, "$n %ss on $p.", text);
   act(buf, TRUE, ch, furniture, 0, TO_ROOM);
   release_buffer(buf);
   }


ACMD(do_stand)
   {
   char *arg=get_buffer(MAX_STRING_LENGTH);
   one_argument(argument, arg);

   if (FURNITURE(ch))
      char_from_object(ch, FURNITURE(ch));


   if(!*arg)
      {
      switch (GET_POS(ch))
         {
         case POS_STANDING:
            send_to_char(ch, "You are already standing.\r\n");
            break;
         case POS_SITTING:
            send_to_char(ch, "You stand up.\r\n");
            act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            break;
         case POS_RESTING:
            send_to_char(ch, "You stop resting, and stand up.\r\n");
            act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0,
                TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            break;         
	 case POS_BANDAGE:
            send_to_char(ch, "You unwrap your wounds and stand up.\r\n");
            act("$n removes $s bandages, and jumps to $s feet.", TRUE, ch, 0, 0,
                TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            break;
         case POS_SLEEPING:
            send_to_char(ch, "You have to wake up first!\r\n");
            act("$n mumbles something unintelligible in $s sleep.",
                TRUE, ch, 0, 0, TO_ROOM);
            break;
         case POS_FIGHTING:
            send_to_char(ch, "Do you not consider fighting as standing?\r\n");
            act("$n yawns loudly, daydreaming of sleep.",
                TRUE, ch, 0, 0, TO_ROOM);
            break;
         default:
            send_to_char(ch,"You stop floating around, and put your feet "
                         "on the ground.\r\n");
            act("$n stops floating around, and puts $s feet on the ground.",
                TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            break;
         }
      }
   else
      furniture_ok(ch, arg, "stand", POS_STANDING);
   release_buffer(arg);


   if(FIGHTING(ch))
      GET_POS(ch)=POS_FIGHTING;
   }


ACMD(do_sit)
   {
   char *arg=get_buffer(MAX_STRING_LENGTH);
   one_argument(argument, arg);

   if (FURNITURE(ch))
      char_from_object(ch, FURNITURE(ch));


   if(!*arg)
      {
      switch (GET_POS(ch))
         {
         case POS_STANDING:
            send_to_char(ch, "You sit down.\r\n");
            act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
         case POS_SITTING:
            send_to_char(ch, "You're sitting already.\r\n");
            act("$n sits up as straight as $e can.",
                TRUE, ch, 0, 0, TO_ROOM);
            break;
         case POS_RESTING:
            send_to_char(ch, "You stop resting, and sit up.\r\n");
            act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
         case POS_SLEEPING:
            send_to_char(ch, "You have to wake up first.\r\n");
            act("$n sits up in $s sleep, then falls over and begins to snore.",
                TRUE, ch, 0, 0, TO_ROOM);
            break;
         case POS_FIGHTING:
            send_to_char(ch, "Sit down while fighting? are you MAD?\r\n");
            act("$n tries to sit this one out.",
                TRUE, ch, 0, 0, TO_ROOM);
            break;
	 case POS_BANDAGE:
            send_to_char(ch, "You fling away your bandages and sit up.\r\n");
            act("$n gets tired of $s bandages, casts them aside, and sits up.", TRUE, ch, 0, 0,
                TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
	 default:
            send_to_char(ch, "You stop floating around, and sit down.\r\n");
            act("$n stops floating around, and sits down.", TRUE,ch,0,0,
                TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
         }
      }
   else
      furniture_ok(ch, arg, "sit", POS_SITTING);
   release_buffer(arg);
   }


ACMD(do_rest)
   {
   char *arg=get_buffer(MAX_STRING_LENGTH);
   one_argument(argument, arg);

   if (FURNITURE(ch))
      char_from_object(ch, FURNITURE(ch));


   if(!*arg)
      {
      switch (GET_POS(ch))
         {
         case POS_STANDING:
            send_to_char(ch, "You sit down and rest your tired bones.\r\n");
            act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_RESTING;
            break;
         case POS_SITTING:
            send_to_char(ch, "You rest your tired bones.\r\n");
            act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_RESTING;
            break;
         case POS_RESTING:
            send_to_char(ch, "You are already resting.\r\n");
            break;
         case POS_SLEEPING:
            send_to_char(ch,"You have to wake up first.\r\n");
            act("$n rolls around in $s sleep, getting little rest.",
                TRUE, ch, 0, 0, TO_ROOM);
            break;
         case POS_FIGHTING:
            send_to_char(ch, "Rest while fighting?  Are you MAD?\r\n");
            act("$n takes a quick break, then jumps back into the fight.",
                TRUE, ch, 0, 0, TO_ROOM);
            break;
	 case POS_BANDAGE:
            send_to_char(ch, "You toss your bandages and get into a comforable position.\r\n");
            act("$n unwraps $s bandages and gets comfortable.", TRUE, ch, 0, 0,
                TO_ROOM);
            GET_POS(ch) = POS_RESTING;
            break;
	 default:
            send_to_char(ch, "You stop floating around, and stop to rest "
                         "your tired bones.\r\n");
            act("$n stops floating around, and rests.", FALSE, ch, 0, 0,
                TO_ROOM);
            GET_POS(ch) = POS_RESTING;
            break;
         }
      }
   else
      furniture_ok(ch, arg, "rest", POS_RESTING);
   release_buffer(arg);
   }


ACMD(do_sleep)
   {
   char *arg=get_buffer(MAX_STRING_LENGTH);
   one_argument(argument, arg);

   if (FURNITURE(ch))
      char_from_object(ch, FURNITURE(ch));


   if(!*arg)
      {
      switch (GET_POS(ch))
         {
         case POS_STANDING:
         case POS_SITTING:
         case POS_RESTING:
            send_to_char(ch, "You go to sleep.\r\n");
            act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SLEEPING;
            break;
         case POS_SLEEPING:
            send_to_char(ch, "You are already sound asleep.\r\n");
            break;
         case POS_FIGHTING:
            send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
            act("$n dreams of sleep while fighting.",
                TRUE, ch, 0, 0, TO_ROOM);
            break;
	 case POS_BANDAGE:
            send_to_char(ch, "You unwrap your wounds and lie down to sleep.\r\n");
            act("$n removes $s bandages, and falls to the floor, unconscious.", TRUE, ch, 0, 0,
                TO_ROOM);
	    act("$n begins to snore, loudly.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SLEEPING;
            break;
         default:
            send_to_char(ch, "You stop floating around, and lie down "
                         "to sleep.\r\n");
            act("$n stops floating around, and lays down to sleep.",
                TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SLEEPING;
            break;
         }
      }
   else
      furniture_ok(ch, arg, "sleep", POS_SLEEPING);
   release_buffer(arg);
   }


ACMD(do_wake)
   {
   struct char_data *vict;
   int self = 0;
   char *arg=get_buffer(SMALL_BUFSIZE);

   one_argument(argument, arg);
   if (*arg)
      {
      if (GET_POS(ch) == POS_SLEEPING)
         send_to_char(ch, "Maybe you should wake yourself up first.\r\n");
      else if ((vict = get_char_vis(ch, arg,FIND_CHAR_ROOM)) == NULL)
         send_to_char(ch, "%s", NOPERSON);
      else if (vict == ch)
         self = 1;
      else if (AWAKE(vict))
         act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
      else if (AFF_FLAGGED(vict, AFF_SLEEP))
         act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
      else if (GET_POS(vict) < POS_SLEEPING)
         act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
      else
         {
         act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
         act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT |TO_SLEEP);
         GET_POS(vict) = POS_STANDING;
         }

      if (!self)
         {
         release_buffer(arg);
         return;
         }
      }

   release_buffer(arg);
   if (AFF_FLAGGED(ch, AFF_SLEEP))
      send_to_char(ch, "You can't wake up!\r\n");
   else if (AWAKE(ch))
      send_to_char(ch, "You are already awake...\r\n");
   else
      {
      send_to_char(ch, "You awaken, and stand up.\r\n");
      act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_STANDING;
      }
   }


ACMD(do_follow)
   {
   struct char_data *leader;
   char *buf=get_buffer(SMALL_BUFSIZE);

   one_argument(argument, buf);

   if (*buf)
      {
      if (!(leader = get_char_vis(ch, buf,FIND_CHAR_ROOM)))
         {
         send_to_char(ch, "%s", NOPERSON);
         release_buffer(buf);
         return;
         }
      }
   else
      {
      send_to_char(ch, "Whom do you wish to follow?\r\n");
      release_buffer(buf);
      return;
      }

   if (ch->master == leader)
      {
      act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
      release_buffer(buf);
      return;
      }
   if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master))
      {
      act("But you only feel like following $N!", FALSE, ch, 0, ch->master,
          TO_CHAR);
      }
   else
      {
      /* Not Charmed follow person */
      if (leader == ch)
         {
         if (!ch->master)
            {
            send_to_char(ch, "You are already following yourself.\r\n");
            release_buffer(buf);
            return;
            }
         stop_follower(ch);
         }
      else
         {
         if (circle_follow(ch, leader))
            {
            send_to_char(ch, "Sorry, but following in loops is not allowed.\r\n");
            release_buffer(buf);
            return;
            }
         if (ch->master)
            stop_follower(ch);
         REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
         add_follower(ch, leader);
         }
      }
   release_buffer(buf);
   }

ACMD(do_mount)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   struct char_data *vict;

   one_argument(argument, arg);

   if (!arg || !*arg)
      {
      send_to_char(ch, "Mount who?\r\n");
      }
   else if (!(vict = get_char_room_vis(ch, arg)))
      {
      send_to_char(ch, "There is no-one by that name here.\r\n");
      }
   else if (vict==ch)
      {
      send_to_char(ch, "You just aren't that flexible.\r\n");
      }
   else if (!IS_NPC(vict))
      {
      send_to_char(ch, "Eh....no.\r\n");
      }
   else if (RIDING(ch) || RIDDEN_BY(ch))
      {
      send_to_char(ch, "You are already mounted.\r\n");
      }
   else if (RIDING(vict) || RIDDEN_BY(vict))
      {
      send_to_char(ch, "It is already mounted.\r\n");
      }
   else if (!MOB_FLAGGED(vict, MOB_MOUNT)&& GET_RACE(vict)!=MRACE_EQUINE)
      {
      send_to_char(ch, "You can't mount that!\r\n");
      }
   else if(vict->master &&(vict->master !=ch))
      {
      send_to_char(ch,"But that mount belongs to %s.\r\n",
                   GET_NAME(vict->master));
      }
   else if (GET_LEVEL(vict) > GET_LEVEL(ch))
     {
       send_to_char(ch, "That mount is too powerful for you.\r\n");
     }
   /*   else if (!GET_SKILL(ch, SKILL_MOUNT))
        { 
        send_to_char(ch, "First you need to learn *how* to mount.\r\n"); 
        } 
        else if (GET_SKILL(ch, SKILL_MOUNT) <= number(1, 101)) 
        { 
        act("You try to mount $N, but slip and fall off.", FALSE, ch, 0, vict,
        TO_CHAR); 
        act("$n tries to mount you, but slips and falls off.", FALSE, ch, 0,vict,
        TO_VICT); 
        act("$n tries to mount $N, but slips and falls off.", TRUE, ch, 0, vict, 
        TO_NOTVICT); 
        improve_skill(ch,SKILL_MOUNT,USE_FAIL);
        damage(ch, ch, dice(1, 2), -1,IMM_BLUNT); 
        } */
   else if (circle_follow(vict, ch))
      {
      send_to_char(ch, "How can you mount it and follow it at the same time?\r\n");
      }
   else
      {
      act("You mount $N.", FALSE, ch, 0, vict, TO_CHAR);
      act("$n mounts you.", FALSE, ch, 0, vict, TO_VICT);
      act("$n mounts $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      mount_char(ch, vict);
      /* improve_skill(ch,SKILL_MOUNT,USE_PASS);
        
         if (IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_TAMED) && 
         GET_SKILL(ch, SKILL_MOUNT) <= number(1, 101)) 
         { 
         act("$N suddenly bucks upwards, throwing you violently to the ground!", 
         FALSE, ch, 0, vict, TO_CHAR); 
         act("$n is thrown to the ground as $N violently bucks!", TRUE, ch, 0,
         vict, TO_NOTVICT); 
         act("You buck violently and throw $n to the ground.", FALSE, ch, 0, 
         vict, TO_VICT); 
         improve_skill(ch,SKILL_MOUNT,AUTO_FAIL);
         dismount_char(ch); 
         damage(ch, ch, dice(1,3), -1,IMM_BLUNT); 
         } 
         */
      }
   release_buffer(arg);
   }


ACMD(do_dismount)
   {
   if (!RIDING(ch))
      {
      send_to_char(ch, "You aren't even riding anything.\r\n");
      return;
      }
   else if (SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM && !has_boat(ch))
      {
      send_to_char(ch, "Yah, right, and then drown...\r\n");
      return;
      }
   else if ((SECT(IN_ROOM(ch)) == SECT_FLYING) &&
            !AFF2_FLAGGED(ch, AFF2_FLYING) &&
            !(!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      send_to_char(ch, "You take a quick glance at the ground far "
                   "below and change your mind...\r\n");
      return;
      }

   act("You dismount $N.", FALSE, ch, 0, RIDING(ch), TO_CHAR);
   act("$n dismounts from you.", FALSE, ch, 0, RIDING(ch), TO_VICT);
   act("$n dismounts $N.", TRUE, ch, 0, RIDING(ch), TO_NOTVICT);
   dismount_char(ch);
   }


ACMD(do_buck)
   {
   if (!RIDDEN_BY(ch))
      {
      send_to_char(ch, "You're not even being ridden!\r\n");
      return;
      }
   else if (AFF_FLAGGED(ch, AFF_TAMED))
      {
      send_to_char(ch, "But you're tamed!\r\n");
      return;
      }

   act("You quickly buck, throwing $N to the ground.", FALSE, ch, 0,
       RIDDEN_BY(ch), TO_CHAR);
   act("$n quickly bucks, throwing you to the ground.", FALSE, ch, 0,
       RIDDEN_BY(ch), TO_VICT);
   act("$n quickly bucks, throwing $N to the ground.", FALSE, ch, 0,
       RIDDEN_BY(ch), TO_NOTVICT);

   /* Aleks--modified this code so that the rider takes damage, rather than
      the mount.  Also, moved the call to damage and the change of 
      positions outside of this if block. 
      */

   if (number(0, 4))
      {
      send_to_char(RIDDEN_BY(ch), "You hit the ground hard!\r\n");
      damage(RIDDEN_BY(ch), RIDDEN_BY(ch), dice(2,4), -1,IMM_BLUNT);
      }

   GET_POS(RIDDEN_BY(ch)) = POS_SITTING;
   dismount_char(ch);

   /* you might want to call set_fighting() or some non-sense here if you
      want the mount to attack the unseated rider or vice-versa. */
   }

ACMD(do_descend)
   {
   if(AFF_FLAGGED(ch,AFF_LEV))
      {
      struct affected_type *af;
      for (af = ch->affected; af; af = af->next)
         if (af->bitvector == AFF_LEV)
            {
            affect_remove(ch, af);
            break;
            }
      /* if you don't want it to affect equipment with levitate, put
         REMOVE_BIT() before affect_total(). - Nomikos */
      affect_total(ch);
      REMOVE_BIT(AFF_FLAGS(ch), AFF_LEV);
      act("$n stops floating as $s feet touch the ground.",FALSE,ch,0,0,TO_ROOM);
      send_to_char(ch, "You stop levitating as your feet touch the ground.\r\n");
      }
   else
      {
      send_to_char(ch, "But you aren't levitating!\r\n");
      }
   }

ACMD(do_land)
   {
   if(AFF2_FLAGGED(ch,AFF2_FLYING))
      {
      if ((SECT(IN_ROOM(ch)) == SECT_FLYING) &&
              !(!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE)))
         send_to_char(ch, "There is no place to land here!\r\n");
      else
         {
         REMOVE_BIT(AFF2_FLAGS(ch), AFF2_FLYING);
         act("$n gently floats to the ground.",FALSE,ch,0,0,TO_ROOM);
         send_to_char(ch, "You gently float to the ground.\r\n");
         }
      }
   else
      {
      send_to_char(ch, "But you aren't flying!\r\n");
      }
   }

ACMD(do_fly)
   {
   if(AFF2_FLAGGED(ch,AFF2_FLYING))
      send_to_char(ch, "But you are already flying!\r\n");
   else if(CAN_FLY(ch))
      {
      SET_BIT(AFF2_FLAGS(ch), AFF2_FLYING);
      act("$n soars up into the air.",FALSE,ch,0,0,TO_ROOM);
      send_to_char(ch, "You soar up into the air.\r\n");
      }
   else
      {
      send_to_char(ch, "But you can't fly!\r\n");
      }
   }

