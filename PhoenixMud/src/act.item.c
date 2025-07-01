/* ************************************************************************
*   File: act.item.c                                    Part of CircleMUD * 
*  Usage: object handling routines -- get/drop and container handling     * 
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
#include "dg_scripts.h"
#include "constants.h"
#include "assemblies.h"
#include "vnum.h"

/* extern variables */
extern struct room_data *world;
extern struct obj_data *obj_proto;
extern struct index_data *obj_index;              /*. db.c .*/
extern struct zone_data *zone_table;              /*. db.c .*/
extern int top_of_zone_table;                     /*. db.c .*/

extern room_vnum donation_rooms[];
extern int num_donation;
/* External functions */
void Crash_count_items(struct obj_data * obj, long *nitems);
void save_corpses(void);
ACMD(do_assemble);
ACMD(do_split);
extern char *times_message(struct obj_data * obj, char *name, int num);
void mprog_give_trigger(struct char_data * mob, struct char_data * ch,
                        struct obj_data * obj);
void mprog_bribe_trigger(struct char_data * mob, struct char_data * ch,
                         int amount);
int can_give_gold(struct char_data *ch, int amount);
int min_level(struct char_data *ch,int spellnum);


void log_corpse(struct char_data *ch,struct obj_data *cont,char *ztString)
   {
   long nitems =0;
   /* don't exist */
   if(!cont||!ch)
      return;
   /* isn't a corpse */
   if((GET_OBJ_TYPE(cont)!=ITEM_CONTAINER) || !GET_OBJ_VAL(cont,3))
      return;
   /* isn't a PC corpse */
   if(!GET_OBJ_VAL(cont,6))
      return;
   /* is my corpse */
   if(GET_OBJ_VAL(cont,6)==GET_IDNUM(ch))
      return;
   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)||Z_FLAGGED(IN_ROOM(ch),Z_PKILL))
      return;
   Crash_count_items(cont->contains,&nitems);
   mudlogf(CMP,GOD_LOG(ch),TRUE,"CORPSE: %s %s %s's corpse.(%ld items)",
           GET_NAME(ch),
           ztString,get_name_by_id(GET_OBJ_VAL(cont,6)),nitems);

   }


void perform_put(struct char_data * ch, struct obj_data * obj,
                 struct obj_data * cont)
   {
   if (!drop_otrigger(obj, ch))
      return;
   if (IS_OBJ_STAT(obj, ITEM_NODROP)&&
           (IS_NPC(ch)||!PRF_FLAGGED(ch,PRF_NOHASSLE)))
      act("You can't put $p there, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
   else  if (GET_OBJ_VAL(cont,5) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0))
      act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
   else
      {
      obj_from_char(obj);
      obj_to_obj(obj, cont);
      save_char(ch,IN_ROOM(ch));
      Crash_crashsave(ch);
      act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
      act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
      if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
         {
         mudlogf(NRM, GOD_LOG(ch),TRUE,
                 "(GC) %s has put %s [%ld] into %s at [%ld].",
                 GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj),
                 GET_OBJ_NAME(cont), GET_ROOM_VNUM(IN_ROOM(ch)));
         }
      save_corpses();
      }
   }


/* The following put modes are supported by the code below:
 
 1) put <object> <container> 
 2) put all.<object> <container> 
 3) put all <container> 
 
 <container> must be in inventory or on ground. 
 all objects to be put into container must be in inventory. 
*/

ACMD(do_put)
   {
   char *arg1=get_buffer(MAX_INPUT_LENGTH);
   char *arg2=get_buffer(MAX_INPUT_LENGTH);
   char *arg3=get_buffer(MAX_INPUT_LENGTH);
   struct obj_data *obj, *next_obj, *cont;
   struct char_data *tmp_char;
   int obj_dotmode, cont_dotmode, found = 0,howmany=1;
   char *theobj,*thecont;

   argument = two_arguments(argument, arg1, arg2);
   one_argument(argument, arg3);

   if(*arg3&& is_number(arg1))
      {
      howmany=atoi(arg1);
      theobj=arg2;
      thecont=arg3;
      }
   else
      {
      theobj=arg1;
      thecont=arg2;
      }
   obj_dotmode = find_all_dots(theobj);
   cont_dotmode = find_all_dots(thecont);

   if (!*theobj)
      send_to_char(ch, "Put what in what?\r\n");
   else if (cont_dotmode != FIND_INDIV)
      send_to_char(ch, "You can only put things into one container at a time.\r\n");
   else if (!*thecont)
      {
      send_to_char(ch, "What do you want to put %s in?\r\n",
                   ((obj_dotmode == FIND_INDIV) ? "it" : "them"));
      }
   else
      {
      generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_ROOM|FIND_OBJ_EQUIP,
                   ch, &tmp_char, &cont);
      if (!cont)
         {
         send_to_char(ch, "You don't see %s %s here.\r\n",
                      AN(thecont), thecont);
         }
      else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
         act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
      else if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
         send_to_char(ch, "You'd better open it first!\r\n");
      else
         {
         if (obj_dotmode == FIND_INDIV)
            {
            /* put <obj> <container> */
            if (!(obj = get_obj_in_list_vis(ch, theobj, ch->carrying)))
               {
               send_to_char(ch, "You aren't carrying %s %s.\r\n", AN(theobj),
                            theobj);
               }
            else if (IS_OBJ_STAT(obj, ITEM_BATTLE_ITEM))
               send_to_char(ch, "You cannot put that into a container.\r\n");
            else if (obj == cont)
               send_to_char(ch, "You attempt to fold it into itself, but fail.\r\n");
            else
               {
               while(obj&&howmany--)
                  {
                  next_obj=obj->next_content;
                  perform_put(ch,obj,cont);
                  obj=get_obj_in_list_vis(ch,theobj,next_obj);
                  }
               }
            }
         else
            {
            for (obj = ch->carrying; obj; obj = next_obj)
               {
               next_obj = obj->next_content;
               if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
                       (obj_dotmode == FIND_ALL || isname(theobj, obj->name)))
                  {
                  if (IS_OBJ_STAT(obj, ITEM_BATTLE_ITEM))
                     {
                     send_to_char(ch, "You cannot put that into a container.\r\n");
                     continue;
                     }
                  found = 1;
                  perform_put(ch, obj, cont);
                  }
               }
            if (!found)
               {
               if (obj_dotmode == FIND_ALL)
                  send_to_char(ch, "You don't seem to have anything to put in it.\r\n");
               else
                  {
                  send_to_char(ch,"You don't seem to have any %ss.\r\n",
                               theobj);
                  }
               }
            }
         }
      }
   release_buffer(arg1);
   release_buffer(arg2);
   release_buffer(arg3);
   }



int can_take_obj(struct char_data * ch, struct obj_data * obj)
   {
   if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      {
      act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
      return (0);
      }
   else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
      {
      act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
      return (0);
      }
   else if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)))
      {
      act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
      return (0);
      }
   else if ((GET_OBJ_TYPE(obj) == ITEM_FURNITURE) && (obj->people) &&
            (GET_LEVEL(ch) < LVL_GRGOD))
      {
      act("$p: you can't take that it's being used as furniture!", FALSE,
          ch, obj, 0, TO_CHAR);
      return (0);
      }

   return (1);
   }


void get_check_money(struct char_data * ch, struct obj_data * obj, int subcmd)
   {
   char *buf;
   int value=GET_OBJ_VAL(obj,0);

   if ((GET_OBJ_TYPE(obj) != ITEM_MONEY) ||value< 0)
      return;

   buf = get_buffer(512);
   obj_from_char(obj);
   extract_obj(obj);
   save_char(ch,IN_ROOM(ch));
   Crash_crashsave(ch);
   if (value== 1)
      send_to_char(ch,"There was 1 coin.\r\n");
   else
      {
      send_to_char(ch, "There were %d coins.\r\n",value);
      }
   /*mudlogf(NRM,GOD_LOG(ch),TRUE,"Gold: %s has picked up %d coins.",
     GET_NAME(ch), value);*/
   GET_GOLD(ch) +=value;
   sprintf(buf,"%d",value);
   if(!IS_NPC(ch) && AFF_FLAGGED(ch,AFF_GROUP)&& (ch->followers||ch->master) &&
           (subcmd==SCMD_LOOT) && PRF_FLAGGED(ch,PRF_AUTOSPLIT))
      do_split(ch,buf,0,0);
   release_buffer(buf);
   }


void perform_get_from_container(struct char_data * ch, struct obj_data * obj,
                                struct obj_data * cont, int mode, int subcmd)
   {
   if (mode == FIND_OBJ_INV || can_take_obj(ch, obj))
      {
      if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
         act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
      else if(get_otrigger(obj,ch))
         {
         obj_from_obj(obj);
         obj_to_char(obj, ch);
         act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
         act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
         if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
            {
            mudlogf(NRM, GOD_LOG(ch),TRUE,
                    "(GC) %s has gotten %s [%ld] from %s at [%ld].",
                    GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj),
                    GET_OBJ_NAME(cont), GET_ROOM_VNUM(IN_ROOM(ch)));
            }

         get_check_money(ch, obj, subcmd);
         }
      }
   save_corpses();
   }


void get_from_container(struct char_data * ch, struct obj_data * cont,
                        char *arg, int mode, int subcmd,int howmany)
   {
   struct obj_data *obj, *next_obj;
   int obj_dotmode, found = 0;
   struct obj_data *obj_next;

   obj_dotmode = find_all_dots(arg);

   if (OBJVAL_FLAGGED(cont,CONT_CLOSED))
      act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
   else if (obj_dotmode == FIND_INDIV)
      {
      if (!(obj = get_obj_in_list_vis(ch, arg, cont->contains)))
         {
         char *buf = get_buffer(512);
         sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg), arg);
         act(buf, FALSE, ch, cont, 0, TO_CHAR);
         release_buffer(buf);
         }
      else
         {
         char *buf=get_buffer(256);
         sprintf(buf,"got %s from",arg);
         log_corpse(ch,cont,buf);
         release_buffer(buf);
         /*   perform_get_from_container(ch, obj, cont, mode, subcmd);  */
         while(obj &&howmany>0)
            {
            obj_next = obj->next_content;
            perform_get_from_container(ch,obj,cont,mode,subcmd);
            obj=get_obj_in_list_vis(ch,arg,obj_next);
            howmany--;
            }
         }
      }
   else
      {
      char *buf;
      if (obj_dotmode == FIND_ALLDOT && !(*arg))
         {
         send_to_char(ch, "Get all of what?\r\n");
         return;
         }
      buf = get_buffer(256);
      sprintf(buf,"got %s from",arg);
      log_corpse(ch,cont,buf);
      release_buffer(buf);
      for (obj = cont->contains; obj; obj = next_obj)
         {
         next_obj = obj->next_content;
         if (CAN_SEE_OBJ(ch, obj) &&
                 (obj_dotmode == FIND_ALL || isname(arg, obj->name)))
            {
            found = 1;
            perform_get_from_container(ch, obj, cont, mode, subcmd);
            }
         }
      if (!found)
         {
         if (obj_dotmode == FIND_ALL)
            act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
         else
            {
            char *buf2 = get_buffer(512);
            sprintf(buf2, "You can't seem to find any %ss in $p.", arg);
            act(buf2, FALSE, ch, cont, 0, TO_CHAR);
            release_buffer(buf2);
            }
         }
      }
   }


int perform_get_from_room(struct char_data * ch,struct obj_data * obj,int num)
   {
   if(num>0)
      {
      if ((can_take_obj(ch, obj))&&(obj!=NULL)&&get_otrigger(obj,ch))
         {
         char *buf = get_buffer(128);
         obj_from_room(obj);
         obj_to_char(obj, ch);
         send_to_char(ch, "You get %s (x %d).\r\n", GET_OBJ_NAME(obj), num+1);
         sprintf(buf,"%s gets %s (x %d).",GET_NAME(ch),GET_OBJ_NAME(obj),
                 num+1);
         act(buf, TRUE, ch, obj, 0, TO_ROOM);
         if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
            {
            mudlogf(NRM,  GOD_LOG(ch),TRUE,
                    "(GC) %s has picked up %s (x %d) [%ld] at [%ld].",
                    GET_NAME(ch), GET_OBJ_NAME(obj), num+1, GET_OBJ_VNUM(obj),
                    GET_ROOM_VNUM(IN_ROOM(ch)));
            }
         get_check_money(ch, obj, 0);
         release_buffer(buf);
         }
      save_corpses();
      return (1);
      }
   if(num==0)
      {
      if ((can_take_obj(ch, obj))&&get_otrigger(obj,ch))
         {
         log_corpse(ch,obj,"picked up");
         obj_from_room(obj);
         obj_to_char(obj, ch);
         act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
         act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);
         if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
            {
            mudlogf(NRM, GOD_LOG(ch),TRUE,
                    "(GC) %s has picked up %s [%ld] at [%ld].",
                    GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj),
                    GET_ROOM_VNUM(IN_ROOM(ch)));
            }
         get_check_money(ch, obj, 0);
         save_corpses();
         return (1);
         }
      }
   return (0);
   }


void get_from_room(struct char_data * ch, char *arg,int howmany)
   {
   struct obj_data *obj=0, *next_obj =0;
   int dotmode,num=0, found = 0;
   struct obj_data *obj_next=0;

   dotmode = find_all_dots(arg);

   if (dotmode == FIND_INDIV)
      {
      if (!(obj = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents)))
         {
         send_to_char(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
         }
      else
         {
         while(obj && howmany>0)
            {
            obj_next = obj->next_content;
            perform_get_from_room(ch, obj,0);
            obj = get_obj_in_list_vis(ch, arg, obj_next);
            howmany--;
            }
         }
      }
   else
      {
      if (dotmode == FIND_ALLDOT && !*arg)
         {
         send_to_char(ch, "Get all of what?\r\n");
         return;
         }
      if(((dotmode==FIND_ALLDOT)||(dotmode==FIND_ALL))&&
              ROOM_FLAGGED(IN_ROOM(ch),ROOM_DONATION)&&(GET_LEVEL(ch)<LVL_IMMORT))
         {
         send_to_char(ch,"You can't get all from a donation room.\r\n");
         send_info("[ INFO ] %s just tried to \"get all\" in a donation room!!\r\n", GET_NAME(ch));
         return;
         }
      for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj)
         {
         next_obj = obj->next_content;
         if (CAN_SEE_OBJ(ch, obj) && CAN_WEAR(obj, ITEM_WEAR_TAKE) &&
                 (dotmode == FIND_ALL || isname(arg, obj->name)))
            {
            found = 1;

            if ((CAN_GET_OBJ(ch, obj))&&(obj->next_content!=NULL) &&
                    (obj->item_number==obj->next_content->item_number))
               {
               if (can_take_obj(ch, obj))
                  {
                  if(GET_OBJ_VAL(obj, 3)&&(GET_OBJ_TYPE(obj)== ITEM_CONTAINER))
                     {
                     log_corpse(ch,obj,"picked up");
                     perform_get_from_room(ch, obj, 0);
                     }
                  else
                     {
                     num++;
                     log_corpse(ch,obj,"picked up");
                     obj_from_room(obj);
                     obj_to_char(obj, ch);
                     get_check_money(ch, obj, 0);
                     }
                  }
               }
            else
               {
               log_corpse(ch,obj,"picked up");
               perform_get_from_room(ch, obj, num);
               num=0;
               }
            }
         }
      if (!found)
         {
         if (dotmode == FIND_ALL)
            send_to_char(ch,"There doesn't seem to be anything here.\r\n");
         else
            {
            send_to_char(ch, "You don't see any %ss here.\r\n", arg);
            }
         }
      }
   }



ACMD(do_get)
   {
   char *arg1=get_buffer(MAX_INPUT_LENGTH);
   char *arg2=get_buffer(MAX_INPUT_LENGTH);
   char *arg3=get_buffer(MAX_INPUT_LENGTH);
   int amount=1;
   int cont_dotmode, found = 0, mode;
   struct obj_data *cont;
   struct char_data *tmp_char;

   argument = two_arguments(argument, arg1, arg2);
   one_argument(argument, arg3);

   if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      send_to_char(ch, "Your arms are already full!\r\n");
   else if (!*arg1)
      send_to_char(ch, "Get what?\r\n");
   else if (!*arg2)
      get_from_room(ch, arg1,1);
   else if (is_number(arg1) && !*arg3)
      get_from_room(ch, arg2, atoi(arg1));
   else
      {
      if (is_number(arg1))
         {
         amount = atoi(arg1);
         strcpy(arg1, arg2);
         strcpy(arg2, arg3);
         }
      cont_dotmode = find_all_dots(arg2);
      if (cont_dotmode == FIND_INDIV)
         {
         mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM|FIND_OBJ_EQUIP,
                             ch, &tmp_char, &cont);
         if (!cont)
            {
	      if (!strcmp(arg2, "npccorpse")) {
		send_to_char(ch, "You don't seem to have a corpse.\r\n");
	      } else {
		send_to_char(ch, "You don't have %s %s.\r\n", AN(arg2), arg2);
	      }
            }
         else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
            act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
         else
            get_from_container(ch, cont, arg1, mode, subcmd,amount);
         }
      else if (cont_dotmode == FIND_ALLDOT && !*arg2)
         {
         send_to_char(ch, "Get from all of what?\r\n");
         }
      else
         {
         for (cont = ch->carrying; cont; cont = cont->next_content)
            if (CAN_SEE_OBJ(ch, cont) &&
                    (cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
               {
               if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER)
                  {
                  found = 1;
                  get_from_container(ch, cont,arg1,FIND_OBJ_INV,subcmd,amount);
                  }
               else if (cont_dotmode == FIND_ALLDOT)
                  {
                  found = 1;
                  act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
                  }
               }
         for(cont=world[IN_ROOM(ch)].contents; cont; cont = cont->next_content)
            if (CAN_SEE_OBJ(ch, cont) &&
                    (cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
               {
               if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER)
                  {
                  get_from_container(ch,cont,arg1,FIND_OBJ_ROOM,subcmd,amount);
                  found = 1;
                  }
               else if (cont_dotmode == FIND_ALLDOT)
                  {
                  act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
                  found = 1;
                  }
               }
         if (!found)
            {
            if (cont_dotmode == FIND_ALL)
               send_to_char(ch, "You can't seem to find any containers.\r\n");
            else
               {
               send_to_char(ch, "You can't seem to find any %ss here.\r\n", arg2);
               }
            }
         }
      }
   release_buffer(arg3);
   release_buffer(arg2);
   release_buffer(arg1);
   }


void perform_drop_gold(struct char_data * ch, int amount,
                       byte mode, room_rnum RDR)
   {
   struct obj_data *obj;
   char *buf = get_buffer(SMALL_BUFSIZE);

   if (amount <= 0)
      send_to_char(ch, "Heh heh heh.. we are jolly funny today, eh?\r\n");
   else if (GET_GOLD(ch) < amount)
      send_to_char(ch, "You don't have that many coins!\r\n");
   else if (!can_give_gold(ch, amount))
      send_to_char(ch, "The auctioneer smacks you upside the head.\r\n");
   else
      {
      if (mode != SCMD_JUNK)
         {
         WAIT_STATE(ch, SKILL_LAG); /* to prevent coin-bombing */
         obj = create_money(amount);
         if ((mode == SCMD_DONATE)||(mode == SCMD_CDONATE))
            {
            send_to_char(ch, "You throw some gold into the air where it disappears in a puff of smoke!\r\n");
            act("$n throws some gold into the air where it disappears in a puff of smoke!", FALSE, ch, 0, 0, TO_ROOM);
            if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
               {
               mudlogf(NRM, GOD_LOG(ch),TRUE,
                       "(GC) %s has donated %d coins.", GET_NAME(ch), amount);
               }
            obj_to_room(obj, RDR);
            act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
            }
         else
            {
            if(!drop_wtrigger(obj,ch))
               {
               extract_obj(obj);
               return;
               }
            send_to_char(ch, "You drop some gold.\r\n");
            sprintf(buf, "$n drops %s.", money_desc(amount));
            act(buf, FALSE, ch, 0, 0, TO_ROOM);
            obj_to_room(obj, IN_ROOM(ch));
	    /*mudlogf(NRM,GOD_LOG(ch),TRUE,"Gold: %s has dropped %d coins.",
	      GET_NAME(ch), amount);*/
            if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
               {
               mudlogf(NRM, GOD_LOG(ch),TRUE,
                       "(GC) %s has dropped %d coins.", GET_NAME(ch), amount);
               }
            }
         }
      else
         {
         sprintf(buf, "$n drops %s which disappears in a puff of smoke!",
                 money_desc(amount));
         act(buf, TRUE, ch, 0, 0, TO_ROOM);
         send_to_char(ch, "You drop some gold which disappears in a puff of smoke!\r\n");
         if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
            {
            mudlogf(NRM, GOD_LOG(ch),TRUE,
                    "(GC) %s has junked %d coins.", GET_NAME(ch), amount);
            }
         }
      GET_GOLD(ch) -= amount;
      }
   save_char(ch,IN_ROOM(ch));
   Crash_crashsave(ch);
   release_buffer(buf);
   }


#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK || mode == SCMD_CDONATE) ? \
        "  It vanishes in a puff of smoke!" : "")

int perform_drop(struct char_data * ch, struct obj_data * obj,
                 byte mode, char *sname, room_rnum RDR, int num)
   {
   int value;
   long nitems=0;
   char *buf;
   if (!drop_otrigger(obj, ch))
      return 0;
   if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
      return 0;

   buf = get_buffer(SMALL_BUFSIZE);
   if (IS_OBJ_STAT(obj, ITEM_NODROP)&&
           (IS_NPC(ch)||!PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      sprintf(buf, "You can't %s $p, it must be CURSED!", sname);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      release_buffer(buf);

      return (0);
      }
   if (((mode == SCMD_DONATE)||(mode == SCMD_CDONATE)) && IS_OBJ_STAT(obj, ITEM_NODONATE))
      {
      act("You cannot donate $p.",FALSE,ch,obj,0,TO_CHAR);
      release_buffer(buf);
      return (0);
      }

   if(num>0)
      {
      sprintf(buf, "You %s $p.%s (x %d)", sname, VANISH(mode), num+1);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      sprintf(buf, "$n %ss $p.%s (x %d)", sname, VANISH(mode), num+1);
      act(buf, TRUE, ch, obj, 0, TO_ROOM);
      obj_from_char(obj);
      }
   else
      {
      sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      sprintf(buf, "$n %ss $p.%s", sname, VANISH(mode));
      act(buf, TRUE, ch, obj, 0, TO_ROOM);
      /* this will help to keep track of missing items */
      Crash_count_items(obj->contains, &nitems);
      if (obj->contains && !IS_NPC(ch) && !IS_CORPSE(obj) && (nitems>2) &&
            (GET_LEVEL(ch) < LVL_IMMORT))
         {
         send_to_char(ch, "&RWas that an accident?? You just %s%s %s with items in it!!&n\r\n",
                 sname, (mode==SCMD_DROP?"ped":(mode==SCMD_JUNK?"ed":"d")), GET_OBJ_NAME(obj));
	 if (mode == SCMD_JUNK || mode == SCMD_DONATE) {
	   release_buffer(buf);
	   send_to_char(ch, "&RYour actions have been stopped by the gods!&n\r\n");
	   return 0; /* Prevent morons from donating or junking containers. */
	 }
         log("OOPS: %s used %s on %s with %ld items in it(room %ld).", GET_NAME(ch),
              sname, GET_OBJ_NAME(obj), nitems, GET_ROOM_VNUM(IN_ROOM(ch)));
         }
      obj_from_char(obj);
      }
   save_char(ch,IN_ROOM(ch));
   Crash_crashsave(ch);
   release_buffer(buf);


   switch (mode)
      {
   case SCMD_DROP:
      obj_to_room(obj, IN_ROOM(ch));
      if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
         {
         mudlogf(NRM,GOD_LOG(ch),TRUE,
                 "(GC) %s has dropped %s [%ld] at [%ld].",
                 GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj),
                 GET_ROOM_VNUM(IN_ROOM(ch)));
         }
      return (0);
      break;
   case SCMD_DONATE:
     /* New code for donating (brewed) potions- set the newbie flag and send
	them to the old policy room.  Donated newbie potions decay after one
	real-time week. */
     if (GET_OBJ_TYPE(obj) == ITEM_POTION && IS_OBJ_STAT(obj, ITEM_NORENT)) {
       SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NEWBIE);
       SET_BIT(GET_OBJ_EXTRA(obj), ITEM_DONATED);
       RDR = real_room(1298); /* The old policy room. */
       GET_OBJ_TIMER(obj) = 8064; /* Reset the decay timer. */
       obj_to_room(obj, RDR);
       act("$p suddenly appears in a puff a smoke!", FALSE,0,obj,0,TO_ROOM);
       break;
     }
   case SCMD_CDONATE:
      if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
         {
         mudlogf(NRM,GOD_LOG(ch),TRUE,
                 "(GC) %s has donated %s [%ld] to [%ld].",
                 GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj), RDR);
         }
      obj_to_room(obj, RDR);
      act("$p suddenly appears in a puff a smoke!", FALSE,0,obj,0,TO_ROOM);
      SET_BIT(GET_OBJ_EXTRA(obj),ITEM_DONATED);
      return (0);
      break;
   case SCMD_JUNK:
      value = MAX(1, MIN(200, GET_OBJ_COST(obj) /16));
      if ((GET_LEVEL(ch)<LVL_NO_LOG)&&(GET_LEVEL(ch) >= LVL_IMMORT))
         {
         mudlogf(NRM,GOD_LOG(ch),TRUE,
                 "(GC) %s has junked %s [%ld].",
                 GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj));
         }
      extract_obj(obj);
      return value;
      break;
   default:
      log("SYSERR: Incorrect argument: %d passed to perform_drop",mode);
      break;
      }
   return (0);
   }



ACMD(do_drop)
   {
   struct obj_data *obj, *next_obj;
   room_rnum RDR = 0;
   byte mode = SCMD_DROP;
   int dotmode, amount = 0,num=0,multi;
   char *sname, *arg = get_buffer(MAX_INPUT_LENGTH);
   char *buf;
   int which_room;

   switch (subcmd)
      {
   case SCMD_JUNK:
      sname = "junk";
      mode = SCMD_JUNK;
      break;
   case SCMD_DONATE:
      sname = "donate";
      mode = SCMD_DONATE;
      which_room = number(0, num_donation-1);
      if(which_room>=num_donation)
         {
         mode=SCMD_JUNK;
         }
      else
         {
         RDR = real_room(donation_rooms[which_room]);
         }

      if ((mode!=SCMD_JUNK)&&(RDR == NOWHERE))
         {
         send_to_char(ch,"Sorry, you can't donate anything right now.\r\n");
         release_buffer(arg);
         return;
         }
      break;
   case SCMD_CDONATE:
      if(GET_CLAN(ch)<=0)
         {
         send_to_char(ch, "You don't belong to a clan!");
         release_buffer(arg);
         return;
         }
      sname = "cdonate";
      mode = SCMD_CDONATE;
      RDR = real_room(GET_CLAN_DONATE(ch));
      if (RDR == NOWHERE)
         {
         send_to_char(ch, "Sorry, you can't donate anything right now.\r\n");
         release_buffer(arg);
         return;
         }
      break;

   default:
      sname = "drop";
      break;
      }

   argument = one_argument(argument, arg);

   if (!*arg)
      {
      send_to_char(ch , "What do you want to %s?\r\n", sname);
      release_buffer(arg);
      return;
      }
   else if (is_number(arg))
      {
      multi= atoi(arg);
      one_argument(argument, arg);
      if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
         {
         perform_drop_gold(ch, multi, mode, RDR);
         }
      else if (multi <= 0)
         send_to_char(ch, "Yeah, that makes sense.\r\n");
      else if (!*arg)
         {
         send_to_char(ch, "What do you want to %s %d of?\r\n", sname, multi);
         }
      else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
         {
         send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
         }
      else
         {
         do
            {
            next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
            amount += perform_drop(ch, obj, mode, sname, RDR,0);
            obj = next_obj;
            }
         while (obj && --multi);
         }
      }
   else
      {
      dotmode = find_all_dots(arg);

      /* Can't junk or donate all */
      if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK||subcmd ==SCMD_DONATE))
         {
         if (subcmd == SCMD_JUNK)
            send_to_char(ch, "Go to the dump if you want to junk EVERYTHING!\r\n");
         else
            send_to_char(ch, "Go do the donation room if you want to donate EVERYTHING!\r\n");
         release_buffer(arg);
         return;
         }
      if (dotmode == FIND_ALL)
         {
         if (!ch->carrying)
            send_to_char(ch, "You don't seem to be carrying anything.\r\n");
         else
            for (obj = ch->carrying; obj; obj = next_obj)
               {
               next_obj = obj->next_content;
               /** Anduin*/
               if ((obj->next_content!=NULL)
                       &&(obj->item_number==obj->next_content->item_number))
                  {
                  num++;
                  amount++;
                  obj_from_char(obj);
                  if(mode == SCMD_DROP)
                     {
                     if(GET_OBJ_VAL(obj,3)
                             &&(GET_OBJ_TYPE(obj)==ITEM_CONTAINER))
                        {
                        log_corpse(ch,obj,"dropped");
                        buf = get_buffer(MAX_INPUT_LENGTH);
                        sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
                        act(buf, FALSE, ch, obj, 0, TO_CHAR);
                        sprintf(buf, "$n %ss $p.%s", sname, VANISH(mode));
                        act(buf, TRUE, ch, obj, 0, TO_ROOM);
                        release_buffer(buf);
                        obj_to_room(obj, IN_ROOM(ch));
                        }
                     else if (mode == SCMD_DONATE)
		       {
			 /* New code for donating (brewed) potions- set the newbie flag and send
			    them to the old policy room.  Donated newbie potions decay after one
			    real-time week. */
			 if (GET_OBJ_TYPE(obj) == ITEM_POTION && IS_OBJ_STAT(obj, ITEM_NORENT)) {
			   SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NEWBIE);
			   SET_BIT(GET_OBJ_EXTRA(obj), ITEM_DONATED);
			   GET_OBJ_TIMER(obj) = 8064; /* Reset the decay timer. */
			   obj_to_room(obj, real_room(1298)); /* The old policy room. */
			   act("$p suddenly appears in a puff a smoke!", FALSE,0,obj,0,TO_ROOM);
			 }
		       }
		     else
                        obj_to_room(obj, IN_ROOM(ch));

                     }
                  else
                     {
                     mudlogf(CMP,GOD_LOG(ch),TRUE,
                             "ERROR: do_drop, strange subcmd: %d",mode);
                     obj_to_room(obj,1);
                     }
                  }
               else
                  {
                  if(GET_OBJ_VAL(obj,3)&&(GET_OBJ_TYPE(obj) == ITEM_CONTAINER))
                     num =0;
                  log_corpse(ch,obj,sname);
                  amount += perform_drop(ch, obj, mode, sname, RDR,num);
                  num=0;
                  }
               }
         }
      else if (dotmode == FIND_ALLDOT)
         {
         if (!*arg)
            {
            send_to_char(ch, "What do you want to %s all of?\r\n", sname);
            release_buffer(arg);
            return;
            }
         if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
            {
            send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
            release_buffer(arg);
            return;
            }
         num=0;
         for(obj= get_obj_in_list_vis(ch,arg,ch->carrying);obj;obj = next_obj)
            {
            next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);

            if ((next_obj!=NULL)&&(obj->item_number==next_obj->item_number))
               {
               num++;
               amount++;
               obj_from_char(obj);
               if(mode == SCMD_DROP)
                  {
                  if(GET_OBJ_VAL(obj, 3)&&(GET_OBJ_TYPE(obj)==ITEM_CONTAINER))
                     {
                     log_corpse(ch,obj,"dropped");
                     buf = get_buffer(MAX_INPUT_LENGTH);
                     sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
                     act(buf, FALSE, ch, obj, 0, TO_CHAR);
                     sprintf(buf, "$n %ss $p.%s", sname, VANISH(mode));
                     act(buf, TRUE, ch, obj, 0, TO_ROOM);
                     obj_to_room(obj, IN_ROOM(ch));
                     release_buffer(buf);
                     }
                  else
                     {
                       obj_to_room(obj, IN_ROOM(ch));
                       #if 0
		       /* New code for donating (brewed) potions- set the newbie flag and send
			  them to the old policy room.  Donated newbie potions decay after one
			  real-time week. */
		       if (GET_OBJ_TYPE(obj) == ITEM_POTION && IS_OBJ_STAT(obj, ITEM_NORENT)) {
			 SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NEWBIE);
			 SET_BIT(GET_OBJ_EXTRA(obj), ITEM_DONATED);
			 GET_OBJ_TIMER(obj) = 8064; /* Reset the decay timer. */
			 obj_to_room(obj, real_room(1298)); /* The old policy room. */
			 act("$p suddenly appears in a puff a smoke!", FALSE,0,obj,0,TO_ROOM);
		       } else {
			 obj_to_room(obj, IN_ROOM(ch));
		       }
                       #endif
                     }
                  }
               else if((mode==SCMD_CDONATE)||(mode == SCMD_DONATE))
                  {
                  SET_BIT(GET_OBJ_EXTRA(obj),ITEM_NOAUC);
                  if(GET_OBJ_VAL(obj,3)&&(GET_OBJ_TYPE(obj) == ITEM_CONTAINER))
                     {
                     log_corpse(ch,obj,"donated");
                     buf = get_buffer(MAX_INPUT_LENGTH);
                     sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
                     act(buf, FALSE, ch, obj, 0, TO_CHAR);
                     sprintf(buf, "$n %ss $p.%s", sname, VANISH(mode));
                     act(buf, TRUE, ch, obj, 0, TO_ROOM);
                     obj_to_room(obj, RDR);
                     release_buffer(buf);
                     }
                  else
                     {
		       /* New code for donating (brewed) potions- set the newbie flag and send
			  them to the old policy room.  Donated newbie potions decay after one
			  real-time week. */
		       if (GET_OBJ_TYPE(obj) == ITEM_POTION && IS_OBJ_STAT(obj, ITEM_NORENT)) {
			 SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NEWBIE);
			 SET_BIT(GET_OBJ_EXTRA(obj), ITEM_DONATED);
			 GET_OBJ_TIMER(obj) = 8064; /* Reset the decay timer. */
			 obj_to_room(obj, real_room(1298)); /* The old policy room. */
			 act("$p suddenly appears in a puff a smoke!", FALSE,0,obj,0,TO_ROOM);
		       } else {
			 obj_to_room(obj, RDR);
		       }
                     }
                  }
               else if(mode == SCMD_JUNK)
                  {
                  if(GET_OBJ_VAL(obj, 3)&&(GET_OBJ_TYPE(obj)==ITEM_CONTAINER))
                     {
                     log_corpse(ch,obj,"junked");
                     buf = get_buffer(MAX_INPUT_LENGTH);
                     sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
                     act(buf, FALSE, ch, obj, 0, TO_CHAR);
                     sprintf(buf, "$n %ss $p.%s", sname, VANISH(mode));
                     act(buf, TRUE, ch, obj, 0, TO_ROOM);
                     extract_obj(obj);
                     release_buffer(buf);
                     }
                  else
                     {
                     extract_obj(obj);
                     }
                  }
               }
            else
               {
               if(GET_OBJ_VAL(obj, 3)&&(GET_OBJ_TYPE(obj) == ITEM_CONTAINER))
                  num=0;
               log_corpse(ch,obj,sname);
               amount += perform_drop(ch, obj, mode, sname, RDR,num);
               num=0;
               }

            }
         /*
          * next_obj = get_obj_in_list_vis(ch, arg, obj->next_content); 
          * while ((obj->item_number==next_obj->item_number) && (obj->next_content!=NULL)) { 
          * num++; 
          * amount++; 
          * obj_from_char(obj); 
          * obj_to_room(obj,IN_ROOM(ch));  
          * obj = next_obj; 
          * next_obj = get_obj_in_list_vis(ch, arg, obj->next_content); 
          * } 
          * amount += perform_drop(ch, obj, mode, sname, RDR,num); 
          */
         }
      else
         {
         if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
            {
            send_to_char(ch,"You don't seem to have %s %s.\r\n", AN(arg), arg);
            }
         else
            {
            log_corpse(ch,obj,sname);
            amount += perform_drop(ch, obj, mode, sname, RDR, 0);
            }
         }

      }

   if (amount && (subcmd == SCMD_JUNK))
      {
      send_to_char(ch,"You have been rewarded by the gods!\r\n");
      act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
      GET_GOLD(ch) += amount;
      }
   save_char(ch,IN_ROOM(ch));
   Crash_crashsave(ch);
   release_buffer(arg);
   }


void perform_give(struct char_data * ch, struct char_data * vict,
                  struct obj_data * obj)
   {
   if (IS_OBJ_STAT(obj, ITEM_NODROP)&&
           (IS_NPC(ch)||!PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
      return;
      }
   if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict))
      {
      act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
      act("$n tried to give you something, but your hands are full.", FALSE,
          ch, 0, vict, TO_VICT);
      return;
      }
   if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
      {
      act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
      act("$n tried to give you something, but you can't carry that much weight.", FALSE, ch, 0, vict, TO_VICT);
      return;
      }
   /* NOGIVE , Anduin 4/22/97 */
   if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOGIVE))
      {
      act("$N politely refuses your gift.", FALSE, ch, 0, vict, TO_CHAR);
      return;
      }

   if (!give_otrigger(obj, ch, vict) || !receive_mtrigger(vict, ch, obj))
      return;

   obj_from_char(obj);
   obj_to_char(obj, vict);
   save_char(ch,IN_ROOM(ch));
   Crash_crashsave(ch);

   if (!IS_NPC(ch)&&
           ((GET_LEVEL(ch)<LVL_NO_LOG)&&
            ((GET_LEVEL(ch) >= LVL_IMMORT) ||
             (GET_LEVEL(vict) >= LVL_IMMORT))))
      {
      mudlogf(NRM,GOD_LOG(ch),TRUE,
              "(GC) %s has given %s %s [%ld].",
              GET_NAME(ch), GET_NAME(vict), GET_OBJ_NAME(obj),
              GET_OBJ_VNUM(obj));
      }


   MOBTrigger = FALSE;
   act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
   MOBTrigger = FALSE;
   act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
   MOBTrigger = FALSE;
   act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
   mprog_give_trigger(vict, ch, obj);
   }

/* utility function for give */
struct char_data *give_find_vict(struct char_data * ch, char *arg)
   {
   struct char_data *vict;

   if (!*arg)
      {
      send_to_char(ch, "To who?\r\n");
      return (NULL);
      }
   else if (!(vict = get_char_vis(ch, arg,FIND_CHAR_ROOM)))
      {
      send_to_char(ch, "%s", NOPERSON);
      return (NULL);
      }
   else if (vict == ch)
      {
      send_to_char(ch, "What's the point of that?\r\n");
      return (NULL);
      }
   else
      return (vict);
   }


void perform_give_gold(struct char_data * ch, struct char_data * vict,
                       int amount)
   {
   char *buf;

   if (amount <= 0)
      {
      send_to_char(ch,"Heh heh heh ... we are jolly funny today, eh?\r\n");
      return;
      }
   if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_DGOD)))
      {
      send_to_char(ch,"You don't have that many coins!\r\n");
      return;
      }
   if (!can_give_gold(ch, amount))
      {
      send_to_char(ch, "The auctioneer smacks you upside the head.\r\n");
      return;
      }
   if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOGIVE))
      {
      act("$N politely refuses your gift.", FALSE, ch, 0, vict, TO_CHAR);
      return;
      }
   send_to_char(ch, "%s", OK);
   buf=get_buffer(SMALL_BUFSIZE);
   mprog_bribe_trigger(vict, ch, amount);
   sprintf(buf, "$n gives you %d gold coin%s.", amount,amount==1?"":"s");
   MOBTrigger = FALSE;
   act(buf, FALSE, ch, 0, vict, TO_VICT);
   sprintf(buf, "$n gives %s to $N.", money_desc(amount));
   MOBTrigger = FALSE;
   act(buf, TRUE, ch, 0, vict, TO_NOTVICT);

   mudlogf(NRM,GOD_LOG(ch),TRUE,"Gold: %s has given %s %d coins.",
           GET_NAME(ch), GET_NAME(vict), amount);
   release_buffer(buf);
   if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_DGOD))
      GET_GOLD(ch) -= amount;
   GET_GOLD(vict) += amount;
   bribe_mtrigger(vict,ch,amount);
   }


ACMD(do_give)
   {
   int amount, dotmode;
   struct char_data *vict;
   struct obj_data *obj, *next_obj;
   char *arg=get_buffer(MAX_INPUT_LENGTH);

   argument = one_argument(argument, arg);

   if (!*arg)
      send_to_char(ch, "Give what to who?\r\n");
   else if (is_number(arg))
      {
      amount = atoi(arg);
      argument = one_argument(argument, arg);
      skip_spaces(&argument);
      if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
         {
         one_argument(argument, arg);
         if ((vict = give_find_vict(ch, arg))!=NULL)
            perform_give_gold(ch, vict, amount);
         release_buffer(arg);
         save_char(ch,IN_ROOM(ch));
         Crash_crashsave(ch);
         return;
         }
      else if (!*arg)         /* Give multiple code. */
         {
         send_to_char(ch, "What do you want to give %d of?\r\n", amount);
         }
      else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
         {
         send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
         }
      else if (!(vict = give_find_vict(ch, argument)))
         {
         release_buffer(arg);
         return;
         }
      else
         {
         while (obj && amount--)
            {
            next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
            perform_give(ch, vict, obj);
            obj = next_obj;
            }
         }
      }
   else
      {
      char *buf1=get_buffer(MAX_INPUT_LENGTH);
      one_argument(argument, buf1);
      if (!(vict = give_find_vict(ch, buf1)))
         {
         release_buffer(buf1);
         release_buffer(arg);
         return;
         }
      release_buffer(buf1);

      dotmode = find_all_dots(arg);
      if (dotmode == FIND_INDIV)
         {
         if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
            {
            send_to_char(ch,"You don't seem to have %s %s.\r\n", AN(arg), arg);
            }
         else
            perform_give(ch, vict, obj);
         }
      else
         {
         if (dotmode == FIND_ALLDOT && !*arg)
            {
            send_to_char(ch, "All of what?\r\n");
            release_buffer(arg);
            return;
            }
         if (!ch->carrying)
            send_to_char(ch, "You don't seem to be holding anything.\r\n");
         else
            for (obj = ch->carrying; obj; obj = next_obj)
               {
               next_obj = obj->next_content;
               if (CAN_SEE_OBJ(ch, obj) &&
                       ((dotmode == FIND_ALL || isname(arg, obj->name))))
                  perform_give(ch, vict, obj);
               }
         }
      }
   save_char(ch,IN_ROOM(ch));
   Crash_crashsave(ch);
   release_buffer(arg);
   }



void weight_change_object(struct obj_data * obj, int weight)
   {
   struct obj_data *tmp_obj;
   struct char_data *tmp_ch;

   if (IN_ROOM(obj) != NOWHERE)
      {
      GET_OBJ_WEIGHT(obj) += weight;
      }
   else if ((tmp_ch = obj->carried_by))
      {
      obj_from_char(obj);
      GET_OBJ_WEIGHT(obj) += weight;
      obj_to_char(obj, tmp_ch);
      }
   else if ((tmp_obj = obj->in_obj))
      {
      obj_from_obj(obj);
      GET_OBJ_WEIGHT(obj) += weight;
      obj_to_obj(obj, tmp_obj);
      }
   else
      {
      log("SYSERR: Unknown attempt to subtract weight from an object.");
      }
   }



void name_from_drinkcon(struct obj_data * obj)
   {
   int i;
   char *new_name;

   for(i=0; (*((obj->name) + i) != ' ') && (*((obj->name) + i) != '\0'); i++)
      ;

   if (*((obj->name) + i) == ' ')
      {
      new_name = str_dup((obj->name) + i + 1);
      if(GET_OBJ_RNUM(obj)<0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
         free(obj->name);
      obj->name = new_name;
      }
   }



void name_to_drinkcon(struct obj_data * obj, int type)
   {
   char *new_name;

   CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
   sprintf(new_name, "%s %s", drinknames[type], obj->name);
   if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
      free(obj->name);
   obj->name = new_name;
   }



ACMD(do_drink)
   {
   struct obj_data *temp;
   struct affected_type af;
   int amount, weight;
   int on_ground = 0;
   char *arg;

   if(IS_NPC(ch))  /* cannot us GET_COND() on mobs */
      return;

   if(FIGHTING(ch))
      {
      send_to_char(ch, "You are too busy fighting to drink right now!!\r\n");
      return;
      }

   arg = get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      send_to_char(ch, "Drink from what?\r\n");
      release_buffer(arg);
      return;
      }
   if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying)))
      {
      if (!(temp = get_obj_in_list_vis(ch, arg, world[IN_ROOM(ch)].contents)))
         {
         send_to_char(ch, "You can't find it!\r\n");
         release_buffer(arg);
         return;
         }
      else
         on_ground = 1;
      }
   release_buffer(arg);

   if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
           (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN))
      {
      send_to_char(ch, "You can't drink from that!\r\n");
      return;
      }
   if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON))
      {
      send_to_char(ch, "You have to be holding that to drink from it.\r\n");
      return;
      }
   if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0))
      {
      /* The pig is drunk */
      send_to_char(ch,"You can't seem to get close enough to your mouth.\r\n");
      act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
      return;
      }
   if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 10))
      {
      send_to_char(ch,"Your stomach can't contain anymore!\r\n");
      return;
      }
   if (GET_COND(ch, THIRST) > 20)
      {
      send_to_char(ch,"Your stomach can't contain anymore!\r\n");
      return;
      }
   if (!GET_OBJ_VAL(temp, 1))
      {
      send_to_char(ch,"It's empty.\r\n");
      return;
      }
   if (subcmd == SCMD_DRINK)
      {
      char *buf=get_buffer(SMALL_BUFSIZE);
      sprintf(buf, "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp, 2)]);
      act(buf, TRUE, ch, temp, 0, TO_ROOM);
      release_buffer(buf);
      send_to_char(ch, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);

      if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
         amount = (25 - GET_COND(ch, THIRST))
                  / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
      else
         amount = number(3, 10);

      }
   else
      {
      act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
      send_to_char(ch, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
      amount = 1;
      }

   amount = MIN(amount, GET_OBJ_VAL(temp, 1));

   /* You can't subtract more than the object weighs */
   weight = MIN(amount, GET_OBJ_WEIGHT(temp));

   weight_change_object(temp, -weight); /* Subtract amount */

   gain_condition(ch, DRUNK,
                  +(int)((int)drink_aff[GET_OBJ_VAL(temp,2)][DRUNK]*amount)/4);

   gain_condition(ch, FULL,
                  +(int)((int) drink_aff[GET_OBJ_VAL(temp,2)][FULL]*amount)/4);

   gain_condition(ch, THIRST,
                  +(int)((int)drink_aff[GET_OBJ_VAL(temp,2)][THIRST]*amount)/4);

   if (GET_COND(ch, DRUNK) > 10)
      send_to_char(ch,"You feel drunk.\r\n");

   if (GET_COND(ch, THIRST) > 20)
      send_to_char(ch,"You don't feel thirsty any more.\r\n");

   if (GET_COND(ch, FULL) > 20)
      send_to_char(ch,"You are full.\r\n");

   if (GET_OBJ_VAL(temp, 3))
      {
      /* The shit was poisoned ! */
      send_to_char(ch,"Oops, it tasted rather strange!\r\n");
      act("$n chokes and utters some strange sounds.",TRUE,ch, 0, 0, TO_ROOM);

      af.type = SPELL_POISON;
      af.duration = amount * 3;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_POISON;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      }
   /* empty the container, and no longer poison. */
   GET_OBJ_VAL(temp, 1) -= amount;
   if (!GET_OBJ_VAL(temp, 1))
      {
      /* The last bit */
      GET_OBJ_VAL(temp, 2) = 0;
      GET_OBJ_VAL(temp, 3) = 0;
      name_from_drinkcon(temp);
      }
   return;
   }



ACMD(do_eat)
   {
   struct obj_data *food;
   struct affected_type af;
   int amount,j;
   int level =0;
   char *arg;

   if(IS_NPC(ch))  /* cannot use GET_COND on mobs */
      return;
   arg = get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      send_to_char(ch,"Eat what?\r\n");
      release_buffer(arg);
      return;
      }
   if(FIGHTING(ch))
      {
      send_to_char(ch,"You are too busy fighting to eat right now!!\r\n");
      release_buffer(arg);
      return;
      }
   if (!(food = get_obj_in_list_vis(ch, arg, ch->carrying)))
      {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
                                (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN)))
      {
      do_drink(ch, argument, 0, SCMD_SIP);
      return;
      }
   if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && (GET_LEVEL(ch) < LVL_DGOD))
      {
      send_to_char(ch, "You can't eat THAT!\r\n");
      return;
      }
   if (GET_COND(ch, FULL) > 20) /* Stomach full */
      {

      send_to_char(ch, "You are too full to eat more!\r\n");
      return;
      }
   if (subcmd == SCMD_EAT)
      {
      act("You eat $p.", FALSE, ch, food, 0, TO_CHAR);
      act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
      level = MAX(1,MIN(10,GET_OBJ_VAL(food,0)));
      }
   else
      {
      act("You nibble a little bit of $p.", FALSE, ch, food, 0, TO_CHAR);
      act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
      level = 1;
      }
   for(j=0;j<MAX_OBJ_AFFECT;j++)
      {
      if(food->affected[j].location == APPLY_EAT_SPELL)
         {
         call_magic(ch,ch,NULL,NULL,NULL,food->affected[j].modifier,level,
                    CAST_POTION);
         GET_OBJ_VAL(food,0)=MIN(GET_OBJ_VAL(food,0),10);
         }
      }

   amount = +(subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

   gain_condition(ch, FULL, amount);

   if (GET_COND(ch, FULL) > 20)
      send_to_char(ch, "You are full.\r\n");

   if (GET_OBJ_VAL(food, 3) && (GET_LEVEL(ch) < LVL_IMMORT))
      {
      /* The shit was poisoned ! */
      send_to_char(ch, "Oops, that tasted rather strange!\r\n");
      act("$n coughs and utters some strange sounds.",FALSE,ch,0, 0, TO_ROOM);

      af.type = SPELL_POISON;
      af.duration = amount * 2;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_POISON;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      }
   if (subcmd == SCMD_EAT)
      extract_obj(food);
   else
      {
      if (!(--GET_OBJ_VAL(food, 0)))
         {
         send_to_char(ch, "There's nothing left now.\r\n");
         extract_obj(food);
         }
      }
   }


ACMD(do_pour)
   {
   char *arg1=get_buffer(MAX_INPUT_LENGTH);
   char *arg2=get_buffer(MAX_INPUT_LENGTH);
   struct obj_data *from_obj = NULL, *to_obj = NULL;
   int amount;

   two_arguments(argument, arg1, arg2);

   if (subcmd == SCMD_POUR)
      {
      if (!*arg1)   /* No arguments */
         {
         send_to_char(ch, "From what do you want to pour?\r\n");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
         {
         send_to_char(ch, "You can't find it!\r\n");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON)
         {
         send_to_char(ch, "You can't pour from that!\r\n");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      }
   if (subcmd == SCMD_FILL)
      {
      if (!*arg1)   /* no arguments */
         {
         send_to_char(ch, "What do you want to fill?  And what are you filling it from?\r\n");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
         {
         send_to_char(ch, "You can't find it!\r\n");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON)
         {
         act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      if (!*arg2)   /* no 2nd argument */
         {
         act("What do you want to fill $p from?",FALSE,ch,to_obj, 0, TO_CHAR);
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      if (!(from_obj=get_obj_in_list_vis(ch,arg2,world[IN_ROOM(ch)].contents)))
         {
         send_to_char(ch, "There doesn't seem to be %s %s here.\r\n",
                      AN(arg2),arg2);
         release_buffer(arg1);
         release_buffer(arg2);
         return;
         }
      if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN)
         {
         act("You can't fill something from $p.",FALSE,ch,from_obj,0,TO_CHAR);
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      }
   if (GET_OBJ_VAL(from_obj, 1) == 0)
      {
      act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }
   if (subcmd == SCMD_POUR)
      {
      /* pour */
      if (!*arg2)
         {
         send_to_char(ch, "Where do you want it?  Out or in what?\r\n");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      if (!str_cmp(arg2, "out"))
         {
         act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
         act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

         weight_change_object(from_obj, MAX(-GET_OBJ_VAL(from_obj, 1),-GET_OBJ_WEIGHT(from_obj))); /* Empty */


         GET_OBJ_VAL(from_obj, 1) = 0;
         GET_OBJ_VAL(from_obj, 2) = 0;
         GET_OBJ_VAL(from_obj, 3) = 0;
         name_from_drinkcon(from_obj);
         release_buffer(arg2);
         release_buffer(arg1);

         return;
         }
      if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying)))
         {
         send_to_char(ch, "You can't find it!\r\n");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
              (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN))
         {
         send_to_char(ch, "You can't pour anything into that.\r\n");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      }
   if (to_obj == from_obj)
      {
      send_to_char(ch, "A most unproductive effort.\r\n");
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }
   if ((GET_OBJ_VAL(to_obj, 1) != 0) &&
           (GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2)))
      {
      send_to_char(ch, "There is already another liquid in it!\r\n");
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }
   if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0)))
      {
      send_to_char(ch, "There is no room for more.\r\n");
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }
   if (subcmd == SCMD_POUR)
      {
      send_to_char(ch, "You pour the %s into %s.\r\n",
                   drinks[GET_OBJ_VAL(from_obj, 2)], GET_OBJ_NAME(to_obj));
      }
   if (subcmd == SCMD_FILL)
      {
      act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
      act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
      }
   /* New alias */
   if (GET_OBJ_VAL(to_obj, 1) == 0)
      name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

   /* First same type liq. */
   GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

   /* Then how much to pour */
   GET_OBJ_VAL(from_obj, 1)-=(amount =
                                 (GET_OBJ_VAL(to_obj,0)-GET_OBJ_VAL(to_obj, 1)));

   GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

   if (GET_OBJ_VAL(from_obj, 1) < 0)
      {
      /* There was too little */
      GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
      amount += GET_OBJ_VAL(from_obj, 1);
      GET_OBJ_VAL(from_obj, 1) = 0;
      GET_OBJ_VAL(from_obj, 2) = 0;
      GET_OBJ_VAL(from_obj, 3) = 0;
      name_from_drinkcon(from_obj);
      }
   /* Then the poison boogie */
   GET_OBJ_VAL(to_obj, 3) =
      (GET_OBJ_VAL(to_obj,3)|| GET_OBJ_VAL(from_obj, 3));

   /* And the weight boogie */
   weight_change_object(from_obj, -amount);
   weight_change_object(to_obj, amount); /* Add weight */
   release_buffer(arg2);
   release_buffer(arg1);
   }



void wear_message(struct char_data * ch, struct obj_data * obj, int where_pos)
   {
   char *wear_messages[][2] =
      {
         {"$n does some weird shit.",
          "YOU SHOULD NEVER SEE THIS MESSAGE!!!(wear_messages)" },
         {"$n slides $p on to $s right ring finger.",
          "You slide $p on to your right ring finger." } ,
         {"$n slides $p on to $s left ring finger.",
          "You slide $p on to your left ring finger." } ,
         {"$n wears $p around $s neck.",
          "You wear $p around your neck." } ,
         {"$n wears $p around $s neck.",
          "You wear $p around your neck." } ,
         {"$n wears $p on $s body.",
          "You wear $p on your body.", } ,
         {"$n wears $p on $s head.",
          "You wear $p on your head." } ,
         {"$n puts $p on $s legs.",
          "You put $p on your legs." } ,
         {"$n wears $p on $s feet.",
          "You wear $p on your feet." } ,
         {"$n puts $p on $s hands.",
          "You put $p on your hands." } ,
         {"$n wears $p on $s arms.",
          "You wear $p on your arms." } ,
         {"$n straps $p around $s arm as a shield.",
          "You start to use $p as a shield." } ,
         {"$n wears $p about $s body.",
          "You wear $p around your body." } ,
         {"$n wears $p around $s waist.",
          "You wear $p around your waist." } ,
         {"$n puts $p on around $s right wrist.",
          "You put $p on around your right wrist." } ,
         {"$n puts $p on around $s left wrist.",
          "You put $p on around your left wrist." } ,
         {"$n wields $p.",
          "You wield $p." } ,
         {"$n wields $p.",
          "You wield $p." } ,
         {"$n grabs $p.",
          "You grab $p." } ,
         {"$n grabs $p.",
          "You grab $p." } ,
         {"$n clips $p on $s left ear.", /* New EQ positions--Aleks */
          "You clip $p on your left ear." } ,
         {"$n clips $p on $s right ear.", /* New EQ positions--Aleks */
          "You clip $p on your right ear." } ,
         {"$n puts $p on $s face.",  /* New EQ positions--Aleks */
          "You put $p on your face." } ,
         {"$n puts $p on $s back.",  /* New EQ positions--Aleks */
          "You put $p on your back." } ,
         {"$n attaches $p to $s chest.", /* New EQ position--Faron */
          "You attach $p to your chest." }
      } ;

   act(wear_messages[where_pos][0], TRUE, ch, obj, 0, TO_ROOM);
   act(wear_messages[where_pos][1], FALSE, ch, obj, 0, TO_CHAR);
   }

int perform_remove(struct char_data * ch, int pos)
   {
   struct obj_data *obj;

   if (!(obj = GET_EQ(ch, pos)))
      {
      log("SYSERR: in perform_remove: bad pos: %d passed. obj: %ld who: %s",
          pos, GET_OBJ_VNUM(obj),GET_NAME(ch));
      return 0;
      }
   else if(IS_OBJ_STAT(obj, ITEM_NODROP)&&
           (IS_NPC(ch)||!PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      act("You can't remove $p, it must be CURSED!",FALSE,ch,obj,0,TO_CHAR);
      return 0;
      }
   else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      {
      act("$p: you can't carry that many items!",FALSE,ch,obj,0,TO_CHAR);
      return 0;
      }
   else if (FIGHTING(ch) && IS_SET(obj->obj_flags.bitvector, AFF_NO_FLEE))
      {
      act("You are too busy to remove $p!",FALSE,ch,obj,0,TO_CHAR);
      return 0;
      }
   else
      {
      if (!remove_otrigger(obj, ch))
         return 0;
      obj_to_char(unequip_char(ch, pos), ch);
      act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
      if(pos==WEAR_WIELD_1)
         {
         if(GET_EQ(ch,WEAR_WIELD_2))
            {
            send_to_char(ch, "You switch your remaining weapon to your good "
                         "hand.\r\n");
            equip_char(ch,unequip_char(ch,WEAR_WIELD_2), WEAR_WIELD_1);
            }
         }
      }
   check_weapon_weight(ch);
   return 1;
   }

int hands_full(struct char_data *ch)
   {
   int counter, worn_count;
   struct obj_data * weapon;

   worn_count = 0;
   for (counter = 0; counter < NUM_HAND_POSITIONS; counter ++)
      if (GET_EQ(ch, hand_position[counter]))
         worn_count++;
   if (worn_count >= 2)
      return TRUE;
   if ((weapon = GET_EQ(ch, WEAR_WIELD_1))||(weapon=GET_EQ(ch,WEAR_WIELD_2)))
      if (TWO_HANDED(weapon))
         return TRUE;
   return FALSE;
   }

/*
** Checks to see if the character can wear the item 
** after level and remort restrictions(if present)
** note: messages can be suppressed by setting message as FALSE
*/
int can_wear_lr(struct char_data *ch, struct obj_data *obj, int message)
   {
   /* immortals can wear anything */
   if (GET_LEVEL(ch) >= LVL_IMMORT)
      return 1;

   /* level restricted 104+ god only */
   if ((GET_OBJ_LR(obj) >= LVL_WANKER) && (GET_LEVEL(ch) < LVL_IMMORT))
      {
      if (message)
         {
         act("$n tries to use $p but is not godly enough.", FALSE, ch,
             obj, 0, TO_ROOM);
         act("You are not godly enough to use $p.", FALSE, ch,
             obj, 0, TO_CHAR);
         }
      return 0;
      }

   /* newbie items: level restricted to non-remorts levels 20 or below */
   if (IS_OBJ_STAT(obj, ITEM_NEWBIE) &&
       ((GET_LEVEL(ch) > LVL_NEWBIE) || !is_remort_level(ch, NON_REMORT)))
      {
      if (message)
         {
         act("$n tries to use $p, but succeeds only in "
             "making $mself look like a newbie.",
             FALSE, ch, obj, 0, TO_ROOM);
         act("You try to use $p, but decide instead to save "
             "it for a newbie that might need it.",
             FALSE, ch, obj, 0, TO_CHAR);
         }
      return 0;
      }

   if (REMORT_LEVEL(ch) == TRIPLE_REMORT)
   {
     return 1;
   }

   /* items lr < 104*/
   if (GET_OBJ_LR(obj) > GET_LEVEL(ch))
      {
      /*** if item is restricted above character's level, then:           ****
      ****   no character can wear double remort items,                   ****
      ****   less than a double remort cannot wear remort items, and      ****
      ****   non-remorts cannot wear non-remort items.                    ***/
      if (IS_DBLREMORT_ITEM(obj) ||
          (IS_REMORT_ITEM(obj) && !is_remort_level(ch, DOUBLE_REMORT)) ||
          (!IS_DBLREMORT_ITEM(obj) && !IS_REMORT_ITEM(obj) && 
                     is_remort_level(ch, NON_REMORT)))
         {
         if (message)
            {
            act("$n tries to use $p but is too inexperienced.", FALSE, ch,
                obj, 0, TO_ROOM);
            act("You try to use $p but are too inexperienced.", FALSE, ch,
                obj, 0, TO_CHAR);
            }
         return 0;
         }
      }
   else /* obj lr is less than or equal to level of ch */
      {
      /*** if character level is above item level restriction, then:      ****
      ****   non-double-remorts cannot wear double remort items,          ****
      ****   non-remorts cannot wear remort items.                        ***/
      if ( REMORT_LEVEL(ch) < TRIPLE_REMORT &&
           ((IS_DBLREMORT_ITEM(obj) && !is_remort_level(ch, DOUBLE_REMORT)) ||
            (IS_REMORT_ITEM(obj) && is_remort_level(ch, NON_REMORT)) ))
         {
         if (message)
            {
            act("$n tries to use $p but is too inexperienced.", FALSE, ch,
                obj, 0, TO_ROOM);
            act("You try to use $p but are too inexperienced.", FALSE, ch,
                obj, 0, TO_CHAR);
            }
         return 0;
         }
      }
   /* passing those, they should adhere to these guidelines by default:   ****
   ***************************************************************************
   ** double remorts can wear all single-remort and non-remort items of   ****
   **   any level restriction, plus double-remort items restricted to     ****
   **   their level and below.                                            ****
   ** remorts can wear all non-remort items of any level restriction,     ****
   **   plus single-remort items restricted to their level and below.     ****
   ** non-remorts can only wear items restricted to their level and below.****
   **************************************************************************/
   return 1;
   }


void perform_wear(struct char_data * ch, struct obj_data * obj, int where_pos, int visible)
   {
   int counter;
   int retval=1;
   /*
    * ITEM_WEAR_TAKE is used for objects that do not require special bits 
    * to be put into that position (e.g. you can hold any object, not just 
    * an object with a HOLD bit.) 
    */

   int wear_bitvectors[] =
      {
         ITEM_WEAR_TAKE,
         ITEM_WEAR_FINGER,
         ITEM_WEAR_FINGER,
         ITEM_WEAR_NECK,
         ITEM_WEAR_NECK,
         ITEM_WEAR_BODY,
         ITEM_WEAR_HEAD,
         ITEM_WEAR_LEGS,
         ITEM_WEAR_FEET,
         ITEM_WEAR_HANDS,
         ITEM_WEAR_ARMS,
         ITEM_WEAR_SHIELD,
         ITEM_WEAR_ABOUT,
         ITEM_WEAR_WAIST,
         ITEM_WEAR_WRIST,
         ITEM_WEAR_WRIST,
         ITEM_WEAR_WIELD,
         ITEM_WEAR_WIELD,
         ITEM_WEAR_TAKE,
         ITEM_WEAR_TAKE,
         ITEM_WEAR_EAR,
         ITEM_WEAR_EAR,
         ITEM_WEAR_FACE,
         ITEM_WEAR_BACK,
         ITEM_WEAR_HEART
      } ;

   /* first, make sure that the wear position is valid. */
   if (!CAN_WEAR(obj, wear_bitvectors[where_pos]))
      {
      act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
      return;
      }

   /* Next, make sure the char is the appropriate level to wear the item */
   if (!can_wear_lr(ch, obj, TRUE))
      return;

   /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
   /* Added ear--New EQ positions--Aleks */
   if ((where_pos == WEAR_FINGER_R)||
           (where_pos == WEAR_EAR_L)   ||
           (where_pos == WEAR_NECK_1)  ||
           (where_pos == WEAR_WRIST_R) ||
           (where_pos == WEAR_WIELD_1) ||
           (where_pos == WEAR_HOLD_1))
      if (GET_EQ(ch, where_pos))
         where_pos++;

   /* if you can't dual_wield, replace your current weapon. */
   if (
     !IS_NPC(ch)
     &&
     (where_pos == WEAR_WIELD_1 || where_pos == WEAR_WIELD_2)
     &&
     !GET_SKILL(ch, SKILL_DUAL_WIELD)
     &&
     (GET_EQ(ch, WEAR_WIELD_1) || GET_EQ(ch, WEAR_WIELD_2))
     )
      {
      where_pos=WEAR_WIELD_1;
      }

   if (!IS_NPC(ch)
       && (where_pos == WEAR_WIELD_1 || where_pos == WEAR_WIELD_2)
       && (GET_EQ(ch, WEAR_WIELD_1) || GET_EQ(ch, WEAR_WIELD_2))
       && !SCR_SKILLCHECK(ch, SKILL_DUAL_WIELD))
   {
     where_pos = WEAR_WIELD_1;
   }


   /* Allows you to wear something while removing the old piece in that position*/

   if(TWO_HANDED(obj)&&((where_pos==WEAR_WIELD_1)||(where_pos== WEAR_WIELD_2)))
      {
      where_pos=WEAR_WIELD_1;
      for(counter=0;counter <NUM_HAND_POSITIONS;counter++)
         if(GET_EQ(ch,hand_position[counter]))
            if((retval = perform_remove(ch,hand_position[counter]))==0)
               break;
      }
   else if((GET_EQ(ch,WEAR_WIELD_1)) && TWO_HANDED(GET_EQ(ch,WEAR_WIELD_1)))
      retval = perform_remove(ch,WEAR_WIELD_1);
   else if((GET_EQ(ch,WEAR_WIELD_2)) && TWO_HANDED(GET_EQ(ch,WEAR_WIELD_2)))
      retval = perform_remove(ch,WEAR_WIELD_2);
   else if((where_pos==WEAR_WIELD_1)  ||(where_pos == WEAR_WIELD_2)||
           (where_pos == WEAR_SHIELD) ||(where_pos == WEAR_HOLD_1) ||
           (where_pos == WEAR_HOLD_2))
      {
      if(GET_EQ(ch,where_pos))
         retval = perform_remove(ch,where_pos);
      else if(hands_full(ch))
         {
         if(GET_EQ(ch,WEAR_HOLD_2))
            retval = perform_remove(ch,WEAR_HOLD_2);
         else if(GET_EQ(ch,WEAR_HOLD_1))
            retval = perform_remove(ch,WEAR_HOLD_1);
         else if(GET_EQ(ch,WEAR_SHIELD))
            retval = perform_remove(ch,WEAR_SHIELD);
         else if(GET_EQ(ch,WEAR_WIELD_2))
            retval = perform_remove(ch,WEAR_WIELD_2);
         else if(GET_EQ(ch,WEAR_WIELD_2))
            retval = perform_remove(ch,WEAR_WIELD_1);
         }
      }
   else if (GET_EQ(ch, where_pos))
      retval = perform_remove(ch, where_pos);

   if (GET_EQ(ch, where_pos))
      return;

   if (!wear_otrigger(obj, ch, where_pos))
      return;

   if(retval!=0)
      {
      if (visible)
         wear_message(ch, obj, where_pos);
      obj_from_char(obj);
      equip_char(ch, obj, where_pos);
      }
   else
      send_to_char(ch, "There is already something in your hands.\r\n");

   }

int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg)
   {
   int where_pos = -1;

   static char *keywords[] =
      {    /* \r to prevent explicit wearing */
         "\r!RESERVED!",  /* take */
         "finger",
         "finger_2",
         "neck",
         "neck_2",
         "body",
         "head",
         "legs",
         "feet",
         "hands",
         "arms",
         "shield",
         "about",
         "waist",
         "wrist",
         "wrist_2",
         "\r!RESERVED!",  /* wield 1 */
         "\r!RESERVED!",  /* wield 2 */
         "\r!RESERVED!",  /* hold 1 */
         "\r!RESERVED!",  /* hold 2 */
         "ear",           /* New EQ positions--Aleks */
         "ear_2",         /* New EQ positions--Aleks */
         "face",          /* New EQ positions--Aleks */
         "back",          /* New EQ positions--Aleks */
         "heart",         /* New EQ position--Faron/Nomikos */
         "\n"
      }
      ;

   if (!arg || !*arg)
      {
      if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
         where_pos = WEAR_FINGER_R;
      if (CAN_WEAR(obj, ITEM_WEAR_NECK))
         where_pos = WEAR_NECK_1;
      if (CAN_WEAR(obj, ITEM_WEAR_BODY))
         where_pos = WEAR_BODY;
      if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
         where_pos = WEAR_HEAD;
      if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
         where_pos = WEAR_LEGS;
      if (CAN_WEAR(obj, ITEM_WEAR_FEET))
         where_pos = WEAR_FEET;
      if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
         where_pos = WEAR_HANDS;
      if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
         where_pos = WEAR_ARMS;
      if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
         where_pos = WEAR_SHIELD;
      if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
         where_pos = WEAR_ABOUT;
      if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
         where_pos = WEAR_WAIST;
      if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
         where_pos = WEAR_WRIST_R;
      /* New EQ positions--Aleks */
      if (CAN_WEAR(obj, ITEM_WEAR_EAR))
         where_pos = WEAR_EAR_L;
      if (CAN_WEAR(obj, ITEM_WEAR_FACE))
         where_pos = WEAR_FACE;
      if (CAN_WEAR(obj, ITEM_WEAR_BACK))
         where_pos = WEAR_BACK;
      if (CAN_WEAR(obj, ITEM_WEAR_HEART))
         where_pos = WEAR_HEART;
      }
   else
      {
      if (((where_pos = search_block(arg, keywords, FALSE)) < 0) ||
              (*arg=='\r'))
         {
         send_to_char(ch, "'%s'?  What part of your body is THAT?\r\n", arg);
         return (-1);
         }
      }

   return (where_pos);
   }



ACMD(do_wear)
   {
   char *arg1=get_buffer(MAX_INPUT_LENGTH);
   char *arg2=get_buffer(MAX_INPUT_LENGTH);
   struct obj_data *obj, *next_obj;
   int where_pos, dotmode, items_worn = 0;

   two_arguments(argument, arg1, arg2);

   if (!*arg1)
      {
      send_to_char(ch, "Wear what?\r\n");
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }
   dotmode = find_all_dots(arg1);

   if (*arg2 && (dotmode != FIND_INDIV))
      {
      send_to_char(ch, "You can't specify the same body location for more than one item!\r\n");
      release_buffer(arg2);
      release_buffer(arg1);
      return;
      }
   if (dotmode == FIND_ALL)
      {
      for (obj = ch->carrying; obj; obj = next_obj)
         {
         next_obj = obj->next_content;
         if (CAN_SEE_OBJ(ch, obj) && (where_pos = find_eq_pos(ch, obj, 0))>=0)
            {
            if (IS_NPC(ch) && (!trait_info[GET_RACE(ch)].has_hands) && (where_pos == WEAR_SHIELD))
               send_to_char(ch, "Your lack of hands prevents you from using %s!\r\n",
                            GET_OBJ_NAME(obj));
	    else if (GET_OBJ_CSLOTS(obj) < 0)
	       send_to_char(ch, "You try wearing %s but it is too badly damaged!\r\n",
			    GET_OBJ_NAME(obj));
            else
               {
               items_worn++;
               perform_wear(ch, obj, where_pos, TRUE);
               }
            }
         }
      if (!items_worn)
         send_to_char(ch, "You don't seem to have anything wearable.\r\n");
      }
   else if (dotmode == FIND_ALLDOT)
      {
      if (!*arg1)
         {
         send_to_char(ch, "Wear all of what?\r\n");
         release_buffer(arg2);
         release_buffer(arg1);
         return;
         }
      if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
         {
         send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
         }
      else
         while (obj)
            {
            next_obj = get_obj_in_list_vis(ch, arg1, obj->next_content);
            if ((where_pos = find_eq_pos(ch, obj, 0)) >= 0)
               {
               if (IS_NPC(ch) && (!trait_info[GET_RACE(ch)].has_hands) && (where_pos==WEAR_SHIELD))
                  send_to_char(ch, "Your lack of hands prevents you from using %s!\r\n", 
                               GET_OBJ_NAME(obj));
	       else if (GET_OBJ_CSLOTS(obj) < 0)
		  send_to_char(ch, "You try wearing %s but it is too badly damaged!\r\n",
			       GET_OBJ_NAME(obj));
               else
                  perform_wear(ch, obj, where_pos, TRUE);
               }
            else
               act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
            obj = next_obj;
            }
      }
   else
      {
      if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
         {
         send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
         }
      else
         {
         if ((where_pos = find_eq_pos(ch, obj, arg2)) >= 0)
            { 
            if (IS_NPC(ch) && (!trait_info[GET_RACE(ch)].has_hands) && (where_pos==WEAR_SHIELD))
               send_to_char(ch, "Your lack of hands prevents you from using %s!\r\n",
                            GET_OBJ_NAME(obj));
	    else if (GET_OBJ_CSLOTS(obj) < 0)
	       send_to_char(ch, "You try wearing %s but it is too badly damaged!\r\n",
			    GET_OBJ_NAME(obj));
            else
               perform_wear(ch, obj, where_pos, TRUE);
            }
         else if (!*arg2)
            act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
         }
      }
   release_buffer(arg2);
   release_buffer(arg1);
   }



ACMD(do_wield)
   {
   struct obj_data *obj;
   char *arg=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, arg);
   if(IS_MONK(ch)&&GET_LEVEL(ch)<LVL_IMMORT)
      {
      send_to_char(ch,"Monks do not need weapons, only their bare hands!\n\r");
      release_buffer(arg);
      return;
      }

   if (!*arg)
      send_to_char(ch, "Wield what?\r\n");
   else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
      {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      }
   else
      {
      if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
         send_to_char(ch, "You can't wield that.\r\n");
      else if (IS_NPC(ch) && (!trait_info[GET_RACE(ch)].has_hands))
         send_to_char(ch, "But you don't have any hands to wield that!\r\n");
      else if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
         send_to_char(ch, "It's too heavy for you to use.\r\n");
      else if ((IS_CLERIC(ch)||IS_DRUID(ch))&&
               ((GET_OBJ_VAL(obj,3)==2) ||
                (GET_OBJ_VAL(obj,3)==3) ||
                (GET_OBJ_VAL(obj,3)==6) ||
                (GET_OBJ_VAL(obj,3)==7) ||
                (GET_OBJ_VAL(obj,3)==8) ||
                (GET_OBJ_VAL(obj,3)==9) ||
                (GET_OBJ_VAL(obj,3)==10)||
                (GET_OBJ_VAL(obj,3)==13)||
                (GET_OBJ_VAL(obj,3)==15)))
         send_to_char(ch, "The Gods that grant your powers forbid you to cut "
                      "flesh!!\r\n");
      else if ((IS_MAGIC_USER(ch))&&
                ((GET_OBJ_VAL(obj,3)==3) ||
                /*   (GET_OBJ_VAL(obj,3)==4) || */
                /*   (GET_OBJ_VAL(obj,3)==5) || */
                (GET_OBJ_VAL(obj,3)==10) ||
                (GET_OBJ_VAL(obj,3)==13) ||
                (GET_OBJ_VAL(obj,3)==15)))
         send_to_char(ch, "That weapon would interfere with your magic!\r\n");
      else if (GET_OBJ_CSLOTS(obj) < 0)
         send_to_char(ch, "You sense the futility in wielding a broken weapon.\r\n");
      else
	 perform_wear(ch, obj, WEAR_WIELD_1, TRUE);
      }
   release_buffer(arg);
   }



ACMD(do_grab)
   {
   struct obj_data *obj;
   char *arg=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, arg);

   if (!*arg)
      send_to_char(ch, "Hold what?\r\n");
   else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
      {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      }
   else if (IS_NPC(ch) && (!trait_info[GET_RACE(ch)].has_hands))
      send_to_char(ch, "But you don't have any hands to hold that!\r\n");
   /* Just in case a broken piece of equipment can be held. */
   else if (GET_OBJ_CSLOTS(obj) < 0)
      send_to_char(ch, "You try to hold %s but can't seem to figure out how.\r\n",
                   GET_OBJ_NAME(obj));
   else
      {
      if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
         perform_wear(ch, obj, WEAR_HOLD_1, TRUE);
      else
         {
         /* Pill modification--Aleks */
         if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) &&
                 (GET_OBJ_TYPE(obj) != ITEM_WAND) &&
                 (GET_OBJ_TYPE(obj) != ITEM_STAFF) &&
                 (GET_OBJ_TYPE(obj) != ITEM_SCROLL) &&
                 (GET_OBJ_TYPE(obj) != ITEM_POTION) &&
                 (GET_OBJ_TYPE(obj) != ITEM_PILL))
            send_to_char(ch, "You can't hold that.\r\n");
         else
            perform_wear(ch, obj, WEAR_HOLD_1, TRUE);
         }
      }
   release_buffer(arg);
   }



ACMD(do_remove)
{
  int i, dotmode, found;
  char *arg=get_buffer(MAX_INPUT_LENGTH);
  
  one_argument(argument, arg);
  
  if (!*arg)
  {
    send_to_char(ch, "Remove what?\r\n");
    release_buffer(arg);
    return;
  }
  dotmode = find_all_dots(arg);

  if (dotmode == FIND_ALL)
  {
    found = 0;
    for (i = NUM_WEARS-1; i >=0 ; i--)
    {
      if (i == WEAR_HEART)
	continue;
      if (GET_EQ(ch, i))
      {
	perform_remove(ch, i);
	found = 1;
      }
    }
    if (!found)
      send_to_char(ch, "You're not using anything.\r\n");
  }
  else if (dotmode == FIND_ALLDOT)
  {
    if (!*arg)
      send_to_char(ch, "Remove all of what?\r\n");
    else
    {
      found = 0;
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (i == WEAR_HEART)
	  continue;
	if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) &&
	    isname(arg, GET_EQ(ch, i)->name))
	{
	  perform_remove(ch, i);
	  found = 1;
	}
      }
      if (!found) {
	send_to_char(ch, "You don't seem to be using any %ss.\r\n", arg);
      }
    }
  }
  else
  {
    if (!get_object_in_equip_vis(ch, arg, ch->equipment, &i))
    {
      /* Check for graffiti. */
      if (starts_with("graffiti", arg)) {
	char *buf = get_buffer(MAX_STRING_LENGTH);
	strcpy(buf, argument);
	strtok(buf, " "); /* graffiti */
	char *token = strtok(NULL, " ");
	int rem = 0, count = 0;
	for (i = 0; i < num_graffiti; i++) {
	  if (graffiti[i].room_vnum != GET_ROOM_VNUM(IN_ROOM(ch))) {
	    continue;
	  }
	  count++;
	  if (!token && graffiti[i].author == GET_IDNUM(ch)) {
	    remove_graffiti_at(i--); /* "rem graffiti" */
	    rem++;
	  } else if (token && !str_cmp(token, "all") && (graffiti[i].author == GET_IDNUM(ch) || GET_LEVEL(ch) >= LVL_DETY)) {
	    remove_graffiti_at(i--); /* "rem graffiti all" */
	    rem++;
	  } else if (token && atoi(token) == count && (graffiti[i].author == GET_IDNUM(ch) || GET_LEVEL(ch) >= LVL_DETY)) {
	    remove_graffiti_at(i--); /* "rem graffiti 4" */
	    rem++;
	  }
	}
	if (rem) {
	  save_graffiti();
	  send_to_char(ch, "You wipe away the graffiti.\r\n");
	  mudlogf(CMP, GOD_LOG(ch), TRUE, "Graffiti: %s removed %d graffiti%s at #%ld (%s)", 
		  GET_NAME(ch), 
		  rem, rem > 1 ? "s" : "",
		  GET_ROOM_VNUM(IN_ROOM(ch)), 
		  world[IN_ROOM(ch)].name
		  );
	} else {
	  send_to_char(ch, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
	}
	release_buffer(buf);
	release_buffer(arg);
	return;
      }
      
      send_to_char(ch,"You don't seem to be using %s %s.\r\n", AN(arg),arg);
    }
    else
      if (i == WEAR_HEART)
	send_to_char(ch,"You cannot remove your heartworn!\r\n");
      else
	perform_remove(ch, i);
  }
  release_buffer(arg);
}

/* Refuelable light mod--Aleks */
ACMD(do_refuel)
   {
   char *arg1=get_buffer(MAX_INPUT_LENGTH);
   char *arg2=get_buffer(MAX_INPUT_LENGTH);
   struct obj_data *from_obj = NULL, *to_obj = NULL;
   int amount, found;

   two_arguments(argument, arg1, arg2);

   if (!*arg1) /* no arguments */
      {
      send_to_char(ch,"What do you want to refuel?  And what are you filling it with?\r\n");
      }
   else if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
      {
      send_to_char(ch, "You can't find it!\r\n");
      }
   /* make sure item is a lightsource and refuelable */
   else if(GET_OBJ_TYPE(to_obj) != ITEM_LIGHT && GET_OBJ_VAL(to_obj, 1) != 0)
      {
      act("You can't refuel $p!", FALSE, ch, to_obj, 0, TO_CHAR);
      }
   else if (!*arg2) /* no 2nd argument */
      {
      act("What do you want to refuel $p with?", FALSE,ch,to_obj, 0, TO_CHAR);
      }
   else if (!(from_obj = get_obj_in_list_vis(ch, arg2, ch->carrying)))
      {
      send_to_char(ch, "You can't find it!\r\n");
      }
   /* make sure second item is a fuel source */
   else if(GET_OBJ_TYPE(from_obj) != ITEM_FUEL)
      {
      act("You can't refuel something with $p.",FALSE,ch,from_obj,0, TO_CHAR);
      }
   /* make sure it is the correct type of fuel */
   else if( GET_FUEL_TYPE(from_obj) != GET_FUEL_TYPE(to_obj))
      {
      act("That is the wrong type of fuel for $p!",FALSE,ch,to_obj,0,TO_CHAR);
      }
   /* check if the light source even needs fuel */
   else if( GET_CUR_FUEL(to_obj) >= GET_MAX_FUEL(to_obj) )
      {
      act("$p is already fully fueled.", FALSE, ch, to_obj, 0, TO_CHAR);
      }
   /* make sure there is still fuel available in the fuel source. */
   else if( (found = GET_OBJ_VAL(from_obj, 0)) <= 0 )
      {
      act("But $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
      }
   else
      {
      amount = GET_MAX_FUEL(to_obj) - GET_CUR_FUEL(to_obj);
      /* More fuel than light source can handle */
      if(amount < found)
         {
         char *mesg=get_buffer(256);
         GET_CUR_FUEL(to_obj) = GET_MAX_FUEL(to_obj);
         GET_OBJ_VAL(from_obj, 0) -= amount;
         sprintf(mesg, "You refuel $p with %s from %s.",
                 fuels[GET_FUEL_TYPE(from_obj)], GET_OBJ_NAME(from_obj));
         act(mesg, FALSE, ch, to_obj, 0, TO_CHAR);
         act("$p is completely refueled.", FALSE, ch, to_obj, 0, TO_CHAR);
         release_buffer(mesg);
         }
      else
         {
         char *mesg=get_buffer(256);
         GET_CUR_FUEL(to_obj) += found;
         GET_OBJ_VAL(from_obj, 0) = 0;
         sprintf(mesg, "You refuel $p with %s from %s.",
                 fuels[GET_FUEL_TYPE(from_obj)], GET_OBJ_NAME(from_obj));
         act(mesg, FALSE, ch, to_obj, 0, TO_CHAR);
         release_buffer(mesg);
         }
      }
   release_buffer(arg2);
   release_buffer(arg1);
   }

ACMD(do_pull)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      send_to_char(ch, "Pull what?\r\n");
      release_buffer(arg);
      return;
      }
   /* for now only pull pin will work and must be wielding a grenade */
   if(!str_cmp(arg,"pin"))
      {
      if(GET_EQ(ch, WEAR_WIELD_1)||GET_EQ(ch, WEAR_WIELD_2))
         {
         if(GET_EQ(ch,WEAR_WIELD_1)&&(GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD_1)) == ITEM_GRENADE))
            {
            send_to_char(ch, "You pull the pin, the grenade is activated!\r\n");
            SET_BIT(GET_OBJ_EXTRA(GET_EQ(ch, WEAR_WIELD_1)), ITEM_LIVE_GRENADE);
            }
         else if(GET_EQ(ch,WEAR_WIELD_2)&&(GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD_2)) == ITEM_GRENADE))
            {
            send_to_char(ch,"You pull the pin, the grenade is activated!\r\n");
            SET_BIT(GET_OBJ_EXTRA(GET_EQ(ch, WEAR_WIELD_2)), ITEM_LIVE_GRENADE);
            }
         else
            send_to_char(ch, "That's NOT a grenade!\r\n");
         }
      else
         send_to_char(ch, "You aren't wielding anything!\r\n");
      }

   /* put other 'pull' options here later */
   release_buffer(arg);
   return;
   }

ACMD(do_repair)
   {
   struct obj_data *repair;
   int percent, prob;
   char *arg;

   if(!IS_NPC(ch)&& (GET_SKILL(ch, SKILL_REPAIR) <= 0))
      {
      send_to_char(ch, "You don't know how to repairs things!\r\n");
      return;
      }

   if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM)) {
     send_to_char(ch, "Sorry.\r\n");
     return;
   }

   arg=get_buffer(256);
   one_argument(argument, arg);

   if (!*arg)
      {
      send_to_char(ch, "Repair what?\r\n");
      release_buffer(arg);
      return;
      }

   if (!(repair = get_obj_in_list_vis(ch, arg, ch->carrying)))
      {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      release_buffer(arg);
      return;
      }

   release_buffer(arg);

   if(IS_OBJ_STAT(repair,ITEM_NO_REPAIR))
      {
      send_to_char(ch, "You can not repair this.\r\n");
      return;
      }

   if(IS_NPC(ch)||(GET_LEVEL(ch)>LVL_IMMORT))
      {
      percent=1;
      prob=100;
      }
   else
      {
      percent = number(1, 101);
      prob = GET_SKILL(ch, SKILL_REPAIR);
      }

   if ((GET_OBJ_CSLOTS(repair) == 0) && (GET_OBJ_TSLOTS(repair) == 0))
      {
      act("$p seems to be indestructable!", FALSE, ch, repair, 0, TO_CHAR);
      return;
      }

   if (GET_OBJ_CSLOTS(repair) == GET_OBJ_TSLOTS(repair))
      {
      act("$p seems to be in perfect condition!", FALSE,ch,repair,0,TO_CHAR);
      return;
      }

   if (GET_OBJ_CSLOTS(repair) < 0)
      {
      act("You completely ruin $p and it crumbles away!", FALSE, ch,
          repair, 0, TO_CHAR);
      act("$n tries to repair $p, but it crumbles away!", TRUE, ch,
          repair, 0, TO_ROOM);
      extract_obj(repair);
      return;
      }

   if (percent > prob)
      {
      act("Your clumsy attempt at repairing $p damages it even more!",
          FALSE, ch, repair, 0, TO_CHAR);
      act("$n tries to repair $p, but only makes it worse!", TRUE, ch,
          repair, 0, TO_ROOM);
      GET_OBJ_CSLOTS(repair) -= 2;
      GET_OBJ_TSLOTS(repair) -= 2;
      improve_skill(ch,SKILL_REPAIR,USE_FAIL);
      }
   else
      {
      act("You repair $p and it looks in excellent condition again!",
          FALSE, ch, repair, 0, TO_CHAR);
      act("$n repairs $p, making it as good as new again!", TRUE, ch,
          repair, 0, TO_ROOM);
      if(IS_NPC(ch))
         GET_OBJ_TSLOTS(repair) -= 2;
      else
         GET_OBJ_TSLOTS(repair) -= 3;
      improve_skill(ch,SKILL_REPAIR,USE_PASS);
      GET_OBJ_CSLOTS(repair) = GET_OBJ_TSLOTS(repair);
      }
   if(GET_OBJ_TSLOTS(repair)<1)
      {
      act("$p cannot stand another repair and crumbles to dust", TRUE,ch,
          repair,0,TO_ROOM);
      extract_obj(repair);
      }
   }


ACMD(do_recharge)
   {
   struct obj_data *recharge;
   char *arg;

   /*
      if(!IS_NPC(ch)&& (GET_SKILL(ch, SKILL_RECHARGE) <= 0))
      {
      send_to_char(ch, "You don't know how to repairs things!\r\n");
      return;
      }
      */
   arg=get_buffer(256);
   one_argument(argument, arg);

   if (!*arg)
      {
      send_to_char(ch, "Recharge what?\r\n");
      release_buffer(arg);
      return;
      }

   if (!(recharge = get_obj_in_list_vis(ch, arg, ch->carrying)))
      {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if((GET_OBJ_TYPE(recharge)!=ITEM_WAND)&&
           (GET_OBJ_TYPE(recharge)!=ITEM_STAFF))
      {
      send_to_char(ch, "That is not a charged item!!!\r\n");
      return;
      }

   GET_OBJ_VAL(recharge,2)=GET_OBJ_VAL(recharge,1);
   act("You recharge $s and it looks in excellent condition again!",
       FALSE, ch, recharge, 0, TO_CHAR);
   act("$n recharge $p, making it as good as new again!", TRUE, ch,
       recharge, 0, TO_ROOM);
   }

ACMD(do_assemble)
   {
   long         lVnum = NOTHING;
   struct obj_data *pObject = NULL;
   char *buf;

   skip_spaces(&argument);

   if (*argument == '\0')
      {
      buf = get_buffer(256);
      strcpy(buf,CMD_NAME);
      send_to_char(ch, "%s what?\r\n", CAP(buf));
      release_buffer(buf);
      return;
      }
   else if ((lVnum = assemblyFindAssembly(argument)) < 0)
      {
      send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
      return;
      }
   else if (assemblyGetType(lVnum) != subcmd)
      {
      send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
      return;
      }
   else if (!assemblyCheckComponents(lVnum, ch))
      {
      send_to_char(ch, "You haven't all the parts to make that.\r\n");
      return;
      }

   /* Create the assembled object. */
   if ((pObject = read_object(lVnum, VIRTUAL)) == NULL )
      {
      send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
      return;
      }

   /* Now give the object to the character. */
   obj_to_char(pObject, ch);
   
   buf= get_buffer(MAX_STRING_LENGTH);
   /* Tell the character they made something. */
   sprintf(buf, "You %s $p.", CMD_NAME);
   act(buf, FALSE, ch, pObject, NULL, TO_CHAR);

   /* Tell the room the character made something. */
   sprintf(buf, "$n %ss $p.", CMD_NAME);
   act(buf, FALSE, ch, pObject, NULL, TO_ROOM);
   release_buffer(buf);
   }

ACMD(do_castout)
   {
   int fail, pos;

   if (PLR_FLAGGED(ch, PLR_FISHING))
      {
      send_to_char(ch, "You are already fishing!\r\n");
      return;
      }

   if (GET_EQ(ch, WEAR_HOLD_1) &&
       (GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD_1)) == ITEM_POLE)) 
      {
      pos = WEAR_HOLD_1;
      }
   else if (GET_EQ(ch, WEAR_HOLD_2) &&                                  
            (GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD_2)) == ITEM_POLE))
      {
      pos = WEAR_HOLD_2;
      }
   else
      {
      send_to_char(ch, "You need to be holding a fishing pole first.\r\n");
      return;
      }

   if (!ROOM2_FLAGGED(ch->in_room, ROOM2_SALTWATER_FISH) &&
       !ROOM2_FLAGGED(ch->in_room, ROOM2_FRESHWATER_FISH)) 
      {
      send_to_char(ch, "This is not a good place to fish, you'll want to "
                   "find a better spot.\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_CHARM))
      {
      send_to_char(ch, "You toss your pole.. err.. your line into the water.\r\n");
      act("$n casts $s pole into the water with a sure-fire "
          "lack of finesse, woops!", FALSE, ch, 0, 0, TO_ROOM);
      obj_to_room(unequip_char(ch, pos), IN_ROOM(ch));
      return;
      }

   fail = number(1, 100);

   if (fail > 50+(GET_SKILL(ch, SKILL_FISHING)/2))
      {
      send_to_char(ch, "You pull your arm back and try to cast out your line, "
                  "but it gets all tangled up.\r\nTry again.\r\n");
      act("$n pulls $s arm back, trying to cast $s fishing line out into the "
          "water, but ends up just a bit tangled.",
          FALSE, ch, 0, 0, TO_ROOM);
      GET_WAIT_STATE(ch) = 3 RL_SEC; /* lag them for 3 seconds */
      return;
      }

   /* Ok, now they've gone through the checks, now set them fishing */
   SET_BIT(PLR_FLAGS(ch), PLR_FISHING);
   send_to_char(ch, "You cast your line out into the water, hoping for a bite.\r\n");
   act("$n casts $s line out into the water, hoping to catch some food.",
        FALSE, ch, 0, 0, TO_ROOM);
   return;
   }

ACMD(do_reelin)
   {
   int success;
   long catch_junk, f_num, fish_num;
   struct obj_data *fish;

   if (!PLR_FLAGGED(ch, PLR_FISHING)) 
      {
      send_to_char(ch, "You aren't even fishing!\r\n");
      return;
      }
   if (!PLR_FLAGGED(ch, PLR_FISH_ON))
      {
      send_to_char(ch, "You reel in your line, but alas... nothing on the end.\r\n"
                   "Better luck next time.\r\n");
      REMOVE_BIT(PLR_FLAGS(ch), PLR_FISHING);
      act("$n reels $s line in, but with nothing on the end.",
           FALSE, ch, 0, 0, TO_ROOM);
      return;
      }

   /* Ok, they are fishing and have a fish on */
   success = number(1, 10);

   REMOVE_BIT(PLR_FLAGS(ch), PLR_FISHING);
   REMOVE_BIT(PLR_FLAGS(ch), PLR_FISH_ON);

   if (success <= (6 - (GET_SKILL(ch, SKILL_FISHING) / 30)))
      {
      send_to_char(ch, "You reel in your line, putting up a good fight, but you "
                   "lose him!\r\nTry again?\r\n");
      act("$n reels $s line in, fighting with whatever is on the end, but loses "
          "the catch.", FALSE, ch, 0, 0, TO_ROOM);
      improve_skill(ch, SKILL_FISHING, USE_FAIL);
      return;
      }

   improve_skill(ch, SKILL_FISHING, USE_PASS);
   if (ROOM2_FLAGGED(ch->in_room, ROOM2_SALTWATER_FISH)) 
      {
      catch_junk = number(1, 3);
      if (catch_junk == 1)
         fish_num = number(FISHING_JUNK_BASE, FISHING_JUNK_BASE+NUM_FISH_JUNK-1);
      else
         fish_num = number(FRESHWATER_FISH_BASE, FRESHWATER_FISH_BASE+NUM_FW_FISH-1);
      f_num = real_object(fish_num);
      if (f_num == -1)
         {
         send_to_char(ch, "You reel in a bug!  Better report it!\r\n");
         return;
         }
      fish = read_object(f_num, REAL);
      act("Wow! $n reels in a helluva catch! Looks like $p!",
          FALSE, ch, fish, 0, TO_ROOM);
      send_to_char(ch, "You reel in %s! Nice catch!\r\n", fish->short_description);
      obj_to_char(fish, ch);
      return;
      } 
   else if (ROOM2_FLAGGED(ch->in_room, ROOM2_FRESHWATER_FISH))
      {
      catch_junk = number(1, 3);
      /* 33% chance of reeling in junk */
      if (catch_junk == 1)
         fish_num = number(FISHING_JUNK_BASE, FISHING_JUNK_BASE+NUM_FISH_JUNK-1);
      else
         fish_num = number(SALTWATER_FISH_BASE, SALTWATER_FISH_BASE+NUM_SW_FISH-1);
      f_num = real_object(fish_num);
      if (f_num == -1)
         {
         send_to_char(ch, "You reel in a bug!  Better report it!\r\n");
         return;
         }
      fish = read_object(f_num, REAL);
      act("Wow! $n reels in a helluva catch! Looks like $p!",
          FALSE, ch, fish, 0, TO_ROOM);
      send_to_char(ch, "You reel in %s! Nice catch!\r\n", fish->short_description);
      obj_to_char(fish, ch);
      return;
      }
   else
      send_to_char(ch, "You should never see this message, please report it.\r\n");
   return;
   }
