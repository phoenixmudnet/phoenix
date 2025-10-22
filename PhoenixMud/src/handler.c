/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD * 
*  Usage: internal funcs: moving and finding chars/objs                   * 
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
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "queue.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"

#define WHITESPACE " \t"

/* external vars */
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern char *MENU;
extern struct zone_data *zone_table;        /* zone table  */
extern int top_of_zone_table;
extern int port;
extern struct char_data *combat_list;
extern int item_decay;
extern int max_npc_corpse_time, max_pc_corpse_time;
extern struct str_app_type str_app[];
extern char last_command[MAX_STRING_LENGTH];

/* external functions */
int Crash_is_unrentable(struct obj_data * obj);
void free_char(struct char_data * ch);
void stop_fighting(struct char_data * ch);
void clearMemory(struct char_data * ch);
void reset_casting_data(struct char_data *ch);
void purge_zone(int zone);
void reset_zone(int zone);
void dismount_char(struct char_data *ch);
int  invalid_class(struct char_data *ch, struct obj_data *obj);
int  invalid_race(struct char_data *ch, struct obj_data *obj);
void perform_remove(struct char_data * ch, int pos);
void rage_check(struct char_data *ch);
ACMD(do_return);
void save_corpses(void);
void Crash_heartwornsave(struct char_data *ch);
/*
 * begin add - Bon 07/25/97 
 * room teleport code from Phoenix 3 
 */
void teleport_update(struct char_data *ch);
/*
 * end   add - Bon 07/25/97 
 */
void    make_scraps(struct obj_data *obj, int r_num);

char *fname(char *namelist)
   {
   static char holder[60];
   register char *point;

   for (point = holder; isalpha((int)*namelist); namelist++, point++)
      *point = *namelist;

   *point = '\0';

   return (holder);
   }


int isname(const char *str, const char *namelist)
{
  char *newlist;
  char *curtok;

  if (!strcmp(str, namelist)) /* the easy way */
    return 1;

  newlist = strdup(namelist); /* make a copy since strtok 'modifies' strings */
  for(curtok = strtok(newlist, WHITESPACE); curtok; curtok = strtok(NULL, WHITESPACE))
     if(curtok && is_abbrev((char *)str, curtok))
       {
       free(newlist);
       return 1;
       }
     free(newlist);
     return 0;
}

/* Stock isname(). */

int is_name(const char *str, const char *namelist)
   {
   const char *curname, *curstr;

   /* MANWE: Handle an empty str */
   if (*str == '\0')
      return 0;

   curname = namelist;
   for (;;)
      {
      for (curstr = str;; curstr++, curname++)
         {
         if (!*curstr)
            {
            if(CHECK==1)
               log("1 %s | %s | %s | %s",curstr,curname,str,namelist);
            return (1);
            }
         if (*curstr=='.' && !isalpha((int)*curname))
            {
            if(CHECK==1)
               log("2 %s | %s | %s | %s",curstr,curname,str,namelist);
            return (1);
            }

         if (!*curname)
            {
            if(CHECK==1)
               log("3 %s | %s | %s | %s",curstr,curname,str,namelist);
            return (0);
            }

         if (*curname == ' ')
            {
            if(CHECK==1)
               log("4 %s | %s | %s | %s",curstr,curname,str,namelist);
            break;
            }

         if (LOWER(*curstr) != LOWER(*curname))
            {
            if(CHECK==1)
               log("5 %s | %s | %s | %s",curstr,curname,str,namelist);
            break;
            }
         }

      /* skip to next name */

      for (; isalnum((int)*curname) || (*curname=='_'); curname++)
         ;
      if (!*curname)
         return (0);
      curname++;
      while (*curname == ' ')
         curname++;
      }
   if(CHECK==1)
      log("6 %s | %s | %s | %s",curstr,curname,str,namelist);

   }


void affect_modify(struct char_data * ch, byte loc, long mod, bitvector_t bitv,
                   bool add)
   {
   switch(loc)
      {
   case APPLY_EAT_SPELL:
      break;
   case APPLY_IMMUNE:
      if(add)
         SET_BIT(IMMUNE(ch),mod);
      else
         REMOVE_BIT(IMMUNE(ch),mod);
      break;
   case APPLY_RESIST:
      if(add)
         SET_BIT(RESIST(ch),mod);
      else
         REMOVE_BIT(RESIST(ch),mod);
      break;
   case APPLY_SUSC:
      if(add)
         SET_BIT(SUCCEPT(ch),mod);
      else
         REMOVE_BIT(SUCCEPT(ch),mod);
      break;
   case APPLY_FLY:
      if(add)
         {
         SET_BIT(AFF_FLAGS(ch), AFF_FLY);
         }
      else
         {
         REMOVE_BIT(AFF_FLAGS(ch), AFF_FLY);
         }
      break;
   default:
      if(loc==APPLY_AFF2)
         {
         if (add)
            {
            SET_BIT(AFF2_FLAGS(ch), bitv);
            }
         else
            {
            REMOVE_BIT(AFF2_FLAGS(ch), bitv);
            }
         }
      else
         {
         if (add)
            {
            SET_BIT(AFF_FLAGS(ch), bitv);
            }
         else
            {
            REMOVE_BIT(AFF_FLAGS(ch), bitv);
            }
         }

      if (!add)
         mod= -mod;


      switch (loc)
         {
         /*
          * these are used elsewhere or not at all (legacy type stuff)
          */
      case APPLY_NONE:
      case APPLY_CLASS:
      case APPLY_LEVEL:
      case APPLY_SEX:
      case APPLY_GOLD:
      case APPLY_EXP:
      case APPLY_AFF2:
         break;

      case APPLY_STR:
         GET_STR(ch) += mod;
         break;
      case APPLY_DEX:
         GET_DEX(ch) += mod;
         break;
      case APPLY_INT:
         GET_INT(ch) += mod;
         break;
      case APPLY_WIS:
         GET_WIS(ch) += mod;
         break;
      case APPLY_CON:
         GET_CON(ch) += mod;
         break;
      case APPLY_CHA:
         GET_CHA(ch) += mod;
         break;

      case APPLY_AGE:
         ch->player.time.birth -= (mod * SECS_PER_MUD_YEAR);
         break;

      case APPLY_CHAR_WEIGHT:
         GET_WEIGHT(ch) += mod;
         break;

      case APPLY_CHAR_HEIGHT:
         GET_HEIGHT(ch) += mod;
         break;

      case APPLY_MANA:
         GET_MAX_MANA(ch) += mod;
         break;

      case APPLY_HIT:
         GET_MAX_HIT(ch) += mod;
         break;

      case APPLY_MOVE:
         GET_MAX_MOVE(ch) += mod;
         break;

      case APPLY_AC:
         GET_AC(ch) += mod;
         break;

      case APPLY_HITROLL:
         GET_HITROLL(ch) += mod;
         break;

      case APPLY_DAMROLL:
         GET_DAMROLL(ch) += mod;
         break;

      case APPLY_SAVING_PARA:
         GET_SAVE(ch, SAVING_PARA) += mod;
         break;

      case APPLY_SAVING_ROD:
         GET_SAVE(ch, SAVING_ROD) += mod;
         break;

      case APPLY_SAVING_PETRI:
         GET_SAVE(ch, SAVING_PETRI) += mod;
         break;

      case APPLY_SAVING_BREATH:
         GET_SAVE(ch, SAVING_BREATH) += mod;
         break;

      case APPLY_SAVING_SPELL:
         GET_SAVE(ch, SAVING_SPELL) += mod;
         break;

      case APPLY_LIGHT:
         world[IN_ROOM(ch)].light += mod;
         GET_LIGHT(ch) += mod;
         break;

      case APPLY_SPELL_FAIL:
         GET_SPELL_FAIL(ch) += mod;
         break;

      default:
         log("SYSERR: Unknown apply adjust attempt (handler.c, affect_modify). (%d)",loc);
         break;

         }
      /* switch */
      }

   }

/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
void affect_total(struct char_data * ch)
   {
   struct affected_type *af;
   int i, j;

   for (i = 0; i < NUM_WEARS; i++)
      {
      if (GET_EQ(ch, i))
         for (j = 0; j < MAX_OBJ_AFFECT; j++)
            affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
                          GET_EQ(ch, i)->affected[j].modifier,
                          GET_EQ(ch, i)->obj_flags.bitvector, FALSE);
      }


   for (af = ch->affected; af; af = af->next)
      affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);

   /*
    * do NOT put anything in here that can force an update, a player with +hp
    * items who is close to death can get killed here
    */
   ch->aff_abils = ch->real_abils;

   for (i = 0; i < NUM_WEARS; i++)
      {
      if (GET_EQ(ch, i))
         for (j = 0; j < MAX_OBJ_AFFECT; j++)
            affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
                          GET_EQ(ch, i)->affected[j].modifier,
                          GET_EQ(ch, i)->obj_flags.bitvector, TRUE);
      }


   for (af = ch->affected; af; af = af->next)
      affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);

   /* Make certain values are between 0..25, not < 0 and not > 25! */

   i = (IS_NPC(ch) ? 125 : 25);

   GET_DEX(ch) = MAX(0, MIN(GET_DEX(ch), i));
   GET_INT(ch) = MAX(0, MIN(GET_INT(ch), i));
   GET_WIS(ch) = MAX(0, MIN(GET_WIS(ch), i));
   GET_CON(ch) = MAX(0, MIN(GET_CON(ch), i));
   GET_STR(ch) = MAX(0, MIN(GET_STR(ch) ,i));
   GET_CHA(ch) = MAX(0, MIN(GET_CHA(ch) ,i));

   set_racial_traits(ch);
   }


/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void affect_to_char(struct char_data * ch, struct affected_type * af)
   {
   struct affected_type *affected_alloc;

   CREATE(affected_alloc, struct affected_type, 1);

   *affected_alloc = *af;
   affected_alloc->next = ch->affected;
   ch->affected = affected_alloc;

   affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
   affect_total(ch);
   check_weapon_weight(ch);
   }



/*
 * Remove an affected_type structure from a char (called when duration 
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls 
 * affect_location_apply 
 */
void affect_remove(struct char_data * ch, struct affected_type * af)
   {
   struct affected_type *temp;

   if(ch->affected==NULL)
      {
      log("SYSERR: affect_remove used on %s: no affects present!",
          GET_NAME(ch));
      core_dump();
      return;
      }

   affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);
   REMOVE_FROM_LIST(af, ch->affected, next);
   free(af);
   affect_total(ch);
   }


/* Remove all affects from a character */
void affect_remove_all(struct char_data *ch)
   {
   if (ch->affected != NULL)
      {
      while(ch->affected)
         {
         affect_remove(ch,ch->affected);
         }
      }
   }



/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(struct char_data * ch, int type)
   {
   struct affected_type *hjp, *next;

   for (hjp = ch->affected; hjp; hjp = next)
      {
      next = hjp->next;
      if (hjp->type == type)
         affect_remove(ch, hjp);
      }
   check_weapon_weight(ch);
   }



/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX), FALSE indicates 
 * not affected 
 */
bool affected_by_spell(struct char_data * ch, int type)
   {
   struct affected_type *hjp;

   for (hjp = ch->affected; hjp; hjp = hjp->next)
      if (hjp->type == type)
         return TRUE;

   return FALSE;
   }

struct affected_type* get_affected_by_spell(struct char_data * ch, int type)
   {
   struct affected_type *hjp;

   for (hjp = ch->affected; hjp; hjp = hjp->next)
      if (hjp->type == type)
         return hjp;

   return 0x0;
   }



void affect_join(struct char_data * ch, struct affected_type * af,
                 bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
   {
   struct affected_type *hjp, *next;
   bool found = FALSE;

   for (hjp = ch->affected; !found && hjp; hjp = next)
      {
      next = hjp->next;

      if ((hjp->type == af->type) && (hjp->location == af->location))
         {
         if (add_dur)
            af->duration += hjp->duration;
         if (avg_dur)
            af->duration /= 2;

         if (add_mod)
            af->modifier += hjp->modifier;
         if (avg_mod)
            af->modifier /= 2;

         affect_remove(ch, hjp);
         affect_to_char(ch, af);
         found = TRUE;
         }
      }
   if (!found)
      affect_to_char(ch, af);
   }



/* ************
 * ROOM AFFECTS
 * ************/
void affect_modify_room(struct room_data *rm, byte loc, long mod,
                        bitvector_t bitv, bool add)
   {
   switch(loc)
      {

   default:
      if (add)
         {
         SET_BIT(rm->room_flags, bitv);
         }
      else
         {
         REMOVE_BIT(rm->room_flags, bitv);
         mod = -mod;
         }

      switch (loc)
         {
      case APPLY_NONE:
         break;

      case APPLY_LIGHT:
         rm->light += mod;
         break;

      default:
         log("SYSERR: Unknown apply adjust attempt (handler.c, "
             "affect_modify_room). (%d)",loc);
         break;

         }
      /* switch */
      }

   }

/* This updates a room by subtracting everything it is affected by,     */
/* restoring original abilities, and then affecting all again           */
void affect_total_room(struct room_data *rm)
   {
   struct room_affected_type *af;


   for (af = rm->affected; af; af = af->next)
      affect_modify_room(rm, af->location, af->modifier, af->bitvector, FALSE);
   /*
    * code to set room to default stats should be here if possible
    */

   for (af = rm->affected; af; af = af->next)
      affect_modify_room(rm, af->location, af->modifier, af->bitvector, TRUE);

   }



/* Insert an affect_type in a room_data structure
   Automatically sets apropriate bits and applies */
void affect_to_room(struct room_data *rm, struct room_affected_type * af)
   {
   struct room_affected_type *affected_alloc;
   struct queue_event *this;

   CREATE(affected_alloc, struct room_affected_type, 1);
   *affected_alloc = *af;
   affected_alloc->next = rm->affected;
   rm->affected = affected_alloc;

   affect_modify_room(rm, af->location, af->modifier, af->bitvector, TRUE);
   affect_total_room(rm);
   this = add_function_to_queue(af->duration*75,0,0,2,affect_from_room,
                                rm,af->type);
   affected_alloc->events=this;
   }



/*
 * Remove a room_affected_type structure from a room (called when duration 
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls 
 * affect_location_apply 
 */
void affect_remove_room(struct room_data *rm, struct room_affected_type * af)
   {
   struct room_affected_type *temp;

   if(rm->affected==NULL)
      {
      log("SYSERR: Room affects missing!! (in affect_remove_room)");
      core_dump();
      return;
      }

   if(af->events)
      {
      del_event_queue(af->events);
      }

   affect_modify_room(rm, af->location, af->modifier, af->bitvector, FALSE);
   REMOVE_FROM_LIST(af, rm->affected, next);
   free(af);
   /*    affect_total_room(rm);  */
   }



/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_room(struct room_data *rm, int type)
   {
   struct room_affected_type *hjp, *next;
   if(!rm->affected)
      return;
   for (hjp = rm->affected; hjp; hjp = next)
      {
      next = hjp->next;
      if (hjp->type == type)
         affect_remove_room(rm, hjp);
      }
   }



/*
 * Return if a room is affected by a spell (SPELL_XXX), NULL indicates 
 * not affected 
 */
bool affected_by_spell_room(struct room_data *rm, int type)
   {
   struct room_affected_type *hjp;

   for (hjp = rm->affected; hjp; hjp = hjp->next)
      if (hjp->type == type)
         return TRUE;

   return FALSE;
   }



void affect_join_room(struct room_data *rm, struct room_affected_type * af,
                      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
   {
   struct room_affected_type *hjp;
   bool found = FALSE;

   for (hjp = rm->affected; !found && hjp; hjp = hjp->next)
      {

      if ((hjp->type == af->type) && (hjp->location == af->location))
         {
         if (add_dur)
            af->duration += hjp->duration;
         if (avg_dur)
            af->duration /= 2;

         if (add_mod)
            af->modifier += hjp->modifier;
         if (avg_mod)
            af->modifier /= 2;

         affect_remove_room(rm, hjp);
         affect_to_room(rm, af);
         found = TRUE;
         }
      }
   if (!found)
      affect_to_room(rm, af);
   }

/* ******************
 * End of affect code
 * ******************/

/* move a player out of a room */
void char_from_room(struct char_data * ch)
   {
   struct char_data *temp;

   if (ch == NULL || IN_ROOM(ch) == NOWHERE)
      {
      log("SYSERR: NULL or NOWHERE in handler.c, char_from_room");
      exit(1);
      }

   /* If you're picking a door exit or an object on the ground, and you're about to
    * move rooms, you stop the picking lock.
    */
   if (AFF2_FLAGGED(ch, AFF2_PICKING_STAY)) {
      send_to_char(ch, "You abandon picking the lock.\r\n");
      REMOVE_BIT(AFF2_FLAGS(ch), AFF2_PICKING_STAY);
      /* Clear the pick lock skill lag. */
      GET_WAIT_STATE(ch) = 0;
      }

   if(FURNITURE(ch))
      char_from_object(ch,FURNITURE(ch));
   if (FIGHTING(ch) != NULL)
      stop_fighting(ch);
   if (IN_ROOM(ch) != NOWHERE) {
     world[IN_ROOM(ch)].light -= GET_LIGHT(ch);
   }

   if (IN_ROOM(ch) != NOWHERE) {
     REMOVE_FROM_LIST(ch, world[IN_ROOM(ch)].people, next_in_room);
   }
   ch->next_in_room = NULL;
   }

int real_zone2(int target_zone)
   {
   int zone;

   if (target_zone < 0 || target_zone > top_of_zone_table)
      return 0;

   for (zone = target_zone; zone >= 0; zone--)
      if (zone_table[zone].top / 100 < target_zone)
         break;

   return zone + 1;
   }

/* place a character in a room */
void char_to_room(struct char_data * ch, room_rnum room)
   {
   int zone;
   int vnum;

   if (ch==NULL || room < 0 || room > top_of_world)
      log("SYSERR: Illegal value(s) passed to char_to_room(Room:%ld/%ld ch:%s)",
          room,top_of_world,ch?GET_NAME(ch):"NULL");
   else
      {
      ch->next_in_room = world[room].people;
      world[room].people = ch;
      IN_ROOM(ch) = room;

      /* Get the vnum for this room rnum. */
      vnum = GET_ROOM_VNUM(room);
      /* Ensure the vnum is within the bounds of rooms we're recording. */
      if (vnum >= 0 && vnum < EXPLORED_TOP_VNUM) {
	/* If this room isn't previous explored, mark it and update the total count. */
	char *vnums =  ch->player_specials->explored_vnums;
	int *total = &ch->player_specials->explored_total;
	int mask = 1 << (vnum%8);
	if (!(vnums[vnum/8] & mask)) {
	  vnums[vnum/8] |= 1 << (vnum%8);
	  (*total)++;
	}
      }

      /** real_zone2 is another method of calculatin zone number **/

      if (!IS_NPC(ch))
         {
         zone = world[room].zone;
         if ((real_zone2(zone)) && ZONE_FLAGGED(zone, Z_IDLE))
            {
            mudlogf(CMP, GET_INVIS_LEV(ch), TRUE,
              "Idle zone %d activated by %s: %s", zone, GET_NAME(ch), zone_table[zone].name);
            reset_zone(zone);
            zone_table[zone].idle_time = 0;
            zone_table[zone].age = 0;
            }
         }


      world[IN_ROOM(ch)].light += GET_LIGHT(ch);
      /*
       * Stop fighting now, if we left.
       */
      if(FIGHTING(ch) && IN_ROOM(ch)!=IN_ROOM(FIGHTING(ch)))
         {
         stop_fighting(FIGHTING(ch));
         rage_check(FIGHTING(ch));
         stop_fighting(ch);
         rage_check(ch);
         }
      /*
       * room teleport code from Phoenix 3 
       */
      teleport_update(ch);
      }
   }


/* give an object to a char   */
void obj_to_char(struct obj_data * object, struct char_data * ch)
   {
   if (object && ch)
      {
      object->next_content = ch->carrying;
      ch->carrying = object;
      object->carried_by = ch;
      object->worn_by=NULL;
      object->in_obj=NULL;
      IN_ROOM(object) = NOWHERE;
      IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
      IS_CARRYING_N(ch)++;
      if (IS_CORPSE(object))
         GET_OBJ_TIMER(object)=max_pc_corpse_time;
      else if (!IS_OBJ_STAT(object, ITEM_NORENT))
         GET_OBJ_TIMER(object)=-1;

      /* set flag for crash-save system but not on mobs */
      if(!IS_NPC(ch))
         SET_BIT(PLR_FLAGS(ch), PLR_CRASH);
      if((IN_ROOM(ch)>0)&&world[IN_ROOM(ch)].tele&&
              (world[IN_ROOM(ch)].tele->obj==GET_OBJ_VNUM(object)))
         teleport_update(ch);
      }
   else
      {
      /* 2/27/97, Anduin - silencing syserr
         2/08/03, Nomikos - unsilencing syserr */
      log("SYSERR: NULL obj(%s) or char(%s) passed to obj_to_char",
          object?GET_OBJ_NAME(object):"NULL", ch?GET_NAME(ch):"NULL"); 
      }

   }


/* take an object from a char */
void obj_from_char(struct obj_data * object)
   {
   struct obj_data *temp;
   struct char_data *ch;

   if (object == NULL)
      {
      log("SYSERR: NULL object passed to obj_from_char");
      return;
      }
   ch = object->carried_by;
   REMOVE_FROM_LIST(object, ch->carrying, next_content);

   /* set flag for crash-save system */
   if(!IS_NPC(ch))
      SET_BIT(PLR_FLAGS(ch), PLR_CRASH);

   IS_CARRYING_W(ch) -= GET_OBJ_WEIGHT(object);
   IS_CARRYING_N(ch)--;
   object->worn_by = NULL;
   object->carried_by = NULL;
   object->next_content = NULL;
   if((IN_ROOM(ch)>0)&&world[IN_ROOM(ch)].tele&&
           (world[IN_ROOM(ch)].tele->obj==GET_OBJ_VNUM(object)))
      teleport_update(ch);
   }


/* place a character in an object */
void char_to_object(struct char_data * ch, struct obj_data *obj)
   {
   if (!ch || !obj)
      log("SYSERR: Illegal value(s) passed to char_to_object");
   else
      {
      ch->next_in_furniture = obj->people;
      obj->people = ch;

      if (GET_OBJ_TYPE(obj) == ITEM_FURNITURE)
         FURNITURE(ch) = obj;
      }
   }

/* move a player out of an object */
void char_from_object(struct char_data * ch, struct obj_data * obj)
   {
   struct char_data *temp;


   if (ch == NULL || obj == NULL || FURNITURE(ch) == NULL)
      {
      log("SYSERR: NULL in handler.c, char_from_object");
      exit(1);
      }

   /* Sort out what type of object it is */
   if (GET_OBJ_TYPE(obj) == ITEM_FURNITURE)
      {
      REMOVE_FROM_LIST(ch, obj->people, next_in_furniture);
      FURNITURE(ch) = NULL;
      }
   }


/* Return the effect of a piece of armor in position eq_pos */
int apply_ac(struct char_data * ch, int eq_pos)
   {
   int factor;

   if (GET_EQ(ch, eq_pos) == NULL)
      {
      log("SYSERR: apply_ac on %s: piece of eq(position %d) NULL!", 
          GET_NAME(ch), eq_pos);
      core_dump();
      return 0;
      }


   if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
      return 0;

   switch (eq_pos)
      {

   case WEAR_BODY:
      factor = 3;
      break;   /* 30% */
   case WEAR_HEAD:
      factor = 2;
      break;   /* 20% */
   case WEAR_LEGS:
      factor = 2;
      break;   /* 20% */
   case WEAR_SHIELD:
      factor = 3;
      break;
   default:
      factor = 1;
      break;   /* all others 10% */
      }

   return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
   }


int invalid_align(struct char_data *ch, struct obj_data *obj)
   {
     /*
     if (IS_SCR(ch))
     {
       return FALSE;
     }
     */
     if (REMORT_LEVEL(ch) == TRIPLE_REMORT)
     {
       return FALSE;
     }

   if (IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
      return TRUE;
   if (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
      return TRUE;
   if (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))
      return TRUE;
   return FALSE;
   }

void equip_char(struct char_data * ch, struct obj_data * obj, int pos)
   {
   int j;
   struct obj_data *obj2;

   if (pos < 0 || pos >= NUM_WEARS)
      {
      log("SYSERR: equip_char on %s: bad eq position value(%d)",
          GET_NAME(ch), pos);
      core_dump();
      return;
      }

   if((pos==WEAR_WIELD_2)&&!GET_EQ(ch,WEAR_WIELD_1))
      pos=WEAR_WIELD_1;
   if((pos==WEAR_HOLD_2)&&!GET_EQ(ch,WEAR_HOLD_1))
      pos=WEAR_HOLD_1;

   if (GET_EQ(ch, pos))
      {
      if((pos==WEAR_WIELD_1)&&!GET_EQ(ch,WEAR_WIELD_2))
         pos=WEAR_WIELD_2;
      else if((pos==WEAR_HOLD_1)&&!GET_EQ(ch,WEAR_HOLD_2))
         pos=WEAR_HOLD_2;
      else
         {
         mudlogf(NRM,LVL_SERP,TRUE,
                 "SYSERR: Char is already equipped: (ch)[%ld]%s (obj)%s zedit %ld",
                 GET_MOB_VNUM(ch),GET_NAME(ch), obj->short_description,
                 GET_ROOM_VNUM(IN_ROOM(ch)));
         return;
         }
      }
   if (obj->carried_by)
      {
      log("SYSERR: EQUIP: Obj is carried_by when equip.");
      return;
      }
   if (IN_ROOM(obj) != NOWHERE)
      {
      log("SYSERR: EQUIP: Obj is in_room when equip.");
      return;
      }
   if ((invalid_align(ch, obj) ||
           invalid_class(ch, obj) ||
           invalid_race(ch,  obj))&&GET_LEVEL(ch)<LVL_IMMORT)
      {
      act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj,
          0, TO_CHAR);
      act("$n is zapped by $p and instantly lets go of it.", FALSE, ch, obj,
          0, TO_ROOM);
      obj_to_char(obj, ch); /* changed to drop in inventory instead of
              * ground */
      return;
      }

   GET_EQ(ch, pos) = obj;
   obj->worn_by = ch;
   obj->worn_on = pos;

   if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
      GET_AC(ch) -= apply_ac(ch, pos);

   /*
    * Light Stuff
    */
   if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      {
      if (GET_OBJ_VAL(obj, 2))
         {
         if (IN_ROOM(ch) != NOWHERE)
            world[IN_ROOM(ch)].light += VALUE_LIGHT;
         GET_LIGHT(ch) += VALUE_LIGHT;
         }
      }

   if(IS_OBJ_STAT(obj,ITEM_DARK))
      {
      if (IN_ROOM(ch) != NOWHERE)
         world[IN_ROOM(ch)].light -= VALUE_LIGHT;
      GET_LIGHT(ch) -= VALUE_LIGHT;
      }
   if(IS_OBJ_STAT(obj,ITEM_GLOW))
      {
      if (IN_ROOM(ch) != NOWHERE)
         world[IN_ROOM(ch)].light += VALUE_GLOW;
      GET_LIGHT(ch) += VALUE_GLOW;
      }

   /*else
     2/27/97, Anduin - silencing syserr as opposed to changing rooms 
     log("SYSERR: IN_ROOM(ch) = NOWHERE when equipping char."); 
     */

   for (j = 0; j < MAX_OBJ_AFFECT; j++)
      affect_modify(ch, obj->affected[j].location,
                    obj->affected[j].modifier,
                    obj->obj_flags.bitvector, TRUE);

   if(GET_EQ(ch,WEAR_WIELD_1)&&GET_EQ(ch,WEAR_WIELD_2))
      {
      obj =GET_EQ(ch,WEAR_WIELD_1);
      obj2=GET_EQ(ch,WEAR_WIELD_2);
      if(GET_OBJ_WEIGHT(obj)<GET_OBJ_WEIGHT(obj2))
         {
         pos=obj->worn_on;
         obj->worn_on=obj2->worn_on;
         obj2->worn_on=pos;
         GET_EQ(ch,obj->worn_on)=obj;
         GET_EQ(ch,obj2->worn_on)=obj2;
         }
      }
   affect_total(ch);
   check_weapon_weight(ch);
   }



struct obj_data *unequip_char_inner(struct char_data * ch, int pos)
   {
   int j;
   struct obj_data *obj;

   if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == NULL)
      {
      log("SYSERR: unequip_char_inner on %s, bad eq position (%d)",
          GET_NAME(ch), pos);
      core_dump();
      return NULL;
      }

   obj = GET_EQ(ch, pos);
   obj->worn_by = NULL;
   obj->worn_on = -1;

   if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
      GET_AC(ch) += apply_ac(ch, pos);

   /*
    * Light Stuff
    */
   if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      {
      if (GET_OBJ_VAL(obj, 2))
         {
         if (IN_ROOM(ch) != NOWHERE)
            world[IN_ROOM(ch)].light -= VALUE_LIGHT;
         GET_LIGHT(ch) -= VALUE_LIGHT;
         }
      }

   if(IS_OBJ_STAT(obj,ITEM_DARK))
      {
      if (IN_ROOM(ch) != NOWHERE)
         world[IN_ROOM(ch)].light += VALUE_LIGHT;
      GET_LIGHT(ch) += VALUE_LIGHT;
      }
   if(IS_OBJ_STAT(obj,ITEM_GLOW))
      {
      if (IN_ROOM(ch) != NOWHERE)
         world[IN_ROOM(ch)].light -= VALUE_GLOW;
      GET_LIGHT(ch) -= VALUE_GLOW;
      }



   /*  removed by masque, for some reason is called on load
       else 
       log("SYSERR: IN_ROOM(ch) = NOWHERE when unequipping char."); 
       */
   GET_EQ(ch, pos) = NULL;

   for (j = 0; j < MAX_OBJ_AFFECT; j++)
      affect_modify(ch, obj->affected[j].location,
                    obj->affected[j].modifier,
                    obj->obj_flags.bitvector, FALSE);

   affect_total(ch);
   return (obj);
   }

struct obj_data *unequip_char(struct char_data * ch, int pos)
   {
   struct obj_data *obj;
   obj=unequip_char_inner(ch,pos);
   if(!AFF_FLAGGED(ch,AFF_FLY)&&AFF2_FLAGGED(ch,AFF2_FLYING))
      {
      send_to_char(ch, "You gently float to the ground.\r\n");
      REMOVE_BIT(AFF2_FLAGS(ch),AFF2_FLYING);
      }

   return obj;
   }

int get_number(char **name)
   {
   int i;
   int iNumber;
   char *ppos;
   char *vnumber=get_buffer(MAX_INPUT_LENGTH);


   vnumber[0] = '\0';
   if ((ppos = strchr(*name, '.'))!=NULL)
      {
      char *buftmp=get_buffer(256);
      *ppos++ = '\0';
      strcpy(vnumber, *name);
      strcpy(*name, ppos);
      release_buffer(buftmp);
      for (i = 0; *(vnumber + i); i++)
         if (!isdigit((int)*(vnumber + i)))
            {
            release_buffer(vnumber);
            return 0;
            }
      iNumber=atoi(vnumber);
      release_buffer(vnumber);
      return iNumber;
      }
   release_buffer(vnumber);
   return 1;
   }



/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data * list)
   {
   struct obj_data *i;

   for (i = list; i; i = i->next_content)
      if (GET_OBJ_RNUM(i) == num)
         return i;

   return NULL;
   }



/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(obj_rnum nr)
   {
   struct obj_data *i;

   for (i = object_list; i; i = i->next)
      if (GET_OBJ_RNUM(i) == nr)
         return i;

   return NULL;
   }



/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, room_rnum room)
   {
   struct char_data *i;
   int j = 0, vnumber;
   char *tmp =get_buffer(MAX_INPUT_LENGTH);

   strcpy(tmp, name);
   if (!(vnumber = get_number(&tmp)))
      {
      release_buffer(tmp);
      return NULL;
      }

   for (i = world[room].people; i && (j <= vnumber); i = i->next_in_room)
      if (isname(tmp, i->player.name))
         if (++j == vnumber)
            {
            release_buffer(tmp);
            return i;
            }

   release_buffer(tmp);
   return NULL;
   }
/* search all over the world for a char, and return a pointer if found */
struct char_data *get_char(char *name)
   {
   struct char_data *i;
   int  j, vnumber;
   char *tmp =get_buffer(MAX_INPUT_LENGTH);

   strcpy(tmp, name);

   if (!(vnumber = get_number(&tmp)))
      {
      release_buffer(tmp);
      return NULL;
      }

   for (i = character_list, j = 1; i && (j <= vnumber); i = i->next)
      if (!IS_NPC(i))
         {
         if (is_abbrev(tmp, i->player.name))
            {
            if (j == vnumber)
               {
               release_buffer(tmp);
               return(i);
               }
            j++;
            }
         }
      else
         {
         if (isname(tmp, i->player.name))
            {
            if (j == vnumber)
               {
               release_buffer(tmp);
               return(i);
               }
            j++;
            }
         }
   release_buffer(tmp);
   return NULL;
   }

/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(mob_rnum nr)
   {
   struct char_data *i;

   for (i = character_list; i; i = i->next)
      if (GET_MOB_RNUM(i) == nr)
         return i;

   return NULL;
   }



/* put an object in a room */
void obj_to_room(struct obj_data * object, room_rnum room)
   {
   int j;

   if (!object || room < 0 || room > top_of_world)
      log("SYSERR: Illegal value(s) passed to obj_to_room: %s %ld %s",
          object?"obj_ok":"NO OBJECT", room,last_command);
   else
      {
      /* MANWE: The object must be update BEFORE it is put in the room
      contents list. As well, for sake of completeness, also
      update worn_by                                          */
	int timer = GET_OBJ_TIMER(object);
      object->carried_by = NULL;
      object->worn_by = NULL;
      object->next_content = world[room].contents;
      world[room].contents = object;
      IN_ROOM(object) = room;
      if(ROOM_FLAGGED(room, ROOM_NO_DECAY)||IS_OBJ_STAT(object,ITEM_NODECAY)||
              ROOM_FLAGGED(room,ROOM_HOUSE))
         {
         if(Crash_is_unrentable(object))
            {
            if(ROOM_FLAGGED(room,ROOM_HOUSE))
               GET_OBJ_TIMER(object)=3;
            else
               GET_OBJ_TIMER(object)=item_decay;
            }
         else
            GET_OBJ_TIMER(object)=-1;
         }
      else if(IS_CORPSE(object))
         {
         if(object->touched==TRUE)
            {
            if(GET_OBJ_VAL(object,6)==0)
               GET_OBJ_TIMER(object)=max_npc_corpse_time;
            else
               GET_OBJ_TIMER(object)=max_pc_corpse_time;
            object->touched=FALSE;
            }
         else
            GET_OBJ_TIMER(object)=item_decay;
         }
      else
         GET_OBJ_TIMER(object)=item_decay;

      if (ROOM_FLAGGED(room, ROOM_HOUSE))
         SET_BIT(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);

      if((GET_OBJ_TYPE(object)==ITEM_LIGHT)&&(GET_OBJ_VAL(object,2)!=0))
         world[IN_ROOM(object)].light += VALUE_LIGHT;

      if(IS_OBJ_STAT(object,ITEM_DARK))
         world[IN_ROOM(object)].light -= VALUE_LIGHT;
      if(IS_OBJ_STAT(object,ITEM_GLOW))
         world[IN_ROOM(object)].light += VALUE_GLOW;

      for(j=0;j<MAX_OBJ_AFFECT;j++)
         {
         if(object->affected[j].location == APPLY_LIGHT)
            world[IN_ROOM(object)].light += object->affected[j].modifier;
         }

      if (IS_OBJ_STAT(object, ITEM_NORENT)) {
	GET_OBJ_TIMER(object) = timer;
      }
      }
   }


/* Take an object from a room */
void obj_from_room(struct obj_data * object)
   {
   struct obj_data *temp;
   int j;

   if (!object || IN_ROOM(object) == NOWHERE)
      {
      log("SYSERR: NULL object or obj not in a room passed to obj_from_room");
      return;
      }

   if (IS_OBJ_STAT(object, ITEM_PC_CORPSE))
      {
      REMOVE_BIT(GET_OBJ_EXTRA(object), ITEM_PC_CORPSE);
      SET_BIT(GET_OBJ_EXTRA(object), ITEM_NPC_CORPSE);
      save_corpses();
      }

   if((GET_OBJ_TYPE(object)==ITEM_LIGHT)&&(GET_OBJ_VAL(object,2)!=0))
      world[IN_ROOM(object)].light -= VALUE_LIGHT;

   if(IS_OBJ_STAT(object,ITEM_DARK))
      world[IN_ROOM(object)].light += VALUE_LIGHT;
   if(IS_OBJ_STAT(object,ITEM_GLOW))
      world[IN_ROOM(object)].light -= VALUE_GLOW;

   for(j=0;j<MAX_OBJ_AFFECT;j++)
      {
      if(object->affected[j].location == APPLY_LIGHT)
         world[IN_ROOM(object)].light -= object->affected[j].modifier;
      }
   REMOVE_FROM_LIST(object, world[IN_ROOM(object)].contents, next_content);

   if (ROOM_FLAGGED(IN_ROOM(object), ROOM_HOUSE))
      SET_BIT(ROOM_FLAGS(IN_ROOM(object)), ROOM_HOUSE_CRASH);
   IN_ROOM(object) = NOWHERE;
   object->next_content = NULL;
   }


/* put an object in an object (quaint)  */
void obj_to_objt(struct obj_data * obj, struct obj_data * obj_to,
                 const char *func, const int line)
   {
   struct obj_data *tmp_obj;

   if(!obj)
      {
      log("SYSERR: NULL source obj passed to obj_to_obj from %s %d",func,line);
      return;
      }
   if(!obj_to)
      {
      log("SYSERR: NULL target obj passed to obj_to_obj from %s %d",func,line);
      return;
      }
   if (obj == obj_to)
      {
      log("SYSERR: Same source and target obj passed to obj_to_obj from %s %d",
          func,line);
      return;
      }

   obj->next_content = obj_to->contains;
   obj_to->contains = obj;
   obj->in_obj = obj_to;

   for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
      GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

   /* top level object.  Subtract weight from inventory if necessary. */
   GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
   GET_OBJ_VAL(tmp_obj,5)  += GET_OBJ_WEIGHT(obj);
   if (tmp_obj->carried_by)
      IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
   if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && GET_OBJ_VAL(obj, 3))
      GET_OBJ_TIMER(obj)=max_pc_corpse_time;
   else if (!IS_OBJ_STAT(obj, ITEM_NORENT))
      GET_OBJ_TIMER(obj)=-1;
   }


/* remove an object from an object */
void obj_from_obj(struct obj_data * obj)
   {
   struct obj_data *temp, *obj_from;

   if (obj->in_obj == NULL)
      {
      log("SYSERR: (handler.c): trying to illegally extract obj from obj");
      return;
      }
   obj_from = obj->in_obj;
   REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

   /* Subtract weight from containers container */
   for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
      GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

   /* Subtract weight from char that carries the object */
   GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
   GET_OBJ_VAL(temp,5)  -= GET_OBJ_WEIGHT(obj);
   if (temp->carried_by)
      IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);

   obj->in_obj = NULL;
   obj->next_content = NULL;
   }


/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data * list, struct char_data * ch)
   {
   if (list)
      {
      object_list_new_owner(list->contains, ch);
      object_list_new_owner(list->next_content, ch);
      list->carried_by = ch;
      }
   }


/* Extract an object from the world */
void extract_obj(struct obj_data * obj)
   {
   struct obj_data *temp;

   if (obj->worn_by != NULL)
      if (unequip_char(obj->worn_by, obj->worn_on) != obj)
         log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
   if (IN_ROOM(obj) != NOWHERE)
      obj_from_room(obj);
   else if (obj->carried_by)
      obj_from_char(obj);
   else if (obj->in_obj)
      obj_from_obj(obj);

   /* Get the people from the object */
   while (obj->people)
      char_from_object(obj->people, obj);

   /* Get rid of the contents of the object, as well. */
   while (obj->contains)
      extract_obj(obj->contains);

   REMOVE_FROM_LIST(obj, object_list, next);

   if (GET_OBJ_RNUM(obj) >= 0)
      (obj_index[GET_OBJ_RNUM(obj)].number)--;

   if (SCRIPT(obj))
      extract_script(SCRIPT(obj));

   free_obj(obj);
   }



void update_object(struct obj_data * obj, int use)
   {
     /* I'm commenting this out.  Every object in the game's timer is decremented in
      * limits.c:point_update, and this line is doubling the timer decay rate.
      * --Modred
      */
     /*
   if (!SCRIPT_CHECK(obj,OTRIG_TIMER)&&GET_OBJ_TIMER(obj) > 0)
      if((GET_OBJ_TIMER(obj) -= use)<0)
         GET_OBJ_TIMER(obj)=0;
   if (obj->contains)
      update_object(obj->contains, use);
   if (obj->next_content)
      update_object(obj->next_content, use);
     */
   }


void update_char_objects(struct char_data * ch)
   {
   int i,j;

   for (i = 0; i < NUM_WEARS; i++)
      {
      if(GET_EQ(ch,i)!=NULL)
         if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT)
            {
            if (GET_OBJ_VAL(GET_EQ(ch, i), 2) > 0)
               {
               j = --GET_OBJ_VAL(GET_EQ(ch, i), 2);
               if (j == 1)
                  {
                  send_to_char(ch,"Your light begins to flicker and fade.\r\n");
                  act("$n's light begins to flicker and fade.",FALSE,ch,0,0,
                      TO_ROOM);
                  }
               else if (j == 0)
                  {
                  send_to_char(ch,"Your light sputters out and dies.\r\n");
                  act("$n's light sputters out and dies.",FALSE,ch,0,0,TO_ROOM);
                  world[IN_ROOM(ch)].light-=VALUE_LIGHT;
                  GET_LIGHT(ch)-=VALUE_LIGHT;
                  }
               }
            }
      if (GET_EQ(ch, i))
         update_object(GET_EQ(ch, i), 2);
      }
   if (ch->carrying)
      update_object(ch->carrying, 1);
   }



void scrap_item(struct obj_data *obj,struct char_data *ch)
   {
   struct obj_data *ot;

   if(GET_OBJ_CSLOTS(obj)>0)
      {
      act("$p gets extremely hot and you quickly drop it!",FALSE,ch,obj,0,
          TO_CHAR);
      act("$n's $p starts to glow red hot and $e drops it!",FALSE,ch,obj,0,
          TO_ROOM);
      }
   else
      {
      /* Change message to reflect falling into inventory - Nomikos 5/10/2025 */
      act("Your $p is damaged! Please visit a repairman to fix it!",
	      FALSE, ch, obj, 0, TO_CHAR);
      act("$n's $p breaks and is damaged beyond use!",
	      FALSE, ch, obj, 0, TO_ROOM);
	  
      log("Obj-Scrap: %s just broke '%s' (%dC %dT %dO slots)", GET_NAME(ch),
         GET_OBJ_NAME(obj), GET_OBJ_CSLOTS(obj),
		 GET_OBJ_TSLOTS(obj), GET_OBJ_OSLOTS(obj));

      /* Exit out so we don't turn the item into scrap */
      return;
	      
      /* Left here as a comparison for now.  						     **
      ** act("Your $p falls to pieces!",FALSE,ch,obj,0,TO_CHAR);			     **
      ** act("$p falls to the ground in scraps!",FALSE,ch,obj,0,TO_ROOM);		     **
      ** log("Obj-Scrap: %s just lost '%s' (%dC %dT %dO slots)",GET_NAME(ch),		     **
      **    GET_OBJ_NAME(obj),GET_OBJ_CSLOTS(obj),GET_OBJ_TSLOTS(obj),GET_OBJ_OSLOTS(obj));  */
      }

   if(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
      {
      send_to_room(IN_ROOM(obj),"It's contents fall on the ground.\r\n");
      for(ot = obj->contains;ot;ot = obj->contains)
         {
         obj_from_obj(ot);
         obj_to_room(ot,IN_ROOM(obj));
         }
      }
   make_scraps(obj,IN_ROOM(obj));
   extract_obj(obj);
   }


void    make_scraps(struct obj_data *obj, int r_num)
   {
   struct obj_data *scraps;
   char *buf2=get_buffer(256);


   scraps = create_obj();

   scraps->item_number = NOTHING;
   IN_ROOM(scraps) = NOWHERE;

   if((GET_OBJ_CSLOTS(obj)>0)&&
           ((GET_OBJ_TYPE(obj)==ITEM_STAFF) ||
            (GET_OBJ_TYPE(obj)==ITEM_WAND)))
      {
      sprintf(buf2,"%s scraps",obj->name);
      scraps->name = str_dup(buf2);

      sprintf(buf2,"%s that is used up is lying here.",obj->short_description);
      CAP(buf2);
      scraps->description = str_dup(buf2);

      sprintf(buf2, "%s that is used up", obj->short_description);
      scraps->short_description = str_dup(buf2);
      }
   else
      {
      scraps->name = str_dup("scraps");

      sprintf(buf2, "Scraps from %s are lying here.", obj->short_description);
      scraps->description = str_dup(buf2);

      sprintf(buf2, "scraps from %s", obj->short_description);
      scraps->short_description = str_dup(buf2);
      }

   GET_OBJ_TYPE(scraps)   = ITEM_TRASH;
   GET_OBJ_WEAR(scraps)   = ITEM_WEAR_TAKE;
   GET_OBJ_EXTRA(scraps)  = ITEM_NODONATE|ITEM_NORENT;
   GET_OBJ_VAL(scraps,0)  = 0;
   GET_OBJ_VAL(scraps,3)  = 0;
   GET_OBJ_VAL(scraps,7)  = GET_OBJ_VNUM(obj); /* for repairs */
   GET_OBJ_WEIGHT(scraps) = 1;
   GET_OBJ_RENT(scraps)   = 100000;
   GET_OBJ_TIMER(scraps)  = 3;

   GET_OBJ_OSLOTS(scraps) = 30;
   GET_OBJ_TSLOTS(scraps) = 10;
   GET_OBJ_CSLOTS(scraps) = 3;
   scraps->touched        = TRUE;
   scraps->material       = obj->material;
   obj_to_room(scraps,r_num);

   release_buffer(buf2);
   }


extern struct char_data *find_char(int n);

/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char(struct char_data * ch)
{
   struct char_data *k, *temp;
   struct descriptor_data *d;
   struct obj_data *obj;
   int i, freed = 0;

   /*log("extract_char(%s)", ch ? GET_NAME(ch) : "<null>");*/

   if (ch) {
     for (i = 0; i < ch->num_casters; i++) {
       struct char_data *caster = find_char(ch->casting_on_me[i]);
       if (caster) {
	 /*log("Halted %s from casting.", GET_NAME(caster));*/
	 reset_casting_data(caster);
	 send_to_char(caster, "You stop chanting.\r\n");
       }
     }
     if (ch->casting_on_me) {
       free(ch->casting_on_me);
       ch->casting_on_me = NULL;
     }
     ch->num_casters = 0;
   }

   if (ch && GET_GUARDING(ch)) {
     struct char_data *tch = GET_GUARDING(ch);
     act("You stop guarding $N.", TRUE, ch, 0, tch, TO_CHAR);
     act("$n stops guarding you.", TRUE, ch, 0, tch, TO_VICT);
     act("$n stops guarding $N.", TRUE, ch, 0, tch, TO_NOTVICT);
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
     GET_GUARDING(ch) = NULL;
   }
   if (ch && GET_NUM_GUARDING_ME(ch) > 0) {
     for (i = 0; i < GET_NUM_GUARDING_ME(ch); i++) {
       struct char_data *guarder = GET_GUARDING_ME(ch)[i];
       act("You stop guarding $N.", TRUE, guarder, 0, ch, TO_CHAR);
       act("$n stops guarding you.", TRUE, guarder, 0, ch, TO_VICT);
       act("$n stops guarding $N.", TRUE, guarder, 0, ch, TO_NOTVICT);
       /* log("%s stops guarding %s.", GET_NAME(guarder), GET_NAME(ch)); */
       GET_GUARDING(guarder) = NULL;
     }
     free(GET_GUARDING_ME(ch));
     GET_NUM_GUARDING_ME(ch) = 0;
   }

   if (!IS_NPC(ch) && !ch->desc)
      {
      for (d = descriptor_list; d; d = d->next)
         if (d->original == ch)
            do_return(d->character, NULL, 0, 0);
      }
   if (IN_ROOM(ch) == NOWHERE)
      {
      log("SYSERR: NOWHERE extracting char. (handler.c, extract_char)");
      exit(1);
      }
   if (ch->followers || ch->master)
      die_follower(ch);

   if (FURNITURE(ch))
      char_from_object(ch, FURNITURE(ch));
   if (RIDING(ch) || RIDDEN_BY(ch))
      dismount_char(ch);

   /* Forget snooping, if applicable */
   if (ch->desc)
      {
      if(ch->desc->original)
         {
         do_return(ch, NULL, 0, 0);
         }
      else
         {
         for (d = descriptor_list; d; d = d->next)
            {
            if (d == ch->desc)
               continue;
            if (d->character && GET_IDNUM(ch) == GET_IDNUM(d->character))
               {
               STATE(d) = CON_CLOSE;
               mudlogf(NRM, LVL_IMMORT, TRUE,
                       "SYSERR: %s tried to use multiple logins to dupe.",
                       GET_NAME(ch));
               }
            }

         if (ch->desc->snooping)
            {
            ch->desc->snooping->snoop_by = NULL;
            ch->desc->snooping = NULL;
            }
         if (ch->desc->snoop_by)
            {
            SEND_TO_Q(ch->desc->snoop_by,
                      "Your victim is no longer among us.\r\n");
            ch->desc->snoop_by->snooping = NULL;
            ch->desc->snoop_by = NULL;
            }
         }
      }

   /* transfer objects to room, if any */
   while (ch->carrying)
     {
       obj = ch->carrying;
       obj_from_char(obj);
       if (IN_ROOM(ch) != NOWHERE) {
	 obj_to_room(obj, IN_ROOM(ch));
       } else if (IS_NPC(ch)) {
	 extract_obj(obj);
       }
     }
   
   /* transfer equipment to room, if any */
   for (i = 0; i < NUM_WEARS; i++)
     if (GET_EQ(ch, i)) {
       if (i != WEAR_HEART) {
	 obj_to_room(unequip_char(ch, i), IN_ROOM(ch));
       }
     }
     
   if (FIGHTING(ch))
      stop_fighting(ch);

   reset_casting_data(ch);
   char_from_room(ch);
   for (k = combat_list; k; k = temp)
      {
      temp = k->next_fighting;
      if (FIGHTING(k) == ch)
         {
         stop_fighting(k);
         rage_check(k);
         }
      }


   /* pull the char from the list */
   REMOVE_FROM_LIST(ch, character_list, next);

   if (!IS_NPC(ch))
      {
      save_char(ch, NOWHERE);
      if (GET_EQ(ch, WEAR_HEART))
         Crash_heartwornsave(ch);
      else
         Crash_delete_crashfile(ch);
      }
   else
      {
      if (GET_MOB_RNUM(ch) > -1)  /* if mobile */
         mob_index[GET_MOB_RNUM(ch)].number--;
      clearMemory(ch);  /* Only NPC's can have memory */
      if (SCRIPT(ch))
         extract_script(SCRIPT(ch));
      if (SCRIPT_MEM(ch))
         extract_script_mem(SCRIPT_MEM(ch));

      free_char(ch);
      freed = 1;
      }

   if (!freed && ch->desc != NULL)
      {
      STATE(ch->desc) = CON_MENU;
      if(port!=4999)
         SEND_TO_Q(ch->desc, "%s", MENU);
      }
   else
      {
      /* if a player gets purged from within the game */
      if (!freed)
         free_char(ch);
      }
   }



/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions* 
* which incorporate the actual player-data                               *. 
*********************************************************************** */


struct char_data *get_player_vis(struct char_data * ch, char *name, int inroom)
   {
   struct char_data *i;

   for (i = character_list; i; i = i->next)
      {
      if (IS_NPC(i))
         continue;
      if (inroom == FIND_CHAR_ROOM && IN_ROOM(i) != IN_ROOM(ch))
         continue;
      if (!isname(name,i->player.name)) /* If not same, continue */
         continue;
      if (!CAN_SEE(ch, i))
         continue;
      return (i);
      }

   return NULL;
   }


struct char_data *get_char_room_vis(struct char_data * ch, char *name)
   {
   struct char_data *i;
   int j = 0, vnumber;
   char *tmp =get_buffer(MAX_INPUT_LENGTH);

   if(!name)
      return NULL;
   if(!ch)
      return NULL;
   /* JE 7/18/94 :-) :-) */
   if (!str_cmp(name, "self") || !str_cmp(name, "me"))
      {
      release_buffer(tmp);
      return ch;
      }
   if(!str_cmp(name,"target"))
      if(FIGHTING(ch))
         {
         release_buffer(tmp);
         return FIGHTING(ch);
         }
   if(!str_cmp(name,"tank"))
      if(FIGHTING(ch) && FIGHTING(FIGHTING(ch)))
         {
         release_buffer(tmp);
         return FIGHTING(FIGHTING(ch));
         }

   /* 0.<name> means PC with name */
   strcpy(tmp, name);
   if (!(vnumber = get_number(&tmp)))
      {
      i=get_player_vis(ch, tmp, FIND_CHAR_ROOM);
      release_buffer(tmp);
      return i;
      }

   for (i = world[IN_ROOM(ch)].people; i && j <= vnumber; i = i->next_in_room)
      if (isname(tmp, i->player.name))
         if (CAN_SEE(ch, i))
            if (++j == vnumber)
               {
               release_buffer(tmp);
               return i;
               }

   release_buffer(tmp);
   return NULL;
   }

struct room_direction_data *get_other_room_dir(struct room_data *rd,char *name,
               struct char_data *ch)
   {
   /*  struct room_direction_data *i; */
   int j;
   int i;

   if (!str_cmp (name, "north") || !str_cmp(name, "n"))
      {
      j=2;
      i=0;
      }
   else if (!str_cmp (name, "east") || !str_cmp(name, "e"))
      {
      j=3;
      i=1;
      }
   else if (!str_cmp (name, "south") || !str_cmp(name, "s"))
      {
      j=0;
      i=2;
      }
   else if (!str_cmp (name, "west") || !str_cmp(name, "w"))
      {
      j=1;
      i=3;
      }
   else if (!str_cmp (name, "up") || !str_cmp(name, "u"))
      {
      j=5;
      i=4;
      }
   else if (!str_cmp (name, "down") || !str_cmp(name, "d"))
      {
      j=4;
      i=5;
      }
   else
      return NULL;

   if (!EXIT(ch, i) || EXIT(ch, i)->to_room == NOWHERE)
      {
      send_to_char(ch,"There is no exit in that direction.\r\n");
      return NULL;
      }
   else
      return world[(int)rd[IN_ROOM(ch)].dir_option[i]->to_room].dir_option[j];

   return NULL;
   }

struct room_direction_data *get_room_dir(struct room_data *rd,char *name,
               struct char_data *ch)
   {
   /*  struct room_direction_data *i; */
   int j;

   if (!str_cmp (name, "north") || !str_cmp(name, "n"))
      j=0;
   else if (!str_cmp (name, "east") || !str_cmp(name, "e"))
      j=1;
   else if (!str_cmp (name, "south") || !str_cmp(name, "s"))
      j=2;
   else if (!str_cmp (name, "west") || !str_cmp(name, "w"))
      j=3;
   else if (!str_cmp (name, "up") || !str_cmp(name, "u"))
      j=4;
   else if (!str_cmp (name, "down") || !str_cmp(name, "d"))
      j=5;
   else
      return NULL;
   if (!EXIT(ch, j) || EXIT(ch, j)->to_room == NOWHERE)
      {
      send_to_char(ch,"There is no exit in that direction.\r\n");
      return NULL;
      }
   else
      return rd[IN_ROOM(ch)].dir_option[j];

   return NULL;
   }




struct char_data *get_char_vis(struct char_data * ch, char *name,int location)
   {
   struct char_data *i;
   int j = 0, vnumber;
   char *tmp;

   if(!*name)
      return NULL;
   /* check the room first */
   if (location == FIND_CHAR_ROOM)
      return get_char_room_vis(ch, name);
   else if (location == FIND_CHAR_WORLD)
      {
      if ((i = get_char_room_vis(ch, name)) != NULL)
         return i;

      tmp=get_buffer(MAX_INPUT_LENGTH);
      strcpy(tmp, name);
      if (!(vnumber = get_number(&tmp)))
         {
         i=get_player_vis(ch, tmp, 0);
         release_buffer(tmp);
         return i;
         }

      for (i = character_list; i && (j <= vnumber); i = i->next)
         if (isname(tmp, i->player.name) && CAN_SEE(ch, i))
            if (++j == vnumber)
               {
               release_buffer(tmp);
               return i;
               }
      }
   release_buffer(tmp);
   return NULL;
   }



struct obj_data *get_obj_in_list_vis(struct char_data * ch, char *name,
                                              struct obj_data * list)
   {
   struct obj_data *i;
   int j = 0, vnumber;
   char *tmp =get_buffer(MAX_INPUT_LENGTH);

   strcpy(tmp, name);

   if (!(vnumber = get_number(&tmp)))
      {
      release_buffer(tmp);
      return 0;
      }

   for (i = list; i && (j <= vnumber); i = i->next_content)
      {
      if (isname(tmp, i->name))
         {
         if (CAN_SEE_OBJ(ch, i))
            if (++j == vnumber)
               {
               release_buffer(tmp);
               return i;
               }
         /*   j++; */
         }
      }
   release_buffer(tmp);
   return NULL;
   }




/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data * ch, char *name)
   {
   struct obj_data *i;
   int j = 0, vnumber;
   char *tmp =get_buffer(MAX_INPUT_LENGTH);

   /* scan items carried */
   if ((i = get_obj_in_list_vis(ch, name, ch->carrying))!=NULL)
      {
      release_buffer(tmp);
      return i;
      }

   /* scan room */
   if ((i = get_obj_in_list_vis(ch, name, world[IN_ROOM(ch)].contents))!=NULL)
      {
      release_buffer(tmp);
      return i;
      }

   strcpy(tmp, name);
   if ((vnumber = get_number(&tmp))==0)
      {
      release_buffer(tmp);
      return NULL;
      }

   /* ok.. no luck yet. scan the entire obj list   */
   for (i = object_list; i && (j <= vnumber); i = i->next)
      if (isname(tmp, i->name))
         if (CAN_SEE_OBJ(ch, i)&&
                 (!i->carried_by || CAN_SEE(ch,i->carried_by)))
            if (++j == vnumber)
               {
               release_buffer(tmp);
               return i;
               }

   release_buffer(tmp);
   return NULL;
   }



struct obj_data *get_object_in_equip_vis(struct char_data * ch,
               char *arg,struct obj_data *equipment[],
               int *j)
   {
   for ((*j) = 0; (*j) < NUM_WEARS; (*j)++)
      if (equipment[(*j)])
         if (CAN_SEE_OBJ(ch, equipment[(*j)]))
            if (isname(arg, equipment[(*j)]->name))
               return (equipment[(*j)]);

   return NULL;
   }


char *money_desc(int amount)
   {
   char *buf;

   if (amount <= 0)
      {
      log("SYSERR: Try to create negative or 0 money.");
      return NULL;
      }
   if (amount == 1)
      buf= "a gold coin";
   else if (amount <= 10)
      buf= "a tiny pile of gold coins";
   else if (amount <= 20)
      buf= "a handful of gold coins";
   else if (amount <= 75)
      buf= "a little pile of gold coins";
   else if (amount <= 200)
      buf= "a small pile of gold coins";
   else if (amount <= 1000)
      buf= "a pile of gold coins";
   else if (amount <= 5000)
      buf= "a big pile of gold coins";
   else if (amount <= 10000)
      buf= "a large heap of gold coins";
   else if (amount <= 20000)
      buf= "a huge mound of gold coins";
   else if (amount <= 75000)
      buf= "an enormous mound of gold coins";
   else if (amount <= 150000)
      buf= "a small mountain of gold coins";
   else if (amount <= 250000)
      buf= "a mountain of gold coins";
   else if (amount <= 500000)
      buf= "a huge mountain of gold coins";
   else if (amount <= 1000000)
      buf= "an enormous mountain of gold coins";
   else
      buf= "an absolutely colossal mountain of gold coins";

   return buf;
   }


struct obj_data *create_money(int amount)
   {
   struct obj_data *obj;
   struct extra_descr_data *new_descr;
   char *buf;

   if (amount <= 0)
      {
      log("SYSERR: Try to create negative or 0 money.");
      return NULL;
      }
   obj = create_obj();
   CREATE(new_descr, struct extra_descr_data, 1);

   if (amount == 1)
      {
      obj->name = str_dup("coin gold");
      obj->short_description = str_dup("a gold coin");
      obj->description = str_dup("One miserable gold coin is lying here.");
      new_descr->keyword = str_dup("coin gold");
      new_descr->description = str_dup("It's just one miserable little gold coin.");
      }
   else
      {
      obj->name = str_dup("coins gold");
      obj->short_description = str_dup(money_desc(amount));
      buf=get_buffer(200);
      sprintf(buf, "%s is lying here.", money_desc(amount));
      obj->description = str_dup(CAP(buf));

      new_descr->keyword = str_dup("coins gold");
      if (amount < 10)
         {
         sprintf(buf, "There are %d coins.", amount);
         new_descr->description = str_dup(buf);
         }
      else if (amount < 100)
         {
         sprintf(buf, "There are about %d coins.", 10 * (amount / 10));
         new_descr->description = str_dup(buf);
         }
      else if (amount < 1000)
         {
         sprintf(buf, "It looks to be about %d coins.", 100 * (amount / 100));
         new_descr->description = str_dup(buf);
         }
      else if (amount < 100000)
         {
         sprintf(buf, "You guess there are, maybe, %d coins.",
                 1000 * ((amount / 1000) + number(0, (amount / 1000))));
         new_descr->description = str_dup(buf);
         }
      else
         new_descr->description = str_dup("There are a LOT of coins.");

      release_buffer(buf);
      }

   new_descr->next = NULL;
   obj->ex_description = new_descr;

   GET_OBJ_TYPE(obj) = ITEM_MONEY;
   GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
   GET_OBJ_VAL(obj, 0) = amount;
   GET_OBJ_COST(obj) = amount;
   GET_OBJ_OSLOTS(obj)=0;
   GET_OBJ_TSLOTS(obj)=0;
   GET_OBJ_CSLOTS(obj)=0;
   obj->item_number = NOTHING;

   return obj;
   }

/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */

int generic_find(char *arg, bitvector_t bitvector, struct char_data * ch,
                 struct char_data ** tar_ch, struct obj_data ** tar_obj)
   {
   int i, found;
   char *name=get_buffer(256);
   one_argument(arg, name);

   *tar_ch = NULL;
   *tar_obj = NULL;
   if (!*name||!bitvector)
      {
      release_buffer(name);
      return (0);
      }


   if (IS_SET(bitvector, FIND_CHAR_ROOM))
      {
      /* Find person in room */
      if ((*tar_ch = get_char_vis(ch, name,FIND_CHAR_ROOM))!=NULL)
         {
         release_buffer(name);
         return (FIND_CHAR_ROOM);
         }
      }
   if (IS_SET(bitvector, FIND_OBJ_EQUIP))
      {
      for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
         if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->name))
            {
            *tar_obj = GET_EQ(ch, i);
            found = TRUE;
            }
      if (found)
         {
         release_buffer(name);
         return (FIND_OBJ_EQUIP);
         }
      }
   if (IS_SET(bitvector, FIND_OBJ_INV))
      {
      if ((*tar_obj = get_obj_in_list_vis(ch,name,ch->carrying))!=NULL)
         {
         release_buffer(name);
         return (FIND_OBJ_INV);
         }
      }
   if (IS_SET(bitvector, FIND_OBJ_ROOM))
      {
      if ((*tar_obj = get_obj_in_list_vis(ch, name, world[IN_ROOM(ch)].contents))!=NULL)
         {
         release_buffer(name);
         return (FIND_OBJ_ROOM);
         }
      }
   if (IS_SET(bitvector, FIND_OBJ_WORLD))
      {
      if ((*tar_obj = get_obj_vis(ch, name))!=NULL)
         {
         release_buffer(name);
         return (FIND_OBJ_WORLD);
         }
      }
   if (IS_SET(bitvector, FIND_CHAR_WORLD))
      {
      if ((*tar_ch = get_char_vis(ch, name,FIND_CHAR_WORLD))!=NULL)
         {
         release_buffer(name);
         return (FIND_CHAR_WORLD);
         }
      }
   release_buffer(name);
   return (0);
   }


/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
   {
   if (!strcmp(arg, "all"))
      return FIND_ALL;

   else if (!strncmp(arg, "all.", 4)) {
      // Copy remainder of the original argument, and the null byte
      memmove(arg, arg + 4, strlen(arg + 4) + 1);  
      return FIND_ALLDOT;
   } else
      return FIND_INDIV;
   }


/* // dismount_char() / fr: Daniel Koepke (dkoepke@california.com) */
/* //   If a character is mounted on something, we dismount them.  If */
/* //   someone is mounting our character, then we dismount that someone. */
/* //   This is used for cleaning up after a mount is cancelled by */
/* //   something (either intentionally or by death, etc.) */
void dismount_char(struct char_data *ch)
   {
   if (RIDING(ch))
      {
      RIDDEN_BY(RIDING(ch)) = NULL;
      RIDING(ch) = NULL;
      }

   if (RIDDEN_BY(ch))
      {
      RIDING(RIDDEN_BY(ch)) = NULL;
      RIDDEN_BY(ch) = NULL;
      }

   affect_from_char(ch, SKILL_MOUNTED_ATTACK);
   }


/* // mount_char() / fr: Daniel Koepke (dkoepke@california.com) */
/* //   Sets _ch_ to mounting _mount_.  This does not make any checks */
/* //   what-so-ever to see if the _mount_ is mountable, etc.  That is */
/* //   left up to the calling function.  This does not present any */
/* //   messages, either. */
void mount_char(struct char_data *ch, struct char_data *mount)
   {
   struct affected_type af;
   if(mount->master==NULL)
      add_follower(mount,ch);
   else if(mount->master!=ch)
      {
      stop_follower(mount);
      add_follower(mount,ch);
      }

   RIDING(ch) = mount;
   RIDDEN_BY(mount) = ch;

   /* mounted attack - nomikos 10/17/02 */
   affect_from_char(ch,SKILL_MOUNTED_ATTACK);          
   if (!IS_NPC(ch) && SCR_SKILLCHECK(ch, SKILL_MOUNTED_ATTACK) && GET_SKILL(ch, SKILL_MOUNTED_ATTACK)>0)
      {
      af.type = SKILL_MOUNTED_ATTACK;
      af.duration = -1; /* always affected when riding */
      af.bitvector = 0;
      af.location = APPLY_DEX;
      af.modifier = -1; /* you lose a little mobility on a horse */
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      af.location = APPLY_HITROLL;
      af.modifier = GET_SKILL(ch, SKILL_MOUNTED_ATTACK)/30;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      af.location = APPLY_DAMROLL;
      af.modifier = GET_SKILL(ch, SKILL_MOUNTED_ATTACK)/30;
      affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
      }

   }

int get_char_gold(struct char_data *ch)
   {
   if(IS_NPC(ch))
      return GET_GOLD(ch);
   else
      return (GET_GOLD(ch)+GET_BANK_GOLD(ch));
   }

int charge_char_gold(struct char_data *ch, int ammount)
   {
   if(ammount>(GET_GOLD(ch)+GET_BANK_GOLD(ch)))
      {
      return 0;
      }
   else if(ammount>GET_GOLD(ch))
      {
      ammount-=GET_GOLD(ch);
      GET_BANK_GOLD(ch)-=ammount;
      GET_GOLD(ch)=0;
      }
   else
      GET_GOLD(ch)-=ammount;
   return 1;
   }

