/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__
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
#include "screen.h"
#include "house.h"
#include "ident.h"
#include <sys/stat.h>
#include "clan.h"
#include "constants.h"

/* extern variables */
extern struct zone_data *zone_table;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct spell_info_type *spells;
extern struct index_data *mob_index;
extern char *class_abbrevs[];
extern const float race_exp_multipliers[];
extern const float class_exp_multipliers[];
extern const int exp_table[];
extern int free_rent;
extern int pt_allowed;
extern int nameserver_is_slow;
extern int max_filesize;
extern char *pc_race_types[];
extern char *pc_class_types[];
extern char *scr_male_pc_class_types[];
extern char *scr_female_pc_class_types[];
extern int max_obj_save;
extern struct battle_zone battle;
extern int track_through_doors;
extern int max_obj_house;
extern struct house_control_rec house_control[];
extern int num_of_houses;
extern int xap_objs;
extern struct autoauction aauction;
extern char *scr_class_abbrevs[];


/* extern procedures */
ACMD(do_infobar); /* -naj infobar2 12/16/96 - infobar prototype */
ACMD(do_look);
ACMD(do_gen_comm);
ACMD(do_action);
ACMD(do_flush);
SPECIAL(shop_keeper);
void die(struct char_data * ch, struct char_data *killer);
void Crash_rentsave(struct char_data * ch, int cost);
void Crash_count_items(struct obj_data * obj, long *nitems);
int Crash_check_norents_equipped(struct char_data * ch);
int Crash_check_norents(struct obj_data * obj,struct char_data *ch);
void write_aliases(struct char_data *ch); /* Alias mod */
void list_skills(struct char_data * ch, int type);
void appear(struct char_data * ch);
void perform_immort_vis(struct char_data *ch);
int  skill_roll(struct char_data *ch, int skill_num, int penalty);
void log_corpse(struct char_data *ch,struct obj_data *cont,char *ztString);
void dismount_char(struct char_data *ch);
void id_obj_to_char(struct char_data *ch, struct obj_data *obj);
void id_char_to_char(struct char_data *ch, struct char_data *victim);
void prune_crlf(char *string);
int min_level(struct char_data *ch,int spellnum);
int  invalid_class(struct char_data *ch, struct obj_data *obj);
int  invalid_race(struct char_data *ch, struct obj_data *obj);

char *ztquit[] =
   {
      "Quit",
      "Quit!",
      "Camp",
      "Camp!"
   };

ACMD(do_quit)
   {
   room_rnum save_room,rnum;
   struct descriptor_data *d, *next_d;
   long nitems=0,has_norents;
   int i;

   if (IS_NPC(ch) || !ch->desc)
      return;

   if (subcmd == SCMD_QUI && GET_LEVEL(ch) < LVL_IMMORT)
      send_to_char(ch,"  You have to type <quit!> if you really want to quit"
                   " the mud,\n\r  and have all of your equipment fall "
                   "onto the ground. I suggest you\n\r  use the <camp> "
                   "command, see <help camp> for more information.\n\r");
   else if (GET_POS(ch) == POS_FIGHTING)
      send_to_char(ch, "No way!  You're fighting for your life!\r\n");
   else if (GET_POS(ch) < POS_STUNNED)
      {
      send_to_char(ch, "You die before your time...\r\n");
      die(ch,ch);
      }
   else if (((subcmd==SCMD_CAMP)||(subcmd==SCMD_CAMPR)) &&
            !OUTSIDE(ch) && !ROOM_FLAGGED(IN_ROOM(ch),ROOM_HOUSE) &&
            !ROOM_FLAGGED(IN_ROOM(ch),ROOM_CAMP) &&
            (GET_LEVEL(ch) < LVL_IMMORT))
      {
      send_to_char(ch, "How do you expect to set up a camp indoors?\r\n"
                   "You must be outside to camp.\r\n");
      return;
      }
   else if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_NO_CAMP)&&(GET_LEVEL(ch) < LVL_IMMORT))
      {
      send_to_char(ch,"You start to set up your tent, but then think "
                   "better of it.\r\n");
      return;
      }
   else if(SECT(IN_ROOM(ch))==SECT_FLYING)
      {
      send_to_char(ch, "How do you expect to set up a camp in the middle of"
                   " the air while\r\n"
                   " you are flying?  You must find solid ground.\r\n");
      return;
      }
   else
      {
      for(i=0;i<num_of_houses;i++)
         {
         if(str_cmp(GET_NAME(ch),house_control[i].owner)==0)
            {
            if((rnum=real_room(house_control[i].vnum))==-1)
               nitems=-1;
            else
               {
               nitems=0;
               Crash_count_items(world[rnum].contents,&nitems);
               }
            if((nitems>max_obj_house)&&(subcmd==SCMD_CAMP)&&
                    (GET_LEVEL(ch) < LVL_IMMORT))
               {
               send_to_char(ch, "You have %ld items in %s. There is a max of "
                            "%d.  It is suggested that you clear out this "
                            "room or it might be done for you.\r\n",
                            nitems,world[rnum].name,max_obj_house);
               return;
               }
            }

         }
      nitems=0;

      for(i=0;i<NUM_WEARS;i++)
         Crash_count_items(GET_EQ(ch,i),&nitems);
      Crash_count_items(ch->carrying,&nitems);

      /* find and scream about unrentables. then leave.*/
      has_norents=0;
      has_norents+=Crash_check_norents_equipped(ch);
      has_norents+=Crash_check_norents(ch->carrying,ch);

      if((has_norents>0)&&(subcmd==SCMD_CAMP))
         {
         send_to_char(ch,"You cannot camp because you have %ld items that are "
                      "not allowed in camp.  Remove them or use camp! to "
                      "automatically junk them.\r\n",has_norents);
         }
      else if((nitems>max_obj_save)&&(!PRF_FLAGGED(ch,PRF_NOHASSLE))&&
              ((subcmd==SCMD_CAMP)||(subcmd==SCMD_CAMPR)))
         {
         send_to_char(ch, "You cannot camp with more than %d items in your "
                      "camp!\r\nYou have %ld items on your person. Time to "
                      "donate some stuff!\r\n",max_obj_save,nitems);
         }
      else
         {

         /* nomikos 11/18/02 - take their gold if they bail an auction */
         if ((aauction.in_progress == TRUE) && (GET_IDNUM(ch) == aauction.bidder_id_num))
            {
            if (get_char_gold(ch) >= (aauction.last_bid / 3))
               charge_char_gold(ch, aauction.last_bid / 3);
            else
               {
               GET_GOLD(ch) = 0;
               GET_BANK_GOLD(ch) = 0;
               }
            send_to_char(ch, "The auctioneer drops into the room, beats you up, and "
                             "takes some of your money.\r\n");
            mudlogf(CMP, LVL_DGOD, TRUE, "Gold: The auctioneer has taken %ld "
                    "coins from %s for bailing on an auction.",
                    aauction.last_bid / 3, GET_NAME(ch));
            }

	 REMOVE_BIT(PLR_FLAGS(ch), PLR_LINKLOADED);
         write_aliases(ch); /* Alias mod */
         save_char(ch, IN_ROOM(ch));
         Crash_crashsave(ch);
         if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
            House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));

         if (PLR_FLAGGED(ch, PLR_FISHING) || PLR_FLAGGED(ch, PLR_FISH_ON))
            {
            REMOVE_BIT(PLR_FLAGS(ch), PLR_FISHING | PLR_FISH_ON);
            send_to_char(ch, "You pack up your fishing gear and call it a day.\r\n");
            }

         if (!GET_INVIS_LEV(ch))
            act("$n sets up $s tent and camps.", TRUE, ch, 0, 0, TO_ROOM);
         mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                 "%s has left the game using %s.", GET_NAME(ch),
                 ztquit[subcmd]);
         if (GET_INVIS_LEV(ch) == 0) {
            send_info("[ INFO ] %s has left the game.\n\r", GET_NAME(ch));
         }
         send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");
         /*
          * kill off all sockets connected to the same player as the one who is
          * trying to quit.  Helps to maintain sanity as well as prevent duping.
          */
         for (d = descriptor_list; d; d = next_d)
            {
            next_d = d->next;
            if (d == ch->desc)
               continue;
            if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
               STATE(d)=CON_DISCONNECT;
            }

         save_room = IN_ROOM(ch);
         GET_LOADROOM(ch)=GET_ROOM_VNUM(IN_ROOM(ch));
         if (free_rent&&((subcmd==SCMD_CAMP)||(subcmd==SCMD_CAMPR)))
            Crash_rentsave(ch, 0);
         else
            send_to_char(ch, "You just left the game leaving all your stuff "
                         "on the ground, hope this is what you want.\r\n");
         extract_char(ch);  /* Char is saved in extract char */

         /* If someone is quitting in their house, let them load back here */
         save_char(ch, save_room);
         }
      }
   }


ACMD(do_save)
   {
   long nitems=0;
   int i, xap_objs_backup;
   if (IS_NPC(ch) || !ch->desc)
      return;

   skip_spaces(&argument);

   /* Only tell the char if they actually typed save */
   if (cmd)
      {
      send_to_char(ch,"Saving %s.\r\n", GET_NAME(ch));
      }

   write_aliases(ch); /* Alias mod */
   save_char(ch, IN_ROOM(ch));
   Crash_crashsave(ch);
   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
      House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
   if(*argument)
      {
      if(is_abbrev(argument,"reimburse"))
         {
         for(i=0;i<NUM_WEARS;i++)
            Crash_count_items(GET_EQ(ch,i),&nitems);
         Crash_count_items(ch->carrying,&nitems);

         if((nitems>max_obj_save)&&(GET_LEVEL(ch) < LVL_IMMORT))
            {
            send_to_char(ch, "You cannot have more than %d items to save reimburse.\r\n"
                             "Clear up some of the %ld items in your inventory and try again.\r\n",
                             max_obj_save, nitems);
            return;
            }

         xap_objs_backup=xap_objs;
         xap_objs = 2;
         Crash_crashsave(ch);
         xap_objs = xap_objs_backup;
         if (cmd)
            {
            send_to_char(ch, "Reimb file saved.\r\n");
            }
         }
      }

   }


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
/* made less generic, but still generic enough - nomikos 5/28/03 */
ACMD(do_not_here)
   {
   if (cmd == find_command("balance") ||
       cmd == find_command("withdraw") ||
       cmd == find_command("deposit") ||
       cmd == find_command("cbalance") ||
       cmd == find_command("cwithdraw") ||
       cmd == find_command("cdeposit"))
      send_to_char(ch, "You can only do that at the bank!\r\n");
   else if (cmd == find_command("divine"))
      send_to_char(ch, "There are no sages here to divine for you!\r\n");
   else if (cmd == find_command("value") ||
            cmd == find_command("sell") ||
            cmd == find_command("buy"))
      send_to_char(ch, "You cannot do that here, find a merchant!\r\n");
   else if (cmd == find_command("stable"))
      send_to_char(ch, "Look for some stables if you wish to stable your horse.\r\n");
   else if (cmd == find_command("exchange"))
      send_to_char(ch, "You can only exchange your ticket at a stable!\r\n");
   else if (cmd == find_command("reward"))
      send_to_char(ch, "If you wish to trade in your quest points, find the questmaster!\r\n");
   else if (cmd == find_command("rent") ||
            cmd == find_command("offer"))
      {
      if (free_rent)
         send_to_char(ch, "You don't need to rent a room, just camp for free!\r\n");
      else
         send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
      }
   else if (cmd == find_command("cost") ||
            cmd == find_command("check") ||
            cmd == find_command("move"))
      send_to_char(ch, "Find a hometown receptionist to do that!\r\n");
   else if (cmd == find_command("receive") ||
            cmd == find_command("check") ||
            cmd == find_command("mail"))
      send_to_char(ch, "You can only do that at the postmaster.\r\n");
   else if (cmd == find_command("learn") ||
            cmd == find_command("gain"))
      send_to_char(ch, "Only a guildmaster can help you do that.\r\n");
   else if (cmd == find_command("heal"))
      send_to_char(ch, "You can only be healed by a healer.\r\n");
   else if (cmd == find_command("fix"))
      send_to_char(ch, "You cannot do that here, find a repairman!\r\n");
   else if (cmd == find_command("bet"))
      send_to_char(ch, "Find a roulette wheel and you can bet, but not here!\r\n");
   else if (cmd == find_command("butcher"))
      send_to_char(ch, "You can't butcher things!\r\n");
   else if (cmd == find_command("list"))
      send_to_char(ch, "You cannot do that here, find a merchant or repairman!\r\n");
   else if (cmd == find_command("fight") ||
            cmd == find_command("safe") ||
            cmd == find_command("how"))
      send_to_char(ch, "Find the Battle Master to do that!\r\n");
   else
      send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
   }

ACMD(do_identify)
   {
   int bits;
   struct char_data *found_char = NULL;
   struct obj_data *found_obj = NULL;

   if (GET_LEVEL(ch) >= LVL_IMMORT)
      {
      skip_spaces(&argument);
      bits = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
                          FIND_CHAR_ROOM, ch, &found_char, &found_obj);

      if (bits)
         {
         if (found_obj != NULL)
            id_obj_to_char(ch, found_obj);
         else if (found_char != NULL)
            id_char_to_char(ch, found_char);
         else
            send_to_char(ch,"You do not see that here.\r\n");
         }
      else
         send_to_char(ch,"What do you wish to identify?\r\n");
      }
   else
      send_to_char(ch, "Sorry, but you need a scroll to identify things!\r\n");
   }
/**4/11/97 Anduin recall function **/
ACMD(do_recall)
   {
   if(IS_NPC(ch))
      {
      act("$n disappears.", TRUE, ch, 0, 0, TO_ROOM);
      char_from_room(ch);
      char_to_room(ch, ch->orig_room);
      act("$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM);
      return;
      }

   if (ch->char_specials.in_battle == TRUE)
      {
      send_to_char(ch, "You can not recall out of battle!!\r\n");
      return;
      }

   if (ch->char_specials.fighting)
      {
      send_to_char(ch, "The gods have forsaken you!!\r\n");
      return;
      }

   if (IS_SET(world[IN_ROOM(ch)].room_flags, ROOM_NO_RECALL)||
           Z_FLAGGED(IN_ROOM(ch),Z_NO_RECALL))
      {
      send_to_char(ch, "The gods have forsaken you!!\r\n");
      return;
      }

   if((GET_LEVEL(ch)>15)&&(!PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      send_to_char(ch, "You forgot your scroll, didn't you?  Ready for a stroll?\r\n");
      return;
      }

   act("$n disappears.", TRUE, ch, 0, 0, TO_ROOM);
   GET_MOVE(ch) = (int)(GET_MOVE(ch)/2);
   if (GET_MOVE(ch) <= 0)
      GET_MOVE(ch) = 1;
   dismount_char(ch);
   char_from_room(ch);
   char_to_room(ch, real_room(GET_HOME(ch)));
   act("$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "It was free that time, but at level 16 you will need "
                "a scroll.\r\n");
   do_look(ch,"",0,0);
   }

ACMD(do_clan_recall)
   {
   if(IS_NPC(ch))
      return;

   if (ch->char_specials.in_battle == TRUE)
      {
      send_to_char(ch, "You can not recall out of battle!!\r\n");
      return;
      }

   if (ch->char_specials.fighting)
      {
      send_to_char(ch, "The gods have forsaken you!!\r\n");
      return;
      }

   if (IS_SET(world[IN_ROOM(ch)].room_flags, ROOM_NO_RECALL)||
           Z_FLAGGED(IN_ROOM(ch),Z_NO_RECALL))
      {
      send_to_char(ch, "The gods have forsaken you!!\r\n");
      return;
      }

   if((GET_CLAN(ch)==0)&&(!PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      send_to_char(ch, "You are not in a clan!\r\n");
      return;
      }

   act("$n disappears.", TRUE, ch, 0, 0, TO_ROOM);
   GET_MOVE(ch) = (int)(GET_MOVE(ch)/2);
   if (GET_MOVE(ch) <= 0)
      GET_MOVE(ch) = 1;
   dismount_char(ch);
   char_from_room(ch);
   char_to_room(ch, real_room(GET_CLAN_ROOM(ch)));
   act("$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM);
   do_look(ch,"",0,0);
   }


ACMD(do_sneak)
   {
   struct affected_type af;
   byte percent;
   int skill;

   send_to_char(ch, "Okay, you'll try to move silently for a while.\r\n");

   if (AFF_FLAGGED(ch, AFF_SNEAK))
      affect_from_char(ch, SKILL_SNEAK);

   if ((!IS_NPC(ch)&&GET_SKILL(ch, SKILL_SNEAK) <= 0) || !SCR_SKILLCHECK(ch, SKILL_SNEAK))
      {
      send_to_char(ch,"You fail miserably.\r\n");
      return;
      }

   percent = number(1, 101); /* 101% is a complete failure */
   if(IS_NPC(ch))
      skill = MIN(95,GET_LEVEL(ch)*2);
   else
      skill = GET_SKILL(ch, SKILL_SNEAK);

   if(percent > skill + dex_app_skill[stat_index(GET_DEX(ch))].sneak)
      {
      improve_skill(ch,SKILL_SNEAK,USE_FAIL);
      return;
      }

   improve_skill(ch,SKILL_SNEAK,USE_PASS);
   af.type = SKILL_SNEAK;
   af.duration = GET_LEVEL(ch);
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bitvector = AFF_SNEAK;
   affect_to_char(ch, &af);
   }


ACMD(do_rove)
   {
   struct affected_type af;
   byte percent;
   int skill;

   send_to_char(ch, "Okay, you'll try to move silently for a while.\r\n");

   if (AFF2_FLAGGED(ch, AFF2_ROVE))
      affect_from_char(ch, SKILL_ROVE);

   if ((!IS_NPC(ch)&&GET_SKILL(ch, SKILL_ROVE) <= 0) || !SCR_SKILLCHECK(ch, SKILL_ROVE))
      {
      send_to_char(ch,"You fail miserably.\r\n");
      return;
      }

   percent = number(1, 101); /* 101% is a complete failure */
   if(IS_NPC(ch))
      skill = MIN(95,GET_LEVEL(ch)*2);
   else
      skill = GET_SKILL(ch, SKILL_ROVE);

   if(percent > skill + dex_app_skill[stat_index(GET_DEX(ch))].sneak)
      {
      improve_skill(ch,SKILL_ROVE,USE_FAIL);
      return;
      }

   improve_skill(ch,SKILL_ROVE,USE_PASS);
   af.type = SKILL_ROVE;
   af.duration = GET_LEVEL(ch)/3;
   af.modifier = 0;
   af.location = APPLY_AFF2;
   af.bitvector = AFF2_ROVE;
   affect_to_char(ch, &af);
   }



ACMD(do_hide)
   {
   if (ch && FIGHTING(ch)) {
     send_to_char(ch, "Hide while fighting?  That doesn't make sense.\r\n");
     return;
   }

   send_to_char(ch, "You attempt to hide yourself.\r\n");

   if ((!IS_NPC(ch)&&GET_SKILL(ch, SKILL_HIDE) <= 0) || !SCR_SKILLCHECK(ch, SKILL_HIDE))
      {
      send_to_char(ch,"You fail miserably.\r\n");
      return;
      }

   if (AFF_FLAGGED(ch, AFF_HIDE))
      REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);


   if (!skill_roll(ch, SKILL_HIDE,dex_app_skill[stat_index(GET_DEX(ch))].hide))
      {
      improve_skill(ch,SKILL_HIDE,USE_FAIL);
      return;
      }
   improve_skill(ch,SKILL_HIDE,USE_PASS);
   SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
   }


/* Away from guildmasters, shows current skills and spells.
   Added ability to see either skills or spells - Nomikos 9-17-2025 */
ACMD(do_practice)
   {
   char *arg=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, arg);

   if (*arg)
      {
	  if (is_abbrev(arg, "skills"))
	     list_skills(ch, IS_SKILL);
      else if (is_abbrev(arg, "spells"))
	     list_skills(ch, IS_SPELL);
	  else
         send_to_char(ch, "You can only practice skills in your guild.\r\n");
	  }
   else if (IS_NPC(ch))
      send_to_char(ch, "You practice your mob skills and feel pretty confident in yourself.\r\n");
   else
      list_skills(ch, IS_UNUSED);
   release_buffer(arg);
   }



ACMD(do_visible)
   {
   if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT)
      {
      perform_immort_vis(ch);
      return;
      }

   if AFF_FLAGGED(ch, AFF_INVISIBLE)
      {
      appear(ch);
      send_to_char(ch, "You break the spell of invisibility.\r\n");
      }
   else if AFF2_FLAGGED(ch, AFF2_SHADOW)
      {
      appear(ch);
      send_to_char(ch, "You come out of the shadows.\r\n");
      }
   else
      send_to_char(ch, "You are already visible.\r\n");
   }



ACMD(do_title)
   {
   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   skip_spaces(&argument);
   delete_doubledollar(argument);

   if (IS_NPC(ch))
      send_to_char(ch, "Your title is fine... go away.\r\n");
   else if (PLR_FLAGGED(ch, PLR_NOTITLE))
      send_to_char(ch,"You can't title yourself -- you shouldn't have abused it!\r\n");
   else if (strstr(argument, "(") || strstr(argument, ")"))
      send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
   else if (strlen(argument) > MAX_TITLE_LENGTH)
      {
      send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n",
                   MAX_TITLE_LENGTH);
      }
   else
      {
      set_title(ch, argument);
      send_to_char(ch, "Okay, you're now %s %s.\r\n", GET_NAME(ch),
                   GET_TITLE(ch));
      }
   }


/*
void check_group_inspire(struct char_data *ch, int add_affect)
{
  if (!ch || !AFF_FLAGGED(ch, AFF_GROUP)) {
    return;
  }

  struct char_data *leader = ch->master ? ch->master : ch;
  struct follow_type *f;

  int total_bards = (!IS_NPC(leader) && GET_CLASS(leader) == CLASS_BARD) ? 1 : 0;
  int max_bard_levels = total_bards > 0 ? GET_LEVEL(leader) : 0;
  affect_from_char(leader, SPELL_GROUP_INSPIRE);
  log("removing affect from leader %s", GET_NAME(leader));

  for (f = leader->followers; f; f = f->next) {
    if (!IS_NPC(f->follower) && GET_CLASS(f->follower) == CLASS_BARD) {
      total_bards++;
      max_bard_levels = MAX(max_bard_levels, GET_LEVEL(f->follower));
    }
    affect_from_char(ch, SPELL_GROUP_INSPIRE);
    log("removing affect from follower %s", GET_NAME(ch));
  }

  if (!add_affect) {
    return;
  }

  if (total_bards > 0) {
    struct affected_type aff;
    aff.type = SPELL_GROUP_INSPIRE;
    aff.duration = -1;
    aff.bitvector = 0;
    aff.modifier = max_bard_levels/20;
    aff.location = APPLY_DAMROLL;
    affect_join(leader, &aff, FALSE, FALSE, FALSE, FALSE);
    aff.modifier = max_bard_levels/20;
    aff.location = APPLY_HITROLL;
    affect_join(leader, &aff, FALSE, FALSE, FALSE, FALSE);
    log("joining with leader %s", GET_NAME(leader));
    for (f = leader->followers; f; f = f->next) {
      aff.modifier = max_bard_levels/20;
      aff.location = APPLY_DAMROLL;
      affect_join(f->follower, &aff, FALSE, FALSE, FALSE, FALSE);
      aff.modifier = max_bard_levels/20;
      aff.location = APPLY_HITROLL;
      affect_join(f->follower, &aff, FALSE, FALSE, FALSE, FALSE);
      log("joining with follower %s", GET_NAME(f->follower));
    }
  }
}
*/

int perform_group(struct char_data *ch, struct char_data *vict)
   {
   if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
      return (0);

   SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
   if (ch != vict)
      act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
   act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
   act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);

   return (1);
   }


/* Updated Nomikos 1/29/2026 to allow players to see who are following them outside of the 
   group. A group member is also able to see if someone is following them outside of the 
   group structure 
   Note: 2/16/2026 if the leader follows someone this leads to weird behavior */

#define GRP     0
#define CH_FOL  1
#define PC_FOL  2
#define NPC_FOL 3

void print_group(struct char_data *ch)
   {
   struct char_data *k;
   struct follow_type *f;
   int ftype[200] = { 0 };
   int msg[4] = { 1,0,0,0 };
   int ii, jj;

   char *buf=get_buffer(MAX_STRING_LENGTH);

   /* Give grouped message, let you know who's the head of the group, and list members of group */
   if (!AFF_FLAGGED(ch, AFF_GROUP))
      {
      // ch is not the leader and is not grouped
      if (ch->master)
         send_to_char(ch, "You are following %s.\r\n", GET_NAME(ch->master));
      else
         send_to_char(ch, "But you are not the member of a group!\r\n");
      }
   else
      {
      /* only check for master in here, since the group is only shown here */
      k = (ch->master ? ch->master : ch);

      send_to_char(ch, "Your group consists of:\r\n");

      char *buf2 = get_buffer(MAX_STRING_LENGTH);
      buf2[0] = '\x0';
      if (GET_NUM_GUARDING_ME(k) > 0)
         {
         sprintf(buf2, "(guarded by ");
         for (int i = 0; i < GET_NUM_GUARDING_ME(k)-1; i++)
            {
            strcat(buf2, GET_NAME(GET_GUARDING_ME(k)[i]));
            strcat(buf2, ", ");
            }
         strcat(buf2, GET_NAME(GET_GUARDING_ME(k)[GET_NUM_GUARDING_ME(k)-1]));
         strcat(buf2, ")");
         }
      sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N (Head of group) %s",
              GET_HIT(k),GET_MANA(k),GET_MOVE(k),GET_LEVEL(k),
              CLASS_ABBR(k), buf2);
      act(buf, FALSE, ch, 0, k, TO_CHAR);
      
      /* Cycle through grouped followers of leader */
      for (f = k->followers; f; f = f->next)
         {
         if (!AFF_FLAGGED(f->follower, AFF_GROUP))
            continue;

         /* Print the guarded by stuff */
         struct char_data *tch = f->follower;
         buf2[0] = '\x0';
         if (GET_NUM_GUARDING_ME(tch) > 0)
            {
            sprintf(buf2, "(guarded by ");
            for (int i = 0; i < GET_NUM_GUARDING_ME(tch)-1; i++)
               {
               strcat(buf2, GET_NAME(GET_GUARDING_ME(tch)[i]));
               strcat(buf2, ", ");
               }
  	         strcat(buf2, GET_NAME(GET_GUARDING_ME(tch)[GET_NUM_GUARDING_ME(tch)-1]));
            strcat(buf2, ")");
            }

         /* Send a line for each follower */
         sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N %s",
                 GET_HIT(f->follower),GET_MANA(f->follower),
                 GET_MOVE(f->follower),GET_LEVEL(f->follower),
                 CLASS_ABBR(f->follower), buf2);
         act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
         }
      release_buffer(buf2);
      }

   /* default back to show the ungrouped followers of the person that used the group command */
   k = ch;

   /* Cycle through the followers and tally their type */
   for (ii = 0, f = k->followers; f && ii < 200; f = f->next, ii++)
      {
      if (AFF_FLAGGED(f->follower, AFF_GROUP))
         ftype[ii] = GRP;
      else if (AFF_FLAGGED(f->follower, AFF_CHARM))
         ftype[ii] = CH_FOL;
      else if (IS_NPC(f->follower))
         ftype[ii] = NPC_FOL;
      else
         ftype[ii] = PC_FOL;
      }

   /* Cycle through the follow types, group them together */
   for (ii = 1; ii < 4; ii++)
      {
      /* Cycle through followers and show them if they're the right kind */
      for (jj = 0, f = k->followers; f && jj < 200; f = f->next, jj++)
         {
         if (ftype[jj] != ii)
            continue;

         /* Print the follower type message once */
         if (!msg[ii])
            {
            msg[ii] = 1;
            switch (ftype[jj])
               {
               case CH_FOL:  send_to_char(ch, "Charmed followers:\r\n"); break;
               case PC_FOL:  send_to_char(ch, "Player followers:\r\n"); break;
               case NPC_FOL: send_to_char(ch, "NPC followers:\r\n"); break;
               default: break;
               }
            }
 
         /* Print the guarded by stuff */
         struct char_data *tch = f->follower;
         char *buf2 = get_buffer(MAX_STRING_LENGTH);
         buf2[0] = '\x0';
         if (GET_NUM_GUARDING_ME(tch) > 0)
            {
            sprintf(buf2, "(guarded by ");
            for (int i = 0; i < GET_NUM_GUARDING_ME(tch)-1; i++)
               {
               strcat(buf2, GET_NAME(GET_GUARDING_ME(tch)[i]));
               strcat(buf2, ", ");
               }
  	         strcat(buf2, GET_NAME(GET_GUARDING_ME(tch)[GET_NUM_GUARDING_ME(tch)-1]));
            strcat(buf2, ")");
            }

         /* Send a line for each follower */
         sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N %s",
                 GET_HIT(f->follower),GET_MANA(f->follower),
                 GET_MOVE(f->follower),GET_LEVEL(f->follower),
                 CLASS_ABBR(f->follower), buf2);
         act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
         release_buffer(buf2);
         }
      }
   release_buffer(buf);
   }



ACMD(do_group)
   {
   struct char_data *vict;
   struct follow_type *f;
   int found;
   char *buf=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, buf);

   if (!*buf)
      {
      print_group(ch);
      }
   else if (ch->master)
      {
      send_to_char(ch, "You can not enroll group members without being "
                   "head of a group.\r\n");
      }
   else if (!str_cmp(buf, "all"))
      {
      perform_group(ch, ch);
      for (found = 0, f = ch->followers; f; f = f->next)
         found += perform_group(ch, f->follower);
      if (!found)
         send_to_char(ch, "Everyone following you is already in your group.\r\n");
      }
   else if (!(vict = get_char_vis(ch, buf,FIND_CHAR_ROOM)))
      send_to_char(ch, "%s", NOPERSON);
   else if ((vict->master != ch) && (vict != ch))
      act("$N must follow you to enter your group.", FALSE, ch, 0, vict,
          TO_CHAR);
   else
      {
      if (!AFF_FLAGGED(vict, AFF_GROUP))
         {
         if((ch!=vict)&&!AFF_FLAGGED(ch,AFF_GROUP))
            send_to_char(ch, "You have to be in the group before you can "
                         "add people to it!!\r\n");
         else
            perform_group(ch, vict);
         }
      else
         {
         if(ch==vict)
            send_to_char(ch, "Use 'ungroup' to disband your group.\r\n");
         else
            {
            if (ch != vict)
               act("$N is no longer a member of your group.", FALSE, ch, 0,
                   vict, TO_CHAR);
            act("You have been kicked out of $n's group!", FALSE, ch, 0, vict,
                TO_VICT);
            act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict,
                TO_NOTVICT);
            REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
            }
         }
      }
   release_buffer(buf);
   }



ACMD(do_ungroup)
   {
   struct follow_type *f, *next_fol;
   struct char_data *tch;
   char *buf=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, buf);

   if (!*buf)
   {
     if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP)))
     {
       send_to_char(ch, "But you lead no group!\r\n");
     }
     else
     {
       for (f = ch->followers; f; f = next_fol)
       {
	 next_fol = f->next;
	 if (AFF_FLAGGED(f->follower, AFF_GROUP))
	 {
	   REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
	   send_to_char(f->follower, "%s has disbanded the group.\r\n", GET_NAME(ch));
	   if (!AFF_FLAGGED(f->follower, AFF_CHARM))
	     stop_follower(f->follower);
	 }
       }
       REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
       send_to_char(ch, "You disband the group.\r\n");
     }
   }
   else if (!(tch = get_char_vis(ch, buf,FIND_CHAR_ROOM)))
   {
     send_to_char(ch, "There is no such person!\r\n");
   }
   else if (tch->master != ch)
   {
     send_to_char(ch, "That person is not following you!\r\n");
   }
   else if (!AFF_FLAGGED(tch, AFF_GROUP))
   {
     send_to_char(ch, "That person isn't in your group.\r\n");
   }
   else
   {
     REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

     act("$N is no longer a member of your group.",FALSE,ch,0,tch,TO_CHAR);
     act("You have been kicked out of $n's group!",FALSE,ch,0,tch,TO_VICT);
     act("$N has been kicked out of $n's group!",  FALSE,ch,0,tch,TO_NOTVICT);

     if (!AFF_FLAGGED(tch, AFF_CHARM))
       stop_follower(tch);
   }
   release_buffer(buf);
}



ACMD(do_report)
   {
   char *buf;
   int exp;
   exp = (int)(GET_EXP_FOR_CH(ch)
               - GET_EXP(ch));
   buf=get_buffer(SMALL_BUFSIZE);
   sprintf(buf, "$n reports: %d/%dH, %d/%dM, %d/%dV %dX\r\n",
           GET_HIT(ch), GET_MAX_HIT(ch),
           GET_MANA(ch), GET_MAX_MANA(ch),
           GET_MOVE(ch), GET_MAX_MOVE(ch),exp);
   act(buf,FALSE,ch,0,0,TO_ROOM);
   release_buffer(buf);

   send_to_char(ch, "You report: %d/%dH, %d/%dM, %d/%dV %dX\r\n",
                GET_HIT(ch), GET_MAX_HIT(ch),
                GET_MANA(ch), GET_MAX_MANA(ch),
                GET_MOVE(ch), GET_MAX_MOVE(ch),exp);
   }


ACMD(do_greport)
   {
   struct char_data *k;
   struct follow_type *f;
   char *buf;
   int exp;
   exp = (int)(GET_EXP_FOR_CH(ch)
               - GET_EXP(ch));

   if (!AFF_FLAGGED(ch, AFF_GROUP))
      {
      send_to_char(ch, "But you are not a member of any group!\r\n");
      return;
      }
   buf=get_buffer(SMALL_BUFSIZE);
   sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV %dX\r\n",
           GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
           GET_MANA(ch), GET_MAX_MANA(ch),
           GET_MOVE(ch), GET_MAX_MOVE(ch),exp);

   CAP(buf);

   k = (ch->master ? ch->master : ch);

   for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
         send_to_char(f->follower, "%s",buf);
   if (k != ch)
      send_to_char(k, "%s",buf);
   send_to_char(ch, "You report to the group.\r\n");
   release_buffer(buf);
   }


ACMD(do_split)
   {
   int amount, num, share;
   struct char_data *k;
   struct follow_type *f;
   char *buf=get_buffer(MAX_INPUT_LENGTH);

   if (IS_NPC(ch))
      return;

   one_argument(argument, buf);

   if (is_number(buf))
      {
      amount = atoi(buf);
      release_buffer(buf);
      if (amount <= 0)
         {
         send_to_char(ch, "Sorry, you can't do that.\r\n");
         return;
         }
      if (amount > GET_GOLD(ch))
         {
         send_to_char(ch,"You don't seem to have that "
                      "much gold to split.\r\n");
         return;
         }
      k = (ch->master ? ch->master : ch);

      if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
         num = 1;
      else
         num = 0;

      for (f = k->followers; f; f = f->next)
         if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
                 (!IS_NPC(f->follower)) &&
                 (IN_ROOM(f->follower) == IN_ROOM(ch)))
            num++;

      if (num && AFF_FLAGGED(ch, AFF_GROUP))
         {
         share = amount / num;
         }
      else
         {
         send_to_char(ch, "With whom do you wish to share your gold?\r\n");
         return;
         }

      GET_GOLD(ch) -= share * (num - 1);

      if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch))
              && !(IS_NPC(k)) && k != ch)
         {
         GET_GOLD(k) += share;
         send_to_char(k, "%s splits %d coins; you receive %d.\r\n",
                      GET_NAME(ch), amount, share);
         }

      for (f = k->followers; f; f = f->next)
         {
         if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
                 (!IS_NPC(f->follower)) &&
                 (IN_ROOM(f->follower) == IN_ROOM(ch)) &&
                 f->follower != ch)
            {
            GET_GOLD(f->follower) += share;
            send_to_char(f->follower,"%s splits %d coins; you receive %d.\r\n",
                         GET_NAME(ch), amount, share);
            }
         }
      send_to_char(ch, "You split %d coins among %d members --"
                   " %d coins each.\r\n", amount, num, share);
      }
   else
      {
      send_to_char(ch,"How many coins do you wish to split with your group?\r\n");
      release_buffer(buf);
      return;
      }
   }



ACMD(do_use)
   {
   struct obj_data *mag_item;
   struct obj_data *mag_item2;
   char *buf=get_buffer(MAX_INPUT_LENGTH);
   char *arg=get_buffer(MAX_INPUT_LENGTH);


   half_chop(argument, arg, buf);
   if (!*arg)
      {
      send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
      release_buffer(arg);
      release_buffer(buf);
      return;
      }
   mag_item=NULL;
   mag_item2=NULL;

   if(IS_NPC(ch) && AFF_FLAGGED(ch,AFF_CHARM)) {
      send_to_char(ch, "You can't seem to figure out how to do that.\r\n");
      release_buffer(arg);
      release_buffer(buf);
      return;
      }

   if(GET_EQ(ch,WEAR_HOLD_1))
      mag_item = GET_EQ(ch, WEAR_HOLD_1);
   if(GET_EQ(ch,WEAR_HOLD_2))
      mag_item2 = GET_EQ(ch, WEAR_HOLD_2);

   if(mag_item2)
      if(isname(arg,mag_item2->name))
         {
         mag_item=mag_item2;
         }
   if((!mag_item)||(!isname(arg,mag_item->name)))
      {
      switch (subcmd)
         {
      case SCMD_SWALLOW:  /* Pill modification--Aleks */
      case SCMD_RECITE:
      case SCMD_QUAFF:
         if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying)))
            {
            send_to_char(ch,"You don't seem to have %s %s.\r\n",
                         AN(arg), arg);
            release_buffer(arg);
            release_buffer(buf);
            return;
            }
         break;
      case SCMD_USE:
         send_to_char(ch,"You don't seem to be holding %s %s.\r\n",AN(arg),
                      arg);
         release_buffer(arg);
         release_buffer(buf);
         return;
         break;
      default:
         log("SYSERR: Unknown subcmd: %d passed to do_use",subcmd);
         release_buffer(arg);
         release_buffer(buf);
         return;
         break;
         }
      }
   release_buffer(arg);
   switch (subcmd)
      {
   case SCMD_QUAFF:
      if (GET_OBJ_TYPE(mag_item) != ITEM_POTION)
         {
         send_to_char(ch,"You can only quaff potions.\r\n");
         release_buffer(buf);
         return;
         }
      break;
      /* New case for Pill modification--Aleks */
   case SCMD_SWALLOW:
      if (GET_OBJ_TYPE(mag_item) != ITEM_PILL)
         {
         send_to_char(ch,"You can only swallow pills.\r\n");
         release_buffer(buf);
         return;
         }
      break;
   case SCMD_RECITE:
      if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL)
         {
         send_to_char(ch, "You can only recite scrolls.\r\n");
         release_buffer(buf);
         return;
         }
      if(!IS_NPC(ch) && GET_SKILL(ch,SKILL_READ_MAGIC)<95)
         {
         GET_SKILL(ch,SKILL_READ_MAGIC)=95;
         GET_SKILL_LEARN(ch,SKILL_READ_MAGIC)=0;
         }
      if(!skill_roll(ch,SKILL_READ_MAGIC,-5))
         {
         send_to_char(ch,"You stumble over the magical incantation and "
                      "the spell fails.\r\n");
         release_buffer(buf);
         if(mag_item!=NULL)
            extract_obj(mag_item);
         return;
         }

      break;
   case SCMD_USE:
      if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
              (GET_OBJ_TYPE(mag_item) != ITEM_STAFF))
         {
         send_to_char(ch, "You can't seem to figure out how to use it.\r\n");
         release_buffer(buf);
         return;
         }
      break;
      }


   mag_objectmagic(ch, mag_item, buf);
   release_buffer(buf);
   }



ACMD(do_wimpy)
   {
   int wimp_lev;
   char *arg;

   /* 'wimp_level' is a player_special. -gg 2/25/98 */
   if (IS_NPC(ch))
      return;

   arg=get_buffer(MAX_INPUT_LENGTH);
   one_argument(argument, arg);

   if (!*arg)
      {
      if (GET_WIMP_LEV(ch))
         {
         send_to_char(ch, "Your current wimp level is %d hit points.\r\n",
                      GET_WIMP_LEV(ch));
         }
      else
         {
         send_to_char(ch,"At the moment, you're not a wimp.  (sure, sure...)\r\n");
         }
      release_buffer(arg);
      return;
      }
   if (isdigit((int)*arg))
      {
      if ((wimp_lev = atoi(arg))!=0)
         {
         if (wimp_lev < 0)
            send_to_char(ch,"Heh, heh, heh.. we are jolly funny today, eh?\r\n");
         else if (wimp_lev > GET_MAX_HIT(ch))
            send_to_char(ch,"That doesn't make much sense, now does it?\r\n");
         else if (wimp_lev > (GET_MAX_HIT(ch)/ 2))
            send_to_char(ch, "You can't set your wimp level above half your hit points.\r\n");
         else
            {
            send_to_char(ch, "Okay, you'll wimp out if you drop below %d "
                         "hit points.\r\n", wimp_lev);
            GET_WIMP_LEV(ch) = wimp_lev;
            }
         }
      else
         {
         send_to_char(ch, "Okay, you'll now tough out fights to the bitter end.\r\n");
         GET_WIMP_LEV(ch) = 0;
         }
      }
   else
      send_to_char(ch, "Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n");

   release_buffer(arg);
   }


ACMD(do_display)
   {
   size_t i;
   char *usage = "Usage: prompt { { H | M | V | A | G | X | T | E } | all | none }\r\n";
   char *immusage = "Usage: prompt { { H | M | V | A | G | X | T | E | 1 | 2 } | all | none }\r\n";

   if (IS_NPC(ch))
      {
      send_to_char(ch, "Mosters don't need displays.  Go away.\r\n");
      return;
      }
   skip_spaces(&argument);

   if (!*argument)
      {
      if (GET_LEVEL(ch) < LVL_IMMORT)
         send_to_char(ch, "%s", usage);
      else
         send_to_char(ch, "%s", immusage);
      return;
      }
   if ((!str_cmp(argument, "on")) || (!str_cmp(argument, "all")))
      {
      SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
      SET_BIT(PRF2_FLAGS(ch), PRF2_DISPGOLD | PRF2_DISPEXP | PRF2_DISPALIGN |
		                      PRF2_DISPMAX | PRF2_DISPEXPLORED);
      }
   else
      {
      REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
      REMOVE_BIT(PRF2_FLAGS(ch), PRF2_DISPGOLD | PRF2_DISPEXP | PRF2_DISPALIGN | PRF2_DISPMAX);
      REMOVE_BIT(PRF2_FLAGS(ch), PRF2_DISPTIME | PRF2_DISPDATE | PRF2_DISPEXPLORED);

      for (i = 0; i < strlen(argument); i++)
         {
         switch (LOWER(argument[i]))
            {
         case 'h':
            SET_BIT(PRF_FLAGS(ch), PRF_DISPHP);
            break;
         case 'm':
            SET_BIT(PRF_FLAGS(ch), PRF_DISPMANA);
            break;
         case 'v':
            SET_BIT(PRF_FLAGS(ch), PRF_DISPMOVE);
            break;
         case 'a':
            SET_BIT(PRF2_FLAGS(ch), PRF2_DISPALIGN);
            break;
         case 'g':
            SET_BIT(PRF2_FLAGS(ch), PRF2_DISPGOLD);
            break;
         case 'x':
            SET_BIT(PRF2_FLAGS(ch), PRF2_DISPEXP);
            break;
		 case 'e':
            SET_BIT(PRF2_FLAGS(ch), PRF2_DISPEXPLORED);
            break;
         case 't':
            SET_BIT(PRF2_FLAGS(ch), PRF2_DISPMAX);
            break;
         case '1':
            if (GET_LEVEL(ch) >= LVL_IMMORT)
               SET_BIT(PRF2_FLAGS(ch), PRF2_DISPTIME);
            else
               send_to_char(ch, "%s", usage);
            break;
         case '2':
            if (GET_LEVEL(ch) >= LVL_IMMORT)
               SET_BIT(PRF2_FLAGS(ch), PRF2_DISPDATE);
            else
               send_to_char(ch, "%s", usage);
            break;
         default:
            if (GET_LEVEL(ch) < LVL_IMMORT)
               send_to_char(ch, "%s", usage);
            else
               send_to_char(ch, "%s", immusage);
            return;
            break;
            }
         }
      }
   send_to_char(ch, "%s", OK);
   }



ACMD(do_gen_write)
   {
   FILE *fl;
   char *tmp, *filename;
   char *buf;
   struct stat fbuf;
   time_t ct;
   char *bufptr;

   switch (subcmd)
      {
   case SCMD_BUG:
      filename = BUG_FILE;
      break;
   case SCMD_TYPO:
      filename = TYPO_FILE;
      break;
   case SCMD_IDEA:
      filename = IDEA_FILE;
      break;
   default:
      return;
      }

   ct = time(0);
   tmp = asctime(localtime(&ct));

   if (IS_NPC(ch))
      {
      send_to_char(ch,"Monsters can't have ideas - Go away.\r\n");
      return;
      }

   skip_spaces(&argument);
   delete_doubledollar(argument);

   if (!*argument)
      {
      send_to_char(ch,"That must be a mistake...\r\n");
      return;
      }
   bufptr=argument;
   while(*bufptr !='\0')
      {
      if(*bufptr=='%')
         *bufptr='_';
      bufptr++;
      }
   buf=get_buffer(MAX_STRING_LENGTH);
   sprintf(buf, "%s [%ld] %s: %s", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)),
           CMD_NAME, argument);
   mudlog(buf, CMP, (subcmd==SCMD_BUG)?GOD_LOG(ch):GET_INVIS_LEV(ch), FALSE);
   release_buffer(buf);

   if (stat(filename, &fbuf) < 0)
      {
      perror("SYSERR: Error statting file");
      return;
      }
   if (fbuf.st_size >= max_filesize)
      {
      send_to_char(ch,"Sorry, the file is full right now.. try again later.\r\n");
      return;
      }
   if (!(fl = fopen(filename, "a")))
      {
      perror("SYSERR: do_gen_write");
      send_to_char(ch,"Could not open the file.  Sorry.\r\n");
      return;
      }
   fprintf(fl, "Name: %s\n Date: %s\n RoomNum: %ld -- %s\n Comment: %s\n",
           GET_NAME(ch), (tmp + 4), GET_ROOM_VNUM(IN_ROOM(ch)),
           zone_table[world[IN_ROOM(ch)].zone].name, argument);
   fclose(fl);
   send_to_char(ch,"Okay.  Thanks!\r\n");
   }



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
   {
   long result;
   int i;

   char *tog_messages[][2] =
      {
         {"You are now safe from summoning by other players.\r\n",
          "You may now be summoned by other players.\r\n"
         } ,
         {"Nohassle disabled.\r\n",
          "Nohassle enabled.\r\n"
         } ,
         {"Brief mode off.\r\n",
          "Brief mode on.\r\n"
         } ,
         {"Compact mode off.\r\n",
          "Compact mode on.\r\n"
         } ,
         {"You can now hear tells.\r\n",
          "You are now deaf to tells.\r\n"
         } ,
         {"You can now hear auctions.\r\n",
          "You are now deaf to auctions.\r\n"
         } ,
         {"You can now hear shouts.\r\n",
          "You are now deaf to shouts.\r\n"
         } ,
         {"You can now hear gossip.\r\n",
          "You are now deaf to gossip.\r\n"
         } ,
         {"You can now hear the congratulation messages.\r\n",
          "You are now deaf to the congratulation messages.\r\n"
         } ,
         {"You can now hear the Wiz-channel.\r\n",
          "You are now deaf to the Wiz-channel.\r\n"
         } ,
         {"You are now deaf to Qsay.\r\n",
          "You can now hear Qsay.\r\n"
         } ,
         {"You will no longer see the room flags.\r\n",
          "You will now see the room flags.\r\n"
         } ,
         {"You will now have your communication repeated.\r\n",
          "You will no longer have your communication repeated.\r\n"
         } ,
         {"HolyLight mode off.\r\n",
          "HolyLight mode on.\r\n"
         } ,
         {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
          "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"
         } ,
         {"Autoexits disabled.\r\n",
          "Autoexits enabled.\r\n"
         } ,
         {"Ident is disabled.\r\n",
          "Ident is now disabled... but it was turned on. BUG?\r\n"
         } ,
         {"Infobar disabled.\r\n",
          "Infobar enabled.  There are supplemental commands:\r\n"
          "SCOREBAR toggles another, more detailed, infobar at the top of your screen.\r\n"
          "METER toggles a graph of your opponent's health during combat.\r\n"
          "RESIZE adjusts the infobar to properly fit your screen length.\r\n"
          "REDRAW will redraw your screen in case it gets garbled.\r\n"
          "ASCII toggles 8'th bit graphics characters. (nice!)\r\n"
         } ,
         {"Scorebar disabled.\r\n",
          "Scorebar enabled.\r\n"
         } ,
         {"Opponent health graph disabled.\r\n",
          "Opponent health graph enabled.\r\n"
         } ,
         {"ASCII graphics disabled.\r\n",
          "ASCII graphics enabled.\r\n"
         } ,
         /* The following is the autosplit/loot code from the snippts page --Erika */
         {"AutoSplit disabled.\r\n",
          "AutoSplit enabled.\r\n"
         } ,
         {"AutoLooting disabled.\r\n",
          "AutoLooting enabled.\r\n"
         } ,
         {"You can now hear battle.\r\n",
          "You are now deaf to battle.\r\n"
         } ,
         {"AutoAssist disabled.\r\n",
          "AutoAssist enabled.\r\n"
         } ,
         {"AutoSacrifice disabled.\r\n",
          "AutoSacrifice enabled.\r\n"
         } ,
         {"AutoGold disabled.\r\n",
          "AutoGold enabled.\r\n"
         } ,
         { "Welcome back!! You are no longer AFK.\n\r",
           "You are now away from the keyboard! Come back soon:)\n\r"
         },
         {"You can now hear ooc.\r\n",
          "You are now deaf to ooc.\r\n"
         },
         {"Page Ok disabled.\r\n",
          "Page Ok enabled.\r\n"
         },
         {"You can now hear info.\r\n",
          "You are now deaf to info.\r\n"
         },
         {"You now see fight messages.\r\n",
          "Fight spam off.\r\n"
         },
         {"Will no longer track through doors.\r\n",
          "Will now track through doors.\r\n"
         },
         {"ASCII objects turned off.\r\n",
          "ASCII objects turned on.\r\n"
         },
         {"You are now safe from being recalled by other players.\r\n",
          "You may now be recalled by other players.\r\n"
         },
         {"You can now hear the music channel.\r\n",
          "You are now deaf to the music channel.\r\n"
         },
	 {"You are now immortal.\r\n",
	  "You are now MORTAL!  Don't look surprised if you die...\r\n",
	 },
      };


   if (IS_NPC(ch))
      return;

   switch (subcmd)
      {
   case SCMD_NOSUMMON:
      result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
      break;
   case SCMD_NOHASSLE:
      result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
      break;
   case SCMD_BRIEF:
      result = PRF_TOG_CHK(ch, PRF_BRIEF);
      break;
   case SCMD_COMPACT:
      result = PRF_TOG_CHK(ch, PRF_COMPACT);
      break;
   case SCMD_NOTELL:
      result = PRF_TOG_CHK(ch, PRF_NOTELL);
      break;
   case SCMD_NOAUCTION:
      result = PRF_TOG_CHK(ch, PRF_NOAUCT);
      break;
   case SCMD_DEAF:
      result = PRF_TOG_CHK(ch, PRF_DEAF);
      break;
   case SCMD_NOGOSSIP:
      result = PRF_TOG_CHK(ch, PRF_NOGOSS);
      break;
   case SCMD_NOGRATZ:
      result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
      break;
   case SCMD_NOWIZ:
      result = PRF_TOG_CHK(ch, PRF_NOWIZ);
      break;
   case SCMD_QUEST:
      result = PRF_TOG_CHK(ch, PRF_QUEST);
      break;
   case SCMD_ROOMFLAGS:
      result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
      break;
   case SCMD_NOREPEAT:
      result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
      break;
   case SCMD_HOLYLIGHT:
      result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
      break;
   case SCMD_SLOWNS:
      result = (nameserver_is_slow = !nameserver_is_slow);
      break;
   case SCMD_AUTOEXIT:
      result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
      break;
   case SCMD_IDENT:
      // No reason to use ident anymore. This will allow disabling it if it
      // somehow managed to get the bit flipped. Messages above should indicate
      // if it _was_ on, which would indicate a problem
      result = 0;
      if (ident) {
         result = 1;
      }
      ident = 0;
      break;
   case SCMD_INFOBAR: /* -naj infobar2 12/16/96 - do_gen_tog */
      if (PRF_FLAGGED(ch,PRF_INFOBAR))
         {
         do_infobar(ch, 0, 0, SCMDB_CLEAR);
         result = PRF_TOG_CHK(ch, PRF_INFOBAR);
         }
      else
         {
         result = PRF_TOG_CHK(ch, PRF_INFOBAR);
         do_infobar(ch, 0, 0, SCMDB_REDRAW);
         }
      break;
   case SCMD_SCOREBAR:  /* -naj infobar2 12/16/96 - do_gen_tog */
      result = PRF_TOG_CHK(ch, PRF_SCOREBAR);
      do_infobar(ch, 0, 0, SCMDB_REDRAW);
      break;
   case SCMD_METER: /* -naj infobar2 12/16/96 - do_gen_tog */
      result = PRF_TOG_CHK(ch, PRF_METER);
      break;
   case SCMD_ASCII: /* -naj infobar2 12/16/96 - do_gen_tog */
      result = PRF_TOG_CHK(ch, PRF_ASCII);
      break;
   case SCMD_AUTOSPLIT: /* autosplit/loot code from snippets page --Erika */
      result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
      break;
   case SCMD_NOBATTLE:
      result = PRF_TOG_CHK(ch, PRF_NOBATTLE);
      break;
   case SCMD_AUTOLOOT:
      result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
      break;
   case SCMD_AUTOASSIST:
      result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
      break;
   case SCMD_AUTOSAC:
      result = PRF_TOG_CHK(ch, PRF_AUTOSAC);
      break;
   case SCMD_AUTOGOLD:
      result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
      break;
   case SCMD_AFK:
      result = PRF_TOG_CHK2(ch, PRF2_AFK);
      break;
   case SCMD_NOOOC:
      result = PRF_TOG_CHK2(ch, PRF2_NOOOC);
      break;
   case SCMD_PAGE_OK:
      result = PRF_TOG_CHK2(ch, PRF2_PAGE_OK);
      break;
   case SCMD_NOINFO:
      result = PRF_TOG_CHK2(ch, PRF2_NOINFO);
      break;
   case SCMD_NOSPAM:
      result = PRF_TOG_CHK2(ch, PRF2_NOSPAM);
      break;
   case SCMD_TRACK:
      result = (track_through_doors = !track_through_doors);
      break;
   case SCMD_XAP_OBJS:
      result = (xap_objs = !xap_objs);
      for(i=0;i<num_of_houses;i++)
         {
         room_rnum real_house;
         if((real_house = real_room(house_control[i].vnum)) != NOWHERE)
            House_crashsave(house_control[i].vnum);
         }
      break;
   case SCMD_NORECALL:
      result = PRF_TOG_CHK2(ch, PRF2_RECALLABLE);
      break;
   case SCMD_NOMUSIC:
      result = PRF_TOG_CHK2(ch, PRF2_NOMUSIC);
      break;
   case SCMD_MORTAL:
     if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT) {
       result = PRF_TOG_CHK2(ch, PRF2_MORTAL);
       if (PRF2_FLAGGED(ch, PRF2_MORTAL)) {
	 act("$n has become MORTAL!", TRUE, ch, NULL, NULL, TO_ROOM);
       } else {
	 act("$n has become immortal.", TRUE, ch, NULL, NULL, TO_ROOM);
       }
       break;
     } else {
       return;
     }
   default:
      log("SYSERR: Unknown subcmd: %d in do_gen_toggle",subcmd);
      return;
      break;
      }

   if (result)
      send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
   else
      send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

   return;
   }

ACMD(do_sac)
   {
   struct obj_data *obj;
   struct obj_data *tmp_obj;
   char *buf=get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, buf);

   if (*buf)
      {
      if ((obj = get_obj_in_list_vis(ch, buf, world[IN_ROOM(ch)].contents)) &&
              !((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && 
	        OBJVAL_FLAGGED(obj, CONT_LOCKED) &&
	        IS_OBJ_STAT(obj, ITEM_DONATED)) &&
              CAN_WEAR(obj, ITEM_WEAR_TAKE))
         {
         log_corpse(ch,obj,"sacrificed");
         while (obj->contains)
            {
            tmp_obj=obj->contains;
            obj_from_obj(tmp_obj);
            obj_to_room(tmp_obj,IN_ROOM(ch));
            }
         act("$n sacrifices $p.", FALSE, ch, obj, 0, TO_ROOM);
         act("You sacrifice $p to your god.", FALSE, ch, obj, 0, TO_CHAR);
         extract_obj(obj);
         }
      else if (!obj)
         {
         send_to_char(ch,"You can't seem to find that here.\r\n");
         }
      else
         {
         send_to_char(ch,"You can't sacrifice that!\r\n");
         }
      }
   else
      send_to_char(ch,"Sacrifice what?\r\n");
   release_buffer(buf);
   return;
   }

/* remort! command to remort. Added same-race remort (SRR) -Nomikos 8/3/2025   *
 * Also, if you're crazy enough, added Triple Remort for all classes and races */
ACMD(do_remort)
   {
   int i, same=0;
   struct obj_data *tmp_obj = NULL;

   int scr_remort_bonuses[12][6] = {
   // str int wis dex con cha
     { 1,  0,  0,  0,  1,  0  },// warrior
     { 0,  1,  1,  0,  0,  0  },// cleric
     { 0,  1,  0,  1,  0,  0  },// thief
     { 0,  1,  1,  0,  0,  0  },// magic-user
     { 0,  0,  1,  0,  1,  0  },// ranger
     { 0,  1,  1,  0,  0,  0  },// bard
     { 0,  0,  1,  0,  1,  0  },// monk
     { 0,  0,  0,  0,  0,  0  },// *unused*
     { 1,  0,  0,  0,  1,  0  },// barbarian
     { 1,  0,  1,  0,  0,  0  },// paladin
     { 1,  1,  0,  0,  0,  0  },// anti-paladin
     { 0,  1,  1,  0,  0,  0  },// druid
   };

   skip_spaces(&argument);
   if(IS_NPC(ch))
      {
      send_to_char(ch,"Mobs Can't remort!!\r\n");
      return;
      }
   for(i=0;i<NUM_WEARS;i++)
      {
      if((GET_EQ(ch,i)!=NULL) && (i != WEAR_HEART))
         {
         send_to_char(ch,"To be reborn into this world, you need to be as you"
                      " were born into it: naked. (That's right, take it off"
                      " baby.)\r\n");
         return;
         }
      }

   if (!*argument)
      {
      /* Single Remort */
      if((GET_CLASS(ch)<CLASS_KENSAI)&&is_remort_level(ch,NON_REMORT))
         {
         send_to_char(ch,"Usage: remort! <New Class>\r\n");
         send_to_char(ch,"Valid Classes: %s, %s, %s, %s, or SAME\r\n",
                      pc_class_types[CLASS_KENSAI],
                      pc_class_types[CLASS_ASSASSIN],
                      pc_class_types[CLASS_NECROMANCER],
                      pc_class_types[CLASS_DEVA]);
	 }
      /* Double Remort */
      else if((GET_RACE(ch)<RACE_DRACONIAN)&&is_remort_level(ch,SINGLE_REMORT))
         {
         send_to_char(ch, "Usage: remort! <New Race>\r\n");
         send_to_char(ch, "Valid Races: %s, %s, %s, and %s\r\n",
                      pc_race_types[RACE_DRACONIAN],
                      pc_race_types[RACE_SHADOW],
                      pc_race_types[RACE_TITAN],
                      pc_race_types[RACE_AESIR]);
         }
      /* Triple Remort */
      else if (REMORT_LEVEL(ch) == DOUBLE_REMORT)
      {
         send_to_char(ch, "Usage: remort! SAME\r\n");
      }
      else
      {
        send_to_char(ch,"The only place you have left to go is "
                     "Immortal!\r\nBetter submit that builder app!\r\n");
      }
      return;
      }
   else if((GET_CLASS(ch)<CLASS_KENSAI)&&(GET_LEVEL(ch)==LVL_HERO))
      {
      if(is_abbrev(argument,pc_class_types[CLASS_KENSAI]))
         GET_CLASS(ch)=CLASS_KENSAI;
      else if(is_abbrev(argument,pc_class_types[CLASS_ASSASSIN]))
         GET_CLASS(ch)=CLASS_ASSASSIN;
      else if(is_abbrev(argument,pc_class_types[CLASS_NECROMANCER]))
         GET_CLASS(ch)=CLASS_NECROMANCER;
      else if(is_abbrev(argument,pc_class_types[CLASS_DEVA]))
         GET_CLASS(ch)=CLASS_DEVA;
      else if(is_abbrev(argument,"SAME"))
         same=1;/* GET_CLASS(ch)=GET_CLASS(ch); */
      else
         {
         send_to_char(ch,"That is not a valid class!\r\n");
         return;
         }

      affect_remove_all(ch);
      REMORT_LEVEL(ch)=SINGLE_REMORT;
      GET_LEVEL(ch)=1;
      GET_EXP(ch)=1;
      if (GET_EQ(ch, WEAR_HEART))
         tmp_obj = unequip_char(ch, WEAR_HEART);
      GET_MAX_HIT(ch)=30;
      GET_MAX_MANA(ch)=125;
      GET_MAX_MOVE(ch)=100;

      advance_level(ch,0);
      GET_HIT(ch) = GET_MAX_HIT(ch);
      GET_MANA(ch) = GET_MAX_MANA(ch);
      GET_MOVE(ch) = GET_MAX_MOVE(ch);

      GET_WIMP_LEV(ch) = 0;

      if (same != 1)
      {
        for (i = 1; i <= MAX_SKILLS; i++)
          SET_SKILL(ch, i, 0);
        GET_SKILL(ch,PROF_FISTICUFFS)=40;
        GET_SKILL(ch,SKILL_READ_MAGIC)=60;
      }

      if (GET_CLASS(ch) < CLASS_KENSAI)
      {
         ch->real_abils.str=MIN(ch->real_abils.str + scr_remort_bonuses[(int)GET_CLASS(ch)][0], race_max_stats[GET_RACE(ch)][0]);
         ch->real_abils.intel=MIN(ch->real_abils.intel + scr_remort_bonuses[(int)GET_CLASS(ch)][1], race_max_stats[GET_RACE(ch)][1]);
         ch->real_abils.wis=MIN(ch->real_abils.wis + scr_remort_bonuses[(int)GET_CLASS(ch)][2], race_max_stats[GET_RACE(ch)][2]);
         ch->real_abils.dex=MIN(ch->real_abils.dex + scr_remort_bonuses[(int)GET_CLASS(ch)][3], race_max_stats[GET_RACE(ch)][3]);
         ch->real_abils.con=MIN(ch->real_abils.con + scr_remort_bonuses[(int)GET_CLASS(ch)][4], race_max_stats[GET_RACE(ch)][4]);
         ch->real_abils.cha=MIN(ch->real_abils.cha + scr_remort_bonuses[(int)GET_CLASS(ch)][5], race_max_stats[GET_RACE(ch)][5]);
      }

      switch (GET_CLASS(ch))
         {
      case CLASS_KENSAI:
         ch->real_abils.str=MIN(ch->real_abils.str+1,
                                race_max_stats[GET_RACE(ch)][0]);
         ch->real_abils.con=MIN(ch->real_abils.con+1,
                                race_max_stats[GET_RACE(ch)][4]);
         GET_SKILL(ch,PROF_SWORD)=35;
         GET_SKILL(ch,PROF_HAMMER)=35;
         GET_SKILL(ch,PROF_AXE)=35;
         GET_SKILL(ch,PROF_DAGGER)=35;
         break;
      case CLASS_ASSASSIN:
         ch->real_abils.dex=MIN(ch->real_abils.dex+1,
                                race_max_stats[GET_RACE(ch)][3]);
         ch->real_abils.intel=MIN(ch->real_abils.intel+1,
                                  race_max_stats[GET_RACE(ch)][1]);
         GET_SKILL(ch,PROF_SWORD)=35;
         GET_SKILL(ch,PROF_HAMMER)=35;
         GET_SKILL(ch,PROF_AXE)=35;
         GET_SKILL(ch,PROF_DAGGER)=35;
         break;
      case CLASS_NECROMANCER:
         ch->real_abils.wis=MIN(ch->real_abils.wis+1,
                                race_max_stats[GET_RACE(ch)][2]);
         ch->real_abils.intel=MIN(ch->real_abils.intel+1,
                                  race_max_stats[GET_RACE(ch)][1]);
         GET_SKILL(ch,SKILL_READ_MAGIC)=90;
         GET_SKILL(ch,PROF_CLUB)=35;
         GET_SKILL(ch,PROF_DAGGER)=35;
         GET_ALIGNMENT(ch)=-1000;
         break;
      case CLASS_DEVA:
         ch->real_abils.wis=MIN(ch->real_abils.wis+1,
                                race_max_stats[GET_RACE(ch)][2]);
         ch->real_abils.intel=MIN(ch->real_abils.intel+1,
                                  race_max_stats[GET_RACE(ch)][1]);
         GET_SKILL(ch,SKILL_READ_MAGIC)=90;
         GET_SKILL(ch,PROF_CLUB)=35;
         GET_SKILL(ch,PROF_DAGGER)=35;
         GET_ALIGNMENT(ch)=1000;
         break;
      default:
         break;
         } /* end switch */

      send_info("[ INFO ] %s has just remorted %s %s!!\r\n",
                GET_NAME(ch), same?"SAME-CLASS":"into a",
                same ? scr_male_pc_class_types[(int)GET_CLASS(ch)] : pc_class_types[(int)GET_CLASS(ch)]);
      send_to_char(ch,"%s", OK);
      }
   else if((GET_RACE(ch)<RACE_DRACONIAN)&&(GET_LEVEL(ch)==LVL_ANGEL))
      {
      if(is_abbrev(argument,pc_race_types[RACE_DRACONIAN]))
         {
         GET_RACE(ch)=RACE_DRACONIAN;
         ch->real_abils.intel=MIN(ch->real_abils.intel+1,
                                  race_max_stats[GET_RACE(ch)][1]);
         }
      else if(is_abbrev(argument,pc_race_types[RACE_SHADOW]))
         {
         GET_RACE(ch)=RACE_SHADOW;
         ch->real_abils.dex=MIN(ch->real_abils.dex+1,
                                race_max_stats[GET_RACE(ch)][3]);
         }
      else if(is_abbrev(argument,pc_race_types[RACE_AESIR]))
         {
         GET_RACE(ch)=RACE_AESIR;
         ch->real_abils.wis=MIN(ch->real_abils.wis+1,
                                race_max_stats[GET_RACE(ch)][2]);
         }
      else if(is_abbrev(argument,pc_race_types[RACE_TITAN]))
         {
         GET_RACE(ch)=RACE_TITAN;
         ch->real_abils.str=MIN(ch->real_abils.str+1,
                                race_max_stats[GET_RACE(ch)][0]);
         }
      else if(is_abbrev(argument,"SAME"))
         same=1;
      else
         {
         send_to_char(ch, "%s", "That is not a valid RACE!\r\n");
         return;
         }
      affect_remove_all(ch);
      REMORT_LEVEL(ch)=DOUBLE_REMORT;
      GET_LEVEL(ch)=1;
      GET_EXP(ch)=1;
      if (GET_EQ(ch, WEAR_HEART))
         tmp_obj = unequip_char(ch, WEAR_HEART);
      GET_MAX_HIT(ch)=40;
      GET_MAX_MANA(ch)=150;
      GET_MAX_MOVE(ch)=125;
      ch->real_abils.wis=MIN(ch->real_abils.wis+1,
                             race_max_stats[GET_RACE(ch)][2]);
      ch->real_abils.intel=MIN(ch->real_abils.intel+1,
                               race_max_stats[GET_RACE(ch)][1]);
      ch->real_abils.str=MIN(ch->real_abils.str+1,
                             race_max_stats[GET_RACE(ch)][0]);
      ch->real_abils.con=MIN(ch->real_abils.con+1,
                             race_max_stats[GET_RACE(ch)][4]);
      ch->real_abils.dex=MIN(ch->real_abils.dex+1,
                             race_max_stats[GET_RACE(ch)][3]);
      ch->real_abils.cha=MIN(ch->real_abils.cha+1,
                             race_max_stats[GET_RACE(ch)][5]);

      advance_level(ch,0);
      GET_HIT(ch) = GET_MAX_HIT(ch);
      GET_MANA(ch) = GET_MAX_MANA(ch);
      GET_MOVE(ch) = GET_MAX_MOVE(ch);

      GET_WIMP_LEV(ch) = 0;

      switch (GET_CLASS(ch))
      {
      case CLASS_NECROMANCER:
         GET_ALIGNMENT(ch)=-1000;
         break;
      case CLASS_DEVA:
         GET_ALIGNMENT(ch)=1000;
         break;
      }

      send_info("[ INFO ] %s has just remorted into a %s!!\r\n",GET_NAME(ch),
                pc_race_types[GET_RACE(ch)]);
      send_to_char(ch, "%s", OK);
      }
   else if (REMORT_LEVEL(ch) == DOUBLE_REMORT)
   {
      if(!is_abbrev(argument,"SAME"))
      {
        send_to_char(ch, "Usage: remort! SAME\r\n");
        return;
      }

      affect_remove_all(ch);
      REMORT_LEVEL(ch)=TRIPLE_REMORT;
      GET_LEVEL(ch)=1;
      GET_EXP(ch)=1;
      if (GET_EQ(ch, WEAR_HEART))
         tmp_obj = unequip_char(ch, WEAR_HEART);
      GET_MAX_HIT(ch)=50;
      GET_MAX_MANA(ch)=175;
      GET_MAX_MOVE(ch)=150;
      ch->real_abils.wis=MIN(ch->real_abils.wis+1,
                             race_max_stats[GET_RACE(ch)][2]);
      ch->real_abils.intel=MIN(ch->real_abils.intel+1,
                               race_max_stats[GET_RACE(ch)][1]);
      ch->real_abils.str=MIN(ch->real_abils.str+1,
                             race_max_stats[GET_RACE(ch)][0]);
      ch->real_abils.con=MIN(ch->real_abils.con+1,
                             race_max_stats[GET_RACE(ch)][4]);
      ch->real_abils.dex=MIN(ch->real_abils.dex+1,
                             race_max_stats[GET_RACE(ch)][3]);
      ch->real_abils.cha=MIN(ch->real_abils.cha+1,
                             race_max_stats[GET_RACE(ch)][5]);

      advance_level(ch,0);
      GET_HIT(ch) = GET_MAX_HIT(ch);
      GET_MANA(ch) = GET_MAX_MANA(ch);
      GET_MOVE(ch) = GET_MAX_MOVE(ch);

      GET_WIMP_LEV(ch) = 0;

      send_info("[ INFO ] %s has just THIRD remorted!!!!\r\n",GET_NAME(ch));
      send_to_char(ch, "%s", OK);
   }
   else
      send_to_char(ch, "%s", "Just isn't gonna happen\r\n");
   if (tmp_obj)
      equip_char(ch, tmp_obj, WEAR_HEART);
   affect_total(ch);
   }

ACMD(do_battle)
   {
   int tmp, tmp2;
   tmp = 0;
   tmp2 = 0;

   if (IS_NPC(ch) || !ch->desc)
      return;

   if (FIGHTING(ch))
      {
      send_to_char(ch,"You cant enter battle while fighting!!.\n\r");
      return;
      }


   if (battle.zone_state == FALSE)
      {
      send_to_char(ch,"The battle field is not open at this time.\n\r");
      return;
      }

   if (battle.locked == TRUE)
      {
      send_to_char(ch,"The battle field is not open to new contestants now.\n\r");
      return;
      }

   if (IS_SET(world[IN_ROOM(ch)].room_flags, ROOM_NO_RECALL)||
           Z_FLAGGED(IN_ROOM(ch),Z_NO_RECALL))
      {
      send_to_char(ch, "The gods have forsaken you!!\r\n");
      return;
      }

   if (GET_LEVEL(ch) < battle.low_level)
      {
      send_to_char(ch, "You must be above level %d to enter the battle!\n\r",
                   battle.low_level);
      return;
      }

   if (GET_BATTLE(ch) == TRUE)
      {
      send_to_char(ch, "You are already in battle!! Watch out!\n\r");
      return;
      }


   if (GET_LEVEL(ch) > battle.high_level)
      {
      send_to_char(ch, "You must be below level %d to enter the battle!\n\r",
                   battle.high_level);
      return;
      }

   /*    GET_MANA(ch) = GET_MAX_MANA(ch); */
   /*    GET_HIT(ch) = GET_MAX_HIT(ch); */
   /*    GET_MOVE(ch) = GET_MAX_MOVE(ch); */
   GET_BATTLE(ch) = TRUE;

   send_to_char(ch,"Entering the Battle Field... Good Luck!!!\n\r");
   char_from_room(ch);

   /* This is for the random location in the bfield code. */
   /* bfield_start/end defines are in structs.h */

   tmp2 = number(BFIELD_START, BFIELD_END);

   /* dont touch this code below... its essential */

   tmp = real_room(tmp2);
   if(tmp<0)
      {
      log("ERROR IN do_battle.  unknown room number");
      tmp=0;
      }
   char_to_room(ch, tmp);
   do_look(ch, "", 0, 0);
   send_battle("[ BATTLE ] %s has entered the battle field.\n\r",GET_NAME(ch));
   if ((battle.do_tag == TRUE) && (battle.tagged == FALSE))
      {
      send_battle("[ BATTLE ] %s is now IT!!!\r\n",GET_NAME(ch));
      battle.tagged = TRUE;
      TAGGED(ch) = TRUE;
      }
   }

/* makes sure that player cannot give/drop more than they bid on an auction */
int can_give_gold(struct char_data *ch, int amount)
{
   if ((aauction.in_progress == FALSE) ||
       (GET_IDNUM(ch) != aauction.bidder_id_num))
      return 1;

   if ((get_char_gold(ch) - amount) >= aauction.last_bid)
      return 1;

   return 0;
}


ACMD (do_tag)
{
   struct char_data *victim;
   char *arg=get_buffer(MAX_INPUT_LENGTH);

   any_one_arg(argument, arg);

   if (ch->char_specials.in_battle == TRUE)
      if (*arg)
         if ((victim = get_char_vis(ch, arg,FIND_CHAR_ROOM)) && (ch!=victim))
            {
            /* this is assuming that only one person on the mud is tagged */
            if (TAGGED(ch))
               {
               if (FIGHTING(ch))
                  {
                  act("You're too busy getting whooped on to tag!", FALSE,
                      ch, 0, 0, TO_CHAR);
                  release_buffer(arg);
                  return;
                  }
               TAGGED(ch) = FALSE;
               TAGGED(victim) = TRUE;
               GET_WAIT_STATE(victim) = 3 RL_SEC;
               send_battle("[ BATTLE ] %s has been tagged by %s!  %s is now IT!!!.\n\r",
                           GET_NAME(victim), GET_NAME(ch), GET_NAME(victim));
               }
            else if (TAGGED(victim))
               {
               act("$N is already IT!! You better run for the hills!", TRUE,
                   ch, 0, victim, TO_CHAR);
               act("$n just tried to tag you! What a BUFFOON!!", TRUE,
                   ch, 0, victim, TO_VICT);
               act("$n just tried to tag $N..  $e is REALLY asking for it!", TRUE,
                   ch, 0, victim, TO_NOTVICT);
               release_buffer(arg);
               return;
               }
            else if (!TAGGED(ch) && !TAGGED(victim))
               {
               act("You aren't IT silly!  (better go hide)", TRUE,
                   ch, 0, victim, TO_CHAR);
               act("$n just tried to tag you, what a dork!", TRUE,
                   ch, 0, victim, TO_VICT);
               act("$n tries to tag $N but isn't buff enough.", TRUE,
                   ch, 0, victim, TO_NOTVICT);
               release_buffer(arg);
               return;
               }
            }

   do_action(ch, arg, find_command("tag_social"), 0);
   release_buffer(arg);
}

ACMD(do_graffiti)
{
  if (IS_NPC(ch)) {
    return;
  }

  if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
    return;
  }

  char *buf = get_buffer(MAX_STRING_LENGTH);
  strcpy(buf, argument);
  char *sub = strtok(buf, " ");
  char *buf2 = get_buffer(MAX_STRING_LENGTH);

  if (starts_with("permanent", sub)) {
    if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
      send_to_char(ch,"You can only commune with the gods!\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
    }
    if (!ROOM2_FLAGGED(IN_ROOM(ch), ROOM2_GRAFFITI) && !Z_FLAGGED(IN_ROOM(ch), Z_GRAFFITI) && GET_LEVEL(ch) < LVL_DETY) {
      send_to_char(ch, "The gods forbid graffiti in this area.\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
    }
    if (get_graffiti_count(GET_IDNUM(ch), 1) >= MAX_PERM_GRAFFITI_PER_PLAYER && GET_LEVEL(ch) < LVL_DGODI) {
      send_to_char(ch, "You've left enough of your mark on this world already.\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
    }
    char *text = strtok(NULL, "");
    if (!text) {
      send_to_char(ch, "Usage: graffiti perm <string>\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
    }
    graffiti = (struct graffiti *)realloc(graffiti, ++num_graffiti * sizeof(struct graffiti));
    graffiti[num_graffiti-1].author = GET_IDNUM(ch);
    graffiti[num_graffiti-1].permanent = 1;
    graffiti[num_graffiti-1].room_vnum = GET_ROOM_VNUM(IN_ROOM(ch));
    if (strlen(text) >= 128) {
      send_to_char(ch, "Line too long, truncated.\r\n");
      text[128] = '\x0';
    }
    sprintf(buf2, "%s wrote, '%s' (permanent)", GET_NAME(ch), text);
    graffiti[num_graffiti-1].text = strdup(buf2);
    send_to_char(ch, "You scrawl your graffiti across the area.\r\n");
    save_graffiti();
    mudlogf(CMP, LVL_IMMORT, TRUE, "Graffiti: %s permanently wrote \"%s\" at #%ld (%s).",
      GET_NAME(ch),
      text,
      GET_ROOM_VNUM(IN_ROOM(ch)),
      world[IN_ROOM(ch)].name
    );
  } else if (starts_with("temporary", sub)) {
    if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
      send_to_char(ch,"You can only commune with the gods!\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
    }
    if (!ROOM2_FLAGGED(IN_ROOM(ch), ROOM2_GRAFFITI) && !Z_FLAGGED(IN_ROOM(ch), Z_GRAFFITI) && GET_LEVEL(ch) < LVL_DETY) {
      send_to_char(ch, "The gods forbid graffiti in this area.\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
    }
    if (get_graffiti_count(GET_IDNUM(ch), 0) >= MAX_TEMP_GRAFFITI_PER_PLAYER && GET_LEVEL(ch) < LVL_DGODI) {
      send_to_char(ch, "You've left enough of your mark on this world already.\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
    }
    char *text = strtok(NULL, "");
    if (!text) {
      send_to_char(ch, "Usage: graffiti temp <string>\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
    }
    graffiti = (struct graffiti *)realloc(graffiti, ++num_graffiti * sizeof(struct graffiti));
    graffiti[num_graffiti-1].author = GET_IDNUM(ch);
    graffiti[num_graffiti-1].permanent = 0;
    graffiti[num_graffiti-1].room_vnum = GET_ROOM_VNUM(IN_ROOM(ch));
    if (strlen(text) >= 128) {
      send_to_char(ch, "Line too long, truncated.\r\n");
      text[128] = '\x0';
    }
    sprintf(buf2, "%s wrote, '%s' (temporary)", GET_NAME(ch), text); /* %s wrote, '%s' ? */
    graffiti[num_graffiti-1].text = strdup(buf2);
    send_to_char(ch, "You scrawl your graffiti across the area.\r\n");
    save_graffiti();
    mudlogf(CMP, LVL_IMMORT, TRUE, "Graffiti: %s temporarily wrote \"%s\" at #%ld (%s).",
      GET_NAME(ch),
      text,
      GET_ROOM_VNUM(IN_ROOM(ch)),
      world[IN_ROOM(ch)].name
    );
  } else {
    send_to_char(ch, "Usage: graffiti <perm|temp>\r\n");
  }

  release_buffer(buf);
  release_buffer(buf2);
}








extern int top_of_p_table;
extern struct player_index_element *player_table;
extern char *wizlist;
extern char *herolist;

int strlen_nospace(char *string)
{
  int chars = 0, i;
  for (i = 0; i < strlen(string); i++) {
    chars += string[i] != ' ' ? 1 : 0;
  }
  return chars;
}

const char *imm_name_strings[] = {
  "Head Implementor",
  "Implementors",
  "Administrators",
  "Greater Gods",
  "Gods",
  "Demi Gods",
  "Deities",
  "Seraph",
  "ArchAngels",
  "Ambassadors",
  "Demi-Gods",
  "Avatars",
  "Angels",
  "Heroes"
};

typedef struct _imm_map {
  char name[64];
  int level;
} imm_map;
imm_map mapping[1024];
int nMaps = 0;

void print_imms(char *buf, int min, int max, int string_index)
{
  int i, j;
  buf[0] = '\x0'; /* The entire output for this group of imms, including the title. */



  int count = 0;
  for (i = 0; i < nMaps; i++) {
    if (mapping[i].level >= min && mapping[i].level <= max) {
     count++;
    }
  }
  if (count == 0) {
    return;
  }


  const char *title = imm_name_strings[string_index];
  int len = strlen(title);

  for (i = 0; i < 40 - len/2; i++) {
    strcat(buf, " ");
  }
  strcat(buf, title);
  for (i = 0; i < 40 - len/2; i++) {
    strcat(buf, " ");
  }
  strcat(buf, "\r\n");
  for (i = 0; i < 40 - len/2; i++) {
    strcat(buf, " ");
  }
  for (i = 0; i < len; i++) {
    strcat(buf, "~");
  }
  strcat(buf, "\r\n");

  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];

  buf2[0] = '\x0';

  int c = 0;
  for (i = 0; i < 1024; i++) {
    if (mapping[i].level >= min && mapping[i].level <= max) {
      strcpy(buf3, mapping[i].name); /* A single imm name. */
      buf3[0] = toupper(buf3[0]);
      if (++c < count) {
        for (j = 0; j < 12-strlen(mapping[i].name); j++) {
          strcat(buf3, " ");
        }
      }
      if (strlen(buf2) + strlen(buf3) >= 80) {
        for (j = 0; j < 40 - strlen(buf2)/2; j++) {
          strcat(buf, " ");
        }
        strcat(buf, buf2);
        strcat(buf, "\r\n");
        strcpy(buf2, buf3);
      } else {
        strcat(buf2, buf3);
      }
    }
  }
  for (j = 0; j < 40 - strlen(buf2)/2; j++) {
    strcat(buf, " ");
  }
  strcat(buf, buf2);

  strcat(buf, "\r\n");
  strcat(buf, "\r\n");
}

void update_wizlist(void)
{
  char *tname = get_buffer(SMALL_BUFSIZE);
  int tlevel = 0;

  FILE *fp = fopen(WIZLIST_FILE, "w");
  if (!fp) {
    log("SYSERR: Could not open text/wizlist for writing.");
    return;
  }

  FILE *fo = fopen("etc/players_ascii/index", "r");
  if (!fo) {
    log("SYSERR: Could not open players_ascii/index for reading.");
    return;
  }


  char *buf = get_buffer(65536);
  char *buf2 = get_buffer(MAX_STRING_LENGTH);

  nMaps = 0;
  while (fgets(buf, 1024, fo)) {//1
    prune_crlf(buf);
    strcpy(tname, buf);
    fgets(buf, 1024, fo);// 2
    fgets(buf, 1024, fo);// 3
    strtok(buf, " ");
    tlevel = atoi(strtok(NULL, " "));
    /* Only count heroes and above - Nomikos 10/19/2025 */
    if (tlevel >= LVL_HERO) {
      strcpy(mapping[nMaps].name, tname);
      mapping[nMaps].level = tlevel;
      nMaps++;
    }
    fgets(buf, 1024, fo);// 4
    fgets(buf, 1024, fo);// 5
  }
  fclose(fo);
  release_buffer(tname);

  sprintf(buf,
"****************************************************************************\r\n"
"* Here is a list of PhoenixMUD Immortals.  They are the builders and       *\r\n"
"* shapers of this great world.  If you see one on, do not be shy, they     *\r\n"
"* are usually a friendly bunch!  They should be treated with respect,      *\r\n"
"* for their hard work keeps this game running!                             *\r\n"
"****************************************************************************\r\n"
"\r\n");

  print_imms(buf2, LVL_HIMPL, LVL_HIMPL, 0);
  strcat(buf, buf2);
  print_imms(buf2, LVL_SIMP, LVL_IMPLII, 1);
  strcat(buf, buf2);
  print_imms(buf2, LVL_ADMIN, LVL_ADMIN, 2);
  strcat(buf, buf2);
  print_imms(buf2, LVL_GRGOD, LVL_GRGODII, 3);
  strcat(buf, buf2);
  print_imms(buf2, LVL_GOD, LVL_GODII, 4);
  strcat(buf, buf2);
  print_imms(buf2, LVL_DGOD, LVL_DGODII, 5);
  strcat(buf, buf2);
  print_imms(buf2, LVL_DETY, LVL_DETYII, 6);
  strcat(buf, buf2);
  print_imms(buf2, LVL_SERP, LVL_SERPII, 7);
  strcat(buf, buf2);
  print_imms(buf2, LVL_ARCH, LVL_ARCHII, 8);
  strcat(buf, buf2);
  print_imms(buf2, LVL_IMMORT, LVL_AMBASS, 9);
  strcat(buf, buf2);

  if (wizlist) {
    free(wizlist);
    wizlist = strdup(buf);
  }

  fputs(buf, fp);
  fclose(fp);

  fp = fopen(HEROLIST_FILE, "w");
  if (!fp) {
    log("SYSERR: Could not open text/herolist for writing.");
    return;
  }

  sprintf(buf,
"****************************************************************************\r\n"
"* Here is a list of PhoenixMUD Heroes.  They have done the work, fought    *\r\n"
"* the battles, and conquered the tests.  If you see one on, do not be      *\r\n"
"* shy, they are usually a friendly bunch!  They should be treated with     *\r\n"
"* respect and awe for their dedication to PhoenixMUD!                      *\r\n"
"****************************************************************************\r\n"
"\r\n");

  print_imms(buf2, LVL_WANKER, LVL_WANKER, 10);
  strcat(buf, buf2);
  print_imms(buf2, LVL_AVATAR, LVL_AVATAR, 11);
  strcat(buf, buf2);
  print_imms(buf2, LVL_ANGEL, LVL_ANGEL, 12);
  strcat(buf, buf2);
  print_imms(buf2, LVL_HERO, LVL_HERO, 13);
  strcat(buf, buf2);

  if (herolist) {
    free(herolist);
    herolist = strdup(buf);
  }

  fputs(buf, fp);
  fclose(fp);

  release_buffer(buf);
  release_buffer(buf2);
}

ACMD(do_uwizlist)
{
  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT) {
    return;
  }

  update_wizlist();
  send_to_char(ch, "Okay.\r\n");
}


struct char_data *get_char_room_vis(struct char_data *ch, char *name);
extern struct obj_data *obj_proto;


const char *reimb_usage =
"Usage: reimb <\"skill\"|\"eq\"|\"reimb!\">\r\n"
"\r\n"
"To add a skill or spell to the reimb list:\r\n"
"  reimb skill <skill/spell name>\r\n"
"If you change your mind or make a mistake, type the same command again:\r\n"
"  reimb skill <skill/spell name>\r\n"
"\r\n"
"To add/change equipment:\r\n"
"  reimb eq <slot> <substring of eq name>\r\n"
"e.g.\r\n"
"  reimb eq finger1 ring of the magi\r\n"
"\r\n"
"Valid slots are:\r\n"
"  finger1, finger2, neck1, neck2, body, head, legs, feet, hands, arms, shield\r\n"
"  about, waist, wrist1, wrist2, wield, hold, ear1, ear2, face, back\r\n";

const char* slots[] = {"-", "finger1", "finger2", "neck1", "neck2", "body", // 0-5
		       "head", "legs", "feet", "hands", "arms", // 6-10
		       "shield", "about", "waist", "wrist1", "wrist2", // 11-15
		       "wield", "-", "hold", "-", "ear1", // 16-20
		       "ear2", "face", "back"}; // 21-23

int get_slot(char *slot)
{
  int i;
  for (i = 1; i < 24; i++) {
    if (is_abbrev(slot, slots[i])) {
      return i;
    }
  }
  return 0;
}

void reimb_mortal_show_skills(struct char_data* ch)
{
  send_to_char(ch, "Skills and Spells you are currently being reimb'd:\r\n");
  int total = 0;
  int i;
  for (i = 0; i < MAX_SPELLS; i++) {
    if (ch->player_specials->reimb_skills[i] > 0) {
      send_to_char(ch, "%2d. %s\r\n", ++total, spells[i].spell_name);
    }
  }
  send_to_char(ch, "You have %d skills and %d spells left to reimb.\r\n",
	       ch->player_specials->reimb_num_skills,
	       ch->player_specials->reimb_num_spells);
}

void reimb_mortal_show_equipment(struct char_data* ch)
{
  send_to_char(ch, "Equipment you are currently being reimb'd:\r\n");
  int i;
  for (i = 1; i < NUM_WEARS-1; i++) {
    int vnum;
    if ((vnum = ch->player_specials->reimb_obj_vnums[i]) > 0) {
      send_to_char(ch, "%2d. %s %s\r\n", i, where[i], GET_OBJ_NAME(&obj_proto[vnum]));
    } else {
      send_to_char(ch, "%2d. %s <EMPTY>\r\n", i, where[i]);
    }
  }
}

int position_ok(struct obj_data* obj, int islot)
{
  int conv[] = {0,1,1,2,2,3,4,5,6,7,8,9,10,11,12,12,13,13,14,14,17,17,18,19};
  return CAN_WEAR(obj, 1 << conv[islot]);
}

extern int top_of_objt;
extern struct obj_data *obj_proto;
extern struct index_data *obj_index;

char *str_str(char *cs, char *ct);
int can_wear_lr(struct char_data *ch, struct obj_data *obj, int message);

void commit_reimb(struct char_data* ch)
{
  if (GET_EQ(ch, WEAR_HEART)) {
    struct obj_data* tmp_obj = unequip_char(ch, WEAR_HEART);
    extract_obj(tmp_obj);
  }
  int i;
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i) && (i != WEAR_HEART)) {
      send_to_char(ch, "You have to be completely naked for this to work.\r\n");
      return;
    }
  }

  for (i = 0; i < MAX_SPELLS; i++) {
    if (min_level(ch, i) <= 0 || min_level(ch, i) >= LVL_IMMORT) {
      GET_SKILL(ch, i) = 0;
    } else {
      GET_SKILL(ch, i) = MAX(GET_SKILL(ch, i), ch->player_specials->reimb_skills[i]);
      if (spells[i].is_spell == IS_SPELL) {
	GET_SKILL(ch, i) = MAX(1, GET_SKILL(ch, i));
      } else {
	GET_SKILL(ch, i) = MAX(25, GET_SKILL(ch, i));
      }
    }
  }

  for (i = 0; i < 25; i++) {
    int rnum = ch->player_specials->reimb_obj_vnums[i];
    if (rnum > 0) {
      struct obj_data *obj = read_object(rnum, REAL);
      obj_to_char(obj, ch);
    }
  }

  if (REMORT_LEVEL(ch) == TRIPLE_REMORT)
    GET_GOLD(ch) += 25000 * GET_LEVEL(ch);	
  else if (REMORT_LEVEL(ch) == DOUBLE_REMORT)
    GET_GOLD(ch) += 20000 * GET_LEVEL(ch);
  else if (REMORT_LEVEL(ch) == SINGLE_REMORT)
    GET_GOLD(ch) += 15000 * GET_LEVEL(ch);
  else
    GET_GOLD(ch) += 10000 * GET_LEVEL(ch);

  mudlogf(BRF, LVL_IMMORT, TRUE, "(GC) %s has been reimbursed to level %d.", GET_NAME(ch), GET_LEVEL(ch));

  ch->player_specials->is_being_reimbd = 0;

  send_to_char(ch, "Okay.\r\n");
  send_info("[ INFO ] %s has just been REIMBURSED!\r\n", GET_NAME(ch));
}

ACMD(do_old_reimb)
{
  if (IS_NPC(ch)) {
    return;
  }

  if (GET_LEVEL(ch) < 106 && !ch->player_specials->is_being_reimbd) {
    send_to_char(ch, "Ask an immortal about being reimbursed.\r\nType \"help old_reimb\" and follow the instructions.\r\n");
    return;
  }

  char buf[1024];
  strcpy(buf, argument);

  if (GET_LEVEL(ch) >= 106) {
    char *target_name = strtok(buf, " ");
    struct char_data* target = get_char_room_vis(ch, target_name);
    if (!(target = get_char_room_vis(ch, target_name))) {
      send_to_char(ch, "They aren't here.\r\n");
      return;
    }
    send_to_char(ch, "Okay.  %s can be reimb'd now.\r\n", GET_NAME(target));
    send_to_char(target, "%s has set you for a reimb.\r\n", GET_NAME(ch));
    target->player_specials->is_being_reimbd = 1;
    float factor;
    factor = REMORT_LEVEL(target) == DOUBLE_REMORT ? 2.5f : (REMORT_LEVEL(target) == SINGLE_REMORT ? 2.0f : 1.5f);
    target->player_specials->reimb_num_spells = 1+(int)(GET_LEVEL(target)/10.0*factor);
    factor = REMORT_LEVEL(target) == DOUBLE_REMORT ? 1.5f : (REMORT_LEVEL(target) == SINGLE_REMORT ? 1.3f : 1.0f);
    target->player_specials->reimb_num_skills = 1+(int)(GET_LEVEL(target)/10.0*factor);
    memset(target->player_specials->reimb_obj_vnums, 0, NUM_WEARS*sizeof(int));
    memset(target->player_specials->reimb_skills, 0, MAX_SPELLS*sizeof(int));
    return;
  }

  char *reimb_type = strtok(buf, " ");
  if (!reimb_type) {
    send_to_char(ch, "%s", reimb_usage);
    return;
  }

  if (!str_cmp(reimb_type, "eq")) {
    char *slot = strtok(NULL, " ");
    if (!slot) {
      reimb_mortal_show_equipment(ch);
      return;
    }
    int islot = get_slot(slot);
    if (islot <= 0) {
      send_to_char(ch, "%s", reimb_usage);
      return;
    }
    char *name = strtok(NULL, "");
    if (!name) {
      send_to_char(ch, "%s", reimb_usage);
      return;
    }
    int i;
    int matches[10], tot_match = 0;
    memset(matches, 0, 10*sizeof(int));
    for (i = 0; i < top_of_objt; i++) {
      struct obj_data* obj = &obj_proto[i];
      int znum = GET_OBJ_VNUM(obj)/100;
      if (znum == 0 || znum == 7 || znum == 8 || znum == 12 || znum == 69 || znum == 219) {
	continue;
      }
      if (str_str(GET_OBJ_NAME(obj), name)) {
	matches[tot_match++] = i;//GET_OBJ_RNUM(obj);
	if (tot_match >= 10) {
	  break;
	}
      }
    }
    if (tot_match > 1) {
      send_to_char(ch, "%s", "Too many matches.  Please refine your query.\r\n");
    } else if (tot_match == 1) {
      struct obj_data* obj = &obj_proto[matches[0]];
      if (!position_ok(obj, islot)) {
	send_to_char(ch, "%s", "You can't wear that object in that slot.\r\n");
	return;
      }
      if (!can_wear_lr(ch, obj, TRUE)) {
	send_to_char(ch, "%s", "You're too low a level.\r\n");
	return;
      }
      if (invalid_align(ch, obj) || invalid_class(ch, obj) || invalid_race(ch, obj)) {
	send_to_char(ch, "%s", "You aren't the proper race, class or alignment.\r\n");
	return;
      }
      ch->player_specials->reimb_obj_vnums[islot] = matches[0];
      send_to_char(ch, "Added equipment \"%s\" in slot %s.\r\n", GET_OBJ_NAME(&obj_proto[matches[0]]), slots[islot]);
    } else {
      send_to_char(ch, "%s", "Possible matches:\r\n");
      for (i = 0; i < tot_match; i++) {
	send_to_char(ch, "  %s\r\n", GET_OBJ_NAME(&obj_proto[matches[i]]));
      }
    }
  } else if (starts_with("skills", reimb_type)) {
    char *skill = strtok(NULL, "");
    if (!skill) {
      reimb_mortal_show_skills(ch);
      return;
    }
    int i;
    for (i = 0; i < MAX_SPELLS; i++) {
      if (is_abbrev(skill, spells[i].spell_name)) {
	if (GET_LEVEL(ch) < min_level(ch, i)) {
	  send_to_char(ch, "Your character does not know %s.\r\n", spells[i].spell_name);
	  return;
	}
	if (spells[i].is_spell == IS_SKILL) {
	  if (ch->player_specials->reimb_skills[i] > 0) {
	    ch->player_specials->reimb_skills[i] = 0;
	    ch->player_specials->reimb_num_skills++;
	    send_to_char(ch, "Removed %s from skill list.\r\n", spells[i].spell_name);
	  } else if (ch->player_specials->reimb_num_skills <= 0) {
	    send_to_char(ch, "You have no more skills left to reimb.\r\n");
	  } else {
	    ch->player_specials->reimb_skills[i] = 95;
	    ch->player_specials->reimb_num_skills--;
	    send_to_char(ch, "Added %s to the skill list.\r\n", spells[i].spell_name);
	  }
	} else {
	  if (ch->player_specials->reimb_skills[i] > 0) {
	    ch->player_specials->reimb_skills[i] = 0;
	    ch->player_specials->reimb_num_spells++;
	    send_to_char(ch, "Removed %s from spell list.\r\n", spells[i].spell_name);
	  } else if (ch->player_specials->reimb_num_spells <= 0) {
	    send_to_char(ch, "You have no more spells left to reimb.\r\n");
	  } else {
	    ch->player_specials->reimb_skills[i] = 10;
	    ch->player_specials->reimb_num_spells--;
	    send_to_char(ch, "Added %s to the spell list.\r\n", spells[i].spell_name);
	  }
	}
	return;
      }
    }
    send_to_char(ch, "That skill or spell is unknown.\r\n");
    return;
  } else if (!strcmp(reimb_type, "reimb!")) {
    commit_reimb(ch);
  } else {
    send_to_char(ch, "%s", reimb_usage);
  }

  return;
}

// reimb eq wrist1 assassin's bracelet
// reimb eq wrist
// reimb eq

// reimb spell wrath of god

ACMD(do_flush)
{
  struct descriptor_data* d = ch->desc;

  if (IS_NPC(ch)) {
    return;
  }

  printf("Flushing: %s\n", GET_NAME(ch));

  if (d->large_outbuf)
    {
      release_buffer(d->large_outbuf);
      d->large_outbuf=NULL;
      d->output=d->small_outbuf;
    }
  while (d->input.head)
    {
      struct txt_block *tmp = d->input.head;
      d->input.head = d->input.head->next;
      free(tmp->text);
      free(tmp);
    }

  send_to_char(ch, "Command queue flushed.\r\n");
}


/* release command, allows players or mobs to release charmed followers - Nomikos 1/31/2026 */
ACMD(do_release)
   {
   struct char_data *vict;
   char *arg;

   arg = get_buffer(MAX_INPUT_LENGTH);
   any_one_arg(argument, arg);
   
   if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
      {
      send_to_char(ch, "Who or what do you wish to release???\r\n");
      release_buffer(arg);
      return;
      }
   release_buffer(arg);

   if (vict == ch)
      send_to_char(ch,"If you love someone, set them free!\r\n");
   else if (!AFF_FLAGGED(vict, AFF_CHARM))
      send_to_char(ch,"You can't release this one, as you hold no sway over them!\r\n");
   else if (vict->master != ch)
      send_to_char(ch,"Nice try! This one's heart belongs to another.\r\n");
   else
      {
      act("You release $N from your control. They are free to go as they please.", FALSE, ch, 0, vict, TO_CHAR);
      act("$n releases you from their control. You have been set free!", FALSE, ch, 0, vict, TO_VICT);
      act("$n must really love $N, because they have set them free!", FALSE, ch, 0, vict, TO_NOTVICT);
      
      stop_follower(vict);
      }
   }
