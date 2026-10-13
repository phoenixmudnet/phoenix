/**************************************************************************
*  File: objcmd.c                                                         *
*  Usage: contains the command_interpreter for objects,                   *
*         object commands.                                                *
*                                                                         *
*                                                                         *
*  $Author: lucas $
*  $Date: 2006/09/15 02:02:06 $
*  $Revision: 1.1.1.1 $
**************************************************************************/

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"


#include "structs.h"
#include "screen.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "buffer.h"
#include "constants.h"
#include "hero_state.h"

extern struct room_data *world;
extern struct index_data *obj_index;
extern int dg_owner_purged;

char_data *get_char_by_obj(obj_data *obj, char *name);
obj_data *get_obj_by_obj(obj_data *obj, char *name);
void sub_write(char *arg, char_data *ch, byte find_invis, int targets);
void die(struct char_data * ch, struct char_data *killer);
void script_log(char *msg);
room_data *get_room(char *name);
long asciiflag_conv(char *flag);
void death_cry(struct char_data *ch);
int handleGetOutOfDeathFree(struct char_data*);

#define OCMD(name)  \
void (name)(obj_data *obj, char *argument, int cmd, int subcmd)


struct obj_command_info {
   char *command;
   void (*command_pointer)(obj_data *obj, char *argument, int cmd, int subcmd);
   int subcmd;
   };


/* do_osend */
#define SCMD_OSEND         0
#define SCMD_OECHOAROUND   1



/* attaches object name and vnum to msg and sends it to script_log */
void obj_log(obj_data *obj, char *msg)
   {
   char *buf=get_buffer(MAX_INPUT_LENGTH + 100);


   sprintf(buf, "Obj (%s, VNum %ld): %s",
           obj->short_description, GET_OBJ_VNUM(obj), msg);
   script_log(buf);
   release_buffer(buf);
   }


/* returns the real room number that the object or object's carrier is in */
int obj_room(obj_data *obj)
   {
   if (obj->in_room != NOWHERE)
      return obj->in_room;
   else if (obj->carried_by)
      return IN_ROOM(obj->carried_by);
   else if (obj->worn_by)
      return IN_ROOM(obj->worn_by);
   else if (obj->in_obj)
      return obj_room(obj->in_obj);
   else
      return NOWHERE;
   }


/* returns the real room number, or NOWHERE if not found or invalid */
room_rnum find_obj_target_room(obj_data *obj, char *rawroomstr)
   {
   int tmp;
   room_rnum location;
   char_data *target_mob;
   obj_data *target_obj;
   char *roomstr=get_buffer(MAX_INPUT_LENGTH);

   one_argument(rawroomstr, roomstr);

   if (!*roomstr)
      {
      release_buffer(roomstr);
      return NOWHERE;
      }

   if (isdigit((int)*roomstr) && !strchr(roomstr, '.'))
      {
      tmp = atoi(roomstr);
      if ((location = real_room(tmp)) < 0)
         {
         release_buffer(roomstr);
         return NOWHERE;
         }
      }
   else if ((target_mob = get_char_by_obj(obj, roomstr)))
      location = IN_ROOM(target_mob);
   else if ((target_obj = get_obj_by_obj(obj, roomstr)))
      {
      if (target_obj->in_room != NOWHERE)
         location = target_obj->in_room;
      else
         {
         release_buffer(roomstr);
         return NOWHERE;
         }
      }
   else
      {
      release_buffer(roomstr);
      return NOWHERE;
      }

   /* a room has been found.  Check for permission */
   if (ROOM_FLAGGED(location, ROOM_GODROOM) ||
#ifdef ROOM_IMPROOM
           ROOM_FLAGGED(location, ROOM_IMPROOM) ||
#endif
           ROOM_FLAGGED(location, ROOM_HOUSE))
      {
      release_buffer(roomstr);
      return NOWHERE;
      }

   if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
           world[location].people && world[location].people->next_in_room)
      {
      release_buffer(roomstr);
      return NOWHERE;
      }

   release_buffer(roomstr);
   return location;
   }



/* Object commands */

OCMD(do_oecho)
   {
   int room;

   skip_spaces(&argument);

   if (!*argument)
      obj_log(obj, "oecho called with no args");

   else if ((room = obj_room(obj)) != NOWHERE)
      {
      if (world[room].people)
         sub_write(argument, world[room].people, TRUE, TO_ROOM | TO_CHAR);
      }

   else
      obj_log(obj, "oecho called by object in NOWHERE");
   }


OCMD(do_oforce)
   {
   char_data *ch, *next_ch;
   int room;
   char *arg1=get_buffer(MAX_INPUT_LENGTH);
   char *line;

   line = one_argument(argument, arg1);

   if (!*arg1 || !*line)
      {
      obj_log(obj, "oforce called with too few args");
      release_buffer(arg1);
      return;
      }

   if (!str_cmp(arg1, "all"))
      {
      if ((room = obj_room(obj)) == NOWHERE)
         obj_log(obj, "oforce called by object in NOWHERE");
      else
         {
         for (ch = world[room].people; ch; ch = next_ch)
            {
            next_ch = ch->next_in_room;

            if (IS_NPC(ch) || !PRF_FLAGGED(ch,PRF_NOHASSLE))
               {
               command_interpreter(ch, line);
               }
            }
         }
      }

   else
      {
      if ((ch = get_char_by_obj(obj, arg1)))
         {
         if (IS_NPC(ch) || !PRF_FLAGGED(ch,PRF_NOHASSLE))
            {
            command_interpreter(ch, line);
            }
         }

      else
         obj_log(obj, "oforce: no target found");
      }
   release_buffer(arg1);
   }


OCMD(do_osend)
   {
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   char *msg;
   char_data *ch;

   msg = any_one_arg(argument, buf);

   if (!*buf)
      {
      obj_log(obj, "osend called with no args");
      release_buffer(buf);
      return;
      }

   skip_spaces(&msg);

   if (!*msg)
      {
      obj_log(obj, "osend called without a message");
      release_buffer(buf);
      return;
      }

   if ((ch = get_char_by_obj(obj, buf)))
      {
      if (subcmd == SCMD_OSEND)
         sub_write(msg, ch, TRUE, TO_CHAR);
      else if (subcmd == SCMD_OECHOAROUND)
         sub_write(msg, ch, TRUE, TO_ROOM);
      }

   else
      obj_log(obj, "no target found for osend");
   release_buffer(buf);
   }

/* increases the target's exp */
OCMD(do_oexp)
   {
   char_data *ch;
   char *name=get_buffer(MAX_INPUT_LENGTH);
   char *amount=get_buffer(MAX_INPUT_LENGTH);

   two_arguments(argument, name, amount);

   if (!*name || !*amount)
      {
      obj_log(obj, "oexp: too few arguments");
      release_buffer(amount);
      release_buffer(name);
      return;
      }

   if ((ch = get_char_by_obj(obj, name)))
      gain_exp(ch, atoi(amount));
   else
      {
      obj_log(obj, "oexp: target not found");
      release_buffer(amount);
      release_buffer(name);
      return;
      }
   release_buffer(amount);
   release_buffer(name);
   }

/* set the object's timer value */
OCMD(do_otimer)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, arg);

   if (!*arg)
      obj_log(obj, "otimer: missing argument");
   else if (!isdigit((int)*arg))
      obj_log(obj, "otimer: bad argument");
   else
      GET_OBJ_DGTIMER(obj) = atoi(arg);
   release_buffer(arg);
   }

/* transform into a different object */
/* note: this shouldn't be used with containers unless both objects */
/* are containers! */
OCMD(do_otransform)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   obj_data *o, tmpobj;
   struct char_data *wearer=NULL;
   int pos=-1;

   one_argument(argument, arg);

   if (!*arg)
      obj_log(obj, "otransform: missing argument");
   else if (!isdigit((int)*arg))
      obj_log(obj, "otransform: bad argument");
   else
      {
      o = read_object(atoi(arg), VIRTUAL);
      if (o==NULL)
         {
         obj_log(obj, "otransform: bad object vnum");
         release_buffer(arg);
         return;
         }

      if (obj->worn_by)
         {
         pos = obj->worn_on;
         wearer = obj->worn_by;
         unequip_char(obj->worn_by, pos);
         }

      /* move new obj info over to old object and delete new obj */
      memcpy(&tmpobj, o, sizeof(*o));
      tmpobj.in_room = obj->in_room;
      tmpobj.carried_by = obj->carried_by;
      tmpobj.worn_by = obj->worn_by;
      tmpobj.worn_on = obj->worn_on;
      tmpobj.in_obj = obj->in_obj;
      tmpobj.contains = obj->contains;
      tmpobj.id = obj->id;
      tmpobj.proto_script = obj->proto_script;
      tmpobj.script = obj->script;
      tmpobj.next_content = obj->next_content;
      tmpobj.next = obj->next;
      tmpobj.obj_flags.timer = obj->obj_flags.timer;
      /* the copy's own: a class weapon's state and a binding */
      tmpobj.hero = obj->hero;
      tmpobj.attune = obj->attune;
      tmpobj.bound_idnum = obj->bound_idnum;
      memcpy(tmpobj.bound_name, obj->bound_name, sizeof(tmpobj.bound_name));
      tmpobj.checkpoint_of = obj->checkpoint_of;
      memcpy(obj, &tmpobj, sizeof(*obj));

      if (wearer)
         {
         equip_char(wearer, obj, pos);
         }

      extract_obj(o);
      }
   release_buffer(arg);
   }



/* purge all objects an npcs in room, or specified object or mob */
OCMD(do_opurge)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   char_data *ch, *next_ch;
   obj_data *o, *next_obj;
   int rm;

   one_argument(argument, arg);

   if (!*arg)
      {
      if ((rm = obj_room(obj)) != NOWHERE)
         {
         for (ch = world[rm].people; ch; ch = next_ch )
            {
            next_ch = ch->next_in_room;
            if (IS_NPC(ch))
               extract_char(ch);
            }

         for (o = world[rm].contents; o; o = next_obj )
            {
            next_obj = o->next_content;
            if (o != obj)
               extract_obj(o);
            }
         }
      release_buffer(arg);
      return;
      }

   if (!(ch = get_char_by_obj(obj, arg)))
      {
      if ((o = get_obj_by_obj(obj, arg)))
         {
         if (o==obj)
            dg_owner_purged = 1;
         extract_obj(o);
         }
      else
         obj_log(obj, "opurge: bad argument");
      release_buffer(arg);
      return;
      }

   if (!IS_NPC(ch))
      {
      obj_log(obj, "opurge: purging a PC");
      release_buffer(arg);
      return;
      }

   extract_char(ch);
   release_buffer(arg);
   }


OCMD(do_oteleport)
   {
   char_data *ch, *next_ch;
   room_rnum target, rm;
   char *arg1=get_buffer(MAX_INPUT_LENGTH);
   char *arg2=get_buffer(MAX_INPUT_LENGTH);

   two_arguments(argument, arg1, arg2);

   if (!*arg1 || !*arg2)
      {
      obj_log(obj, "oteleport called with too few args");
      return;
      }

   target = find_obj_target_room(obj, arg2);

   if (target == NOWHERE)
      obj_log(obj, "oteleport target is an invalid room");

   else if (!str_cmp(arg1, "all"))
      {
      rm = obj_room(obj);
      if (target == rm)
         obj_log(obj, "oteleport target is itself");

      for (ch = world[rm].people; ch; ch = next_ch)
         {
         next_ch = ch->next_in_room;

         if (IS_NPC(ch) || !PRF_FLAGGED(ch,PRF_NOHASSLE))
            {
            char_from_room(ch);
            char_to_room(ch, target);
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
         }
      }
   else
      {
      if ((ch = get_char_by_obj(obj, arg1)))
         {
         if (IS_NPC(ch) || !PRF_FLAGGED(ch,PRF_NOHASSLE))
            {
            char_from_room(ch);
            char_to_room(ch, target);
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
         }
      else
         obj_log(obj, "oteleport: no target found");
      }
   release_buffer(arg1);
   release_buffer(arg2);
   }


OCMD(do_dgoload)
   {
   char *arg1=get_buffer(MAX_INPUT_LENGTH);
   char *arg2=get_buffer(MAX_INPUT_LENGTH);
   int vnumber = 0, room;
   char_data *mob;
   obj_data *object;

   two_arguments(argument, arg1, arg2);

   if (!*arg1 || !*arg2 || !is_number(arg2) || ((vnumber = atoi(arg2)) < 0))
      {
      obj_log(obj, "oload: bad syntax");
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }

   if ((room = obj_room(obj)) == NOWHERE)
      {
      obj_log(obj, "oload: object in NOWHERE trying to load");
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }

   if (is_abbrev(arg1, "mob"))
      {
      if ((mob = read_mobile(vnumber, VIRTUAL)) == NULL)
         {
         obj_log(obj, "oload: bad mob vnum");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      GET_MOB_VAL(mob,0)=GET_ROOM_VNUM(room);
      mob->orig_room=room;
      char_to_room(mob, room);
      load_mtrigger(mob);
      }

   else if (is_abbrev(arg1, "obj"))
      {
      if ((object = read_object(vnumber, VIRTUAL)) == NULL)
         {
         obj_log(obj, "oload: bad object vnum");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }

      obj_to_room(object, room);
      load_otrigger(object);
      }

   else
      obj_log(obj, "oload: bad type");
   release_buffer(arg2);
   release_buffer(arg1);
   }

OCMD(do_odamage) {
   char *name=get_buffer(MAX_INPUT_LENGTH);
   char *amount=get_buffer(MAX_INPUT_LENGTH);
   int dam = 0;
   char_data *ch;

   two_arguments(argument, name, amount);

   if (!*name || !*amount || !isdigit((int)*amount))
      {
      obj_log(obj, "odamage: bad syntax");
      release_buffer(amount);
      release_buffer(name);
      return;
      }

   dam = atoi(amount);

   if ((ch = get_char_by_obj(obj, name)))
      {
	if (IS_NPC(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
	  obj_log(obj, "odamage: peaceful room");
	  send_to_char(ch,"This room just has such a peaceful, easy feeling...\r\n");
	  release_buffer(amount);
	  release_buffer(name);	 
	  return;
	}

      if (!IS_NPC(ch) && PRF_FLAGGED(ch,PRF_NOHASSLE))
         {
         send_to_char(ch,"Being the cool immortal you are, you sidestep a trap, obviously placed to kill you.");
         release_buffer(amount);
         release_buffer(name);
         return;
         }
      GET_HIT(ch) -= dam;
      GET_HIT(ch) = MAX(GET_HIT(ch),-10);
      update_pos(ch);
      switch (GET_POS(ch))
         {
      case POS_MORTALLYW:
         act("$n is mortally wounded, and will die soon, if not aided.", TRUE, ch, 0, 0, TO_ROOM);
         send_to_char(ch,"You are mortally wounded, and will die soon, if not aided.\r\n");
         break;
      case POS_INCAP:
         act("$n is incapacitated and will slowly die, if not aided.", TRUE, ch, 0, 0, TO_ROOM);
         send_to_char(ch,"You are incapacitated and will slowly die, if not aided.\r\n");
         break;
      case POS_STUNNED:
         act("$n is stunned, but will probably regain consciousness again.", TRUE, ch, 0, 0, TO_ROOM);
         send_to_char(ch,"You're stunned, but will probably regain consciousness again.\r\n");
         break;
      case POS_DEAD:
         act("$n is dead!  R.I.P.", FALSE, ch, 0, 0, TO_ROOM);
         send_to_char(ch,"You are dead!  Sorry...\r\n");
         break;

      default:   /* >= POSITION SLEEPING */
         if (dam > (GET_MAX_HIT(ch) >> 2))
            act("That really did HURT!", FALSE, ch, 0, 0, TO_CHAR);
         if (GET_HIT(ch) < (GET_MAX_HIT(ch) >> 2))
            {
            send_to_char(ch, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
                         CCRED(ch, C_SPR), CCNRM(ch, C_SPR));
            }
         }
      if (GET_POS(ch) == POS_DEAD)
         {
         if (!IS_NPC(ch))
            {
            mudlogf(BRF, LVL_IMMORT, TRUE,"RIP: %s killed by a trap at %s",
                    GET_NAME(ch), world[ch->in_room].name);
            }
         die(ch, ch);
         }
      }
   else
      obj_log(obj, "odamage: target not found");
   release_buffer(amount);
   release_buffer(name);
   }


OCMD(do_odoor)
   {
   char *target=get_buffer(MAX_INPUT_LENGTH);
   char *direction=get_buffer(MAX_INPUT_LENGTH);
   char *field=get_buffer(MAX_INPUT_LENGTH);
   char *value;
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


   argument = two_arguments(argument, target, direction);
   value = one_argument(argument, field);
   skip_spaces(&value);

   if (!*target || !*direction || !*field)
      {
      obj_log(obj, "odoor called with too few args");
      release_buffer(field);
      release_buffer(direction);
      release_buffer(target);
      return;
      }

   if ((rm = get_room(target)) == NULL)
      {
      obj_log(obj, "odoor: invalid target");
      release_buffer(field);
      release_buffer(direction);
      release_buffer(target);
      return;
      }

   if ((dir = search_block(direction, dirs, FALSE)) == -1)
      {
      obj_log(obj, "odoor: invalid direction");
      release_buffer(field);
      release_buffer(direction);
      release_buffer(target);
      return;
      }

   if ((fd = search_block(field, door_field, FALSE)) == -1)
      {
      obj_log(obj, "odoor: invalid field");
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
            obj_log(obj, "odoor: invalid door target");
         break;
         }
      }
   release_buffer(field);
   release_buffer(direction);
   release_buffer(target);
   }

OCMD(do_osetval)
   {
   char *arg1=get_buffer(MAX_INPUT_LENGTH);
   char *arg2=get_buffer(MAX_INPUT_LENGTH);
   int position, new_value;

   two_arguments(argument, arg1, arg2);
   if (!arg1 || !*arg1 || !arg2 || !*arg2 ||
           !is_number(arg1) || !is_number(arg2))
      {
      obj_log(obj, "osetval: bad syntax");
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }

   position = atoi(arg1);
   new_value = atoi(arg2);
   if (position>=0 && position<NUM_OBJ_VAL_POSITIONS)
      GET_OBJ_VAL(obj, position) = new_value;
   else
      obj_log(obj, "osetval: position out of bounds!");
   release_buffer(arg2);
   release_buffer(arg1);
   }



const struct obj_command_info obj_cmd_info[] = {
            { "RESERVED", 0, 0 }
         ,/* this must be first -- for specprocs */

         { "oecho"      , do_oecho    , 0 },
         { "oechoaround", do_osend    , SCMD_OECHOAROUND },
         { "oexp"       , do_oexp     , 0 },
         { "oforce"     , do_oforce   , 0 },
         { "oload"   , do_dgoload , 0 },
         { "opurge"     , do_opurge   , 0 },
         { "osend"      , do_osend    , SCMD_OSEND },
         { "osetval"    , do_osetval  , 0 },
         { "oteleport"  , do_oteleport, 0 },
         { "odamage"    , do_odamage  , 0 },
         { "otimer"     , do_otimer   , 0 },
         { "otransform" , do_otransform, 0 },
         { "odoor"      , do_odoor    , 0 },
         { "\n", 0, 0 } /* this must be last */
         };



/*
 *  This is the command interpreter used by objects, called by script_driver.
 */
void obj_command_interpreter(obj_data *obj, char *argument)
   {
   int cmd, length;
   char *line, *arg;

   skip_spaces(&argument);

   /* just drop to next line for hitting CR */
   if (!*argument)
      return;
   arg=get_buffer(MAX_INPUT_LENGTH);
   line = any_one_arg(argument, arg);


   /* find the command */
   for (length = strlen(arg),cmd = 0;
           *obj_cmd_info[cmd].command != '\n'; cmd++)
      if (!strncmp(obj_cmd_info[cmd].command, arg, length))
         break;

   if (*obj_cmd_info[cmd].command == '\n')
      {
      char *buf=get_buffer(256);
      sprintf(buf, "Unknown object cmd: '%s'", argument);
      obj_log(obj, buf);
      release_buffer(buf);
      }
   else
      ((*obj_cmd_info[cmd].command_pointer)
              (obj, line, cmd, obj_cmd_info[cmd].subcmd));
   release_buffer(arg);
   }



