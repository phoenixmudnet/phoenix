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
#include "gremort_exam.h"

#define ZCMD zone_table[zone].cmd[cmd_no]

/*   external vars  */
extern long total_bytes_written;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct room_data *world;
extern time_t boot_time;
extern int circle_shutdown, circle_reboot;
extern struct attack_hit_type attack_hit_text[];
extern FILE *player_fl;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern int buf_switches, buf_largecount, buf_overflows;
extern const float class_exp_multipliers[];
extern const float race_exp_multipliers[];
extern char * pc_class_types[];
extern char * pc_race_types[];
extern char * remort_level_types[];
extern const int exp_table[LVL_IMPL + 1];
extern struct player_index_element *player_table;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct index_data **trig_index;
extern zone_rnum top_of_zone_table;
extern int circle_restrict;
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern int top_of_p_table;
extern int top_of_helpt;
extern int top_of_trigt;
extern struct help_index_element *help_table;
extern int top_shop;  /*. shop.c .*/
extern struct shop_data *shop_index;   /*. shop.c .*/
extern long top_idnum;
extern int xap_objs;
/* Anduin - Battle code */
extern struct battle_zone battle;
extern struct player_shop* player_shops;

/* for chars */
extern char *npc_race_types[];
extern int spell_sort_info[];
extern struct spell_info_type *spells;
extern char *class_abbrevs[];
extern char *race_abbrevs[];
extern int total_repair;
extern int total_recharge;

extern char *credits;
extern char *news;
extern char *motd;
extern char *imotd;
extern char *help;
extern char *info;
extern char *background;
extern char *handbook;
extern char *policies;
extern char *teams;
extern char *marriages;
extern struct dg_quest *dg_quests;

extern int boot_high;

extern room_vnum donation_rooms[];
extern room_vnum mortal_start_room;

ACMD(do_look);
ACMD(do_action);
SPECIAL(home_keeper);
void check_autowiz(struct char_data * ch);
void do_start(struct char_data *ch, bool from_scratch);
void gain_exp(struct char_data * ch, long gain);
void appear(struct char_data *ch);
void roll_real_abils(struct char_data *ch);
void show_shops(struct char_data * ch, char *values);
void hcontrol_list_houses(struct char_data *ch);
void reset_zone(zone_rnum zone);
void justify_mob(struct char_data *mob);
void show_gm(struct char_data * ch, char *arg);
void show_race(struct char_data * ch, char *value);
void save_corpses(void);
int  parse_class(char *arg);
int  parse_race(char *arg);
int  real_zone(zone_vnum vnumber);
int  Valid_Name(char *newname);
int  find_first_step(room_rnum src, room_rnum target,long iFlag);
int  min_level(struct char_data *ch,int spellnum);
struct char_data *find_char(int n);
void find_uid_name(char *uid, char *name);
int find_name(char *name);
struct help_index_element *find_help(char *keyword,int times);
int process_output(struct descriptor_data *t);
void Crash_count_items(struct obj_data * obj, long *nitems);
void read_aliases(struct char_data *ch);
void dismount_char(struct char_data *ch);
char *str_str(char *cs, char *ct);
struct char_data *item_owner(struct obj_data *obj);
void show_spellstat(struct char_data *ch, char *value);
void save_char_ascii(struct char_file_u *ch);
void load_char_ascii(struct char_file_u *ch, char *name);
void read_line_ascii(FILE *fp, char *string, int len);
void write_player_index_file(void);
void load_player_index_file(void);
void proc_color(char*, int);
int get_shop_item_count(struct player_shop*);

ACMD(do_echo)
   {
   skip_spaces(&argument);


   if(!IS_NPC(ch)&&PLR_FLAGGED(ch, PLR_NOSHOUT))
      send_to_char(ch,"You can only commune with the gods!\r\n");
   else if (!*argument)
      send_to_char(ch, "Yes.. but what?\r\n");
   else
      {
      char *buf=get_buffer(MAX_INPUT_LENGTH*2);
      if (subcmd == SCMD_EMOTE)
         sprintf(buf, "$n %s", argument);
      else
         strcpy(buf, argument);
      MOBTrigger = FALSE;
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(ch, "%s", OK);
      else
         {
         MOBTrigger = FALSE;
         act(buf, FALSE, ch, 0, 0, TO_CHAR);
         }
      release_buffer(buf);
      }
   }


ACMD(do_linkload)
   {
   struct char_data *victim = 0;
   struct char_file_u tmp_store;
   char *arg=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, arg);


   if (!*arg)
      {
      send_to_char(ch,"Who did you want to link-load?\r\n");
      release_buffer(arg);
      return;
      }
   if (get_player_vis(ch, arg, 0))
      {
      send_to_char(ch,"That player is already connected/loaded!\r\n");
      release_buffer(arg);
      return;
      }
   CREATE(victim, struct char_data, 1);
   clear_char(victim);
   if (load_char(arg, &tmp_store) > -1)
      {
      store_to_char(&tmp_store, victim);
      if (GET_LEVEL(victim) < GET_LEVEL(ch))
         {
         char *buf=get_buffer(SMALL_BUFSIZE);
         Crash_load(victim);
         read_aliases(victim);
         victim->next = character_list;
         character_list = victim;
         victim->desc = NULL;
         char_to_room(victim, IN_ROOM(ch));
         SET_BIT(PLR_FLAGS(victim), PLR_LINKLOADED);
         act("You reach into the pfile and link-load $N.", FALSE, ch, 0, victim,
             TO_CHAR);
         act("$n reaches into the pfile and link-loads $N.", FALSE, ch, 0,
             victim,TO_NOTVICT);
         sprintf(buf, "(GC) %s has linkloaded %s.", GET_NAME(ch),
                 GET_NAME(victim));
         mudlog(buf, BRF, GOD_LOG(ch), TRUE);
         release_buffer(buf);
         }
      else
         {
         send_to_char(ch,"Sorry, you aren't high enough to link-load that char.\r\n");
         free_char(victim);
         }
      }
   else
      {
      send_to_char(ch,"No such player exists in the pfiles!\r\n");
      free(victim);
      }
   release_buffer(arg);
   }




ACMD(do_send)
   {
   struct char_data *vict;
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   char *buf=get_buffer(MAX_INPUT_LENGTH);

   half_chop(argument, arg, buf);

   if (!*arg)
      {
      send_to_char(ch,"Send what to who?\r\n");
      }
   else if (!(vict = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
      {
      send_to_char(ch,"%s",NOPERSON);
      }
   else
      {
      send_to_char(vict,"%s\r\n",buf);
      if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(ch,"Sent.\r\n");
      else
         {
         send_to_char(ch, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));

         }
      }
   release_buffer(buf);
   release_buffer(arg);
   }





/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data * ch, char *rawroomstr)
   {
   room_vnum tmp;
   room_rnum location;
   struct char_data *target_mob;
   struct obj_data *target_obj;
   char *roomstr=get_buffer(MAX_INPUT_LENGTH);


   one_argument(rawroomstr, roomstr);

#ifdef PLAYERS_PORT
   /*
   if (!*roomstr) {
     send_to_char(ch, "Usage: goto <player>\r\n");
     release_buffer(roomstr);
     return NOWHERE;
   }
   if (!(target_mob = get_char_vis(ch, roomstr,FIND_CHAR_WORLD))) {
     send_to_char(ch, "They aren't here.\r\n");
     release_buffer(roomstr);
     return NOWHERE;
   }
   if (IS_NPC(target_mob)) {
     send_to_char(ch, "Try a player.\r\n");
     release_buffer(roomstr);
     return NOWHERE;
   }
   */
#endif


   if (!*roomstr)
      {
      send_to_char(ch,"You must supply a room number or name.\r\n");
      release_buffer(roomstr);
      return (NOWHERE);
      }
   if (isdigit((int)*roomstr) && !strchr(roomstr, '.'))
      {
      tmp = atoi(roomstr);
      if ((location = real_room(tmp)) < 0)
         {
         send_to_char(ch,"No room exists with that number.\r\n");
         release_buffer(roomstr);
         return (NOWHERE);
         }
      }
   else if ((target_mob = get_char_vis(ch, roomstr,FIND_CHAR_WORLD)))
      location = IN_ROOM(target_mob);
   else if ((target_obj = get_obj_vis(ch, roomstr))!=NULL)
      {
      if (IN_ROOM(target_obj) != NOWHERE)
         location = IN_ROOM(target_obj);
      else
         {
         send_to_char(ch,"That object is not available.\r\n");
         release_buffer(roomstr);
         return (NOWHERE);
         }
      }
   else
      {
      send_to_char(ch,"No such creature or object around.\r\n");
      release_buffer(roomstr);
      return (NOWHERE);
      }

   release_buffer(roomstr);

   /* a location has been found -- if you're < GRGOD, check restrictions. */
   if (GET_LEVEL(ch) < LVL_GOD)
      {
      if (ROOM_FLAGGED(location, ROOM_GODROOM))
         {
         send_to_char(ch,"You are not godly enough to use that room!\r\n");
         return (NOWHERE);
         }
      if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
              world[location].people && world[location].people->next_in_room&&
              GET_LEVEL(ch)<LVL_GRGOD)
         {
         send_to_char(ch,"There's a private conversation going on in that room.\r\n");
         return (NOWHERE);
         }
      if (ROOM_FLAGGED(location, ROOM_HOUSE) &&
              !House_can_enter(ch, GET_ROOM_VNUM(location)))
         {
         send_to_char(ch,"That's private property -- no trespassing!\r\n");
         return (NOWHERE);
         }
      }
   return (location);
   }





ACMD(do_at)
   {
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   char *command=get_buffer(MAX_INPUT_LENGTH);
   room_rnum location, original_loc;
   int zone;


   half_chop(argument, buf, command);
   if (!*buf)
      {
      send_to_char(ch,"You must supply a room number or a name.\r\n");
      }
   else if (!*command)
      {
      send_to_char(ch,"What do you want to do there?\r\n");
      }
   else if ((location = find_target_room(ch, buf)) < 0)
      send_to_char(ch,"Could not find that room/name\r\n");
   else if ((zone = GET_ROOM_VNUM(location)/100) >= 0 && zone != 12 && zone != 30 && GET_LEVEL(ch) < WALK_INTO_LEVEL && !is_olc_set(ch, zone)) {
     send_to_char(ch, "You do not have permissions to at there.\r\n");
     release_buffer(buf);
     release_buffer(command);
     return;
   }

      {
      /* a location has been found. */
      if (IN_ROOM(ch) != NOWHERE)
         {
         original_loc = IN_ROOM(ch);
         char_from_room(ch);
         }
      else
         original_loc = 0;

      char_to_room(ch, location);
      command_interpreter(ch, command);


      /* check if the char is still there */
      if (IN_ROOM(ch) == location)
         {
         char_from_room(ch);
         char_to_room(ch, original_loc);
         }
      }
   release_buffer(buf);
   release_buffer(command);
   }



ACMD(do_control_battle)
   {
   int tmp;
   int low, high;
   char  *highlevel,*lowlevel;
   char  *open_close;
   char  *buf2;
   char  *tag;
   struct descriptor_data *d;
   struct obj_data *o;
   struct char_data *victim = 0;
   int i;

   if (IS_NPC(ch))
      {
      send_to_char(ch,"%sNpc's can not open or close the battle field!!%s\r\n",
                   QRED,QNRM);
      return;
      }

   open_close = get_buffer(MAX_INPUT_LENGTH);
   buf2 = get_buffer(SMALL_BUFSIZE);
   half_chop (argument,open_close,buf2);
   low = 0;
   high = 0;

   if (!*open_close)
      {
      send_to_char(ch,"Usage: bcontrol <open/close/lock> <low level> <high level> [tag]\r\n");
      }
   else if (!str_cmp(open_close, "close"))
      {
      if (battle.zone_state == FALSE)
         {
         send_to_char(ch,"The battle field is already closed!!\r\n");
         }
      else
         {
         send_battle("[ BATTLE ] The battle field is now closed.\r\n");
         log("(BATTLE) %s has closed the bfield",GET_NAME(ch));
         battle.zone_state = FALSE; /* wether the zone is open or not */
         battle.low_level = 0;   /* The lowest level that can enter the battle */
         battle.high_level = 0;  /* The highest level that can enter the battle */
         battle.locked = FALSE;
         battle.tagged = FALSE;
         battle.do_tag = FALSE;
         for (d = descriptor_list; d; d = d->next)
            {
            victim = d->character;
            if(!victim)
               continue;

            if (victim->char_specials.in_battle == TRUE)
               {
               if (FIGHTING(victim))
                  stop_fighting(victim);
               /* transfer any BATTLE_ITEMs from the char to the room
               first check items being carried, then items being worn.
               battle_items cannot be put into a container, so don't
               check in containers, and just drop them on the ground */

               /* items being carried */
               for (o = victim->carrying; o != NULL; o = o->next_content)
                  {
                  if (IS_OBJ_STAT(o, ITEM_BATTLE_ITEM))
                     {
                     obj_from_char(o);
                     obj_to_room(o, IN_ROOM(victim));
                     }
                  }

               /* objects worn */
               for (i = 0; i < NUM_WEARS; i++)
                  {
                  if (GET_EQ(victim, i))
                     {
                     if (IS_OBJ_STAT(GET_EQ(victim,i), ITEM_BATTLE_ITEM))
                        {
                        remove_otrigger(GET_EQ(victim,i),victim);
                        obj_to_room(unequip_char(victim, i), IN_ROOM(victim));
                        }
                     }
                  }
               victim->char_specials.in_battle = FALSE;
               TAGGED(victim) = FALSE;
               send_to_char(victim,"%sYou have been booted out of the battle field!%s\r\n", CCRED(victim, C_SPR),CCNRM(victim, C_SPR));
               if( (tmp=real_room(GET_HOME(victim)))<0)
                  tmp=real_room(mortal_start_room);
               if(tmp<0)
                  {
                  send_to_char(ch,"error!");
                  log("ERROR IN battle control.  unknown %ld room",
                      mortal_start_room);
                  tmp=0;
                  }
               char_from_room(victim);
               char_to_room(victim, tmp);
               do_look(victim, "", 0, 0);
               GET_MANA(victim) = GET_MAX_MANA(victim);
               GET_HIT(victim) = GET_MAX_HIT(victim);
               GET_MOVE(victim) = GET_MAX_MOVE(victim);
               }
            }
         send_to_char(ch,"The battle field has been cleared of all players.\r\n");
         }
      }
   else if (!str_cmp(open_close, "lock"))
      {
      if ((battle.zone_state == TRUE) && (battle.locked == TRUE))
         {
         send_to_char(ch,"The battle field is now unlocked.\r\n");
         send_battle("[ BATTLE ] The battle field is now open to new contestants.\r\n");
         battle.locked = FALSE;
         log("(BATTLE) %s has unlocked the bfield.",GET_NAME(ch));
         }
      else if ((battle.zone_state == TRUE) && (battle.locked == FALSE))
         {
         send_to_char(ch,"The battle field is now locked.\r\n");
         send_battle("[ BATTLE ] The battle field is now closed to new contestants.\r\n");
         log("(BATTLE) %s has locked the bfield.",GET_NAME(ch));
         battle.locked = TRUE;
         }
      else if (battle.zone_state == FALSE)
         {
         send_to_char(ch,"The battle field is Closed already!!\r\n");
         }
      }
   else if (!str_cmp(open_close, "open"))
      {
      highlevel = get_buffer(MAX_INPUT_LENGTH);
      lowlevel  = get_buffer(MAX_INPUT_LENGTH);
      tag = two_arguments(buf2,highlevel,lowlevel);
      if (!is_number(highlevel)||!is_number(lowlevel))
         {
         send_to_char(ch,"Usage: bcontrol <open/close/lock> <low level> <high level> [tag]\r\n");
         high = atoi(highlevel);
         }
      else
         {
         high = atoi(highlevel);
         low = atoi(lowlevel);
         if (low == high)
            {
            send_to_char(ch,"There must be at least a one level difference.\r\n");
            }
         else if ((low > LVL_IMPL) || (high < 1))
            {
            send_to_char(ch,"The lowest level possible is 1, highest is 125.\r\n");
            }
         else if ((low < high) || (high > low))
            {
            send_to_char(ch,"Usage: bcontrol <open/close/lock> <low level> <high level>\r\n");
            }
         else if ((battle.zone_state == TRUE))
            {
            send_to_char(ch,"You must close the battlefield before you can open it again.\r\n");
            }
         else
            {
            send_battle("[ BATTLE ] The battle field has been opened for levels %d to %d.\r\n",high,low);
            if (isname("tag", tag))
               {
               battle.do_tag = TRUE;  /* enable tagging */
               send_to_char(ch, "Tagging enabled!\r\n");
               }
            battle.zone_state = TRUE; /* wether the zone is open or not */
            battle.low_level = high;   /* The lowest level that can enter the battle */
            battle.high_level = low;  /* The highest level that can enter the battle */
            battle.locked = FALSE;
            log("(BATTLE) %s has opened the bfield for levels %d through %d.",
                GET_NAME(ch),low,high);
            }
         }
      release_buffer(lowlevel);
      release_buffer(highlevel);
      }
   else
      {
      send_to_char(ch,"Usage: bcontrol <open/close/lock> <low level> <high level>\r\n");
      }
   release_buffer(buf2);
   release_buffer(open_close);
   }

ACMD(do_who_battle)
   {
   struct descriptor_data *d;
   struct char_data *victim = 0;
   char *IT = " (tagged)";

   if (battle.zone_state == FALSE)
      {
      send_to_char(ch,"The battle field is not open at this time.\r\n"
                   "So no one is there of course...\r\n");
      return;
      }


   send_to_char(ch," Those Presently in battle\r\n -------------------------\r\n\r\n");

   for (d = descriptor_list; d; d = d->next)
      {
      victim = d->character;
      if(!victim)
         continue;
      if ((victim->char_specials.in_battle == TRUE) && CAN_SEE(ch,victim))
         {
         if(GET_CLAN(victim) <= 0)
            send_to_char(ch, "[ %3d ] %s %s%s\r\n",GET_LEVEL(victim),
                         GET_NAME(victim), GET_TITLE(victim), TAGGED(victim)?IT:"");
         else
            send_to_char(ch, "[ %3d ] %s %s (%s)%s\r\n" ,GET_LEVEL(victim),
                         GET_NAME(victim), GET_TITLE(victim),
                         GET_CLAN_NAME(victim), TAGGED(victim)?IT:"");
         }
      }

   send_to_char(ch,"  Battle Levels -> %d to %d\r\n",battle.high_level,
                battle.low_level);
   }


/* Bug/Typo/Idea file viewing from snippets --Erika */

   ACMD(do_gen_vfile)
   {
      char *syscom, *buf, *buf1, *buf2, *txtbuf;
      FILE *pfp;
      char *filename;

      switch (subcmd)
      {
      case SCMD_V_BUGS:
         filename = BUG_FILE;
         break;
      case SCMD_V_IDEAS:
         filename = IDEA_FILE;
         break;
      case SCMD_V_TYPOS:
         filename = TYPO_FILE;
         break;
      case SCMD_V_CHANGES:
         filename = CHANGES_FILE;
         break;
      case SCMD_V_SYSLOG:
         filename = SYSLOG_FILE;
         break;
      case SCMD_V_ERRORS:
         filename = ERRORS_FILE;
         break;
      case SCMD_V_MAILLOG:
         filename = MAIL_LOG;
         break;
      case SCMD_V_BIGRENT:
         filename = BIGRENT_LOG;
         break;
      case SCMD_V_BUF:
         filename = BUF_LOG;
         break;
      case SCMD_V_LASTCMD:
         filename = LASTCMD_LOG;
         break;
      case SCMD_V_GODCMD:
         filename = GODCMD_LOG;
         break;
      case SCMD_V_DELETE:
         filename = DELETE_LOG;
         break;
      case SCMD_V_RIP:
         filename = RIP_LOG;
         break;
      case SCMD_V_CORPSE:
         filename = CORPSE_LOG;
         break;
      case SCMD_V_CRASH:
         filename = CRASH_LOG;
         break;
      case SCMD_V_SCRIPTERR:
         filename = SCRIPTERR_LOG;
         break;
      case SCMD_V_HELP:
         filename = HELP_LOG;
         break;
      case SCMD_V_GOLD:
         filename = GOLD_LOG;
         break;
      case SCMD_V_LOCATE_OBJ:
         filename = LOCATE_OBJ_LOG;
         break;
      case SCMD_V_LEVELS:
         filename = LOG_LEVELS;
         break;
      case SCMD_V_NEWPLAYERS:
         filename = LOG_NEWPLAYERS;
         break;
      case SCMD_V_DEATH:
         filename = DT_LOG;
         break;
      case SCMD_V_BAN:
         filename = BAN_LOG;
         break;
      case SCMD_V_OBJSCRAP:
         filename = OBJSCRAP_LOG;
         break;
      case SCMD_V_OLC:
         filename = OLC_LOG;
         break;
      case SCMD_V_USAGE:
         filename = USAGE_LOG;
         break;
      case SCMD_V_RESTARTS:
         filename = RESTART_LOG;
         break;
      case SCMD_V_RENTGONE:
         filename = RENTGONE_LOG;
         break;
      case SCMD_V_BADPWS:
         filename = BADPW_LOG;
         break;
      case SCMD_V_GODFIGHT:
         filename = GODFIGHT_LOG;
         break;
      case SCMD_V_SCRIPTLOG:
         filename = SCRIPT_LOG;
         break;
      case SCMD_V_SHOP:
         filename = SHOP_LOG;
         break;
      default:
         send_to_char(ch, "INVALID COMMAND!!: go_gen_vfile\r\n");
         return;
      }

      buf = get_buffer(MAX_INPUT_LENGTH);
      buf1 = get_buffer(MAX_INPUT_LENGTH);
      buf2 = get_buffer(MAX_INPUT_LENGTH);
      syscom = get_buffer(512);
      txtbuf = get_buffer(32750);

      half_chop(argument, buf, buf1);
      half_chop(buf1, buf2, buf1);

      sprintf(syscom, "/usr/bin/tail -200 %s", filename);
      if ((pfp = popen(syscom, "r")) == NULL)
      {
         send_to_char(ch, "No entries found.\r\n");
         log("%s", syscom);
         perror("tail failed");
         if ((pfp = popen("pwd", "r")) == NULL)
         {
            system("pwd");
         }
         else
         {
            fgets(syscom, 120, pfp);
            log("%s", syscom);
            send_to_char(ch, "%s", syscom);
            pclose(pfp);
         }
      }
      else
      {
         txtbuf[0] = '\0';
         send_to_char(ch, "Contents of file: \r\n");
         while (fgets(syscom, 254, pfp) != NULL)
         {
            strcat(txtbuf, syscom);
            strcat(txtbuf, "\r");
         }
         pclose(pfp);
      }
      if (ch->desc)
         page_string(ch->desc, txtbuf, TRUE, "");
      release_buffer(txtbuf);
      release_buffer(syscom);
      release_buffer(buf2);
      release_buffer(buf1);
      release_buffer(buf);
   }

ACMD(do_tedit)
   {
   int l,i;
   char *field;
   char *buf;

   long SMALL_SIZE_FILE_BUFFER = 8192;
   long MEDIUM_SIZE_FILE_BUFFER = 16384;
   long LARGE_SIZE_FILE_BUFFER = 32768;
   long HUGE_SIZE_FILE_BUFFER = 65536;

   struct editor_struct {
      char *cmd;
      char level;
      char *buffer;
      long size;
      char *filename;
      }
   fields[]=
      {
         /* edit the lvls to your own needs */
         { "credits",    LVL_IMPL,       credits,    SMALL_SIZE_FILE_BUFFER  /*8192*/,    CREDITS_FILE},
         { "news",       LVL_ADMIN,      news,       HUGE_SIZE_FILE_BUFFER   /*65536*/,   NEWS_FILE},
         { "motd",       LVL_ADMIN,      motd,       SMALL_SIZE_FILE_BUFFER  /*8192*/,    MOTD_FILE},
         { "imotd",      LVL_SIMP,       imotd,      SMALL_SIZE_FILE_BUFFER  /*8192*/,    IMOTD_FILE},
         { "help",       LVL_ADMIN,      help,       MEDIUM_SIZE_FILE_BUFFER /*16384*/,   HELP_PAGE_FILE},
         { "info",       LVL_ADMIN,      info,       MEDIUM_SIZE_FILE_BUFFER /*16384*/,   INFO_FILE},
         { "background", LVL_IMPL,       background, SMALL_SIZE_FILE_BUFFER  /*8192*/,    BACKGROUND_FILE},
         { "handbook",   LVL_SIMP,       handbook,   LARGE_SIZE_FILE_BUFFER  /*32768*/,   HANDBOOK_FILE},
         { "teams",      LVL_SIMP,       teams,      MEDIUM_SIZE_FILE_BUFFER /*16384*/,   TEAMS_FILE},
         { "policies",   LVL_SIMP,       policies,   HUGE_SIZE_FILE_BUFFER   /*65536*/,   POLICIES_FILE},
         { "marriages",  LVL_GRGOD,      marriages,  SMALL_SIZE_FILE_BUFFER  /*8192*/,    MARRIAGES_FILE},
         { "\n",         0,              NULL,       0,                                   NULL }
      };

   if (ch->desc == NULL)
      {
      send_to_char(ch,"Get outta here you linkdead head!\r\n");
      return;
      }

   buf=get_buffer(MAX_INPUT_LENGTH);
   field=get_buffer(MAX_INPUT_LENGTH);
   half_chop(argument, field, buf);

   if (!*field)
      {
      send_to_char(ch, "Files available to be edited:\r\n");
      i = 1;
      for (l = 0; *fields[l].cmd != '\n'; l++)
         {
         if (GET_LEVEL(ch) >= fields[l].level)
            {
            send_to_char(ch, "  %-20.20s   Level:%4d   Size: %ld\r\n",
                         fields[l].cmd,fields[l].level,fields[l].size);
            i++;
            }
         }
      if (i == 1)
         send_to_char(ch, "None.\r\n");
      release_buffer(buf);
      release_buffer(field);
      return;
      }
   release_buffer(buf);
   for (l = 0; *(fields[l].cmd) != '\n'; l++)
      if (!strncmp(field, fields[l].cmd, strlen(field)))
         break;

   release_buffer(field);
   if (*fields[l].cmd == '\n')
      {
      send_to_char(ch,"Invalid text editor option.\r\n");
      return;
      }

   if (GET_LEVEL(ch) < fields[l].level)
      {
      send_to_char(ch,"You are not godly enough for that!\r\n");
      return;
      }

   switch (l)
      {
   case 0:
      ch->desc->str = &credits;
      break;
   case 1:
      ch->desc->str = &news;
      break;
   case 2:
      ch->desc->str = &motd;
      break;
   case 3:
      ch->desc->str = &imotd;
      break;
   case 4:
      ch->desc->str = &help;
      break;
   case 5:
      ch->desc->str = &info;
      break;
   case 6:
      ch->desc->str = &background;
      break;
   case 7:
      ch->desc->str = &handbook;
      break;
   case 8:
      ch->desc->str = &teams;
      break;
   case 9:
      ch->desc->str = &policies;
      break;
   case 10:
      ch->desc->str = &marriages;
      break;
   default:
      send_to_char(ch,"Invalid text editor option.\r\n");
      return;
      }

   /* set up editor stats */
   send_to_char(ch,"\x1B[H\x1B[J");
   send_to_char(ch,"Edit file below: (/s saves /h for help)\r\n");
   ch->desc->backstr = NULL;
   if (fields[l].buffer)
      {
      send_to_char(ch,"%s",fields[l].buffer);
      ch->desc->backstr = str_dup(fields[l].buffer);
      }
   ch->desc->max_str = fields[l].size;
   ch->desc->mail_to = 0;
   ch->desc->storage = str_dup(fields[l].filename);
   act("$n begins editing a scroll.", TRUE, ch, 0, 0, TO_ROOM);
   SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
   STATE(ch->desc) = CON_TEXTED;
   }


ACMD(do_goto)
   {
   room_rnum location;
   char *buf;

   if ((location = find_target_room(ch, argument)) < 0)
      return;

   int zone = GET_ROOM_VNUM(location)/100;
   if (zone != 0 && zone != 12 && zone != 30 && GET_LEVEL(ch) < WALK_INTO_LEVEL && !is_olc_set(ch, zone)) {
     send_to_char(ch, "You do not have permissions to goto there.\r\n");
     return;
   }

   buf=get_buffer(SMALL_BUFSIZE);
   if (POOFOUT(ch))
      sprintf(buf, "$n %s", POOFOUT(ch));
   else
      strcpy(buf, "$n disappears in a puff of smoke.");


   act(buf, TRUE, ch, 0, 0, TO_ROOM);
   if (IN_ROOM(ch) != NOWHERE)
      char_from_room(ch);
   char_to_room(ch, location);


   if (POOFIN(ch))
      sprintf(buf, "$n %s", POOFIN(ch));
   else
      strcpy(buf, "$n appears with an ear-splitting bang.");


   act(buf, TRUE, ch, 0, 0, TO_ROOM);
   look_at_room(ch, 0);
   release_buffer(buf);
   }



extern int port;


ACMD(do_trans)
   {
   struct descriptor_data *i;
   struct char_data *victim;
   struct follow_type *f, *f_next;
   char *buf=get_buffer(MAX_INPUT_LENGTH);

   if (port == 4000 && GET_LEVEL(ch) < LVL_SERP) {
     send_to_char(ch, "Huh?!?\r\n");
     release_buffer(buf);
     return;
   }

   one_argument(argument, buf);
   if (!*buf)
      send_to_char(ch,"Whom do you wish to transfer?\r\n");
   else if (str_cmp("all", buf))
      {
      if (!(victim = get_char_vis(ch, buf,FIND_CHAR_WORLD)))
         send_to_char(ch, "%s", NOPERSON);
      else if (victim == ch)
         send_to_char(ch,"That doesn't make much sense, does it?\r\n");
      else
         {
         if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim))
            {
            send_to_char(ch,"Go transfer someone your own size.\r\n");
            release_buffer(buf);
            return;
            }
         act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
         if (IN_ROOM(victim) != NOWHERE)
            char_from_room(victim);
         char_to_room(victim, IN_ROOM(ch));
         if (RIDING(victim) && (IN_ROOM(victim) != IN_ROOM(RIDING(victim))))
            dismount_char(victim);

         mudlogf(BRF,GOD_LOG(ch),TRUE,
                 "(GC) %s transfers %s to %ld.",GET_NAME(ch),GET_NAME(victim),
                 GET_ROOM_VNUM(IN_ROOM(ch)));

         /* component mobs */
         if (IS_NPC(victim) && victim->followers)
            for (f = victim->followers; f; f = f_next)
               {
               f_next = f->next;
               if (MOB2_FLAGGED(f->follower, MOB2_COMPONENT))
                  {
                  char_from_room(f->follower);
                  char_to_room(f->follower, IN_ROOM(victim));
                  }
               }

         act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
         act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
         look_at_room(victim, 0);
         }
      }
   else
      {
      /* Trans All */
      if (GET_LEVEL(ch) < LVL_GRGOD)
         {
         send_to_char(ch,"I think not.\r\n");
         release_buffer(buf);
         return;
         }


      for (i = descriptor_list; i; i = i->next)
         if (STATE(i)==CON_PLAYING && i->character && i->character != ch)
            {
            victim = i->character;
            if (GET_LEVEL(victim) >= GET_LEVEL(ch))
               continue;
            act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
            if (IN_ROOM(victim) != NOWHERE)
               char_from_room(victim);
            char_to_room(victim, IN_ROOM(ch));
            act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
            act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
            mudlogf(BRF,GOD_LOG(ch),TRUE,
                    "(GC) %s transfers everyone to %ld.",GET_NAME(ch),
                    GET_ROOM_VNUM(IN_ROOM(ch)));
            look_at_room(victim, 0);
            }
      send_to_char(ch,"%s", OK);
      }
   release_buffer(buf);
   }






ACMD(do_teleport)
   {
   struct char_data *victim;
   struct follow_type *f, *f_next;
   room_rnum target;
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);

   two_arguments(argument, buf, buf2);

   if (port == 4000 && GET_LEVEL(ch) < LVL_GRGOD) {
     send_to_char(ch, "Huh?!?\r\n");
     release_buffer(buf);
     release_buffer(buf2);
     return;
   }

   if (port == 9000 && GET_LEVEL(ch) < LVL_GRGOD) {
     send_to_char(ch, "Huh?!?\r\n");
     release_buffer(buf);
     release_buffer(buf2);
     return;
   }

   if (!*buf)
      send_to_char(ch,"Whom do you wish to teleport?\r\n");
   else if (!(victim = get_char_vis(ch, buf,FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", NOPERSON);
   else if (victim == ch)
      send_to_char(ch,"Use 'goto' to teleport yourself.\r\n");
   else if ((GET_LEVEL(victim) >= GET_LEVEL(ch)) && !IS_NPC(victim))
      send_to_char(ch,"Maybe you shouldn't do that.\r\n");
   else if (!*buf2)
      send_to_char(ch,"Where do you wish to send this person?\r\n");
   else if ((target = find_target_room(ch, buf2)) >= 0)
      {
      send_to_char(ch, "%s", OK);
      act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      if (IN_ROOM(victim) != NOWHERE)
         char_from_room(victim);
      char_to_room(victim, target);
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
      look_at_room(victim, 0);
      mudlogf(BRF,GOD_LOG(ch),TRUE,
              "(GC) %s teleports %s to %ld",GET_NAME(ch),GET_NAME(victim),
              GET_ROOM_VNUM(target));
      /* component mobs */
      if (IS_NPC(victim) && victim->followers)
         for (f = victim->followers; f; f = f_next)
            {
            f_next = f->next;
            if (MOB2_FLAGGED(f->follower, MOB2_COMPONENT))
               {
               char_from_room(f->follower);
               char_to_room(f->follower, IN_ROOM(victim));
               }
            }
      }
   release_buffer(buf2);
   release_buffer(buf);
   }






ACMD(do_vnum)
   {
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);
   two_arguments(argument, buf, buf2);


   if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "room")
       && !is_abbrev(buf, "obj")))
      {
      send_to_char(ch,"Usage: vnum { obj | mob | room } <name>\r\n");
      }
   else if (is_abbrev(buf, "mob"))
      {
      if (!vnum_mobile(buf2, ch))
         send_to_char(ch,"No mobiles by that name.\r\n");
      }
   else if (is_abbrev(buf, "obj"))
      {
      if (!vnum_object(buf2, ch))
         send_to_char(ch,"No objects by that name.\r\n");
      }
   else if (is_abbrev(buf, "room"))
      {
      if (!vnum_room(buf2, ch))
         send_to_char(ch, "No room name containing that word.\r\n");
      }
   release_buffer(buf2);
   release_buffer(buf);
   }






void do_stat_room(struct char_data * ch)
   {
   struct extra_descr_data *desc;
   struct room_data *rm = &world[IN_ROOM(ch)];
   struct room_affected_type *aff;
   int i, found = 0;
   obj_vnum obj_num;
   struct obj_data *j;
   struct char_data *k;

   if (GET_LEVEL(ch) < STAT_ROOM_LEVEL && !is_olc_set(ch, GET_ROOM_VNUM(IN_ROOM(ch))/100)) {
     send_to_char(ch, "You do not have permissions to stat this room.\r\n");
     return;
   }

   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *buf1=get_buffer(SMALL_BUFSIZE);
   char *buf2=get_buffer(SMALL_BUFSIZE);
   char *buf3=get_buffer(SMALL_BUFSIZE);

   send_to_char(ch, "Room name: %s%s%s\r\n", NCYN, rm->name, NNRM);


   sprinttype(rm->sector_type, sector_types, buf1);
   send_to_char(ch, "Zone: [%s%3ld%s], VNum: [%s%5ld%s], RNum: [%s%5ld%s], Type: %s%s%s\r\n",
                NCYN,zone_table[rm->zone].number,NNRM,
                NCYN, rm->number, NNRM,
                NCYN,IN_ROOM(ch),NNRM,
                NCYN,buf1,NNRM);

   send_to_char(ch,"Light [%s%2d%s]\r\n",NCYN,rm->light,NNRM);

   sprintbit(rm->room_flags, room_bits, buf1);
   sprintbit(rm->room2_flags, room2_bits, buf3);

   send_to_char(ch, "SpecProc: %s%s%s, Flags: %s%s%s%s\r\n",
                NCYN,(rm->func == NULL) ? "None" : "Exists",NNRM,
                NCYN,buf1,!strcmp(buf3, "NOBITS ")?"":buf3,NNRM);

   if (rm->affected)
      {
      send_to_char(ch, "Room Affects:\r\n");
      for (aff = rm->affected; aff; aff = aff->next)
         {
         send_to_char(ch,  "Aff: (%3dhr) %s%-26s%s ",
                      aff->duration + 1, NCYN,
                      ((aff->type>=0)&&(aff->type<MAX_SPELLS))?
                      spells[aff->type].spell_name:"TYPE_UNDEFINED",
                      NNRM);
         if (aff->modifier)
            {
            send_to_char(ch,"%+ld to %s%s", aff->modifier,
                         apply_types[(int) aff->location],
                         aff->bitvector?", ":" ");
            }
         if (aff->bitvector)
            {
            sprintbit(aff->bitvector, room_affect_bits, buf2);
            send_to_char(ch, "sets %s", buf2);
            }
         send_to_char(ch, "\r\n");
         }
      }

   send_to_char(ch,"Description:\r\n%s%s%s",NYEL,
                (rm->description) ? rm->description : "  None.\r\n", NNRM);

   if (rm->ex_description)
      {
      sprintf(buf, "Extra descs:%s", NCYN);
      for (desc = rm->ex_description; desc; desc = desc->next)
         {
         strcat(buf, " ");
         strcat(buf, desc->keyword);
         }
      send_to_char(ch,"%s%s",buf,NNRM);
      }
   sprintf(buf, "Chars present:%s", NYEL);
   for (found = 0, k = rm->people; k; k = k->next_in_room)
      {
      if (!CAN_SEE(ch, k))
         continue;
      sprintf(buf+strlen(buf), "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
              (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
      if (strlen(buf) >= 62)
         {
         if (k->next_in_room)
            send_to_char(ch, "%s,\r\n", buf);
         else
            send_to_char(ch, "%s\r\n", buf);
         *buf = found = 0;
         }
      }


   if (*buf)
      send_to_char(ch, "%s\r\n",buf);
   send_to_char(ch,NNRM);


   if (rm->contents)
      {
      sprintf(buf, "Contents:%s", NWHT);
      for (found = 0, j = rm->contents; j; j = j->next_content)
         {
         if (!CAN_SEE_OBJ(ch, j))
            continue;
         sprintf(buf+strlen(buf), "%s %s",
                 found++ ? "," : "", j->short_description);
         if (strlen(buf) >= 62)
            {
            if (j->next_content)
               send_to_char(ch, "%s,\r\n", buf);
            else
               send_to_char(ch, "%s\r\n", buf);
            *buf = found = 0;
            }
         }


      if (*buf)
         send_to_char(ch, "%s\r\n", buf);
      send_to_char(ch, NNRM);
      }
   for (i = 0; i < NUM_OF_DIRS; i++)
      {
      if (rm->dir_option[i])
         {
         if (rm->dir_option[i]->to_room == NOWHERE)
            sprintf(buf1, " %sNONE%s", NCYN, NNRM);
         else
            sprintf(buf1, "%s%5ld%s", NCYN,
                    GET_ROOM_VNUM(rm->dir_option[i]->to_room), NNRM);
         sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
         send_to_char(ch, "Exit %s%-5s%s:  To: [%s], Key: [%s%5ld%s], Keywrd: %s%s%s, Type: %s%s%s WLL: %s%d%s\r\n ",
                      NCYN, dirs[i], NNRM, buf1,
                      NCYN,rm->dir_option[i]->key, NNRM,
                      NCYN,rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None", NNRM,
                      NCYN,buf2,NNRM,
                      NCYN,GET_LOCK_LEVEL(rm->dir_option[i]),NNRM);

         if (rm->dir_option[i]->general_description)
            send_to_char(ch,"%s%s%s",NYEL,
                         rm->dir_option[i]->general_description,NNRM);
         else
            send_to_char(ch,"%s  No exit description.\r\n%s",NYEL,NNRM);
         }
      }
   /*
    * begin add - Bon 07/25/97
    */
   if (rm->tele != NULL)
      {
      sprintf(buf1, "%s%5ld%s", NCYN, rm->tele->targ,
              NNRM);
      send_to_char(ch, "%sTeleport%s  :  To: [%s], Delay: [%s%3d%s]\r\n ",
                   NYEL, NNRM, buf1,
                   NCYN,rm->tele->time,NNRM);
      sprintbit(rm->tele->bitvector,teleport_bits,buf1);
      send_to_char(ch, "              : %sFlags:%s %s%s%s\r\n",
                   NYEL,NNRM,NCYN,buf1,NNRM);
      if (IS_SET(rm->tele->bitvector, TELE_OBJ) ||
              IS_SET(rm->tele->bitvector, TELE_NOOBJ))
         {
         obj_num = real_object(rm->tele->obj);
         send_to_char(ch, "               : Object : %s\r\n",
                      (obj_num>=0)?
                      obj_proto[obj_num].short_description:"MISSING");
         }
      }

   /*
    * end   add - Bon 07/25/97
    */
   if(IS_SET(rm->room_flags,ROOM_MINE))
      {
      send_to_char(ch,"Mine present:\r\n");
      for(i=0;i<NUM_ORE_SLOTS;i++)
         {
         obj_num = real_object(rm->ore_types[i]);
         if(obj_num>=0)
            {
            send_to_char(ch,"%s %d%% (#%d)\r\n",
                         obj_proto[obj_num].short_description,
                         rm->ore_percent[i],rm->ore_types[i]);
            }
         else
            {
            send_to_char(ch,"NOTHING\r\n");
            }
         }
      }

   /* Check for graffiti. */
   int counter = 0;
   for (i = 0; i < num_graffiti; i++) {
     if (graffiti[i].room_vnum == rm->number) {
       if (!counter) {
     send_to_char(ch, "Graffiti:\r\n");
       }
       counter++;
       sprintf(buf, " %3d - %s\r\n", counter, graffiti[i].text);
       send_to_char(ch, "%s", buf);
     }
   }

   release_buffer(buf3);
   release_buffer(buf2);
   release_buffer(buf1);
   release_buffer(buf);
   /* check the room for a script */
   do_sstat_room(ch);

   }






void do_stat_object(struct char_data * ch, struct obj_data * j)
   {
   int i,  found;
   obj_vnum vnum;
   int disp_all=FALSE;
   struct obj_data *j2;
   struct char_data *jch;
   struct extra_descr_data *desc;
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *buf1=get_buffer(SMALL_BUFSIZE);
   char *buf2=get_buffer(SMALL_BUFSIZE);

   if (GET_LEVEL(ch) < STAT_OBJ_LEVEL && !is_olc_set(ch, GET_OBJ_VNUM(j)/100)) {
     send_to_char(ch, "You do not have permissions to stat this object.\r\n");
     release_buffer(buf);
     release_buffer(buf1);
     release_buffer(buf2);
     return;
   }

   vnum = GET_OBJ_VNUM(j);
   send_to_char(ch, "Name: '%s%s%s', Aliases: %s%s%s\r\n",
                NCYN,
                ((j->short_description) ? j->short_description : "<None>"),
                NNRM,
                NCYN,j->name,NNRM);

   sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
   if (GET_OBJ_RNUM(j) >= 0)
      strcpy(buf2, (obj_index[GET_OBJ_RNUM(j)].func ? "Exists" : "None"));
   else
      strcpy(buf2, "None");
   send_to_char(ch, "VNum: [%s%5ld%s], RNum: [%s%5ld%s], ID: [%s%ld%s], Type: %s%s%s, "
                    "SpecProc: %s%s%s\r\n",
                NCYN,vnum,NNRM,
                NCYN,GET_OBJ_RNUM(j),NNRM,
                NCYN,GET_ID(j),NNRM,
                NCYN,buf1,NNRM,
                NCYN,buf2,NNRM);

   send_to_char(ch, "L-Des: %s%s%s\r\n",
                NCYN,((j->description) ? j->description : "None"),NNRM);

   if(j->action_description)
      {
      if ((item_owner(j) != ch) && (GET_OBJ_TYPE(j) == ITEM_NOTE) &&
               !strcmp(j->name, "mail paper letter"))
         send_to_char(ch, "A-Des: %sYou can't read other people's mail, sorry.%s\r\n",
                      NCYN, NNRM);
      else
         send_to_char(ch, "A-Des: %s%s%s\r\n",
                      NCYN,
                      ((j->action_description) ? j->action_description : "None"),
                      NNRM);
      }

   /* Ponder (04/02/1997) - Echo the material type as well */
   sprinttype(j->material, material_types, buf1);
   send_to_char(ch,
                "Made of       : %s%s%s Quality: %s%d%s(cur)/%s%d%s(tot)/%s%d%s(orig)\r\n",
                NCYN,buf1,NNRM,
                NCYN,GET_OBJ_CSLOTS(j),NNRM,
                NCYN,GET_OBJ_TSLOTS(j),NNRM,
                NCYN,GET_OBJ_OSLOTS(j),NNRM);


   sprintbit(j->obj_flags.wear_flags, wear_bits, buf1);
   send_to_char(ch, "Can be worn on: %s%s%s\r\n",NCYN,buf1,NNRM);


   send_to_char(ch, "Weight: %s%d%s, Value: %s%d%s, Cost/day: %s%d%s, "
                    "Timer: %s%d%s, Amount: %s%d%s\r\n",
                NCYN,GET_OBJ_WEIGHT(j),NNRM,
                NCYN,GET_OBJ_COST(j),NNRM,
                NCYN,GET_OBJ_RENT(j),NNRM,
                NCYN,GET_OBJ_TIMER(j),NNRM,
                NCYN,(vnum>=0)?obj_index[GET_OBJ_RNUM(j)].number:0,NNRM);


   send_to_char(ch, "In room: ");
   if (IN_ROOM(j) == NOWHERE)
      send_to_char(ch, "%sNowhere%s",NCYN,NNRM);
   else
      {
      send_to_char(ch, "%s%ld%s", NCYN,GET_ROOM_VNUM(IN_ROOM(j)),NNRM);
      }
   send_to_char(ch, ", In object: %s%s%s",
                NCYN,(j->in_obj ? j->in_obj->short_description : "None"),NNRM);
   send_to_char(ch, ", Carried by: %s%s%s",
                NCYN, (j->carried_by ? GET_NAME(j->carried_by) : "Nobody"),NNRM);
   send_to_char(ch, ", Worn by: %s%s%s\r\n",
                NCYN, (j->worn_by ? GET_NAME(j->worn_by) : "Nobody"),NNRM);

   sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf1);
   sprintbit(GET_OBJ_EXTRA2(j), extra_bits2, buf2);
   send_to_char(ch, "Extra flags   : %s%s%s%s\r\n",
                NCYN,buf1,!strcmp(buf2, "NOBITS ")?"":buf2,NNRM);

   sprintbit(GET_OBJ_ANTI(j), anti_bits, buf1);
   send_to_char(ch, "Anti flags    : %s%s%s\r\n",NCYN,buf1,NNRM);

   switch (GET_OBJ_TYPE(j))
      {
   case ITEM_LIGHT:
      if (GET_OBJ_VAL(j, 2) == -1)
         sprintf(buf, "Hours left: %sInfinite%s",NCYN,NNRM);
      else
         /* Refuelable light mod--Aleks */
         {
         sprinttype(GET_OBJ_VAL(j, 1), fuels, buf2);
         sprintf(buf, "Refuelable: %s%s%s, Type: %s%s%s, Maximum capacity: [%s%ld%s], Hours left: [%s%ld%s]",
                 NCYN,YESNO(GET_OBJ_VAL(j, 0)),NNRM,
                 NCYN,buf2,NNRM,
                 NCYN,GET_OBJ_VAL(j, 3),NNRM,
                 NCYN,GET_OBJ_VAL(j,2),NNRM);
         }
      break;
   case ITEM_SCROLL:
      /* New case added--Pill modification--Aleks */
   case ITEM_PILL:
   case ITEM_POTION:
      sprintf(buf, "Spells: (Level %s%ld%s) %s%s, %s, %s%s",
              NRED,GET_OBJ_VAL(j, 0), NNRM,
              NCYN,skill_name(GET_OBJ_VAL(j, 1)),
              skill_name(GET_OBJ_VAL(j, 2)),
              skill_name(GET_OBJ_VAL(j, 3)),NNRM);
      break;
   case ITEM_WAND:
   case ITEM_STAFF:
      sprintf(buf, "Spell: %s%s%s at level %s%ld%s, %s%ld%s (of %s%ld%s) charges remaining",
              NCYN,skill_name(GET_OBJ_VAL(j, 3)),NNRM,
              NRED,GET_OBJ_VAL(j, 0), NNRM,
              NCYN,GET_OBJ_VAL(j, 2),NNRM,
              NCYN,GET_OBJ_VAL(j, 1),NNRM);
      break;
   case ITEM_THROW:
   case ITEM_ROCK:
   case ITEM_BOLT:
   case ITEM_ARROW:
      sprintf(buf, "Number dam dice: %s%ld%s Size dam dice: %s%ld%s",
              NCYN,GET_OBJ_VAL(j, 1),NNRM,
              NCYN,GET_OBJ_VAL(j, 2),NNRM);
      break;
   case ITEM_GRENADE:
      sprintf(buf, "Timer: %s%ld%s Num dam dice: %s%ld%s Size dam dice: %s%ld%s",
              NCYN,GET_OBJ_VAL(j, 0),NNRM,
              NCYN,GET_OBJ_VAL(j, 1),NNRM,
              NCYN,GET_OBJ_VAL(j, 2),NNRM);
      break;
   case ITEM_BOW:
   case ITEM_CROSSBOW:
   case ITEM_SLING:
      sprintf(buf, "Range: %s%ld%s", NCYN,GET_OBJ_VAL(j, 1),NNRM);
      break;
   case ITEM_WEAPON:
      sprintf(buf, "Dice: %s%ldd%ld%s, Average: %s%.1f%s, Message type: %s%s%s",
              NCYN,GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),NNRM,
              NCYN,(((GET_OBJ_VAL(j, 2) + 1) / 2.0) * GET_OBJ_VAL(j,1)),NNRM,
              NCYN,attack_hit_text[GET_OBJ_VAL(j,3)].singular,NNRM);
      break;
   case ITEM_ARMOR:
      sprintf(buf, "AC-apply: [%s%ld%s]", NCYN,GET_OBJ_VAL(j, 0),NNRM);
      break;
   case ITEM_TRAP:
      sprintf(buf, "Spell: %s%ld%s, - Hitpoints: %s%ld%s",
              NCYN,GET_OBJ_VAL(j, 0),NNRM,
              NCYN,GET_OBJ_VAL(j, 1),NNRM);
      break;
   case ITEM_CONTAINER:
      sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf2);
      sprintf(buf, "Weight capacity: %s%ld%s, Lock Type: %s%s%s, Key Num: %s%ld%s, Corpse: %s%s%s",
              NCYN,GET_OBJ_VAL(j, 0),NNRM,
              NCYN,buf2,NNRM,
              NCYN,GET_OBJ_VAL(j, 2), NNRM,
              NCYN,YESNO(GET_OBJ_VAL(j, 3)),NNRM);
      break;
   case ITEM_DRINKCON:
   case ITEM_FOUNTAIN:
      sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
      sprintf(buf, "Capacity: %s%ld%s, Contains: %s%ld%s, Poisoned: %s%s%s, Liquid: %s%s%s",
              NCYN,GET_OBJ_VAL(j, 0),NNRM,
              NCYN,GET_OBJ_VAL(j, 1),NNRM,
              NCYN,YESNO(GET_OBJ_VAL(j, 3)),NNRM,
              NCYN,buf2,NNRM);
      break;
   case ITEM_NOTE:
      sprintf(buf, "Tongue: %s%ld%s",NCYN, GET_OBJ_VAL(j, 0),NNRM);
      break;
   case ITEM_KEY:
      strcpy(buf, "");
      break;
   case ITEM_FOOD:
      sprintf(buf, "Makes full: %s%ld%s, Poisoned: %s%s%s",
              NCYN,GET_OBJ_VAL(j, 0), NNRM,
              NCYN,YESNO(GET_OBJ_VAL(j, 3)),NNRM);
      break;
   case ITEM_MONEY:
      sprintf(buf, "Coins: %s%ld%s", NCYN,GET_OBJ_VAL(j, 0),NNRM);
      break;
   case ITEM_FUEL:
      sprinttype(GET_OBJ_VAL(j, 1), fuels, buf2);
      sprintf(buf, "Quantity: %s%ld%s, Type: %s%s%s",
              NCYN,GET_OBJ_VAL(j, 0),NNRM,
              NCYN,buf2,NNRM);
      break;
   case ITEM_FURNITURE:
      sprintf(buf, "Minimum position: %s, Capacity: %ld, Gain Bonus: %ld",
              position_types[(int) GET_OBJ_VAL(j, 0)],
              GET_OBJ_VAL(j, 1),GET_OBJ_VAL(j, 2));
      break;

   default:
      sprintf(buf, "Values 0-7: [%s%ld%s] [%s%ld%s] [%s%ld%s] [%s%ld%s] [%s%ld%s] [%s%ld%s] [%s%ld%s] [%s%ld%s]",
              NCYN,GET_OBJ_VAL(j, 0), NNRM,
              NCYN,GET_OBJ_VAL(j, 1), NNRM,
              NCYN,GET_OBJ_VAL(j, 2), NNRM,
              NCYN,GET_OBJ_VAL(j, 3), NNRM,
              NCYN,GET_OBJ_VAL(j, 4), NNRM,
              NCYN,GET_OBJ_VAL(j, 5), NNRM,
              NCYN,GET_OBJ_VAL(j, 6), NNRM,
              NCYN,GET_OBJ_VAL(j, 7), NNRM);
      disp_all=TRUE;
      break;
      }
   send_to_char(ch, "%s\r\n", buf);
   if ((GET_OBJ_VAL(j,4) > 1) || (GET_OBJ_EXTRA2(j) & 3))
      {
      sprintbit((GET_OBJ_EXTRA2(j) & 3), extra_bits2_id, buf2);
      send_to_char(ch, "Level Restriction: %s%s%ld%s\r\n",
                   NCYN,!strcmp(buf2, "NOBITS ")?"":buf2,
                   GET_OBJ_VAL(j,4),NNRM);
      }
   else
      {
      send_to_char(ch,"Level Restriction: %sNone%s\r\n", NCYN,NNRM);
      }

   if(disp_all==FALSE)
      {
      send_to_char(ch, "Values 0-7: [%s%ld%s] [%s%ld%s] [%s%ld%s] [%s%ld%s] [%s%ld%s] [%s%ld%s] [%s%ld%s] [%s%ld%s]\r\n",
                   NCYN,GET_OBJ_VAL(j, 0), NNRM,
                   NCYN,GET_OBJ_VAL(j, 1), NNRM,
                   NCYN,GET_OBJ_VAL(j, 2), NNRM,
                   NCYN,GET_OBJ_VAL(j, 3), NNRM,
                   NCYN,GET_OBJ_VAL(j, 4), NNRM,
                   NCYN,GET_OBJ_VAL(j, 5), NNRM,
                   NCYN,GET_OBJ_VAL(j, 6), NNRM,
                   NCYN,GET_OBJ_VAL(j, 7), NNRM);
      }



   /*
    * I deleted the "equipment status" code from here because it seemed
    * more or less useless and just takes up valuable screen space.
    */


   if (j->contains)
      {
      sprintf(buf, "\r\nContents:%s", NCYN);
      for (found = 0, j2 = j->contains; j2; j2 = j2->next_content)
         {
         sprintf(buf2, "%s %s", found++ ? "," : "", j2->short_description);
         strcat(buf, buf2);
         if (strlen(buf) >= 62)
            {
            if (j2->next_content)
               send_to_char(ch, "%s,\r\n", buf);
            else
               send_to_char(ch, "%s\r\n", buf);
            *buf = found = 0;
            }
         }


      if (*buf)
         send_to_char(ch, "%s\r\n", buf);
      send_to_char(ch,NNRM);
      }

   /* List the people using this piece of furniture */
   if (j->people)
      {
      sprintf(buf, "\r\nChars using Object:%s", CCCYN(ch, C_NRM));
      for (found = 0, jch = j->people; jch; jch = jch->next_in_furniture)
         {
         if (!CAN_SEE(ch, jch))
            continue;
         sprintf(buf2, "%s %s (%s)", found++ ? "," : "", GET_NAME(jch),
                 (!IS_NPC(jch) ? "PC" : (!IS_MOB(jch) ? "NPC" : "MOB")));
         strcat(buf, buf2);
         if (strlen(buf) >= 62)
            {
            if (jch->next_in_furniture)
               send_to_char(ch, "%s,\r\n",buf);
            else
               send_to_char(ch, "%s\r\n",buf);
            *buf = 0;
            found = 0;
            }
         }
      if (*buf)
         send_to_char(ch, "%s\r\n",buf);

      send_to_char(ch,CCNRM(ch, C_NRM));
      }


   sprintbit(j->obj_flags.bitvector, affected_bits, buf1);
   send_to_char(ch, "Set char bits : %s%s%s\r\n",NCYN,buf1,NNRM);

   found = 0;
   for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (j->affected[i].modifier)
         {
         if(found==0)
            send_to_char(ch,"Affections:");
         if(j->affected[i].location==APPLY_IMMUNE)
            {
            sprintbit(j->affected[i].modifier,immunity_names,buf2);
            send_to_char(ch,"%s Makes Immune to %s%s%s",found++?",":"",
                         NCYN,buf2,NNRM);
            }
         else if(j->affected[i].location==APPLY_RESIST)
            {
            sprintbit(j->affected[i].modifier,immunity_names,buf2);
            send_to_char(ch,"%s Makes Resistant to %s%s%s",found++?",":"",
                         NCYN,buf2,NNRM);
            }
         else if(j->affected[i].location==APPLY_SUSC)
            {
            sprintbit(j->affected[i].modifier,immunity_names,buf2);
            send_to_char(ch,"%s Makes Susceptable to %s%s%s",found++?",":"",
                         NCYN,buf2,NNRM);
            }
         else
            {
            sprinttype(j->affected[i].location, apply_types, buf2);
            send_to_char(ch, "%s %s%+ld%s to %s%s%s", found++ ? "," : "",
                         NCYN,j->affected[i].modifier,NNRM,
                         NCYN,buf2,NNRM);
            }
         }
   if(found!=0)
      send_to_char(ch,"\r\n");


   found=0;
   for(i=0;i<MAX_SPELL_AFFECT;i++)
      if(j->spell_affect[i].spelltype!=0)
         {
         if(found==0)
            send_to_char(ch,"Spells Affects:\r\n");
         send_to_char(ch,"Spl: %s%s%s Lvl: %s%d%s Per: %s%d%%%s\r\n",
                      NCYN,spells[j->spell_affect[i].spelltype].spell_name,NNRM,
                      NRED,j->spell_affect[i].level,NNRM,
                      NCYN,j->spell_affect[i].percentage,NNRM);
         found++;
         }

   if (j->ex_description)
      {
      send_to_char(ch,"Extra descs:%s", NCYN);
      for (desc = j->ex_description; desc; desc = desc->next)
         {
         send_to_char(ch,"\r\nKeyWords: %s\r\nDesc: \r\n%s",desc->keyword,
                      desc->description);
         }
      send_to_char(ch,NNRM);
      }

   release_buffer(buf2);
   release_buffer(buf1);
   release_buffer(buf);
   /* check the object for a script */
   do_sstat_object(ch, j);
   }


ACMD(do_vwear)
   {
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, buf);


   if (!*buf)
      {
      send_to_char(ch,"Usage: vwear <wear position>\r\n"
                   "Possible positions are:\r\n"
                   "finger    neck    body    head    legs    feet    hands\r\n"
                   "shield    arms    about   waist   wrist   wield   hold\r\n"
                   "ear       face    back\r\n");
      }
   else if (is_abbrev(buf, "finger"))
      vwear_object(ITEM_WEAR_FINGER, ch);
   else if (is_abbrev(buf, "neck"))
      vwear_object(ITEM_WEAR_NECK, ch);
   else if (is_abbrev(buf, "body"))
      vwear_object(ITEM_WEAR_BODY, ch);
   else if (is_abbrev(buf, "head"))
      vwear_object(ITEM_WEAR_HEAD, ch);
   else if (is_abbrev(buf, "legs"))
      vwear_object(ITEM_WEAR_LEGS, ch);
   else if (is_abbrev(buf, "feet"))
      vwear_object(ITEM_WEAR_FEET, ch);
   else if (is_abbrev(buf, "hands"))
      vwear_object(ITEM_WEAR_HANDS, ch);
   else if (is_abbrev(buf, "arms"))
      vwear_object(ITEM_WEAR_ARMS, ch);
   else if (is_abbrev(buf, "shield"))
      vwear_object(ITEM_WEAR_SHIELD, ch);
   else if (is_abbrev(buf, "about body"))
      vwear_object(ITEM_WEAR_ABOUT, ch);
   else if (is_abbrev(buf, "waist"))
      vwear_object(ITEM_WEAR_WAIST, ch);
   else if (is_abbrev(buf, "wrist"))
      vwear_object(ITEM_WEAR_WRIST, ch);
   else if (is_abbrev(buf, "wield"))
      vwear_object(ITEM_WEAR_WIELD, ch);
   else if (is_abbrev(buf, "hold"))
      vwear_object(ITEM_WEAR_HOLD, ch);
   else if (is_abbrev(buf, "ear"))
      vwear_object(ITEM_WEAR_EAR, ch);
   else if (is_abbrev(buf, "face"))
      vwear_object(ITEM_WEAR_FACE, ch);
   else if (is_abbrev(buf, "back"))
      vwear_object(ITEM_WEAR_BACK, ch);
   else
      send_to_char(ch,"Possible positions are:\r\n"
                   "finger    neck    body    head    legs    feet    hands\r\n"
                   "shield    arms    about   waist   wrist   wield   hold\r\n"
                   "ear       face    back\r\n");
   release_buffer(buf);
   }


void do_stat_character(struct char_data * ch, struct char_data * k)
   {
   long i, i2;
   int found = 0;
   struct follow_type *fol;
   struct affected_type *aff;
   char *buf1=get_buffer(SMALL_BUFSIZE);
   char *buf2=get_buffer(SMALL_BUFSIZE);
   obj_rnum skin;

   if (GET_LEVEL(ch) < STAT_MOB_LEVEL && IS_NPC(k) && !is_olc_set(ch, GET_MOB_VNUM(k)/100)) {
     send_to_char(ch, "You do not have permissions to stat that mob.\r\n");
     release_buffer(buf1);
     release_buffer(buf2);
     return;
   }


   switch (GET_SEX(k))
      {
   case SEX_NEUTRAL:
      strcpy(buf2, "NEUTRAL-SEX");
      break;
   case SEX_MALE:
      strcpy(buf2, "MALE");
      break;
   case SEX_FEMALE:
      strcpy(buf2, "FEMALE");
      break;
   default:
      strcpy(buf2, "ILLEGAL-SEX!!");
      break;
      }

   if(IN_ROOM(k)<=0)
      IN_ROOM(k)=0;

   if (!IS_NPC(k)) {
   send_to_char(ch,"%s %s '%s%s%s'  IDNum: [%s%5ld%s], In room [%s%5ld%s], E-mail [%s%s%s]\r\n",
                buf2,(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
                NCYN,GET_NAME(k),NNRM,
                NCYN,IS_NPC(k)?GET_ID(k):GET_IDNUM(k),NNRM,
                NCYN,GET_ROOM_VNUM(IN_ROOM(k)),NNRM,
        NCYN,GET_EMAIL(k)[0] ? GET_EMAIL(k) : "NONE", NNRM
        );
   } else {
   send_to_char(ch,"%s %s '%s%s%s'  IDNum: [%s%5ld%s], In room [%s%5ld%s]\r\n",
                buf2,(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
                NCYN,GET_NAME(k),NNRM,
                NCYN,IS_NPC(k)?GET_ID(k):GET_IDNUM(k),NNRM,
                NCYN,GET_ROOM_VNUM(IN_ROOM(k)),NNRM
        );
   }

   if (IS_MOB(k))
      {
      send_to_char(ch,"Alias: %s%s%s, VNum: [%s%5ld%s], RNum: [%s%5ld%s]\r\n",
                   NCYN,k->player.name,  NNRM,
                   NCYN,GET_MOB_VNUM(k), NNRM,
                   NCYN,GET_MOB_RNUM(k), NNRM);
      }

   if (!IS_NPC(k))
      {
      send_to_char(ch, "Title: %s%s%s\r\n",
                   NCYN,
                   (k->player.title ? k->player.title : "<None>"),
                   NNRM);
      }
   else
      {
      send_to_char(ch, "L-Des: %s%s%s",
                   NCYN,
                   (k->player.long_descr ? k->player.long_descr : "<None>\r\n"),
                   NNRM);
      }

   /* 10/27/96, Echo - races added. */
   if (!IS_NPC(k))
      {
      sprinttype(k->player.class, pc_class_types, buf2);
      sprinttype(REMORT_LEVEL(k), remort_level_types, buf1);
      send_to_char(ch, "Race: %s%s%s, Class: %s%s%s, Remort Level: %s%s Remort%s",
                   NCYN,pc_race_types[(int)GET_RACE(k)],NNRM,
                   NCYN,buf2,NNRM,
                   NCYN,buf1,NNRM);
      }
   else
      {
      send_to_char(ch, "Race: %s%s%s, Class: %s%s%s, Amount: %s%d%s",
                   NCYN,npc_race_types[(int)GET_RACE(k)],NNRM,
                   NCYN,npc_class_types[(int)GET_CLASS(k)],NNRM,
                   NCYN,mob_index[GET_MOB_RNUM(k)].number,NNRM);
      }


   send_to_char(ch, "\r\nLev:[%s%2d%s], XP:[%s%7ld%s], Align:[%s%4d%s]",
                NCYN, GET_LEVEL(k), NNRM,
                NCYN, GET_EXP(k), NNRM,
                NCYN, GET_ALIGNMENT(k), NNRM);

   if (IS_NPC(k))
      send_to_char(ch, ", Mood:[%s%4d%s]", NCYN, GET_MOOD(k), NNRM);
   else
      send_to_char(ch, ", Clan:[%s%s%s]",
                   NCYN, (GET_CLAN(k)?GET_CLAN_NAME(k):"NONE"), NNRM);
   send_to_char(ch, "\r\n");


   if (!IS_NPC(k))
      {
      strftime(buf1, 20, "%a %b %d %Y", localtime(&(k->player.time.birth)));
      strftime(buf2, 20, "%a %b %d %Y", localtime(&(k->player.time.logon)));


      send_to_char(ch, "Created: [%s%s%s], Last Logon: [%s%s%s], Played [%s%ldh %ldm%s]\r\n",
                   NCYN,buf1, NNRM,
                   NCYN,buf2, NNRM,
                   NCYN,k->player.time.played / 3600,
                   ((k->player.time.played % 3600) / 60), NNRM);

      send_to_char(ch, "Hometown: [%s%ld%s], Loadroom: [%s%ld%s], (LL[%s%d%s]/LT[%s%d%s])",
                   NCYN,GET_HOME(k), NNRM,
                   NCYN,GET_LOADROOM(k), NNRM,
                   NCYN,GET_LAST_LEARN(k), NNRM,
                   NCYN,GET_LEARN_TIC(k), NNRM);
      /*. Display OLC zone for immorts .*/
      if(GET_LEVEL(k) >= LVL_IMMORT)
         send_to_char(ch, ", OLC[%s%d,%d,%d,%d,%d%s]",
                      NCYN,GET_OLC_ZONE(k,0),
                      GET_OLC_ZONE(k,1),
                      GET_OLC_ZONE(k,2),
                      GET_OLC_ZONE(k,3),
                      GET_OLC_ZONE(k,4), NNRM
                     );
      send_to_char(ch, "\r\n");
      }

   send_to_char(ch, "Str: [%s%d%s]  Int: [%s%d%s]  Wis: [%s%d%s]  Dex: [%s%d%s]  "
                    "Con: [%s%d%s]  Cha: [%s%d%s], Age: [%s%d%s]\r\n",
                NCYN, GET_STR(k), NNRM,
                NCYN, GET_INT(k), NNRM,
                NCYN, GET_WIS(k), NNRM,
                NCYN, GET_DEX(k), NNRM,
                NCYN, GET_CON(k), NNRM,
                NCYN, GET_CHA(k), NNRM,
                NCYN, age(k)->year, NNRM);

   send_to_char(ch, "Hit p.:[%s%d/%d+%d%s]  Mana p.:[%s%d/%d+%d%s] "
                    "Move p.:[%s%d/%d+%d%s]  Ht/Wt:[%s%d/%d%s]\r\n",
                NCYN, GET_HIT(k), GET_MAX_HIT(k), hit_gain(k),
                NNRM, NCYN, GET_MANA(k), GET_MAX_MANA(k),
                mana_gain(k), NNRM, NCYN, GET_MOVE(k),
                GET_MAX_MOVE(k), move_gain(k), NNRM,
                NCYN, GET_HEIGHT(k), GET_WEIGHT(k), NNRM);

   send_to_char(ch, "Coins: [%s%9ld%s], Bank: [%s%9ld%s] (Total: %s%ld%s), SplFail: [%s%d%s]\r\n",
                NCYN,GET_GOLD(k), NNRM,
                NCYN,GET_BANK_GOLD(k), NNRM,
                NRED,GET_GOLD(k) + GET_BANK_GOLD(k), NNRM,
                NCYN,GET_SPELL_FAIL(k),NNRM);

   send_to_char(ch, "AC: [%s%d%+d/10%s], Hitroll: [%s%2d%s], Damroll: [%s%2d%s], Saving throws: [%s%d/%d/%d/%d/%d%s]\r\n",
                NCYN, GET_AC(k),dex_app[stat_index(GET_DEX(k))].defensive, NNRM,
                NCYN, k->points.hitroll, NNRM,
                NCYN, k->points.damroll, NNRM,
                NCYN, GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2),
                GET_SAVE(k, 3), GET_SAVE(k, 4), NNRM);


   sprinttype(GET_POS(k), position_types, buf2);
   send_to_char(ch, "Pos: %s%s%s, Fighting: %s%s%s",
                NCYN, buf2, NNRM,
                NCYN, (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody"),
                NNRM);


   if (IS_NPC(k))
      {
      send_to_char(ch, ", Attack type: %s%s%s",
                   NCYN,
                   attack_hit_text[k->mob_specials.attack_type].singular,
                   NNRM);
      }
   if (k->desc)
      {
      sprinttype(STATE(k->desc), connected_types, buf2);
      send_to_char(ch, ", Connected: %s%s%s",
                   NCYN, buf2, NNRM);
      }

   send_to_char(ch, ", Guarding: %s%s%s",
        NCYN, GET_GUARDING(k) ? GET_NAME(GET_GUARDING(k)) : "Nobody", NNRM);

   send_to_char(ch, "\r\n");


   sprinttype((k->mob_specials.default_pos), position_types, buf2);
   send_to_char(ch,"Default position: %s%s%s",
                NCYN, buf2, NNRM);


   if(k->desc)
      {
      send_to_char(ch, ", Idle Timer (in tics) [%s%d%s]\r\n",
                   NCYN, k->char_specials.timer, NNRM);
      }
   else if(IS_NPC(k))
      {
      send_to_char(ch, ", Wait State [%s%d%s]\r\n",
                   NCYN,GET_MOB_WAIT(k), NNRM);
      }
   else
      {
      send_to_char(ch,"\r\n");
      }

   if (IS_NPC(k))
      {
      sprintbit(MOB_FLAGS(k), action_bits, buf2);
      send_to_char(ch, "NPC flags: %s%s%s\r\n", NCYN, buf2,NNRM);
      sprintbit(MOB2_FLAGS(k), action2_bits, buf2);
      send_to_char(ch, "NPC2 flags: %s%s%s\r\n", NCYN, buf2,NNRM);
      }
   else
      {
      sprintbit(PLR_FLAGS(k), player_bits, buf2);
      send_to_char(ch, "PLR: %s%s%s\r\n", NCYN, buf2, NNRM);
      sprintbit(PRF_FLAGS(k), preference_bits, buf2);
      send_to_char(ch, "PRF: %s%s%s\r\n", NCYN, buf2, NNRM);
      sprintbit(PRF2_FLAGS(k), preference2_bits, buf2);
      send_to_char(ch, "PRF2: %s%s%s\r\n", NCYN, buf2, NNRM);
      }

   if(IMMUNE(k))
      {
      sprintbit(IMMUNE(k),immunity_names,buf1);
      send_to_char(ch, "IMM: %s%s%s\r\n", NCYN, buf1, NNRM);
      }
   if(RESIST(k))
      {
      sprintbit(RESIST(k),immunity_names,buf1);
      send_to_char(ch, "RES: %s%s%s\r\n", NCYN, buf1, NNRM);
      }
   if(SUCCEPT(k))
      {
      sprintbit(SUCCEPT(k),immunity_names,buf1);
      send_to_char(ch, "SUC: %s%s%s\r\n", NCYN, buf1, NNRM);
      }


   if (IS_MOB(k))
      {
      send_to_char(ch,
                   "Mob Spec-Proc: %s%s%s, NPC Bare Hand Dam: %s%dd%d%s\r\n",
                   NCYN,
                   get_mob_spec_name(mob_index[GET_MOB_RNUM(k)].func),
                   NNRM, NCYN,
                   k->mob_specials.damnodice,
                   k->mob_specials.damsizedice,NNRM);
      }
   send_to_char(ch,  "Carried: weight: %s%d%s, items: %s%d%s; ",
                NCYN,IS_CARRYING_W(k),NNRM,
                NCYN,IS_CARRYING_N(k),NNRM);


   i=0;
   Crash_count_items(k->carrying,&i);
   send_to_char(ch,  "Items in: inventory: %s%ld%s, ",
                NCYN,i,NNRM);


   for (i = 0, i2 = 0; i < NUM_WEARS; i++)
      Crash_count_items(GET_EQ(k,i),&i2);
   send_to_char(ch, "eq: %s%ld%s\r\n",NCYN, i2,NNRM);


   if(!IS_NPC(k))
      {
    float fraction = 100*GET_EXPLORED(k) / (float)top_of_world;
      send_to_char(ch,  "Hunger: %s%d%s, Thirst: %s%d%s, Drunk: %s%d%s "
                   "Light: %s%d%s QP: %s%d%s Explored: %s%d%s (%.3f%%)\r\n",
                   NCYN,GET_COND(k, FULL),NNRM,
                   NCYN,GET_COND(k, THIRST),NNRM,
                   NCYN,GET_COND(k, DRUNK),NNRM,
                   NCYN,GET_LIGHT(k),NNRM,
                   /*NCYN,k->player_specials->explored_total,NNRM,*/
           NCYN,GET_QPOINTS(k),NNRM,
           NCYN,GET_EXPLORED(k),NNRM, fraction
           );
      }
   else
      send_to_char(ch, "Light: %s%d%s\r\n",NCYN,GET_LIGHT(k),NNRM);

   send_to_char(ch, "Master is: %s%s%s, Followers are:%s",
                NCYN,
                ((k->master) ? GET_NAME(k->master) : "<none>"),
                NNRM,
                NCYN);


   if(k->followers)
      {
      char *buf=get_buffer(MAX_STRING_LENGTH);
      for (fol = k->followers; fol; fol = fol->next)
         {
         sprintf(buf+strlen(buf), "%s %s", found++ ? "," : "",
                 PERS(fol->follower, ch));
         if (strlen(buf) >= 62)
            {
            if (fol->next)
               send_to_char(ch, "%s,\r\n",buf);
            else
               send_to_char(ch, "%s\r\n",buf);
            *buf = found = 0;
            }
         }

      if (*buf)
         send_to_char(ch,  "%s",buf);
      release_buffer(buf);
      }
   send_to_char(ch, "%s\r\n", NNRM);

   if (FURNITURE(k))
      {
      char *buf=get_buffer(128);
      strcpy(buf,position_types[(int) GET_POS(k)]);
      send_to_char(ch,  "%s on: %s\r\n",
                   CAP(buf),
                   FURNITURE(k)->short_description);
      release_buffer(buf);
      }


   if(IS_NPC(k))
      {
      if(MOB_FLAGGED(k,MOB_MEMORY))
         {
         send_to_char(ch, "Hunted Chars:%s ",NCYN);
         if(MEMORY(k))
            {
            memory_rec *names;
            for(names=MEMORY(k);names;names=names->next)
               {
               send_to_char(ch, "%ld, ",names->id);
               }
            send_to_char(ch, "\r\n%s",NNRM);
            }
         else
            {
            send_to_char(ch, "None\r\n%s",NNRM);
            }
         }
      /*      GET_MOB_VAL(ch,1)=1;
       */    send_to_char(ch, "Values: %s%8ld %8ld %8ld %8ld %8ld\r\n"
                          "        %8ld %8ld %8ld %8ld %8ld%s\r\n",
                          NCYN,
                          GET_MOB_VAL(k,0),
                          GET_MOB_VAL(k,1),
                          GET_MOB_VAL(k,2),
                          GET_MOB_VAL(k,3),
                          GET_MOB_VAL(k,4),
                          GET_MOB_VAL(k,5),
                          GET_MOB_VAL(k,6),
                          GET_MOB_VAL(k,7),
                          GET_MOB_VAL(k,8),
                          GET_MOB_VAL(k,9),
                          NNRM);
      }

   if(IS_NPC(k))
      {
      if(k->mob_specials.skin!=NOTHING)
         {
         skin=real_object(k->mob_specials.skin);
         if(skin!=NOTHING)
            {
            send_to_char(ch, "Skin: %s (#%ld)\r\n",
                         obj_proto[skin].short_description,k->mob_specials.skin);
            }
         else
            {
            send_to_char(ch, "Skin: SOMETHING!!! (#%ld)\r\n",
                         k->mob_specials.skin);
            }
         }
      }
   /* Showing the bitvector */
   sprintbit(AFF_FLAGS(k), affected_bits, buf2);
   send_to_char(ch, "AFF: %s%s%s\r\n", NCYN, buf2, NNRM);
   sprintbit(AFF2_FLAGS(k), affected2_bits, buf2);
   send_to_char(ch, "AFF2: %s%s%s\r\n", NCYN, buf2, NNRM);


   /* Routine to show what spells a char is affected by */
   if (k->affected)
      {
      for (aff = k->affected; aff; aff = aff->next)
         {
         *buf2 = '\0';
         send_to_char(ch,  "SPL: (%3dhr %2dlvl) %s%-26s%s ", aff->duration + 1,
              aff->spell_level,
                      NCYN,
                      ((aff->type>=0)&&(aff->type<MAX_SPELLS))?
                      spells[aff->type].spell_name:"TYPE_UNDEFINED",
                      NNRM);
         if(aff->location==APPLY_IMMUNE)
            {
            sprintbit(aff->modifier,immunity_names,buf2);
            send_to_char(ch, "Immune to %s%s%s", NCYN,buf2,NNRM);
            }
         else if(aff->location==APPLY_RESIST)
            {
            sprintbit(aff->modifier,immunity_names,buf2);
            send_to_char(ch, "Resistant to %s%s%s", NCYN,buf2,NNRM);
            }
         else if(aff->location==APPLY_SUSC)
            {
            sprintbit(aff->modifier,immunity_names,buf2);
            send_to_char(ch, "Susceptable to %s%s%s",NCYN,buf2,NNRM);
            }
         else if (aff->modifier)
            {
            send_to_char(ch,"%+ld to %s%s", aff->modifier,
                         apply_types[(int) aff->location],
                         aff->bitvector?", ":" ");
            }
         if (aff->bitvector)
            {
            sprintbit(aff->bitvector, WHICH_BITS(aff->location), buf2);
            send_to_char(ch, "sets %s", buf2);
            }
         send_to_char(ch, "\r\n");
         }
      }

   /* check mobiles for a script */
   if (IS_NPC(k))
      {
      do_sstat_character(ch, k);
      if (SCRIPT_MEM(k))
         {
         struct script_memory *mem = SCRIPT_MEM(k);
         send_to_char(ch,"Script memory:\r\n  Remember             Command\r\n");
         while (mem)
            {
            struct char_data *mc = find_char(mem->id);
            if (!mc)
               send_to_char(ch,"  ** Corrupted!\r\n");
            else
               {
               if (mem->cmd)
                  send_to_char(ch, "  %-20.20s%s\r\n",GET_NAME(mc),mem->cmd);
               else
                  send_to_char(ch, "  %-20.20s <default>\r\n",GET_NAME(mc));
               }
            mem = mem->next;
            }
         }
      }
   else
      {
      /* this is a PC, display their global variables */
      if (k->script && k->script->global_vars)
         {
         struct trig_var_data *tv;
         char *name=get_buffer(MAX_INPUT_LENGTH);

         send_to_char(ch,"Global Variables:\r\n");

         /* currently, variable context for players is always 0, so it is */
         /* not displayed here. in the future, this might change */
         for (tv = k->script->global_vars; tv; tv = tv->next)
            {
            if (*(tv->value) == UID_CHAR)
               {
               find_uid_name(tv->value, name);
               send_to_char(ch, "    %10s:  [UID]: %s\r\n", tv->name, name);
               }
            else
               send_to_char(ch, "    %10s:  %s\r\n", tv->name, tv->value);
            }
         release_buffer(name);
         }
      }

   release_buffer(buf2);
   release_buffer(buf1);
   }




ACMD(do_stat)
   {
   struct char_data *victim;
   struct obj_data *object;
   struct char_file_u tmp_store;
   int tmp;
   char *buf1=get_buffer(MAX_INPUT_LENGTH);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);


   half_chop(argument, buf1, buf2);


   if (!*buf1)
      {
      send_to_char(ch,"Stats on who or what?\r\n");
      release_buffer(buf2);
      release_buffer(buf1);
      return;
      }
   else if (is_abbrev(buf1, "room"))
      {
      do_stat_room(ch);
      }
   else if (is_abbrev(buf1, "mob"))
      {
      if (!*buf2)
         send_to_char(ch,"Stats on which mobile?\r\n");
      else
         {
         if ((victim = get_char_vis(ch, buf2,FIND_CHAR_WORLD)))
            do_stat_character(ch, victim);
         else
            send_to_char(ch,"No such mobile around.\r\n");
         }
      }
   else if (is_abbrev(buf1, "player"))
      {
      if (!*buf2)
         {
         send_to_char(ch,"Stats on which player?\r\n");
         }
      else
         {
         if ((victim = get_player_vis(ch, buf2, FIND_CHAR_WORLD)))
            do_stat_character(ch, victim);
         else
            send_to_char(ch,"No such player around.\r\n");
         }
      }
   else if (is_abbrev(buf1, "file"))
      {
      if (!*buf2)
         {
         send_to_char(ch,"Stats on which player?\r\n");
         }
      else
         {
         CREATE(victim, struct char_data, 1);
         clear_char(victim);
         if (load_char(buf2, &tmp_store) > -1)
            {
            store_to_char(&tmp_store, victim);
            victim->player.time.logon=tmp_store.last_logon;
            char_to_room(victim,0);
            if (GET_LEVEL(victim) > GET_LEVEL(ch))
               send_to_char(ch,"Sorry, you can't do that.\r\n");
            else
               do_stat_character(ch, victim);
         if(IN_ROOM(victim)!=NOWHERE){
           char_from_room(victim);
         }
            free_char(victim);
            }
         else
            {
            send_to_char(ch,"There is no such player.\r\n");
            free(victim);
            }
         }
      }
   else if (is_abbrev(buf1, "object"))
      {
      if (!*buf2)
         send_to_char(ch,"Stats on which object?\r\n");
      else
         {
         if ((object = get_obj_vis(ch, buf2))!=NULL)
            do_stat_object(ch, object);
         else
            send_to_char(ch,"No such object around.\r\n");
         }
      }
   else
      {
      if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp))!=NULL)
         do_stat_object(ch, object);
      else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying))!=NULL)
         do_stat_object(ch, object);
      else if ((victim = get_char_vis(ch, buf1,FIND_CHAR_ROOM))!=NULL)
         do_stat_character(ch, victim);
      else if ((object = get_obj_in_list_vis(ch, buf1,
                                             world[IN_ROOM(ch)].contents))!=NULL)
         do_stat_object(ch, object);
      else if ((victim = get_char_vis(ch, buf1,FIND_CHAR_WORLD))!=NULL)
         do_stat_character(ch, victim);
      else if ((object = get_obj_vis(ch, buf1))!=NULL)
         do_stat_object(ch, object);
      else
         send_to_char(ch,"Nothing around by that name.\r\n");
      }
   release_buffer(buf2);
   release_buffer(buf1);
   }




ACMD(do_shutdown)
   {
   char *arg=get_buffer(MAX_STRING_LENGTH);

   one_argument(argument, arg);

   if (subcmd != SCMD_SHUTDOWN)
      {
      send_to_char(ch,"If you want to shut something down, say so!\r\n");
      }
   else if (!*arg)
      {
      log("(GC) Shutdown by %s.", GET_NAME(ch));
      send_to_all("Shutting down.\r\n");
      circle_shutdown = 1;
      }
   else if (!str_cmp(arg, "reboot"))
      {
      log("(GC) Reboot by %s.", GET_NAME(ch));
      send_to_all("Rebooting.. come back in a minute or two.\r\n");
      touch(FASTBOOT_FILE);
      circle_shutdown = circle_reboot = 1;
      }
   else if (!str_cmp(arg, "die"))
      {
      log("(GC) Shutdown (die) by %s.", GET_NAME(ch));
      send_to_all("Shutting down for maintenance.\r\n");
      touch(KILLSCRIPT_FILE);
      circle_shutdown = 1;
      }
   else if (!str_cmp(arg, "pause"))
      {
      log("(GC) Shutdown (pause) by %s.", GET_NAME(ch));
      send_to_all("Shutting down for maintenance.\r\n");
      touch(PAUSE_FILE);
      circle_shutdown = 1;
      }
   else if (!str_cmp(arg, "now"))
      {
      log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
      send_to_all("Rebooting.. come back in a minute or two.\r\n");
      circle_shutdown = 1;
      circle_reboot = 2;
      }
   else
      send_to_char(ch,"Unknown shutdown option.\r\n");
   release_buffer(arg);
   }




void stop_snooping(struct char_data * ch)
   {
   if (!ch->desc->snooping)
      send_to_char(ch,"You aren't snooping anyone.\r\n");
   else
      {
      send_to_char(ch,"You stop snooping.\r\n");
      ch->desc->snooping->snoop_by = NULL;
      ch->desc->snooping = NULL;
      }
   }




ACMD(do_snoop)
   {
   struct char_data *victim, *tch;
   char *arg;

   if (!ch->desc)
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);


   if (!*arg)
      stop_snooping(ch);
   else if (!(victim = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
      send_to_char(ch,"No such person around.\r\n");
   else if (!victim->desc)
      send_to_char(ch,"There's no link.. nothing to snoop.\r\n");
   else if (victim == ch)
      stop_snooping(ch);
   else if (victim->desc->snoop_by)
      send_to_char(ch,"Busy already. \r\n");
   else if ((victim->desc->snooping == ch->desc)&&(subcmd==SCMD_REVSNOOP))
      {
      stop_snooping(victim);
      send_to_char(ch,"They stop snooping you.\r\n");
      REMOVE_BIT(PRF_FLAGS(ch),PRF_NOTELL);
      REMOVE_BIT(PRF_FLAGS(ch),PRF_NOWIZ);
      }
   else if (victim->desc->snooping == ch->desc)
      send_to_char(ch,"Don't be stupid.\r\n");
   else
      {
      if (victim->desc->original)
         tch = victim->desc->original;
      else
         tch = victim;


      if (GET_LEVEL(tch) >= GET_LEVEL(ch))
         {
         send_to_char(ch,"You can't.\r\n");
         release_buffer(arg);
         return;
         }
      send_to_char(ch, "%s", OK);


      if(subcmd==SCMD_SNOOP)
         {
         if (ch->desc->snooping)
            ch->desc->snooping->snoop_by = NULL;


         ch->desc->snooping = victim->desc;
         victim->desc->snoop_by = ch->desc;
         if(GET_LEVEL(ch) < LVL_ADMIN)
            {
            mudlogf(BRF,GOD_LOG(ch),TRUE,
                    "(GC) %s begins snooping %s.",GET_NAME(ch),GET_NAME(victim));
            }
         }
      else
         {
         if (victim->desc->snooping)
            victim->desc->snooping->snoop_by = NULL;


         send_to_char(ch,"They start snooping you.\r\n");
         victim->desc->snooping = ch->desc;
         ch->desc->snoop_by = victim->desc;
         SET_BIT(PRF_FLAGS(ch),PRF_NOTELL);
         SET_BIT(PRF_FLAGS(ch),PRF_NOWIZ);
         }
      }
   release_buffer(arg);
   }


ACMD(do_become)
   {
   struct char_data *victim = 0;
   char *arg;

   if (IS_NPC(ch))
      return;


   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);


   if (!*arg)
      {
      send_to_char(ch,"Become who?\r\n");
      release_buffer(arg);
      return;
      }


   if (get_char(arg) && get_char_room(arg, IN_ROOM(ch)))
      {
      victim = get_char_room(arg, IN_ROOM(ch));
      }
   else
      if (get_char(arg) && (!get_char_room(arg, IN_ROOM(ch))))
         {
         victim = get_char(arg);
         }
      else
         if (!get_char(arg))
            {
            send_to_char(ch,"That person is not here.\r\n");
            release_buffer(arg);
            return;
            }


   if (ch == victim)
      {
      send_to_char(ch,"He he he... We are jolly funny today, eh?\r\n");
      release_buffer(arg);
      return;
      }


   if (!ch->desc || ch->desc->snooping || ch->desc->snoop_by)
      {
      release_buffer(arg);
      return;
      }
   if(GET_LEVEL(victim)>GET_LEVEL(ch))
      {
      send_to_char(ch,"You are not godly enough to do that!\r\n");
      release_buffer(arg);
      return;
      }
   if ((victim->desc) || (IS_NPC(victim)))
      {
      send_to_char(ch,"You can't become mobs or active players!\r\n");
      }
   else
      {
      mudlogf(BRF, GOD_LOG(ch), TRUE, "(GC) %s has become %s.", GET_NAME(ch),
              GET_NAME(victim));
      send_to_char(ch, "%s", OK);


      ch->desc->character = victim;
      ch->desc->original = ch;


      victim->desc = ch->desc;
      ch->desc = 0;
      }
   release_buffer(arg);
   }


ACMD(do_switch)
   {
   struct char_data *victim;
   char *arg=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, arg);


   if (ch->desc->original)
      send_to_char(ch,"You're already switched.\r\n");
   else if (!*arg)
      send_to_char(ch,"Switch with who?\r\n");
   else if (!(victim = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
      send_to_char(ch,"No such character.\r\n");
   else if (ch == victim)
      send_to_char(ch,"Hee hee... we are jolly funny today, eh?\r\n");
   else if (victim->desc)
      send_to_char(ch,"You can't do that, the body is already in use!\r\n");
   else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
      send_to_char(ch,"You aren't holy enough to use a mortal's body.\r\n");
   else if (GET_LEVEL(ch) < LVL_GOD && ROOM_FLAGGED(IN_ROOM(victim),
            ROOM_GODROOM))
      send_to_char(ch,"You are not godly enough to use that room!\r\n");
   else
      {
      send_to_char(ch, "%s", OK);
      mudlogf(BRF,GOD_LOG(ch),TRUE,
              "(GC) %s switches into %s.",GET_NAME(ch),GET_NAME(victim));


      ch->desc->character = victim;
      ch->desc->original = ch;


      victim->desc = ch->desc;
      ch->desc = NULL;
      }
   release_buffer(arg);
   }




ACMD(do_return)
   {
   if (ch->desc && ch->desc->original)
      {
      send_to_char(ch,"You return to your original body.\r\n");


      /* JE 2/22/95 */
      /* if someone switched into your original body, disconnect them */
      if (ch->desc->original->desc)
         STATE(ch->desc->original->desc)=CON_DISCONNECT;


      /* XM 1/27/03 */
      /* mudlog if switching out of a mob */
      if (IS_NPC(ch->desc->character))
         mudlogf(BRF,GOD_LOG(ch->desc->original),TRUE,
                 "(GC) %s returns from %s.",
                 GET_NAME(ch->desc->original),
                 GET_NAME(ch->desc->character));
      ch->desc->character = ch->desc->original;
      ch->desc->original = NULL;


      ch->desc->character->desc = ch->desc;
      ch->desc = NULL;
      }
   }






ACMD(do_load)
   {
   struct char_data *mob;
   struct obj_data *obj;
   mob_vnum vnumber;
   mob_rnum r_num;
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);


   two_arguments(argument, buf, buf2);


   if (!*buf || !*buf2 || !isdigit((int)*buf2))
      {
      send_to_char(ch,"Usage: load { obj | mob } <number>\r\n");
      }
   else if ((vnumber = atoi(buf2)) < 0)
      {
      send_to_char(ch,"A NEGATIVE number??\r\n");
      }
   else if (is_abbrev(buf, "mob"))
      {
      if ((r_num = real_mobile(vnumber)) < 0)
         {
         send_to_char(ch,"There is no monster with that number.\r\n");
         }
      else
         {
       if (GET_LEVEL(ch) < LOAD_MOB_LEVEL && !is_olc_set(ch, vnumber/100)) {
         send_to_char(ch, "You do not have permissions to load that mob.\r\n");
         release_buffer(buf);
         release_buffer(buf2);
         return;
       }

         mob = read_mobile(r_num, REAL);
         fprintf(stderr, "Load: %ld\n", IN_ROOM(ch));
         char_to_room(mob, IN_ROOM(ch));
         GET_MOB_VAL(mob,0)=GET_ROOM_VNUM(IN_ROOM(ch));
         mob->orig_room=IN_ROOM(ch);
         act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
             0, 0, TO_ROOM);
         act("$n has created $N!", TRUE, ch, 0, mob, TO_ROOM);
         act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
         load_mtrigger(mob);
         mudlogf(CMP,GOD_LOG(ch),TRUE,
                 "(GC) %s loads %s [%ld] at [%ld].",GET_NAME(ch),GET_NAME(mob),
                 GET_MOB_VNUM(mob),GET_ROOM_VNUM(IN_ROOM(ch)));
         }
      }
   else if (is_abbrev(buf, "obj"))
      {
      if ((r_num = real_object(vnumber)) < 0)
         {
         send_to_char(ch,"There is no object with that number.\r\n");
         }
      else if ((GET_LEVEL(ch) < LVL_IMPL) && (real_zone(vnumber) == 0))
         {
         send_to_char(ch,"You cannot load from MUD Internals.\r\n");
         }
      else
         {
       if (GET_LEVEL(ch) < LOAD_OBJ_LEVEL && !is_olc_set(ch, vnumber/100)) {
         send_to_char(ch, "You do not have permissions to load that obj.\r\n");
         release_buffer(buf);
         release_buffer(buf2);
         return;
       }

         obj = read_object(r_num, REAL);
         obj_to_char(obj, ch);
         act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
         act("$n has created $p!", TRUE, ch, obj, 0, TO_ROOM);
         act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
         load_otrigger(obj);
         mudlogf(CMP,GOD_LOG(ch),TRUE,
                 "(GC) %s loads %s [%ld] at [%ld].",GET_NAME(ch),
                 GET_OBJ_NAME(obj), GET_OBJ_VNUM(obj),
                 GET_ROOM_VNUM(IN_ROOM(ch)));
         }
      }
   else
      send_to_char(ch,"That'll have to be either 'obj' or 'mob'.\r\n");

   release_buffer(buf2);
   release_buffer(buf);
   }




ACMD(do_vstat)
   {
   struct char_data *mob;
   struct obj_data *obj;
   mob_vnum vnumber;
   mob_rnum r_num;
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);


   two_arguments(argument, buf, buf2);


   if (!*buf || !*buf2 || !isdigit((int)*buf2))
      {
      send_to_char(ch,"Usage: vstat { obj | mob } <number>\r\n");
      }
   else if ((vnumber = atoi(buf2)) < 0)
      {
      send_to_char(ch,"A NEGATIVE number??\r\n");
      }
   else if (is_abbrev(buf, "mob"))
      {
      if ((r_num = real_mobile(vnumber)) < 0)
         {
         send_to_char(ch,"There is no monster with that number.\r\n");
         }
      else
         {
       if (GET_LEVEL(ch) < VSTAT_LEVEL && !is_olc_set(ch, vnumber/100)) {
         send_to_char(ch, "You do not have permissions to vstat that mob.\r\n");
         release_buffer(buf);
         release_buffer(buf2);
         return;
       }
         mob = read_mobile(r_num, REAL);
         char_to_room(mob, 0);
         do_stat_character(ch, mob);
         extract_char(mob);
         }
      }
   else if (is_abbrev(buf, "obj"))
      {
      if ((r_num = real_object(vnumber)) < 0)
         {
         send_to_char(ch,"There is no object with that number.\r\n");
         }
      else
         {
       if (GET_LEVEL(ch) < VSTAT_LEVEL && !is_olc_set(ch, vnumber/100)) {
         send_to_char(ch, "You do not have permissions to vstat that obj.\r\n");
         release_buffer(buf);
         release_buffer(buf2);
         return;
       }

         obj = read_object(r_num, REAL);
         do_stat_object(ch, obj);
         extract_obj(obj);
         }
      }
   else
      send_to_char(ch,"That'll have to be either 'obj' or 'mob'.\r\n");

   release_buffer(buf2);
   release_buffer(buf);
   }




ACMD(do_peace)
   {
   struct char_data *vict, *next_v;
   act ("$n decides that everyone should just be friends.",
        FALSE,ch,0,0,TO_ROOM);
   send_to_room(IN_ROOM(ch),"Everything is quite peaceful now.\r\n");
   for(vict=world[IN_ROOM(ch)].people;vict;vict=next_v)
      {
      next_v=vict->next_in_room;
      if(FIGHTING(vict))
         {
         stop_fighting(vict);
         }
      }
   }


/* clean a room of all mobiles and objects */
ACMD(do_purge)
   {
   struct char_data *vict, *next_v;
   struct obj_data *obj, *next_o;
   char *buf=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, buf);


   if (*buf)
      {
      /* argument supplied. destroy single object
       * or char */
      if ((vict = get_char_vis(ch, buf,FIND_CHAR_ROOM)))
         {
         if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict)))
            {
            send_to_char(ch,"Fuuuuuuuuu!\r\n");
            release_buffer(buf);
            return;
            }
         act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);


            mudlogf(BRF, GOD_LOG(ch), TRUE,
                    "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));

         if (!IS_NPC(vict))
            {
            if (vict->desc)
               {
               STATE(vict->desc)=CON_CLOSE;
               vict->desc->character = NULL;
               vict->desc = NULL;
               }
            }
         extract_char(vict);
         }
      else if ((obj = get_obj_in_list_vis(ch, buf,
                                          world[IN_ROOM(ch)].contents))!=NULL)
         {
         act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
         extract_obj(obj);
         save_corpses();
         }
      else
         {
         send_to_char(ch,"Nothing here by that name.\r\n");
         release_buffer(buf);
         return;
         }


      send_to_char(ch, "%s", OK);
      }
   else
      {
      /* no argument. clean out the room */
      act("$n gestures... You are surrounded by scorching flames!",
          FALSE, ch, 0, 0, TO_ROOM);
      send_to_room(IN_ROOM(ch),"The world seems a little cleaner.\r\n");
      for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v)
         {
         next_v = vict->next_in_room;
         if (IS_NPC(vict))
            extract_char(vict);
         }


      for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_o)
         {
         next_o = obj->next_content;
         extract_obj(obj);
         }
      save_corpses();
      }
   release_buffer(buf);
   }






ACMD(do_advance)
   {
   struct char_data *victim;
   char *name  = get_buffer(MAX_INPUT_LENGTH);
   char *level = get_buffer(MAX_INPUT_LENGTH);
   char *buf   = get_buffer(MAX_STRING_LENGTH);
   int newlevel, oldlevel, difference, i;
   bool advanced;

   two_arguments(argument, name, level);


   if (!*name)
      {
      send_to_char(ch,"Advance who?\r\n");
      }
   else if (!(victim = get_char_vis(ch, name,FIND_CHAR_WORLD)))
      {
      send_to_char(ch,"That player is not here.\r\n");
      }
   else if (GET_LEVEL(ch) <= GET_LEVEL(victim))
      {
      send_to_char(ch,"Maybe that's not such a great idea.\r\n");
      }
   else if (IS_NPC(victim))
      {
      send_to_char(ch,"NO!  Not on NPC's.\r\n");
      }
   else if (!*level || (newlevel = atoi(level)) <= 0)
      {
      send_to_char(ch,"That's not a level!\r\n");
      }
   else if (newlevel > LVL_IMPL)
      {
      send_to_char(ch, "%d is the highest possible level.\r\n", LVL_IMPL);
      }
   else if (newlevel > GET_LEVEL(ch))
      {
      send_to_char(ch,"Yeah, right.\r\n");
      }
   else if (newlevel == GET_LEVEL(victim))
      {
      send_to_char(ch,"They are already at that level.\r\n");
      }
   else
      {
      /* 10/27/96, Echo - advance code rewritten to work despite possible
       *   amonotonicities in the exp scale.
       */

      oldlevel = GET_LEVEL(victim);
      advanced = (newlevel > oldlevel ? 1 : 0);
      difference  = (advanced ? newlevel - oldlevel : oldlevel - newlevel);

      if (!advanced)
         {
         /* if the advance is actually a demotion to a lower level,
          * start the victim from the beginning and bring them back
          * up to the lower level (a bit further below).  10/27/96, Echo
          */
         GET_MAX_HIT(victim)=20;
         GET_MAX_MANA(victim)=100;
         GET_MAX_MOVE(victim)=100;
         send_to_char(victim,"You are momentarily enveloped by darkness!\r\n"
                      "You feel somewhat diminished.\r\n");
         }
      else
         {
         act("$n makes some strange gestures.\r\n"
             "A strange feeling comes upon you,\r\n"
             "As colored lights shine down through you from above,\r\n"
             "Penetrating straight through you, and reconstructing\r\n"
             "Your very essence from within, twisting time and space...\r\n"
             "Suddenly all snaps back to the way it was, and\r\n"
             "You feel slightly different.\r\n", FALSE, ch, 0, victim,TO_VICT);
         }


      send_to_char(ch, "%s", OK);


      /* Record the advance in the syslog */
      mudlogf(BRF,GOD_LOG(ch),TRUE,
              "(GC) %s has %s %s to level %d (from %d).",
              GET_NAME(ch), advanced ? "advanced" : "demoted",
              GET_NAME(victim), newlevel, oldlevel);
      mudlogf(BRF,GOD_LOG(ch),FALSE,
              "Levels: %s has been %s to level %d (from %d).",
              GET_NAME(victim), advanced ? "advanced" : "demoted",
              newlevel, oldlevel);

      /* Send a message to the victim */
      if (difference == 1)
         send_to_char(victim, "%s  You have %s a level!\r\n",
                      advanced ? "CONGRATULATIONS!" : "Deepest condolences...",
                      advanced ? "risen" : "fallen");
      else
         send_to_char(victim, "%s  You have %s %d levels!\r\n",
                      advanced ? "CONGRATULATIONS!" : "Deepest condolences...",
                      advanced ? "risen" : "fallen", difference);

      if(advanced&&(oldlevel<LVL_IMMORT)&&(newlevel>=LVL_IMMORT))
         {
         SET_BIT(PRF_FLAGS(victim),PRF_NOHASSLE);
         SET_BIT(PRF_FLAGS(victim),PRF_ROOMFLAGS);
         SET_BIT(PRF_FLAGS(victim),PRF_LOG1);
         SET_BIT(PRF_FLAGS(victim),PRF_LOG2);
         SET_BIT(PRF_FLAGS(victim), PRF_HOLYLIGHT);
         }

      if (!advanced)
         {
         /* if the victim is a baby, start him/her off */
         do_start(victim, FALSE);
         oldlevel = 0;
         }

      /* otherwise, cruise through all of the levels advancing when appropriate */
      for (i = 1; (i <= LVL_IMPL) && (i <= newlevel); i++)
         {
         GET_EXP(ch)=GET_EXP_FOR_LEVEL(GET_RACE(ch),(int)GET_CLASS(ch), i, REMORT_LEVEL(ch));
         GET_LEVEL(victim) = i;
         if(i>oldlevel)
            advance_level(victim, FALSE); /* advance w/o a message */
         }

      check_autowiz(victim);
      save_char(victim, IN_ROOM(victim));
      }
   release_buffer(buf);
   release_buffer(name);
   release_buffer(level);
   }


ACMD(do_restore) {
    struct char_data *vict;
    int i;
    char *buf = get_buffer(MAX_INPUT_LENGTH);


    one_argument(argument, buf);
    if (!*buf)
        send_to_char(ch,"Whom do you wish to restore?\r\n");
    else if (!(vict = get_char_vis(ch, buf,FIND_CHAR_WORLD)))
        send_to_char(ch, "%s", NOPERSON);
    else {
        if (!IS_NPC(vict) && GET_LEVEL(ch) >= LVL_HERO) {
            for (i = 0; i < MAX_SPELLS; i++) {
                if (GET_LEVEL(vict) >= min_level(vict, i)) {
                    if (GET_LEVEL(vict) >= LVL_AVATAR) {
                        GET_SKILL(vict, i) = MAX(GET_SKILL(vict, i), spells[i].is_spell == IS_SPELL ? 10 : 95);
                    } else if (GET_LEVEL(vict) == LVL_ANGEL) {
                        GET_SKILL(vict, i) = MAX(GET_SKILL(vict, i), spells[i].is_spell == IS_SPELL ? 7 : 70);
                    } else if (GET_LEVEL(vict) == LVL_HERO) {
                        GET_SKILL(vict, i) = MAX(GET_SKILL(vict, i), spells[i].is_spell == IS_SPELL ? 5 : 50);
                    }
                }
            }
        }

        GET_HIT(vict) = GET_MAX_HIT(vict);
        GET_MANA(vict) = GET_MAX_MANA(vict);
        GET_MOVE(vict) = GET_MAX_MOVE(vict);


        /*
        if((GET_LEVEL(ch)<LVL_DGOD)&&
        (str_cmp(GET_NAME(vict),GET_NAME(ch))!=0))
        {
        release_buffer(buf);
        return;
        }
        GET_HIT(vict) = GET_MAX_HIT(vict);
        GET_MANA(vict) = GET_MAX_MANA(vict);
        GET_MOVE(vict) = GET_MAX_MOVE(vict);


        if (!IS_NPC(vict) && (GET_LEVEL(ch) >= LVL_GRGOD) &&
        (GET_LEVEL(vict) >= LVL_IMMORT))
        {
        for (i = 1; i <= MAX_SKILLS; i++)
        {
        if(spells[i].is_spell==IS_SPELL)
        {
        if (GET_LEVEL(vict) >= LVL_GRGOD)
        SET_SKILL(vict, i, 10);
        else
        SET_SKILL(vict, i, 7);
        }
        else
        {
        if (GET_LEVEL(vict) >= LVL_GRGOD)
        SET_SKILL(vict, i, 95);
        else
        SET_SKILL(vict, i, 75);
        }
        }
        */
        /*
        if (GET_LEVEL(vict) >= LVL_GRGOD)
        {
        vict->real_abils.str_add = 100;
        vict->real_abils.intel = 25;
        vict->real_abils.wis = 25;
        vict->real_abils.dex = 25;
        vict->real_abils.str = 25;
        vict->real_abils.con = 25;
        vict->real_abils.cha = 25;
        }
        vict->aff_abils = vict->real_abils;*/
        //}
        update_pos(vict);
        send_to_char(ch, "%s", OK);
        act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
        mudlogf(BRF,GOD_LOG(ch),TRUE,
        "(GC) %s has restored %s.",GET_NAME(ch),GET_NAME(vict));
        }
    release_buffer(buf);
}




void perform_immort_vis(struct char_data *ch)
   {
   if (GET_INVIS_LEV(ch) == 0 && !AFF_FLAGGED(ch, AFF_HIDE | AFF_INVISIBLE))
      {
      send_to_char(ch,"You are already fully visible.\r\n");
      return;
      }

   GET_INVIS_LEV(ch) = 0;
   appear(ch);
   send_to_char(ch,"You are now fully visible.\r\n");
   }




void perform_immort_invis(struct char_data *ch, int level)
   {
   struct char_data *tch;
   int old_invis;

   if (IS_NPC(ch))
      return;

   old_invis = GET_INVIS_LEV(ch);
   for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
      {
      if (tch == ch)
         continue;
      if (GET_LEVEL(tch) >= old_invis && GET_LEVEL(tch) < level)
         act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
             tch, TO_VICT);
      GET_INVIS_LEV(ch) = level;
      if (GET_LEVEL(tch) < old_invis && GET_LEVEL(tch) >= level)
         act("You suddenly realize that $n is standing beside you.",FALSE,ch,0,
             tch, TO_VICT);
      GET_INVIS_LEV(ch) = old_invis;
      }

   GET_INVIS_LEV(ch) = level;

   send_to_char(ch, "Your invisibility level is %d.\r\n", level);
   }



ACMD(do_invis)
   {
   int level;
   char *arg;

   if (IS_NPC(ch))
      {
      send_to_char(ch,"You can't do that!\r\n");
      }
   else if (GET_LEVEL(ch)<LVL_IMMORT)
      {
      if(GET_RACE(ch)!=RACE_SPRITE)
         {
         send_to_char(ch,"You can't do that!\r\n");
         }
      else if (FIGHTING(ch))
         {
         send_to_char(ch,"You are too busy fighting to turn invis!\r\n");
         }
      else
         {
         send_to_char(ch,"You turn invisible!\r\n");
         SET_BIT(AFF_FLAGS(ch),AFF_INVISIBLE);
         }
      }
   else
      {
      arg = get_buffer(MAX_INPUT_LENGTH);
      one_argument(argument, arg);
      if (!*arg)
         {
         if (GET_LEVEL(ch) == GET_INVIS_LEV(ch))
            send_to_char(ch,"You are already invisible to your level.\r\n");
         else
            perform_immort_invis(ch, GET_LEVEL(ch));
         }
      else
         {
         level = atoi(arg);
         if (level > GET_LEVEL(ch))
            send_to_char(ch,"You can't go invisible above your own level.\r\n");
         else if (level < 1)
            perform_immort_vis(ch);
         else
            perform_immort_invis(ch, level);
         }
      release_buffer(arg);
      }
   }




ACMD(do_gecho)
   {
   struct descriptor_data *pt;


   skip_spaces(&argument);
   delete_doubledollar(argument);

   if (!*argument)
      send_to_char(ch,"That must be a mistake...\r\n");
   else
      {
      char *buf = get_buffer(SMALL_BUFSIZE);
      sprintf(buf, "%s\r\n", argument);
      for (pt = descriptor_list; pt; pt = pt->next)
         if (STATE(pt)==CON_PLAYING && pt->character && pt->character != ch)
            send_to_char(pt->character,"%s",buf);
      if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(ch, "%s", OK);
      else
         send_to_char(ch,"%s",buf);
      if(GET_LEVEL(ch)<LVL_IMPL)
         mudlogf(CMP,GOD_LOG(ch),TRUE, "(GC) %s gechoed \"%s\".",
                 GET_NAME(ch),argument);

      release_buffer(buf);
      }
   }




ACMD(do_poofset)
   {
   char *buf=get_buffer(512);

   skip_spaces(&argument);

   if(strlen(argument)>2)
      {
      switch (subcmd)
         {
      case SCMD_POOFIN :
         sprintf(POOFIN(ch),"%s",argument);
         send_to_char(ch,"Your poofin is now...\n\r");
         send_to_char(ch, "%s %s\n\r", GET_NAME(ch), POOFIN(ch));
         break;
      case SCMD_POOFOUT:
         sprintf(POOFOUT(ch),"%s",argument);
         send_to_char(ch,"Your poofout is now...\n\r");
         send_to_char(ch, "%s %s\n\r", GET_NAME(ch), POOFOUT(ch));
         break;
      default:
         send_to_char(ch, "Your goto messages are:\n\r");
         send_to_char(ch, "Poofin: %s %s\n\r", GET_NAME(ch), POOFIN(ch));
         send_to_char(ch, "Poofout: %s %s\n\r", GET_NAME(ch), POOFOUT(ch));
         break;
         }
      }
   else
      {
      switch (subcmd)
         {
      case SCMD_POOFIN :
         send_to_char(ch, "Clearing poofin... it used to be:\n\r");
         send_to_char(ch, "%s %s\n\r", GET_NAME(ch), POOFIN(ch));
         strcpy(POOFIN(ch),"\0");
         break;
      case SCMD_POOFOUT :
         send_to_char(ch, "Clearing poofout... it used to be:\n\r");
         send_to_char(ch, "%s %s\n\r", GET_NAME(ch), POOFOUT(ch));
         strcpy(POOFOUT(ch),"\0");
         break;
      default :
         send_to_char(ch, "Your goto messages are:\n\r");
         send_to_char(ch, "Poofin: %s %s\n\r", GET_NAME(ch), POOFIN(ch));
         send_to_char(ch, "Poofout: %s %s\n\r", GET_NAME(ch), POOFOUT(ch));
         break;
         }
      }
   release_buffer(buf);
   }




ACMD(do_dc)
   {
   struct descriptor_data *d;
   struct char_data *vict;
   int num_to_dc;
   char *arg = get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, arg);
   if (!(num_to_dc = atoi(arg)))
      {
      if (!(vict = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
         {
         send_to_char(ch,"Usage: DC <victim name> or <user number> (type USERS for a list)\r\n");
         release_buffer(arg);
         return;
         }
      else
         {
         for (d = descriptor_list; d && d->character != vict; d = d->next)
           ;
         }
      }
   else
      {
      for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next)
         ;
      }

   if (!d)
      {
      send_to_char(ch,"No such connection.\r\n");
      }
   else if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch))
      {
      if (!CAN_SEE(ch, d->character))
         send_to_char(ch,"No such connection.\r\n");
      else
         send_to_char(ch,"Umm.. maybe that's not such a good idea...\r\n");
      }
   else
      {
      /* close_socket(d);  */
      /* We used to just close the socket here using close_socket(), but
       * various people pointed out this could cause a crash if you're
       * closing the person below you on the descriptor list.  Just setting
       * to CON_CLOSE leaves things in a massively inconsistent state so I
       * had to add this new flag to the descriptor.
       *
       * It is a much more logical extension for a CON_DISCONNECT to be used
       * for in-game socket closes and CON_CLOSE for out of game closings.
       * This will retain the stability of the close_me hack while being
       * neater in appearance. -gg 12/1/97
        */
      STATE(d)=CON_DISCONNECT;
      if (num_to_dc)
         {
         send_to_char(ch, "Connection #%d(%s) closed.\r\n",
                 num_to_dc, d->character?GET_NAME(d->character):"NULL");
         mudlogf(BRF,GOD_LOG(ch),TRUE, "(GC) Connection #%d(%s) closed by %s.",
                 num_to_dc, d->character?GET_NAME(d->character):"NULL",GET_NAME(ch));
         }
      else
         {
         send_to_char(ch, "%s's connection closed.\r\n", GET_NAME(d->character));
         mudlogf(BRF,GOD_LOG(ch),TRUE, "(GC) %s's connection closed by %s.",
                                       GET_NAME(d->character), GET_NAME(ch));
         }
      }
   release_buffer(arg);
   }





ACMD(do_wizlock)
   {
   int value;
   char *when;
   char *arg = get_buffer(MAX_INPUT_LENGTH);


   one_argument(argument, arg);
   if (*arg)
      {
      value = atoi(arg);
      if (value < 0 || value > GET_LEVEL(ch))
         {
         send_to_char(ch,"Invalid wizlock value.\r\n");
         release_buffer(arg);
         return;
         }
      circle_restrict = value;
      when = "now";
      }
   else
      when = "currently";


   switch (circle_restrict)
      {
   case 0:
      send_to_char(ch, "The game is %s completely open.\r\n", when);
      mudlogf(BRF,GOD_LOG(ch),TRUE,
              "(GC) %s has un-wiz-locked the game.",GET_NAME(ch));
      break;
   case 1:
      send_to_char(ch, "The game is %s closed to new players.\r\n", when);
      mudlogf(BRF,GOD_LOG(ch),TRUE,
              "(GC) %s has wizlocked the game against new players.",
              GET_NAME(ch));
      break;
   default:
      send_to_char(ch,"Only level %d and above may enter the game %s.\r\n",
                   circle_restrict, when);
      mudlogf(BRF,GOD_LOG(ch),TRUE,
              "(GC) %s has wizlocked the game for level %d and above.",
              GET_NAME(ch),circle_restrict);
      break;
      }
   release_buffer(arg);
   }




ACMD(do_date)
   {
   char *tmstr;
   time_t mytime;
   int d, h, m;

   if (subcmd == SCMD_DATE)
      mytime = time(0);
   else
      mytime = boot_time;


   tmstr = (char *) asctime(localtime(&mytime));
   *(tmstr + strlen(tmstr) - 1) = '\0';


   if (subcmd == SCMD_DATE)
      send_to_char(ch, "Current machine time: %s\r\n", tmstr);
   else
      {
      mytime = time(0) - boot_time;
      d = mytime / 86400;
      h = (mytime / 3600) % 24;
      m = (mytime / 60) % 60;


      send_to_char(ch, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
                   ((d == 1) ? "" : "s"), h, m);
      }
   }






ACMD(do_last)
   {
   struct char_file_u chdata;
   char *arg = get_buffer(MAX_INPUT_LENGTH);


   one_argument(argument, arg);
   if (!*arg)
      {
      send_to_char(ch,"For whom do you wish to search?\r\n");
      }
   else if (load_char(arg, &chdata) < 0)
      {
      send_to_char(ch,"There is no such player.\r\n");
      }
   else if (GET_LEVEL(ch) < LVL_IMMORT)
      {
      if(chdata.level >= LVL_IMMORT)
         {
         send_to_char(ch,"You are not sufficiently godly for that!\r\n");
         }
      else
         {
         send_to_char(ch, "[%2d][%s] %-12s : %-20s\r\n",
                      chdata.level,
                      class_abbrevs[(int)chdata.class],
                      chdata.name, ctime(&chdata.last_logon));
         }
      }
   else
      {
      if((chdata.level>GET_LEVEL(ch))&&(GET_LEVEL(ch)<LVL_ADMIN))
         {
         send_to_char(ch,"You are not sufficiently godly for that!\r\n");
         }
      else
         {
         send_to_char(ch, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
                      chdata.char_specials_saved.idnum, (int) chdata.level,
                      class_abbrevs[(int) chdata.class], chdata.name, chdata.host,
                      ctime(&chdata.last_logon));
         }
      }
   release_buffer(arg);
   }


ACMD(do_flux)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   struct descriptor_data *pt;
   struct char_data *i;

   any_one_arg(argument, arg);
   if (!str_cmp(arg,"plague") && GET_LEVEL(ch) >= LVL_IMPL)
      {
      for (i = character_list; i ; i = i->next)
         {
         if (AFF_FLAGGED(i, AFF_PLAGUE))
            {
            affect_from_char(i, SPELL_PLAGUE);
            }
         }
      mudlogf(NRM, GOD_LOG(ch), TRUE, "(GC) %s cured the mud of the plague.", GET_NAME(ch));
      }
   else
      {
      for (pt = descriptor_list; pt; pt = pt->next)
         {
         if (STATE(pt)==CON_PLAYING && pt->character)
            {
            GET_HIT(pt->character) = GET_MAX_HIT(pt->character);
            GET_MANA(pt->character) = GET_MAX_MANA(pt->character);
            GET_MOVE(pt->character) = GET_MAX_MOVE(pt->character);
            send_to_char(pt->character,"%sA massive wave of healing spreads over the realm.%s\r\n",
                         CCCYN(pt->character, C_SPR),CCNRM(pt->character, C_SPR));
            }
         }
      mudlogf(NRM, GOD_LOG(ch), TRUE, "(GC) %s fluxed.", GET_NAME(ch));
      }
   release_buffer(arg);
   }


ACMD(do_force)
   {
   char *invis = "";
   struct descriptor_data *i, *next_desc;
   struct char_data *vict, *next_force;
   char *arg = get_buffer(MAX_INPUT_LENGTH);
   char *buf1 = get_buffer(MAX_INPUT_LENGTH);
   char *buf2 = get_buffer(MAX_INPUT_LENGTH);
   char *to_force = get_buffer(MAX_INPUT_LENGTH);


   half_chop(argument, arg, to_force);


   sprintf(buf1, "$n has forced you to '%s'.", to_force);
   sprintf(buf2, "You feel compelled to '%s'.", to_force);

   if (subcmd == SCMD_IFORCE)
      invis = "i";

   if (!*arg || !*to_force)
      send_to_char(ch, "Whom do you wish to force do what?\r\n");
   else if ((GET_LEVEL(ch) < LVL_GRGOD) || (str_cmp("all", arg) &&
            str_cmp("room", arg)))
      {
      if (!(vict = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
         send_to_char(ch, "%s", NOPERSON);
      else if ((GET_LEVEL(ch) <= GET_LEVEL(vict)) && !IS_NPC(vict))
         send_to_char(ch,"No, no, no!\r\n");
      else
         {
         send_to_char(ch, "%s", OK);
         if (subcmd == SCMD_FORCE)
            {
            if (GET_INVIS_LEV(ch) > GET_LEVEL(vict))
               act(buf2, FALSE, NULL, NULL, vict, TO_VICT);
            else
               act(buf1, TRUE, ch, NULL, vict, TO_VICT);
            }
     char *b = get_buffer(MAX_INPUT_LENGTH);
     strcpy(b, to_force);
     char *token = strtok(b, " ");
     /* If you're forcing a mob to gremort, log the god command regardless of level. */
         if (GET_LEVEL(ch) < LVL_NO_LOG || starts_with("mgremort", token)) {
       mudlogf(NRM, GOD_LOG(ch), TRUE,"(GC) %s %sforced %s to %s",
                 GET_NAME(ch), invis, GET_NAME(vict), to_force);
     }
     if (starts_with("mgremort", token) && GET_LEVEL(ch) < LVL_ADMIN) {
       send_to_char(ch, "Sorry, forcing mobs to gremort is illegal.\r\n");
     } else {
       command_interpreter(vict, to_force);
     }
     release_buffer(b);
         }
      }
   else if (!str_cmp("room", arg))
      {
      send_to_char(ch, "%s", OK);
      for (vict = world[IN_ROOM(ch)].people; vict; vict = next_force)
         {
         next_force = vict->next_in_room;
         if (!IS_NPC(vict)&&(GET_LEVEL(vict) >= GET_LEVEL(ch)))
            continue;
         if (subcmd == SCMD_FORCE)
            {
            if (GET_INVIS_LEV(ch) > GET_LEVEL(vict))
               act(buf2, FALSE, NULL, NULL, vict, TO_VICT);
            else
               act(buf1, TRUE, ch, NULL, vict, TO_VICT);
            }
         command_interpreter(vict, to_force);
         }
      if (GET_LEVEL(ch) < LVL_NO_LOG)
         mudlogf(NRM, GOD_LOG(ch), TRUE, "(GC) %s %sforced room %ld to %s",
                 GET_NAME(ch), invis, GET_ROOM_VNUM(IN_ROOM(ch)), to_force);
      }
   else /* force all */
      {
      send_to_char(ch, "%s", OK);
      for (i = descriptor_list; i; i = next_desc)
         {
         next_desc = i->next;
         if (STATE(i)!=CON_PLAYING || !(vict = i->character) ||
                 GET_LEVEL(vict) >= GET_LEVEL(ch))
            continue;
         if (subcmd == SCMD_FORCE)
            {
            if (GET_INVIS_LEV(ch) > GET_LEVEL(vict))
               act(buf2, FALSE, NULL, NULL, vict, TO_VICT);
            else
               act(buf1, TRUE, ch, NULL, vict, TO_VICT);
            }
         command_interpreter(vict, to_force);
         }
      if (GET_LEVEL(ch) < LVL_NO_LOG)
         mudlogf(NRM, GOD_LOG(ch), TRUE, "(GC) %s %sforced all to %s",
                 GET_NAME(ch), invis, to_force);

      }
   release_buffer(to_force);
   release_buffer(buf2);
   release_buffer(buf1);
   release_buffer(arg);

   }


ACMD(do_wiznet)
   {
   struct descriptor_data *d;
   struct char_data *vict;
   char emote = FALSE;
   char any = FALSE;
   int level = LVL_IMMORT;
   char *buf1=get_buffer(MAX_RAW_INPUT_LENGTH);
   char *buf2;

   skip_spaces(&argument);
   delete_doubledollar(argument);


   if (!*argument)
      {
      send_to_char(ch,"Usage: wiznet { <text> | #<level> <text> | *<emotetext> | @ | + | - }\r\n");
      release_buffer(buf1);
      return;
      }
   switch (*argument)
      {
   case '*':
      emote = TRUE;
   case '#':
      one_argument(argument + 1, buf1);
      if (is_number(buf1))
         {
         half_chop(argument+1, buf1, argument);
         level = MAX(atoi(buf1), LVL_IMMORT);
         if (level > GET_LEVEL(ch))
            {
            send_to_char(ch,"You can't wizline above your own level.\r\n");
            release_buffer(buf1);
            return;
            }
         }
      else if (emote)
         argument++;
      break;
   case '@':
      for (d = descriptor_list; d; d = d->next)
         {
         if (STATE(d)==CON_PLAYING &&
                 GET_LEVEL(d->character) >= LVL_IMMORT &&
                 !PRF_FLAGGED(d->character, PRF_NOWIZ) &&
                 (CAN_SEE(ch, d->character) || GET_LEVEL(ch) == LVL_IMPL))
            {
            if (!any)
               {
               send_to_char(ch, "Gods online:\r\n");
               any = TRUE;
               }
            send_to_char(ch, "  %s", GET_NAME(d->character));
            if (PLR_FLAGGED(d->character, PLR_WRITING))
               send_to_char(ch, " (Writing)\r\n");
            else if (PLR_FLAGGED(d->character, PLR_MAILING))
               send_to_char(ch, " (Writing mail)\r\n");
            else
               send_to_char(ch, "\r\n");
            }
         }
      any = FALSE;
      for (d = descriptor_list; d; d = d->next)
         {
         if (STATE(d)==CON_PLAYING &&
                 GET_LEVEL(d->character) >= LVL_IMMORT &&
                 PRF_FLAGGED(d->character, PRF_NOWIZ) &&
                 CAN_SEE(ch, d->character))
            {
            if (!any)
               {
               send_to_char(ch, "Gods offline:\r\n");
               any = TRUE;
               }
            send_to_char(ch, "  %s\r\n",GET_NAME(d->character));
            }
         }
      release_buffer(buf1);
      return;
      break;
   case '-':
      if (PRF_FLAGGED(ch, PRF_NOWIZ))
         send_to_char(ch,"You are already offline!\n\r");
      else
         {
         send_to_char(ch,"You will no longer hear the wizline.\n\r");
         SET_BIT(PRF_FLAGS(ch), PRF_NOWIZ);
         }
      release_buffer(buf1);
      return;
      break;
   case '+':
      if (!PRF_FLAGGED(ch, PRF_NOWIZ))
         send_to_char(ch,"You are already online!\n\r");
      else
         {
         send_to_char(ch,"You can now hear the wizline again.\n\r");
         REMOVE_BIT(PRF_FLAGS(ch), PRF_NOWIZ);
         }
      release_buffer(buf1);
      return;
      break;
   case '\\':
      ++argument;
      break;
   default:
      break;
      }
   if (PRF_FLAGGED(ch, PRF_NOWIZ))
      {
      send_to_char(ch,"You are offline!\r\n");
      release_buffer(buf1);
      return;
      }
   skip_spaces(&argument);


   if (!*argument)
      {
      send_to_char(ch,"Don't bother the gods like that!\r\n");
      release_buffer(buf1);
      return;
      }
   buf2=get_buffer(MAX_RAW_INPUT_LENGTH);
   if (level > LVL_IMMORT)
      {
      sprintf(buf1, "[W] %s: <%d> %s%s\r\n", GET_NAME(ch), level,
              emote ? "<--- " : "", argument);
      sprintf(buf2, "[W] Someone: <%d> %s%s\r\n", level,
              emote ? "<--- " : "", argument);
      }
   else
      {
      sprintf(buf1, "[W] %s: %s%s\r\n", GET_NAME(ch),
              emote ? "<--- " : "", argument);
      sprintf(buf2, "[W] Someone: %s%s\r\n", emote ? "<--- " : "",
              argument);
      }


   for (d = descriptor_list; d; d = d->next)
      {
      if(d->original)
         vict = d->original;
      else
         vict = d->character;
      if ((STATE(d)==CON_PLAYING) && (GET_LEVEL(vict) >= level) &&
              (!PRF_FLAGGED(vict, PRF_NOWIZ)) &&
              (!PLR_FLAGGED(vict, PLR_WRITING | PLR_MAILING))
              && (d != ch->desc || !(PRF_FLAGGED(vict, PRF_NOREPEAT))))
         {
         send_to_char(vict,CCCYN(vict, C_NRM));
         if (CAN_SEE(vict, ch))
            send_to_char(vict,"%s",buf1);
         else
            send_to_char(vict,"%s",buf2);
         send_to_char(vict,CCNRM(vict, C_NRM));
         }
      }

   release_buffer(buf2);
   release_buffer(buf1);
   if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", OK);
   }


ACMD(do_godlog)
   {
   skip_spaces(&argument);

   if(!*argument)
      send_to_char(ch,"Yes, but what do you wish to log?\r\n");
   else
      {
      log("(GC) %s logs: %s",GET_NAME(ch),argument);
      if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(ch, "%s", OK);
      else
         {
         send_to_char(ch, "You log, '%s%s%s'\r\n",NCYN,argument,NNRM);
         }
      }
   }



ACMD(do_zreset)
   {
   zone_rnum i;
   zone_vnum j;
   char *arg=get_buffer(MAX_INPUT_LENGTH);


   one_argument(argument, arg);
   if (!*arg)
      {
      send_to_char(ch,"You must specify a zone.\r\n");
      release_buffer(arg);
      return;
      }
   if (*arg == '*')
      {
      for (i = 0; i <= top_of_zone_table; i++)
         reset_zone(i);
      send_to_char(ch,"Reset world.\r\n");
      mudlogf(NRM, GOD_LOG(ch), TRUE, "(GC) %s reset entire world.",
              GET_NAME(ch));
      release_buffer(arg);
      return;
      }
   else if (*arg == '.')
      i = world[IN_ROOM(ch)].zone;
   else
      {
      j = atoi(arg);
      for (i = 0; i <= top_of_zone_table; i++)
         if (zone_table[i].number == j)
            break;
      }
   if (i >= 0 && i <= top_of_zone_table)
      {
      reset_zone(i);
      send_to_char(ch, "Reset zone %ld (#%ld): %s.\r\n", i,
                   zone_table[i].number, zone_table[i].name);
      mudlogf(NRM, GOD_LOG(ch), TRUE, "(GC) %s reset zone %ld (%s|#%ld)",
              GET_NAME(ch), i, zone_table[i].name,zone_table[i].number);
      }
   else
      send_to_char(ch,"Invalid zone number.\r\n");
   release_buffer(arg);
   }


ACMD(do_idle)
   {
   zone_rnum i;
   zone_vnum j;
   char *arg=get_buffer(MAX_INPUT_LENGTH);


   one_argument(argument, arg);
   if (!*arg)
      {
      send_to_char(ch,"You must specify a zone.\r\n");
      release_buffer(arg);
      return;
      }
   if (*arg == '*')
      {
      for (i = 0; i <= top_of_zone_table; i++)
         if(zone_table[i].reset_mode!=0)
            zone_table[i].age=zone_table[i].lifespan;

      send_to_char(ch,"You idled the world.\r\n");
      mudlogf(NRM, GOD_LOG(ch), TRUE,
              "(GC) %s idled out the entire world.", GET_NAME(ch));
      release_buffer(arg);
      return;
      }
   else if (*arg == '.')
      i = world[IN_ROOM(ch)].zone;
   else
      {
      j = atoi(arg);
      for (i = 0; i <= top_of_zone_table; i++)
         if (zone_table[i].number == j)
            break;
      }
   if (i >= 0 && i <= top_of_zone_table)
      {
      if(zone_table[i].reset_mode!=0)
         {
         zone_table[i].age=zone_table[i].lifespan;
         send_to_char(ch, "You idled zone %ld (#%ld): %s.\r\n", i,
                      zone_table[i].number,
                      zone_table[i].name);
         mudlogf(NRM, GOD_LOG(ch), TRUE, "(GC) %s idled zone %ld (%s)",
                 GET_NAME(ch), i, zone_table[i].name);
         }
      else
         {
         send_to_char(ch,
                      "You cannot idle zone %ld, is has reset_mode of 0.\r\n",
                      zone_table[i].number);
         }
      }
   else
      send_to_char(ch,"Invalid zone number.\r\n");
   release_buffer(arg);
   }

/*
*  General fn for wizcommands of the sort: cmd <player>
*/


ACMD(do_wizutil)
   {
   struct char_data *vict;
   long result;
   char *arg=get_buffer(MAX_INPUT_LENGTH);
   char *buf=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, arg);


   if (!*arg)
      send_to_char(ch,"Yes, but for whom?!?\r\n");
   else if (!(vict = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
      send_to_char(ch,"There is no such player.\r\n");
   else if (IS_NPC(vict))
      send_to_char(ch,"You can't do that to a mob!\r\n");
   else if (GET_LEVEL(vict) > GET_LEVEL(ch))
      send_to_char(ch,"Hmmm...you'd better not.\r\n");
   else
      {
      switch (subcmd)
         {
      case SCMD_REROLL:
         send_to_char(ch,"Rerolled...\r\n");
         roll_real_abils(vict);
         log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
         send_to_char(ch, "New stats: Str %d/%d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
                      GET_STR(vict), GET_ADD(vict), GET_INT(vict),
                      GET_WIS(vict), GET_DEX(vict), GET_CON(vict),
                      GET_CHA(vict));
         break;
      case SCMD_PARDON:
         if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER))
            {
            send_to_char(ch,"Your victim is not flagged.\r\n");
            release_buffer(buf);
            release_buffer(arg);
            return;
            }
         REMOVE_BIT(PLR_FLAGS(vict), PLR_THIEF | PLR_KILLER);
         send_to_char(ch,"Pardoned.\r\n");
         send_to_char(vict,"You have been pardoned by the Gods!\r\n");
         mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
         if (GET_LEVEL(vict) < LVL_IMPL)
            send_info("[ INFO ] %s has been pardoned and is no longer a Player Killer.\n\r",
                      (GET_INVIS_LEV(vict)>=LVL_IMMORT) ? "An immortal" : GET_NAME(vict));
         break;
      case SCMD_NOTITLE:
         result = PLR_TOG_CHK(vict, PLR_NOTITLE);
         mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "(GC) Notitle %s for %s by %s.", ONOFF(result),
                 GET_NAME(vict), GET_NAME(ch));
         send_to_char(ch, "No title set %s for %s.", ONOFF(result),
                      GET_NAME(vict));

         if(!result)
            {
            sprintf(buf, "needs to set %s title correctly this time!",
                    HSHR(vict));
            set_title(vict,buf);
            }
         else
            {
            sprintf(buf,"can no longer set %s own title! (notitle)",
                    HSHR(vict));
            set_title(vict,buf);
            }
         break;
      case SCMD_SQUELCH:
         result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
         mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "(GC) Squelch %s for %s by %s.", ONOFF(result),
                 GET_NAME(vict), GET_NAME(ch));
         send_to_char(ch,"You have muted %s.",GET_NAME(vict));
         break;
      case SCMD_FREEZE:
         if (ch == vict)
            {
            send_to_char(ch,"Oh, yeah, THAT'S real smart...\r\n");
            release_buffer(buf);
            release_buffer(arg);
            return;
            }
         if (PLR_FLAGGED(vict, PLR_FROZEN))
            {
            send_to_char(ch,"Your victim is already pretty cold.\r\n");
            release_buffer(buf);
            release_buffer(arg);
            return;
            }
         SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
         GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
         send_to_char(vict,"A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
         send_to_char(ch,"Frozen.\r\n");
         act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict,
             0, 0, TO_ROOM);
         mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
         break;
      case SCMD_THAW:
         if (!PLR_FLAGGED(vict, PLR_FROZEN))
            {
            send_to_char(ch,"Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
            release_buffer(buf);
            release_buffer(arg);
            return;
            }
         if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch))
            {
            send_to_char(ch, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
                         GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
            release_buffer(buf);
            release_buffer(arg);
            return;
            }
         mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
         REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
         send_to_char(vict,"A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
         send_to_char(ch,"Thawed.\r\n");
         act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0,
             0, TO_ROOM);
         break;
      case SCMD_UNAFFECT:
         mudlogf(BRF,MAX(LVL_IMMORT,GET_INVIS_LEV(ch)),TRUE,
                 "(GC) %s has unaffected %s.",GET_NAME(ch),GET_NAME(vict));

         if (vict->affected)
            {
            while (vict->affected)
               affect_remove(vict, vict->affected);
            send_to_char(vict,"There is a brief flash of light!\r\n"
                         "You feel slightly different.\r\n");
            send_to_char(ch,"All spells removed.\r\n");
            }
         else
            {
            send_to_char(ch,"Your victim does not have any affections!\r\n");
            release_buffer(buf);
            release_buffer(arg);
            return;
            }
         break;
      default:
         log("SYSERR: Unknown subcmd: %d passed to do_wizutil (act.wizard.c)", subcmd);
         break;
         }
      save_char(vict, IN_ROOM(vict));
      }
   release_buffer(buf);
   release_buffer(arg);
   }




/* single zone printing fn used by "show zone" so it's not repeated in the
code 3 times ... -je, 4/6/93 */



void print_zone_to_buf(char *bufptr, zone_rnum zone)
   {
   sprintf(bufptr+strlen(bufptr),
           "%3ld %s%-30.30s&n Age: %3d;Reset: %3d(%1d);Tp: %s%5ld%s %c%c\r\n",
           zone_table[zone].number,
           zone_continent[zone_table[zone].continent][1],
           zone_table[zone].name,
           zone_table[zone].age, zone_table[zone].lifespan,
           zone_table[zone].reset_mode,
           ((zone_table[zone].top-(zone_table[zone].number*100))>100)?KRED:KNUL,
           zone_table[zone].top,KNRM,
           zone_status[(int)zone_table[zone].status][0],
           zone_source[(int)zone_table[zone].source][0]);
   }


void    print_idle_to_buf(char *bufptr, zone_rnum zone)
   {
   sprintf(bufptr+strlen(bufptr),
           "%3ld %s%-30.30s&n Age: %3d;Reset: %3d(%1d);Idle: %5d %c%c\r\n",
           zone_table[zone].number,
           zone_continent[zone_table[zone].continent][1],
           zone_table[zone].name,
           zone_table[zone].age, zone_table[zone].lifespan,
           zone_table[zone].reset_mode, zone_table[zone].idle_time,
           zone_status[(int)zone_table[zone].status][0],
           zone_source[(int)zone_table[zone].source][0]);
   }




void find_plague_mobs(struct char_data *ch)
   {
   struct char_data *i;
   char *buf=get_buffer(32750);
   char *buf1=get_buffer(SMALL_BUFSIZE);

   for (i = character_list; i ; i = i->next)
      {
      if (AFF_FLAGGED(i, AFF_PLAGUE))
         {
         if (IS_NPC(i))
            {
            sprintf(buf1, "%5ld", mob_index[i->nr].vnum);
            }
         else
            {
            strcpy(buf1, "  P  ");
            }
         sprintf(buf+strlen(buf),
                 "[%s] %-15.15s- %-25.25s %5d/%5dhp (%5ld)\r\n",
                 buf1, GET_NAME(i),
                 world[IN_ROOM(i)].name,
                 GET_HIT(i),GET_MAX_HIT(i),
                 GET_ROOM_VNUM(IN_ROOM(i)));
         if(strlen(buf)>32500)
            {
            strcat(buf,"Almost overran the buffer\r\n");
            break;
            }
         }
      }
   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");
   release_buffer(buf1);
   release_buffer(buf);
   }


void show_connections(struct char_data *ch,char *arg)
   {
   int zone_num;
   int j,i,k,tele_targ;
   char *buf;

   if(!*arg)
      {
      send_to_char(ch,"USAGE: show connections .\r\n");
      send_to_char(ch,"USAGE: show connections <zone_num>\r\n");
      return;
      }
   else if (*arg=='.')
      zone_num=world[IN_ROOM(ch)].zone;
   else
      {
      j=atoi(arg);
      for(zone_num=0;zone_num<=top_of_zone_table;zone_num++)
         if(zone_table[zone_num].number==j)
            break;
      }
   if(zone_num>=0 && zone_num <=top_of_zone_table)
      {
      buf=get_buffer(32750);

      sprintf(buf,"Connections from %-30.30s\r\n"
              "----------------------------------------------------------------------\r\n",
              zone_table[zone_num].name);

      for(i=0,k=0;i<=top_of_world;i++)
         {
         for(j=0;j<NUM_OF_DIRS;j++)
            {
            if(world[i].zone==zone_num    &&
                    world[i].dir_option[j]     &&
                    world[i].dir_option[j]->to_room!=-1 &&
                    world[world[i].dir_option[j]->to_room].zone!=zone_num)
               {
               sprintf(buf+strlen(buf),"%3d: [%5ld] %-23.23s -(%-5.5s)-> [%5ld] %-23.23s\r\n",++k,
                       GET_ROOM_VNUM(i), world[i].name,dirs[j],
                       GET_ROOM_VNUM(world[i].dir_option[j]->to_room),
                       world[world[i].dir_option[j]->to_room].name);
               }
            }

         if((world[i].zone==zone_num) &&
                 world[i].tele             &&
                 (world[i].tele->targ!=0))
            {
            if(((tele_targ=real_room(world[i].tele->targ))>=0)&&
                    (world[tele_targ].zone!=zone_num))
               {
               sprintf(buf+strlen(buf),"%3d: [%5ld] %-23.23s -(%-5.5s)-> [%5ld] %-23.23s\r\n",++k,
                       GET_ROOM_VNUM(i), world[i].name,"tport",
                       GET_ROOM_VNUM(tele_targ),
                       world[tele_targ].name);
               }
            }


         }
      sprintf(buf+strlen(buf),"\r\nConnections to %-30.30s\r\n"
              "----------------------------------------------------------------------\r\n",
              zone_table[zone_num].name);
      for(i=0,k=0;i<=top_of_world;i++)
         {
         for(j=0;j<NUM_OF_DIRS;j++)
            {
            if(world[i].zone!=zone_num    &&
                    world[i].dir_option[j]     &&
                    world[i].dir_option[j]->to_room!=-1 &&
                    world[world[i].dir_option[j]->to_room].zone==zone_num)
               {
               sprintf(buf+strlen(buf),"%3d: [%5ld] %-23.23s -(%-5.5s)-> [%5ld] %-23.23s\r\n",++k,
                       GET_ROOM_VNUM(i), world[i].name,dirs[j],
                       GET_ROOM_VNUM(world[i].dir_option[j]->to_room),
                       world[world[i].dir_option[j]->to_room].name);
               }
            }
         if((world[i].zone!=zone_num) &&
                 world[i].tele             &&
                 (world[i].tele->targ!=0))
            {
            if(((tele_targ=real_room(world[i].tele->targ))>=0)&&
                    (world[tele_targ].zone==zone_num))
               {
               sprintf(buf+strlen(buf),"%3d: [%5ld] %-23.23s -(%-5.5s)-> [%5ld] %-23.23s\r\n",++k,
                       GET_ROOM_VNUM(i), world[i].name,"tport",
                       GET_ROOM_VNUM(tele_targ),
                       world[tele_targ].name);
               }
            }
         }
      for(i=0;i<=top_of_objt;i++)
         {
         if((obj_proto[i].obj_flags.type_flag==ITEM_PORTAL) &&
                 (obj_proto[i].obj_flags.value[0]!=0))
            {
            if(((tele_targ=real_room(obj_proto[i].obj_flags.value[0]))>=0) &&
                    (world[tele_targ].zone==zone_num))
               sprintf(buf+strlen(buf),"%3d: [%5ld] %-23.23s -(%-5.5s)-> [%5ld] %-23.23s\r\n",++k,
                       obj_index[i].vnum,
                       obj_proto[i].short_description,
                       "prtal",
                       GET_ROOM_VNUM(tele_targ),
                       world[tele_targ].name);
            }
         }

      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      release_buffer(buf);
      return;
      }
   }


void show_rconnect(struct char_data *ch,char *arg)
   {
   int room_num;
   int j,i,k,tele_targ;
   char *buf;

   if(!*arg)
      {
      send_to_char(ch,"USAGE: show rconnect .\r\n");
      send_to_char(ch,"USAGE: show rconnect <room_num>\r\n");
      return;
      }
   else if (*arg=='.')
      room_num=IN_ROOM(ch);
   else
      {
      j=atoi(arg);
      for(room_num=0;room_num<=top_of_world;room_num++)
         if(GET_ROOM_VNUM(room_num)==j)
            break;
      }
   if(room_num>=0 && room_num <=top_of_world)
      {
      buf=get_buffer(32750);

      sprintf(buf,"Connections from %-30.30s\r\n"
              "----------------------------------------------------------------------\r\n",
              world[room_num].name);

      for(k=0,j=0;j<NUM_OF_DIRS;j++)
         {
         if(world[room_num].dir_option[j]     &&
                 world[room_num].dir_option[j]->to_room!=-1)
            {
            sprintf(buf+strlen(buf),"%3d: [%5ld] %-23.23s -(%-5.5s)-> [%5ld] %-23.23s\r\n",++k,
                    GET_ROOM_VNUM(room_num), world[room_num].name,dirs[j],
                    GET_ROOM_VNUM(world[room_num].dir_option[j]->to_room),
                    world[world[room_num].dir_option[j]->to_room].name);
            }
         }
      if(world[room_num].tele&&(world[room_num].tele->targ!=0))
         {
         if((tele_targ=real_room(world[room_num].tele->targ))>=0)
            sprintf(buf+strlen(buf),"%3d: [%5ld] %-23.23s -(%-5.5s)-> [%5ld] %-23.23s\r\n",++k,
                    GET_ROOM_VNUM(room_num), world[room_num].name,"tport",
                    GET_ROOM_VNUM(tele_targ),
                    world[tele_targ].name);
         }


      sprintf(buf+strlen(buf),"\r\nConnections to %-30.30s\r\n"
              "----------------------------------------------------------------------\r\n",
              world[room_num].name);
      for(i=0,k=0;i<=top_of_world;i++)
         {
         for(j=0;j<NUM_OF_DIRS;j++)
            {
            if(world[i].dir_option[j]     &&
                    world[i].dir_option[j]->to_room!=-1 &&
                    world[i].dir_option[j]->to_room==room_num)

               {
               sprintf(buf+strlen(buf),"%3d: [%5ld] %-23.23s -(%-5.5s)-> [%5ld] %-23.23s\r\n",++k,
                       GET_ROOM_VNUM(i), world[i].name,dirs[j],
                       GET_ROOM_VNUM(world[i].dir_option[j]->to_room),
                       world[world[i].dir_option[j]->to_room].name);
               }
            }
         if(world[i].tele&&(world[i].tele->targ!=0))
            {
            if((world[i].tele->targ==GET_ROOM_VNUM(room_num))&&
                    ((tele_targ=real_room(world[i].tele->targ))>=0))

               sprintf(buf+strlen(buf),"%3d: [%5ld] %-23.23s -(%-5.5s)-> [%5ld] %-23.23s\r\n",++k,
                       GET_ROOM_VNUM(i), world[i].name,"tport",
                       GET_ROOM_VNUM(tele_targ),
                       world[tele_targ].name);
            }

         }
      for(i=0;i<=top_of_objt;i++)
         {
         if((obj_proto[i].obj_flags.type_flag==ITEM_PORTAL) &&
                 (obj_proto[i].obj_flags.value[0]==GET_ROOM_VNUM(room_num)))
            {
            tele_targ=MAX(0,real_room(obj_proto[i].obj_flags.value[0]));
            sprintf(buf+strlen(buf),"%3d: [%5ld] %-23.23s -(%-5.5s)-> [%5ld] %-23.23s\r\n",++k,
                    obj_index[i].vnum,
                    obj_proto[i].short_description,
                    "prtal",
                    GET_ROOM_VNUM(tele_targ),
                    world[tele_targ].name);
            }
         }

      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      release_buffer(buf);
      return;
      }
   }

void show_mobs(struct char_data *ch) {
   const size_t linelength = 76; // Should match the headers and format strings below

   char *buf = get_buffer(linelength * (100)); // Room for 100 mobs

   int char_zone_num = world[IN_ROOM(ch)].zone;
   struct zone_data current_zone = zone_table[char_zone_num];

   // Zone 
   int first_mob_vnum = current_zone.number * 100;
   int last_mob_vnum = current_zone.top; // PM always sets this to first_mob_vnum + 99

   send_to_char(ch, "VNUM  Mob Name             LV   EXP      AC   HitP  HR  DR  Gold     Align\r\n");
   send_to_char(ch, "--------------------------------------------------------------------------\r\n");

   *buf = '\0';
   for (int possible_mob_vnum = first_mob_vnum; possible_mob_vnum <= last_mob_vnum; possible_mob_vnum++) {
      for (struct char_data* mob = character_list; mob; mob = mob->next) {
         if (GET_MOB_VNUM(mob) != possible_mob_vnum) continue;
         char line[linelength];

         // Color codes in mob name screw up table alignment
         char* mob_name_no_color = strdup(GET_NAME(mob));
         proc_color(mob_name_no_color, C_OFF);
         sprintf(line, "%5d %-20.20s %4d %8ld %4d %5d %3d %3d %8ld %5d\r\n",
                  possible_mob_vnum, mob_name_no_color, GET_LEVEL(mob), GET_EXP(mob),
                  GET_AC(mob), GET_MAX_HIT(mob),
                  GET_HITROLL(mob), GET_DAMROLL(mob),
                  GET_GOLD(mob), GET_ALIGNMENT(mob));
         strcat(buf, line);
      }
   }

   if (ch->desc)
      page_string(ch->desc, buf, TRUE, "");

   if (buf[0] == '\0')
      send_to_char(ch, "\r\nThere are no mobs in this zone... Sorry\r\n");

   release_buffer(buf);
}

void show_objs(struct char_data *ch) {
   struct obj_data *obj;
   int i, zone, top_zone, found;
   char *buf3=get_buffer(64);
   char *buf1=get_buffer(64);
   char *buf=get_buffer(32750);
   bool any = FALSE;


   *buf3 = '\0';
   zone = zone_table[(world[IN_ROOM(ch)].zone)].number * 100;
   top_zone = zone_table[(world[IN_ROOM(ch)].zone)].top;


   send_to_char(ch, "VNUM   Obj Name             Type\r\n");
   send_to_char(ch, "--------------------------------\r\n");


   *buf  = '\0';
   *buf1 = '\0';


   for (i = zone; i <= top_zone; i++)
      {
      found = FALSE;
      for (obj = object_list; obj; obj = obj->next)
         {
         if ((GET_OBJ_VNUM(obj) == i) && found == FALSE)
            {
            found = TRUE;
            any = TRUE;
            strncpy(buf3, obj->short_description, 30);
            sprinttype(GET_OBJ_TYPE(obj), item_types, buf1);
            sprintf(buf,"%s%-5d %-30s Type: %-20s\r\n",
                    buf,i, buf3,
                    buf1);
            *buf3 = '\0';
            }
         }
      }


   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");


   if (any == FALSE)
      send_to_char(ch,"\r\nThere are no objs in this zone... Sorry\r\n");
   release_buffer(buf);
   release_buffer(buf1);
   release_buffer(buf3);
   }

void show_item_spell(struct char_data *ch,char *value)
   {
   struct obj_data *obj;
   int i, level,spellnum;
   char *buf3;
   char *buf1;
   char *buf;
   char *buf2;
   char *arg;

   if(!value||!*value)
      {
      send_to_char(ch,"USAGE: show itemspell <spell_name>\r\n");
      return;
      }
   buf2=get_buffer(256);
   arg=get_buffer(128);
   half_chop(value,arg,buf2);
   release_buffer(arg);
   spellnum=find_skill_num(buf2);

   if((spellnum<1)||(spellnum>=MAX_SPELLS))
      {
      send_to_char(ch,"USAGE: show itemspell <A VALID spell_name>\r\n");
      release_buffer(buf2);
      return;
      }

   buf3=get_buffer(64);
   buf1=get_buffer(64);
   buf=get_buffer(32750);

   sprintf(buf,"Searching for spell: %s\r\n",spells[spellnum].spell_name);
   strcat(buf,"VNUM   Obj Name                  Type      SpellLevel\r\n");
   strcat(buf, "-----------------------------------------------------\r\n");

   for (i=0;i<=top_of_objt;i++)
      {
      obj=&obj_proto[i];
      level=0;
      switch(GET_OBJ_TYPE(obj))
         {
      case ITEM_SCROLL:
      case ITEM_PILL:
      case ITEM_POTION:
         if((GET_OBJ_VAL(obj,1)==spellnum)||(GET_OBJ_VAL(obj,2)==spellnum)||(GET_OBJ_VAL(obj,3)==spellnum))
            level =GET_OBJ_VAL(obj,0);
         break;
      case ITEM_WAND:
      case ITEM_STAFF:
         if(GET_OBJ_VAL(obj,3)==spellnum)
            level =GET_OBJ_VAL(obj,0);
         break;
      default:
         level = 0;
         break;
         }
      if(level>0)
         {
         strncpy(buf3, obj->short_description, 30);
         sprinttype(GET_OBJ_TYPE(obj), item_types, buf1);
         sprintf(buf+strlen(buf),"%-6ld %-25.25s %-10.10s %d\r\n",
                 GET_OBJ_VNUM(obj), buf3, buf1,level);
         *buf3 = '\0';
         if(strlen(buf)>32500)
            {
            strcat(buf,"Almost overran the buffer\r\n");
            break;
            }
         }
      }

   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");
   release_buffer(buf);
   release_buffer(buf1);
   release_buffer(buf2);
   release_buffer(buf3);

   }

void show_bad_spell_lvl(struct char_data *ch,char *value)
   {
   struct obj_data *obj;
   int i,j, level, null_spell;
   int type;
   char *buf3;
   char *buf;
   char *valid_types="Valid Item types are potion, scroll, staff, pill, and wand.\r\n";
   if(!value||!*value)
      {
      send_to_char(ch,"USAGE: show badspllvl <item_type>\r\n");
      send_to_char(ch, "%s", valid_types);
      return;
      }
   for(j=0;*item_types[j]!='\n';j++)
      {
      if(isname(value,item_types[j]))
         break;
      }
   if(*item_types[j]=='\n')
      {
      send_to_char(ch,"USAGE: show badspllvl <A VALID item_type>\r\n");
      send_to_char(ch, "%s", valid_types);
      return;
      }
   type=0;
   switch(j)
      {
   case ITEM_SCROLL:
   case ITEM_PILL:
   case ITEM_WAND:
   case ITEM_STAFF:
   case ITEM_POTION:
      type =1;
      break;
   default:
      type =0;
      break;
      }
   if(!type)
      {
      send_to_char(ch,"That type of item has no spells.\r\n");
      send_to_char(ch, "%s", valid_types);
      return;
      }

   buf3=get_buffer(64);
   buf=get_buffer(32750);

   sprintf(buf,"Searching for type: %s\r\n",item_types[j]);
   strcat(buf,"VNUM   Obj Name                  SpellLevel\r\n");
   strcat(buf, "------------------------------------------\r\n");

   for (i=0;i<=top_of_objt;i++)
      {
      obj=&obj_proto[i];
      level=0;
      null_spell=0;
      if(GET_OBJ_TYPE(obj)==j)
         switch(type)
            {
         case 1:
            level = GET_OBJ_VAL(obj,0);
            if ((GET_OBJ_VAL(obj,1) == 0) ||
                (GET_OBJ_VAL(obj,2) == 0) ||
                (GET_OBJ_VAL(obj,3) == 0))
               null_spell = 1;
            break;
         default:
            level =0;
            break;
            }
      if(level>10 || null_spell)
         {
         strncpy(buf3, obj->short_description, 30);
         sprintf(buf+strlen(buf),"%-6ld %-25.25s %d\r\n",
                 GET_OBJ_VNUM(obj), buf3,level);
         *buf3 = '\0';
         if(strlen(buf)>32500)
            {
            strcat(buf,"Almost overran the buffer\r\n");
            break;
            }
         }
      }

   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");
   release_buffer(buf);
   release_buffer(buf3);
   }


#define DIR_NOR (1<<0)
#define DIR_EAS (1<<1)
#define DIR_SOU (1<<2)
#define DIR_WES (1<<3)
#define DIR_UP  (1<<4)
#define DIR_DOW (1<<5)

extern int nExamRecords;
extern struct gremort_exam_record *examRecords;
extern const char *gremort_exam_types[3];
extern const char *gremort_exam_results[5];

ACMD(do_show)
   {
   struct char_file_u vbuf;
   long i, j, k, l, t, con,zstt,zsrc;
   obj_rnum orn;
   zone_rnum zrn;
   zone_vnum zvn;
   struct descriptor_data *d, *v;
   int y,pos,room_exits[100];
   char self = 0;
   struct char_data *vict, *cbuf = NULL,*victim,*mob;
   struct char_file_u tmp_store;
   struct obj_data *obj;
   char *field=get_buffer(MAX_INPUT_LENGTH);
   char *value=get_buffer(MAX_INPUT_LENGTH);
   char *buf=get_buffer(32750);
   char *birth=get_buffer(80);
   char *arg;
   char *tmpPtr;
   int sortpos,zone,room,rmob_num;
   int count;
   int zonestat[5];
   int zonesrc[5];
   int xap_objs_backup;
   struct show_struct
      {
      char *cmd;
      char level;
      int  position;
      }
   fields[] =  {
                  { "nothing",    0, 0 } ,
                  { "active",     LVL_SERPII, SHOW_ACTIVE } ,
                  { "assemblies", LVL_SERPII, SHOW_ASSEMBLIES },
                  { "badspllvl",  LVL_SERPII, SHOW_BADITEMSPLLVL } ,
                  { "buffer",     LVL_SERP,   SHOW_BUFFER } ,
                  { "clan",       LVL_SERPII, SHOW_CLAN } ,
                  { "connections",LVL_SERPII, SHOW_CONNECTIONS },
                  { "death",      LVL_DGOD,   SHOW_DEATH } ,
                  { "donation",   LVL_SERPII, SHOW_DONATION } ,
                  { "errors",     LVL_SERPII, SHOW_ERRORS } ,
                  { "fighting",   LVL_SERPII, SHOW_FIGHTING },
                  { "godcommand", LVL_DGOD,   SHOW_GODCOMMAND },
                  { "godrooms",   LVL_DGOD,   SHOW_GODROOMS } ,
                  { "guildmaster",LVL_SERPII, SHOW_GUILDMASTER },
                  { "hometowns",  LVL_SERPII, SHOW_HOMETOWNS },
                  { "houses",     LVL_SERPII, SHOW_HOUSES } ,
                  { "idle",       LVL_SERPII, SHOW_IDLE } ,
                  { "itemspell",  LVL_GRGOD,  SHOW_ITEMSPELL } ,
                  { "kills",      LVL_GRGOD,  SHOW_KILLS },
                  { "logbuf",     LVL_SIMP,   SHOW_LOGBUF } ,
                  { "maze",       LVL_ADMIN,  SHOW_MAZE } ,
                  { "mobs",       LVL_SERPII, SHOW_MOBS } ,
                  { "mobspecs",   LVL_SERPII, SHOW_MOBSPECS },
                  { "newbieeq",   LVL_SERPII, SHOW_NEWBIEEQ },
                  { "nodecay",    LVL_SERPII, SHOW_NODECAY} ,
                  { "nomagic",    LVL_SERPII, SHOW_NOMAGIC } ,
                  { "nomob",      LVL_SERPII, SHOW_NOMOB } ,
                  { "noposchk",   LVL_SERPII, SHOW_NOPOSCHK },
                  { "norecall",   LVL_SERPII, SHOW_NORECALL } ,
                  { "nosummon",   LVL_SERPII, SHOW_NOSUMMON } ,
                  { "notrack",    LVL_SERPII, SHOW_NOTRACK } ,
                  { "objs",       LVL_SERPII, SHOW_OBJS } ,
                  { "olczones",   LVL_IMMORT, SHOW_OLCZONES },
                  { "peaceful",   LVL_SERPII, SHOW_PEACEFUL } ,
                  { "pkill",      LVL_SERPII, SHOW_PKILL } ,
                  { "plague",     LVL_SERPII, SHOW_PLAGUE } ,
                  { "player",     LVL_DGOD,   SHOW_PLAYER } ,
                  { "private",    LVL_SERPII, SHOW_PRIVATE } ,
                  { "race",       LVL_DGOD,   SHOW_RACE },
                  { "rconnect",   LVL_SERPII, SHOW_RCONNECT },
                  { "reimb",      LVL_DGOD,   SHOW_REIMB },
                  { "regen",      LVL_SERPII, SHOW_REGEN } ,
                  { "rent",       LVL_DGOD,   SHOW_RENT } ,
                  { "shops",      LVL_SERPII, SHOW_SHOPS } ,
                  { "skills",     LVL_IMMORT, SHOW_SKILLS } ,
                  { "skillstat",  LVL_DGODI,  SHOW_SPELLSTAT } ,
                  { "snoop",      LVL_ADMIN,  SHOW_SNOOP } ,
                  { "spells",     LVL_IMMORT, SHOW_SPELLS } ,
                  { "spellstat",  LVL_GODI,   SHOW_SPELLSTAT } ,
                  { "stats",      LVL_SERPII, SHOW_STATS } ,
                  { "switch",     LVL_IMMORT, SHOW_SWITCH } ,
                  { "teleport"   ,LVL_SERPII, SHOW_TELEPORT },
                  { "tunnel",     LVL_SERPII, SHOW_TUNNEL } ,
                  { "twohanded",  LVL_SERPII, SHOW_TWOHANDED },
                  { "zones",      LVL_SERPII, SHOW_ZONES },
                  { "objdata",    LVL_ADMIN,  SHOW_OBJDATA },
                  { "spellaffects",LVL_GRGOD, SHOW_SPELLAFFECTS },
                  { "weaponspell",LVL_GRGOD,  SHOW_WEAPONSPELL },
                  { "oldremorteq",LVL_DGOD,   SHOW_OLD_REMORTEQ },
                  { "olddblremorteq",LVL_DGOD,SHOW_OLD_DBLREMORTEQ },
                  { "dblremorteq", LVL_DGOD,  SHOW_DBLREMORTEQ },
                  { "remorteq",   LVL_DGOD,   SHOW_REMORTEQ },
                  { "godeq",      LVL_DGOD,   SHOW_GOD_EQ },
                  { "resists",    LVL_GRGOD,  SHOW_OBJRESIST },
                  { "ignored",    LVL_GRGOD,  SHOW_IGNORED },
                  { "nevermob",   LVL_SERPII, SHOW_NEVERMOB },
          { "quests",     LVL_SERPII, SHOW_QUESTS },
          { "graffiti",   LVL_IMMORT, SHOW_GRAFFITI },
          { "explored",   LVL_IMMORT, SHOW_EXPLORED },
          { "email",      LVL_DGOD,   SHOW_EMAIL },
          { "gremort_record", LVL_GRGOD, SHOW_GREMORT_RECORDS },
          { "player_shops", LVL_IMMORT, SHOW_PLAYER_SHOPS },
                  { "\n",         0, 0 }
               };
   /* search for: */
   /**** END ****/

   if(GET_LEVEL(ch)<LVL_IMMORT)
      {
      send_to_char(ch,"Find a hometown receptionist to do that!\r\n");
      release_buffer(buf);
      release_buffer(field);
      release_buffer(birth);
      release_buffer(value);
      return;
      }

   skip_spaces(&argument);


   if (!*argument)
      {
      strcpy(buf, "Show options:\r\n");
      for (j = 0, i = 1; fields[i].level; i++)
         if (fields[i].level <= GET_LEVEL(ch))
            sprintf(buf+strlen(buf), "|%-13.13s %-16.16s|%s", fields[i].cmd,
                    WizLevels[((int)fields[i].level)-LVL_IMMORT],
                    (!(++j % 2) ? "\r\n" : ""));
      strcat(buf, "\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      release_buffer(buf);
      release_buffer(field);
      release_buffer(birth);
      release_buffer(value);
      return;
      }

   arg=get_buffer(MAX_INPUT_LENGTH);
   strcpy(arg, two_arguments(argument, field, value));
   release_buffer(arg);

   for (l = 0; *(fields[l].cmd) != '\n'; l++)
      if (!strncmp(field, fields[l].cmd, strlen(field)))
         break;


   if (GET_LEVEL(ch) < fields[l].level)
      {
      send_to_char(ch,"You are not godly enough for that!\r\n");
      release_buffer(buf);
      release_buffer(field);
      release_buffer(birth);
      release_buffer(value);
      return;
      }
   if (!strcmp(value, "."))
      self = 1;
   buf[0] = '\0';
   switch (fields[l].position)
      {
   case SHOW_ZONES:           /* zone */
      /* tightened up by JE 4/6/93 */
      if (self)
         {
         char *buf1=get_buffer(256);
         j=zone_table[world[IN_ROOM(ch)].zone].number;
         print_zone_to_buf(buf, world[IN_ROOM(ch)].zone);
         sprintbit(zone_table[world[IN_ROOM(ch)].zone].bitvector,
                   zone_bits,buf1);
         sprintf(buf+strlen(buf)," Zone Flags: %s\r\n",buf1);
         for(count=0,k=0;k<=top_of_objt;k++)
            {
            if((obj_index[k].vnum/100)==j)
               count++;
            }
         sprintf(buf+strlen(buf)," Objects: %d, ",count);
         for(count=0,k=0;k<=top_of_mobt;k++)
            {
            if((mob_index[k].vnum/100)==j)
               count++;
            }
         sprintf(buf+strlen(buf)," Mobiles: %d, ",count);
         for(count=0,k=0;k<=top_of_world;k++)
            {
            if((GET_ROOM_VNUM(k)/100)==j)
               count++;
            }
         sprintf(buf+strlen(buf)," Rooms: %d, ",count);
         for(count=0,k=0;k<top_of_trigt;k++)
            {
            if((trig_index[k]->vnum/100)==j)
               count++;
            }
         sprintf(buf+strlen(buf)," Triggers: %d\r\n",count);
         sprintf(buf+strlen(buf)," Continent: %s%s&n\r\n",
                 zone_continent[zone_table[world[IN_ROOM(ch)].zone].continent][1],
                 zone_continent[zone_table[world[IN_ROOM(ch)].zone].continent][0]);
         sprintf(buf+strlen(buf)," Author: %s\r\n",
                 zone_table[world[IN_ROOM(ch)].zone].author);
         sprintf(buf+strlen(buf)," Editor: %s\r\n",
                 zone_table[world[IN_ROOM(ch)].zone].editor);
         sprintf(buf+strlen(buf)," Levels: %s\r\n",
                 zone_table[world[IN_ROOM(ch)].zone].levels);
         sprintf(buf+strlen(buf)," Status: %s\r\n",
                 zone_status[(int)zone_table[world[IN_ROOM(ch)].zone].status]);
         sprintf(buf+strlen(buf)," Source: %s\r\n",
                 zone_source[(int)zone_table[world[IN_ROOM(ch)].zone].source]);
         tmpPtr = (char *)asctime(localtime(&zone_table[world[IN_ROOM(ch)].zone].dateStarted));
         *(tmpPtr+strlen(tmpPtr) - 1) = '\0';
         sprintf(buf+strlen(buf)," Date Started      : %s\r\n",tmpPtr);
         tmpPtr = (char *)asctime(localtime(&zone_table[world[IN_ROOM(ch)].zone].dateImped));
         *(tmpPtr+strlen(tmpPtr) - 1) = '\0';
         sprintf(buf+strlen(buf)," Date Imped        : %s\r\n",tmpPtr);
         tmpPtr = (char *)asctime(localtime(&zone_table[world[IN_ROOM(ch)].zone].dateLastMod));
         *(tmpPtr+strlen(tmpPtr) - 1) = '\0';
         sprintf(buf+strlen(buf)," Date Last Modified  : %s\r\n",tmpPtr);
         sprintf(buf+strlen(buf)," Last Modified by    : %s\r\n",zone_table[world[IN_ROOM(ch)].zone].nameLastMod);
         sprintf(buf+strlen(buf)," World Proofed by    : %s\r\n",zone_table[world[IN_ROOM(ch)].zone].worldProof);
         sprintf(buf+strlen(buf)," Triggers proofed by : %s\r\n",zone_table[world[IN_ROOM(ch)].zone].trigProof);
         sprintf(buf+strlen(buf)," Objects Balanced by : %s\r\n",zone_table[world[IN_ROOM(ch)].zone].objBalanced);
         sprintf(buf+strlen(buf)," Comment             : %s\r\n",zone_table[world[IN_ROOM(ch)].zone].comment);
         release_buffer(buf1);

         }
      else if (*value && is_number(value))
         {
         char *buf1=get_buffer(256);
         for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++)
            ;
         if (zrn <= top_of_zone_table)
            print_zone_to_buf(buf, zrn);
         else
            {
            send_to_char(ch,"That is not a valid zone.\r\n");
            release_buffer(buf1);
            release_buffer(buf);
            release_buffer(field);
            release_buffer(birth);
            release_buffer(value);
            return;
            }
         sprintbit((long)zone_table[zrn].bitvector, zone_bits,buf1);
         sprintf(buf+strlen(buf)," Zone Flags: %s\r\n",buf1);
         for(count=0,k=0;k<=top_of_objt;k++)
            {
            if((obj_index[k].vnum/100)==zvn)
               count++;
            }
         sprintf(buf+strlen(buf)," Objects: %d, ",count);
         for(count=0,k=0;k<=top_of_mobt;k++)
            {
            if((mob_index[k].vnum/100)==zvn)
               count++;
            }
         sprintf(buf+strlen(buf)," Mobiles: %d, ",count);
         for(count=0,k=0;k<=top_of_world;k++)
            {
            if((GET_ROOM_VNUM(k)/100)==zvn)
               count++;
            }
         sprintf(buf+strlen(buf)," Rooms: %d, ",count);
         for(count=0,k=0;k<top_of_trigt;k++)
            {
            if((trig_index[k]->vnum/100)==zvn)
               count++;
            }
         sprintf(buf+strlen(buf)," Triggers: %d\r\n",count);
         sprintf(buf+strlen(buf)," Continent: %s%s&n\r\n",
                 zone_continent[zone_table[zrn].continent][1],
                 zone_continent[zone_table[zrn].continent][0]);
         sprintf(buf+strlen(buf)," Author: %s\r\n",
                 zone_table[zrn].author);
         sprintf(buf+strlen(buf)," Editor: %s\r\n",
                 zone_table[zrn].editor);
         sprintf(buf+strlen(buf)," Levels: %s\r\n",
                 zone_table[zrn].levels);
         sprintf(buf+strlen(buf)," Status: %s\r\n",
                 zone_status[(int)zone_table[zrn].status]);
         sprintf(buf+strlen(buf)," Source: %s\r\n",
                 zone_source[(int)zone_table[zrn].source]);
         tmpPtr = (char *)asctime(localtime(&zone_table[zrn].dateStarted));
         *(tmpPtr+strlen(tmpPtr) - 1) = '\0';
         sprintf(buf+strlen(buf)," Date Started      : %s\r\n",tmpPtr);
         tmpPtr = (char *)asctime(localtime(&zone_table[zrn].dateImped));
         *(tmpPtr+strlen(tmpPtr) - 1) = '\0';
         sprintf(buf+strlen(buf)," Date Imped        : %s\r\n",tmpPtr);
         tmpPtr = (char *)asctime(localtime(&zone_table[zrn].dateLastMod));
         *(tmpPtr+strlen(tmpPtr) - 1) = '\0';
         sprintf(buf+strlen(buf)," Date Last Modified: %s\r\n",tmpPtr);
         sprintf(buf+strlen(buf)," Last Modified by  : %s\r\n",zone_table[zrn].nameLastMod);
         sprintf(buf+strlen(buf)," World Proofed by    : %s\r\n",zone_table[zrn].worldProof);
         sprintf(buf+strlen(buf)," Triggers proofed by : %s\r\n",zone_table[zrn].trigProof);
         sprintf(buf+strlen(buf)," Objects Balanced by : %s\r\n",zone_table[zrn].objBalanced);
         sprintf(buf+strlen(buf)," Comment           : %s\r\n",zone_table[zrn].comment);
         release_buffer(buf1);
         }
      else if(*value)
         {
         zrn=0;
         zstt=0;
         zsrc=0;

         while(*(zone_continent[zrn][0])!='\n')
            {
            if(is_abbrev(value,zone_continent[zrn][0]))
               {
               sprintf(buf,"Zones in the %s%s&n continent:\r\n",
                       zone_continent[zrn][1],
                       zone_continent[zrn][0]);
               for(j=0;j<=top_of_zone_table;j++)
                  {
                  if(zone_table[j].continent==zrn)
                     print_zone_to_buf(buf,j);
                  }
               break;
               }
            zrn++;
            }
         while(*(zone_status[zstt])!='\n')
            {
            if(is_abbrev(value,zone_status[zstt]))
               {
               sprintf(buf,"Zones with the status of %s:\r\n",
                       zone_status[zstt]);
               for(j=0;j<=top_of_zone_table;j++)
                  {
                  if(zone_table[j].status==zstt)
                     print_zone_to_buf(buf,j);
                  }
               break;
               }
            zstt++;
            }
         while(*(zone_source[zsrc])!='\n')
            {
            if(is_abbrev(value,zone_source[zsrc]))
               {
               sprintf(buf,"Zones with the source of %s:\r\n",
                       zone_source[zsrc]);
               for(j=0;j<=top_of_zone_table;j++)
                  {
                  if(zone_table[j].source==zsrc)
                     print_zone_to_buf(buf,j);
                  }
               break;
               }
            zsrc++;
            }
         if((*(zone_continent[zrn][0])=='\n')&&
                 (*(zone_status[zstt]) == '\n')&&
                 (*(zone_source[zsrc]) == '\n'))
            {
            strcat(buf,
                   "Invalid argument.  Valid arguments are:\r\n");
            j=0;
            while(*(zone_continent[j][0])!='\n')
               {
               sprintf(buf+strlen(buf),"%s%s&n\r\n",zone_continent[j][1],
                       zone_continent[j][0]);
               j++;
               }
            j=0;
            while(*(zone_status[j])!='\n')
               {
               sprintf(buf+strlen(buf),"%s\r\n",zone_status[j]);
               j++;
               }
            j=0;
            while(*(zone_source[j])!='\n')
               {
               sprintf(buf+strlen(buf),"%s\r\n",zone_source[j]);
               j++;
               }
            }
         }
      else
         for (i = 0; i <= top_of_zone_table; i++)
            print_zone_to_buf(buf, i);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_PLAYER:           /* player */
      if (!*value)
         {
         send_to_char(ch,"A name would help.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(birth);
         release_buffer(value);
         return;
         }


      if (load_char(value, &vbuf) < 0)
         {
         send_to_char(ch,"There is no such player.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(birth);
         release_buffer(value);
         return;
         }
      send_to_char(ch, "Player: %-12s (%s) [%2d %s]\r\n", vbuf.name,
                   genders[(int) vbuf.sex], vbuf.level,
                   class_abbrevs[(int) vbuf.class]);
      send_to_char(ch,
                   "Au: %-8ld  Bal: %-8ld  Exp: %-8ld  Align: %-5d  Last Learnt: %-3d\r\n",
                   vbuf.points.gold[0],vbuf.points.bank_gold[0],vbuf.points.exp,
                   vbuf.char_specials_saved.alignment,
                   vbuf.player_specials_saved.last_learnt);
      strcpy(birth, ctime(&vbuf.birth));
      send_to_char(ch,
                   "Started: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n",
                   birth, ctime(&vbuf.last_logon), (int) (vbuf.played / 3600),
                   (int) (vbuf.played / 60 % 60));
      break;
   case SHOW_RENT:
      if (!*value)
         send_to_char(ch, "Rent file for whom?\r\n");
      else
         Crash_listrent(ch, value);
      break;
   case SHOW_REIMB:
      if (!*value)
         send_to_char(ch, "Reimb file for whom?\r\n");
      else
         {
         xap_objs_backup = xap_objs;
         xap_objs = 2;
         Crash_listrent(ch, value);
         xap_objs = xap_objs_backup;
         }
      break;
   case SHOW_STATS:
      i = 0;
      j = 0;
      k = 0;
      con = 0;
      for (vict = character_list; vict; vict = vict->next)
         {
         if (IS_NPC(vict))
            j++;
         else if (CAN_SEE(ch, vict))
            {
            i++;
            if (vict->desc)
               con++;
            }
         }
      for (obj = object_list; obj; obj = obj->next)
         k++;
      send_to_char(ch, "Current stats:\r\n");
      send_to_char(ch, " Players:\r\n");
      send_to_char(ch, "  %5ld players in game  %5ld connected\r\n",
                   i, con);
      send_to_char(ch, "  %5d registered       %ld top idnum\r\n",
                   top_of_p_table + 1,top_idnum);

      send_to_char(ch, " World Database:\r\n");
      send_to_char(ch, "  %5ld mobiles          %5ld prototypes\r\n",
                   j, top_of_mobt + 1);
      send_to_char(ch, "  %5ld objects          %5ld prototypes\r\n",
                   k, top_of_objt + 1);
      send_to_char(ch, "  %5ld rooms            %5ld zones\r\n",
                   top_of_world + 1, top_of_zone_table + 1);
      send_to_char(ch, "  %5d paths\r\n",top_path+1);
      for(i=0;i<5;i++)
         {
         zonestat[i]=0;
         zonesrc[i]=0;
         }
      for(i=0;i<=top_of_zone_table;i++)
         {
         if(zone_table[i].status<5)
            zonestat[(int)zone_table[i].status]++;
         if(zone_table[i].source<4)
            zonesrc[(int)zone_table[i].source]++;
         }
      send_to_char(ch, " Zone Sources:\r\n");
      send_to_char(ch, "  %5d %-10.10s       %5d %-10.10s\r\n",
                   zonesrc[0],zone_source[0],zonesrc[1],zone_source[1]);
      send_to_char(ch, "  %5d %-10.10s       %5d %-10.10s\r\n",
                   zonesrc[2],zone_source[2],zonesrc[3],zone_source[3]);

      send_to_char(ch, " Zone Status:\r\n");
      send_to_char(ch, "  %5d %-11.11s      %5d %-22.22s\r\n",
                   zonestat[0],zone_status[0],zonestat[1],zone_status[1]);
      send_to_char(ch, "  %5d %-10.10s       %5d %-10.10s\r\n",
                   zonestat[2],zone_status[2],zonestat[3],zone_status[3]);
      send_to_char(ch, "  %5d %-10.10s\r\n",zonestat[4],zone_status[4]);

      send_to_char(ch, " Internal Buffers\r\n");
      send_to_char(ch, "  %5d large bufs\r\n", buf_largecount);
      send_to_char(ch, "  %5d buf switches     %5d overflows\r\n",
                   buf_switches, buf_overflows);
      send_to_char(ch,
                   " %6d buf cache hits  %6d buf cache misses\r\n",
                   buffer_cache_stat[BUFFER_CACHE_HITS],
                   buffer_cache_stat[BUFFER_CACHE_MISSES]);
      send_to_char(ch, "  %5ldk Total Bytes writen to players\r\n",
                   total_bytes_written/1024);
      send_to_char(ch, "  %5d charged for repairs   %5d charged for recharges\r\n",total_repair,total_recharge);
      break;
   case SHOW_ERRORS:
      strcpy(buf, "Errant Rooms\r\n------------\r\n");
      for (i = 0, k = 0; i <= top_of_world; i++)
         for (j = 0; j < NUM_OF_DIRS; j++)
            if (world[i].dir_option[j]&&world[i].dir_option[j]->to_room==0)
               sprintf(buf+strlen(buf),"%2ld: [%5ld] %s\r\n",++k,
                       GET_ROOM_VNUM(i), world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_DEATH:
      strcpy(buf, "Death Traps\r\n-----------\r\n");
      for (i = 0, j = 0; i <= top_of_world; i++)
         if (ROOM_FLAGGED(i, ROOM_DEATH))
            sprintf(buf+strlen(buf), "%2ld: [%5ld] %s\r\n", ++j,
                    GET_ROOM_VNUM(i), world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_GODROOMS:
      strcpy(buf, "Godrooms\r\n--------------------------\r\n");
      for (i = 0, j = 0; i < top_of_world; i++)
         if (ROOM_FLAGGED(i, ROOM_GODROOM))
            sprintf(buf+strlen(buf),"%2ld: [%5ld] %s\r\n",++j,
                    GET_ROOM_VNUM(i),world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_SHOPS:
      show_shops(ch, value);
      break;
   case SHOW_HOUSES:
      hcontrol_list_houses(ch);
      break;
   case SHOW_SPELLS:
   case SHOW_SKILLS:
      /* show spell and skill knowledge of a character (BK) */
      if (!*value)
         {
         send_to_char(ch,"Spell and skill knowledge of who?\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(birth);
         release_buffer(value);
         return;
         }

      if ((vict = get_char_room_vis(ch, value)) ||
              (vict = get_char_vis(ch, value,FIND_CHAR_WORLD)))
         {
         if(IS_NPC(vict) ||
                 (GET_LEVEL(vict) >= LVL_IMMORT && GET_LEVEL(ch) < LVL_IMPL))
            {
            release_buffer(buf);
            release_buffer(field);
            release_buffer(birth);
            release_buffer(value);
            return;
            }

         buf[0]='\0';
         for (j = 0, sortpos = 1; sortpos < MAX_SPELLS; sortpos++)
            {
            i = spell_sort_info[sortpos];
            if (strlen(buf) >= (MAX_STRING_LENGTH - 60))
               {
               strcat(buf, "**OVERFLOW**\r\n");
               break;
               }
            if (min_level(vict,i) <= GET_LEVEL(vict))
               {
               if ((k = GET_SKILL(vict,i)) > 0)
                  {
                  if((j++)%2)
                     sprintf(buf+strlen(buf),"%-21s %3ld%% %3d \r\n",
                             spells[i].spell_name,k,
                             GET_SKILL_LEARN(vict,i));
                  else
                     sprintf(buf+strlen(buf),"%-21s %3ld%% %3d ",
                             spells[i].spell_name,k,
                             GET_SKILL_LEARN(vict,i));
                  }
               /*      break;  */
               }
            }
         }
      else
         {
         /* currently not playing */
#if 0 /* reading from file not yet working */
         sprintf(buf, "Sorry, can't see %s.", value);
#endif


         CREATE(cbuf, struct char_data, 1);
         clear_char(cbuf);
         if (load_char(value, &tmp_store) > -1)
            {
            store_to_char(&tmp_store, cbuf);
            if (GET_LEVEL(cbuf) >= GET_LEVEL(ch) && GET_LEVEL(ch) < LVL_IMPL)
               {
               free_char(cbuf);
               send_to_char(ch,"Sorry, you can't do that.\r\n");
               release_buffer(buf);
               release_buffer(field);
               release_buffer(birth);
               release_buffer(value);
               return;
               }
            vict = cbuf;
            }
         else
            {
            free(cbuf);
            send_to_char(ch,"There is no such player.\r\n");
            release_buffer(buf);
            release_buffer(field);
            release_buffer(birth);
            release_buffer(value);
            return;
            }


         if (GET_LEVEL(vict) >= LVL_IMMORT && GET_LEVEL(ch) < LVL_IMPL)
            {
            release_buffer(buf);
            release_buffer(field);
            release_buffer(birth);
            release_buffer(value);
            free_char(vict);
            return;
            }

         buf[0] = '\0';
         for (j = 0, sortpos = 1; sortpos < MAX_SPELLS; sortpos++)
            {
            i = spell_sort_info[sortpos];
            if (strlen(buf) >= (MAX_STRING_LENGTH - 60))
               {
               strcat(buf, "**OVERFLOW**\r\n");
               break;
               }
            if (min_level(vict,i) <= GET_LEVEL(vict))
               {
               if ((k = GET_SKILL(vict,i)) > 0)
                  {
                  if ((j++)%2)
                     sprintf(buf,"%s%-21s %3ld%%     \r\n",buf,spells[i].spell_name,k);
                  else
                     sprintf(buf,"%s%-21s %3ld%%     ",buf,spells[i].spell_name,k);
                  }
               /*      break;  */
               }
            }
         free_char(vict);
         }
      page_string(ch->desc, buf, TRUE,"");
      break;
      /****/
   case SHOW_SPELLSTAT:
         if (!*value) {
            send_to_char(ch,"Which spell or skill would you like to see?\r\n");
            release_buffer(buf);
            release_buffer(field);
            release_buffer(birth);
            release_buffer(value);
            return;
         }
      show_spellstat(ch,value);
      break;
   case SHOW_PLAGUE: /* plague! -shroom */
      send_to_char(ch,"Mobs affected by the plague:\r\n");
      find_plague_mobs(ch);
      break;
   case SHOW_MOBS:
      show_mobs(ch);
      break;
   case SHOW_OBJS:
      show_objs(ch);
      break;
   case SHOW_IDLE:
      send_to_char(ch,"Idle Zones\r\n-----------------------\r\n");
      for (i = 0; i <= top_of_zone_table; i++)
         if (ZONE_FLAGGED(i, Z_IDLE))
            print_idle_to_buf(buf, i);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_PEACEFUL:
      strcpy(buf, "Peaceful Rooms\r\n--------------------------\r\n");
      for (i = 0, j = 0; i < top_of_world; i++)
         if (IS_SET(world[i].room_flags, ROOM_PEACEFUL))
            sprintf(buf, "%s%2ld: [%5ld] %s\r\n", buf, ++j,
                    GET_ROOM_VNUM(i), world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_NORECALL:
   case SHOW_NOSUMMON:
   case SHOW_NOMOB:
   case SHOW_NOTRACK:
   case SHOW_NEVERMOB:
      sprintf(buf, "%s Rooms\r\n--------------------------\r\n", fields[l].cmd);
      if (*value && is_number(value))
         {
         count=atoi(value);
         }
      else
         {
         count=0;
         }
      for (i = 0, j = 0; i < top_of_world; i++)
         {
         if ((IS_SET(world[i].room_flags, ROOM_NOTRACK)&&(fields[l].position==SHOW_NOTRACK))||
             (IS_SET(world[i].room_flags, ROOM_NOMOB)&&(fields[l].position==SHOW_NOMOB))||
             (IS_SET(world[i].room_flags, ROOM_NO_SUMMON)&&(fields[l].position==SHOW_NOSUMMON))||
             (IS_SET(world[i].room_flags, ROOM_NO_RECALL)&&(fields[l].position==SHOW_NORECALL))||
             (IS_SET(world[i].room2_flags, ROOM2_NEVERMOB)&&(fields[l].position==SHOW_NEVERMOB)))
            {
            j++;
            if(j>=count)
               sprintf(buf, "%s%2ld: [%5ld] %s\r\n", buf, j,
                      GET_ROOM_VNUM(i), world[i].name)
               ;
            if(strlen(buf)>32400)
               {
               sprintf(buf+strlen(buf), "Buffer Size Exceeded.  Use show %s <number> "
                                        "(%ld)to view more.\r\n", fields[l].cmd, j);
               break;
               }
            }
         }
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_NOMAGIC:
      strcpy(buf, "No Magic Rooms\r\n--------------------------\r\n");
      for (i = 0, j = 0; i < top_of_world; i++)
         if (IS_SET(world[i].room_flags, ROOM_NOMAGIC))
            sprintf(buf, "%s%2ld: [%5ld] %s\r\n", buf, ++j,
                    GET_ROOM_VNUM(i), world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_NODECAY:
      strcpy(buf, "No Decay Rooms\r\n--------------------------\r\n");
      for (i = 0, j = 0; i < top_of_world; i++)
         if (IS_SET(world[i].room_flags, ROOM_NO_DECAY))
            sprintf(buf, "%s%2ld: [%5ld] %s\r\n", buf, ++j,
                    GET_ROOM_VNUM(i), world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_CLAN:
      strcpy(buf, "Clan Rooms\r\n--------------------------\r\n");
      for (i = 0, j = 0; i < top_of_world; i++)
         if (IS_SET(world[i].room_flags, ROOM_CLAN))
            sprintf(buf, "%s%2ld: [%5ld] %s\r\n", buf, ++j,
                    GET_ROOM_VNUM(i), world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_REGEN:
      strcpy(buf, "Regeneration Rooms\r\n--------------------------\r\n");
      for (i = 0, j = 0; i < top_of_world; i++)
         if (IS_SET(world[i].room_flags, ROOM_REGEN))
            sprintf(buf, "%s%2ld: [%5ld] %s\r\n", buf, ++j,
                    GET_ROOM_VNUM(i), world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_TUNNEL:
      strcpy(buf, "Tunnel Rooms\r\n--------------------------\r\n");
      for (i = 0, j = 0; i < top_of_world; i++)
         if (IS_SET(world[i].room_flags, ROOM_TUNNEL))
            sprintf(buf, "%s%2ld: [%5ld] %s\r\n", buf, ++j,
                    GET_ROOM_VNUM(i), world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_PRIVATE:
      strcpy(buf, "Private Rooms\r\n--------------------------\r\n");
      for (i = 0, j = 0; i < top_of_world; i++)
         if (IS_SET(world[i].room_flags, ROOM_PRIVATE))
            sprintf(buf, "%s%2ld: [%5ld] %s\r\n", buf, ++j,
                    GET_ROOM_VNUM(i), world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_PKILL:
      strcpy(buf, "P-Kill Rooms\r\n--------------------------\r\n");
      for (i = 0, j = 0; i < top_of_world; i++)
         if (IS_SET(world[i].room_flags, ROOM_PKILL))
            sprintf(buf, "%s%2ld: [%5ld] %s\r\n", buf, ++j,
                    GET_ROOM_VNUM(i), world[i].name);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_ACTIVE:
      send_to_char(ch,"Active Zones\r\n-----------------------\r\n");
      for (i = 0; i <= top_of_zone_table; i++)
         if (!ZONE_FLAGGED(i, Z_IDLE))
            print_idle_to_buf(buf, i);
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_BUFFER:
      show_buffers(ch,-1,1);
      show_buffers(ch,-1,2);
      show_buffers(ch,-1,0);
      break;
   case SHOW_LOGBUF:
      show_buffers(ch,-1,0);
      show_buffers(0,-1,0);
      break;
   case SHOW_MOBSPECS:
      send_to_char(ch,"MobSpecs\r\n-----------------------\r\n");
      list_mob_spec_procs(ch,value);
      break;
   case SHOW_HOMETOWNS:
      send_to_char(ch,"HomeTowns\r\n-----------------------\r\n");
      send_to_char(ch,"  Mob#  Mult  Room# Zone# Zone Name                 Room Name\r\n");
      for (i = 0; i <= top_of_mobt; i++)
         {
         if(mob_index[i].func==home_keeper)
            {
            zone=real_zone(mob_proto[i].mob_specials.special_value[1]);
            room=real_room(mob_proto[i].mob_specials.special_value[1]);
            if ((zone<0)||(room<0))
               continue;
            sprintf(buf+strlen(buf),
                    "%6ld %5ld %6ld %5ld %-19.19s %-30.30s\r\n",
                    mob_index[i].vnum,
                    mob_proto[i].mob_specials.special_value[2],
                    mob_proto[i].mob_specials.special_value[1],
                    zone_table[zone].number,
                    zone_table[zone].name,
                    world[room].name
                   );
            }
         }
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
      /* olczones */
   case SHOW_OLCZONES:
      if (!*value)
         strcpy(value, GET_NAME(ch));
      if (load_char(value, &vbuf) < 0)
         {
         send_to_char(ch,"There is definitely no such player.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(birth);
         release_buffer(value);
         return;
         }
      CREATE(victim, struct char_data, 1);
      clear_char(victim);

      store_to_char(&vbuf,victim);

      send_to_char(ch, "%s has permission to edit these zones.\r\n",
                   GET_NAME(victim));

      for (i = 0; i < MAX_OLC_ZONES;i++)
         {
         if((zone=real_zone(GET_OLC_ZONE(victim,i)*100))==-1)
            {
            zone=0;
            GET_OLC_ZONE(victim,i)=0;
            }
         send_to_char(ch, "OLC %ld : %d, %s.\r\n", i+1,
                      GET_OLC_ZONE(victim, i),
                      zone_table[zone].name);
         }
      free_char(victim);
      break;
      /* Kill list */
   case SHOW_KILLS:
      l = FALSE;
      if (!(victim = get_player_vis(ch, value, FIND_CHAR_WORLD)))
         {
         l = TRUE;
         if (load_char(value, &vbuf) < 0)
            {
            send_to_char(ch,"There is definitely no such player.\r\n");
            release_buffer(buf);
            release_buffer(field);
            release_buffer(birth);
            release_buffer(value);
            return;
            }
         CREATE(victim, struct char_data, 1);
         clear_char(victim);

         store_to_char(&vbuf, victim);
         }
      buf[0] = '\0';

      sprintf(buf, "|#  ) VNUM [ Name          ](Lvl)Count||#  ) VNUM [ Name          ](Lvl)Count|\r\n");
      sprintf(buf + strlen(buf), "------------------------------------------------------------------------------\r\n");

      /* Lowered kill buffer to 64 from 125 - Nomikos 5/8/2025 */
      for(i = 0; i < 64; i++)
         {

         rmob_num = real_mobile(GET_KILLS_VNUM(victim, i));
         sprintf(buf + strlen(buf), "|%-3ld) %-5ld[%-15.15s](%-3d) %-3d |",
                 i + 1,
                 GET_KILLS_VNUM(victim, i),
                 (GET_KILLS_VNUM(victim, i) != 0) ? ((rmob_num == -1) ? "ERROR" : GET_NAME(mob_proto + rmob_num)) : "None",
                 (GET_KILLS_VNUM(victim, i) != 0) ? ((rmob_num == -1) ? "ERR" : GET_LEVEL(mob_proto + rmob_num)) : 0,
                 GET_KILLS_AMMOUNT(victim, i)
                 );

         if(i % 2)
            strcat(buf, "\r\n");
         }

      if(ch->desc)
         page_string(ch->desc, buf, TRUE, "");

      if (l)
         free_char(victim);
      break;

      /* list newbie flagged eq */
   case SHOW_NEWBIEEQ:
      strcpy(buf, "Newbie Equipment\r\n-------------------------");
      for(i=0,j=1; i<top_of_objt;i++)
         {
         if(obj_proto[i].obj_flags.extra_flags & ITEM_NEWBIE)
            {
            if(j%2)
               strcat(buf,"\r\n");
            sprintf(buf+strlen(buf),"|%3ld) [%5ld]%-24.24s |",j,
                    obj_index[i].vnum,obj_proto[i].short_description);
            j++;
            }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;

      /* list all people fighting */
   case SHOW_FIGHTING:
      strcpy(buf, "Combatants \r\n--------------------------------\r\n");
      for(mob=character_list;mob;mob=mob->next)
         {
         if(FIGHTING(mob))
            sprintf(buf+strlen(buf),
                    "%-20.20s is fighting %-20.20s at [%5ld] %d/%d\r\n",
                    GET_NAME(mob),GET_NAME(FIGHTING(mob)),
                    (IN_ROOM(mob)>=0)?GET_ROOM_VNUM(IN_ROOM(mob)):-1,
                    GET_HIT(mob),GET_MAX_HIT(mob));
         }
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_TWOHANDED:
      strcpy(buf, "2-Hand Equipment\r\n-------------------------");
      for(i=0,j=1; i<top_of_objt;i++)
         {
         if(obj_proto[i].obj_flags.extra_flags & ITEM_TWO_HAND)
            {
            if(j%2)
               strcat(buf,"\r\n");
            sprintf(buf+strlen(buf),"|%3ld) [%5ld]%-24.24s |",j,
                    obj_index[i].vnum,obj_proto[i].short_description);
            j++;
            }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_NOPOSCHK:
      strcpy(buf, "NoPosChk Equipment\r\n-------------------------");
      for(i=0,j=1; i<top_of_objt;i++)
         {
         if(obj_proto[i].obj_flags.extra_flags & ITEM_NO_POS_CHK)
            {
            if(j%2)
               strcat(buf,"\r\n");
            sprintf(buf+strlen(buf),"|%3ld) [%5ld]%-24.24s |",j,
                    obj_index[i].vnum,obj_proto[i].short_description);
            j++;
            }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_GUILDMASTER:
      show_gm(ch, value);
      break;
   case SHOW_CONNECTIONS:
      show_connections(ch,value);
      break;
   case SHOW_RCONNECT:
      show_rconnect(ch,value);
      break;
   case SHOW_TELEPORT:
      strcpy(buf, "Teleport Rooms\r\n--------------------------\r\n");
      if (*value && is_number(value))
         {
         count=atoi(value);
         }
      else
         {
         count=0;
         }
      for (i = 0, j = 0; i < top_of_world; i++)
         {
         if (world[i].tele!=NULL)
            {
            k=real_room(world[i].tele->targ);
            j++;
            if(j>=count)
               sprintf(buf+strlen(buf),
                       "%2ld : [%5ld] %-23.23s to [%5ld] %-23.23s (%d sec)\r\n",
                       j, GET_ROOM_VNUM(i), world[i].name,
                       world[i].tele->targ,world[k].name,
                       world[i].tele->time)
               ;
            if(strlen(buf)>32400)
               {
               sprintf(buf+strlen(buf), "Buffer Size Exceeded.  Use show teleport <number> (%ld)to view more.\r\n",j);
               break;
               }
            }
         }
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;

   case SHOW_MAZE:
      if(!*value||!is_number(value))
         {
         send_to_char(ch,"You need to enter a zone number.\r\n");
         break;
         }
      zone=atoi(value);
      for(j=0;(zone_table[j].number!=zone)&&(j<=top_of_zone_table);j++)
         ;
      if(j>top_of_zone_table)
         {
         send_to_char(ch,"Invalid Zone.\r\n");
         break;
         }
      zone *=100;
      room=real_room(zone);
      if((room==-1)||(world[room+99].number!=(zone+99)))
         {
         send_to_char(ch,"That zone does not have a full 100 rooms.\r\n");
         break;
         }

      for(i=0;i<100;i++)
         {
         room_exits[i]=0;
         for(k=0;k<NUM_OF_DIRS;k++)
            {
            if(world[room+i].dir_option[k]&&
                    world[room+i].dir_option[k]->to_room!=-1)
               room_exits[i]|=(1<<k);
            }
         }


      send_to_char(ch,
                   "Maze Map - if your zone isn't a 10x10 cube this won't work\r\n"
                   "----------------------------------------------------------\r\n");
      for(y=0;y<10;y++)
         {
         pos=y*10;

         send_to_char(ch,
                      "  %c%c %c%c %c%c %c%c %c%c %c%c %c%c %c%c %c%c %c%c\r\n",
                      room_exits[0+pos]&DIR_NOR?'|':' ',
                      room_exits[0+pos]&DIR_UP ?'/':' ',
                      room_exits[1+pos]&DIR_NOR?'|':' ',
                      room_exits[1+pos]&DIR_UP ?'/':' ',
                      room_exits[2+pos]&DIR_NOR?'|':' ',
                      room_exits[2+pos]&DIR_UP ?'/':' ',
                      room_exits[3+pos]&DIR_NOR?'|':' ',
                      room_exits[3+pos]&DIR_UP ?'/':' ',
                      room_exits[4+pos]&DIR_NOR?'|':' ',
                      room_exits[4+pos]&DIR_UP ?'/':' ',
                      room_exits[5+pos]&DIR_NOR?'|':' ',
                      room_exits[5+pos]&DIR_UP ?'/':' ',
                      room_exits[6+pos]&DIR_NOR?'|':' ',
                      room_exits[6+pos]&DIR_UP ?'/':' ',
                      room_exits[7+pos]&DIR_NOR?'|':' ',
                      room_exits[7+pos]&DIR_UP ?'/':' ',
                      room_exits[8+pos]&DIR_NOR?'|':' ',
                      room_exits[8+pos]&DIR_UP ?'/':' ',
                      room_exits[9+pos]&DIR_NOR?'|':' ',
                      room_exits[9+pos]&DIR_UP ?'/':' ');
         send_to_char(ch,
                      " %cO%c%cO%c%cO%c%cO%c%cO%c%cO%c%cO%c%cO%c%cO%c%cO%c\r\n",
                      room_exits[0+pos]&DIR_WES?'-':' ',
                      room_exits[0+pos]&DIR_EAS?'-':' ',
                      room_exits[1+pos]&DIR_WES?'-':' ',
                      room_exits[1+pos]&DIR_EAS?'-':' ',
                      room_exits[2+pos]&DIR_WES?'-':' ',
                      room_exits[2+pos]&DIR_EAS?'-':' ',
                      room_exits[3+pos]&DIR_WES?'-':' ',
                      room_exits[3+pos]&DIR_EAS?'-':' ',
                      room_exits[4+pos]&DIR_WES?'-':' ',
                      room_exits[4+pos]&DIR_EAS?'-':' ',
                      room_exits[5+pos]&DIR_WES?'-':' ',
                      room_exits[5+pos]&DIR_EAS?'-':' ',
                      room_exits[6+pos]&DIR_WES?'-':' ',
                      room_exits[6+pos]&DIR_EAS?'-':' ',
                      room_exits[7+pos]&DIR_WES?'-':' ',
                      room_exits[7+pos]&DIR_EAS?'-':' ',
                      room_exits[8+pos]&DIR_WES?'-':' ',
                      room_exits[8+pos]&DIR_EAS?'-':' ',
                      room_exits[9+pos]&DIR_WES?'-':' ',
                      room_exits[9+pos]&DIR_EAS?'-':' ');
         send_to_char(ch,
                      " %c%c %c%c %c%c %c%c %c%c %c%c %c%c %c%c %c%c %c%c\r\n",
                      room_exits[0+pos]&DIR_DOW?'/':' ',
                      room_exits[0+pos]&DIR_SOU?'|':' ',
                      room_exits[1+pos]&DIR_DOW?'/':' ',
                      room_exits[1+pos]&DIR_SOU?'|':' ',
                      room_exits[2+pos]&DIR_DOW?'/':' ',
                      room_exits[2+pos]&DIR_SOU?'|':' ',
                      room_exits[3+pos]&DIR_DOW?'/':' ',
                      room_exits[3+pos]&DIR_SOU?'|':' ',
                      room_exits[4+pos]&DIR_DOW?'/':' ',
                      room_exits[4+pos]&DIR_SOU?'|':' ',
                      room_exits[5+pos]&DIR_DOW?'/':' ',
                      room_exits[5+pos]&DIR_SOU?'|':' ',
                      room_exits[6+pos]&DIR_DOW?'/':' ',
                      room_exits[6+pos]&DIR_SOU?'|':' ',
                      room_exits[7+pos]&DIR_DOW?'/':' ',
                      room_exits[7+pos]&DIR_SOU?'|':' ',
                      room_exits[8+pos]&DIR_DOW?'/':' ',
                      room_exits[8+pos]&DIR_SOU?'|':' ',
                      room_exits[9+pos]&DIR_DOW?'/':' ',
                      room_exits[9+pos]&DIR_SOU?'|':' ');

         }
      break;

   case SHOW_GODCOMMAND:
      strcpy(buf, "God Commands\r\n--------------------------\r\n");
      for (j=0,i = 1;*cmd_info[i].command!='\n'; i++)
         {
         if((cmd_info[i].minimum_level>=LVL_IMMORT) &&
            (cmd_info[i].minimum_level<=GET_LEVEL(ch)))
            {
            sprintf(buf+strlen(buf), "|%-14.14s %3d|%s",
                    cmd_info[i].command,
                    cmd_info[i].minimum_level,
                    (!(++j%4)?"\r\n":""));
            }
         }
      strcat(buf, "\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_BADITEMSPLLVL:
      show_bad_spell_lvl(ch,value);
      break;
   case SHOW_ITEMSPELL:
      show_item_spell(ch,argument);
      break;
   case SHOW_DONATION:
      send_to_char(ch,"The following vnums are donation rooms: \r\n");
      for(j=0;donation_rooms[j]>0;j++)
         {
         room=real_room(donation_rooms[j]);
         i=0;
         if(room!=NOWHERE)
            {
            Crash_count_items(world[room].contents,&i);
            }
         send_to_char(ch, "|%8ld:%4ld items|%s",donation_rooms[j],i,
                      (j%2)?"\r\n":"");
         }
      if(j%2)
         send_to_char(ch,"\r\n");
      break;
   case SHOW_RACE:
      show_race(ch,value);
      break;
   case SHOW_SNOOP:
      send_to_char(ch,"People currently snooping:\r\n");
      send_to_char(ch,"--------------------------\r\n");
      for (d = descriptor_list; d; d = d->next)
         {
         if (d->snooping == NULL || d->character == NULL)
            continue;
         if(STATE(d)!=CON_PLAYING||GET_LEVEL(ch) < GET_LEVEL(d->character))
            continue;
         if (!CAN_SEE(ch,d->character) || IN_ROOM(d->character) == NOWHERE)
            continue;
         send_to_char(ch, "%-10s - snooped by %s.\r\n",
                      GET_NAME(d->snooping->character), GET_NAME(d->character));
         }
      break; /* snoop */
   case SHOW_SWITCH:
      send_to_char(ch,"People currently switched:\r\n");
      send_to_char(ch,"--------------------------\r\n");
      for (d = descriptor_list; d; d = d->next)
         {
         if (d->original == NULL && d->character != NULL)
            continue;
         if(STATE(d)!=CON_PLAYING||GET_LEVEL(ch) < GET_LEVEL(d->original))
            continue;
         if (!CAN_SEE(ch,d->character) || IN_ROOM(d->character) == NOWHERE)
            continue;
         send_to_char(ch, "%-15s - switched into %s.\r\n",
                      GET_NAME(d->original), GET_NAME(d->character));
         }
      break; /* snoop */
   case SHOW_WEAPONSPELL:
      strcpy(buf, "Weapon Spells:\r\n"
                  "--------------\r\n");
      for(i=0; i<top_of_objt;i++)
         {
         for(t=0;t<MAX_SPELL_AFFECT;t++)
            if(obj_proto[i].spell_affect[t].spelltype!=0)
               {
               sprintf(buf+strlen(buf),"(%5ld) %-20.20s| Spl: %s%s%s Lvl: %s%d%s Per: %s%d%%%s\r\n",
                            obj_index[i].vnum ,obj_proto[i].short_description,
                            NCYN,spells[obj_proto[i].spell_affect[t].spelltype].spell_name,NNRM,
                            NRED,obj_proto[i].spell_affect[t].level,NNRM,
                            NCYN,obj_proto[i].spell_affect[t].percentage,NNRM);
               }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_SPELLAFFECTS:
      strcpy(buf, "Object Spell Affects:\r\n"
                  "---------------------\r\n");
      for(i=0; i<top_of_objt;i++)
         {
         if(obj_proto[i].obj_flags.bitvector)
            {
            sprintf(buf+strlen(buf),"(%5ld) %-30.30s :",
                    obj_index[i].vnum ,obj_proto[i].short_description);
            sprintbit(obj_proto[i].obj_flags.bitvector, affected_bits, buf+strlen(buf));
            strcat(buf,"\r\n");
            }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_OLD_REMORTEQ:
      strcpy(buf, "Remort Equipment(old method):\r\n"
                  "-----------------");
      for(i=0,j=1; i<top_of_objt;i++)
         {
         if((obj_proto[i].obj_flags.value[4]==LVL_ANGEL) &&
            !(IS_SET(obj_proto[i].obj_flags.extra_flags2,ITEM2_DBLREMORT) ||
              IS_SET(obj_proto[i].obj_flags.extra_flags2,ITEM2_REMORT)))
            {
            if(j%2)
               strcat(buf,"\r\n");
            sprintf(buf+strlen(buf),"(%5ld) %-30.30s, ",
                    obj_index[i].vnum ,obj_proto[i].short_description);
            j++;
            }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_REMORTEQ:
      strcpy(buf, "Remort Equipment:\r\n"
                  "-----------------");
      for(i=0,j=1; i<top_of_objt;i++)
         {
         if(IS_SET(obj_proto[i].obj_flags.extra_flags2, ITEM2_REMORT))
            {
            if(j%2)
               strcat(buf,"\r\n");
            sprintf(buf+strlen(buf),"(%5ld) %-30.30s, ",
                    obj_index[i].vnum ,obj_proto[i].short_description);
            j++;
            }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_OLD_DBLREMORTEQ:
      strcpy(buf, "Double Remort Equipment(old method):\r\n"
                  "------------------------");
      for(i=0,j=1; i<top_of_objt;i++)
         {
         if((obj_proto[i].obj_flags.value[4]==LVL_AVATAR) &&
            !(IS_SET(obj_proto[i].obj_flags.extra_flags2,ITEM2_DBLREMORT) ||
              IS_SET(obj_proto[i].obj_flags.extra_flags2,ITEM2_REMORT)))
            {
            if(j%2)
               strcat(buf,"\r\n");
            sprintf(buf+strlen(buf),"(%5ld) %-30.30s ",
                    obj_index[i].vnum, obj_proto[i].short_description);
            j++;
            }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_DBLREMORTEQ:
      strcpy(buf, "Double Remort Equipment:\r\n"
                  "------------------------");
      for(i=0,j=1; i<top_of_objt;i++)
         {
         if(IS_SET(obj_proto[i].obj_flags.extra_flags2, ITEM2_DBLREMORT))
            {
            if(j%2)
               strcat(buf,"\r\n");
            sprintf(buf+strlen(buf),"(%5ld) %-30.30s ",
                    obj_index[i].vnum, obj_proto[i].short_description);
            j++;
            }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_GOD_EQ:
      strcpy(buf, "God Only Equipment:\r\n"
                  "------------------------");
      for(i=0,j=1; i<top_of_objt;i++)
         {
         if(obj_proto[i].obj_flags.value[4]>=LVL_IMMORT)
            {
            if(j%2)
               strcat(buf,"\r\n");
            sprintf(buf+strlen(buf),"(%5ld) %-30.30s ",
                    obj_index[i].vnum, obj_proto[i].short_description);
            j++;
            }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_OBJRESIST:
      strcpy(buf, "Resist Equipment:\r\n"
                  "-----------------");
      for(i=0; i<top_of_objt;i++)
         {
         for (count=0; count<MAX_OBJ_AFFECT; count++)
            {
            if(obj_proto[i].affected[count].location==APPLY_RESIST)
               {
               sprintf(buf+strlen(buf),"(%s%5ld%s) %-30.30s| %sRESIST%s: ",
                       NCYN, obj_index[i].vnum, NNRM, obj_proto[i].short_description,
                       NYEL, NNRM);
               sprintbit(obj_proto[i].affected[count].modifier,immunity_names,buf+strlen(buf));
               strcat(buf,"\r\n");
               }
            else if(obj_proto[i].affected[count].location==APPLY_IMMUNE)
               {
               sprintf(buf+strlen(buf),"(%s%5ld%s) %-30.30s| %sIMMUNE%s: ",
                       NCYN, obj_index[i].vnum, NNRM, obj_proto[i].short_description,
                       NRED, NNRM);
               sprintbit(obj_proto[i].affected[count].modifier,immunity_names,buf+strlen(buf));
               strcat(buf,"\r\n");
               }
            else if(obj_proto[i].affected[count].location==APPLY_SUSC)
               {
               sprintf(buf+strlen(buf),"(%s%5ld%s) %-30.30s| SUSC:   ",
                       NCYN, obj_index[i].vnum, NNRM, obj_proto[i].short_description);
               sprintbit(obj_proto[i].affected[count].modifier,immunity_names,buf+strlen(buf));
               strcat(buf,"\r\n");
               }
            }
         }
      strcat(buf,"\r\n");
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");
      break;
   case SHOW_IGNORED:
      send_to_char(ch, "Players and who they are ignoring:\r\n");
      for (d = descriptor_list; d; d = d->next)
         {
         if (d->character && CAN_SEE(ch, d->character))
            {
            send_to_char(ch, "%s:    ", GET_NAME(d->character));
            for (i=0; i<5; i++)
               if (GET_IGNORED(d->character,i))
                  {
                  for (v = descriptor_list; v; v = v->next)
                     if (GET_IGNORED(d->character, i) == GET_IDNUM(v->character))
                        {
                        send_to_char(ch, "%s ", GET_NAME(v->character));
                        break;
                        }
                  }
            send_to_char(ch, "\r\n");
            }
         }
      break;
   case SHOW_OBJDATA:
      if (*value && is_number(value))
         {
         count=atoi(value);
         }
      else
         {
         count=0;
         }

      for (orn = count; orn <= top_of_objt; orn++)
         {
         if(strlen(buf)>32500)
            {
            sprintf(buf+strlen(buf),"Buffer limit exceeded, continue your search with %ld\r\n",
                    orn);
            orn=top_of_objt;
            }
         else
            {
            char *buf2=get_buffer(256);
            char *buf3=get_buffer(256);
            sprintf(buf+strlen(buf), "[%5ld] %s(%s),",
                    obj_index[orn].vnum,obj_proto[orn].short_description,
                    obj_proto[orn].name);
            for (i = 0; i < MAX_OBJ_AFFECT; i++)
               if (obj_proto[orn].affected[i].location)
                  {
                  if((obj_proto[orn].affected[i].location==APPLY_IMMUNE)||
                        (obj_proto[orn].affected[i].location==APPLY_RESIST)||
                        (obj_proto[orn].affected[i].location==APPLY_SUSC))
                     sprintbit(obj_proto[orn].affected[i].modifier, immunity_names,buf3);
                  else
                     sprintf(buf3, "%ld", obj_proto[orn].affected[i].modifier);
                  sprinttype(obj_proto[orn].affected[i].location, apply_types, buf2);
                  sprintf(buf+strlen(buf), "|%s:%s",buf2, buf3);
                  }
               else
                   strcpy(buf+strlen(buf), "|X:0");
            strcpy(buf+strlen(buf), "\r\n");
            release_buffer(buf3);
            release_buffer(buf2);
            }
         }
      if(ch->desc)
         page_string(ch->desc,buf,TRUE,"");

      break;
   case SHOW_ASSEMBLIES:
      assemblyListToChar(ch);
      break;
   case SHOW_QUESTS: {
     char *quest_name = NULL;
     victim = NULL;
     if (value) {
       victim = get_player_vis(ch, value, FIND_CHAR_WORLD);
       if (victim) {
     sprintf(buf, "Quests %s has completed:\r\n", GET_NAME(victim));
       }
     }
     if (!victim) {
       if (value && value[0]) {
     quest_name = value;
     sprintf(buf, "Quests matching your criteria:\r\n");
       } else {
     quest_name = NULL;
     sprintf(buf, "Usage: show quests <player-name|quest-substring>\r\n\r\nQuests:\r\n");
       }
     }
     char *buf2 = get_buffer(MAX_STRING_LENGTH);
     if (victim) {
       for (i = 0; i < num_dg_quests; i++) {
     for (j = 0; j < dg_quests[i].num_completed; j++) {
       if (dg_quests[i].completed_by[j] == victim->pfilepos) {
         sprintf(buf2, "%3d - %s\r\n", (int)i+1, dg_quests[i].quest_name);
         if (strlen(buf) + strlen(buf2) < 32700) {
           strcat(buf, buf2);
         }
         break;
       }
     }
       }
     } else if (quest_name) {
       for (i = 0; i < num_dg_quests; i++) {
     if (stristr(dg_quests[i].quest_name, quest_name)) {
       sprintf(buf2, "%3d - %s\r\n", (int)i+1, dg_quests[i].quest_name);
       if (strlen(buf) + strlen(buf2) < 32700) {
         strcat(buf, buf2);
       }
     }
       }
     } else {
       for (i = 0; i < num_dg_quests; i++) {
     sprintf(buf2, "%3d - %s\r\n", (int)i+1, dg_quests[i].quest_name);
     if (strlen(buf) + strlen(buf2) < 32700) {
       strcat(buf, buf2);
     }
       }
     }
     send_to_char(ch, "%s", buf);
     release_buffer(buf2);
     break;
       /*
       if (quest_name && strstr(dg_quests[i].quest_name, quest_name)) {
     sprintf(buf2, "(%d) %s :", i+1, dg_quests[i].quest_name);
     if (strlen(buf) + strlen(buf2) < 32700) {
       strcat(buf, buf2);
     }
       }
       for (j = 0; j < dg_quests[i].num_completed; j++) {
     sprintf(buf2, " %s", (player_table + dg_quests[i].completed_by[j])->name);
     if (strlen(buf) + strlen(buf2) < 32700) {
       strcat(buf, buf2);
     }
       }
       if (strlen(buf) < 32700) {
     strcat(buf, "\r\n");
       }
       */
   }
   case SHOW_GRAFFITI: {
     buf[0] = '\x0';
     int *vnums = (int *)malloc(5000*sizeof(int));
     int nvnums = 0;
     char *buf2 = get_buffer(MAX_STRING_LENGTH);
     for (i = 0; i < num_graffiti; i++) {
       int found = 0;
       for (j = 0; j < nvnums; j++) {
     if (graffiti[i].room_vnum == vnums[j]) {
       found = 1;
       break;
     }
       }
       if (!found) {
     if (nvnums < 5000) {
       vnums[nvnums++] = graffiti[i].room_vnum;
     }
       }
     }
     sprintf(buf, "Rooms containing graffiti:\r\n");
     for (i = 0; i < nvnums; i++) {
       int rnum = real_room(vnums[i]);
       if (rnum >= 0) {
     sprintf(buf2, "  [%5d] %s\r\n", vnums[i], world[rnum].name);
     if (strlen(buf) + strlen(buf2) < 32700) {
       strcat(buf, buf2);
     }
       }
     }
     page_string(ch->desc, buf, TRUE, "");
     /* send_to_char(ch, buf); */
     release_buffer(buf2);
     break;
   }
   case SHOW_EXPLORED: {
     char *buf2 = get_buffer(MAX_STRING_LENGTH);
     strcpy(buf2, argument);
     strtok(buf2, " ");
     char *name = strtok(NULL, " ");
     char *zone_s = strtok(NULL, " ");
     if (!name) {
       send_to_char(ch, "Usage: show explored <player> [zone]\r\n");
       release_buffer(buf2);
       break;
     }
     if (!(victim = get_player_vis(ch, value, FIND_CHAR_WORLD))) {
       send_to_char(ch, "There is definitely no such player.\r\n");
       release_buffer(buf2);
       break;
     }
     zone = zone_s ? atoi(zone_s) : -1;
     if (zone < -1 || zone > top_of_zone_table+1) {
       send_to_char(ch, "That is not a valid zone.\r\n");
       release_buffer(buf2);
       break;
     }
     if (zone == -1) {
     /*  char *buf1=get_buffer(256); */
       int tzone;
       sprintf(buf, "Zones %s has explored:\r\n", GET_NAME(victim));
       for (i = 0; i <= MIN(EXPLORED_TOP_VNUM/100, top_of_zone_table+1); i++) {
     for (j = 0; j < 100; j++) {
       int vnum = 100*i + j;
       if (victim->player_specials->explored_vnums[vnum/8] & (1 << (vnum%8))) {
         for (tzone = 0; tzone <= top_of_zone_table && zone_table[tzone].number != i; tzone++);
         sprintf(buf2, "%3d - %s\r\n", (int)i, zone_table[tzone].name);
         if (strlen(buf) + strlen(buf2) < 32700) {
           strcat(buf, buf2);
         }
         break;
       }
     }
       }
     } else {
       int tzone;
       for (tzone = 0; tzone <= top_of_zone_table && zone_table[tzone].number != zone; tzone++);
       sprintf(buf, "Rooms %s has explored in zone #%d (%s):\r\n", GET_NAME(victim), zone, zone_table[tzone].name);
       for (i = zone*100; i < (zone+1)*100; i++) {
     if (victim->player_specials->explored_vnums[i/8] & (1 << i%8)) {
       sprintf(buf2, "%6d - %s\r\n", (int)i, world[real_room(i)].name);
       if (strlen(buf) + strlen(buf2) < 32700) {
         strcat(buf, buf2);
       }
     }
       }
     }
     /* send_to_char(ch, buf); */
     page_string(ch->desc, buf, TRUE, "");
     release_buffer(buf2);
     break;
   }
   case SHOW_EMAIL: {
     char *buf2 = get_buffer(MAX_STRING_LENGTH);
     sprintf(buf, "Players with e-mail addresses:\r\n");
     count = 0;
     for (i = 0; i < top_of_p_table; i++) {
       char *name = player_table[i].name;
       if (!name) {
     continue;
       }
       struct char_file_u cf;
       memset(&cf, 0, sizeof(struct char_file_u));
       load_char_ascii(&cf, name);
       if (cf.email && cf.email[0]) {
     if (name[0]) {
       name[0] = toupper(name[0]);
     }
     sprintf(buf2, "%3d. [%4ld] %-20s : %s\r\n", ++count, cf.char_specials_saved.idnum, name, cf.email);
     strcat(buf, buf2);
       }
     }
     release_buffer(buf2);
     page_string(ch->desc, buf, TRUE, "");
   }
   break;
   case SHOW_GREMORT_RECORDS:
   {
      char *buf2 = get_buffer(MAX_STRING_LENGTH);
      strcpy(buf2, argument);
      strtok(buf2, " ");
      buf[0] = '\x0';
      char *name = strtok(NULL, " ");
      if (!name)
      {
         send_to_char(ch, "Usage: show gremort_record [player_name]\r\n\r\n");
         release_buffer(buf2);
         break;
      }
      name = stolower(strdup(name));
      int counter = 0;
      for (i = 0; i < nExamRecords; i++) {
         char* cmpname = stolower(strdup(examRecords[i].player_name));
         int match = starts_with(cmpname, name);
         free(cmpname);

         if (!match) continue;

         snprintf(buf2, MAX_STRING_LENGTH, "%3d. ", ++counter);

         strftime(buf2 + strlen(buf2), MAX_STRING_LENGTH - strlen(buf2), "%a %b %d %Y %H:%M:%S ", localtime(&examRecords[i].date_taken));

         snprintf(buf2 + strlen(buf2),
            MAX_STRING_LENGTH - strlen(buf2),
            "%8s - %s : %s\r\n",
            gremort_exam_types[examRecords[i].exam_type],
            examRecords[i].player_name,
            gremort_exam_results[examRecords[i].result]
         );

         if (strlen(buf) + strlen(buf2) > 32750) {
            break;
         }

         strcat(buf, buf2);
      }
      free(name);
      page_string(ch->desc, buf, TRUE, "");
      release_buffer(buf2);
      break;
   }
   case SHOW_PLAYER_SHOPS: {
     char buf2[1024];
     buf[0] = '\x0';
     struct player_shop* shop = player_shops;
     sprintf(buf2, "%3s  %-15s   %-10s %10s   %s\r\n", " ", "Name", "Location", "Rent", "");
     strcat(buf, buf2);
     for (int ii = 1; shop; shop = shop->next, ii++) {
       sprintf(buf2, "%3d. %-15s %10d %10d %s  (%3d items)\r\n", ii, shop->player_name, shop->vnum_location, shop->rent, shop->is_active ? "Active" : "Not active", get_shop_item_count(shop));
       strcat(buf, buf2);
     }
     page_string(ch->desc, buf, TRUE, "");
     break;
   }
   default:   /**** END ****/
      send_to_char(ch,"Sorry, I don't understand that.\r\n");
      break;
      }
   release_buffer(buf);
   release_buffer(field);
   release_buffer(birth);
   release_buffer(value);
   }




#define PC   1
#define NPC  2
#define BOTH 3

const char *target_type[]={
                             "None",
                             "Players",
                             "Mobs",
                             "Both"
                          };

#define MISC   0
#define BINARY 1
#define NUMBER 2
#define DUAL_NUM 3

const char *arg_type[]={
                          "String",
                          "On/Off",
                          "Number",
                          "Two Numbers"
                       };

#define SET_OR_REMOVE(flagset, flags) { \
     if (on) SET_BIT(flagset, flags); \
     else if (off) REMOVE_BIT(flagset, flags);}


#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))
/*#define RANGE(low, high) value*/


ACMD(do_set)
   {
   long i,j, l;
   struct char_data *vict = NULL, *cbuf = NULL;
   struct char_file_u tmp_store;
   struct obj_data *obj;
   mob_rnum r_num;
   char *field=get_buffer(MAX_INPUT_LENGTH);
   char *name=get_buffer(MAX_INPUT_LENGTH);
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   char *output;
   char *val_arg;
   char *val_arg1;
   int on = 0, off = 0, value = 0,value1=0;
   char is_file = 0, is_player = 0;
   int player_i = 0;
   int password_change =0;
   zone_vnum zone_num = 0, zone_found = 0;
   int dot_mode=FIND_INDIV;

   struct set_struct
      {
      char *cmd;
      char level;
      char pcnpc;
      char type;
      }
   fields[] =
      {
         { "brief",       LVL_DGOD,      PC,        BINARY } ,  /* 0 */
         { "invstart",    LVL_DGOD,      PC,        BINARY } ,  /* 1 */
         { "title",       LVL_DGOD,      PC,        MISC } ,
         { "nosummon",    LVL_GRGOD,     PC,        BINARY } ,
         { "maxhit",      LVL_GRGOD,     BOTH,      NUMBER } ,
         { "maxmana",     LVL_GRGOD,     BOTH,      NUMBER } ,  /* 5 */
         { "maxmove",     LVL_GRGOD,     BOTH,      NUMBER } ,
         { "hit",         LVL_GRGOD,     BOTH,      NUMBER } ,
         { "mana",        LVL_GRGOD,     BOTH,      NUMBER } ,
         { "move",        LVL_GRGOD,     BOTH,      NUMBER } ,
         { "align",       LVL_DGOD,      BOTH,      NUMBER } ,  /* 10 */
         { "str",         LVL_GRGOD,     BOTH,      NUMBER } ,
         { "stradd",      LVL_GRGOD,     BOTH,      NUMBER } ,
         { "int",         LVL_GRGOD,     BOTH,      NUMBER } ,
         { "wis",         LVL_GRGOD,     BOTH,      NUMBER } ,
         { "dex",         LVL_GRGOD,     BOTH,      NUMBER } ,  /* 15 */
         { "con",         LVL_GRGOD,     BOTH,      NUMBER } ,
         { "sex",         LVL_GRGOD,     BOTH,      MISC } ,
         { "ac",          LVL_GRGOD,     BOTH,      NUMBER } ,
         { "gold",        LVL_DGOD,      BOTH,      NUMBER } ,
         { "bank",        LVL_DGOD,      PC,        NUMBER } ,  /* 20 */
         { "exp",         LVL_GRGOD,     BOTH,      NUMBER } ,
         { "hitroll",     LVL_GRGOD,     BOTH,      NUMBER } ,
         { "damroll",     LVL_GRGOD,     BOTH,      NUMBER } ,
         { "invis",       LVL_ADMIN,      PC,       NUMBER } ,
         { "nohassle",    LVL_GRGOD,     PC,        BINARY } ,  /* 25 */
         { "frozen",      LVL_FREEZE,    PC,        BINARY } ,
         { "lastlearn",   LVL_GRGOD,     PC,        NUMBER } ,
         { "lessons",     LVL_GRGOD,     PC,        NUMBER } ,
         { "drunk",       LVL_GRGOD,     BOTH,      MISC } ,
         { "hunger",      LVL_GRGOD,     BOTH,      MISC } ,    /* 30 */
         { "thirst",      LVL_GRGOD,     BOTH,      MISC } ,
         { "killer",      LVL_DGOD,      PC,        BINARY } ,
         { "thief",       LVL_DGOD,      PC,        BINARY } ,
         { "level",       LVL_ADMIN,     BOTH,      NUMBER } ,
         { "room",        LVL_SIMP,      BOTH,      NUMBER } ,  /* 35 */
         { "roomflag",    LVL_GRGOD,     PC,        BINARY } ,
         { "siteok",      LVL_GRGOD,     PC,        BINARY } ,
         { "deleted",     LVL_ADMIN,     PC,        BINARY } ,
         { "race",        LVL_GRGOD,     PC,        MISC } , /* 10/27/96, Echo */
         { "class",       LVL_GRGOD,     BOTH,      MISC }   , /* 40 */
         { "nowizlist",   LVL_DGOD,      PC,        BINARY } ,
         { "quest",       LVL_DGOD,      PC,        BINARY } ,
         { "loadroom",    LVL_GRGOD,     PC,        MISC }   ,
         { "color",       LVL_DGOD,      PC,        BINARY } ,
         { "idnum",       LVL_IMPL,      PC,        NUMBER } , /* 45 */
         { "passwd",      LVL_ADMIN,     PC,        MISC }   ,
         { "nodelete",    LVL_DGOD,      PC,        BINARY } ,
         { "cha",         LVL_GRGOD,     BOTH,      NUMBER } ,
         { "olc",         LVL_DGOD,      PC,        NUMBER } ,
         { "hometown",    LVL_DGOD,      PC,        NUMBER } , /* 50 */
         { "rezone",      LVL_DGOD,      PC,        DUAL_NUM}, /* 51 */
         { "mood",        LVL_DGOD,      NPC,       NUMBER } , /* 52 */
         { "mobgo",       LVL_IMMORT,    NPC,       NUMBER } , /* 53 */
         { "e_mprog",     LVL_IMMORT,    PC,        BINARY } , /* 54 */
         { "t_mprog",     LVL_ADMIN,     PC,        BINARY } , /* 55 */
         { "age",         LVL_GRGOD,     BOTH,      NUMBER } , /* 56 */
         { "dgattach",    LVL_DGODI,     PC,        BINARY } , /* 57 */
         { "name",        LVL_GRGOD,     PC,        MISC   } , /* 58 */
         { "hedit",       LVL_SIMP,      PC,        BINARY } , /* 59 */
         { "qps",         LVL_GOD,       PC,        NUMBER } , /* 60 */
         { "heartworn",   LVL_GRGOD,     PC,        NUMBER } , /* 61 */
         { "remortlev",   LVL_IMPL,      PC,        NUMBER } , /* 62 */
         { "holylight",   LVL_GOD,       PC,        BINARY } , /* 63 */
         { "pk",          LVL_ADMIN,     PC,        BINARY } , /* 64 */
         { "nocommune",   LVL_SERP,      PC,        BINARY } , /* 65 */
     { "explored",    LVL_IMPL,      PC,        NUMBER } , /* 66 */
         { "\n" ,         0,             BOTH,      MISC }
      } ;


   half_chop(argument, name, buf);
   if (!strcmp(name, "file"))
      {
      is_file = 1;
      half_chop(buf, name, buf);
      }
   else if (!str_cmp(name, "player"))
      {
      is_player = 1;
      half_chop(buf, name, buf);
      }
   else if (!str_cmp(name, "mob"))
      {
      half_chop(buf, name, buf);
      }

   half_chop(buf, field, buf);

   if (!*name || !*field)
      {
      output=get_buffer(MAX_STRING_LENGTH);
      sprintf(output,"Usage: set <victim> <field> <value>\r\n");
      sprintf(output,
              " Num  Name                 Target   Type               Lvl\r\n");
      j=1;
      for(l=0;*(fields[l].cmd)!='\n';l++)
         {
         if(GET_LEVEL(ch) >= fields[l].level)
            {
            sprintf(output+strlen(output),
                    " %3ld. %-20.20s %-8.8s %-12.12s %s\r\n",j,
                    fields[l].cmd,
                    target_type[(int)fields[l].pcnpc],
                    arg_type[(int)fields[l].type],
                    WizLevels[((int)fields[l].level)-LVL_IMMORT]);
            j++;
            }
         }
      release_buffer(buf);
      release_buffer(field);
      release_buffer(name);
      if(ch->desc)
         page_string(ch->desc,output,TRUE,"");
      release_buffer(output);
      return;
      }
   if (!is_file)
      {
      if (is_player)
         {
         if (!(vict = get_player_vis(ch, name, 0)))
            {
            send_to_char(ch,"There is no such player.\r\n");
            release_buffer(buf);
            release_buffer(field);
            release_buffer(name);
            return;
            }
         }
      else
         {
         dot_mode=find_all_dots(name);
         if(dot_mode==FIND_INDIV)
            if (!(vict = get_char_vis(ch, name,FIND_CHAR_WORLD)))
               {
               send_to_char(ch,"There is no such creature.\r\n");
               release_buffer(buf);
               release_buffer(field);
               release_buffer(name);
               return;
               }
         }
      }
   else if (is_file)
      {
      CREATE(cbuf, struct char_data, 1);
      clear_char(cbuf);
      if ((player_i = load_char(name, &tmp_store)) > -1)
         {
         store_to_char(&tmp_store, cbuf);
         cbuf->player.time.logon=tmp_store.last_logon;
         if (GET_LEVEL(cbuf) >= GET_LEVEL(ch))
            {
            free_char(cbuf);
            send_to_char(ch,"Sorry, you can't do that.\r\n");
            release_buffer(buf);
            release_buffer(field);
            release_buffer(name);
            return;
            }
         vict = cbuf;
         }
      else
         {
         free(cbuf);
         send_to_char(ch,"There is no such player.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(name);
         return;
         }
      }
   if (GET_LEVEL(ch) != LVL_IMPL)
      {
    /*if((GET_LEVEL(ch)==GET_LEVEL(vict)) && !str_cmp(GET_NAME(ch),"iluvatar"))
         {
         send_to_char(ch,"Iluvatar special power mode ON!\r\n");
         }
     else*/ if ((dot_mode==FIND_INDIV)&&!IS_NPC(vict) &&
               (GET_LEVEL(ch) <= GET_LEVEL(vict)) && (vict != ch))
         {
         send_to_char(ch,"Maybe that's not such a great idea...\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      }
   for (l = 0; *(fields[l].cmd) != '\n'; l++)
      if (!strncmp(field, fields[l].cmd, strlen(field)))
         break;


   if((dot_mode!=FIND_INDIV)&&(l!=52))
      {
      send_to_char(ch,"You cannont use all with this command\r\n");
      release_buffer(buf);
      release_buffer(field);
      release_buffer(name);
      return;
      }

   if (GET_LEVEL(ch) < fields[l].level)
      {
      send_to_char(ch,"You are not godly enough for that!\r\n");
      release_buffer(buf);
      release_buffer(field);
      release_buffer(name);
      if (is_file)
         free_char(vict);
      return;
      }
   if ((dot_mode==FIND_INDIV)&&IS_NPC(vict) && !(fields[l].pcnpc & NPC))
      {
      send_to_char(ch,"You can't do that to a beast!\r\n");
      release_buffer(buf);
      release_buffer(field);
      release_buffer(name);
      if (is_file)
         free_char(vict);
      return;
      }
   else if ((dot_mode==FIND_INDIV)&&!IS_NPC(vict) && !(fields[l].pcnpc & PC))
      {
      send_to_char(ch,"That can only be done to a beast!\r\n");
      release_buffer(buf);
      release_buffer(field);
      release_buffer(name);
      if (is_file)
         free_char(vict);
      return;
      }

   val_arg=get_buffer(MAX_INPUT_LENGTH);
   val_arg1=get_buffer(MAX_INPUT_LENGTH);

   if (fields[l].type == DUAL_NUM)
      {
      half_chop(buf, val_arg, val_arg1);
      value = atoi(val_arg);
      value1 = atoi(val_arg1);
      }
   else if (fields[l].type == BINARY)
      {
      strcpy(val_arg, buf);
      if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
         on = 1;
      else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
         off = 1;
      if (!(on || off))
         {
         send_to_char(ch,"Value must be on or off.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      }
   else if (fields[l].type == NUMBER)
      {
      strcpy(val_arg, buf);
      value = atoi(val_arg);
      }
   else
      strcpy(val_arg, buf);

   mudlogf(BRF,GOD_LOG(ch),TRUE,
           "(GC) %s typed 'set%s'",GET_NAME(ch),argument);
   strcpy(buf, "Okay.");  /* can't use OK macro here 'cause of \r\n */
   switch (l)
      {
   case 0:
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
      break;
   case 1:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
      break;
   case 2:
      set_title(vict, val_arg);
      sprintf(buf, "%s's title is now: %s", GET_NAME(vict), GET_TITLE(vict));
      break;
   case 3:
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
      break;
   case 4:
      vict->points.max_hit = RANGE(1, 5000);
      affect_total(vict);
      break;
   case 5:
      vict->points.max_mana = RANGE(1, 5000);
      affect_total(vict);
      break;
   case 6:
      vict->points.max_move = RANGE(1, 5000);
      affect_total(vict);
      break;
   case 7:
      vict->points.hit = RANGE(-9, vict->points.max_hit);
      affect_total(vict);
      break;
   case 8:
      vict->points.mana = RANGE(0, vict->points.max_mana);
      affect_total(vict);
      break;
   case 9:
      vict->points.move = RANGE(0, vict->points.max_move);
      affect_total(vict);
      break;
   case 10:
      GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
      affect_total(vict);
      break;
   case 11:
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
         RANGE(3, 125);
      else
         RANGE(3, race_max_stats[GET_RACE(vict)][0]);
      vict->real_abils.str = value;
      vict->real_abils.str_add = 0;
      affect_total(vict);
      break;
   case 12:
      vict->real_abils.str_add = RANGE(0, 100);
      if (value > 0)
         vict->real_abils.str = 18;
      affect_total(vict);
      break;
   case 13:
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
         RANGE(3, 125);
      else
         RANGE(3,race_max_stats[GET_RACE(vict)][1]);
      vict->real_abils.intel = value;
      affect_total(vict);
      break;
   case 14:
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
         RANGE(3, 125);
      else
         RANGE(3, race_max_stats[GET_RACE(vict)][2]);
      vict->real_abils.wis = value;
      affect_total(vict);
      break;
   case 15:
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
         RANGE(3, 125);
      else
    RANGE(3, race_max_stats[GET_RACE(vict)][3]);
      vict->real_abils.dex = value;
      affect_total(vict);
      break;
   case 16:
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
         RANGE(3, 125);
      else
         RANGE(3, race_max_stats[GET_RACE(vict)][4]);
      vict->real_abils.con = value;
      affect_total(vict);
      break;
   case 17:
      if (!str_cmp(val_arg, "male"))
         vict->player.sex = SEX_MALE;
      else if (!str_cmp(val_arg, "female"))
         vict->player.sex = SEX_FEMALE;
      else if (!str_cmp(val_arg, "neutral"))
         vict->player.sex = SEX_NEUTRAL;
      else
         {
         send_to_char(ch,"Must be 'male', 'female', or 'neutral'.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      break;
   case 18:
      vict->points.armor = RANGE(-200, 200);
      affect_total(vict);
      break;
   case 19:
      GET_GOLD(vict) = RANGE(0, 100000000);
      break;
   case 20:
      GET_BANK_GOLD(vict) = RANGE(0, 100000000);
      break;
   case 21:
      vict->points.exp = RANGE(0, 200000000);
      break;
   case 22:
      vict->points.hitroll = RANGE(-100, 100);
      affect_total(vict);
      break;
   case 23:
      vict->points.damroll = RANGE(-100, 100);
      affect_total(vict);
      break;
   case 24:
      if (GET_LEVEL(ch) < LVL_IMPL && ch != vict)
         {
         send_to_char(ch,"You aren't godly enough for that!\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      //GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
      GET_INVIS_LEV(vict) = RANGE(0, LVL_IMPL);
      break;
   case 25:
      if (GET_LEVEL(ch) < LVL_IMPL && ch != vict)
         {
         send_to_char(ch,"You aren't godly enough for that!\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
      break;
   case 26:
      if (ch == vict)
         {
         send_to_char(ch,"Better not -- could be a long winter!\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
      break;
   case 27:
   case 28:
      GET_LAST_LEARN(vict) = RANGE(0, MAX_SKILLS);
      GET_LEARN_TIC(vict)=0;
      break;
   case 29:
   case 30:
   case 31:
      if (!str_cmp(val_arg, "off"))
         {
         GET_COND(vict, (l - 29)) = (char) -1;
         sprintf(buf, "%s's %s now off.", GET_NAME(vict), fields[l].cmd);
         }
      else if (is_number(val_arg))
         {
         value = atoi(val_arg);
         RANGE(0, 24);
         GET_COND(vict, (l - 29)) = (char) value;
         sprintf(buf, "%s's %s set to %d.", GET_NAME(vict), fields[l].cmd,
                 value);
         }
      else
         {
         send_to_char(ch,"Must be 'off' or a value from 0 to 24.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      break;
   case 32:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
      break;
   case 33:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
      break;
   case 34:
      if (value > GET_LEVEL(ch) || value > LVL_IMPL)
         {
         send_to_char(ch,"You can't do that.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      RANGE(0, LVL_IMPL);
      vict->player.level = (byte) value;
      break;
   case 35:
      if ((i = real_room(value)) < 0)
         {
         send_to_char(ch,"No room exists with that number.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      if(IN_ROOM(vict)!=NOWHERE)
         char_from_room(vict);
      char_to_room(vict, i);
      break;
   case 36:
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
      break;
   case 37:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
      break;
   case 38:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
      if (on)
         REMOVE_BIT(PLR_FLAGS(vict), PLR_NODELETE);
      if((i=find_name(GET_PC_NAME(vict)))==-1)
         {
         send_to_char(ch, "ERROR: there is no player %s.\r\n",
                      GET_PC_NAME(vict));
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         return;
         break;
         }
      player_table[i].plr_flags=PLR_FLAGS(vict);
      Crash_delete_file(GET_NAME(vict));
      break;
   case 39:  /* 10/27/96, Echo - race added. Others below bumped down one. */
      if ((i = parse_race(val_arg)) == CLASS_UNDEFINED)
         {
         send_to_char(ch,"That is not a race.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }

      if (!IS_NPC(vict) && IS_DBLREMORT_OLD(vict) && (i < RACE_DRACONIAN))
         {
         if (IS_REMORT_OLD(vict))
            REMORT_LEVEL(vict) = SINGLE_REMORT;
         else
            REMORT_LEVEL(vict) = NON_REMORT;
         }

      GET_RACE(vict) = i;

      if (!IS_NPC(vict) && IS_DBLREMORT_OLD(vict))
         REMORT_LEVEL(vict) = DOUBLE_REMORT;

      if (GET_SEX(vict) == SEX_MALE) /* racial stats added - 10-5-02 Nomi*/
         {
         GET_WEIGHT(vict) = MIN(number(race_size_info[i].MWmin,
                                       race_size_info[i].MWmax), 255);
         GET_HEIGHT(vict) = MIN(number(race_size_info[i].MHmin,
                                       race_size_info[i].MHmax), 255);
         }
      else
         {
         GET_WEIGHT(vict) = MIN(number(race_size_info[i].FWmin,
                                       race_size_info[i].FWmax), 255);
         GET_HEIGHT(vict) = MIN(number(race_size_info[i].FHmin,
                                       race_size_info[i].FHmax), 255);
         }
      break;
   case 40:
      if ((i = parse_class(val_arg)) == CLASS_UNDEFINED)
         {
         send_to_char(ch,"That is not a class.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }

      if (!IS_NPC(vict) && !IS_DBLREMORT_OLD(vict) && (i < CLASS_KENSAI))
         REMORT_LEVEL(vict) = NON_REMORT;

      GET_CLASS(vict) = i;

      if (!IS_NPC(vict) && IS_REMORT_OLD(vict))
         {
         if (IS_DBLREMORT_OLD(vict))
            REMORT_LEVEL(vict) = DOUBLE_REMORT;
         else
            REMORT_LEVEL(vict) = SINGLE_REMORT;
         }
      break;
   case 41:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
      break;
   case 42:
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
      break;
   case 43:
      if (!str_cmp(val_arg, "off"))
         REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
      else if (is_number(val_arg))
         {
         value = atoi(val_arg);
         if (real_room(value) != NOWHERE)
            {
            SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
            GET_LOADROOM(vict) = value;
            sprintf(buf, "%s will enter at room #%ld.", GET_NAME(vict),
                    GET_LOADROOM(vict));
            }
         else
            {
            sprintf(buf, "That room does not exist!");
            }
         }
      else
         {
         strcpy(buf, "Must be 'off' or a room's virtual number.\r\n");
         }
      break;
   case 44:
      SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
      break;
   case 45:
     if (GET_IDNUM(ch) != 1 || !IS_NPC(vict))
         {
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      GET_IDNUM(vict) = value;
      break;
   case 46:
      if (!is_file)
         {
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      /*   if (GET_IDNUM(ch) > 1)
            {
            send_to_char(ch,"Please don't use this command, yet.\r\n");
            release_buffer(buf);
            release_buffer(field);
            release_buffer(val_arg1);
            release_buffer(val_arg);
            release_buffer(name);
            if (is_file)
        free_char(vict);
            return;
            } */
      if (GET_LEVEL(vict) >= LVL_GRGOD)
         {
         send_to_char(ch,"You cannot change that.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      strncpy(tmp_store.pwd, CRYPT(val_arg,tmp_store.name),MAX_PWD_LENGTH);
      tmp_store.pwd[MAX_PWD_LENGTH] = '\0';
      sprintf(buf, "Password changed to '%s'.", val_arg);
      password_change=1;
      break;
   case 47:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
      break;
   case 48:
      if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
         RANGE(3, 125);
      else
         RANGE(3, race_max_stats[GET_RACE(vict)][5]);
      vict->real_abils.cha = value;
      affect_total(vict);
      break;
   case 49:
      if (value < 1)
         {
         send_to_char(ch,"Daft, try with a real number my friend!");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      for(i=0;i<=top_of_zone_table;i++)
         {
         if(zone_table[i].number==value)
            break;
         else if(zone_table[i].number>value)
            {
            send_to_char(ch, "Zone %d doesn't exist.\r\n", value);
            release_buffer(buf);
            release_buffer(field);
            release_buffer(val_arg1);
            release_buffer(val_arg);
            release_buffer(name);
            if (is_file)
               free_char(vict);
            return;
            }
         }
      if((GET_LEVEL(ch)<LVL_ADMIN)&&(zone_table[i].status>=4))
         {
         send_to_char(ch,"You do not have permission to set olc to an active or finished zone.  Please contact a ADMIN+.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }



      for (zone_num = 0; zone_num < MAX_OLC_ZONES;zone_num++)
         if (GET_OLC_ZONE(vict, zone_num) == value)
            zone_found = 1;
      if (zone_found == 1) /* We found the bugger */
         {
         send_to_char(ch, "%s has allready that zone set.\r\n",
                      GET_NAME(vict));
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      else  /* Okay, we didn't find the zone anyway */
         {
         GET_OLC_ZONE(vict, 4) = GET_OLC_ZONE(vict, 3);
         GET_OLC_ZONE(vict, 3) = GET_OLC_ZONE(vict, 2);
         GET_OLC_ZONE(vict, 2) = GET_OLC_ZONE(vict, 1);
         GET_OLC_ZONE(vict, 1) = GET_OLC_ZONE(vict, 0);
         GET_OLC_ZONE(vict, 0) = value;
         }

      break;
   case 50:
      GET_HOME(vict)=value;
      break;
   case 51:
      zone_found = 0;
      /* First some sanity checks! Not really needed, but nice to have! */
      if (value < 0 || value1 < 0 || value == value1)
         {
         send_to_char(ch,"Usage: set <victim> rezone <oldzone> <newzone>\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }
      for(i=0;i<=top_of_zone_table;i++)
         {
         if(zone_table[i].number==value1)
            break;
         else if(zone_table[i].number>value1)
            {
            send_to_char(ch, "Zone %d doesn't exist.\r\n", value1);
            release_buffer(buf);
            release_buffer(field);
            release_buffer(val_arg1);
            release_buffer(val_arg);
            release_buffer(name);
            if (is_file)
               free_char(vict);
            return;
            }
         }
      if((GET_LEVEL(ch)<LVL_ADMIN)&&(zone_table[i].status>=4))
         {
         send_to_char(ch,"You do not have permission to set olc to an active or finished zone.  Please contact a ADMIN+.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         }

      for (zone_num = 0;zone_num < MAX_OLC_ZONES;zone_num++)
         {
         if (GET_OLC_ZONE(vict, zone_num) == value)
            {
            GET_OLC_ZONE(vict, zone_num) = value1;
            zone_found = 1;
            break;
            }
         }
      if (zone_found != 1)
         {
         send_to_char(ch, "%s does not have zone %d\r\n",GET_NAME(vict),
                      value);
         }
      break;
   case 52:
      if(dot_mode==FIND_INDIV)
         GET_MOOD(vict)=RANGE(-1000,1000);
      else if(dot_mode==FIND_ALLDOT)
         {
         for(vict=character_list;vict;vict=vict->next)
            {
            if(IS_MOB(vict))
               if(isname(name,vict->player.name))
                  GET_MOOD(vict)=RANGE(-1000,1000);
            }
         }
      else
         {
         for(vict=character_list;vict;vict=vict->next)
            {
            if(IS_MOB(vict))
               GET_MOOD(vict)=RANGE(-1000,1000);
            }
         }
      break;
   case 53:
      if(real_room(value)<=0)
         {
         send_to_char(ch,"You have to enter a valid room vnum\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         if (is_file)
            free_char(vict);
         return;
         break;
         }

      GET_MOB_VAL(vict,9)=value;
      SET_BIT(MOB_FLAGS(vict),MOB_GOPATH);
      break;

   case 54:
      if((GET_LEVEL(ch)<LVL_ADMIN)&&!PRF2_FLAGGED(ch,PRF2_TMPROG))
         {
         send_to_char(ch,"You can't set that!");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         return;
         break;
         }
      SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_EMPROG);
      break;
   case 55:
      SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_TMPROG);
      break;
   case 56: /* set age */
      if (value < 2 || value > 200)  /* Arbitrary limits. */
         {
         send_to_char(ch,"Ages 2 to 200 accepted.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         return;
         }
      /*
       * NOTE: May not display the exact age specified due to the integer
       * division used elsewhere in the code.  Seems to only happen for
       * some values below the starting age (17) anyway. -gg 5/27/98
       */
      vict->player.time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
      break;
   case 57:
      SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_DG_ATTACH);
      break;

   case 58:
      if(is_file)
         {
         sprintf(buf,"You can't set file <name> name <newname>\r\n");
         }
      if((i=find_name(GET_PC_NAME(vict)))==-1)
         {
         send_to_char(ch, "ERROR: there is no player %s.\r\n",
                      GET_PC_NAME(vict));
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         return;
         break;
         }
      if(find_name(val_arg) != -1)
         {
         mudlogf(BRF,GOD_LOG(ch),TRUE,
                 "(GC) but they failed, name in use.");
         send_to_char(ch,"Slow down there hoss, that name is already taken!\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         return;
         break;
         }
      if(player_table[i].name)
         free(player_table[i].name);
      CAP(val_arg);
      player_table[i].name=str_dup(val_arg);
      if(GET_PC_NAME(vict))
         free(GET_PC_NAME(vict));
      GET_PC_NAME(vict) = str_dup(val_arg);
      send_to_char(vict, "You are now known as %s.\r\n",val_arg);
      sprintf(buf, "Ok, new name set to %s.\r\n", GET_PC_NAME(vict));

      break;
   case 59:
      SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_HEDIT);
      break;
   case 60:
      value = MAX(MIN(value, 30000), -30000);
      GET_QPOINTS(vict) = value;
      send_to_char(vict, "You now have %d quest points.\r\n", value);
      mudlogf(BRF, GET_LEVEL(ch), TRUE, "(GC) %s has set %s's quest points to %d.",
              GET_NAME(ch), GET_NAME(vict), value);
      break;
   case 61:
      if (is_file)
         {
         send_to_char(ch,"You cannot change heartworns in file.  Use linkload first.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         free_char(vict);
         return;
         }
      if (value <= 0)
         {
         send_to_char(ch, "Ok, %s's heartworn has been removed.\r\n", GET_NAME(vict));
         if (GET_EQ(vict, WEAR_HEART))
            extract_obj(unequip_char(vict, WEAR_HEART));
         }
      else if ((r_num = real_object(value)) < 0)
         {
         send_to_char(ch,"There is no object with that number.\r\n");
         release_buffer(buf);
         release_buffer(field);
         release_buffer(val_arg1);
         release_buffer(val_arg);
         release_buffer(name);
         return;
         }
      else
         {
         obj = read_object(r_num, REAL);
         send_to_char(ch, "Ok, %s's heartworn set to %s.\r\n", GET_NAME(vict),
                 GET_OBJ_NAME(obj));
         if (GET_EQ(vict, WEAR_HEART))
            extract_obj(unequip_char(vict, WEAR_HEART));
         equip_char(vict, obj, WEAR_HEART);
         }
      break;
   case 62:
      REMORT_LEVEL(vict) = (value < 0) ? 0 : value;
      break;
   case 63:
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_HOLYLIGHT);
      break;
   case 64:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_PK);
      break;
   case 65:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOCOMMUNE);
      break;
   case 66:
     if (is_file) {
       send_to_char(ch, "You can't set explored on a file.\r\n");
     } else {
       GET_EXPLORED(vict) = value;
     }
     break;
   default:
      sprintf(buf, "Can't set that!");
      break;
      }


   if (fields[l].type == BINARY)
      {
      sprintf(buf, "%s %s for %s.\r\n", fields[l].cmd, ONOFF(on),
              GET_NAME(vict));
      }
   else if (fields[l].type == NUMBER)
      {
      sprintf(buf, "%s's %s set to %d.\r\n", vict?GET_NAME(vict):"NONE",
              fields[l].cmd, value);
      }
   else
      strcat(buf, "\r\n");
   send_to_char(ch,"%s",CAP(buf));


   if ((dot_mode==FIND_INDIV)&&!is_file && !IS_NPC(vict))
      save_char(vict, IN_ROOM(vict));


   if (is_file)
      {
      char *buf2=get_buffer(64);
      if(password_change==1)
         strcpy(buf2,tmp_store.pwd);
      char_to_store(vict, &tmp_store,FALSE);
      if(password_change==1)
         strcpy(tmp_store.pwd,buf2);
      release_buffer(buf2);
      /*
      fseek(player_fl, (player_i) * sizeof(struct char_file_u), SEEK_SET);
      fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
      */
      save_char_ascii(&tmp_store);

      int table_pos;
      int k;
      if((table_pos = find_id(GET_IDNUM(vict)))!=-1)
      {
    player_table[table_pos].level = GET_LEVEL(vict);
    player_table[table_pos].plr_flags=PLR_FLAGS(vict);

    for(k=0;k<5;k++)
      player_table[table_pos].gold[k] = vict->points.gold[k];
    for(k=0;k<32;k++)
      player_table[table_pos].bank_gold[k] = vict->points.bank_gold[k];
      }

      write_player_index_file();
      free_char(cbuf);
      send_to_char(ch,"Saved in file.\r\n");
      }
   release_buffer(buf);
   release_buffer(field);
   release_buffer(val_arg1);
   release_buffer(val_arg);
   release_buffer(name);
   }



void out_rent(char *name)
   {
   FILE *fl,*fp;
   char *filename =get_buffer(MAX_INPUT_LENGTH);
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *filename2=get_buffer(MAX_INPUT_LENGTH);
   struct obj_file_elem object;
   struct obj_data *obj;
   struct rent_info rent;


   if (!get_filename(name, filename, CRASH_FILE))
      {
      release_buffer(filename2);
      release_buffer(buf);
      release_buffer(filename);
      return;
      }
   if (!(fl = fopen(filename, "rb")))
      {
      release_buffer(filename2);
      release_buffer(buf);
      release_buffer(filename);
      return;
      }

   sprintf(buf, "%s\r\n", filename);
   if (!feof(fl))
      fread(&rent, sizeof(struct rent_info), 1, fl);

   if (!get_filename(name, filename2, NEW_OBJ_FILES))
      {
      log("SYSERR: Unable to complete conversion - unable to get new object filename: %s\r\n",name);
      release_buffer(filename2);
      release_buffer(buf);
      release_buffer(filename);
      return;
      }
   if (!(fp = fopen(filename2, "w+")))
      {
      log("SYSERR: Unable to open new object file.\r\n");
      release_buffer(filename2);
      release_buffer(buf);
      release_buffer(filename);
      return;
      }
   rent.net_cost_per_diem = 0;
   rent.rentcode = RENT_CRASH;
   rent.time = time(0);
   rent.gold = 1000000;
   rent.account = 1000000;
   rent.nitems=0;
   fprintf(fp,"@Version: %d\n",CUR_POBJ_VER);

   fprintf(fp,"%d %d %d %d %d %d\r\n",rent.rentcode,rent.time,
           rent.net_cost_per_diem,rent.gold,rent.account,rent.nitems);
   /* for rent code */

   while (!feof(fl))
      {
      fread(&object, sizeof(struct obj_file_elem), 1, fl);

      if (ferror(fl) || ferror(fp))
         {
         fclose(fl);
         fclose(fp);
         log("SYSERR: Error in conversion of rent files.");
         release_buffer(filename2);
         release_buffer(buf);
         release_buffer(filename);
         return;
         }

      if (!feof(fl))
         {
         if (real_object(object.item_number) > 0)
            {
            /* none of these will be unique items. just can't happen */
            obj = read_object(object.item_number, VIRTUAL);
            my_obj_save_to_disk(fp, obj,0);
            extract_obj(obj);
            }
         }
      }
   fclose(fl);

   /* write final line - this is never actually read.. but hey! */
   /*   fprintf(fp, "$~\n");*/
   fclose(fp);
   release_buffer(filename2);
   release_buffer(buf);
   release_buffer(filename);
   }

/* Xap - To keep old rent files (binary) and not lose all objects, you'll
   have to convert to ascii files.  Guess what?  This doesn't work as well
   as you'd think.  Well, It works, but here's some limitations:
    1. Sucks up memory. I don't know why. Maybe its specific to my mud,
        but I'm pretty sure I'm freeing all that needs it.
    2. It takes time.  You'll notice I use the process output command
        a few times.  This is because with something like 2500 rent files,
        it can take up to 2 or 3 minutes.
 I guess, use at your own risk. */

ACMD(do_objconv)
   {
   int counter;
   float percent;
   struct char_data *victim;
   struct char_file_u tmp_store;
   int flag=0;
   struct descriptor_data *d;


   if(GET_LEVEL(ch) != LVL_IMPL && !IS_NPC(ch))
      {
      send_to_char(ch,"You may not.\r\n");
      return;
      }

   send_to_all("Please hold on - object conversion taking place.\r\n");

   for (d = descriptor_list; d; d = d->next)
      {
      process_output(d);
      }

   /* okay, this is where we load every char, apparently.. but we'll
      do it one at a time, thank you very much */
   for (counter = 0; counter <= top_of_p_table; counter++)
      {

      /* individual load of characters so that way you may use this
      opportunity to use the rest of the character information..
      (like deleted, etc.. to choose if you want) */

      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char((player_table + counter)->name, &tmp_store) > -1)
         store_to_char(&tmp_store, victim);
      out_rent(GET_NAME(victim));
      free_char(victim);
      percent = (float)((float) counter/(float) top_of_p_table);
      if(percent > .25 && flag==0)
         {
         send_to_char(ch,"...");
         flag=1;
         process_output(ch->desc);
         }
      else if(percent > .50 && flag==1)
         {
         send_to_char(ch,"...");
         flag=2;
         process_output(ch->desc);
         }
      else if(percent > .75 && flag==2)
         {
         send_to_char(ch,"...");
         flag=3;
         process_output(ch->desc);
         }
      else if(percent > .90 && flag==3)
         {
         send_to_char(ch,"...");
         flag=4;
         process_output(ch->desc);
         }
      }
   send_to_char(ch,". Done.\r\n");
   }


extern int port;

ACMD(do_home)
   {
   struct char_data *victim = 0 ;
   int tmp;
   char *arg;

   if (IS_NPC(ch))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

#ifdef PLAYERS_PORT
   if (GET_LEVEL(ch) < 108) {
     tmp = real_room(GET_HOME(ch));
     char_from_room(ch);
     char_to_room(ch, tmp);
     do_look(ch, "\0", 0, 0);
     send_to_char(ch,"You have been sent home.\r\n");
     release_buffer(arg);
     return;
   }
#endif

   if (!*arg)
      {
      tmp = real_room(GET_HOME(ch));
      char_from_room(ch);
      char_to_room(ch, tmp);
      do_look(ch, "\0", 0, 0);
      send_to_char(ch,"You have been sent home.\r\n");
      }
   else
      {
      if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
         {
         send_to_char(ch,"They aren't here.\r\n");
         }
      else
         {
         if (!IS_NPC(victim))
            {
            if (GET_LEVEL(ch) >= GET_LEVEL(victim))
               {
               if (GET_LEVEL(victim) < LVL_IMMORT)
                  {
            int vnum = GET_ROOM_VNUM(IN_ROOM(victim));

                  tmp = real_room(GET_HOME(victim));
                  char_from_room(victim);
                  char_to_room(victim, tmp);
                  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
                  do_look(victim, "\0", 0, 0);
                  send_to_char(ch,"Your victim has been sent to the temple.\r\n");
                  if (GET_LEVEL(ch) <= LVL_ADMIN)
                     {
                     mudlogf(BRF,GOD_LOG(ch),TRUE,
                 "(GC) %s has sent %s home from room %d.",GET_NAME(ch),GET_NAME(victim), vnum);
                     }
                  }
               else
                  {
                  tmp = real_room(GET_HOME(victim));
                  char_from_room(victim);
                  char_to_room(victim, tmp);
                  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
                  do_look(victim, "\0", 0, 0);
                  if (victim != ch)
                     {
                     send_to_char(ch,"%s has been sent home.\r\n", GET_NAME(victim));
                     }
                  }
               }
            else
               {
               send_to_char(ch,"Sending a person higher lvl than you home is not possible.\r\n");
               }
            }
         else
            {
            send_to_char(ch,"%s has been sent to %s original room.\r\n",
                         GET_NAME(victim), HSHR(victim));
            char_from_room(victim);
            char_to_room(victim, victim->orig_room);
            do_look(victim, "\0", 0, 0);
            }
         }
      }
   release_buffer(arg);
   }


static char *logtypes[] =
   {
      "off", "brief", "normal", "complete", "\n"
   }
   ;


ACMD(do_syslog)
   {
   int tp;
   char *arg=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, arg);


   if (!*arg)
      {
      tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
            (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
      send_to_char(ch, "Your syslog is currently %s.\r\n", logtypes[tp]);
      }
   else if (((tp = search_block(arg, logtypes, FALSE)) == -1))
      {
      send_to_char(ch,"Usage: syslog { Off | Brief | Normal | Complete }\r\n");
      }
   else
      {
      REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
      SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2)>>1));


      send_to_char(ch, "Your syslog is now %s.\r\n", logtypes[tp]);
      }
   release_buffer(arg);
   }

void room_flags(int nr, char *buf2)
   {

   if(ROOM_FLAGGED(nr,ROOM_DARK))
      buf2[0]='A';
   else
      buf2[0]='_';
   if(ROOM_FLAGGED(nr,ROOM_DEATH))
      buf2[1]='B';
   else
      buf2[1]='_';
   if(ROOM_FLAGGED(nr,ROOM_NOMOB))
      buf2[2]='C';
   else
      buf2[2]='_';
   if(ROOM_FLAGGED(nr,ROOM_REGEN))
      buf2[3]='D';
   else
      buf2[3]='_';
   if(ROOM_FLAGGED(nr,ROOM_NOTRACK))
      buf2[4]='E';
   else
      buf2[4]='_';
   if(ROOM_FLAGGED(nr,ROOM_NOMAGIC))
      buf2[5]='F';
   else
      buf2[5]='_';
   if(ROOM_FLAGGED(nr,ROOM_TUNNEL))
      buf2[6]='G';
   else
      buf2[6]='_';
   if(ROOM_FLAGGED(nr,ROOM_PRIVATE))
      buf2[7]='H';
   else
      buf2[7]='_';
   if(ROOM_FLAGGED(nr,ROOM_NO_DECAY))
      buf2[8]='I';
   else
      buf2[8]='_';
   if(ROOM_FLAGGED(nr,ROOM_NO_RECALL))
      buf2[9]='J';
   else
      buf2[9]='_';
   if(ROOM_FLAGGED(nr,ROOM_NO_SUMMON))
      buf2[10]='K';
   else
      buf2[10]='_';
   if(ROOM_FLAGGED(nr,ROOM_PKILL))
      buf2[11]='L';
   else
      buf2[11]='_';
   if(ROOM_FLAGGED(nr,ROOM_PEACEFUL))
      buf2[12]='M';
   else
      buf2[12]='_';
   if(ROOM_FLAGGED(nr,ROOM_SOUNDPROOF))
      buf2[13]='N';
   else
      buf2[13]='_';
   if(ROOM_FLAGGED(nr,ROOM_INDOORS))
      buf2[14]='O';
   else
      buf2[14]='_';
   if(ROOM_FLAGGED(nr,ROOM_NO_CAMP))
      buf2[15]='P';
   else
      buf2[15]='_';
   if(ROOM_FLAGGED(nr,ROOM_TRAVEL))
      buf2[16]='Q';
   else
      buf2[16]='_';
   if(ROOM2_FLAGGED(nr,ROOM2_NEVERMOB))
      buf2[17]='R';
   else
      buf2[17]='_';
   buf2[17]='\0';

   }

ACMD(do_rlist)
   {
   char *buf=get_buffer(32750);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);
   char *buf3;
   char *tmpptr;

   int first, last, nr, found = 0;
   int lines;

   two_arguments(argument, buf, buf2);

   if (!*buf)
      {
      send_to_char(ch,"Usage: rlist <begining number> <ending number>\r\n");
      send_to_char(ch,"Usage: rlist <begining number>\r\n");
      send_to_char(ch,"Usage: rlist -h\r\n\r\n");




      release_buffer(buf2);
      release_buffer(buf);
      return;
      }


   if(!strcmp(buf,"-h"))
      {
      send_to_char(ch,"Meaning of Room Flag letters\r\n");
      send_to_char(ch," A - Dark\r\n");
      send_to_char(ch," B - Death\r\n");
      send_to_char(ch," C - NoMob\r\n");
      send_to_char(ch," D - Regen\r\n");
      send_to_char(ch," E - NoTrack\r\n");
      send_to_char(ch," F - NoMagic\r\n");
      send_to_char(ch," G - Tunnel\r\n");
      send_to_char(ch," H - Private\r\n");
      send_to_char(ch," I - NoDecay\r\n");
      send_to_char(ch," J - NoRecall\r\n");
      send_to_char(ch," K - NoSummon\r\n");
      send_to_char(ch," L - PKill\r\n");
      send_to_char(ch," M - Peaceful\r\n");
      send_to_char(ch," N - SoundProof\r\n");
      send_to_char(ch," O - Indoors\r\n");
      send_to_char(ch," P - NoCamp\r\n");
      send_to_char(ch," Q - Travel\r\n");
      send_to_char(ch," R - NeverMob\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      return;
      }

   first = atoi(buf);
   if(!*buf2)
      last=first+99;
   else
      last = atoi(buf2);

   if ((first < 0) || (first > 330000) || (last < 0) || (last > 330000))
      {
      send_to_char(ch,"Values must be between 0 and 330000.\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      return;
      }


   if (first >= last)
      {
      send_to_char(ch,"Second value must be greater than first.\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      return;
      }

   nr=real_room(first);
   if(nr==-1)
      nr=0;

   buf[0]='\0';
   buf3=get_buffer(128);
   sprintf(buf,"Num  Vnum   Zon   Room Name                   Flags          Lns Type\r\n");

   for (; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++)
      {
      if (GET_ROOM_VNUM(nr) >= first)
         {
       if (GET_LEVEL(ch) < RLIST_LEVEL && !is_olc_set(ch, GET_ROOM_VNUM(nr)/100)) {
         continue;
       }

         if(strlen(buf)>32000)
            {
            sprintf(buf+strlen(buf),"Too Many Items In Search\r\n");
            nr=top_of_world+1;
            }
         else
            {
            lines=0;
            room_flags(nr,buf3);
            sprinttype(world[nr].sector_type,sector_types,buf2);
            tmpptr=world[nr].description;
            while(*tmpptr!='\0')
               {
               if(*tmpptr=='\n')
                  lines++;
               tmpptr++;
               }
            sprintf(buf+strlen(buf),
                    "%4d.[%5ld](%3ld) %-27.27s %-17.17s %s%3d%s %-12.12s\r\n",
                    ++found,
                    GET_ROOM_VNUM(nr),
                    zone_table[world[nr].zone].number,
                    world[nr].name,
                    buf3,
                    lines>2?NCYN:NRED, lines, NNRM,
                    buf2);
            }
         }
      }
   release_buffer(buf3);
   release_buffer(buf2);
   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");

   if (!found)
      send_to_char(ch,"No rooms were found in those parameters.\r\n");
   release_buffer(buf);
   }


ACMD(do_mlist)
   {
   char *buf=get_buffer(32750);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);
   int first, last, nr, found = 0;

   two_arguments(argument, buf, buf2);


   if (!*buf)
      {
      send_to_char(ch,"Usage: mlist <begining number> <ending number>\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      return;
      }


   first = atoi(buf);
   if(!*buf2)
      last=first+99;
   else
      last = atoi(buf2);

   release_buffer(buf2);

   if ((first < 0) || (first > 330000) || (last < 0) || (last > 330000))
      {
      send_to_char(ch,"Values must be between 0 and 330000.\r\n");
      release_buffer(buf);
      return;
      }

   if (first >= last)
      {
      send_to_char(ch,"Second value must be greater than first.\r\n");
      release_buffer(buf);
      return;
      }

   nr=real_mobile(first);
   if(nr==-1)
      nr=0;

   strcpy(buf,"Num  Virtual Description                         lvl|cl|race|spec\r\n");
   for (; nr <= top_of_mobt && (mob_index[nr].vnum <= last); nr++)
      {
      if (mob_index[nr].vnum >= first)
         {
       if (GET_LEVEL(ch) < MLIST_LEVEL && !is_olc_set(ch, mob_index[nr].vnum/100)) {
         continue;
       }

         if(strlen(buf)>32000)
            {
            sprintf(buf+strlen(buf),"Too Many Items In Search\r\n");
            nr=top_of_mobt+1;
            }
         else
            sprintf(buf+strlen(buf),
                    "%3d. [%5ld] %-35.35s %3d|%-2.2s|%-4.4s|%-15.15s\r\n",
                    ++found,
                    mob_index[nr].vnum,
                    mob_proto[nr].player.short_descr,
                    mob_proto[nr].player.level,
                    class_abbrevs[(int)mob_proto[nr].player.class],
                    race_abbrevs[(int)mob_proto[nr].player.race],
                    mob_index[nr].func?get_mob_spec_name(mob_index[nr].func):" ");
         }
      }

   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");

   if (!found)
      send_to_char(ch,"No mobiles were found in those parameters.\r\n");
   release_buffer(buf);
   }


ACMD(do_zlist)
   {
   char *buf =get_buffer(32750);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);
   int first;
   int zone;
   int cmd_no;
   int last_room=0;
   int vis_loads=FALSE;
   one_argument(argument,buf2);


   if (!*buf2)
      {
      send_to_char(ch,"Usage: zlist <zone_num)\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      return;
      }

   first = atoi(buf2);
   release_buffer(buf2);

   if (GET_LEVEL(ch) < ZLIST_LEVEL && !is_olc_set(ch, first)) {
     send_to_char(ch, "You do not have permission to zlist that zone.\r\n");
     release_buffer(buf);
     return;
   }

   if (GET_LEVEL(ch) >= LVL_DGOD)
      vis_loads = TRUE;

   if ((first < 0) || (first > zone_table[top_of_zone_table].number))
      {
      send_to_char(ch, "Values must be between 0 and %ld.\r\n",
                   zone_table[top_of_zone_table].number);
      release_buffer(buf);
      return;
      }
   for(zone=0;(zone_table[zone].number<first);zone++)
      ;
   if((zone_table[zone].number!=first)||(zone>top_of_zone_table))
      {
      send_to_char(ch,"That is not a valid zone.\r\n");
      release_buffer(buf);
      return;
      }
   sprintf(buf,"Reset Commands for: %s(%ld)\r\n",zone_table[zone].name,
           zone_table[zone].number);

   for(cmd_no=0;ZCMD.command!='S';cmd_no++)
      {
      if(last_room!=ZCMD.room_num)
         {
         sprintf(buf+strlen(buf),"\r\nCommands for room: %ld\r\n",
                 ZCMD.room_num);
         last_room=ZCMD.room_num;
         }
      switch(ZCMD.command)
         {
      case 'M':
         sprintf(buf+strlen(buf),
                 "  %sLoad %s (%ld), Max: %ld (ln:%ld)\r\n",
                 ZCMD.if_flag?" ..then ":"",
                 mob_proto[ZCMD.arg1].player.short_descr,
                 ZCMD.oarg1,ZCMD.arg2,
                 ZCMD.line);
         break;
      case 'G':
         sprintf(buf+strlen(buf),
                 "  %sGive it %s (%ld), Chance: %ld%% (ln:%ld)\r\n",
                 ZCMD.if_flag?" ..then ":"",
                 obj_proto[ZCMD.arg1].short_description,
                 ZCMD.oarg1,
                 vis_loads?(ZCMD.arg3?(101-ZCMD.arg3):100):-1,
                 ZCMD.line);
         break;
      case 'O':
         sprintf(buf+strlen(buf),
                 "  %sLoad %s (%ld), Chance: %ld%% (ln:%ld)\r\n",
                 ZCMD.if_flag?" ..then ":"",
                 obj_proto[ZCMD.arg1].short_description,
                 ZCMD.oarg1,
                 vis_loads?(ZCMD.arg4?(101-ZCMD.arg4):100):-1,
                 ZCMD.line);
         break;
      case 'E':
         sprintf(buf+strlen(buf),
                 "  %sEquip with %s (%ld), %s, Chance: %ld%% (ln:%ld)\r\n",
                 ZCMD.if_flag?" ..then ":"",
                 obj_proto[ZCMD.arg1].short_description,
                 ZCMD.oarg1,equipment_types[ZCMD.arg3],
                 vis_loads?(ZCMD.arg4?(101-ZCMD.arg4):100):-1,
                 ZCMD.line);
         break;
      case 'P':
         sprintf(buf+strlen(buf),
                 "  %sPut %s (%ld) in %s (%ld), Chance: %ld%% (ln:%ld)\r\n",
                 ZCMD.if_flag?" ..then ":"",
                 obj_proto[ZCMD.arg1].short_description,
                 ZCMD.oarg1,
                 obj_proto[ZCMD.arg3].short_description,
                 ZCMD.oarg3,
                 vis_loads?(ZCMD.arg4?(101-ZCMD.arg4):100):0,
                 ZCMD.line);
         break;
      case 'R':
         sprintf(buf+strlen(buf),
                 "  %sRemove %s (%ld) from room. (ln:%ld)\r\n",
                 ZCMD.if_flag?" ..then ":"",
                 obj_proto[ZCMD.arg2].short_description,
                 ZCMD.oarg2,
                 ZCMD.line);
         break;
      case 'D':
         sprintf(buf+strlen(buf),
                 "  %sSet door %s as %s. (ln:%ld)\r\n",
                 ZCMD.if_flag?" ..then ":"",
                 dirs[ZCMD.arg2],
                 ZCMD.arg3 ? ((ZCMD.arg3 == 1)?"closed":"locked"):"open",
                       ZCMD.line);
         break;
      case 'F':
         sprintf(buf+strlen(buf),
                 "  %sForce to %s (ln:%ld)\r\n",
                 ZCMD.if_flag?" ..then ":"",
                 ZCMD.sarg,
                 ZCMD.line);
         break;
      default:
         sprintf(buf+strlen(buf),"  Unknown command %c %d %ld %ld %ld %ld %s "
                 "%ld, tell masque\r\n",ZCMD.command,ZCMD.if_flag,
                 ZCMD.oarg1,ZCMD.oarg2, ZCMD.oarg3,ZCMD.oarg4,
                 ZCMD.sarg,ZCMD.line);
         break;
         }
      }

   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");

   release_buffer(buf);
   }

ACMD(do_mlev)
   {
   char *buf=get_buffer(32750);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);
   int first, last, nr, found = 0;

   two_arguments(argument, buf, buf2);


   if (!*buf)
      {
      send_to_char(ch,"Usage: mlev <low level> <high level>\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      return;
      }


   first = atoi(buf);
   if(!*buf2)
      last=first+9;
   else
      last = atoi(buf2);

   release_buffer(buf2);

   if ((first < 0) || (first > 125) || (last < 0) || (last > 125))
      {
      send_to_char(ch,"Values must be between 0 and 125.\r\n");
      release_buffer(buf);
      return;
      }


   if (first >= last)
      {
      send_to_char(ch,"Second value must be greater than first.\r\n");
      release_buffer(buf);
      return;
      }


   strcpy(buf,"Num  Virtual Description                         lvl|cl|race|spec\r\n");
   for (nr=0; nr <= top_of_mobt; nr++)
      {
    if (GET_LEVEL(ch) < MLEV_LEVEL && !is_olc_set(ch, mob_index[nr].vnum/100)) {
      continue;
    }

      if((mob_proto[nr].player.level >= first) &&
              (mob_proto[nr].player.level <= last))
         {
         if(strlen(buf)>32000)
            {
            sprintf(buf+strlen(buf),"Too Many Items In Search\r\n");
            nr=top_of_mobt+1;
            }
         else
            sprintf(buf+strlen(buf),
                    "%3d. [%5ld] %-35.35s %3d|%-2.2s|%-4.4s|%-15.15s\r\n",
                    ++found,
                    mob_index[nr].vnum,
                    mob_proto[nr].player.short_descr,
                    mob_proto[nr].player.level,
                    class_abbrevs[(int)mob_proto[nr].player.class],
                    race_abbrevs[(int)mob_proto[nr].player.race],
                    mob_index[nr].func?get_mob_spec_name(mob_index[nr].func):" ");
         }
      }

   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");

   if (!found)
      send_to_char(ch,"No mobiles were found in those parameters.\r\n");
   release_buffer(buf);
   }

ACMD(do_distribute) {
   char b1[64];
   char b2[64];
   char b3[64];
   char b4[64];
   char b5[64];

   /** b1 = obj/mob
   ** b2 = vnum
   ** b3 = start room
   ** b4 = end room
   ** b5 = number of items
   **/

   five_arguments(argument, b1, b2, b3, b4, b5);

   if (!*b1 || !*b2 || !*b3 || !*b4 || !*b5) {
      send_to_char(ch,"Usage: distribute { obj | mob } <v_number> <start> <end> <amount>\r\n");
      return;
   }

   int vnum = atoi(b2); 
   int start_room = atoi(b3);
   int end_room = atoi(b4);
   int amount = atoi(b5);

   if (vnum <= 0) {
      send_to_char(ch, "Invalid vnum");
      return;
   }

   if (start_room < 99 || end_room < 99 || end_room < start_room || end_room - start_room > 99) {
      send_to_char(ch,"Invalid starting room or ending room.\r\n");
      return;
   }

   int rstart = -1;
   int rend = -1;

   for (room_rnum room = real_room(start_room); start_room <= end_room; room = real_room(++start_room)) {
      if (room == -1) continue;

      if(rstart == -1) rstart = start_room;

      rend = start_room;
   }

   if (rstart == -1) {
      send_to_char(ch,"No rooms in range.\r\n");
      return;
   }

   if (amount <= 0) {
      send_to_char(ch, "Invalid amount");
      return;
   }

   int mobobj = -1;

   if (is_abbrev(b1, "mob")) {
      mobobj = 1;
   } else if (is_abbrev(b1, "obj")) {
      mobobj = 0;
   }

   if (mobobj == -1) {
      send_to_char(ch,"That'll have to be either 'obj' or 'mob'.\r\n");
      return;
   }

   if(mobobj) { // mob
      mob_rnum rmob = real_mobile(vnum);
      if (rmob == -1) {
         send_to_char(ch,"There is no monster with that number.\r\n");
         return;
      }

      char* mob_name = 0;
      for (size_t ii = 0; ii < amount; ii++) {
         room_rnum random_room = -1;
         while(random_room == -1) {
            random_room = real_room(number(rstart, rend));
         }

         struct char_data* mob = read_mobile(rmob, REAL);

         if (mob == NULL) {
            send_to_char(ch,"Error reading mobile.\r\n");
            return;
         }

         if (!mob_name) mob_name = strdup(GET_NAME(mob));

         fprintf(stderr, "Distribute: %ld\n", random_room);
         char_to_room(mob, random_room);
         GET_MOB_VAL(mob,0)=GET_ROOM_VNUM(random_room);
         mob->orig_room=random_room;
         load_mtrigger(mob);
      }

      mudlogf(NRM, GOD_LOG(ch), TRUE, "(GC) %s distributed %d of mob #%d(%s) to room(s) %d-%d.", GET_NAME(ch), amount, vnum, mob_name, rstart, rend); 
      free(mob_name);
   } else {
      obj_rnum robj = real_object(vnum);
      if (robj == -1) {
         send_to_char(ch,"There is no object with that number.\r\n");
         return;
      }

      if ((GET_LEVEL(ch) < LVL_IMPL) && (real_zone(vnum) == 0)) {
         send_to_char(ch,"You cannot distribute from MUD Internals.\r\n");
         return;
      }

      char* obj_name = 0;

      for (size_t ii = 0; ii < amount; ii++) {
         struct obj_data* obj = read_object(robj, REAL);

         if (obj == NULL) {
            send_to_char(ch,"Error reading object.\r\n");
            return;
         }

         if (!obj_name) obj_name = GET_OBJ_NAME(obj);

         room_rnum random_room = -1;
         while(random_room == -1) {
            random_room = real_room(number(rstart, rend));
         }

         obj_to_room(obj, random_room);
         load_otrigger(obj);
      }

      mudlogf(NRM, GOD_LOG(ch), TRUE, "(GC) %s distributed %d of obj #%d(%s) to room(s) %d-%d.", GET_NAME(ch), amount, vnum, obj_name, rstart, rend);
      free(obj_name);
   }
}

ACMD(do_olist)
   {
   char *buf=get_buffer(32750);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);


   int first, last, nr, found = 0;

   two_arguments(argument, buf, buf2);

   if (!*buf)
      {
      send_to_char(ch,"Usage: olist <begining number> <ending number>\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      return;
      }
   first = atoi(buf);
   if(!*buf2)
      last=first+99;
   else
      last = atoi(buf2);

   release_buffer(buf2);

   if ((first < 0) || (first > 330000) || (last < 0) || (last > 330000))
      {
      send_to_char(ch,"Values must be between 0 and 330000.\r\n");
      release_buffer(buf);
      return;
      }

   if (first >= last)
      {
      send_to_char(ch,"Second value must be greater than first.\r\n");
      release_buffer(buf);
      return;
      }

   nr=real_object(first);
   if(nr==-1)
      nr=0;

   strcpy(buf,"Num   Virtual Description                 cost   lbs mat   ex\r\n");
   for (; nr <= top_of_objt && (obj_index[nr].vnum <= last); nr++)
      {
      if (obj_index[nr].vnum >= first)
         {
       if (GET_LEVEL(ch) < OLIST_LEVEL && !is_olc_set(ch, obj_index[nr].vnum/100)) {
         continue;
       }

         if(strlen(buf)>32000)
            {
            sprintf(buf+strlen(buf),"Too Many Items In Search\r\n");
            nr=top_of_objt+1;
            }
         else
            sprintf(buf+strlen(buf), "%5d.[%5ld] %-25.25s %6d %5d %-6.6s %c\r\n",
                    ++found,
                    obj_index[nr].vnum,
                    obj_proto[nr].short_description,
                    obj_proto[nr].obj_flags.cost,
                    obj_proto[nr].obj_flags.weight,
                    material_types[obj_proto[nr].material],
                    obj_proto[nr].ex_description?'|':'-');
         }
      }
   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");


   if (!found)
      send_to_char(ch,"No objects were found in those parameters.\r\n");

   release_buffer(buf);
   }


ACMD(do_plist)
   {
   char *buf=get_buffer(32750);
   char *buf2=get_buffer(MAX_INPUT_LENGTH);


   int first, last, nr, found = 0;


   two_arguments(argument, buf, buf2);


   if (!*buf)
      {
      send_to_char(ch,"Usage: plist <begining number> <ending number>\r\n");
      send_to_char(ch,"Usage: plist <begining number>\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      return;
      }
   first = atoi(buf);
   if(!*buf2)
      last=first+99;
   else
      last = atoi(buf2);

   release_buffer(buf2);

   if ((first < 0) || (first > 330000) || (last < 0) || (last > 330000))
      {
      send_to_char(ch,"Values must be between 0 and 330000.\r\n");
      release_buffer(buf);
      return;
      }


   if (first >= last)
      {
      send_to_char(ch,"Second value must be greater than first.\r\n");
      release_buffer(buf);
      return;
      }

   nr=real_path(first);
   if(nr==-1)
      nr=0;

   strcpy(buf,"Num    Virtual Description                             NumCmds\r\n");
   for (; nr <= top_path && (path_index[nr].number <= last); nr++)
      {
      if (path_index[nr].number >= first)
         {
         if(strlen(buf)>32000)
            {
            sprintf(buf+strlen(buf),"Too Many Items In Search\r\n");
            nr=top_path+1;
            }
         else
            sprintf(buf+strlen(buf), "%5d. [%5d] %-40.40s %3d\r\n",++found,
                    path_index[nr].number,
                    path_index[nr].name,
                    path_index[nr].num_commands);
         }
      }
   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");


   if (!found)
      send_to_char(ch,"No paths were found in those parameters.\r\n");

   release_buffer(buf);
   }



ACMD(do_gremort)
{
   struct char_data *victim = 0;

   skip_spaces(&argument);
   if (!*argument)
      {
      send_to_char(ch,"Which player has completed his hero quest?\r\n");
      }
   else if(!(victim = get_char(argument)))
      {
      send_to_char(ch,"That player is not here.\n\r");
      return;
      }
   else if(IS_NPC(victim))
      {
      send_to_char(ch,"You can't Hero a mob!!\n\r");
      return;
      }
   else if(GET_LEVEL(victim)!=(LVL_HERO-1))
      {
      send_to_char(ch, "%s needs %d more levels to become a hero!!!\r\n",
                   GET_NAME(victim),(LVL_HERO-1)-GET_LEVEL(victim));
      return;
      }
   else if(1)//GET_EXP(victim)>=GET_EXP_FOR_CH(victim))
      {
      if(REMORT_LEVEL(victim) == NON_REMORT)
         {
         GET_EXP(victim)=GET_EXP(victim)-GET_EXP_FOR_CH(victim);
         GET_LEVEL(victim)++;
         send_info("[ INFO ] %s is now a HERO!!!!!\r\n",GET_NAME(victim));
         advance_level(victim,TRUE);
         }
      else if(REMORT_LEVEL(victim) == SINGLE_REMORT)
         {
         GET_EXP(victim)=GET_EXP(victim)-GET_EXP_FOR_CH(victim);
         GET_LEVEL(victim)+=2;
         send_info("[ INFO ] %s is now an ANGEL!!!!!\r\n",GET_NAME(victim));
         advance_level(victim,FALSE);
         advance_level(victim,TRUE);
         }
      else if(REMORT_LEVEL(victim) == DOUBLE_REMORT)
         {
         GET_EXP(victim)=GET_EXP(victim)-GET_EXP_FOR_CH(victim);
         GET_LEVEL(victim)+=3;
         send_info("[ INFO ] %s is now an AVATAR!!!!!\r\n",GET_NAME(victim));
         advance_level(victim,FALSE);
         advance_level(victim,FALSE);
         advance_level(victim,TRUE);
         GET_COND(victim, FULL) = -1;
         GET_COND(victim, THIRST) = -1;
         }
      else if (REMORT_LEVEL(victim) == TRIPLE_REMORT)
      {
         GET_EXP(victim)=GET_EXP(victim)-GET_EXP_FOR_CH(victim);
         GET_LEVEL(victim) += 4;
         send_info("[ INFO ] %s is now an AVATAR AGAIN!!!!!\r\n",GET_NAME(victim));
         advance_level(victim,FALSE);
         advance_level(victim,FALSE);
         advance_level(victim,FALSE);
         advance_level(victim,TRUE);
         GET_COND(victim, FULL) = -1;
         GET_COND(victim, THIRST) = -1;
      }
      else
         {
         mudlogf(BRF,MAX(LVL_IMMORT,GET_INVIS_LEV(ch)),TRUE,
                 "SYSERR: Invalid remort level when trying to gremort %s!!",
                 GET_NAME(victim));
         return;
         }
      mudlogf(CMP,MAX(LVL_IMMORT,GET_INVIS_LEV(ch)),TRUE,
              "(GC) %s gremorts %s.",GET_NAME(ch),GET_NAME(victim));

      send_to_char(ch, "%s", OK);
      }
   else
      {
      send_to_char(ch, "%s needs %ld more exp to become a Hero.\r\n",
                   GET_NAME(victim),(long int)(GET_EXP_FOR_CH(victim)-GET_EXP(victim)));
      }

   }



ACMD(do_findpath)
   {
   int dir, done=FALSE;
   room_rnum sroom, troom, temp_room;
   char *arg=get_buffer(256);
   char *buf;
   int last_dir;
   int count;

   sroom=IN_ROOM(ch);

   one_argument(argument,arg);

   if(!*arg)
      {
      send_to_char(ch,"USAGE: findpath <destination_vnum>\r\n");
      release_buffer(arg);
      return;
      }

   if(!(troom=real_room(atoi(arg))))
      {
      send_to_char(ch,"No such destination vnum!\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);
   if(troom==-1)
      {
      send_to_char(ch,"No such destination vnum!\r\n");
      return;
      }

   buf=get_buffer(1024);
   sprintf(buf,"Path from %ld to %ld : ",GET_ROOM_VNUM(sroom),
           GET_ROOM_VNUM(troom));
   temp_room=IN_ROOM(ch);
   count=1;
   last_dir=-1;
   do
      {
      done=FALSE;
      dir=find_first_step(sroom,troom, IGNORE_NOTRACK|IGNORE_ZNOTRACK|IGNORE_THROUGHDOOR|IGNORE_WATER|IGNORE_FLY);
      switch(dir)
         {
      case BFS_ERROR:
         send_to_char(ch,"[BFS_ERR]");
         done=TRUE;
         break;
      case BFS_ALREADY_THERE:
         done=TRUE;
         if(last_dir==-1)
            strcat(buf,"Same start and destination rooms");
         else if(count==1)
            {
            sprintf(buf+strlen(buf),"%c",*dirs[last_dir]);
            last_dir=dir;
            }
         else if(count>1)
            {
            sprintf(buf+strlen(buf),"%d%c",count,*dirs[last_dir]);
            count=1;
            last_dir=dir;
            }
         else
            {
            send_to_char(ch,"Find Path Error!!\r\n");
            log("SYSERR:FindPath: count: %d, last_dir: %d, dir: %d,"
                " start: %ld, end: %ld",count,last_dir,dir,
                GET_ROOM_VNUM(IN_ROOM(ch)),GET_ROOM_VNUM(troom));
            }
         break;
      case BFS_NO_PATH:
         send_to_char(ch,"[NO_PATH]");
         done = TRUE;
         break;
      default:
         if(last_dir==dir)
            count++;
         else if(last_dir==-1)
            last_dir=dir;
         else if(count==1)
            {
            sprintf(buf+strlen(buf),"%c",*dirs[last_dir]);
            last_dir=dir;
            }
         else if(count>1)
            {
            sprintf(buf+strlen(buf),"%d%c",count,*dirs[last_dir]);
            count=1;
            last_dir=dir;
            }
         else
            {
            send_to_char(ch,"Find Path Error!!\r\n");
            log("SYSERR:FindPath: count: %d, last_dir: %d, dir: %d,"
                " start: %ld, end: %ld",count,last_dir,dir,
                GET_ROOM_VNUM(IN_ROOM(ch)),GET_ROOM_VNUM(troom));
            }
         done=FALSE;
         break;
         }
      if(!done && EXIT2(temp_room,dir)->to_room!=NOWHERE)
         {
         sroom = EXIT2(temp_room,dir)->to_room;
         temp_room=sroom;
         }
      }
   while(!done)
      ;
   send_to_char(ch,"%s\r\n",buf);
   release_buffer(buf);
   }



ACMD(adjust_mobs)
   {
   int nr;
   int vznum;
   obj_rnum skin;

   if(str_cmp(GET_NAME(ch),"masque") != 0)
      {
      send_to_char(ch,"Please talk to Masque about using this command.\r\n");
      }

   for (nr = 0; nr <= top_of_mobt; nr++)
      {
      /*justify_mob(&mob_proto[nr]); */
      skin=real_object(mob_proto[nr].mob_specials.skin);
      if(skin==NOTHING || skin==0)
         mob_proto[nr].mob_specials.skin=NOTHING;

      vznum=mob_index[nr].vnum/100;
      olc_add_to_save_list(vznum,OLC_SAVE_MOB);
      }
   send_to_char(ch, "%s", OK);
   }

ACMD(adjust_objs)
   {
   int nr;
   int vznum;

   if(str_cmp(GET_NAME(ch),"masque") != 0)
      {
      send_to_char(ch,"Please talk to Masque about using this command.\r\n");
      }

   for (nr = 0; nr <= top_of_objt; nr++)
      {
      vznum=obj_index[nr].vnum/100;
      obj_proto[nr].obj_flags.total_dam_slots
      =material_affs[obj_proto[nr].material].default_dam_slots;
      obj_proto[nr].obj_flags.curr_dam_slots
      =material_affs[obj_proto[nr].material].default_dam_slots;
      obj_proto[nr].obj_flags.orig_dam_slots
      =material_affs[obj_proto[nr].material].default_dam_slots;
      olc_add_to_save_list(vznum,OLC_SAVE_OBJ);
      }
   send_to_char(ch, "%s", OK);
   }

ACMD(adjust_rooms)
   {
   int nr;
   int vznum;

   if(str_cmp(GET_NAME(ch),"masque") != 0)
      {
      send_to_char(ch,"Please talk to Masque about using this command.\r\n");
      }

   for (nr = 0; nr <= top_of_world; nr++)
      {

      vznum=GET_ROOM_VNUM(nr)/100;
      olc_add_to_save_list(vznum,OLC_SAVE_ROOM);
      }
   send_to_char(ch, "%s", OK);
   }

ACMD(adjust_zones)
   {
   int nr;
   int vznum;

   if(str_cmp(GET_NAME(ch),"masque") != 0)
      {
      send_to_char(ch,"Please talk to Masque about using this command.\r\n");
      }

   for (nr = 0; nr <= top_of_zone_table; nr++)
      {

      vznum=zone_table[nr].number;
      olc_add_to_save_list(vznum,OLC_SAVE_ZONE);
      }
   send_to_char(ch, "%s", OK);
   }

ACMD(adjust_shops)
   {
   int nr;
   int vznum;

   if(str_cmp(GET_NAME(ch),"masque") != 0)
      {
      send_to_char(ch,"Please talk to Masque about using this command.\r\n");
      }

   for (nr = 0; nr < top_shop; nr++)
      {
      vznum=SHOP_ROOM(nr,0)/100;
      olc_add_to_save_list(vznum,OLC_SAVE_SHOP);
      }
   send_to_char(ch, "%s", OK);
   }



ACMD(do_helpcheck)
   {
   int i;
   int w=0;
   char *buf=get_buffer(32750);

   if(!help_table)
      {
      send_to_char(ch,"The help_table doesn't exits !?!?!\r\n");
      release_buffer(buf);
      return;
      }

   strcpy(buf,"Commands without help entries:\r\n");
   strcat(buf,"------------------------------\r\n");

   for(i=1;*(cmd_info[i].command)!='\n';i++)
      {
      if(find_help(cmd_info[i].command,1)==NULL)
         {
         if(cmd_info[i].command_pointer==do_action)
            continue;
         w++;
         w=w%3;
         sprintf(buf+strlen(buf)," %-20.20s%s",
                 cmd_info[i].command?cmd_info[i].command:"missing?!?!",
                 (w?"|":"\r\n"));
         }
      }

   if(w)
      strcat(buf,"\r\n");

   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");
   release_buffer(buf);
   }


ACMD(do_moblevels)
   {
   int total_level[13];
   int zone_level[13];
   int zone,cmd_no,level,i;
   char *buf=get_buffer(65536);

   strcpy(buf,"  # Zone Name                  1-9 10s 20s 30s 40s 50s 60s 70s 80s 90s 100s 110s 120s Tot\r\n");

   for(i=0;i<13;i++)
      {
      total_level[i]=0;
      }
   for(zone=0;zone<=top_of_zone_table;zone++)
      {
      for(i=0;i<13;i++)
         {
         zone_level[i]=0;
         }
      for(cmd_no=0;ZCMD.command!='S';cmd_no++)
         {
         if(ZCMD.command=='M')
            {
            level=mob_proto[ZCMD.arg1].player.level/10;
            total_level[level]++;
            zone_level[level]++;
            }
         }
      level=0;
      for(i=0;i<13;i++)
         {
         level+=zone_level[i];
         }

      sprintf(buf+strlen(buf), "%4ld %-25.25s %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %4d %4d %4d %4d\r\n",
              zone_table[zone].number,
              zone_table[zone].name,
              zone_level[0],  zone_level[1],  zone_level[2],
              zone_level[3],  zone_level[4],  zone_level[5],
              zone_level[6],  zone_level[7],  zone_level[8],
              zone_level[9],  zone_level[10], zone_level[11],
              zone_level[12], level);

      }
   level=0;
   for(i=0;i<13;i++)
      {
      level+=total_level[i];
      }
   sprintf(buf+strlen(buf), "%-29.29s %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %4d %4d %4d %4d\r\n","World Totals",
           total_level[0],  total_level[1],  total_level[2],
           total_level[3],  total_level[4],  total_level[5],
           total_level[6],  total_level[7],  total_level[8],
           total_level[9],  total_level[10], total_level[11],
           total_level[12], level);

   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");
   release_buffer(buf);
   }


ACMD(do_addpoint)
   {
   struct char_data *victim;
   char *buf;
   char *arg;
   if (IS_NPC(ch))
      return;
   arg = get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      send_to_char(ch,"Usage: addpoint <player>\n\r");
      release_buffer(arg);
      return;
      }

   if (!(victim = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
      {
      send_to_char(ch,"No such person around.\n\r");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if (IS_NPC(victim))
      {
      send_to_char(ch,"Mobs can't have quest points.\n\r");
      return;
      }
   buf=get_buffer(512);
   GET_QPOINTS(victim)++;
   send_to_char(ch, "A point has been added to %s's Quest points.\n\r",
                GET_NAME(victim));
   send_to_char(ch, "%s now has %d Quest points...\n\r", GET_NAME(victim),
                GET_QPOINTS(victim));
   send_to_char(victim, "You have been rewarded with a quest point.\r\n");
   mudlogf(BRF, GOD_LOG(ch), TRUE, "(GC) %s has added a quest point to %s",
           GET_NAME(ch), GET_NAME(victim));
   release_buffer(buf);
   }

ACMD(do_delpoint)
   {
   struct char_data *victim;
   char *buf;
   char *arg;
   if (IS_NPC(ch))
      return;
   arg = get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      send_to_char(ch,"Usage: delpoint <player>\n\r");
      release_buffer(arg);
      return;
      }

   if (!(victim = get_char_vis(ch, arg,FIND_CHAR_WORLD)))
      {
      send_to_char(ch,"No such person around.\n\r");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if (IS_NPC(victim))
      {
      send_to_char(ch,"Mobs can't have quest points.\n\r");
      return;
      }
   buf=get_buffer(512);
   GET_QPOINTS(victim)--;
   send_to_char(ch, "A point has been removed from %s's Quest points.\n\r",
                GET_NAME(victim));
   send_to_char(ch, "%s now has %d Quest points...\n\r", GET_NAME(victim),
                GET_QPOINTS(victim));
   send_to_char(victim,"You have been charged a quest point.\r\n");
   mudlogf(BRF, GOD_LOG(ch), TRUE, "(GC) %s has removed a quest point from %s",
           GET_NAME(ch), GET_NAME(victim));
   release_buffer(buf);
   }

ACMD(do_reimb)
   {
   char *arg = get_buffer(MAX_INPUT_LENGTH);
   struct char_data *vict;
   int xap_objs_backup,i;

   one_argument(argument, arg);

   if (!*arg)
      {
      send_to_char(ch,"Who do you want to reimb?\r\n");
      release_buffer(arg);
      return;
      }

   if (!(vict = get_char_room_vis(ch, arg)))
      {
      send_to_char(ch,"They aren't here.\r\n");
      release_buffer(arg);
      return;
      }
   if (GET_LEVEL(ch) < GET_LEVEL(vict))
      {
      send_to_char(ch,"You mayn't.\r\n");
      release_buffer(arg);
      return;
      }
   for (i = 0; i < NUM_WEARS; i++)
      {
      if (GET_EQ(vict,i) && (i != WEAR_HEART))
         {
         send_to_char(ch,"The target char must not have any equipment or inv.\r\n");
         send_to_char(ch,"(Target has equipment still)\r\n");
         release_buffer(arg);
         return;
         }
      }
   if(vict->carrying)
      {
      send_to_char(ch,"The target char must not have any equipment or inv.\r\n");
      release_buffer(arg);
      return;
      }
   xap_objs_backup=xap_objs;
   xap_objs=2;

   /* actually, crash_load_xapobjs uses somewhat random return codes.
      Better set them for your own system, else even a good reimb
      will say it's an error */

   if(!Crash_load_xapobjs(vict))
      {
      send_to_char(ch,"There was an error with that person's reimb file.\r\n");
      send_to_char(vict,"A reimb was attempted, but there was an error with your file.\r\n");
      release_buffer(arg);
      xap_objs=xap_objs_backup;
      return;
      }
   xap_objs=xap_objs_backup;
   send_to_char(ch,"Reimb successful.\r\n");
   mudlogf(CMP,GOD_LOG(ch),TRUE,"(GC) %s reimbursed by %s.",GET_NAME(vict),GET_NAME(ch));
   send_to_char(vict,"You've been reimbed by the gods!\r\n");
   release_buffer(arg);
   }


ACMD(do_itake)
   {
   int found, i;
   struct char_data *victim;
   struct obj_data *tar_obj = NULL;
   char *buf = get_buffer(256);
   char *buf2 = get_buffer(256);

   half_chop(argument, buf2, buf);

   if (!*buf2 || !*buf)
      {
      send_to_char(ch, "Usage: itake <item> <victim>\r\n");
      }
   else if (!(victim = get_char_vis(ch, buf,FIND_CHAR_WORLD)))
      {
      send_to_char(ch, "Can't find the person you are looking for.\r\n");
      }
   else
      {
      for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
         if (GET_EQ(victim, i) && isname(buf2, GET_EQ(victim, i)->name))
            {
            tar_obj = GET_EQ(victim, i);
            found = TRUE;
            break;
            }
      if (!found)
         {
         if (!(tar_obj = get_obj_in_list_vis(victim,buf2,victim->carrying)))
            {
            send_to_char(ch, "%s does not seem to have \"%s\"\r\n",
                         GET_NAME(victim), buf2);
            release_buffer(buf2);
            release_buffer(buf);
            return;
            }
         }

      if (found)
         obj_to_char(unequip_char(victim, i), victim);
      obj_from_char(tar_obj);
      obj_to_char(tar_obj, ch);
      send_to_char(ch, "You took %s from %s.\r\n",
                   GET_OBJ_NAME(tar_obj), GET_NAME(victim));
      mudlogf(BRF, GOD_LOG(ch), TRUE, "(GC) itake: %s took %s from %s.",
              GET_NAME(ch), GET_OBJ_NAME(tar_obj), GET_NAME(victim));
      }
   release_buffer(buf);
   release_buffer(buf2);
   return;
   }


ACMD(do_igive)
   {
   struct obj_data *igive_obj;
   struct char_data *victim;
   char *arg = get_buffer(256);
   char *arg2 = get_buffer(256);

   half_chop(argument, arg, arg2);

   if (!*arg || !*arg2)
      {
      send_to_char(ch, "Usage: igive <item> <victim>\r\n");
      }
   else if (!(victim = get_char_vis(ch, arg2 ,FIND_CHAR_WORLD)))
      {
      send_to_char(ch, "Can't find the person you are looking for.\r\n");
      }
   else if (!(igive_obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
      {
      send_to_char(ch, "You don't seem to have \"%s\"\r\n", arg);
      }
   else
      {
      obj_from_char(igive_obj);
      obj_to_char(igive_obj, victim);
      send_to_char(ch, "You give %s to %s.\r\n",
                   GET_OBJ_NAME(igive_obj), GET_NAME(victim));
      mudlogf(BRF, GOD_LOG(ch), TRUE, "(GC) igive: %s has given %s to %s.",
              GET_NAME(ch), GET_OBJ_NAME(igive_obj), GET_NAME(victim));
      }
   release_buffer(arg2);
   release_buffer(arg);
   }


ACMD(do_logsearch) {

    char shname[MAX_INPUT_LENGTH], searchstr[MAX_INPUT_LENGTH];
    unsigned long l = 0, s = 0, match = 0, start = 0, lines = 300;
    char *temp;
    FILE *f;
    int i;
    char *buf = get_buffer(512);
    char *buf2 = get_buffer(32750);

    const struct log_struct {
        const char *name;
        const char *path;
        int minlev;
        const char *purpose;
    } loginf[] = {
        {"syslog1"  , LOG_SYSLOG1    , LVL_ADMIN , "Oldest syslog file"},
        {"syslog2"  , LOG_SYSLOG2    , LVL_ADMIN , "Uptime prior to syslog3"},
        {"syslog3"  , LOG_SYSLOG3    , LVL_ADMIN , "Uptime prior to syslog4"},
        {"syslog4"  , LOG_SYSLOG4    , LVL_ADMIN , "Uptime prior to syslog5"},
        {"syslog5"  , LOG_SYSLOG5    , LVL_ADMIN , "Uptime prior to syslog6"},
        {"syslog6"  , LOG_SYSLOG6    , LVL_ADMIN , "Uptime prior to system"},
        {"badpws"   , BADPW_LOG      , LVL_GOD   , "Bad passwords"},
        {"ban"      , BAN_LOG        , LVL_DGOD  , "Bans, Unbans, and attempts"},
        {"bigrent"  , BIGRENT_LOG    , LVL_IMMORT, "Idling out with large rent"},
        {"buffer"   , BUF_LOG        , LVL_GOD   , "Buffer errors/logs"},
        {"bugs"     , BUG_FILE       , LVL_DETY  , "Bug file"},
        {"corpse"   , CORPSE_LOG     , LVL_IMMORT, "Player corpses"},
        {"changes"  , CHANGES_FILE   , LVL_IMMORT, "Code changes"},
        {"crash"    , CRASH_LOG      , LVL_ADMIN , "Tail of syslog before last crash"},
        {"delete"   , DELETE_LOG     , LVL_DGOD  , "Player self-deletions"},
        {"dts"      , DT_LOG         , LVL_DGOD  , "Death traps"},
        {"errors"   , ERRORS_FILE    , LVL_IMMORT, "System errors"},
        {"gold"     , GOLD_LOG       , LVL_DGOD  , "Gold logging"},
        {"godcmds"  , GODCMD_LOG     , LVL_ADMIN , "Immortal commands log"},
        {"help"     , HELP_LOG       , LVL_IMMORT, "Non-existant, requested help files"},
        {"ideas"    , IDEA_FILE      , LVL_IMMORT, "Ideas file"},
        {"levels"   , LOG_LEVELS     , LVL_IMMORT, "Mort and Immort levels"},
        {"locateobj", LOCATE_OBJ_LOG , LVL_IMMORT, "Log of Locate Object spell"},
        {"maillog"  , MAIL_LOG       , LVL_IMPL  , "Log of all mails"},
        {"new"      , LOG_NEWPLAYERS , LVL_IMMORT, "New players"},
        {"objscrap" , OBJSCRAP_LOG   , LVL_IMMORT, "Objects that have fallen apart"},
        {"olc"      , OLC_LOG        , LVL_IMMORT, "OLC activity"},
        {"rentgone" , RENTGONE_LOG   , LVL_GOD   , "Instances of possible rent wipes"},
        {"restarts" , RESTART_LOG    , LVL_GOD   , "Date/Time of mud reboots"},
        {"rip"      , RIP_LOG        , LVL_IMMORT, "Death by mob logs"},
        {"scripterr", SCRIPTERR_LOG  , LVL_ARCH  , "DG Script errors"},
        {"scriptlog", SCRIPT_LOG     , LVL_ARCH  , "Dg Script logs(via dg_log)"},
        {"system"   , SYSLOG_FILE    , LVL_ADMIN , "Main syslog file"},
        {"typos"    , TYPO_FILE      , LVL_IMMORT, "Typo file"},
        {"usage"    , USAGE_LOG      , LVL_DGOD  , "Usage logs(5 minute increment)"},
        {"quests"   , DG_QUEST_LOG   , LVL_IMMORT, "DG Quest log"},
        {"graffiti" , GRAFFITI_LOG   , LVL_IMMORT, "Graffiti log"},
        {"upload"   , UPLOAD_LOG     , LVL_ADMIN , "Zone file upload log"},
        {"\0",0,0}
    };

    temp = two_arguments(argument, shname, buf);

    if (!*argument) {
        send_to_char(ch, "\r\n Num  Log Name                 "
                         " Level            Description\r\n");
        for (i = 0; *loginf[i].name; i++)
            if (GET_LEVEL(ch) >= loginf[i].minlev) {
                send_to_char(ch, " %3d. %-20s %-20s %s\r\n",
                             i+1, loginf[i].name,
                             WizLevels[((int)loginf[i].minlev)-LVL_IMMORT],
                             loginf[i].purpose);
            }
        release_buffer(buf);
        release_buffer(buf2);
        return;
    } else if (!isdigit(*buf))
        half_chop(argument, shname, searchstr);
    else {
        lines = atol(buf);
        skip_spaces(&temp);
        strcpy(searchstr, temp);
    }

    if (subcmd == SCMD_VIEWLOG && *searchstr) {
        start = lines;
        lines = atol(searchstr);
    }

    if (subcmd == SCMD_LOGSEARCH && !*searchstr)
        send_to_char(ch, "Please, at minimum, specify a valid log name and a search string.\r\n");
    else {

        for (i = 0; *loginf[i].name && !is_abbrev(shname, loginf[i].name); i++);

        if (!*loginf[i].name)
            send_to_char(ch, "No such log file.\r\n");
        else if (GET_LEVEL(ch) < loginf[i].minlev)
            send_to_char(ch, "You don't have access to that log file.\r\n");
        else if (!(f = fopen(loginf[i].path, "r")))
            send_to_char(ch, "Error opening log file.\r\n");
        else {

            if (subcmd == SCMD_VIEWLOG)
                if (start)
                    for (l = 1; l < start; l++)
                        get_line(f, buf);
                else {
                    /* count lines */
                    do { l += get_line(f, buf); } while (!feof(f));
                    rewind(f);

                    /* sync if necessary */
                    if (lines > l)
                        l = 1;
                    else {
                        start = (l - lines) + 1;
                        for (l = 1; l < start; l++)
                            get_line(f, buf);
                    }
                }
            else {
                do {
                    l += get_line(f, buf);
                    if (str_str(buf, searchstr))
                        s++;
                } while (!feof(f));
                rewind(f);

                start = s;
                if (lines > s)
                    l = 1;
                else {
                    for (l = 1; lines < s; l++) {
                        get_line(f, buf);
                        if (str_str(buf, searchstr))
                            s--;
                    }
                }
            }

            for (;match < lines; l++) {

                get_line(f, buf);

                if (feof(f))
                    break;

                if (subcmd == SCMD_VIEWLOG || str_str(buf, searchstr)) {
                    sprintf(buf2+strlen(buf2), "[%ld] %s\r\n", l, buf);
                    if (strlen(buf2) > 32000)
                       {
                       sprintf(buf2+strlen(buf2), "***Too many results, refine your search.\r\n");
                       break;
                       }
                    /* send_to_char(ch, buf2); */
                    match++;
               }

            }
            if (!match)
               send_to_char(ch, "No lines matched your criteria.\r\n");
            else if (subcmd == SCMD_LOGSEARCH)
               send_to_char(ch, "%sThe search phrase \'%s\' was matched %ld times."
                                 " Now showing most recent %ld matches.%s\r\n",
                                 NCYN, searchstr, start, match, NNRM);
            fclose(f);
            if (ch->desc)
               page_string(ch->desc,buf2,TRUE,"");
        }
     }
     release_buffer(buf);
     release_buffer(buf2);
  }


/* erase last tell to victim if it was from you */
ACMD(do_end_discussion)
{
   struct char_data *tch;

   if (IS_NPC(ch))
      return;

   skip_spaces(&argument);
   if (!*argument)
      {
      send_to_char(ch, "Usage: endreply <player>\r\n");
      return;
      }

   if (!(tch = get_player_vis(ch, argument, FIND_CHAR_WORLD)))
      {
      send_to_char(ch, "Cannot find victim to end discussion with.\r\n");
      return;
      }

   if (GET_IDNUM(ch) == GET_LAST_TELL(tch))
      {
      send_to_char(ch, "%s will no longer be able to reply to you.\r\n",
          GET_NAME(tch));
      GET_LAST_TELL(tch) = NOBODY;
      }
   else
      {
      send_to_char(ch, "The last tell to %s is not from you.\r\n", GET_NAME(tch));
      }

}



ACMD(do_add_news)
{
   char **news_add;

   send_to_char(ch, "Go ahead and add to the news, /a will cancel.\r\n");
   act("$n pulls out a large scroll and begins scribbling all over it.",
        TRUE, ch, 0, 0, TO_ROOM);

   CREATE(news_add, char *, 1);
   ch->desc->storage = str_dup(NEWS_FILE);
   string_write(ch->desc, news_add, MAX_STRING_LENGTH, 0, NULL);
   STATE(ch->desc) = CON_ADD_NEWS;
}
