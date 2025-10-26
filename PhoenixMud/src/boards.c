/* ************************************************************************
*   File: boards.c                              a new   Part of CircleMUD * 
*  Usage: handling of multiple bulletin boards                            * 
*                                                                         *
*  This is a nearly Complete Rewrite by Raolin of Phoenix Mud.            * 
*  There was some really DainBramaged stuff going on.  I'm here to fix    * 
*  that.                                                                  * 
*                                                                         * 
*  All rights reserved.  See license.doc for complete information.        * 
*                                                                         * 
*  Copyright (C) 1993 by the Trustees of the Johns Hopkins University     * 
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
************************************************************************ */

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"
#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "boards.h"
#include "interpreter.h"
#include "handler.h"
#include "screen.h"
#include "clan.h"

extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct command_info cmd_info[];
extern char *str_str(char *cs, char *ct);

void Board_display_msg(int board_type, struct char_data *ch, char *arg);
void Board_show_board(int board_type, struct char_data *ch, char *arg);
void Board_remove_msg(int board_type, struct char_data *ch, char *arg);
void Board_save_board(int board_type);
void Board_load_board(int board_type);
void Board_reset_board(int board_num);
void Board_write_message(int board_type, struct char_data *ch, char *arg);
void Board_check(struct char_data *ch);
void Board_search(struct char_data *ch, char *arg);
int can_see_board(int board_num,struct char_data *ch);
/*
format: vnum, read lvl, write lvl, remove lvl, filename, 0 at end  
Be sure to also change NUM_OF_BOARDS in board.h 
*/

struct board_info_type board_info[NUM_OF_BOARDS] =
      {
      /* vnum,read_lvl,write_lvl,del_lvl,filename,rnum,clan_num */
      { 3096, 0         , 0         , LVL_SIMP, "boards/Board.social"     ,0,0},
      { 3099, 0         , 0         , LVL_SIMP, "boards/Board.mort"       ,0,0},
      { 3094, 0         , 0         , LVL_SIMP, "boards/Board.mort_jobs"  ,0,0},
      { 3090, LVL_SIMP  , 0         , LVL_SIMP, "boards/Board.mort_to_imp",0,0},
      { 3040, 0         , LVL_IMMORT, LVL_SIMP, "boards/Board.quest"      ,0,0},
      { 3092, 0         , LVL_IMMORT, LVL_SIMP, "boards/Board.info"       ,0,0},
      { 1   , 0         , 0         , LVL_SIMP, "boards/Board.forsaken"   ,0,99},
      { 3   , 0         , 0         , LVL_SIMP, "boards/Board.legion"     ,0,3},
      { 3098, LVL_IMMORT, LVL_IMMORT, LVL_SIMP, "boards/Board.immort"     ,0,0},
      { 3095, LVL_IMMORT, LVL_IMMORT, LVL_SIMP, "boards/Board.imm_todo"   ,0,0},
      { 3091, LVL_IMMORT, LVL_IMMORT, LVL_SIMP, "boards/Board.builder"    ,0,0},
      { 3097, LVL_IMMORT, LVL_FREEZE, LVL_SIMP, "boards/Board.freeze"     ,0,0},
      { 1200, LVL_IMMORT, LVL_IMMORT, LVL_ADMIN, "boards/Board.coder"     ,0,0},
      { 4   , LVL_ADMIN , LVL_ADMIN , LVL_IMPL, "boards/Board.imp"        ,0,0},
      { 5   , LVL_IMMORT, LVL_IMMORT, LVL_SIMP, "boards/Board.caledon"    ,0,0},
      { 6   , LVL_IMMORT, LVL_IMMORT, LVL_SIMP, "boards/Board.aglaron"    ,0,0},
      { 2   , 0         , 0         , LVL_SIMP, "boards/Board.syndicate"  ,0,2},
      { 7   , LVL_IMMORT, LVL_IMMORT, LVL_SIMP, "boards/Board.underdark"  ,0,0},
      { 8   , 0         , 0         , LVL_SIMP, "boards/Board.chaos"      ,0,4},
      { 9   , 0         , 0         , LVL_SIMP, "boards/Board.apocalypse" ,0,1},
      { 10  , 0         , 0         , LVL_SIMP, "boards/Board.warder"     ,0,6},
      { 11  , 0         , 0         , LVL_SIMP, "boards/Board.crusader"   ,0,7},
      { 12  , LVL_IMMORT, LVL_IMMORT, LVL_SIMP, "boards/Board.srccode"    ,0,0},
      { 13  , LVL_IMMORT, LVL_IMMORT, LVL_SIMP, "boards/Board.balance"    ,0,0}
      };

struct show_struct
   {
   char      *cmd;
   }
fields[] =
   {
      { "write" } ,
      { "look" } ,
      { "read" } ,
      { "remove" } ,
      { "list" } ,
      { "switch" } ,
      { "search" } ,
      { "check" } ,
      { "\n" }
   };

struct usage_struct
   {
   char      *option;
   }
options[] =
   {
      { "  Usage: board write <header>          *" } ,
      { "  Usage: board look                    *" } ,
      { "  Usage: board read <message number>   *" } ,
      { "  Usage: board remove <message number> *" } ,
      { "  Usage: board list                    *" } ,
      { "  Usage: board switch <board number>   *" } ,
      { "  Usage: board search <search string>  *" } ,
      { "  Usage: board check                   *" } ,
      { "\n" }
   };


struct name_struct
   {
   char      *name;
   }
names[] =
   {
      { "Social Bulletin Board" } ,
      { "Player's Bulletin Board" } ,
      { "Player's Job Bulletin Board" } ,
      { "Player To Implementor Bulletin Board  *Write Only*" } ,
      { "Quest Announcements Bulletin Board" } ,
      { "Mud Information Bulletin Board" } ,
      { "Forsaken Clan Bulletin Board" } ,
      { "Legion Clan Bulletin Board" } ,
      { "Immortal's Bulletin Board" } ,
      { "Immortal Todo Bulletin Board" } ,
      { "World Builder Bulletin Board" } ,
      { "Frozen Bulletin Board" } ,
      { "Trigger Coding Board" } ,
      { "Implementor Bulletin Board" } ,
      { "Caledon Bulletin Board" } ,
      { "Aglaron Bulletin Board" } ,
      { "Syndicate Clan Bulletin Board" } ,
      { "Underdark Bulletin Board" } ,
      { "Chosen Clan Bulletin Board" } ,
      { "Apocalypse Clan Bulletin Board" } ,
      { "Warders Guild Bulletin Board" } ,
      { "Crusaders Guild Bulletin Board" } ,
      { "Coders Bulletin Board" },
      { "World Balance Bulletin Board" },
      { "\n" }
   };

char *msg_storage[INDEX_SIZE];
int  msg_storage_taken[INDEX_SIZE];
int  num_of_msgs[NUM_OF_BOARDS];
struct board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];


int find_slot(void)
   {
   int i;

   for (i = 0; i < INDEX_SIZE; i++)
      if (!msg_storage_taken[i])
         {
         msg_storage_taken[i] = 1;
         return i;
         }

   return -1;
   }


/* search the room ch is standing in to find which board he's looking at */
int find_board(struct char_data *ch)
   {
   int i;

   for (i = 0; i < NUM_OF_BOARDS; i++)
      if (ch->player_specials->saved.board_number == i)
         return i;
   return -1;
   }


void init_boards(void)
   {
   int i, j, fatal_error = 0;

   for (i = 0; i < INDEX_SIZE; i++)
      {
      msg_storage[i] = 0;
      msg_storage_taken[i] = 0;
      }

   for (i = 0; i < NUM_OF_BOARDS; i++)
      {
      num_of_msgs[i] = 0;
      for (j = 0; j < MAX_BOARD_MESSAGES; j++)
         {
         memset(&(msg_index[i][j]), '\0', sizeof(struct board_msginfo ));
         msg_index[i][j].slot_num = -1;
         }
      Board_load_board(i);
      }

   if (fatal_error)
      {
      log("SYSERR: END Fatal Board Error");
      }
   }


ACMD(do_board)
   {
   int board_type, j, l, choice;
   static int loaded = 0;
   char *command;
   char *arg;

   if (IS_NPC(ch))
      {
      send_to_char(ch,"Mobs don't need boards!\r\n");
      return;
      }

   if (FIGHTING(ch))
      {
      send_to_char(ch,"I don't think you have time to deal with boards now!\r\n");
      return;
      }


   if (!ch->desc)
      return;
   if(ch->player_specials->saved.board_number>=NUM_OF_BOARDS)
      ch->player_specials->saved.board_number=0;

   if (!*argument)
      {
      send_to_char(ch,CCCYN(ch, C_SPR));
      send_to_char(ch,"\r\nBoard options:\r\n"
                   " **************************************************\r\n");
      for (j = 0; *(fields[j].cmd) != '\n';  j++)
         {
         send_to_char(ch," * %6.6s  %s\r\n", fields[j].cmd,options[j].option);
         }
      send_to_char(ch,
                   " **************************************************\r\n");
      send_to_char(ch,"\r\n Your current board is:\r\n  (%d) The %s\r\n",
                   ch->player_specials->saved.board_number+1,
                   names[ch->player_specials->saved.board_number].name);
      send_to_char(ch,CCNRM(ch, C_SPR));
      return;
      }

   arg=get_buffer(MAX_INPUT_LENGTH);
   command=get_buffer(MAX_INPUT_LENGTH);
   half_chop(argument, arg, command);

   for (l = 0; *(fields[l].cmd) != '\n'; l++)
      if (!strncmp(arg, fields[l].cmd, strlen(arg)))
         break;

   if (!loaded)
      {
      init_boards();
      loaded = 1;
      }

   if ((board_type = find_board(ch)) == -1)
      {
      log("BIG BIG PROBLEM WITH THE BOARD CODE!!!!");
      release_buffer(command);
      release_buffer(arg);
      return;
      }

   switch (l)
      {
   case 0:
      Board_write_message(board_type, ch, command);
      break;
   case 1:
      Board_show_board(board_type, ch, command);
      Board_save_board(board_type);
      break;
   case 2:
      Board_display_msg(board_type, ch, command);
      break;
   case 3:
      Board_remove_msg(board_type, ch, command);
      break;
   case 4:
      send_to_char(ch,CCCYN(ch, C_SPR));
      send_to_char(ch,"\r\nBoards available on Phoenix Mud:\r\n"
                   "--------------------------------\r\n");
      for (j = 0; j < NUM_OF_BOARDS;  j++)
         {
         if(can_see_board(j,ch))
            {
            if(GET_LEVEL(ch)>=LVL_ADMIN)
               {
               send_to_char(ch, "(%2d) %-60.60s %3d %3d %3d\r\n",
                            j + 1, names[j].name,READ_LVL(j),
                            WRITE_LVL(j),REMOVE_LVL(j));
               }
            else
               {
               send_to_char(ch,"(%2d) %-60.60s\r\n", j + 1, names[j].name);
               }

            }
         }
      send_to_char(ch,"\r\nNote: Not all boards are available to all characters\r\n"
                   "to be switched to. Some require that you be a certain level or\r\n"
                   "or in a specifc clan. Please see HELP BOARD for more information.\r\n"
                   "------------------------------------------------------------------\r\n");
      send_to_char(ch, "Your current board is: (%d) The %s\r\n%s",
                   ch->player_specials->saved.board_number + 1,
                   names[ch->player_specials->saved.board_number].name,
                   CCNRM(ch, C_SPR));
      break;
   case 5:
      if (is_number(command))
         {
         choice = atoi(command);
         }
      else
         {
         send_to_char(ch,"Usage: Board switch <number of the board>\r\n");
         break;
         }

      if ((choice < 1) || (choice > (NUM_OF_BOARDS + 1)))
         {
         send_to_char(ch,"Usage: Board switch <number of the board>\r\n");
         break;
         }

      choice = (choice - 1);

      if (!can_see_board(choice,ch))
         {
         send_to_char(ch,"Invalid Board Number.\r\n");
         break;
         }

      ch->player_specials->saved.board_number = choice;
      send_to_char(ch,"\r\nYour board has been switched to:\r\n");
      send_to_char(ch,  "(%d) The %s\r\n", choice + 1, names[choice].name);
      break;
   case 6:
      Board_search(ch, command);
      break;
   case 7:
      Board_check(ch);
      break;
   default:
      send_to_char(ch,"Sorry, I don't understand that.\r\n");
      send_to_char(ch,"Type: BOARD by itself to show valid options.\r\n");
      break;
      }
   release_buffer(command);
   release_buffer(arg);
   }


void Board_write_message(int board_type, struct char_data *ch, char *arg)
   {
   char *tmstr;
   int len;
   long ct;
   char *buf;
   char *buf1;

   if (GET_LEVEL(ch) < WRITE_LVL(board_type))
      {
      send_to_char(ch,"You are not holy enough to write on this board.\r\n");
      return;
      }

   if (num_of_msgs[board_type] >= MAX_BOARD_MESSAGES)
      {
      send_to_char(ch,"The board is full.\r\n");
      return;
      }

   if ((NEW_MSG_INDEX(board_type).slot_num = find_slot()) == -1)
      {
      send_to_char(ch,"The board is malfunctioning - sorry.\r\n");
      log("SYSERR: Board: failed to find empty slot on write.");
      return;
      }

   /* skip blanks */
   skip_spaces(&arg);
   if (!*arg)
      {
      send_to_char(ch,"We must have a headline!\r\n");
      return;
      }

   ct = time(0);
   tmstr = (char *) asctime(localtime(&ct));
   *(tmstr + strlen(tmstr) - 1) = '\0';
   strcpy((tmstr+11), (tmstr+20));

   buf=get_buffer(MAX_STRING_LENGTH);
   buf1=get_buffer(MAX_STRING_LENGTH);
   sprintf(buf1,"(%s)",GET_NAME(ch));
   sprintf(buf, "%6.15s %-12s :: %s", tmstr, buf1, arg);
   release_buffer(buf1);
   len = strlen(buf) + 1;

   CREATE(NEW_MSG_INDEX(board_type).heading,char,len);

   strcpy(NEW_MSG_INDEX(board_type).heading, buf);
   release_buffer(buf);
   NEW_MSG_INDEX(board_type).heading[len-1] = '\0';
   NEW_MSG_INDEX(board_type).date_posted = time(NULL);
   NEW_MSG_INDEX(board_type).level = GET_LEVEL(ch);
   send_to_char(ch, "\r\nThe %s\r\n", names[board_type].name);
   send_to_char(ch,"----------------------------------------\r\n"
                "Write your message (/s saves /h for help).\r\n\r\n");

   if (!IS_NPC(ch))
      SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

   ch->desc->str = &(msg_storage[NEW_MSG_INDEX(board_type).slot_num]);
   ch->desc->max_str = MAX_MESSAGE_LENGTH;
   /*
    * begin add - Bon 07/12/97 
    */
   ch->desc->mail_to = board_type + BOARD_MAGIC;
   /*
    * end   add - Bon 07/12/97 
    */

   num_of_msgs[board_type]++;
   }


void Board_show_board(int board_type, struct char_data *ch, char *arg)
   {
   int i;
   char *buf;
   if (!ch->desc)
      return;

   if (GET_LEVEL(ch) < READ_LVL(board_type))
      {
      send_to_char(ch,"You try but fail to understand the holy words.\r\n");
      return;
      }

   send_to_char(ch, "\r\nThe %s\r\n", names[board_type].name);
   send_to_char(ch,"-----------------------------------\r\n");

   buf=get_buffer(MAX_STRING_LENGTH);
   strcpy(buf,"\r\n");
   if (!num_of_msgs[board_type])
      strcat(buf, "The board is empty.\r\n");
   else
      {
      sprintf(buf + strlen(buf), "There %s %d %s on the board.\r\n\r\n",
              (num_of_msgs[board_type] > 1) ? "are" : "is", num_of_msgs[board_type], (num_of_msgs[board_type] > 1) ? "messages" : "message");
      for (i = 0; i < num_of_msgs[board_type]; i++)
         {
          char *header = msg_index[board_type][i].heading;
          char buf2[2048];
          time_t now = time(NULL);
          time_t date_posted = msg_index[board_type][i].date_posted;
          if (now - date_posted < 60*60*24) {
            sprintf(buf2, "&M%s&n", header);
          } else if (now - date_posted < 60*60*24*3) {
            sprintf(buf2, "&Y%s&n", header);
          } else if (now - date_posted < 60*60*24*7) {
            sprintf(buf2, "&C%s&n", header);
          } else {
            strcpy(buf2, header);
          }
          sprintf(buf + strlen(buf), "%-2d : %s\r\n", i + 1, buf2);
         }
      }
   page_string(ch->desc, buf, TRUE,"");
   release_buffer(buf);
   return;
   }


void Board_display_msg(int board_type, struct char_data *ch, char *arg)
   {
   char *msg_number=get_buffer(MAX_STRING_LENGTH);
   char *buf;
   int msg, ind;

   one_argument(arg, msg_number);
   if (!*msg_number || !isdigit((int)*msg_number))
      {
      release_buffer(msg_number);
      return;
      }
   if (!(msg = atoi(msg_number)))
      {
      release_buffer(msg_number);
      return;
      }
   release_buffer(msg_number);

   if (GET_LEVEL(ch) < READ_LVL(board_type))
      {
      send_to_char(ch,"You try but fail to understand the holy words.\r\n");
      return;
      }

   if (!num_of_msgs[board_type])
      {
      send_to_char(ch,"The board is empty!\r\n");
      return;
      }
   if (msg < 1 || msg > num_of_msgs[board_type])
      {
      send_to_char(ch,"That message number exists only in your imagination..\r\n");
      return;
      }

   ind = msg - 1;
   if (MSG_SLOTNUM(board_type, ind) < 0 ||
           MSG_SLOTNUM(board_type, ind) >= INDEX_SIZE)
      {
      send_to_char(ch,"Sorry, the board is not working.\r\n");
      log("SYSERR: Board is screwed up.");
      return;
      }

   if (!(MSG_HEADING(board_type, ind)))
      {
      send_to_char(ch,"That message appears to be screwed up.\r\n");
      return;
      }

   if (!(msg_storage[MSG_SLOTNUM(board_type, ind)]))
      {
      send_to_char(ch,"That message seems to be empty.\r\n");
      return;
      }

   send_to_char(ch,"\r\nMessage written on the %s\r\n",names[board_type].name);

   buf=get_buffer(MAX_STRING_LENGTH);
   sprintf(buf, "Message %d : %s\r\n\r\n%s\r\n", msg,
           MSG_HEADING(board_type, ind),
           msg_storage[MSG_SLOTNUM(board_type, ind)]);

   page_string(ch->desc, buf, TRUE,"");
   release_buffer(buf);
   return;
   }


void Board_remove_msg(int board_type, struct char_data *ch, char *arg)
   {
   int ind, msg, slot_num;
   char *msg_number=get_buffer(MAX_INPUT_LENGTH);
   char *buf;
   struct descriptor_data *d;

   one_argument(arg, msg_number);

   if (!*msg_number || !isdigit((int)*msg_number))
      {
      release_buffer(msg_number);
      return;
      }
   if (!(msg = atoi(msg_number)))
      {
      release_buffer(msg_number);
      return;
      }
   release_buffer(msg_number);

   if (!num_of_msgs[board_type])
      {
      send_to_char(ch, "\r\nThere are no messages on the %s\r\n",
                   names[board_type].name);
      return;
      }

   if (msg < 1 || msg > num_of_msgs[board_type])
      {
      send_to_char(ch,"That message number exists only in your imagination..\r\n");
      return;
      }

   ind = msg - 1;
   if (!MSG_HEADING(board_type, ind))
      {
      send_to_char(ch,"That message appears to be screwed up.\r\n");
      return;
      }
   buf=get_buffer(MAX_INPUT_LENGTH);
   sprintf(buf, "(%s)", GET_NAME(ch));
   if((BOARD_CLAN(board_type)==GET_CLAN(ch))&&(GET_LEADER(ch)==1))
      {
      send_to_char(ch,"Since you are your clan's leader you can remove messages.\r\n");
      }
   else if (GET_LEVEL(ch) < REMOVE_LVL(board_type) &&
            !(strstr(MSG_HEADING(board_type, ind), buf)))
      {
      send_to_char(ch,"You are not holy enough to remove other people's messages.\r\n");
      release_buffer(buf);
      return;
      }
   else if (GET_LEVEL(ch) < MSG_LEVEL(board_type, ind)&&
            GET_LEVEL(ch) >=REMOVE_LVL(board_type))
      {
      send_to_char(ch,"You can't remove a message holier than yourself.\r\n");
      release_buffer(buf);
      return;
      }

   slot_num = MSG_SLOTNUM(board_type, ind);
   if (slot_num < 0 || slot_num >= INDEX_SIZE)
      {
      log("SYSERR: The board is seriously screwed up.");
      send_to_char(ch,"That message is majorly screwed up.\r\n");
      release_buffer(buf);
      return;
      }

   for (d = descriptor_list; d; d = d->next)
      if (STATE(d)==CON_PLAYING && d->str == &(msg_storage[slot_num]))
         {
         send_to_char(ch,"At least wait until the author is finished before removing it!\r\n");
         release_buffer(buf);
         return;
         }

   if (msg_storage[slot_num])
      free(msg_storage[slot_num]);
   msg_storage[slot_num] = 0;
   msg_storage_taken[slot_num] = 0;
   if (MSG_HEADING(board_type, ind))
      free(MSG_HEADING(board_type, ind));

   for (; ind < num_of_msgs[board_type] - 1; ind++)
      {
      MSG_HEADING(board_type, ind) = MSG_HEADING(board_type, ind + 1);
      MSG_SLOTNUM(board_type, ind) = MSG_SLOTNUM(board_type, ind + 1);
      MSG_LEVEL(board_type, ind) = MSG_LEVEL(board_type, ind + 1);
      }
   num_of_msgs[board_type]--;

   buf[0] = '\0';

   send_to_char(ch, "\r\nMessage removed from the %s\r\n",
                names[board_type].name);

   Board_save_board(board_type);

   release_buffer(buf);
   return;
   }


void Board_save_board(int board_type)
   {
   FILE * fl;
   int i;
   char *tmp1 = 0, *tmp2 = 0;

   if (!num_of_msgs[board_type])
      {
      unlink(FILENAME(board_type));
      return;
      }

   if (!(fl = fopen(FILENAME(board_type), "wb")))
      {
      perror("SYSERR: Error writing board");
      return;
      }

   fwrite(&(num_of_msgs[board_type]), sizeof(int), 1, fl);

   for (i = 0; i < num_of_msgs[board_type]; i++)
      {
      if ((tmp1 = MSG_HEADING(board_type, i)))
         msg_index[board_type][i].heading_len = strlen(tmp1) + 1;
      else
         msg_index[board_type][i].heading_len = 0;

      if (MSG_SLOTNUM(board_type, i) < 0 ||
              MSG_SLOTNUM(board_type, i) >= INDEX_SIZE ||
              (!(tmp2 = msg_storage[MSG_SLOTNUM(board_type, i)])))
         msg_index[board_type][i].message_len = 0;
      else
         msg_index[board_type][i].message_len = strlen(tmp2) + 1;

      fwrite(&(msg_index[board_type][i]), sizeof(struct board_msginfo ), 1,fl);
      if (tmp1)
         fwrite(tmp1, sizeof(char), msg_index[board_type][i].heading_len, fl);
      if (tmp2)
         fwrite(tmp2, sizeof(char), msg_index[board_type][i].message_len, fl);
      }

   fclose(fl);
   }


void Board_load_board(int board_type)
   {
   FILE * fl;
   int i, len1 = 0, len2 = 0;
   char *tmp1 = 0, *tmp2 = 0;


   if (!(fl = fopen(FILENAME(board_type), "rb")))
      {
      char *emsg = get_buffer(SMALL_BUFSIZE);
      sprintf(emsg, "SYSERR: Error reading %s", FILENAME(board_type));
      perror(emsg);
      release_buffer(emsg);
      return;
      }

   fread(&(num_of_msgs[board_type]), sizeof(int), 1, fl);
   if (num_of_msgs[board_type]<1||num_of_msgs[board_type]>MAX_BOARD_MESSAGES)
      {
      log("SYSERR: Board file corrupt.  Resetting.");
      Board_reset_board(board_type);
      return;
      }

   for (i = 0; i < num_of_msgs[board_type]; i++)
      {
      fread(&(msg_index[board_type][i]), sizeof(struct board_msginfo ), 1, fl);
      if (!(len1 = msg_index[board_type][i].heading_len))
         {
         log("SYSERR: Board file corrupt!  Resetting.");
         Board_reset_board(board_type);
         return;
         }

      CREATE(tmp1,char,len1);

      fread(tmp1, sizeof(char), len1, fl);
      MSG_HEADING(board_type, i) = tmp1;

      if ((len2 = msg_index[board_type][i].message_len))
         {
         if ((MSG_SLOTNUM(board_type, i) = find_slot()) == -1)
            {
            log("SYSERR: Out of slots booting board!  Resetting..");
            Board_reset_board(board_type);
            return;
            }
         CREATE(tmp2,char,len2);

         fread(tmp2, sizeof(char), len2, fl);
         msg_storage[MSG_SLOTNUM(board_type, i)] = tmp2;
         }
      }

   fclose(fl);
   }


void Board_reset_board(int board_type)
   {
   int i;

   for (i = 0; i < MAX_BOARD_MESSAGES; i++)
      {
      if (MSG_HEADING(board_type, i))
         free (MSG_HEADING(board_type, i));
      if (msg_storage[MSG_SLOTNUM(board_type, i)])
         free (msg_storage[MSG_SLOTNUM(board_type, i)]);
      msg_storage_taken[MSG_SLOTNUM(board_type, i)] = 0;
      memset(&(msg_index[board_type][i]), '\0', sizeof(struct board_msginfo ));
      msg_index[board_type][i].slot_num = -1;
      }
   num_of_msgs[board_type] = 0;
   unlink(FILENAME(board_type));
   }

void Board_store_lastcheck(struct char_data *ch)
   {


   }

void Board_check(struct char_data *ch)
   {
   int j = 0;
   char *buf=get_buffer(MAX_STRING_LENGTH);
   send_to_char(ch, "\r\nThe latest board items.\r\n");
   send_to_char(ch,"-----------------------------------\r\n");

   for(j=0;j<NUM_OF_BOARDS;j++)
   {
      if(can_see_board(j,ch)&&GET_LEVEL(ch)>=READ_LVL(j))
      {
         if(num_of_msgs[j]==0)
         {
            sprintf(buf+strlen(buf), "%-2d -- No Messages.\r\n",j+1);
         }
         else
         {
           char *header = msg_index[j][num_of_msgs[j]-1].heading;
           if (!header) {
             log("SYSERR: STATUS-NonFatal Type-board DESC- board code failed at MSG_HEADING(board_type)\r\n");
             sprintf(buf+strlen(buf),"%-2d -- Board out of order.  Please contact your service representative.\r\n",j+1);
             return;
           }
           char buf2[2048];
           time_t now = time(NULL);
           time_t date_posted = msg_index[j][num_of_msgs[j]-1].date_posted;
           if (now - date_posted < 60*60*24) {
             sprintf(buf2, "&M%s&n", header);
           } else if (now - date_posted < 60*60*24*3) {
             sprintf(buf2, "&Y%s&n", header);
           } else if (now - date_posted < 60*60*24*7) {
             sprintf(buf2, "&C%s&n", header);
           } else {
             strcpy(buf2, header);
           }
           sprintf(buf + strlen(buf), "%-2d -- %-3d %s\r\n", j + 1, num_of_msgs[j], buf2);
        }
      }
   }
   page_string(ch->desc, buf, TRUE,"");
   release_buffer(buf);
   return;
   }

int can_see_board(int board_num,struct char_data *ch)
   {
   int see_test = FALSE;
   if((GET_LEVEL(ch)>=READ_LVL(board_num))||(GET_LEVEL(ch)>=WRITE_LVL(board_num)))
      {
      if((BOARD_CLAN(board_num)==0))
         {
         see_test = TRUE;
         }
      else if(GET_LEVEL(ch)>=LVL_ADMIN)
         {
         see_test = TRUE;
         }
      else if(BOARD_CLAN(board_num)==GET_CLAN(ch))
         {
         see_test = TRUE;
         }
      }
   return see_test;
   }

/* Added board search function - Nomikos 10/25/2025 */
void Board_search(struct char_data *ch, char *arg)
   {
   int j, k;
   char *buf;

   if (!arg || !*arg)
      {
      send_to_char(ch, "Usage: board search <search string>\r\n");
      return;
      }

   buf = get_buffer(MAX_STRING_LENGTH);

   send_to_char(ch, "\r\nMessages matching '%s'.\r\n", arg);
   send_to_char(ch,"-----------------------------------\r\n");

   for(j = 0; j < NUM_OF_BOARDS; j++)
      {
      if(can_see_board(j, ch) && GET_LEVEL(ch) >= READ_LVL(j))
         {
         if(num_of_msgs[j] > 0)
            {
            for (k = 0; k < num_of_msgs[j]; k++)
               {
               char *header = msg_index[j][k].heading;
               if (!header) 
                  {
                  log("SYSERR: STATUS-NonFatal Type-board DESC- board code failed at MSG_HEADING(board_type)\r\n");
                  release_buffer(buf);
                  return;
                  }
               if (str_str(header, arg) || str_str(msg_storage[MSG_SLOTNUM(j, k)], arg))
                  sprintf(buf + strlen(buf), "%-2d -- %-3d %s\r\n", j + 1, k + 1, header);
               }
            }
         }
      }
   page_string(ch->desc, buf, TRUE,"");
   release_buffer(buf);
   return;
   }
