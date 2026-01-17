/* *********************************************************************** * 
 *   File: modify.c                                      Part of CircleMUD * 
 *  Usage: Run-time modification of game variables                         * 
 *                                                                         * 
 *  All rights reserved.  See license.doc for complete information.        * 
 *                                                                         * 
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University * 
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
 * *********************************************************************** */ 
 
#include "../localHeader/conf.h" 
#include "../localHeader/sysdep.h" 
 
#include "structs.h" 
#include "buffer.h"
#include "utils.h" 
#include "interpreter.h" 
#include "handler.h" 
#include "db.h" 
#include "comm.h" 
#include "spells.h" 
#include "mail.h" 
#include "boards.h" 
#include "olc.h" 
 
void show_string(struct descriptor_data *d, char *input); 
void smash_tilde(char *str);
void oedit_disp_menu(struct descriptor_data *d); 
void oedit_disp_extradesc_menu(struct descriptor_data *d); 
void redit_disp_menu(struct descriptor_data *d); 
void redit_disp_extradesc_menu(struct descriptor_data *d); 
void redit_disp_exit_menu(struct descriptor_data *d); 
void medit_disp_menu(struct descriptor_data *d); 
void hedit_disp_menu(struct descriptor_data *d); 
void trigedit_disp_menu(struct descriptor_data *d);
char *find_name_by_id(long id);
int file_to_string_alloc(char *name, char **buf);
void write_clan_file (void);
 
#if defined(OASIS_MPROG)
void medit_change_mprog(struct descriptor_data *d);
#endif

extern const char *MENU; 
extern struct spell_info_type *spells;
extern char *news;
 
/*
 * action modes for parse_action 
 */ 
#define PARSE_FORMAT  0  
#define PARSE_REPLACE  1  
#define PARSE_HELP  2  
#define PARSE_DELETE  3 
#define PARSE_INSERT  4 
#define PARSE_LIST_NORM  5 
#define PARSE_LIST_NUM  6 
#define PARSE_EDIT  7 
 
char *string_fields[] = 
{ 
   "name", 
   "short", 
   "long", 
   "description", 
   "title", 
   "delete-description", 
   "\n" 
} ; 
  
 
/*
 * maximum length for text field x+1 
 */ 
int length[] = 
{ 
   15, 
   60, 
   256, 
   240, 
   60 
} ; 
 
void smash_tilde(char *str)
{
#if 1
  /*
   * Erase any ~'s inserted by people in the editor.  This prevents anyone
   * using online creation from causing parse errors in the world files.
   * Derived from an idea by Sammy <samedi@dhc.net> (who happens to like
   * his tildes thank you very much.), -gg 2/20/98
   */
    while ((str = strchr(str, '~')) != NULL)
      *str = ' ';
#endif
}

/*
 * Basic API function to start writing somewhere.
 *
 * 'data' isn't used in stock CircleMUD but you can use it to pass whatever
 * else you may want through it.  The improved editor patch when updated
 * could use it to pass the old text buffer, for instance.
 */
void string_write(struct descriptor_data *d, char **writeto, size_t len, 
		  long mailto, void *data)
{
   if (d->character && !IS_NPC(d->character))
      SET_BIT(PLR_FLAGS(d->character), PLR_WRITING);
   
   if (data)
      mudlog("SYSERR: string_write: I don't understand special data.", BRF,
	     LVL_IMMORT, TRUE);
   
   d->str = writeto;
   d->max_str = len;
   d->mail_to = mailto;
}

/* *********************************************************************** * 
 *		     modification of malloc'ed strings                     *
 * *********************************************************************** */ 
 
/* 
 * handle some editor commands 
 */ 
void parse_action(int command, char *string, struct descriptor_data *d) 
{ 
   int indent = 0, rep_all = 0, flags = 0, total_len, replaced; 
   register int j = 0;
   int i, count, line_low, line_high; 
   char *s, *t, temp; 
   char *buf=get_buffer(MAX_STRING_LENGTH),*buf2;
   char *strtemp, *temppos;

   switch (command) 
      {  
       case PARSE_HELP:  
	  SEND_TO_Q(d,
		  "Editor command formats: /<letter>\r\n\r\n" 
		  "/a         -  aborts editor\r\n" 
		  "/c         -  clears buffer\r\n" 
		  "/d#        -  deletes a line #\r\n" 
		  "/e# <text> -  changes the line at # with <text>\r\n" 
		  "/f         -  formats text\r\n" 
		  "/fi        -  indented formatting of text\r\n" 
		  "/h         -  list text editor commands\r\n" 
		  "/i# <text> -  inserts <text> before line #\r\n" 
		  "/l         -  lists buffer\r\n" 
		  "/n         -  lists buffer with line numbers\r\n" 
		  "/r 'a' 'b' -  replace 1st occurance of text <a> in buffer with text <b>\r\n" 
		  "/ra 'a' 'b'-  replace all occurances of text <a> within buffer with text <b>\r\n" 
		  "              usage: /r[a] 'pattern' 'replacement'\r\n" 
                  "              NOTE: use \\' to replace apostraphe(ex. /r 'cant' 'can\\'t')\r\n"
		  "/s         -  saves text\r\n"); 
	  break; 
       case PARSE_FORMAT:  
	  while (isalpha((int)string[j]) && j < 2) 
	     { 
	     switch (string[j]) 
		{ 
		 case 'i': 
		    if (!indent) 
		       { 
		       indent = 1; 
		       flags += FORMAT_INDENT; 
		       }              
		    break; 
		 default: 
		    break; 
		}      
	     j++; 
	     } 
	  format_text(d->str, flags, d, d->max_str); 
	  SEND_TO_Q(d,"Text formated with%s indent.\r\n", (indent ? "" : "out"));  
	  break;     
       case PARSE_REPLACE:  
	  while (isalpha((int)string[j]) && j < 2) 
	     { 
	     switch (string[j]) 
		{ 
		 case 'a': 
		    if (!indent) 
		       { 
		       rep_all = 1; 
		       }              
		    break; 
		 default: 
		    break; 
		}      
	     j++; 
	     } 
          /* 9-23-02 Nomi, added ability to replace ' */
          j = 0;
          count = 0;
          strtemp = get_buffer(MAX_INPUT_LENGTH+1);
          temppos = strtemp;
          while ((string[j] != '\0') && (j < strlen(string)))
          {
            if (string[j] == '\'')
            {
              if(count == 0)
              {
                string[j] = '`';
                count++;
              }
              else if (count == 1)
              {
                string[j] = '`';
                count--;
              }
            }
            if (string[j] == '\\')
            {
              if (string[j+1] == '\'')
              {
                j++;       
              }
            } 
            *strtemp = string[j];
            strtemp++;
            j++;
          } 
          *strtemp = '\0';
          strtemp = temppos;
          strcpy(string, strtemp);
          release_buffer(strtemp);
/*        SEND_TO_Q(d, "DEBUG: String is %s\r\n", string); */
	  s = strtok(string, "`"); 
	  if (s == NULL) 
	     { 
	     SEND_TO_Q(d,"Invalid format.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
	  s = strtok(NULL, "`"); 
	  if (s == NULL) 
	     { 
	     SEND_TO_Q(d,"Target string must be enclosed in single quotes.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
	  t = strtok(NULL, "`"); 
	  if (t == NULL) 
	     { 
	     SEND_TO_Q(d,"No replacement string.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
	  t = strtok(NULL, "`"); 
	  if (t == NULL) 
	     { 
	     SEND_TO_Q(d,"Replacement string must be enclosed in single quotes.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
	  total_len = ((strlen(t) - strlen(s)) + strlen(*d->str)); 
	  if (total_len <= d->max_str) 
	     { 
	     if ((replaced = replace_str(d->str, s, t, rep_all, d->max_str)) > 0) 
		{ 
		SEND_TO_Q(d,"Replaced %d occurance%sof '%s' with '%s'.\r\n", replaced, ((replaced != 1)?"s ":" "), s, t);  
		} 
	     else if (replaced == 0) 
		{ 
		SEND_TO_Q(d,"String '%s' not found.\r\n", s);  
		} 
	     else 
		{ 
		SEND_TO_Q(d,"ERROR: Replacement string causes buffer overflow, aborted replace.\r\n"); 
		} 
	     } 
	  else 
	     SEND_TO_Q(d,"Not enough space left in buffer.\r\n"); 
	  break; 
       case PARSE_DELETE: 
	  switch (sscanf(string, " %d - %d ", &line_low, &line_high)) 
	     { 
	      case 0: 
		 SEND_TO_Q(d,"You must specify a line number or range to delete.\r\n"); 
		 release_buffer(buf);
		 return; 
	      case 1: 
		 line_high = line_low; 
		 break; 
	      case 2: 
		 if (line_high < line_low) 
		    { 
		    SEND_TO_Q(d,"That range is invalid.\r\n"); 
		    release_buffer(buf);
		    return; 
		    } 
		 break; 
	     } 
       
	  i = 1; 
	  total_len = 1; 
	  if ((s = *d->str) == NULL) 
	     { 
	     SEND_TO_Q(d,"Buffer is empty.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
	  if (line_low > 0) 
	     { 
	     while (s && (i < line_low)) 
		if ((s = strchr(s, '\n')) != NULL) 
		   { 
		   i++; 
		   s++; 
		   } 
	     if ((i < line_low) || (s == NULL)) 
		{ 
		SEND_TO_Q(d,"Line(s) out of range; not deleting.\r\n"); 
		release_buffer(buf);
		return; 
		} 
   
	     t = s; 
	     while (s && (i < line_high)) 
		if ((s = strchr(s, '\n')) != NULL) 
		   { 
		   i++; 
		   total_len++; 
		   s++; 
		   } 
	     if ((s) && ((s = strchr(s, '\n')) != NULL)) 
		{ 
		s++; 
		while (*s != '\0') *(t++) = *(s++); 
		} 
	     else total_len--; 
	     *t = '\0'; 
	     RECREATE(*d->str, char, strlen(*d->str) + 3); 
	     SEND_TO_Q(d,"%d line%sdeleted.\r\n", total_len, 
		     ((total_len != 1)?"s ":" ")); 
	     } 
	  else 
	     { 
	     SEND_TO_Q(d,"Invalid line numbers to delete must be higher than 0.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
	  break; 
       case PARSE_LIST_NORM: 
	 /* note: my buf,buf1,buf2 vars are defined at 32k sizes so they 
	  * are prolly ok fer what i want to do here. */ 
	  *buf = '\0'; 
	  if (*string != '\0') 
	     switch (sscanf(string, " %d - %d ", &line_low, &line_high)) 
		{ 
		 case 0: 
		    line_low = 1; 
		    line_high = 999999; 
		    break; 
		 case 1: 
		    line_high = line_low; 
		    break; 
		} 
	  else 
	     { 
	     line_low = 1; 
	     line_high = 999999; 
	     } 
       
	  if (line_low < 1) 
	     { 
	     SEND_TO_Q(d,"Line numbers must be greater than 0.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
	  if (line_high < line_low) 
	     { 
	     SEND_TO_Q(d,"That range is invalid.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
	  *buf = '\0'; 
	  if ((line_high < 999999) || (line_low > 1)) 
	     { 
	     sprintf(buf, "Current buffer range [%d - %d]:\r\n", line_low, line_high); 
	     } 
	  i = 1; 
	  total_len = 0; 
	  s = *d->str; 
	  while (s && (i < line_low)) 
	     if ((s = strchr(s, '\n')) != NULL) 
		{ 
		i++; 
		s++; 
		} 
	  if ((i < line_low) || (s == NULL)) 
	     { 
	     SEND_TO_Q(d,"Line(s) out of range; no buffer listing.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
       
	  t = s; 
	  while (s && (i <= line_high)) 
	     if ((s = strchr(s, '\n')) != NULL) 
		{ 
		i++; 
		total_len++; 
		s++; 
		} 
	  if (s) 
	     { 
	     temp = *s; 
	     *s = '\0'; 
	     strcat(buf, t); 
	     *s = temp; 
	     } 
	  else strcat(buf, t); 
	 /* this is kind of annoying.. will have to take a poll and see.. 
	    sprintf(buf, "%s\r\n%d line%sshown.\r\n", buf, total_len, 
	    ((total_len != 1)?"s ":" ")); 
	    */ 
	  page_string(d, buf, TRUE,""); 
	  break; 
       case PARSE_LIST_NUM: 
	 /* note: my buf,buf1,buf2 vars are defined at 32k sizes so they 
	  * are prolly ok fer what i want to do here. */ 
	  *buf = '\0'; 
	  if (*string != '\0') 
	     switch (sscanf(string, " %d - %d ", &line_low, &line_high)) 
		{ 
		 case 0: 
		    line_low = 1; 
		    line_high = 999999; 
		    break; 
		 case 1: 
		    line_high = line_low; 
		    break; 
		} 
	  else 
	     { 
	     line_low = 1; 
	     line_high = 999999; 
	     } 
       
	  if (line_low < 1) 
	     { 
	     SEND_TO_Q(d,"Line numbers must be greater than 0.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
	  if (line_high < line_low) 
	     { 
	     SEND_TO_Q(d,"That range is invalid.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
	  *buf = '\0'; 
	  i = 1; 
	  total_len = 0; 
	  s = *d->str; 
	  while (s && (i < line_low)) 
	     if ((s = strchr(s, '\n')) != NULL) 
		{ 
		i++; 
		s++; 
		} 
	  if ((i < line_low) || (s == NULL)) 
	     { 
	     SEND_TO_Q(d,"Line(s) out of range; no buffer listing.\r\n"); 
	     release_buffer(buf);
	     return; 
	     } 
       
	  t = s; 
	  while (s && (i <= line_high)) 
	     if ((s = strchr(s, '\n')) != NULL) 
		{ 
		i++; 
		total_len++; 
		s++; 
		temp = *s; 
		*s = '\0'; 
		sprintf(buf, "%s%4d:\r\n", buf, (i-1)); 
		strcat(buf, t); 
		*s = temp; 
		t = s; 
		} 
	  if (s && t) 
	     { 
	     temp = *s; 
	     *s = '\0'; 
	     strcat(buf, t); 
	     *s = temp; 
	     } 
	  else if (t) strcat(buf, t); 
	 /* this is kind of annoying .. seeing as the lines are #ed 
	    sprintf(buf, "%s\r\n%d numbered line%slisted.\r\n", buf, total_len, 
	    ((total_len != 1)?"s ":" ")); 
	    */ 
	  page_string(d, buf, TRUE,""); 
	  break; 
 
       case PARSE_INSERT: 
	  buf2=get_buffer(32750);
	  half_chop(string, buf, buf2); 
	  if (*buf == '\0') 
	     { 
	     SEND_TO_Q(d,"You must specify a line number before which to insert text.\r\n"); 
	     release_buffer(buf2);
	     release_buffer(buf);
	     return; 
	     } 
	  line_low = atoi(buf); 
	  strcat(buf2, "\r\n"); 
       
	  i = 1; 
	  *buf = '\0'; 
	  if ((s = *d->str) == NULL) 
	     { 
	     SEND_TO_Q(d,"Buffer is empty, nowhere to insert.\r\n"); 
	     release_buffer(buf2);
	     release_buffer(buf);
	     return; 
	     } 
	  if (line_low > 0) 
	     { 
	     while (s && (i < line_low)) 
		if ((s = strchr(s, '\n')) != NULL) 
		   { 
		   i++; 
		   s++; 
		   } 
	     if ((i < line_low) || (s == NULL)) 
		{ 
		SEND_TO_Q(d,"Line number out of range; insert aborted.\r\n"); 
		release_buffer(buf2);
		release_buffer(buf);
		return; 
		} 
	     temp = *s; 
	     *s = '\0'; 
	     if ((strlen(*d->str) + strlen(buf2) + strlen(s+1) + 3) > d->max_str) 
		{ 
		*s = temp; 
		SEND_TO_Q(d,"Insert text pushes buffer over maximum size, insert aborted.\r\n"); 
		release_buffer(buf2);
		release_buffer(buf);
		return; 
		} 
	     if (*d->str && (**d->str != '\0')) strcat(buf, *d->str); 
	     *s = temp; 
	     strcat(buf, buf2); 
	     if (s && (*s != '\0')) strcat(buf, s); 
	     RECREATE(*d->str, char, strlen(buf) + 3); 
	     strcpy(*d->str, buf); 
	     SEND_TO_Q(d,"Line inserted.\r\n"); 
	     } 
	  else 
	     { 
	     SEND_TO_Q(d,"Line number must be higher than 0.\r\n"); 
	     release_buffer(buf2);
	     release_buffer(buf);
	     return; 
	     } 
	  release_buffer(buf2);
	  break; 
 
       case PARSE_EDIT: 
	  buf2=get_buffer(32750);
	  half_chop(string, buf, buf2); 
	  if (*buf == '\0') 
	     { 
	     SEND_TO_Q(d,"You must specify a line number at which to change text.\r\n"); 
	     release_buffer(buf2);
	     release_buffer(buf);
	     return; 
	     } 
	  line_low = atoi(buf); 
	  strcat(buf2, "\r\n"); 
       
	  i = 1; 
	  *buf = '\0'; 
	  if ((s = *d->str) == NULL) 
	     { 
	     SEND_TO_Q(d,"Buffer is empty, nothing to change.\r\n"); 
	     release_buffer(buf2);
	     release_buffer(buf);
	     return; 
	     } 
	  if (line_low > 0) 
	     { 
	    /* loop through the text counting /n chars till we get to the line */ 
	     while (s && (i < line_low)) 
		if ((s = strchr(s, '\n')) != NULL) 
		   { 
		   i++; 
		   s++; 
		   } 
	    /* make sure that there was a THAT line in the text */ 
	     if ((i < line_low) || (s == NULL)) 
		{ 
		SEND_TO_Q(d,"Line number out of range; change aborted.\r\n"); 
		release_buffer(buf2);
		release_buffer(buf);
		return; 
		} 
	    /* if s is the same as *d->str that means im at the beginning of the 
	     * message text and i dont need to put that into the changed buffer */ 
	     if (s != *d->str) 
		{ 
	       /* first things first .. we get this part into buf. */ 
		temp = *s; 
		*s = '\0'; 
	       /* put the first 'good' half of the text into storage */ 
		strcat(buf, *d->str); 
		*s = temp; 
		} 
	    /* put the new 'good' line into place. */ 
	     strcat(buf, buf2); 
	     if ((s = strchr(s, '\n')) != NULL) 
		{ 
	       /* this means that we are at the END of the line we want outta there. */ 
	       /* BUT we want s to point to the beginning of the line AFTER 
		* the line we want edited */ 
		s++; 
	       /* now put the last 'good' half of buffer into storage */ 
		strcat(buf, s); 
		} 
	    /* check for buffer overflow */ 
	     if (strlen(buf) > d->max_str) 
		{ 
		SEND_TO_Q(d,"Change causes new length to exceed buffer maximum size, aborted.\r\n"); 
		release_buffer(buf2);
		release_buffer(buf);
		return; 
		} 
	    /* change the size of the REAL buffer to fit the new text */ 
	     RECREATE(*d->str, char, strlen(buf) + 3); 
	     strcpy(*d->str, buf); 
	     SEND_TO_Q(d,"Line changed.\r\n"); 
	     } 
	  else 
	     { 
	     SEND_TO_Q(d,"Line number must be higher than 0.\r\n"); 
	     release_buffer(buf2);
	     release_buffer(buf);
	     return; 
	     } 
	  release_buffer(buf2);
	  break; 
       default: 
	  SEND_TO_Q(d,"Invalid option.\r\n"); 
	  mudlogf(BRF, LVL_IMPL, TRUE,
		  "SYSERR: invalid command passed to parse_action"); 
	  release_buffer(buf);
	  return; 
      } 
   release_buffer(buf);
} 
 
 
/* Add user input to the 'current' string (as defined by d->str) */ 
void string_add(struct descriptor_data *d, char *str) 
{ 
   FILE *fl;
   int terminator = 0, action = 0,too_long; 
   register int i = 2, j = 0; 
   char *actions=get_buffer(MAX_INPUT_LENGTH); 
 
  /* determine if this is the terminal string, and truncate if so */ 
  /* changed to accept '/<letter>' style editing commands - instead */ 
  /* of solitary '@' to end - (modification of improved_edit patch) */ 
  /*   M. Scott 10/15/96 */ 
 
   delete_doubledollar(str); 
 
  /*
   * removed old handling of '@' char 
   * if ((terminator = (*str == '@'))) *str = '\0'; 
   */ 
 
   if ((action = (*str == '/'))) 
      { 
      while (str[i] != '\0') 
	 { 
	 actions[j] = str[i];               
	 i++; 
	 j++; 
	 } 
      actions[j] = '\0'; 
      *str = '\0'; 
      switch (str[1]) 
	 { 
	  case 'a': 
	     terminator = 2; /* working on an abort message */ 
	     break; 
	  case 'c': 
	     if (*(d->str)) 
		{ 
		free(*d->str); 
		*(d->str) = NULL; 
		SEND_TO_Q(d,"Current buffer cleared.\r\n"); 
		} 
	     else 
		SEND_TO_Q(d,"Current buffer empty.\r\n"); 
	     break; 
	  case 'd': 
	     parse_action(PARSE_DELETE, actions, d); 
	     break; 
	  case 'e': 
	     parse_action(PARSE_EDIT, actions, d); 
	     break; 
	  case 'f':  
	     if (*(d->str)) 
		parse_action(PARSE_FORMAT, actions, d); 
	     else 
		SEND_TO_Q(d,"Current buffer empty.\r\n"); 
	     break; 
	  case 'i': 
	     if (*(d->str)) 
		parse_action(PARSE_INSERT, actions, d); 
	     else 
		SEND_TO_Q(d,"Current buffer empty.\r\n"); 
	     break; 
	  case 'h':  
	     parse_action(PARSE_HELP, actions, d); 
	     break; 
	  case 'l': 
	     if (*d->str) 
		parse_action(PARSE_LIST_NORM, actions, d); 
	     else SEND_TO_Q(d,"Current buffer empty.\r\n"); 
	     break; 
	  case 'n': 
	     if (*d->str) 
		parse_action(PARSE_LIST_NUM, actions, d); 
	     else SEND_TO_Q(d,"Current buffer empty.\r\n"); 
	     break; 
	  case 'r':    
	     parse_action(PARSE_REPLACE, actions, d); 
	     break; 
	  case 's': 
	     terminator = 1; 
	     *str = '\0'; 
	     break; 
	  default: 
	     SEND_TO_Q(d,"Invalid option.\r\n"); 
	     break; 
	 } 
      } 
   too_long=0;
   smash_tilde(str);
   if (!(*d->str)) 
      { 
      if (strlen(str) > d->max_str) 
	 { 
	 send_to_char(d->character, "String too long - Truncated.\r\n");
	 *(str + d->max_str) = '\0'; 
	/* changed this to NOT abort out.. just give warning. */ 
	/* terminator = 1; */ 
	 } 
      CREATE(*d->str, char, strlen(str) + 3); 
      strcpy(*d->str, str); 
      } 
   else 
      { 
      if (strlen(str) + strlen(*d->str)+4 > d->max_str) 
	 { 
	 send_to_char(d->character, "String too long, limit reached on message.  Last line ignored.\r\n"); 
	 too_long=1;
/* 	 terminator = 1; */
	 } 
      else 
	 { 
	 if (!(*d->str = (char *) realloc(*d->str, strlen(*d->str) + 
					  strlen(str) + 3))) 
	    { 
	    perror("SYSERR: string_add"); 
	    exit(1); 
	    } 
	 strcat(*d->str, str); 
	 } 
      } 
 
   if (terminator) 
      { 
     /*. OLC Edits .*/ 
      
#if defined(OASIS_MPROG)
      
      if (STATE(d) == CON_MEDIT) 
	 {
	 switch (OLC_MODE(d)) 
	    {
	     case MEDIT_D_DESC:
		medit_disp_menu(d);
		break;
	     case MEDIT_MPROG_COMLIST:
		medit_change_mprog(d);
		break;
	    }
	 }
#endif
     /* here we check for the abort option and reset the pointers */ 
      if ((terminator == 2) && 
	  ((STATE(d) == CON_REDIT) || 
	   (STATE(d) == CON_MEDIT) || 
	   (STATE(d) == CON_OEDIT) || 
	   (STATE(d) == CON_HEDIT) || 
	   (STATE(d) == CON_TEXTED)|| 
	   (STATE(d) == CON_TRIGEDIT) ||
	   (STATE(d) == CON_EXDESC))) 
	 { 
	 free(*d->str); 
	 if (d->backstr) 
	    { 
	    *d->str = d->backstr; 
	    } 
	 else 
	    *d->str = NULL; 
	 d->backstr = NULL; 
	 d->str = NULL; 
	 } 
     /*
      * this fix causes the editor to NULL out empty messages -- M. Scott 
      * Fixed to fix the fix for empty fixed messages. -- gg
      */ 
      else if ((d->str) && (*d->str) && (**d->str == '\0')) 
	 { 
	 free(*d->str); 
	 *d->str = str_dup("Nothing.\r\n");
	 } 
      
      if (STATE(d) == CON_MEDIT) 
	 medit_disp_menu(d); 

      if (STATE(d) == CON_TRIGEDIT)
	 trigedit_disp_menu(d);

      if (STATE(d) == CON_OEDIT) 
	 { 
	 switch(OLC_MODE(d)) 
	    { 
	     case OEDIT_ACTDESC: 
		oedit_disp_menu(d); 
		break; 
	     case OEDIT_EXTRADESC_DESCRIPTION: 
		oedit_disp_extradesc_menu(d); 
	    } 
	 } 
      else if (STATE(d) == CON_HEDIT) 
	 { 
	 hedit_disp_menu(d);
	 } 
      else if (STATE(d) == CON_REDIT) 
	 { 
	 switch(OLC_MODE(d)) 
	    { 
	     case REDIT_DESC: 
		redit_disp_menu(d); 
		break; 
	     case REDIT_EXIT_DESCRIPTION: 
		redit_disp_exit_menu(d); 
		break; 
	     case REDIT_EXTRADESC_DESCRIPTION: 
		redit_disp_extradesc_menu(d); 
		break; 
	    } 
	 } 
      else if (STATE(d)==CON_PLAYING&&(PLR_FLAGGED(d->character, PLR_MAILING)))
	 { 
	 if ((terminator == 1) && *d->str)  
	    { 
	    store_mail(find_name_by_id(d->mail_to), GET_NAME(d->character),
		       *d->str); 
	    SEND_TO_Q(d,"Message sent!\r\n"); 
	    } 
	 else 
	    SEND_TO_Q(d,"Mail aborted.\r\n"); 
	 d->mail_to = 0; 
	 free(*d->str); 
	 free(d->str); 
	 } 
      else if (d->mail_to >= BOARD_MAGIC) 
	 { 
	 if (terminator == 2) 
	    SEND_TO_Q(d,"Board messages cannot be aborted.\r\n"); 
	 SEND_TO_Q(d,"Message posted, use board remove <message number> to delete.\r\n"); 
	 Board_save_board(d->mail_to - BOARD_MAGIC); 
	 d->mail_to = 0; 
	 } 
      else if (STATE(d) == CON_EXDESC) 
	 { 
	 if (terminator != 1)
	    SEND_TO_Q(d,"Description aborted.\r\n"); 
	 SEND_TO_Q(d, "%s", MENU); 
	 STATE(d) = CON_MENU; 
	 } 
      else if (STATE(d) == CON_TEXTED || STATE(d) == CON_ADD_NEWS)
	 {
	 if (terminator == 1) 
	    {
	    if (!(fl = fopen((char *)d->storage, "w"))) 
	       {
	       mudlogf(CMP, GOD_LOG(d->character), TRUE,
		       "SYSERR: Can't write file '%s'.",d->storage);
	       }
	    else 
	       {
               if (STATE(d) == CON_TEXTED)
                  {
   	          if (*d->str)
		     {
		     char *buf1=get_buffer(32750);
		     fputs(stripcr(buf1, *d->str), fl);
		     release_buffer(buf1);
		     }
	          fclose(fl);
	          mudlogf(CMP, GOD_LOG(d->character),TRUE, "OLC: %s saves '%s'.", 
		         GET_NAME(d->character), d->storage);
	          SEND_TO_Q(d,"Saved.\r\n");
	          }
               else
                  {
                  if (*d->str)
                     {
                     int found = FALSE;
                     char *s = news;
                     char tmp;
                     time_t mytime = time(0);
                     char *dispdate = get_buffer(64);
                     char *buf1=get_buffer(32750);
                     while (s && (found != TRUE))
                        if ((s = strchr(s, '^')) != NULL)
                           {
                           found = TRUE;
                           s+=2;
                           }
                     if (!s)
                        {
                        mudlogf(BRF, LVL_IMMORT, TRUE,
                           "SYSERR: addnews needs to have a ^ character "
                           "where data is to be entered.");
                        fputs(news, fl);
                        }
                     else
                        {
                        tmp = *s;
                        *s = '\0';
                        strftime(dispdate, 20, "%a %b %d, %Y", localtime(&mytime));
                        fprintf(fl, "%s\n", stripcr(buf1, news));
                        fprintf(fl, "%s\n", dispdate);
                        *s = tmp;
                        fprintf(fl, "%s", stripcr(buf1, *d->str));
                        fprintf(fl, "%s", stripcr(buf1, s));
                        }
                     release_buffer(dispdate);
                     release_buffer(buf1);
                     }
                  fclose(fl);
                  file_to_string_alloc(NEWS_FILE, &news);
                  mudlogf(CMP, GOD_LOG(d->character),TRUE, "OLC: %s adds an entry to news",
                         GET_NAME(d->character));
                  SEND_TO_Q(d,"News entry added and saved.\r\n");
                  }
               }
	    }
	 else 
	    SEND_TO_Q(d,"Edit aborted.\r\n");
	 act("$n stops editing some scrolls.",TRUE,d->character,0,0,TO_ROOM);
	 free(d->storage);
	 d->storage = NULL;
	 STATE(d) = CON_PLAYING;
	 }
      else if (STATE(d)==CON_PLAYING && d->character && !IS_NPC(d->character)) 
	 { 
	 if (terminator == 1) 
	    { 
	    if (strlen(*d->str) == 0) 
	       { 
	       free(*d->str); 
	       *d->str = NULL; 
	       } 
	    } 
	 else 
	    { 
	    free(*d->str); 
	    if (d->backstr) 
	       { 
	       *d->str = d->backstr; 
	       } 
	    else
	       *d->str = NULL; 
	    d->backstr = NULL; 
	    SEND_TO_Q(d,"Message aborted.\r\n"); 
	    } 
	 } 
      
      if (d->character && !IS_NPC(d->character)) 
	 {
	 if (PLR_FLAGGED(d->character, PLR_CHARTER))
	    write_clan_file();
	 REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_MAILING | PLR_CHARTER); 
	 }
      if (d->backstr) 
	 free(d->backstr); 
      d->backstr = NULL; 
      d->str = NULL; 
      } 
   else if (!action&&too_long==0)
      strcat(*d->str, "\r\n"); 

   release_buffer(actions);
} 
 
 
 
 
/* ********************************************************************** 
*  Modification of character skills                                     * 
********************************************************************** */ 
 
ACMD(do_skillset) 
{ 
   struct char_data *vict; 
   char *name = get_buffer(MAX_INPUT_LENGTH), *buf, *help;
   int skill, value, i, qend; 

   if (!argument || !*argument) {
     send_to_char(ch, "Usage: skillset <player> '<skill/spell name>' <value>\r\n");
     return;
   }
 
   argument = one_argument(argument, name); 
 
  /* 
   * no arguments. print an informative text 
   */ 
   if (!*name) 
      {   
      release_buffer(name);
      help = get_buffer(MAX_STRING_LENGTH);
      send_to_char(ch, "Syntax: skillset <name> '<skill>' <value>\r\n"); 
      strcpy(help, "Skill being one of the following:\r\n"); 
      for (i = 0; spells[i].spell_name[0] != '\n'; i++) 
	 { 
	 if (spells[i].spell_name[0] == '!') 
	    continue; 
	 sprintf(help + strlen(help), "%-25.25s", spells[i].spell_name); 
	 if (i % 3 == 2) 
	    { 
	    strcat(help, "\r\n"); 
	    send_to_char(ch, "%s",help); 
	    *help = '\0'; 
	    } 
	 } 
      if (*help) 
	 send_to_char(ch, "%s", help); 
      send_to_char(ch, "\r\n"); 
      release_buffer(help);
      return; 
      } 
   if (!(vict = get_char_vis(ch, name,FIND_CHAR_WORLD))) 
      { 
      send_to_char(ch, "%s", NOPERSON); 
      release_buffer(name);
      return; 
      } 
   release_buffer(name);
   skip_spaces(&argument); 
 
  /*
   * If there is no chars in argument 
   */ 
   if (!*argument) 
      { 
      send_to_char(ch, "Skill name expected.\r\n"); 
      return; 
      } 
   else if (*argument != '\'') 
      { 
      send_to_char(ch, "Skill must be enclosed in: ''\r\n"); 
      return; 
      } 
  /*
   * Locate the last quote && lowercase the magic words (if any) 
   */ 
 
   for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++) 
      *(argument + qend) = LOWER(*(argument + qend)); 
 
   if (*(argument + qend) != '\'') 
      { 
      send_to_char(ch, "Skill must be enclosed in: ''\r\n"); 
      return; 
      } 

   help = get_buffer(MAX_STRING_LENGTH);
   strcpy(help, (argument + 1)); 
   help[qend - 1] = '\0'; 
   if ((skill = find_skill_num(help)) <= 0) 
      { 
      send_to_char(ch, "Unrecognized skill.\r\n"); 
      release_buffer(help);
      return; 
      } 
   release_buffer(help);
   argument += qend + 1;  
  /*
   * skip to next parameter 
   */ 
   buf = get_buffer(MAX_INPUT_LENGTH);
   argument = one_argument(argument, buf); 
 
   if (!*buf) 
      { 
      send_to_char(ch, "Learned value expected.\r\n"); 
      release_buffer(buf);
      return; 
      } 
   value = atoi(buf); 
   release_buffer(buf);
   if (value < 0) 
      { 
      send_to_char(ch, "Minimum value for learned is 0.\r\n"); 
      return; 
      } 
   else if ((value > 100)&&subcmd==SCMD_SETLEARN)
      { 
      send_to_char(ch, "Max value for learned is 100.\r\n"); 
      return; 
      } 
   else if((value>95)&&(spells[skill].is_spell==IS_SKILL)
	   &&(subcmd==SCMD_SETSKILL))
      { 
      send_to_char(ch, "Max value for skills is 95.\r\n"); 
      return; 
      } 
   else if((value>10)&&(spells[skill].is_spell==IS_SPELL)
	   &&(subcmd==SCMD_SETSKILL))
      { 
      send_to_char(ch, "Max value for spells is 10.\r\n"); 
      return; 
      } 
   else if (IS_NPC(vict)) 
      { 
      send_to_char(ch, "You can't set NPC skills.\r\n"); 
      return; 
      } 
   mudlogf(BRF, GOD_LOG(ch), TRUE,"(GC) %s changed %s's %s to %d.", 
	   GET_NAME(ch), GET_NAME(vict), spells[skill].spell_name, value);  
 
   if(subcmd==SCMD_SETSKILL)
      GET_SKILL(vict, skill)= value; 
   else if(subcmd==SCMD_SETLEARN)
      GET_SKILL_LEARN(vict,skill)=value;

   send_to_char(ch, "You change %s's %s to %d.\r\n", GET_NAME(vict), 
	   spells[skill].spell_name, value); 
} 
 
 
 
/********************************************************************* 
* New Pagination Code 
* Michael Buselli submitted the following code for an enhanced pager 
* for CircleMUD.  All functions below are his.  --JE 8 Mar 96 
* 
*********************************************************************/ 
 
#define PAGE_LENGTH     21
#define PAGE_WIDTH      90 
 
/*
 * Traverse down the string until the begining of the next page has been 
 * reached.  Return NULL if this is the last page of the string. 
 */ 
/* 
 * begin add - Bon 07/28/97 added page_length
 */ 
char *next_page(char *str, int page_length) 
{ 
   int col = 1, line = 1, spec_code = FALSE; 
 
   for (;; str++) 
      { 
     /* If end of string, return NULL. */ 
      if (*str == '\0') 
	 return NULL; 
 
      else if (line > page_length) 
	 return str; 
 
     /* Check for the begining of an ANSI color code block. */ 
      else if (*str == '\x1B' && !spec_code) 
	 {
	 spec_code = TRUE; 
	 }
     /* Check for the end of an ANSI color code block. */ 
      else if (*str == 'm' && spec_code) 
	 {
	 spec_code = FALSE; 
	 }
     /* Check for everything else. */ 
      else if (!spec_code) 
	 { 
	/* Carriage return puts us in column one. */ 
	 if (*str == '\r') 
	    col = 1; 
	/* Newline puts us on the next line. */ 
	 else if (*str == '\n') 
	    line++; 
 
	/* We need to check here and see if we are over the page width, 
	 * and if so, compensate by going to the begining of the next line. 
	 */ 
	 else if (col++ > PAGE_WIDTH) 
	    { 
	    col = 1; 
	    line++; 
	    } 
	 } 
      } 
} 
 
 
/* Function that returns the number of pages in the string. */ 
int count_pages(char *str, int page_length) 
{ 
   int pages; 
   for (pages = 1; (str = next_page(str, page_length)); pages++); 
   return pages; 
} 
 
 
/* This function assigns all the pointers for showstr_vector for the 
 * page_string function, after showstr_vector has been allocated and 
 * showstr_count set. 
 */ 
void paginate_string(char *str, struct descriptor_data *d) 
{ 
   int i; 
   int screensize;
   if (d->showstr_count) 
      *(d->showstr_vector) = str; 
 
   if(!d->character)
      screensize=23;
   else if(d->original)
      screensize=d->original->player_specials->saved.screensize-2;
   else
      screensize=d->character->player_specials->saved.screensize-2;
   screensize=MAX(13,MIN(screensize,87));
   for (i = 1; i < d->showstr_count && str; i++) 
      {
      str = d->showstr_vector[i] = next_page(str, screensize); 
      }
   d->showstr_page = 0; 
} 
 
 
/* The call that gets the paging ball rolling... */ 
void page_string(struct descriptor_data *d, char *str, int keep_internal, char *sHeader) 
{ 
   int screensize;
   if (!d) 
      return; 
 
   if (!str || !*str) 
      { 
      send_to_char(d->character,"%c",'\0'); 
      return; 
      } 
   if( strlen( sHeader ) < 1 )
      d->sHeader[0] = '\0';
   else
      strcpy( d->sHeader, sHeader );
   if(!d->character)
      screensize=23;
   else if(d->original)
      screensize=d->original->player_specials->saved.screensize-2;
   else
      screensize=d->character->player_specials->saved.screensize-2;
   screensize=MAX(13,MIN(screensize,87));
   d->showstr_count=count_pages(str,screensize);
   CREATE(d->showstr_vector, char *, d->showstr_count); 

   if (keep_internal) 
      { 
      d->showstr_head = str_dup(str); 
      paginate_string(d->showstr_head, d); 
      } 
   else 
      paginate_string(str, d); 
 
   show_string(d, ""); 
} 
 
 
/* The call that displays the next page. */ 
void show_string(struct descriptor_data *d, char *input) 
{ 
   char *buf=get_buffer(MAX_STRING_LENGTH); 
   int diff; 
   int screensize;
   any_one_arg(input, buf); 
 
  /* Q is for quit. :) */ 
   if (LOWER(*buf) == 'q') 
      { 
      free(d->showstr_vector); 
      d->showstr_count = 0; 
      if (d->showstr_head) 
	 { 
	 free(d->showstr_head); 
	 d->showstr_head = 0; 
	 } 
      release_buffer(buf);
      return; 
      } 
  /* R is for refresh, so back up one page internally so we can display 
   * it again. 
   */ 
   else if (LOWER(*buf) == 'r') 
      d->showstr_page = MAX(0, d->showstr_page - 1); 
 
  /* B is for back, so back up two pages internally so we can display the 
   * correct page here. 
   */ 
   else if (LOWER(*buf) == 'b') 
      d->showstr_page = MAX(0, d->showstr_page - 2); 
 
  /* Feature to 'goto' a page.  Just type the number of the page and you 
   * are there! 
   */ 
   else if (isdigit((int)*buf)) 
      d->showstr_page = MAX(0, MIN(atoi(buf) - 1, d->showstr_count - 1)); 
 
   else if (*buf) 
      { 
      send_to_char(d->character, 
	 "Valid commands while paging are RETURN, Q, R, B, or a numeric value.\r\n"); 
      release_buffer(buf);
      return; 
      } 
   release_buffer(buf);

  /* Send the header, if it exists */
   if(d->sHeader[0]!='\0')
      send_to_char(d->character, "%s", d->sHeader);

  /* If we're displaying the last page, just send it to the character, and 
   * then free up the space we used. 
   */ 
   if(!d->character)
      screensize=23;
   else if(d->original)
      screensize=d->original->player_specials->saved.screensize-2;
   else
      screensize=d->character->player_specials->saved.screensize-2;
   screensize=MAX(13,MIN(screensize,87));
   if (d->showstr_page + 1 >= d->showstr_count) 
      { 
      send_to_char(d->character, "%s", d->showstr_vector[d->showstr_page]); 
      free(d->showstr_vector); 
      d->showstr_count = 0; 
      if (d->showstr_head) 
	 { 
	 free(d->showstr_head); 
	 d->showstr_head = NULL; 
	 } 
      } 
  /* Or if we have more to show.... */ 
   else 
      { 
      char *disp_buf = get_buffer((MAX(screensize*100,MAX_STRING_LENGTH)));
      diff = d->showstr_vector[d->showstr_page + 1] - d->showstr_vector[d->showstr_page];

      if (diff >= MAX_STRING_LENGTH)
	 diff = MAX_STRING_LENGTH - 1;
      strncpy(disp_buf, d->showstr_vector[d->showstr_page], diff);
      disp_buf[diff] = '\0'; 
      send_to_char(d->character, "%s", disp_buf); 
      d->showstr_page++; 
      release_buffer(disp_buf);
      } 
} 

