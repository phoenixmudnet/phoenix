/**************************************************************************
*  File: scripts.c                                                        *
*  Usage: contains general functions for using scripts.                   *
*                                                                         *
*                                                                         *
*  $Author: lucas $
*  $Date: 2006/09/15 02:02:06 $
*  $Revision: 1.1.1.1 $
**************************************************************************/

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"


#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "clan.h"
#include "dg_event.h"
#include "db.h"
#include "screen.h"
#include "buffer.h"
#include "constants.h"
#include "queue.h"
#include "spells.h"

#define PULSES_PER_MUD_HOUR     (SECS_PER_MUD_HOUR*PASSES_PER_SEC)


/* External Vars From db.c */
extern int top_of_trigt;
extern struct index_data **trig_index;

/* External Vars From dg_triggers.c */
extern char *trig_types[], *otrig_types[], *wtrig_types[];

/* Other External Vars */
extern struct command_info cmd_info[];
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern char *npc_class_types[];
extern char *npc_race_types[];
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern long dg_global_pulse;
extern struct spell_info_type spell_info[];
extern struct zone_data *zone_table;

extern const float race_exp_multipliers[];
extern const float class_exp_multipliers[];
extern const int exp_table[];



/* External Functions */
struct obj_data *get_object_in_equip(struct char_data * ch, char *name);
obj_data *find_obj_dg(long n);
room_rnum find_target_room(char_data * ch, char *rawroomstr);
room_rnum find_target_room(struct char_data * ch, char *rawroomstr);
trig_data *read_trigger(int nr);
int obj_room(obj_data *obj);
int is_empty(int zone_nr);
int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);
int eval_lhs_op_rhs(char *expr, char *result, void *go, struct script_data *sc,
                    trig_data *trig, int type);
void free_varlist(struct trig_var_data *vd);
void extract_trigger(struct trig_data *trig);
void obj_command_interpreter(obj_data *obj, char *argument);
void wld_command_interpreter(struct room_data *room, char *argument);
char *queue_time_to_string(char *mybuf, time_t qtime);
char *skill_percent(struct char_data *ch, char *skill);

/* function protos from this file */
int valid_script(struct trig_data *trig, struct char_data *ch);
int script_driver(void *go, trig_data *trig, int type, int mode);
int trgvar_in_room(int vnum);
void extract_value(struct script_data *sc, trig_data *trig, char *cmd);
void script_log(char *msg);
struct cmdlist_element *find_done(struct cmdlist_element *cl);
struct cmdlist_element *find_case(struct trig_data *trig,
                                           struct cmdlist_element *cl,
                                           void *go, struct script_data *sc,
                                           int type, char *cond);

/* MANWE_EMBEDDED: Allows dereferencing of script variables */
void                    parse_embedded(void *go, struct script_data *sc,
                                       struct trig_data *trig, int type,
                                       char *str, char *buf);
void                    process_unset(void *go, struct script_data *sc,
                                      struct trig_data *trig,
                                      int type, char *str);

extern char *dg_get_ch_prf(struct char_data *ch, char *prf);
extern char *dg_get_ch_aff(struct char_data *ch, char *aff);
extern char *dg_get_ch_res(struct char_data *ch, char *res);
extern char *dg_get_ch_sus(struct char_data *ch, char *sus);
extern char *dg_get_ch_imm(struct char_data *ch, char *imm);
extern char *dg_get_room_flags(struct room_data *room, char *flag);

/* local structures */
struct wait_event_data {
   trig_data *trigger;
   void *go;
   int type;
   };


struct trig_data *trigger_list = NULL;  /* all attached triggers */

/* Return pointer to first occurrence in string ct in */
/* cs, or NULL if not present.  Case insensitive */
char *str_str(char *cs, char *ct)
   {
   char *s, *t;

   if (!cs || !ct)
      return NULL;

   while (*cs)
      {
      t = ct;

      while (*cs && (LOWER(*cs) != LOWER(*t)))
         cs++;

      s = cs;

      while (*t && *cs && (LOWER(*cs) == LOWER(*t)))
         {
         t++;
         cs++;
         }

      if (!*t)
         return s;
      }

   return NULL;
   }


int trgvar_in_room(int vnum)
   {
   int i = 0;
   char_data *ch;
   int rnum;

   if (NOWHERE == (rnum=real_room(vnum)))
      {
      script_log("people.vnum: world[vnum] does not exist");
      return (-1);
      }

   for (ch = world[rnum].people; ch !=NULL; ch = ch->next_in_room)
      i++;

   return i;
   }

obj_data *get_obj_in_list(char *name, obj_data *list)
   {
   obj_data *i;
   long id;

   if (*name == UID_CHAR)
      {
      id = atoi(name + 1);

      for (i = list; i; i = i->next_content)
         if (id == GET_ID(i))
            return i;
      }
   else
      {
      for (i = list; i; i = i->next_content)
         if (isname(name, i->name))
            return i;
      }

   return NULL;
   }

obj_data *get_object_in_equip(char_data * ch, char *name)
   {
   int j, n = 0, vnumber;
   obj_data *obj;
   char *tmpname=get_buffer(MAX_INPUT_LENGTH);
   char *tmp = tmpname;
   long id;

   if (*name == UID_CHAR)
      {
      id = atoi(name + 1);

      for (j = 0; j < NUM_WEARS; j++)
         if ((obj = GET_EQ(ch, j)))
            if (id == GET_ID(obj))
               {
               release_buffer(tmpname);
               return (obj);
               }
      }
   else
      {
      strcpy(tmp, name);
      if (!(vnumber = get_number(&tmp)))
         {
         release_buffer(tmpname);
         return NULL;
         }

      for (j = 0; (j < NUM_WEARS) && (n <= vnumber); j++)
         if ((obj = GET_EQ(ch, j)))
            if (isname(tmp, obj->name))
               if (++n == vnumber)
                  {
                  release_buffer(tmpname);
                  return (obj);
                  }
      }
   release_buffer(tmpname);
   return NULL;
   }

/************************************************************
 * search by number routines
 ************************************************************/

/* return char with UID n */
struct char_data *find_char(long n)
   {
   struct char_data *ch;

   for (ch = character_list; ch; ch=ch->next)
      {
      if (GET_ID(ch)==n)
         return (ch);
      }

   return NULL;
   }


/* return object with UID n */
obj_data *find_obj_dg(long n)
   {
   obj_data *i;

   for (i = object_list; i; i = i->next)
      if (n == GET_ID(i))
         return i;

   return NULL;
   }

/* return room with UID n */
room_data *find_room(long n)
   {
   n -= ROOM_ID_BASE;

   if ((n >= 0) && (n <= top_of_world))
      return &world[n];

   return NULL;
   }



/************************************************************
 * generic searches based only on name
 ************************************************************/

/* search the entire world for a char, and return a pointer */
char_data *get_char_dg(char *name)
   {
   char_data *i;

   if (*name == UID_CHAR)
      {
      i = find_char(atoi(name + 1));

      if (i && (IS_NPC(i)||!GET_INVIS_LEV(i)))
         return i;
      }
   else
      {
      for (i = character_list; i; i = i->next)
         if (isname(name, i->player.name) && (IS_NPC(i)||!GET_INVIS_LEV(i)))
            return i;
      }

   return NULL;
   }


/* returns the object in the world with name name, or NULL if not found */
obj_data *get_obj(char *name)
   {
   obj_data *obj;
   long id;

   if (*name == UID_CHAR)
      {
      id = atoi(name + 1);

      for (obj = object_list; obj; obj = obj->next)
         if (id == GET_ID(obj))
            return obj;
      }
   else
      {
      for (obj = object_list; obj; obj = obj->next)
         if (isname(name, obj->name))
            return obj;
      }

   return NULL;
   }


/* finds room by with name.  returns NULL if not found */
room_data *get_room(char *name)
   {
   int nr;

   /* MANWE: Need to make sure it can resolve to a number */
   if (*name == '\0')
      return NULL;

   if (*name == UID_CHAR)
      return find_room(atoi(name + 1));
   else if ((nr = real_room(atoi(name))) == NOWHERE)
      return NULL;
   else
      return &world[nr];
   }


/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching with the person owing the object
 */
char_data *get_char_by_obj(obj_data *obj, char *name)
   {
   char_data *ch;

   if (*name == UID_CHAR)
      {
      ch = find_char(atoi(name + 1));
      if(!ch)
      {
         return NULL;
      }
      
      if (!IS_NPC(ch)) /* to avoid "mob using player specials" error */
         {
         if (!GET_INVIS_LEV(ch))
            {
            return ch;
            }
         }
      else
         {
         return ch;
         }
      }
   else
      {
      if (obj->carried_by && isname(name, obj->carried_by->player.name) &&
              (IS_NPC(obj->carried_by)||!GET_INVIS_LEV(obj->carried_by)))
         {
         return obj->carried_by;
         }

      if (obj->worn_by && isname(name, obj->worn_by->player.name) &&
              (IS_NPC(obj->worn_by)||!GET_INVIS_LEV(obj->worn_by)))
         {
         return obj->worn_by;
         }

      for (ch = character_list; ch; ch = ch->next)
         if (isname(name, ch->player.name) && (IS_NPC(ch)||!GET_INVIS_LEV(ch)))
            {
            return ch;
            }
      }

   return NULL;
   }


/*
 * returns a pointer to the first character in world by name name,
 * or NULL if none found.  Starts searching in room room first
 */
char_data *get_char_by_room(room_data *room, char *name)
   {
   char_data *ch;

   if (*name == UID_CHAR)
      {
      ch = find_char(atoi(name + 1));

      if (ch && (IS_NPC(ch)||!GET_INVIS_LEV(ch)))
         return ch;
      }
   else
      {
      for (ch = room->people; ch; ch = ch->next_in_room)
         if (isname(name, ch->player.name) && (IS_NPC(ch)||!GET_INVIS_LEV(ch)))
            return ch;

      for (ch = character_list; ch; ch = ch->next)
         if (isname(name, ch->player.name) && (IS_NPC(ch)||!GET_INVIS_LEV(ch)))
            return ch;
      }

   return NULL;
   }


/*
 * returns the object in the world with name name, or NULL if not found
 * search based on obj
 */
obj_data *get_obj_by_obj(obj_data *obj, char *name)
   {
   obj_data *i = NULL;
   int rm;
   long id;

   if (!str_cmp(name, "self") || !str_cmp(name, "me"))
      return obj;

   if (obj->contains && (i = get_obj_in_list(name, obj->contains)))
      return i;

   if (obj->in_obj)
      {
      if (*name == UID_CHAR)
         {
         id = atoi(name + 1);

         if (id == GET_ID(obj->in_obj))
            return obj->in_obj;
         }
      else if (isname(name, obj->in_obj->name))
         return obj->in_obj;
      }
   else if (obj->worn_by && (i = get_object_in_equip(obj->worn_by, name)))
      return i;
   else if (obj->carried_by &&
            (i = get_obj_in_list(name, obj->carried_by->carrying)))
      return i;
   else if (((rm = obj_room(obj)) != NOWHERE) &&
            (i = get_obj_in_list(name, world[rm].contents)))
      return i;

   if (*name == UID_CHAR)
      {
      id = atoi(name + 1);

      for (i = object_list; i; i = i->next)
         if (id == GET_ID(i))
            break;
      }
   else
      {
      for (i = object_list; i; i = i->next)
         if (isname(name, i->name))
            break;
      }
   return i;
   }


/* returns obj with name */
obj_data *get_obj_by_room(room_data *room, char *name)
   {
   obj_data *obj;
   long id;

   if (*name == UID_CHAR)
      {
      id = atoi(name + 1);

      for (obj = room->contents; obj; obj = obj->next_content)
         if (id == GET_ID(obj))
            return obj;

      for (obj = object_list; obj; obj = obj->next)
         if (id == GET_ID(obj))
            return obj;
      }
   else
      {
      for (obj = room->contents; obj; obj = obj->next_content)
         if (isname(name, obj->name))
            return obj;

      for (obj = object_list; obj; obj = obj->next)
         if (isname(name, obj->name))
            return obj;
      }
   return NULL;
   }



/* checks every PLUSE_SCRIPT for random triggers */
void script_trigger_check(void)
   {
   char_data *ch;
   obj_data *obj;
   struct room_data *room=NULL;
   int nr;
   struct script_data *sc;

   for (ch = character_list; ch; ch = ch->next)
      {
      if (SCRIPT(ch))
         {
         sc = SCRIPT(ch);

         if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
                 (!is_empty(world[IN_ROOM(ch)].zone) ||
                  IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
            random_mtrigger(ch);
         }
      }

   for (obj = object_list; obj; obj = obj->next)
      {
      if (SCRIPT(obj))
         {
         sc = SCRIPT(obj);

         if (IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM))
            random_otrigger(obj);
         }
      }

   for (nr = 0; nr <= top_of_world; nr++)
      {
      if (SCRIPT(&world[nr]))
         {
         room = &world[nr];
         sc = SCRIPT(room);

         if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
                 (!is_empty(room->zone) ||
                  IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
            random_wtrigger(room);
         }
      }
   }


EVENT(trig_wait_event)
   {
   struct wait_event_data *wait_event_obj = (struct wait_event_data *)info;
   trig_data *trig;
   void *go;
   int type;

   trig = wait_event_obj->trigger;
   go = wait_event_obj->go;
   type = wait_event_obj->type;

   free(wait_event_obj);
   GET_TRIG_WAIT(trig) = NULL;

   script_driver(go, trig, type, TRIG_RESTART);
   }


void do_stat_trigger(struct char_data *ch, trig_data *trig)
   {
   struct cmdlist_element *cmd_list;
   char *sb;
   char *buf;
   char *buf1;
   char_data *tch;
   obj_data *obj;
   int nr;
   trig_data *t;

   if (!trig)
      {
      log("SYSERR: NULL trigger passed to do_stat_trigger.");
      return;
      }

   if (GET_LEVEL(ch) < TSTAT_LEVEL && !is_olc_set(ch, GET_TRIG_VNUM(trig)/100)) {
     send_to_char(ch, "You do not have permissions to tstat that trigger.\r\n");
     return;
   }   

   sb=get_buffer(32750);
   buf=get_buffer(256);
   buf1=get_buffer(8192);

   sprintf(sb, "Name: '%s%s%s',  VNum: [%s%5ld%s], RNum: [%5ld]\r\n",
           CCYEL(ch, C_NRM), GET_TRIG_NAME(trig), CCNRM(ch, C_NRM),
           CCGRN(ch, C_NRM), GET_TRIG_VNUM(trig), CCNRM(ch, C_NRM),
           GET_TRIG_RNUM(trig));

   if (trig->attach_type==OBJ_TRIGGER)
      {
      sprintf(sb+strlen(sb),"Trigger Intended Assignment: Objects\r\n");
      sprintbit(GET_TRIG_TYPE(trig), otrig_types, buf);
      for(obj=object_list;obj;obj=obj->next)
         if(SCRIPT(obj))
            for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
               if(GET_TRIG_VNUM(t)==GET_TRIG_VNUM(trig))
                  sprintf(buf1+strlen(buf1)," %s [%ld]\r\n",GET_OBJ_NAME(obj),
                          GET_OBJ_VNUM(obj));
      }
   else if (trig->attach_type==WLD_TRIGGER)
      {
      sprintf(sb+strlen(sb),"Trigger Intended Assignment: Rooms\r\n");
      sprintbit(GET_TRIG_TYPE(trig), wtrig_types, buf);
      for (nr = 0; nr <= top_of_world; nr++)
         if (SCRIPT(&world[nr]))
            for (t = TRIGGERS(SCRIPT(&world[nr])); t; t = t->next)
               if(GET_TRIG_VNUM(t)==GET_TRIG_VNUM(trig))
                  sprintf(buf1+strlen(buf1)," %s [%ld]\r\n",world[nr].name,
                          GET_ROOM_VNUM(nr));
      }
   else
      {
      sprintf(sb+strlen(sb),"Trigger Intended Assignment: Mobiles\r\n");
      sprintbit(GET_TRIG_TYPE(trig), trig_types, buf);
      for (tch = character_list; tch; tch = tch->next)
         if (SCRIPT(tch))
            for (t = TRIGGERS(SCRIPT(tch)); t; t = t->next)
               if(GET_TRIG_VNUM(t)==GET_TRIG_VNUM(trig))
                  sprintf(buf1+strlen(buf1)," %s [%ld]\r\n",GET_NAME(tch),
                          GET_MOB_VNUM(tch));

      }

   sprintf(sb+strlen(sb),"Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n%s",
           buf, GET_TRIG_NARG(trig),
           ((GET_TRIG_ARG(trig)&&*GET_TRIG_ARG(trig))?GET_TRIG_ARG(trig):"None"),buf1);
   release_buffer(buf);
   release_buffer(buf1);

   strcat(sb,"Commands:\r\n   ");

   cmd_list = trig->cmdlist;
   while (cmd_list)
      {
      if (cmd_list->cmd)
         {
         strcat(sb,cmd_list->cmd);
         strcat(sb,"\r\n   ");
         }

      cmd_list = cmd_list->next;
      }

   page_string(ch->desc, sb, TRUE,"");
   release_buffer(sb);
   }


/* find the name of what the uid points to */
void find_uid_name(char *uid, char *name)
   {
   char_data *ch;
   obj_data *obj;

   if ((ch = get_char_dg(uid)))
      strcpy(name, ch->player.name);
   else if ((obj = get_obj(uid)))
      strcpy(name, obj->name);
   else
      sprintf(name, "uid = %s, (not found)", uid + 1);
   }


/* general function to display stats on script sc */
void script_stat (char_data *ch, struct script_data *sc)
   {
   struct trig_var_data *tv;
   trig_data *t;
   char *name=get_buffer(MAX_INPUT_LENGTH);
   char *namebuf=get_buffer(512);
   char *buf1=get_buffer(512);

   send_to_char(ch, "Global Variables: %s\r\n", sc->global_vars ? "" : "None");
   send_to_char(ch, "Global context: %ld\r\n", sc->context);

   for (tv = sc->global_vars; tv; tv = tv->next)
      {
      sprintf(namebuf,"%s:%ld", tv->name, tv->context);
      if (*(tv->value) == UID_CHAR)
         {
         find_uid_name(tv->value, name);
         send_to_char(ch, "    %15s:  %s\r\n", tv->context?namebuf:tv->name, name);
         }
      else
         send_to_char(ch, "    %15s:  %s\r\n", tv->context?namebuf:tv->name,
                      tv->value);
      }

   for (t = TRIGGERS(sc); t; t = t->next)
      {
      send_to_char(ch, "\r\n  Trigger: %s%s%s, VNum: [%s%5ld%s], RNum: [%5ld]\r\n",
                   CCYEL(ch, C_NRM), GET_TRIG_NAME(t), CCNRM(ch, C_NRM),
                   CCGRN(ch, C_NRM), GET_TRIG_VNUM(t), CCNRM(ch, C_NRM),
                   GET_TRIG_RNUM(t));

      if (t->attach_type==OBJ_TRIGGER)
         {
         send_to_char(ch,"  Trigger Intended Assignment: Objects\r\n");
         sprintbit(GET_TRIG_TYPE(t), otrig_types, buf1);
         }
      else if (t->attach_type==WLD_TRIGGER)
         {
         send_to_char(ch,"  Trigger Intended Assignment: Rooms\r\n");
         sprintbit(GET_TRIG_TYPE(t), wtrig_types, buf1);
         }
      else
         {
         send_to_char(ch,"  Trigger Intended Assignment: Mobiles\r\n");
         sprintbit(GET_TRIG_TYPE(t), trig_types, buf1);
         }

      send_to_char(ch, "  Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
                   buf1, GET_TRIG_NARG(t),
                   ((GET_TRIG_ARG(t) && *GET_TRIG_ARG(t)) ? GET_TRIG_ARG(t) :
                    "None"));

      if (GET_TRIG_WAIT(t))
         {
         struct queue_event *q_ev;
         q_ev=t->wait_event;
         queue_time_to_string(buf1,q_ev->time);
         send_to_char(ch, "    Waiting: %s, Current line: %s\r\n",
                      buf1,t->curr_state?t->curr_state->cmd:"Finished");

         send_to_char(ch, "  Variables: %s\r\n",
                      GET_TRIG_VARS(t) ? "" : "None");

         for (tv = GET_TRIG_VARS(t); tv; tv = tv->next)
            {
            if (*(tv->value) == UID_CHAR)
               {
               find_uid_name(tv->value, name);
               send_to_char(ch, "    %15s:  %s\r\n", tv->name, name);
               }
            else
               send_to_char(ch, "    %15s:  %s\r\n", tv->name, tv->value);
            }
         }
      }
   release_buffer(buf1);
   release_buffer(name);
   release_buffer(namebuf);
   }


void do_sstat_room(struct char_data * ch)
   {
   struct room_data *rm = &world[ch->in_room];

   send_to_char(ch,"Script information:\r\n");
   if (!SCRIPT(rm))
      {
      send_to_char(ch,"  None.\r\n");
      return;
      }

   script_stat(ch, SCRIPT(rm));
   }


void do_sstat_object(char_data *ch, obj_data *j)
   {
   send_to_char(ch,"Script information:\r\n");
   if (!SCRIPT(j))
      {
      send_to_char(ch,"  None.\r\n");
      return;
      }

   script_stat(ch, SCRIPT(j));
   }


void do_sstat_character(char_data *ch, char_data *k)
   {
   send_to_char(ch,"Script information:\r\n");
   if (!SCRIPT(k))
      {
      send_to_char(ch,"  None.\r\n");
      return;
      }

   script_stat(ch, SCRIPT(k));
   }


/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 */
void add_trigger(struct script_data *sc, trig_data *t, int loc)
   {
   trig_data *i;
   int n;

   for (n = loc, i = TRIGGERS(sc); i && i->next && (n != 0); n--, i = i->next)
      ;

   if (!loc)
      {
      t->next = TRIGGERS(sc);
      TRIGGERS(sc) = t;
      }
   else if (!i)
      TRIGGERS(sc) = t;
   else
      {
      t->next = i->next;
      i->next = t;
      }

   SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(t);

   t->next_in_world = trigger_list;
   trigger_list = t;
   }


ACMD(do_attach)
   {
   char_data *victim;
   obj_data *object;
   trig_data *trig;
   char *targ_name=get_buffer(MAX_INPUT_LENGTH);
   char *trig_name=get_buffer(MAX_INPUT_LENGTH);
   char *loc_name=get_buffer(MAX_INPUT_LENGTH);
   int loc, room, tn, rn;
   char *arg=get_buffer(512);

   argument = two_arguments(argument, arg, trig_name);
   two_arguments(argument, targ_name, loc_name);

   if (!*arg || !*targ_name || !*trig_name)
      {
      send_to_char(ch,"Usage: attach { mtr | otr | wtr } { trigger } { name } [ location ]\r\n");
      release_buffer(arg);
      release_buffer(loc_name);
      release_buffer(trig_name);
      release_buffer(targ_name);
      return;
      }

   tn = atoi(trig_name);
   loc = (*loc_name) ? atoi(loc_name) : -1;

   if (is_abbrev(arg, "mtr"))
      {
      if ((victim = get_char_vis(ch, targ_name,FIND_CHAR_WORLD)))
         {
         if (IS_NPC(victim))
            {

            /* have a valid mob, now get trigger */
            rn = real_trigger(tn);
            if ((rn >= 0) && (trig = read_trigger(rn)))
               {

               if (!SCRIPT(victim))
                  CREATE(SCRIPT(victim), struct script_data, 1);
               add_trigger(SCRIPT(victim), trig, loc);

               send_to_char(ch, "Trigger %d (%s) attached to %s.\r\n",
                            tn, GET_TRIG_NAME(trig), GET_SHORT(victim));
               }
            else
               send_to_char(ch,"That trigger does not exist.\r\n");
            }
         else
            send_to_char(ch,"Players can't have scripts.\r\n");
         }
      else
         send_to_char(ch,"That mob does not exist.\r\n");
      }

   else if (is_abbrev(arg, "otr"))
      {
      if ((object = get_obj_vis(ch, targ_name)))
         {

         /* have a valid obj, now get trigger */
         rn = real_trigger(tn);
         if ((rn >= 0) && (trig = read_trigger(rn)))
            {

            if (!SCRIPT(object))
               CREATE(SCRIPT(object), struct script_data, 1);
            add_trigger(SCRIPT(object), trig, loc);

            send_to_char(ch, "Trigger %d (%s) attached to %s.\r\n",
                         tn, GET_TRIG_NAME(trig),
                         (object->short_description ?
                          object->short_description : object->name));
            }
         else
            send_to_char(ch,"That trigger does not exist.\r\n");
         }
      else
         send_to_char(ch,"That object does not exist.\r\n");
      }

   else if (is_abbrev(arg, "wtr"))
      {
      if (isdigit((int)*targ_name) && !strchr(targ_name, '.'))
         {
         if ((room = find_target_room(ch, targ_name)) != NOWHERE)
            {

            /* have a valid room, now get trigger */
            rn = real_trigger(tn);
            if ((rn >= 0) && (trig = read_trigger(rn)))
               {

               if (!(world[room].script))
                  CREATE(world[room].script, struct script_data, 1);
               add_trigger(world[room].script, trig, loc);

               send_to_char(ch, "Trigger %d (%s) attached to room %ld.\r\n",
                            tn, GET_TRIG_NAME(trig), world[room].number);
               }
            else
               send_to_char(ch,"That trigger does not exist.\r\n");
            }
         }
      else
         send_to_char(ch,"You need to supply a room number.\r\n");
      }

   else
      send_to_char(ch,"Please specify 'mtr', otr', or 'wtr'.\r\n");
   release_buffer(arg);
   release_buffer(loc_name);
   release_buffer(trig_name);
   release_buffer(targ_name);
   }


/* adds a variable with given name and value to trigger */
void add_var(struct trig_var_data **var_list, char *name, char *value,long id)
   {
   struct trig_var_data *vd;

   for (vd = *var_list; vd ; vd = vd->next)
      {
      if(!str_cmp(vd->name,name))
         {
         if(vd->context == id)
            break;
         }
      }
   if(!vd)
      {
      for (vd = *var_list; vd ; vd = vd->next)
         {
         if(vd->context && id && id!=vd->context)
            continue;
         if(!str_cmp(vd->name,name))
            {
            break;
            }
         }
      }
   if (vd &&(!vd->context||vd->context==id))
      {
      free(vd->value);
      CREATE(vd->value, char, strlen(value) + 1);
      }
   else
      {
      CREATE(vd, struct trig_var_data, 1);

      CREATE(vd->name, char, strlen(name) + 1);
      strcpy(vd->name, name);

      CREATE(vd->value, char, strlen(value) + 1);

      vd->next = *var_list;
      vd->context=id;
      *var_list = vd;
      }
   strcpy(vd->value, value);
   }


/*
 *  removes the trigger specified by name, and the script of o if
 *  it removes the last trigger.  name can either be a number, or
 *  a 'silly' name for the trigger, including things like 2.beggar-death.
 *  returns 0 if did not find the trigger, otherwise 1.  If it matters,
 *  you might need to check to see if all the triggers were removed after
 *  this function returns, in order to remove the script.
 */
int remove_trigger(struct script_data *sc, char *name)
   {
   trig_data *i, *j;
   int num = 0, string = FALSE, n;
   char *cname;


   if (!sc)
      return 0;

   if ((cname = strstr(name,".")) || (!isdigit((int)*name)) )
      {
      string = TRUE;
      if (cname)
         {
         *cname = '\0';
         num = atoi(name);
         name = ++cname;
         }
      }
   else
      num = atoi(name);

   for (n = 0, j = NULL, i = TRIGGERS(sc); i; j = i, i = i->next)
      {
      if (string)
         {

         if (isname(name, GET_TRIG_NAME(i)))
            if (++n >= num)
               break;
         }
      /* this isn't clean... */
      /* a numeric value will match if it's position OR vnum */
      /* is found. originally the number was position-only */
      else if (++n >= num)
         break;
      else if (trig_index[i->nr]->vnum == num)
         break;

      }

   if (i)
      {
      if (j)
         {
         j->next = i->next;
         extract_trigger(i);
         }

      /* this was the first trigger */
      else
         {
         TRIGGERS(sc) = i->next;
         extract_trigger(i);
         }

      /* update the script type bitvector */
      SCRIPT_TYPES(sc) = 0;
      for (i = TRIGGERS(sc); i; i = i->next)
         SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(i);

      return 1;
      }
   else
      return 0;
   }

ACMD(do_detach)
   {
   char_data *victim = NULL;
   obj_data *object = NULL;
   struct room_data *room;
   char *arg1=get_buffer(MAX_INPUT_LENGTH);
   char *arg2=get_buffer(MAX_INPUT_LENGTH);
   char *arg3=get_buffer(MAX_INPUT_LENGTH);
   char *trigger = 0;
   int tmp;

   argument = two_arguments(argument, arg1, arg2);
   one_argument(argument, arg3);

   if (!*arg1 || !*arg2)
      {
      send_to_char(ch,"Usage: detach [ mob | object | room] { target } "
                   "{ trigger | 'all' }\r\n");
      release_buffer(arg3);
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }

   if (!str_cmp(arg1, "room"))
      {
      room = &world[IN_ROOM(ch)];
      if (!SCRIPT(room))
         send_to_char(ch,"This room does not have any triggers.\r\n");
      else if (!str_cmp(arg2, "all"))
         {
         extract_script(SCRIPT(room));
         SCRIPT(room) = NULL;
         send_to_char(ch,"All triggers removed from room.\r\n");
         }

      else if (remove_trigger(SCRIPT(room), arg2))
         {
         send_to_char(ch,"Trigger removed.\r\n");
         if (!TRIGGERS(SCRIPT(room)))
            {
            extract_script(SCRIPT(room));
            SCRIPT(room) = NULL;
            }
         }
      else
         send_to_char(ch,"That trigger was not found.\r\n");
      }

   else
      {
      if (is_abbrev(arg1, "mob"))
         {
         if (!(victim = get_char_vis(ch, arg2,FIND_CHAR_WORLD)))
            send_to_char(ch,"No such mobile around.\r\n");
         else if (!arg3 || !*arg3)
            send_to_char(ch,"You must specify a trigger to remove.\r\n");
         else
            trigger = arg3;
         }

      else if (is_abbrev(arg1, "object"))
         {
         if (!(object = get_obj_vis(ch, arg2)))
            send_to_char(ch,"No such object around.\r\n");
         else if (!arg3 || !*arg3)
            send_to_char(ch,"You must specify a trigger to remove.\r\n");
         else
            trigger = arg3;
         }
      else
         {
         if ((object = get_object_in_equip_vis(ch, arg1, ch->equipment, &tmp)))
            ;
         else if ((object = get_obj_in_list_vis(ch, arg1, ch->carrying)))
            ;
         else if ((victim = get_char_room_vis(ch, arg1)))
            ;
         else if ((object = get_obj_in_list_vis(ch, arg1, world[IN_ROOM(ch)].contents)))
            ;
         else if ((victim = get_char_vis(ch, arg1,FIND_CHAR_WORLD)))
            ;
         else if ((object = get_obj_vis(ch, arg1)))
            ;
         else
            send_to_char(ch,"Nothing around by that name.\r\n");

         trigger = arg2;
         }

      if (victim)
         {
         if (!IS_NPC(victim))
            send_to_char(ch,"Players don't have triggers.\r\n");

         else if (!SCRIPT(victim))
            send_to_char(ch,"That mob doesn't have any triggers.\r\n");
         else if (!str_cmp(arg2, "all"))
            {
            extract_script(SCRIPT(victim));
            SCRIPT(victim) = NULL;
            send_to_char(ch, "All triggers removed from %s.\r\n", GET_SHORT(victim));
            }

         else if (trigger && remove_trigger(SCRIPT(victim), trigger))
            {
            send_to_char(ch,"Trigger removed.\r\n");
            if (!TRIGGERS(SCRIPT(victim)))
               {
               extract_script(SCRIPT(victim));
               SCRIPT(victim) = NULL;
               }
            }
         else
            send_to_char(ch,"That trigger was not found.\r\n");
         }

      else if (object)
         {
         if (!SCRIPT(object))
            send_to_char(ch,"That object doesn't have any triggers.\r\n");

         else if (!str_cmp(arg2, "all"))
            {
            extract_script(SCRIPT(object));
            SCRIPT(object) = NULL;
            send_to_char(ch, "All triggers removed from %s.\r\n",
                         object->short_description ? object->short_description :
                         object->name);
            }

         else if (remove_trigger(SCRIPT(object), trigger))
            {
            send_to_char(ch,"Trigger removed.\r\n");
            if (!TRIGGERS(SCRIPT(object)))
               {
               extract_script(SCRIPT(object));
               SCRIPT(object) = NULL;
               }
            }
         else
            send_to_char(ch,"That trigger was not found.\r\n");
         }
      }
   release_buffer(arg3);
   release_buffer(arg2);
   release_buffer(arg1);
   }


/* frees memory associated with var */
void free_var_el(struct trig_var_data *var)
   {
   free(var->name);
   free(var->value);
   free(var);
   }


/*
 * remove var name from var_list
 * returns 1 if found, else 0
 */
int remove_var(struct trig_var_data **var_list, char *name, long context)
   {
   struct trig_var_data *i, *j;
   for (j = NULL, i = *var_list; i ; j = i, i = i->next)
      {
      if(!str_cmp(name, i->name))
         {
         if((context == -1) || (context == i->context))
            break;
         }
      }

   if (i)
      {
      if (j)
         {
         j->next = i->next;
         free_var_el(i);
         }
      else
         {
         *var_list = i->next;
         free_var_el(i);
         }

      return 1;
      }

   return 0;
   }


/*
 *  Logs any errors caused by scripts to the system log.
 *  Will eventually allow on-line view of script errors.
 */
void script_log(char *msg)
   {
   mudlogf(NRM, LVL_IMMORT, TRUE,"SCRIPT ERR: %s", msg);
   }

int text_processed(char *field, char *subfield,struct trig_var_data *vd,
                   char *str)
   {
   char *p, *p2;

   if (!str_cmp(field, "strlen"))                      /* strlen    */
      {
      /* Check for zero length */
      if (*vd->value)
         sprintf(str, "%d", strlen(vd->value));
      else
         sprintf(str, "%d", 0);
      return TRUE;
      }
   /* MANWE_STRINGS: Add string functions left, mid, and right */
   else if (!str_cmp(field, "left"))
      {
      char *temp = get_buffer(256);
      strcpy(temp, vd->value);
      if (atoi(subfield) < 1)
         strcpy(temp, "STRING_ERROR - string.left(): Size must be > 0");
      else if (atoi(subfield) < strlen(vd->value))
         *(temp + atoi(subfield)) = '\0';
      sprintf(str, "%s", temp);
      release_buffer(temp);
      return TRUE;
      }
   else if (!str_cmp(field, "right"))
      {
      char *temp = get_buffer(256);
      size_t i, len;

      strcpy(temp, vd->value);
      if (atoi(subfield) < 1)
         strcpy(temp, "STRING_ERROR - string.right(): Size must be > 0");
      else if (atoi(subfield) < strlen(temp))
         {
         len = strlen(temp);
         i = len - atoi(subfield);
         strcpy(temp, temp+i);
         }
      sprintf(str, "%s", temp);
      release_buffer(temp);
      return TRUE;
      }
   else if (!str_cmp(field, "mid"))
      {
      char *temp = get_buffer(256);
      size_t start, stop, len;
      char *bp, *ep, *cp;

      /* Get the start and end positions */
      for (cp = subfield ; *cp != ',' ; cp++)
         ;
      *cp = '\0';
      bp = subfield;
      ep = cp + 1;
      start = atoi(bp);
      stop = atoi(ep);

      /* Validate the parameters and return accordingly */
      strcpy(temp, vd->value);
      len = strlen(temp);
      if (start <= 0)
         strcpy(temp, "NULL");
      else if ((start+stop-1) > len)
         strcpy(temp, "NULL");
      else
         {
         *(temp + start + stop - 1) = '\0';
         strcpy(temp, temp + start - 1);
         }
      sprintf(str, "%s", temp);
      release_buffer(temp);
      return TRUE;
      }
   else if (!str_cmp(field, "trim"))                 /* trim      */
      {
      /* trim whitespace from ends */
      p = vd->value;
      p2 = vd->value + strlen(vd->value) - 1;
      while (*p && isspace((int)*p))
         p++;
      while ((p>=p2) && isspace((int)*p2))
         p2--;
      if (p>p2)  /* nothing left */
         {
         *str = '\0';
         return TRUE;
         }
      while (p<=p2)
         *str++ = *p++;
      *str = '\0';
      return TRUE;
      }
   else if (!str_cmp(field, "str_contains"))             /* contains  */
      {
      if (str_str(vd->value, subfield))
         sprintf(str, "1");
      else
         sprintf(str, "0");
      return TRUE;
      }
   else if (!str_cmp(field, "car"))                  /* car       */
      {
      char *car = vd->value;
      while (*car && !isspace((int)*car))
         *str++ = *car++;
      *str = '\0';
      return TRUE;
      }
   else if (!str_cmp(field, "cdr"))                  /* cdr       */
      {
      char *cdr = vd->value;
      while (*cdr && !isspace((int)*cdr))
         cdr++; /* skip 1st field */
      while (*cdr && isspace((int)*cdr))
         cdr++;  /* skip to next */
      while (*cdr)
         *str++ = *cdr++;
      *str = '\0';
      return TRUE;
      }
   else if (!str_cmp(field, "mudcommand"))
      {
      /* find the mud command returned from this text */
      /* NOTE: you may need to replace "cmd_info" with "complete_cmd_info", */
      /* depending on what patches you've got applied.                      */
      /* on older source bases:    extern struct command_info *cmd_info; */
      int length, cmd;
      for (length = strlen(vd->value), cmd = 0;
              *cmd_info[cmd].command != '\n'; cmd++)
         if (!strncmp(cmd_info[cmd].command, vd->value, length))
            break;

      if (*cmd_info[cmd].command == '\n')
         strcpy(str,"");
      else
         strcpy(str, cmd_info[cmd].command);
      return TRUE;
      }

   return FALSE;
   }


/* sets str to be the value of var.field */
void find_replacement(void *go, struct script_data *sc, trig_data *trig,
                      int type,char *var,char *field,char *subfield,char *str)
   {
   struct trig_var_data *vd=NULL;
   char_data *ch, *c = NULL, *rndm;
   obj_data *obj, *o = NULL;
   struct room_data *room, *r = NULL;
   char *name;
   int num, count,addition;
   char *send_cmd[] =       {"msend",       "osend",       "wsend"};
   char *echo_cmd[] =       {"mecho",       "oecho",       "wecho"};
   char *echoaround_cmd[] = {"mechoaround", "oechoaround", "wechoaround"};
   char *door[] =           {"mdoor",       "odoor",       "wdoor"};
   char *force[] =          {"mforce",      "oforce",      "wforce"};
   char *load[] =           {"mload",       "oload",       "wload"};
   char *purge[] =          {"mpurge",      "opurge",      "wpurge"};
   char *teleport[] =       {"mteleport",   "oteleport",   "wteleport"};
   char *dgdamage[] =       {"mdamage",     "odamage",     "wdamage"};

   /* X.global() will have a NULL trig */
   if (trig)
      for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
         if (!str_cmp(vd->name, var))
            break;

   if (!vd)
      for (vd = sc->global_vars; vd; vd = vd->next)
         if (!str_cmp(vd->name, var) &&
                 (vd->context==0 || vd->context==sc->context))
            break;

   if (!*field)
      {
      if (vd)
         strcpy(str, vd->value);
      else
         {
         if (!str_cmp(var, "self"))
            strcpy(str, "self");
         else if (!str_cmp(var, "send"))
            strcpy(str,send_cmd[type]);
         else if (!str_cmp(var, "echo"))
            strcpy(str,echo_cmd[type]);
         else if (!str_cmp(var, "echoaround"))
            strcpy(str,echoaround_cmd[type]);
         else if (!str_cmp(var, "door"))
            strcpy(str,door[type]);
         else if (!str_cmp(var, "force"))
            strcpy(str,force[type]);
         else if (!str_cmp(var, "load"))
            strcpy(str,load[type]);
         else if (!str_cmp(var, "purge"))
            strcpy(str,purge[type]);
         else if (!str_cmp(var, "teleport"))
            strcpy(str,teleport[type]);
         else if (!str_cmp(var, "damage"))
            strcpy(str,dgdamage[type]);
         else
            *str = '\0';
         }

      return;
      }

   else
      {
      if (vd)
         {
         name = vd->value;

         switch (type)
            {
         case MOB_TRIGGER:
            ch = (char_data *) go;

            if ((o = get_object_in_equip(ch, name)))
               ;
            else if ((o = get_obj_in_list(name, ch->carrying)))
               ;
            else if ((c = get_char_room(name, IN_ROOM(ch))))
               ;
            else if ((o = get_obj_in_list(name,world[IN_ROOM(ch)].contents)))
               ;
            else if ((c = get_char_dg(name)))
               ;
            else if ((o = get_obj(name)))
               ;
            else if ((r = get_room(name)))
               {
               }

            break;
         case OBJ_TRIGGER:
            obj = (obj_data *) go;

            if ((c = get_char_by_obj(obj, name)))
               ;
            else if ((o = get_obj_by_obj(obj, name)))
               ;
            else if ((r = get_room(name)))
               {
               }

            break;
         case WLD_TRIGGER:
            room = (struct room_data *) go;

            if ((c = get_char_by_room(room, name)))
               ;
            else if ((o = get_obj_by_room(room, name)))
               ;
            else if ((r = get_room(name)))
               {
               }

            break;
            }
         }

      else
         {
         if (!str_cmp(var, "self"))
            {
            switch (type)
               {
            case MOB_TRIGGER:
               c = (char_data *) go;
               break;
            case OBJ_TRIGGER:
               o = (obj_data *) go;
               break;
            case WLD_TRIGGER:
               r = (struct room_data *) go;
               break;
               }
            }

         else if (!str_cmp(var, "people"))
            {
            sprintf(str,"%d",((num=atoi(field))>0)?trgvar_in_room(num):0);
            return;
            }
         else if (!str_cmp(var, "time"))
            {
            if (!str_cmp(field, "hour"))
               sprintf(str,"%d", time_info.hours);
            else if (!str_cmp(field, "day"))
               sprintf(str,"%d", time_info.day + 1);
            else if (!str_cmp(field, "day_name"))      
               sprintf(str,"%s", weekdays[time_info.day]);
            else if (!str_cmp(field, "month"))
               sprintf(str,"%d", time_info.month + 1);
            else if (!str_cmp(field, "month_name"))
               sprintf(str,"%s", month_name[time_info.month]);
            else if (!str_cmp(field, "year"))
               sprintf(str,"%d", time_info.year);
            else
               *str = '\0';
            return;
            }
         else if (!str_cmp(var, "random"))
            {
            if (!str_cmp(field, "char"))
               {
               rndm = NULL;
               count = 0;

               if (type == MOB_TRIGGER)
                  {
                  ch = (char_data *) go;
                  for (c = world[IN_ROOM(ch)].people; c; c = c->next_in_room)
                     if ((IS_NPC(c)||!PRF_FLAGGED(c, PRF_NOHASSLE)) && (c != ch) &&
                             CAN_SEE(ch, c))
                        {
                        if (!number(0, count))
                           rndm = c;
                        count++;
                        }
                  }

               else if (type == OBJ_TRIGGER)
                  {
                  for (c = world[obj_room((obj_data *) go)].people; c;
                          c = c->next_in_room)
                     if (IS_NPC(c)||
                             (!PRF_FLAGGED(c, PRF_NOHASSLE) && !GET_INVIS_LEV(c)))
                        {
                        if (!number(0, count))
                           rndm = c;
                        count++;
                        }
                  }

               else if (type == WLD_TRIGGER)
                  {
                  for (c = ((struct room_data *) go)->people; c;
                          c = c->next_in_room)
                     if  (IS_NPC(c)||
                             (!PRF_FLAGGED(c, PRF_NOHASSLE) && !GET_INVIS_LEV(c)))
                        {
                        if (!number(0, count))
                           rndm = c;
                        count++;
                        }
                  }

               if (rndm)
                  sprintf(str, "%c%ld", UID_CHAR, GET_ID(rndm));
               else
                  *str = '\0';
               }

            else
               sprintf(str, "%d", ((num = atoi(field)) > 0) ? number(1, num) : 0);

            return;
            }
         }

      if (c)
         {
         if (text_processed(field,subfield, vd, str))
            return;
         else if (!str_cmp(field, "global"))/* get global of something else */
            {
            if (IS_NPC(c) && c->script)
               {
               find_replacement(go, c->script, NULL, MOB_TRIGGER,
                                subfield, NULL, NULL, str);
               }
            }
         else if (!str_cmp(field, "name"))
            {
            if (GET_SHORT(c))
               strcpy(str, GET_SHORT(c));
            else
               strcpy(str, GET_NAME(c));
            }

         else if (!str_cmp(field, "id"))
            sprintf(str, "%ld", GET_ID(c));

         else if (!str_cmp(field, "alias"))
            strcpy(str, GET_PC_NAME(c));

         else if (!str_cmp(field, "level"))
            sprintf(str, "%d", GET_LEVEL(c));

         else if (!str_cmp(field, "remort"))
            sprintf(str, "%d", REMORT_LEVEL(c));

         else if (!str_cmp(field, "align"))
            sprintf(str, "%d", GET_ALIGNMENT(c));

         else if (!str_cmp(field, "gold"))
            {
            if (subfield && *subfield)
               {
               /* MANWE - fixed to use MIN instead of MAX, as well, increased
               ** limit to 100000 vice 10000 also allowed imbedded variables
               ** to be used as a parameter.
               */
               var_subst(go, c->script, trig, MOB_TRIGGER, subfield, str);
               addition = MIN(atoi(str),100000);
               GET_GOLD(c) += addition;
               }
            sprintf(str, "%ld", GET_GOLD(c));
            }

         else if (!str_cmp(field, "exp"))
            {
            if (subfield && *subfield)
               {
               var_subst(go, c->script, trig, MOB_TRIGGER, subfield, str);
               addition = MIN(atoi(str), 50000);
               GET_EXP(c) += addition;
               }
            sprintf(str, "%ld", GET_EXP(c));
            }

	 else if (!str_cmp(field, "exp_tnl")) {
	   if (IS_NPC(c) || GET_LEVEL(c) >= LVL_HERO) {
	     sprintf(str, "0");
	   } else {
	     sprintf(str, "%ld", GET_EXP_FOR_CH(c) - GET_EXP(c));
	   }
	 }

         else if (!str_cmp(field, "sex"))
            strcpy(str, genders[(int)GET_SEX(c)]);

         else if (!str_cmp(field, "position"))
            strcpy(str, position_types[(int)GET_POS(c)]);

         else if (!str_cmp(field, "weight"))
            sprintf(str, "%d",GET_WEIGHT(c));

         else if (!str_cmp(field, "age"))
            sprintf(str, "%d",GET_AGE(c));

         else if (!str_cmp(field, "canbeseen"))
            {
            if ((type == MOB_TRIGGER) && !CAN_SEE(((char_data *)go), c))
               strcpy(str, "0");
            else
               strcpy(str, "1");
            }

         else if (!str_cmp(field, "class"))
            sprinttype(GET_CLASS(c), npc_class_types, str);

         else if (!str_cmp(field, "race"))
            sprinttype(GET_RACE(c), npc_race_types, str);

         else if (!str_cmp(field, "clan"))
            {
            if (GET_CLAN(c))
               strcpy(str, GET_CLAN_NAME(c));
            else
               strcpy(str, "");
            }

         else if (!str_cmp(field, "vnum"))
            sprintf(str, "%ld", GET_MOB_VNUM(c));

         else if (!str_cmp(field, "hitp"))
            sprintf(str, "%d", GET_HIT(c));
         else if (!str_cmp(field, "maxhitp"))
            sprintf(str, "%d", GET_MAX_HIT(c));
         else if (!str_cmp(field, "mana"))
            sprintf(str, "%d", GET_MANA(c));
         else if (!str_cmp(field, "maxmana"))
            sprintf(str, "%d", GET_MAX_MANA(c));
         else if (!str_cmp(field, "move"))
            sprintf(str, "%d", GET_MOVE(c));
         else if (!str_cmp(field, "maxmove"))
            sprintf(str, "%d", GET_MAX_MOVE(c));

         else if (!str_cmp(field, "ac"))
            sprintf(str, "%d", GET_AC(c));

         else if (!str_cmp(field, "str"))
            sprintf(str, "%d", GET_STR(c));
         else if (!str_cmp(field, "stradd"))
            sprintf(str, "%d", GET_ADD(c));
         else if (!str_cmp(field, "int"))
            sprintf(str, "%d", GET_INT(c));
         else if (!str_cmp(field, "wis"))
            sprintf(str, "%d", GET_WIS(c));
         else if (!str_cmp(field, "dex"))
            sprintf(str, "%d", GET_DEX(c));
         else if (!str_cmp(field, "con"))
            sprintf(str, "%d", GET_CON(c));
         else if (!str_cmp(field, "cha"))
            sprintf(str, "%d", GET_CHA(c));

         else if (!str_cmp(field, "room"))
            sprintf(str, "%ld", world[IN_ROOM(c)].number);
         else if (!str_cmp(field,"fighting"))
            {
            if(!FIGHTING(c))
               strcpy(str,"");
            else
               sprintf(str,"%c%ld",UID_CHAR,GET_ID(FIGHTING(c)));
            }
         else if (!str_cmp(field, "is_killer"))
            {
            if (subfield && *subfield)
               {
               if (!str_cmp("on", subfield))
                  SET_BIT(PLR_FLAGS(c), PLR_KILLER);
               else if (!str_cmp("off", subfield))
                  REMOVE_BIT(PLR_FLAGS(c), PLR_KILLER);
               }
            if (PLR_FLAGGED(c, PLR_KILLER))
               strcpy(str, "1");
            else
               strcpy(str, "0");
            }

         else if (!str_cmp(field, "is_thief"))
            {
            if (subfield && *subfield)
               {
               if (!str_cmp("on", subfield))
                  SET_BIT(PLR_FLAGS(c), PLR_THIEF);
               else if (!str_cmp("off", subfield))
                  REMOVE_BIT(PLR_FLAGS(c), PLR_THIEF);
               }
            if (PLR_FLAGGED(c, PLR_THIEF))
               strcpy(str, "1");
            else
               strcpy(str, "0");
            }

         else if (!str_cmp(field,"riding"))
            {
            if(!RIDING(c))
               strcpy(str,"");
            else
               sprintf(str,"%c%ld",UID_CHAR,GET_ID(RIDING(c)));
            }
         else if (!str_cmp(field,"ridden_by"))
            {
            if(!RIDDEN_BY(c))
               strcpy(str,"");
            else
               sprintf(str,"%c%ld",UID_CHAR,GET_ID(RIDDEN_BY(c)));
            }
         else if (!str_cmp(field,"master"))
            {
            if(!c->master)
               strcpy(str,"");
            else
               sprintf(str,"%c%ld",UID_CHAR,GET_ID(c->master));
            }

         else if (!str_cmp(field, "skill"))
            strcpy(str,skill_percent(c, subfield));

	 else if (!str_cmp(field, "is_affected")) {
	   strcpy(str, dg_get_ch_aff(c, subfield));
	 } else if (!str_cmp(field, "is_preference")) {
	   strcpy(str, dg_get_ch_prf(c, subfield));
	 } else if (!str_cmp(field, "is_resist")) {
	   strcpy(str, dg_get_ch_res(c, subfield));
	 } else if (!str_cmp(field, "is_suscept")) {
	   strcpy(str, dg_get_ch_sus(c, subfield));
	 } else if (!str_cmp(field, "is_immune")) {
	   strcpy(str, dg_get_ch_imm(c, subfield));
	 }

         else if (!str_cmp(field, "eq"))
            {
            int pos=-1;
            var_subst(go, c->script, trig, MOB_TRIGGER, subfield, str);
            if((pos==-1) && !strn_cmp(str,"wield",5))
               {
               if(str_cmp("wield_2",str)==0)
                  pos = WEAR_WIELD_2;
               else
                  pos = WEAR_WIELD_1;
               }
            else if((pos == -1) && !strn_cmp(str,"held",4))
               {
               if(str_cmp("held_2",str)==0)
                  pos = WEAR_HOLD_2;
               else
                  pos = WEAR_HOLD_1;
               }
            else if (pos==-1 && isdigit((int)*str))
               pos = atoi(str);
            else if(pos==-1)
               pos = find_eq_pos(c, NULL, str);

            if (!subfield || !*str || pos < 0 || pos > NUM_WEARS)
               strcpy(str,"");
            else
               {
               if(!GET_EQ(c,pos))
                  {
                  strcpy(str,"");
                  }
               else
                  sprintf(str,"%c%ld",UID_CHAR, GET_ID(GET_EQ(c, pos)));
               }
            }
         else if (!str_cmp(field, "varexists"))
            {
            struct trig_var_data *mvd;
            strcpy(str, "0");
            if (SCRIPT(c))
               {
               for (mvd = SCRIPT(c)->global_vars; mvd; mvd = mvd->next)
                  {
                  if (!str_cmp(mvd->name, subfield))
                     break;
                  }
               if (mvd)
                  strcpy(str, "1");
               }
            }
	 else if (!str_cmp(field, "has_completed_quest"))
	 {
	   struct dg_quest *quest = get_dg_quest(subfield);
	   if (!quest) {
	     strcpy(str, "0");
	   } else {
	     int i;
	     strcpy(str, "0");
	     for (i = 0; i < quest->num_completed; i++) {
	       if (c->pfilepos == quest->completed_by[i]) {
		 strcpy(str, "1");
		 return;
	       }
	     }
	   }
	 }
	 else if (!str_cmp(field, "num_rooms_explored_in_zone")) {
	   if (IS_NPC(c)) {
	     sprintf(str, "0");
	     return;
	   }
	   int znum;
	   if (subfield == NULL || strlen(subfield) == 0) {
	     struct zone_data* zone = &zone_table[world[IN_ROOM(c)].zone];
	     znum = zone->number;
	   } else {
	     znum = atoi(subfield);
	   }
	   if (100*znum >= EXPLORED_TOP_VNUM) {
	     sprintf(str, "0");
	     return;
	   }
	   int num_explored = 0;
	   int rnum;
	   for (rnum = 100*znum; rnum < 100*(znum+1); rnum++) {
	     int b = c->player_specials->explored_vnums[rnum/8];
	     if (b & (1 << (rnum%8))) {
	       num_explored++;
	     }
	   }
	   sprintf(str, "%d", num_explored);
	 }
	 else if (!str_cmp(field, "explored")) {
	   if (IS_NPC(c)) {
	     sprintf(str, "0");
	   } else {
	     sprintf(str, "%d", GET_EXPLORED(c));
	   }
	 } else if (!str_cmp(field, "has_explored")) {
	   if (!subfield || IS_NPC(c)) {
	     sprintf(str, "0");
	   } else {
	     int vnum = atoi(subfield);
	     if (vnum < 0 || vnum >= EXPLORED_TOP_VNUM) {
	       sprintf(str, "0");
	     } else {
	       char *vnums = c->player_specials->explored_vnums;
	       sprintf(str, "%d", vnums[vnum/8] & (1 << (vnum%8)) ? 1 : 0);
	     }
	   }
	 }

         else if (!str_cmp(field, "next_in_room"))
            {
            if (c->next_in_room)
               sprintf(str,"%c%ld",UID_CHAR, GET_ID(c->next_in_room));
            else
               strcpy(str,"");
            }
         else if (!str_cmp(field, "inventory"))
            {
            if (c->carrying)
               sprintf(str, "%c%ld", UID_CHAR, GET_ID(c->carrying));
            else
               strcpy(str,"");
            }
         else
            {
            char *buf2=get_buffer(512);
            if (SCRIPT(c))
               {
               for (vd = (SCRIPT(c))->global_vars; vd; vd = vd->next)
                  if (!str_cmp(vd->name, field))
                     break;
               if (vd)
                  sprintf(str, "%s", vd->value);
               else
                  {
                  *str = '\0';
                  sprintf(buf2,
                          "Trigger: %s, VNum %ld. unknown char field: '%s'",
                          GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), field);
                  script_log(buf2);
                  }
               }
            else
               {
               *str = '\0';
               sprintf(buf2,
                       "Trigger: %s, VNum %ld. unknown char field: '%s'",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), field);
               script_log(buf2);
               }
            release_buffer(buf2);
            }
         }
      else if (o)
         {
         if (text_processed(field,subfield,vd,str))
            return;
	 /* Fixed name and alias - Nomikos 6/14/2025 */
         else if (!str_cmp(field, "name"))
            strcpy(str, o->short_description);
         else if (!str_cmp(field, "alias"))
            strcpy(str, o->name);
         /* MANWE_OACCESS: Allow access to object data*/
         else if (!str_cmp(field, "in_obj"))
            {
            if (o->in_obj)
               sprintf(str, "%c%ld", UID_CHAR, GET_ID(o->in_obj));
            else
               strcpy(str,"");
            }
         else if (!str_cmp(field, "contains"))
            {
            if (o->contains)
               sprintf(str, "%c%ld", UID_CHAR, GET_ID(o->contains));
            else
               strcpy(str,"");
            }
         else if (!str_cmp(field, "next_content"))
            {
            if (o->next_content)
               sprintf(str, "%c%ld", UID_CHAR, GET_ID(o->next_content));
            else
               strcpy(str,"");
            }
         /* MANWE_OACCESS: End of additions */

         else if (!str_cmp(field, "id"))
            sprintf(str, "%ld", GET_ID(o));
         else if (!str_cmp(field, "shortdesc"))
            strcpy(str, o->short_description);

         else if (!str_cmp(field, "vnum"))
            sprintf(str, "%ld", GET_OBJ_VNUM(o));

         else if (!str_cmp(field, "type"))
            sprinttype(GET_OBJ_TYPE(o), item_types, str);

         else if (!str_cmp(field, "timer"))
            sprintf(str, "%d", GET_OBJ_DGTIMER(o));

         else if (!str_cmp(field, "worn_by"))
            {
            if(o->worn_by != NULL)
               {
               sprintf(str,"%c%ld",UID_CHAR, GET_ID(o->worn_by));
               }
            else
               {
               strcpy(str,"");
               }
            }
         else if (!str_cmp(field, "carried_by"))
            {
            if(o->carried_by != NULL)
               {
               sprintf(str,"%c%ld",UID_CHAR, GET_ID(o->carried_by));
               }
            else
               {
               strcpy(str,"");
               }
            }
         /* Some scripts don't return an actor field, so this allows the owner to be found - Nomikos 9-8-2025 */
		 else if (!str_cmp(field, "owner"))
		    {
            obj_data *tObj = o;

            /* If the object is in one ore more containers, find the outermost container */
            if (tObj->in_obj != NULL)
               {
        		while (tObj->in_obj)
                   tObj = tObj->in_obj;
     		   }

            /* If the object is carried, that is the owner */
            if (tObj->carried_by != NULL)
               sprintf(str,"%c%ld", UID_CHAR, GET_ID(tObj->carried_by));
            /* If the object is worn, that is the owner */
            else if (tObj->worn_by != NULL)
               sprintf(str,"%c%ld", UID_CHAR, GET_ID(tObj->worn_by));
            /* Otherwise, it doesn't have an owner (on ground or in container on ground) */
            else
			   strcpy(str, "");
			}
         else if (!str_cmp(field, "val0"))
            sprintf(str, "%ld", GET_OBJ_VAL(o, 0));
         else if (!str_cmp(field, "val1"))
            sprintf(str, "%ld", GET_OBJ_VAL(o, 1));
         else if (!str_cmp(field, "val2"))
            sprintf(str, "%ld", GET_OBJ_VAL(o, 2));
         else if (!str_cmp(field, "val3"))
            sprintf(str, "%ld", GET_OBJ_VAL(o, 3));
         else if (!str_cmp(field, "val4"))
            sprintf(str, "%ld", GET_OBJ_VAL(o, 4));
         else if (!str_cmp(field, "val5"))
            sprintf(str, "%ld", GET_OBJ_VAL(o, 5));
         else if (!str_cmp(field, "val6"))
            sprintf(str, "%ld", GET_OBJ_VAL(o, 6));
         else if (!str_cmp(field, "val7"))
            sprintf(str, "%ld", GET_OBJ_VAL(o, 7));

         else if (!str_cmp(field, "weight"))
            sprintf(str, "%d", GET_OBJ_WEIGHT(o));

         else if (!str_cmp(field, "material"))
            sprinttype(o->material, material_types, str);

         else
            {
            char *buf2=get_buffer(512);
            *str = '\0';
            sprintf(buf2,
                    "Trigger: %s, VNum %ld, type: %d. unknown object field: '%s'",
                    GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
            script_log(buf2);
            release_buffer(buf2);
            }
         }

      else if (r)
         {
         if (text_processed(field,subfield, vd, str))
            return;
         /* MANWE_RFLAGS: Allow access to the room flags */
         else if (!str_cmp(field, "flags"))
            sprintbit(r->room_flags, room_bits, str);
         else if (!str_cmp(field, "name"))
            strcpy(str, r->name);
         /* MANWE_UID: Ensure it is a UID */
         else if (!str_cmp(field, "vnum"))
            sprintf(str, "%c%ld",UID_CHAR, r->number);
         else if (!str_cmp(field, "people"))
            {
            if (r->people)
               sprintf(str,"%c%ld",UID_CHAR,GET_ID(r->people));
            else
               *str = '\0';
            }
         /* MANWE_RCONTENTS: Allow access to objects in a room */
         else if (!str_cmp(field, "contents"))
            {
            if (r->contents)
               sprintf(str, "%c%ld", UID_CHAR, GET_ID(r->contents));
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "north"))
            {
            if (r->dir_option[NORTH])
               sprintbit(r->dir_option[NORTH]->exit_info ,exit_bits, str);
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "east"))
            {
            if (r->dir_option[EAST])
               sprintbit(r->dir_option[EAST]->exit_info ,exit_bits, str);
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "south"))
            {
            if (r->dir_option[SOUTH])
               sprintbit(r->dir_option[SOUTH]->exit_info ,exit_bits, str);
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "west"))
            {
            if (r->dir_option[WEST])
               sprintbit(r->dir_option[WEST]->exit_info ,exit_bits, str);
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "up"))
            {
            if (r->dir_option[UP])
               sprintbit(r->dir_option[UP]->exit_info ,exit_bits, str);
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "down"))
            {
            if (r->dir_option[DOWN])
               sprintbit(r->dir_option[DOWN]->exit_info ,exit_bits, str);
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "num_north"))
            {
            if (r->dir_option[NORTH])
               sprintf(str, "%ld", GET_ROOM_VNUM(r->dir_option[NORTH]->to_room));
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "num_east"))
            {
            if (r->dir_option[EAST])
               sprintf(str, "%ld", GET_ROOM_VNUM(r->dir_option[EAST]->to_room));
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "num_south"))
            {
            if (r->dir_option[SOUTH])
               sprintf(str, "%ld", GET_ROOM_VNUM(r->dir_option[SOUTH]->to_room));
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "num_west"))
            {
            if (r->dir_option[WEST])
               sprintf(str, "%ld", GET_ROOM_VNUM(r->dir_option[WEST]->to_room));
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "num_up"))
            {
            if (r->dir_option[UP])
               sprintf(str, "%ld", GET_ROOM_VNUM(r->dir_option[UP]->to_room));
            else
               *str = '\0';
            }
         else if (!str_cmp(field, "num_down"))
            {
            if (r->dir_option[DOWN])
               sprintf(str, "%ld", GET_ROOM_VNUM(r->dir_option[DOWN]->to_room));
            else
               *str = '\0';
            }
	 else if (!str_cmp(field, "is_flagged")) {
	   strcpy(str, dg_get_room_flags(r, subfield));
	 } else
            {
            char *buf2=get_buffer(512);
            *str = '\0';
            sprintf(buf2,
                    "Trigger: %s, VNum %ld, type: %d. unknown room field: '%s'",
                    GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
            script_log(buf2);
            release_buffer(buf2);
            }
         }

      else
         *str = '\0';
      }
   }


/* substitutes any variables into line and returns it as buf */
void var_subst(void *go, struct script_data *sc, trig_data *trig,
               int type, char *line, char *buf)
   {
   char *tmp=get_buffer(MAX_INPUT_LENGTH);
   char *repl_str=get_buffer(MAX_INPUT_LENGTH);
   char *subfield=get_buffer(MAX_INPUT_LENGTH);
   char *var, *field, *p;
   char *subfield_p;
   int left, len;
   int paren_count = 0;

   if (!strchr(line, '%'))
      {
      strcpy(buf, line);
      release_buffer(subfield);
      release_buffer(repl_str);
      release_buffer(tmp);
      return;
      }

   /* We now know there are at least some variables to convert */
   /* If within there is at least 1 '[', we go check */
   if (strchr(line, '['))
      {
      parse_embedded(go,sc,trig,type,line,repl_str);
      p = strcpy(tmp, repl_str);
      }
   else
      p = strcpy(tmp, line);

   subfield_p = subfield;

   left = MAX_INPUT_LENGTH - 1;

   while (*p && (left > 0))
      {

      while (*p && (*p != '%') && (left > 0))
         {
         *(buf++) = *(p++);
         left--;
         }

      *buf = '\0';

      /* double % */
      if (*p && (*(++p) == '%') && (left > 0))
         {
         *(buf++) = *(p++);
         *buf = '\0';
         left--;
         continue;
         }

      else if (*p && (left > 0))
         {

         for (var = p; *p && (*p != '%') && (*p != '.'); p++)
            ;

         field = p;
         if (*p == '.')
            {
            *(p++) = '\0';
            for (field = p; *p && ((*p != '%')||(paren_count)); p++)
               {
               if (*p=='(')
                  {
                  *p = '\0';
                  paren_count++;
                  }
               else if (*p==')')
                  {
                  *p = '\0';
                  paren_count--;
                  }
               else if (paren_count)
                  *subfield_p++ = *p;

               }
            }

         *(p++) = '\0';
         *subfield_p = '\0';

         find_replacement(go, sc, trig, type, var, field, subfield,repl_str);

         strncat(buf, repl_str, left);
         len = strlen(repl_str);
         buf += len;
         left -= len;
         }
      }
   release_buffer(subfield);
   release_buffer(repl_str);
   release_buffer(tmp);
   }

void parse_embedded(void *go, struct script_data *sc, struct trig_data *trig,
                    int type, char *str, char* buf)
   {
   char *field = get_buffer(512);
   char *subfield = get_buffer(512);
   char *p, *cp, *tailp;
   char *replstr = get_buffer(1024);
   char *interim = get_buffer(1024);
   size_t i, j, len;
   size_t delimStart = 0;
   size_t delimStop = 0;
   int bFoundDelim = 0;
   int bFoundNormal = 0;
   int bDone = 0;

   field[0] = '\0';
   subfield[0] = '\0';

   strcpy(interim, str);
   strcpy(buf, interim);
   while (!bDone)
      {
      bFoundDelim = 0;
      bFoundNormal = 0;
      delimStart = 0;
      delimStop = 0;
      len = strlen(buf);
      for (i = 0, p = buf ; i < len ; i++)
         {
         switch (*(p+i))
            {
         case '[':
            if (bFoundNormal)
               delimStart = i;
            break;
         case ']':
            if (delimStart)
               delimStop = i;
            break;
         case '%':
            if (!bFoundNormal)
               {
               bFoundNormal++;
               delimStart=0;
               }
            else
               {
               bFoundNormal--;
               if (delimStop)
                  bFoundDelim++;
               }
            break;
            }
         if (bFoundDelim && delimStop)
            {
            char *tmp;
            cp = replstr;
            for (j = delimStart + 1 ; j < delimStop ; j++, cp++)
               *cp = *(p+j);
            *(cp) = '\0';
            if ((tmp = strchr(replstr, '.')))
               {
               subfield = replstr;
               *tmp='\0';
               field = (tmp+1);
               }
            find_replacement(go,sc,trig,type,replstr,field,subfield,replstr);
            *(p+delimStart) = '\0';
            tailp = p + delimStop + 1;
            sprintf(interim, "%s%s%s", p, replstr, tailp);
            strcpy(buf, interim);
            break;
            }
         }
      if (!bFoundDelim)
         bDone = 1;
      }
   release_buffer(replstr);
   release_buffer(interim);
   release_buffer(field);
   release_buffer(subfield);
   }



/* returns 1 if string is all digits, else 0 */
int is_num(char *num)
   {
   while (*num && (isdigit((int)*num) || *num=='-'))
      num++;

   if (!*num || isspace((int)*num))
      return 1;
   else
      return 0;
   }


/* evaluates 'lhs op rhs', and copies to result */
void eval_op(char *op, char *lhs, char *rhs, char *result, void *go,
             struct script_data *sc, trig_data *trig)
   {
   char *p;
   int n;

   /* strip off extra spaces at begin and end */
   while (*lhs && isspace((int)*lhs))
      lhs++;
   while (*rhs && isspace((int)*rhs))
      rhs++;

   for (p = lhs; *p; p++)
      ;
   for (--p; (p > lhs) && isspace((int)*p); *p-- = '\0')
      ;
   for (p = rhs; *p; p++)
      ;
   for (--p; (p > rhs) && isspace((int)*p); *p-- = '\0')
      ;


   /* find the op, and figure out the value */
   if (!strcmp("||", op))
      {
      if ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0')))
         strcpy(result, "0");
      else
         strcpy(result, "1");
      }

   else if (!strcmp("&&", op))
      {
      if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
         strcpy (result, "0");
      else
         strcpy (result, "1");
      }

   else if (!strcmp("==", op))
      {
      if (is_num(lhs) && is_num(rhs))
         sprintf(result, "%d", atoi(lhs) == atoi(rhs));
      else
         sprintf(result, "%d", !str_cmp(lhs, rhs));
      }

   else if (!strcmp("!=", op))
      {
      if (is_num(lhs) && is_num(rhs))
         sprintf(result, "%d", atoi(lhs) != atoi(rhs));
      else
         sprintf(result, "%d", str_cmp(lhs, rhs));
      }

   else if (!strcmp("<=", op))
      {
      if (is_num(lhs) && is_num(rhs))
         sprintf(result, "%d", atoi(lhs) <= atoi(rhs));
      else
         sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
      }

   else if (!strcmp(">=", op))
      {
      if (is_num(lhs) && is_num(rhs))
         sprintf(result, "%d", atoi(lhs) >= atoi(rhs));
      else
         sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
      }

   else if (!strcmp("<", op))
      {
      if (is_num(lhs) && is_num(rhs))
         sprintf(result, "%d", atoi(lhs) < atoi(rhs));
      else
         sprintf(result, "%d", str_cmp(lhs, rhs) < 0);
      }

   else if (!strcmp(">", op))
      {
      if (is_num(lhs) && is_num(rhs))
         sprintf(result, "%d", atoi(lhs) > atoi(rhs));
      else
         sprintf(result, "%d", str_cmp(lhs, rhs) > 0);
      }

   else if (!strcmp("/=", op))
      /* MANWE_STRINGS: Return where it was found instead of */
      /*                just whether it was found.           */
      sprintf(result, "%d", (p=str_str(lhs, rhs)) ?
              ((p-lhs)/sizeof(char))+1 : 0);

   else if (!strcmp("*", op))
      sprintf(result, "%d", atoi(lhs) * atoi(rhs));

   else if (!strcmp("/", op))
      sprintf(result, "%d", (n = atoi(rhs)) ? (atoi(lhs) / n) : 0);

   else if (!strcmp("+", op))
      sprintf(result, "%d", atoi(lhs) + atoi(rhs));

   else if (!strcmp("-", op))
      sprintf(result, "%d", atoi(lhs) - atoi(rhs));

   else if (!strcmp("!", op))
      {
      if (is_num(rhs))
         sprintf(result, "%d", !atoi(rhs));
      else
         sprintf(result, "%d", !*rhs);
      }
   }


/*
 * p points to the first quote, returns the matching
 * end quote, or the last non-null char in p.
*/
char *matching_quote(char *p)
   {
   for (p++; *p && (*p != '"'); p++)
      {
      if (*p == '\\')
         p++;
      }

   if (!*p)
      p--;

   return p;
   }

/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
char *matching_paren(char *p)
   {
   int i;

   for (p++, i = 1; *p && i; p++)
      {
      if (*p == '(')
         i++;
      else if (*p == ')')
         i--;
      else if (*p == '"')
         p = matching_quote(p);
      }

   return --p;
   }


/* evaluates line, and returns answer in result */
void eval_expr(char *line, char *result, void *go, struct script_data *sc,
               trig_data *trig, int type)
   {
   char *expr=get_buffer(MAX_INPUT_LENGTH);
   char *p;

   while (*line && isspace((int)*line))
      line++;

   if (eval_lhs_op_rhs(line, result, go, sc, trig, type))
      ;

   else if (*line == '(')
      {
      p = strcpy(expr, line);
      p = matching_paren(expr);
      *p = '\0';
      eval_expr(expr + 1, result, go, sc, trig, type);
      }

   else
      var_subst(go, sc, trig, type, line, result);
   release_buffer(expr);
   }


/*
 * evaluates expr if it is in the form lhs op rhs, and copies
 * answer in result.  returns 1 if expr is evaluated, else 0
 */
int eval_lhs_op_rhs(char *expr, char *result, void *go, struct script_data *sc,
                    trig_data *trig, int type)
   {
   char *p, *tokens[MAX_INPUT_LENGTH];
   char *line=get_buffer(MAX_INPUT_LENGTH);
   char *lhr=get_buffer(MAX_INPUT_LENGTH);
   char *rhr=get_buffer(MAX_INPUT_LENGTH);
   int i, j;

   /*
    * valid operands, in order of priority
    * each must also be defined in eval_op()
    */
   static char *ops[] = {
                           "||",
                           "&&",
                           "==",
                           "!=",
                           "<=",
                           ">=",
                           "<",
                           ">",
                           "/=",
                           "-",
                           "+",
                           "/",
                           "*",
                           "!",
                           "\n"
                        };

   p = strcpy(line, expr);

   /*
    * initialize tokens, an array of pointers to locations
    * in line where the ops could possibly occur.
    */
   for (j = 0; *p; j++)
      {
      tokens[j] = p;
      if (*p == '(')
         p = matching_paren(p) + 1;
      else if (*p == '"')
         p = matching_quote(p) + 1;
      else if (isalnum((int)*p))
         for (p++; *p && (isalnum((int)*p) || isspace((int)*p)); p++)
            ;
      else
         p++;
      }
   tokens[j] = NULL;

   for (i = 0; *ops[i] != '\n'; i++)
      for (j = 0; tokens[j]; j++)
         if (!strn_cmp(ops[i], tokens[j], strlen(ops[i])))
            {
            *tokens[j] = '\0';
            p = tokens[j] + strlen(ops[i]);

            eval_expr(line, lhr, go, sc, trig, type);
            eval_expr(p, rhr, go, sc, trig, type);
            eval_op(ops[i], lhr, rhr, result, go, sc, trig);
            release_buffer(line);
            release_buffer(lhr);
            release_buffer(rhr);
            return 1;
            }

   release_buffer(line);
   release_buffer(lhr);
   release_buffer(rhr);
   return 0;
   }



/* returns 1 if cond is true, else 0 */
int process_if(char *cond, void *go, struct script_data *sc,
               trig_data *trig, int type)
   {
   char *result=get_buffer(MAX_INPUT_LENGTH);
   char *p;

   eval_expr(cond, result, go, sc, trig, type);

   p = result;
   skip_spaces(&p);

   if (!*p || *p == '0')
      {
      release_buffer(result);
      return 0;
      }
   else
      {
      release_buffer(result);
      return 1;
      }
   }


/*
 * scans for end of if-block.
 * returns the line containg 'end', or the last
 * line of the trigger if not found.
 */
struct cmdlist_element *find_end(struct cmdlist_element *cl)
   {
   struct cmdlist_element *c;
   char *p;

   if (!(cl->next))
      return cl;

   for (c = cl->next; c->next; c = c->next)
      {
      for (p = c->cmd; *p && isspace((int)*p); p++)
         ;

      if (!strn_cmp("if ", p, 3))
         c = find_end(c);
      else if (!strn_cmp("end", p, 3))
         return c;
      }

   return c;
   }


/*
 * searches for valid elseif, else, or end to continue execution at.
 * returns line of elseif, else, or end if found, or last line of trigger.
 */
struct cmdlist_element *find_else_end(trig_data *trig,
                                               struct cmdlist_element *cl, void *go,
                                               struct script_data *sc, int type)
   {
   struct cmdlist_element *c;
   char *p;

   if (!(cl->next))
      return cl;

   for (c = cl->next; c->next; c = c->next)
      {
      for (p = c->cmd; *p && isspace((int)*p); p++)
         ;

      if (!strn_cmp("if ", p, 3))
         c = find_end(c);

      else if (!strn_cmp("elseif ", p, 7))
         {
         if (process_if(p + 7, go, sc, trig, type))
            {
            GET_TRIG_DEPTH(trig)++;
            return c;
            }
         }

      else if (!strn_cmp("else", p, 4))
         {
         GET_TRIG_DEPTH(trig)++;
         return c;
         }

      else if (!strn_cmp("end", p, 3))
         return c;
      }

   return c;
   }


/* processes any 'wait' commands in a trigger */
void process_wait(void *go, trig_data *trig, int type, char *cmd,
                  struct cmdlist_element *cl)
   {
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   struct wait_event_data *wait_event_obj;
   long time_delay, hr, min, ntime;
   char c, *arg;



   arg = any_one_arg(cmd, buf);
   skip_spaces(&arg);

   if (!*arg)
      {
      char *buf2=get_buffer(512);
      sprintf(buf2, "Trigger: %s, VNum %ld. wait w/o an arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cl->cmd);
      script_log(buf2);
      release_buffer(buf2);
      }

   else if (!strn_cmp(arg, "until ", 6))
      {

      /* valid forms of time are 14:30 and 1430 */
      if (sscanf(arg, "until %ld:%ld", &hr, &min) == 2)
         min += (hr * 60);
      else
         min = (hr % 100) + ((hr / 100) * 60);

      /* calculate the pulse of the day of "until" time */
      ntime = (min * SECS_PER_MUD_HOUR * PASSES_PER_SEC) / 60;

      /* calculate pulse of day of current time */
      time_delay = (dg_global_pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)) +
                   (time_info.hours * SECS_PER_MUD_HOUR * PASSES_PER_SEC);

      if (time_delay >= ntime) /* adjust for next day */
         time_delay = (SECS_PER_MUD_DAY * PASSES_PER_SEC) - time_delay + ntime;
      else
         time_delay = ntime - time_delay;
      }

   else
      {
      if (sscanf(arg, "%ld %c", &time_delay, &c) == 2)
         {
         if (c == 't')
            time_delay *= PULSES_PER_MUD_HOUR;
         else if (c == 's')
            time_delay *= PASSES_PER_SEC;
         }
      }

   CREATE(wait_event_obj, struct wait_event_data, 1);
   wait_event_obj->trigger = trig;
   wait_event_obj->go = go;
   wait_event_obj->type = type;

   GET_TRIG_WAIT(trig) = add_event(time_delay,trig_wait_event, wait_event_obj);
   trig->curr_state = cl->next;
   release_buffer(buf);
   }


/* DG Usage: hold %actor% <seconds>
 * "Stuns" the user, but without any messages (RP this. :-)
 * Note this STACKS the lag you ask for on top of anything else they have.
 */
void process_hold(struct script_data *sc, trig_data *trig, char *cmd)
{
  char *buf = get_buffer(MAX_STRING_LENGTH);
  strcpy(buf, cmd);
  strtok(buf, " ");

  char *arg1 = strtok(NULL, " ");
  char *arg2 = strtok(NULL, " ");
  if (!arg1 || !arg2 || atoi(arg2) <= 0) {
    sprintf(buf, "Trigger: %s, VNum %ld. hold w/missing or invalid arg: '%s'", GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    script_log(buf);
    release_buffer(buf);
    return;
  }

  struct char_data *ch = find_char(atoi(arg1+1)); /* Find a char by forming the UID... which is done by ignoring the first digit (for mobs or PCs). */
  if (!ch) {
    sprintf(buf, "Trigger: %s, VNum %ld. hold no target: '%s'", GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    script_log(buf);
    release_buffer(buf);
    return;
  }

  GET_WAIT_STATE(ch) += (atoi(arg2)) RL_SEC;
  release_buffer(buf);
}

/* processes a script set command */
void process_set(struct script_data *sc, trig_data *trig, char *cmd)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   char *name=get_buffer(MAX_INPUT_LENGTH);
   char *value;

   value = two_arguments(cmd, arg, name);

   skip_spaces(&value);

   if (!*name)
      {
      char *buf2=get_buffer(512);
      sprintf(buf2, "Trigger: %s, VNum %ld. set w/o an arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      script_log(buf2);
      release_buffer(buf2);
      release_buffer(name);
      release_buffer(arg);
      return;
      }

   add_var(&GET_TRIG_VARS(trig), name, value,sc->context);
   release_buffer(name);
   release_buffer(arg);
   }

/* processes a script eval command */
void process_eval(void *go, struct script_data *sc, trig_data *trig,
                  int type, char *cmd)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   char *name=get_buffer(MAX_INPUT_LENGTH);
   char *result=get_buffer(MAX_INPUT_LENGTH);
   char *expr;

   expr = two_arguments(cmd, arg, name);

   skip_spaces(&expr);

   if (!*name)
      {
      char *buf2=get_buffer(512);
      sprintf(buf2, "Trigger: %s, VNum %ld. eval w/o an arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      script_log(buf2);
      release_buffer(buf2);
      release_buffer(result);
      release_buffer(name);
      release_buffer(arg);
      return;
      }

   eval_expr(expr, result, go, sc, trig, type);
   add_var(&GET_TRIG_VARS(trig), name, result,sc->context);
   release_buffer(result);
   release_buffer(name);
   release_buffer(arg);
   }

/* script attaching a trigger to something */
void process_attach(void *go, struct script_data *sc, trig_data *trig,
                    int type, char *cmd)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   char *trignum_s=get_buffer(MAX_INPUT_LENGTH);
   char *result=get_buffer(MAX_INPUT_LENGTH);
   char *id_p;
   trig_data *newtrig;
   char_data *c=NULL;
   obj_data *o=NULL;
   room_data *r=NULL;
   long trignum, id;

   id_p = two_arguments(cmd, arg, trignum_s);
   skip_spaces(&id_p);

   if (!*trignum_s)
      {
      char *buf2=get_buffer(256);
      sprintf(buf2, "Trigger: %s, VNum %ld. attach w/o an arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      script_log(buf2);
      release_buffer(buf2);
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   /* Strip the UID prefix */
   if (*id_p == UID_CHAR)
      id_p++;


   if (!id_p || !*id_p)
      {
      char *buf2=get_buffer(256);
      sprintf(buf2, "Trigger: %s, VNum %ld. attach w/o id arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      script_log(buf2);
      release_buffer(buf2);
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   /* parse and locate the id specified */
   eval_expr(id_p, result, go, sc, trig, type);
   if (!(id = atoi(result)))
      {
      char *buf2=get_buffer(256);
      sprintf(buf2, "Trigger: %s, VNum %ld. attach invalid id arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      script_log(buf2);
      release_buffer(buf2);
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }
   c = find_char(id);
   if (!c)
      {
      o = find_obj_dg(id);
      if (!o)
         {
         r = find_room(id);
         if (!r)
            {
            char *buf2=get_buffer(256);
            sprintf(buf2, "Trigger: %s, VNum %ld. attach invalid id arg: '%s'",
                    GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
            script_log(buf2);
            release_buffer(buf2);
            release_buffer(result);
            release_buffer(trignum_s);
            release_buffer(arg);
            return;
            }
         }
      }

   /* locate and load the trigger specified */
   trignum = real_trigger(atoi(trignum_s));
   if (trignum<0 || !(newtrig=read_trigger(trignum)))
      {
      char *buf2=get_buffer(256);
      sprintf(buf2, "Trigger: %s, VNum %ld. attach invalid trigger: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), trignum_s);
      script_log(buf2);
      release_buffer(buf2);
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   if (c)
      {
      if (!SCRIPT(c))
         CREATE(SCRIPT(c), struct script_data, 1);
      add_trigger(SCRIPT(c), newtrig, -1);
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   if (o)
      {
      if (!SCRIPT(o))
         CREATE(SCRIPT(o), struct script_data, 1);
      add_trigger(SCRIPT(o), newtrig, -1);
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   if (r)
      {
      if (!SCRIPT(r))
         CREATE(SCRIPT(r), struct script_data, 1);
      add_trigger(SCRIPT(r), newtrig, -1);
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   release_buffer(result);
   release_buffer(trignum_s);
   release_buffer(arg);
   }


/* script detaching a trigger from something */
void process_detach(void *go, struct script_data *sc, trig_data *trig,
                    int type, char *cmd)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   char *trignum_s=get_buffer(MAX_INPUT_LENGTH);
   char *result=get_buffer(MAX_INPUT_LENGTH);
   char *id_p;
   char_data *c=NULL;
   obj_data *o=NULL;
   room_data *r=NULL;
   long id;

   id_p = two_arguments(cmd, arg, trignum_s);
   skip_spaces(&id_p);

   if (!*trignum_s)
      {
      char *buf2=get_buffer(256);
      sprintf(buf2, "Trigger: %s, VNum %ld. detach w/o an arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      script_log(buf2);
      release_buffer(buf2);
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   if (!id_p || !*id_p || atoi(id_p)==0)
      {
      char *buf2=get_buffer(256);
      sprintf(buf2, "Trigger: %s, VNum %ld. detach invalid id arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      script_log(buf2);
      release_buffer(buf2);
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   /* parse and locate the id specified */
   eval_expr(id_p, result, go, sc, trig, type);
   if (!(id = atoi(result)))
      {
      char *buf2=get_buffer(256);
      sprintf(buf2, "Trigger: %s, VNum %ld. detach invalid id arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      script_log(buf2);
      release_buffer(buf2);
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }
   c = find_char(id);
   if (!c)
      {
      o = find_obj_dg(id);
      if (!o)
         {
         r = find_room(id);
         if (!r)
            {
            char *buf2=get_buffer(256);
            sprintf(buf2, "Trigger: %s, VNum %ld. detach invalid id arg: '%s'",
                    GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
            script_log(buf2);
            release_buffer(buf2);
            release_buffer(result);
            release_buffer(trignum_s);
            release_buffer(arg);
            return;
            }
         }
      }


   if (c && SCRIPT(c))
      {
      if (remove_trigger(SCRIPT(c), trignum_s))
         {
         if (!TRIGGERS(SCRIPT(c)))
            {
            extract_script(SCRIPT(c));
            SCRIPT(c) = NULL;
            }
         }
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   if (o && SCRIPT(o))
      {
      if (remove_trigger(SCRIPT(o), trignum_s))
         {
         if (!TRIGGERS(SCRIPT(o)))
            {
            extract_script(SCRIPT(o));
            SCRIPT(o) = NULL;
            }
         }
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   if (r && SCRIPT(r))
      {
      if (remove_trigger(SCRIPT(r), trignum_s))
         {
         if (!TRIGGERS(SCRIPT(r)))
            {
            extract_script(SCRIPT(r));
            SCRIPT(r) = NULL;
            }
         }
      release_buffer(result);
      release_buffer(trignum_s);
      release_buffer(arg);
      return;
      }

   release_buffer(result);
   release_buffer(trignum_s);
   release_buffer(arg);
   }


extern int top_of_zone_table;
extern void reset_zone(zone_rnum);

void process_zreset(void *go, struct script_data *sc, trig_data *trig, int type, char *cmd)
{
  int vnum = GET_TRIG_VNUM(trig);
  int zone = vnum/100;
  int i;
  for (i = 0; i <= top_of_zone_table; i++) {
    if (zone_table[i].number == zone) {
      reset_zone(i);
      return;
    }
  }
}





struct room_data *dg_room_of_obj(struct obj_data *obj)
   {
   if (obj->in_room > NOWHERE)
      return &world[obj->in_room];
   if (obj->carried_by)
      return &world[obj->carried_by->in_room];
   if (obj->worn_by)
      return &world[obj->worn_by->in_room];
   if (obj->in_obj)
      return (dg_room_of_obj(obj->in_obj));
   return NULL;
   }



/* create a UID variable from the id number */
void makeuid_var(void *go, struct script_data *sc, trig_data *trig,
                 int type, char *cmd)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   char *varname=get_buffer(MAX_INPUT_LENGTH);
   char *result=get_buffer(MAX_INPUT_LENGTH);
   char *uid_p;
   char *uid=get_buffer(MAX_INPUT_LENGTH);

   uid_p = two_arguments(cmd, arg, varname);
   skip_spaces(&uid_p);

   if (!*varname)
      {
      char *buf2=get_buffer(512);
      sprintf(buf2, "Trigger: %s, VNum %ld. makeuid w/o an arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      script_log(buf2);
      release_buffer(uid);
      release_buffer(buf2);
      release_buffer(result);
      release_buffer(varname);
      release_buffer(arg);
      return;
      }

   if (!uid_p || !*uid_p || atoi(uid_p)==0)
      {
      char *buf2=get_buffer(512);
      sprintf(buf2, "Trigger: %s, VNum %ld. makeuid invalid id arg: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      script_log(buf2);
      release_buffer(uid);
      release_buffer(buf2);
      release_buffer(result);
      release_buffer(varname);
      release_buffer(arg);
      return;
      }

   eval_expr(uid_p, result, go, sc, trig, type);
   sprintf(uid,"%c%s",UID_CHAR, result);
   add_var(&GET_TRIG_VARS(trig), varname, uid, sc->context);
   release_buffer(uid);
   release_buffer(result);
   release_buffer(varname);
   release_buffer(arg);
   }




/*
 * processes a script return command.
 * returns the new value for the script to return.
 */
int process_return(trig_data *trig, char *cmd)
                     {
                     char *arg1=get_buffer(MAX_INPUT_LENGTH);
                     char *arg2=get_buffer(MAX_INPUT_LENGTH);
                     int itmp;
                     two_arguments(cmd, arg1, arg2);

                     if (!*arg2)
                        {
                        char *buf2=get_buffer(512);
                        sprintf(buf2, "Trigger: %s, VNum %ld. return w/o an arg: '%s'",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
                        script_log(buf2);
                        release_buffer(buf2);
                        release_buffer(arg2);
                        release_buffer(arg1);
                        return 1;
                        }
                     itmp=atoi(arg2);
                     release_buffer(arg2);
                     release_buffer(arg1);

                     return itmp;
                     }


                  /*
                   * removes a variable from the global vars of sc,
                   * or the local vars of trig if not found in global list.
                   */
                  void process_unset(void *go, struct script_data *sc, trig_data *trig,
                                     int type, char *cmd)
                     {
                     char *arg=get_buffer(MAX_INPUT_LENGTH);
                     char *var;

                     /* MANWE: To allow unsetting of referenced variables */
                     if (strchr(cmd, '%'))
                        {
                        char *orig=get_buffer(MAX_INPUT_LENGTH);
                        char *replstr = get_buffer(MAX_INPUT_LENGTH);
                        size_t len, i;
                        strcpy(orig, cmd);
                        if (strchr(orig, '['))
                           {
                           parse_embedded(go,sc,trig,type,orig,replstr);
                           strcpy(orig, replstr);
                           }
                        len = strlen(replstr);
                        for (i = 0 ; i < len ; i++)
                           if (*(replstr+i) == '%')
                              *(replstr+i) = ' ';
                        strcpy(cmd, replstr);
                        release_buffer(orig);
                        release_buffer(replstr);
                        }


                     var = any_one_arg(cmd, arg);
                     skip_spaces(&var);

                     if (!*var)
                        {
                        char *buf2=get_buffer(512);
                        sprintf(buf2, "Trigger: %s, VNum %ld. unset w/o an arg: '%s'",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
                        script_log(buf2);
                        release_buffer(buf2);
                        release_buffer(arg);
                        return;
                        }
                     if (!remove_var(&(sc->global_vars), var,sc->context))
                        remove_var(&GET_TRIG_VARS(trig), var,sc->context);
                     release_buffer(arg);
                     }

                  /*
                  * copy a locally owned variable to the globals of another script
                  *     'remote <variable_name> <uid>'
                  */
                  void process_remote(struct script_data *sc, trig_data *trig, char *cmd)
                     {
                     struct trig_var_data *vd;
                     struct script_data *sc_remote=NULL;
                     char *line, *var, *uid_p;
                     char                  *arg = get_buffer(MAX_INPUT_LENGTH);
                     long                   uid, context;
                     char                  *tgtUID=get_buffer(512);
                     char                  *tgtVar=get_buffer(512);

                     room_data *room;
                     char_data *mob;
                     obj_data *obj;

                     line = any_one_arg(cmd, arg);
                     two_arguments(line, tgtVar, tgtUID);
                     var = tgtVar;
                     uid_p = tgtUID;
                     skip_spaces(&var);
                     skip_spaces(&uid_p);


                     /* If either the variable or the UID is missing, it is an error */
                     if (!*tgtVar || !*tgtUID)
                        {
                        sprintf(tgtUID, "Trigger: %s, VNum %ld. remote: invalid arguments '%s'",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
                        script_log(tgtUID);
                        release_buffer(tgtVar);
                        release_buffer(tgtUID);
                        release_buffer(arg);
                        return;
                        }

                     /* find the locally owned variable */
                     for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
                        if (!str_cmp(vd->name, var))
                           break;

                     /* If not found locally, try the global list */
                     if (!vd)
                        for (vd = sc->global_vars; vd; vd = vd->next)
                           if (!str_cmp(vd->name, var) &&
                                   (vd->context==0 || vd->context==sc->context))
                              break;

                     /* If still not found, it is an error */
                     if (!vd)
                        {
                        sprintf(tgtUID,
                                "Trigger: %s, VNum %ld. local var '%s' not found in remote call",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), tgtVar);
                        script_log(tgtUID);
                        release_buffer(tgtVar);
                        release_buffer(tgtUID);
                        release_buffer(arg);
                        return;
                        }


                     /* Strip the UID prefix */
                     if (*uid_p == UID_CHAR)
                        uid_p++;

                     /* If the UID is 0 or negative, it is an error */
                     uid = atoi(uid_p);
                     if (uid<=0)
                        {
                        sprintf(tgtVar, "Trigger: %s, VNum %ld. remote: illegal uid '%s'",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), tgtUID);
                        script_log(tgtVar);
                        release_buffer(tgtVar);
                        release_buffer(tgtUID);
                        release_buffer(arg);
                        return;
                        }

                     /* Find the target script */
                     /* Does the UID refer to a room? */
                     if ((room = find_room(uid)))
                        {
                        sc_remote = SCRIPT(room);
                        }
                     /* If not, does it refer to a character? */
                     else if ((mob = find_char(uid)))
                        {
                        sc_remote = SCRIPT(mob);
                        if(!IS_NPC(mob))
                           context=0;
                        }
                     /* If not, does it refer to an object? */
                     else if ((obj = find_obj_dg(uid)))
                        {
                        sc_remote = SCRIPT(obj);
                        }
                     /* None of the above, it is an error */
                     else
                        {
                        sprintf(tgtVar, "Trigger: %s, VNum %ld. remote: uid '%ld' invalid",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), uid);
                        script_log(tgtVar);
                        release_buffer(tgtVar);
                        release_buffer(tgtUID);
                        release_buffer(arg);
                        return;
                        }

                     /* If there are no triggers attached to this UID, ignore */
                     if (sc_remote==NULL)
                        {
                        release_buffer(tgtVar);
                        release_buffer(tgtUID);
                        release_buffer(arg);
                        return;
                        }

                     /* All is fine, add the variable to the remote script */
                     add_var(&(sc_remote->global_vars), vd->name, vd->value, vd->context);
                     release_buffer(tgtVar);
                     release_buffer(tgtUID);
                     release_buffer(arg);
                     }


                  /*
                   * command-line interface to rdelete
                   * named vdelete so people didn't think it was to delete rooms
                   */
                  ACMD(do_vdelete)
                     {
                     struct trig_var_data *vd, *vd_prev=NULL;
                     struct script_data *sc_remote=NULL;
                     char *var, *uid_p;
                     long uid, context;
                     room_data *room;
                     char_data *mob;
                     obj_data *obj;
                     char *buf = get_buffer(512);
                     char *buf2 = get_buffer(512);

                     argument = two_arguments(argument, buf, buf2);
                     var = buf;
                     uid_p = buf2;
                     skip_spaces(&var);
                     skip_spaces(&uid_p);


                     if (!*buf || !*buf2)
                        {
                        send_to_char(ch,"Usage: vdelete <variablename> <id>\r\n");
                        release_buffer(buf2);
                        release_buffer(buf);
                        return;
                        }


                     /* find the target script from the uid number */
                     uid = atoi(buf2);
                     if (uid<=0)
                        {
                        send_to_char(ch,"vdelete: illegal id specified.\r\n");
                        release_buffer(buf2);
                        release_buffer(buf);
                        return;
                        }
                     release_buffer(buf);
                     release_buffer(buf2);


                     if ((room = find_room(uid)))
                        {
                        sc_remote = SCRIPT(room);
                        }
                     else if ((mob = find_char(uid)))
                        {
                        sc_remote = SCRIPT(mob);
                        if (!IS_NPC(mob))
                           context = 0;
                        }
                     else if ((obj = find_obj_dg(uid)))
                        {
                        sc_remote = SCRIPT(obj);
                        }
                     else
                        {
                        send_to_char(ch,"vdelete: cannot resolve specified id.\r\n");
                        return;
                        }

                     if ((sc_remote==NULL) || (sc_remote->global_vars==NULL))
                        {
                        send_to_char(ch,"That id represents no global variables.\r\n");
                        return;
                        }

                     /* find the global */
                     for (vd = sc_remote->global_vars; vd; vd_prev = vd, vd = vd->next)
                        if (!str_cmp(vd->name, var))
                           break;

                     if (!vd)
                        {
                        send_to_char(ch,"That variable cannot be located.\r\n");
                        return;
                        }

                     /* ok, delete the variable */
                     if (vd_prev)
                        vd_prev->next = vd->next;
                     else
                        sc_remote->global_vars = vd->next;

                     /* and free up the space */
                     free(vd->value);
                     free(vd->name);
                     free(vd);

                     send_to_char(ch,"Deleted.\r\n");
                     }

                  /*
                   * delete a variable from the globals of another script
                   *     'rdelete <variable_name> <uid>'
                   */
                  void process_rdelete(struct script_data *sc, trig_data *trig, char *cmd)
                     {
                     struct trig_var_data *vd, *vd_prev=NULL;
                     struct script_data *sc_remote=NULL;
                     char *line, *var, *uid_p;
                     long uid, context;
                     room_data *room;
                     char_data *mob;
                     obj_data *obj;
                     char *arg = get_buffer(MAX_INPUT_LENGTH);
                     char *buf = get_buffer(MAX_INPUT_LENGTH);
                     char *buf2 = get_buffer(MAX_INPUT_LENGTH);

                     line = any_one_arg(cmd, arg);
                     two_arguments(line, buf, buf2);
                     var = buf;
                     uid_p = buf2;
                     skip_spaces(&var);
                     skip_spaces(&uid_p);


                     if (!*buf || !*buf2)
                        {
                        sprintf(buf2, "Trigger: %s, VNum %ld. rdelete: invalid arguments '%s'",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
                        script_log(buf2);
                        release_buffer(buf2);
                        release_buffer(buf);
                        release_buffer(arg);
                        return;
                        }


                     /* find the target script from the uid number */
                     uid = atoi(buf2);
                     if (uid<=0)
                        {
                        sprintf(buf, "Trigger: %s, VNum %ld. rdelete: illegal uid '%s'",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), buf2);
                        script_log(buf);
                        release_buffer(buf2);
                        release_buffer(buf);
                        release_buffer(arg);
                        return;
                        }


                     if ((room = find_room(uid)))
                        {
                        sc_remote = SCRIPT(room);
                        }
                     else if ((mob = find_char(uid)))
                        {
                        sc_remote = SCRIPT(mob);
                        if (!IS_NPC(mob))
                           context = 0;
                        }
                     else if ((obj = find_obj_dg(uid)))
                        {
                        sc_remote = SCRIPT(obj);
                        }
                     else
                        {
                        sprintf(buf, "Trigger: %s, VNum %ld. remote: uid '%ld' invalid",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), uid);
                        script_log(buf);
                        release_buffer(buf2);
                        release_buffer(buf);
                        release_buffer(arg);
                        return;
                        }

                     if (sc_remote==NULL)
                        {
                        release_buffer(buf2);
                        release_buffer(buf);
                        release_buffer(arg);
                        return; /* no script to delete a trigger from */
                        }
                     if (sc_remote->global_vars==NULL)
                        {
                        release_buffer(buf2);
                        release_buffer(buf);
                        release_buffer(arg);
                        return; /* no script globals */
                        }

                     /* find the global */
                     for (vd = sc_remote->global_vars; vd; vd_prev = vd, vd = vd->next)
                        if (!str_cmp(vd->name,var)&&(vd->context==0||vd->context==sc->context))
                           break;

                     if (!vd)
                        {
                        release_buffer(buf2);
                        release_buffer(buf);
                        release_buffer(arg);
                        return; /* the variable doesn't exist, or is the wrong context */
                        }
                     /* ok, delete the variable */
                     if (vd_prev)
                        vd_prev->next = vd->next;
                     else
                        sc_remote->global_vars = vd->next;

                     /* and free up the space */
                     free(vd->value);
                     free(vd->name);
                     free(vd);
                     release_buffer(buf2);
                     release_buffer(buf);
                     release_buffer(arg);
                     }



                  /*
                   * makes a local variable into a global variable
                   */
                  void process_global(struct script_data *sc, trig_data *trig, char *cmd,long id)
                     {
                     struct trig_var_data *vd;
                     char *arg=get_buffer(MAX_INPUT_LENGTH);
                     char *var;

                     var = any_one_arg(cmd, arg);

                     skip_spaces(&var);

                     if (!*var)
                        {
                        char *buf2=get_buffer(512);
                        sprintf(buf2, "Trigger: %s, VNum %ld. global w/o an arg: '%s'",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
                        script_log(buf2);
                        release_buffer(buf2);
                        release_buffer(arg);
                        return;
                        }

                     for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
                        if (!str_cmp(vd->name, var))
                           break;

                     if (!vd)
                        {
                        char *buf2=get_buffer(512);
                        sprintf(buf2, "Trigger: %s, VNum %ld. local var '%s' not found in global call",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), var);
                        script_log(buf2);
                        release_buffer(buf2);
                        release_buffer(arg);
                        return;
                        }

                     add_var(&(sc->global_vars), vd->name, vd->value,id);
                     remove_var(&GET_TRIG_VARS(trig), vd->name,id);
                     release_buffer(arg);
                     }



                  /* set the current context for a script */

                  void process_context(struct script_data *sc, trig_data *trig, char *cmd)
                     {
                     char *arg=get_buffer(MAX_INPUT_LENGTH);
                     char *var;
                     char *buf2=get_buffer(MAX_INPUT_LENGTH);
                     /* MANWE: Added to hold the return value from atol() */
                     long value = 0;

                     var = any_one_arg(cmd, arg);

                     skip_spaces(&var);

                     if (!*var)
                        {
                        sprintf(buf2, "Trigger: %s, VNum %ld. context w/o an arg: '%s'",
                                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
                        script_log(buf2);
                        release_buffer(buf2);
                        release_buffer(arg);
                        return;
                        }

                     /* MANWE: Handle UID type value by stripping its leading 0x1b */
                     if (*var == 0x1b)
                        var++;

                     /* MANWE: Need to handle a return of 0 from atol() when either the value to*/
                     /*        convert is 0, or could not be converted. */

                     if ((!strcmp(var,"0") || (value = atol(var))) && (*var != '-'))
                        {
                        /* Changed to use 'value', now that it is computed properly */
                        sc->context = value;
                        }
                     /* MANWE: Added the 'else' below to warn of incorrect parameter */
                     else
                        {
                        sprintf(buf2,"Trigger: %s, VNum %ld. Context requires a "
                                "positive number: '%s'", GET_TRIG_NAME(trig),
                                GET_TRIG_VNUM(trig), cmd);
                        script_log(buf2);
                        }
                     release_buffer(buf2);
                     release_buffer(arg);
                     }

                  void extract_value(struct script_data *sc, trig_data *trig, char *cmd)
                     {
                     char *buf=get_buffer(MAX_INPUT_LENGTH);
                     char *buf2=get_buffer(MAX_INPUT_LENGTH);
                     char *buf3;
                     char *to=get_buffer(128);
                     int num;

                     buf3 = any_one_arg(cmd, buf);
                     half_chop(buf3, buf2, buf);
                     strcpy(to, buf2);

                     num = atoi(buf);
                     if (num < 1)
                        {
                        script_log("extract number < 1!");
                        release_buffer(to);
                        release_buffer(buf2);
                        release_buffer(buf);
                        return;
                        }

                     half_chop(buf, buf3, buf2);

                     while (num>0)
                        {
                        half_chop(buf2, buf, buf2);
                        num--;
                        }

                     add_var(&GET_TRIG_VARS(trig), to, buf, sc->context);
                     release_buffer(to);
                     release_buffer(buf2);
                     release_buffer(buf);
                     }

                  int dg_owner_purged;


/*  This is the core driver for scripts. */
int script_driver(void *go, trig_data *trig, int type, int mode)
   {
   static int depth = 0;
   int ret_val = 1;
   struct cmdlist_element *cl;
   char *cmd=get_buffer(MAX_INPUT_LENGTH);
   char *p;
   struct script_data *sc = 0;
   struct cmdlist_element *temp;
   unsigned long loops = 0;

   if (depth > MAX_SCRIPT_DEPTH)
      {
      script_log("Triggers recursed beyond maximum allowed depth (10).");
      release_buffer(cmd);
      return ret_val;
      }

   depth++;

   switch (type)
      {
   case MOB_TRIGGER:
      sc = SCRIPT((char_data *) go);
      break;
   case OBJ_TRIGGER:
      sc = SCRIPT((obj_data *) go);
      break;
   case WLD_TRIGGER:
      sc = SCRIPT((struct room_data *) go);
      break;
      }

   if (mode == TRIG_NEW)
      {
      GET_TRIG_DEPTH(trig) = 1;
      GET_TRIG_LOOPS(trig) = 0;
      sc->context=0;
      }
   dg_owner_purged =0;

   for (cl = (mode == TRIG_NEW) ? trig->cmdlist : trig->curr_state;
           cl && GET_TRIG_DEPTH(trig); cl = cl->next)
      {
      for (p = cl->cmd; *p && isspace((int)*p); p++)
         ;

      if (*p == '*')  /* script comment, next line */
         continue;

      else if (!strn_cmp(p, "if ", 3))
         {
         if (process_if(p + 3, go, sc, trig, type))
            GET_TRIG_DEPTH(trig)++;
         else
            cl = find_else_end(trig, cl, go, sc, type);
         }

      else if (!strn_cmp("elseif ", p, 7) || !strn_cmp("else", p, 4))
         {
         cl = find_end(cl);
         GET_TRIG_DEPTH(trig)--;
         }
      else if (!strn_cmp("while ", p, 6))
         {
         temp = find_done(cl);
         if (process_if(p + 6, go, sc, trig, type))
            {
            temp->original = cl;
            }
         else
            {
            cl = temp;
            loops = 0;
            }
         }
      else if (!strn_cmp("switch ", p, 7))
         {
         cl = find_case(trig, cl, go, sc, type, p + 7);
         }
      else if (!strn_cmp("end", p, 3))
         {
         GET_TRIG_DEPTH(trig)--;
         }
      else if (!strn_cmp("done", p, 4))
         {
         char *orig_cmd = "";
         if (cl->original)
            orig_cmd = cl->original->cmd;

         while (*orig_cmd && isspace(*orig_cmd))
            orig_cmd++;

         if (cl->original && process_if(orig_cmd + 6, go, sc, trig, type))
            {
            cl = cl->original;
            loops++;
            GET_TRIG_LOOPS(trig)++;
            if ((loops % 50) == 0)
               {
               process_wait(go, trig, type, "wait 1", cl);
               depth--;
               release_buffer(cmd);
               return ret_val;
               }
            if (GET_TRIG_LOOPS(trig) == 500)
               {
               mudlogf(NRM, LVL_IMMORT, TRUE,
                       "SCRIPT ERR: Trigger VNum %ld has looped 500 times!!!",
                       GET_TRIG_VNUM(trig));
               }
            }
         }
      else if (!strn_cmp("break", p, 5))
         {
         cl = find_done(cl);
         }
      else if (!strn_cmp("case", p, 4))
         {
         /* Do nothing, this allows multiple cases to a single instance */
         }


      else
         {

         var_subst(go, sc, trig, type, p, cmd);

         if (!strn_cmp(cmd, "eval ", 5))
            process_eval(go, sc, trig, type, cmd);

         else if (!strn_cmp(cmd, "nop ", 4))
            ; /* nop: do nothing */

         else if (!strn_cmp(cmd, "halt", 4))
            break;

         else if (!strn_cmp(cmd, "dg_cast ", 8))
            do_dg_cast(go, sc, trig, type, cmd);

         else if (!strn_cmp(cmd, "dg_affect ", 10))
            do_dg_affect(go, sc, trig, type, cmd);

         else if (!strn_cmp(cmd, "dg_log ", 7))
            {
            /* Silent if log is preceded by - */
            if (*(cmd+7) == '-')
               log("Script-Log: (%s,%ld) %s", GET_TRIG_NAME(trig), 
                   GET_TRIG_VNUM(trig), cmd+7);
            else
               mudlogf(CMP, LVL_IMMORT, TRUE, "Script-Log: (%s,%ld) %s",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd+7);
            }

         else if (!strn_cmp(cmd, "dg_unaffect ", 12) &&
                  (type == MOB_TRIGGER))
            do_dg_unaffect((struct char_data *)go, trig, cmd+12);

         else if (!strn_cmp(cmd, "extract ", 8))
            extract_value(sc, trig, cmd);

         else if (!strn_cmp(cmd, "makeuid ", 8))
            makeuid_var(go, sc, trig, type, cmd);

         else if (!strn_cmp(cmd, "global ", 7))
            process_global(sc, trig, cmd,sc->context);

         else if (!strn_cmp(cmd, "context ", 8))
            process_context(sc, trig, cmd);

         else if (!strn_cmp(cmd, "remote ", 7))
            process_remote(sc, trig, cmd);

         else if (!strn_cmp(cmd, "rdelete ", 8))
            process_rdelete(sc, trig, cmd);

         else if (!strn_cmp(cmd, "return ", 7))
            ret_val = process_return(trig, cmd);

         else if (!strn_cmp(cmd, "set ", 4))
            process_set(sc, trig, cmd);

         else if (!strn_cmp(cmd, "unset ", 6))
            process_unset(go, sc, trig, type, cmd);

	 else if (!strn_cmp(cmd, "hold ", 5))
	   process_hold(sc, trig, cmd);

         else if (!strn_cmp(cmd, "wait ", 5))
            {
            process_wait(go, trig, type, cmd, cl);
            depth--;
            release_buffer(cmd);
            return ret_val;
            }

         else if (!strn_cmp(cmd, "attach ", 7))
            process_attach(go, sc, trig, type, cmd);

         else if (!strn_cmp(cmd, "detach ", 7))
            process_detach(go, sc, trig, type, cmd);

	 else if (!strn_cmp(cmd, "zreset", 6))
	   process_zreset(go, sc, trig, type, cmd);

         else if (!strn_cmp(cmd, "version", 7))
            mudlog(circlemud_version, NRM, LVL_GOD, TRUE);

         else
            {
            switch (type)
               {
            case MOB_TRIGGER:
               command_interpreter((char_data *) go, cmd);
               break;
            case OBJ_TRIGGER:
               obj_command_interpreter((obj_data *) go, cmd);
               break;
            case WLD_TRIGGER:
               wld_command_interpreter((struct room_data *) go, cmd);
               break;
               }

            if(dg_owner_purged)
               {
               depth--;
               release_buffer(cmd);
               return ret_val;
               }
            }
         }
      }

   free_varlist(GET_TRIG_VARS(trig));
   GET_TRIG_VARS(trig) = NULL;
   GET_TRIG_DEPTH(trig) = 0;

   depth--;
   release_buffer(cmd);
   return ret_val;
   }

/* Received argument contains the parameters passed to this function
 
   Command                                           argument
 
   tlist                                        ""
   tlist <start_vnum>                           " <start_vnum>"
   tlist <start_vnum> <stop_vnum>               " <start_vnum> <stop_vnum>"
   tlist -[o|m|w]                               " -[o|m|w]"
   tlist -[o|m|w] <start_vnum>                  " -[o|m|w] <start_vnum>"
   tlist -[o|m|w] <start_vnum> <stop_vnum>      " -[o|m|w] <start_vnum>
<stop_vnum>"
*/
ACMD(do_tlist)
   {

   const int ALL_TRIGGERS = -1;          /* Selector for all triggers */

   long int top_trig_vnum;                               /* Highest trigger
                vnum available */
   int   first;                          /* Vnum of starting trigger */
   int   last;                           /* Vnum of ending trigger   */
   int   nr;                                                     /* Loop
                 counter */
   int   cselect;                        /* Color selector */
   int   tselect = ALL_TRIGGERS;         /* Type to be used for display */
   int   found = 0;                      /* Boolean */
   char  input_switch;                   /* Holds the entered switch */
   char *pagebuf = get_buffer(65536);    /* Holds data to be displayed */
   char *buf = get_buffer(256);          /* Holds the first vnum entered */
   char *buf2 = get_buffer(256);                 /* Holds the second vnum
               entered */
   char *trig_tgt = get_buffer(256);     /* Translate attach_type to string
            */
   char *trig_type = get_buffer(256);    /* Translate trig type to string */
   char *remaining;                                              /* Points
     the rest of the arguments */

   /* Clear all buffers */
   *pagebuf = '\0';
   *buf = '\0';
   *buf2 = '\0';
   *trig_tgt = '\0';
   *trig_type = '\0';

   /* For out of range determinations */
   top_trig_vnum = trig_index[top_of_trigt - 1]->vnum;

   remaining = two_arguments(argument, buf, buf2);

   /* No arguments given */
   if (!*buf)
      {
      send_to_char(ch, "%sUsage: tlist [-<[o|m|w]>] <begining trigger number> [<ending trigger number>]%s\r\n\r\n",
                   CCRED(ch, C_NRM),
                   CCNRM(ch, C_NRM));
      release_buffer(pagebuf);
      release_buffer(buf);
      release_buffer(buf2);
      release_buffer(trig_tgt);
      release_buffer(trig_type);
      return;
      }

   /* If so, buf2 should hold <start_vnum> and remaining may hold
   <stop_vnum> */
   if (*buf == '-')
      {
      switch (*(buf+1))
         {
      case 'o':
         tselect = OBJ_TRIGGER;
         break;
      case 'w':
         tselect = WLD_TRIGGER;
         break;
      case 'm':
         tselect = MOB_TRIGGER;
         break;
      default:
         input_switch = *(buf+1);
         send_to_char(ch, "%sAllowed switches are -o (object) -w (world), and -m (mobs) \r\n"
                      "Switch -%c not recognized - defaulting to full report.%s\r\n\r\n",
                      CCRED(ch, C_NRM),
                      input_switch,
                      CCNRM(ch, C_NRM));
         break;
         }
      /* Check <start_vnum> - Must be a positive number */
      if ((*buf2 != '0') && ((first = atoi(buf2)) == 0))
         {
         send_to_char(ch, "%sYou must use a trigger vnum between 0 and %ld as your first parameter. (ex. tlist -o 1000)%s\r\n\r\n",
                      CCRED(ch, C_NRM),
                      top_trig_vnum -1,
                      CCNRM(ch, C_NRM));
         release_buffer(pagebuf);
         release_buffer(buf);
         release_buffer(buf2);
         release_buffer(trig_tgt);
         release_buffer(trig_type);
         return;
         }
      /* Check <stop_number>, if any */
      if (*remaining)
         {
         if ((*remaining != '0') && ((last = atoi(remaining)) == 0))
            {
            send_to_char(ch, "%sIf present, the last parameter must be a trigger number less or equal to %ld . (ex. tlist -o 1000 1050)%s\r\n\r\n",
                         CCRED(ch, C_NRM),
                         top_trig_vnum,
                         CCNRM(ch, C_NRM));
            release_buffer(pagebuf);
            release_buffer(buf);
            release_buffer(buf2);
            release_buffer(trig_tgt);
            release_buffer(trig_type);
            return;
            }
         }
      else
         last = first + 99;
      }

   /* Otherwise, buf should hold <start_vnum> and buf2 may hold <stop_vnum>
   */
   else
      {
      if ((*buf != '0') && ((first = atoi(buf)) == 0))
         {
         send_to_char(ch, "%sYou must use a trigger vnum between 0 and %ld as your first parameter. (ex. tlist 1000)%s\r\n\r\n",
                      CCRED(ch, C_NRM),
                      top_trig_vnum - 1,
                      CCNRM(ch, C_NRM));
         release_buffer(pagebuf);
         release_buffer(buf);
         release_buffer(buf2);
         release_buffer(trig_tgt);
         release_buffer(trig_type);
         return;
         }
      if (*buf2)
         {
         if ((*buf2 != '0') && ((last = atoi(buf2)) == 0))
            {
            send_to_char(ch, "%sIf present, the last parameter must be a trigger number less or equal to %ld . (ex. tlist 1000 1050)%s\r\n\r\n",
                         CCRED(ch, C_NRM),
                         top_trig_vnum,
                         CCNRM(ch, C_NRM));
            release_buffer(pagebuf);
            release_buffer(buf);
            release_buffer(buf2);
            release_buffer(trig_tgt);
            release_buffer(trig_type);
            return;
            }
         }
      else
         last = first + 99;
      }

   /* Adjust for top of trigger and report if out of range*/
   if ((first >= 0) && (first < top_trig_vnum))
      {
      if (last > top_trig_vnum)
         last = top_trig_vnum;
      }
   else
      {
      send_to_char(ch, "%sStart value must be between 0 and %ld\r\n"
                   "Stop value must be higher than the start value and not evaluate to more than %ld\r\n"
                   "Remember that the stop value defaults to <start value> + 99, which in this case is %d%s\r\n\r\n",
                   CCRED(ch, C_NRM),
                   top_trig_vnum - 1,
                   top_trig_vnum,
                   last,
                   CCNRM(ch, C_NRM));
      release_buffer(pagebuf);
      release_buffer(buf);
      release_buffer(buf2);
      release_buffer(trig_tgt);
      release_buffer(trig_type);
      return;
      }

   /* Invalid order */
   if (first >= last)
      {
      send_to_char(ch, "%sSecond value must be greater than first.%s\r\n\r\n",
                   CCRED(ch, C_NRM),
                   CCNRM(ch, C_NRM));
      release_buffer(pagebuf);
      release_buffer(buf);
      release_buffer(buf2);
      release_buffer(trig_tgt);
      release_buffer(trig_type);
      return;
      }

   /* Build the page with data from selected triggers */
   switch (tselect)
      {
   case MOB_TRIGGER:
      strcpy(buf, "(Mobile triggers only)");
      break;
   case WLD_TRIGGER:
      strcpy(buf, "(World triggers only)");
      break;
   case OBJ_TRIGGER:
      strcpy(buf, "(Object triggers only)");
      break;
   default:
      strcpy(buf, "(All triggers)");
      break;
      }
   sprintf(pagebuf, "%s**** Trigger list from %5d to %5d - %s ****%s\r\n\r\n",
           CCYEL(ch, C_NRM),
           first,
           last,
           buf,
           CCNRM(ch, C_NRM));

   /* Go look for the requested triggers */
   for (nr=0; nr < top_of_trigt && (trig_index[nr]->vnum <= last); nr++)
      {
	if (GET_LEVEL(ch) < TLIST_LEVEL && !is_olc_set(ch, trig_index[nr]->vnum/100)) {
	  continue;
	}
	
	
      if (trig_index[nr]->vnum >= first)
         {
         switch (trig_index[nr]->proto->attach_type)
            {
         case OBJ_TRIGGER:
            cselect = OBJ_TRIGGER;
            strcpy(trig_tgt, "OBJ");
            sprintbit(GET_TRIG_TYPE(trig_index[nr]->proto),
                      otrig_types, trig_type);
            break;
         case WLD_TRIGGER:
            cselect = WLD_TRIGGER;
            strcpy(trig_tgt, "WLD");
            sprintbit(GET_TRIG_TYPE(trig_index[nr]->proto),
                      wtrig_types, trig_type);
            break;
         default:
            cselect = MOB_TRIGGER;
            strcpy(trig_tgt, "MOB");
            sprintbit(GET_TRIG_TYPE(trig_index[nr]->proto),
                      trig_types, trig_type);
            break;
            }
         if ((tselect == ALL_TRIGGERS) || (tselect == cselect))
            sprintf(pagebuf+strlen(pagebuf), "%s%3d. [%5ld] [%s] %-30s -- %s%s\r\n",
                    (cselect==OBJ_TRIGGER) ? CCMAG(ch, C_NRM) :
                    ((cselect==WLD_TRIGGER) ? CCCYN(ch, C_NRM):
                     CCYEL(ch, C_NRM)),
                    ++found,
                    trig_index[nr]->vnum,
                    trig_tgt,
                    trig_index[nr]->proto->name,
                    trig_type,
                    CCNRM(ch, C_NRM));
         }
      }


   if (!found)
      {
      switch (tselect)
         {
      case MOB_TRIGGER:
         strcpy(buf, " Mobile ");
         break;
      case WLD_TRIGGER:
         strcpy(buf, " World ");
         break;
      case OBJ_TRIGGER:
         strcpy(buf, " Object ");
         break;
      default:
         strcpy(buf, " ");
         break;
         }
      send_to_char(ch, "%sThere were no%striggers found in the range %5d to %5d %s.\r\n\r\n",
                   CCYEL(ch, C_NRM),
                   buf,
                   first,
                   last,
                   CCNRM(ch, C_NRM));
      }
   else
      page_string(ch->desc, pagebuf, TRUE,"");

   release_buffer(pagebuf);
   release_buffer(buf);
   release_buffer(buf2);
   release_buffer(trig_tgt);
   release_buffer(trig_type);
   }

int real_trigger(int vnum)
   {
   int rnum;

   for (rnum=0; rnum < top_of_trigt; rnum++)
      {
      if (trig_index[rnum]->vnum==vnum)
         break;
      }

   if (rnum==top_of_trigt)
      rnum = -1;
   return (rnum);
   }

ACMD(do_tstat)
   {
   int vnum, rnum;
   char *str=get_buffer(MAX_INPUT_LENGTH);

   half_chop(argument, str, argument);
   if (*str)
      {
      vnum = atoi(str);
      rnum = real_trigger(vnum);
      if (rnum<0)
         {
         send_to_char(ch,"That vnum does not exist.\r\n");
         release_buffer(str);
         return;
         }

      do_stat_trigger(ch, trig_index[rnum]->proto);
      valid_script(trig_index[rnum]->proto, NULL);
      }
   else
      send_to_char(ch,"Usage: tstat <vnum>\r\n");
   release_buffer(str);
   }

/*
* scans for a case/default instance
* returns the line containg the correct case instance, or the last
* line of the trigger if not found.
*/
struct cmdlist_element *
         find_case(struct trig_data *trig, struct cmdlist_element *cl,
                   void *go, struct script_data *sc, int type, char *cond)
   {
   struct cmdlist_element *c;
   char *p;
   char *result;

   if (!(cl->next))
      return cl;
   result = get_buffer(MAX_INPUT_LENGTH);
   eval_expr(cond, result, go, sc, trig, type);

   for (c = cl->next; c->next; c = c->next)
      {
      for (p = c->cmd; *p && isspace((int)*p); p++)
         ;
      if (!strn_cmp("while ", p, 6) || !strn_cmp("switch", p, 6))
         c = find_done(c);
      else if (!strn_cmp("case ", p, 5))
         {
         char *buf = get_buffer(MAX_STRING_LENGTH);
#if 0

         sprintf(buf, "(%s) == (%s)", cond, p + 5);
         if (process_if(buf, go, sc, trig, type))
#else

            eval_op("==", result, p + 5, buf, go, sc, trig);
         if (*buf && *buf!='0')
#endif

            {
            release_buffer(buf);
            release_buffer(result);
            return c;
            }
         release_buffer(buf);
         }
      else if (!strn_cmp("default", p, 7))
         {
         release_buffer(result);
         return c;
         }
      else if (!strn_cmp("done", p, 3))
         {
         release_buffer(result);
         return c;
         }
      }
   release_buffer(result);
   return c;
   }

/*
* scans for end of while/switch-blocks.
* returns the line containg 'end', or the last
* line of the trigger if not found.
*/
struct cmdlist_element * find_done(struct cmdlist_element *cl)
   {
   struct cmdlist_element *c;
   char *p;

   if (!(cl->next))
      return cl;

   for (c = cl->next; c->next; c = c->next)
      {
      for (p = c->cmd; *p && isspace((int)*p); p++)
         ;

      if (!strn_cmp("while ", p, 6) || !strn_cmp("switch ", p, 7))
         c = find_done(c);
      else if (!strn_cmp("done", p, 3))
         return c;
      }

   return c;
   }

/* read a line in from a file, return the number of chars read */
int fgetline(FILE *file, char *p)
   {
   int count = 0;

   do
      {
      *p = fgetc(file);
      if (*p != '\n' && !feof(file))
         {
         p++;
         count++;
         }
      }
   while (*p != '\n' && !feof(file))
      ;

   if (*p == '\n')
      *p = '\0';

   return count;
   }


/* load in a character's saved variables */
void read_saved_vars(struct char_data *ch)
   {
   FILE *file;
   long context;
   char fn[127];
   char input_line[1024], *p;
   char varname[32], *v;
   char context_str[16], *c;

   /* create the space for the script structure which holds the vars */
   CREATE(SCRIPT(ch), struct script_data, 1);

   /* find the file that holds the saved variables and open it*/
   if (!get_filename(GET_NAME(ch),fn,SCRIPT_VARS_FILE))
      return;

   file = fopen(fn,"r");

   /* if we failed to open the file, return */
   if( !file )
      return;

   /* walk through each line in the file parsing variables */
   do
      {
      if (fgetline(file, input_line)>0)
         {
         p = input_line;
         v = varname;
         c = context_str;
         skip_spaces(&p);
         while (*p && *p != ' ' && *p != '\t')
            *v++ = *p++;
         *v = '\0';
         skip_spaces(&p);
         while (*p && *p != ' ' && *p != '\t')
            *c++ = *p++;
         *c = '\0';
         skip_spaces(&p);

         context = atol(context_str);
         add_var(&(SCRIPT(ch)->global_vars), varname, p, context);
         }
      }
   while( !feof(file) )
      ;

   /* close the file and return */
   fclose(file);
   }

/* save a characters variables out to disk */
void save_char_vars(struct char_data *ch)
   {
   FILE *file;
   char fn[127];
   struct trig_var_data *vars;

   /* immediate return if no script (and therefore no variables) structure */
   /* has been created. this will happen when the player is logging in */
   if (SCRIPT(ch) == NULL)
      return;

   /* we should never be called for an NPC, but just in case... */
   if (IS_NPC(ch))
      return;

   if (!get_filename(GET_NAME(ch),fn,SCRIPT_VARS_FILE))
      {
      log("get_filename failed in save_char_vars");
      return;
      }
   unlink(fn);

   /* make sure this char has global variables to save */
   if (ch->script->global_vars == NULL)
      return;

   vars = ch->script->global_vars;

   file = fopen(fn,"wt");

   if (!file)
      {
      log("SYSERR: fopen failed in save_char_vars: %s", fn);
      return;
      }

   /* note that currently, context will always be zero. this may change */
   /* in the future */
   while (vars)
      {
      if (*vars->name != '-') /* don't save if it begins with - */
         fprintf(file, "%s %ld %s\n", vars->name, vars->context, vars->value);
      vars = vars->next;
      }

   fclose(file);
   }
#define CHECK_IF 1
#define CHECK_WHILE 2
#define CHECK_SWITCH 3
#define CHECK_CASE 4
char *checkWords[]={"NOTHING","if","while","switch","case"};

void sendValidScriptOutput(char *line,int position,struct char_data *ch,
                           int isError)
   {
   char *buf;
   int i;
   if(ch==NULL)
      return;
   buf = get_buffer(256);
   for(i=0;i<=position;i++)
      {
      strcat(buf," ");
      }
   send_to_char(ch,"%s%s%s\r\n",buf,line,isError?" &r(ERROR)&n":"");
   release_buffer(buf);
   }

int valid_script(trig_data *trig, struct char_data *ch)
   {
   struct cmdlist_element *c;
   int levels[50];
   int position=0;
   int extraDone=0;
   int extraEnd=0;
   int extraDefault=0;
   int i;
   char *p;

   for(i=0;i<50;i++)
      levels[i]=0;

   position++;

   sendValidScriptOutput("SCRIPT LOG",0,ch,FALSE);

   for(c = trig->cmdlist; c; c = c->next)
      {
      for(p = c->cmd; p && isspace((int)*p); p++)
         ;

      if(!strn_cmp("if ", p, 3))
         {
         sendValidScriptOutput(p,position,ch,FALSE);
         levels[position] = CHECK_IF;
         position++;
         }
      else if(!strn_cmp("while ", p, 6))
         {
         sendValidScriptOutput(p,position,ch,FALSE);
         levels[position] = CHECK_WHILE;
         position++;
         }
      else if(!strn_cmp("switch ", p, 7))
         {
         sendValidScriptOutput(p,position,ch,FALSE);
         levels[position] = CHECK_SWITCH;
         position++;
         }
      else if(!strn_cmp("case", p, 4))
         {
         if(levels[position-1]==CHECK_SWITCH)
            {
            sendValidScriptOutput(p,position,ch,FALSE);
            /*levels[position] = CHECK_CASE;
             position++;*/
            }
         else
            {
            sendValidScriptOutput(p,position,ch,TRUE);
            }
         }
      else if(!strn_cmp("end",p,3))
         {
         if(levels[position-1]==CHECK_IF)
            {
            position--;
            levels[position] = 0;
            sendValidScriptOutput(p,position,ch,FALSE);
            }
         else
            {
            sendValidScriptOutput(p,position,ch,TRUE);
            extraEnd++;
            }
         }
      else if(!strn_cmp("done",p,4))
         {
         if((levels[position-1] == CHECK_WHILE) ||
                 (levels[position-1] == CHECK_SWITCH))
            {
            position--;
            levels[position] = 0;
            sendValidScriptOutput(p,position,ch,FALSE);
            }
         else
            {
            sendValidScriptOutput(p,position,ch,TRUE);
            extraDone++;
            }
         }
      else if(!strn_cmp("default",p,7))
         {
         if(levels[position-1] == CHECK_SWITCH)
            {
            /*position--;
             levels[position] = 0;*/
            sendValidScriptOutput(p,position,ch,FALSE);
            }
         else
            {
            sendValidScriptOutput(p,position,ch,TRUE);
            extraDefault++;
            }
         }
      else if(!strn_cmp("else", p, 4))
         {
         if(levels[position-1]==CHECK_IF)
            sendValidScriptOutput(p,position-1,ch,FALSE);
         else
            sendValidScriptOutput(p,position-1,ch,TRUE);
         }
      else if(!strn_cmp("elseif", p, 6))
         {
         if(levels[position-1]==CHECK_IF)
            sendValidScriptOutput(p,position-1,ch,FALSE);
         else
            sendValidScriptOutput(p,position-1,ch,TRUE);
         }

      }
   position--;
   if(position)
      {
      char *buf = get_buffer(512);
      for(i=0;i<position;i++)
         {
         sprintf(buf+strlen(buf),"%s -> ",checkWords[levels[i]]);
         }
      if(ch == NULL)
         mudlogf(NRM,LVL_IMMORT,TRUE,
                 "SCRIPT ERR: Trigger Vnum %ld has a missing loop/if/case "
                 "terminator.  Debug chain: %s extraEnd: %d  extraDone: %d  extraDefault: %d",GET_TRIG_VNUM(trig),buf,extraEnd, extraDone,extraDefault);
      else
         send_to_char(ch,"SCRIPT ERR: Trigger Vnum %ld has a missing "
                      "loop/if/case terminator.  \r\nDebug chain: %s"
                      "extraEnd: %d  extraDone: %d  extraDefault: %d\r\n",
                      GET_TRIG_VNUM(trig),buf,extraEnd, extraDone,
                      extraDefault);
      release_buffer(buf);
      return FALSE;
      }
   return TRUE;
   }


ACMD(do_tcheck)
   {
   int vnum, rnum;
   char *str=get_buffer(MAX_INPUT_LENGTH);

   half_chop(argument, str, argument);
   if (*str)
      {
      vnum = atoi(str);

      if (GET_LEVEL(ch) < TCHECK_LEVEL && !is_olc_set(ch, vnum/100)) {
	send_to_char(ch, "You do not have permissions to tcheck that trigger.\r\n");
	release_buffer(str);
	return;
      }

      rnum = real_trigger(vnum);
      if (rnum<0)
         {
         send_to_char(ch,"That trigger does not exist.\r\n");
         release_buffer(str);
         return;
         }

      valid_script(trig_index[rnum]->proto, ch);
      }
   else
      send_to_char(ch,"Usage: tcheck <vnum>\r\n");
   release_buffer(str);
   }
