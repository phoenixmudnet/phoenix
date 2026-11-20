/* ************************************************************************ 
*   File: act.social.c                                  Part of CircleMUD * 
*  Usage: Functions to handle socials                                     * 
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
#include "constants.h"

/* extern variables */ 
extern struct room_data *world; 
extern struct descriptor_data *descriptor_list; 
extern struct room_data *world; 
extern struct command_info cmd_info[]; 
extern struct zone_data *zone_table;

/* extern functions */ 
SPECIAL(moods);
int get_mood_type(int cmd,int mood);
 
/* local globals */ 
static int list_top = -1; 

/* local functions */ 
char *fread_action(FILE * fl, int nr); 
 
struct social_messg 
{ 
   int act_nr; 
   int hide; 
   int min_victim_position; /* Position of victim */ 
 
  /* No argument was supplied */ 
   char *char_no_arg; 
   char *others_no_arg; 
 
  /* An argument was there, and a victim was found */ 
   char *char_found;  /* if NULL, read no further, ignore args */ 
   char *others_found; 
   char *vict_found; 
 
  /* An argument was there, but no victim was found */ 
   char *not_found; 
 
  /* The victim turned out to be the character */ 
   char *char_auto; 
   char *others_auto; 
}           
*soc_mess_list = NULL; 
 
 
 
int find_action(int cmd) 
{ 
   int bot, top, mid; 
 
   bot = 0; 
   top = list_top; 
 
   if (top < 0) 
      return (-1); 
 
   for (;;) 
      { 
      mid = (bot + top)/2; 
 
      if (soc_mess_list[mid].act_nr == cmd) 
	 return (mid); 
      if (bot >= top) 
	 return (-1); 
 
      if (soc_mess_list[mid].act_nr > cmd) 
	 top = --mid; 
      else 
	 bot = ++mid; 
      } 
} 
 
 
 
ACMD(do_action) 
{ 
   int act_nr, act_mood, old_mood; 
   struct social_messg *action; 
   struct char_data *vict; 
   char *buf;

   if ((act_nr = find_action(cmd)) < 0) 
      { 
      send_to_char(ch,"That action is not supported.\r\n"); 
      return; 
      } 
   action = &soc_mess_list[act_nr]; 

   buf=get_buffer(MAX_INPUT_LENGTH);
   if (action->char_found && argument) 
      phrase_argument(argument, buf); 
   else 
      *buf = '\0'; 
 
   if (!*buf) 
      { 
      send_to_char(ch,"%s\r\n",action->char_no_arg); 
      act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM); 
      release_buffer(buf);
      return; 
      } 
   if (!(vict = get_char_vis(ch, buf,FIND_CHAR_ROOM))) 
      { 
      send_to_char(ch,"%s\r\n",action->not_found);
      } 
   else if (vict == ch) 
      { 
      send_to_char(ch,"%s\r\n",action->char_auto); 
      act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM); 
      } 
   else 
      { 
      if (GET_POS(vict) < action->min_victim_position) 
	 act("$N is not in a proper position for that.", 
	     FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP); 
      else 
	 { 
	 MOBTrigger=TRUE;
	 act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP); 
	 MOBTrigger=TRUE;
	 act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT); 
	 MOBTrigger=TRUE;
	 act(action->vict_found, action->hide, ch, 0, vict, TO_VICT); 
	 } 
      /* change mood a bit */
      if (IS_NPC(vict)&&!AFF_FLAGGED(vict,AFF_CHARM)&&
	  !AFF_FLAGGED(ch,AFF_CHARM)&&!MOB_FLAGGED(vict,MOB_NOMOOD))
	 {
	 act_mood = get_mood_type(cmd, GET_MOOD(vict));
	 old_mood = GET_MOOD(vict);
	 if (IS_MOB(ch))
	    GET_MOOD(vict) += GET_MOOD(ch)/25;
	 else
	    GET_MOOD(vict) += act_mood;
	 GET_MOOD(vict) = MIN(1000, GET_MOOD(vict));
	 GET_MOOD(vict) = MAX(-1000, GET_MOOD(vict)); 
	 
	 if (GET_MOOD(vict) <= old_mood)
	    {
	    if (old_mood <= -850)
	       {
	       if(!FIGHTING(vict))
		  {
		  act("$n seems really upset.",TRUE,vict,0,0,TO_ROOM);
		  if (GET_MOOD(vict) < -950 && !number(0,1))
		     {
		     act("$n gets really enraged at $N.",TRUE,vict,0,ch,
			 TO_NOTVICT);
		     if ((!MOB_FLAGGED(ch, MOB_SENTINEL) ||
			 !MOB_FLAGGED(vict, MOB_SENTINEL))&&
			 !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)&&
                         CAN_SEE(vict,ch))

			{
			hit(vict,ch,TYPE_UNDEFINED);
			if (MOB_FLAGGED(vict, MOB_MEMORY) && !IS_NPC(ch))
			   remember(vict, ch); 

/*	      if(ch&&IS_NPC(ch)&&vict&&IS_NPC(vict)&&FIGHTING(vict))
	      mudlogf(CMP,LVL_SERP, TRUE,
	      "MOOD: %s starts beating on %s at [%d]",
	      GET_NAME(vict), GET_NAME(ch),
	      GET_ROOM_VNUM(IN_ROOM(vict)));*/
			}
		     release_buffer(buf);
		     return;
		     }
		  }
	       }
	    }
	 if (ch&&vict&&(!IS_NPC(ch) || !number(0,5)) && !FIGHTING(ch))
            moods(vict,ch,SPEC_MOBACT, "");
	 } 
      } 
   release_buffer(buf);
}
 
int get_mood_type(int cmd, int mood)
{
   int i;

   i = 0;
   while(hyper_soc[i][0]!='\n')
      {
      if (CMD_IS(hyper_soc[i]))
	 return 40 + mood/100;
      i++;
      }
   i = 0;
   while(happy_soc[i][0]!='\n')
      {
      if (CMD_IS(happy_soc[i]))
	 return 30 + mood/100;
      i++;
      }
   i = 0;
   while(cheery_soc[i][0]!='\n')
      {
      if (CMD_IS(cheery_soc[i]))
	 return 20 + mood/100;
      i++;
      }
   i = 0;
   while(neutral_soc[i][0]!='\n')
      {
      if (CMD_IS(neutral_soc[i]))
	 return number(-10,10) + mood/500;
      i++;
      }
   i = 0;
   while(sad_soc[i][0]!='\n')
      {
      if (CMD_IS(sad_soc[i]))
	 return -20 + mood/100;
      i++;
      }
   i = 0;
   while(grouchy_soc[i][0]!='\n')
      {
      if (CMD_IS(grouchy_soc[i]))
	 return -28 + mood/100;
      i++;
      }               
   i = 0;
   while(homicidal_soc[i][0]!='\n')
      {
      if (CMD_IS(homicidal_soc[i]))
	 return -33 + mood/100;
      i++;
      }               
   return(0);
}
 
ACMD(do_insult) 
{ 
   struct char_data *victim; 
 
   char *arg = get_buffer(MAX_INPUT_LENGTH);
   phrase_argument(argument, arg); 
 
   if (*arg) 
      { 
      if (!(victim = get_char_vis(ch, arg,FIND_CHAR_ROOM))) 
	 send_to_char(ch,"Can't hear you!\r\n"); 
      else 
	 { 
	 if (victim != ch) 
	    { 
	    send_to_char(ch, "You insult %s.\r\n", GET_NAME(victim)); 
	    if (IS_MOB(victim))
	       {
	       if (IS_NPC(ch))
		  GET_MOOD(victim) -= number(1,20);
	       else
		  GET_MOOD(victim) -= number(1,4);
	       }
	    switch (number(0, 2)) 
	       { 
		case 0: 
		   if (GET_SEX(ch) == SEX_MALE) 
		      { 
		      if (GET_SEX(victim) == SEX_MALE) 
			 act("$n accuses you of fighting like a woman!", FALSE,
			     ch, 0, victim, TO_VICT); 
		      else 
			 act("$n says that women can't fight.", FALSE, ch, 0, 
			     victim, TO_VICT); 
		      } 
		   else 
		      {  
/* Ch == Woman */ 
		      if (GET_SEX(victim) == SEX_MALE) 
			 act("$n accuses you of having the smallest... (brain?)", 
			     FALSE, ch, 0, victim, TO_VICT); 
		      else 
			 act("$n tells you that you'd lose a beauty contest against a troll.", 
			     FALSE, ch, 0, victim, TO_VICT); 
		      } 
		   break; 
		case 1: 
		   act("$n calls your mother a bitch!", FALSE, ch, 0, victim, 
		       TO_VICT); 
		   break; 
		default: 
		   act("$n tells you to get lost!",FALSE,ch,0,victim, TO_VICT);
		   break; 
	       }   
/* end switch */ 
 
	    act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT); 
	    } 
	 else 
	    {   
/* ch == victim */ 
	    send_to_char(ch,"You feel insulted.\r\n"); 
	    } 
	 } 
      } 
   else 
      send_to_char(ch,"I'm sure you don't want to insult *everybody*...\r\n");
   release_buffer(arg);
} 
 
 
char *fread_action(FILE * fl, int nr) 
{ 
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *rslt;

   fgets(buf, MAX_STRING_LENGTH, fl); 
   if (feof(fl)) 
      { 
      log("SYSERR: fread_action: unexpected EOF near action #%d", nr); 
      release_buffer(buf);
      exit(1); 
      } 
   if (*buf == '#') 
      {
      release_buffer(buf);
      return (NULL);
      } 
   buf[strlen(buf)-1] ='\0';
   rslt=str_dup(buf);
   release_buffer(buf);
   return (rslt); 
} 
 
 
void boot_social_messages(void) 
{ 
   FILE *fl; 
   int nr, i, hide, min_pos, curr_soc = -1; 
   char *next_soc=get_buffer(100); 
   struct social_messg temp; 
 
  /* open social file */ 
   if (!(fl = fopen(SOCMESS_FILE, "r"))) 
      { 
      log("SYSERR: Can't open socials file '%s'", SOCMESS_FILE); 
      release_buffer(next_soc);
      exit(1); 
      } 
  /* count socials & allocate space */ 
   for (nr = 0; *cmd_info[nr].command != '\n'; nr++) 
      if (cmd_info[nr].command_pointer == do_action) 
	 list_top++; 
 
   CREATE(soc_mess_list, struct social_messg, list_top + 1); 
 
  /* now read 'em */ 
   for (;;) 
      { 
      fscanf(fl, " %s ", next_soc); 
      if (*next_soc == '$') 
	 break; 
      if ((nr = find_command(next_soc)) < 0) 
	 { 
	 log("SYSERR: Unknown social '%s' in social file", next_soc); 
	 exit(1);
	 } 
      if (fscanf(fl, " %d %d \n", &hide, &min_pos) != 2) 
	 { 
	 log("SYSERR: Format error in social file near social '%s'\n", 
		 next_soc); 
	 release_buffer(next_soc);
	 exit(1); 
	 } 
     /* read the stuff */ 
      curr_soc++; 
      soc_mess_list[curr_soc].act_nr = nr; 
      soc_mess_list[curr_soc].hide = hide; 
      soc_mess_list[curr_soc].min_victim_position = min_pos; 
 
      soc_mess_list[curr_soc].char_no_arg = fread_action(fl, nr); 
      soc_mess_list[curr_soc].others_no_arg = fread_action(fl, nr); 
      soc_mess_list[curr_soc].char_found = fread_action(fl, nr); 
 
     /* if no char_found, the rest is to be ignored */ 
      if (!soc_mess_list[curr_soc].char_found) 
	 continue; 
 
      soc_mess_list[curr_soc].others_found = fread_action(fl, nr); 
      soc_mess_list[curr_soc].vict_found = fread_action(fl, nr); 
      soc_mess_list[curr_soc].not_found = fread_action(fl, nr); 
      soc_mess_list[curr_soc].char_auto = fread_action(fl, nr); 
      soc_mess_list[curr_soc].others_auto = fread_action(fl, nr); 
      } 
 
  /* close file & set top */ 
   fclose(fl); 
   if(curr_soc > list_top)
      {
      log("SYSERR: There are more socials than space to hold them. %d %d\n",
	  curr_soc, list_top);
      exit(1);
      }
   list_top = curr_soc; 
 
  /* now, sort 'em */ 
   for (curr_soc = 0; curr_soc < list_top; curr_soc++) 
      { 
      min_pos = curr_soc; 
      for (i = curr_soc + 1; i <= list_top; i++) 
	 if (soc_mess_list[i].act_nr < soc_mess_list[min_pos].act_nr) 
	    min_pos = i; 
      if (curr_soc != min_pos) 
	 { 
	 temp = soc_mess_list[curr_soc]; 
	 soc_mess_list[curr_soc] = soc_mess_list[min_pos]; 
	 soc_mess_list[min_pos] = temp; 
	 } 
      } 
   release_buffer(next_soc);
} 
