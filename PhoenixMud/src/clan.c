/***************************************************************************
 *  File:  clan.c                                       Part of PhoenixMUD *
 *  Usage: Clan stuff                                                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        * 
 *  Written by War                                                         *
 *  Modified by Angus Mezick Feb 98                                        *
 *                                                                         *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
 *  PhoenixMUD is based on CircleMUD, Copyright (C) 1996-98.               *
 ***************************************************************************/

#define __CLAN_C__

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
#include "clan.h"
#include "constants.h"

/*   external vars  */
extern FILE *player_fl;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern int    exp_table[];
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_p_table;

/* for chars */
extern struct spell_info_type *spells;
extern char *pc_class_types[];

struct clan_data *clan_list;

void init_clan_list(void)
   {
   char *string=get_buffer(1024);
   char *send=get_buffer(256);
   /* char report[256]; */
   struct clan_data *cur,*new,*prev;
   short cl_room;
   long cl_piece;
   int count=0, cnumber, ver=1;
   int cl_banks=TRUE;
   FILE *clan_fl;

   if (!(clan_fl=fopen(CLAN_FILE,"r")))
      {
      perror("Error opening clan file");
      exit(1);
      }
   rewind (clan_fl);
   clan_list=NULL;

   while (fgets(string,1024,clan_fl) != NULL)
      {

      if (string[0] == ' ' || string[0] == '\n')
         {
         fclose(clan_fl);
         release_buffer(send);
         release_buffer(string);
         return;
         }

      cl_banks=TRUE;
      if (string[0]=='V')
         {
         ver = atoi(string+1);
         fgets(string,1024,clan_fl);
         }

      sprintf (send,"length = %d",strlen(string));
      CREATE(new,struct clan_data,1);
      string[strlen(string)-1]='\0';
      cnumber=atoi(string);
      new->cl_number=cnumber;

      fgets(string,1024,clan_fl);
      string[strlen(string)-1]='\0';
      cl_room=atoi(string);
      new->cl_room = cl_room;

      new->cl_piece = 0;
      if (ver == 2)
         {
         fgets(string,1024,clan_fl);
         string[strlen(string)-1]='\0';
         cl_piece=atol(string);
         new->cl_piece = cl_piece;
         }

      fgets(string,1024,clan_fl);
      string[strlen(string)-1]='\0';
      CREATE (new->cl_name,char,strlen(string)+1);
      strcpy (new->cl_name,string);
      if(cl_banks==TRUE)
         {
         fgets(string,1024,clan_fl);
         new->cl_bank=atoi(string);
         }
      else
         {
         new->cl_bank=0;
         }
      while (fgets(string,1024,clan_fl) != NULL)
         {
         if (string[0] == '\n')
            break;
         string[strlen(string)-1]='\0';
         CREATE (new->cl_players[count],char,strlen(string)+1);
         strcpy (new->cl_players[count++],string);
         }
      new->cl_players[count]=NULL;
      count=0;
      prev=NULL;
      cur = clan_list;

      while (cur!=NULL)
         {
         prev=cur;
         cur = cur->next;
         }
      if (prev == NULL)
         {
         clan_list = new;
         new->next = NULL;
         }
      else
         {
         prev->next = new;
         new->next = NULL;
         }

      }

   release_buffer(send);
   release_buffer(string);
   }

void init_clan_vari (struct descriptor_data *d)
   {
   struct clan_data *cur;
   int count =0;
   cur = clan_list;
   GET_CLAN(d->character) =0;
   GET_LEADER(d->character) =0;
   GET_CLAN_ROOM(d->character)=0;
   while (cur != NULL)
      {
      while (cur->cl_players[count] != NULL)
         {
         if (str_cmp (GET_NAME(d->character),cur->cl_players[count++]) ==0)
            {
            strcpy (GET_CLAN_NAME(d->character),cur->cl_name);
            if (count==1)
               GET_LEADER(d->character) =1;
            GET_CLAN(d->character) = cur->cl_number;
            GET_CLAN_ROOM(d->character)= cur->cl_room;
            return;
            }
         }
      count=0;
      cur=cur->next;
      }
   }

void write_clan_file (void)
   {
   FILE *clan_fl;
   int count2=0;
   struct clan_data *cur;

   if (!(clan_fl=fopen(CLAN_FILE,"w")))
      {
      perror("Error opening clan file");
      exit(1);
      }

   cur = clan_list;
   fprintf(clan_fl,"V%d\n", CUR_CLAN_VER);
   while (cur != NULL)
      {
      fprintf (clan_fl,"%d\n",cur->cl_number);
      fprintf (clan_fl,"%d\n",cur->cl_room);
      fprintf (clan_fl,"%ld\n",cur->cl_piece);
      fprintf (clan_fl,"%s\n",cur->cl_name);
      fprintf (clan_fl,"%ld\n",cur->cl_bank);
      while (cur->cl_players[count2] != NULL)
         {
         fprintf (clan_fl,"%s\n",cur->cl_players[count2++]);
         }
      fprintf (clan_fl,"\n");
      cur=cur->next;
      count2=0;
      }
   fclose (clan_fl);
   }

ACMD (do_makeclan)
   {
   char *string_send=get_buffer(1024);
   char *temp_room=get_buffer(256);
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   struct clan_data *cur,*prev,*new;
   struct char_data *victim;
   int count =1;
   room_vnum cl_room;

   half_chop (argument,buf,buf2);
   half_chop (buf2,temp_room,buf2);
   if (!(is_number(temp_room)))
      {
      send_to_char(ch,
                   "format: makeclan <who> <clan_room> <clan_name>\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      release_buffer(temp_room);
      release_buffer(string_send);
      return;
      }
   cl_room=atoi(temp_room);
   if (!(victim=get_char_vis(ch,buf,FIND_CHAR_WORLD)))
      {
      send_to_char(ch,"Who is that?\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      release_buffer(temp_room);
      release_buffer(string_send);
      return;
      }

   /* 10/21/96, Echo - changed following test to >= from > */
   /* 10/07/99, Manwe - removed limitation on Imms running clans */
   /* if (GET_LEVEL(victim) >= LVL_IMMORT)
       {
       send_to_char(ch,"Immortals can't run clans.\r\n");
       release_buffer(buf2);
       release_buffer(buf);
       release_buffer(temp_room);
       release_buffer(string_send);
       return;
       }
   */
   if (GET_CLAN(victim) !=0)
      {
      send_to_char(ch,"They're already in a clan.\r\n");
      release_buffer(buf2);
      release_buffer(buf);
      release_buffer(temp_room);
      release_buffer(string_send);
      return;
      }
   if (GET_GOLD(victim) < COST_CREATE)
      {
      send_to_char(ch, "This player doesn't have %d coins on hand.\r\n",
                   COST_CREATE);
      send_to_char(victim, "You need %d coins to sacrifice to the gods.\r\n",
                   COST_CREATE);
      release_buffer(buf2);
      release_buffer(buf);
      release_buffer(temp_room);
      release_buffer(string_send);
      return;
      }
   CREATE(new,struct clan_data,1);
   CREATE(new->cl_name,char,strlen(buf2)+1);
   strcpy(new->cl_name,buf2);
   CREATE(new->cl_players[0],char,strlen(GET_NAME(victim))+1);
   strcpy(new->cl_players[0],GET_NAME(victim));
   new->cl_room=cl_room;
   new->cl_piece=0;
   new->cl_bank=0;
   new->cl_players[1] = NULL;
   prev = NULL;
   cur = clan_list;
   while (1)
      {
      if (cur==NULL)
         break;
      if (cur->cl_number > count)
         break;
      log ("%s", cur->cl_name);
      prev=cur;
      cur = cur->next;
      count++;
      }
   new->cl_number = count;
   GET_CLAN_ROOM(victim)=cl_room;
   GET_CLAN(victim) = count;
   GET_LEADER(victim) =1;
   strcpy (GET_CLAN_NAME(victim),new->cl_name);
   if (prev == NULL)  /*beginning of list*/
      {
      clan_list = new;
      new->next = cur;
      }
   else
      {
      prev->next = new; /* somewhere in the middle or the end*/
      new->next = cur;
      }
   send_to_char(ch, "%s is now the leader of %s!\r\n",new->cl_players[0],
                new->cl_name);
   send_to_char(victim, "You're now the leader of %s!\r\n",new->cl_name);
   write_clan_file();
   GET_GOLD(victim) -= COST_CREATE;
   release_buffer(buf2);
   release_buffer(buf);
   release_buffer(temp_room);
   release_buffer(string_send);
   }

ACMD(do_listclans)
   {
   struct clan_data *cur;
   int i;

   cur=clan_list;
   send_to_char(ch, "Clans now present on Phoenix MUD...\r\n");
   if (GET_LEVEL(ch) < LVL_DGOD)
      {
      send_to_char(ch,
              "----------------------------------------------------\r\n");
      send_to_char(ch,
              "| Clan Name       |  Clan Leader  |     Members    |\r\n");
      send_to_char(ch,
              "----------------------------------------------------\r\n");                                
      while (cur != NULL)
         {
         for (i=1; cur->cl_players[i] !=NULL; i++)
            ;
         send_to_char(ch, "| %-16.16s| %-14.14s| %8d       |\r\n",
                      cur->cl_name, cur->cl_players[0], i-1);
         send_to_char(ch,   
                 "----------------------------------------------------\r\n");
         cur=cur->next;
         }
      }
   else
      {
      send_to_char(ch,
              "---------------------------------------------------------------------------------\r\n");
      send_to_char(ch,
              "| Clan Leader  |   Clan Name   | Clan room | Clan Number | Clan Bank  | Piece   |\r\n");
      send_to_char(ch,
              "---------------------------------------------------------------------------------\r\n");
      while (cur != NULL)
         {
         send_to_char(ch, "| %-13.13s| %-14.14s| %7d   | %10d  | %10ld | %7ld |\r\n",
                      cur->cl_players[0],cur->cl_name,cur->cl_room,cur->cl_number,
                      cur->cl_bank, cur->cl_piece);
         send_to_char(ch,
              "---------------------------------------------------------------------------------\r\n");
         cur=cur->next;
         }
      }
   }

ACMD (do_listmembers)
   {
   char *clan_name=get_buffer(256);
   struct clan_data *cur;
   int count=0,i=0;
   int counter = 1;
   cur=clan_list;

   if(GET_LEVEL(ch)<LVL_DGOD)
      {
      if(GET_CLAN(ch)<=0)
         send_to_char(ch,"But you aren't in a clan!\r\n");
      else
         strcpy(clan_name,GET_CLAN_NAME(ch));
      }
   else
      strcpy(clan_name,argument);

   for (i=0;*(clan_name+i)==' ';i++)
      ;
   while (cur != NULL)
      {
      if (!str_cmp(clan_name+i, cur->cl_name) || isname(clan_name+i, cur->cl_name))
         {
         send_to_char(ch,"\r\n");
         send_to_char(ch, "Clan Name:   %s\r\n",cur->cl_name);
         send_to_char(ch, "-------------------------------\r\n");
         send_to_char(ch, "Clan Room:   %d\r\n",cur->cl_room);
         send_to_char(ch, "Clan leader: %s\r\n",cur->cl_players[count++]);
         while (cur->cl_players[count] !=NULL)
            {
            send_to_char(ch, "Member(%d):  %s \r\n",counter,
                         cur->cl_players[count++]);
            counter = counter +1;
            }
         release_buffer(clan_name);
         return;
         }
      cur=cur->next;
      }
   send_to_char(ch, "There is no such clan with that name.\r\n");
   release_buffer(clan_name);
   }

ACMD(do_clanleader)
   {
   struct clan_data *cur;
   struct clan_data *select_cl;
   struct char_data *new_leader;
   struct char_data *old_leader;
   int count=1;
   int clan_num=0;
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *buf2=get_buffer(MAX_STRING_LENGTH);

   cur=clan_list;
   select_cl=clan_list;
   half_chop(argument,buf2,buf);

   if(!buf||(*buf=='\0'))
      {
      send_to_char(ch,"Format: clanleader <new leader> <clan name>\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
      }
   while(select_cl!=NULL)
      {
      if (!(!str_cmp(buf, select_cl->cl_name) || isname(buf, select_cl->cl_name)))
         {
         select_cl=select_cl->next;
         }
      else
         {
         clan_num=select_cl->cl_number;
         break;
         }
      }
   release_buffer(buf);

   if(clan_num==0)
      {
      send_to_char(ch,"Not a valid clan!\r\n");
      release_buffer(buf2);
      return;
      }

   new_leader = get_char_vis(ch,buf2,FIND_CHAR_WORLD);
       
   if (!new_leader)
      {   
      send_to_char(ch,"The new leader must be online.\r\n");
      release_buffer(buf2);
      return;
      }

   if (GET_CLAN(new_leader)!=clan_num)
      {
      send_to_char(ch,"The new clan leader must be in the same clan!\r\n");
      release_buffer(buf2);
      return;
      }

   while (cur!= NULL)
      {
      if (clan_num==cur->cl_number)
         {
         old_leader = get_char_vis(ch,cur->cl_players[0],FIND_CHAR_WORLD);
         if (!old_leader)
            {
            send_to_char(ch,"The old clan leader must be online.\r\n");
            release_buffer(buf2);
            return;
            }

         if (new_leader == old_leader)
            {
            send_to_char(ch,"They are already the clan leader.\r\n");
            release_buffer(buf2);
            return;
            }

         /* first remove the new leader */
         while (1)
            {
            if(cur->cl_players[count] == NULL)
               {
               send_to_char(ch,"That player is not in the clan.\r\n");
               release_buffer(buf2);
               return;
               }
            if (str_cmp (buf2,cur->cl_players[count])==0)
               {
               while (cur->cl_players[count] != NULL)
                  {
                  if (cur->cl_players[count+1]!=NULL)
                     strcpy(cur->cl_players[count],cur->cl_players[count+1]);
                  count++;
                  }
               break;
               }
            count++;
            }
         /* strip the current leader of status */
         GET_LEADER(old_leader) = 0;
         /* now shift the everyone 1 space down */
         count--;
         while (count > 0)
            {
            strcpy(cur->cl_players[count],cur->cl_players[count-1]);
            count--;
            }
         /* now put the new leader on the seat */
         strcpy (cur->cl_players[0],GET_NAME(new_leader));
         GET_LEADER(new_leader) = 1;
         send_to_char(ch, "%s is now the leader of %s.\r\n", 
		      GET_NAME(new_leader), cur->cl_name);
         send_to_char(new_leader, "Congratulations!  You are the new "
                      "leader of %s!\r\n",cur->cl_name);
         send_to_char(old_leader, "Your leadership of %s has been passed "
                      "on to %s.\r\n",cur->cl_name,GET_NAME(new_leader));
         mudlogf(CMP,GET_INVIS_LEV(ch),TRUE,"CLAN: %s has made %s the "
                 "new clan leader of %s.",GET_NAME(ch), 
                 GET_NAME(new_leader), cur->cl_name);
         write_clan_file();
         release_buffer(buf2);
         return;
         }
      cur=cur->next;
      }
   release_buffer(buf2);
   }


ACMD (do_addmember)
   {
   int count=0;
   struct clan_data *cur;
   struct clan_data *select_cl;
   struct char_data *victim;
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   int clan_num=0;
/*   struct obj_data *obj;
     mob_rnum r_num;*/

   cur=clan_list;
   select_cl=clan_list;
   half_chop(argument,buf,buf2);

   if ((GET_LEADER(ch) == 0)&&(GET_LEVEL(ch)<LVL_ADMIN))
      {
      send_to_char(ch,"You are not a clan leader!! GO Away!!\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
      }
   if (!(victim = get_char_vis(ch,buf,FIND_CHAR_WORLD)))
      {
      send_to_char(ch,"Who is that? They must be in the same room as you.\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
      }
   if(IS_NPC(victim))
      {
      send_to_char(ch,"Go away, you bother me.\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
      }
   if (GET_LEVEL(victim) < 30)
      {
      send_to_char(ch,"This player is too inexperienced to be in a clan.\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
      }
   if ((victim != ch) && (GET_LEVEL(victim) >= LVL_IMMORT))
      {
      send_to_char(ch,"You can't add immortals to your clan\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
      }

   clan_num=GET_CLAN(ch);

   /* if a god, find what clan is being added to */
   if(GET_LEVEL(ch)>=LVL_ADMIN)
      {
      if(!buf2||(*buf2=='\0'))
         {
         send_to_char(ch,"Format: addmember <name> <clan name>\r\n");
         release_buffer(buf);
         release_buffer(buf2);
         return;
         }
      while(select_cl!=NULL)
         {
         if (!(!str_cmp(buf2, select_cl->cl_name) || isname(buf2, select_cl->cl_name)))
            {
            select_cl=select_cl->next;
            }
         else
            {
            clan_num=select_cl->cl_number;
            break;
            }
         }
      }
   release_buffer(buf2);

   if(clan_num==0)
      {
      send_to_char(ch,"Not a valid clan!\r\n");
      release_buffer(buf);
      return;
      }

   if ((GET_CLAN(victim) != 0) && (GET_CLAN(victim) != clan_num))
      {
      send_to_char(ch,"They already belong to another clan.\r\n");
      release_buffer(buf);
      return;
      }
   if (GET_CLAN(victim) == clan_num)
      {
      send_to_char(ch,"They're already in your clan!!\r\n");
      release_buffer(buf);
      return;
      }
   /* 10/21/96, Echo - changed following inequality to be ">=" */
   /*   if (GET_LEVEL(victim) >= LVL_IMMORT)
         {
         send_to_char(ch,"Immortals can't be in clans!!\r\n");
         release_buffer(buf);
         return;
         }*/
   if (GET_GOLD(victim) < COST_JOIN)
      {
      send_to_char(ch, "Your new member needs %d coins on hand to join.\r\n",
                   COST_JOIN);
      send_to_char(ch,
                   "You need %d coins on hand to sacrifice to the gods.\r\n",
                   COST_JOIN);
      release_buffer(buf);
      return;
      }
   GET_GOLD(victim) -= COST_JOIN;

   while (cur != NULL)
      {
      if (clan_num==cur->cl_number)
         {
         while (1)
            {
            if (cur->cl_players[count]==NULL)
               {
               CREATE (cur->cl_players[count],char,strlen(buf)+1);
               strcpy (cur->cl_players[count],GET_NAME(victim));
               send_to_char(ch,"%s is now in your clan!\r\n",GET_NAME(victim));
               send_to_char(ch, "%s has made %s a member of %s",GET_NAME(ch),
                            GET_NAME(victim), cur->cl_name);
               mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                       "CLAN: %s has made %s a member of %s",GET_NAME(ch),
                       GET_NAME(victim), cur->cl_name);
               send_to_char(victim, "You now belong to %s.\r\n",cur->cl_name);
               GET_CLAN(victim) = clan_num;
               GET_CLAN_ROOM(victim) = cur->cl_room;
               strcpy (GET_CLAN_NAME(victim),cur->cl_name);
               if (cur->cl_piece <= 0)
                  send_to_char(ch,"Have a god set your clan piece, please.\r\n");
               else
                  {
                  if (GET_EQ(victim, WEAR_HEART))
                     extract_obj(unequip_char(victim, WEAR_HEART));
                  equip_char(victim, read_object(real_object(cur->cl_piece), REAL), WEAR_HEART);
                  }
               write_clan_file();
               release_buffer(buf);
               return;
               }
            count++;
            }
         }
      cur=cur->next;
      }
   release_buffer(buf);
   }

ACMD (do_removemember)
   {
   struct clan_data *cur;
   struct clan_data *select_cl;
   struct char_data *victim;
   int count=1;
   int clan_num=0;
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *buf2=get_buffer(MAX_STRING_LENGTH);

   cur=clan_list;
   select_cl=clan_list;
   half_chop(argument,buf,buf2);
   if ((GET_LEADER(ch) == 0)&&(GET_LEVEL(ch)<LVL_ADMIN))
      {
      send_to_char(ch,"You are not a clan leader!! Go Away!!\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
      }

   victim = get_char_vis(ch,buf,FIND_CHAR_WORLD);

   if (victim && !str_cmp(GET_NAME(ch),GET_NAME(victim)) &&
       (GET_LEVEL(ch) < LVL_ADMIN))
      {
      send_to_char(ch,"I don't think so....\r\n");
      release_buffer(buf);
      release_buffer(buf2);
      return;
      }

   clan_num=GET_CLAN(ch);

   /* if a god, find what clan is being added to */
   if(GET_LEVEL(ch)>=LVL_ADMIN)
      {
      if(!buf2||(*buf2=='\0'))
         {
         send_to_char(ch,"Format: delmember <name> <clan name>\r\n");
         release_buffer(buf);
         release_buffer(buf2);
         return;
         }
      while(select_cl!=NULL)
         {
         if (!(!str_cmp(buf2, select_cl->cl_name) || isname(buf2, select_cl->cl_name)))
            {
            select_cl=select_cl->next;
            }
         else
            {
            clan_num=select_cl->cl_number;
            break;
            }
         }
      }
   release_buffer(buf2);

   if(clan_num==0)
      {
      send_to_char(ch,"Not a valid clan!\r\n");
      release_buffer(buf);
      return;
      }

   while (cur!= NULL)
      {
      if (clan_num==cur->cl_number)
         {
         while (1)
            {
            if(cur->cl_players[count] == NULL)
               {
               send_to_char(ch,"That player is not in the clan.\r\n");
               release_buffer(buf);
               return;
               }
            if (str_cmp (buf,cur->cl_players[count])==0)
               {
               while (cur->cl_players[count] != NULL)
                  {
                  if (cur->cl_players[count+1]!=NULL)
                     strcpy (cur->cl_players[count],cur->cl_players[count+1]);
                  count++;
                  }
               free(cur->cl_players[count-1]);
               cur->cl_players[count-1] = NULL;
               if(victim)
                  {
                  GET_CLAN(victim)=0;
                  GET_CLAN_ROOM(victim)=0;
                  victim->player_specials->saved.board_number = 0;
                  strcpy(GET_CLAN_NAME(victim),"");
                  if (GET_EQ(victim, WEAR_HEART))
                     extract_obj(unequip_char(victim, WEAR_HEART));
                  send_to_char(victim, "You are no longer a member of %s.\r\n",
                               GET_CLAN_NAME(ch));
                  }
               mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                       "CLAN: %s has removed %s from the clan %s",GET_NAME(ch),
                       buf,cur->cl_name);
               send_to_char(ch, "%s is no longer a member of your clan.\r\n",
                            buf);
               write_clan_file();
               release_buffer(buf);
               return;
               }
            count++;
            }
         }
      cur=cur->next;
      }
   release_buffer(buf);
   }

ACMD (do_csay)
   {
   struct descriptor_data *i,*d;
   struct char_data *vict;
   char *buf1;
   int any=0;
   int emote=FALSE; 
   struct clan_data *select_cl;

   if ((GET_CLAN(ch)==0)&&(GET_LEVEL(ch)<LVL_ADMIN))
      {
      send_to_char(ch, "You're not the member of any clan!!");
      return;
      }
   if(!IS_NPC(ch)&&PLR_FLAGGED(ch, PLR_NOSHOUT)&&(GET_LEVEL(ch)<LVL_IMMORT))
      {
      send_to_char(ch,"You can only commune with the gods!\r\n");
      return;
      }

   if(GET_LEVEL(ch)<LVL_IMMORT)
      strip_color(argument);
   skip_spaces(&argument);

   if(!(*argument))
      {
      send_to_char(ch,"format: csay <message>\r\n");
      send_to_char(ch,"format: csay *<message>\r\n");
      send_to_char(ch,"format: csay \\<message>\r\n");
      send_to_char(ch,"format: csay +\r\n");
      send_to_char(ch,"format: csay -\r\n");
      send_to_char(ch,"format: csay @\r\n");
      if (GET_LEVEL(ch) >= LVL_ADMIN)
         {
         send_to_char(ch,"format: csay >[clan name]\r\n");
         send_to_char(ch,"format: csay <[clan name]\r\n");
         }
      return;
      }

   switch(*argument)
      {
   case '*':
      emote=TRUE;
      argument++;
      break;
   case '+':
      REMOVE_BIT(PRF2_FLAGS(ch),PRF2_NOCSAY);
      send_to_char(ch,"You now hear your clan's channel.\r\n");
      return;
      break;
   case '-':
      SET_BIT(PRF2_FLAGS(ch),PRF2_NOCSAY);
      send_to_char(ch,"You are now deaf to your clan's channel.\r\n");
      return;
      break;
   case '>':
      argument++;
      skip_spaces(&argument);
      if (GET_LEVEL(ch) < LVL_ADMIN)
         {
         send_to_char(ch,"usage: csay <message>\r\n");
         return;
         }
      for (select_cl=clan_list; select_cl!=NULL; select_cl=select_cl->next)
         if (!str_cmp(argument, select_cl->cl_name) || isname(argument, select_cl->cl_name))      
            {
            if (IS_SET(MUTE_CHANNELS(ch), 1 << select_cl->cl_number))
               {
               REMOVE_BIT(MUTE_CHANNELS(ch), 1 << select_cl->cl_number);
               send_to_char(ch, "You start listening to %s.\r\n", select_cl->cl_name);
               }
            else
               send_to_char(ch, "You are already listening to %s.\r\n", select_cl->cl_name);
            return;
            }
      send_to_char(ch, "Which clan channel do you wish to start listening to?\r\n");
      return;
      break;
   case '<':
      argument++;
      skip_spaces(&argument);
      if (GET_LEVEL(ch) < LVL_ADMIN)
         {
         send_to_char(ch,"usage: csay <message>\r\n");
         return;
         }   
      for (select_cl=clan_list; select_cl!=NULL; select_cl=select_cl->next)
         if (!str_cmp(argument, select_cl->cl_name) || isname(argument, select_cl->cl_name))
            {
            if (!IS_SET(MUTE_CHANNELS(ch), 1 << select_cl->cl_number))
               {
               SET_BIT(MUTE_CHANNELS(ch), 1 << select_cl->cl_number);
               send_to_char(ch, "You stop listening to %s.\r\n", select_cl->cl_name);
               }
            else
               send_to_char(ch, "You are already deaf to %s.\r\n", select_cl->cl_name);
            return;
            }
      send_to_char(ch, "Which clan channel do you wish to stop listening to?\r\n");
      return; 
      break;
   case '@':
      for(d=descriptor_list;d;d=d->next)
         {
         if(STATE(d)==CON_PLAYING &&
                 ((GET_CLAN(ch)&&(GET_CLAN(ch)==GET_CLAN(d->character)))||
                  ((GET_LEVEL(ch)>=LVL_ADMIN)&&
                   (GET_CLAN(d->character)!=0)))&&
                 !PRF2_FLAGGED(d->character,PRF2_NOCSAY))
            {
            if(!any)
               {
               send_to_char(ch, "Clan members online:\r\n");
               any=TRUE;
               }
            send_to_char(ch, "  %s %s(%s)",GET_NAME(d->character),
                         GET_TITLE(d->character),GET_CLAN_NAME(d->character));
            if(PLR_FLAGGED(d->character, PLR_WRITING))
               send_to_char(ch, "(Writing)");
            else if(PLR_FLAGGED(d->character, PLR_MAILING))
               send_to_char(ch, "(Mailing)");
            if(AFF_FLAGGED(d->character,AFF_INVISIBLE))
               send_to_char(ch, "(invis)");
            send_to_char(ch, "\r\n");
            }
         }
      if(any)
         send_to_char(ch, "\r\n");
      any=FALSE;
      for(d=descriptor_list;d;d=d->next)
         {
         if(STATE(d)==CON_PLAYING &&
                 ((GET_CLAN(ch)==GET_CLAN(d->character))||
                  ((GET_LEVEL(ch)>=LVL_ADMIN)&&
                   (GET_CLAN(d->character)!=0)))&&
                 PRF2_FLAGGED(d->character,PRF2_NOCSAY))
            {
            if(!any)
               {
               send_to_char(ch, "Clan members offline:\r\n");
               any=TRUE;
               }
            send_to_char(ch, "  %s %s(%s)",GET_NAME(d->character),
                         GET_TITLE(d->character),GET_CLAN_NAME(d->character));
            if(PLR_FLAGGED(d->character, PLR_WRITING))
               send_to_char(ch, "(Writing)");
            else if(PLR_FLAGGED(d->character, PLR_MAILING))
               send_to_char(ch, "(Mailing)");
            if(AFF_FLAGGED(d->character,AFF_INVISIBLE))
               send_to_char(ch, "(invis)");
            send_to_char(ch, "\r\n");
            }
         }
      any=FALSE;
      if (GET_LEVEL(ch) >= LVL_ADMIN)
         {
         send_to_char(ch, "Clan channels that you are snooping:\r\n");
         if (!PRF2_FLAGGED(ch,PRF2_NOCSAY))
            {
            for (select_cl=clan_list; select_cl!=NULL; select_cl=select_cl->next)
               {
               if (!IS_SET(MUTE_CHANNELS(ch), 1 << select_cl->cl_number))
                  {
                  if (any)
                     send_to_char(ch, ",");
                  send_to_char(ch, " %s", select_cl->cl_name);
                  any=TRUE;
                  }
               }
            }

         if (!any)
            send_to_char(ch, "  None.");
         }
         send_to_char(ch, "\r\n");
      return;
      break;
   case '\\':
      ++argument;
      break;
   default:
      break;
      }
   if(PRF2_FLAGGED(ch,PRF2_NOCSAY))
      {
      send_to_char(ch,"You are not on your clan's channel!\r\n");
      return;
      }
   if(GET_CLAN(ch)==0)
      {
      send_to_char(ch,"You can only snoop the channel");
      return;
      }
   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF))
      {
      send_to_char(ch,"The walls absorb your words.\r\n");
      return;
      }

   buf1=get_buffer(512);
   if (PRF_FLAGGED(ch,PRF_NOREPEAT))
      {
      send_to_char(ch, "%s", OK);
      }
   else
      {
      send_to_char(ch,"&Y[%s] You: %s%s&n\r\n",GET_CLAN_NAME(ch),
                   emote?"<--- ":"", argument);
      }

   sprintf (buf1,"&Y[%s] %s: %s%s&n\r\n",GET_CLAN_NAME(ch),GET_NAME(ch),
            emote?"<--- ":"",argument);
   for (i = descriptor_list; i; i = i->next)
      {
      if(i->original)
         vict = i->original;
      else
         vict = i->character;
      if (STATE(i)==CON_PLAYING &&
              !PLR_FLAGGED(vict, PLR_WRITING) &&
              ((GET_CLAN(vict) == GET_CLAN(ch))||
               (GET_LEVEL(vict)>=LVL_ADMIN))&&
              (vict != ch)&&
              !PRF2_FLAGGED(vict,PRF2_NOCSAY)&&
              !IS_SET(MUTE_CHANNELS(vict), 1<<GET_CLAN(ch)))
         {
         if(IN_ROOM(vict)&&
                 !ROOM_FLAGGED(IN_ROOM(vict),ROOM_SOUNDPROOF))
            send_to_char(vict,"%s", buf1);
         }
      }
   release_buffer(buf1);
   }



ACMD (do_clanpiece)
   {
   struct clan_data *clan;
   long clan_piece;
   int found = FALSE;
   char *name = get_buffer(MAX_STRING_LENGTH);
   char *piece = get_buffer(MAX_STRING_LENGTH);

   half_chop(argument, piece, name);

   if (!*name || !*piece)
      {
      send_to_char(ch, "Usage: clanpiece <vnum of piece> <clan name>\r\n");
      release_buffer(piece);
      release_buffer(name);
      return;
      }

   send_to_char(ch, "Okay\r\n");

   clan_piece = atol(piece);
   release_buffer(piece);

   if (real_object(clan_piece) < 0)
      {
      send_to_char(ch,"There is no object with that number.\r\n");
      release_buffer(name);
      return;
      }

   clan = clan_list;

   while (clan != NULL)
      {
      if (!str_cmp(name, clan->cl_name) || isname(name, clan->cl_name))
         {
         clan->cl_piece = clan_piece;
         write_clan_file();
         found = TRUE;
         break;
         }
      clan = clan->next;
      }

   if (found != TRUE)
      send_to_char(ch, "Clan not found.\r\n");

   release_buffer(name);
   }


ACMD (do_change_leader)
{
  if ((GET_LEADER(ch) == 0) && (GET_LEVEL(ch) < LVL_ADMIN)) {
    send_to_char(ch, "You aren't a clan leader.\r\n");
    return;
  }

  char buf[1024];
  char buf2[1024];
  half_chop(argument, buf, buf2);

  struct char_data* victim = get_char_vis(ch, buf, FIND_CHAR_ROOM);
  if (!victim) {
    send_to_char(ch, "The new leader must be in the same room.\r\n");
    return;
  }
  if (victim == ch) {
    send_to_char(ch, "Um... no.\r\n");
    return;
  }

  if (IS_NPC(victim)) {
    send_to_char(ch, "Mobs can't lead clans.\r\n");
    return;
  }

  if (GET_CLAN(ch) != GET_CLAN(victim)) {
    send_to_char(ch, "They're not in the same clan.\r\n");
    return;
  }

  struct clan_data* clan = clan_list;
  while (clan && clan->cl_number != GET_CLAN(ch)) {
    clan = clan->next;
  }
  if (!clan) {
    send_to_char(ch, "Couldn't find your clan.  Ask an implementor.\r\n");
    return;
  }

  int i = 0;
  while (i < 150 && clan->cl_players[i] && str_cmp(clan->cl_players[i], GET_NAME(victim))) {
    i++;
  }
  if (i >= 150 || str_cmp(clan->cl_players[i], GET_NAME(victim))) {
    send_to_char(ch, "Couldn't find the player.  Ask an implementor.\r\n");
    return;
  }

  // This is a memory leak, but I don't care.  You don't change leaders that often.
  clan->cl_players[0] = strdup(GET_NAME(victim));
  clan->cl_players[i] = strdup(GET_NAME(ch));

  GET_LEADER(ch) = 0;
  GET_LEADER(victim) = 1;

  // Swap heartworns.
  struct obj_data* leader_obj = GET_EQ(ch, WEAR_HEART);
  if (leader_obj) {
    unequip_char(ch, WEAR_HEART);
  }
  struct obj_data* member_obj = GET_EQ(victim, WEAR_HEART);
  if (member_obj) {
    unequip_char(victim, WEAR_HEART);
  }
  
  if (leader_obj) {
    equip_char(victim, leader_obj, WEAR_HEART);
  }
  if (member_obj) {
    equip_char(ch, member_obj, WEAR_HEART);
  }

  // Send happy messages.
  send_to_char(victim, "You now lead %s.\r\n", GET_CLAN_NAME(ch));
  send_to_char(ch, "%s is the new leader of %s.\r\n", GET_NAME(victim), GET_CLAN_NAME(victim));
  mudlogf(BRF, LVL_IMMORT, TRUE, "CLAN: %s changed leadership of %s to %s.", GET_NAME(ch), GET_CLAN_NAME(victim), GET_NAME(victim));

  write_clan_file();
}
