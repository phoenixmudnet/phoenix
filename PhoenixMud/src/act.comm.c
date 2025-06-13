/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
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
#include "screen.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"
#include "queue.h"

#define MAX_NOTE_LENGTH 1000 /* arbitrary */

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct spell_info_type *spells;
extern struct zone_data *zone_table;
extern char *item_types[];
extern char *item_condition[];
extern char *item_wear[];
extern char *extra_bits_id[];
extern char *immunity_names[];
extern char *wear_strings[];
extern char *anti_bits_id[];
extern char *apply_types[];
extern char *affected_bits[];
extern int level_can_shout;
extern int holler_move_cost;
extern int auction_cost;
extern float auction_profit;
/** auction code Anduin */
extern struct autoauction aauction;
extern struct index_data *obj_index;
extern struct queue_event *auction_event;
void mprog_speech_trigger(char *txt, struct char_data *mob);
void do_auction_update(void);
void id_obj_to_char(struct char_data *ch, struct obj_data *obj);
/* commune code from incabolus */
int ignoring(struct char_data *ch, struct char_data *vict);

ACMD(do_commune)
   {
   bool is_mort = FALSE;
   struct descriptor_data *i;
   char *buf = get_buffer(MAX_STRING_LENGTH);
   char *buf2 = get_buffer(MAX_STRING_LENGTH);

   if(PLR_FLAGGED(ch,PLR_NOCOMMUNE) && GET_LEVEL(ch) < LVL_IMMORT)
     {
     send_to_char(ch,"You can't commune with the gods.\r\n");
     release_buffer(buf);     
     release_buffer(buf2);
     return;
     }

   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   skip_spaces(&argument);

   if (!*argument)
      {
      send_to_char(ch,"Commune? Yes, but WHAT do you wish to commune?\r\n");
      }
   else
      {
      send_to_char(ch,"You commune, '&M%s&n'\r\n",argument);

      if (GET_LEVEL(ch) < LVL_IMMORT)
         {
         sprintf(buf,"%s beseeches the gods, '&M%s&n'",GET_NAME(ch),argument);
         is_mort = TRUE;
         }
      else
         {
         sprintf(buf,"%s shouts down from above, '&M%s&n'",GET_NAME(ch),
                 argument);
         sprintf(buf2,"A god shouts down from above, '&M%s&n'",argument);
         is_mort = FALSE;
         }

      for (i = descriptor_list; i; i = i->next)
         {

         if (is_mort)
            if ((STATE(i)==CON_PLAYING) && (i != ch->desc) &&
                    (GET_LEVEL(i->character) >= LVL_IMMORT))
               act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);

         if (!is_mort)
            if ((STATE(i)==CON_PLAYING) && i != ch->desc)
               {
               if(!IS_NPC(ch) && GET_LEVEL(i->character)<GET_INVIS_LEV(ch))
                  act(buf2, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
               else
                  act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
               }
         }
      }

   release_buffer(buf);
   release_buffer(buf2);
   }


void do_april_fools_drunk(struct char_data* ch, char* argument)
{
  if( !IS_NPC(ch)
      && GET_COND(ch, DRUNK) > 0
      && (GET_LEVEL(ch) > 30 || REMORT_LEVEL(ch) > 0)
      && GET_LEVEL(ch) < LVL_IMMORT ) {
    int i;
    for( i = 0; i < strlen(argument); i++ ) {
      if( rand() % 25 > 25 - GET_COND(ch, DRUNK) ) {
	argument[i] = (rand()%2?'a':'A') + (rand() % 26);
      }
    }
  }
}

ACMD(do_say)
   {
   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   skip_spaces(&argument);

   if(!IS_NPC(ch)&&PLR_FLAGGED(ch, PLR_NOSHOUT))
      send_to_char(ch,"You can only commune with the gods!\r\n");
   else if (!*argument)
      send_to_char(ch,"Yes, but WHAT do you want to say?\r\n");
   else
      {
	do_april_fools_drunk(ch, argument);

      char *buf = get_buffer(MAX_STRING_LENGTH);
      sprintf(buf, "$n says, '&W%s&n'", argument);
      MOBTrigger = FALSE;
      act(buf, FALSE, ch, 0, 0, TO_ROOM|DG_NO_TRIG);
      if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(ch,"%s", OK);
      else
         {
         send_to_char(ch,"You say, '&W%s&n'\r\n", argument);
         }
      release_buffer(buf);

      /* trigger check */
      speech_mtrigger(ch, argument);
      speech_wtrigger(ch, argument);

      mprog_speech_trigger(argument, ch);
      }
   }


ACMD(do_gsay)
   {
   struct char_data *k;
   struct follow_type *f;

   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   skip_spaces(&argument);

   if(!IS_NPC(ch)&&PLR_FLAGGED(ch, PLR_NOSHOUT))
      {
      send_to_char(ch,"You can only commune with the gods!\r\n");
      return;
      }

   if (!AFF_FLAGGED(ch, AFF_GROUP))
      {
      send_to_char(ch,"But you are not the member of a group!\r\n");
      return;
      }
   if (!*argument)
      send_to_char(ch,"Yes, but WHAT do you want to group-say?\r\n");
   else
      {
      char *buf = get_buffer(MAX_STRING_LENGTH);
      if (ch->master)
         k = ch->master;
      else
         k = ch;

      sprintf(buf, "$n tells the group, '&W%s&n'", argument);

      if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch))
         act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
      for (f = k->followers; f; f = f->next)
         if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch))
            act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);

      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(ch, "%s", OK);
      else
         send_to_char(ch, "You tell the group, '&W%s&n'\r\n", argument);
      release_buffer(buf);
      }

   }


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
   {
   char *buf = get_buffer(MAX_STRING_LENGTH);
   char *buf2 = get_buffer(MAX_STRING_LENGTH);
   sprintf(buf, "%s%s tells you, '%s'%s",CCRED(vict, C_NRM), 
           CAP(strdup(GET_NAME(ch))), arg, CCNRM(vict, C_NRM));
   sprintf(buf2, "%sAn immortal tells you, '%s'%s",CCRED(vict, C_NRM), arg,
           CCNRM(vict, C_NRM));
   if (!(!IS_NPC(vict) && ignoring(vict, ch) && GET_LEVEL(vict)<LVL_IMMORT)) {
      if (GET_LEVEL(ch) >= LVL_IMMORT && !CAN_SEE(vict, ch))
      act(buf2, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      else
      act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      }

   if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", OK);
   else
      {
      sprintf(buf, "%sYou tell $N, '%s'%s",CCRED(ch, C_CMP), arg,
              CCNRM(ch, C_CMP));
      sprintf(buf2, "%sYou tell an immortal, '%s'%s",CCRED(ch, C_CMP), arg,
              CCNRM(ch, C_CMP));
      if  (GET_LEVEL(vict) >= LVL_IMMORT && !CAN_SEE(ch, vict))
      act(buf2, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      else
      act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      }
   if(!IS_NPC(vict) && !(IS_NPC(ch) && GET_LEVEL(vict)<LVL_IMMORT))
      GET_LAST_TELL(vict) = IS_NPC(ch)?GET_ID(ch):GET_IDNUM(ch);
   release_buffer(buf2);
   release_buffer(buf);
   }
   
   
bool is_tell_ok(struct char_data *ch, struct char_data *vict)
   {
   if (ch == vict)
      send_to_char(ch,"You try to tell yourself something.\r\n");
   else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF)&&
            GET_LEVEL(ch)<LVL_IMMORT)
      send_to_char(ch,"The walls seem to absorb your words.\r\n");
   else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
      act("$E's linkless at the moment.", FALSE, ch, 0,vict,TO_CHAR|TO_SLEEP);
   else if (!IS_NPC(vict) && PLR_FLAGGED(vict, PLR_WRITING))
      act("$E's writing a message right now; try again later.", FALSE, ch, 0,
          vict, TO_CHAR | TO_SLEEP);
   else if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOTELL))
      {
      if(GET_LEVEL(ch)<LVL_IMMORT)
         send_to_char(ch, "You can't tell other people while you have notell on.\r\n");
      else
         {
         send_to_char(ch, "They are notell.\r\n");
         return (TRUE);
         }
      }
   else if ((!IS_NPC(vict)&&PRF_FLAGGED(vict, PRF_NOTELL)) ||
            ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF))
      {
      if(GET_LEVEL(ch)<LVL_IMMORT)
         act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      else
         {
         send_to_char(ch, "They are soundproof.\r\n");
         return (TRUE);
         }
      }
   else if (!IS_NPC(vict)&&PRF2_FLAGGED(vict, PRF2_AFK))
      {
      act("$E is away from the keyboard...try again later.", FALSE, ch, 0,
          vict, TO_CHAR);
      return (TRUE);
      }
   else
      return (TRUE);

   return (FALSE);
   }



/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
   {
   struct char_data *vict=NULL;
   char *buf = get_buffer(MAX_STRING_LENGTH);
   char *buf2 = get_buffer(MAX_STRING_LENGTH);

   if(GET_LEVEL(ch)<LVL_IMMORT&&!IS_NPC(ch))
      strip_color(argument);


   half_chop(argument, buf, buf2);
   if(!IS_NPC(ch)&&PLR_FLAGGED(ch, PLR_NOSHOUT))
      send_to_char(ch, "You can only commune with the gods!\r\n");
   else if (!*buf || !*buf2)
      send_to_char(ch, "Who do you wish to tell what??\r\n");
   else if (GET_LEVEL(ch) < LVL_IMMORT &&
            !(vict = get_player_vis(ch, buf, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", NOPERSON);
   else if (GET_LEVEL(ch) >= LVL_IMMORT &&
            !(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", NOPERSON);
   else if(is_tell_ok(ch,vict))
      perform_tell(ch, vict, buf2);
   release_buffer(buf);
   release_buffer(buf2);
   }


ACMD(do_reply)
   {
   struct char_data *tch = character_list;

   if(IS_NPC(ch))
      {
      send_to_char(ch, "Sorry, you can't use reply; use tell instead.\r\n");
      return;
      }

   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   skip_spaces(&argument);

   if(PLR_FLAGGED(ch, PLR_NOSHOUT))
      send_to_char(ch, "You can only commune with the gods!\r\n");
   else if (GET_LAST_TELL(ch) == NOBODY)
      send_to_char(ch, "You have no-one to reply to!\r\n");
   else if (!*argument)
      send_to_char(ch, "What is your reply?\r\n");
   else
      {
      /*
       * Make sure the person you're replying to is still playing by searching
       * for them.  Note, now last tell is stored as player IDnum instead of
       * a pointer, which is much better because it's safer, plus will still
       * work if someone logs out and back in again.
       *
       * XXX: A descriptor list based search would be faster although
       *      we could not find link dead people.  Not that they can
       *      hear tells anyway. :) -gg 2/24/98
       */
      while (tch != NULL && ((IS_NPC(tch)?GET_ID(tch):GET_IDNUM(tch)) != GET_LAST_TELL(ch)))
         tch = tch->next;

      if (tch == NULL)
         send_to_char(ch, "They are no longer playing.\r\n");
      else if(is_tell_ok(ch,tch))
         perform_tell(ch, tch, argument);
      }
   }


ACMD(do_spec_comm)
   {
   struct char_data *vict;
   char *action_sing, *action_plur, *action_others;
   char *buf;
   char *buf2;

   if(!IS_NPC(ch)&&PLR_FLAGGED(ch, PLR_NOSHOUT))
      {
      send_to_char(ch, "You can only commune with the gods!\r\n");
      return;
      }
   buf = get_buffer(MAX_STRING_LENGTH);
   buf2 = get_buffer(MAX_STRING_LENGTH);

   if (subcmd == SCMD_WHISPER)
      {
      action_sing = "whisper to";
      action_plur = "whispers to";
      action_others = "$n whispers something to $N.";
      }
   else
      {
      action_sing = "ask";
      action_plur = "asks";
      action_others = "$n asks $N a question.";
      }

   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   half_chop(argument, buf, buf2);

   if (!*buf || !*buf2)
      {
      send_to_char(ch, "Whom do you want to %s.. and what??\r\n", action_sing);
      }
   else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
      send_to_char(ch, "%s", NOPERSON);
   else if (vict == ch)
      send_to_char(ch, "You can't get your mouth close enough to your ear...\r\n");
   else
      {
      sprintf(buf, "$n %s you, '%s'", action_plur, buf2);
      if (!(!IS_NPC(vict) && ignoring(vict, ch) && GET_LEVEL(vict)<LVL_IMMORT))
         act(buf, FALSE, ch, 0, vict, TO_VICT);

      if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(ch, "%s", OK);
      else
         {
         send_to_char(ch, "You %s %s, '%s'\r\n", action_sing,
                      GET_NAME(vict), buf2);
         }
      act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
      if(vict)
         {
         speech_mtrigger(vict, buf2);
         speech_wtrigger(vict, buf2);
         }
      }

   release_buffer(buf);
   release_buffer(buf2);
   }



ACMD(do_write)
   {
   struct obj_data *paper = NULL, *pen = NULL;
   char *papername, *penname;
   char *buf1 = get_buffer(MAX_STRING_LENGTH);
   char *buf2 = get_buffer(MAX_STRING_LENGTH);

   papername = buf1;
   penname = buf2;

   two_arguments(argument, papername, penname);

   if (!ch->desc)
      {
      release_buffer(buf1);
      release_buffer(buf2);
      return;
      }
   else if (!*papername)
      {  /* nothing was delivered */
      send_to_char(ch, "Write?  With what?  ON what?  What are you trying to do?!?\r\n");
      release_buffer(buf1);
      release_buffer(buf2);
      return;
      }
   else if (*penname)
      {  /* there were two arguments */
      if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
         {
         send_to_char(ch, "You have no %s.\r\n", papername);
         release_buffer(buf1);
         release_buffer(buf2);
         return;
         }
      if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying)))
         {
         send_to_char(ch, "You have no %s.\r\n", penname);
         release_buffer(buf1);
         release_buffer(buf2);
         return;
         }
      }
   else
      {  /* there was one arg.. let's see what we can find */
      if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
         {
         send_to_char(ch, "There is no %s in your inventory.\r\n", papername);
         release_buffer(buf1);
         release_buffer(buf2);
         return;
         }
      if (GET_OBJ_TYPE(paper) == ITEM_PEN)
         { /* oops, a pen.. */
         pen = paper;
         paper = NULL;
         }
      else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
         {
         send_to_char(ch, "That thing has nothing to do with writing.\r\n");
         release_buffer(buf1);
         release_buffer(buf2);
         return;
         }
      /* One object was found.. now for the other one. */
      if (!GET_EQ(ch, WEAR_HOLD_1))
         {
         send_to_char(ch, "You can't write with %s %s alone.\r\n",
                      AN(papername), papername);
         release_buffer(buf1);
         release_buffer(buf2);
         return;
         }
      if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD_1)))
         {
         send_to_char(ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
         release_buffer(buf1);
         release_buffer(buf2);
         return;
         }
      if (pen)
         paper = GET_EQ(ch, WEAR_HOLD_1);
      else
         pen = GET_EQ(ch, WEAR_HOLD_1);
      }


   /* ok.. now let's see what kind of stuff we've found */
   if (GET_OBJ_TYPE(pen) != ITEM_PEN)
      act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
   else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
      act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
   else if (paper->action_description)
      send_to_char(ch, "There's something written on it already.\r\n");
   else
      {
      /* we can write - hooray! */
      /* this is the PERFECT code example of how to set up:
       * a) the text editor with a message already loaed
       * b) the abort buffer if the player aborts the message
       */
      ch->desc->backstr = NULL;
      send_to_char(ch, "Write your note.  (/s saves /h for help)\r\n");
      /* ok, here we check for a message ALREADY on the paper */
      if (paper->action_description)
         {
         /* we str_dup the original text to the descriptors->backstr */
         ch->desc->backstr = str_dup(paper->action_description);
         /* send to the player what was on the paper (cause this is already */
         /* loaded into the editor) */
         send_to_char(ch, "%s", paper->action_description);
         }
      act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
      /* assign the descriptor's->str the value of the pointer to the text */
      /* pointer so that we can reallocate as needed (hopefully that made */
      /* sense :>) */
      SET_BIT(GET_OBJ_EXTRA(paper),ITEM_UNIQUE_SAVE);
      string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH,
                   0, NULL);
      }
   release_buffer(buf1);
   release_buffer(buf2);

   }


ACMD(do_page)
   {
   struct descriptor_data *d;
   struct char_data *vict;
   char *arg = get_buffer(MAX_STRING_LENGTH);
   char *buf2 = get_buffer(MAX_STRING_LENGTH);

   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   half_chop(argument, arg, buf2);

   if(!IS_NPC(ch)&&PLR_FLAGGED(ch, PLR_NOSHOUT))
      send_to_char(ch, "You can only commune with the gods!\r\n");
   else if (IS_NPC(ch))
      send_to_char(ch,"Monsters can't page.. go away.\r\n");
   else if (!*arg)
      send_to_char(ch, "Whom do you wish to page?\r\n");
   else
      {
      char *buf = get_buffer(MAX_STRING_LENGTH);

      sprintf(buf, "\007\007*$n* %s\r\n", buf2);
      if (!str_cmp(arg, "all"))
         {
         if (GET_LEVEL(ch) > LVL_DGOD)
            {
            for (d = descriptor_list; d; d = d->next)
               if ((STATE(d)==CON_PLAYING)&& d->character)
                  act(buf, FALSE, ch, 0, d->character, TO_VICT);
            }
         else
            send_to_char(ch, "You will never be godly enough to do that!\r\n");
         release_buffer(arg);
         release_buffer(buf2);
         release_buffer(buf);

         return;
         }
      if ((vict = get_player_vis(ch, arg, FIND_CHAR_WORLD)) != NULL)
         {
         if(GET_LEVEL(ch)<LVL_GOD && !PRF2_FLAGGED(vict,PRF2_PAGE_OK))
            send_to_char(ch, "They do not wish to be paged!\r\n");
         else
            {
            if (!(!IS_NPC(vict) && ignoring(vict, ch) && GET_LEVEL(vict)<LVL_IMMORT))
               act(buf, FALSE, ch, 0, vict, TO_VICT);

            if (PRF_FLAGGED(ch, PRF_NOREPEAT))
               send_to_char(ch, "%s", OK);
            else
               act(buf, FALSE, ch, 0, vict, TO_CHAR);
            }
         }
      else
         send_to_char(ch, "There is no such person in the game!\r\n");
      release_buffer(buf);
      }
   release_buffer(arg);
   release_buffer(buf2);
   }


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
   {
   struct descriptor_data *i;
   struct char_data *tch;
   char *color_on, *buf;


   /* Array of flags which must _not_ be set in order for comm to be heard */
   int channels[] =
      {
         0,
         PRF_DEAF,
         PRF_NOGOSS,
         PRF_NOAUCT,
         PRF_NOGRATZ,
         PRF2_NOOOC,
         PRF2_NOMUSIC,
         0
      };

   /*
    * com_msgs: [0] Message if you can't perform the action because of noshout
    *           [1] name of the action
    *           [2] message if you're not on the channel
    *           [3] a color string.
    */
   char *com_msgs[][4] =
      {
         {"You cannot holler!!\r\n",
          "holler",
          "",
          KYEL},

         {"You cannot shout!!\r\n",
          "shout",
          "Turn off your noshout flag first!\r\n",
          KYEL},

         {"You cannot gossip!!\r\n",
          "gossip",
          "You aren't even on the channel!\r\n",
          KYEL},

         {"You cannot auction!!\r\n",
          "auction",
          "You aren't even on the channel!\r\n",
          KMAG},

         {"You cannot congratulate!\r\n",
          "congrat",
          "You aren't even on the channel!\r\n",
          KGRN},

         { "You cannot ooc!!\r\n",
           "ooc",
           "You aren't even on the channel!\r\n",
           KWHT},

         { "Better get some vocal chords first!!\r\n",
           "sing",
           "You aren't even on the channel!\r\n",
           "&y"}
      };


   /* to keep pets, etc from being ordered to shout */
   if ((!ch->desc)&&(ch->master))
      return;

   if(!IS_NPC(ch)&&PLR_FLAGGED(ch, PLR_NOSHOUT))
      {
      send_to_char(ch, "You can only commune with the gods!\r\n");
      return;
      }

   if (PLR_FLAGGED(ch, PLR_NOSHOUT))
      {
      send_to_char(ch, "%s", com_msgs[subcmd][0]);
      return;
      }
   else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF)&&
            GET_LEVEL(ch)<LVL_IMMORT)
      {
      send_to_char(ch, "The walls seem to absorb your words.\r\n");
      return;
      }
   /* level_can_shout defined in config.c */
   if (GET_LEVEL(ch) < level_can_shout)
      {
      send_to_char(ch, "You must be at least level %d before you can %s.\r\n",
                   level_can_shout, com_msgs[subcmd][1]);
      return;
      }

   /* make sure the char is on the channel */
   if(subcmd<SCMD_OOC)
      {
      if (!IS_NPC(ch)&&PRF_FLAGGED(ch, channels[subcmd]))
         {
         send_to_char(ch, "%s", com_msgs[subcmd][2]);
         return;
         }
      }
   else
      {
      if (!IS_NPC(ch)&&PRF2_FLAGGED(ch, channels[subcmd]))
         {
         send_to_char(ch, "%s", com_msgs[subcmd][2]);
         return;
         }
      }
   /* skip leading spaces */
   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   skip_spaces(&argument);

   /* make sure that there is something there to say! */
   if (!*argument)
      {
      send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n",
                   com_msgs[subcmd][1], com_msgs[subcmd][1]);
      return;
      }

   if (subcmd == SCMD_HOLLER)
      {
      if (GET_MOVE(ch) < holler_move_cost)
         {
         send_to_char(ch, "You're too exhausted to holler.\r\n");
         return;
         }
      else
         GET_MOVE(ch) -= holler_move_cost;
      }
   /* set up the color on code */
   color_on = com_msgs[subcmd][3];

   do_april_fools_drunk(ch, argument);

   /* first, set up strings to be given to the communicator */
   if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", OK);
   else
      {
      char *buf1 = get_buffer(MAX_STRING_LENGTH);
      if (COLOR_LEV(ch) >= C_CMP) 
         sprintf(buf1, "%s%sYou %s, '%s'%s", color_on,
                 (subcmd==SCMD_MUSIC)?"[MUSIC] ":"", com_msgs[subcmd][1],
                 argument, KNRM);
      else
         sprintf(buf1, "%sYou %s, '%s'", (subcmd==SCMD_MUSIC)?"[MUSIC] ":"", 
                 com_msgs[subcmd][1], argument);
      act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
      release_buffer(buf1);
      }

   buf = get_buffer(MAX_STRING_LENGTH);
   sprintf(buf, "%s$n %ss, '%s'", (subcmd==SCMD_MUSIC)?"[MUSIC] ":"",
           com_msgs[subcmd][1], argument);

   /* now send all the strings out */
   for (i = descriptor_list; i; i = i->next)
      {
      if(i->original)
         tch=i->original;
      else
         tch= i->character;

      if ((STATE(i)==CON_PLAYING) && (i != ch->desc) && tch &&
              i->character &&
              ((!PRF_FLAGGED(tch, channels[subcmd])&&
                (subcmd<SCMD_OOC))||
               (!PRF2_FLAGGED(tch, channels[subcmd])&&
                (subcmd>=SCMD_OOC))) &&
              !PLR_FLAGGED(tch, PLR_WRITING) &&
              (!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF)||
               GET_LEVEL(ch)>=LVL_IMMORT))
         {
         if (subcmd == SCMD_SHOUT &&
                 ((world[IN_ROOM(ch)].zone != world[IN_ROOM(i->character)].zone) ||
                  !AWAKE(i->character)))
            continue;

         if (subcmd == SCMD_HOLLER &&
                 ((zone_table[world[IN_ROOM(ch)].zone].continent != zone_table[world[IN_ROOM(i->character)].zone].continent) ||
                  !AWAKE(i->character)))
            continue;

         if (!IS_NPC(tch) && ignoring(tch, ch) && GET_LEVEL(tch)<LVL_IMMORT)
            continue;

         if (COLOR_LEV(tch) >= C_NRM)
            send_to_char(i->character, "%s", color_on);
         act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
         if (COLOR_LEV(tch) >= C_NRM)
            send_to_char(i->character, "%s", KNRM);
         }
      }
   release_buffer(buf);
   }


/** 2/24/97, Anduin - Auction code **/
ACMD(do_auction)
   {
   struct obj_data *obj;
   struct obj_data *i;
   int price;
   char  *arg1=get_buffer(MAX_INPUT_LENGTH);
   char  *arg2=get_buffer(MAX_INPUT_LENGTH);


   two_arguments(argument, arg1, arg2);

   if (IS_NPC(ch))
      {
      send_to_char(ch, "Mobs aren't cool enough to auction.\r\n");
      release_buffer(arg1);
      release_buffer(arg2);
      return;
      }
   if (PRF_FLAGGED(ch, PRF_NOAUCT))
      {
      send_to_char(ch,"You aren't even listening to the auction channel!\r\n");
      release_buffer(arg1);
      release_buffer(arg2);
      return;
      }
   if (!*arg1)
      {
      send_to_char(ch, "What do you want to auction?\r\n");
      release_buffer(arg1);
      release_buffer(arg2);
      return;
      }
   if (!*arg2)
      {
      send_to_char(ch, "What price do you want to auction it for?\r\n");
      release_buffer(arg1);
      release_buffer(arg2);
      return;
      }

   if (is_number(arg2))
      {
      price = atoi(arg2);
      release_buffer(arg2);
      }
   else
      {
      send_to_char(ch, "The price must be a number silly!\r\n");
      release_buffer(arg1);
      release_buffer(arg2);
      return;
      }

   if (price < 1)
      {
      send_to_char(ch, "The price must be a POSITIVE number!\r\n");
      release_buffer(arg1);
      return;
      }

   if (aauction.in_progress == TRUE)
      {
      send_to_char(ch, "You must wait until the auction now in progress "
                   "is over!\r\n");
      release_buffer(arg1);
      return;
      }

   if (!ch->carrying)
      {
      send_to_char(ch, "You don't seem to have anything to auction!\r\n");
      release_buffer(arg1);
      return;
      }

   if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
      {
      send_to_char(ch, "You don't seem to have %s %s in your inventory!\r\n",
                   AN(arg1), arg1);
      release_buffer(arg1);
      return;
      }

   release_buffer(arg1);

   if ((GET_OBJ_TYPE(obj) == ITEM_NOTE) ||
           (GET_OBJ_TYPE(obj) == ITEM_KEY))
      {
      send_to_char(ch, "You can't auction THAT!!\r\n");
      return;
      }

   if (IS_OBJ_STAT(obj, ITEM_NODROP)&&
           (IS_NPC(ch)||!PRF_FLAGGED(ch,PRF_NOHASSLE)))
      {
      send_to_char(ch, "How do you expect to auction it if you can't even "
                   "let go of it!\r\n");
      return;
      }

   if (IS_OBJ_STAT(obj, ITEM_NOAUC))
      {
      send_to_char(ch, "You cannot auction that.\r\n");
      return;
      }

   if (IS_OBJ_STAT(obj, ITEM_DONATED))
      {
      send_auction_god("[AUCTION] %s tried to auction %s asking %d coins\r\n"
                       "          which was stolen from donation.\r\n",
                       GET_NAME(ch),obj->short_description, price);
      send_auction_mort("[AUCTION] %s tried to auction %s asking %d coins\r\n"
                        "          which was stolen from donation.\r\n",
                        GET_NAME(ch),obj->short_description, price);
/*      mudlogf(NRM,MAX(LVL_IMMORT, GET_INVIS_LEV(ch)),TRUE,
              "%s tried to auction %s for %d from donation.",
              GET_NAME(ch), obj->short_description, price);
*/
      send_to_char(ch, "CHA! RIGHT!!\r\n");
      return;
      }



   if (GET_OBJ_RNUM(obj) <= 0)
      {
      send_to_char(ch, "You can't auction THAT!!\r\n");
      return;
      }
   /* odinian 8/23/00 */
   /* see if the object is a container */
   if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
      {
      /* see if the container contains anything */
      if (obj->contains)
         {
         for (i = obj->contains ; i; i = i->next_content)
            {
            if (IS_OBJ_STAT(i, ITEM_NOAUC) ||
                    (GET_OBJ_TYPE(i) == ITEM_NOTE) ||
                    (GET_OBJ_TYPE(i) == ITEM_KEY))
               {
               send_to_char(ch, "Shame on you!  Trying to auction a "
                            "container with a no auction item in it.\r\n");
               return;
               }
            if (IS_OBJ_STAT(i, ITEM_DONATED))
               {
               send_auction_god("[AUCTION] %s tried to auction %s asking %d coins\r\n"
                                "          which was stolen from donation.\r\n",
                                GET_NAME(ch),i->short_description, price);
               send_auction_mort("[AUCTION] %s tried to auction %s asking %d coins\r\n"
                                 "          which was stolen from donation.\r\n",
                                 GET_NAME(ch),i->short_description, price);
               mudlogf(NRM,MAX(LVL_IMMORT, GET_INVIS_LEV(ch)),TRUE,
                       "%s tried to auction %s for %d from donation.",
                       GET_NAME(ch), i->short_description, price);
               send_to_char(ch, "CHA! RIGHT!!\r\n");
               return;
               }
            }
         }
      }


   /* odinian, 10/26/99
    * check to make sure the seller has the gold to cover
    * the cost of the auction.
    */
   if (GET_GOLD(ch) < auction_cost)
      {
      send_to_char(ch, "Sorry, you don't have enough coins to "
                   "start an auction.\r\n");
      return;
      }

   if(price<1)
      {
      send_to_char(ch, "You need to sell it for at least 1 coin.\r\n");
      return;
      }
   else if(price>(((GET_OBJ_COST(obj)>100?GET_OBJ_COST(obj):100)*10)+10))
      {
      send_to_char(ch, "You may begin the auction for no more "
                   "than %d coins.\r\n",
                  (((GET_OBJ_COST(obj)>100?GET_OBJ_COST(obj):100)*10)+10));
      return;
      }

   /* odinian, 10/26/99
      get the gold from the player
      */
   GET_GOLD(ch) -= auction_cost;


   /* passed all the checks now we begin the auction */
   send_to_char(ch, "You begin to auction %s asking %d coins.\r\n",
                obj->short_description, price);
   send_auction_mort("[AUCTION] Someone begins auctioning %s "
                     "asking %d coins.\r\n", obj->short_description, price);
   send_auction_god("[AUCTION] %s begins auctioning %s asking %d "
                    "coins.[%ld]\r\n", GET_NAME(ch),obj->short_description,
                    price,GET_OBJ_VNUM(obj));
   mudlogf(NRM,MAX(LVL_IMMORT, GET_INVIS_LEV(ch)),TRUE,
           "%s auctions %s for %d.[%ld]", GET_NAME(ch),
           obj->short_description, price,GET_OBJ_VNUM(obj));

   aauction.item_auc = GET_OBJ_VNUM(obj);
   obj_from_char(obj);
   aauction.obj =  obj;
   save_char(ch, IN_ROOM(ch));
   aauction.in_progress = TRUE;
   aauction.bid_on = FALSE;
   aauction.seller_id_num = GET_IDNUM(ch);
   aauction.previous_bid = 0;
   aauction.last_bid = 0;
   aauction.previous_bidder_id_num = 0;
   aauction.bidder_id_num = 0;
   aauction.selling_price = price;
   aauction.state_of_sale = 0;
   auction_event=add_function_to_queue(45,NULL,0,0,do_auction_update);
   }

/**2/24/97 Anduin - bid part of the auction **/
ACMD(do_bid)
   {

   struct show_struct
      {
      char      *cmd;
      }
   fields[] =
      {
         { "test" },
         { "Going" },
         { "Going Once" },
         { "Going Twice" },
         { "Going Three Times" },
         { "Sold!" },
         { "\n" }
      };
   struct obj_data *obj, *obj2;
   struct descriptor_data *tch, *next_dude;
   int bid;
   int bid2, now_bid;
   char *arg1;

   if (IS_NPC(ch))
      {
      return;
      }

   if (PRF_FLAGGED(ch, PRF_NOAUCT))
      {
      send_to_char(ch,"You aren't even listening to the auction channel!\r\n");
      return;
      }

   if (aauction.in_progress == FALSE)
      {
      send_to_char(ch, "There is no auction going on at the moment!\r\n");
      return;
      }

   if((GET_IDNUM(ch) == aauction.seller_id_num)&&(GET_LEVEL(ch)<LVL_ADMIN))
      {
      send_to_char(ch, "The current bid for %s is %ld.\r\n",
                   (aauction.obj)->short_description,aauction.last_bid);
      if(aauction.state_of_sale==0)
         send_to_char(ch, "  No bids.\r\n");
      else
         send_to_char(ch,"  %s.\r\n",fields[aauction.state_of_sale-1].cmd);
      return;
      }

   arg1 = get_buffer(MAX_STRING_LENGTH);
   one_argument(argument, arg1);
   /* free identify */
   if ((!*arg1))
      {
      send_to_char(ch, "The auctioneer tells you:\r\n");

      id_obj_to_char(ch,aauction.obj);

      /* Gods should be able to see who is auctioning */
      if(GET_LEVEL(ch)>=LVL_DGOD)
         {
         for (tch = descriptor_list; tch; tch = next_dude)
            {
            next_dude = tch->next;
            if ((aauction.bid_on>0)&&(tch->character) &&
                    (GET_IDNUM(tch->character) == aauction.bidder_id_num))
               {
               send_to_char(ch, "\r\n %s is the highest bidder at %ld",
                            GET_NAME(tch->character),aauction.last_bid);
               }
            else if ((tch->character) &&
                     (GET_IDNUM(tch->character) == aauction.seller_id_num))
               {
               send_to_char(ch, "\r\n %s started the auction at %ld",
                            GET_NAME(tch->character),aauction.selling_price);
               }
            }
         send_to_char(ch,"\r\n");
         }
      if(aauction.state_of_sale==0)
         {
         send_to_char(ch, "\r\n Bid is starting at %ld.\r\n",
                      aauction.selling_price);
         }
      else if(aauction.state_of_sale==1)
         {
         send_to_char(ch,"\r\n Someone has started the bidding at %ld.\r\n",
                      aauction.last_bid);
         }
      else
         {
         send_to_char(ch,"\r\n %s for %ld.\r\n",
                      fields[aauction.state_of_sale-1].cmd, aauction.last_bid);
         }
      if(GET_IDNUM(ch) == aauction.bidder_id_num)
         send_to_char(ch, " You are the current bidder.\r\n");
      release_buffer(arg1);
      return;
      }

   /* Get the bid off the person */

   if (is_number(arg1))
      {
      bid = atoi(arg1);
      release_buffer(arg1);
      }
   else if(!strcmp(arg1,"stop")&&(GET_LEVEL(ch)>=LVL_ADMIN))
      {
      obj = aauction.obj;
      send_auction_mort("[AUCTION] Auction has been stopped by a higher power!\r\n");
      send_auction_god("[AUCTION] Auction has been stopped by %s!\r\n",
                       GET_NAME(ch));
      mudlogf(NRM,MAX(LVL_IMMORT, GET_INVIS_LEV(ch)),TRUE,
              "[AUCTION] %s stopped the auction of %s [%ld bid]"
              "[%ld sell price].",
              GET_NAME(ch),
              obj->short_description,
              aauction.last_bid,
              aauction.selling_price);
      obj_to_char(obj, ch);
      aauction.in_progress = FALSE;/*Wether an auction is taking place */
      aauction.bid_on = FALSE;     /*Wether the item has been bid upon */
      aauction.previous_bid = 0;
      aauction.last_bid = 0;       /* The last bid on the item */
      aauction.previous_bidder_id_num = 0;
      aauction.bidder_id_num = 0;  /*The person that last bid idnumber */
      aauction.seller_id_num = 0;  /*The person that is selling's idnum*/
      aauction.selling_price = 0;  /* Price asking for the item */
      aauction.state_of_sale = 0;  /* Going 1, 2, 3, sold */
      aauction.item_auc = 0;

      del_event_queue(auction_event);
      auction_event=NULL;
      release_buffer(arg1);
      return;

      }
   else
      {
      send_to_char(ch, "The bid must be a number silly!\r\n");
      release_buffer(arg1);
      return;
      }

   if (bid < 1)
      {
      send_to_char(ch, "The bid must be at least one coin!\r\n");
      return;
      }

   if (bid < aauction.selling_price)
      {
      send_to_char(ch, "You must at least the asking price!\r\n");
      return;
      }


   if (bid > get_char_gold(ch))
      {
      send_to_char(ch, "You dont have that many coins on hand!!\r\n");
      return;
      }

   if (aauction.last_bid >= bid)
      {
      send_to_char(ch,"Someone has already out bid you! "
                   "You must bid higher!\r\n");
      return;
      }

   if ((aauction.last_bid != 0))
      {
      bid2 = (int)((float)aauction.last_bid * (float)0.1);
      bid2=MIN(bid2,5000);
      now_bid = aauction.last_bid + bid2;
      if (now_bid > bid)
         {
         send_to_char(ch,"You must bid at least %d coins!\r\n", now_bid);
         return;
         }
      }

   obj2 = aauction.obj;

   /* passed all the checks now we file the bid */
   send_to_char(ch,"You bid %d coins for %s.\r\n",bid,obj2->short_description);
   send_auction_mort("[AUCTION] Someone bids %d coins on %s.\r\n", bid,
                     obj2->short_description);
   send_auction_god("[AUCTION] %s bids %d coins on %s.\r\n", GET_NAME(ch),bid,
                    obj2->short_description);
   mudlogf(NRM,MAX(LVL_IMMORT, GET_INVIS_LEV(ch)),TRUE,
           "%s bids %d coins on %s.", GET_NAME(ch),bid,obj2->short_description);

   aauction.bid_on = TRUE;
   if (aauction.previous_bidder_id_num != aauction.bidder_id_num)
      {
      aauction.previous_bidder_id_num = aauction.bidder_id_num;
      aauction.previous_bid = aauction.last_bid;
      }
   aauction.bidder_id_num = GET_IDNUM(ch);
   aauction.last_bid = bid;
   aauction.state_of_sale = 2;
   del_event_queue(auction_event);
   auction_event=NULL;
   auction_event=add_function_to_queue(30,NULL,0,0,do_auction_update);
   /* now the update auction function in the game loop takes it away */
   }

/** 2/24/97, Anduin - do_auction_update **/

void do_auction_update(void)
   {
   struct descriptor_data *tch, *next_dude;
   struct char_data *target =0;
   struct obj_data *obj;

   /* odinian, 10/26/99
      var needed to determine the auctioneer's cur of the take
      */
   int profit;

   bool here;
   struct show_struct {
      char      *cmd;
      }
   fields[] =
      {
         { "test" },
         { "Going" },
         { "Going Once" },
         { "Going Twice" },
         { "Going Three Times" },
         { "Sold!" },
         { "\n" }
      };

   if (aauction.in_progress == TRUE)
      {
      obj = aauction.obj;
      /********** Is bidder here ********/

      here = FALSE;

      if (aauction.bid_on != FALSE)
         {
         for (tch = descriptor_list; tch; tch = next_dude)
            {
            next_dude = tch->next;

            if ((tch->character) &&
                    (GET_IDNUM(tch->character) == aauction.bidder_id_num))
               {
               target = tch->character;
               here = TRUE;
               }
            }
         if (here == FALSE)
            {
            for (tch = descriptor_list; tch; tch = next_dude)
               {
               next_dude = tch->next;

               if ((tch->character) &&
                       (GET_IDNUM(tch->character) == aauction.previous_bidder_id_num))
                  {
                  target = tch->character;
                  here = TRUE;
                  }
               }
            if (here == TRUE)
               {
               send_auction_mort("[AUCTION] Auction transfered to the previous bidder "
                                 "for %ld coins, since the current bidder has left!\r\n",
                                 aauction.previous_bid);
               send_auction_god("[AUCTION] Auction transfered to the previous bidder "
                                "for %ld coins, since the current bidder has left!\r\n",
                                aauction.previous_bid);
               aauction.bidder_id_num = aauction.previous_bidder_id_num;
               aauction.previous_bidder_id_num = 0;
               aauction.last_bid = aauction.previous_bid;
               aauction.previous_bid = 0;
               aauction.state_of_sale = 1;
               }
            else
               {
               send_auction_mort("[AUCTION] Auction has been cancelled because "
                                 "the highest bidder has left!\r\n");
               send_auction_god("[AUCTION] Auction has been cancelled because "
                                "the highest bidder has left!\r\n");
               here = FALSE;
               for (tch = descriptor_list; tch; tch = next_dude)
                  {
                  next_dude = tch->next;
                  if ((tch->character) &&
                          (GET_IDNUM(tch->character) == aauction.seller_id_num))
                     {
                     target = tch->character;
                     here = TRUE;
                     }
                  }

               if (here == TRUE)
                  {
                  obj_to_char(obj, target);
                  }
               else
                  {
                  obj_to_room(obj,real_room(3063));
                  }
               aauction.in_progress = FALSE;/*Whether an auction is taking place */
               aauction.bid_on = FALSE;     /*Whether the item has been bid upon */
               aauction.previous_bid = 0;
               aauction.last_bid = 0;       /* The last bid on the item */
               aauction.previous_bidder_id_num = 0;
               aauction.bidder_id_num = 0;  /*The person that last bid idnumber */
               aauction.seller_id_num = 0;  /*The person that is selling's idnum*/
               aauction.selling_price = 0;  /* Price asking for the item */
               aauction.state_of_sale = 0;  /* Going 1, 2, 3, sold */
               aauction.item_auc = 0;
               return;
               }
            }
         }

      /********* Is seller still here? *******/

      here = FALSE;

      for (tch = descriptor_list; tch; tch = next_dude)
         {
         next_dude = tch->next;
         if ((tch->character) &&
                 (GET_IDNUM(tch->character) == aauction.seller_id_num))
            {
            target = tch->character;
            here = TRUE;
            }
         }

      if (here == FALSE)
         {
         send_auction_god("[AUCTION] Auction has been canceled because the "
                          "seller has left!\r\n");
         send_auction_mort("[AUCTION] Auction has been canceled because the "
                           "seller has left!\r\n");
         aauction.in_progress = FALSE; /* Wether an auction is taking place */
         aauction.bid_on = FALSE;    /* Wether the item has been bid upon */
         aauction.previous_bid = 0;
         aauction.last_bid = 0;      /* The last bid on the item */
         aauction.previous_bidder_id_num = 0;
         aauction.bidder_id_num = 0; /* The person that last bid idnumber */
         aauction.seller_id_num = 0; /* The person that is selling's idnum */
         aauction.selling_price = 0; /* Price asking for the item */
         aauction.state_of_sale = 0; /* Going 1, 2, 3, sold */
         obj_to_room(obj,real_room(3063));
         aauction.item_auc = 0;
         return;
         }

      /********** N0 Interest on the item *****/

      if ((aauction.state_of_sale == 0) && (aauction.bid_on == FALSE))
         {
         send_auction_mort("[AUCTION]  Auction cancelled due to lack of "
                           "interest on %s!\r\n", obj->short_description);
         send_auction_god("[AUCTION]  Auction cancelled due to lack of "
                          "interest on %s!\r\n", obj->short_description);
         /* search for the seller and give back item */
         here = FALSE;

         for (tch = descriptor_list; tch; tch = next_dude)
            {
            next_dude = tch->next;
            if ((tch->character) &&
                    (GET_IDNUM(tch->character) == aauction.seller_id_num))
               {
               target = tch->character;
               here = TRUE;
               }
            }

         if (here == TRUE)
            {
            obj_to_char(obj, target);
            save_char(target, IN_ROOM(target));
            }
         else
            {
            obj_to_room(obj,real_room(3063));
            }
         aauction.in_progress = FALSE;/*Wether an auction is taking place */
         aauction.bid_on = FALSE;    /* Wether the item has been bid upon */
         aauction.previous_bid = 0;
         aauction.last_bid = 0;      /* The last bid on the item */
         aauction.previous_bidder_id_num = 0;
         aauction.bidder_id_num = 0; /* The person that last bid idnumber */
         aauction.seller_id_num = 0; /* The person that is selling's idnum*/
         aauction.selling_price = 0; /* Price asking for the item */
         aauction.state_of_sale = 0; /* Going 1, 2, 3, sold */
         aauction.item_auc = 0;
         return;
         }

      if (aauction.state_of_sale==1)
         {
         aauction.state_of_sale = aauction.state_of_sale+1;
         }
      else
         {
         send_auction_mort("[AUCTION] %s for %ld coins! (%s)\r\n",
                           fields[aauction.state_of_sale].cmd,
                           aauction.last_bid, obj->short_description);
         send_auction_god("[AUCTION] %s for %ld coins! (%s)\r\n",
                          fields[aauction.state_of_sale].cmd,
                          aauction.last_bid, obj->short_description);
         if (aauction.state_of_sale != 5)
            {
            aauction.state_of_sale = (aauction.state_of_sale + 1);
            }
         else
            {
            send_auction_mort("[AUCTION] The Auction Block is now open for "
                              "items!\r\n");
            send_auction_god("[AUCTION] The Auction Block is now open for "
                             "items!\r\n");

            /*****  search for the bidder make sure they have the money *****/

            here = FALSE;
            for (tch = descriptor_list; tch; tch = next_dude)
               {
               next_dude = tch->next;
               if ((tch->character) &&
                       (GET_IDNUM(tch->character) == aauction.bidder_id_num))
                  {
                  target = tch->character;
                  here = TRUE;
                  }
               }

            if (here == TRUE)
               {
               if (get_char_gold(target) < aauction.last_bid)
                  {
                  send_auction_god("[AUCTION] Sale Cancelled because %s doesnt"
                                   " have the cash! \r\n",GET_NAME(target));
                  send_auction_mort("[AUCTION] Sale Cancelled because bidder "
                                    "doesnt have the cash! \r\n");
                  save_char(target, IN_ROOM(target));
                  aauction.in_progress = FALSE;
                  aauction.bid_on = FALSE;
                  aauction.previous_bid = 0;
                  aauction.last_bid = 0;
                  aauction.previous_bidder_id_num = 0;
                  aauction.bidder_id_num = 0;
                  aauction.selling_price = 0;
                  aauction.state_of_sale = 0;
                  aauction.item_auc = 0;

                  /* search for the seller and give back item */
                  here = FALSE;
                  for (tch = descriptor_list; tch; tch = next_dude)
                     {
                     next_dude = tch->next;
                     if ((tch->character) &&
                             (GET_IDNUM(tch->character) == aauction.seller_id_num))
                        {
                        target = tch->character;
                        here = TRUE;
                        }
                     }

                  if (here == TRUE)
                     {
                     obj_to_char(obj, target);
                     }
                  aauction.seller_id_num = 0;
                  return;
                  }
               }


            /* search for the seller and give gold */
            here = FALSE;
            for (tch = descriptor_list; tch; tch = next_dude)
               {
               next_dude = tch->next;
               if ((tch->character) &&
                       (GET_IDNUM(tch->character) == aauction.seller_id_num))
                  {
                  target = tch->character;
                  here = TRUE;
                  }
               }

            if (here == TRUE)
               {
               /* odinian, 10/26/99
               remove percentage of auction bid from the total
               the profit variable holds the player's profit
               */
               profit=(int)((float)(1.0 - auction_profit) * aauction.last_bid);

               GET_GOLD(target)=GET_GOLD(target)+profit;
               save_char(target, IN_ROOM(target));
               send_to_char(target,"You receive %d coins for %s from the "
                            "auction (the auctioneer has retained a small "
                            "percentage).\r\n", profit,obj->short_description);
               }

            /* search for the bidder and give item */
            here = FALSE;
            for (tch = descriptor_list; tch; tch = next_dude)
               {
               next_dude = tch->next;
               if ((tch->character) &&
                       (GET_IDNUM(tch->character) == aauction.bidder_id_num))
                  {
                  target = tch->character;
                  here = TRUE;
                  }
               }

            if (here == TRUE)
               {
               if (get_char_gold(target) >= aauction.last_bid)
                  {
                  charge_char_gold(target, aauction.last_bid);
                  save_char(target, IN_ROOM(target));
                  send_to_char(target, "You bought %s for %ld coins from the "
                               "auction.\r\n", obj->short_description,
                               aauction.last_bid);
                  send_to_char(target, "The auctioneer gives you %s and takes "
                               "%ld coins.\r\n", obj->short_description,
                               aauction.last_bid);
                  obj_to_char(obj,target);
                  }
               aauction.in_progress = FALSE;
               aauction.bid_on = FALSE;
               aauction.previous_bid = 0;
               aauction.last_bid = 0;
               aauction.previous_bidder_id_num = 0;
               aauction.bidder_id_num = 0;
               aauction.seller_id_num = 0;
               aauction.selling_price = 0;
               aauction.state_of_sale = 0;
               aauction.item_auc = 0;
               }
            }
         }
      if(aauction.in_progress==TRUE)
         auction_event=add_function_to_queue(45,NULL,0,0,do_auction_update);
      }
   }
/** End Auction Code , Anduin **/

ACMD(do_qcomm)
   {
   struct descriptor_data *i;
   char *buf;

   if (!IS_NPC(ch)&&!PRF_FLAGGED(ch, PRF_QUEST))
      {
      send_to_char(ch, "You aren't even part of the quest!\r\n");
      return;
      }
   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   skip_spaces(&argument);

   if(!IS_NPC(ch)&&PLR_FLAGGED(ch, PLR_NOSHOUT))
      {
      send_to_char(ch, "You can only commune with the gods!\r\n");
      return;
      }

   if (!*argument)
      {
      buf = get_buffer(128);
      strcpy(buf,CMD_NAME);
      send_to_char(ch, "%s?  Yes, fine, %s we must, but WHAT??\r\n",
                   CAP(buf), CMD_NAME);
      release_buffer(buf);
      }
   else
      {
      buf = get_buffer(512);
      if (!IS_NPC(ch)&&PRF_FLAGGED(ch, PRF_NOREPEAT))
         send_to_char(ch,"%s", OK);
      else
         {
         if (subcmd == SCMD_QSAY)
            sprintf(buf, "&WYou quest-say, '&C%s&W'&n", argument);
         else
            strcpy(buf, argument);
         act(buf, FALSE, ch, 0, argument, TO_CHAR | TO_SLEEP);
         }

      if (subcmd == SCMD_QSAY)
         sprintf(buf, "&W$n quest-says, '&C%s&W'&n", argument);
      else
         strcpy(buf, argument);

      for (i = descriptor_list; i; i = i->next)
         if ((STATE(i)==CON_PLAYING) && (i != ch->desc) &&
                 (IS_NPC(i->character)||PRF_FLAGGED(i->character, PRF_QUEST)) &&
                 i->character && !(!IS_NPC(i->character) && 
                  ignoring(i->character, ch) && GET_LEVEL(i->character)<LVL_IMMORT))
            act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
      release_buffer(buf);
      }
   }




ACMD(do_remortnet)
   {
   struct descriptor_data *d;
   struct char_data *vict;
   char emote = FALSE;
   char any = FALSE;
   char *buf1;
   char *buf2;
   char *buf3;

   if (!IS_NPC(ch) && (GET_LEVEL(ch) < LVL_HERO && REMORT_LEVEL(ch) == 0))
      {
      send_to_char(ch, "Huh?!?\r\n");
      return;
      }
      /* Nomikos 8-24-2002, Made it so morts can't remortnet if muted. */
   if(!IS_NPC(ch)&&PLR_FLAGGED(ch, PLR_NOSHOUT)&&(GET_LEVEL(ch)<LVL_IMMORT))
      {
      send_to_char(ch,"You can only commune with the gods!\r\n");
      return;
      }
      
   if(AFF_FLAGGED(ch,AFF_CHARM))
      {
      send_to_char(ch,"Huh?!?\r\n");
      return;
      }

   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   skip_spaces(&argument);
   delete_doubledollar(argument);



   if (!*argument)
      {
      send_to_char(ch,
                   "Usage: remortnet { <text> | * <text> | @ | + | - }\r\n");
      return;
      }
   switch (*argument)
      {
   case '*':
      emote = TRUE;
      argument++;
      break;
   case '@':
      for (d = descriptor_list; d; d = d->next)
         {
         if ((STATE(d)==CON_PLAYING)&&
	     (GET_LEVEL(d->character) > 100 || REMORT_LEVEL(d->character) > 0) &&
                 !PRF2_FLAGGED(d->original?d->original:d->character, PRF2_NOREMO) &&
                 (CAN_SEE(ch, d->character) || GET_LEVEL(ch) == LVL_IMPL))
            {
            if (!any)
               {
               send_to_char(ch, "People on the remortnet Channel:\r\n");
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
                 ((GET_LEVEL(d->character) > 100)||
		  REMORT_LEVEL(d->character)>0) &&
                 PRF2_FLAGGED(d->original?d->original:d->character, PRF2_NOREMO) &&
                 CAN_SEE(ch, d->character))
            {
            if (!any)
               {
               send_to_char(ch, "Players offline:\r\n");
               any = TRUE;
               }
            send_to_char(ch, "  %s\r\n", GET_NAME(d->character));
            }
         }
      return;
      break;
   case '-':
      if (PRF2_FLAGGED(ch, PRF2_NOREMO))
         send_to_char(ch,"You are already offline!\n\r");
      else
         {
         send_to_char(ch,"You will no longer hear the remortnet.\n\r");
         SET_BIT(PRF2_FLAGS(ch), PRF2_NOREMO);
         }
      return;
      break;
   case '+':
      if (!PRF2_FLAGGED(ch, PRF2_NOREMO))
         send_to_char(ch, "You are already online!\n\r");
      else
         {
         send_to_char(ch, "You can now hear the remortnet again.\n\r");
         REMOVE_BIT(PRF2_FLAGS(ch), PRF2_NOREMO);
         }
      return;
      break;
   case '\\':
      ++argument;
      break;
   default:
      break;
      }
   if (!IS_NPC(ch) && PRF2_FLAGGED(ch, PRF2_NOREMO))
      {
      send_to_char(ch ,"You aren't on remortnet!\r\n");
      return;
      }
   skip_spaces(&argument);


   if (!*argument)
      {
      send_to_char(ch, "Don't bother the people like that!\r\n");
      return;
      }
   buf1=get_buffer(512);
   buf2=get_buffer(512);
   buf3=get_buffer(512);
   sprintf(buf1, "[Remort] %s: %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
           argument);
   sprintf(buf2, "[Remort] Someone: %s%s\r\n", emote ? "<--- " : "", argument);
   sprintf(buf3, "[Remort] An immortal: %s%s\r\n", emote ? "<--- " : "", argument);


   for (d = descriptor_list; d; d = d->next)
      {
      if(d->original)
         vict = d->original;
      else
         vict = d->character;
      if ((STATE(d)==CON_PLAYING) &&
              ((GET_LEVEL(vict) > 100)||
               REMORT_LEVEL(vict) > 0) &&
              (!PRF2_FLAGGED(vict, PRF2_NOREMO)) &&
              (!PLR_FLAGGED(vict, PLR_WRITING | PLR_MAILING))
              && (d != ch->desc || !(PRF_FLAGGED(vict, PRF_NOREPEAT))))
         {
         if (!(!IS_NPC(vict) && ignoring(vict, ch) && GET_LEVEL(vict)<LVL_IMMORT))
            {
            send_to_char(vict, CCYEL(vict, C_NRM));
            if (!CAN_SEE(vict, ch) && GET_LEVEL(ch) >= LVL_IMMORT)
               send_to_char(vict, "%s", buf3);
            else if (!CAN_SEE(vict, ch))
               send_to_char(vict, "%s", buf2);
            else
               send_to_char(vict, "%s", buf1);
            send_to_char(vict, CCNRM(vict, C_NRM));
            }
         }
      }

   release_buffer(buf2);
   release_buffer(buf1);
   release_buffer(buf3);
   if (IS_NPC(ch) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT)))
      send_to_char(ch, "%s", OK);
   }


ACMD(do_ignore)
   {
   struct char_data *vict=NULL;
   char *buf = get_buffer(MAX_STRING_LENGTH);
   struct descriptor_data *d;
   int i;

   any_one_arg(argument, buf);

   if (IS_NPC(ch))
      send_to_char(ch, "You're too ignorant to ignore.\r\n");
   else if (!*buf)
      {
      send_to_char(ch, "Players online that you are currently ignoring:\r\n");
      for (i=0; i<5; i++)
         {
         for (d = descriptor_list; d; d = d->next)
            if (d->character && (GET_IDNUM(d->character) == GET_IGNORED(ch,i)))
               {
               send_to_char(ch, " %s", GET_NAME(d->character));
               break;
               }
         }
      send_to_char(ch, "\r\nUsage: ignore <player>\r\n\r\n");
      send_to_char(ch, "Ignore the player again to stop ignoring them, or\r\n");
      send_to_char(ch, " type: ignore clear   to clear all ignores.\r\n");
      }
   else if (is_abbrev(buf, "clear"))
      {
      for (i=0; i<5; i++)
         GET_IGNORED(ch,i) = 0;
      send_to_char(ch, "You are no longer ignoring anyone.\r\n");
      }
   else if (!(vict = get_player_vis(ch, buf, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", NOPERSON);
   else if (vict == ch)
      send_to_char(ch, "You ignore yourself - hmm hmm la di da... dang, not working.\r\n");
   else if (GET_LEVEL(vict)>GET_LEVEL(ch) && GET_LEVEL(vict)>=LVL_IMMORT)
      send_to_char(ch, "Awww... now why would you ignore a god?\r\n");
   else if (GET_LEVEL(ch)>=LVL_IMMORT && GET_LEVEL(vict)<LVL_IMMORT)
      send_to_char(ch, "Now why would you ignore a mortal? They're so cute and cuddly!\r\n");
   else if (GET_LEVEL(ch)>=LVL_IMMORT) /* temporary */
      send_to_char(ch, "You can't ignore anyone.... for now.\r\n");
   else
      {
      for (i=0; i<5; i++)
         {
         if (GET_IGNORED(ch,i) == GET_IDNUM(vict))
            {
            send_to_char(ch, "You stop ignoring %s.\r\n", GET_NAME(vict));
            GET_IGNORED(ch,i) = 0;
            break;
            }
         else if (GET_IGNORED(ch,i) == 0)
            {
            send_to_char(ch, "You start ignoring %s.\r\n", GET_NAME(vict));
            GET_IGNORED(ch,i) = GET_IDNUM(vict);
            break;
            }
         }
      if (i == 5)
         send_to_char(ch, "Perhaps you should turn off all your channels instead!\r\n");
      }
   release_buffer(buf); 
   }


/* Returns TRUE if vict is ignored, FALSE if not ignored */
int ignoring(struct char_data *ch, struct char_data *vict)
   {
   int i;

   /* can't ignore NPCs */
   if (IS_NPC(vict))
      return FALSE;

   if (GET_LEVEL(vict) >= LVL_ADMIN) {
     return FALSE;
   }

   for (i=0; i<5; i++)
      if (GET_IGNORED(ch,i) == GET_IDNUM(vict))
         return TRUE;

   return FALSE;
   }


extern struct descriptor_data *descriptor_list;

ACMD(do_nonewbie)
{
  if (PRF2_FLAGGED(ch, PRF2_NONEWBIE)) {
    REMOVE_BIT(PRF2_FLAGS(ch), PRF2_NONEWBIE);
    send_to_char(ch, "You can now hear the newbie channel.\r\n");
  } else {
    SET_BIT(PRF2_FLAGS(ch), PRF2_NONEWBIE);
    send_to_char(ch, "You will no longer hear the newbie channel.\r\n");
  }
}

ACMD(do_newbie)
{
  if (IS_NPC(ch)) {
    return;
  }
  if (PRF2_FLAGGED(ch, PRF2_NONEWBIE)) {
    send_to_char(ch, "You aren't listening to the newbie channel.\r\n");
    return;
  }
  if (!*argument) {
    send_to_char(ch, "Usage: newbie <text>\r\n");
    return;
  }

  char buf[1024];
  sprintf(buf, "&C[Newbie] %s:%s&n\r\n", GET_NAME(ch), argument);

  struct descriptor_data *i;
  struct char_data *tch;
  for (i = descriptor_list; i; i = i->next) {
    if (i->original) {
      tch = i->original;
    } else {
      tch = i->character;
    }
    if (STATE(i) == CON_PLAYING
          && tch
          && i->character
          && !PLR_FLAGGED(tch, PLR_WRITING)
	  && (!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) || GET_LEVEL(ch)>=LVL_IMMORT)
    ) {
      if (!IS_NPC(tch) && ignoring(tch, ch) && GET_LEVEL(tch) < LVL_IMMORT) {
	continue;
      }
      send_to_char(i->character, "%s", buf);
    }
  }
}
