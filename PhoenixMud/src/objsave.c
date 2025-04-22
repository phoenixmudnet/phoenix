/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD * 
*  Usage: loading/saving player objects for rent and crash-save           * 
*                                                                         * 
*  All rights reserved.  See license.doc for complete information.        * 
*                                                                         * 
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University * 
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
************************************************************************ */

/* now with auto-equip - BK */

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"


#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "buffer.h"
#include "utils.h"
#include "spells.h"

/* these factors should be unique integers */
#define RENT_FACTOR  1
#define CRYO_FACTOR  4

extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int min_rent_cost;
extern int max_obj_save; /* change in config.c */
extern int free_rent;
extern int rent_file_timeout, crash_file_timeout;
extern int xap_objs;
extern struct zone_data *zone_table;

/* Extern functions */
ACMD(do_action);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
void Crash_crashsave(struct char_data * ch);
void Crash_count_items(struct obj_data * obj, long *nitems);
void get_rent_filename(char *pztName,char *pztFileName);
void check_obj(struct char_data *ch,struct obj_data *obj);
int can_wear_lr(struct char_data *ch, struct obj_data *obj, int message);

extern struct obj_data *obj_proto;


/* SKIPPED WITH ASCII OBJS */
struct obj_data *Obj_from_store_to(struct obj_file_elem object, int *locate)
   {
   struct obj_data *obj;
   int j;

   if (real_object(object.item_number) > -1)
      {
      obj = read_object(object.item_number, VIRTUAL);
      *locate = (int) object.locate;
      GET_OBJ_VAL(obj, 0) = object.value[0];
      GET_OBJ_VAL(obj, 1) = object.value[1];
      GET_OBJ_VAL(obj, 2) = object.value[2];
      GET_OBJ_VAL(obj, 3) = object.value[3];
      GET_OBJ_VAL(obj, 4) = object.value[4];
      if(GET_OBJ_TYPE(obj)==ITEM_CONTAINER)
         GET_OBJ_VAL(obj, 5) = 0;
      else
         GET_OBJ_VAL(obj, 5) = object.value[5];
      GET_OBJ_VAL(obj, 6) = object.value[6];
      GET_OBJ_VAL(obj, 7) = object.value[7];
      GET_OBJ_EXTRA(obj)  = object.extra_flags;
      GET_OBJ_EXTRA2(obj) = object.extra_flags2;
      GET_OBJ_EXTRA3(obj) = object.extra_flags3;
      GET_OBJ_ANTI(obj)   = object.anti_flags;
      if(object.weight>0)
         {
         GET_OBJ_WEIGHT(obj) = object.weight;
         }
      GET_OBJ_TIMER(obj)  = object.timer;
      GET_OBJ_CSLOTS(obj) = object.curr_dam_slots;
      GET_OBJ_TSLOTS(obj) = object.total_dam_slots;
      GET_OBJ_OSLOTS(obj) = object.orig_dam_slots;
      obj->obj_flags.bitvector = object.bitvector;
      obj->touched = FALSE;

      for (j = 0; j < MAX_OBJ_AFFECT; j++)
         obj->affected[j] = object.affected[j];

      return obj;
      }
   else
      return NULL;
   }



/* this function used in house.c */
struct obj_data *Obj_from_store(struct obj_file_elem object)
   {
   int locate;

   return Obj_from_store_to(object, &locate);
   }



int Obj_to_store_from(struct obj_data * obj, FILE * fl, int locate)
   {
   int j;
   struct obj_file_elem object;

   if(!xap_objs)
      {
      object.item_number = GET_OBJ_VNUM(obj);
      object.locate = (sh_int) locate; /* where worn or inventory? */
      object.value[0] = GET_OBJ_VAL(obj, 0);
      object.value[1] = GET_OBJ_VAL(obj, 1);
      object.value[2] = GET_OBJ_VAL(obj, 2);
      object.value[3] = GET_OBJ_VAL(obj, 3);
      object.value[4] = GET_OBJ_VAL(obj, 4);
      if(GET_OBJ_TYPE(obj)==ITEM_CONTAINER)
         object.value[5] = 0;
      else
         object.value[5] = GET_OBJ_VAL(obj, 5);
      object.value[6] = GET_OBJ_VAL(obj, 6);
      object.value[7] = GET_OBJ_VAL(obj, 7);
      object.extra_flags  = GET_OBJ_EXTRA(obj);
      object.extra_flags2 = GET_OBJ_EXTRA2(obj);
      object.extra_flags3 = GET_OBJ_EXTRA3(obj);
      object.anti_flags   = GET_OBJ_ANTI(obj);
      object.weight = GET_OBJ_WEIGHT(obj);
      object.timer = GET_OBJ_TIMER(obj);
      object.curr_dam_slots = GET_OBJ_CSLOTS(obj);
      object.total_dam_slots = GET_OBJ_TSLOTS(obj);
      object.orig_dam_slots = GET_OBJ_OSLOTS(obj);
      object.bitvector = obj->obj_flags.bitvector;
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
         object.affected[j] = obj->affected[j];

      if (fwrite(&object, sizeof(struct obj_file_elem), 1, fl) < 1)
         {
         perror("SYSERR: Error writing object in Obj_to_store");
         return 0;
         }
      }
   else
      {
      return my_obj_save_to_disk(fl, obj, locate);
      }
   return 1;
   }



int Obj_to_store(struct obj_data * obj, FILE * fl)
   {
   return Obj_to_store_from(obj, fl, 0);
   }



int Crash_delete_file(char *name)
   {
   char *filename=get_buffer(64);
   FILE *fl;

   get_rent_filename(name,filename);
   if(filename == NULL)
      {
      release_buffer(filename);
      return 0;
      }

   if (!(fl = fopen(filename, "rb")))
      {
      if (errno != ENOENT)
         { /* if it fails but NOT because of no file */
         char *buf1 = get_buffer(SMALL_BUFSIZE);
         sprintf(buf1, "SYSERR: deleting crash file %s (1)", filename);
         perror(buf1);
         release_buffer(buf1);
         }
      release_buffer(filename);
      return 0;
      }
   fclose(fl);

   if (unlink(filename) < 0)
      {
      if (errno != ENOENT)
         { /* if it fails, NOT because of no file */
         char *buf1 = get_buffer(SMALL_BUFSIZE);
         sprintf(buf1, "SYSERR: deleting crash file %s (2)", filename);
         perror(buf1);
         release_buffer(buf1);
         }
      }
   release_buffer(filename);
   return (1);
   }


int Crash_delete_crashfile(struct char_data * ch)
   {
   char *Fname=get_buffer(MAX_INPUT_LENGTH);
   struct rent_info rent;
   FILE *fl;
   int rentcode,timed,netcost,gold,account,nitems;
   char *line;

   get_rent_filename(GET_NAME(ch),Fname);
   if(Fname == NULL)
      {
      release_buffer(Fname);
      return 0;
      }

   if (!(fl = fopen(Fname, "rb")))
      {
      if (errno != ENOENT)
         { /* if it fails, NOT because of no file */
         char *buf1=get_buffer(256);
         sprintf(buf1, "SYSERR: checking for crash file %s (3)", Fname);
         perror(buf1);
         release_buffer(buf1);
         }
      release_buffer(Fname);
      return 0;
      }

   if(!xap_objs)
      {
      if (!feof(fl))
         fread(&rent, sizeof(struct rent_info), 1, fl);
      fclose(fl);
      }
   else
      {
      line = get_buffer(MAX_STRING_LENGTH);
      if (!feof(fl))
         get_line(fl,line);
      if(*line=='@')
         get_line(fl,line);

      sscanf(line,"%d %d %d %d %d %d",&rentcode,&timed,&netcost,&gold,
             &account,&nitems);
      fclose(fl);
      release_buffer(line);
      }
   if(!xap_objs)
      {
      if (rent.rentcode == RENT_CRASH)
         Crash_delete_file(GET_NAME(ch));
      }
   else
      {
      if (rentcode == RENT_CRASH)
         Crash_delete_file(GET_NAME(ch));
      }

   release_buffer(Fname);
   return 1;
   }


int Crash_clean_file(char *name)
   {
   char *Fname=get_buffer(MAX_STRING_LENGTH);
   char *filetype,*buf;
   struct rent_info rent;
   FILE *fl;
   int rentcode,timed,netcost,gold,account,nitems;
   char *line;

   rentcode = RENT_UNDEF;

   get_rent_filename(name,Fname);
   if(Fname == NULL)
      {
      release_buffer(Fname);
      return 0;
      }

   /*
    * open for write so that permission problems will be flagged now, at boot 
    * time. 
    */
   if (!(fl = fopen(Fname, "r+b")))
      {
      if (errno != ENOENT)
         {
         /* if it fails, NOT because of no file */
         buf=get_buffer(1024);
         sprintf(buf, "SYSERR: OPENING OBJECT FILE %s (4)", Fname);
         perror(buf);
         release_buffer(buf);
         }
      release_buffer(Fname);
      return 0;
      }

   if(!xap_objs)
      {
      if (!feof(fl))
         fread(&rent, sizeof(struct rent_info), 1, fl);
      fclose(fl);
      }
   else
      {
      line = get_buffer(MAX_STRING_LENGTH);
      if (!feof(fl))
         get_line(fl,line);
      sscanf(line, "%d %d %d %d %d %d",&rentcode,&timed,&netcost,
             &gold,&account,&nitems);
      fclose(fl);
      release_buffer(line);
      }

   if(!xap_objs)
      {
      rentcode=rent.rentcode;
      timed=rent.time;
      }


   release_buffer(Fname);

   if ((rentcode == RENT_CRASH) ||
           (rentcode == RENT_FORCED) || (rentcode == RENT_TIMEDOUT))
      {
      if (timed < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY))
         {
         Crash_delete_file(name);
         switch (rentcode)
            {
            case RENT_CRASH:
               filetype = "crash";
               break;
            case RENT_FORCED:
               filetype = "forced rent";
               break;
            case RENT_TIMEDOUT:
               filetype = "idlesave";
               break;
            default:
               filetype = "UNKNOWN!";
               break;
            }
         log("    Deleting %s's %s file.", name, filetype);
         return 1;
         }
      /* Must retrieve rented items w/in 30 days */
      }
   else if (rentcode == RENT_RENTED)
      if (timed < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY))
         {
         Crash_delete_file(name);
         log("    Deleting %s's rent file.", name);
         return 1;
         }
   return (0);
   }



void update_obj_file(void)
   {
   int i;

   for (i = 0; i <= top_of_p_table; i++)
      if (*player_table[i].name)
         Crash_clean_file(player_table[i].name);
   return;
   }



void Crash_listrent(struct char_data * ch, char *name)
   {
   FILE *fl;
   char *Fname=get_buffer(MAX_INPUT_LENGTH), *buf;
   struct obj_file_elem object;
   struct obj_data *obj;
   struct rent_info rent;
   int counter=0,t[10],nr;
   int rentcode,timed,netcost,gold,account,nitems;
   char *line;
   char *sdesc;

   get_rent_filename(name,Fname);
   if(Fname == NULL)
      {
      release_buffer(Fname);
      return;
      }

   buf = get_buffer(MAX_STRING_LENGTH);
   if (!(fl = fopen(Fname, "rb")))
      {
      send_to_char(ch, "%s has no rent file.\r\n", name);
      release_buffer(buf);
      release_buffer(Fname);
      return;
      }
   sprintf(buf, "%s\r\n", Fname);
   release_buffer(Fname);

   if (!feof(fl))
      {
      if(!xap_objs)
         {
         fread(&rent, sizeof(struct rent_info), 1, fl);
         }
      else
         {
         line = get_buffer(MAX_STRING_LENGTH);
         /*clip version number*/
         get_line(fl,line);
         if(feof(fl))
            {
            send_to_char(ch,"Incomplete rent file ERROR!!\r\n");
            fclose(fl);
            release_buffer(buf);
            release_buffer(line);
            return;
            }
         else if(*line=='@')
            {
            get_line(fl,line);
            }
         sscanf(line,"%d %d %d %d %d %d",&rentcode,&timed,&netcost,
                &gold,&account,&nitems);
         release_buffer(line);
         }
      }

   if(!xap_objs)
      {
      rentcode=rent.rentcode;
      }

   switch (rentcode)
      {
      case RENT_RENTED:
         strcat(buf, "Rent\r\n");
         break;
      case RENT_CRASH:
         strcat(buf, "Crash\r\n");
         break;
      case RENT_CRYO:
         strcat(buf, "Cryo\r\n");
         break;
      case RENT_TIMEDOUT:
      case RENT_FORCED:
         strcat(buf, "TimedOut\r\n");
         break;
      default:
         strcat(buf, "Undef\r\n");
         break;
      }

   if(!xap_objs)
      {
      while (!feof(fl))
         {
         fread(&object, sizeof(struct obj_file_elem), 1, fl);
         if (ferror(fl))
            {
            fclose(fl);
            release_buffer(buf);
            return;
            }
         if (!feof(fl))
            {
            counter++;
            if (real_object(object.item_number) > -1)
               {
               obj = read_object(object.item_number, VIRTUAL);
               sprintf(buf+strlen(buf), " [%5ld] (%5dau) <%2d> %-20s\r\n",
                       object.item_number, GET_OBJ_RENT(obj),
                       object.locate, obj->short_description);
               extract_obj(obj);
               if(strlen(buf)>(MAX_STRING_LENGTH-160))
                  {
                  strcat(buf, "** Excessive rent listing. **\r\n");
                  break;
                  }
               }
            }
         }
      }
   else
      {
      line = get_buffer(MAX_STRING_LENGTH);
      while(!feof(fl))
         {
         get_line(fl,line);
         if(*line == '#')
            { /* swell - its an item */
            counter++;
            sscanf(line,"#%d",&nr);
            if(nr != NOTHING)
               {  /* then we can dispense with it easily */
               if((obj=read_object(nr,VIRTUAL))==NULL)
                  continue;
               get_line(fl,line);
               get_line(fl,line);
               sscanf(line,"%d %d %d %d %d %d %d %d",
                      t,t+1,t+2,t+3,t+4,t+5,t+6,t+7);
               sprintf(buf+strlen(buf),"[%5d] (%3d/%3d/%3d) %-20s\r\n",
                       nr, *t, *(t+1), *(t+2), obj->short_description);
               extract_obj(obj);
               }
            else
               { /* its nothing, and a unique item. bleh. partial parse.*/
               char *buf2 = get_buffer(1024);
               get_line(fl,line);    /* this is obj+val */
               get_line(fl,line);    /* this is XAP */
               strcpy(buf2,"xap objects in listrent");
               fread_string(fl,buf2);  /* screw the name */
               sdesc=fread_string(fl,buf2);
               /* may need this stuff for future changes - nomikos */
               fread_string(fl,buf2); /* screw the long desc */
               fread_string(fl,buf2); /* screw the action desc. */
               get_line(fl,line);    /* this is an important line.rent..*/
               sscanf(line,"%d %d %d %d %d",t,t+1,t+2,t+3,t+4);
               /* great we got it all, make the buf */
               sprintf(buf+strlen(buf),"[%5d]               %-20s\r\n",
                       nr,sdesc);
               /* best of all, we don't care if there's descs, or stuff..*/
               /* since we're only doing operations on lines beginning in # */
               /* i suppose you don't want to make exdescs start with # .:) */
               release_buffer(buf2);
               }
            if(strlen(buf)>(MAX_STRING_LENGTH - 256))
               {
               strcat(buf, "**    Excessive rent listing.     **\r\n"
                           "** Somebody clean this guy out!!! **\r\n");
               break;
               }
            }
         }
      release_buffer(line);
      }
   fclose(fl);
   sprintf(buf+strlen(buf), "...%d items listed.\r\n", counter);
   page_string(ch->desc,buf,TRUE,"");
   release_buffer(buf);
   }



int Crash_write_rentcode(struct char_data * ch, FILE * fl,
                         struct rent_info * rent)
   {
   if(!xap_objs)
      {
      if (fwrite(rent, sizeof(struct rent_info), 1, fl) < 1)
         {
         perror("SYSERR: writing rent code");
         return 0;
         }
      }
   else
      {
      if(fprintf(fl,"@Version: %d\n",CUR_POBJ_VER)<1)
         {
         perror("SYSERR: Writing rent version");
         log("SYSERR OBJSAVE: Error writing rent version for %s %d",
             GET_NAME(ch),errno);
         return 0;
         }
      if(fprintf(fl,"%d %d %d %d %d %d\r\n",rent->rentcode, rent->time,
                 rent->net_cost_per_diem,rent->gold,
                 rent->account,rent->nitems) < 1)
         {
         perror("SYSERR: Writing rent code");
         log("SYSERR OBJSAVE: Error writing rent code for %s %d",
             GET_NAME(ch),errno);
         return 0;
         }
      }
   return 1;
   }



/* so this is gonna be the auto equip (hopefully) */
void auto_equip(struct char_data *ch, struct obj_data *obj, int locate)
{
   int j;

   if (locate > 0) {
      /* was worn */
      switch (j = locate - 1) {
         case WEAR_FINGER_R:
         case WEAR_FINGER_L:
            if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
               locate = 0;
            break;
         case WEAR_NECK_1:
         case WEAR_NECK_2:
            if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
               locate = 0;
            break;
         case WEAR_BODY:
            if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
               locate = 0;
            break;
         case WEAR_HEAD:
            if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
               locate = 0;
            break;
         case WEAR_LEGS:
            if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
               locate = 0;
            break;
         case WEAR_FEET:
            if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
               locate = 0;
            break;
         case WEAR_HANDS:
            if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
               locate = 0;
            break;
         case WEAR_ARMS:
            if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
               locate = 0;
            break;
         case WEAR_SHIELD:
            if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
               locate = 0;
            break;
         case WEAR_ABOUT:
            if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
               locate = 0;
            break;
         case WEAR_WAIST:
            if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
               locate = 0;
            break;
         case WEAR_WRIST_R:
         case WEAR_WRIST_L:
            if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
               locate = 0;
            break;
         case WEAR_WIELD_1:
         case WEAR_WIELD_2:
            if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
               locate = 0;
            break;
         case WEAR_HOLD_1:
         case WEAR_HOLD_2:
            if (!CAN_WEAR(obj, ITEM_WEAR_HOLD))
               locate = 0;
            break;
         case WEAR_EAR_L:
         case WEAR_EAR_R:
            if (!CAN_WEAR(obj, ITEM_WEAR_EAR))
               locate = 0;
            break;
         case WEAR_FACE:
            if (!CAN_WEAR(obj, ITEM_WEAR_FACE))
               locate = 0;
            break;
         case WEAR_BACK:
            if (!CAN_WEAR(obj, ITEM_WEAR_BACK))
               locate = 0;
            break;
         case WEAR_HEART:
            break;
         default:
            locate = 0;
      }
      if (GET_EQ(ch, j)) locate = 0;

      // Triple remorts are not affected by alignment restrictions
      if (GET_LEVEL(ch) < LVL_IMMORT && REMORT_LEVEL(ch) < TRIPLE_REMORT) {
         if (IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(ch)   ) locate = 0;
         if (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(ch)   ) locate = 0;
         if (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)) locate = 0;
      }

      // can_wear_lr already handles immortals and triple remorts
      if (!can_wear_lr(ch, obj, FALSE)) locate = 0;

      // If a playerfile was corrupted, the character may have been created with
      // their original equipment (which is in a differnet file) AND given the
      // starting equipment. This can result in characters with saved equipment
      // for more than two hands.
      if (j == WEAR_SHIELD || j == WEAR_WIELD_1 || j == WEAR_WIELD_2 || j == WEAR_HOLD_1 || j == WEAR_HOLD_2) {
         int hands = 0;

         if (GET_EQ(ch, WEAR_SHIELD)) hands++;
         if (GET_EQ(ch, WEAR_HOLD_1)) hands++;
         if (GET_EQ(ch, WEAR_HOLD_2)) hands++;

         if (GET_EQ(ch, WEAR_WIELD_1)) {
            struct obj_data * weapon = GET_EQ(ch, WEAR_WIELD_1);
            if (TWO_HANDED(weapon)) {
               hands++;
            }
            hands++;
         } 

         if (GET_EQ(ch, WEAR_WIELD_2)) {
            struct obj_data * weapon = GET_EQ(ch, WEAR_WIELD_2);
            if (TWO_HANDED(weapon)) {
               hands++;
            }
            hands++;
         }

         if (hands >= 2) {
            locate = 0;
         }
      }
   }

   if (locate > 0) {
      equip_char(ch, obj, j);
   } else {
      obj_to_char(obj, ch);
   }
}

#define MAX_BAG_ROW 5
/* should be enough - who would carry a bag in a bag in a bag in a
   bag in a bag in a bag ?!? */

int Crash_load(struct char_data * ch)
/* return values:
 0 - successful load, keep char in rent room. 
 1 - load failure or load of crash items -- put char in temple. 
 2 - rented equipment lost (no $) 
*/
   {
   FILE *fl;
   char *Fname=get_buffer(MAX_STRING_LENGTH),*buf;
   struct obj_file_elem object;
   struct rent_info rent;
   int cost, orig_rent_code;
   float num_of_days;
   struct obj_data *obj;
   int locate, j;
   struct obj_data *obj1;
   struct obj_data *cont_row[MAX_BAG_ROW];
   int counter=0;

   /*
   // Wipe the explored bitvector and reset the total when a player is loaded.
   memset(ch->player_specials->explored_vnums, 0, (1+EXPLORED_TOP_VNUM/8)*sizeof(char));
   ch->player_specials->explored_total = 0;
   // Try opening the "explored" file.
   FILE *fp = fopen(EXPLORED_FILE, "r");
   if (!fp) {
     // It failed.  Maybe it doesn't exist?  Make a blank one.
     fp = fopen(EXPLORED_FILE, "w");
     if (fp) {
       fclose(fp);
       // OK, we created a blank one.  Now try reopening it.
       fp = fopen(EXPLORED_FILE, "r");
     }
   }
   // Now try reading from it, if it exists.
   if (fp) {
     int id = GET_IDNUM(ch);
     // log("EXPLORE: Loading %s (id=%d).", GET_NAME(ch), id);
     if (id >= 0 && id < 100000) {
       // The size of a record is the number of bytes in the vector.
       int offset = id * EXPLORED_BYTES;
       fseek(fp, offset, SEEK_SET);
       fread(&ch->player_specials->explored_vnums, sizeof(char), EXPLORED_BYTES, fp);
       fclose(fp);
       // We've read in explored rooms, now compute the total.
       int i;
       for (i = 0; i < EXPLORED_TOP_VNUM; i++) {
	 if ((ch->player_specials->explored_vnums[i/8] & (1 << (i%8))) != 0) {
	   ch->player_specials->explored_total++;
	 }
       }
     }
   }
   */

   if(xap_objs)
      {
      release_buffer(Fname);
      return (Crash_load_xapobjs(ch));
      }

   if (!get_filename(GET_NAME(ch), Fname, CRASH_FILE))
      {
      release_buffer(Fname);
      return 1;
      }

   if (!(fl = fopen(Fname, "r+b")))
      {
      buf = get_buffer(SMALL_BUFSIZE);
      if (errno != ENOENT)
         {
         /* if it fails, NOT because of no file */
         sprintf(buf, "SYSERR: READING OBJECT FILE %s (5)", Fname);
         perror(buf);
         send_to_char(ch,"\r\n********************* NOTICE *********************\r\n"
                      "There was a problem loading your objects from disk.\r\n"
                      "Contact a God for assistance.\r\n");
         }
      mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
              "%s entering game with no equipment.", GET_NAME(ch));
      release_buffer(buf);
      release_buffer(Fname);
      return 1;
      }

   release_buffer(Fname);
   if (!feof(fl))
      fread(&rent, sizeof(struct rent_info), 1, fl);

   if (rent.rentcode == RENT_RENTED || rent.rentcode == RENT_TIMEDOUT)
      {
      num_of_days = (float) (time(0) - rent.time) / SECS_PER_REAL_DAY;
      cost = (int) (rent.net_cost_per_diem * num_of_days);
      cost =0;
      if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch))
         {
         fclose(fl);
         mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "RENTGONE: %s entering game, rented equipment lost (no $).",
                 GET_NAME(ch));
         Crash_crashsave(ch);
         return 2;
         }
      else
         {
         GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
         GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
         save_char(ch, IN_ROOM(ch));
         }
      }
   buf = get_buffer(SMALL_BUFSIZE);
   switch (orig_rent_code = rent.rentcode)
      {
      case RENT_RENTED:
         sprintf(buf, "%s un-renting and entering game.(#%ld)", GET_NAME(ch),
                 world[IN_ROOM(ch)].number);
         break;
      case RENT_CRASH:
         sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
         break;
      case RENT_CRYO:
         sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
         break;
      case RENT_FORCED:
      case RENT_TIMEDOUT:
         sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
         break;
      default:
         sprintf(buf, "SYSERR: %s entering game with undefined rent code.", GET_NAME(ch));
         break;
      }
   for (j = 0;j < MAX_BAG_ROW;j++)
      cont_row[j] = NULL; /* empty all cont lists (you never know ...) */

   while (!feof(fl))
      {
      fread(&object, sizeof(struct obj_file_elem), 1, fl);
      if (ferror(fl))
         {
         perror("SYSERR: Reading crash file: Crash_load.");
         fclose(fl);
         mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
         release_buffer(buf);
         return 1;
         }
      if (!feof(fl))
         {
         counter++;
         if ((obj = Obj_from_store_to(object, &locate)))
            {
            auto_equip(ch, obj, locate);

            /*
               what to do with a new loaded item: 
             
               if there's a list with <locate> less than 1 below this: 
               (equipped items are assumed to have <locate>==0 here) then its 
               container has disappeared from the file   *gasp* 
               -> put all the list back to ch's inventory 
             
               if there's a list of contents with <locate> 1 below this: 
               check if it's a container 
               - if so: get it from ch, fill it, and give it back to ch (this way the 
               container has its correct weight before modifying ch) 
               - if not: the container is missing -> put all the list to ch's inventory 
             
               for items with negative <locate>: 
               if there's already a list of contents with the same <locate> put obj to it 
               if not, start a new list 
             
               Confused? Well maybe you can think of some better text to be put here ... 
             
               since <locate> for contents is < 0 the list indices are switched to 
               non-negative 
               */

            if (locate > 0)
               {
               /* item equipped */
               for (j = MAX_BAG_ROW-1;j > 0;j--)
                  if (cont_row[j])
                     {
                     /* no container -> back to ch's inventory */
                     for (;cont_row[j];cont_row[j] = obj1)
                        {
                        obj1 = cont_row[j]->next_content;
                        obj_to_char(cont_row[j], ch);
                        }
                     cont_row[j] = NULL;
                     }
               if (cont_row[0])
                  {
                  /* content list existing */
                  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
                     {
                     /* rem item ; fill ; equip again */
                     obj = unequip_char(ch, locate-1);
                     obj->contains = NULL; /* should be empty - but who knows */
                     for (;cont_row[0];cont_row[0] = obj1)
                        {
                        obj1 = cont_row[0]->next_content;
                        obj_to_obj(cont_row[0], obj);
                        }
                     equip_char(ch, obj, locate-1);
                     }
                  else
                     {
                     /* object isn't container -> empty content list */
                     for (;cont_row[0];cont_row[0] = obj1)
                        {
                        obj1 = cont_row[0]->next_content;
                        obj_to_char(cont_row[0], ch);
                        }
                     cont_row[0] = NULL;
                     }
                  }
               }
            else
               {
               /* locate <= 0 */
               for (j = MAX_BAG_ROW-1;j > -locate;j--)
                  if (cont_row[j])
                     {
                     /* no container -> back to ch's inventory */
                     for (;cont_row[j];cont_row[j] = obj1)
                        {
                        obj1 = cont_row[j]->next_content;
                        obj_to_char(cont_row[j], ch);
                        }
                     cont_row[j] = NULL;
                     }

               if (j == -locate && cont_row[j])
                  {
                  /* content list existing */
                  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
                     {
                     /* take item ; fill ; give to char again */
                     obj_from_char(obj);
                     obj->contains = NULL;
                     for (;cont_row[j];cont_row[j] = obj1)
                        {
                        obj1 = cont_row[j]->next_content;
                        obj_to_obj(cont_row[j], obj);
                        }
                     obj_to_char(obj, ch); /* add to inv first ... */
                     }
                  else
                     {
                     /* object isn't container -> empty content list */
                     for (;cont_row[j];cont_row[j] = obj1)
                        {
                        obj1 = cont_row[j]->next_content;
                        obj_to_char(cont_row[j], ch);
                        }
                     cont_row[j] = NULL;
                     }
                  }

               if (locate < 0 && locate >= -MAX_BAG_ROW)
                  {
                  /* let obj be part of content list
                     but put it at the list's end thus having the items 
                     in the same order as before renting */
                  obj_from_char(obj);
                  if ((obj1 = cont_row[-locate-1]))
                     {
                     while (obj1->next_content)
                        obj1 = obj1->next_content;
                     obj1->next_content = obj;
                     obj->in_obj = obj1;
                     }
                  else
                     cont_row[-locate-1] = obj;
                  }
               }
            }
         }
      }

   mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s(%d Items)",buf,counter);

   release_buffer(buf);
   if(counter>max_obj_save)
      {
      mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
              "\007\007ALERT: %s un-rented with %d items.",
              GET_NAME(ch),counter);
      }
   /* turn this into a crash file by re-writing the control block */
   rent.rentcode = RENT_CRASH;
   rent.time = time(0);
   rewind(fl);
   Crash_write_rentcode(ch, fl, &rent);

   fclose(fl);

   if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
      return 0;
   else
      return 1;
   }



int Crash_save(struct obj_data * obj, FILE * fp, int locate)
   {
   struct obj_data *tmp;
   int result = TRUE;

   if (obj)
      {
      result = Crash_save(obj->next_content, fp, locate);
      if(result == TRUE)
         result = Crash_save(obj->contains, fp, MIN(0,locate)-1);
      if(result == TRUE)
         result = Obj_to_store_from(obj, fp, locate);

      for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
         GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

      return result;
      }
   return TRUE;
   }


void Crash_restore_weight(struct obj_data * obj)
   {
   if (obj)
      {
      Crash_restore_weight(obj->contains);
      Crash_restore_weight(obj->next_content);
      if (obj->in_obj)
         GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
      }
   }



void Crash_extract_objs(struct obj_data * obj)
   {
   if (obj)
      {
      Crash_extract_objs(obj->contains);
      Crash_extract_objs(obj->next_content);
      extract_obj(obj);
      }
   }


int Crash_is_unrentable(struct obj_data * obj)
   {
   if (!obj)
      return 0;

   if(!xap_objs)
      {
      if (IS_OBJ_STAT(obj, ITEM_NORENT) ||
              IS_OBJ_STAT(obj, ITEM_BATTLE_ITEM) ||
              GET_OBJ_RENT(obj) < 0 ||
              GET_OBJ_TYPE(obj) == ITEM_KEY ||
              (GET_OBJ_RNUM(obj) <= NOTHING &&
               !IS_SET(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE)))
         return 1;
      }
   else
      {
      if (IS_OBJ_STAT(obj, ITEM_NORENT) ||
              IS_OBJ_STAT(obj, ITEM_BATTLE_ITEM) ||
              GET_OBJ_RENT(obj) < 0 ||
              GET_OBJ_TYPE(obj) == ITEM_KEY)
         return 1;
      }

   return 0;
   }


void Crash_extract_norents(struct obj_data * obj)
   {
   if (obj)
      {
      Crash_extract_norents(obj->contains);
      Crash_extract_norents(obj->next_content);
      if (Crash_is_unrentable(obj))
         extract_obj(obj);
      }
   }


/* get norent items from eq to inventory and
   extract norents out of worn containers */
void Crash_extract_norents_from_equipped(struct char_data * ch)
   {
   int j;
   struct obj_data *obj;

   for (j = 0;j < NUM_WEARS;j++)
      {
      if ((obj = GET_EQ(ch, j)) == NULL) /* A huge || looks ugly. */
         continue;
      else if (IS_OBJ_STAT(obj, ITEM_NORENT))
         obj_to_char(unequip_char(ch, j), ch);
      else if (GET_OBJ_RENT(obj) < 0)
         obj_to_char(unequip_char(ch, j), ch);
      else if (GET_OBJ_RNUM(obj) <= NOTHING)
         obj_to_char(unequip_char(ch, j), ch);
      else if (GET_OBJ_TYPE(obj) == ITEM_KEY)
         obj_to_char(unequip_char(ch, j), ch);
      else if(GET_OBJ_RNUM(obj) <= NOTHING &&
              !IS_SET(GET_OBJ_EXTRA(obj),ITEM_UNIQUE_SAVE))
         obj_to_char(unequip_char(ch, j), ch);
      else
         Crash_extract_norents(obj);
      }
   }

int Crash_check_norents(struct obj_data * obj,struct char_data *ch)
   {
   int total=0;
   if (obj)
      {
      total+=Crash_check_norents(obj->contains,ch);
      total+=Crash_check_norents(obj->next_content,ch);
      if (Crash_is_unrentable(obj))
         {
         send_to_char(ch, "You cannot store %s in your camp.\r\n",
                      GET_OBJ_NAME(obj));
         return total+1;
         }
      }
   return total;
   }

int Crash_check_norents_equipped(struct char_data * ch)
   {
   int j;
   int total=0;
   for (j = 0;j < NUM_WEARS;j++)
      {
      if (GET_EQ(ch,j))
         {
         total+=Crash_check_norents(GET_EQ(ch,j),ch);
         }
      }
   return total;
   }


void Crash_extract_expensive(struct obj_data * obj)
   {
   struct obj_data *tobj, *max;

   max = obj;
   for (tobj = obj; tobj; tobj = tobj->next_content)
      if (GET_OBJ_RENT(tobj) > GET_OBJ_RENT(max))
         max = tobj;
   extract_obj(max);
   }



void Crash_calculate_rent(struct obj_data * obj, int *cost)
   {
   if (obj)
      {
      *cost += MIN(0, GET_OBJ_RENT(obj)); /* RENT FIX! */
      Crash_calculate_rent(obj->contains, cost);
      Crash_calculate_rent(obj->next_content, cost);
      }
   }


void Crash_crashsave(struct char_data * ch)
   {
   char *buf;
   struct rent_info rent;
   int j;
   FILE *fp;
   int result=TRUE;

   if (IS_NPC(ch))
      return;

   buf = get_buffer(MAX_INPUT_LENGTH);
   get_rent_filename(GET_NAME(ch),buf);
   if(buf == NULL)
      {
      log("SYSERR OBJSAVE:(crash) Could not get rent filename for %s",
          GET_NAME(ch));
      release_buffer(buf);
      return;
      }

   if(!(fp=fopen(buf,"wb")))
      {
      mudlogf(CMP,LVL_IMMORT,TRUE,
              "SYSERR OBJSAVE: Can't open %s to save players objects. %d",
              buf,errno);
      release_buffer(buf);
      return;
      }
   release_buffer(buf);

   rent.rentcode = RENT_CRASH;
   rent.time = time(0);
   rent.net_cost_per_diem =0;
   rent.gold = GET_GOLD(ch);
   rent.account = GET_BANK_GOLD(ch);
   rent.nitems=0;
   if (!Crash_write_rentcode(ch, fp, &rent))
      {
      fclose(fp);
      return;
      }

   for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(ch,j))
         {
         result = Crash_save(GET_EQ(ch,j), fp, j+1);
         if (result != TRUE)
            {
            log("SYSERR OBJSAVE:(crash) Error saving %s in position %d on %s: %d",
                GET_OBJ_NAME(GET_EQ(ch,j)),j,GET_NAME(ch),result);
            fclose(fp);
            return;
            }
         Crash_restore_weight(GET_EQ(ch,j));
         }

   result = Crash_save(ch->carrying, fp, 0);
   if (result != TRUE)
      {
      log("SYSERR OBJSAVE:(crash) Error saving inventory on %s: %d",
          GET_NAME(ch),result);
      fclose(fp);
      return;
      }
   Crash_restore_weight(ch->carrying);

   fclose(fp);
   REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
   }


void Crash_idlesave(struct char_data * ch)
   {
   char *buf;
   struct rent_info rent;
   int j;
   FILE *fp;
   int result = TRUE;

   if (IS_NPC(ch))
      return;

   buf=get_buffer(MAX_INPUT_LENGTH);
   get_rent_filename(GET_NAME(ch),buf);
   if(buf == NULL)
      {
      log("SYSERR OBJSAVE:(idle) Could not get rent filename for %s",
          GET_NAME(ch));
      release_buffer(buf);
      return;
      }

   if (!(fp = fopen(buf, "wb")))
      {
      mudlogf(CMP,LVL_IMMORT,TRUE,
              "SYSERR OBJSAVE:(idle) Can't open %s to save players objects.",
              buf);
      release_buffer(buf);
      return;
      }
   release_buffer(buf);

   Crash_extract_norents_from_equipped(ch);

   Crash_extract_norents(ch->carrying);

   if (!ch->carrying)
      {
      for (j = 0; j < NUM_WEARS && !(GET_EQ(ch,j)); j++)
         ;
      if (j == NUM_WEARS)
         {
         /* no eq nor inv */
         fclose(fp);
         Crash_delete_file(GET_NAME(ch));
         return;
         }
      }
   rent.net_cost_per_diem = 0;

   rent.rentcode = RENT_TIMEDOUT;
   rent.time = time(0);
   rent.gold = GET_GOLD(ch);
   rent.account = GET_BANK_GOLD(ch);
   rent.nitems=0;

   if (!Crash_write_rentcode(ch, fp, &rent))
      {
      fclose(fp);
      return;
      }


   for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(ch,j))
         {
         result = Crash_save(GET_EQ(ch,j), fp, j+1);
         if (result != TRUE)
            {
            log("SYSERR OBJSAVE:(idle) Error saving %s in position %d on %s: %d",
                GET_OBJ_NAME(GET_EQ(ch,j)),j,GET_NAME(ch),result);
            fclose(fp);
            return;
            }
         Crash_restore_weight(GET_EQ(ch,j));
         Crash_extract_objs(GET_EQ(ch,j));
         }
   result = Crash_save(ch->carrying, fp, 0);
   if (result != TRUE)
      {
      log("SYSERR OBJSAVE:(idle) Error saving inventory on %s: %d",
          GET_NAME(ch),result);
      fclose(fp);
      return;
      }
   fclose(fp);

   Crash_extract_objs(ch->carrying);
   }

void Crash_heartwornsave(struct char_data * ch)
   {
   char *buf;
   struct rent_info rent;
   FILE *fp;
   int result=TRUE;

   if (IS_NPC(ch))
      return;

   buf=get_buffer(MAX_INPUT_LENGTH);
   get_rent_filename(GET_NAME(ch),buf);
   if(buf == NULL)
      {
      log("SYSERR OBJSAVE:(rent) Could not get rent filename for %s",
          GET_NAME(ch));
      release_buffer(buf);
      return;
      }

   if (!(fp = fopen(buf, "wb")))
      {
      mudlogf(CMP,LVL_IMMORT,TRUE,
              "SYSERR OBJSAVE:(rent) Can't open %s to save players objects.",
              buf);
      release_buffer(buf);
      return;
      }
   release_buffer(buf);


   rent.net_cost_per_diem = 0;
   rent.rentcode = RENT_RENTED;
   rent.time = time(0);
   rent.gold = GET_GOLD(ch);
   rent.account = GET_BANK_GOLD(ch);
   rent.nitems=0;


   if (!Crash_write_rentcode(ch, fp, &rent))
      {
      fclose(fp);
      mudlogf(CMP,LVL_IMMORT,TRUE,
              "SYSERR OBJSAVE:(rent) Crash_write_rentcode failed "
              "in Crash_heartwornsave.");
      return;
      }

   result = Crash_save(GET_EQ(ch,WEAR_HEART), fp, WEAR_HEART+1);
   if (result != TRUE)
      {
      log("SYSERR OBJSAVE:(rent) Error saving %s in "
          "heartworn position on %s: %d",
          GET_OBJ_NAME(GET_EQ(ch,WEAR_HEART)),GET_NAME(ch),result);
      fclose(fp);
      return;
      }
   fclose(fp);

   extract_obj(GET_EQ(ch,WEAR_HEART));
   }

void Crash_rentsave(struct char_data * ch, int cost)
   {
   char *buf;
   struct rent_info rent;
   int j;
   FILE *fp;
   long nitems;
   int result=TRUE;

   if (IS_NPC(ch))
      return;

   buf=get_buffer(MAX_INPUT_LENGTH);
   get_rent_filename(GET_NAME(ch),buf);
   if(buf == NULL)
      {
      log("SYSERR OBJSAVE:(rent) Could not get rent filename for %s",
          GET_NAME(ch));
      release_buffer(buf);
      return;
      }

   if (!(fp = fopen(buf, "wb")))
      {
      mudlogf(CMP,LVL_IMMORT,TRUE,
              "SYSERR OBJSAVE:(rent) Can't open %s to save players objects.",
              buf);
      release_buffer(buf);
      return;
      }
   release_buffer(buf);

   Crash_extract_norents_from_equipped(ch);
   Crash_extract_norents(ch->carrying);

   /****   count here ****/
   nitems=0;
   for(j=0;j<NUM_WEARS;j++)
      Crash_count_items(GET_EQ(ch,j),&nitems);
   Crash_count_items(ch->carrying, &nitems);

   if((nitems>max_obj_save) && (GET_LEVEL(ch)<LVL_IMMORT))
      {
      mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
              "\007\007CAMP ALERT: %s camped with %ld items.",
              GET_NAME(ch),nitems);
      }
   else
      {
      mudlogf(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
              "CAMP: %s camped with %ld items.",GET_NAME(ch),nitems);
      }

   cost =0;
   rent.net_cost_per_diem = cost;
   rent.rentcode = RENT_RENTED;
   rent.time = time(0);
   rent.gold = GET_GOLD(ch);
   rent.account = GET_BANK_GOLD(ch);
   rent.nitems=0;


   if (!Crash_write_rentcode(ch, fp, &rent))
      {
      fclose(fp);
      return;
      }


   for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(ch,j))
         {
         result = Crash_save(GET_EQ(ch,j), fp, j+1);
         if (result != TRUE)
            {
            log("SYSERR OBJSAVE:(rent) Error saving %s in "
                "position %d on %s: %d",
                GET_OBJ_NAME(GET_EQ(ch,j)),j,GET_NAME(ch),result);
            fclose(fp);
            return;
            }
         Crash_restore_weight(GET_EQ(ch,j));
         Crash_extract_objs(GET_EQ(ch,j));
         }
   result = Crash_save(ch->carrying, fp, 0);
   if (result != TRUE)
      {
      log("SYSERR OBJSAVE:(rent) Error saving inventory on %s: %d",
          GET_NAME(ch),result);
      fclose(fp);
      return;
      }
   fclose(fp);

   Crash_extract_objs(ch->carrying);
   }


void Crash_cryosave(struct char_data * ch, int cost)
   {
   char *buf;
   struct rent_info rent;
   int j;
   FILE *fp;
   int result=TRUE;

   if (IS_NPC(ch))
      return;

   buf=get_buffer(MAX_INPUT_LENGTH);
   get_rent_filename(GET_NAME(ch),buf);
   if(buf == NULL)
      {
      log("SYSERR OBJSAVE:(rent) Could not get rent filename for %s",
          GET_NAME(ch));
      release_buffer(buf);
      return;
      }

   if (!(fp = fopen(buf, "wb")))
      {
      mudlogf(CMP,LVL_IMMORT,TRUE,
              "SYSERR OBJSAVE:(cryo) Can't open %s to save players objects.",
              buf);
      release_buffer(buf);
      return;
      }
   release_buffer(buf);

   Crash_extract_norents_from_equipped(ch);

   Crash_extract_norents(ch->carrying);

   GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);

   rent.rentcode = RENT_CRYO;
   rent.time = time(0);
   rent.gold = GET_GOLD(ch);
   rent.account = GET_BANK_GOLD(ch);
   rent.net_cost_per_diem = 0;
   rent.nitems=0;

   if (!Crash_write_rentcode(ch, fp, &rent))
      {
      fclose(fp);
      return;
      }

   for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(ch,j))
         {
         result = Crash_save(GET_EQ(ch,j), fp, j+1);
         if (result != TRUE)
            {
            log("SYSERR OBJSAVE:(cryo) Error saving %s in "
                "position %d on %s: %d",
                GET_OBJ_NAME(GET_EQ(ch,j)),j,GET_NAME(ch),result);
            fclose(fp);
            return;
            }
         Crash_restore_weight(GET_EQ(ch,j));
         Crash_extract_objs(GET_EQ(ch,j));
         }
   result = Crash_save(ch->carrying, fp, 0);
   if (result != TRUE)
      {
      log("SYSERR OBJSAVE:(cryo) Error saving inventory on %s: %d",
          GET_NAME(ch),result);
      fclose(fp);
      return;
      }
   fclose(fp);

   Crash_extract_objs(ch->carrying);
   SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
   }


/* ************************************************************************
* Routines used for the receptionist       * 
************************************************************************* */

void Crash_rent_deadline(struct char_data * ch, struct char_data * recep,
                         long cost)
   {
   long rent_deadline;
   char *buf;

   if (!cost)
      return;

   rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);
   buf=get_buffer(SMALL_BUFSIZE);
   sprintf(buf,
           "$n tells you, 'You can rent for %ld day%s with the gold you have\r\n"
           "on hand and in the bank.'\r\n",
           rent_deadline, (rent_deadline > 1) ? "s" : "");
   act(buf, FALSE, recep, 0, ch, TO_VICT);
   release_buffer(buf);
   }

int Crash_report_unrentables(struct char_data * ch, struct char_data * recep,
                             struct obj_data * obj)
   {
   char *buf=get_buffer(128);
   int has_norents = 0;

   if (obj)
      {
      if (Crash_is_unrentable(obj))
         {
         has_norents = 1;
         sprintf(buf, "$n tells you, 'You cannot store %s.'", OBJS(obj, ch));
         act(buf, FALSE, recep, 0, ch, TO_VICT);
         }
      has_norents += Crash_report_unrentables(ch, recep, obj->contains);
      has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
      }
   release_buffer(buf);
   return (has_norents);
   }

void Crash_count_items(struct obj_data * obj, long *nitems)
   {
   if(obj)
      {
      (*nitems)++;
      Crash_count_items(obj->contains,nitems);
      Crash_count_items(obj->next_content,nitems);
      }
   }


void Crash_report_rent(struct char_data * ch, struct char_data * recep,
                       struct obj_data * obj, long *cost, long *nitems, int display, int factor)
   {
   char *buf=get_buffer(256);

   if (obj)
      {
      if (!Crash_is_unrentable(obj))
         {
         (*nitems)++;
         *cost += MAX(0, (GET_OBJ_RENT(obj) * factor));
         if (display)
            {
            sprintf(buf, "$n tells you, '%5d coins for %s..'",
                    (GET_OBJ_RENT(obj) * factor), OBJS(obj, ch));
            act(buf, FALSE, recep, 0, ch, TO_VICT);
            }
         }
      Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor);
      Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor);
      }
   release_buffer(buf);
   }



int Crash_offer_rent(struct char_data * ch, struct char_data * reception,
                     int display, int factor)
   {
   char *buf;
   int i;
   long totalcost = 0, numitems = 0, norent = 0;

   norent = Crash_report_unrentables(ch, reception, ch->carrying);
   for (i = 0; i < NUM_WEARS; i++)
      norent += Crash_report_unrentables(ch, reception, GET_EQ(ch,i));

   if (norent)
      return 0;

   totalcost = min_rent_cost * factor;

   Crash_report_rent(ch, reception, ch->carrying, &totalcost, &numitems, display, factor);

   for (i = 0; i < NUM_WEARS; i++)
      Crash_report_rent(ch, reception, GET_EQ(ch,i), &totalcost, &numitems, display, factor);

   if (!numitems)
      {
      act("$n tells you, 'But you are not carrying anything!  Just quit!'",
          FALSE, reception, 0, ch, TO_VICT);
      return (0);
      }
   if (numitems > max_obj_save)
      {
      buf=get_buffer(MAX_INPUT_LENGTH);
      sprintf(buf, "$n tells you, 'Sorry, but I cannot store more than %d items.'",
              max_obj_save);
      act(buf, FALSE, reception, 0, ch, TO_VICT);
      release_buffer(buf);
      return (0);
      }
   if (display)
      {
      buf=get_buffer(MAX_INPUT_LENGTH);
      sprintf(buf, "$n tells you, 'Plus, my %d coin fee..'",
              min_rent_cost * factor);
      act(buf, FALSE, reception, 0, ch, TO_VICT);
      sprintf(buf, "$n tells you, 'For a total of %ld coins%s.'",
              totalcost, (factor == RENT_FACTOR ? " per day" : ""));
      act(buf, FALSE, reception, 0, ch, TO_VICT);
      release_buffer(buf);
      if (totalcost > GET_GOLD(ch)+GET_BANK_GOLD(ch))
         {
         act("$n tells you, '...which I see you can't afford.'",
             FALSE, reception, 0, ch, TO_VICT);
         return (0);
         }
      else if (factor == RENT_FACTOR)
         Crash_rent_deadline(ch, reception, totalcost);
      }
   return (totalcost);
   }



int gen_receptionist(struct char_data * ch, struct char_data * recep,
                     int cmd, char *arg, int mode)
   {
   int cost = 0;
   room_rnum save_room;
   char *action_table[] =
      {"smile", "dance", "sigh", "blush", "burp",
       "cough", "fart", "twiddle", "yawn"
      } ;

   if (!ch->desc || IS_NPC(ch))
      return FALSE;

   if (!cmd && !number(0, 5))
      {
      do_action(recep, NULL, find_command(action_table[number(0, 8)]), 0);
      return FALSE;
      }
   if (!CMD_IS("offer") && !CMD_IS("rent"))
      return FALSE;
   if (!AWAKE(recep))
      {
      send_to_char(ch, "She is unable to talk to you...\r\n");
      return TRUE;
      }
   if (!CAN_SEE(recep, ch))
      {
      act("$n says, 'I don't deal with people I can't see!'", FALSE, recep, 0, 0, TO_ROOM);
      return TRUE;
      }
   if (free_rent)
      {
      act("$n tells you, 'Rent is free here.  Just quit, and your objects will be saved!'",
          FALSE, recep, 0, ch, TO_VICT);
      return 1;
      }
   if (CMD_IS("rent"))
      {
      char *buf;
      if (!(cost = Crash_offer_rent(ch, recep, FALSE, mode)))
         return TRUE;
      buf=get_buffer(SMALL_BUFSIZE);
      if (mode == RENT_FACTOR)
         sprintf(buf, "$n tells you, 'Rent will cost you %d gold coins per day.'", cost);
      else if (mode == CRYO_FACTOR)
         sprintf(buf, "$n tells you, 'It will cost you %d gold coins to be frozen.'", cost);
      act(buf, FALSE, recep, 0, ch, TO_VICT);
      release_buffer(buf);
      if (cost > GET_GOLD(ch)+GET_BANK_GOLD(ch))
         {
         act("$n tells you, '...which I see you can't afford.'",
             FALSE, recep, 0, ch, TO_VICT);
         return TRUE;
         }
      if (cost && (mode == RENT_FACTOR))
         Crash_rent_deadline(ch, recep, cost);

      if (mode == RENT_FACTOR)
         {
         act("$n stores your belongings and helps you into your private chamber.",
             FALSE, recep, 0, ch, TO_VICT);
         Crash_rentsave(ch, cost);
         mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "%s has rented (%d/day, %ld tot.)", GET_NAME(ch),
                 cost, GET_GOLD(ch) + GET_BANK_GOLD(ch));
         }
      else
         {
         /* cryo */
         act("$n stores your belongings and helps you into your private chamber.\r\n"
             "A white mist appears in the room, chilling you to the bone...\r\n"
             "You begin to lose consciousness...",
             FALSE, recep, 0, ch, TO_VICT);
         Crash_cryosave(ch, cost);
         mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "%s has cryo-rented.", GET_NAME(ch));
         SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
         }

      act("$n helps $N into $S private chamber.",FALSE,recep,0,ch,TO_NOTVICT);
      save_room = IN_ROOM(ch);
      extract_char(ch);
      save_char(ch, save_room);
      }
   else
      {
      Crash_offer_rent(ch, recep, TRUE, mode);
      act("$N gives $n an offer.", FALSE, ch, 0, recep, TO_ROOM);
      }
   return TRUE;
   }


SPECIAL(receptionist)
   {
   return (gen_receptionist(ch, (struct char_data *)me, cmd, argument,
                            RENT_FACTOR));
   }


SPECIAL(cryogenicist)
   {
   return (gen_receptionist(ch, (struct char_data *)me, cmd, argument,
                            CRYO_FACTOR));
   }


void Crash_save_all(void)
   {
   struct descriptor_data *d;
   for (d = descriptor_list; d; d = d->next)
      {
      if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character))
         {
         if (PLR_FLAGGED(d->character, PLR_CRASH))
            {
            Crash_crashsave(d->character);
            save_char(d->character, IN_ROOM(d->character));
            REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
            }
         }
      }
   }

int parse_xap_obj(char *filename, struct obj_data **obj,char *line,
                  FILE *fl, int version, int *locate)
   {
   int t[14];
   int zwei = 0;
   int j = 0;
   struct extra_descr_data *new_descr;
   long lVector;
   obj_vnum nr=NOTHING;
   /* first, we get the number. Not too hard. */
   *locate=0;
   if(*line != '#')
      {
      return FALSE;
      }
   if (sscanf(line, "#%ld", &nr) != 1)
      {
      log("SYSERR OBJLOAD: Odd first line in %s obj file: %s",
          filename,line);
      return FALSE;
      }
   /* we have the number, check it, load obj. */
   if (nr == NOTHING)
      {   /* then it is unique */
      *obj = create_obj();
      (*obj)->item_number=NOTHING;
      }
   else if (nr < 0)
      {
      log("SYSERR OBJLOAD: Odd first line in %s obj file: %s "
          "THIS SHOULD NEVER HAPPEN!",
          filename,line);
      return FALSE;
      }
   else
      {
      if(nr >= 999999)
         {
         log("SYSERR OBJLOAD: Odd first line in %s obj file: %s",
             filename,line);
         return FALSE;
         }
      *obj=read_object(nr,VIRTUAL);
      if (!(*obj))
         {
         log("WARNING OBJLOAD:Tried to get obj %ld for %s but it "
             "doesn't exist.", nr,filename);
         return FALSE;
         }
      }

   get_line(fl,line);
   sscanf(line,"%d %d %d %d %d %d %d %d %d %d %d %d %ld",t,t+1,t+2,t+3,t+4,t+5,t+6,t+7,t+8,t+9,t+10,t+11,&lVector);
   *locate=t[0];
   GET_OBJ_EXTRA((*obj)) = t[9];
   GET_OBJ_VROOM((*obj)) = t[10];
   GET_OBJ_TIMER((*obj)) = t[11];

   for(j=0;j<8;j++)
      {
      if(IS_SET(lVector,(1<<j)))
         GET_OBJ_VAL((*obj),j) = t[j+1]; /* locate takes 0 field */
      }

   get_line(fl,line);
   if(version==1)
      {
      sscanf(line,"%d %d %d %d %d %d %d\n",
             t,t+1,t+2,t+3,t+4,t+5,t+6);
      GET_OBJ_SHOP_ORDER((*obj)) = 0;
      }
   else
      {
      sscanf(line,"%d %d %d %d %d %d %d %d\n",
             t,t+1,t+2,t+3,t+4,t+5,t+6,t+7);
      GET_OBJ_SHOP_ORDER((*obj)) = t[7];
      }
   GET_OBJ_CSLOTS((*obj)) = t[0];
   GET_OBJ_TSLOTS((*obj)) = t[1];
   GET_OBJ_OSLOTS((*obj)) = t[2];
   GET_OBJ_EXTRA2((*obj)) = t[3];
   GET_OBJ_EXTRA3((*obj)) = t[4];
   GET_OBJ_ANTI((*obj))   = t[5];
   (*obj)->obj_flags.bitvector = t[6];

   GET_OBJ_DGTIMER((*obj)) = -1;

   get_line(fl,line);
   /* read line check for xap. */
   if(!strcasecmp("XAP",line))
      {  /* then this is a Xap Obj, requires special care */
      char *buf2=get_buffer(2048);
      sprintf(buf2,"1 file: %s  obj: %ld",filename,nr);
      if (((*obj)->name = fread_string(fl, buf2)) == NULL)
         {
         (*obj)->name = "undefined";
         }

      buf2[0]='2';
      if (((*obj)->short_description = fread_string(fl, buf2)) == NULL)
         {
         (*obj)->short_description = "undefined";
         }

      buf2[0]='3';
      if (((*obj)->description = fread_string(fl, buf2)) == NULL)
         {
         (*obj)->description = "undefined";
         }

      buf2[0]='4';
      if (((*obj)->action_description = fread_string(fl, buf2)) == NULL)
         {
         (*obj)->action_description=0;
         }
      if (!get_line(fl, line) ||
              (sscanf(line, "%d %d %d %d %d", t,t+1,t+2,t+3,t+4) != 5))
         {
         log("SYSERR OBJLOAD: Format error in first numeric line "
             "(expecting _x_ args) in %s.  Got %s",filename,line);
         extract_obj((*obj));
         return FALSE;
         }
      (*obj)->obj_flags.type_flag = t[0];
      (*obj)->obj_flags.wear_flags = t[1];
      (*obj)->obj_flags.weight = t[2];
      (*obj)->obj_flags.cost = t[3];
      (*obj)->obj_flags.cost_per_day = t[4];

      /* buf2 is error codes pretty much */
      strcat(buf2, ", after numeric constants (expecting E/#xxx)");

      /* we're clearing these for good luck */

      for (j = 0; j < MAX_OBJ_AFFECT; j++)
         {
         (*obj)->affected[j].location = APPLY_NONE;
         (*obj)->affected[j].modifier = 0;
         }
      /* You have to null out the extradescs when youre parsing a xap_obj.
      This is done right before the extradescs are read. */

      if ((*obj)->ex_description)
         {
         (*obj)->ex_description = NULL;
         }

      get_line(fl,line);
      for (j=zwei=0;!zwei && !feof(fl);)
         {
         switch (*line)
            {
            case 'E':
               CREATE(new_descr, struct extra_descr_data, 1);
               new_descr->keyword = fread_string(fl, buf2);
               new_descr->description = fread_string(fl, buf2);
               new_descr->next = (*obj)->ex_description;
               (*obj)->ex_description = new_descr;
               get_line(fl,line);
               break;
            case 'A':
               if (j >= MAX_OBJ_AFFECT)
                  {
                  log("SYSERR OBJLOAD: Too many object affectations "
                      "in loading rent file: %s:%s",filename,line);
                  get_line(fl, line);
                  break;
                  }
               get_line(fl, line);
               sscanf(line, "%d %d", t, t + 1);

               (*obj)->affected[j].location = t[0];
               (*obj)->affected[j].modifier = t[1];
               j++;
               get_line(fl,line);
               break;

            case '$':
            case '#':
               zwei=1;
               break;
            default:
               zwei=1;
               break;
            }
         }      /* exit our for loop */
      release_buffer(buf2);
      }   /* exit our xap loop */
   return TRUE;
   }


int Crash_load_xapobjs(struct char_data *ch)
   {
   FILE *fl;
   char *filename=get_buffer(MAX_STRING_LENGTH);
   char *line=get_buffer(256);
   int num_of_days;
   int orig_rent_code;
   struct obj_data *temp;
   int locate=0, j, cost,num_objs=0;
   struct obj_data *obj1;
   struct obj_data *cont_row[MAX_BAG_ROW];
   int rentcode,timed,netcost,gold,account,nitems;
   int version;

   get_rent_filename(GET_NAME(ch),filename);
   if(filename == NULL)
      {
      release_buffer(filename);
      release_buffer(line);
      return 1;
      }


   if (!(fl = fopen(filename, "r+b")))
      {
      if (errno != ENOENT)
         {     /* if it fails, NOT because of no file */
         char *buf1 = get_buffer(256);
         sprintf(buf1, "SYSERR OBJLOAD: READING OBJECT FILE %s (5)", filename);
         perror(buf1);
         log("%s", buf1);
         log("SYSERR OBJLOAD: Errno for opening %s = %d",filename,errno);
         send_to_char(ch, "\r\n********************* NOTICE *********************\r\n"
                      "There was a problem loading your objects from disk.\r\n"
                      "Contact a God for assistance.\r\n");
         release_buffer(buf1);
         }
      mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
              "%s entering game with no equipment.", GET_NAME(ch));
      release_buffer(filename);
      release_buffer(line);
      return 1;
      }
   if (!feof(fl))
      get_line(fl,line);

   if(*line == '@')
      {
      if(sscanf(line,"@Version: %d",&version)!=1)
         {
         mudlogf(CMP,LVL_IMMORT,TRUE,
                 "SYSERR OBJLOAD: Format error in %s with line: %s",
                 filename,line);
         exit(1);
         }
      if(!feof(fl))
         get_line(fl,line);
      }
   else
      {
      version=1;
      }

   sscanf(line,"%d %d %d %d %d %d",&rentcode, &timed,
          &netcost,&gold,&account,&nitems);

   if (!free_rent)
      {
      if (rentcode == RENT_RENTED || rentcode == RENT_TIMEDOUT)
         {
         num_of_days = (float) (time(0) - timed) / SECS_PER_REAL_DAY;
         cost = (int) (netcost * num_of_days);
         if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch))
            {
            fclose(fl);
            mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                    "RENTGONE: %s entering game, rented equipment lost (no $).",
                    GET_NAME(ch));
            Crash_crashsave(ch);
            release_buffer(filename);
            release_buffer(line);
            return 2;
            }
         else
            {
            GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
            GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
            save_char(ch, NOWHERE);
            }
         }
      }

   switch (orig_rent_code = rentcode)
      {
      case RENT_RENTED:
         mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "%s un-renting and entering game.(#%ld)", GET_NAME(ch),
                 world[IN_ROOM(ch)].number);
         break;
      case RENT_CRASH:
         mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "%s retrieving crash-saved items and entering game.(#%ld)",
                 GET_NAME(ch), world[IN_ROOM(ch)].number);
         break;
      case RENT_CRYO:
         mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "%s un-cryo'ing and entering game.(#%ld)", GET_NAME(ch),
                 world[IN_ROOM(ch)].number);
         break;
      case RENT_FORCED:
      case RENT_TIMEDOUT:
         mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "%s retrieving force-saved items and entering game.(#%ld)",
                 GET_NAME(ch), world[IN_ROOM(ch)].number);
         break;
      default:
         mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "WARNING OBJLOAD: %s entering game with undefined rent "
                 "code.(#%ld) %d %s",
                 GET_NAME(ch), world[IN_ROOM(ch)].number,rentcode,line);
         break;
      }

   for (j = 0;j < MAX_BAG_ROW;j++)
      cont_row[j] = NULL; /* empty all cont lists (you never know ...) */

   if(!feof(fl))
      get_line(fl, line);
   while (!feof(fl))
      {
      temp=NULL;
      if(parse_xap_obj(filename,&temp,line,fl,version,&locate))
         {
         if(temp != NULL)
            {
            num_objs++;
            if (GET_OBJ_VNUM(temp) != NOTHING) /*bad mail! BAD BAD MAIL!*/
               check_obj(ch,temp);
            auto_equip(ch, temp, locate);
            }
         else
            {
            continue;
            }
         /*
            what to do with a new loaded item:

            if there's a list with <locate> less than 1 below this:
            (equipped items are assumed to have <locate>==0 here) then its
            container has disappeared from the file   *gasp*
            -> put all the list back to ch's inventory
            if there's a list of contents with <locate> 1 below this:
            check if it's a container
            - if so: get it from ch, fill it, and give it back to ch (this way the
            container has its correct weight before modifying ch)
            - if not: the container is missing -> put all the list to ch's inventory

            for items with negative <locate>:
            if there's already a list of contents with the same <locate> put obj to it
            if not, start a new list

            Confused? Well maybe you can think of some better text to be put here ...

            since <locate> for contents is < 0 the list indices are switched to
            non-negative
            */

         if (locate > 0)
            { /* item equipped */
            for (j = MAX_BAG_ROW-1;j > 0;j--)
               if (cont_row[j])
                  { /* no container -> back to ch's inventory */
                  for (;cont_row[j];cont_row[j] = obj1)
                     {
                     obj1 = cont_row[j]->next_content;
                     obj_to_char(cont_row[j], ch);
                     }
                  cont_row[j] = NULL;
                  }
            if (cont_row[0])
               { /* content list existing */
               if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER)
                  {
                  /* rem item ; fill ; equip again */
                  temp = unequip_char(ch, locate-1);
                  temp->contains = NULL; /* should be empty - but who knows */
                  for (;cont_row[0];cont_row[0] = obj1)
                     {
                     obj1 = cont_row[0]->next_content;
                     obj_to_obj(cont_row[0], temp);
                     }
                  equip_char(ch, temp, locate-1);
                  }
               else
                  { /* object isn't container -> empty content list */
                  for (;cont_row[0];cont_row[0] = obj1)
                     {
                     obj1 = cont_row[0]->next_content;
                     obj_to_char(cont_row[0], ch);
                     }
                  cont_row[0] = NULL;
                  }
               }
            }
         else
            { /* locate <= 0 */
            for (j = MAX_BAG_ROW-1;j > -locate;j--)
               if (cont_row[j])
                  { /* no container -> back to ch's inventory */
                  for (;cont_row[j];cont_row[j] = obj1)
                     {
                     obj1 = cont_row[j]->next_content;
                     obj_to_char(cont_row[j], ch);
                     }
                  cont_row[j] = NULL;
                  }

            if (j == -locate && cont_row[j])
               { /* content list existing */
               if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER)
                  {
                  /* take item ; fill ; give to char again */
                  obj_from_char(temp);
                  temp->contains = NULL;
                  for (;cont_row[j];cont_row[j] = obj1)
                     {
                     obj1 = cont_row[j]->next_content;
                     obj_to_obj(cont_row[j], temp);
                     }
                  obj_to_char(temp, ch); /* add to inv first ... */
                  }
               else
                  { /* object isn't container -> empty content list */
                  for (;cont_row[j];cont_row[j] = obj1)
                     {
                     obj1 = cont_row[j]->next_content;
                     obj_to_char(cont_row[j], ch);
                     }
                  cont_row[j] = NULL;
                  }
               }

            if (locate < 0 && locate >= -MAX_BAG_ROW)
               {
               /* let obj be part of content list
               but put it at the list's end thus having the items
               in the same order as before renting */
               obj_from_char(temp);
               if ((obj1 = cont_row[-locate-1]))
                  {
                  while (obj1->next_content)
                     obj1 = obj1->next_content;
                  obj1->next_content = temp;
                  }
               else
                  cont_row[-locate-1] = temp;
               }
            } /* locate less than zero */
         }
      else
         {
         if(!feof(fl))
            get_line(fl,line);
         else
            break;
         }
      }

   /* Little hoarding check. -gg 3/1/98 */
   mudlogf(NRM, MAX(LVL_GOD,GET_INVIS_LEV(ch)), TRUE,
           "%s (level %d) has %d objects (max %d). rentcode: %d",
           GET_NAME(ch), GET_LEVEL(ch), num_objs, max_obj_save,orig_rent_code);

   fclose(fl);
   release_buffer(filename);
   release_buffer(line);

   if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
      return 0;
   else
      return 1;
   }


void get_rent_filename(char *pztName,char *pztFileName)
   {
   if(xap_objs == 0)
      {
      if (!get_filename(pztName, pztFileName, CRASH_FILE))
         {
         mudlogf(CMP,LVL_IMMORT,TRUE,
                 "SYSERR: Can't get name for %s's save file.",
                 pztName);
         return;
         }
      }
   else if(xap_objs == 1)
      {
      if (!get_filename(pztName, pztFileName, NEW_OBJ_FILES))
         {
         mudlogf(CMP,LVL_IMMORT,TRUE,
                 "SYSERR: Can't get name for %s's save file.",
                 pztName);
         return;
         }
      }
   else if(xap_objs == 2)
      {
      if (!get_filename(pztName, pztFileName, REIMB_FILE))
         {
         mudlogf(CMP,LVL_IMMORT,TRUE,
                 "SYSERR: Can't get name for %s's reimb save file.",
                 pztName);
         return;
         }
      }
   else
      {
      log("ERROR: xap_objs somehow got set to %d!  FATAL!",xap_objs);
      exit(1);
      }
   }

void check_obj(struct char_data *ch,struct obj_data *obj)
   {
   int warn=FALSE;
   struct obj_data *temp;
   temp = &obj_proto[GET_OBJ_RNUM(obj)];
   if(!IS_REMORT_ITEM(obj) && IS_REMORT_ITEM(temp))
      {
      if (warn)
         {
         mudlogf(CMP,LVL_ADMIN,TRUE,"SYSERR: Player %s is unrenting with %s(#%ld), "
                 "which should be a REMORT item but is not. FIXING...", 
                 GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj));
         }
      SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_REMORT);
      }
   else if(IS_REMORT_ITEM(obj) && !IS_REMORT_ITEM(temp))
      {
      if (warn) 
         {
         mudlogf(CMP,LVL_ADMIN,TRUE,"SYSERR: Player %s is unrenting with %s(#%ld), "
                 "which is set as a REMORT item but should not. FIXING...",
                 GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj));
         }
      REMOVE_BIT(GET_OBJ_EXTRA2(obj), ITEM2_REMORT);
      }
   if(!IS_DBLREMORT_ITEM(obj) && IS_DBLREMORT_ITEM(temp)) 
      {
      if (warn) 
         {
         mudlogf(CMP,LVL_ADMIN,TRUE,"SYSERR: Player %s is unrenting with %s(#%ld), "
                 "which should be a DOUBLE REMORT item but is not. FIXING...",
                 GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj));
         }
      SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_DBLREMORT);
      }
   else if(IS_DBLREMORT_ITEM(obj) && !IS_DBLREMORT_ITEM(temp))
      {
      if (warn) 
         {
         mudlogf(CMP,LVL_ADMIN,TRUE,"SYSERR: Player %s is unrenting with %s(#%ld), "
                 "which is set as a DOUBLE REMORT item but should not. FIXING...",
                 GET_NAME(ch), GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj));
         }
      REMOVE_BIT(GET_OBJ_EXTRA2(obj), ITEM2_DBLREMORT);
      }

   if(GET_OBJ_TYPE(obj) == ITEM_WEAPON)
      {
      if((GET_LEVEL(ch) < LVL_IMMORT) && 
         ((GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(temp, 1)) ||(GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(temp, 2)+1)))
         {
         mudlogf(CMP,LVL_ADMIN,TRUE,
                 "FORGE-WEAPON: %s loaded with %s at %ldd%ld. It should be %ldd%ld",
                 GET_NAME(ch),((obj->short_description) ? obj->short_description : "<None>"),
                 GET_OBJ_VAL(obj,1),GET_OBJ_VAL(obj,2),GET_OBJ_VAL(temp,1),GET_OBJ_VAL(temp,2));
         }
      }

   }

