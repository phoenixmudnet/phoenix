/* ************************************************************************ 
*   File: utils.c                                       Part of CircleMUD * 
*  Usage: various internal functions of a utility nature                  * 
*                                                                         * 
*  All rights reserved.  See license.doc for complete information.        * 
*                                                                         * 
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University * 
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
************************************************************************ */ 
 
#include <stdarg.h>
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
#include "olc.h"
 
extern struct time_data time_info; 
extern struct room_data *world; 
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
int perform_remove(struct char_data * ch, int pos);
void parse_log_to_file(char *logbuf);
void stop_fighting(struct char_data * ch);

ACMD(do_assist); 
/* 
   adds spaces to the end of strings so that you can be sure what length they 
   will be.  First used to make the score command come out correctly 
   takes in the string to alter and the total number of spaces the string 
   should take up, puts out (in source) a string of length total_length 
   ending with a \0 
   masque 3/11/95 
*/ 
void str_add_spaces(char *source,int total_length) 
{ 
   char *string = source; 
   int i=0; 
  
   while((total_length>=i)&&(*string!='\0')) 
      { 
      string++;
      i++; 
      } 
  
   for(;i<=total_length;i++) 
      { 
      *string=' '; 
      string++; 
      } 
   *string='\0'; 
  
} 

int stat_index(int stat_ch)
{ 
   if(stat_ch<=25)
      return stat_ch;
   else 
      return ((stat_ch-25)/5)+25;
}

/* creates a random number in interval [from;to] */ 
int number(int from, int to) 
{ 
  /* error checking in case people call number() incorrectly */ 
   if (from > to) 
      { 
      int tmp = from; 
      from = to; 
      to = tmp; 
      log("SYSERR: number() should be called with lowest, then highest."
	  " number(%d, %d), not number(%d, %d).", from, to, to, from);
      } 
 
   return ((circle_random() % (to - from + 1)) + from); 
} 
 
 
/* simulates dice roll */ 
int dice(int die_number, int size) 
{ 
   int sum = 0; 
 
   if (size <= 0 || die_number <= 0) 
      return 0; 
 
   while (die_number-- > 0) 
      sum += ((circle_random() % size) + 1); 
 
   return sum; 
} 
 
 
int MIN(int a, int b) 
{ 
   return (a < b ? a : b); 
} 
 
 
int MAX(int a, int b) 
{ 
   return (a > b ? a : b); 
} 
 
char *CAP(char *txt)
{
  *txt = UPPER(*txt);
  return (txt);
}

/* Capitalize the sentence, account for color escape codes - Nomi 9/29/25 */
char *CAP_LINE(char *txt)
{
   int ii;
   if (!txt || !*txt)
	  return(txt);

   /* Assumption:
   ** 1. Color codes are either 4 or 8 characters long (not always true)
   */
   if (*txt == '\x1B')
   {
      if ((strlen(txt) > 4) && (*(txt + 4) == '\x1B'))
         *(txt + 9) = UPPER(*(txt + 9));
      else
	     *(txt + 5) = UPPER(*(txt + 5));
   }
   else if (*txt == '&')
   {
	  /* cycle until there are no more '&' codes */
	  for (ii = 2; (strlen(txt) <= ii) && (*(txt + ii) == '&'); ii += 2) 
		 ;
 	  *(txt + ii) = UPPER(*(txt + ii));
   }
   else
 	  *txt = UPPER(*txt);
  
   return(txt);
}


 
#if BUFFER_MEMORY == FALSE
/* Create a duplicate of a string */ 
char *str_dup(const char *source) 
{ 
   char *new_z; 
 
   CREATE(new_z, char, strlen(source) + 1); 
   return (strcpy(new_z, source)); 
} 
#endif
 
/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt)
{
  for (ssize_t ii = strlen(txt) - 1; ii > 0; ii--) {
      if (txt[ii] == '\n' || txt[ii] == '\r') {
         txt[ii] = '\0';
      } else {
         break;
      }
  }
}
 
/* str_cmp: a case-insensitive version of strcmp */ 
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */ 
/* scan 'till found different or end of both                 */ 
int str_cmp(char *arg1, char *arg2) 
{ 
   int chk, i; 
   if(!arg1||!arg2||!*arg1||!*arg2)
      return 1;
   for (i = 0; *(arg1 + i) || *(arg2 + i); i++) 
      if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))) 
	 {
	 if (chk < 0) 
	    return (-1); 
	 else 
	    return (1); 
	 }
   return (0); 
} 
 
 
/* strn_cmp: a case-insensitive version of strncmp */ 
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */ 
/* scan 'till found different, end of both, or n reached     */ 
int strn_cmp(char *arg1, char *arg2, int n) 
{ 
   int chk, i; 
 
   for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--) 
      if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))) 
	 {
	 if (chk < 0) 
	    return (-1); 
	 else 
	    return (1); 
	 }
   return (0); 
} 
 
 
/* log a death trap hit */ 
void log_death_trap(struct char_data * ch) 
{ 
   mudlogf(BRF, LVL_IMMORT, TRUE, "DeathTrap: %s has hit a death trap #%ld (%s)", 
	  GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), world[IN_ROOM(ch)].name); 
   send_info("[ INFO ] %s hit a death trap.\n\r", GET_NAME(ch));
} 


/* writes a string to the log */ 
void log(const char *format, ...)
{
   va_list args;
   time_t ct = time(0);
   char *time_s = asctime(localtime(&ct));
   char *log_buf = get_buffer(MAX_STRING_LENGTH);
   
   *(time_s + strlen(time_s) - 1) = '\0';
   
   fprintf(stderr, "%-20.20s :: ", time_s+4);
   
   va_start(args, format);
   vsprintf(log_buf, format, args);
   va_end(args);
   
   fprintf(stderr, "%s\r\n", log_buf);
   parse_log_to_file(log_buf);
   release_buffer(log_buf);
}
 
/* the "touch" command, essentially. */ 
int touch(char *path) 
{ 
   FILE *fl; 
 
   if (!(fl = fopen(path, "a"))) 
      { 
      log("SYSERR: touch %s: %s", path, strerror(errno));
      return -1; 
      } 
   else 
      { 
      fclose(fl); 
      return 0; 
      } 
} 
 
 
void mudlogf(char type, int level, byte file, const char *format, ...)
{
   va_list args;
   time_t ct = time(0);
   char *time_s = asctime(localtime(&ct));
   struct descriptor_data *i;
   char *mudlog_buf = get_buffer(MAX_STRING_LENGTH);
   char tp;
   struct char_data *wch;

   *(time_s + strlen(time_s) - 1) = '\0';

   if (file)
      fprintf(stderr, "%-20.20s :: ", time_s+4);

   va_start(args, format);
   if (file)
      vfprintf(stderr, format, args);
   if (level >= 0)
      vsprintf(mudlog_buf, format, args);
   va_end(args);

   if (file)
      fprintf(stderr, "\r\n");

   if (level < 0)
      {
      release_buffer(mudlog_buf);
      return;
      }

   for (i = descriptor_list; i; i = i->next)
      {
      if(i->original)
	 wch=i->original;
      else
	 wch=i->character;
	     
      if (STATE(i)==CON_PLAYING && !PLR_FLAGGED(wch, PLR_WRITING)) 
	 {
	 tp = ((PRF_FLAGGED(wch, PRF_LOG1) ? 1 : 0) +
	       (PRF_FLAGGED(wch, PRF_LOG2) ? 2 : 0));

	 if ((GET_LEVEL(wch) >= level) && (tp >= type)) 
	    {
	    send_to_char(wch,"%s[%s]%s\r\n",CCGRN(wch, C_NRM), mudlog_buf,
			 (CCNRM(wch, C_NRM)));
	    }
	 }
      }

   /* this will update logs instantaneously,  */
   /*   instead of at reboot - nomikos 1-6-03 */
   parse_log_to_file(mudlog_buf);

   release_buffer(mudlog_buf);
}


void sprintbit(bitvector_t bitvector, char *names[], char *result) 
{ 
  long nr; 
 
  *result = '\0'; 
 
/* 
 * begin add - Bon 07/18/97 
 * prevent negative number from creating an infinite loop 
 */ 
  for (nr = 0; bitvector; bitvector = ((bitvector >> 1) & 0x7FFFFFFF))
     { 
    /* 
     * end   add - Bon 07/18/97 
     */ 
     if (IS_SET(bitvector, 1)) 
	{ 
	if (*names[nr] != '\n') 
	   { 
	   strcat(result, names[nr]); 
	   strcat(result, " "); 
	   } 
	else 
	   strcat(result, "UNDEFINED "); 
	} 
     if (*names[nr] != '\n') 
	nr++; 
     } 
  
  if (!*result) 
     strcpy(result, "NOBITS "); 
} 

void sprintbit2(bitvector_t bitvector, char *names[], char *result,
		int start_pos) 
{ 
  long nr; 
  int found=0;
  int i,count=0;
  *result = '\0';

/* 
 * begin add - Bon 07/18/97 
 * prevent negative number from creating an infinite loop 
 */ 
  for(i=start_pos;i<32;i++)
     if(((1<<i)&bitvector)==(1<<i))
	count++;
	
  for (nr = start_pos,i=start_pos; i<32;i++)
     { 
    /* 
     * end   add - Bon 07/18/97 
     */ 
     if (IS_SET(bitvector, (1<<i))) 
	{ 
	if(found)
	   strcat(result," ");
	if (*names[nr] != '\n') 
	   { 
	   strcat(result, names[nr]); 
	   } 
	else 
	   {
	   strcat(result, "unknown"); 
	   }
	found++;
	if((count>2) && (found<=(count-1)))
	   strcat(result,",");
	if((count>1)&&(found==(count-1)))
	   strcat(result," and");
	} 
     
     
     if (*names[nr] != '\n') 
	nr++; 
     else
	i=32;
     } 
  
  if (!*result) 
     strcpy(result, "none."); 
} 
 
 
 
void sprinttype(int type, char *names[], char *result) 
{ 
   int nr = 0; 
 
   while (type && *names[nr] != '\n') 
      { 
      type--; 
      nr++; 
      } 
 
   if (*names[nr] != '\n') 
      strcpy(result, names[nr]); 
   else 
      strcpy(result, "UNDEFINED"); 
} 
 
 
/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */ 
struct time_info_data *real_time_passed(time_t t2, time_t t1) 
{ 
   long secs; 
   static struct time_info_data now; 
 
   secs = (long) (t2 - t1); 
 
   now.hours = (secs / SECS_PER_REAL_HOUR) % 24; /* 0..23 hours */ 
   secs -= SECS_PER_REAL_HOUR * now.hours; 
 
   now.day = (secs / SECS_PER_REAL_DAY); /* 0..34 days  */ 
  /*secs -= SECS_PER_REAL_DAY * now.day; */
 
   now.month = -1; 
   now.year = -1; 
 
   return &now; 
} 
 
 
 
/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */ 
struct time_info_data *mud_time_passed(time_t t2, time_t t1) 
{ 
   long secs; 
   static struct time_info_data now; 
 
   secs = (long) (t2 - t1); 

  /*  log("T1: %ld  T2: %ld  Secs: %ld",(long)t1,(long)t2,secs); */

   now.minutes = secs%75;

   now.hours = (secs / SECS_PER_MUD_HOUR) % 24; /* 0..23 hours */ 
   secs -= SECS_PER_MUD_HOUR * now.hours; 
 
   now.day = (secs / SECS_PER_MUD_DAY) % 35; /* 0..34 days  */ 
   secs -= SECS_PER_MUD_DAY * now.day; 
 
   now.month = (secs / SECS_PER_MUD_MONTH) % 17; /* 0..16 months */ 
   secs -= SECS_PER_MUD_MONTH * now.month; 
 
   now.year = (secs / SECS_PER_MUD_YEAR); /* 0..XX? years */ 
 
   return &now; 
} 
 
 
 
struct time_info_data *age(struct char_data * ch) 
{ 
   static struct time_info_data player_age; 
 
   player_age = *mud_time_passed(time(0), ch->player.time.birth); 
 
   player_age.year += 17; /* All players start at 17 */ 
 
   return &player_age; 
} 
 
 
/* Check if making CH follow VICTIM will create an illegal */ 
/* Follow "Loop/circle"                                    */ 
bool circle_follow(struct char_data * ch, struct char_data * victim) 
{ 
   struct char_data *k; 
 
   for (k = victim; k; k = k->master) 
      { 
      if (k == ch) 
	 return TRUE; 
      } 
 
   return FALSE; 
} 
 
 
 
/* Called when stop following persons, or stopping charm */ 
/* This will NOT do if a character quits/dies!!          */ 
void stop_follower(struct char_data * ch) 
{ 
   struct follow_type *j, *k; 
 
   if(ch->master==NULL)
      {
      core_dump();
      return;
      }
 
   if (AFF_FLAGGED(ch, AFF_CHARM)) 
      { 
      act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR); 
      act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT); 
      act("$n hates your guts!", FALSE, ch, 0, ch->master, TO_VICT); 
      if (affected_by_spell(ch, SPELL_CHARM)) 
	 affect_from_char(ch, SPELL_CHARM); 

      /* If the charmie is fighting, and master is gone, send 
      **  it elsewhere -Nomikos 8/31/03 */
      if (FIGHTING(ch) && IN_ROOM(ch->master)!=IN_ROOM(ch))
         {
         act("$n takes one good look at $N and runs like hell!",
             FALSE, ch, 0, FIGHTING(ch), TO_ROOM);
         stop_fighting(ch);
         if (ch->orig_room == IN_ROOM(ch))
            {
            char_from_room(ch);
            char_to_room(ch, 0);
            }
         else
            {
            char_from_room(ch);
            char_to_room(ch, ch->orig_room);
            }
         }
      } 
   else 
      { 
      if (!MOB2_FLAGGED(ch, MOB2_COMPONENT))
         {
         act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR); 
         act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT); 
         if(GET_LEVEL(ch)<LVL_IMMORT||CAN_SEE(ch->master,ch))
	    {
	    act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT); 
   	    }
         }
      } 

   if (ch->master->followers->follower == ch) 
      { 
/* Head of follower-list? */ 
      k = ch->master->followers; 
      ch->master->followers = k->next; 
      k->follower=NULL;
      free(k); 
      k=NULL;
      } 
   else 
      {   
/* locate follower who is not head of list */ 
      for (k = ch->master->followers; k->next->follower != ch; k = k->next); 
 
      j = k->next; 
      k->next = j->next; 
      j->follower=NULL;
      free(j); 
      j=NULL;
      } 
 
   ch->master = NULL; 
   REMOVE_BIT(AFF_FLAGS(ch), AFF_CHARM | AFF_GROUP); 
   if(IS_NPC(ch))
      REMOVE_BIT(MOB_FLAGS(ch),MOB_GUARD);
} 
 
 
 
/* Called when a character that follows/is followed dies */ 
void die_follower(struct char_data * ch) 
{ 
   struct follow_type *j, *k; 
 
   if (ch->master) 
      stop_follower(ch); 
 
   for (k = ch->followers; k; k = j) 
      { 
      j = k->next; 
      if (MOB2_FLAGGED(k->follower, MOB2_COMPONENT))
         extract_char(k->follower);
      else
         stop_follower(k->follower); 
      } 
} 
 
 
 
/* Do NOT call this before having checked if a circle of followers */ 
/* will arise. CH will follow leader                               */ 
void add_follower(struct char_data * ch, struct char_data * leader) 
{ 
   struct follow_type *k; 
 
   if (ch->master) 
      {
      core_dump();
      return;
      }
   

   ch->master = leader; 
 
   CREATE(k, struct follow_type, 1); 
 
   k->follower = ch; 
   k->next = leader->followers; 
   leader->followers = k; 
   
   if (!MOB2_FLAGGED(ch, MOB2_COMPONENT))
      {
      act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR); 
      if (CAN_SEE(leader, ch)) 
         act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT); 
      act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT); 
      }

   if(IS_NPC(ch))
      {
      SET_BIT(MOB_FLAGS(ch),MOB_GUARD);
      SET_BIT(MOB_FLAGS(ch),MOB_SENTINEL);
      if(IS_NPC(leader))
	 {	
	 SET_BIT(MOB_FLAGS(leader),MOB_GUARD);
	 SET_BIT(AFF_FLAGS(leader),AFF_GROUP);
	 SET_BIT(AFF_FLAGS(ch),AFF_GROUP);
         if (!MOB2_FLAGGED(ch, MOB2_COMPONENT))
            act("$N is now a member of $n's group.",FALSE,leader,0,ch,TO_NOTVICT);
	 }
      }
      
} 

void check_follower(struct char_data *leader, struct char_data *ch)
{
   struct follow_type *f;
   for (f=leader->followers; f; f=f->next) 
      {
      if (!f->follower)
         mudlogf(BRF, LVL_IMMORT, TRUE,
                 "SYSERR: No follower in check_follower()!");
      else if ((f->follower!=ch) &&
	  AFF_FLAGGED(f->follower, AFF_GROUP) &&
          (IN_ROOM(ch) == IN_ROOM(f->follower)) &&
          !FIGHTING(f->follower) && 
          ((!IS_NPC(f->follower)&&
            PRF_FLAGGED(f->follower, PRF_AUTOASSIST)) ||
           (IS_NPC(f->follower)&&
            MOB_FLAGGED(f->follower, MOB_GUARD))) && 
          FIGHTING(ch))
         {
         do_assist(f->follower, ch->player.name, 0, 0);
         }
      if(f->follower&&f->follower->followers)
         check_follower(f->follower,ch);
      } 
}
 

/* Returns number of charmies among followers */
int num_charmies(struct char_data * ch)
{
   struct follow_type *tmp;
   int num = 0;
   
   for (tmp = ch->followers; tmp; tmp = tmp->next)
      {
      if (AFF_FLAGGED(tmp->follower, AFF_CHARM)) 
	 {
	 num++;
	 }
      }
   return num;
}

/* How many mobs/players are fighting you? */
int num_fighting(struct char_data *ch)
{
   struct char_data *tmp;
   int num = 0;
   
   for (tmp = world[IN_ROOM(ch)].people; tmp; tmp = tmp->next_in_room)
      {
      if (FIGHTING(tmp) == ch)
	 {
	 num++;
	 }
      }
   return num;
}

/* How many mobs/players are fighting you? */
/* TODO: this should ensure the PCs are GROUPED. */
int num_pcs_fighting(struct char_data *ch)
{
   struct char_data *tmp;
   int num = 0;
   
   for (tmp = world[IN_ROOM(ch)].people; tmp; tmp = tmp->next_in_room)
      {
      if (FIGHTING(tmp) == ch && !IS_NPC(tmp))
	 {
	 num++;
	 }
      }
   return num;
}

/* How many mobs/players are fighting you? */
/* TODO: this should ensure the PCs are GROUPED. */
int num_npcs_in_room(struct char_data *ch)
{
   struct char_data *tmp;
   int num = 0;
   
   for (tmp = world[IN_ROOM(ch)].people; tmp; tmp = tmp->next_in_room)
      {
      if (IS_NPC(tmp))
	 {
	 num++;
	 }
      }
   return num;
}
                

/* 
 * get_line reads the next non-blank line off of the input stream. 
 * The newline character is removed from the input.  Lines which begin 
 * with '*' are considered to be comments. 
 * 
 * Returns the number of lines advanced in the file. 
 */ 
int get_line(FILE * fl, char *buf) 
{ 
   char *temp=get_buffer(1024); 
   int lines = 0; 
 
   do 
      { 
      lines++; 
      fgets(temp, 1020, fl);
      if (ferror(fl)) {
	break;
      }
      if (*temp) 
	 temp[strlen(temp) - 1] = '\0'; 
      } 
   while (!feof(fl) && (*temp == '*' || !*temp)); 
 
   if (feof(fl)) 
      {
      *buf='\0';
      lines= 0; 
      }
   else 
      { 
      strcpy(buf, temp); 
      } 
   release_buffer(temp);
   return lines; 
} 
 
 
int get_filename(char *orig_name, char *filename, int mode) 
{ 
   char *prefix, *middle, *suffix, *ptr, *name=get_buffer(512); 
 
   switch (mode) 
      { 
       case CRASH_FILE: 
	  prefix = "plrobjs"; 
	  suffix = "objs"; 
	  break; 
       case ETEXT_FILE: 
	  prefix = "plrtext"; 
	  suffix = "text"; 
	  break; 
       case ALIAS_FILE:      /* Alias mod */ 
	  prefix = "plralias"; 
	  suffix = "alias"; 
	  break; 
       case SCRIPT_VARS_FILE:
	  prefix = "plrvar";
	  suffix  = "mem";
	  break;
       case NEW_OBJ_FILES:
	  prefix = "plrobjs";
	  suffix = "new";
	  break;
       case REIMB_FILE:
	  prefix = "plrobjs";
	  suffix = "reimb";
	  break;
       case COMMS_FILE:
	  prefix = "plrcomms";
	  suffix = "comms";
	  break;
       default: 
	  release_buffer(name);
	  return 0; 
	  break; 
      } 
 
   if (!*orig_name) 
      {
      release_buffer(name);
      return 0; 
      }
   strcpy(name, orig_name); 
   for (ptr = name; *ptr; ptr++) 
      *ptr = LOWER(*ptr); 
 
   switch (LOWER(*name)) 
      { 
       case 'a':  case 'b':  case 'c':  case 'd':  case 'e': 
	  middle = "A-E"; 
	  break; 
       case 'f':  case 'g':  case 'h':  case 'i':  case 'j': 
	  middle = "F-J"; 
	  break; 
       case 'k':  case 'l':  case 'm':  case 'n':  case 'o': 
	  middle = "K-O"; 
	  break; 
       case 'p':  case 'q':  case 'r':  case 's':  case 't': 
	  middle = "P-T"; 
	  break; 
       case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z': 
	  middle = "U-Z"; 
	  break; 
       default: 
	  middle = "ZZZ"; 
	  break; 
      } 
 
   sprintf(filename, "%s/%s/%s.%s", prefix, middle, name, suffix); 
   release_buffer(name);
   return 1; 
} 
 
 
int num_pc_in_room(struct room_data *room) 
{ 
   int i = 0; 
   struct char_data *ch; 
 
   for (ch = room->people; ch != NULL; ch = ch->next_in_room) 
      if (!IS_NPC(ch) && (GET_LEVEL(ch)<LVL_IMMORT)) 
	 i++; 
 
   return i; 
} 
 
/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
void core_dump_unix(const char *who, ush_int line)
{
   log("SYSERR: *** Dumping core from %s:%d. ***", who, line);
   
  /* These would be duplicated otherwise... */
   fflush(stdout);
   fflush(stderr);
   
  /*
   * Kill the child so the debugger or script doesn't think the MUD
   * crashed.  The 'autorun' script would otherwise run it again.
   */
   if (fork() == 0)
      abort();
}

/* string manipulation fucntion originally by Darren Wilson */ 
/* (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) */ 
/* completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), */ 
/* substitute appearances of 'pattern' with 'replacement' in string */ 
/* and return the # of replacements */ 
int replace_str(char **string, char *pattern, char *replacement, int rep_all, 
		int max_size) 
{ 
   char *replace_buffer = NULL; 
   char *flow, *jetsam, temp; 
   int len, i; 
    
   if ((strlen(*string) - strlen(pattern)) + strlen(replacement) > max_size) 
      return -1; 
    
   CREATE(replace_buffer, char, max_size); 
   i = 0; 
   jetsam = *string; 
   flow = *string; 
   *replace_buffer = '\0'; 
   if (rep_all) 
      { 
      while ((flow = (char *)strstr(flow, pattern)) != NULL) 
	 { 
	 i++; 
	 temp = *flow; 
	 *flow = '\0'; 
	 if ((strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) > max_size) 
	    { 
	    i = -1; 
	    break; 
	    } 
	 strcat(replace_buffer, jetsam); 
	 strcat(replace_buffer, replacement); 
	 *flow = temp; 
	 flow += strlen(pattern); 
	 jetsam = flow; 
	 } 
      strcat(replace_buffer, jetsam); 
      } 
   else 
      { 
      if ((flow = (char *)strstr(*string, pattern)) != NULL) 
	 { 
	 i++; 
	 flow += strlen(pattern);   
	 len = ((char *)flow - (char *)*string) - strlen(pattern); 
    
	 strncpy(replace_buffer, *string, len); 
	 strcat(replace_buffer, replacement); 
	 strcat(replace_buffer, flow); 
	 } 
      } 
   if (i == 0) return 0; 
   if (i > 0) 
      { 
      RECREATE(*string, char, strlen(replace_buffer) + 3); 
      strcpy(*string, replace_buffer); 
      } 
   free(replace_buffer); 
   return i; 
} 
 
 
/* re-formats message type formatted char * */ 
/* (for strings edited with d->str) (mostly olc and mail)     */ 
void format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen) 
{ 
   int total_chars, cap_next = TRUE, cap_next_next = FALSE; 
   char *flow, *start = NULL, temp; 
  /* warning: do not edit messages with max_str's of over this value */ 
   char *formated=get_buffer(MAX_STRING_LENGTH); 
    
   flow   = *ptr_string; 
   if (!flow)
      {
      release_buffer(formated);
      return; 
      }
 
   if (IS_SET(mode, FORMAT_INDENT)) 
      { 
      strcpy(formated, "   "); 
      total_chars = 3; 
      } 
   else 
      { 
      *formated = '\0'; 
      total_chars = 0; 
      }  
 
   while (*flow != '\0') 
      { 
      while ((*flow == '\n') || 
	     (*flow == '\r') || 
	     (*flow == '\f') || 
	     (*flow == '\t') || 
	     (*flow == '\v') || 
	     (*flow == ' ')) flow++; 
 
      if (*flow != '\0') 
	 { 
 
	 start = flow++; 
	 while ((*flow != '\0') && 
		(*flow != '\n') && 
		(*flow != '\r') && 
		(*flow != '\f') && 
		(*flow != '\t') && 
		(*flow != '\v') && 
		(*flow != ' ') && 
		(*flow != '.') && 
		(*flow != '?') && 
		(*flow != '!')) flow++; 
 
	 if (cap_next_next) 
	    { 
	    cap_next_next = FALSE; 
	    cap_next = TRUE; 
	    } 
 
	/* this is so that if we stopped on a sentance .. we move off the
	 * sentance delim. */ 
	 while ((*flow == '.') || (*flow == '!') || (*flow == '?')) 
	    { 
	    cap_next_next = TRUE; 
	    flow++; 
	    } 
   
	 temp = *flow; 
	 *flow = '\0'; 
 
	 if ((total_chars + strlen(start) + 1) > 78) 
	    { 
	    strcat(formated, "\r\n"); 
	    total_chars = 0; 
	    } 
 
	 if (!cap_next) 
	    { 
	    if (total_chars > 0) 
	       { 
	       strcat(formated, " "); 
	       total_chars++; 
	       } 
	    } 
	 else 
	    { 
	    cap_next = FALSE; 
	    *start = UPPER(*start); 
	    } 
 
	 total_chars += strlen(start); 
	 strcat(formated, start); 
 
	 *flow = temp; 
	 } 
 
      if (cap_next_next) 
	 { 
	 if ((total_chars + 3) > 78)
	    { 
	    strcat(formated, "\r\n"); 
	    total_chars = 0; 
	    } 
	 else 
	    { 
	    strcat(formated, "  "); 
	    total_chars += 2; 
	    } 
	 } 
      } 
   while (isspace(*(formated+strlen(formated)-1)))
      *(formated+strlen(formated)-1) = '\0';
   if (strcmp(formated+strlen(formated)-4, "\r\n"))
      strcat(formated, "\r\n"); 
 
   if (strlen(formated) > maxlen) formated[maxlen] = '\0'; 
   RECREATE(*ptr_string, char, MIN(maxlen, strlen(formated)+3)); 
   strcpy(*ptr_string, formated); 
   release_buffer(formated);
} 
   
/* strips \r's from line */
char *stripcr(char *dest, const char *src) 
{
   int i, length;
   char *temp;
   
   if (!dest || !src)
      return NULL;
   temp = &dest[0];
   length = strlen(src);
   for (i = 0; *src && (i < length); i++, src++)
      if (*src != '\r')
	 *(temp++) = *src;
   *temp = '\0';
   return dest;
}

void count_items(struct char_data * ch, struct obj_data * obj, long *nitems)
{ 
   if (obj) 
      { 
      (*nitems)++; 
      count_items(ch, obj->contains, nitems); 
      count_items(ch, obj->next_content, nitems); 
      } 
} 
int find_obj(struct char_data * ch, struct obj_data *obj,obj_rnum obj_real_num)
{
   if(obj)
      {
      if(GET_OBJ_RNUM(obj)==obj_real_num)
	 return TRUE;
      if(find_obj(ch,obj->contains,obj_real_num))
	 return TRUE;
      if(find_obj(ch,obj->next_content,obj_real_num))
	 return TRUE;
      }
   return FALSE;
}


int has_object(struct char_data *ch, obj_rnum obj_real_num)
{
   int i;
   if(find_obj(ch,ch->carrying,obj_real_num))
      return TRUE;
   for(i=0;i<NUM_WEARS;i++)
      if(find_obj(ch,GET_EQ(ch,i),obj_real_num))
	 return TRUE;
   return FALSE;

}

extern struct index_data *obj_index;

struct obj_data *find_obj_ref(struct char_data * ch, struct obj_data *obj,int iobj_vnum)
{
  struct obj_data *tmp = NULL;
   if(obj)
      {
	if(GET_OBJ_VNUM(obj)==iobj_vnum)
	  return obj;
	if((tmp = find_obj_ref(ch,obj->contains,iobj_vnum)))
	  return tmp;
	if((tmp = find_obj_ref(ch,obj->next_content,iobj_vnum)))
	  return tmp;
      }
   return NULL;
}


struct obj_data *has_object_ref(struct char_data *ch, int iobj_vnum)
{
   int i;
   struct obj_data *obj;
   if((obj = find_obj_ref(ch,ch->carrying,iobj_vnum)))
      return obj;
   for(i=0;i<NUM_WEARS;i++)
     if((obj = find_obj_ref(ch,GET_EQ(ch,i),iobj_vnum)))
       return obj;
   return NULL;

}

int handleGetOutOfDeathFree(struct char_data* ch)
{
  if (IS_NPC(ch)) {
    return 0;
  }
  struct obj_data *getOutOfDeathFreeToken = has_object_ref(ch, 1294);
  if (getOutOfDeathFreeToken) {
    send_info("[ INFO ] %s hit a death trap, but was saved by a get-out-of-death-free token!\r\n", GET_NAME(ch));
    mudlogf(BRF, LVL_IMMORT, TRUE, "RIP: %s used a get-out-of-death-free token.", GET_NAME(ch));
    extract_obj(getOutOfDeathFreeToken);
    char_from_room(ch);
    GET_HIT(ch) = 10;
    GET_MANA(ch) = 10;
    GET_MOVE(ch) = 10;
    GET_POS(ch) = POS_STANDING;
    if (real_room(GET_HOME(ch)) >= 0) {
      char_to_room(ch, real_room(GET_HOME(ch)));
    } else {
      char_to_room(ch, real_room(3014));
    }
    command_interpreter(ch, "look");
    WAIT_STATE(ch, PULSE_VIOLENCE);

    return 1;
  }
  return 0;
}

void check_weapon_weight(struct char_data *ch)
{
   struct obj_data *obj;
   struct obj_data *obj2;
   int iTotWt=0;
   if(!IS_NPC(ch))
      {
      if((obj=GET_EQ(ch,WEAR_WIELD_2)))
	 {
	 iTotWt=GET_OBJ_WEIGHT(obj);
	 if((obj2=GET_EQ(ch,WEAR_WIELD_1)))
	    iTotWt+=GET_OBJ_WEIGHT(obj2);
	 if(iTotWt > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
	    {
	    act("$p is too heavy and you lose your grip on it.",FALSE,ch,obj,0,
		TO_CHAR);
	    if(perform_remove(ch,WEAR_WIELD_2)==0)
	       {
	       act("You fumble $p in your hands and drop it to the floor.",
		   FALSE,ch,obj,0,TO_CHAR);
	       act("$N's weak fingers drop $p to the ground.",FALSE,ch,obj,0,
		   TO_ROOM);
	       obj_to_room(unequip_char(ch,WEAR_WIELD_2),IN_ROOM(ch));
	       }
	    }
	 }
      if((obj=GET_EQ(ch,WEAR_WIELD_1)))
	 {
	 if(GET_OBJ_WEIGHT(obj)>
	    str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
	    {
	    act("$p is too heavy and you lose your grip on it.",FALSE,ch,obj,0,
		TO_CHAR);
	    if(perform_remove(ch,WEAR_WIELD_1)==0)
	       {
	       act("You fumble $p in your hands and drop it to the floor.",
		   FALSE,ch,obj,0,TO_CHAR);
	       act("$N's weak fingers drop $p to the ground.",FALSE,ch,obj,0,
		   TO_ROOM);
	       obj_to_room(unequip_char(ch,WEAR_WIELD_1),IN_ROOM(ch));
	       }
	    }
	 }
      }
   
}

/* Adjusts the cost of an item, based on charisma and vendor alignment, from between
   a plus 20% for those really fugly ones, to minus 46% for our rare beauties     */
int price_adjust (struct char_data * ch, struct char_data * vendor, int price)
{
         int iPrice2;
	 int iChCha;
	 int iAlignMod;

	 iChCha = stat_index(GET_CHA(ch));
	 /* As it was it didn't make sense and the cha_align_table shouldn't be **
   	 ** using player's alignment. The vendor will adjust prices because     **
         ** they are either good or evil, as is their nature -Nomikos 5/30/2025 **
	 ** iAlignMod = cha_align_table[10+(int)((GET_ALIGNMENT(vendor))/100)]  **
  	 **			       [10+(int)((GET_ALIGNMENT(vendor))/100)]; */
	 iAlignMod = ((GET_ALIGNMENT(vendor)) / 200); /* +/- 5 */
	 iChCha = iChCha + iAlignMod;
	 iChCha = MIN(35,MAX(0,iChCha));
	 iPrice2 = price + (int)(((float)cha_app[iChCha].adj_price/100.0) * (float)price);

         if (iPrice2 <= 0) 
	    iPrice2 = 1;
         return (iPrice2);
}

/* added nomikos 10-05-02 either i'm stupid or there isn't one of these */
char *stolower (char * string) {
   char* ptr = string;

   while (*ptr) {
      *ptr++ = tolower(*ptr);
   }

   return string;
}

/* Case insensitive strstr -- search for the first occurance of "key" in "string".
 * e.g. stristr("abcdefghijklmnopqrstuvwxyz", "ghijk") will return a pointer to
 * the 'g' character in the first argument.
 */
const char *stristr(const char *string, const char *key)
{
  if (!string) {
    return NULL;
  } else if (!key) {
    return string;
  } else if (strlen(string) < strlen(key)) {
    return NULL;
  }

  int i, j;
  for (i = 0; i <= strlen(string)-strlen(key); i++) {
    int found = 1;
    for (j = 0; j < strlen(key); j++) {
      if (tolower(string[i+j]) != tolower(key[j])) {
	found = 0;
	break;
      }
    }
    if (found) {
      return &string[i];
    }
  }
  return NULL;
}


#define NUM_LOGS       27

/* parse through mudlogs and redirect to appropriate log 
   files, as well as to syslog file.  */
void parse_log_to_file(char *logbuf)
{
   FILE *fl;
   int i;
   time_t ct = time(0);
   char *time_s = asctime(localtime(&ct));
   /* organize these from most used to least used */
   struct log_data_struct
      {
      char *pattern;
      char *file;
      }
   log_data[NUM_LOGS] = 
      {
         { "(GC)"           , GODCMD_LOG       },
         { "Gold:"          , GOLD_LOG         },
         { "Levels:"        , LOG_LEVELS       },
         { "RIP:"           , RIP_LOG          },
         { "SCRIPT ERR:"    , SCRIPTERR_LOG    },
         { "nusage:"        , USAGE_LOG        },
         { "SYSERR:"        , ERRORS_FILE      },
         { "OLC:"           , OLC_LOG          },
         { "CORPSE:"        , CORPSE_LOG       },
         { "Locate-Obj:"    , LOCATE_OBJ_LOG   }, 
         { "BUF:"           , BUF_LOG          },
         { "New:"           , LOG_NEWPLAYERS   },
         { "Help:"          , HELP_LOG         },
         { "\007\007CAMP ALERT:", BIGRENT_LOG  },
         { "Fight:"         , GODFIGHT_LOG     },
         { "Bad PW:"        , BADPW_LOG        },
         { "Site-Ban:"      , BAN_LOG          },
         { "Script-Log:"    , SCRIPT_LOG       },
         { "DELETE:"        , DELETE_LOG       },
         { "DeathTrap:"     , DT_LOG           },
         { "RENTGONE:"      , RENTGONE_LOG     },
         { "Running "       , RESTART_LOG      },
         { "Obj-Scrap"      , OBJSCRAP_LOG     },
	 { "DG-Quest"       , DG_QUEST_LOG     },
         { "Graffiti"       , GRAFFITI_LOG     },
	 { "Upload"         , UPLOAD_LOG       },
	 { "PLAYER_SHOP"    , SHOP_LOG       }
      };

   *(time_s + strlen(time_s) - 1) = '\0';

   /* write to file, if applicable */
   for (i=0; i<NUM_LOGS; i++)
      if (is_abbrev(log_data[i].pattern, logbuf))
         {
         if (!(fl = fopen(log_data[i].file, "a")))
            {
            perror("SYSERR: parse_log_to_file");
            return;
            }
         fprintf(fl, "%-20.20s :: %s\r\n", time_s+4, logbuf);
         fclose(fl);
         break;
         }

#if 0 /* using stderr and piping to syserr much less intensive */
   /* now write to syslog */
   if (!(fl = fopen(SYSLOG_FILE, "a")))
      {
      perror("SYSERR: parse_log_to_file");
      return;
      }
   fprintf(fl, "%-20.20s :: %s\r\n", time_s+4, logbuf);
   fclose(fl);
#endif
}


/* Check to see if the player is of the correct level -Nomikos 5/31/03 */
int is_remort_level(struct char_data *ch, int remort_type)
{
   if IS_NPC(ch)
      {
      /*
      ** remort NPC is a class of Kensai, 
      ** Deva, Assassin, or Necromancer.    
      **
      ** double remort NPC has to be BOTH one of
      ** the remort classes above, and either a
      ** Draconian, Shadow, Titan, or Aesir.
      */
      switch (remort_type)
         {
         case NON_REMORT:
            if ((GET_CLASS(ch) < MCLASS_KENSAI) ||
                (GET_CLASS(ch) > MCLASS_DEVA))
               {
               return TRUE;
               }
            break;
         case SINGLE_REMORT:
            if ((GET_CLASS(ch) >= MCLASS_KENSAI) &&
                (GET_CLASS(ch) <= MCLASS_DEVA))
               {
               return TRUE;
               }
            break;
         case DOUBLE_REMORT:
            if ((GET_CLASS(ch) >= MCLASS_KENSAI) && 
                (GET_CLASS(ch) <= MCLASS_DEVA) &&
                (GET_RACE(ch) >= MRACE_DRACONIAN) &&
                (GET_RACE(ch) <= MRACE_AESIR))
               {
               return TRUE;
               }
            break;
         default:
            log("SYSERR: Bad remort_type(%d) passed to is_remort!", remort_type);
         }
      }
   else
      {
      if (REMORT_LEVEL(ch) == remort_type)
         return TRUE;
      }

   return FALSE;
}


/* see who, if possible, owns the item, through containers or not */
struct char_data *item_owner(struct obj_data *obj)
{
   struct obj_data *temp = NULL;

   temp = obj;

   while (temp->in_obj)
      temp = temp->in_obj;

   if (temp->carried_by)
      return temp->carried_by;

   if (temp->worn_by)
      return temp->worn_by;

   /* NULL means it is not on a character */
   return NULL;
}

int starts_with(const char *string, const char *key)
{
  if (!string) {
    return 0;
  }
  if (!key) {
    return 0;
  }
  if (strlen(string) < strlen(key)) {
    return 0;
  }
  int i;
  for (i = 0; i < strlen(key); i++) {
    if (string[i] != key[i]) {
      return 0;
    }
  }
  return 1;
}


extern int port;

int is_olc_set(struct char_data *ch, int zone)
{
  if (port != 9000) {
    return 1;
  }
  if (!ch || zone == 0) {
    return 0;
  }
  return 
    zone == GET_OLC_ZONE(ch, 0) ||
    zone == GET_OLC_ZONE(ch, 1) ||
    zone == GET_OLC_ZONE(ch, 2) ||
    zone == GET_OLC_ZONE(ch, 3) ||
    zone == GET_OLC_ZONE(ch, 4)
    ;
}

