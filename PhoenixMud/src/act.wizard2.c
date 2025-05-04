/* ************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
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
#include "screen.h"
#include "olc.h"
#include "spec_assign.h"
#include "shop.h"
#include "clan.h"
#include "path.h"
#include "dg_scripts.h"
#include "constants.h"
#include "time.h"
#include "assemblies.h"

extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct index_data *obj_index;
extern struct index_data *mob_index;
extern obj_rnum top_of_objt;
extern mob_rnum top_of_mobt;
extern zone_rnum top_of_zone_table;
extern struct zone_data *zone_table;
extern struct room_data *world;
int damage_obj_range(int base,int pos,int material,int ImmBit);
int damage_obj_chance(int dex, int wis);

ACMD(do_get_obj_list)
   {
   int nr=0;
   char *buf = get_buffer(32750);
   char *buf1=get_buffer(SMALL_BUFSIZE);
   /*char *buf2=get_buffer(SMALL_BUFSIZE);*/

   for(nr=0;nr<=top_of_objt;nr++)
      {
      sprintf(buf,"%ld|%s|%s|%d|%d|%s|",obj_index[nr].vnum,
              obj_proto[nr].short_description,obj_proto[nr].name,
              obj_proto[nr].obj_flags.cost,
              obj_proto[nr].obj_flags.weight,
              material_types[obj_proto[nr].material]);
      sprinttype(obj_proto[nr].obj_flags.type_flag, item_types, buf1);
      sprintf(buf+strlen(buf),"%s|",buf1);

      }

   }

ACMD(do_test)
   {
   int j;
   int base;
   float total_chance=0.0;
   int range;
   int chance;
   float percent =0.0;
   base = atoi(argument);

   send_to_char(ch,"Testing base = %d\r\n",base);
   for(j=1;j<NUM_WEARS;j++)
      {
      range=damage_obj_range(base,j,0,0);
      chance = damage_obj_chance(GET_DEX(ch), GET_WIS(ch));
      percent = (float)chance/(float)range;
      total_chance += percent;
      send_to_char(ch,"%s %d in %d = %f\r\n",where[j],chance,range,percent);
      }
   send_to_char(ch,"%f total_chance\r\n",total_chance);
   }

#define ZCOM zone_table[zone].cmd[cmd_no]  /*from DB.C*/

/*
  type = 'O' or 'M'
  which= real num.
  name = common name of mob or object  
*/
void check_load_status (char type, mob_rnum which, struct char_data *ch, char *name)
   {
   zone_rnum zone;
   int cmd_no;
   room_vnum lastroom_v = 0;
   room_rnum lastroom_r = 0;
   mob_rnum lastmob_r = 0;
   char *mobname=get_buffer(100);
   char *buf = get_buffer(32750);
   struct char_data *mob;
   struct obj_data *obj = NULL;

   sprintf(buf, "Checking load info for %s...\r\n", name);

   for (zone = 0; zone <= top_of_zone_table; zone++)
      {
      for (cmd_no = 0; ZCOM.command != 'S'; cmd_no++)
         {
         if (type == 'm' || type == 'M')
            {
            switch (ZCOM.command)
               {
            case 'M':                   /* read a mobile */
               if (ZCOM.arg1 == which)
                  sprintf(buf + strlen(buf), "  [%5ld] %s (%ld Max)\r\n",
                          world[ZCOM.arg3].number, world[ZCOM.arg3].name, ZCOM.arg2);
               break;
            case 'R':                   /* rem mob from room */
               lastroom_v = world[ZCOM.arg1].number;
               lastroom_r = ZCOM.arg1;
               if (!ZCOM.arg2 && ZCOM.arg3 == which)
                  sprintf(buf + strlen(buf), "  [%5ld] %s (Removed from room)\r\n",
                          lastroom_v, world[lastroom_r].name);
               break;
               }
            }
         else
            {
            switch (ZCOM.command)
               {
            case 'M':               /* read a mobile */
               lastroom_v = world[ZCOM.arg3].number;
               lastroom_r = ZCOM.arg3;
               lastmob_r = ZCOM.arg1;
               mob = read_mobile(lastmob_r, REAL);
               char_to_room(mob, 0);
               strcpy(mobname, GET_NAME(mob));
               extract_char(mob);
               break;
            case 'O':               /* read an object */
               lastroom_v = world[ZCOM.arg3].number;
               lastroom_r = ZCOM.arg3;
               obj = read_object(which, REAL);
               extract_obj(obj);
               if (ZCOM.arg1 == which)
                  sprintf(buf + strlen(buf), "  [%5ld] %s (%ld%% Load)\r\n",
                          lastroom_v, world[lastroom_r].name, ZCOM.arg4?(101-ZCOM.arg4):100);
               break;
            case 'P':               /* object to object */
               if (ZCOM.arg1 == which)
                  sprintf(buf + strlen(buf), "  [%5ld] %s (Put in another object [%ld%% Load])\r\n",
                          lastroom_v, world[lastroom_r].name, ZCOM.arg4?(101-ZCOM.arg4):100);
               break;
            case 'E':               /* equip/give object */
               if (ZCOM.arg1 == which)
               {
                     sprintf(buf + strlen(buf), "  [%5ld] %s (Equipped to %s [%ld]pos %ld [%ld%% Load])\r\n",
                             lastroom_v, world[lastroom_r].name, mobname, mob_index[lastmob_r].vnum,
                             ZCOM.arg3,ZCOM.arg4?(101-ZCOM.arg4):100);
                  }
               break;
            case 'G':               /* equip/give object */
               if (ZCOM.arg1 == which)
               {
                     sprintf(buf + strlen(buf), "  [%5ld] %s (Given to %s [%ld][%ld%% Load])\r\n",
                             lastroom_v, world[lastroom_r].name, mobname, mob_index[lastmob_r].vnum,
                             ZCOM.arg3?(101-ZCOM.arg3):100);
               }
               break;
            case 'R':               /* rem obj from room */
               lastroom_v = world[ZCOM.arg1].number;
               lastroom_r = ZCOM.arg1;
               if (ZCOM.arg2 && ZCOM.arg2 == which)
                  sprintf(buf + strlen(buf), "  [%5ld] %s (Removed from room)\r\n",
                          lastroom_v, world[lastroom_r].name);
               break;
               }  /*switch for object*/
            }
         }
      }

   page_string (ch->desc, buf, TRUE,"");
   release_buffer(buf);
   release_buffer(mobname);
   }

ACMD(do_vload)
   {
   mob_rnum which;
   struct char_data *mob;
   struct obj_data *obj;
   char *buf1 = get_buffer(MAX_STRING_LENGTH);
   char *buf2 = get_buffer(MAX_STRING_LENGTH);
   two_arguments(argument, buf1, buf2);
   if ((!*buf1) || (!*buf2) || (!isdigit(*buf2)))
      {
      send_to_char(ch,"Usage: vload { obj | mob } <number>\r\n");
      release_buffer(buf1);
      release_buffer(buf2);
      return;
      }

   /*These lines do nothing but look up the name of the object or mob
     so the output is more user friendly*/
   if (*buf1 == 'm')
      {
      which = real_mobile(atoi(buf2));
      if (which < 0)
         {
         send_to_char(ch,"That mob does not exist.\r\n");
         release_buffer(buf1);
         release_buffer(buf2);
         return;
         }
      mob = read_mobile(which, REAL);
      char_to_room(mob, 0);
      strcpy(buf2, GET_NAME(mob));
      extract_char(mob);
      }
   else if (*buf1 == 'o')
      {
      which = real_object(atoi(buf2));
      if (which < 0)
         {
         send_to_char(ch,"That object does not exist.\r\n");
         release_buffer(buf1);
         release_buffer(buf2);
         return;
         }
      obj = read_object(which, REAL);
      strcpy(buf2, obj->short_description);
      extract_obj(obj);
      }
   else
      {
      send_to_char(ch,"Usage: vload { obj | mob } <number>\r\n");
      release_buffer(buf1);
      release_buffer(buf2);
      return;
      }

   /*end name look up*/

   check_load_status(*buf1, which, ch, buf2);
   release_buffer(buf1);
   release_buffer(buf2);
   }



void adjust_fight_usage(struct char_data *ch)
{
  char *buf = get_buffer(MAX_STRING_LENGTH);
  sprintf(buf, "Usage: adjustfight f1 f2 f3 f4\r\n");
  strcat(buf, "Usage2: adjustfight groups\r\n");
  strcat(buf, "\r\n");
  strcat(buf, "f1 = fight_group_exp_divisor\r\n");
  strcat(buf, "  The lower this value, the more exp people get when grouping.\r\n");
  strcat(buf, "f2 = fight_ldiff_dodge_multiplier\r\n");
  strcat(buf, "  The larger this value, the harder it becomes to land attacks on higher-level targets.\r\n");
  strcat(buf, "f3 = fight_grouped_dodge_multiplier\r\n");
  strcat(buf, "  The larger this value, the easier it becomes to land attacks when grouped.\r\n");
  strcat(buf, "f4 = fight_dodge_random_bound\r\n");
  strcat(buf, "  The lower this number, the better chance (in general) of landing attacks.\r\n");
  strcat(buf, "\r\n");
  strcat(buf,   "Defaults are: 2.70, 4, 0, 190.\r\n");
  char buf2[256];
  sprintf(buf2, "Current vals: %.2f, %d, %d, %d.\r\n",
    fight_group_exp_divisor,
    fight_ldiff_dodge_multiplier,
    fight_grouped_dodge_multiplier,
    fight_dodge_random_bound
  );
  strcat(buf, buf2);
  strcat(buf, "\r\n");
  strcat(buf, "DO NOT EDIT THESE VALUES IF YOU DO NOT KNOW WHAT YOU ARE DOING!\r\n");
  send_to_char(ch, "%s", buf);
  release_buffer(buf);
}

extern int penalize_large_groups;

ACMD(do_adjust_fight)
{
  if (!argument || !argument[0]) {
    adjust_fight_usage(ch);
    return;
  }

  char *buf = get_buffer(MAX_STRING_LENGTH);
  strcpy(buf, argument);
  char *f1 = strtok(buf, " ");

  if (!str_cmp(f1, "groups")) {
    if (penalize_large_groups) {
      penalize_large_groups = 0;
      send_to_char(ch, "Okay.  There is now NO damroll penality for unlimited-size PCs groups.\r\n");
    } else {
      penalize_large_groups = 1;
      send_to_char(ch, "Okay.  The original damroll penalty for large PC groups is back in place.\r\n");
    }
    return;
  }

  char *f2 = strtok(NULL, " ");
  char *f3 = strtok(NULL, " ");
  char *f4 = strtok(NULL, " ");
  if (!f1 || !f2 || !f3 || !f4) {
    adjust_fight_usage(ch);
    release_buffer(buf);
    return;
  }
  float v1 = atof(f1);
  int v2 = atoi(f2);
  int v3 = atoi(f3);
  int v4 = atoi(f4);
  release_buffer(buf);
  if (v1 == 0.0) {
    adjust_fight_usage(ch);
    send_to_char(ch, "\r\nf1 can not be zero (dividing by zero = bad).\r\n");
    return;
  }
  fight_group_exp_divisor = v1;
  fight_ldiff_dodge_multiplier = v2;
  fight_grouped_dodge_multiplier = v3;
  fight_dodge_random_bound = v4;

  mudlogf(CMP, LVL_IMMORT, TRUE, "(GC) FIGHT: %s adjusted constants to %.2f, %d, %d, %d.", GET_NAME(ch), v1, v2, v3, v4);
}

extern int port;

ACMD(do_backup)
{
  if (port != 9000 && port != 24000) {
    send_to_char(ch, "This command can only be used on builder's port.\r\n");
    return;
  }

  if (!GET_EMAIL(ch) || !*GET_EMAIL(ch)) {
    send_to_char(ch, "Please set your e-mail address at the main menu first.\r\n");
    return;
  }

  if (!argument || !*argument) {
    send_to_char(ch, "Usage: backup <zone>\r\n");
    return;
  }
  
  int znum = atoi(argument);
  if (znum <= 0) {
    send_to_char(ch, "That is not a valid zone.\r\n");
    return;
  }

  int rnum;
  for (rnum = 0; rnum <= top_of_zone_table; rnum++) {
    if (zone_table[rnum].number == znum) {
      break;
    }
  }
  if (rnum > top_of_zone_table) {
    send_to_char(ch, "There is no such zone.\r\n");
    return;
  }

  int allowed = 0;
  // check authors
  char *authors = strdup(zone_table[rnum].author);
  int firstTime = 1;
  while (TRUE) {
    char *author = strtok(firstTime ? authors : NULL, "/: ");
    firstTime = 0;
    if (!author) {
      break;
    }
    if (!str_cmp(author, GET_NAME(ch))) {
      allowed = 1;
      break;
    }
  }
  free(authors);
  // check editors
  char *editors = strdup(zone_table[rnum].editor);
  firstTime = 1;
  while (TRUE) {
    char *editor = strtok(firstTime ? editors : NULL, "/: ");
    firstTime = 0;
    if (!editor) {
      break;
    }
    if (!str_cmp(editor, GET_NAME(ch))) {
      allowed = 1;
      break;
    }
  }
  free(editors);
  if (!allowed) {
    send_to_char(ch, "You are not the author or editor of that zone.\r\n");
    return;
  }

  char buf[4096];
  sprintf(buf, "/bin/mkdir -p %s", BACKUP_TMP_DIR);
  system(buf);
  sprintf(buf, "/bin/cp %s/guild/%d.gld %s", BUILDERS_DIR, znum, BACKUP_TMP_DIR);
  system(buf);
  sprintf(buf, "/bin/cp %s/mob/%d.mob %s", BUILDERS_DIR, znum, BACKUP_TMP_DIR);
  system(buf);
  sprintf(buf, "/bin/cp %s/obj/%d.obj %s", BUILDERS_DIR, znum, BACKUP_TMP_DIR);
  system(buf);
  sprintf(buf, "/bin/cp %s/trg/%d.trg %s", BUILDERS_DIR, znum, BACKUP_TMP_DIR);
  system(buf);
  sprintf(buf, "/bin/cp %s/wld/%d.wld %s", BUILDERS_DIR, znum, BACKUP_TMP_DIR);
  system(buf);
  sprintf(buf, "/bin/cp %s/shp/%d.shp %s", BUILDERS_DIR, znum, BACKUP_TMP_DIR);
  system(buf);
  sprintf(buf, "/bin/cp %s/zon/%d.zon %s", BUILDERS_DIR, znum, BACKUP_TMP_DIR);
  system(buf);

  sprintf(buf, "cd %s; /usr/bin/zip zone%d.zip %d.*", BACKUP_TMP_DIR, znum, znum);
  system(buf);

  sprintf(buf, "%s/body.txt", BACKUP_TMP_DIR);
  FILE *fp = fopen(buf, "w");
  if (!fp) {
    sprintf(buf, "/bin/rm -f %s/*", BACKUP_TMP_DIR);
    system(buf);
    send_to_char(ch, "There was a problem e-mailing you the zones.  Please ask a coder for help.\r\n");
    return;
  }

  fprintf(fp, "Zone %d - %s world files are attached.\n", znum, zone_table[rnum].name);
  fprintf(fp, "\n");
  fclose(fp);

  sprintf(buf, "/usr/bin/mutt -s \"Zone %d - %s world files\" -a %s/zone%d.zip %s < %s/body.txt",
	  znum, zone_table[rnum].name, 
	  BACKUP_TMP_DIR, znum,
	  GET_EMAIL(ch),
	  BACKUP_TMP_DIR
	  );
  system(buf);
 
  sprintf(buf, "/bin/rm -f %s/*", BACKUP_TMP_DIR);
  system(buf);

  mudlogf(CMP, GOD_LOG(ch), TRUE, "Upload: %s has e-mailed zone files for %d - %s.", GET_NAME(ch), znum, zone_table[rnum].name);
 
  send_to_char(ch, "Sent files for zone %d - %s to %s.\r\n",
	       znum, zone_table[rnum].name, GET_EMAIL(ch));
}
