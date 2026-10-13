/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"


#include "structs.h"
#include "dg_scripts.h"
#include "db.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "comm.h"
#include "spells.h"
#include "buffer.h"
#include "constants.h"
#include "screen.h"
#include "gremort_exam.h"
#include "classweapon.h"

extern struct descriptor_data *descriptor_list;
extern room_rnum find_target_room(char_data * ch, char *rawroomstr);
extern struct index_data *mob_index;
extern struct room_data *world;
extern int dg_owner_purged;
extern struct zone_data *zone_table;

extern void die(struct char_data *ch, struct char_data *killer);

void script_log(char *msg);
struct char_data *get_char_dg(char *name);
void rage_check(struct char_data *ch);

void sub_write(char *arg, char_data *ch, byte find_invis, int targets);
long asciiflag_conv(char *flag);
room_data *get_room(char *name);
void death_cry(struct char_data *ch);
int handleGetOutOfDeathFree(struct char_data*);

/*
 * Local functions.
 */

/* attaches mob's name and vnum to msg and sends it to script_log */
void mob_log(char_data *mob, char *msg)
   {
   char *buf=get_buffer(MAX_INPUT_LENGTH + 100);


   sprintf(buf, "Mob (%s, VNum %ld): %s",
           GET_SHORT(mob), GET_MOB_VNUM(mob), msg);
   script_log(buf);
   release_buffer(buf);
   }
/*
** macro to determine if a mob is permitted to use these commands
*/
#define MOB_OR_IMPL(ch) \
(IS_NPC(ch) && (!(ch)->desc || GET_LEVEL((ch)->desc->original)>=LVL_IMPL))



/* mob commands */

/* prints the argument to all the rooms aroud the mobile */
ACMD(do_masound)
   {
   room_rnum was_in_room;
   int  door;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (!*argument)
      {
      mob_log(ch, "masound called with no argument");
      return;
      }

   skip_spaces(&argument);

   was_in_room = IN_ROOM(ch);
   for (door = 0; door < NUM_OF_DIRS; door++)
      {
      struct room_direction_data *exit_dir;

      if (((exit_dir = world[was_in_room].dir_option[door]) != NULL) &&
              exit_dir->to_room != NOWHERE && exit_dir->to_room != was_in_room)
         {
         IN_ROOM(ch) = exit_dir->to_room;
         sub_write(argument, ch, TRUE, TO_ROOM);
         }
      }

   IN_ROOM(ch) = was_in_room;
   }

ACMD(do_mdamage)
   {
   char      *name = get_buffer(MAX_INPUT_LENGTH);
   char      *amount = get_buffer(MAX_INPUT_LENGTH);
   int        dam = 0;
   char_data *victim;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      release_buffer(amount);
      release_buffer(name);
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   two_arguments(argument, name, amount);

   if (!*name || !*amount || !isdigit((int)*amount))
      {
      mob_log(ch, "mdamage: bad syntax");
      release_buffer(amount);
      release_buffer(name);
      return;
      }

   dam = atoi(amount);

   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
      {
      mob_log(ch, "mdamage: peaceful room");
      send_to_char(ch,"This room just has such a peaceful, easy feeling...\r\n");
      release_buffer(amount);
      release_buffer(name);
      return;
      }

   if ((victim = get_char_dg(name)))
      {
      if (!IS_NPC(ch)&&PRF_FLAGGED(ch,PRF_NOHASSLE))
         {
         send_to_char(victim,"Being the cool immortal you are, you sidestep a trap, obviously placed to kill you.");
         release_buffer(amount);
         release_buffer(name);
         return;
         }

      GET_HIT(victim) -= dam;
      GET_HIT(victim) = MAX(GET_HIT(victim),-10);
      update_pos(victim);
      switch (GET_POS(victim))
         {
      case POS_MORTALLYW:
         act("$n is mortally wounded, and will die soon, if not aided.",
             TRUE, victim, 0, 0, TO_ROOM);
         send_to_char(victim,"You are mortally wounded, and will die soon, if not aided.\r\n");
         break;
      case POS_INCAP:
         act("$n is incapacitated and will slowly die, if not aided.",
             TRUE, victim, 0, 0, TO_ROOM);
         send_to_char(victim,"You are incapacitated and will slowly die, if not aided.\r\n");
         break;
      case POS_STUNNED:
         act("$n is stunned, but will probably regain consciousness again.",
             TRUE, victim, 0, 0, TO_ROOM);
         send_to_char(victim,"You're stunned, but will probably regain consciousness again.\r\n");
         break;
      case POS_DEAD:
         act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
         send_to_char(victim,"You are dead!  Sorry...\r\n");
         break;
      default:   /* >= POSITION SLEEPING */
         if (dam > (GET_MAX_HIT(victim) >> 2))
            act("That really did HURT!", FALSE, victim, 0, 0, TO_CHAR);
         if (GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2))
            {
            send_to_char(victim,"%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
                         CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
            }
         }
      if (GET_POS(victim) == POS_DEAD)
         {
         if (!IS_MOB(victim))
            {
            mudlogf(BRF, LVL_IMMORT, TRUE,"%s killed by %s",
                    GET_NAME(victim), GET_NAME(ch));
            }
         die(victim, ch);
         }
      }
   else
      mob_log(ch, "mdamage: target not found");

   release_buffer(amount);
   release_buffer(name);
   }

/* lets the mobile kill any player or mobile without murder*/
ACMD(do_mkill)
   {
   char *arg;
   char_data *victim;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      mob_log(ch, "mkill called with no argument");
      release_buffer(arg);
      return;
      }

   if (*arg == UID_CHAR)
      {
      if (!(victim = get_char_dg(arg)))
         {
         char *buf=get_buffer(MAX_STRING_LENGTH);
         sprintf(buf, "mkill: victim (%s) not found",arg);
         mob_log(ch, buf);
         release_buffer(buf);
         release_buffer(arg);
         return;
         }
      }
   else if (!(victim = get_char_room_vis(ch, arg)))
      {
      char *buf=get_buffer(MAX_STRING_LENGTH);
      sprintf(buf, "mkill: victim (%s) not found",arg);
      mob_log(ch, buf);
      release_buffer(buf);
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if (victim == ch)
      {
      mob_log(ch, "mkill: victim is self");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master == victim )
      {
      mob_log(ch, "mkill: charmed mob attacking master");
      return;
      }

   if (FIGHTING(ch))
      {
      mob_log(ch, "mkill: already fighting");
      return;
      }

   hit(ch, victim, TYPE_UNDEFINED);
   return;
   }


/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy 
 * items using all.xxxxx or just plain all of them
 */
ACMD(do_mjunk)
   {
   char *arg;
   int pos, junk_all=FALSE;
   obj_data *obj;
   obj_data *obj_next;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      mob_log(ch, "mjunk called with no argument");
      release_buffer(arg);
      return;
      }

   if(!str_cmp(arg,"all"))
      junk_all=TRUE;

   if ((find_all_dots(arg) != FIND_INDIV) && !junk_all)
      {
      if ((obj=get_object_in_equip_vis(ch,arg,ch->equipment,&pos))!= NULL)
         {
         unequip_char(ch, pos);
         extract_obj(obj);
         release_buffer(arg);
         return;
         }
      if ((obj = get_obj_in_list_vis(ch, arg, ch->carrying)) != NULL )
         extract_obj(obj);
      release_buffer(arg);
      return;
      }
   else
      {
      for (obj = ch->carrying; obj != NULL; obj = obj_next)
         {
         obj_next = obj->next_content;
         if (arg[3] == '\0' || isname(arg+4, obj->name))
            {
            extract_obj(obj);
            }
         }
      while ((obj=get_object_in_equip_vis(ch,arg,ch->equipment,&pos)))
         {
         unequip_char(ch, pos);
         extract_obj(obj);
         }
      }
   release_buffer(arg);
   return;
   }


/* prints the message to everyone in the room other than the mob and victim */
ACMD(do_mechoaround)
   {
   char *arg;
   char_data *victim;
   char *p;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch, "Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   p = one_argument(argument, arg);
   skip_spaces(&p);

   if (!*arg)
      {
      mob_log(ch, "mechoaround called with no argument");
      release_buffer(arg);
      return;
      }

   if (*arg == UID_CHAR)
      {
      if (!(victim = get_char_dg(arg)))
         {
         char *buf=get_buffer(MAX_STRING_LENGTH);
         sprintf(buf, "mechoaround: victim (%s) does not exist",arg);
         mob_log(ch, buf);
         release_buffer(buf);
         release_buffer(arg);
         return;
         }
      }
   else if (!(victim = get_char_room_vis(ch, arg)))
      {
      char *buf=get_buffer(MAX_STRING_LENGTH);
      sprintf(buf, "mechoaround: victim (%s) does not exist",arg);
      mob_log(ch, buf);
      release_buffer(buf);
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   sub_write(p, victim, TRUE, TO_ROOM);
   }


/* sends the message to only the victim */
ACMD(do_msend)
   {
   char *arg;
   char_data *victim;
   char *p;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch, "Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   p = one_argument(argument, arg);
   skip_spaces(&p);

   if (!*arg)
      {
      mob_log(ch, "msend called with no argument");
      release_buffer(arg);
      return;
      }

   if (*arg == UID_CHAR)
      {
      if (!(victim = get_char_dg(arg)))
         {
         char *buf=get_buffer(256);
         sprintf(buf, "msend: victim (%s) does not exist",arg);
         mob_log(ch, buf);
         release_buffer(buf);
         release_buffer(arg);
         return;
         }
      }
   else if (!(victim = get_char_room_vis(ch, arg)))
      {
      char *buf=get_buffer(256);
      sprintf(buf, "msend: victim (%s) does not exist",arg);
      mob_log(ch, buf);
      release_buffer(buf);
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   sub_write(p, victim, TRUE, TO_CHAR);
   }


/* prints the message to the room at large */
ACMD(do_mecho)
   {
   char *p;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch, "Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (!*argument)
      {
      mob_log(ch, "mecho called with no arguments");
      return;
      }
   p = argument;
   skip_spaces(&p);

   sub_write(p, ch, TRUE, TO_ROOM);
   }


/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory, unless it is NO-TAKE. 
 */
ACMD(do_mload)
   {
   char *arg1;
   char *arg2;
   int vnumber = 0;
   char_data *mob;
   obj_data *object;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if( ch->desc && GET_LEVEL(ch->desc->original) < LVL_IMPL)
      return;

   arg1=get_buffer(MAX_INPUT_LENGTH);
   arg2=get_buffer(MAX_INPUT_LENGTH);
   two_arguments(argument, arg1, arg2);

   if (!*arg1 || !*arg2 || !is_number(arg2) || ((vnumber = atoi(arg2)) < 0))
      {
      mob_log(ch, "mload: bad syntax");
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }

   if (is_abbrev(arg1, "mob"))
      {
      if ((mob = read_mobile(vnumber, VIRTUAL)) == NULL)
         {
         mob_log(ch, "mload: bad mob vnum");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      GET_MOB_VAL(mob,0)=GET_ROOM_VNUM(IN_ROOM(ch));
      mob->orig_room=IN_ROOM(ch);
      char_to_room(mob, IN_ROOM(ch));
      load_mtrigger(mob);
      }

   else if (is_abbrev(arg1, "obj"))
      {
      if ((object = read_object(vnumber, VIRTUAL)) == NULL)
         {
         mob_log(ch, "mload: bad object vnum");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      if (CAN_WEAR(object, ITEM_WEAR_TAKE))
         {
         obj_to_char(object, ch);
         }
      else
         {
         obj_to_room(object, IN_ROOM(ch));
         }
      load_otrigger(object);
      }

   else
      mob_log(ch, "mload: bad type");
   release_buffer(arg2);
   release_buffer(arg1);
   }


/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
ACMD(do_mpurge)
   {
   char *arg;
   char_data *victim;
   obj_data  *obj;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      /* 'purge' */
      char_data *vnext;
      obj_data  *obj_next;

      for (victim = world[IN_ROOM(ch)].people; victim; victim = vnext)
         {
         vnext = victim->next_in_room;
         if (IS_NPC(victim) && victim != ch)
            extract_char(victim);
         }

      for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj_next)
         {
         obj_next = obj->next_content;
         extract_obj(obj);
         }
      release_buffer(arg);
      return;
      }

   if (*arg == UID_CHAR)
      victim = get_char_dg(arg);
   else
      victim = get_char_room_vis(ch, arg);

   if (victim == NULL)
      {
      if ((obj = get_obj_vis(ch, arg)))
         {
         extract_obj(obj);
         }
      else
         mob_log(ch, "mpurge: bad argument");

      release_buffer(arg);
      return;
      }

   if (!IS_NPC(victim))
      {
      mob_log(ch, "mpurge: purging a PC");
      release_buffer(arg);
      return;
      }

   if (victim==ch)
      dg_owner_purged = 1;

   extract_char(victim);
   release_buffer(arg);
   }


/* lets the mobile goto any location it wishes that is not private */
ACMD(do_mgoto)
   {
   char *arg;
   room_rnum location;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      mob_log(ch, "mgoto called with no argument");
      release_buffer(arg);
      return;
      }

   if ((location = find_target_room(ch, arg)) == NOWHERE)
      {
      mob_log(ch, "mgoto: invalid location");
      release_buffer(arg);
      return;
      }

   if (FIGHTING(ch))
      stop_fighting(ch);

   char_from_room(ch);
   char_to_room(ch, location);
   rage_check(ch);
   release_buffer(arg);
   }


/* lets the mobile do a command at another location. Very useful */
ACMD(do_mat)
   {
   char *arg;
   room_rnum location;
   room_rnum original;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   argument = one_argument( argument, arg );

   if (!*arg || !*argument)
      {
      mob_log(ch, "mat: bad argument");
      release_buffer(arg);
      return;
      }

   if ((location = find_target_room(ch, arg)) == NOWHERE)
      {
      mob_log(ch, "mat: invalid location");
      release_buffer(arg);
      return;
      }

   original = IN_ROOM(ch);
   char_from_room(ch);
   char_to_room(ch, location);
   command_interpreter(ch, argument);

   /*
    * See if 'ch' still exists before continuing!
    * Handles 'at XXXX quit' case.
    */
   if (IN_ROOM(ch) == location)
      {
      char_from_room(ch);
      char_to_room(ch, original);
      }
   release_buffer(arg);
   }


/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 */
ACMD(do_mteleport)
   {
   char *arg1;
   char *arg2;
   room_rnum target;
   char_data *vict, *next_ch;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   arg1=get_buffer(MAX_INPUT_LENGTH);
   arg2=get_buffer(MAX_INPUT_LENGTH);
   argument = two_arguments(argument, arg1, arg2);

   if (!*arg1 || !*arg2)
      {
      mob_log(ch, "mteleport: bad syntax");
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }

   target = find_target_room(ch, arg2);

   if (target == NOWHERE)
      mob_log(ch, "mteleport target is an invalid room");

   else if (!str_cmp(arg1, "all"))
      {
      if (target == IN_ROOM(ch))
         {
         mob_log(ch, "mteleport all target is itself");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }

      for (vict = world[IN_ROOM(ch)].people; vict; vict = next_ch)
         {
         next_ch = vict->next_in_room;

         if (IS_NPC(vict) || !PRF_FLAGGED(vict,PRF_NOHASSLE))
            {
            char_from_room(vict);
            char_to_room(vict, target);
            if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_DEATH))
               {
               if (GET_LEVEL(vict) < LVL_IMMORT)
                  {

		    if (handleGetOutOfDeathFree(vict)) {
		      return;
		    }

                  log_death_trap(vict);
                  death_cry(vict);
                  extract_char(vict);
                  }
               }
            }
         }
      }

   else
      {
      if (*arg1 == UID_CHAR)
         {
         if (!(vict = get_char_dg(arg1)))
            {
            char *buf=get_buffer(256);
            sprintf(buf, "mteleport: victim (%s) does not exist",arg1);
            mob_log(ch, buf);
            release_buffer(buf);
            release_buffer(arg2);
            release_buffer(arg1);
            return;
            }
         }
      else if (!(vict = get_char_vis(ch, arg1,FIND_CHAR_WORLD)))
         {
         char *buf=get_buffer(256);
         sprintf(buf, "mteleport: victim (%s) does not exist",arg1);
         mob_log(ch, buf);
         release_buffer(buf);
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }

      if (IS_NPC(vict) || !PRF_FLAGGED(vict,PRF_NOHASSLE))
         {
         char_from_room(vict);
         char_to_room(vict, target);
         if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_DEATH))
            {
            if (GET_LEVEL(vict) < LVL_IMMORT)
               {
		 if (handleGetOutOfDeathFree(vict)) {
		   return;
		 }

               log_death_trap(vict);
               death_cry(vict);
               extract_char(vict);
               }
            }
         }
      }
   release_buffer(arg2);
   release_buffer(arg1);
   }


/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
ACMD(do_mforce)
   {
   char *arg;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
      return;
   arg=get_buffer(MAX_INPUT_LENGTH);
   argument = one_argument(argument, arg);

   if (!*arg || !*argument)
      {
      mob_log(ch, "mforce: bad syntax");
      release_buffer(arg);
      return;
      }

   if (!str_cmp(arg, "all"))
      {
      struct descriptor_data *i;
      char_data *vch;

      for (i = descriptor_list; i ; i = i->next)
         {
         if ((i->character != ch) && !i->connected &&
                 (IN_ROOM(i->character) == IN_ROOM(ch)))
            {
            vch = i->character;
            if (GET_LEVEL(vch) < GET_LEVEL(ch) && CAN_SEE(ch, vch) &&
                    (IS_NPC(vch) || !PRF_FLAGGED(vch,PRF_NOHASSLE)))
               {
               command_interpreter(vch, argument);
               }
            }
         }
      }
   else
      {
      char_data *victim;

      if (*arg == UID_CHAR)
         {
         if (!(victim = get_char_dg(arg)))
            {
            char *buf=get_buffer(256);
            sprintf(buf, "mforce: victim (%s) does not exist",arg);
            mob_log(ch, buf);
            release_buffer(buf);
            release_buffer(arg);
            return;
            }
         }
      else if ((victim = get_char_room_vis(ch, arg)) == NULL)
         {
         mob_log(ch, "mforce: no such victim");
         release_buffer(arg);
         return;
         }

      if (victim == ch)
         {
         mob_log(ch, "mforce: forcing self");
         release_buffer(arg);
         return;
         }

      if (IS_NPC(victim) || !PRF_FLAGGED(victim,PRF_NOHASSLE))
         command_interpreter(victim, argument);
      }
   release_buffer(arg);
   }


/* increases the target's exp */
ACMD(do_mexp)
   {
   char_data *victim;
   char *name;
   char *amount;

   mob_log(ch, "WARNING: mexp command is deprecated! Use: %actor.exp(amount-to-add)%");

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
      return;

   name=get_buffer(MAX_INPUT_LENGTH);
   amount=get_buffer(MAX_INPUT_LENGTH);
   two_arguments(argument, name, amount);

   if (!*name || !*amount)
      {
      mob_log(ch, "mexp: too few arguments");
      release_buffer(amount);
      release_buffer(name);
      return;
      }

   if (*name == UID_CHAR)
      {
      if (!(victim = get_char_dg(name)))
         {
         char *buf=get_buffer(256);
         sprintf(buf, "mexp: victim (%s) does not exist",name);
         mob_log(ch, buf);
         release_buffer(buf);
         release_buffer(amount);
         release_buffer(name);
         return;
         }
      }
   else if (!(victim = get_char_vis(ch, name,FIND_CHAR_WORLD)))
      {
      char *buf=get_buffer(256);
      sprintf(buf, "mexp: victim (%s) does not exist",name);
      mob_log(ch, buf);
      release_buffer(buf);
      release_buffer(amount);
      release_buffer(name);
      return;
      }

   gain_exp(victim, atoi(amount));
   release_buffer(amount);
   release_buffer(name);
   }


/* increases the target's gold */
ACMD(do_mgold)
   {
   char_data *victim;
   char *name;
   char *amount;

   mob_log(ch, "WARNING: mgold command is deprecated! Use: %actor.gold(amount-to-add)%");

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
      return;

   name=get_buffer(MAX_INPUT_LENGTH);
   amount=get_buffer(MAX_INPUT_LENGTH);
   two_arguments(argument, name, amount);

   if (!*name || !*amount)
      {
      mob_log(ch, "mgold: too few arguments");
      release_buffer(amount);
      release_buffer(name);
      return;
      }

   if (*name == UID_CHAR)
      {
      if (!(victim = get_char_dg(name)))
         {
         char *buf=get_buffer(256);
         sprintf(buf, "mgold: victim (%s) does not exist",name);
         mob_log(ch, buf);
         release_buffer(buf);
         release_buffer(amount);
         release_buffer(name);
         return;
         }
      }
   else if (!(victim = get_char_vis(ch, name,FIND_CHAR_WORLD)))
      {
      char *buf=get_buffer(256);
      sprintf(buf, "mgold: victim (%s) does not exist",name);
      mob_log(ch, buf);
      release_buffer(buf);
      release_buffer(amount);
      release_buffer(name);
      return;
      }

   if ((GET_GOLD(victim) += atoi(amount)) < 0)
      {
      mob_log(ch, "mgold subtracting more gold than character has");
      GET_GOLD(victim) = 0;
      }
   release_buffer(amount);
   release_buffer(name);
   }

/* hunt for someone */
ACMD(do_mhunt)
   {
   char_data *victim;
   char *arg;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
      return;

   if (FIGHTING(ch))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      mob_log(ch, "mhunt called with no argument");
      release_buffer(arg);
      return;
      }

   if (*arg == UID_CHAR)
      {
      if (!(victim = get_char_dg(arg)))
         {
         char *buf=get_buffer(256);
         sprintf(buf, "mhunt: victim (%s) does not exist", arg);
         mob_log(ch, buf);
         release_buffer(buf);
         release_buffer(arg);
         return;
         }
      }
   else if (!(victim = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
      {
      char *buf=get_buffer(256);
      sprintf(buf, "mhunt: victim (%s) does not exist", arg);
      mob_log(ch, buf);
      release_buffer(buf);
      release_buffer(arg);
      return;
      }
/*   HUNTING(ch) = victim; */
   remember(ch, victim);
   release_buffer(arg);
   }


/* place someone into the mob's memory list */
ACMD(do_mremember)
   {
   char_data *victim;
   struct script_memory *mem;
   char *arg;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   argument = one_argument(argument, arg);

   if (!*arg)
      {
      mob_log(ch, "mremember: bad syntax");
      release_buffer(arg);
      return;
      }

   if (*arg == UID_CHAR)
      {
      if (!(victim = get_char_dg(arg)))
         {
         char *buf=get_buffer(256);
         sprintf(buf, "mremember: victim (%s) does not exist", arg);
         mob_log(ch, buf);
         release_buffer(buf);
         release_buffer(arg);
         return;
         }
      }
   else if (!(victim = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
      {
      char *buf=get_buffer(256);
      sprintf(buf, "mremember: victim (%s) does not exist", arg);
      mob_log(ch, buf);
      release_buffer(buf);
      release_buffer(arg);
      return;
      }

   /* create a structure and add it to the list */
   CREATE(mem, struct script_memory, 1);
   if (!SCRIPT_MEM(ch))
      SCRIPT_MEM(ch) = mem;
   else
      {
      struct script_memory *tmpmem = SCRIPT_MEM(ch);
      while (tmpmem->next)
         tmpmem = tmpmem->next;
      tmpmem->next = mem;
      }

   /* fill in the structure */
   mem->id = GET_ID(victim);
   if (argument && *argument)
      {
      mem->cmd = strdup(argument);
      }
   release_buffer(arg);
   }


/* remove someone from the list */
ACMD(do_mforget)
   {
   char_data *victim;
   struct script_memory *mem, *prev;
   char *arg;

   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      mob_log(ch, "mforget: bad syntax");
      release_buffer(arg);
      return;
      }

   if (*arg == UID_CHAR)
      {
      if (!(victim = get_char_dg(arg)))
         {
         char *buf=get_buffer(256);
         sprintf(buf, "mforget: victim (%s) does not exist", arg);
         mob_log(ch, buf);
         release_buffer(buf);
         release_buffer(arg);
         return;
         }
      }
   else if (!(victim = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
      {
      char *buf=get_buffer(256);
      sprintf(buf, "mforget: victim (%s) does not exist", arg);
      mob_log(ch, buf);
      release_buffer(buf);
      release_buffer(arg);
      return;
      }

   mem = SCRIPT_MEM(ch);
   prev = NULL;
   while (mem)
      {
      if (mem->id == GET_ID(victim))
         {
         if (mem->cmd)
            free(mem->cmd);
         if (prev==NULL)
            {
            SCRIPT_MEM(ch) = mem->next;
            free(mem);
            mem = SCRIPT_MEM(ch);
            }
         else
            {
            prev->next = mem->next;
            free(mem);
            mem = prev->next;
            }
         }
      else
         {
         prev = mem;
         mem = mem->next;
         }
      }
   release_buffer(arg);
   }


/* transform into a different mobile */
ACMD(do_mtransform)
   {
   char *arg;
   char_data *m, tmpmob;
   obj_data *obj[NUM_WEARS];
   int pos;
   int keep_hp = 1; /* new mob keeps the old mob's hp/max hp/exp */


   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;

   if (ch->desc)
      {
      send_to_char(ch,"You've got no VNUM to return to, dummy! try 'switch'\r\n");
      return;
      }

   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      mob_log(ch, "mtransform: missing argument");
   else if (!isdigit((int)*arg)&&(*arg!='-'))
      mob_log(ch, "mtransform: bad argument");
   else
      {
      if(isdigit((int)*arg))
         m = read_mobile(atoi(arg), VIRTUAL);
      else
         {
         keep_hp=0;
         m = read_mobile(atoi(arg+1), VIRTUAL);
         }

      if (m==NULL)
         {
         mob_log(ch, "mtransform: bad mobile vnum");
         release_buffer(arg);
         return;
         }

      /* move new obj info over to old object and delete new obj */

      for (pos = 0; pos < NUM_WEARS; pos++)
         {
         if (GET_EQ(ch, pos))
            obj[pos] = unequip_char(ch, pos);
         else
            obj[pos] = NULL;
         }

      /* put the mob in the same room as ch so extract will work */
      char_to_room(m, IN_ROOM(ch));

      memcpy(&tmpmob, m, sizeof(*m));
      tmpmob.id = ch->id;
      tmpmob.affected = ch->affected;
      tmpmob.carrying = ch->carrying;
      tmpmob.proto_script = ch->proto_script;
      tmpmob.script = ch->script;
      tmpmob.memory = ch->memory;
      tmpmob.next_in_room = ch->next_in_room;
      tmpmob.next = ch->next;
      tmpmob.next_fighting = ch->next_fighting;
      tmpmob.followers = ch->followers;
      tmpmob.master = ch->master;
      GET_MOB_VAL(&tmpmob,0)=GET_MOB_VAL(ch,0);
      tmpmob.orig_room=ch->orig_room;

      GET_WAS_IN(&tmpmob) = GET_WAS_IN(ch);
      if(keep_hp)
         {
         GET_HIT(&tmpmob) = GET_HIT(ch);
         GET_MAX_HIT(&tmpmob) = GET_MAX_HIT(ch);
         GET_EXP(&tmpmob) = GET_EXP(ch);
         }
      GET_GOLD(&tmpmob) = GET_GOLD(ch);
      GET_POS(&tmpmob) = GET_POS(ch);
      IS_CARRYING_W(&tmpmob) = IS_CARRYING_W(ch);
      IS_CARRYING_N(&tmpmob) = IS_CARRYING_N(ch);
      FIGHTING(&tmpmob) = FIGHTING(ch);
      HUNTING(&tmpmob) = HUNTING(ch);
      memcpy(ch, &tmpmob, sizeof(*ch));

      for (pos = 0; pos < NUM_WEARS; pos++)
         {
         if (obj[pos])
            equip_char(ch, obj[pos], pos);
         }

      extract_char(m);
      }
   release_buffer(arg);
   }


ACMD(do_mdoor)
   {
   char *target, *direction, *field, *value;
   room_data *rm;
   struct room_direction_data *dexit;
   int dir, fd, to_room;

   char *door_field[] =
      {
         "purge",
         "description",
         "flags",
         "key",
         "name",
         "room",
         "\n"
      };


   if (!MOB_OR_IMPL(ch))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      return;
   target=get_buffer(MAX_INPUT_LENGTH);
   direction=get_buffer(MAX_INPUT_LENGTH);
   field=get_buffer(MAX_INPUT_LENGTH);
   argument = two_arguments(argument, target, direction);
   value = one_argument(argument, field);
   skip_spaces(&value);

   if (!*target || !*direction || !*field)
      {
      mob_log(ch, "mdoor called with too few args");
      release_buffer(field);
      release_buffer(direction);
      release_buffer(target);
      return;
      }

   if ((rm = get_room(target)) == NULL)
      {
      mob_log(ch, "mdoor: invalid target");
      release_buffer(field);
      release_buffer(direction);
      release_buffer(target);
      return;
      }

   if ((dir = search_block(direction, dirs, FALSE)) == -1)
      {
      mob_log(ch, "mdoor: invalid direction");
      release_buffer(field);
      release_buffer(direction);
      release_buffer(target);
      return;
      }

   if ((fd = search_block(field, door_field, FALSE)) == -1)
      {
      mob_log(ch, "mdoor: invalid field");
      release_buffer(field);
      release_buffer(direction);
      release_buffer(target);
      return;
      }

   dexit = rm->dir_option[dir];

   /* purge exit */
   if (fd == 0)
      {
      if (dexit)
         {
         if (dexit->general_description)
            free(dexit->general_description);
         if (dexit->keyword)
            free(dexit->keyword);
         free(dexit);
         rm->dir_option[dir] = NULL;
         }
      }

   else
      {
      if (!dexit)
         {
         CREATE(dexit, struct room_direction_data, 1);
         rm->dir_option[dir] = dexit;
         dexit->to_room = -1;
         }

      switch (fd)
         {
      case 1:  /* description */
         if (dexit->general_description)
            free(dexit->general_description);
         CREATE(dexit->general_description, char, strlen(value) + 3);
         strcpy(dexit->general_description, value);
         strcat(dexit->general_description, "\r\n");
         break;
      case 2:  /* flags       */
         dexit->exit_info = (sh_int)asciiflag_conv(value);
         break;
      case 3:  /* key         */
         dexit->key = atoi(value);
         break;
      case 4:  /* name        */
         if (dexit->keyword)
            free(dexit->keyword);
         CREATE(dexit->keyword, char, strlen(value) + 1);
         strcpy(dexit->keyword, value);
         break;
      case 5:  /* room        */
         if ((to_room = real_room(atoi(value))) != NOWHERE)
            dexit->to_room = to_room;
         else
            mob_log(ch, "mdoor: invalid door target");
         break;
         }
      }
   release_buffer(field);
   release_buffer(direction);
   release_buffer(target);
   }

/* usage: maddqp %actor.name% <number of QPs> <quest name> */
ACMD(do_maddqp)
{
  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch,"Huh?!?\r\n");
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    return;
  }

  char *buf = get_buffer(MAX_STRING_LENGTH);
  strcpy(buf, argument);
  
  char *completed_by = strtok(buf, " ");
  char *how_many_s = strtok(NULL, " ");
  char *quest_name = strtok(NULL, " ");
  if (!quest_name || !how_many_s || !completed_by) {
    log("SYSERR: do_maddqp called with insufficient arguments (completed_by=%s, how_many=%s quest_name=%s).", completed_by, how_many_s, quest_name);
    release_buffer(buf);
    return;
  }
  
  struct char_data *victim = get_char(completed_by);
  if (!victim) {
    log("SYSERR: do_maddqp(%s, %s) could not find player %s.", completed_by, quest_name, completed_by);
    release_buffer(buf);
    return;
  }
  if (IS_NPC(victim)) {
    log("SYSERR: do_maddqp(%s, %s) found a MOB? (%s)!", completed_by, quest_name, GET_NAME(victim));
    release_buffer(buf);
    return;
  }

  int how_many = atoi(how_many_s);

  struct dg_quest *quest = get_dg_quest(quest_name);
  if (!quest) {
    /* We have to assume it's a new quest. */
    dg_quests = (struct dg_quest *)realloc(dg_quests, ++num_dg_quests * sizeof(struct dg_quest));
    quest = &dg_quests[num_dg_quests-1];
    quest->quest_name = strdup(quest_name);
    quest->num_completed = 0;
    quest->completed_by = NULL;
  }
  
  int i;
  for (i = 0; i < quest->num_completed; i++) {
    if (victim->pfilepos == quest->completed_by[i]) {
      release_buffer(buf);
      return;
    }
  }

  quest->completed_by = (int *)realloc(quest->completed_by, ++quest->num_completed * sizeof(int));
  quest->completed_by[quest->num_completed-1] = victim->pfilepos;

  mudlogf(BRF, LVL_IMMORT, TRUE, "DG-Quest: %s has completed quest %s and was awarded %d Quest point%s.", GET_NAME(victim), quest->quest_name, how_many, how_many!=1 ? "s" : "");

  for (i = 0; i < how_many; i++) {
    GET_QPOINTS(victim)++;
    send_to_char(victim, "You have been rewarded with a quest point.\r\n");
  }
  save_dg_quests();
  release_buffer(buf);
}

/* Usage: mdoonce %actor.name% QuestLabel <unique command> */
ACMD(do_mdoonce)
{
  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch,"Huh?!?\r\n");
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    return;
  }

  char *buf = get_buffer(MAX_STRING_LENGTH);
  strcpy(buf, argument);
  
  char *completed_by = strtok(buf, " ");
  char *quest_name = strtok(NULL, " ");
  char *command = strtok(NULL, "");
  if (!completed_by || !quest_name || !command) {
    log("SYSERR: do_mdoonce called with insufficient arguments completed_by=%s quest_name=%s command=%s.", completed_by, quest_name, command);
    release_buffer(buf);
    return;
  }
  
  struct char_data *victim = get_char(completed_by);
  if (!victim) {
    log("SYSERR: do_mdoonce could not find player %s.", completed_by);
    release_buffer(buf);
    return;
  }
  if (IS_NPC(victim)) {
    log("SYSERR: do_mdoonce found a MOB? %s!", GET_NAME(victim));
    release_buffer(buf);
    return;
  }

  struct dg_quest *quest = get_dg_quest(quest_name);
  if (!quest) {
    /* We have to assume it's a new quest. */
    dg_quests = (struct dg_quest *)realloc(dg_quests, ++num_dg_quests * sizeof(struct dg_quest));
    quest = &dg_quests[num_dg_quests-1];
    quest->quest_name = strdup(quest_name);
    quest->num_completed = 0;
    quest->completed_by = NULL;
  }
  
  int i;
  for (i = 0; i < quest->num_completed; i++) {
    if (victim->pfilepos == quest->completed_by[i]) {
      release_buffer(buf);
      return;
    }
  }

  quest->completed_by = (int *)realloc(quest->completed_by, ++quest->num_completed * sizeof(int));
  quest->completed_by[quest->num_completed-1] = victim->pfilepos;

  mudlogf(BRF, LVL_IMMORT, TRUE, "DG-Quest: %s has completed quest %s and will now execute command \"%s\".", GET_NAME(victim), quest->quest_name, command);

  command_interpreter(ch, command);
  save_dg_quests();

  release_buffer(buf);
}

extern float race_exp_multipliers[];
extern float class_exp_multipliers[];
extern int exp_table[LVL_IMPL + 1];  

extern void gremort_record_quest_result(struct char_data *ch, int result);

ACMD(do_mgremort)
{
  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch,"Huh?!?\r\n");
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    return;
  }

  skip_spaces(&argument);
  if (!*argument) {	
    log("SYSERR: MGREMORT missing argument.");
    return;
  }
  struct char_data *victim = get_char(argument);
  
  if (!victim) {
    log("SYSERR: MGREMORT called on a non-existant argument (%s)!", argument);
    return;
  }
  if (IS_NPC(victim)) {
    log("SYSERR: MGREMORT called on a mob (%s)!", GET_NAME(victim));
    return;
  }
  if (GET_LEVEL(victim) != LVL_HERO-1) {
    log("SYSERR: MGREMORT illegal level for player %s!", GET_NAME(victim));
    return;
  }
  /*
  if (GET_EXP_FOR_CH(victim) - GET_EXP(victim) >= 0) {
    log("SYSERR: MGREMORT called on player %s with positive exp!", GET_NAME(victim));
    return;
  }
  */
  
  GET_EXP(victim) -= GET_EXP_FOR_CH(victim);

  gremort_record_quest_result(victim, GREMORT_RESULT_PASSED_QUEST);

  if (REMORT_LEVEL(victim) == NON_REMORT) {
    GET_LEVEL(victim)++;
    advance_level(victim, TRUE);
    send_info("[ INFO ] %s is now a HERO!!!!!\r\n", GET_NAME(victim));
    cw_give_first(victim);
  } else if (REMORT_LEVEL(victim) == SINGLE_REMORT) {
    GET_LEVEL(victim) += 2;
    advance_level(victim, FALSE);
    advance_level(victim, TRUE);
    send_info("[ INFO ] %s is now an ANGEL!!!!!\r\n", GET_NAME(victim));
  } else if (REMORT_LEVEL(victim) == DOUBLE_REMORT) {
    GET_LEVEL(victim) += 3;
    advance_level(victim, FALSE);
    advance_level(victim, FALSE);
    advance_level(victim, TRUE);
    send_info("[ INFO ] %s is now an AVATAR!!!!!\r\n", GET_NAME(victim));
    GET_COND(victim, FULL) = -1;
    GET_COND(victim, THIRST) = -1; 
  } else {
    log("SYSERR: invalid remortlevel %d for %s.", REMORT_LEVEL(victim), GET_NAME(victim));
  }
 
  mudlogf(CMP,LVL_IMMORT, TRUE, "(GC) %s has been gremorted to level %d by %s.", GET_NAME(victim), GET_LEVEL(victim), GET_NAME(ch));  
}
