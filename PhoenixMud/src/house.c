/* ************************************************************************
*   File: house.c                                       Part of CircleMUD * 
*  Usage: Handling of player houses                                       * 
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
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "vnum.h"

extern struct room_data *world;
extern struct index_data *obj_index;
extern struct obj_data *obj_proto;
extern int xap_objs;

void Crash_count_items(struct obj_data * obj, long *nitems);
struct obj_data *Obj_from_store(struct obj_file_elem object,sh_int *location);
int Obj_to_store(struct obj_data * obj, FILE * fl,int location);
int find_name(char *name);
int Crash_is_unrentable(struct obj_data * obj);
int parse_xap_obj(char *filename, struct obj_data **obj,char *line,
                  FILE *fl, int version, int *locate);
void check_house_obj(room_rnum housenum,struct obj_data *obj);

struct house_control_rec house_control[MAX_HOUSES];
int num_of_houses = 0;


/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
int House_get_filename(int vnum, char *filename)
   {
   if (vnum < 0)
      return 0;

   if(xap_objs==0)
      sprintf(filename, "house/%d.house", vnum);
   else
      sprintf(filename, "house/%d.aschouse",vnum);

   return 1;
   }


/* Load all objects for a house */
int House_load(room_vnum vnum)
   {
   FILE *fl;
   char *Fname;
   struct obj_file_elem object;
   room_rnum rnum;
   sh_int i;
   char *line;
   char *buf1;
   char *buf2;
   int locate=0;
   int version;
   struct obj_data *temp = NULL;

   if ((rnum = real_room(vnum)) == -1)
      return 0;
   Fname  = get_buffer(MAX_STRING_LENGTH);
   if (!House_get_filename(vnum, Fname))
      {
      release_buffer(Fname);
      return 0;
      }
   if (!(fl = fopen(Fname, "r+b")))
      {
      /* no file found */
      release_buffer(Fname);
      return 0;
      }
   if(!xap_objs)
      {
      while (!feof(fl))
         {
         fread(&object, sizeof(struct obj_file_elem), 1, fl);
         if (ferror(fl))
            {
            perror("SYSERR: Reading house file: House_load.");
            fclose(fl);
            release_buffer(Fname);
            return 0;
            }
         if (!feof(fl))
            obj_to_room(Obj_from_store(object,&i), rnum);
         }
      }
   else
      {
      line=get_buffer(256);
      buf1=get_buffer(2048);
      buf2=get_buffer(2048);

      if(!feof(fl))
         get_line(fl, line);

      if(*line == '@')
         {
         if(sscanf(line,"@Version: %d",&version)!=1)
            {
            mudlogf(CMP,LVL_IMMORT,TRUE,
                    "SYSERR OBJLOAD: Format error in %s with line: %s",
                    Fname,line);
            exit(1);
            }
         if(!feof(fl))
            get_line(fl,line);
         }
      else
         {
         version=1;
         }

      while (!feof(fl))
         {
         temp=NULL;
         /* first, we get the number. Not too hard. */
         if(parse_xap_obj(Fname,&temp,line,fl,version,&locate))
            {
            if(temp!=NULL)
               {
               check_house_obj(rnum,temp);
               obj_to_room(temp, rnum);
               }
            }
         else
            {
            if(!feof(fl))
               get_line(fl, line);
            else
               break;
            }
         }
      release_buffer(buf2);
      release_buffer(buf1);
      release_buffer(line);
      }
   fclose(fl);
   release_buffer(Fname);
   return 1;
   }


/* Save all objects for a house (recursive; initial call must be followed
   by a call to House_restore_weight)  Assumes file is open already. */
int House_save(struct obj_data * obj, FILE * fp)
   {
   struct obj_data *tmp;
   int result;

   if (obj)
      {
      House_save(obj->contains, fp);
      House_save(obj->next_content, fp);
      if(Crash_is_unrentable(obj)==0)
         {
         result = Obj_to_store(obj, fp,0);
         if (!result)
            return 0;

         for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
            GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);
         }
      }
   return 1;
   }


/* restore weight of containers after House_save has changed them for saving */
void House_restore_weight(struct obj_data * obj)
   {
   if (obj)
      {
      House_restore_weight(obj->contains);
      House_restore_weight(obj->next_content);
      if (obj->in_obj)
         GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
      }
   }


/* Save all objects in a house */
void House_crashsave(room_vnum vnum)
   {
   long nitems = 0, excess = 0;
   room_rnum rnum;
   char *buf;
   FILE *fp;
   struct obj_data *j, *k;

   if ((rnum = real_room(vnum)) == -1)
      return;
   buf = get_buffer(MAX_STRING_LENGTH);
   if (!House_get_filename(vnum, buf))
      {
      release_buffer(buf);
      return;
      }
   if (!(fp = fopen(buf, "wb")))
      {
      release_buffer(buf);
      perror("SYSERR: Error saving house file");
      return;
      }
   if(fprintf(fp,"@Version: %d\n",CUR_POBJ_VER)<1)
      {
      perror("SYSERR: Writing rent version");
      log("SYSERR OBJSAVE: Error writing rent version for house %s %d",
          buf,errno);
      release_buffer(buf);
      return ;
      }
   release_buffer(buf);

   /* clear out items in donation, oldest first to go */
   if (ROOM_FLAGGED(rnum, ROOM_DONATION) || ROOM_FLAGGED(rnum, ROOM_CLAN))
      {
      /* first clear out undesirable contents */
      for (j = world[rnum].contents; j; j = k)
         {
         k = j->next_content;
         if ((GET_OBJ_TYPE(j)==ITEM_FOOD) ||
             (GET_OBJ_TYPE(j)==ITEM_DRINKCON) ||
             (GET_OBJ_VNUM(j)==VNUM_CREATE_LIGHT) ||
             (GET_OBJ_VNUM(j)==VNUM_CONT_LIGHT))
            extract_obj(j);
         }

      /* now count items in room */
      for (j = world[rnum].contents; j; j = j->next_content, ++nitems)
         ;

      /* now clean excess */
      if (nitems > MAX_OBJ_DONATION)
         {
         excess = nitems - MAX_OBJ_DONATION;
         log("Purging excessive items in donation rooms... " 
               "(%ld items, excess of %ld)", nitems, excess);
         for (j = world[rnum].contents; j && (nitems>0); j = k, nitems--)
            {
            k = j->next_content;
            if ((nitems <= excess) && !IS_CORPSE(j))
               {
               /*log("...purging %s. #%ld", GET_OBJ_NAME(j), nitems);*/
               extract_obj(j);
               }
            }
         }
      }

   
   if (!House_save(world[rnum].contents, fp))
      {
      fclose(fp);
      return;
      }
   fclose(fp);
   House_restore_weight(world[rnum].contents);
   REMOVE_BIT(ROOM_FLAGS(rnum), ROOM_HOUSE_CRASH);
   }


/* ---- 4.2 persistent potion room -------------------------------------------
 * Room POTION_ROOM_VNUM (3055, one above the temple altar room 3054) is the
 * donation store for brewed potions and scribed scrolls capped out at camp
 * (do_quit donate_excess_brews). It sits in zone 30, whose reset does not load
 * into it, so these two carry its floor across reboots, modeled on
 * House_crashsave/House_load and reusing the same object serializer. The room
 * is ROOM_NO_DECAY, so nothing in it ages. */
#define POTION_ROOM_VNUM  3055
#define POTION_ROOM_FILE  "house/potion_room.aschouse"

void save_potion_room(void)
   {
   room_rnum rnum;
   FILE *fp;

   if ((rnum = real_room(POTION_ROOM_VNUM)) == NOWHERE)
      return;
   if (!(fp = fopen(POTION_ROOM_FILE, "wb")))
      {
      perror("SYSERR: Error saving potion room file");
      return;
      }
   if (fprintf(fp, "@Version: %d\n", CUR_POBJ_VER) < 1)
      {
      log("SYSERR OBJSAVE: Error writing potion room version");
      fclose(fp);
      return;
      }
   if (!House_save(world[rnum].contents, fp))
      {
      fclose(fp);
      return;
      }
   fclose(fp);
   House_restore_weight(world[rnum].contents);
   }

void load_potion_room(void)
   {
   FILE *fl;
   struct obj_file_elem object;
   room_rnum rnum;
   sh_int i;
   char *line;
   int locate = 0;
   int version;
   struct obj_data *temp = NULL;

   if ((rnum = real_room(POTION_ROOM_VNUM)) == NOWHERE)
      return;
   if (!(fl = fopen(POTION_ROOM_FILE, "r+b")))
      return;  /* no file yet — the room boots empty, which is correct */
   if (!xap_objs)
      {
      while (!feof(fl))
         {
         fread(&object, sizeof(struct obj_file_elem), 1, fl);
         if (ferror(fl))
            {
            perror("SYSERR: Reading potion room file: load_potion_room.");
            fclose(fl);
            return;
            }
         if (!feof(fl))
            obj_to_room(Obj_from_store(object, &i), rnum);
         }
      }
   else
      {
      line = get_buffer(256);
      if (!feof(fl))
         get_line(fl, line);
      if (*line == '@')
         {
         if (sscanf(line, "@Version: %d", &version) != 1)
            version = CUR_POBJ_VER;
         if (!feof(fl))
            get_line(fl, line);
         }
      else
         version = 1;
      while (!feof(fl))
         {
         temp = NULL;
         if (parse_xap_obj(POTION_ROOM_FILE, &temp, line, fl, version, &locate))
            {
            if (temp != NULL)
               obj_to_room(temp, rnum);
            }
         else
            {
            if (!feof(fl))
               get_line(fl, line);
            else
               break;
            }
         }
      release_buffer(line);
      }
   fclose(fl);
   }


/* Delete a house save file */
void House_delete_file(int vnum)
   {
   char *Fname = get_buffer(MAX_INPUT_LENGTH);
   FILE *fl;

   if (!House_get_filename(vnum, Fname))
      {
      release_buffer(Fname);
      return;
      }
   if (!(fl = fopen(Fname, "rb")))
      {
      if (errno != ENOENT)
         {
         log("SYSERR: Error deleting house file #%d. (1):%s",vnum,
             strerror(errno));
         }
      release_buffer(Fname);
      return;
      }
   fclose(fl);
   if (remove(Fname) < 0)
      {
      log("SYSERR: Error deleting house file #%d. (2):%s",vnum,
          strerror(errno));
      }
   release_buffer(Fname);
   }


/* List all objects in a house file */
void House_listrent(struct char_data * ch, room_vnum vnum)
   {
   FILE *fl;
   char *Fname = get_buffer(MAX_STRING_LENGTH);
   struct obj_file_elem object;
   struct obj_data *obj;
   sh_int i;

   if (!House_get_filename(vnum, Fname))
      {
      release_buffer(Fname);
      return;
      }

   if (!(fl = fopen(Fname, "rb")))
      {
      send_to_char(ch, "No objects on file for house #%ld.\r\n", vnum);
      release_buffer(Fname);
      return;
      }
   release_buffer(Fname);

   while (!feof(fl))
      {
      fread(&object, sizeof(struct obj_file_elem), 1, fl);
      if (ferror(fl))
         {
         fclose(fl);
         return;
         }
      if (!feof(fl) && (obj = Obj_from_store(object,&i)) != NULL)
         {
         send_to_char(ch, " [%5ld] (%5dau) %s\r\n",
                      GET_OBJ_VNUM(obj), GET_OBJ_RENT(obj),
                      obj->short_description);
         free_obj(obj);
         }
      }
   fclose(fl);
   }




/******************************************************************
 *  Functions for house administration (creation, deletion, etc.  * 
 *****************************************************************/

int find_house(room_vnum vnum)
   {
   int i;

   for (i = 0; i < num_of_houses; i++)
      if (house_control[i].vnum == vnum)
         return i;

   return -1;
   }



/* Save the house control information */
void House_save_control(void)
   {
   FILE *fl;

   if (!(fl = fopen(HCONTROL_FILE, "wb")))
      {
      perror("SYSERR: Unable to open house control file");
      return;
      }
   /* write all the house control recs in one fell swoop.  Pretty nifty, eh? */
   fwrite(house_control, sizeof(struct house_control_rec), num_of_houses, fl);

   fclose(fl);
   }


/* call from boot_db - will load control recs, load objs, set atrium bits */
/* should do sanity checks on vnums & remove invalid records */
void House_boot(void)
   {
   struct house_control_rec temp_house;
   room_rnum real_house, real_atrium;
   FILE *fl;

   memset((char *)house_control,0,sizeof(struct house_control_rec)*MAX_HOUSES);

   if (!(fl = fopen(HCONTROL_FILE, "rb")))
      {
      if (errno == ENOENT)
         log("   House control file '%s' does not exist.", HCONTROL_FILE);
      else
         perror("SYSERR: " HCONTROL_FILE);
      return;
      }
   while (!feof(fl) && num_of_houses < MAX_HOUSES)
      {
      fread(&temp_house, sizeof(struct house_control_rec), 1, fl);

      if (feof(fl))
         break;

      if (find_name(temp_house.owner) == -1)
         continue;   /* owner no longer exists -- skip */

      if ((real_house = real_room(temp_house.vnum)) < 0)
         continue;   /* this vnum doesn't exist -- skip */

      if (find_house(temp_house.vnum) >= 0)
         continue;   /* this vnum is already a hosue -- skip */

      if ((real_atrium = real_room(temp_house.atrium)) < 0)
         continue;   /* house doesn't have an atrium -- skip */

      if (temp_house.exit_num < 0 || temp_house.exit_num >= NUM_OF_DIRS)
         continue;   /* invalid exit num -- skip */

      if (TOROOM(real_house, temp_house.exit_num) != real_atrium)
         continue;   /* exit num mismatch -- skip */

      house_control[num_of_houses++] = temp_house;

      SET_BIT(ROOM_FLAGS(real_house), ROOM_HOUSE | ROOM_PRIVATE);
      SET_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
      House_load(temp_house.vnum);
      }

   fclose(fl);
   House_save_control();
   }



/* "House Control" functions */

char *HCONTROL_FORMAT =
   "Usage: hcontrol build <house vnum> <exit direction> <player name>\r\n"
   "       hcontrol destroy <house vnum>\r\n"
   "       hcontrol pay <house vnum>\r\n"
   "       hcontrol show\r\n";

#define NAME(x) ((temp = get_name_by_id(x)) == NULL ? "<UNDEF>" : temp)

void hcontrol_list_houses(struct char_data * ch)
   {
   int i, j;
   char *timestr, *built_on, *last_pay, *own_name, *buf;
   room_rnum rnum;
   long nitems=0;

   if (!num_of_houses)
      {
      send_to_char(ch,"No houses have been defined.\r\n");
      return;
      }

   built_on = get_buffer(128);
   last_pay = get_buffer(128);
   own_name = get_buffer(128);
   buf = get_buffer(32750);

   strcpy(buf, "Address  Atrium  Build Date  Guests  Owner        Last Paymt Num Items\r\n");
   strcat(buf, "-------  ------  ----------  ------  ------------ ---------- ---------\r\n");

   for (i = 0; i < num_of_houses; i++)
      {
      /* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
      if (house_control[i].built_on)
         {
         timestr = asctime(localtime(&(house_control[i].built_on)));
         *(timestr + 10) = '\0';
         strcpy(built_on, timestr);
         }
      else
         strcpy(built_on, "Unknown");

      if (house_control[i].last_payment)
         {
         timestr = asctime(localtime(&(house_control[i].last_payment)));
         *(timestr + 10) = '\0';
         strcpy(last_pay, timestr);
         }
      else
         strcpy(last_pay, "None");

      strcpy(own_name, house_control[i].owner);
      nitems=0;
      if((rnum=real_room(house_control[i].vnum))==-1)
         nitems=-1;
      else
         Crash_count_items(world[rnum].contents,&nitems);

      sprintf(buf+strlen(buf), "%7ld %7ld  %-10s    %2d    %-12s %s %9ld\r\n",
              house_control[i].vnum, house_control[i].atrium, built_on,
              house_control[i].num_of_guests, CAP(own_name), last_pay,nitems);

      if (house_control[i].num_of_guests)
         {
         strcat(buf, "     Guests: ");
         for (j = 0; j < house_control[i].num_of_guests; j++)
            {
            sprintf(own_name, "%s ", house_control[i].guests[j]);
            strcat(buf, CAP(own_name));
            }
         strcat(buf, "\r\n");
         }
      if(strlen(buf)>32500)
         {
         strcat(buf,"House list too long!\r\n");
         break;
         }
      }

   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");
   else
      send_to_char(ch,"%s",buf);
   release_buffer(buf);
   release_buffer(built_on);
   release_buffer(last_pay);
   release_buffer(own_name);
   }




void hcontrol_build_house(struct char_data * ch, char *arg)
   {
   char *arg1;
   struct house_control_rec temp_house;
   int exit_num;
   room_vnum virt_house,virt_atrium;
   room_rnum  real_house, real_atrium;
   char owner[MAX_NAME_LENGTH];

   if (num_of_houses >= MAX_HOUSES)
      {
      send_to_char(ch,"Max houses already defined.\r\n");
      return;
      }

   /* first arg: house's vnum */
   arg1 = get_buffer(MAX_INPUT_LENGTH);
   arg = one_argument(arg, arg1);
   if (!*arg1)
      {
      send_to_char(ch, "%s", HCONTROL_FORMAT);
      release_buffer(arg1);
      return;
      }
   virt_house = atoi(arg1);
   release_buffer(arg1);
   if ((real_house = real_room(virt_house)) < 0)
      {
      send_to_char(ch,"No such room exists.\r\n");
      return;
      }
   if ((find_house(virt_house)) >= 0)
      {
      send_to_char(ch,"House already exists.\r\n");
      return;
      }

   /* second arg: direction of house's exit */
   arg1 = get_buffer(MAX_INPUT_LENGTH);
   arg = one_argument(arg, arg1);
   if (!*arg1)
      {
      send_to_char(ch, "%s", HCONTROL_FORMAT);
      release_buffer(arg1);
      return;
      }
   if ((exit_num = search_block(arg1, dirs, FALSE)) < 0)
      {
      send_to_char(ch, "'%s' is not a valid direction.\r\n", arg1);
      release_buffer(arg1);
      return;
      }
   release_buffer(arg1);

   if (TOROOM(real_house, exit_num) == NOWHERE)
      {
      send_to_char(ch,"There is no exit %s from room %ld.\r\n", dirs[exit_num],
                   virt_house);
      return;
      }
   real_atrium = TOROOM(real_house, exit_num);
   virt_atrium = GET_ROOM_VNUM(real_atrium);

   if ((TOROOM(real_atrium, rev_dir[exit_num]) != real_house) &&
           !ROOM_FLAGGED(real_house,ROOM_DONATION))
      {
      send_to_char(ch,"A house's exit must be a two-way door.\r\n");
      return;
      }

   /* third arg: player's name */
   arg1 = get_buffer(MAX_INPUT_LENGTH);
   one_argument(arg, arg1);
   if (!*arg1)
      {
      send_to_char(ch, "%s", HCONTROL_FORMAT);
      release_buffer(arg1);
      return;
      }
   if (get_id_by_name(arg1) < 0)
      {
      send_to_char(ch, "Unknown player '%s'.\r\n", arg1);
      release_buffer(arg1);
      return;
      }
   else
      strcpy(owner,arg1);

   release_buffer(arg1);
   temp_house.mode = HOUSE_PRIVATE;
   temp_house.vnum = virt_house;
   temp_house.atrium = virt_atrium;
   temp_house.exit_num = exit_num;
   temp_house.built_on = time(0);
   temp_house.last_payment = 0;
   strcpy(temp_house.owner,owner);
   temp_house.num_of_guests = 0;

   house_control[num_of_houses++] = temp_house;

   SET_BIT(ROOM_FLAGS(real_house), ROOM_HOUSE | ROOM_PRIVATE);
   SET_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
   House_crashsave(virt_house);

   send_to_char(ch,"House built.  Mazel tov!\r\n");
   House_save_control();
   }



void hcontrol_destroy_house(struct char_data * ch, char *arg)
   {
   int i, j;
   room_rnum real_atrium, real_house;

   if (!*arg)
      {
      send_to_char(ch, "%s", HCONTROL_FORMAT);
      return;
      }
   if ((i = find_house(atoi(arg))) < 0)
      {
      send_to_char(ch, "Unknown house.\r\n");
      return;
      }
   if ((real_atrium = real_room(house_control[i].atrium)) < 0)
      log("SYSERR: House %d had invalid atrium %ld!",atoi(arg),
          house_control[i].atrium);
   else
      REMOVE_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

   if ((real_house = real_room(house_control[i].vnum)) < 0)
      log("SYSERR: House %d had invalid vnum %ld!",atoi(arg),
          house_control[i].vnum);
   else
      REMOVE_BIT(ROOM_FLAGS(real_house),
                 ROOM_HOUSE | ROOM_PRIVATE | ROOM_HOUSE_CRASH);

   House_delete_file(house_control[i].vnum);

   for (j = i; j < num_of_houses - 1; j++)
      house_control[j] = house_control[j + 1];

   num_of_houses--;

   send_to_char(ch, "House deleted.\r\n");
   House_save_control();

   /*
    * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums, 
    * just in case the house we just deleted shared an atrium with another 
    * house.  --JE 9/19/94 
    */
   for (i = 0; i < num_of_houses; i++)
      if ((real_atrium = real_room(house_control[i].atrium)) >= 0)
         SET_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
   }


void hcontrol_pay_house(struct char_data * ch, char *arg)
   {
   int i;

   if (!*arg)
      send_to_char(ch, "%s", HCONTROL_FORMAT);
   else if ((i = find_house(atoi(arg))) < 0)
      send_to_char(ch, "Unknown house.\r\n");
   else
      {
      mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
              "HOUSE: Payment for house %s collected by %s.", arg,
              GET_NAME(ch));
      house_control[i].last_payment = time(0);
      House_save_control();
      send_to_char(ch, "Payment recorded.\r\n");
      }
   }


/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
   {
   char *arg1 = get_buffer(MAX_INPUT_LENGTH);
   char *arg2 = get_buffer(MAX_INPUT_LENGTH);
   int valid_user=FALSE;

   half_chop(argument, arg1, arg2);
   if(GET_LEVEL(ch)>=LVL_SIMP)
      valid_user=TRUE;
      else if(!strcmp("Ceria",GET_NAME(ch)))
         valid_user=TRUE;
      else if(!strcmp("Nomikos",GET_NAME(ch)))
         valid_user=TRUE;
   /*   else if(!strcmp("Siriah",GET_NAME(ch)))
         valid_user=TRUE;*/

   if (is_abbrev(arg1, "build")&&valid_user)
      hcontrol_build_house(ch, arg2);
   else if (is_abbrev(arg1, "destroy")&&valid_user)
      hcontrol_destroy_house(ch, arg2);
   else if (is_abbrev(arg1, "pay")&&valid_user)
      hcontrol_pay_house(ch, arg2);
   else if (is_abbrev(arg1, "show"))
      hcontrol_list_houses(ch);
   else
      send_to_char(ch, "%s", HCONTROL_FORMAT);

   release_buffer(arg2);
   release_buffer(arg1);
   }


/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
   {
   int i, j, id;
   int not_owner;
   char  *arg = get_buffer(MAX_INPUT_LENGTH);


   one_argument(argument, arg);


   if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
      send_to_char(ch, "You must be in your house to set guests.\r\n");
   else if ((i = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) < 0)
      send_to_char(ch, "Um.. this house seems to be screwed up.\r\n");
   else if ((not_owner = (str_cmp(GET_NAME(ch),house_control[i].owner)!=0)) &&
            GET_LEVEL(ch)<LVL_ADMIN)
      send_to_char(ch,"Only the primary owner can set guests.\r\n");
   else if (!*arg)
      {
      send_to_char(ch, "Guests of %s:\r\n",
                   not_owner?house_control[i].owner:"your house");
      if (house_control[i].num_of_guests == 0)
         send_to_char(ch, "  None.\r\n");
      else
         {
         for (j = 0; j < house_control[i].num_of_guests; j++)
            {
            send_to_char(ch,"%s\r\n",house_control[i].guests[j]);
            }
         }

      }
   else if (not_owner && GET_LEVEL(ch)<LVL_IMPL)
      send_to_char(ch,"Only the primary owner can set guests.\r\n");
   else if ((id = get_id_by_name(arg)) < 0)
      send_to_char(ch, "No such player.\r\n");
   else
      {
      for (j = 0; j < house_control[i].num_of_guests; j++)
         if (str_cmp(house_control[i].guests[j],arg)==0)
            {
            for (; j < house_control[i].num_of_guests; j++)
               strcpy(house_control[i].guests[j],
                      house_control[i].guests[j + 1]);
            house_control[i].num_of_guests--;
            House_save_control();
            send_to_char(ch, "Guest deleted.\r\n");
            release_buffer(arg);
            j++;
            for(;j<MAX_GUESTS;j++)
               strcpy(house_control[i].guests[j],"\0");
            return;
            }
      if(house_control[i].num_of_guests<MAX_GUESTS)
         {
         j = house_control[i].num_of_guests++;
         strcpy(house_control[i].guests[j],arg);
         House_save_control();
         send_to_char(ch, "Guest added.\r\n");
         }
      else
         {
         send_to_char(ch, "You cannot have any more guests.\r\n");
         }
      }
   release_buffer(arg);
   }



/* Misc. administrative functions */


/* crash-save all the houses */
void House_save_all(void)
   {
   int i;
   room_rnum real_house;

   for (i = 0; i < num_of_houses; i++)
      if ((real_house = real_room(house_control[i].vnum)) != NOWHERE)
         if (ROOM_FLAGGED(real_house, ROOM_HOUSE_CRASH))
            House_crashsave(house_control[i].vnum);
   }


/* note: arg passed must be house vnum, so there. */
int House_can_enter(struct char_data * ch, room_vnum house)
   {
   int i, j;

   if (GET_LEVEL(ch) >= LVL_GRGOD || (i = find_house(house)) < 0)
      return 1;

   switch (house_control[i].mode)
      {
      case HOUSE_PRIVATE:
         if (str_cmp(GET_NAME(ch), house_control[i].owner)==0)
            return 1;
         for (j = 0; j < house_control[i].num_of_guests; j++)
            if (str_cmp(GET_NAME(ch), house_control[i].guests[j])==0)
               return 1;
         return 0;
         break;
      }

   return 0;
   }
void check_house_obj(room_rnum housenum,struct obj_data *obj)
   {
   struct obj_data *temp;
   temp = &obj_proto[GET_OBJ_RNUM(obj)];
   if(GET_OBJ_TYPE(obj) == ITEM_WEAPON)
      {
      if((GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(temp, 1)) ||(GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(temp, 2)+1))
         {
         mudlogf(CMP,LVL_ADMIN,TRUE,
                 "FORGE-WEAPON:(house) %s[%ld] loaded with %s at %ldd%ld. It should be %ldd%ld",
                 world[housenum].name,world[housenum].number,
                 ((obj->short_description) ? obj->short_description : "<None>"),
                 GET_OBJ_VAL(obj,1),GET_OBJ_VAL(obj,2),GET_OBJ_VAL(temp,1),GET_OBJ_VAL(temp,2));
         }
      }

   }

